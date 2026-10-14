#pragma once

#include <memory>

#include "control/pollingcontrolproxy.h"
#include "effects/backends/builtin/stemeq.h"
#include "effects/backends/effectprocessor.h"

/// DJ Mantra: the deck EQ and the "Neural Mix" (djmantra::StemEq): the three bands
/// (parameter1..3, kills button_parameter1..3) and the stem levels drums, vocals,
/// harmonies (parameter4..6), all applied together (V63). button_parameter4 = which
/// three the knobs turn (the screen's mode; the controller's SHIFT flips it).
class StemEQEffectGroupState final : public EffectState {
  public:
    explicit StemEQEffectGroupState(const mixxx::EngineParameters& engineParameters)
            : EffectState(engineParameters),
              m_eq(engineParameters.sampleRate().toDouble()) {
    }
    djmantra::StemEq m_eq;
};

class StemEQEffect : public EffectProcessorImpl<StemEQEffectGroupState> {
  public:
    StemEQEffect();
    ~StemEQEffect() override = default;

    static QString getId();
    static EffectManifestPointer getManifest();

    void loadEngineEffectParameters(
            const QMap<QString, EngineEffectParameterPointer>& parameters) override;

    void processChannel(
            StemEQEffectGroupState* pState,
            const CSAMPLE* pInput,
            CSAMPLE* pOutput,
            const mixxx::EngineParameters& engineParameters,
            const EffectEnableState enableState,
            const GroupFeatureState& groupFeatureState) override;

  private:
    StemEQEffect(const StemEQEffect&) = delete;
    void operator=(const StemEQEffect&) = delete;

    QString debugString() const {
        return getId();
    }

    EngineEffectParameterPointer m_pPotLow;
    EngineEffectParameterPointer m_pPotMid;
    EngineEffectParameterPointer m_pPotHigh;
    EngineEffectParameterPointer m_pKillLow;
    EngineEffectParameterPointer m_pKillMid;
    EngineEffectParameterPointer m_pKillHigh;
    EngineEffectParameterPointer m_pStems;
    EngineEffectParameterPointer m_pDrums;
    EngineEffectParameterPointer m_pVocals;
    EngineEffectParameterPointer m_pHarmonies;

    PollingControlProxy m_pLoFreqCorner;
    PollingControlProxy m_pHiFreqCorner;
};
