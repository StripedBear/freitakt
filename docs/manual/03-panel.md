# The panel
<!--blurb: Every button, knob and key, and the rules that apply on every page.-->

## Overview

The FM-1 panel has a 240 by 240 screen, 14 buttons, four rotary controls (**SELECT**, **ALGORITHM**, **PRESETS**, **MASTER**), four **KNOB 1 to 4** under the screen, and 27 keys: 16 white and 11 black. The keys are the 16 steps of the selected track in most places; they play tracks, notes or effects elsewhere, as stated in each section.

Pressing a section button opens its first page. Pressing it again (it wraps to the first page), or turning [[SELECT]] (it stops at the ends), steps through its pages. A family reopens at the last page used. [[HOME]] returns to the main screen.

## Rotary controls

| Control | What it does |
| --- | --- |
| **MASTER** | The volume: the speaker, the headphones and USB audio. |
| **SELECT** | Turns the pages of a section; on the carousels (the MENU and SAVE's pages) it scrolls to the next / previous item and wraps round. On HOME's TRACKS mode it sets the tempo instead (`CLK USB` / `CLK TRS` when following a clock). |
| **ALGORITHM** | With KEYS on (SEL lit): picks the selected track (T1 to T6), on any page. With KEYS off (SEL unlit): **NOTE** of the selected track, on any page, as SOUND 1's NOTE knob: with step keys held it sets their notes, otherwise the track's NOTE (the note new steps get); the knob popup shows it (`NOTE C#4`, `STEP 5 NOTE C#4`) and [[OCT-]] undoes. Tracks are always one [[SCL]] + black key away. In MIDI LEARN it picks NEXT / PREV TRACK (KEYS on or off). |
| **PRESETS** | Loads the next or previous preset onto the selected track at each detent; a screen of its own replaces the page (the bank and track on top, the preset large on an inverted bar, three presets before and after it with their machines) and the page comes back 3 s after the last detent. A factory preset sets the sound only; a USER preset also sets the track's FX. |
| **KNOB 1 to KNOB 4** | Edit the four rows of a list page, top to bottom; a turn shows the parameter full screen (the knob popup, [[OCT-]] undoes; see *Screens and the top bar*). Knob turns can be recorded as MOTION. |

## Buttons

| Button | Opens |
| --- | --- |
| **HOME** | Two modes: **TRACKS** (KNOB 1 TRACK, 2 LEVEL, 3 LEN, 4 PAN; the selected track's steps on the keys) and **HOME** (the minimal status screen, KNOB 1 BPM and KNOB 2 SWING; with KEYS off the white keys are the selected track's steps, as everywhere). A tap on HOME switches between them; from any other screen it opens the one last shown. Power-on: TRACKS. **Hold HOME** for the device menu. |
| **EDIT** | The selected track's sound: the SOUND pages (MODEL, NOTE and the model's knobs but its envelope, which is on ENV; ARP has its own button). **Hold EDIT** (about 0.4 s) for the PRESETS list. |
| **SEQ** | PATTERN, with the steps on the white keys (a short tap opens it, on its release). A hold opens it too, on the release (there is no TOOLS screen any more). **SEQ + black key 1 to 6**: mute / unmute track 1 to 6 (see *Combos*); a white key with SEQ does nothing. |
| **FX** | The track's FILTER, FX (the sends), INSERT (with SRR) and COMP (the compressor): 6 pages. **Hold FX** for PERFORM. |
| **LFO** | The track's LFO (two pages), then DEST. |
| **ENV** | The selected track's envelope page: its machine's ATTACK / DECAY (a SAMPL's ATK DECAY SUS REL) and the filter's decay FDEC (see *Sounds, ENV*). Pressed again: its next page, if any. |
| **SCL** | Labelled SEL. KEYS is on at power-on (SEL lit); a tap (on the release, no key pressed while held) toggles it. While SEL is lit the keys play the selected track chromatically on every page, TRACKS and COMP included; while SEL is unlit the white keys are the selected track's steps on every page (see *KEYS and STEPS*). **SEL + black key 1 to 6**: select track 1 to 6. |
| **GLO** | A tap (on the release, within 0.4 s, no key pressed): MIDI OUT, DLY 1, DLY 2, REVERB and CHORUS (5 pages). **GLO + a white key**: queue a pattern; several white keys: a chain; **GLO + black 1 to 8**: a variation (see *Combos*). |
| **ARP** | A tap (on the release, no key, knob or other button used while held): the selected track's ARP page 1 (MODE OFF / RTRG / ARP1 / ARP2, RATE, GATE, FADE); tapped again there, page 2 (DIR CHORD RANGE; a drum has no page 2, so it stays on page 1). The ARP LED is lit while an ARP page is shown. **ARP + black key 1 to 6**: solo track 1 to 6; **ARP + F#5**: SOLO TOGGLE / MOMENTARY (a hold used for a combo opens no page). |
| **SAVE** | A tap (on the release): PROJECT, TOOLS and COPY (3 carousel pages, the next one each tap; COPY's KNOB 4 turns MIDI LEARN on and off). **Hold SAVE**: the cue map. |
| **PLAY** | Play and stop. |
| **REC** | On TRACKS a tap arms the selected track; elsewhere it is REC ALL. Hold 0.4 s (on any page): a fill. **REC + SEL held 5 s**: clear the selected track; **SAVE / GLO + REC**: copy (see *Combos*). See *Playing and recording*. |
| **OCT- / OCT+** | On PATTERN and TRACKS: the previous / next sheet (bank) of 16 steps; the octave in KEYS (-2 to +2; as the MIDI KEYBOARD -4 to +4, see *MIDI*); NO / YES, BACK / OK in dialogs. They *breathe* (fade in and out) when pressing them would do something. |

### HOME in detail

HOME has two modes. **TRACKS** is the mixer: the six tracks as a large list, one row each with the track's number, its machine and, at the right, its preset's name (`*` once a SOUND knob moved it). A muted track's row is inverted (MUTE, LEVEL 0 or another track's SOLO), the selected track has a pointer (`>`) at its left, and a REC-armed track a small square after its number (solid while playing). [[KNOB 1]] picks the track, [[KNOB 2]] its LEVEL (a muted track's first turn unmutes it), [[KNOB 3]] its LEN (length), [[KNOB 4]] its PAN; the selected track's steps are on the keys. **HOME** is the performance screen, kept minimal (Model:Cycles style): one large status line such as `P1 V2 T6 L1`, the tempo large as `BPM 120` near the top, the CPU load (`CPU 23%`) and the battery small at the top, and nothing else. In the status line **P** is the pattern playing (1 to 4 for A to D; `P-` before a pattern is loaded), **V** its playing variation (1 to 8), **T** the selected track and **L** the track's step page (bank of 16 steps). While a pattern chain runs, its line (`CH A1 B3 A2`: each entry's pattern and variation, up to 6 shown, the entry playing inverted, the next underlined) shows under the status line. [[KNOB 1]] sets the BPM and [[KNOB 2]] the global SWING; [[KNOB 3]] and [[KNOB 4]] do nothing on HOME. While the tempo follows an external clock the BPM line names the source with the measured tempo (`USB 120`, `TRS 98`), and KNOB 1 only says `CLK USB`. HOME has no top bar: the screen only redraws when something on it changes.

### KEYS and STEPS (SEL)

SEL switches the white keys between two jobs, Model:Cycles style, on every screen. **SEL lit (KEYS):** the keys play the selected track chromatically. **SEL unlit (STEPS):** the 16 white keys are the selected track's steps on every page: HOME, TRACKS (nothing armed), EDIT, FX, LFO, GLO, SAVE. A tap toggles a step, a held step plus a knob makes a parameter lock (on PATTERN and TRACKS: the step's PROB / RATCH / ACCENT / NUDGE), and the step copy / paste combos work everywhere. A few screens keep their own keys: the menu, NAME, the PRESETS list (the keys play), PERFORM ([[FX]] held), the held-button combos, TRACKS with a track armed (the keys record hits) and COMP (a white key ducks a track). Select tracks with **SEL + black 1 to 6** or [[ALGORITHM]].

