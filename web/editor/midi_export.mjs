// SPDX-License-Identifier: GPL-3.0-only
// Drum machine fork: 2026 DEADACTIVE
// Export MIDI: a Standard MIDI File (type 1: a tempo track, then one MTrk per track) of a pattern's variation, or of a
// chain of them played in order, built in the browser from what the editor reads (STEPS_GET's steps and timing, the
// tracks' knobs by GET). The timing is the firmware's (seq.c), in ticks instead of samples:
//  - a step's period: its DIV as num / den beats (FIT: LEN steps over 2, 4, 8, 16 beats), the remainder carried from
//    step to step (den steps last exactly num beats: no drift, polymetry stays locked);
//  - SWING (the track's + the global, 0..100): even steps (counted from the start) longer, odd ones shorter by
//    swing * step / 250;
//  - NUDGE (micro, -50..+50 hundredths of a step): late inside its step, early inside the step before;
//  - RATCH: hits spread evenly over the step; ARP / RTRG (MODE): a roll, a repeat every RATE (a whole note / RATE's
//    divisor, as the step's share), FADE, GATE, ARP1's (a step's ARP2 too) CHORD x RANGE in DIR's order (RND: a fixed seed, so not the
//    device's dice), through the TIE steps after it;
//  - PROB: a seeded die (not the device's); 1-SHOT the first pass only; FILL steps silent, !FILL ones play; a/b by pass;
//  - MASTER LENGTH (1/16 steps, 0 / 1 OFF): every track restarts at step 0 after it.
// Notes as MIDI OUT sends them (midi_out.c): a pitched model's (ACID SAMPL FM SKIN) the step's note with ARP's
// intervals; a drum's the track's NOTE knob. Velocity ACC 127 else 96 (FADE on rolls). One note a track: a new hit ends
// the sounding one. Its end: ACID's gate (GATE knob E7; held into a SLIDE or TIE step); a roll's hit: GATE of its
// repeat; else its step (swung) / RATCH. Channels: the track's MIDI OUT channel; OFF exports on track n's channel n
// (a file has no port to switch off). Not rendered: parameter / machine LOCKS (the track's knobs as they are now), the
// note's own LFO / pitch, live input.

export const PPQ = 480;
/** core.h / dmodels.c model ids whose MIDI note is the step's (params.c arp_model) */
export const PITCHED = new Set([14, 21, 22, 23]); // SAMPL SKIN FM ACID
export const ACID = 23; // the gated model (dmodel gate): its GATE knob is E7
const DIV_DEN = [1, 2, 4, 8, 3, 6], DIV_FIT = 10;
const ARATE_D = [4, 6, 8, 12, 16, 24, 32];
const ARP_CHORD = [[0], [0, 7], [0, 4, 7], [0, 3, 7], [0, 5, 7], [0, 4, 7, 11], [0, 3, 7, 10], [0, 4, 7, 10]];
const COND_1SHOT = 21, COND_FILL = 57, COND_NFILL = 58;
const STEP_ON = 0x01, STEP_ACC = 0x02, STEP_SLIDE = 0x04, STEP_TIE = 0x08, RAT_SHIFT = 4;

const clamp = (v, a, b) => (v < a ? a : v > b ? b : v);
const tdiv = (a, b) => Math.trunc(a / b); // C's integer division

/** a division as num / den beats (seq.c trk_num_den) */
export function numDen(div, len) {
  if (div >= DIV_FIT) return { num: 2 << ((div - DIV_FIT) & 3), den: clamp(len, 1, 64) };
  if (div >= 6 && div < 10) return { num: 1 << (div - 5), den: 1 };
  return { num: 1, den: DIV_DEN[div % 6] };
}
/** a/b of a PROB code (22..56): 1/2, 2/2, 1/3 .. 8/8 */
function condAB(c) {
  let k = 22;
  for (let b = 2; b <= 8; b++) for (let a = 1; a <= b; a++) if (k++ === c) return { a, b };
  return { a: 1, b: 1 };
}

