# UI test 41241FDH2S10SJ

Controls tested: 62, failed: 57. Deck 1 playing: yes. Sound: peak 0.0126871

| Orientation | Page | Widget | Control | Action | Values | Result |
|---|---|---|---|---|---|---|
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter3` | turn | 1.04463 1.06769 1.11534 1.16512 1.19083 1.24398 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter2` | turn | 1.02207 1.06769 1.09125 1.13995 1.19083 1.24398 | pass |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter1` | turn | 1.02207 1.06769 1.11534 1.16512 1.19083 1.24398 | pass |
| portrait | mixer | Knob | `[QuickEffectRack1_[Channel1]],super1` | turn | 0.515748 0.523622 0.53937 0.555118 0.570866 0.586614 | pass |
| portrait | mixer | LoadDeckButton | `[DJMantra],load_deck1` | tap | 1 0 | pass |
| portrait | mixer | LoadDeckButton | `[DJMantra],load_deck2` | tap |  | **FAIL** |
| portrait | mixer | Knob | `[Master],gain` | turn |  | **FAIL** |
| portrait | mixer | PflButton | `[Channel1],pfl` | tap |  | **FAIL** |
| portrait | mixer | PflButton | `[Channel2],pfl` | tap |  | **FAIL** |
| portrait | mixer | ChannelFader | `[Channel1],volume` | drag |  | **FAIL** |
| portrait | mixer | ChannelFader | `[Channel2],volume` | drag |  | **FAIL** |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter3` | turn |  | **FAIL** |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter2` | turn |  | **FAIL** |
| portrait | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter1` | turn |  | **FAIL** |
| portrait | mixer | Knob | `[QuickEffectRack1_[Channel2]],super1` | turn |  | **FAIL** |
| portrait | mixer | SyncButton | `[Channel1],sync_enabled` | tap |  | **FAIL** |
| portrait | mixer | RoundButton | `[Channel1],keylock` | tap |  | **FAIL** |
| portrait | mixer | SyncButton | `[Channel2],sync_enabled` | tap |  | **FAIL** |
| portrait | mixer | CueButton | `[Channel1],cue_default` | tap |  | **FAIL** |
| portrait | mixer | CueButton | `[Channel2],cue_default` | tap |  | **FAIL** |
| portrait | mixer | PlayButton | `[Channel1],play` | tap |  | **FAIL** |
| portrait | mixer | Crossfader | `[Master],crossfader` | drag |  | **FAIL** |
| portrait | mixer | PlayButton | `[Channel2],play` | tap |  | **FAIL** |
| portrait | waveforms | TabButton | `[DJMantra],p_waveforms` | page |  | **FAIL** |
| portrait | pads | TabButton | `[DJMantra],p_pads` | page |  | **FAIL** |
| portrait | mixer | TabButton | `[DJMantra],p_mixer` | page |  | **FAIL** |
| portrait | deck1 | SelectorButton | `[DJMantra],p_deck1` | page |  | **FAIL** |
| portrait | deck2 | SelectorButton | `[DJMantra],p_deck2` | page |  | **FAIL** |
| portrait | mixer | SelectorButton | `[DJMantra],p_mixer` | page |  | **FAIL** |
| landscape | mixer | SyncButton | `[Channel1],sync_enabled` | tap |  | **FAIL** |
| landscape | mixer | PitchFader | `[Channel1],rate` | drag |  | **FAIL** |
| landscape | mixer | BendButton | `[Channel1],rate_temp_down` | tap |  | **FAIL** |
| landscape | mixer | BendButton | `[Channel1],rate_temp_up` | tap |  | **FAIL** |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter3` | turn |  | **FAIL** |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter2` | turn |  | **FAIL** |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel1]_Effect1],parameter1` | turn |  | **FAIL** |
| landscape | mixer | Knob | `[QuickEffectRack1_[Channel1]],super1` | turn |  | **FAIL** |
| landscape | mixer | LoadDeckButton | `[DJMantra],load_deck1` | tap |  | **FAIL** |
| landscape | mixer | LoadDeckButton | `[DJMantra],load_deck2` | tap |  | **FAIL** |
| landscape | mixer | Knob | `[Master],gain` | turn |  | **FAIL** |
| landscape | mixer | PflButton | `[Channel1],pfl` | tap |  | **FAIL** |
| landscape | mixer | PflButton | `[Channel2],pfl` | tap |  | **FAIL** |
| landscape | mixer | ChannelFader | `[Channel1],volume` | drag |  | **FAIL** |
| landscape | mixer | ChannelFader | `[Channel2],volume` | drag |  | **FAIL** |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter3` | turn |  | **FAIL** |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter2` | turn |  | **FAIL** |
| landscape | mixer | Knob | `[EqualizerRack1_[Channel2]_Effect1],parameter1` | turn |  | **FAIL** |
| landscape | mixer | Knob | `[QuickEffectRack1_[Channel2]],super1` | turn |  | **FAIL** |
| landscape | mixer | BendButton | `[Channel2],rate_temp_down` | tap |  | **FAIL** |
| landscape | mixer | BendButton | `[Channel2],rate_temp_up` | tap |  | **FAIL** |
| landscape | mixer | SyncButton | `[Channel2],sync_enabled` | tap |  | **FAIL** |
| landscape | mixer | PitchFader | `[Channel2],rate` | drag |  | **FAIL** |
| landscape | mixer | PlayButton | `[Channel1],play` | tap |  | **FAIL** |
| landscape | mixer | CueButton | `[Channel1],cue_default` | tap |  | **FAIL** |
| landscape | mixer | RoundButton | `[Channel1],keylock` | tap |  | **FAIL** |
| landscape | mixer | Crossfader | `[Master],crossfader` | drag |  | **FAIL** |
| landscape | mixer | RoundButton | `[Channel2],keylock` | tap |  | **FAIL** |
| landscape | mixer | CueButton | `[Channel2],cue_default` | tap |  | **FAIL** |
| landscape | mixer | PlayButton | `[Channel2],play` | tap |  | **FAIL** |
| landscape | waveforms | TabButton | `[DJMantra],l_waveforms` | page |  | **FAIL** |
| landscape | pads | TabButton | `[DJMantra],l_pads` | page |  | **FAIL** |
| landscape | mixer | TabButton | `[DJMantra],l_mixer` | page |  | **FAIL** |

Screenshots: one per orientation and page (`<orientation>-<page>.png`).
