package com.djmantra.app;

import android.content.Context;
import android.media.AudioDeviceInfo;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Locale;
import java.util.Random;

/**
 * ROOM CORRECTION (Marko 14.10.2026, as IK Multimedia ARC: "I press the button, the sweep is
 * happening ... and then the curve is generated"; industry-standard measurement and correction,
 * up to seven places in a room averaged).
 *
 * The test signal is a WAV file the app plays on its own mixer (QML: a sampler), so it reaches
 * every output the mixer feeds (the PA over USB, Bluetooth). This class writes that file, records
 * the chosen microphone (the phone's or one plugged in), and turns the recording into the room's
 * answer:
 *  - SWEEP (the measurement rooms are measured with: Farina's exponential sine sweep, 20 Hz -
 *    20 kHz): the recording is deconvolved into the room's impulse response, gated to 500 ms
 *    after its peak (the room's modes, not the background noise), its spectrum is the response;
 *  - PINK NOISE (as a real-time analyzer): the recording's average spectrum.
 * Either way the same analysis of the signal that was sent is subtracted, so only the speakers,
 * the room and the microphone remain. Output: 31 third-octave bands (ISO, 20 Hz - 20 kHz) and 61
 * sixth-octave columns for the graph, in dB, relative to the 200 Hz - 2 kHz average.
 *
 * The live analyzer (live / liveColumns) is the same spectrum of the microphone, as the
 * Spectral Analyzer draws it (FFT 8192, 4-term Blackman-Harris, log frequency).
 */
public final class RoomEqBridge {
    private static final String TAG = "DJMantraRoomEq";
    private static final int RATE = 48000;
    private static final int N = 8192;
    private static final double F0 = 20;
    private static final double F1 = 20000;
    private static final double LEAD = 0.5;  // s of silence before the signal
    private static final double TAIL = 1.5;  // s after it (the room's decay)
    public static final double[] THIRDS = {20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315,
            400, 500, 630, 800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000,
            12500, 16000, 20000};
    private static final int COLUMNS = 61; // 1/6 octave from 20 Hz

    private static volatile boolean sLive;
    private static volatile String sLiveText = "";

    private RoomEqBridge() {
    }

    // --- the microphones ---------------------------------------------------------