/** the steps of one track over `span` ticks from 0: [{ cnt, idx, start, len }] (one more past the end for NUDGE) */
export function stepGrid(tr, span, ppq = PPQ, gswing = 0) {
  const len = clamp(tr.len | 0, 1, 64), { num, den } = numDen(tr.div | 0, len);
  const sws = clamp((tr.swing | 0) + (gswing | 0), 0, 100);
  const divT = Math.floor(ppq * num / den); // fx.c div_samples / FIT's trk_period(t, 0)
  const sw = tdiv(sws * divT, 250), out = [];
  let rem = 0, at = 0;
  for (let cnt = 0; at < span || out.length < 2; cnt++) {
    const r = rem < den ? rem : den - 1, q = ppq * num + r, period = Math.floor(q / den);
    rem = q % den;
    const l = period + ((cnt & 1) ? -sw : sw);
    out.push({ cnt, idx: cnt % len, start: at, len: l });
    at += l;
    if (cnt > 100000) break;
  }
  return out;
}
/** one cycle (LEN steps) of a track, in ticks */
export function cycleTicks(tr, ppq = PPQ, gswing = 0) {
  const len = clamp(tr.len | 0, 1, 64);
  let t = 0, rem = 0;
  const { num, den } = numDen(tr.div | 0, len), sws = clamp((tr.swing | 0) + (gswing | 0), 0, 100);
  const sw = tdiv(sws * Math.floor(ppq * num / den), 250);
  for (let cnt = 0; cnt < len; cnt++) {
    const r = rem < den ? rem : den - 1, q = ppq * num + r;
    rem = q % den;
    t += Math.floor(q / den) + ((cnt & 1) ? -sw : sw);
  }
  return t;
}

/** one track's notes over `span` ticks (a part): [{ on, off, note, vel }] sorted, one note at a time */
export function renderTrack(tr, span, opt = {}) {
  const ppq = opt.ppq ?? PPQ, gswing = opt.swing ?? 0, fill = !!opt.fill;
  const model = tr.model | 0, pitched = PITCHED.has(model), gated = model === ACID;
  const steps = tr.steps, len = clamp(tr.len | 0, 1, 64), { num, den } = numDen(tr.div | 0, len);
  const grid = stepGrid(tr, span, ppq, gswing);
  const ap = tr.arp ?? {}, amode0 = Math.min(clamp(ap.mode | 0, 0, 3), 2), amode = amode0 === 2 && !pitched ? 1 : amode0; // (ARP2 on a step: ARP1)
  const gateK = 10 + tdiv(clamp(tr.gate ?? 64, 0, 127) * 90, 127);
  let rng = (opt.seed ?? 0x2545f491) >>> 0, arng = 0x9e3779b9;
  const die = () => { rng = (Math.imul(rng, 1664525) + 1013904223) >>> 0; return (((rng >>> 16) * 100) >>> 16); };
  const hits = []; // { t, note, vel, off (Infinity: held), legato }
  let held = null, slideIn = false; // ACID: the note a SLIDE / TIE holds
  const noteOf = (n) => (pitched ? clamp(n, 0, 127) : clamp(tr.note ?? 60, 0, 127));
  const plays = (s, cnt) => {
    const c = s.cond | 0, loop = Math.floor(cnt / len);
    if (!c) return true;
    if (c < COND_1SHOT) return die() < (c - 1) * 5;
    if (c === COND_1SHOT) return loop === 0;
    if (c === COND_FILL || c === COND_NFILL) return (c === COND_FILL) === fill;
    const { a, b } = condAB(c);
    return loop % b === a - 1;
  };
  for (let i = 0; i < grid.length; i++) {
    const g = grid[i], s = steps[g.idx] ?? { flags: 0, cond: 0, note: 60, micro: 0 };
    const on = !!(s.flags & STEP_ON), tie = !!(s.flags & STEP_TIE);
    if (g.start >= span && !(i + 1 < grid.length)) break;
    const m = clamp(s.micro | 0, -50, 50);
    let at = g.start, lenS = g.len;
    if (m > 0) { const d = Math.floor(g.len * m / 100); at += d; lenS -= d; }
    else if (m < 0 && i > 0) { const p = grid[i - 1], d = Math.floor(p.len * (100 + m) / 100); at = p.start + d; lenS = p.len - d + g.len; }
    if (at >= span) break;
    if (tie) {
      if (gated && held) slideIn = !!(s.flags & STEP_SLIDE);
      continue; // a TIE holds the ACID note; a drum's TIE plays nothing (a roll goes on: below)
    }
    if (!on || !plays(s, g.cnt)) {
      if (held) { held.off = Math.min(held.off, g.start); held = null; }
      slideIn = false;
      continue;
    }
    const vel0 = s.flags & STEP_ACC ? 127 : 96, nt = (s.note | 0);
    const legato = gated && !!held && slideIn;
    if (held) { held.off = at; held = null; }
    const next = steps[(g.idx + 1) % len] ?? { flags: 0 };
    if (amode) { // ARP / RTRG: the roll through this step and the TIE steps after it
      const rate = ARATE_D[clamp(ap.rate | 0, 0, 6)], agate = clamp(ap.gate ?? 50, 1, 100), fade = clamp(ap.fade | 0, -64, 63);
      const dir = clamp(ap.dir | 0, 0, 3), chord = ARP_CHORD[clamp(ap.chord | 0, 0, 7)], rng4 = clamp(ap.range ?? 1, 1, 4);
      const cn = chord.length, n = cn * rng4, per = dir === 2 && n > 1 ? 2 * n - 2 : n;
      let ai = 0, k = 0, fr = 0, end = g.start + g.len, j = i, rt = at, alive = true;
      const rnote = () => {
        if (amode !== 2) return nt;
        const p = ai % per;
        ai = (p + 1) % per;
        let x;
        if (dir === 3) { arng = (Math.imul(arng, 1664525) + 1013904223) >>> 0; x = Math.floor(((arng >>> 16) * n) / 65536); }
        else x = dir === 1 ? n - 1 - p : p < n ? p : per - p;
        return nt + chord[x % cn] + 12 * Math.floor(x / cn);
      };
      const rvel = () => (fade >= 0 ? Math.min(vel0 - tdiv(vel0 * fade, 64) + tdiv(k * fade, 4), vel0) : vel0 + tdiv(k * fade, 4));
      while (alive) {
        const cur = grid[j].len, b = rate * num, q = cur * 4 * den + fr, iv = Math.max(1, Math.floor(q / b));
        fr = q % b;
        if (rt >= span) break;
        const v = rvel();
        if (k === 0 || v >= 1) {
          const gl = Math.max(1, Math.floor(iv * agate / 100));
          hits.push({ t: rt, note: noteOf(rnote()), vel: clamp(v, 1, 127), off: rt + gl, legato: k === 0 && legato });
        } else if (fade <= 0) alive = false;
        else rnote();
        k++;
        rt += iv;
        while (alive && rt >= end) { // the step ended: a TIE after it carries the roll on
          const nx = grid[j + 1];
          if (!nx || !(steps[nx.idx]?.flags & STEP_TIE) || nx.start >= span) { alive = false; break; }
          j++;
          end = nx.start + nx.len;
        }
      }
      slideIn = gated && !!(s.flags & STEP_SLIDE);
      continue;
    }
    const r = Math.min(((s.flags >> RAT_SHIFT) & 3) + 1, 4);
    for (let h = 0; h < r; h++) {
      const ht = at + Math.floor(h * lenS / r);
      if (ht >= span) break;
      let off;
      if (gated) {
        const last = h === r - 1;
        off = last && ((s.flags & STEP_SLIDE) || (next.flags & STEP_TIE)) ? Infinity : ht + Math.max(1, Math.floor(Math.floor(lenS / r) * gateK / 100));
      } else off = ht + Math.max(1, Math.floor(g.len / r));
      const hit = { t: ht, note: noteOf(nt), vel: vel0, off, legato: h === 0 && legato };
      hits.push(hit);
      if (off === Infinity) held = hit;
    }
    slideIn = gated && !!(s.flags & STEP_SLIDE);
  }
  // one note a track: a hit ends the one sounding; a legato hit of the same note goes on
  hits.sort((a, b) => a.t - b.t);
  const out = [];
  for (const h of hits) {
    const p = out[out.length - 1];
    if (p && p.off > h.t) {
      if (h.legato && p.note === h.note) { p.off = h.off; continue; }
      p.off = h.t;
      p.legato = h.legato;
    }
    if (p && p.off <= p.on) out.pop();
    out.push({ on: h.t, off: h.off, note: h.note, vel: h.vel, legato: false });
  }
  for (const n of out) n.off = Math.min(n.off, span);
  return out.filter((n) => n.off > n.on);
}

