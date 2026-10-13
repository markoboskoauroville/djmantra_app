#pragma once

#include <array>
#include <vector>

/// DJ Mantra: djay Pro's favourite FX that Mixxx has no match for (Marko 13.10.2026:
/// "Out of all DJPro effects, this is one I choose to replicate": Flanger, Underwater,
/// Cooldown, Echo Out; the Flanger is Mixxx's own). Plain signal processing, no
/// allocation in process(); interleaved stereo float buffers. Each FX has one
/// "amount" (djay's slider, 0..1) and a dry / wet mix (djay's D/W knob, 0..1).
/// Tuned by ear first, then against the tester's djay recordings (testing/djay/fx/).
namespace djmantra {

/// A topology-preserving state variable filter (Zavalishin), one per channel.
class Svf {
  public:
    void set(double sampleRate, double cutoff, double q);
    void reset() {
        m_ic1 = {0, 0};
        m_ic2 = {0, 0};
    }
    // Returns the lowpass; *pHigh (if given) gets the highpass.
    float process(float x, int channel, float* pHigh = nullptr);

  private:
    double m_a1 = 0, m_a2 = 0, m_a3 = 0, m_k = 1;
    std::array<double, 2> m_ic1{0, 0};
    std::array<double, 2> m_ic2{0, 0};
};

/// Underwater (djay's, measured from its recordings, testing/djay/fx/notes.md): a
/// slightly resonant 12 dB lowpass, the level kept. amount 0 = open (16 kHz), 0.5
/// (djay's middle) = ~4 kHz, 1 = 400 Hz.
class Underwater {
  public:
    explicit Underwater(double sampleRate = 44100);
    void setSampleRate(double sampleRate);
    void reset();
    void process(const float* pIn, float* pOut, int frames, double amount, double mix);
    static double cutoffFor(double amount);

  private:
    double m_sampleRate;
    double m_cutoff = 16000; // smoothed
    Svf m_a;
};

/// Cooldown (djay's, measured): the deck steps back: a band-pass (the lows below
/// ~300 Hz and the highs above ~4 kHz cut; ~6 dB quieter overall), at once (~0.1 s), back
/// at once when off. amount 0 = no change, 0.5 = as above, above 0.6 it also fades the
/// deck out (1 = ~15 dB a second, near silence in ~3 s).
class Cooldown {
  public:
    explicit Cooldown(double sampleRate = 44100);
    void setSampleRate(double sampleRate);
    void reset();
    void process(const float* pIn, float* pOut, int frames, double amount, double mix);

  private:
    double m_sampleRate;
    double m_lowpass = 20000; // smoothed
    double m_highpass = 20;   // smoothed
    double m_level = 1;       // smoothed gain
    double m_fadeDb = 0;      // the fade-out so far (amount > 0.6)
    Svf m_lp, m_lp2, m_hp;
};

/// Echo Out (djay's, measured): the deck leaves on an echo of its last beats. While
/// not engaged the input passes and fills a delay line; when engaged the input stops
/// at once and the last `beats` beats repeat, quieter each round (2 beats -6 dB, 1 beat
/// -4.6 dB), until
/// silence; disengaged = the song back at once. The echo time is fixed when engaged.
class EchoOut {
  public:
    static constexpr double kMaxDelaySeconds = 4.0; // 2 beats down to 30 BPM

    explicit EchoOut(double sampleRate = 44100);
    void setSampleRate(double sampleRate);
    void reset();
    void process(const float* pIn,
            float* pOut,
            int frames,
            bool engaged,
            double beats,
            double beatSeconds,
            double mix);

  private:
    double m_sampleRate;
    std::vector<float> m_buffer; // interleaved stereo
    int m_write = 0;
    int m_delayFrames = 0; // fixed when engaged
    float m_wet = 0;       // 0 = dry, 1 = the echo (5 ms ramps)
};

/// Pioneer DJM Sound Color FX FILTER (Marko 13.10.2026: "it didn't sound like Pioneer"):
/// one knob, the middle does nothing (a small dead zone), left = a resonant low-pass
/// sweeping down from 20 kHz to ~45 Hz, right = a resonant high-pass sweeping up from
/// 20 Hz to ~9 kHz, both on an exponential (musical) travel; the DJM's peaky resonance
/// (Q ~2 by default, the PARAMETER knob's "resonance" 0..1 = Q 0.7..4) is what makes its
/// sweep sing. The filter comes in from dry at the edge of the dead zone (no click).
class DjmFilter {
  public:
    static constexpr double kDeadZone = 0.03; // of the knob's travel, each side of the middle

    explicit DjmFilter(double sampleRate = 44100);
    void setSampleRate(double sampleRate);
    void reset();
    void process(const float* pIn, float* pOut, int frames, double knob, double resonance);
    /// the corner for a knob position: < 0 = low-pass, > 0 = high-pass, 0 = off
    static double cutoffFor(double knob);

  private:
    double m_sampleRate;
    double m_knob = 0.5; // smoothed
    Svf m_svf;
};

} // namespace djmantra
