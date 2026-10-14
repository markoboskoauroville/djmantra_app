package com.djmantra.app;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.util.Log;

import java.util.Random;

/**
 * ROOM EQ (Marko 14.10.2026: "a special EQ for calibrating the room ... a noise generator ... a
 * frequency sweep ... the EQ will automatically adjust for the signal coming from the microphone
 * ... 3 measures in the different parts of the room, and then it will average").
 *
 * The microphone's spectrum the way Marko's Spectral Analyzer draws it (FFT 8192, 4-term
 * Blackman-Harris, power per bin, log frequency): live, for the analyzer under the EQ curve, and
 * as a measurement: pink noise (or a log sweep, 20 Hz - 20 kHz) is played while the microphone
 * listens, and the level of each octave band (31.5 Hz .. 16 kHz) is the room's answer. Pink noise
 * and a log sweep both carry the same energy in every octave, so a flat room reads flat.
 *
 * All calls come from QmlDJMantra; measure() blocks (off the UI thread).
 */
public final class RoomEqBridge {
    private static final String TAG = "DJMantraRoomEq";
    private static final int RATE = 48000;
    private static final int N = 8192;
    public static final double[] BANDS = {31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
    // the analyzer's display: 1/6 octave from 20 Hz to 20 kHz
    private static final int COLUMNS = 61;

    private static volatile boolean sLive;
    private static volatile String sLiveText = "";
    private static Thread sLiveThread;

    private RoomEqBridge() {
    }

    // --- the live analyzer --------------------------------------------------------

    /** Starts (true) or stops the live microphone analyzer. */
    public static synchronized void live(boolean on) {
        if (on == sLive) {
            return;
        }
        sLive = on;
        if (!on) {
            return;
        }
        sLiveThread = new Thread(() -> {
            AudioRecord rec = openMic();
            if (rec == null) {
                sLive = false;
                return;
            }
            float[] ring = new float[N];
            short[] chunk = new short[2048];
            double[] avg = new double[N / 2];
            int filled = 0;
            rec.startRecording();
            try {
                while (sLive) {
                    int n = rec.read(chunk, 0, chunk.length);
                    if (n <= 0) {
                        continue;
                    }
                    System.arraycopy(ring, n, ring, 0, N - n);
                    for (int i = 0; i < n; ++i) {
                        ring[N - n + i] = chunk[i] / 32768f;
                    }
                    filled = Math.min(N, filled + n);
                    if (filled < N) {
                        continue;
                    }
                    double[] power = power(ring, 0);
                    for (int k = 0; k < avg.length; ++k) {
                        // FabFilter-like fall: fast up, slower down
                        avg[k] = power[k] > avg[k] ? power[k] : avg[k] * 0.7 + power[k] * 0.3;
                    }
                    sLiveText = columnsText(avg);
                }
            } finally {
                rec.stop();
                rec.release();
            }
        }, "RoomEqLive");
        sLiveThread.start();
    }

    /** The live spectrum: COLUMNS dB values (1/6 octave, 20 Hz - 20 kHz), comma separated. */
    public static String liveColumns() {
        return sLiveText;
    }

    // --- a measurement -------------------------------------------------------------

    /**
     * Plays `signal` ("noise" = pink noise, "sweep" = a log sine sweep) for `seconds` while the
     * microphone listens. Returns "band0,...,band9|col0,...,col60": the room's level per octave
     * band and per display column in dB, relative to the 250 Hz - 4 kHz average; "error: ...".
     */
    public static String measure(String signal, double seconds) {
        boolean wasLive = sLive;
        live(false);
        AudioRecord rec = openMic();
        if (rec == null) {
            return "error: no microphone (allow DJ Mantra to use it)";
        }
        int frames = (int) (seconds * RATE);
        short[] out = signal("sweep".equals(signal), frames);
        AudioTrack track = new AudioTrack.Builder()
                .setAudioAttributes(new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_MEDIA)
                        .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
                .setAudioFormat(new AudioFormat.Builder().setSampleRate(RATE)
                        .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                        .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build())
                .setTransferMode(AudioTrack.MODE_STATIC)
                .setBufferSizeInBytes(out.length * 2)
                .build();
        float[] heard = new float[frames];
        try {
            track.write(out, 0, out.length);
            rec.startRecording();
            track.play();
            short[] chunk = new short[4096];
            int got = 0;
            long end = System.currentTimeMillis() + (long) (seconds * 1000) + 300;
            while (got < frames && System.currentTimeMillis() < end + 2000) {
                int n = rec.read(chunk, 0, Math.min(chunk.length, frames - got));
                if (n <= 0) {
                    continue;
                }
                for (int i = 0; i < n; ++i) {
                    heard[got + i] = chunk[i] / 32768f;
                }
                got += n;
            }
        } catch (Exception e) {
            Log.w(TAG, "measure: " + e);
            return "error: " + e.getMessage();
        } finally {
            try {
                track.stop();
            } catch (Exception ignored) {
            }
            track.release();
            rec.stop();
            rec.release();
            if (wasLive) {
                live(true);
            }
        }
        double[] sum = welch(heard, frames);
        if (sum == null) {
            return "error: the measurement was too short";
        }
        // the test signal itself, analysed the same way: subtracted, so only the room (and the
        // speakers and the microphone) remain
        float[] sent = new float[frames];
        for (int i = 0; i < frames; ++i) {
            sent[i] = out[2 * i] / 32768f;
        }
        double[] own = welch(sent, frames);
        double[] bands = new double[BANDS.length];
        for (int b = 0; b < BANDS.length; ++b) {
            bands[b] = energyDb(sum, BANDS[b] / Math.sqrt(2), BANDS[b] * Math.sqrt(2))
                    - energyDb(own, BANDS[b] / Math.sqrt(2), BANDS[b] * Math.sqrt(2));
        }
        double ref = 0;
        for (int b = 3; b <= 7; ++b) { // 250 Hz .. 4 kHz
            ref += bands[b] / 5;
        }
        StringBuilder text = new StringBuilder();
        for (int b = 0; b < bands.length; ++b) {
            text.append(b > 0 ? "," : "").append(String.format(java.util.Locale.ROOT, "%.1f", bands[b] - ref));
        }
        // the columns relative to the same reference (pink: the columns' energy falls with
        // their width, so each column is shown per octave)
        String[] cols = columnsText(sum).split(",");
        String[] ownCols = columnsText(own).split(",");
        StringBuilder colText = new StringBuilder();
        for (int c = 0; c < cols.length; ++c) {
            double v = Double.parseDouble(cols[c]) - Double.parseDouble(ownCols[c]) - ref;
            colText.append(c > 0 ? "," : "").append(String.format(java.util.Locale.ROOT, "%.1f", v));
        }
        Log.i(TAG, signal + " " + seconds + " s: " + text);
        return text + "|" + colText;
    }

    // --- DSP ----------------------------------------------------------------------

    // the average power per bin of x[0..frames), from 0.5 s on (the room filling up), N / 2 hops
    static double[] welch(float[] x, int frames) {
        double[] sum = new double[N / 2];
        int windows = 0;
        for (int start = RATE / 2; start + N <= frames; start += N / 2) {
            double[] p = power(x, start);
            for (int k = 0; k < sum.length; ++k) {
                sum[k] += p[k];
            }
            ++windows;
        }
        if (windows == 0) {
            return null;
        }
        for (int k = 0; k < sum.length; ++k) {
            sum[k] /= windows;
        }
        return sum;
    }

    private static AudioRecord openMic() {
        try {
            int min = AudioRecord.getMinBufferSize(RATE, AudioFormat.CHANNEL_IN_MONO,
                    AudioFormat.ENCODING_PCM_16BIT);
            // UNPROCESSED: no automatic gain, no noise suppression (a measurement microphone)
            int source = android.os.Build.VERSION.SDK_INT >= 24
                    ? MediaRecorder.AudioSource.UNPROCESSED : MediaRecorder.AudioSource.MIC;
            AudioRecord rec = new AudioRecord(source, RATE, AudioFormat.CHANNEL_IN_MONO,
                    AudioFormat.ENCODING_PCM_16BIT, Math.max(min, 8 * 4096));
            if (rec.getState() != AudioRecord.STATE_INITIALIZED) {
                rec.release();
                rec = new AudioRecord(MediaRecorder.AudioSource.MIC, RATE, AudioFormat.CHANNEL_IN_MONO,
                        AudioFormat.ENCODING_PCM_16BIT, Math.max(min, 8 * 4096));
            }
            if (rec.getState() != AudioRecord.STATE_INITIALIZED) {
                rec.release();
                return null;
            }
            return rec;
        } catch (SecurityException e) {
            Log.w(TAG, "microphone: " + e);
            return null;
        }
    }

    /** Pink noise (Paul Kellet's filter) or a log sweep, stereo 16 bit at -12 dBFS. */
    static short[] signal(boolean sweep, int frames) {
        short[] out = new short[2 * frames];
        double amp = Math.pow(10, -12 / 20.0);
        Random random = new Random(1);
        double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
        double f0 = 20, f1 = 20000, t = (double) frames / RATE, k = Math.log(f1 / f0);
        int fade = RATE / 20;
        for (int i = 0; i < frames; ++i) {
            double v;
            if (sweep) {
                double time = (double) i / RATE;
                v = Math.sin(2 * Math.PI * f0 * t / k * (Math.exp(time / t * k) - 1));
            } else {
                double white = random.nextGaussian() * 0.25;
                b0 = 0.99886 * b0 + white * 0.0555179;
                b1 = 0.99332 * b1 + white * 0.0750759;
                b2 = 0.96900 * b2 + white * 0.1538520;
                b3 = 0.86650 * b3 + white * 0.3104856;
                b4 = 0.55000 * b4 + white * 0.5329522;
                b5 = -0.7616 * b5 - white * 0.0168980;
                v = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362) * 0.11;
                b6 = white * 0.115926;
            }
            double edge = Math.min(1, Math.min(i, frames - 1 - i) / (double) fade);
            short s = (short) Math.max(-32767, Math.min(32767, v * amp * edge * 32767));
            out[2 * i] = s;
            out[2 * i + 1] = s;
        }
        return out;
    }

