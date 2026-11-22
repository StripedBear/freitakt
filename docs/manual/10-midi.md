# MIDI
<!--blurb: MIDI in over USB and TRS, clock in and out, routing, THRU and MIDI LEARN.-->

## Connections

- **USB MIDI** appears as **Hortator** (earlier versions: Felucca) in a DAW, AUM or Audio MIDI Setup. While an update is being written it appears as **Felucca Update**.
- **TRS jack**: MIDI in only. The FM-1's TRS jack is an input, so there is no TRS out.
- **Bluetooth MIDI** in the Bluetooth version (see that section).

## MIDI settings

| Setting | Where |
| --- | --- |
| **CH** (drum channel, 10 by default) | menu (hold [[HOME]]): MIDI CHANNEL |
| **CLK** (INT / USB / TRS, BLE, LINK) | menu: CLOCK |
| **SYNC** (clock out) | menu: MIDI SYNC |
| **ROUT** (note routing) | menu: MIDI ROUTE |
| **MUTE** (NOW / NEXT BAR) | menu: MUTE MODE |
| **METRO** | menu: METRONOME |
| **THRU** | menu: MIDI THRU |
| **QUANT** (live-record grid) | menu: REC QUANT |
| **OUT** (the sequencer's notes out) | GLO: MIDI OUT |
| **TnCH** (a track's MIDI OUT channel) | GLO: MIDI OUT |
| **KBD** (the MIDI KEYBOARD: OFF, TRK, 1 to 16) | GLO: MIDI OUT |
| **LOCL** (the MIDI KEYBOARD's keys sound here too) | GLO: MIDI OUT |

SYNC, ROUT, OUT and the tracks' channels are saved with each project; projects saved before them load SYNC OFF, ROUT NOTE, OUT USB and track n on channel n. THRU is a setting of the FM-1 itself, kept over power-off, OFF until you turn it on.

## ROUT: how MIDI notes in play the tracks

Applies to notes from USB, the TRS jack and, in MIDI mode, Bluetooth.

| ROUT | Notes |
| --- | --- |
| **NOTE** (default) | On the drum channel (CH), each note plays the tracks whose NOTE it is, at their tuning. |
| **SEL** | On the drum channel, every note plays the selected track, pitched: C4 (note 60) is its tuning, a semitone per note. |
| **TRACK** | MIDI channels 1 to 6 play tracks 1 to 6, pitched from C4 as SEL (CH plays no part; channels 9 to 16 do nothing). |

Note-offs are ignored (a hit is a hit; an ACID note ends after its GATE). A note into an armed track while playing is recorded, with its pitch on SEL and TRACK. The white keys send their notes to USB: on the drum channel with the track's NOTE, and with ROUT TRACK on the track's own channel at C4 (so a recording in a DAW plays back the same tracks).

The power-on kit notes are: kick 36, snare 38, clap 39, closed hat 42, open hat 46, tom 45, rim 37, cymbal 49 (the last two have no track now).

## CLK: following a clock

CLK chooses where the tempo comes from: **INT** (the FM-1's own), **USB**, **TRS** (or **BLE** in the Bluetooth version, and **LINK** for FM-Link). While following, the tempo knobs say `CLK USB` / `CLK TRS` / `CLK BLE`. Following **CLK USB**, a USB reconnect pauses the clock (`CLK USB PAUSED`); the steps wait up to 2 s for it to come back. A project saved with CLK BLE or CLK LINK loads with CLK INT in the plain version.

## SYNC: MIDI clock out

Set SYNC to OFF or USB (and BLE or BOTH in the Bluetooth version).

- **On its own clock (CLK INT)**: PLAY sends Start, then 24 clocks per quarter note; STOP sends Stop. PLAY always starts from the top, so it never sends Continue or a song position. The clocks keep to the sequencer's grid through tempo changes and never swing, whatever SWING says. Each clock is sent when the step it goes with is heard; if the receiving DAW plays slightly early, give its sync a few ms of delay.
- **Following a clock (CLK USB / TRS / BLE)**: incoming Clock, Start, Continue and Stop go on to the SYNC outputs as they arrive, never back out of the port they came in on.
- **Timing**: within a quarter of a millisecond with a light kit; a busy kit adds up to half the render time (about 1.5 ms when the CPU shows 50 %). Bluetooth MIDI is much looser: use it for tempo, not tight sync.
- If the computer does not read the FM-1's MIDI port, the clock waits no more than 8 messages deep. During a firmware update (and 3 s after) nothing is sent.

## MIDI OUT: the sequencer's notes

Tap [[GLO]] for **MIDI OUT**, its first page. While playing, every track sends its hits as Note On / Note Off, track n on its own channel, so a DAW can record the pattern or play other instruments with it.

| Knob | Label | Values (default) |
| --- | --- | --- |
| **KNOB 1** | OUT | OFF, USB (default); BLE and BOTH in the Bluetooth version |
| **KNOB 2** | T1CH .. T6CH | the selected track's channel: OFF or 1 to 16 (track n on channel n) |

KNOB 2 sets the channel of the selected track (selected as on any page); OFF silences that track's notes only.

- **What is sent**: the sequencer's hits, as the sound engine plays them: every step, RATCH, NUDGE, swing, FIT and MASTER LENGTH, and ARP / RTRG's repeats. The keys' notes go out as before (see *ROUT*), their live ARP / RTRG repeats do not; REPEAT STEP, MIDI in and THRU are not sent again as MIDI OUT notes.
- **The note**: an ACID, SAMPL, FM or SKIN track sends the note it plays (the step's NOTE, with ARP1's chord and octaves). A CHORD track sends its chord's notes (up to four, on and off together), or with ARP1 the one note each hit plays. A drum track sends its **NOTE** knob, the kit's GM note (36 kick, 38 snare, 39 clap, 42 closed hat ...).
- **Velocity**: 127 on an accented step, else 96; ARP / RTRG's FADE lowers its repeats.
- **Note Off**: ACID when its gate closes (a SLIDE plays legato: the new note before the old one's Off); other tracks after the step (or the RATCH / repeat). One note a track at a time.
- **No hanging notes**: STOP, PLAY again, muting a track or soloing another sends the Note Offs at once; if a Note Off was lost, STOP also sends All Notes Off (CC 123) on the channels played.
- **Timing** is the SYNC clock's (see above): each note leaves when it is heard. If the computer does not read the port, notes are dropped (and counted), never delayed; the clock and Note Offs keep their room. During a firmware update (and 3 s after) nothing is sent; notes cut by it get All Notes Off afterwards.

The web editor can also write a pattern or a chain as a MIDI file (*Web editor*, Patterns, **Export MIDI…**).

## MIDI KEYBOARD

The FM-1 as a plain MIDI keyboard for a DAW or a phone app: each of the 27 keys sends its own Note On / Note Off.

| Knob | Label | Values (default) |
|---|---|---|
| [[KNOB 3]] | KBD | OFF (default), TRK, 1 to 16 |
| [[KNOB 4]] | LOCL | ON (default), OFF |

Both are on **MIDI OUT** and saved with the device settings (not in projects).

- **On**: turn KBD away from OFF. KEYS turns on with it, and HOME's status line reads `MIDI KBD CH1 OCT+1` (channel and octave).
- **Off**: KBD back to OFF, or turn KEYS off (a SEL tap with no key pressed, or ENV). With KEYS on again and KBD not OFF, it is the MIDI KEYBOARD again.
- **Channel**: TRK sends on the selected track's MIDI OUT channel (T*n*CH; `CH-` on HOME and nothing sent if it is OFF), 1 to 16 a fixed channel.
- **Notes**: F3 (the leftmost key) is note 53 at octave 0. [[OCT-]] / [[OCT+]] move the keys an octave down / up, -4 to +4 (notes above 127 are clamped). The OCT buttons light the way they do in KEYS.
- **Velocity**: always 100. The keys are on/off switches; they don't sense velocity.
- **Where the notes go**: USB MIDI out, and in the Bluetooth version also the Bluetooth MIDI device (ROLE DEVICE) while it is linked. This doesn't depend on OUT.
- **LOCL**: ON, the keys also play the selected track as in KEYS. OFF, they send MIDI only and play nothing here.
- **The sequencer** keeps running and keeps sending its own MIDI OUT notes. Step editing is off in this mode: no live recording of the keys, and the white keys do nothing with SEQ (SEQ + black key still mutes). The other combos and pages work as usual.
- **No hung notes**: a key's Off always goes out on the channel and note its On used, even if the octave or KBD changed while the key was held. Leaving the mode sends an Off for every held key. If the Bluetooth link drops, every held key's Off goes out at once, and those keys stay silent until pressed again. While a firmware update runs, nothing is sent. If an Off cannot be sent (full queue), an All Notes Off follows on that channel once no key is held.
- **The knobs** don't send CCs in this mode. To send CCs from the knobs, use MIDI LEARN.

## THRU

**MIDI THRU** ON (in the menu) passes notes and CCs from each MIDI input to the other outputs (the TRS jack to USB; Bluetooth and USB to each other in the Bluetooth version), never back to the input.

## MIDI LEARN

Up to **32** controls of any controller. A learned CC moves one knob of one track; a CC or a note presses PLAY, REC, next / previous track, or holds a PERFORM effect.

| Learn | How to pick it (LEARN on) | What it takes | What it does |
| --- | --- | --- | --- |
| a knob of a track | turn the knob (it edits as usual) | a CC | moves that knob of that track, 0..127 over its range |
| PLAY | press PLAY (nothing plays) | a CC or a note | a press = a PLAY tap: play / stop |
| REC | press REC (nothing arms) | a CC or a note | a press = a REC tap: TRACKS arms the selected track, elsewhere REC ALL |
| NEXT TRACK / PREV TRACK | turn ALGORITHM right / left | a CC or a note | selects the next / previous track |
| a PERFORM effect | hold FX, press its black key | a CC or a note | held while the controller's button is held (FX need not be held) |
| a track's REPEAT STEP | hold FX, press its white key F3..D4 | a CC or a note | the track repeats its step while held, as the key (a control learned to a PERFORM mute in an older version does this now) |
| a PERFORM macro | on the PERFORM screen, turn KNOB 1..4 | a CC | FILTER (0 at 64: left LPF, right HPF), CRUSH, THROW, DEPTH; stays on |

### How to learn a control

1. **Turn it on:** tap [[SAVE]] to its **COPY** page (3/3) and turn [[KNOB 4]] (**LEARN**) to `ON`: `MIDI LEARN ON` pops up. Go to the page you want; the top bar shows `LEARN: TURN OR PRESS`.
2. **Pick a knob:** turn a knob of the selected track on any page: EDIT (SOUND), FX (FILTER, the sends, INSERT and SRR), LFO, PATTERN, or LEVEL / LEN / PAN on TRACKS. It edits as usual and the top bar says `T3 DECAY: SEND A CC`.
3. **Or pick an action:** while LEARN is on, PLAY, REC, ALGORITHM and the PERFORM keys pick their action instead of doing their job. The top bar says `PLAY: SEND CC/NOTE`, `FX R1/16: SEND CC/NOTE`, `FX RPT T3: SEND CC/NOTE`.
4. **Move the controller's knob, or press its button or pad.** The first CC (or, for an action, note) that arrives on any channel is learned: `CC74 -> T3 DECAY`, `NOTE36 -> PLAY`, `CC20 -> FX FILTER`. Then pick the next one. An already learned one shows `T3 DECAY = CC74`; sending another replaces it.
5. **Finish:** [[OCT+]] ends LEARN (`LEARN DONE`), as turning COPY's LEARN knob back to `OFF` does.

### Rules

- **The binding is to that track:** CC74 moves track 3's DECAY whichever track is selected later.
- **One control, one thing:** learning again moves it to the new pick. `LEARN FULL` after 32. CC 0 and 32 (bank select) and 120 to 127 (channel mode) are never learned (`CC NOT LEARNABLE`). MODEL cannot be learned (`NOT LEARNABLE`).
- **While LEARN is on, [[OCT-]] clears** the pick's CC or note (`T3 DECAY CC CLEARED`, `PLAY CLEARED`); with nothing picked it asks `CLEAR ALL LEARNED?` ([[OCT+]] yes, [[OCT-]] no). So pick the page first (the step bank, a DEST slot), then turn LEARN on. The menu, a confirm, or NAME also end it. EDIT held does nothing during LEARN. It is always off at power-on.
- **What a CC does on a knob:** 0 to 127 spans the knob's whole range; a centred knob (PAN, an LFO's DEPTH, FILTER ENV) is 0 at 64; a list (FILTER TYPE, DIV, WAVE) steps through it in order. A model's knob follows the track's model. An LFO on the same knob keeps moving it around the new value. CC moves are not recorded as motion.
- **Buttons:** a CC of 64 or more is a press, under 64 a release (only the change counts); a note on is a press, its note off (or velocity 0) the release. A learned note plays no drum. Learned names read like `T3 I.DRIVE`, `T3 SRR`, `T3 L1.RATE`, `T3 L.SLOT`.
- Learned CCs and notes act whether LEARN is on or off, on every input except Bluetooth in SMC DAW mode. PLAY, REC and track buttons act where the FM-1's own would (not in the menu or a confirm).
- They are saved in the FM-1's flash (**not in projects**) after each change, when stopped. The 16 CCs of the first MIDI LEARN load as they were.
