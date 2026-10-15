// DJ Mantra: the frequency response of the channel EQ and the filter knob,
// measured through the real effect processors with sine tones, knob position
// by knob position, as a DJ turns them. Compared with Pioneer's DJM mixers in
// docs/EQ_FILTER_VS_DJM.md.
//
// Set DJMANTRA_CURVES_OUT to a folder to write the tables (eq_curves.csv,
// filter_curves.csv); the assertions check what must hold in any case.

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <memory>
#include <vector>

#include "control/controlbehavior.h"
#include "control/controlobject.h"
#include "effects/backends/builtin/biquadfullkilleqeffect.h"
#include "effects/backends/builtin/filtereffect.h"
#include "effects/backends/effectmanifest.h"
#include "effects/defs.h"
#include "engine/effects/engineeffectparameter.h"
#include "engine/effects/groupfeaturestate.h"
#include "test/mixxxtest.h"
#include "util/math.h"

namespace {

constexpr int kRate = 48000;
constexpr int kFrames = 1024;

// 1/3 octave centres from 20 Hz to 20 kHz
std::vector<double> testFrequencies() {
    std::vector<double> f;
    for (int i = -17; i <= 13; ++i) {
        f.push_back(1000.0 * std::pow(2.0, i / 3.0));
    }
    return f;
}

QMap<QString, EngineEffectParameterPointer> parametersOf(const EffectManifestPointer& pManifest) {
    QMap<QString, EngineEffectParameterPointer> parameters;
    for (const auto& pParameter : pManifest->parameters()) {
        parameters.insert(pParameter->id(),
                EngineEffectParameterPointer(new EngineEffectParameter(pParameter)));
    }
    return parameters;
}

/// Gain in dB of a sine of freq through processor (fresh state per tone)
template<class Effect, class State>
double gainDb(Effect* pEffect, double freq) {
    const mixxx::EngineParameters engineParameters(
            mixxx::audio::SampleRate(kRate), kFrames);
    State state(engineParameters);
    const int samples = engineParameters.samplesPerBuffer();
    std::vector<CSAMPLE> in(samples);
    std::vector<CSAMPLE> out(samples);
    // long enough for 20 Hz and the filters' settling: 1.5 s, measure the last 0.5 s
    const int buffers = (kRate * 3 / 2) / kFrames;
    const int measureFrom = buffers - (kRate / 2) / kFrames;
    double sumIn = 0;
    double sumOut = 0;
    long frame = 0;
    GroupFeatureState features;
    for (int b = 0; b < buffers; ++b) {
        for (int i = 0; i < kFrames; ++i, ++frame) {
            const auto v = static_cast<CSAMPLE>(
                    0.25 * std::sin(2 * M_PI * freq * frame / kRate));
            in[2 * i] = v;
            in[2 * i + 1] = v;
        }
        pEffect->processChannel(&state,
                in.data(),
                out.data(),
                engineParameters,
                EffectEnableState::Enabled,
                features);
        if (b >= measureFrom) {
            for (int i = 0; i < samples; ++i) {
                sumIn += in[i] * in[i];
                sumOut += out[i] * out[i];
            }
        }
    }
    if (sumOut <= 0) {
        return -200;
    }
    return 10 * std::log10(sumOut / sumIn);
}

double atFreq(const std::vector<double>& freqs, const std::vector<double>& db, double f) {
    int best = 0;
    for (int i = 0; i < static_cast<int>(freqs.size()); ++i) {
        if (std::abs(std::log(freqs[i] / f)) < std::abs(std::log(freqs[best] / f))) {
            best = i;
        }
    }
    return db[best];
}

class DJMantraEqCurveTest : public MixxxTest {
  protected:
    void SetUp() override {
        // Mixxx's default crossover frequencies (preferences: Mixer)
        m_pLo = std::make_unique<ControlObject>(ConfigKey(kMixerProfile, kLowEqFrequency));
        m_pHi = std::make_unique<ControlObject>(ConfigKey(kMixerProfile, kHighEqFrequency));
        m_pLo->set(250.0);
        m_pHi->set(2500.0);
    }

    void writeCsv(const QString& name, const QString& header, const QStringList& rows) {
        const QString dir = qEnvironmentVariable("DJMANTRA_CURVES_OUT");
        if (dir.isEmpty()) {
            return;
        }
        QDir().mkpath(dir);
        QFile file(QDir(dir).filePath(name));
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream stream(&file);
        stream << header << "\n" << rows.join("\n") << "\n";
    }

