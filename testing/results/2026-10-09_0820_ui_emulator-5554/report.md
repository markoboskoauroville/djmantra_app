# UI test emulator-5554

Controls tested: 134, failed: 23. Deck 1 playing: yes. Sound: peak 0

| Orientation | Page | Widget | Control | Action | Values | Result |
|---|---|---|---|---|---|---|
| portrait | mixer | Knob | `[QuickEffectRack1_[Channel1]],super1` | turn | 0.507874 0.531496 0.547244 0.562992 0.57874 0.594488 | pass |
| portrait | mixer | ChannelFader | `[Channel1],volume` | drag |  | **FAIL** |
| portrait | mixer | TrimKnob | `[Channel1],pregain` | turn | 1.022 1.06745 1.09093 1.11492 1.13944 1.19012 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter3` | turn | 1.06769 1.09125 1.13995 1.19083 1.24398 1.27143 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter2` | turn | 1.06769 1.11534 1.16512 1.19083 1.24398 1.2995 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter1` | turn | 1.04463 1.09125 1.13995 1.16512 1.21711 1.27143 | pass |
| portrait | mixer | - | `[Channel1],vu_meter` | tap |  | **FAIL** |
| portrait | mixer | - | `[Channel2],vu_meter` | tap |  | **FAIL** |
| portrait | mixer | TrimKnob | `[Channel2],pregain` | turn | 1.022 1.06745 1.09093 1.11492 1.13944 1.19012 | pass |
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
| portrait | mixer | PlayButton | `[Channel1],play` | tap | 1 | pass |
| portrait | mixer | Crossfader | `[Master],crossfader` | drag | 0.0299439 0.0532335 0.0765232 0.0998129 0.123103 0.146392 | pass |
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
| portrait | pads | Pad | `[Channel1],hotcue_1_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel1],hotcue_2_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel1],hotcue_3_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel1],hotcue_4_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel1],hotcue_5_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel1],hotcue_6_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel1],hotcue_7_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel1],hotcue_8_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel2],hotcue_1_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel2],hotcue_2_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel2],hotcue_3_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel2],hotcue_4_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel2],hotcue_5_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel2],hotcue_6_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel2],hotcue_7_activate` | tap | 1 0 | pass |
| portrait | pads | Pad | `[Channel2],hotcue_8_activate` | tap | 1 0 | pass |
| portrait | mixer | TabButton | `[DJMantra],p_mixer` | page | 1 | pass |
| portrait | deck1 | SelectorButton | `[DJMantra],p_deck1` | page | 1 | pass |
| portrait | deck1 | PitchFader | `[Channel1],rate` | drag | 0.0142985 0.0449381 0.0653645 0.0898762 0.112345 0.132772 | pass |
| portrait | deck1 | BendButton | `[Channel1],rate_temp_down` | tap | 1 0 | pass |
| portrait | deck1 | BendButton | `[Channel1],rate_temp_up` | tap | 1 0 | pass |
| portrait | deck2 | SelectorButton | `[DJMantra],p_deck2` | page | 1 | pass |
| portrait | deck2 | PitchFader | `[Channel2],rate` | drag | 0.0388102 0.0633218 0.0837482 0.106217 0.128686 0.151155 | pass |
| portrait | deck2 | BendButton | `[Channel2],rate_temp_down` | tap | 1 0 | pass |
| portrait | deck2 | BendButton | `[Channel2],rate_temp_up` | tap | 1 0 | pass |
| portrait | mixer | SelectorButton | `[DJMantra],p_mixer` | page | 1 | pass |
| portrait | library | LibraryButton | `[DJMantra],show_library` | page | 1 | pass |
| portrait | library | LoadButton | `[Channel1],LoadSelectedTrack` | tap | 1 0 | pass |
| portrait | library | LoadButton | `[Channel2],LoadSelectedTrack` | tap | 1 0 | pass |
| portrait | decks | BackButton | `[DJMantra],show_library` | page | 0 | pass |
| landscape | mixer | SyncButton | `[Channel1],sync_enabled` | tap | 1 0 | pass |
| landscape | mixer | PitchFader | `[Channel1],rate` | drag | -0.000979915 0.0247601 0.0505001 0.0762402 0.0968322 0.122572 | pass |
| landscape | mixer | BendButton | `[Channel1],rate_temp_down` | tap | 1 0 | pass |
| landscape | mixer | BendButton | `[Channel1],rate_temp_up` | tap | 1 0 | pass |
| landscape | mixer | Knob | `[QuickEffectRack1_[Channel1]],super1` | turn | 0.877953 0.893701 0.909449 0.925197 0.933071 0.948819 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter3` | turn | 2.55678 2.67089 2.72984 2.7901 2.91462 2.97895 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter2` | turn | 2.67089 2.72984 2.85168 2.91462 2.97895 3.1119 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter1` | turn | 2.61322 2.67089 2.7901 2.85168 2.97895 3.0447 | pass |
| landscape | mixer | TrimKnob | `[Channel1],pregain` | turn | 1.96297 2.00614 2.05027 2.09537 2.14145 2.18856 | pass |
| landscape | mixer | TrimKnob | `[Channel2],pregain` | turn |  | **FAIL** |
| landscape | mixer | ChannelFader | `[Channel1],volume` | drag | 0.488572 0.509381 0.527792 0.54355 0.559698 0.579601 | pass |
| landscape | mixer | - | `[Channel1],vu_meter` | tap |  | **FAIL** |
| landscape | mixer | - | `[Channel2],vu_meter` | tap |  | **FAIL** |
| landscape | mixer | ChannelFader | `[Channel2],volume` | drag |  | **FAIL** |
| landscape | mixer | Knob | `[QuickEffectRack1_[Channel2]],super1` | turn | 0.515748 0.531496 0.547244 0.555118 0.570866 0.586614 | pass |
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
| landscape | mixer | Crossfader | `[Master],crossfader` | drag | 0.0226002 0.0451566 0.067713 0.0902693 0.112826 0.132876 | pass |
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
| landscape | library | LibraryButton | `[DJMantra],show_library` | page | 1 | pass |
| landscape | library | LoadButton | `[Channel1],LoadSelectedTrack` | tap | 1 0 | pass |
| landscape | library | LoadButton | `[Channel2],LoadSelectedTrack` | tap | 1 0 | pass |
| landscape | decks | BackButton | `[DJMantra],show_library` | page | 0 | pass |

Screenshots: one per orientation and page (`<orientation>-<page>.png`).
