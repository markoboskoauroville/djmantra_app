#pragma once

#include <array>
#include <memory>
#include <vector>

class FFTReal;

/// DJ Mantra: the deck EQ with djay Pro's "Neural Mix" mode (Marko 13.10.2026:
/// "a button to change the mode of EQ from normal EQ to highs are harmonies, mids
/// are vocals, and lows are percussion / drums / rhythm"). Light signal processing
/// that runs on any phone, no neural network: a short-time FFT (1024 points, hop
/// 256, Hann windows; ~23 ms latency in both modes, so two decks stay aligned when
/// one switches) with one real mask per bin applied to both channels:
/// - normal: 3 bands split at the mixer's EQ frequencies (smooth over an octave);
/// - stems: drums = what is short in time (harmonic / percussive separation: the
///   median over 17 bins vs over the last 9 frames), vocals = what is steady AND in
///   the middle of the stereo image (150 Hz - 7 kHz), harmonies = the rest.
/// The masks add up to 1: at unity gains the sound passes unchanged (delayed).
/// Switching modes cross-fades over ~0.2 s.
namespace djmantra {

class StemEq {
  public:
    static constexpr int kSize = 1024;
    static constexpr int kHop = 256;
    static constexpr int kBins = kSize / 2 + 1;
    static constexpr int kHistory = 9;    // frames, the harmonic median
    static constexpr int kSpread = 8;     // bins on each side, the percussive median

    struct Gains {
        double low = 1;  // normal: the low band; stems: drums
        double mid = 1;  // normal: the mid band; stems: vocals
        double high = 1; // the high band
        // the Neural Mix levels (V63, Marko 14.10.2026: SHIFT turns the other mode's knobs):
        // their own knobs, applied together with the bands; "stems" only says which of
        // the two sets the screen's and the controller's knobs turn
        double drums = 1;
        double vocals = 1;
        double harmonies = 1;
        bool stems = false;
    };

    explicit StemEq(double sampleRate = 44100);
    ~StemEq();

    void setSampleRate(double sampleRate);
    void setCrossovers(double lowHz, double highHz);
    /// Interleaved stereo in / out (may be the same buffer), `frames` frames
    void process(const float* pIn, float* pOut, int frames, const Gains& gains);
    void reset();
    static constexpr int latencyFrames() {
        return kSize;
    }

  private:
    void processFrame(const Gains& gains);
    void computeBandMasks();

    double m_sampleRate;
    double m_lowHz = 246;
    double m_highHz = 2484;
    std::unique_ptr<FFTReal> m_pFft;
    std::vector<double> m_window;
    std::array<std::vector<double>, 2> m_in;    // the last kSize input samples
    std::array<std::vector<double>, 2> m_acc;   // the overlap-add sum
    std::array<std::vector<double>, 2> m_ready; // kHop samples to output
    std::vector<double> m_frame, m_re[2], m_im[2], m_time;
    std::vector<double> m_mag;
    std::vector<std::vector<double>> m_history; // kHistory x kBins magnitudes
    int m_historyPos = 0;
    std::vector<double> m_bandLow, m_bandHigh, m_vocalBand;
    std::vector<double> m_mask, m_scratch, m_sorted;
    int m_fill = 0;
    // smoothed per frame
    double m_blend = 0; // 0 no stem cut .. 1 the stem levels applied (gates the analysis)
    double m_gLow = 1, m_gMid = 1, m_gHigh = 1;
    double m_gDrums = 1, m_gVocals = 1, m_gHarmonies = 1;
};

} // namespace djmantra
