# Settings and menu
<!--blurb: MIDI OUT and the device menu: colour, backlight, speaker EQ, accel, clock and MIDI settings, calibration.-->

## GLO pages

Tap [[GLO]] for five pages: **MIDI OUT**, then **DLY 1**, **DLY 2**, **REVERB** and **CHORUS** (the last four are described under *Effects*). The tempo and global swing are HOME's knobs: [[KNOB 1]] BPM, 40 to 240 (120), [[KNOB 2]] SWING, 0 to 100 % (0). The other device settings are rows of the menu (below); the CPU load is on HOME.

| Page | Knob | Label | Values (default) |
| --- | --- | --- | --- |
| MIDI OUT | **KNOB 1** | OUT | the sequencer's notes out: OFF, USB (default); BLE and BOTH in the Bluetooth version |
| MIDI OUT | **KNOB 2** | TnCH | the selected track's channel: OFF, 1 to 16 (track n on n) |

## The menu

Hold [[HOME]] (0.7 s) for the **MENU**. It is a carousel: one item at a time, its name big in the middle, its value under it, a row of dots at the top for where you are (one dot an item) and the keys' hints at the bottom.

- [[SELECT]] scrolls to the next or previous item; past the last it wraps round to the first, and back.
- [[KNOB 1]] changes the item's value (on COLOR it wraps; on SPEAKER EQ and BACKLIGHT it stops at the ends; on ON / OFF items right is ON, left is OFF). The hint line says `K1 SET`.
- [[OCT+]] steps the value on and wraps, or toggles (`OCT+ NEXT`); on HARDWARE CALIBRATION it starts it (`OCT+ START`), on BLUETOOTH it opens the Bluetooth page (`OCT+ OPEN`). USB MIDI is only shown (no K1 or OCT+ hint).
- [[OCT-]] closes the menu (`OCT- BACK`), as does holding [[HOME]] again. The menu saves when it closes.

There are 16 items, 17 in the Bluetooth version:

| Item | Values (default) | Notes |
| --- | --- | --- |
| **COLOR** | ORANGE, ORANGE INV, RED, RED INV, BLUE, BLUE INV, GREEN, GREEN INV (ORANGE) | the screen's two colours; applies at once. ORANGE, the default, is black ink on a bright orange ground (about RGB ff6000; the Model:Cycles look); ORANGE INV the inverse, orange on black; RED a red ground and dark ink; RED INV red on black; BLUE white on blue; BLUE INV blue on white; GREEN dark on green; GREEN INV green on black |
| **SPEAKER EQ** | FLAT, LOWCUT, BASS+ (FLAT) | LOWCUT is a high-pass at about 110 Hz; BASS+ lifts about 220 Hz and adds bass harmonics for the small speaker |
| **USB LEVEL** | MASTER, FIXED (MASTER) | whether USB follows the MASTER knob |
| **PERFORM** | HOLD, PAGE (HOLD) | PAGE keeps the PERFORM screen up after FX is released |
| **ACCEL** | OFF, ON (ON) | knob acceleration |
| **BACKLIGHT** | OFF, DIM LO, DIM HI, INV, INV LO, INV HI (OFF) | key and button LED levels |
| **CLOCK** | INT (default), USB, TRS; BLE and LINK in the Bluetooth version | the tempo source |
| **MIDI SYNC** | OFF (default), USB; BLE and BOTH in the Bluetooth version | MIDI clock out |
| **MIDI CHANNEL** | 1 to 16 (10) | the drum MIDI channel |
| **MIDI ROUTE** | NOTE (default), SEL, TRACK | MIDI-in routing |
| **USB MIDI** | OFF, WAIT, BUS, ENUM, MIDI | read-only USB status |
| **METRONOME** | OFF, LOW, MID, HIGH | the click (below) |
| **MUTE MODE** | NOW, BAR (NEXT BAR) | when mutes land |
| **MIDI THRU** | OFF (default), ON | |
| **REC QUANT** | 1/16 (default), 1/8, 1/32, OFF | where live recording puts a hit (see Sequencer) |
| **HARDWARE CALIBRATION** | OCT+ runs it | teach each button and knob; also reachable by holding [[OCT-]] and [[OCT+]] at power-on; times out after 30 s idle |
| **BLUETOOTH** | Bluetooth version only | the last item; [[OCT+]] opens the Bluetooth page (a list, see *Bluetooth*; [[OCT-]] back to the menu) |

