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

/// A small stereo reverb: four delay lines with a Hadamard feedback matrix and
/// damping in the loop (a feedback delay network).
class SmallReverb {
  public:
    explicit SmallReverb(double sampleRate = 44100);
    void setSampleRate(double sampleRate);
    void clear();
    // Adds the reverb of (inL, inR) to *pL, *pR at the given level.
    void process(float inL, float inR, float decay, float level, float* pL, float* pR);

  private:
    static constexpr int kLines = 4;
    std::array<std::vector<float>, kLines> m_lines;
    std::array<int, kLines> m_pos{0, 0, 0, 0};
    std::array<float, kLines> m_damp{0, 0, 0, 0};
};

/// Underwater: the song heard from below the surface: a resonant 24 dB lowpass
/// (two cascaded filters) whose corner drifts slowly (0.35 Hz) like a wave.
/// amount 0 = 1.2 kHz, 1 = 180 Hz.
class Underwater {
  public:
    explicit Underwater(double sampleRate = 44100);
    void setSampleRate(double sampleRate);
    void reset();
    void process(const float* pIn, float* pOut, int frames, double amount, double mix);

  private:
    double m_sampleRate;
    double m_phase = 0;
    double m_cutoff = 1200; // smoothed
    Svf m_a, m_b;
};

/// Cooldown: the energy drops: the highs are filtered away (a lowpass sliding down
/// from 16 kHz over ~0.6 s), the lows thin out (a highpass up to 120 Hz), a reverb
/// washes in and the level falls a little. amount 0 = mild (lowpass 3 kHz),
/// 1 = deep (lowpass 400 Hz, more reverb).
class Cooldown {
  public:
    explicit Cooldown(double sampleRate = 44100);
    void setSampleRate(double sampleRate);
    void reset();
    void process(const float* pIn, float* pOut, int frames, double amount, double mix);

  private:
    double m_sampleRate;
    double m_lowpass = 16000; // smoothed
    double m_highpass = 20;   // smoothed
    double m_wet = 0;         // smoothed 0..1, the glide in
    Svf m_lp, m_hp;
    SmallReverb m_reverb;
};

/// Echo Out: press it and the deck leaves on its own echo: the next `beats` beats
/// are sent into a beat-synced delay while the dry signal fades out over them;
/// then only the echoes ring on, each repeat darker and quieter (feedback 0.62,
/// a lowpass and a highpass in the loop). mix = the echo level.
class EchoOut {
  public:
    static constexpr double kMaxDelaySeconds = 4.0; // 4 beats down to 60 BPM

    explicit EchoOut(double sampleRate = 44100);
    void setSampleRate(double sampleRate);
    void reset(); // call when the FX is switched on: starts a new echo out
    void process(const float* pIn,
            float* pOut,
            int frames,
            double beats,
            double beatSeconds,
            double mix);

  private:
    double m_sampleRate;
    std::vector<float> m_buffer; // interleaved stereo
    int m_write = 0;
    long m_elapsed = 0;   // frames since reset()
    int m_delayFrames = 0; // fixed at the first buffer after reset()
    Svf m_loop;
    Svf m_loopHigh;
};

} // namespace djmantra
