# Playing and recording
<!--blurb: KEYS, ACID, live record, REC ALL, mutes, PERFORM and parameter locks.-->

## KEYS: the keys as a keyboard

Tap [[SCL]] (labelled SEL; on its release): the 27 keys play the selected track chromatically. [[OCT-]] / [[OCT+]] set the octave, -2 to +2. Drum tracks play their sound transposed (C4 plays as tuned); ACID plays a bass line.

- With KEYS on, [[OCT-]] / [[OCT+]] are the octave. The LFO pages' RATE MODE needs KEYS off.
- On TRACKS the OCT buttons remain the bank.
- TRACKS, COMP, NAME, the menu, PERFORM (FX held), the combos (GLO, SEL, SEQ or ARP held) and the cue map (SAVE held) keep their own key jobs.
- The selected track's **MODE** (ARP page) plays the keys live: RTRG repeats the held key, ARP1 arpeggiates its note, ARP2 arpeggiates all held keys (up to 8), at RATE, on the tempo and the track's swing, playing or stopped. Let go and it stops. See *Sequencer, ARP and RETRIG*.
- KEYS notes send **no MIDI out**. An ACID note (or a tied chain) longer than 5.5 s is cut, a safety limit.
- With KEYS off (SEL unlit) the white keys are the selected track's steps on every page (see *The panel, KEYS and STEPS*).

## Notes on steps

Every step holds a note, C-1 to G9. Drum tracks play their steps transposed from C4 (C4 plays as tuned, by up to two octaves either way). Each track also has a **NOTE**, the note a step you turn on with the white keys takes, on every machine, drums included. Three things set it: a key you play in KEYS, SOUND 1's NOTE ([[KNOB 2]]) and, with KEYS off, [[ALGORITHM]], each with no step held. NOTE is C4 again after LOAD and after clearing the track's pattern.

To set a step's own note, hold the step (or several) and turn [[ALGORITHM]] with KEYS off, or SOUND 1's NOTE (KEYS off, on any page): every held step gets the note (see *Sequencer, Steps*). Live recording writes the notes you play, as before.

## ACID: the 303-style bass

ACID is a bass voice: a saw or square into a resonant 24 dB filter, with accent, slide and tie. Its knobs: **PITCH DECAY CUTOF RESO ENVMD ACCNT WAVE GATE**.

- Up to **two tracks** can be ACID (`ACID: 2 TRACKS MAX`). A loaded project keeps ACID on its first two ACID tracks; the others come back as their power-on kit drum.
- On ACID tracks a step can be a **TIE** (the note continues into the next step) or a **SLIDE** (legato glide).
- A held ACID note is the last voice to be stopped when the CPU is overloaded.

## Live recording

- **Arm one track.** On TRACKS, a [[REC]] tap arms or disarms the selected track (`T3 REC ARMED` / `T3 REC OFF`).
- **REC ALL.** On every other page a [[REC]] tap is REC ALL: every track armed (`REC ALL`, the REC LED lit). Each white track key then plays and records its own track on TRACKS; on the other pages, with KEYS off, the white keys are the selected track's steps and record no hits (play them with KEYS on, or on TRACKS). KEYS and knob motion stay the selected track's.
- **Ending REC ALL.** A [[REC]] tap off TRACKS ends it and arms again the tracks armed before (`T3 REC ARMED`, or `REC OFF` if none). On TRACKS it ends it with the selected track armed.
- **REC never starts the transport.** [[PLAY]] does; an armed track records once playing.
- **What gets written.** A key writes a step as REC QUANT says (the nearest step by default; see Sequencer). A key held into later steps writes TIEs; a legato overlap writes a SLIDE (TIE and SLIDE on ACID tracks only).
- **MIDI notes** into an armed track while playing are recorded, with their pitch on MIDI ROUTE SEL and TRACK.
- **Motion.** Knob turns on an armed, playing track are recorded as MOTION (see below).
- REC ALL is not saved in projects.

## Step record

FREITAKT has no step record. Steps are entered with the white keys (with KEYS off, on any page) and shaped by holding a step and turning a knob (PROB, RATCH, ACCENT, NUDGE, or a parameter lock); notes go in by playing them in KEYS mode on an armed track (see *Live recording*).