## The keys

- **16 white keys** (white 1 = F3 ... white 16 = G5): with KEYS off, the steps 1 to 16 of the sheet shown (up to four sheets of 16, 64 steps). On PATTERN and TRACKS the sheet is the one you chose with OCT- / OCT+; everywhere else, while playing, it follows the selected track's playhead (it stays put while you hold a step, so a lock or a copy does not jump), and stopped it is the one last chosen. HOME's status line `L` shows the sheet on the keys.
- **11 black keys** (black 1 to 11 = F#3 G#3 A#3 C#4 D#4 F#4 G#4 A#4 C#5 D#5 F#5): with [[FX]] held, live effects on the whole mix. In KEYS, the sharps. Alone they keep their page's job (a black key alone no longer queues a variation). Black keys 1 to 6 are tracks 1 to 6 for the combos below.
- **Held buttons** change what the keys do: the combos below; [[FX]] held: black keys are effects, white keys F3 to D4 repeat a track's step (REPEAT STEP), D5 to G5 pick its rate; [[SEQ]] held: black keys mute; [[SAVE]] held: the cue map; [[OCT+]] held: a step tap sets its accent.

### Step LEDs

Wherever the white keys are the selected track's steps (with KEYS off: every page but the exceptions above), the keys light the way Elektron's Model:Cycles does:

| Key | Lit | Blinking (250 ms) | Breathing | Winking (dark 1/8 s each second) |
| --- | --- | --- | --- | --- |
| **White 1 to 16** (the bank's steps) | A trig on the step | A step with a parameter lock or a machine lock | | |
| **Black 1 to 6** (the tracks) | The selected track | | An armed track (REC) | The selected track, armed |

While the pattern plays, the **running cursor** is the selected track's own step (its LEN, division and FIT; only on the sheet shown): its key is toggled, so a lit or blinking step goes dark and an empty step lights. With [[OCT+]] held the keys show the bank's accents instead (no blinking). A held button's overlay (GLO, SEQ's mutes, SEL, ARP, the cue map, PERFORM, NAME, the PRESETS list) takes the keys over while it is up.

## Combos

Hold the button, press the key (Elektron Model:Cycles style). They work on every page; a key pressed with the button plays no note, sends no MIDI and records nothing. Not in the menu, a dialog, NAME, the PRESETS list or the clear's countdown; with PERFORM up the buttons are plain buttons.

| Combo | What it does | LEDs while the button is held |
| --- | --- | --- |
| **SEL + black 1 to 6** | Select track 1 to 6. A SEL tap with no key pressed still turns KEYS on / off, on its release. | The selected track's black key |
| **SEQ + black 1 to 6** | Mute / unmute track 1 to 6, at once or on the next bar (the menu's MUTE MODE). No page for that hold. White keys with SEQ do nothing (no step, no note). | The audible tracks' black keys; a mute waiting for the bar blinks |
| **ARP + black 1 to 6** | Solo track 1 to 6: TOGGLE (a press on / off) or MOMENTARY (while the key is held). An ARP tap with no key pressed opens the ARP pages instead, on its release. | The soloed tracks' black keys |
| **ARP + F#5** (black 11) | SOLO TOGGLE / MOMENTARY (saved). | F#5 lit while MOMENTARY |
| **GLO + white n** | Queue pattern n (A to D: white 1 to 4; whites 5 to 16 do nothing), as the cue map does. A black key after it (still the only white of the hold) queues that pattern's variation too. | The playing pattern's white key; a queued one blinks |
| **GLO + black n** (1 to 8) | Queue variation n of the playing pattern (of the queued one, if one is queued), at the pattern end, as the cue map does. Black 9 / 10 (C#5 / D#5) do nothing with GLO. | Black 1 to 8: the playing variation lit, a queued one blinks, used ones dim, unused dark |
| **SAVE + REC** | Copy the selected track: its 64 steps (everything a step holds, micro timing too), their parameter locks, and its LEN, DIV and SWG, from the pattern and variation playing. The message `COPIED T3`. No REC tap, fill or arm, no projects page, no cue map after it. | as usual |
| **SAVE + PLAY** | Paste the copied track onto the selected track (its steps, locks and LEN / DIV / SWG replaced; its motion and sound stay). The message `PASTED T5` (`NO TRACK COPIED` if the clipboard holds none, `LOCKS FULL` if not all locks fit). The transport does not start or stop. There is no undo. | as usual |
| **GLO + REC** | Copy the pattern: every track's steps, locks and LEN / DIV / SWG, as the variation playing has them. The message `COPIED PATTERN`. No REC action, no GLOBAL page. | as GLO |
| **GLO + PLAY** | Paste the copied pattern over the variation playing (every track replaced; switch pattern or variation first to copy it somewhere else). `PASTED PATTERN`; no start / stop, no undo. | as GLO |
| **Step(s) held + REC** (step views) | Copy steps: one held step, or the lowest to the highest of the held steps (step 3 and step 8 held: steps 3 to 8), inside LEN, from the selected track: each step whole, its parameter locks and machine locks. The message `COPIED 3-8` (`COPIED 5` for one). The keys toggle nothing on their release; REC does not arm, fill or REC ALL. A step that only lit because it was held comes back off first. | as usual |
| **Step(s) held + PLAY** (step views) | Paste the copied steps onto the selected track (any track, pattern or variation): one held step pastes the whole copy from there (steps past LEN are dropped); several held fill the lowest to the highest of them, the copy repeated or cut to fit. The target steps' locks are replaced. `PASTED 9-14` (`NO STEPS COPIED`, `LOCKS FULL` if not all locks fit). No start / stop, no undo. | as usual |
| **REC + SEL** (held 5 s) | Clear the selected track: its steps, parameter locks and motion in the pattern playing. Either button first; while both are held the screen shows `CLEAR T3` and counts 5 to 1; let either go before the end and nothing changes. Then `CLEARED T3`. Neither button does its own job (no REC ALL, fill, arm, KEYS toggle or track select). | as usual |
| **GLO + 2 or more whites** | On GLO's release: the pattern chain, one entry per white key in the order pressed (repeats allowed, up to 16). A black key right after a white sets that entry's variation (GLO + A, black 1, B, black 3, A, black 2 = `A1 B3 A2`); without one the entry takes the pattern's current variation (playing, else last played). One white alone (with or without a black) is a plain cue and ends a chain; a black key before any white is a plain variation cue (and ends a chain). | as above |

## Screens and the top bar

No page has a step strip or a breadcrumb at the bottom. The **top bar** holds the battery icon at the right; at the left it shows only what is going on for the moment: a record icon while a track is armed, MIDI LEARN's line, and held modes (`CUE`, `FILL`, `LOCK STEP`). Messages (such as `T3 DECAY 64`, `LOADED` or `COPIED T3`) appear as a small popup. While REC + SEL are held the whole screen is the clear's warning: `CLEAR T3` and a large countdown from 5 to 1.

Parameter pages are **lists**: one row per knob, `NAME  value` in a large font, [[KNOB 1]] to [[KNOB 4]] the rows top to bottom, with no round dials and no graphs. The SOUND pages show a small waveform of the rendered hit at the top; LFO 1/3 shows a small schematic of one cycle of the wave. TRACKS is a list of the six tracks (see *HOME in detail*); PERFORM keeps its own layout.

**Carousels.** The device MENU and the SAVE pages (PROJECT, TOOLS, COPY) show one item at a time: a title and a row of dots at the top (one dot an item, the current one big), the item's name big in the middle with an arrow at each side, its value under it, a note under that (a state such as `LOADED EDITED`, or the message of the moment), and at the bottom the keys' hints as keycaps (for example `K1 SET  OCT+ NEXT` / `SEL MOVE  OCT- BACK`). [[SELECT]] scrolls and wraps round; what changes a value or acts is said in the hint line. Only what changed is redrawn.

**The knob popup.** Turning a parameter knob (on a list page, TRACKS' LEVEL / LEN / PAN, HOME's BPM and SWING, a held step's PROB / RATCH / ACCENT / NUDGE or its parameter locks, NOTE on SOUND 1 or on [[ALGORITHM]] with KEYS off) clears the screen to the parameter's name and value, very large; 3 s after the last turn the page comes back. While the popup is up, [[OCT-]] is **undo**: the parameter goes back to the value it had before the first turn of this popup (a lock made by the turns goes away, a lock that was there gets its old value back; a muted track's LEVEL mutes again; MODEL brings the old machine and its sound back), the popup stays and shows the value back, and OCT- does nothing else (no octave or bank change, no lock clearing). Undo works once per popup; turning the knob again continues the same popup, so a further OCT- goes back to the same starting value. A parameter lock's value, and a held step's NOTE, are shown inverted; NOTE names the steps it writes (`STEP 5 NOTE`, `3 STEPS NOTE`), and its undo puts back every one of them. Knobs that only pick something (TRACKS' KNOB 1 TRACK, LFO DEST's SLOT) open no popup, and turning one closes it.

| Top-bar sign | Meaning |
| --- | --- |
| record icon (left) | a track is armed |
| `LEARN: TURN OR PRESS`, `T3 DECAY: SEND A CC`... | MIDI LEARN's line, while MIDI LEARN is on |

## Common rules

- **Step keys.** A step toggles only on a short tap (released within 0.3 s with no knob turned), on the release. Held 0.3 s, an off step turns on at once, ready to edit; an on step stays on.
- **Held step.** While a step is held, [[KNOB 1]] / [[KNOB 2]] / [[KNOB 3]] / [[KNOB 4]] set its PROB / RATCH / ACCENT / NUDGE (-50 to +50). This works on PATTERN and TRACKS; the page's own knobs rest while a step is held. On every other page a held step plus a knob is a parameter lock (see *Parameter locks* in *Playing*). [[OCT+]] held plus a step toggles that step's accent.
- **Sheet (bank).** On PATTERN and TRACKS with KEYS off, [[OCT+]] alone (on its release) and [[OCT-]] move to the next / previous sheet of 16. Elsewhere OCT- / OCT+ keep their page jobs (OCT- the knob popup's undo, OCT+ an LFO page's RATE MODE) and the sheet follows the playhead while playing. While [[OCT+]] is held the keys show the sheet's accents.
- **Knob acceleration.** With MENU ACCEL ON, turning a knob fast over a wide value speeds it up (see Settings).
- **Safe start.** Hold [[SEQ]] while powering on: nothing is loaded, every LED is dark, Bluetooth does not autostart.