There is no ABOUT item any more: the firmware version is in the web editor's header bar (and the USB console's `status`), the licence and credits are in `LICENSING.md` and the README.

**METRONOME** is a click on every quarter note while playing, the bar's first higher, on the FM-1's clock or following one. It comes after the PERFORM effects and follows MASTER, on the speaker, headphones and USB audio. METRONOME, MIDI THRU, MUTE MODE, REC QUANT and the solo mode are device settings; MIDI SYNC, MIDI ROUTE, OUT and the tracks' channels are saved with each project. See *MIDI* for how they work.

### COLOR

Eight themes: each colour, then its inverse, ORANGE first (the default on a new device). [[KNOB 1]] steps back and on; [[OCT+]] steps on; both wrap. A device setting, saved when the menu closes. A colour chosen on an older Hortator stays the same.

### Knob acceleration (ACCEL)

With ACCEL ON, a knob turned in one direction over a wide value (more than 32 steps) speeds up with how fast it turns. One click at a time, or up to about 12 clicks per second, is one step a click (fine control). Faster, the steps grow with the speed, in fractions of a step: about x2 at 20 clicks per second, x4 at 40, up to x8 (x4 on a value of 64 steps or less). A 0..127 value takes about 65 clicks at 20 a second, 34 at 40, 24 at 60 (upstream Felucca's: 127, 65, 64). The first clicks of a turn, and the first click back after a reversal, are one step. Lists of names (MODEL, TYPE, WAVE...) are one entry a click at any speed; the PRESETS list at most x4. ACCEL OFF: one step a click. Every click counts up to about 300 a second.

### BACKLIGHT

- **OFF**: LEDs lit or dark, as before.
- **DIM LO**: every key LED that is not lit and every idle button glows at about 3 % (a very short flash about 900 times a second: steady, also with Bluetooth on) so the panel can be found in the dark. Where the white keys are the selected track's steps (PATTERN with KEYS off; TRACKS while no track is armed) the unlit steps 1, 5, 9 and 13 of the bank shown glow at about 25 % (bank 2: 17, 21, 25, 29), marking the beats.
- **DIM HI**: the same, brighter (about 6 %, the beats about 30 %).
- **INV**: as the stock firmware does: idle LEDs lit, active ones (steps that are on, the playhead, held keys, the page's button) dark. **INV LO / INV HI**: the same with the idle ones at about 10 % / 25 %.
- In every mode [[OCT-]] / [[OCT+]] **breathe** (fade in and out) where pressing them would do something: the next / previous bank, the LFO pages' RATE MODE, YES in a dialog, OK in the menu (not on USB MIDI), NAME's RENAME, MIDI LEARN's DONE, the PRESETS list's LOAD (and DELETE on a used user slot). A way back (NO, CANCEL, BACK) is lit; KEYS' octave shift is lit.
- A safe start keeps every LED dark.

## CPU and PRIO

When an audio block takes more than 85 % of its time (95 % before 1.0.0, so a sudden peak no longer runs past the deadline), the FM-1 stops one sounding voice to catch up. A track with **PRIO LOW** loses its voices before a **PRIO HIGH** one, the oldest first; a held ACID note goes last. The same order picks the voice to stop when too many play at once. PRIO is on the track's last EDIT (SOUND) page, after CHOKE: HIGH by default, saved in the project. HOME shows the load (`CPU 23%`).

## Start screen

The start screen is the **FREITAKT splash**, in the chosen COLOR reversed (ORANGE: orange on black): a CRT beam sweeps down revealing FREITAKT through scanlines, glitch slices settle, `Form folgt Function` types itself under the name and a boot bar fills, about 3.2 s in all. It runs after the audio and USB have started, so it delays nothing; any key, button or knob skips it (that press does nothing else). The boot guard: a restart inside the first 30 s counts as a failed start, and two in a row enter the built-in bootloader.
