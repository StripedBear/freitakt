// SPDX-License-Identifier: GPL-3.0-only
// Drum machine fork: 2026 DEADACTIVE
//
// Export MIDI (midi_export.mjs, the Patterns tab's "Export MIDI…"): node, no browser, no hardware.
//   node web/editor/test_midi_export.mjs          (also run by node web/test_web.mjs)
// - the file: MThd type 1, a tempo track + one MTrk per track, PPQ, tempo; it parses back to the notes rendered
// - timing as the firmware's: polymetry (LEN 16 1/16 vs LEN 5 FIT over a bar, remainder carried), SWING, NUDGE,
//   RATCH, MASTER LENGTH, a chain's parts in order
// - notes / channels as MIDI OUT: pitched (ACID ...) the step's note, a drum the track's NOTE; channel OFF -> track n's
// - ARP (MAJ, 1/8 on a 1/4 step), RTRG, ACID's gate / SLIDE legato, one note a track
// - the Patterns tab's exportMidi against the mock device

import { pathToFileURL } from "node:url";
import { PPQ, buildSmf, parseSmf, renderTrack, stepGrid } from "./midi_export.mjs";
import { createMockDevice, loadDeviceData } from "./mock_device.mjs";
import { connect } from "./session.mjs";
import { PatternsModel } from "./tab_patterns.mjs";

const J = JSON.stringify;
const ON = 1, ACC = 2, SLIDE = 4, TIE = 8;
const step = (o = {}) => ({ flags: ON, cond: 0, note: 60, micro: 0, ...o });
const rest = () => ({ flags: 0, cond: 0, note: 60, micro: 0 });
const steps = (f) => Array.from({ length: 64 }, (_, i) => f(i));
const drum = (o = {}) => ({ model: 0, note: 36, gate: 64, ch: 1, arp: { mode: 0 }, ...o });
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const FAST = { timeout: 25, longTimeout: 60, tries: 3, silence: 60, backoff: 25, backoffMax: 100, giveUp: 100000, keepAlive: 100000 };

