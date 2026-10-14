#include "effects/backends/builtin/roomeqeffect.h"

#include <algorithm>

#include "effects/backends/effectmanifest.h"
#include "engine/effects/engineeffectparameter.h"
#include "engine/filters/enginefilterbiquad1.h"
#include "util/sample.h"

namespace {
// one octave wide (Q = sqrt(2)), as a 10-band octave graphic EQ
constexpr double kQ = 1.41421356;
constexpr double kCenters[RoomEQEffectGroupState::kBands] =
        {31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};

// below Nyquist (the filter design needs it)
double usable(double hz, mixxx::audio::SampleRate rate) {
    return std::min(hz, rate.toDouble() * 0.45);
}
} // namespace

// static
QString RoomEQEffect::getId() {
    return "org.djmantra.effects.roomeq";
}

// static
double RoomEQEffect::centerFrequency(int band) {
    return kCenters[band];
}

// static
EffectManifestPointer RoomEQEffect::getManifest() {
    EffectManifestPointer pManifest(new EffectManifest());
    pManifest->setId(getId());
    pManifest->setName(QObject::tr("DJ Mantra Room EQ"));
    pManifest->setShortName(QObject::tr("Room EQ"));
    pManifest->setAuthor("DJ Mantra");
    pManifest->setVersion("1.0");
    pManifest->setDescription(QObject::tr(
            "Ten octave bands (31.5 Hz - 16 kHz) to correct the room, on the "
            "main output only."));
    pManifest->setEffectRampsFromDry(true);
    for (int i = 0; i < RoomEQEffectGroupState::kBands; ++i) {
        EffectManifestParameterPointer band = pManifest->addParameter();
        const QString name = kCenters[i] < 1000
                ? QStringLiteral("%1 Hz").arg(kCenters[i])
                : QStringLiteral("%1 kHz").arg(kCenters[i] / 1000);
        band->setId(QStringLiteral("band%1").arg(i));
        band->setName(name);
        band->setShortName(name);
        band->setDescription(QObject::tr("Gain of this octave in dB"));
        band->setValueScaler(EffectManifestParameter::ValueScaler::Linear);
        band->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
        band->setNeutralPointOnScale(0.5);
        band->setRange(-12, 0, 12);
    }
    return pManifest;
}

RoomEQEffectGroupState::RoomEQEffectGroupState(const mixxx::EngineParameters& engineParameters)
        : EffectState(engineParameters) {
    for (int i = 0; i < kBands; ++i) {
        m_bands[i] = new EngineFilterBiquad1Peaking(engineParameters.sampleRate(),
                usable(kCenters[i], engineParameters.sampleRate()),
                kQ);
    }
    for (auto& buf : m_pBufs) {
        buf = SampleUtil::alloc(engineParameters.samplesPerBuffer());
    }
}

RoomEQEffectGroupState::~RoomEQEffectGroupState() {
    for (auto* pFilter : m_bands) {
        delete pFilter;
    }
    for (auto* buf : m_pBufs) {
        SampleUtil::free(buf);
    }
}

void RoomEQEffect::loadEngineEffectParameters(
        const QMap<QString, EngineEffectParameterPointer>& parameters) {
    for (int i = 0; i < RoomEQEffectGroupState::kBands; ++i) {
        m_pBands[i] = parameters.value(QStringLiteral("band%1").arg(i));
    }
}

void RoomEQEffect::processChannel(
        RoomEQEffectGroupState* pState,
        const CSAMPLE* pInput,
        CSAMPLE* pOutput,
        const mixxx::EngineParameters& engineParameters,
        const EffectEnableState enableState,
        const GroupFeatureState& groupFeatures) {
    Q_UNUSED(groupFeatures);
    const auto rate = engineParameters.sampleRate();
    const SINT samples = engineParameters.samplesPerBuffer();
    const CSAMPLE* pFrom = pInput;
    int buf = 0;
    for (int i = 0; i < RoomEQEffectGroupState::kBands; ++i) {
        const double gain = enableState == EffectEnableState::Disabling || !m_pBands[i]
                ? 0.0
                : m_pBands[i]->value();
        if (gain != pState->m_oldGain[i]) {
            pState->m_bands[i]->setFrequencyCorners(rate, usable(kCenters[i], rate), kQ, gain);
            pState->m_oldGain[i] = gain;
        }
        if (gain == 0) {
            pState->m_bands[i]->pauseFilter();
            continue;
        }
        pState->m_bands[i]->process(pFrom, pState->m_pBufs[buf], samples);
        pFrom = pState->m_pBufs[buf];
        buf = 1 - buf;
    }
    if (pFrom != pOutput) {
        SampleUtil::copy(pOutput, pFrom, samples);
    }
}
