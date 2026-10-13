# CQ-12T MIDI from the Mac (13.10.2026, official CQ MIDI Protocol V1.2 issue 4)

PDF: `CQ_MIDI_Protocol_V1_2_0_iss4.pdf` (allen-heath.com/content/uploads/2024/10/), text in `CQ_MIDI_Protocol.txt`.
Tool: `tools/cq/cq_midi.py` (get / set / tap / listen).

## From the PDF
- **TCP/IP MIDI: the CQ's IP, port 51325, only one connection at a time.** Also **USB MIDI** over the USB-B port
  (the CQ shows up as a MIDI in/out device). MIDI channel 1 for everything.
- **The CQ transmits NRPN whenever a parameter with NRPN control changes on the unit or from a connected app**
  (so the app can follow the CQ's own faders). It does NOT send notes when a soft key is pressed.
- **Soft keys**: Note On `90 SK 7F` press, Note Off `80 SK 00` release; **SK 1/2/3 = 0x30/0x31/0x32** (C3, dec 48-50).
- **Scene**: `B0 00 00 C0 PG` (PG = scene-1).
- **NRPN**: `B0 63 MSB  B0 62 LSB  B0 06 VC  B0 26 VF`; increment `B0 60 00` (+1 dB / mute toggle / pan right),
  decrement `B0 61 00`; **get** `B0 63 MSB B0 62 LSB B0 60 7F` (the CQ answers with the full NRPN).
- **Mute** VC VF = `00 01` on, `00 00` off. Mutes MSB 00: Ip1..16 = LSB 00..0F, St1 18, St2 1A, USB 1C, BT 1E,
  Main LR 44, Out1..6 45..4A, FX1..4 51..54; DCA1..4 = 02 00..03; mute groups 04 00..03.
- **Levels** (input -> mix): Ip n -> Main LR = `40 (n-1)`; Ip1 -> Out1..6 = 40 44..49 (Ip2: 40 50.., +0C per input);
  **Ip1 -> FX1..4 = 4C 14..17**, Ip2 4C 18.., Ip3 4C 1C..1F, Ip4 4C 20.. (4 per input); USB -> FX 4D 04..07;
  FX1..4 -> Main LR = 40 3C..3F. **Outputs / masters**: Main LR 4F 00, Out1..6 4F 01..06, **FX1..4 unit input
  4F 0D..10**, DCA1..4 4F 20..23.
- **Level scale** (VC VF): -inf 00 00, -40 0F 40, -30 1F 00, -20 2E 40, -10 3E 00, -5 4E 40, -3 56 40, 0 dB 62 00,
  +5 73 40, +10 7F 40 (full table on PDF page 15). Relative ±1 dB with 60/61.
- **Pan / balance**: Ip n -> LR = `50 (n-1)`, Ip1 -> Out1/2 50 44 ...; 00 00 full left, 40 00 centre, 7F 7F full right.
- A stereo-linked pair (Ip1/2, Ip3/4) is addressed by its first input (Ip1, Ip3): MixPad shows the pair change.
- **No message sets an FX parameter** (delay time, FX type): only levels, mutes, pans, soft keys, scenes.

## Tested on the CQ-12T (169.254.5.29:51325 over the LAN, MixPad connected meanwhile on 51326)
| What | Sent | Read back (get) | MixPad | Put back |
|---|---|---|---|---|
| read Ip1 -> LR | get 40 00 | 56 40 = -3 dB | -3 dB | - |
| read Ip3 -> LR, Main LR, FX1 input | | 00 00 (-inf), 62 00 (0 dB), 62 00 | Off, 0 dB | - |
| Ip1/2 fader -10 dB | 40 00 3E 00 | 3E 00 | **-10 dB** (`cq_nrpn_ip1_minus10_muted.png`) | 56 40, read back 56 40 |
| Ip1/2 mute | 00 00 00 01 | 00 01 | **red mute** | 00 00 |
| FX1 mute | 00 51 00 01 | 00 01 | | 00 00 |
| Ip1 -> FX1 send -10 dB | 4C 14 3E 00 | 3E 00 | | 00 00 (was -inf) |
All PASS; every value read back after restoring equals the value before.

## Still to do
- Tap tempo: needs a Tap Delay in an FX slot and a soft key set to its tap tempo in CQ MixPad (clicks, Marko's OK).
- Whether the CQ pushes NRPN on its own fader moves: needs one fader moved on the mixer while `cq_midi.py listen`.
