# Projects and tools
<!--blurb: Saving, loading, naming, copying slots, TOOLS, and what survives power-off.-->

## Projects

A project holds the six tracks' sounds, patterns (with each step's NUDGE), timing, locks, motion, variations, effects settings (INSERT, SRR, delay MODE, REVERB TYPE), SAMPLE settings, Grids and compressor. SYNC and ROUT are saved with it. There are **four slots, A to D**, saved in the FM-1's flash. Tap [[SAVE]] (on the release) for **PROJECT 1/3**; tapping [[SAVE]] again gives **TOOLS 2/3**, then **COPY 3/3**, then PROJECT again.

The three SAVE pages are carousels: one item at a time, big in the middle, with its value under it, a note under that (its state, or a message such as `AGAIN: LOAD`), a row of dots at the top for where you are and the keys' hints at the bottom. [[SELECT]] scrolls to the next or previous item and wraps round (D to A, A to D). The knobs keep their jobs; the hint line names them (`K3 LOAD  K4 SAVE`).

PROJECT shows one slot at a time: its letter big, its name under it (`EMPTY` for an unused slot, `USED` for a saved one without a name), and its state: `LOADED` (the current project), `EDITED` (unsaved edits), `COPIED` (the slot COPY took).

| Knob | Label | Behaviour |
| --- | --- | --- |
| **KNOB 1** | SLOT | A to D, as [[SELECT]] (it stops at A and D) |
| **KNOB 2** | NAME | a right turn opens the NAME screen |
| **KNOB 3** | LOAD | GO action: turn right once to arm (`AGAIN: LOAD`), again within about 1.5 s to act |
| **KNOB 4** | SAVE | GO action, armed the same way |

- **The last project saved loads at power-on** (`LOADED` pops up, stopped). If it cannot be loaded, the FM-1 asks `LOAD FAILED: SLOT B` / `START NEW PROJECT?`: [[OCT+]] (YES) forgets it, [[OCT-]] (NO) tries again at the next power-on; both carry on with the power-on kit. A safe start loads nothing.
- **Edits stay in RAM until SAVE.** A reset or power cut brings every slot back as last saved. Leaving a slot by LOAD or a cue keeps its edits in RAM, so going back finds them. LOAD of a slot with unsaved edits asks `DISCARD EDITS?` (a tempo change counts). SAVE onto another slot with unsaved edits asks `OVERWRITE B*?`. SAVE to another slot moves the edits there. RENAME of a slot with unsaved edits renames it in RAM only (`RENAMED (SAVE TO KEEP)`). Messages include `SAVED`, `SAVED (RAM)`, `SAVE A FIRST`, `STOP TO LOAD`, `STOP TO SAVE`. Unsaved variations of one slot can wait while another plays; a cue of a third slot then says `SAVE C FIRST`. A power cut during SAVE keeps the previous save whole.
- **Device settings are not in projects:** the colour theme, ACCEL, BACKLIGHT, SPEAKER EQ, USB LEVEL, PERFORM mode, METRO, THRU, MUTE timing, QUANT, the solo mode (TGL / MOM), MIDI LEARN bindings and Bluetooth settings.

## NAME

Rename a project on the device (up to 12 upper-case characters, transport stopped).

- White keys type with multi-tap (0.8 s): AB CD EF GH IJK LM NO PQ RS TU VW XYZ and 123 456 789 0-. . A mode toggles ABC and 123.
- Black keys: F# cursor left, G# space, A# cursor right, C# delete (held repeats), D# toggles ABC / 123.
- [[KNOB 1]] moves the cursor, [[KNOB 2]] changes the character. [[OCT+]] writes the name; [[OCT-]] or [[HOME]] cancels.

## TOOLS

TOOLS 2/3 is a carousel of four actions: CLEAR TRACK, INIT SOUND, CLEAR ALL, INIT ALL, each with whom it hits under it (`TRACK 3`, `ALL TRACKS`, `EVERYTHING`) and what it does in the note. [[SELECT]] scrolls them; each action keeps its own knob, named in the hint line (`K1 TURN TWICE`), and turning a knob shows its action. All four are GO actions: the first right-turn detent arms (`AGAIN: CLEAR ALL` in the note), a second within about 1.5 s acts. A left turn does nothing.

| Knob | Action | What it does |
| --- | --- | --- |
| **KNOB 1** | CLEAR TRACK | clears the selected track's steps, MOTION and locks (`PATTERN CLEARED`) |
| **KNOB 2** | INIT SOUND | resets the selected track's sound to its model default, including FILTER (`SOUND INIT`) |
| **KNOB 3** | CLEAR ALL | clears every track the same way (steps, MOTION and locks), resets LEN, DIV and SWG to defaults (`ALL PATTERNS CLEARED`) |
| **KNOB 4** | INIT ALL | everything back to power-on: kit, sounds, patterns, FX, COMP, BPM, motion, PERFORM knobs; saved projects and device settings are untouched (`ALL INIT`) |

## COPY and PASTE

COPY 3/3 copies a whole slot. It is a carousel of the slots like PROJECT (the same slot shown on both; [[SELECT]] or [[KNOB 1]] picks it), with the hints `K2 COPY  K3 PASTE` and `K4 LEARN ON` / `OFF`; the copied slot says `COPIED`.

| Knob | Label | Behaviour |
| --- | --- | --- |
| **KNOB 1** | SLOT | A to D, as [[SELECT]] |
| **KNOB 2** | COPY | remembers the slot shown (`COPIED A`; `EMPTY SLOT` if unused) |
| **KNOB 3** | PASTE | turn twice: makes the slot shown a copy (kit, steps, locks, and variations), named `... COPY`, unsaved until SAVE |
| **KNOB 4** | LEARN | MIDI LEARN `ON` / `OFF` (see *MIDI*; not part of COPY / PASTE, it just lives here) |

PASTE again on the same slot before saving undoes it (`PASTE UNDONE`). Not while playing (`STOP TO PASTE`); `COPY A SLOT FIRST`, `SAME SLOT`. The tempo and other globals are kept when pasting onto the loaded slot.

## Compatibility

- **FREITAKT (6 tracks) saves projects in a new format (FRT6) and variation banks as VAR6.** Projects and variation banks saved by Hortator (8 tracks) are not read: the slot shows as empty. Lock banks and user presets are unchanged.
- Projects saved by 1.0.0 or later (they carry NUDGE) show an empty slot (`LOAD FAILED` at power-on) on 0.26.0 and older; going back is safe but not supported. Every older project loads, its steps at NUDGE 0. Projects saved before INSERT and SRR load them OFF; before the delay's MODE, NORM; before HALL they keep ROOM or SPRNG.
- If a project's lock bank cannot be read, it loads without its locks and says `LOCKS LOST` once; unreadable variations say `VARIATIONS LOST`.
- **1.0.1:** FM-1s sold recently with a different flash chip of the same size (a UC25HQ80) now save projects, presets and settings; before, they stayed in RAM (`SAVED (RAM)`).
- **USER presets** are 96 slots and keep the track's FX. At the first start of 1.0.0, used slots 97 to 128 move into the first empty slots (`3 PRESETS LOST` if all are full); a project that used a moved preset finds it; one that used a lost preset shows `PRESET -` with its sound unchanged. Presets saved earlier hold no FX and load as the sound only until saved over.
- **User sample slots** USR2 and USR3 hold 48 KiB each: the rest of their flash holds the projects' variations and lock banks (see Sounds).
- Backups made with the web editor hold what is saved on the FM-1 (see Web editor).
