# The panel
<!--blurb: Every button, knob and key, and the rules that apply on every page.-->

## Overview

The FM-1 panel has a 240 by 240 screen, 14 buttons, four rotary controls (**SELECT**, **ALGORITHM**, **PRESETS**, **MASTER**), four **KNOB 1 to 4** under the screen, and 27 keys: 16 white and 11 black. The keys are the 16 steps of the selected track in most places; they play tracks, notes or effects elsewhere, as stated in each section.

Pressing a section button opens its first page. Pressing it again (it wraps to the first page), or turning [[SELECT]] (it stops at the ends), steps through its pages; the footer shows `n/m` at the right. A family reopens at the last page used. [[HOME]] returns to the main screen.

## Rotary controls

| Control | What it does |
| --- | --- |
| **MASTER** | The volume: the speaker, the headphones and USB audio. |
| **SELECT** | Turns the pages of a section and moves the menu's cursor. On HOME's TRACKS mode it sets the tempo instead (`CLK USB` / `CLK TRS` when following a clock). |
| **ALGORITHM** | Picks the selected track (T1 to T6), on any page. In MIDI LEARN it picks NEXT / PREV TRACK. |
| **PRESETS** | Loads the next or previous preset onto the selected track at each detent; a popup shows the bank and the presets around it. A factory preset sets the sound only; a USER preset also sets the track's FX. |
| **KNOB 1 to KNOB 4** | Edit the four values on the screen, one knob per column. Knob turns can be recorded as MOTION. |

## Buttons

