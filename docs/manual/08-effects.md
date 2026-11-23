# Effects
<!--blurb: FILTER, SRR, DIST, INSERT, delay, reverb, chorus and the master chain.-->

## The signal path

Each track runs: voice, then **FILTER**, **SRR**, **DIST**, **INSERT**, the compressor's duck, then LEVEL and PAN and the three sends (**chorus**, **delay**, **reverb**). The sends and the track mix go into the master: the PERFORM effects, the metronome, SPEAKER EQ, a DC blocker, the limiter and a soft clip.

Press [[FX]] for the track's effect pages. Its pages: **FILTER**, **FX** (the sends), **INSERT 1/2**, **INSERT 2/2**, then **COMP 1/2** and **COMP 2/2** (the compressor, see *Modulation and compressor*). Press [[FX]] again (it wraps to the first page) or turn [[SELECT]] (it stops at the ends) to move. The family reopens at the last page used. Hold [[FX]] for PERFORM instead.

All FX pages edit the selected track. **Delay, reverb and chorus** are global and live on the [[GLO]] pages DLY 1, DLY 2, REVERB and CHORUS (stored in the project); their send knobs (CHO, DLY, REV) stay on the FX page.

## FILTER

A filter on every track, after the voice and before DIST. Each hit sweeps it by ENV (accents sweep further); the sweep's time is FDEC on the [[ENV]] page.

| Page | Knob | Label | Values (default) |
| --- | --- | --- | --- |
| FILTER | **KNOB 1** | TYPE | OFF (default), LP, BP, HP, NOT (notch) |
| FILTER | **KNOB 2** | CUT | 30 Hz to 16 kHz, exponential (127 = 16 kHz) |
| FILTER | **KNOB 3** | RESO | 0 to 127 (0) |
| FILTER | **KNOB 4** | ENV | -64 to +63 (0): cutoff offset by the hit envelope times velocity |
| ENV | last row | FDEC | the filter envelope's decay, 5 ms to 2 s (40, about 32 ms) |

LFO destinations F.CUT and F.RES move the filter.

## FX: DIST and the sends

| Knob | Label | Meaning (default 0) |
| --- | --- | --- |
| **KNOB 1** | DST | the track's distortion insert, shown in % |
| **KNOB 2** | CHO | chorus send |
| **KNOB 3** | DLY | delay send |
| **KNOB 4** | REV | reverb send |

## INSERT (1/2 and 2/2)

An effect on every track, after DIST and before LEVEL, PAN and the sends.

**1/2:** [[KNOB 1]] **TYPE**: OFF (default), SOFT, HARD, FOLD, FUZZ, CRUSH, PHASR, FLANG, CHOR. [[KNOB 2]] to [[KNOB 4]] are **A**, **B**, **C**, named by the TYPE (with TYPE OFF they read A B C):

| TYPE | A | B | C |
| --- | --- | --- | --- |
| SOFT, HARD, FOLD, FUZZ | DRIVE (50 %) | TONE (cutoff, default 96) | LEVEL (dB, default 96) |
| CRUSH | BITS (1 to 16, default 9) | RATE (689 Hz to 44 kHz, default about 11 kHz) | LPF (cutoff) |
| PHASR, FLANG, CHOR | RATE (Hz) | DEPTH (default 75 %) | FDBK (75 %) |

**2/2:** [[KNOB 1]] **MIX** (dry to wet, 100 % by default; for the swept types the wet is the effect added to the dry, so 100 % is the whole effect), [[KNOB 2]] **SRR**.

- A TYPE change fades over 3 ms, without a click. TYPE OFF, MIX 0 and SRR OFF leave the sound exactly as before.
- A, B, C, MIX and SRR take MOTION, step locks, LFO DEST and MIDI LEARN (`T3 I.DRIVE`, `T3 SRR`). TYPE does not.
- Saved per project; projects saved before 1.0.0 load them OFF.

## SRR: sample rate reduction

It samples and holds the track's sound after the FILTER and before DIST, aliasing on purpose, for the gritty sound of the classic drum machines. OFF at 0, down to 347 Hz at 127; its row on INSERT 2/2 (and the knob popup) shows the rate in Hz or kHz.

## Delay (GLO DLY 1 and DLY 2)

Tap [[GLO]] and step to the delay pages.

| Page | Knob | Label | Values (default) |
| --- | --- | --- | --- |
| DLY 1 | **KNOB 1** | TIME | 4BAR 2BAR 1/1 1/2 1/4D 1/4 1/8D 4T 1/8 1/16D 8T 1/16 16T 1/32 32T, tempo-synced (D dotted, T triplet), 1/8 by default. A time longer than the delay holds is cut to 1.49 s |
| DLY 1 | **KNOB 2** | FDBK | feedback, 0 to 100 % (50 %) |
| DLY 1 | **KNOB 3** | COLR | the repeats' colour, % (55 %) |
| DLY 1 | **KNOB 4** | MIX | the return level, % (71 %) |
| DLY 2 | **KNOB 1** | MODE | **NORM** (default) or **PING** (ping-pong) |

In PING the echoes alternate between left and right (the first on the left), with FDBK, COLR and the longest time working as in NORM; they are a little softer in the top end. Changing MODE clears the delay. MODE is saved with the project; older projects load NORM.

## Reverb (GLO REVERB)

| Knob | Label | Values (default) |
| --- | --- | --- |
| **KNOB 1** | TYPE | ROOM (default), SPRNG (spring) or **HALL** |
| **KNOB 2** | SIZE | % (71 %) |
| **KNOB 3** | DAMP | % (47 %) |

**HALL** is a smooth, dense reverb in stereo; its tail is wide on headphones and fades to exact silence. SIZE sets its length (from about half a second to several seconds), DAMP how soon its top end fades. Switching TYPE fades the old tail out without a click. TYPE is saved with the project; older projects keep their ROOM or SPRNG. The ROOM tail also fades to true silence now.

## Chorus (GLO CHORUS)

| Knob | Label | Values (default) |
| --- | --- | --- |
| **KNOB 1** | CRT | rate, 0.05 to 40 Hz, exponential (about 0.4 Hz) |
| **KNOB 2** | CDP | depth, % (47 %) |

## Master chain

- **SPEAKER EQ** (MENU): FLAT, LOWCUT (12 dB/oct high-pass at about 110 Hz) and BASS+ (a lift at about 220 Hz with bass harmonics for the small speaker).
- **Limiter and soft clip** protect the output; the metronome follows MASTER and comes after PERFORM.
- **USB LEVEL** (MENU): MASTER (USB follows the MASTER knob) or FIXED (USB at full level, the speaker and headphones follow MASTER).

## PERFORM effects

The live master effects (REPEAT, REVERSE, TAPE STOP, filter sweeps, FREEZE, OCT UP and DN, CRUSH, THROW) and REPEAT STEP are described under *Playing and recording*.
