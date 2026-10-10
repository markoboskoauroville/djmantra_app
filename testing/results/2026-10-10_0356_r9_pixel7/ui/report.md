# UI test 41241FDH2S10SJ

Controls tested: 104, failed: 24. Deck 1 playing: yes. Sound: peak 0.00211364

| Orientation | Page | Widget | Control | Action | Values | Result |
|---|---|---|---|---|---|---|
| portrait | mixer | Knob | `[QuickEffectRack1_[Channel1]],super1` | turn | 0.515748 0.799213 | pass |
| portrait | mixer | ChannelFader | `[Channel1],volume` | drag |  | **FAIL** |
| portrait | mixer | TrimKnob | `[Channel1],pregain` | turn |  | **FAIL** |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter3` | turn | 1.02207 1.06769 1.11534 1.16512 1.19083 1.24398 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter2` | turn | 1.04463 1.06769 1.11534 1.16512 1.19083 1.24398 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter1` | turn | 1.02207 1.06769 1.11534 1.16512 1.21711 1.24398 | pass |
| portrait | mixer | TrimKnob | `[Channel2],pregain` | turn | 1.022 1.06745 1.09093 1.11492 1.13944 1.16451 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter3` | turn |  | **FAIL** |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter2` | turn |  | **FAIL** |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter1` | turn |  | **FAIL** |
| portrait | mixer | Knob | `[QuickEffectRack1_[Channel2]],super1` | turn |  | **FAIL** |
| portrait | mixer | ChannelFader | `[Channel2],volume` | drag |  | **FAIL** |
| portrait | mixer | SyncButton | `[Channel1],sync_enabled` | tap | 1 0 | pass |
| portrait | mixer | RoundButton | `[Channel1],keylock` | tap | 1 | pass |
| portrait | mixer | SyncButton | `[Channel2],sync_enabled` | tap | 1 0 | pass |
| portrait | mixer | CueSet | `[Channel1],cue_default` | tap | 1 0 | pass |
| portrait | mixer | CueReturn | `[Channel1],cue_gotoandstop` | tap | 1 0 | pass |
| portrait | mixer | CueSet | `[Channel2],cue_default` | tap | 1 0 | pass |
| portrait | mixer | CueReturn | `[Channel2],cue_gotoandstop` | tap | 1 0 | pass |
| portrait | mixer | PlayButton | `[Channel1],play` | tap |  | **FAIL** |
| portrait | mixer | Crossfader | `[Master],crossfader` | drag |  | **FAIL** |
| portrait | mixer | PlayButton | `[Channel2],play` | tap |  | **FAIL** |
| portrait | waveforms | TabButton | `[DJMantra],p_waveforms` | page |  | **FAIL** |
| portrait | pads | TabButton | `[DJMantra],p_pads` | page | 0 | **FAIL** |
| portrait | mixer | TabButton | `[DJMantra],p_mixer` | page | 1 0 | pass |
| portrait | deck1 | SelectorButton | `[DJMantra],p_deck1` | page | 1 | pass |
| portrait | deck1 | PitchFader | `[Channel1],rate` | drag | -0.606664 -0.590323 -0.567854 -0.543342 -0.522916 -0.500447 | pass |
| portrait | deck1 | BendButton | `[Channel1],rate_temp_down` | tap | 1 0 | pass |
| portrait | deck1 | BendButton | `[Channel1],rate_temp_up` | tap | 1 0 | pass |
| portrait | deck1 | IconButton | `[Channel1],waveform_zoom_down` | tap | 1 0 | pass |
| portrait | deck1 | IconButton | `[Channel1],loop_halve` | tap | 1 0 | pass |
| portrait | deck1 | LoopButton | `[Channel1],beatloop_activate` | tap | 1 0 | pass |
| portrait | deck1 | IconButton | `[Channel1],loop_double` | tap | 1 0 | pass |
| portrait | deck1 | IconButton | `[Channel1],waveform_zoom_up` | tap | 1 0 | pass |
| portrait | deck2 | SelectorButton | `[DJMantra],p_deck2` | page | 1 | pass |
| portrait | deck2 | PitchFader | `[Channel2],rate` | drag | -0.604621 -0.582152 -0.559683 -0.539257 -0.514745 -0.494319 | pass |
| portrait | deck2 | BendButton | `[Channel2],rate_temp_down` | tap | 1 0 | pass |
| portrait | deck2 | BendButton | `[Channel2],rate_temp_up` | tap | 1 0 | pass |
| portrait | deck2 | IconButton | `[Channel2],waveform_zoom_down` | tap | 1 0 | pass |
| portrait | deck2 | IconButton | `[Channel2],loop_halve` | tap | 1 0 | pass |
| portrait | deck2 | LoopButton | `[Channel2],beatloop_activate` | tap | 1 0 | pass |
| portrait | deck2 | IconButton | `[Channel2],loop_double` | tap | 1 0 | pass |
| portrait | deck2 | IconButton | `[Channel2],waveform_zoom_up` | tap | 1 0 | pass |
| portrait | mixer | SelectorButton | `[DJMantra],p_mixer` | page | 1 | pass |
| landscape | mixer | SyncButton | `[Channel1],sync_enabled` | tap | 1 0 | pass |
| landscape | mixer | PitchFader | `[Channel1],rate` | drag | -0.606358 -0.580443 -0.554528 -0.528613 -0.507881 -0.481966 | pass |
| landscape | mixer | BendButton | `[Channel1],rate_temp_down` | tap | 1 0 | pass |
| landscape | mixer | BendButton | `[Channel1],rate_temp_up` | tap | 1 0 | pass |
| landscape | mixer | Knob | `[QuickEffectRack1_[Channel1]],super1` | turn | 0.807087 0.822835 0.838583 0.846457 0.862205 0.877953 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter3` | turn | 2.61322 2.67089 2.7901 2.85168 2.97895 3.0447 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter2` | turn | 2.55678 2.67089 2.72984 2.85168 2.91462 3.0447 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter1` | turn | 2.55678 2.67089 2.7901 2.85168 2.91462 3.0447 | pass |
| landscape | mixer | TrimKnob | `[Channel1],pregain` | turn | 1.022 1.06745 1.09093 1.11492 1.13944 1.16451 | pass |
| landscape | mixer | TrimKnob | `[Channel2],pregain` | turn |  | **FAIL** |
| landscape | mixer | ChannelFader | `[Channel1],volume` | drag |  | **FAIL** |
| landscape | mixer | ChannelFader | `[Channel2],volume` | drag |  | **FAIL** |
| landscape | mixer | Knob | `[QuickEffectRack1_[Channel2]],super1` | turn | 0.507874 0.531496 0.53937 0.555118 0.562992 0.586614 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter3` | turn |  | **FAIL** |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter2` | turn |  | **FAIL** |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter1` | turn |  | **FAIL** |
| landscape | mixer | BendButton | `[Channel2],rate_temp_down` | tap |  | **FAIL** |
| landscape | mixer | BendButton | `[Channel2],rate_temp_up` | tap |  | **FAIL** |
| landscape | mixer | SyncButton | `[Channel2],sync_enabled` | tap |  | **FAIL** |
| landscape | mixer | PitchFader | `[Channel2],rate` | drag |  | **FAIL** |
| landscape | mixer | PlayButton | `[Channel1],play` | tap | 1 | pass |
| landscape | mixer | CueSet | `[Channel1],cue_default` | tap | 1 0 | pass |
| landscape | mixer | CueReturn | `[Channel1],cue_gotoandstop` | tap | 1 0 | pass |
| landscape | mixer | RoundButton | `[Channel1],keylock` | tap | 1 | pass |
| landscape | mixer | Crossfader | `[Master],crossfader` | drag | 0.0125313 0.0250627 0.0551378 0.0726817 0.100251 0.120301 | pass |
| landscape | mixer | RoundButton | `[Channel2],keylock` | tap | 1 | pass |
| landscape | mixer | CueSet | `[Channel2],cue_default` | tap | 1 0 | pass |
| landscape | mixer | CueReturn | `[Channel2],cue_gotoandstop` | tap | 1 0 | pass |
| landscape | mixer | PlayButton | `[Channel2],play` | tap | 1 | pass |
| landscape | waveforms | TabButton | `[DJMantra],l_waveforms` | page | 1 | pass |
| landscape | waveforms | Waveform | `[Channel1],waveform` | 2x tap |  | **FAIL** |
| landscape | waveforms | Waveform | `[Channel2],waveform` | 2x tap |  | **FAIL** |
| landscape | waveforms | IconButton | `[Channel1],waveform_zoom_down` | tap | 1 0 | pass |
| landscape | waveforms | IconButton | `[Channel1],loop_halve` | tap | 1 0 | pass |
| landscape | waveforms | LoopButton | `[Channel1],beatloop_activate` | tap | 1 0 | pass |
| landscape | waveforms | IconButton | `[Channel1],loop_double` | tap | 1 0 | pass |
| landscape | waveforms | IconButton | `[Channel1],waveform_zoom_up` | tap | 1 0 | pass |
| landscape | waveforms | IconButton | `[Channel2],waveform_zoom_down` | tap | 1 0 | pass |
| landscape | waveforms | IconButton | `[Channel2],loop_halve` | tap | 1 0 | pass |
| landscape | waveforms | LoopButton | `[Channel2],beatloop_activate` | tap | 1 0 | pass |
| landscape | waveforms | IconButton | `[Channel2],loop_double` | tap | 1 0 | pass |
| landscape | waveforms | IconButton | `[Channel2],waveform_zoom_up` | tap | 1 0 | pass |
| landscape | pads | TabButton | `[DJMantra],l_pads` | page | 1 | pass |
| landscape | pads | Pad | `[Channel1],hotcue_1_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel1],hotcue_2_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel1],hotcue_3_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel1],hotcue_4_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel1],hotcue_5_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel1],hotcue_6_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel1],hotcue_7_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel1],hotcue_8_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel2],hotcue_1_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel2],hotcue_2_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel2],hotcue_3_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel2],hotcue_4_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel2],hotcue_5_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel2],hotcue_6_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel2],hotcue_7_activate` | tap | 1 0 | pass |
| landscape | pads | Pad | `[Channel2],hotcue_8_activate` | tap | 1 0 | pass |
| landscape | mixer | TabButton | `[DJMantra],l_mixer` | page | 1 | pass |

Screenshots: one per orientation and page (`<orientation>-<page>.png`).