| Button | Opens |
| --- | --- |
| **HOME** | Two modes: **TRACKS** (KNOB 1 TRACK, 2 LEVEL, 3 LEN, 4 PAN; the selected track's steps on the keys) and **HOME** (the minimal status screen, the track's first four knobs, the keys play the tracks; SELECT goes on to the compressor's pages). A tap on HOME switches between them; from any other screen it opens the one last shown. Power-on: TRACKS. **Hold HOME** for the device menu. |
| **EDIT** | The selected track's sound: its model and knobs. **Hold EDIT** (about 0.4 s) for the PRESETS list. |
| **SEQ** | The steps on the white keys, then PATTERN and MOTION (a short tap turns pages, on its release). **Hold SEQ** (0.4 s, or press a white key with it) for the step TOOLS screen. **SEQ + black key 1 to 6**: mute / unmute track 1 to 6 (see *Combos*). |
| **FX** | The track's FILTER, FX (the sends), INSERT (with SRR): 5 pages. **Hold FX** for PERFORM. |
| **LFO** | The track's two LFOs side by side, then DEST. |
| **ENV** | STEPS / TRACKS mode and parameter locks (a padlock in the top bar while STEPS). ENV turns KEYS off and SCL turns STEPS off. |
| **SCL** | Labelled SEL. A tap (on the release, no key pressed while held): KEYS, the keys play the selected track chromatically. **SEL + black key 1 to 6**: select track 1 to 6. |
| **GLO** | A tap (on the release, within 0.4 s, no key pressed): GLOBAL, SYSTEM, GLOBAL, then DLY, DLY, REVERB and CHORUS (7 pages). **GLO + a white key**: queue a pattern; several white keys: a chain; **GLO + black 1 to 8**: a variation (see *Combos*). |
| **ARP** | **ARP + EDIT**: the selected track's ARP page (MODE OFF / RTRG / ARP and its settings; again: page 2). **ARP + black key 1 to 6**: solo track 1 to 6; **ARP + F#5**: SOLO TOGGLE / MOMENTARY. A tap does nothing. |
| **SAVE** | A tap (on the release): PROJECT, TOOLS and COPY (3 pages; COPY's KNOB 4 turns MIDI LEARN on and off). **Hold SAVE**: the cue map. |
| **PLAY** | Play and stop. |
| **REC** | On TRACKS a tap arms the selected track; elsewhere it is REC ALL. Hold 0.4 s off the SEQ pages: a fill. Hold 0.7 s on a SEQ page: CLEAR TRACK. See *Playing and recording*. |
| **OCT- / OCT+** | The previous / next bank of 16 steps; the octave in KEYS; NO / YES, BACK / OK in dialogs. They *breathe* (fade in and out) when pressing them would do something. |

### HOME in detail

HOME has two modes. **TRACKS** is the mixer: for each track LEVEL, LEN (length), PAN, and the track's steps on the keys. **HOME** is the performance screen, kept minimal (Model:Cycles style): one large status line such as `P1 V2 T6 L1`, the CPU load (`CPU 23%`) and the battery small at the top, and nothing else. In the status line **P** is the pattern playing (1 to 4 for A to D; `P-` before a pattern is loaded), **V** its playing variation (1 to 8), **T** the selected track and **L** the track's step page (bank of 16 steps). While a pattern chain runs, its line (`CH A1 B3 A2`: each entry's pattern and variation, up to 6 shown, the entry playing inverted, the next underlined) shows under the status line. The four knobs edit the selected track's first four sound knobs; turning one shows its value large in place of the status line (ZOOM, see Settings) until 3 s after the last turn. HOME has no top bar, knob row or breadcrumb: the screen only redraws when something on it changes. [[SELECT]] on HOME goes on to the compressor's pages (see Modulation and compressor).

### EDIT, FX, LFO and the selected track

On HOME, EDIT, FX and LFO the selected track's key (a white key F3 to D4) is lit, and pressing a white key there selects that track. KEYS, COMP, the step pages, GLOBAL and SAVE keep their own key jobs.

## The keys

- **16 white keys** (white 1 = F3 ... white 16 = G5): the steps 1 to 16 of the current bank (up to four banks, 64 steps); on HOME the white keys F3 to D4 play tracks 1 to 6.
- **11 black keys** (black 1 to 11 = F#3 G#3 A#3 C#4 D#4 F#4 G#4 A#4 C#5 D#5 F#5): with [[FX]] held, live effects on the whole mix. In KEYS, the sharps. Alone they keep their page's job (a black key alone no longer queues a variation). Black keys 1 to 6 are tracks 1 to 6 for the combos below.
- **Held buttons** change what the keys do: the combos below; [[FX]] held: black keys are effects, white keys F3 to D4 repeat a track's step (REPEAT STEP), D5 to G5 pick its rate; [[SEQ]] held 0.4 s (or with a white key): step copy and paste tools; [[SAVE]] held: the cue map; [[OCT+]] held: a step tap sets its accent.

### Step LEDs

Wherever the white keys are the selected track's steps (the SEQ pages, TRACKS with no track armed, and every page in ENV's STEPS mode), the keys light the way Elektron's Model:Cycles does:

| Key | Lit | Blinking (250 ms) | Breathing | Winking (dark 1/8 s each second) |
| --- | --- | --- | --- | --- |
| **White 1 to 16** (the bank's steps) | A trig on the step | A step with a parameter lock or a machine lock | | |
| **Black 1 to 6** (the tracks) | The selected track | | An armed track (REC) | The selected track, armed |

While the pattern plays, the **running cursor** is the selected track's own step (its LEN, division and FIT; only on the bank shown): its key is toggled, so a lit or blinking step goes dark and an empty step lights. With [[OCT+]] held the keys show the bank's accents instead (no blinking). A held button's overlay (GLO, SEQ's mutes, SEL, ARP, the cue map, TOOLS, PERFORM, NAME, the PRESETS list) takes the keys over while it is up.

## Combos

Hold the button, press the key (Elektron Model:Cycles style). They work on every page; a key pressed with the button plays no note, sends no MIDI and records nothing. Not in the menu, a dialog, NAME, the PRESETS list or TOOLS; with PERFORM up the buttons are plain buttons.

| Combo | What it does | LEDs while the button is held |
| --- | --- | --- |
| **SEL + black 1 to 6** | Select track 1 to 6. A SEL tap with no key pressed still turns KEYS on / off, on its release. | The selected track's black key |
| **SEQ + black 1 to 6** | Mute / unmute track 1 to 6, at once or on the next bar (GLOBAL's MUTE). No TOOLS screen and no page turn for that hold. Once the TOOLS screen is up (SEQ held 0.4 s, or a white key pressed first), the black keys are its tools again. | The audible tracks' black keys; a mute waiting for the bar blinks |
| **ARP + EDIT** | The selected track's ARP page 1 (MODE RATE GATE FADE); pressed again (ARP still held), page 2 (DIR CHORD RANGE; hidden on a drum). The pages are also the last of EDIT's cycle. See *Sequencer, ARP and RETRIG*. | as usual |
| **ARP + black 1 to 6** | Solo track 1 to 6: TOGGLE (a press on / off) or MOMENTARY (while the key is held). | The soloed tracks' black keys |
| **ARP + F#5** (black 11) | SOLO TOGGLE / MOMENTARY (saved). | F#5 lit while MOMENTARY |
| **GLO + white n** | Queue pattern n (A to D: white 1 to 4; whites 5 to 16 do nothing), as the cue map does. A black key after it (still the only white of the hold) queues that pattern's variation too. | The playing pattern's white key; a queued one blinks |
| **GLO + black n** (1 to 8) | Queue variation n of the playing pattern (of the queued one, if one is queued), at the pattern end, as the cue map does. Black 9 / 10 (C#5 / D#5) do nothing with GLO. | Black 1 to 8: the playing variation lit, a queued one blinks, used ones dim, unused dark |
| **GLO + 2 or more whites** | On GLO's release: the pattern chain, one entry per white key in the order pressed (repeats allowed, up to 16). A black key right after a white sets that entry's variation (GLO + A, black 1, B, black 3, A, black 2 = `A1 B3 A2`); without one the entry takes the pattern's current variation (playing, else last played). One white alone (with or without a black) is a plain cue and ends a chain; a black key before any white is a plain variation cue (and ends a chain). | as above |

## Screens and the top bar

The screen has an inverted **top bar** (title, track T1 to T6, tempo icon, messages such as `T3 DECAY 64`, `REC ALL`, `LOCK STEP 5`, `LOADED`, `BT CONNECTED`), a compact **knob row** with round dials and their values, the **graph area** (scope, sound graph, piano roll...), a **trig strip** of steps and a **breadcrumb** such as `T1>K909>SOUND`.

| Top-bar sign | Meaning |
| --- | --- |
| padlock (left of the battery) | ENV is in STEPS mode |
| amber `FX` in place of the tempo icon | a PERFORM knob effect (FILTER, CRUSH, THROW, DEPTH) is on |
| red CPU icon for 2 s | an audio block took over 85 %: one voice was stopped |
| Bluetooth rune | whole: linked; struck through: on but not linked; absent: off (Bluetooth version) |
| `MIDI LEARN ON` | MIDI LEARN is active |
| `STEP REC nn` | step record, at step nn |
| `CLK USB` / `CLK TRS` / `CLK BLE` / `CLK LINK` | the tempo follows an outside clock |

## Common rules

- **Step keys.** A step toggles only on a short tap (released within 0.3 s with no knob turned), on the release. Held 0.3 s, an off step turns on at once, ready to edit; an on step stays on.
- **Held step.** While a step is held, [[KNOB 1]] / [[KNOB 2]] / [[KNOB 3]] / [[KNOB 4]] set its PROB / RATCH / ACCENT / NUDGE (-50 to +50). This works on STEP, PATTERN, MOTION and TRACKS; the page's own knobs rest while a step is held. [[OCT+]] held plus a step toggles that step's accent.
- **Bank.** With KEYS off, [[OCT+]] alone (on its release) and [[OCT-]] move to the next / previous bank of 16. While [[OCT+]] is held the keys show the bank's accents.
- **Knob acceleration.** With MENU ACCEL ON, turning a knob fast over a wide value speeds it up (see Settings).
- **Safe start.** Hold [[SEQ]] while powering on: nothing is loaded, every LED is dark, Bluetooth does not autostart.
