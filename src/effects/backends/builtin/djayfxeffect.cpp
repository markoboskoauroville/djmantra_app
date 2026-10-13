#include "effects/backends/builtin/djayfxeffect.h"

#include "effects/backends/effectmanifest.h"
#include "engine/effects/engineeffectparameter.h"

void DjayFxEffectBase::loadEngineEffectParameters(
        const QMap<QString, EngineEffectParameterPointer>& parameters) {
    m_pAmount = parameters.value(parameters.contains("beats") ? "beats" : "amount");
    m_pMix = parameters.value("mix");
    m_pEngaged = parameters.value("engaged");
}

// static
EffectManifestPointer DjayFxEffectBase::makeManifest(const QString& id,
        const QString& name,
        const QString& description,
        bool beats) {
    EffectManifestPointer pManifest(new EffectManifest());
    pManifest->setId(id);
    pManifest->setName(name);
    pManifest->setShortName(name);
    pManifest->setAuthor("DJ Mantra");
    pManifest->setVersion("1.0");
    pManifest->setDescription(description);
    pManifest->setEffectRampsFromDry(true);

    EffectManifestParameterPointer amount = pManifest->addParameter();
    if (beats) {
        amount->setId("beats");
        amount->setName(QObject::tr("Beats"));
        amount->setShortName(QObject::tr("Beats"));
        amount->setDescription(QObject::tr("The echo time in beats, fixed when pressed"));
        amount->setUnitsHint(EffectManifestParameter::UnitsHint::Beats);
        amount->setRange(0.25, 2, 2); // djay: 1 or 2 beats
    } else {
        amount->setId("amount");
        amount->setName(QObject::tr("Amount"));
        amount->setShortName(QObject::tr("Amount"));
        amount->setDescription(QObject::tr("How deep the effect goes"));
        amount->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
        amount->setRange(0, 0.5, 1);
    }
    amount->setValueScaler(EffectManifestParameter::ValueScaler::Linear);

    EffectManifestParameterPointer mix = pManifest->addParameter();
    mix->setId("mix");
    mix->setName(QObject::tr("Dry/Wet"));
    mix->setShortName(QObject::tr("D/W"));
    mix->setDescription(QObject::tr("Dry / wet mix"));
    mix->setValueScaler(EffectManifestParameter::ValueScaler::Linear);
    mix->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
    mix->setRange(0, 1, 1);

    if (beats) {
        // Echo Out runs while chosen (it keeps the last beats); this button cuts
        // the song into its echo (djay's ON), [..._EffectN],button_parameter1
        EffectManifestParameterPointer engaged = pManifest->addParameter();
        engaged->setId("engaged");
        engaged->setName(QObject::tr("Echo out"));
        engaged->setShortName(QObject::tr("Out"));
        engaged->setDescription(QObject::tr("On: the song stops and its last beats echo on"));
        engaged->setValueScaler(EffectManifestParameter::ValueScaler::Toggle);
        engaged->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
        engaged->setRange(0, 0, 1);
    }
    return pManifest;
}

// static
QString UnderwaterEffect::getId() {
    return "org.djmantra.effects.underwater";
}

// static
EffectManifestPointer UnderwaterEffect::getManifest() {
    return makeManifest(getId(),
            QObject::tr("Underwater"),
            QObject::tr("The song heard from under water: a resonant, drifting lowpass"),
            false);
}

void UnderwaterEffect::processChannel(DjayFxGroupState* pState,
        const CSAMPLE* pInput,
        CSAMPLE* pOutput,
        const mixxx::EngineParameters& engineParameters,
        const EffectEnableState enableState,
        const GroupFeatureState& groupFeatures) {
    Q_UNUSED(groupFeatures);
    pState->underwater.setSampleRate(engineParameters.sampleRate().toDouble());
    if (enableState == EffectEnableState::Enabling) {
        pState->underwater.reset();
    }
    pState->underwater.process(pInput,
            pOutput,
            static_cast<int>(engineParameters.framesPerBuffer()),
            m_pAmount->value(),
            m_pMix->value());
}

// static
QString CooldownEffect::getId() {
    return "org.djmantra.effects.cooldown";
}

// static
EffectManifestPointer CooldownEffect::getManifest() {
    return makeManifest(getId(),
            QObject::tr("Cooldown"),
            QObject::tr("The energy drops: the highs and the deepest lows glide "
                        "away and a reverb washes in"),
            false);
}

void CooldownEffect::processChannel(DjayFxGroupState* pState,
        const CSAMPLE* pInput,
        CSAMPLE* pOutput,
        const mixxx::EngineParameters& engineParameters,
        const EffectEnableState enableState,
        const GroupFeatureState& groupFeatures) {
    Q_UNUSED(groupFeatures);
    pState->cooldown.setSampleRate(engineParameters.sampleRate().toDouble());
    if (enableState == EffectEnableState::Enabling) {
        pState->cooldown.reset();
    }
    pState->cooldown.process(pInput,
            pOutput,
            static_cast<int>(engineParameters.framesPerBuffer()),
            m_pAmount->value(),
            m_pMix->value());
}

// static
QString EchoOutEffect::getId() {
    return "org.djmantra.effects.echoout";
}

// static
EffectManifestPointer EchoOutEffect::getManifest() {
    return makeManifest(getId(),
            QObject::tr("Echo Out"),
            QObject::tr("Leave on an echo: the song stops and its last beats "
                        "echo on, fading"),
            true);
}

void EchoOutEffect::processChannel(DjayFxGroupState* pState,
        const CSAMPLE* pInput,
        CSAMPLE* pOutput,
        const mixxx::EngineParameters& engineParameters,
        const EffectEnableState enableState,
        const GroupFeatureState& groupFeatures) {
    pState->echoOut.setSampleRate(engineParameters.sampleRate().toDouble());
    if (enableState == EffectEnableState::Enabling) {
        pState->echoOut.reset();
    }
    const double beatSeconds = groupFeatures.beat_length.has_value()
            ? groupFeatures.beat_length->seconds
            : 0.5; // no tempo: 120 BPM
    pState->echoOut.process(pInput,
            pOutput,
            static_cast<int>(engineParameters.framesPerBuffer()),
            m_pEngaged && m_pEngaged->toBool(),
            m_pAmount->value(),
            beatSeconds,
            m_pMix->value());
}
