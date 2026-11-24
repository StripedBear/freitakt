# FREITAKT: the text of the English user manual (HTML fragments), built by tools/make_manual_html.py --lang en.
# A translation of tools/manual_ru_content.py: same structure, same screenshots, same markers.
# Markers: {{PANEL}} the panel schematic; {{shot:FILE|caption}} a screenshot; {{screen:FILE|caption}} ... {{/screen}}
# a screenshot with its description at the right. FILE is relative to build/ui_shots.

CONTENT = r'''
<h2 id="intro">1. FREITAKT in brief</h2>

<p><b>FREITAKT</b> is a fork of the <b>Hortator</b> firmware for the <b>M-VAVE FM-1</b> groovebox, slimmed down and reworked.
Hortator is by DEADACTIVE and is built on <b>Felucca</b> by Leo Kuroshita (kurogedelic, Hugelton Instruments);
FREITAKT, like both of them, is free software under the <b>GPL-3.0</b> licence.</p>

<p>FREITAKT turns the FM-1 into a drum machine in the spirit of the Elektron Model:Cycles: <b>6 tracks</b>, each with its own
<b>machine</b> (sound model), filter, insert effect, sends, LFO and arpeggiator/retrigger. The sequencer holds
<b>4 patterns (A–D) × 8 variations</b>. Every track has its own length (up to 64 steps) and its own step size, including
FIT divisions for polyrhythms. On top of that there is a global Master Length, pattern chains,
<b>parameter locks</b> and <b>machine locks</b> on steps, live recording, MIDI OUT, a MIDI keyboard mode and Bluetooth MIDI.</p>

<p>The terms in this manual are taken from the Model:Cycles: <b>pattern</b> (A–D), <b>variation</b> (1–8),
<b>track</b> (T1–T6), <b>step</b>, <b>parameter lock</b> (a knob value on a single step),
<b>machine lock</b> (a different machine on a single step). On-screen labels are quoted as they appear, in a monospaced font:
<code>LEN</code>, <code>CLEAR T1</code>.</p>

<div class="note"><b>Notation.</b> <kbd>SEQ</kbd> is a button, <kbd class="k">KNOB 1</kbd> is a knob or key.
“<kbd>SAVE</kbd> + <kbd>REC</kbd>” means “hold SAVE and press REC”.
“White 1–16” are the white keys from left to right (F3 … G5), “black 1–11” the black keys from left to right (F#3 … F#5).
“Tap” is a short press, “hold” means press and keep holding (the time is given where it matters).</div>

<h3 id="panel">Panel map</h3>
{{PANEL}}
<p>The schematic is simplified: positions are approximate, labels are as printed on the FM-1 panel.</p>

<table>
<thead><tr><th>Control</th><th>What it does</th></tr></thead>
<tbody>
<tr><td><kbd class="k">MASTER</kbd></td><td>Volume (speaker, headphones). An analogue knob; the firmware does not read it.</td></tr>
<tr><td><kbd class="k">SELECT</kbd> (encoder)</td><td>Pages forward/back through the current section, stopping at the ends.
On TRACKS it sets the tempo. In the menu, the SAVE carousel and the preset list it scrolls through the items.</td></tr>
<tr><td><kbd class="k">ALGORITHM</kbd> (encoder)</td><td>With KEYS on (SEL lit) it selects track T1–T6 on any page. With KEYS off it sets the <b>NOTE</b> of the selected track, like the NOTE knob on SOUND 1: with steps held it sets their notes, without them the track note (which new steps get). A popup shows <code>NOTE C#4</code> / <code>STEP 5 NOTE C#4</code>; <kbd>OCT-</kbd> cancels. A track can always be selected with <kbd>SEL</kbd> + black 1–6. In MIDI LEARN: NEXT / PREV TRACK.</td></tr>
<tr><td><kbd class="k">PRESETS</kbd> (encoder)</td><td>Each click loads the next or previous preset into the selected track; the full-screen page is replaced by the preset list, and the page comes back 3 s after the last click.</td></tr>
<tr><td><kbd class="k">KNOB 1–4</kbd></td><td>The four rows of the current page, top to bottom.</td></tr>
<tr><td><kbd>HOME</kbd></td><td>Tap: HOME ⇄ TRACKS screen (from any other page, whichever of the two was last). Hold 0.7 s: device menu.</td></tr>
<tr><td><kbd>EDIT</kbd></td><td>The selected track's sound: the SOUND pages (ARP has its own button). Acts on press. Hold 0.4 s: preset list.</td></tr>
<tr><td><kbd>FX</kbd></td><td>Tap: filter, send, insert and compressor pages. Hold 0.4 s: PERFORM.</td></tr>
<tr><td><kbd>LFO</kbd></td><td>LFO pages and its destinations (DEST).</td></tr>
<tr><td><kbd>SEQ</kbd></td><td>PATTERN page (LEN / DIV / SWG / MLEN). Hold + black 1–6: mute.</td></tr>
<tr><td><kbd>GLO</kbd></td><td>Tap: global pages (MIDI OUT, delay, reverb, chorus). Hold + keys: pattern or variation change, chain.</td></tr>
<tr><td><kbd>SAVE</kbd></td><td>Tap: PROJECT / TOOLS / COPY carousel. Hold: pattern and variation switching map; track copy.</td></tr>
<tr><td><kbd>ENV</kbd></td><td>Envelope page of the selected track: the machine's ATTACK / DECAY (on SAMPL: ATK DECAY SUS REL) and FDEC, the filter envelope decay (see <a href="#env">ENV</a>). Pressing again goes to the next ENV page, if there is one.</td></tr>
<tr><td><kbd>SEL</kbd></td><td>Tap: KEYS (lit: the keyboard plays the track chromatically) ⇄ STEPS (unlit: the white keys are the selected track's steps on any page). Hold + black 1–6: track select.</td></tr>
<tr><td><kbd>ARP</kbd></td><td>Short press (on release, with no keys, knobs or other buttons touched): the selected track's ARP 1 page; again — ARP 2 (drums don't have it, so it stays on ARP 1). While an ARP page is open the ARP LED is lit. + black 1–6: solo, + F#5: solo mode; with KEYS on (SEL lit) + a white key: live retrigger of that key's note (this press does not open the page).</td></tr>
<tr><td><kbd>PLAY</kbd> / <kbd>REC</kbd></td><td>Start/stop. REC: recording (see section 6); hold 0.4 s: FILL.</td></tr>
<tr><td><kbd>OCT-</kbd> / <kbd>OCT+</kbd></td><td>Depend on the screen: step bank (on PATTERN and TRACKS), octave, “back/yes”, undo an edit (see section 3).</td></tr>
<tr><td>White 1–16</td><td>With KEYS off: the selected track's steps (the 16 steps of the bank shown) on any page.</td></tr>
<tr><td>Black 1–11</td><td>In combos: 1–6 are tracks, 1–8 are variations. On PERFORM: effects.</td></tr>
</tbody></table>

<h3 id="splash">Splash and start screen</h3>
{{screen:fixes/splash_3_name.png|Splash screen}}
<p>After power-on the <code>FREITAKT</code> splash is shown for about 3 s with the caption
<code>FORM FOLGT FUNCTION</code>. Any key, button or knob skips it and does nothing else.</p>
<p>Then the last saved pattern is loaded, with the transport stopped, on the TRACKS screen.
If the pattern could not be loaded, a prompt appears: <code>LOAD FAILED: SLOT A</code> / <code>START NEW PROJECT?</code>:
<kbd>OCT+</kbd> is yes, <kbd>OCT-</kbd> is no.</p>
<p>If you hold <kbd>SEQ</kbd> at power-on, the firmware starts in safe mode (see <a href="#safety">section 8</a>).</p>
{{/screen}}

<h2 id="quick">2. Quick start</h2>
<ol>
<li><b>Power on</b> the FM-1. After the splash, the TRACKS screen opens with a list of the six tracks.</li>
<li><b>Select a track</b>: <kbd>SEL</kbd> + black 1–6, or <kbd class="k">ALGORITHM</kbd> with KEYS on,
or turn <kbd class="k">KNOB 1</kbd> on TRACKS. The selected track is marked with <code>&gt;</code>.</li>
<li><b>Enter steps.</b> Press <kbd>SEQ</kbd> to open the PATTERN page. The white keys are the selected track's 16 steps:
a tap turns a step on or off. A long track (LEN &gt; 16) is handled in banks: <kbd>OCT+</kbd> / <kbd>OCT-</kbd>.
You can do the same directly on TRACKS as long as no track is armed for recording.</li>
<li><b>Start</b>: <kbd>PLAY</kbd>. Press again to stop.</li>
<li><b>Change the machine.</b> <kbd>EDIT</kbd> opens the first sound page, <kbd class="k">KNOB 1</kbd> (<code>MODEL</code>) selects the machine,
<kbd class="k">KNOB 2</kbd> is the track NOTE, <kbd class="k">KNOB 3</kbd> is PITCH; DECAY is on the <kbd>ENV</kbd> page. Quicker: turn <kbd class="k">PRESETS</kbd>, each click is a new preset.</li>
<li><b>Set up a step.</b> Go back to <kbd>SEQ</kbd>, hold a step's white key and turn the knobs:
<kbd class="k">KNOB 1</kbd> PROB (probability or condition), <kbd class="k">KNOB 2</kbd> RATCH (ratchet 1–4),
<kbd class="k">KNOB 3</kbd> ACC (accent), <kbd class="k">KNOB 4</kbd> NUDGE (micro-timing shift).
An accent can also be set with <kbd>OCT+</kbd> + step. The step's note is set with <kbd class="k">ALGORITHM</kbd> (KEYS off).</li>
<li><b>Tempo and swing.</b> <kbd>HOME</kbd> switches to the HOME screen: <kbd class="k">KNOB 1</kbd> is BPM, <kbd class="k">KNOB 2</kbd> the global swing.</li>
<li><b>Save.</b> Stop playback, tap <kbd>SAVE</kbd> (the <code>PROJECT 1/3</code> page),
choose the pattern letter with <kbd class="k">KNOB 1</kbd> and turn <kbd class="k">KNOB 4</kbd> two clicks in a row
(the first shows <code>AGAIN: SAVE</code>).</li>
</ol>
<div class="warn"><b>There is no autosave.</b> Edits live in memory until <kbd>SAVE</kbd>. After power-off
the last saved version is loaded. You cannot save during playback: <code>STOP TO SAVE</code>.</div>

<h2 id="screens">3. Screens</h2>

<h3 id="rules">General rules</h3>
<ul>
<li><b>List pages.</b> Almost every screen consists of four <code>NAME value</code> rows, one per knob,
<kbd class="k">KNOB 1–4</kbd> from top to bottom. There is no page title or page number on screen: you recognise a page by its rows.
The numbers in this manual (“FILTER 1”) are only there for orientation.</li>
<li><b>Navigation.</b> Pressing a section button again opens the next page, cycling round.
<kbd class="k">SELECT</kbd> pages forward/back without wrapping. When you return to a section, the page you were last on opens.
<kbd>EDIT</kbd> and <kbd>LFO</kbd> act on press; <kbd>FX</kbd>, <kbd>GLO</kbd>, <kbd>SAVE</kbd> and <kbd>SEQ</kbd> act on release
(so they can be held for combos).</li>
<li><b>Knob popup.</b> When you turn a knob, the whole screen shows the parameter name and a large value
(a lock value and the note of a held step are shown inverted). The popup closes 3 s after the last turn.
While it is open, <kbd>OCT-</kbd> is <b>undo</b>: the parameter returns to its value before you started turning (for NOTE, the notes of all held steps).</li>
<li><b>White keys</b> with KEYS off (SEL unlit) are the selected track's steps on any page: HOME, TRACKS (while nothing
is armed), EDIT, FX, LFO, GLO, SAVE. A tap turns a step on, holding a step + a knob sets a lock (on PATTERN and TRACKS: PROB / RATCH / ACC / NUDGE),
and step copy works. The keys keep their own function in the menu, NAME, the preset list (the keys play), PERFORM, combos with a held button,
TRACKS with an armed track (the keys record hits) and COMP (the DUCK toggles).</li>
<li><b>Bank on the keys.</b> On PATTERN and TRACKS the bank is chosen with <kbd>OCT-</kbd>/<kbd>OCT+</kbd>. On the other pages, during playback,
the keys show the bank where the selected track's playhead currently is; while a step is held the bank does not change (so a lock or a copy doesn't “slide away”).
When stopped, it is the last selected bank. There, <kbd>OCT-</kbd>/<kbd>OCT+</kbd> keep their own functions (undo in the knob popup, RATE MODE on LFO).</li>
<li><b>Top line</b> (on list pages): the battery on the right.
On the left, the record icon: filled means the selected track is armed, a ring means another track is armed.
Then text: the MIDI LEARN line, the switching map <code>CUE A1*</code>, <code>FILL</code> or <code>LOCK STEP n</code>.</li>
</ul>

{{shot:popup.png|Knob popup}}

<h3 id="home">HOME</h3>
{{screen:00_home.png|HOME}}
<p><b>How to open:</b> tap <kbd>HOME</kbd>. If TRACKS is open, tap <kbd>HOME</kbd> once more.</p>
<p>A minimal screen for playing: CPU load <code>CPU</code> and battery at the top, the tempo <code>BPM 120</code> in large type,
and the status line <code>P1 V2 T6 L1</code>:</p>
<ul>
<li><code>P</code> — pattern (1–4 = A–D; <code>P-</code> means nothing is loaded);</li>
<li><code>V</code> — variation;</li>
<li><code>T</code> — selected track;</li>
<li><code>L</code> — the step bank on the keys.</li>
</ul>
<p>While a chain is playing, a line <code>CH A1 B3 …</code> is shown below (up to 6 links): the current link is inverted, the next one underlined.</p>
<p>With external clock, the source and the measured tempo are shown instead of <code>BPM</code>: <code>USB 120</code>, <code>TRS 98</code>.</p>
<table><thead><tr><th>Control</th><th>Action</th></tr></thead><tbody>
<tr><td><kbd class="k">KNOB 1</kbd></td><td>BPM 40–240. With external clock it shows <code>CLK USB</code> etc. and the tempo doesn't change.</td></tr>
<tr><td><kbd class="k">KNOB 2</kbd></td><td>Global SWING 0–100 % (visible only in the popup).</td></tr>
<tr><td><kbd class="k">KNOB 3/4</kbd>, <kbd class="k">SELECT</kbd></td><td>Nothing.</td></tr>
<tr><td>White 1–6</td><td>Select and play track 1–6.</td></tr>
</tbody></table>
{{/screen}}
<div class="shots">{{shot:popup_home_bpm.png|HOME: turning KNOB 1}}{{shot:home_clk_usb.png|HOME with clock over USB}}</div>
<p>In MIDI keyboard mode the HOME status line changes to <code>MIDI KBD CH1 OCT+1</code> (see <a href="#midikbd">section 6</a>).</p>

<h3 id="tracks">TRACKS (mixer)</h3>
{{screen:24_tracks.png|TRACKS}}
<p><b>How to open:</b> tap <kbd>HOME</kbd> on the HOME screen. This screen opens after power-on.</p>
<p>Six rows, one per track: the number, a record mark (a square: filled while playing, outlined while stopped),
the machine and, small on the right, the preset name (<code>*</code> means the preset has been modified). The selected track is marked <code>&gt;</code>.
A silenced track (MUTE, LEVEL 0 or another track's solo) is shown inverted.</p>
<table><thead><tr><th>Control</th><th>Action</th></tr></thead><tbody>
<tr><td><kbd class="k">KNOB 1</kbd></td><td>Track select.</td></tr>
<tr><td><kbd class="k">KNOB 2</kbd></td><td>Track LEVEL. The first turn on a muted track only unmutes it.</td></tr>
<tr><td><kbd class="k">KNOB 3</kbd></td><td>Track LEN (steps).</td></tr>
<tr><td><kbd class="k">KNOB 4</kbd></td><td>PAN.</td></tr>
<tr><td><kbd class="k">SELECT</kbd></td><td>Tempo (BPM), with acceleration.</td></tr>
<tr><td>White 1–16</td><td>The selected track's steps, as long as no track is armed for recording.</td></tr>
<tr><td><kbd>OCT-</kbd> / <kbd>OCT+</kbd></td><td>Previous / next step bank (if no track is armed).</td></tr>
<tr><td><kbd>REC</kbd> (tap)</td><td>Arm / disarm recording on the selected track.</td></tr>
</tbody></table>
{{/screen}}
{{shot:tracks_muted.png|TRACKS: T3 muted, T2 selected}}

<h3 id="sound">EDIT: the SOUND pages</h3>
<div class="shots">{{shot:01_sound.png|SOUND 1 (K909)}}{{shot:02_sound_2.png|SOUND 2}}{{shot:03_sound_3.png|SOUND 3}}{{shot:93_sound_kboom.png|SOUND 1 of the BOOM machine}}</div>
<p><b>How to open:</b> <kbd>EDIT</kbd>; further presses page through the SOUND pages (the ARP pages are not in this cycle). Holding <kbd>EDIT</kbd> 0.4 s opens the preset list.</p>
<p>At the top the track's hit waveform is drawn. The pages are a list of parameters, four per page:
<code>MODEL</code>, <code>NOTE</code>, the machine's knobs (the first is <code>PITCH</code>; the envelope knobs, DECAY and ATTACK, live on the ENV page), then
<code>LVL</code> (volume), <code>PAN</code>, <code>MNOTE</code> (the track's MIDI note, formerly called NOTE), <code>CHOKE</code> (choke group OFF/1–4)
and <code>PRIO</code> (voice priority HIGH/LOW). The number of pages depends on the machine; SAMPL has five
(5: PLAY CUT LOOP).</p>
<h3 id="env">ENV: the envelope page</h3>
<div class="shots">{{shot:env/c808.png|ENV (drum)}}{{shot:env/sampl_1.png|ENV 1 (SAMPL)}}{{shot:env/sampl_2.png|ENV 2 (SAMPL)}}{{shot:env/chord.png|ENV (CHORD)}}</div>
<p><b>How to open:</b> <kbd>ENV</kbd>; pressing again goes to the next ENV page, if there is one (the ENV LED is lit).
The selected track's envelope knobs are gathered into one list (same knob popup, locks, MOTION and MIDI LEARN; the sound is unchanged, only the knobs moved):</p>
<table><thead><tr><th>Machine</th><th>ENV 1</th><th>ENV 2</th></tr></thead><tbody>
<tr><td>drums, FM, TRX, ACID</td><td><code>DECAY</code>, <code>FDEC</code></td><td>—</td></tr>
<tr><td>SAMPL</td><td><code>ATK</code>, <code>DECAY</code>, <code>SUS</code>, <code>REL</code></td><td><code>FDEC</code></td></tr>
<tr><td>CHORD</td><td><code>ATTACK</code>, <code>DECAY</code>, <code>FDEC</code></td><td>—</td></tr>
</tbody></table>
<p><code>FDEC</code> is the decay of the filter envelope (5 ms – 2 s, default 40 ≈ 32 ms), by which the ENV knob on the FILTER page moves the cutoff.
If you change the machine while on ENV 2 and the new machine has no second page, the screen returns to ENV 1.</p>
<p><code>NOTE</code> (<kbd class="k">KNOB 2</kbd> on SOUND 1) is shown as a note name (<code>C#4</code>, from C-1 to G9).
With no steps held it is the track note: every newly enabled step gets it, on all machines, drums included (they play it
transposed relative to C4). With steps held (KEYS off, any page) the knob writes the note into all held steps at once. The track note
can also be set with a key in KEYS mode; after LOAD and after clearing the pattern it is C4 again. It is not a sound knob: no locks, motion, presets or MIDI LEARN.</p>
<p><code>MODEL</code> shows the machine and the preset name. Turning <kbd class="k">KNOB 1</kbd> cycles through the 46 machines:</p>
<p class="mono">BDTRX SDTRX TMTRX CGTRX CPTRX RSTRX CBTRX CHTRX OHTRX CYTRX SHTRX CLTRX B2TRX ·
BDEFM SDEFM TMEFM CPEFM RSEFM CBEFM HHEFM CYEFM · BOOM PUNCH SNAP CRACK METAL HISS SKIN FM ACID CHORD SAMPL ·
K808 K909 S808 S909 C808 C909 HATC HATO CYMB COWB TOM CONGA RIM CLAVE</p>
<p>On every machine the first knobs are <code>PITCH</code> (the machine's tuning, −24…+24 semitones, shown with a sign; formerly TUNE) and <code>DECAY</code>. White 1–6 select the track.
With a step held (KEYS off) the knobs set a parameter lock, and <code>MODEL</code> sets a machine lock (see <a href="#locks">section 4</a>).</p>
<p><b>CHORD</b> is a polyphonic chord machine (like CHORD on the Elektron Model:Cycles): up to four oscillators play a chord
from a note (PITCH + the step's NOTE, a key in KEYS mode or a MIDI note; C4 is the root at PITCH 0). The knobs in order: <code>PITCH</code>, <code>CHORD</code>,
<code>INV</code>, <code>DECAY</code>, <code>WAVE</code>, <code>TONE</code>, <code>ATTACK</code>, <code>DETUNE</code> (on SOUND 1: MODEL, NOTE, PITCH, CHORD).</p>
<ul>
<li><code>CHORD</code> is the shape: MAJ (0 4 7), MIN (0 3 7), 7 (0 4 7 10), MIN7 (0 3 7 10), MAJ7 (0 4 7 11), SUS2 (0 2 7), SUS4 (0 5 7),
DIM (0 3 6), 6 (0 4 7 9), 9 (0 4 10 14: root, third, seventh, ninth; the fifth is dropped to fit in 4 notes), OCT (0 12), 5TH (0 7).</li>
<li><code>INV</code> 0–3 is the inversion: each step moves the lowest note up an octave (MAJ on C with INV 1 gives E G C).</li>
<li><code>DECAY</code> is the fade after the gate closes, 5 ms … ~4 s; <code>ATTACK</code> is the rise, 1 ms … 2 s (it plays through to the end even on a short step).</li>
<li><code>WAVE</code> is SAW, SQR (anti-aliased) or SIN; <code>TONE</code> is a gentle low-pass filter (~100 Hz at 0, fully open at 127);
<code>DETUNE</code> slightly detunes the chord notes (up to ~30 cents on the outer ones); the root stays in tune.</li>
<li>The gate works like SAMPL with SUS: the chord is held for the step length (on a roll, the repeat GATE), through TIE steps and for as long as a key, pad or MIDI note is held; then DECAY.</li>
<li><b>One chord for the whole machine.</b> There is a single chord engine: a new CHORD hit from any track (its MODEL or a step's machine lock) replaces the sounding
chord with a short fade, without a click. The chord sounds through the track of the latest hit (its LEVEL, PAN, FILTER, INSERT, FX sends);
the other tracks play as usual. CPU load is never more than one 4-note chord (roughly one ACID voice).</li>
<li>With MODE: RTRG repeats the whole chord; ARP1 plays the chord notes one at a time (using the machine's own shape and INV; the CHORD knob on the ARP page is not used);
ARP2 on the KEYS plays a whole chord on each held key in turn. MIDI OUT sends the chord notes (up to four together), one at a time with ARP1.</li>
</ul>

<h3 id="presets">Presets</h3>
<div class="shots">{{shot:presets/06_list.png|Preset list (hold EDIT)}}{{shot:presets/01_popup.png|Preset screen (PRESETS encoder)}}</div>
<p><b>The <kbd class="k">PRESETS</kbd> encoder</b> on any page loads the next or previous preset into the selected track
and shows a separate full-display screen (the page underneath is hidden): bank and track at the top, the current preset
in large type on an inverted bar in the middle, three presets above and below it, machine names on the right; a dotted line marks a bank boundary.
The screen disappears 3 s after the last click (or immediately on another action: changing track, opening the list,
the menu), and the page is redrawn in full.</p>
<p><b>The preset list</b> (hold <kbd>EDIT</kbd> 0.4 s) has the banks MD / MODERN / CLASSIC / USER (96 user slots):</p>
<table><thead><tr><th>Control</th><th>Action</th></tr></thead><tbody>
<tr><td><kbd class="k">KNOB 1</kbd> / <kbd class="k">PRESETS</kbd></td><td>Cursor; the selected preset plays straight away (audition).</td></tr>
<tr><td><kbd class="k">KNOB 2</kbd> / <kbd class="k">SELECT</kbd></td><td>Bank.</td></tr>
<tr><td><kbd class="k">ALGORITHM</kbd>, white 1–6</td><td>Track.</td></tr>
<tr><td><kbd>OCT+</kbd></td><td>LOAD — load into the track.</td></tr>
<tr><td><kbd>SAVE</kbd></td><td>Save the track's sound to a new USER slot; pressing again within 1.5 s overwrites the selected one.</td></tr>
<tr><td><kbd>OCT-</kbd> twice</td><td>Delete a user preset.</td></tr>
<tr><td><kbd>EDIT</kbd> / <kbd>HOME</kbd></td><td>Exit. <kbd>PLAY</kbd> works as usual.</td></tr>
</tbody></table>
<h3 id="arp">ARP: the ARP pages (arpeggiator and retrigger)</h3>
{{screen:04_arp.png|ARP 1}}
<p><b>How to open:</b> press <kbd>ARP</kbd> briefly (it acts on release). Pressing <kbd>ARP</kbd> again on ARP 1 opens ARP 2
(drums have no ARP 2, so it stays on ARP 1); <kbd>SELECT</kbd> also pages ARP 1 / 2. While an ARP page is open the ARP LED is lit.
The ARP pages are no longer in the <kbd>EDIT</kbd> cycle.</p>
<table><thead><tr><th>Knob</th><th>ARP 1</th><th>ARP 2 (only on ACID, SAMPL, FM, SKIN, CHORD)</th></tr></thead><tbody>
<tr><td>1</td><td><code>MODE</code>: OFF / RTRG / ARP1 / ARP2 (ARP1 and ARP2 only on pitched machines)</td><td><code>DIR</code>: UP DOWN UPDN RND</td></tr>
<tr><td>2</td><td><code>RATE</code>: 1/4 4T 1/8 8T 1/16 16T 1/32 (default 1/16)</td><td><code>CHORD</code>: OCT 5TH MAJ MIN SUS4 MAJ7 MIN7 DOM7 (ARP2 doesn't use it)</td></tr>
<tr><td>3</td><td><code>GATE</code>: 0–100 % (50)</td><td><code>RANGE</code>: 1–4 octaves</td></tr>
<tr><td>4</td><td><code>FADE</code>: −64…+63 (repeats fade out or fade in)</td><td>—</td></tr>
</tbody></table>
{{/screen}}

<h3 id="fx">FX: filter, sends, insert, compressor</h3>
<p><b>How to open:</b> tap <kbd>FX</kbd> (it acts on release); further taps cycle through 6 pages.
Holding <kbd>FX</kbd> 0.4 s opens PERFORM (see below). Pages 1–4 belong to the selected track; COMP is global.</p>
<div class="shots">{{shot:06_filter.png|FILTER}}{{shot:07_fx.png|FX (sends)}}{{shot:08_insert.png|INSERT 1}}{{shot:09_insert_2.png|INSERT 2}}{{shot:10_comp.png|COMP 1}}{{shot:11_comp_2.png|COMP 2}}</div>
<table><thead><tr><th>Page</th><th>KNOB 1</th><th>KNOB 2</th><th>KNOB 3</th><th>KNOB 4</th></tr></thead><tbody>
<tr><td>FILTER</td><td><code>TYPE</code> OFF LP BP HP NOT</td><td><code>CUT</code> cutoff (up to 16 kHz)</td><td><code>RESO</code> 0–127</td><td><code>ENV</code> −64…+63 (envelope to cutoff)</td></tr>
<tr><td>FX (sends)</td><td><code>DST</code> distortion</td><td><code>CHO</code> chorus</td><td><code>DLY</code> delay</td><td><code>REV</code> reverb</td></tr>
<tr><td>INSERT 1</td><td><code>TYPE</code> OFF SOFT HARD FOLD FUZZ CRUSH PHASR FLANG CHOR</td><td colspan="3">A / B / C, names depend on TYPE:
SOFT/HARD/FOLD/FUZZ — DRIVE TONE LEVEL; CRUSH — BITS RATE LPF; PHASR/FLANG/CHOR — RATE DEPTH FDBK</td></tr>
<tr><td>INSERT 2</td><td><code>MIX</code> (100 %)</td><td><code>SRR</code> sample-rate reduction (OFF or a rate)</td><td>—</td><td>—</td></tr>
<tr><td>COMP 1</td><td><code>SRC</code> OFF / T1–T6, the ducking source track</td><td><code>THRSH</code></td><td><code>RATIO</code></td><td><code>REL</code></td></tr>
<tr><td>COMP 2</td><td><code>ATK</code></td><td><code>KNEE</code> HARD/SOFT</td><td><code>MKUP</code> makeup gain</td><td><code>GHOST</code> MUTE / KEEP / HIDE</td></tr>
</tbody></table>
<p>On the COMP pages white 1–6 toggle <b>DUCK</b> for tracks 1–6, i.e. which tracks “duck” under the source.
<code>GHOST</code> sets what a muted source does: MUTE — no ducking, KEEP — ducking continues,
HIDE — the source is never heard, it only pumps the others.</p>

<h3 id="perform">PERFORM</h3>
{{screen:perform/map.png|PERFORM}}
<p><b>How to open:</b> hold <kbd>FX</kbd> 0.4 s (or <kbd>FX</kbd> + a key/knob); the screen stays while <kbd>FX</kbd> is held.
The menu item <code>PERFORM</code> = <code>PAGE</code> keeps the screen open after release;
it closes with a tap on <kbd>HOME</kbd> or a page change.</p>
<table><thead><tr><th>Control</th><th>Action</th></tr></thead><tbody>
<tr><td><kbd class="k">KNOB 1–4</kbd></td><td>FILT (0 is off, left LPF, right HPF), CRUSH, THROW (throw into delay/reverb), DEPTH (with OCT UP/DN: SHIMR)</td></tr>
<tr><td>Black 1–10</td><td>REPEAT 1/8, 1/16, 1/32, REVERSE, TAPE STOP, LPF, HPF, FREEZE, OCT UP, OCT DN — while the key is held</td></tr>
<tr><td>White 1–6</td><td>REPEAT STEP: repeats the current step of track 1–6</td></tr>
<tr><td>White 13–16</td><td>Repeat rate 1/4, 1/8, 1/16, 1/32</td></tr>
<tr><td><kbd class="k">SELECT</kbd>, <kbd class="k">ALGORITHM</kbd>, <kbd class="k">PRESETS</kbd></td><td>Ignored</td></tr>
</tbody></table>
{{/screen}}

<h3 id="pattern">SEQ: PATTERN</h3>
{{screen:12_pattern.png|PATTERN}}
<p><b>How to open:</b> <kbd>SEQ</kbd> (on release). The SEQ section has one page.</p>
<table><thead><tr><th>Knob</th><th>Row</th><th>Values</th></tr></thead><tbody>
<tr><td>1</td><td><code>LEN</code></td><td>Track length 1–64 steps (16)</td></tr>
<tr><td>2</td><td><code>DIV</code></td><td>4BAR 2BAR 1/1 1/2 1/4 1/8 8T <b>1/16</b> 16T 1/32, then FIT: F4BR F2BR FBAR F1/2</td></tr>
<tr><td>3</td><td><code>SWG</code></td><td>Track swing 0–100 %</td></tr>
<tr><td>4</td><td><code>MLEN</code></td><td>Master Length: OFF or 2–1024 sixteenths (shared by the pattern)</td></tr>
</tbody></table>
<p>The white keys are the selected track's steps. A held step plus the knobs sets PROB / RATCH / ACC / NUDGE, and <kbd class="k">ALGORITHM</kbd> (KEYS off) sets the step's note
(see <a href="#steps">section 4</a>). <kbd>OCT-</kbd>/<kbd>OCT+</kbd> switch the bank.</p>
{{/screen}}
{{shot:seq/20_step_held_prob.png|Held step + KNOB 1 (PROB)}}

<h3 id="lfo">LFO</h3>
<div class="shots">{{shot:21_lfo.png|LFO 1}}{{shot:22_lfo_2.png|LFO 2 (page)}}{{shot:23_dest.png|DEST}}</div>
<p><b>How to open:</b> <kbd>LFO</kbd>; further presses page through 3 pages. Each track has one LFO with four destinations.</p>
<table><thead><tr><th>Page</th><th>KNOB 1</th><th>KNOB 2</th><th>KNOB 3</th><th>KNOB 4</th></tr></thead><tbody>
<tr><td>1 (with the waveform)</td><td><code>WAVE</code> SQUAR SAW RSAW SINE TRI S&amp;H WANDR EXP+ EXP- RWALK</td><td><code>RATE</code> (in SYNC: from 8BAR to 1/64, with dotted and triplet values)</td><td><code>MORPH</code></td><td><code>PHASE</code></td></tr>
<tr><td>2</td><td><code>TRIG</code> FREE / HIT / PLAY</td><td><code>MODE</code> SYNC / HZ / TIME</td><td>—</td><td>—</td></tr>
<tr><td>DEST</td><td><code>SLOT</code> destination 1–4</td><td><code>SRC</code> LFO1 / OFF</td><td><code>DEST</code> target</td><td><code>DEPTH</code> −64…+64</td></tr>
</tbody></table>
<p>DEST can be: a machine knob, LVL, PAN, F.CUT, F.RES, SRR, the DST/CHO/DLY/REV sends, and the insert's MIX and A/B/C.
On the LFO pages <kbd>OCT+</kbd> switches the RATE mode: SYNC → HZ → TIME. TRIG: FREE — free-running, HIT — restarts on every hit of the track,
PLAY — restarts on PLAY.</p>

<h3 id="glo">GLO: MIDI OUT, delay, reverb, chorus</h3>
<p><b>How to open:</b> a short tap on <kbd>GLO</kbd> (under 0.4 s, no keys); further taps page through 5 pages.
Holding <kbd>GLO</kbd> with keys controls patterns (see <a href="#patterns">section 5</a>).</p>
<div class="shots">{{shot:13_midi out.png|MIDI OUT}}{{shot:14_dly.png|DLY 1}}{{shot:15_dly_2.png|DLY 2}}{{shot:16_reverb.png|REVERB}}{{shot:17_chorus.png|CHORUS}}</div>
<table><thead><tr><th>Page</th><th>KNOB 1</th><th>KNOB 2</th><th>KNOB 3</th><th>KNOB 4</th></tr></thead><tbody>
<tr><td>MIDI OUT</td><td><code>OUT</code> OFF / USB (/ BLE / BOTH)</td><td><code>T1CH</code>…<code>T6CH</code> the selected track's channel, OFF/1–16</td><td><code>KBD</code> OFF / TRK / 1–16</td><td><code>LOCL</code> ON/OFF</td></tr>
<tr><td>DLY 1</td><td><code>TIME</code> (1/8; dotted and triplet values available)</td><td><code>FDBK</code></td><td><code>COLR</code></td><td><code>MIX</code></td></tr>
<tr><td>DLY 2</td><td><code>MODE</code> NORM / PING</td><td>—</td><td>—</td><td>—</td></tr>
<tr><td>REVERB</td><td><code>TYPE</code> ROOM / SPRNG / HALL</td><td><code>SIZE</code></td><td><code>DAMP</code></td><td>—</td></tr>
<tr><td>CHORUS</td><td><code>CRT</code> rate</td><td><code>CDP</code> depth</td><td>—</td><td>—</td></tr>
</tbody></table>
<div class="shots">{{shot:glo_delay_time_dotted.png|DLY: dotted time}}{{shot:glo_reverb_hall.png|REVERB: HALL}}</div>
<h3 id="save">SAVE: PROJECT, TOOLS, COPY</h3>
<p><b>How to open:</b> tap <kbd>SAVE</kbd>; further taps page through a carousel of three pages (<code>PROJECT 1/3</code>,
<code>TOOLS 2/3</code>, <code>COPY 3/3</code>). The carousel shows the current item in large type between arrows, with its value and a hint below it,
and button hints at the bottom. <kbd class="k">SELECT</kbd> cycles through the items.</p>
<p><b>Protection against accidents:</b> LOAD, SAVE, PASTE and all TOOLS actions need two knob clicks within about 1.5 s.
The first click shows <code>AGAIN: …</code>.</p>
<div class="shots">{{shot:18_project.png|PROJECT}}{{shot:19_tools.png|TOOLS}}{{shot:20_copy.png|COPY}}</div>
<table><thead><tr><th>Page</th><th>KNOB 1</th><th>KNOB 2</th><th>KNOB 3</th><th>KNOB 4</th></tr></thead><tbody>
<tr><td>PROJECT</td><td>Pattern A–D (shown as <code>SLOT</code> on screen); the value is the name, <code>EMPTY</code> or <code>USED</code></td><td><code>NAME</code>: turn right to rename</td><td><code>LOAD</code> ×2</td><td><code>SAVE</code> ×2</td></tr>
<tr><td>TOOLS</td><td>CLEAR TRACK: the track's steps, motion and locks</td><td>INIT SOUND: the track's default sound</td><td>CLEAR ALL: all tracks (steps, LEN, DIV, SWG)</td><td>INIT ALL: everything (saved patterns and settings are kept)</td></tr>
<tr><td>COPY</td><td>Pattern A–D</td><td><code>COPY</code></td><td><code>PASTE</code> ×2</td><td><code>LEARN</code> — MIDI LEARN on/off</td></tr>
</tbody></table>
<p>On the TOOLS page each action has its own knob: the first turn shows the action and <code>AGAIN:</code>, the second carries it out.</p>
<p><b>Prompts.</b> SAVE to a different letter while there are unsaved edits in memory asks <code>OVERWRITE B*?</code>;
LOAD with unsaved edits asks <code>DISCARD EDITS?</code>. <kbd>OCT+</kbd> is YES, <kbd>OCT-</kbd> is NO.</p>
<div class="shots">{{shot:save_project_overwrite.png|OVERWRITE?}}{{shot:save_project_discard.png|DISCARD EDITS?}}</div>
<p><b>Name</b> (<kbd class="k">KNOB 2</kbd> on PROJECT): the white keys enter letters by multi-tap; the black keys are F# ←, G# space, A# →, C# delete, D# ABC/123;
<kbd class="k">KNOB 1</kbd> moves the cursor, <kbd class="k">KNOB 2</kbd> changes the character; <kbd>OCT+</kbd> writes it, <kbd>OCT-</kbd> or <kbd>HOME</kbd> cancels.</p>

<h3 id="menu">Device menu</h3>
<div class="shots">{{shot:90_menu.png|MENU: COLOR}}{{shot:91_menu_clock.png|MENU: CLOCK}}</div>
<p><b>How to open:</b> hold <kbd>HOME</kbd> 0.7 s. To close: <kbd>OCT-</kbd> or hold <kbd>HOME</kbd> again.
<kbd class="k">SELECT</kbd> cycles through the items, <kbd class="k">KNOB 1</kbd> sets the value, <kbd>OCT+</kbd> steps it round.
Device settings are stored separately from the patterns.</p>
<table><thead><tr><th>Item</th><th>Values</th><th>Meaning</th></tr></thead><tbody>
<tr><td>COLOR</td><td>8 themes (ORANGE by default)</td><td>Screen colour</td></tr>
<tr><td>SPEAKER EQ</td><td>FLAT / LOWCUT / BASS+</td><td>Correction for the built-in speaker</td></tr>
<tr><td>USB LEVEL</td><td>MASTER / FIXED</td><td>Whether USB audio follows the MASTER knob</td></tr>
<tr><td>PERFORM</td><td>HOLD / PAGE</td><td>PERFORM only while FX is held, or stays open</td></tr>
<tr><td>ACCEL</td><td>OFF / ON</td><td>Knob acceleration</td></tr>
<tr><td>BACKLIGHT</td><td>OFF DIM LO DIM HI INV INV LO INV HI</td><td>Key backlight</td></tr>
<tr><td>CLOCK</td><td>INT / USB / TRS (/ BLE / LINK)</td><td>Tempo source</td></tr>
<tr><td>MIDI SYNC</td><td>OFF / USB (/ BLE / BOTH)</td><td>Sending MIDI clock and start/stop</td></tr>
<tr><td>MIDI CHANNEL</td><td>1–16 (10)</td><td>Input channel</td></tr>
<tr><td>MIDI ROUTE</td><td>NOTE / SEL / TRACK</td><td>Where incoming notes go: NOTE — notes on the MIDI CHANNEL play the track with the same MNOTE; SEL — the selected track, chromatically; TRACK — channels 1–6 play tracks 1–6 chromatically</td></tr>
<tr><td>USB MIDI</td><td>read-only</td><td>USB connection state</td></tr>
<tr><td>METRONOME</td><td>OFF / LOW / MID / HIGH</td><td>A click on quarter notes while the transport is running</td></tr>
<tr><td>MUTE MODE</td><td>NOW / BAR</td><td>Mute immediately or from the next bar</td></tr>
<tr><td>MIDI THRU</td><td>OFF / ON</td><td>Pass incoming MIDI through to the output</td></tr>
<tr><td>REC QUANT</td><td>1/16 / 1/8 / 1/32 / OFF</td><td>Live recording quantisation</td></tr>
<tr><td>HARDWARE CALIBRATION</td><td><kbd>OCT+</kbd> START</td><td>Teaching the buttons and knobs</td></tr>
<tr><td>BLUETOOTH</td><td><kbd>OCT+</kbd> OPEN</td><td>The Bluetooth page (only in a build with Bluetooth)</td></tr>
</tbody></table>

<h2 id="seq">4. Sequencer</h2>

<h3 id="steps">Steps, banks, step properties</h3>
<p>Each track has up to <b>64 steps</b>. The white keys show the 16 steps of the current bank; the number of banks depends on LEN
(LEN 20 gives two banks). With KEYS off the white keys are steps on any page (on TRACKS only while no track is armed;
except on COMP, the preset list, the menu and NAME).</p>
<ul>
<li>A <b>tap</b> on a step turns it on or off (on release). A step that is turned off is reset to a plain step.</li>
<li><b>Holding</b> 0.3 s turns an off step on and makes it “held”: the knobs change the step's properties.</li>
<li><kbd>OCT+</kbd> + step sets or clears an <b>accent</b> (an off step is turned on with an accent). While <kbd>OCT+</kbd> is held,
only the accented steps are lit.</li>
<li><b>Banks:</b> on PATTERN and TRACKS <kbd>OCT-</kbd> goes to the previous bank (immediately), <kbd>OCT+</kbd> to the next one (on release,
if no accent was set). On other pages during playback the bank follows the selected track's playhead (it stays put while a step is held),
and when stopped the last selected bank is shown.</li>
</ul>
<table><thead><tr><th>Held step + knob (PATTERN, TRACKS)</th><th>Values</th></tr></thead><tbody>
<tr><td><kbd class="k">KNOB 1</kbd> PROB</td><td>0 %, 5–100 % in steps of 5 %, 1-SHOT (once after PLAY), A/B conditions: 1/2 … 8/8 (play on pass A of every B passes), FILL, !FILL</td></tr>
<tr><td><kbd class="k">KNOB 2</kbd> RATCH</td><td>1–4 hits per step</td></tr>
<tr><td><kbd class="k">KNOB 3</kbd> ACC</td><td>Right: accent, left: no accent (velocity 127 instead of 96)</td></tr>
<tr><td><kbd class="k">KNOB 4</kbd> NUDGE</td><td>−50…+50 hundredths of a step: earlier / later</td></tr>
<tr><td><kbd class="k">ALGORITHM</kbd> NOTE (KEYS off)</td><td>The step's note, C-1…G9; all held steps get the same note (like the trig note on the Model:Cycles).
Popup: <code>STEP 5 NOTE C#4</code> or <code>3 STEPS NOTE C#4</code>, inverted; <kbd>OCT-</kbd> restores the notes of all the steps</td></tr>
</tbody></table>
<p><b>The note of a new step</b> is the track NOTE: the last value of <kbd class="k">ALGORITHM</kbd> (with no steps held), of the NOTE knob on SOUND 1 or of a key in KEYS mode. It works on any machine.</p>
<p><b>FILL:</b> hold <kbd>REC</kbd> 0.4 s on any page. While the button is held, steps with the FILL condition play and !FILL steps are silent.
<code>FILL</code> is lit in the top line.</p>

<h3 id="len">LEN, DIV, FIT: polymeter and polyrhythm</h3>
<p>Each track has its own length <code>LEN</code> and its own step size <code>DIV</code> (PATTERN page). Step length in beats:
1/16 = ¼ beat, 1/8 = ½, 1/4 = 1, 8T = ⅓, 16T = ⅙, 1/32 = ⅛, 1/2 = 2, 1/1 = 4, 2BAR = 8, 4BAR = 16.</p>
<p><b>FIT divisions</b> (F1/2, FBAR, F2BR, F4BR) stretch <i>all</i> LEN steps of the track over exactly 2, 4, 8 or 16 beats,
with no accumulated error. If you change LEN during playback, the track is recalculated for the new length.</p>
<div class="note"><b>Polymeter example.</b> T1: LEN 16, 1/16. T2: LEN 12, 1/16. MLEN OFF. T2 repeats every 3 beats,
T1 every bar; they line up again after 3 bars.<br>
<b>Polyrhythm example.</b> T1: LEN 4, 1/16 (four hits per beat). T2: LEN 3, FBAR — three equal hits per bar (a half-note triplet).
LEN 5 F1/2 is a quintuplet over half a bar; LEN 7 F4BR is 7 steps over 4 bars.</div>
<p><b>Master Length</b> (<code>MLEN</code>, shared by the pattern) counts sixteenths from the start. Every MLEN sixteenths
all tracks restart from the first step, even if nothing is queued. Example: with MLEN 16 a track with
LEN 12 plays steps 1–12, then 1–4, and everything restarts.
Without MLEN the tracks run freely (true polymeter). A queued pattern change with MLEN happens on its boundary,
and without MLEN at the end of the longest track (LEN × DIV).</p>

<h3 id="swing">Swing</h3>
<p>There are two swings: the global one (HOME, <kbd class="k">KNOB 2</kbd>) and one per track (PATTERN, <code>SWG</code>).
They add up, to a maximum of 100. Even steps are shifted later, odd ones earlier: SWG 50 gives pairs of 1.2 / 0.8 steps,
SWG 100 gives 1.4 / 0.6. With a FIT division the unit is the track's own step length.</p>
<h3 id="locks">Parameter locks and machine locks</h3>
{{screen:locks/sound_held.png|Step 5 held on the SOUND page}}
<ol>
<li>Turn KEYS off (tap <kbd>SEL</kbd>, SEL unlit). The white keys are the selected track's steps on any page.</li>
<li>Open the page with the parameter you want (EDIT, FX, LFO, ARP).</li>
<li>Hold the step and turn the knob. The step is turned on if it was off, and the value is written to that step only.
The top line shows <code>LOCK STEP n</code>, and the lock value is shown inverted.</li>
</ol>
<p><b>Machine lock:</b> hold the step and turn <code>MODEL</code> (SOUND, <kbd class="k">KNOB 1</kbd>). The message is <code>MLOCK STEP n MACHINE</code>.
ACID is skipped. Going back to the track's own machine removes the machine lock (<code>MLOCK STEP n OFF</code>).</p>
<p><b>What can be locked:</b> machine knobs, LVL, PAN, sends, the filter's CUT/RESO/ENV/DECAY, SRR, the insert's A/B/C/MIX,
the sampler's ATK/SUS/REL (on ENV) and PLAY/CUT/LOOP, the LFO's RATE/MORPH/DEPTH, all ARP parameters, MODEL (machine lock).
Not lockable: types (filter, insert) and global parameters. A knob that cannot be locked does nothing while a step is held.</p>
<p><b>How to erase:</b></p>
<ul>
<li>step + tap <kbd>OCT-</kbd> — this step's locks on the four knobs of the current page (<code>STEP n LOCKS CLEARED</code>);</li>
<li>step + hold <kbd>OCT-</kbd> 0.6 s — all of the step's locks (<code>STEP n ALL CLEARED</code>);</li>
<li>tapping a trig off also clears its locks (machine locks too); tapped on again it is a plain step;</li>
<li><kbd>REC</kbd> + <kbd>SEL</kbd> 5 s — the whole track (see below).</li>
</ul>
<p>A pattern has room for 1024 lock and motion events across all tracks; when that runs out you get <code>LOCKS FULL</code> / <code>MOTION FULL</code>.</p>
{{/screen}}
{{shot:popup_lock.png|A lock value in the popup is shown inverted}}

<h3 id="arpseq">ARP and RTRG in the sequencer</h3>
<p><b>RTRG</b> (retrigger) is a roll of a single hit over the whole step at the RATE frequency. <b>ARP1</b> (only ACID, SAMPL, FM, SKIN, CHORD; on CHORD it uses the notes of its own chord) plays the CHORD chord
from the step's note in direction DIR over RANGE octaves. <b>ARP2</b> is a classic arpeggiator over the keys held in KEYS mode; on a step (a single note) it sounds like ARP1.
The old ARP value in projects opens as ARP1; on drums ARP1/ARP2 sound like RTRG. A roll lasts from the hit to the end of the step (including TIE) or to the next hit, and replaces RATCH.
RATE is calculated from the track's step length (with swing and FIT). FADE fades the repeats out or in. All ARP parameters can be locked per step.</p>
<h3 id="leds">Key LEDs</h3>
<table><thead><tr><th>State</th><th>White (steps)</th><th>Black 1–6</th></tr></thead><tbody>
<tr><td><b>Lit</b></td><td>Step is on</td><td>Selected track</td></tr>
<tr><td><b>Blinking</b> (250 ms)</td><td>The step has a parameter lock or machine lock</td><td>—</td></tr>
<tr><td><b>Breathing</b> (smoothly)</td><td>—</td><td>Track armed for recording</td></tr>
<tr><td><b>Winking</b> (lit with a short gap once a second)</td><td>—</td><td>Selected and armed track</td></tr>
<tr><td><b>Flashing</b> (50 ms per hit)</td><td>—</td><td>A hit of the track sounds: a dark key lights up, the selected track's key goes dark for a moment</td></tr>
<tr><td>Inverted</td><td>Running playhead (current step of the selected track)</td><td>—</td></tr>
<tr><td>Dark</td><td>Step is off or beyond LEN</td><td>A track not selected, between its hits</td></tr>
</tbody></table>
<p>Black keys 1–6 flash on every audible hit of their track, like the track buttons on the Model:Cycles: hits from the
sequencer, retrigger, ARP, CHORD and notes played on the keys (also while stopped). A muted track and a track silenced
by solo do not flash. The keys do this wherever they are free: on every page with the steps on the white keys, on COMP
and on TRACKS with recording armed. In KEYS the black keys stay a keyboard (a held key is lit, the selected track is not)
and only flash, so the notes you play flash their track's key. While a modifier is held (GLO, SEQ, ARP, SEL, SAVE), and
in the menu, NAME, the PRESETS list and PERFORM, the keys keep their own lighting.</p>
<p>Keys 1, 5, 9, 13 are dimly lit as beat markers (depends on BACKLIGHT). While a combo button is held, black 1–6 show:
<kbd>SEQ</kbd> — the audible tracks (a mute waiting for the bar blinks), <kbd>ARP</kbd> — solo (F#5 is lit if MOMENTARY), <kbd>SEL</kbd> — the selected track.
Buttons: <kbd>PLAY</kbd> blinks while playing, <kbd>REC</kbd> is lit if anything is armed, <kbd>SEL</kbd> shows KEYS mode,
<kbd>OCT-</kbd>/<kbd>OCT+</kbd> breathe if there is a previous/next bank.</p>

<h3 id="copy">Copy, paste, clear</h3>
<table><thead><tr><th>Combo</th><th>What it does</th></tr></thead><tbody>
<tr><td>step(s) + <kbd>REC</kbd></td><td>Copy the steps from the lowest to the highest held one (within LEN), with locks: <code>COPIED 3-8</code></td></tr>
<tr><td>step + <kbd>PLAY</kbd></td><td>Paste the whole copy starting at this step; anything past LEN is dropped</td></tr>
<tr><td>several steps + <kbd>PLAY</kbd></td><td>Fill the range: the copy is repeated or truncated. The target steps' old locks are erased</td></tr>
<tr><td><kbd>SAVE</kbd> + <kbd>REC</kbd></td><td>Copy the selected track (steps, locks, LEN/DIV/SWG): <code>COPIED T3</code></td></tr>
<tr><td><kbd>SAVE</kbd> + <kbd>PLAY</kbd></td><td>Paste into the selected track: <code>PASTED T5</code></td></tr>
<tr><td><kbd>GLO</kbd> + <kbd>REC</kbd></td><td>Copy the pattern (all tracks of the current variation)</td></tr>
<tr><td><kbd>GLO</kbd> + <kbd>PLAY</kbd></td><td>Paste the pattern into the current variation</td></tr>
<tr><td><kbd>REC</kbd> + <kbd>SEL</kbd> 5 s</td><td>Clear the selected track (steps, motion, locks). A countdown <code>CLEAR T1</code> 5→1 runs; release any button to cancel</td></tr>
<tr><td>SAVE → COPY</td><td>Copy a pattern to another letter (A–D)</td></tr>
</tbody></table>
<p>In these combos REC does not arm recording or engage FILL, and PLAY does not start the transport. The clipboard lasts until power-off.</p>
{{shot:92_clear_countdown.png|Track clear countdown}}

<h2 id="patterns">5. Patterns, variations, chains</h2>
<p>Memory holds <b>4 patterns A–D</b>, each with <b>8 variations</b>. A pattern stores the track sounds, the variations, the locks and motion,
MLEN and the effects. Variations are different step parts within one pattern.</p>
<h3 id="cue">Switching</h3>
<table><thead><tr><th>Combo</th><th>Action</th></tr></thead><tbody>
<tr><td><kbd>GLO</kbd> + white 1–4</td><td>Queue pattern A–D</td></tr>
<tr><td><kbd>GLO</kbd> + black 1–8</td><td>Queue variation 1–8 (of the queued pattern or the current one)</td></tr>
<tr><td>Hold <kbd>SAVE</kbd> 0.4 s (or <kbd>SAVE</kbd> + a key)</td><td>Switching map: the same keys, with <code>CUE A1*</code> in the top line</td></tr>
<tr><td>The same key again</td><td>Switch on the next step</td></tr>
<tr><td>The key of the playing pattern/variation</td><td>Cancel the queue</td></tr>
</tbody></table>
<p>When the transport is stopped, switching is immediate. During playback it happens at the end of the longest track or on the MLEN boundary;
then all tracks start from the first step. <kbd>PLAY</kbd> (stop) clears the queue. An empty pattern cannot be selected (<code>SLOT A EMPTY</code>).</p>
<p><b>Lights in the map:</b> white keys — pattern exists (lit), loaded (winking), queued (blinking), has unsaved edits (breathing);
black keys — variation exists (lit), playing (winking), queued (blinking).</p>
<h3 id="chain">Chains</h3>
<p>Hold <kbd>GLO</kbd> and press <b>two or more</b> of white 1–4 in order. A black key after a white one sets that link's variation.
The chain starts when you release <kbd>GLO</kbd>. Example: <kbd>GLO</kbd> + A, black 1, B, black 3, A, black 2 → <code>A1 B3 A2</code>.</p>
<ul>
<li>Up to 16 links, repeats allowed. Each link plays one pattern length (or one MLEN), then the chain loops. Empty patterns are skipped.</li>
<li>The chain is shown on HOME as the line <code>CH A1 B3 A2</code>: the current link inverted, the next one underlined.</li>
<li>It ends on stop, LOAD, or a single pattern or variation change.</li>
<li>The chain is not saved.</li>
</ul>

<h2 id="perfmidi">6. Mute, solo, recording, MIDI, Bluetooth</h2>
<h3 id="mute">Mute and solo</h3>
<table><thead><tr><th>Combo</th><th>Action</th></tr></thead><tbody>
<tr><td><kbd>SEQ</kbd> + black 1–6</td><td>Mute track 1–6: immediately or from the next bar (menu MUTE MODE)</td></tr>
<tr><td><kbd>ARP</kbd> + black 1–6</td><td>Solo a track: TOGGLE (on/off) or MOMENTARY (while the key is held)</td></tr>
<tr><td><kbd>ARP</kbd> + F#5 (black 11)</td><td>Switch the solo mode TOGGLE / MOMENTARY (it is saved)</td></tr>
<tr><td><kbd>ARP</kbd> + white (KEYS on)</td><td>Not solo: live retrigger of the key's note (see “KEYS mode”)</td></tr>
<tr><td><kbd>SEL</kbd> + black 1–6</td><td>Select a track</td></tr>
</tbody></table>

<h3 id="rec">Recording</h3>
<ul>
<li><b>TRACKS</b>: a tap on <kbd>REC</kbd> arms or disarms recording on the selected track (<code>T3 REC ARMED</code>).</li>
<li><b>Any other page</b>: a tap on <kbd>REC</kbd> gives <b>REC ALL</b>, all tracks armed. Another tap restores the previous state. With KEYS off, the white keys outside TRACKS are steps and don't record hits; to play and record, use TRACKS or KEYS.</li>
<li>REC does not start the transport. Recording begins once <kbd>PLAY</kbd> is running.</li>
<li>Play the tracks with white 1–6 on TRACKS, with notes in KEYS mode or from external MIDI: the hits land on the steps of the armed tracks,
with quantisation set in the REC QUANT menu item (OFF keeps your timing via NUDGE). With KEYS off, outside TRACKS the white keys are steps, so you can't finger-record there.</li>
<li><b>Motion:</b> knob turns are recorded if the selected track is armed and playback is running.</li>
</ul>

<h3 id="keys">KEYS mode</h3>
<p>KEYS is on at startup (SEL lit); a tap on <kbd>SEL</kbd> turns it off and on. While SEL is lit, all 27 keys play the selected track chromatically on any page, and the keys don't control the sequencer.
<kbd>OCT-</kbd>/<kbd>OCT+</kbd> set the octave −2…+2. When SEL is unlit, the white keys are steps on any page.</p>
<p><b>The track's MODE in KEYS.</b> The selected track's MODE (ARP page) also applies to the keys. RTRG repeats the held note at the RATE frequency (with GATE and FADE) while the key is down.
ARP1 arpeggiates the held key's note; a new key (mono, legato as usual) starts the arpeggio from the new note. ARP2 arpeggiates all held keys (up to 8),
sorted by pitch, in DIR order over RANGE octaves; adding and releasing keys changes the set of notes without breaking the rhythm, and releasing the last one stops the arpeggio
(the last note's gate plays out). Tempo and swing follow the track: during playback they follow the track's steps, when stopped they run freely at the tempo from the moment of the key press. Releasing, STOP,
changing track or leaving KEYS stops the roll. The repeats are not sent to MIDI OUT; when recording, the pressed note is recorded, not the repeats (the step's MODE plays those later).</p>
<p><b>Live retrigger: ARP + white key.</b> While SEL is lit, hold <kbd>ARP</kbd> and a white key (in either order): the key's note repeats
at the track's RATE like RTRG, whatever MODE is set (OFF, RTRG, ARP1 or ARP2; drums too), with the track's GATE, FADE and swing. If <kbd>ARP</kbd> is pressed
after the key, the first repeat sounds one RATE step later. Release <kbd>ARP</kbd> and the repeats stop; the key sounds like a normal held key
(a sounding ACID note stays held) and ends when you release it. If several white keys are held, the last one repeats.
The repeats are not recorded and not sent to MIDI OUT (the pressed note itself is recorded as usual); holding ARP this way does not open the ARP page.
<kbd>ARP</kbd> + black keys is still solo; when SEL is unlit, ARP + white does nothing.</p>

<h3 id="midiout">MIDI OUT</h3>
<p>Page GLO → MIDI OUT. <code>OUT</code> is where to send (USB; in a build with Bluetooth also BLE/BOTH). <code>T1CH…T6CH</code> is the selected track's channel
(OFF means the track doesn't send). What the sequencer plays is sent: steps, ratchets, NUDGE, ARP/RTRG rolls.
Pitched machines send the played note, drums send the track's MNOTE. Velocity 127 (accent) or 96. Clock and start/stop: the MIDI SYNC menu item.</p>

<h3 id="midikbd">MIDI keyboard</h3>
<p>On MIDI OUT set <code>KBD</code> to TRK (the selected track's channel) or 1–16. KEYS mode turns on by itself, and the 27 keys start sending notes
(velocity 100) to USB and, if connected, to Bluetooth. <kbd>OCT-</kbd>/<kbd>OCT+</kbd> set the octave −4…+4. <code>LOCL OFF</code> mutes the FM-1's own sound.
HOME shows <code>MIDI KBD CH1 OCT+1</code> (<code>CH-</code> if the track channel is OFF).</p>

<h3 id="midiin">MIDI in, clock, MIDI LEARN</h3>
<p>Menu: CLOCK (INT / USB / TRS …), MIDI CHANNEL, MIDI ROUTE, MIDI THRU. <b>MIDI LEARN:</b> SAVE → COPY, <kbd class="k">KNOB 4</kbd> LEARN ON.
Then choose a target (turn a knob, press PLAY, REC, ALGORITHM or a PERFORM key) and send a CC from the controller.
<kbd>OCT-</kbd> erases the target's mapping (with no target it asks <code>CLEAR ALL LEARNED?</code>), <kbd>OCT+</kbd> is done.</p>

<h3 id="bt">Bluetooth MIDI</h3>
{{screen:bt/00_menu.png|MENU: BLUETOOTH}}
<p>Only in a build with Bluetooth. Menu (hold <kbd>HOME</kbd>) → item <code>BLUETOOTH</code> → <kbd>OCT+</kbd> OPEN.
This page turns on Bluetooth and sets the role, autostart and saved devices; <kbd>OCT-</kbd> is back, <kbd>OCT+</kbd> is OK.
To a computer or tablet the FM-1 appears as <b>Felucca FM-1</b>. Messages: <code>BT CONNECTED</code> / <code>BT LOST</code>.
AUTOSTART turns Bluetooth on 5 s after startup. If you hold <kbd>OCT-</kbd> + <kbd>OCT+</kbd> at that moment,
autostart is skipped; in safe mode Bluetooth does not start.</p>
{{/screen}}

<h2 id="cheat">7. Combo cheat sheet</h2>
<table class="cheat"><thead><tr><th>Topic</th><th>Combo</th><th>Action</th></tr></thead><tbody>
<tr><td rowspan="9">Navigation</td><td><kbd>HOME</kbd></td><td>HOME ⇄ TRACKS</td></tr>
<tr><td><kbd>HOME</kbd> 0.7 s</td><td>Device menu</td></tr>
<tr><td><kbd>EDIT</kbd> / hold 0.4 s</td><td>SOUND / preset list</td></tr>
<tr><td><kbd>ARP</kbd> (short press)</td><td>ARP 1 / 2 pages</td></tr>
<tr><td><kbd>FX</kbd> / hold 0.4 s</td><td>FX pages / PERFORM</td></tr>
<tr><td><kbd>LFO</kbd>, <kbd>SEQ</kbd>, <kbd>GLO</kbd> (tap), <kbd>SAVE</kbd> (tap)</td><td>LFO, PATTERN, global pages, PROJECT/TOOLS/COPY</td></tr>
<tr><td><kbd class="k">SELECT</kbd></td><td>Page forward/back; on TRACKS — BPM</td></tr>
<tr><td><kbd class="k">ALGORITHM</kbd></td><td>KEYS on: track; KEYS off: NOTE (of the steps or the track)</td></tr>
<tr><td><kbd class="k">PRESETS</kbd></td><td>Next/previous preset of the track</td></tr>
<tr><td rowspan="5">Tracks</td><td><kbd>SEL</kbd> + black 1–6, white 1–6 on HOME/sound</td><td>Track select</td></tr>
<tr><td><kbd>SEQ</kbd> + black 1–6</td><td>Mute</td></tr>
<tr><td><kbd>ARP</kbd> + black 1–6</td><td>Solo</td></tr>
<tr><td><kbd>ARP</kbd> + F#5</td><td>Solo mode TOGGLE/MOMENTARY</td></tr>
<tr><td><kbd>ARP</kbd> + white (KEYS on)</td><td>Live note retrigger</td></tr>
<tr><td rowspan="6">Steps</td><td>tap a step / hold 0.3 s</td><td>On/off / held step</td></tr>
<tr><td>step + <kbd class="k">KNOB 1–4</kbd> (PATTERN, TRACKS)</td><td>PROB / RATCH / ACC / NUDGE</td></tr>
<tr><td>step(s) + <kbd class="k">ALGORITHM</kbd> (KEYS off)</td><td>Step note</td></tr>
<tr><td><kbd>OCT+</kbd> + step</td><td>Accent</td></tr>
<tr><td><kbd>OCT-</kbd> / <kbd>OCT+</kbd> (PATTERN, TRACKS)</td><td>Step bank</td></tr>
<tr><td><kbd>REC</kbd> 0.4 s</td><td>FILL while held</td></tr>
<tr><td rowspan="4">Locks</td><td>KEYS off: step + knob</td><td>Parameter lock</td></tr>
<tr><td>KEYS off: step + MODEL</td><td>Machine lock</td></tr>
<tr><td>step + <kbd>OCT-</kbd></td><td>Erase the page's locks</td></tr>
<tr><td>step + <kbd>OCT-</kbd> 0.6 s</td><td>Erase all of the step's locks</td></tr>
<tr><td rowspan="5">Copy</td><td>step(s) + <kbd>REC</kbd> / <kbd>PLAY</kbd></td><td>Copy / paste steps</td></tr>
<tr><td><kbd>SAVE</kbd> + <kbd>REC</kbd> / <kbd>PLAY</kbd></td><td>Copy / paste track</td></tr>
<tr><td><kbd>GLO</kbd> + <kbd>REC</kbd> / <kbd>PLAY</kbd></td><td>Copy / paste pattern</td></tr>
<tr><td><kbd>REC</kbd> + <kbd>SEL</kbd> 5 s</td><td>Clear track</td></tr>
<tr><td>Popup + <kbd>OCT-</kbd></td><td>Undo a knob edit</td></tr>
<tr><td rowspan="4">Patterns</td><td><kbd>GLO</kbd> + white 1–4</td><td>Queue pattern A–D</td></tr>
<tr><td><kbd>GLO</kbd> + black 1–8</td><td>Queue variation 1–8</td></tr>
<tr><td><kbd>GLO</kbd> + 2+ whites (+ blacks)</td><td>Chain</td></tr>
<tr><td><kbd>SAVE</kbd> 0.4 s + keys</td><td>Switching map</td></tr>
<tr><td rowspan="4">Transport and recording</td><td><kbd>PLAY</kbd></td><td>Start/stop</td></tr>
<tr><td><kbd>REC</kbd> on TRACKS</td><td>Arm the selected track</td></tr>
<tr><td><kbd>REC</kbd> on other pages</td><td>REC ALL on/off</td></tr>
<tr><td><kbd>SEL</kbd> (tap)</td><td>KEYS; <kbd>OCT-</kbd>/<kbd>OCT+</kbd> — octave</td></tr>
<tr><td rowspan="2">Dialogs and menu</td><td><kbd>OCT+</kbd> / <kbd>OCT-</kbd></td><td>YES / NO (in the menu: next value / exit)</td></tr>
<tr><td>LOAD/SAVE/PASTE/TOOLS</td><td>Two knob clicks</td></tr>
<tr><td rowspan="4">Power-on</td><td>hold <kbd>SEQ</kbd></td><td>SAFE START</td></tr>
<tr><td>hold <kbd>OCT-</kbd> + <kbd>OCT+</kbd></td><td>Button and knob calibration</td></tr>
<tr><td><kbd>OCT-</kbd> + <kbd>OCT+</kbd> 5 s while running</td><td>The chip's boot loader mode (UBOOT), see section 8</td></tr>
<tr><td>any key on the splash</td><td>Skip the splash</td></tr>
</tbody></table>

<h2 id="safety">8. Safety and recovery</h2>
<h3 id="safestart">SAFE START</h3>
{{screen:safe_start.png|SAFE START}}
<p>Switch the FM-1 off, <b>hold <kbd>SEQ</kbd></b> and switch it on. You will see <code>SAFE START</code> /
<code>NO AUDIO - USB UPDATE READY</code> / <code>POWER OFF TO LEAVE</code>.</p>
<ul>
<li>No pattern is loaded, there is no sound, all LEDs are off, the panel does not respond, Bluetooth does not start.</li>
<li>USB works: you can update the firmware or restore the stock one.</li>
<li>To leave, switch the power off.</li>
</ul>
<p>Use it after a crash screen, repeated reboots or a blank screen.</p>
{{/screen}}
<h3 id="stock">Returning to the stock M-VAVE firmware</h3>
<ol>
<li>Download <b>M-UPGRADE</b> (Downloads → PC Software) and the official <b>FM-1 V15</b> firmware (PC Firmware) from <span class="mono">www.m-vave.com/download</span>.</li>
<li>Connect the FM-1 with a USB data cable (if there are problems, in SAFE START) and flash V15 with M-UPGRADE.
You can also choose <i>Return to official V15</i> in the Hortator web installer.</li>
</ol>
<h3 id="never">What never changes</h3>
<ul>
<li>The <b>update loader</b> (it shows up on USB as <code>Felucca Update</code>, M-UPGRADE protocol) only writes the firmware area
(flash from 0x4000). It does not touch the chip's boot area, so the update path is always there.</li>
<li>The chip's built-in boot mode (UBOOT) cannot be erased. To enter it: hold <kbd>OCT-</kbd> + <kbd>OCT+</kbd> while running.
After 2 s a countdown starts (release to cancel), at 5 s you get <code>UBOOT</code>. It is only needed for recovery from a computer
(<span class="mono">docs/RECOVERY.md</span> in the repository: the tool writes only the 0x4000–0x93000 area and does not touch the boot area,
the chip key or the data area).</li>
<li>Patterns saved by the 8-track Hortator cannot be read by FREITAKT (a different format).</li>
</ul>
<div class="warn">This is unofficial firmware and you use it at your own risk. M-VAVE is not responsible for it.</div>
'''