## Fill, clear, copy and paste

Hold [[REC]] (0.4 s) on any page to play a **fill** while you hold it (`FILL` on top).

Hold [[REC]] and [[SCL]] (SEL) together, either first, for **5 s** to clear the selected track (steps, motion and locks of the pattern playing). While both are held the whole screen warns `CLEAR T3` and counts 5, 4, 3, 2, 1; let either button go before the end and nothing is cleared. Then the message `CLEARED T3`. Neither button does its own job in this combo (no REC ALL, fill, arm, KEYS toggle or track select), and a knob popup open at that moment closes.

Copy and paste are combos too, with no screen: [[SAVE]] + [[REC]] copies the selected track, [[SAVE]] + [[PLAY]] pastes it onto the selected track; [[GLO]] + [[REC]] copies the pattern, [[GLO]] + [[PLAY]] pastes it. PLAY does not start or stop the transport there, and REC arms nothing. See *Sequencer, Copy, paste and clear*.

## Mutes, solo and track select

Black keys 1 to 6 (F#3 G#3 A#3 C#4 D#4 F#4) are tracks 1 to 6. Hold a button and press them, on any page:

| Combo | Does |
| --- | --- |
| [[SEQ]] + black 1 to 6 | toggle the mute of tracks 1 to 6 (their LEDs lit while heard, while SEQ is held) |
| [[ARP]] + black 1 to 6 | **solo** tracks 1 to 6 (several at once; their LEDs lit while soloed, while ARP is held) |
| [[ARP]] tap (no key) | the selected track's ARP page (MODE OFF / RTRG / ARP1 / ARP2, see Sequencer; the MODE also plays the KEYS live); tapped again, page 2; solo is not touched |
| [[ARP]] + F#5 | solo mode: **TOGGLE** (a press solos / unsolos) or **MOMENTARY** (soloed only while the key is held, also after ARP is let go); kept over power-off (`SOLO: TOGGLE` / `SOLO: MOMENTARY`; F#5 lit while MOMENTARY) |
| [[SCL]] (SEL) + black 1 to 6 | select track 1 to 6 (its key lit while SEL is held) |

- A SEQ hold that began with a black key turns no page; a white key with SEQ does nothing.
- **MUTE MODE** in the device menu (hold [[HOME]]) sets `NOW` or `BAR`: with BAR a mute made while playing blinks, then lands on the next downbeat. It also works while following an external clock. A muted COMP source fades out just before the bar's first hit.
- Solo itself is not saved (INIT and power-on clear it); mutes stay as they are under a solo. It is the same solo state the SMC-Mixer uses. HOME's TRACKS mode shows the mutes.
- The keys pressed with the button play nothing, send no MIDI and record nothing.

## PERFORM

Hold [[FX]] for the PERFORM screen (it shows T1 to T6 and the four repeat rates).

| Keys | While held |
| --- | --- |
| **F#3, G#3, A#3** | REPEAT 1/8, 1/16, 1/32 of the whole mix |
| **C#4, D#4, F#4, G#4, A#4** | REVERSE, TAPE STOP, LPF sweep, HPF sweep, FREEZE |
| **C#5, D#5** | OCT UP, OCT DN |
| **white F3 to D4** | **REPEAT STEP** of tracks 1 to 6 (below) |
| **D5, E5, F5, G5** | the REPEAT STEP rate: 1/4, 1/8, 1/16 (power-on default), 1/32 (a tap picks it; the chosen one is lit; not saved) |
| G4, A4, B4, C5, F#5 | nothing |

The buffer effects (repeat, reverse, tape, freeze, octave) are the last pressed one playing; letting it go returns to the one held before. Repeat and reverse start on the next 1/16. A repeat or reverse longer than the loop at the tempo does nothing (shown dimmed).

The four knobs become the macros:

| Knob | Macro |
| --- | --- |
| **KNOB 1** | FILTER (left LPF, right HPF; 0 at centre) |
| **KNOB 2** | CRUSH |
| **KNOB 3** | THROW (sends the dry mix into the delay and reverb) |
| **KNOB 4** | DEPTH (buffer-effect level; shimmer while OCT UP / DN plays) |

While the PERFORM screen is up the knobs are only these, and [[SELECT]] turns no page under it. **The knob effects stay on** after FX is released and on every page. Turn them back to 0 on the PERFORM screen to switch off; TOOLS `INIT*` switches them all off. They are never saved. The buffer effects stop when their key is released. MENU PERFORM HOLD / PAGE: PAGE keeps the screen up after FX is released.

### REPEAT STEP

Hold [[FX]], then hold a track key (F3 = track 1 .. D4 = track 6): the track plays the step it was on, again and again at the rate, while its own steps pause. The repeat plays the step's note, loud when the step has an accent; a step that is off, a TIE or a Grids track repeats the track's last note (no PROB, no RATCH). Let go and its pattern carries on in time as if it had played all along.

While playing, the repeats land on the grid of the rate (following swing, the tempo and an incoming MIDI clock), the first on the next grid point; while stopped they start at once at the tempo. Several tracks can repeat together. A change of rate while repeating takes over at the next grid point. The repeats are drum hits from the sequencer, so the master effects work on top of them. *If you used FX plus a white key to mute a track in an older version, hold SEQ and press its black key instead.* A controller button learned to a PERFORM mute now holds that track's REPEAT STEP.

## Parameter locks

With KEYS off (SEL unlit) the white keys are the selected track's steps on **every page** (HOME, EDIT, FX, LFO, GLO, SAVE...), tapped and held as on SEQ; the exceptions are TRACKS with a track armed, COMP, the PRESETS list, the menu and NAME. While playing, the keys show the sheet of 16 holding the selected track's playhead (PATTERN and TRACKS keep the sheet you chose with OCT- / OCT+); while a step key is held the sheet stays put. [[OCT+]] held plus a step toggles its accent. Finger drumming needs TRACKS (a track armed) or KEYS.

### Locking a step

With KEYS off, on any page whose knobs MOTION can record (SOUND, ENV, FX, INSERT and SRR, FILTER, a SAMPLE's PLAY CUT LOOP, the LFO's RATE and MORPH, the DEST slots' DEPTH), **hold a step and turn a knob**: that step plays its own value for that knob, and the next step plays the knob's value again. An LFO on the knob modulates round the locked value.

- The top bar says `LOCK STEP 5` while a step is held; the rows show that step's values (a locked one inverted).
- Knobs MOTION cannot record (FILTER TYPE, INSERT TYPE, an LFO's WAVE, DEST, SRC) do nothing while a step is held.
- **Machine locks** (Model:Cycles): hold a step and turn MODEL (EDIT / SOUND page knob 1) to lock that step to another model: `MLOCK STEP 5 SDEFM`. The track's own model stays; on that step the hit plays the locked model with the track's current knobs (each model reads them its own way) plus that step's parameter locks, and rings out as any hit until the next one. Turn back to the track's own model (`MLOCK STEP 5 OFF`) or OCT- on a page with MODEL to clear it. ACID cannot be a machine lock (it keeps its gate and track limit). MOTION never records MODEL. Machine locks are stored as ordinary locks: copy / paste of steps, CLRSQ, variations and the editor treat them like the others.
- Turning the knob shows the lock full screen (the knob popup, the value inverted). While that popup is up, [[OCT-]] is undo: the lock goes back to how it was before the turns (no lock, or its old value) and nothing is cleared.
- Hold the step and tap [[OCT-]] (with no knob popup up: 3 s after the last turn) to clear its locks on this page's knobs (`STEP 5 LOCKS CLEARED`); hold [[OCT-]] 0.6 s to clear all its locks (`STEP 5 ALL CLEARED`).
- A step turned off keeps its locks for when it is on again.
- Locks and MOTION share **1024 events per project** (`LOCKS FULL`). Locks always play; the track clear (REC + SEL held 5 s) clears both.
- STOP, a model change (it also lets a held machine lock go), a MIDI LEARN CC and a preset SAVE use the knob's own value, never a lock's.

## MOTION

Knob turns are recorded per step while a track plays, and played back with the pattern: up to 1024 events per project (shared by the 6 tracks and the locks). Motion has no page and no PLAY switch on the panel; the track clear (REC + SEL held 5 s) clears it with the steps and locks.
