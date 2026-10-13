#pragma once

#include "effects/backends/builtin/djayfx.h"
#include "effects/backends/effectprocessor.h"

/// DJ Mantra: djay Pro's Underwater, Cooldown and Echo Out as Mixxx effects
/// (djmantra::Underwater / Cooldown / EchoOut). Parameters: "amount" (djay's slider;
/// Echo Out: "beats") and "mix" (djay's D/W knob). Each starts fresh when it is
/// switched on.
class DjayFxGroupState final : public EffectState {
  public:
    explicit DjayFxGroupState(const mixxx::EngineParameters& engineParameters)
            : EffectState(engineParameters),
              underwater(engineParameters.sampleRate().toDouble()),
              cooldown(engineParameters.sampleRate().toDouble()),
              echoOut(engineParameters.sampleRate().toDouble()),
              djmFilter(engineParameters.sampleRate().toDouble()) {
    }
    djmantra::Underwater underwater;
    djmantra::Cooldown cooldown;
    djmantra::EchoOut echoOut;
    djmantra::DjmFilter djmFilter;
    bool lastEngaged = false;
};

class DjayFxEffectBase : public EffectProcessorImpl<DjayFxGroupState> {
  public:
    void loadEngineEffectParameters(
            const QMap<QString, EngineEffectParameterPointer>& parameters) override;

  protected:
    static EffectManifestPointer makeManifest(const QString& id,
            const QString& name,
            const QString& description,
            bool beats);

    EngineEffectParameterPointer m_pAmount;
    EngineEffectParameterPointer m_pMix;
    EngineEffectParameterPointer m_pEngaged; // Echo Out only
};

class UnderwaterEffect : public DjayFxEffectBase {
  public:
    static QString getId();
    static EffectManifestPointer getManifest();
    void processChannel(DjayFxGroupState* pState,
            const CSAMPLE* pInput,
            CSAMPLE* pOutput,
            const mixxx::EngineParameters& engineParameters,
            const EffectEnableState enableState,
            const GroupFeatureState& groupFeatures) override;
};

class CooldownEffect : public DjayFxEffectBase {
  public:
    static QString getId();
    static EffectManifestPointer getManifest();
    void processChannel(DjayFxGroupState* pState,
            const CSAMPLE* pInput,
            CSAMPLE* pOutput,
            const mixxx::EngineParameters& engineParameters,
            const EffectEnableState enableState,
            const GroupFeatureState& groupFeatures) override;
};

class EchoOutEffect : public DjayFxEffectBase {
  public:
    static QString getId();
    static EffectManifestPointer getManifest();
    void processChannel(DjayFxGroupState* pState,
            const CSAMPLE* pInput,
            CSAMPLE* pOutput,
            const mixxx::EngineParameters& engineParameters,
            const EffectEnableState enableState,
            const GroupFeatureState& groupFeatures) override;
};

/// Pioneer DJM's Sound Color FX FILTER (djmantra::DjmFilter): "filter" (the knob, linked
/// to the deck's FILTER / quick effect knob) and "resonance"
class DjmFilterEffect : public EffectProcessorImpl<DjayFxGroupState> {
  public:
    static QString getId();
    static EffectManifestPointer getManifest();
    void loadEngineEffectParameters(
            const QMap<QString, EngineEffectParameterPointer>& parameters) override;
    void processChannel(DjayFxGroupState* pState,
            const CSAMPLE* pInput,
            CSAMPLE* pOutput,
            const mixxx::EngineParameters& engineParameters,
            const EffectEnableState enableState,
            const GroupFeatureState& groupFeatures) override;

  private:
    EngineEffectParameterPointer m_pFilter;
    EngineEffectParameterPointer m_pResonance;
};