    /** The inputs: [{id, name}] as JSON; id 0 = the phone's own microphone. */
    public static String inputs(Context context) {
        JSONArray list = new JSONArray();
        try {
            list.put(new JSONObject().put("id", 0).put("name", "Phone microphone"));
            AudioManager am = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
            for (AudioDeviceInfo d : am.getDevices(AudioManager.GET_DEVICES_INPUTS)) {
                int t = d.getType();
                if (t == AudioDeviceInfo.TYPE_USB_DEVICE || t == AudioDeviceInfo.TYPE_USB_HEADSET
                        || t == AudioDeviceInfo.TYPE_WIRED_HEADSET || t == AudioDeviceInfo.TYPE_BLUETOOTH_SCO
                        || t == AudioDeviceInfo.TYPE_USB_ACCESSORY) {
                    String kind = t == AudioDeviceInfo.TYPE_BLUETOOTH_SCO ? "Bluetooth"
                            : t == AudioDeviceInfo.TYPE_WIRED_HEADSET ? "Headset" : "USB";
                    list.put(new JSONObject().put("id", d.getId())
                            .put("name", kind + ": " + d.getProductName()));
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "inputs: " + e);
        }
        return list.toString();
    }

    private static AudioRecord openMic(Context context, int inputId) {
        try {
            int min = AudioRecord.getMinBufferSize(RATE, AudioFormat.CHANNEL_IN_MONO,
                    AudioFormat.ENCODING_PCM_16BIT);
            int size = Math.max(min, 16 * 4096);
            // UNPROCESSED: no automatic gain or noise suppression (a measurement microphone)
            AudioRecord rec = new AudioRecord(MediaRecorder.AudioSource.UNPROCESSED, RATE,
                    AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, size);
            if (rec.getState() != AudioRecord.STATE_INITIALIZED) {
                rec.release();
                rec = new AudioRecord(MediaRecorder.AudioSource.MIC, RATE, AudioFormat.CHANNEL_IN_MONO,
                        AudioFormat.ENCODING_PCM_16BIT, size);
            }
            if (rec.getState() != AudioRecord.STATE_INITIALIZED) {
                rec.release();
                return null;
            }
            if (inputId != 0) {
                AudioManager am = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
                for (AudioDeviceInfo d : am.getDevices(AudioManager.GET_DEVICES_INPUTS)) {
                    if (d.getId() == inputId) {
                        rec.setPreferredDevice(d);
                    }
                }
            }
            return rec;
        } catch (SecurityException e) {
            Log.w(TAG, "microphone: " + e);
            return null;
        }
    }

    // --- the live analyzer --------------------------------------------------------

    public static synchronized void live(Context context, boolean on, int inputId) {
        if (on == sLive) {
            return;
        }
        sLive = on;
        if (!on) {
            return;
        }
        new Thread(() -> {
            AudioRecord rec = openMic(context, inputId);
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
                    double[] power = power(ring, 0, N);
                    for (int k = 0; k < avg.length; ++k) {
                        avg[k] = power[k] > avg[k] ? power[k] : avg[k] * 0.7 + power[k] * 0.3;
                    }
                    sLiveText = join(columns(avg, N));
                }
            } finally {
                rec.stop();
                rec.release();
            }
        }, "RoomEqLive").start();
    }

    public static String liveColumns() {
        return sLiveText;
    }

    // --- the test signal ---------------------------------------------------------

    /** Writes the test signal as a 48 kHz stereo WAV (the mixer plays it); returns its path. */
    public static String writeSignal(String dir, String kind, double seconds) {
        boolean sweep = !"noise".equals(kind);
        float[] x = signal(sweep, seconds);
        File folder = new File(dir);
        folder.mkdirs();
        File file = new File(folder, (sweep ? "sweep_" : "pink_") + (int) seconds + "s.wav");
        try (FileOutputStream out = new FileOutputStream(file)) {
            int frames = x.length;
            ByteBuffer b = ByteBuffer.allocate(44 + frames * 4).order(ByteOrder.LITTLE_ENDIAN);
            b.put("RIFF".getBytes()).putInt(36 + frames * 4).put("WAVE".getBytes());
            b.put("fmt ".getBytes()).putInt(16).putShort((short) 1).putShort((short) 2).putInt(RATE)
                    .putInt(RATE * 4).putShort((short) 4).putShort((short) 16);
            b.put("data".getBytes()).putInt(frames * 4);
            for (float v : x) {
                short s = (short) Math.max(-32767, Math.min(32767, Math.round(v * 32767)));
                b.putShort(s).putShort(s);
            }
            out.write(b.array());
        } catch (IOException e) {
            Log.w(TAG, "writeSignal: " + e);
            return "";
        }
        return file.getAbsolutePath();
    }

    /**
     * The test signal (mono): LEAD s of silence, `seconds` of sweep or pink noise at -1 dBFS
     * peak (its level is set on the mixer), TAIL s of silence.
     */
    static float[] signal(boolean sweep, double seconds) {
        int lead = (int) (LEAD * RATE);
        int len = (int) (seconds * RATE);
        float[] x = new float[lead + len + (int) (TAIL * RATE)];
        double t = seconds, k = Math.log(F1 / F0);
        Random random = new Random(1);
        double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
        int fade = RATE / 100; // 10 ms
        double peak = 0;
        double[] v = new double[len];
        for (int i = 0; i < len; ++i) {
            if (sweep) {
                double time = (double) i / RATE;
                v[i] = Math.sin(2 * Math.PI * F0 * t / k * (Math.exp(time / t * k) - 1));
            } else {
                double white = random.nextGaussian();
                b0 = 0.99886 * b0 + white * 0.0555179;
                b1 = 0.99332 * b1 + white * 0.0750759;
                b2 = 0.96900 * b2 + white * 0.1538520;
                b3 = 0.86650 * b3 + white * 0.3104856;
                b4 = 0.55000 * b4 + white * 0.5329522;
                b5 = -0.7616 * b5 - white * 0.0168980;
                v[i] = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362;
                b6 = white * 0.115926;
            }
            v[i] *= Math.min(1, Math.min(i, len - 1 - i) / (double) fade);
            peak = Math.max(peak, Math.abs(v[i]));
        }
        double scale = Math.pow(10, -1 / 20.0) / Math.max(peak, 1e-9);
        for (int i = 0; i < len; ++i) {
            x[lead + i] = (float) (v[i] * scale);
        }
        return x;
    }

    // --- a measurement -------------------------------------------------------------

    /**
     * Records the microphone for the whole test signal (started by the caller on the mixer right
     * after this begins) and analyses it. Returns "thirds(31)|columns(61)|level" (dB; level = the
     * recording's peak in dBFS, to see it was loud enough), or "error: ...". Blocking.
     */
    public static String measure(Context context, String kind, double seconds, int inputId) {
        boolean sweep = !"noise".equals(kind);
        AudioRecord rec = openMic(context, inputId);
        if (rec == null) {
            return "error: no microphone (allow DJ Mantra to use it)";
        }
        float[] sent = signal(sweep, seconds);
        // room for the mixer's start and the output latency
        int frames = sent.length + RATE;
        float[] heard = new float[frames];
        double peak = 0;
        try {
            rec.startRecording();
            short[] chunk = new short[4096];
            int got = 0;
            while (got < frames) {
                int n = rec.read(chunk, 0, Math.min(chunk.length, frames - got));
                if (n <= 0) {
                    break;
                }
                for (int i = 0; i < n; ++i) {
                    heard[got + i] = chunk[i] / 32768f;
                    peak = Math.max(peak, Math.abs(heard[got + i]));
                }
                got += n;
            }
        } finally {
            rec.stop();
            rec.release();
        }
        double level = 20 * Math.log10(Math.max(peak, 1e-6));
        // r41 V70: a recording at -55.8 dBFS (the microphone heard almost nothing) was kept and
        // turned into -12 dB everywhere above 1 kHz: too quiet = no measurement
        if (level < -40) {
            return String.format(Locale.ROOT, "error: the microphone heard too little (peak %.0f dBFS): raise the "
                    + "TEST LEVEL or the speakers, or check the MICROPHONE choice", level);
        }
        double[] heardPower, sentPower;
        int size;
        if (sweep) {
            size = 1 << 15; // the gated response: 0.68 s
            heardPower = gatedResponse(heard, sent, size);
            sentPower = gatedResponse(sent, sent, size);
        } else {
            size = N;
            heardPower = welch(heard, 0, frames);
            sentPower = welch(sent, (int) (LEAD * RATE), sent.length - (int) (TAIL * RATE));
        }
        if (heardPower == null || sentPower == null) {
            return "error: the measurement was too short";
        }
        double[] thirds = new double[THIRDS.length];
        for (int b = 0; b < THIRDS.length; ++b) {
            double lo = THIRDS[b] / Math.pow(2, 1 / 6.0), hi = THIRDS[b] * Math.pow(2, 1 / 6.0);
            thirds[b] = energyDb(heardPower, size, lo, hi) - energyDb(sentPower, size, lo, hi);
        }
        double[] heardCols = columns(heardPower, size), sentCols = columns(sentPower, size);
        double[] cols = new double[COLUMNS];
        for (int c = 0; c < COLUMNS; ++c) {
            cols[c] = heardCols[c] - sentCols[c];
        }
        double ref = 0;
        int refCount = 0;
        for (int b = 0; b < THIRDS.length; ++b) {
            if (THIRDS[b] >= 200 && THIRDS[b] <= 2000) {
                ref += thirds[b];
                ++refCount;
            }
        }
        ref /= refCount;
        for (int b = 0; b < thirds.length; ++b) {
            thirds[b] -= ref;
        }
        for (int c = 0; c < cols.length; ++c) {
            cols[c] -= ref;
        }
        String result = join(thirds) + "|" + join(cols) + "|" + String.format(Locale.ROOT, "%.1f", level);
        Log.i(TAG, (sweep ? "sweep " : "noise ") + seconds + " s, input " + inputId + ", peak "
                + String.format(Locale.ROOT, "%.1f", level) + " dBFS: " + join(thirds));
        return result;
    }

    // --- DSP ----------------------------------------------------------------------

    /**
     * Farina's method: the recording convolved with the sweep's inverse (time reversed, its
     * level falling 6 dB per octave) is the impulse response; gated from 2 ms before its peak to
     * `size` samples (a half Hann over the last 10 %); its power spectrum (size / 2 bins).
     */
    static double[] gatedResponse(float[] y, float[] sweepSignal, int size) {
        int lead = (int) (LEAD * RATE);
        int len = sweepSignal.length - lead - (int) (TAIL * RATE);
        double k = Math.log(F1 / F0);
        int m = 1;
        while (m < y.length + len) {
            m <<= 1;
        }
        double[] yr = new double[m], yi = new double[m], hr = new double[m], hi = new double[m];
        for (int i = 0; i < y.length; ++i) {
            yr[i] = y[i];
        }
        for (int i = 0; i < len; ++i) {
            // the inverse: reversed, amplitude falling with time (high frequencies first)
            double t = (double) i / len;
            hr[i] = sweepSignal[lead + len - 1 - i] * Math.exp(-t * k);
        }
        fft(yr, yi);
        fft(hr, hi);
        for (int i = 0; i < m; ++i) {
            double r = yr[i] * hr[i] - yi[i] * hi[i];
            double im = yr[i] * hi[i] + yi[i] * hr[i];
            yr[i] = r;
            yi[i] = -im; // conjugate: an inverse FFT through the forward one
        }
        fft(yr, yi);
        int at = 0;
        double best = 0;
        for (int i = 0; i < m; ++i) {
            double v = Math.abs(yr[i]);
            if (v > best) {
                best = v;
                at = i;
            }
        }
        int start = Math.max(0, at - RATE / 500);
        double[] re = new double[size], im = new double[size];
        int taper = size / 10;
        for (int i = 0; i < size && start + i < m; ++i) {
            double w = i < size - taper ? 1 : 0.5 + 0.5 * Math.cos(Math.PI * (i - (size - taper)) / taper);
            re[i] = yr[start + i] / m * w;
        }
        fft(re, im);
        double[] p = new double[size / 2];
        for (int i = 0; i < size / 2; ++i) {
            p[i] = re[i] * re[i] + im[i] * im[i];
        }
        return p;
    }

    // the average power per bin over [from, to), N / 2 hops
    static double[] welch(float[] x, int from, int to) {
        double[] sum = new double[N / 2];
        int windows = 0;
        for (int start = from; start + N <= to; start += N / 2) {
            double[] p = power(x, start, N);
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

    // the power per bin of n samples from x[start], 4-term Blackman-Harris
    static double[] power(float[] x, int start, int n) {
        double[] re = new double[n];
        double[] im = new double[n];
        for (int i = 0; i < n; ++i) {
            double w = 0.35875 - 0.48829 * Math.cos(2 * Math.PI * i / n)
                    + 0.14128 * Math.cos(4 * Math.PI * i / n) - 0.01168 * Math.cos(6 * Math.PI * i / n);
            re[i] = x[start + i] * w;
        }
        fft(re, im);
        double[] p = new double[n / 2];
        for (int k = 0; k < n / 2; ++k) {
            p[k] = re[k] * re[k] + im[k] * im[k];
        }
        return p;
    }

    // the energy of [lo, hi] Hz in dB, from a power spectrum of an n-point FFT
    static double energyDb(double[] power, int n, double lo, double hi) {
        double e = 0;
        int a = Math.max(1, (int) Math.floor(lo * n / RATE));
        int b = Math.min(power.length - 1, (int) Math.ceil(hi * n / RATE));
        for (int k = a; k <= b; ++k) {
            e += power[k];
        }
        return 10 * Math.log10(Math.max(e, 1e-30));
    }

    // 1/6-octave columns from 20 Hz, each as an octave's energy (a pink signal reads flat)
    static double[] columns(double[] power, int n) {
        double[] c = new double[COLUMNS];
        for (int i = 0; i < COLUMNS; ++i) {
            double f = F0 * Math.pow(2, i / 6.0);
            c[i] = energyDb(power, n, f / Math.pow(2, 1 / 12.0), f * Math.pow(2, 1 / 12.0)) + 10 * Math.log10(6);
        }
        return c;
    }

    static String join(double[] v) {
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < v.length; ++i) {
            s.append(i > 0 ? "," : "").append(String.format(Locale.ROOT, "%.1f", v[i]));
        }
        return s.toString();
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