/** a part's length: MASTER LENGTH's 1/16s (the global swing's pairs keep it on the beat), else the longest track's
 * cycle */
export function partTicks(part, song, ppq = PPQ) {
  const ml = song.mlen | 0;
  if (ml >= 2) {
    const s16 = ppq / 4, sw = tdiv(clamp(song.swing | 0, 0, 100) * Math.floor(s16), 250);
    let t = 0;
    for (let c = 0; c < ml; c++) t += Math.floor(s16) + (c & 1 ? -sw : sw);
    return t;
  }
  let best = 0;
  for (const tr of part.tracks) if (tr.steps.some((s) => s.flags & STEP_ON)) best = Math.max(best, cycleTicks(tr, ppq, song.swing));
  return best || ppq * 4;
}

const vlq = (n) => {
  const b = [n & 0x7f];
  while ((n >>= 7)) b.unshift((n & 0x7f) | 0x80);
  return b;
};
const text = (s) => [...s].map((c) => c.charCodeAt(0) & 0x7f);
function chunk(id, body) {
  const n = body.length;
  return [...text(id), (n >>> 24) & 255, (n >>> 16) & 255, (n >>> 8) & 255, n & 255, ...body];
}
function mtrk(events) { // [{ t, b: [bytes] }] sorted
  const body = [];
  let last = 0;
  for (const e of events) { body.push(...vlq(e.t - last), ...e.b); last = e.t; }
  body.push(...vlq(0), 0xff, 0x2f, 0x00);
  return chunk("MTrk", body);
}

