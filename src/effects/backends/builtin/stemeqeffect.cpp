#include "effects/backends/builtin/stemeqeffect.h"

#include "effects/backends/builtin/equalizer_util.h"
#include "effects/backends/effectmanifest.h"
#include "effects/defs.h"
#include "engine/effects/engineeffectparameter.h"

// static
QString StemEQEffect::getId() {
    return "org.djmantra.effects.stemeq";
}

// static
EffectManifestPointer StemEQEffect::getManifest() {
    EffectManifestPointer pManifest(new EffectManifest());
    pManifest->setId(getId());
    pManifest->setName(QObject::tr("DJ Mantra EQ / Neural Mix"));
    pManifest->setShortName(QObject::tr("EQ / STEMS"));
    pManifest->setAuthor("DJ Mantra");
    pManifest->setVersion("1.0");
    pManifest->setDescription(QObject::tr(
            "A 3-band equalizer; its STEMS button turns the low, mid and high "
            "knobs into drums, vocals and harmonies."));
    pManifest->setEffectRampsFromDry(true);
    pManifest->setIsMixingEQ(true);

    EqualizerUtil::createCommonParameters(pManifest.data(), false);

    EffectManifestParameterPointer stems = pManifest->addParameter();
    stems->setId("stems");
    stems->setName(QObject::tr("Stems"));
    stems->setDescription(QObject::tr(
            "Neural Mix: low = drums, mid = vocals, high = harmonies"));
    stems->setValueScaler(EffectManifestParameter::ValueScaler::Toggle);
    stems->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
    stems->setRange(0, 0, 1);
    return pManifest;
}

StemEQEffect::StemEQEffect()
        : m_pLoFreqCorner(kMixerProfile, kLowEqFrequency),
          m_pHiFreqCorner(kMixerProfile, kHighEqFrequency) {
}

void StemEQEffect::loadEngineEffectParameters(
        const QMap<QString, EngineEffectParameterPointer>& parameters) {
    m_pPotLow = parameters.value("low");
    m_pPotMid = parameters.value("mid");
    m_pPotHigh = parameters.value("high");
    m_pKillLow = parameters.value("killLow");
    m_pKillMid = parameters.value("killMid");
    m_pKillHigh = parameters.value("killHigh");
    m_pStems = parameters.value("stems");
}

void StemEQEffect::processChannel(
        StemEQEffectGroupState* pState,
        const CSAMPLE* pInput,
        CSAMPLE* pOutput,
        const mixxx::EngineParameters& engineParameters,
        const EffectEnableState enableState,
        const GroupFeatureState& groupFeatures) {
    Q_UNUSED(groupFeatures);
    pState->m_eq.setSampleRate(engineParameters.sampleRate().toDouble());
    pState->m_eq.setCrossovers(m_pLoFreqCorner.get(), m_pHiFreqCorner.get());
    djmantra::StemEq::Gains gains;
    if (enableState != EffectEnableState::Disabling) {
        gains.low = m_pKillLow->toBool() ? 0.0 : m_pPotLow->value();
        gains.mid = m_pKillMid->toBool() ? 0.0 : m_pPotMid->value();
        gains.high = m_pKillHigh->toBool() ? 0.0 : m_pPotHigh->value();
        gains.stems = m_pStems && m_pStems->toBool();
    }
    pState->m_eq.process(pInput,
            pOutput,
            static_cast<int>(engineParameters.framesPerBuffer()),
            gains);
}
