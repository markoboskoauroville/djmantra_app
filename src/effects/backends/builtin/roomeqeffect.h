#pragma once

#include <array>

#include "effects/backends/effectprocessor.h"
#include "util/class.h"
#include "util/types.h"

class EngineFilterBiquad1Peaking;

/// DJ Mantra ROOM EQ (Marko 14.10.2026: "EQ output to adjust actually to the room, correcting
/// the room"): 31 third-octave bands (ISO, 20 Hz .. 20 kHz), +-12 dB each (parameter1..31 in dB),
/// peaking filters a third of an octave wide, as a 31-band graphic / room-correction EQ. It runs on [MasterOutput] (effect unit 4): the speakers hear it, the
/// recording does not. Its curve is measured in the room (RoomEqPage.qml, RoomEqBridge.java) or
/// drawn by hand.
class RoomEQEffectGroupState : public EffectState {
  public:
    static constexpr int kBands = 31;
    explicit RoomEQEffectGroupState(const mixxx::EngineParameters& engineParameters);
    ~RoomEQEffectGroupState() override;

    std::array<EngineFilterBiquad1Peaking*, kBands> m_bands{};
    std::array<double, kBands> m_oldGain{};
    std::array<CSAMPLE*, 2> m_pBufs{};
};

class RoomEQEffect : public EffectProcessorImpl<RoomEQEffectGroupState> {
  public:
    RoomEQEffect() = default;
    ~RoomEQEffect() override = default;

    static QString getId();
    static EffectManifestPointer getManifest();
    static double centerFrequency(int band);

    void loadEngineEffectParameters(
            const QMap<QString, EngineEffectParameterPointer>& parameters) override;

    void processChannel(
            RoomEQEffectGroupState* pState,
            const CSAMPLE* pInput,
            CSAMPLE* pOutput,
            const mixxx::EngineParameters& engineParameters,
            const EffectEnableState enableState,
            const GroupFeatureState& groupFeatureState) override;

  private:
    QString debugString() const {
        return getId();
    }

    std::array<EngineEffectParameterPointer, RoomEQEffectGroupState::kBands> m_pBands;

    DISALLOW_COPY_AND_ASSIGN(RoomEQEffect);
};