/**
 * The .mid file.
 * song: { bpm, swing (global), mlen (MASTER LENGTH, 0 OFF), name, tracks: [{ model, note (drum's), ch (1..16, 0 OFF),
 *   gate (ACID E7), arp: { mode, rate, gate, fade, dir, chord, range } }] }
 * parts: [{ label, tracks: [{ len, div, swing, steps[64] }] }] played in order (a chain; one part: a variation)
 * returns { bytes: Uint8Array, notes: [[{ on, off, note, vel, ch }] per track], length (ticks) }
 */
export function buildSmf(song, parts, opt = {}) {
  const ppq = opt.ppq ?? PPQ, ntrk = song.tracks.length, notes = song.tracks.map(() => []), loops = Math.max(1, opt.loops | 0);
  let at = 0;
  const marks = [];
  for (let l = 0; l < loops; l++) for (const part of parts) {
    const span = partTicks(part, song, ppq);
    marks.push({ t: at, label: part.label ?? "" });
    for (let k = 0; k < ntrk; k++) {
      const st = song.tracks[k], tr = { ...part.tracks[k], ...st, len: part.tracks[k].len, div: part.tracks[k].div, swing: part.tracks[k].swing, steps: part.tracks[k].steps };
      const ch = (st.ch | 0) >= 1 && st.ch <= 16 ? st.ch : k + 1;
      for (const n of renderTrack(tr, span, { ppq, swing: song.swing, seed: 0x2545f491 + k, fill: opt.fill })) notes[k].push({ ...n, on: n.on + at, off: n.off + at, ch });
    }
    at += span;
  }
  const bpm = clamp(song.bpm || 120, 20, 300), us = Math.round(60e6 / bpm);
  const t0 = [{ t: 0, b: [0xff, 0x03, ...vlq(text(song.name || "FREITAKT").length), ...text(song.name || "FREITAKT")] },
    { t: 0, b: [0xff, 0x51, 0x03, (us >> 16) & 255, (us >> 8) & 255, us & 255] },
    { t: 0, b: [0xff, 0x58, 0x04, 4, 2, 24, 8] }];
  for (const m of marks) if (m.label) t0.push({ t: m.t, b: [0xff, 0x06, ...vlq(text(m.label).length), ...text(m.label)] });
  const tracks = [mtrk(t0)];
  for (let k = 0; k < ntrk; k++) {
    const name = `T${k + 1}${song.tracks[k].name ? " " + song.tracks[k].name : ""}`;
    const ev = [{ t: 0, o: 0, b: [0xff, 0x03, ...vlq(text(name).length), ...text(name)] }];
    for (const n of notes[k]) {
      const c = (n.ch - 1) & 15;
      ev.push({ t: n.on, o: 2, b: [0x90 | c, n.note, n.vel] }, { t: n.off, o: 1, b: [0x80 | c, n.note, 0x40] });
    }
    ev.sort((a, b) => a.t - b.t || a.o - b.o); // at the same tick: the Offs before the Ons
    tracks.push(mtrk(ev));
  }
  const head = chunk("MThd", [0, 1, 0, ntrk + 1, (ppq >> 8) & 255, ppq & 255]);
  return { bytes: Uint8Array.from([...head, ...tracks.flat()]), notes, length: at };
}

/** a minimal reader (tests): { format, ntrks, ppq, tracks: [[{ t, st, d1, d2 } | { t, meta, data }]] } */
export function parseSmf(bytes) {
  const b = bytes, rd32 = (i) => ((b[i] << 24) | (b[i + 1] << 16) | (b[i + 2] << 8) | b[i + 3]) >>> 0;
  const out = { format: (b[8] << 8) | b[9], ntrks: (b[10] << 8) | b[11], ppq: (b[12] << 8) | b[13], tracks: [] };
  let p = 14;
  while (p < b.length) {
    const n = rd32(p + 4), end = p + 8 + n, ev = [];
    let i = p + 8, t = 0, rs = 0;
    const v = () => { let x = 0, c; do { c = b[i++]; x = (x << 7) | (c & 0x7f); } while (c & 0x80); return x; };
    while (i < end) {
      t += v();
      let st = b[i];
      if (st === 0xff) { const meta = b[i + 1]; i += 2; const l = v(); ev.push({ t, meta, data: Array.from(b.slice(i, i + l)) }); i += l; continue; }
      if (st & 0x80) i++; else st = rs;
      rs = st;
      ev.push({ t, st, d1: b[i], d2: b[i + 1] });
      i += 2;
    }
    out.tracks.push(ev);
    p = end;
  }
  return out;
}
