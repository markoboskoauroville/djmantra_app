# UI test emulator-5554

Controls tested: 109, failed: 23. Deck 1 playing: yes. Sound: peak 0

| Orientation | Page | Widget | Control | Action | Values | Result |
|---|---|---|---|---|---|---|
| portrait | mixer | Knob | `[QuickEffectRack1_[Channel1]],super1` | turn | 0.507874 0.515748 0.531496 0.555118 0.562992 0.57874 | pass |
| portrait | mixer | ChannelFader | `[Channel1],volume` | drag |  | **FAIL** |
| portrait | mixer | TrimKnob | `[Channel1],pregain` | turn | 1.022 1.06745 1.09093 1.11492 1.13944 1.19012 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter3` | turn | 1.06769 1.09125 1.13995 1.19083 1.24398 1.27143 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter2` | turn | 1.06769 1.11534 1.16512 1.19083 1.24398 1.2995 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter1` | turn | 1.06769 1.11534 1.13995 1.19083 1.24398 1.2995 | pass |
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
| portrait | deck1 | PitchFader | `[Channel1],rate` | drag | 0.00204264 0.028597 0.051066 0.073535 0.0960041 0.118473 | pass |
| portrait | deck1 | BendButton | `[Channel1],rate_temp_down` | tap | 1 0 | pass |
| portrait | deck1 | BendButton | `[Channel1],rate_temp_up` | tap | 1 0 | pass |
| portrait | deck1 | IconButton | `[Channel1],waveform_zoom_down` | tap | 1 0 | pass |
| portrait | deck1 | IconButton | `[Channel1],loop_halve` | tap | 1 0 | pass |
| portrait | deck1 | LoopButton | `[Channel1],beatloop_activate` | tap | 1 0 | pass |
| portrait | deck1 | IconButton | `[Channel1],loop_double` | tap | 1 0 | pass |
| portrait | deck1 | IconButton | `[Channel1],waveform_zoom_up` | tap | 1 0 | pass |
| portrait | deck2 | SelectorButton | `[DJMantra],p_deck2` | page | 1 | pass |
| portrait | deck2 | PitchFader | `[Channel2],rate` | drag | 0.00408528 0.0163411 0.0469807 0.0674071 0.0939614 0.112345 | pass |
| portrait | deck2 | BendButton | `[Channel2],rate_temp_down` | tap | 1 0 | pass |
| portrait | deck2 | BendButton | `[Channel2],rate_temp_up` | tap | 1 0 | pass |
| portrait | deck2 | IconButton | `[Channel2],waveform_zoom_down` | tap | 1 0 | pass |
| portrait | deck2 | IconButton | `[Channel2],loop_halve` | tap | 1 0 | pass |
| portrait | deck2 | LoopButton | `[Channel2],beatloop_activate` | tap | 1 0 | pass |
| portrait | deck2 | IconButton | `[Channel2],loop_double` | tap | 1 0 | pass |
| portrait | deck2 | IconButton | `[Channel2],waveform_zoom_up` | tap | 1 0 | pass |
| portrait | mixer | SelectorButton | `[DJMantra],p_mixer` | page | 1 | pass |
| portrait | library | LibraryButton | `[DJMantra],show_library` | page |  | **FAIL** |
| landscape | mixer | SyncButton | `[Channel1],sync_enabled` | tap | 1 0 | pass |
| landscape | mixer | PitchFader | `[Channel1],rate` | drag | -0.00204264 0.0391414 0.0648814 0.0906215 0.116361 0.136953 | pass |
| landscape | mixer | BendButton | `[Channel1],rate_temp_down` | tap | 1 0 | pass |
| landscape | mixer | BendButton | `[Channel1],rate_temp_up` | tap | 1 0 | pass |
| landscape | mixer | Knob | `[QuickEffectRack1_[Channel1]],super1` | turn | 0.885827 0.901575 0.917323 0.933071 0.940945 0.956693 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter3` | turn | 2.55678 2.61322 2.72984 2.7901 2.85168 2.97895 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter2` | turn | 2.55678 2.61322 2.72984 2.7901 2.91462 2.97895 | pass |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter1` | turn | 2.50157 2.55678 2.67089 2.72984 2.7901 2.91462 | pass |
| landscape | mixer | TrimKnob | `[Channel1],pregain` | turn | 1.96297 2.00614 2.05027 2.09537 2.14145 2.18856 | pass |
| landscape | mixer | TrimKnob | `[Channel2],pregain` | turn |  | **FAIL** |
| landscape | mixer | ChannelFader | `[Channel1],volume` | drag | 0.497217 0.515266 0.537002 0.552988 0.569368 0.586152 | pass |
| landscape | mixer | ChannelFader | `[Channel2],volume` | drag |  | **FAIL** |
| landscape | mixer | Knob | `[QuickEffectRack1_[Channel2]],super1` | turn | 0.523622 0.53937 0.547244 0.562992 0.57874 0.594488 | pass |
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
| landscape | mixer | Crossfader | `[Master],crossfader` | drag | 0.0150376 0.0451128 0.0676692 0.0877193 0.110276 0.132832 | pass |
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