    // power per bin (N/2) of N samples from x[start], 4-term Blackman-Harris
    static double[] power(float[] x, int start) {
        double[] re = new double[N];
        double[] im = new double[N];
        for (int i = 0; i < N; ++i) {
            double w = 0.35875 - 0.48829 * Math.cos(2 * Math.PI * i / N)
                    + 0.14128 * Math.cos(4 * Math.PI * i / N) - 0.01168 * Math.cos(6 * Math.PI * i / N);
            re[i] = x[start + i] * w;
        }
        fft(re, im);
        double[] p = new double[N / 2];
        for (int k = 0; k < N / 2; ++k) {
            p[k] = re[k] * re[k] + im[k] * im[k];
        }
        return p;
    }

    // the energy of [lo, hi] Hz in dB
    static double energyDb(double[] power, double lo, double hi) {
        double e = 0;
        int a = Math.max(1, (int) Math.floor(lo * N / RATE));
        int b = Math.min(power.length - 1, (int) Math.ceil(hi * N / RATE));
        for (int k = a; k <= b; ++k) {
            e += power[k];
        }
        return 10 * Math.log10(Math.max(e, 1e-20));
    }

    // 1/6-octave columns, each shown as an octave's energy (a pink signal reads flat)
    static String columnsText(double[] power) {
        StringBuilder text = new StringBuilder();
        for (int c = 0; c < COLUMNS; ++c) {
            double f = 20 * Math.pow(2, c / 6.0);
            double db = energyDb(power, f / Math.pow(2, 1 / 12.0), f * Math.pow(2, 1 / 12.0)) + 10 * Math.log10(6);
            text.append(c > 0 ? "," : "").append(String.format(java.util.Locale.ROOT, "%.1f", db));
        }
        return text.toString();
    }

    // in-place radix-2 FFT
    static void fft(double[] re, double[] im) {
        int n = re.length;
        for (int i = 1, j = 0; i < n; ++i) {
            int bit = n >> 1;
            for (; (j & bit) != 0; bit >>= 1) {
                j ^= bit;
            }
            j ^= bit;
            if (i < j) {
                double t = re[i];
                re[i] = re[j];
                re[j] = t;
                t = im[i];
                im[i] = im[j];
                im[j] = t;
            }
        }
        for (int len = 2; len <= n; len <<= 1) {
            double ang = -2 * Math.PI / len;
            double wr = Math.cos(ang), wi = Math.sin(ang);
            for (int i = 0; i < n; i += len) {
                double cr = 1, ci = 0;
                for (int j = 0; j < len / 2; ++j) {
                    int a = i + j, b = i + j + len / 2;
                    double xr = re[b] * cr - im[b] * ci;
                    double xi = re[b] * ci + im[b] * cr;
                    re[b] = re[a] - xr;
                    im[b] = im[a] - xi;
                    re[a] += xr;
                    im[a] += xi;
                    double t = cr * wr - ci * wi;
                    ci = cr * wi + ci * wr;
                    cr = t;
                }
            }
        }
    }
}
