# UI test 41241FDH2S10SJ

Controls tested: 140, failed: 45. Deck 1 playing: yes. Sound: peak 0

| Orientation | Page | Widget | Control | Action | Values | Result |
|---|---|---|---|---|---|---|
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter3` | turn | 1.04463 1.09125 1.11534 1.16512 1.21711 1.27143 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter2` | turn | 1.02207 1.06769 1.11534 1.16512 1.19083 1.24398 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter1` | turn | 1.02207 1.04463 1.09125 1.13995 1.19083 1.24398 | pass |
| portrait | mixer | Knob | `[QuickEffectRack1_[Channel1]],super1` | turn | 0.515748 0.531496 0.547244 0.562992 0.57874 0.586614 | pass |
| portrait | mixer | LoadDeckButton | `[DJMantra],load_deck1` | tap | 1 0 | pass |
| portrait | mixer | LoadDeckButton | `[DJMantra],load_deck2` | tap | 1 0 | pass |
| portrait | mixer | Knob | `[Master],gain` | turn | 1.02571 1.07912 1.10686 1.16451 1.19444 1.22515 | pass |
| portrait | mixer | PflButton | `[Channel1],pfl` | tap | 1 | pass |
| portrait | mixer | PflButton | `[Channel2],pfl` | tap | 1 | pass |
| portrait | mixer | ChannelFader | `[Channel1],volume` | drag |  | **FAIL** |
| portrait | mixer | ChannelFader | `[Channel2],volume` | drag |  | **FAIL** |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter3` | turn | 1.04463 1.06769 1.11534 1.16512 1.19083 1.24398 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter2` | turn | 1.02207 1.09125 1.13995 1.16512 1.21711 1.27143 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter1` | turn | 1.02207 1.06769 1.11534 1.13995 1.19083 1.24398 | pass |
| portrait | mixer | Knob | `[QuickEffectRack1_[Channel2]],super1` | turn | 0.515748 0.523622 0.53937 0.555118 0.570866 0.586614 | pass |
| portrait | mixer | SyncButton | `[Channel1],sync_enabled` | tap | 1 0 | pass |
| portrait | mixer | RoundButton | `[Channel1],keylock` | tap | 1 | pass |
| portrait | mixer | SyncButton | `[Channel2],sync_enabled` | tap | 1 0 | pass |
| portrait | mixer | CueButton | `[Channel1],cue_default` | tap | 1 0 | pass |
| portrait | mixer | CueButton | `[Channel2],cue_default` | tap | 1 0 | pass |
| portrait | mixer | PlayButton | `[Channel1],play` | tap | 1 | pass |
| portrait | mixer | Crossfader | `[Master],crossfader` | drag | 0.0133084 0.036598 0.0632148 0.0865045 0.106467 0.129757 | pass |
| portrait | mixer | PlayButton | `[Channel2],play` | tap | 1 | pass |
| portrait | waveforms | TabButton | `[DJMantra],p_waveforms` | page | 1 | pass |
| portrait | waveforms | Waveform | `[Channel1],waveform` | 2x tap |  | **FAIL** |
| portrait | waveforms | Waveform | `[Channel2],waveform` | 2x tap |  | **FAIL** |
| portrait | waveforms | IconButton | `[Channel1],waveform_zoom_down` | tap | 1 0 | pass |
| portrait | waveforms | IconButton | `[Channel1],loop_halve` | tap | 1 0 | pass |
| portrait | waveforms | LoopButton | `[Channel1],beatloop_activate` | tap | 1 0 | pass |
| portrait | waveforms | IconButton | `[Channel1],loop_double` | tap | 1 0 | pass |
| portrait | waveforms | IconButton | `[Channel1],waveform_zoom_up` | tap | 1 0 | pass |
| portrait | waveforms | IconButton | `[Channel2],waveform_zoom_down` | tap | 1 0 | pass |
| portrait | waveforms | IconButton | `[Channel2],loop_halve` | tap | 1 0 | pass |
| portrait | waveforms | LoopButton | `[Channel2],beatloop_activate` | tap | 1 0 | pass |
| portrait | waveforms | IconButton | `[Channel2],loop_double` | tap | 1 0 | pass |
| portrait | waveforms | IconButton | `[Channel2],waveform_zoom_up` | tap | 1 0 | pass |
| portrait | pads | TabButton | `[DJMantra],p_pads` | page | 1 | pass |
| portrait | pads | ModeButton | `[DJMantra],d1_hotcue` | tap | 1 0 | pass |
| portrait | pads | ModeButton | `[DJMantra],d1_loop` | tap | 1 | pass |
| portrait | pads | ModeButton | `[DJMantra],d1_fx` | tap | 1 | pass |
| portrait | pads | ModeButton | `[DJMantra],d1_sampler` | tap | 1 | pass |
| portrait | pads | Pad | `[Channel1],hotcue_1_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel1],hotcue_2_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel1],hotcue_3_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel1],hotcue_4_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel1],hotcue_5_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel1],hotcue_6_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel1],hotcue_7_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel1],hotcue_8_activate` | tap |  | **FAIL** |
| portrait | pads | ModeButton | `[DJMantra],d2_hotcue` | tap | 1 0 | pass |
| portrait | pads | ModeButton | `[DJMantra],d2_loop` | tap | 1 | pass |
| portrait | pads | ModeButton | `[DJMantra],d2_fx` | tap | 1 | pass |
| portrait | pads | ModeButton | `[DJMantra],d2_sampler` | tap | 1 | pass |
| portrait | pads | Pad | `[Channel2],hotcue_1_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel2],hotcue_2_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel2],hotcue_3_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel2],hotcue_4_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel2],hotcue_5_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel2],hotcue_6_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel2],hotcue_7_activate` | tap |  | **FAIL** |
| portrait | pads | Pad | `[Channel2],hotcue_8_activate` | tap |  | **FAIL** |
| portrait | mixer | TabButton | `[DJMantra],p_mixer` | page | 1 | pass |
| portrait | deck1 | SelectorButton | `[DJMantra],p_deck1` | page | 1 | pass |
| portrait | deck1 | PitchFader | `[Channel1],rate` | drag | 0.015998 0.0419948 0.0619923 0.087989 0.103987 0.127984 | pass |
| portrait | deck1 | BendButton | `[Channel1],rate_temp_down` | tap | 1 0 | pass |
| portrait | deck1 | BendButton | `[Channel1],rate_temp_up` | tap | 1 0 | pass |
| portrait | deck2 | SelectorButton | `[DJMantra],p_deck2` | page | 1 | pass |
| portrait | deck2 | PitchFader | `[Channel2],rate` | drag | 0.0179978 0.0419948 0.0759905 0.0979878 0.119985 0.141982 | pass |
| portrait | deck2 | BendButton | `[Channel2],rate_temp_down` | tap | 1 0 | pass |
| portrait | deck2 | BendButton | `[Channel2],rate_temp_up` | tap | 1 0 | pass |
| portrait | mixer | SelectorButton | `[DJMantra],p_mixer` | page | 1 | pass |
| landscape | mixer | SyncButton | `[Channel1],sync_enabled` | tap | 1 0 | pass |
| landscape | mixer | PitchFader | `[Channel1],rate` | drag | 0.03659 0.0674781 0.0932181 0.118958 0.13955 0.16529 | pass |
| landscape | mixer | BendButton | `[Channel1],rate_temp_down` | tap | 1 0 | pass |
| landscape | mixer | BendButton | `[Channel1],rate_temp_up` | tap | 1 0 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter3` | turn | 2.55678 2.67089 2.72984 2.85168 2.91462 3.0447 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter2` | turn | 2.50157 2.61322 2.67089 2.7901 2.91462 2.97895 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter1` | turn | 2.50157 2.61322 2.67089 2.7901 2.85168 2.91462 | pass |
| landscape | mixer | Knob | `[QuickEffectRack1_[Channel1]],super1` | turn | 0.870079 0.885827 0.901575 0.917323 0.925197 0.940945 | pass |
| landscape | mixer | LoadDeckButton | `[DJMantra],load_deck1` | tap | 1 0 | pass |
| landscape | mixer | LoadDeckButton | `[DJMantra],load_deck2` | tap | 1 0 | pass |
| landscape | mixer | Knob | `[Master],gain` | turn | 2.25297 2.31089 2.3703 2.49374 2.55784 2.6236 | pass |
| landscape | mixer | PflButton | `[Channel1],pfl` | tap | 1 | pass |
| landscape | mixer | PflButton | `[Channel2],pfl` | tap | 1 | pass |
| landscape | mixer | ChannelFader | `[Channel1],volume` | drag | 0.502574 0.523839 0.540271 0.557127 0.57442 0.59216 | pass |
| landscape | mixer | ChannelFader | `[Channel2],volume` | drag | 0.507001 0.517619 0.539406 0.55624 0.57351 0.591226 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter3` | turn | 2.61322 2.72984 2.7901 2.85168 2.97895 3.0447 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter2` | turn |  | **FAIL** |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter1` | turn |  | **FAIL** |
| landscape | mixer | Knob | `[QuickEffectRack1_[Channel2]],super1` | turn |  | **FAIL** |
| landscape | mixer | BendButton | `[Channel2],rate_temp_down` | tap |  | **FAIL** |
| landscape | mixer | BendButton | `[Channel2],rate_temp_up` | tap |  | **FAIL** |
| landscape | mixer | SyncButton | `[Channel2],sync_enabled` | tap |  | **FAIL** |
| landscape | mixer | PitchFader | `[Channel2],rate` | drag |  | **FAIL** |
| landscape | mixer | PlayButton | `[Channel1],play` | tap | 1 | pass |
| landscape | mixer | CueButton | `[Channel1],cue_default` | tap | 1 0 | pass |
| landscape | mixer | RoundButton | `[Channel1],keylock` | tap | 1 | pass |
| landscape | mixer | Crossfader | `[Master],crossfader` | drag | 0.0183647 0.0484399 0.0709963 0.0935527 0.116109 0.138665 | pass |
| landscape | mixer | RoundButton | `[Channel2],keylock` | tap | 1 | pass |
| landscape | mixer | CueButton | `[Channel2],cue_default` | tap | 1 0 | pass |
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
| landscape | pads | ModeButton | `[DJMantra],d1_hotcue` | tap | 1 0 | pass |
| landscape | pads | ModeButton | `[DJMantra],d1_loop` | tap | 1 | pass |
| landscape | pads | ModeButton | `[DJMantra],d1_fx` | tap | 1 | pass |
| landscape | pads | ModeButton | `[DJMantra],d1_sampler` | tap | 1 | pass |
| landscape | pads | Pad | `[Channel1],hotcue_1_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel1],hotcue_2_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel1],hotcue_3_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel1],hotcue_4_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel1],hotcue_5_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel1],hotcue_6_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel1],hotcue_7_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel1],hotcue_8_activate` | tap |  | **FAIL** |
| landscape | pads | ModeButton | `[DJMantra],d2_hotcue` | tap | 1 0 | pass |
| landscape | pads | ModeButton | `[DJMantra],d2_loop` | tap | 1 | pass |
| landscape | pads | ModeButton | `[DJMantra],d2_fx` | tap | 1 | pass |
| landscape | pads | ModeButton | `[DJMantra],d2_sampler` | tap | 1 | pass |
| landscape | pads | Pad | `[Channel2],hotcue_1_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel2],hotcue_2_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel2],hotcue_3_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel2],hotcue_4_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel2],hotcue_5_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel2],hotcue_6_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel2],hotcue_7_activate` | tap |  | **FAIL** |
| landscape | pads | Pad | `[Channel2],hotcue_8_activate` | tap |  | **FAIL** |
| landscape | mixer | TabButton | `[DJMantra],l_mixer` | page | 1 | pass |

Screenshots: one per orientation and page (`<orientation>-<page>.png`).
