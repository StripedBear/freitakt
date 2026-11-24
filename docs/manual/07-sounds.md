# Sounds
<!--blurb: The 46 sound models, their knobs, presets and samples.-->

## How a track's sound works

Press [[EDIT]] to open the selected track's sound. **SOUND 1**'s [[KNOB 1]] picks the **MODEL**; the SOUND pages are lists of four rows: SOUND 1 is MODEL, **NOTE**, PITCH and the model's next knob (ACID, SAMPL and CHORD: MODEL, NOTE, **NLEN**, PITCH), the next pages the rest of the model's knobs (pages with nothing on them are skipped), followed by LEVEL, PAN, MNOTE, CHOKE and PRIO (SAMPL adds a fifth page with PLAY CUT LOOP). The envelope knobs are not on SOUND: they are on the **ENV** page (below). Every model has **PITCH** (-24 to +24 semitones, shown with its sign: the machine's tuning, lockable; it was called TUNE before), **DECAY** (on ENV), and then its own knobs (all 0 to 127 unless stated). K909, for one: MODEL NOTE PITCH SWEEP, then CLICK SWPT DRIVE LVL, then PAN MNOTE CHOKE PRIO. ARP (the arpeggiator and retrig) is not in EDIT's cycle: it has its own button, [[ARP]].

**NLEN** (note length, as Model:Cycles' LEN; [[KNOB 3]] on SOUND 1 of ACID, SAMPL and CHORD, the machines with a gate; drums have none and do not show it) is how long a hit's note holds, in steps of the track (its DIV, FIT and swing): `1 2 3 4 6 8 12 16 24 32 48 64` or `INF` (held until the track's next hit or STOP). 1, the default, is the old one-step note. ACID holds the note through the next steps and closes it with GATE in the last one; SAMPL (SUS above 0) and CHORD hold at the SUS level, then release. A new hit on the track ends the note; SLIDE and TIE steps work as before; a roll (ARP / RTRG) and a RATCH step play their own short hits. NLEN takes parameter locks (hold a step, turn the knob), MOTION and MIDI LEARN, sets MIDI OUT's Note Off and is saved in the project.

## ENV: the envelope page

Press [[ENV]] to open the selected track's envelope: its envelope knobs gathered on one list page (same knob popup, locks, MOTION and MIDI LEARN as any page; nothing about the sound changed, only where the knobs are). [[ENV]] again goes to the next ENV page if there is one; the ENV LED is lit on it.

| Machine | ENV 1 | ENV 2 |
| --- | --- | --- |
| a drum / FM / TRX / ACID model | DECAY, FDEC | - |
| SAMPL | ATK, DECAY, SUS, REL | FDEC |
| CHORD | ATTACK, DECAY, FDEC | - |

**FDEC** is the filter's DECAY (5 ms to 2 s, default 40, about 32 ms), the time of the filter envelope that FILTER's ENV knob sweeps the cutoff with. Changing MODEL while on ENV 2 goes back to ENV 1 if the new machine has no second page.

The SOUND pages are lists, one row per knob. The MODEL row shows the track's preset or model name, with `*` once a SOUND knob has moved. At the top each SOUND page shows *the hit*: a small waveform of the voice's real sound (one hit at velocity 100, played silently in the background); it redraws a few frames after MODEL or a knob changes.

Per-track extras on the SOUND pages:

| Parameter | Meaning |
| --- | --- |
| **LEVEL / PAN** | the track's mix level and position (also on TRACKS). |
| **NOTE** | SOUND 1's [[KNOB 2]], shown as a note name (`C#4`, C-1 to G9). With no step held: the track's NOTE, the note a step you turn on takes (drums play it transposed from C4). With steps held (ENV STEPS): their own notes, all at once. Not a sound knob: no lock, no motion, not in presets, not MIDI-learnable; [[ALGORITHM]] does the same with KEYS off (see *Sequencer, Steps*). |
| **MNOTE** | the MIDI note that plays this track (see ROUT NOTE); NOTE before this firmware. |
| **CHOKE** | choke group: a hit in a group cuts the others in it (closed hat cuts open hat). Never changed by a MODEL change or a preset; no model starts with a choke group. |
| **PRIO** | HIGH (default) or LOW: a LOW track loses its voices first when the CPU is overloaded. |
| **VOICES** | the model's own polyphony (1 or 2). |

## Models

The MODEL list has three banks. Display names changed in 0.23.0 (projects, presets and MIDI are unchanged); the old names are in parentheses.

### MD bank (21): a classic digital drum machine, measured

Written from the measured behaviour of a classic digital drum machine's emulator, knob by knob; no code or data of it is used. TRX models are digital, EFM models are FM. **PITCH 0 is the original machine's PTCH 64.**

| # | Model | What it is | Knobs, in order (SOUND 1 holds MODEL, NOTE and the first two) | Voices |
| --- | --- | --- | --- | --- |
| 1 | BDTRX | Sine kick with a pitch drop, transient, half-harmonic SUB, hard CLIP. DECAY runs 7 ms to 4 s. | PITCH DECAY SWEEP SDEC / PUNCH NOISE SUB CLIP | 1 |
| 2 | SDTRX | Snare: two tones crossfaded by TONE, a fixed noise burst; SNAP above 64 adds a 1.44 kHz ring. | PITCH DECAY SNAP TONE / BUMP BENV CLIP | 1 |
| 3 | TMTRX | Tom: a click, then a sine whose RAMP pitch falls over RDEC. | PITCH DECAY RAMP RDEC / DAMP DIST DTYP | 1 |
| 4 | CGTRX | Conga: the TMTRX engine with conga settings. | PITCH DECAY RAMP RDEC / DAMP DIST DTYP | 1 |
| 5 | CPTRX | Clap: four noise bursts (RATE spacing, CLPY level), a resonant low-pass HARD, and a ROOM noise tail (RSIZ length, RTUN colour). | PITCH DECAY CLPY HARD / RATE ROOM RSIZ RTUN | 1 |
| 6 | RSTRX | Rim shot: a sine and a click; DIST warps the sine. | PITCH DECAY DIST | 1 |
| 7 | CBTRX | Cowbell: two tones, ENH a resonator and grit, DAMP a narrow band-pass. | PITCH DECAY ENH DAMP / TONE BUMP PTCH | 1 |
| 8 | CHTRX | Closed hat: MTAL goes from noise to metal; DECAY is a hold, GAP the decay after it. | PITCH DECAY GAP HPF / LPF MTAL | 1 |
| 9 | OHTRX | Open hat: the same engine with a long hold that dips to silence and comes back. | PITCH DECAY GAP HPF / LPF MTAL | 1 |
| 10 | CYTRX | Cymbal: RICH oscillator balance, PEAK a 5.2 kHz bell, SIZE the body, TOP / TTUN a brighter top that fades first. | PITCH DECAY RICH TOP / TTUN SIZE PEAK | 2 |
| 11 | SHTRX | Maracas / shaker: grainy noise, RATL a rattle that grows through the hit, ATT attack, REV a dip and second hold. | PITCH DECAY ATT REV / DAMP RATL TONE HARD | 1 |
| 12 | CLTRX | Claves: two pure tones, DUAL delays the first, ENH raises it, CLICK a short click. | PITCH DECAY DUAL ENH / TUNE2 CLICK | 1 |
| 13 | B2TRX | Second bass drum: about 50 Hz sine from phase 0, RAMP added pitch, HOLD full level before DECAY, TICK click, NOISE hiss, DIRT coarse quantiser, DIST hard clip. | PITCH DECAY RAMP HOLD / TICK NOISE DIRT DIST | 1 |
| 14 | BDEFM | FM bass drum. RAMP sets where the pitch starts, RDEC how fast it returns; MOD FM depth falling over MDEC; MFRQ modulator pitch; MFB grit from about 40, noise at 127. Ends at 0.58 s. | PITCH DECAY RAMP RDEC / MOD MFRQ MDEC MFB | 1 |
| 15 | SDEFM | FM snare: a tone modulated by noise; NOIS / NDEC a separate random noise burst; HPF a high-pass. | PITCH DECAY NOIS NDEC / MOD MFRQ MDEC HPF | 1 |
| 16 | TMEFM | FM tom (an approximation of its kind of grit; exact at MOD 0). Ends at 0.58 s. | PITCH DECAY RAMP RDEC / MOD MFRQ MDEC CLIC | 1 |
| 17 | CPEFM | FM clap: the same FM restarted CLPS / 8 times at CDEC's spacing (1 to 41 ms), then DECAY. | PITCH DECAY CLPS CDEC / MOD MFRQ MDEC HPF | 1 |
| 18 | RSEFM | FM rim shot plus a second "snare" voice: SNAR its level (0 = off), SPTC pitch. | PITCH DECAY MOD HPF / SNAR SPTC SDEC SMOD | 1 |
| 19 | CBEFM | FM cowbell: two FM tones, FB feedback, SNAP a fast 4.5 ms extra decay. | PITCH DECAY SNAP FB / MOD MFRQ MDEC | 1 |
| 20 | HHEFM | FM hat: a sizzling tone, FB from a stalled tone to chaos, a tremolo restarting at each hit (TREM, TFRQ). | PITCH DECAY TREM TFRQ / MOD MFRQ MDEC FB | 1 |
| 21 | CYEFM | FM cymbal: four FM pairs, MDEC from 0.28 to 11 s, HPF two high-passes (off at 0). | PITCH DECAY FB HPF / MOD MFRQ MDEC | 2 |

Former names: BDTRX = KTRX, TMTRX = XTTRX, CGTRX = XCTRX, SHTRX = MATRX, TMEFM = XTEFM. A hit of the CPTRX, SHTRX and the EFM snare/clap/rim models sounds the same every time except where noted (random noise bursts). DECAY on the EFM models other than BDEFM and TMEFM follows the machine's whole 1.6 s range and fades out from 5.5 s at the top.

### MODERN bank (11)

Integer ports of Mutable Instruments Plaits drum algorithms (MIT), DaisySP physical models, an FM voice, ACID, a chord machine and a sample player. Models marked *heavy* count double toward the voice cap.

| # | Model | What it is | Knobs | Voices |
| --- | --- | --- | --- | --- |
| 22 | BOOM | Analog bass drum: pulse into resonator with overdrive (heavy). Was KBOOM. | PITCH DECAY TONE PUNCH | 1 |
| 23 | PUNCH | Synthetic bass drum: a distorted sine with FM and pitch envelopes (heavy). Was KPUNC. | PITCH DECAY TONE FM | 1 |
| 24 | SNAP | Analog snare: five shell modes, pulse exciter, band-passed noise (heavy). Was SSNAP. | PITCH DECAY TONE SNAP | 1 |
| 25 | CRACK | Synthetic snare: two distorted FM sines plus noise (heavy). Was SCRAK. | PITCH DECAY FM SNAP | 1 |
| 26 | METAL | 808-like metallic hat: six square oscillators and a resonant band-pass. Was HMETL. | PITCH DECAY TONE NOISE | 1 |
| 27 | HISS | Noisy trashy hat: ring-modulated square and saw pairs. Was HNOIS. | PITCH DECAY TONE NOISE | 1 |
| 28 | SKIN | A struck drum head (modal physical model): HEAD from ideal to loaded (toms and timpani to tabla), POS centre to rim, BEND, STICK (heavy). Was MEMB. | PITCH DECAY TONE HEAD / POS BEND STICK | 1 |
| 29 | FM | Two-operator FM percussion: a carrier phase-modulated by a sine at RATIO x pitch. MDEC the FM envelope decay, SWEEP a pitch rise, FBK modulator feedback, VEL velocity into INDEX. Bells, metallic toms, woodblocks, zaps, FM kicks. | PITCH DECAY INDEX RATIO / MDEC SWEEP FBK VEL | 2 |
| 30 | ACID | 303-style mono bass: saw or square into a 24 dB ladder filter, with accent, slide and tie from the steps. Max two ACID tracks. | PITCH DECAY CUTOF RESO / ENVMD ACCNT WAVE GATE | 1 |
| 31 | CHORD | Polyphonic chord (as Elektron Model:Cycles' CHORD): up to four oscillators playing a chord shape on the note, one chord at a time over all tracks (see below). | NLEN PITCH CHORD INV / DECAY WAVE TONE ATTACK DETUNE | 1 |
| 32 | SAMPL | Sample player: one sample of a set, with start / end, envelope, loop and chop (see Samples). Was SMPL. | PITCH DECAY TONE DRIVE / SET SLOT START END (+ PLAY CUT LOOP; ATK SUS REL on ENV) | 2 |

Lists: FM **RATIO** is one of 0.5, 1, 1.41, 1.5, 2, 2.76, 3, 3.5, 4, 5.4, 6, 7, 8, 9.2, 11, 14. ACID **WAVE** is SAW or SQR. ACID **GATE** is how long a hit that no step drives stays open, as a fraction of the step. ACID is skipped in the MODEL list and PRESETS once two ACID tracks exist (`ACID: 2 TRACKS MAX`).

### CHORD: the chord machine

CHORD plays a chord on the note: PITCH plus the step's NOTE, the key you press in KEYS, or the MIDI note (C4 is the root at PITCH 0).

| Knob | Meaning | Range |
| --- | --- | --- |
| **PITCH** | semitones, as on every model | -24 to +24 |
| **CHORD** | the shape over the root | MAJ (0 4 7), MIN (0 3 7), 7 (0 4 7 10), MIN7 (0 3 7 10), MAJ7 (0 4 7 11), SUS2 (0 2 7), SUS4 (0 5 7), DIM (0 3 6), 6 (0 4 7 9), 9 (0 4 10 14: root, 3rd, 7th, 9th; the 5th is dropped to fit four notes), OCT (0 12), 5TH (0 7) |
| **INV** | inversion: each step moves the lowest note up an octave. INV 1 of MAJ on C is E G C; a three-note shape at INV 3, or a two-note one at INV 2, is the original an octave up | 0 to 3 |
| **DECAY** | the release after the gate closes, 5 ms to about 4 s | 0 to 127 |
| **WAVE** | the oscillators: SAW, SQR (band-limited) or SIN | SAW SQR SIN |
| **TONE** | a gentle low-pass on the chord, about 100 Hz at 0 to fully open at 127 | 0 to 127 |
| **ATTACK** | the fade-in, 1 ms to 2 s. The attack always runs to its end, even after a short step: a pad on a short step swells, then decays | 0 to 127 |
| **DETUNE** | spreads the chord's notes slightly against the root (up to about 30 cents on the outer notes): a wider, chorused chord | 0 to 127 |

- **The gate.** As SAMPL with SUS: the chord holds at full level for NLEN steps (1 by default; a roll's hit: GATE of its repeat), on through TIE steps, or as long as you hold the key, pad or MIDI note; then DECAY releases it.
- **One chord at a time.** There is a single chord engine for the whole machine. A new CHORD hit from any track (its MODEL, or a step's machine lock to CHORD) replaces the chord that is sounding, with a short fade, never a click. The chord plays through the track that hit last: its LEVEL, PAN, FILTER, INSERT and FX sends. Other tracks play normally alongside, so CHORD costs at most one four-note chord of CPU (about as much as one ACID voice).
- **Locks.** CHORD's knobs take parameter locks like any model's (a different shape or inversion per step), and a step can machine-lock any track to CHORD.
- **MODE** (ARP page): RTRG repeats the whole chord. ARP1 plays the chord's notes one by one, a note every RATE, in DIR's order over RANGE octaves; it uses CHORD's own shape and INV, so the ARP page's CHORD knob is ignored on a CHORD track. ARP2 over held KEYS plays a whole chord on each held key in turn.
- **MIDI OUT** sends the chord's notes (up to four, together) on the track's channel; with ARP1 one note a hit.

### CLASSIC bank (14): 808 and 909 style

| # | Model | What it is | Knobs | Voices |
| --- | --- | --- | --- | --- |
| 33 | K808 | 808 kick: a sine with a pitch dip. | PITCH DECAY TONE DIP / DRIVE | 1 |
| 34 | K909 | 909 kick. | PITCH DECAY SWEEP CLICK / SWPT DRIVE | 1 |
| 35 | S808 | 808 snare: two tones plus band-passed noise. | PITCH DECAY TONE SNAP / NTONE | 1 |
| 36 | S909 | 909 snare. | PITCH DECAY BODY SNAP / NTONE | 1 |
| 37 | C808 | 808 clap. | PITCH DECAY TONE TAIL | 1 |
| 38 | C909 | 909 clap. | PITCH DECAY TONE SPRD | 1 |
| 39 | HATC | Closed 808 metal hat. | PITCH DECAY TONE BRITE | 1 |
| 40 | HATO | Open hat. | PITCH DECAY TONE BRITE | 1 |
| 41 | CYMB | Cymbal. | PITCH DECAY TONE BAL | 2 |
| 42 | COWB | Cowbell; PITCH moves its pitch (since 0.14). | PITCH DECAY TONE TAIL | 1 |
| 43 | TOM | Tom. | PITCH DECAY NOISE DROP | 2 |
| 44 | CONGA | Conga. | PITCH DECAY SLAP DROP | 2 |
| 45 | RIM | Rim shot: 1667 and 455 Hz, driven, high-passed. | PITCH DECAY TONE DRIVE | 1 |
| 46 | CLAVE | Claves. | PITCH DECAY CLICK DROP | 1 |

DRIVE on K808, K909, BOOM, PUNCH and SAMPL is shown as a percentage.

## The power-on kit

At power-on, on TOOLS **INIT ALL** and in a new project the six tracks load the TRX classic kit, each on its default preset: **BDTRX, SDTRX, CPTRX, CHTRX, OHTRX, TMTRX** (Hortator's RSTRX and CYTRX tracks are gone), with MIDI notes kick 36, snare 38, clap 39, closed hat 42, open hat 46, tom 45, and no choke.

## Presets

A **preset** is one voice: MODEL plus the eight SOUND knobs (the SAMPLE model's PLAY, CUT, ATK, SUS, REL and LOOP too) and a name. A **USER** preset also stores the track's **FX**: the DIST, CHO, DLY and REV sends, FILTER, INSERT and SRR. No preset stores LEVEL, PAN, NOTE, CHOKE, PRIO, the LFOs or steps. Factory presets change only the sound; the track's FX stay.

| Bank | Presets |
| --- | --- |
| MACHINEDRUM | 55, from the original machine's factory kits (1 to 4 per voice; none for SHTRX and CLTRX) |
| MODERN | 45 (4 per voice, ACID and SAMPL included; 5 for CHORD) |
| CLASSIC | 56 (4 per voice) |
| USER | 96 slots for yours, stored in flash; each keeps the sound and the track's FX |

156 factory presets in all. Every voice's **first preset is its default sound**: a MODEL change loads it.

- **PRESETS encoder.** Each detent loads the next / previous preset onto the selected track, through every bank with wrap-around. The page gives way to a full screen of its own: the bank tab and the track on top, the preset large on an inverted bar in the middle, three presets before and after it (machine names on the right), a dotted rule where a bank ends. The page returns 3 s after the last detent; another track, the PRESETS list, a menu or a knob that opens its own value screen closes it at once. Empty USER slots, and ACID beyond two tracks, are skipped.
- **The PRESETS list.** Hold [[EDIT]] about 0.4 s (a tap opens EDIT as before). Seven presets on screen. [[KNOB 1]] or PRESETS move the cursor; [[KNOB 2]] or [[SELECT]] pick the bank (MD, MODERN, CLASSIC, USER); every move auditions the preset on the selected track. [[OCT+]] **LOAD** keeps it and closes the list; [[EDIT]] or [[HOME]] closes it and restores the track's own sound. **SAVE** stores the track's own sound and its FX (not the auditioned one) in the first free USER slot or the chosen one (a used slot: SAVE twice); [[OCT-]] twice **DELETE**s a USER preset. USER presets are named from the voice and slot (`SDTRX 03`). Saved while playing, they are written at the next STOP. While MIDI LEARN is on, EDIT held does nothing.
- **Display.** SOUND 1's MODEL row (the machine, then the preset's name) and the track's row on TRACKS show the preset's name, with `*` once a SOUND knob moved. A track whose USER slot now holds another voice (or none) shows `-`. Projects keep each track's preset and its mark; older projects load with no preset and their sound unchanged. SAVE while MOTION plays stores the knobs as set, not MOTION's values.

## Samples

SAMPL plays one sample of a **set**. **SET** chooses the set: **PERC** (built in) or **USR1**, **USR2**, **USR3** (yours). **SLOT** steps through the set's samples, shown `3/16` (`--` for an empty set); each plays at its own pitch, and **PITCH** (-24 to +24) transposes it. Turning SET keeps SLOT inside the new set. **START** and **END** (0 to 127 over the sample) play only part of it. **DECAY**, **TONE** (a one-pole low-pass) and **DRIVE** (a soft clip) shape it. An empty set is silent.

The SAMPLE model has five SOUND pages: 1 MODEL NOTE PITCH TONE; 2 DRIVE SET SLOT START; 3 END LVL PAN MNOTE; 4 CHOKE PRIO; 5 PLAY CUT LOOP. Its ATK DECAY SUS REL are on the ENV page.

- **ATK SUS REL.** ATK fades the hit in (0 at once, up to 2 s). SUS 0 plays the sample once with DECAY; above 0, DECAY falls only to that level and the sound holds while the note is held, then REL fades it out. A step holds for one step of its track, longer through TIE steps; a KEYS key, a track's key or a MIDI note holds until you let go.
- **LOOP** plays the part between START and END again and again: **FWD** (crossfaded seam), **PING** (forward and back; loops up to about a quarter of a second at 44.1 kHz, longer ones play FWD), **LOOPI** (loops until the track's next hit, a choke or STOP; the end of the note does nothing) or **ONE** (START to END whatever the note's length). OFF is the one-shot.
- **PLAY KEY** is the default: the keys play the SLOT's sample up and down. **PLAY CHOP**: the 16 white keys play samples 1 to 16 on any octave, each at its own pitch (PITCH shifts all); the black keys and keys past the set play nothing; the screen shows `CHOP 5/12`. A step stores its sample as its note; a MIDI note picks a sample too (C4 is sample 1, C#4 sample 2, again every 16 notes).
- **CUT ON** makes each hit cut the one still sounding (chops included); OFF lets them overlap.
- ATK, SUS, REL, LOOP, PLAY, START, END and SLOT take MOTION, step locks, MIDI LEARN and LFO DEST; CUT takes MIDI LEARN. All are saved with projects and in USER presets; a factory SAMPLE preset sets ATK 0, SUS 0, REL 40, LOOP OFF, PLAY KEY. With ATK 0, SUS 0 and LOOP OFF a hit sounds as it always did.
- **PERC** has 16 samples, in this order: 1 kick, 2 rim, 3 snare, 4 clap, 5 low tom, 6 closed hat, 7 open hat, 8 high tom, 9 crash, 10 ride, 11 tambourine, 12 cowbell, 13 conga, 14 shaker, 15 claves, 16 wood block. They are Hortator's generated drum samples (Hügelton Sample Pack) and CC0 hand percussion (Versilian Studios VSCO-2 CE and VCSL), stored as 22,050 Hz IMA ADPCM.
- **User slots.** USR1 holds 80 KiB, USR2 and USR3 48 KiB each (the rest of their flash holds the projects' variations and lock banks). Each holds a set of up to 16 samples, mono IMA ADPCM at rates up to 44.1 kHz. USR1 holds about 3.7 s at 44.1 kHz, 7.4 s at 22.05 kHz or 14.8 s at 11.025 kHz; USR2 and USR3 about 2.2, 4.4 and 8.8 s. Load them with the **web editor's Samples tab**, which also trims, normalizes, resamples and chops a sound.
- A longer USR2 or USR3 written under upstream Felucca is shortened once at the next power-on (`USR2 SHORTENED TO 48K`, `USR3 SHORTENED TO 48K`); if that fails (`USR2 CLAMP FAILED`, `USR3 CLAMP FAILED`), SAVE says `USR2 ERROR: NO SAVE` until the next power-on. Do not upload to USR2 or USR3 with upstream Felucca's tools: an 80 KiB upload overwrites Hortator's variations and lock banks.