export async function runMidiExportTests(log = console.log) {
  let passed = 0, failed = 0;
  const ok = (cond, what) => { if (cond) passed++; else { failed++; log(`FAIL midi export: ${what}`); } };
  const ons = (ns) => ns.map((n) => n.on);

  /* the file */
  {
    const song = { bpm: 125, swing: 0, mlen: 0, tracks: Array.from({ length: 6 }, (_, k) => drum({ note: 36 + k, ch: k + 1 })) };
    const part = { label: "A1", tracks: Array.from({ length: 6 }, (_, k) => ({ len: 16, div: 2, swing: 0, steps: steps((i) => (k === 0 && i % 4 === 0 ? step({ flags: i === 0 ? ON | ACC : ON }) : rest())) })) };
    const r = buildSmf(song, [part]), f = parseSmf(r.bytes);
    const b = r.bytes;
    ok(String.fromCharCode(...b.slice(0, 4)) === "MThd" && f.format === 1 && f.ntrks === 7 && f.ppq === PPQ && f.tracks.length === 7,
      "MThd: type 1, 7 tracks (tempo + 6), PPQ 480");
    const tempo = f.tracks[0].find((e) => e.meta === 0x51);
    ok(tempo && ((tempo.data[0] << 16) | (tempo.data[1] << 8) | tempo.data[2]) === 480000, "tempo: 125 BPM = 480000 us a quarter");
    const n1 = f.tracks[1].filter((e) => (e.st & 0xf0) === 0x90);
    ok(J(n1.map((e) => [e.t, e.d1, e.d2])) === J([[0, 36, 127], [480, 36, 96], [960, 36, 96], [1440, 36, 96]]) && n1.every((e) => (e.st & 15) === 0),
      "a drum on 1/16 steps 1 5 9 13: the track's NOTE, ACC 127 else 96, channel 1");
    const off1 = f.tracks[1].filter((e) => (e.st & 0xf0) === 0x80);
    ok(J(off1.map((e) => e.t)) === J([120, 600, 1080, 1560]) && r.length === 1920, "its Offs after the step (120 ticks); the part = one bar");
    ok(f.tracks.slice(2).every((t) => !t.some((e) => e.st)), "the empty tracks: no notes");
  }

  /* polymetry: LEN 16 1/16 vs LEN 5 FIT FBAR (id 11), the remainder carried */
  {
    const t16 = { len: 16, div: 2, swing: 0, steps: steps(() => step()), ...drum() };
    const t5 = { len: 5, div: 11, swing: 0, steps: steps(() => step()), ...drum() };
    ok(J(ons(renderTrack(t5, 1920))) === J([0, 384, 768, 1152, 1536]), "FIT: LEN 5 over a bar, 384 ticks each");
    const t7 = { ...t5, len: 7 }, g7 = stepGrid(t7, 3840).slice(0, 14);
    ok(g7[7].start === 1920 && g7[14 - 1].start + g7[13].len === 3840 && g7.every((g) => g.len === 274 || g.len === 275), "FIT LEN 7: 274 / 275 ticks, 7 steps exactly a bar (no drift)");
    ok(ons(renderTrack(t16, 1920)).length === 16 && renderTrack(t16, 1920)[15].on === 1800, "LEN 16 1/16: 16 notes a bar");
    const song = { bpm: 120, swing: 0, mlen: 0, tracks: [drum(), drum({ note: 38 }), drum(), drum(), drum(), drum()] };
    const empty = { len: 16, div: 2, swing: 0, steps: steps(rest) };
    const r = buildSmf(song, [{ tracks: [t16, t5, empty, empty, empty, empty] }], { loops: 2 });
    ok(r.length === 3840 && r.notes[0].length === 32 && J(ons(r.notes[1])) === J([0, 384, 768, 1152, 1536, 1920, 2304, 2688, 3072, 3456]),
      "two bars: 32 + 10 notes, the 5 still on the bar");
  }

  /* swing, nudge, ratch */
  {
    const t = { len: 16, div: 2, swing: 0, steps: steps(() => step()), ...drum() };
    ok(J(ons(renderTrack(t, 960, { swing: 50 }))) === J([0, 144, 240, 384, 480, 624, 720, 864]), "SWING 50 (global): every odd 1/16 24 ticks late");
    ok(J(ons(renderTrack({ ...t, swing: 25 }, 480, { swing: 25 }))) === J([0, 144, 240, 384]), "the track's SWING + the global");
    const nud = { ...t, steps: steps((i) => (i === 1 ? step({ micro: 50 }) : i === 3 ? step({ micro: -50 }) : rest())) };
    ok(J(ons(renderTrack(nud, 480))) === J([180, 300]), "NUDGE +50: half a step late; -50: half a step early (in the step before)");
    const rat = { ...t, steps: steps((i) => (i === 0 ? step({ flags: ON | (2 << 4) }) : rest())) };
    const rn = renderTrack(rat, 480);
    ok(J(ons(rn)) === J([0, 40, 80]) && rn.every((n) => n.off - n.on === 40), "RATCH 3: hits at 0, 1/3, 2/3 of the step, each a third long");
    const cnd = { ...t, len: 4, steps: steps((i) => (i === 0 ? step({ cond: 22 }) : i === 1 ? step({ cond: 21 }) : i === 2 ? step({ cond: 57 }) : i === 3 ? step({ cond: 58 }) : rest())) };
    ok(J(ons(renderTrack(cnd, 960))) === J([0, 120, 360, 840]), "PROB 1/2 the first pass of two, 1-SHOT once, FILL silent, !FILL plays");
  }

  /* MASTER LENGTH, chain */
  {
    const t = { len: 16, div: 2, swing: 0, steps: steps(() => step()) };
    const song = { bpm: 120, swing: 0, mlen: 6, tracks: Array.from({ length: 6 }, () => drum()) };
    const empty = { len: 16, div: 2, swing: 0, steps: steps(rest) };
    const r = buildSmf(song, [{ label: "A1", tracks: [t, empty, empty, empty, empty, empty] }], { loops: 2 });
    ok(r.length === 1440 && J(ons(r.notes[0])) === J([0, 120, 240, 360, 480, 600, 720, 840, 960, 1080, 1200, 1320]), "MASTER LENGTH 6: the track restarts every 6 1/16s");
    const b4 = { len: 4, div: 2, swing: 0, steps: steps((i) => (i === 0 ? step() : rest())) };
    const q8 = { len: 8, div: 1, swing: 0, steps: steps((i) => (i === 0 ? step() : rest())) };
    const c = buildSmf({ ...song, mlen: 0 }, [{ label: "A1", tracks: [b4, empty, empty, empty, empty, empty] }, { label: "B3", tracks: [q8, empty, empty, empty, empty, empty] }]);
    const f = parseSmf(c.bytes), marks = f.tracks[0].filter((e) => e.meta === 6).map((e) => [e.t, String.fromCharCode(...e.data)]);
    ok(c.length === 480 + 1920 && J(ons(c.notes[0])) === J([0, 480]) && J(marks) === J([[0, "A1"], [480, "B3"]]), "a chain: A1 (one beat) then B3 (a bar) in order, markers");
  }

  /* notes, channels, ARP, ACID */
  {
    const song = { bpm: 120, swing: 0, mlen: 0, tracks: [drum({ ch: 0 }), drum({ model: 22, ch: 10 }), drum(), drum(), drum(), drum()] };
    const empty = { len: 16, div: 2, swing: 0, steps: steps(rest) };
    const a = { len: 4, div: 2, swing: 0, steps: steps((i) => (i === 0 ? step({ note: 67 }) : rest())) };
    const r = buildSmf(song, [{ tracks: [a, a, empty, empty, empty, empty] }]), f = parseSmf(r.bytes);
    const on = (k) => f.tracks[k].filter((e) => (e.st & 0xf0) === 0x90);
    ok(on(1)[0].d1 === 36 && (on(1)[0].st & 15) === 0 && on(2)[0].d1 === 67 && (on(2)[0].st & 15) === 9, "a drum: its NOTE (OFF: channel n); FM: the step's note 67 on its channel 10");
    const arp = { len: 4, div: 0, swing: 0, steps: steps((i) => (i === 0 ? step({ note: 60 }) : rest())), ...drum({ model: 22, arp: { mode: 2, rate: 2, gate: 50, fade: 0, dir: 0, chord: 2, range: 1 } }) };
    const an = renderTrack(arp, 480);
    ok(J(an.map((n) => [n.on, n.off, n.note])) === J([[0, 120, 60], [240, 360, 64]]), "ARP MAJ 1/8 on a 1/4 step: 60 then 64, GATE 50 %");
    const a3 = renderTrack({ ...arp, len: 1, steps: steps(() => step({ note: 60 })), arp: { ...arp.arp, rate: 4, range: 2 } }, 960);
    ok(J(a3.map((n) => n.note)) === J([60, 64, 67, 72, 60, 64, 67, 72]), "ARP UP x 2 octaves at 1/16, restarting with each step");
    const rt = renderTrack({ ...arp, model: 0, note: 38, steps: steps((i) => (i === 0 ? step({ flags: ON | ACC }) : i === 1 ? step({ flags: TIE }) : rest())), arp: { mode: 2, rate: 2, gate: 50, fade: -16 } }, 1920);
    ok(J(rt.map((n) => [n.on, n.note, n.vel])) === J([[0, 38, 127], [240, 38, 123], [480, 38, 119], [720, 38, 115]]), "a drum's ARP is RTRG: its NOTE, FADE down, through the TIE step");
    const acid = { len: 4, div: 2, swing: 0, model: 23, gate: 127, steps: steps((i) => (i === 0 ? step({ flags: ON | SLIDE, note: 60 }) : i === 1 ? step({ note: 62 }) : i === 2 ? rest() : step({ note: 65 }))) };
    const ac = renderTrack(acid, 480);
    ok(J(ac.map((n) => [n.on, n.off, n.note])) === J([[0, 120, 60], [120, 240, 62], [360, 480, 65]]), "ACID: SLIDE holds into the next note (legato), GATE 100 %, a rest closes");
    const ac2 = renderTrack({ ...acid, gate: 0, steps: steps((i) => (i === 0 ? step() : rest())) }, 480);
    ok(ac2[0].off === 12, "ACID GATE 0: 10 % of the step");
  }

  /* the Patterns tab against the mock device */
  {
    const data = await loadDeviceData();
    ok(J([0, 8, 13, 69, 70, 75].map((i) => data.P[i])) === J(["MODEL", "E7", "NOTE", "AMODE", "ARATE", "ARANGE"])
      && J([0, 1, 33, 38].map((i) => data.G[i])) === J(["BPM", "SWING", "MCH1", "MCH6"]), "the ids exportMidi reads = the firmware's (device_data)");
    const m = createMockDevice(data), c = await connect(m.access, { timing: FAST }), s = c.session, pm = new PatternsModel(s);
    await pm.load();
    for (let i = 0; i < 300 && (pm.loading || !s.link.idle); i++) await sleep(2);
    const r = await pm.exportMidi({});
    const f = parseSmf(r.bytes), n0 = m.tracks[0].steps.filter((x, i) => i < m.timingOf(m.tracks[0])[0] && (x.flags & ON) && !(x.flags & TIE)).length;
    ok(f.format === 1 && f.ntrks === 7 && /^freitakt-A1\.mid$/.test(r.name) && r.notes[0].length >= 1 && r.notes[0].length <= n0 * 4, "exportMidi: the shown variation, read from the device");
    const r2 = await pm.exportMidi({ chain: "a1 b2", mlen: 16 });
    ok(r2.parts.length === 2 && r2.parts[1].label === "B2" && r2.name === "freitakt-A1-B2.mid", "exportMidi: a chain");
    let bad = "";
    try { await pm.exportMidi({ chain: "E9" }); } catch (e) { bad = e.message; }
    ok(/not a chain entry/.test(bad), "a bad chain entry: refused, said why");
    s.close();
    pm.destroy();
  }
  return { passed, failed };
}

if (import.meta.url === pathToFileURL(process.argv[1]).href) {
  const { passed, failed } = await runMidiExportTests();
  console.log(failed ? `MIDI EXPORT TESTS FAILED (${failed} of ${passed + failed})` : `midi export tests passed (${passed})`);
  process.exit(failed ? 1 : 0);
}
