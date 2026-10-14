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
            "A 3-band equalizer and the Neural Mix levels (drums, vocals, "
            "harmonies), applied together; the STEMS button says which three the "
            "knobs turn."));
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

    // parameter4..6: the Neural Mix levels, cut only as the bands (V63)
    const struct {
        const char* id;
        const char* name;
    } stemLevels[] = {{"drums", "Drums"}, {"vocals", "Vocals"}, {"harmonies", "Harmonies"}};
    for (const auto& level : stemLevels) {
        EffectManifestParameterPointer p = pManifest->addParameter();
        p->setId(level.id);
        p->setName(QObject::tr(level.name));
        p->setDescription(QObject::tr("Neural Mix level"));
        p->setValueScaler(EffectManifestParameter::ValueScaler::Logarithmic);
        p->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
        p->setNeutralPointOnScale(1.0);
        p->setRange(0, 1.0, 1.0);
    }
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
    m_pDrums = parameters.value("drums");
    m_pVocals = parameters.value("vocals");
    m_pHarmonies = parameters.value("harmonies");
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
        gains.drums = m_pDrums ? m_pDrums->value() : 1.0;
        gains.vocals = m_pVocals ? m_pVocals->value() : 1.0;
        gains.harmonies = m_pHarmonies ? m_pHarmonies->value() : 1.0;
    }
    pState->m_eq.process(pInput,
            pOutput,
            static_cast<int>(engineParameters.framesPerBuffer()),
            gains);
}