    std::unique_ptr<ControlObject> m_pLo;
    std::unique_ptr<ControlObject> m_pHi;
};

TEST_F(DJMantraEqCurveTest, ChannelEq) {
    const auto freqs = testFrequencies();
    EffectManifestPointer pManifest = BiquadFullKillEQEffect::getManifest();
    auto parameters = parametersOf(pManifest);
    // the knob (0..1, centre 0.5) to the gain value, as the effect knob does
    ControlAudioTaperPotBehavior taper(-12, ratio2db(4.0), 0.5);
    const QList<double> positions = {0.0, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.75, 0.9, 1.0};

    QStringList header{"band", "knob", "gain_value"};
    for (double f : freqs) {
        header << QString::number(f, 'f', 0);
    }
    QStringList rows;
    for (const QString& band : {QStringLiteral("low"), QStringLiteral("mid"), QStringLiteral("high")}) {
        for (double knob : positions) {
            for (const QString& b : {QStringLiteral("low"), QStringLiteral("mid"), QStringLiteral("high")}) {
                // clamped as the effect parameter does (db2ratio(12.04) overshoots 4.0)
                parameters.value(b)->setValue(
                        b == band ? math_min(taper.parameterToValue(knob), 4.0) : 1.0);
            }
            for (const QString& kill : {QStringLiteral("killLow"),
                         QStringLiteral("killMid"),
                         QStringLiteral("killHigh")}) {
                parameters.value(kill)->setValue(0);
            }
            BiquadFullKillEQEffect effect;
            effect.loadEngineEffectParameters(parameters);
            std::vector<double> db;
            QStringList row{band, QString::number(knob), QString::number(taper.parameterToValue(knob))};
            for (double f : freqs) {
                db.push_back(gainDb<BiquadFullKillEQEffect, BiquadFullKillEQEffectGroupState>(
                        &effect, f));
                row << QString::number(db.back(), 'f', 1);
            }
            rows << row.join(",");

            if (knob == 0.5) {
                // centre: flat
                for (double v : db) {
                    EXPECT_NEAR(v, 0.0, 0.5) << band.toStdString();
                }
            }
            if (knob == 0.0) {
                // fully down: the band is gone (an isolator's kill)
                const double f = band == "low" ? 50 : band == "mid" ? 800 : 10000;
                EXPECT_LT(atFreq(freqs, db, f), -20.0) << band.toStdString();
            }
        }
    }
    writeCsv("eq_curves.csv", header.join(","), rows);
}

TEST_F(DJMantraEqCurveTest, FilterKnob) {
    const auto freqs = testFrequencies();
    EffectManifestPointer pManifest = FilterEffect::getManifest();
    auto parameters = parametersOf(pManifest);
    ControlLogPotmeterBehavior cutoff(13, 22050, -40);
    QStringList header{"knob", "resonance_q", "lpf_hz", "hpf_hz"};
    for (double f : freqs) {
        header << QString::number(f, 'f', 0);
    }
    QStringList rows;
    const QList<double> positions = {0.0, 0.1, 0.2, 0.3, 0.4, 0.45, 0.48, 0.5,
            0.52, 0.55, 0.6, 0.7, 0.8, 0.9, 1.0};
    for (double q : {0.707106781, 2.0, 4.0}) {
        for (double knob : positions) {
            // the quick effect's super knob: left half lowers the low-pass
            // (LinkedLeft), right half raises the high-pass (LinkedRight)
            const double lpfParameter = knob <= 0.5 ? knob * 2 : 1.0;
            const double hpfParameter = knob >= 0.5 ? (knob - 0.5) * 2 : 0.0;
            const double lpf = cutoff.parameterToValue(lpfParameter);
            const double hpf = cutoff.parameterToValue(hpfParameter);
            parameters.value("lpf")->setValue(lpf);
            parameters.value("hpf")->setValue(hpf);
            parameters.value("q")->setValue(q);
            FilterEffect effect;
            effect.loadEngineEffectParameters(parameters);
            QStringList row{QString::number(knob),
                    QString::number(q, 'f', 3),
                    QString::number(lpf, 'f', 0),
                    QString::number(hpf, 'f', 0)};
            std::vector<double> db;
            for (double f : freqs) {
                db.push_back(gainDb<FilterEffect, FilterGroupState>(&effect, f));
                row << QString::number(db.back(), 'f', 1);
            }
            rows << row.join(",");
            if (knob == 0.5) {
                for (double v : db) {
                    EXPECT_NEAR(v, 0.0, 0.5);
                }
            }
        }
    }
    writeCsv("filter_curves.csv", header.join(","), rows);
}

} // namespace
