# Sequencer
<!--blurb: Steps, accents, probability, ratchets, pattern length and division, swing, MOTION and fills.-->

## The model

Each of the six tracks has its own pattern of up to **64 steps**, played on the 16 white keys in banks of 16 (bank 1 to 4). Each track also has its own **length**, **division** and **swing**, so tracks can run polyrhythms. The transport (PLAY) starts every track from its first step.

Press [[SEQ]] for the sequencer. It has one page, **PATTERN**. On PATTERN (and on TRACKS when no track is armed, and anywhere in STEPS mode) the white keys are the selected track's steps. [[ALGORITHM]] picks the track.

## Steps: tapping in a pattern

- **Tap** a white key: the step toggles on or off, on the release.
- **Hold** a step (0.3 s): an off step turns on at once and the step can be edited; an on step stays on. While held:

| Knob | Sets | Values |
| --- | --- | --- |
| **KNOB 1** | PROB, the chance the step plays | 0 %, 5 % to 100 % in 5 % steps; **1-SHOT** (plays once after PLAY); **1/2 ... 8/8** (plays on loop A of every B: 1/2 is every other loop, 3/4 three loops of four, and so on); **FILL** (only during a fill); **!FILL** (only outside a fill) |
| **KNOB 2** | RATCH, ratchet repeats inside the step (ignored while the track's ARP / RTRG MODE is on) | 1 to 4 |
| **KNOB 3** | ACC, accent | turn right: on, left: off |
| **KNOB 4** | NUDGE | -50 to +50 hundredths of a step |

- **Accent with a tap:** hold [[OCT+]] and tap a step: the step is accented (an off step turns on accented), or plain again. While [[OCT+]] is held the keys show the bank's accents.
- **The keys' lights:** a trig lit, a step with a parameter or machine lock blinking; while playing the selected track's step (the running cursor) toggles its key; black 1 to 6 light the selected track and breathe for armed ones (see *Panel, Step LEDs*).
- **Banks:** [[OCT+]] alone (on its release) goes to the next bank, [[OCT-]] to the previous.
- **Clear:** hold [[REC]] + [[SEL]] together for 5 s (the screen counts `CLEAR T3` down from 5; let go early and nothing changes). It clears the selected track's steps, its MOTION events and its parameter locks (`CLEARED T3`). An off step's first edit turns it on; a held key beyond LEN, or on a Grids track, takes the turn and does nothing.
- **Where the held-step knobs work:** PATTERN and TRACKS (with no track armed). Under ENV STEPS on other pages the same hold plus a knob is a parameter lock.

## ARP and RETRIG

Each track has a MODE: **OFF**, **RTRG** (retrig), **ARP1** or **ARP2**, one at a time. Open its page with [[ARP]] + [[EDIT]] (hold ARP, press EDIT; press EDIT again for page 2), or turn EDIT's pages to the end: page 1 is MODE, RATE, GATE, FADE; page 2 is DIR, CHORD, RANGE.

| Knob | What it does | Values |
| --- | --- | --- |
| **MODE** | OFF, RTRG, ARP1 or ARP2. A drum model offers OFF / RTRG only | OFF RTRG ARP1 ARP2 |
| **RATE** | the time between repeats | 1/4 4T 1/8 8T 1/16 16T 1/32 (T: triplet) |
| **GATE** | a repeat's length, for the models with a gate (ACID, SAMPLE with SUS) | 0 to 100 % |
| **FADE** | each repeat's velocity: below 0 fades down (to silence at -64), above 0 rises from quiet to the hit's, 0 flat | -64 to +63 |
| **DIR** | ARP1's and ARP2's order | UP DOWN UPDN RND |
| **CHORD** | ARP1's notes above the step's note (ARP2 ignores it; a CHORD track uses the machine's own shape instead) | OCT 5TH MAJ MIN SUS4 MAJ7 MIN7 DOM7 |
| **RANGE** | ARP1's and ARP2's octaves | 1 to 4 |

- **RTRG** repeats each hit of the track at RATE from the hit until its step ends (on through TIE steps after it) or the next hit, whichever comes first. A rest, a skipped step, REPEAT STEP or a restart ends it.
- **ARP1** plays the step's note spread by CHORD over RANGE octaves in DIR's order, one note every RATE, for as long as RTRG would. RND picks at random, the same notes after every PLAY. ARP1 and ARP2 are for the models that play notes: ACID, SAMPLE, FM, SKIN and CHORD. On a **CHORD** track RTRG repeats the whole chord, ARP1 plays the chord's own notes (its CHORD shape and INV; the ARP page's CHORD is ignored) one at a time, and ARP2 plays a whole chord on each held key. On the drums MODE stops at RTRG, and an ARP1 / ARP2 that a drum inherits (a model change, a lock, an older project's ARP) plays as RTRG. A project saved with the old **ARP** opens as ARP1.
- **ARP2** is the classic arpeggiator over the keys you hold in KEYS (see below). A sequenced step has one note, so on a step ARP2 plays as ARP1.
- **Live in KEYS**: the selected track's MODE also applies to the keys. RTRG repeats the held key at RATE (with GATE and FADE) until you let go. ARP1 arpeggiates the held key's note; another key (mono, legato as always) restarts from the new note. ARP2 arpeggiates all held keys (up to 8), sorted by pitch, in DIR's order over RANGE octaves; adding or releasing a key changes the notes without restarting the timing, and letting go of the last key stops it (the last note's gate ends normally). The timing follows the tempo and the track's swing: while playing it is on the track's own steps, while stopped it runs free at the tempo from the moment you press. Letting go, STOP, selecting another track or leaving KEYS ends it. The repeats are not sent to MIDI OUT (the keys never are), and recording records the note you press, not the repeats: the step's MODE plays them back.
- The repeats stay on the step's grid. With swing, a long step's repeats are spread over it and a short step's are squeezed. With FIT divisions they follow the step's own length, so nothing drifts at any tempo or clock.
- While MODE is on, a step's RATCH is ignored: the roll replaces the ratchet. With MODE OFF, RATCH works as before.
- All seven are track parameters, so a step can **lock** them: in STEPS mode hold a step and turn the knob. MODE can be locked too, so one step can roll while the rest of the track stays OFF (or the other way around). They are saved with the project.

## Fills

A step with PROB **FILL** plays only while a fill is active; **!FILL** plays only outside one. Hold [[REC]] (0.4 s) on any page other than SEQ's to play a fill while you hold it (`FILL` in the top bar). A shorter [[REC]] press arms as before. A learned REC and the SMC-Mixer's REC are always taps.

## PATTERN: length, division, swing, source

| Knob | Label | Values |
| --- | --- | --- |
| **KNOB 1** | LEN | 1 to 64 steps (default 16) |
| **KNOB 2** | DIV | 4BAR, 2BAR, 1/1, 1/2, 1/4, 1/8, 8T, 1/16 (default), 16T, 1/32, then FIT: F4BR, F2BR, FBAR, F1/2 |
| **KNOB 3** | SWG | 0 to 100 %. The effective swing is this plus the global SWING (HOME's [[KNOB 2]]), capped at 100 |
| **KNOB 4** | MLEN | MASTER LENGTH: OFF (default) or 2 to 1024 sixteenths, the slot's (not a track's) |

TRACKS' LEN knob is the same length.

- **FIT** divisions spread the track's LEN steps evenly over a fixed span: F4BR 16 beats, F2BR 8, FBAR 4, F1/2 2. LEN 5 on FBAR plays 5 steps in exactly one bar, with no drift against the other tracks; changing LEN while it plays keeps it fitted.
- **MLEN** restarts every track at its first step every MLEN sixteenths since PLAY, whatever its LEN, so polymeters line up again (64 = every 4 bars). A cue that ends on the same boundary makes a single restart.

## Motion: recorded knob turns

Knob turns are recorded per step while a track is armed and playing, and play back with the pattern. Motion has no page of its own and no PLAY switch on the panel; the track clear (hold [[REC]] + [[SEL]] 5 s) clears it together with the track's steps and locks. There is no separate CLEAR MOTION.

Up to **1024 events** per project are shared by all tracks and by parameter locks (`MOTION FULL`, `LOCKS FULL`). Fader and knob moves from the SMC-Mixer and MIDI LEARN are recorded as motion only where stated in their sections. LFO RATE, MORPH and DEPTH knobs are recordable; SRC and DEST are not.

## Tempo, swing, clock

- **BPM** 40 to 240 (default 120): on TRACKS [[SELECT]] sets it (a `TEMPO` popup shows); on HOME [[KNOB 1]] sets it (shown large as `BPM 120`).
- **SWING** (HOME's [[KNOB 2]]) is global swing, 0 to 100 %; each track adds its own.
- When the clock is external, the tempo controls say `CLK USB`, `CLK TRS`, `CLK BLE` or `CLK LINK` instead.
- HOME's status line shows the pattern (1 to 4: A to D), the playing variation (1 to 8), the selected track and its step page (`P1 V2 T6 L1`).

## Live cue and variations

While a pattern plays you can queue the next pattern (A to D) or another variation, and it comes in exactly where the pattern ends (its longest track's), every track from its first step, in time.

- **Hold GLO** and press white key 1 to 4 to queue pattern A to D (whites 5 to 16: nothing), or black key 1 to 8 (F#3 G#3 A#3 C#4 D#4 F#4 G#4 A#4) to queue variation 1 to 8 of the playing pattern (of the queued pattern, if one is queued), on any page. Black 9 / 10 (C#5 / D#5) do nothing with GLO. While GLO is held: the playing pattern's white key is lit, a queued one blinks; on black 1 to 8 the playing variation is lit, a queued one blinks, the other used variations are dim and unused ones dark (dim shows at every BACKLIGHT level). Released after a key, GLO opens no page. (A black key alone no longer queues a variation.)
- **Hold SAVE** (0.4 s, or press a key with it) for the **cue map**: white keys 1 to 4 queue patterns A to D; black keys queue variations 1 to 8 (lit: used, winking: playing, blinking: queued, breathing: unsaved edits). A pattern key then a variation key queues both (`CUE>B3` on top). The same key again switches at the next step; the playing pattern's key cancels; stopped, a cue switches at once; STOP drops a queued cue. Keys pressed with SAVE play no note, send no MIDI and record nothing.
- A **pattern** switch brings the pattern's sounds, steps, timing and locks; the tempo, swing, clock, MIDI, master effects, mutes and selected track stay. The old sounds fade out over the last 5 ms. A **variation** switch changes the steps and timing only; the sounds ring on.
- Each pattern has **8 variations** (both versions): variation 1 is its own steps; 2 to 8 hold their own steps and timing and share the pattern's sounds and locks. A variation never used before starts as a copy of the one playing. A pattern comes back on the variation it played last. When a pattern's variations are full: `VARIATIONS FULL`.
- Following an external clock (USB or TRS) the switch comes on the pulse that ends the pattern.
- **Chain:** up to 16 entries, each a pattern and one of its variations, play in turn, each switching at the pattern end (or MLEN's boundary), wrapping at the end; entries of empty patterns are skipped. HOME shows the chain under its status line when it has more than one entry, as `A1 B3 C1` (pattern letter, variation number; up to 6 at a time, playing inverted, next underlined). STOP ends the chain (the pattern playing stays). Set it with **GLO held + 2 or more white keys**: each white key adds an entry (pattern A to D); a black key 1 to 8 pressed after it sets that entry's variation. With no black key the entry takes the pattern's current variation: the playing one if that pattern is playing, else the one it played last (as a plain cue would). On GLO's release the chain is those entries in the order pressed (repeats allowed, up to 16; white keys past D are left out). Example: GLO held, A, black 1, B, black 3, A, black 2, release: `A1 B3 A2`. GLO + a single white key (optionally then a black key) is a plain cue of that pattern (and variation) and ends the chain; a black key before any white key in the hold is a plain variation cue (it ends a running chain too). The chain lives in RAM only. The console also sets it: `chain 1 1 3 2` (patterns A to D as 1 to 4, current variation) or `chain 11 23 12` (pattern, variation: A1 B3 A2); `chain` alone clears it.

## NUDGE: a step early or late

On PATTERN or TRACKS hold a step and turn [[KNOB 4]] (NUDGE): -50 to +50, the hit moved by that many hundredths of a step (+50 is halfway to the next step, -50 halfway to the previous one, 0 on the step). It follows the track's division and swing (+25 is a quarter of the step as it plays).

- A late hit keeps its PROB, RATCH and locks (its ratchet hits spread over what is left of the step). An early hit plays in the step before, its locks from the hit to the end of its own step.
- A tap that turns a step off makes it plain again.
- NUDGE is part of the step: copied, saved and played everywhere (variations, live cue, slot COPY / PASTE). All steps start at 0 and play exactly as before.

## QUANT: where live recording lands

**REC QUANT** in the device menu (hold [[HOME]]) decides where a live-recorded hit goes (keys, MIDI notes and the SMC-Mixer). It is a device setting, kept over power-off.

| QUANT | The hit goes to |
| --- | --- |
| **1/16** (default) | the nearest step |
| **1/8** | the nearest even step |
| **1/32** | the nearest half step (NUDGE 0 or +-50) |
| **OFF** | the nearest step, keeping how early or late you played as its NUDGE (up to +-50) |

## Copy, paste and clear: button combos

There is no TOOLS screen (Elektron Model:Cycles style, no screen, only a short message). Hold the first button and press the second:

| Combo | What it does |
| --- | --- |
| [[SAVE]] + [[REC]] | copy the selected track (`COPIED T3`) |
| [[SAVE]] + [[PLAY]] | paste it onto the selected track (`PASTED T5`, `NO TRACK COPIED`) |
| [[GLO]] + [[REC]] | copy the pattern, all six tracks (`COPIED PATTERN`) |
| [[GLO]] + [[PLAY]] | paste it over the pattern playing (`PASTED PATTERN`, `NO PATTERN COPIED`) |
| step(s) held + [[REC]] | copy steps of the selected track: one, or the lowest to the highest held (`COPIED 3-8`, `COPIED 5`) |
| step(s) held + [[PLAY]] | paste them onto the selected track: from the one held step, or over the held range, repeated / cut (`PASTED 9-14`, `NO STEPS COPIED`) |
| [[REC]] + [[SEL]] held 5 s | clear the selected track (`CLEAR T3` counting 5 to 1, then `CLEARED T3`) |

A copy takes the pattern and variation playing: every step whole (accent, PROB, ratchet, note, slide and tie, its NUDGE), its parameter locks, and the track's LEN, DIV and SWG. A paste replaces the target's steps, locks and LEN / DIV / SWG; the track's motion and its sound stay. With the locks full a paste brings what fits and says `LOCKS FULL`. The clipboard holds one track or one pattern (a new copy replaces it) and is kept until power-off, across projects, so a track can go to another track, pattern, variation or project. There is no undo.

The step combos work in the step views (where the white keys are the selected track's steps), within the bank shown. Hold the step key first, then press REC or PLAY; a range is the lowest to the highest of the step keys held (hold step 3, add step 8: steps 3 to 8). A copied step goes whole, with its parameter locks and machine locks. A paste from one held step lays the whole copy from there and drops what falls past LEN; with two or more held, it fills exactly that range, repeating a short copy or cutting a long one. The target steps' old locks go. Steps share the clipboard with the track and pattern copies (a step copy replaces a copied track and the other way round). The held keys toggle nothing on their release, and an off step that a long hold had lit goes back off before the copy.

None of these buttons does its own job in the combo: no transport start / stop, no REC ALL, fill or arm, no projects page or cue map, no GLOBAL page, no track select or KEYS toggle. The clear works in either order (REC or SEL first); let either go before the countdown ends and nothing changes.

The bank copy, REPLACE / MERGE / FILL, COPY SEL / PASTE SEL and DUPLICATE are gone with the TOOLS screen. Holding SEQ while switching on is still the safe start.
