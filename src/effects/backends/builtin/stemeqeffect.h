#pragma once

#include <memory>

#include "control/pollingcontrolproxy.h"
#include "effects/backends/builtin/stemeq.h"
#include "effects/backends/effectprocessor.h"

/// DJ Mantra: the deck EQ with a "Neural Mix" mode (djmantra::StemEq): the same
/// three knobs and kills as the other mixing EQs; the button "stems" turns them into
/// drums (low), vocals (mid) and harmonies (high).
/// [EqualizerRack1_[ChannelN]_Effect1],button_parameter4 = the mode.
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

    PollingControlProxy m_pLoFreqCorner;
    PollingControlProxy m_pHiFreqCorner;
};
