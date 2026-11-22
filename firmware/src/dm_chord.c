/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* CHORD (spec: the user's design 2026-11-22, Elektron Model:Cycles' CHORD machine): a polyphonic chord, up to 4
 * oscillators (SAW, SQR = two band-limited saws, SIN) on CHORD's shape over the playing note (TUNE + the step's NOTE /
 * the key's), INV inverting it, DETUNE spreading the notes, a 1-pole low-pass (TONE) on their sum, ATTACK's ramp, held
 * while the gate is open, then DECAY. One voice (dmodel voices 1): ONE CHORD AT A TIME over all tracks: a new chord hit
 * on any track (its MODEL or a machine lock) cuts the one sounding elsewhere (drum_core.c chord_claim: dv_cut, the
 * declick tail fades it), so the cost is one chord's on top of the other tracks'. The gate: SAMPL's with SUS (the same
 * voice fields: k[2] the SE_ bits, x[1] the samples left, inc[1] who holds; drum_core.c smpl_gated / smpl_hold /
 * smpl_tie): one step of the track (a roll's hit: GATE of its repeat), a TIE step goes on through, a held key / pad /
 * MIDI note holds it; ATTACK runs to its end even when the gate closes first (a pad on a short step swells, then
 * DECAY). The voice's other fields: env[0] the level (Q24), k[0] DECAY's factor, x[2] ATTACK's samples left, x[0] its
 * step, f[0] the low-pass state, ms.ch the oscillators */
#define CHORD_NS 12u                                    /* N_CHSHAPE */
static const int8_t CHORD_SHAPE[CHORD_NS][4] = {        /* semitones over the root; -1: no note */
    {0, 4, 7, -1}, {0, 3, 7, -1}, {0, 4, 7, 10}, {0, 3, 7, 10}, {0, 4, 7, 11}, {0, 2, 7, -1},
    {0, 5, 7, -1}, {0, 3, 6, -1}, {0, 4, 7, 9}, {0, 4, 10, 14}, {0, 12, -1, -1}, {0, 7, -1, -1}};
static const char *const N_CHSHAPE[CHORD_NS] = {"MAJ", "MIN", "7", "MIN7", "MAJ7", "SUS2", "SUS4", "DIM", "6", "9",
                                                "OCT", "5TH"};   /* 9: root 3rd 7th 9th (the 5th dropped) */
static const char *const N_CHWAVE[3] = {"SAW", "SQR", "SIN"};
static const uint8_t CHORD_GAIN[5] = {0, 24, 16, 13, 12};   /* each note's level by the notes sounding, /32 (the sum's
                                                         * peak <= 1.5 x full scale: mulq15 by the envelope fits) */
static const int8_t CHORD_DSPR[4] = {0, 2, -2, 1};      /* DETUNE: each note's share (the root stays in tune) */
#define CHORD_DET 1147                                  /* DETUNE 127: +-1.75 % (~30 cents) on the outer notes, Q16 */
#define CHORD_C4 HZ(261.63)                             /* TUNE 0, NOTE C4 */

static uint8_t chord_one;      /* seq.c: the roll's next hit is ARP1's on a CHORD voice, one note (drum_play's) */
static uint8_t chord_mono;     /* drum_play: the hit playing is such one note (the trigger, MIDI OUT) */
static void chord_claim(track_t *t, dvoice_t *v);       /* drum_core.c: one chord at a time */

/* CHORD's notes (semitones over the root, at most 4, lowest first) of shape s inverted inv times (INV: the lowest note
 * goes up an octave, inv times); returns how many */
static uint32_t chord_notes(int32_t s, int32_t inv, int32_t *o)
{
    const int8_t *c = CHORD_SHAPE[clamp(s, 0, CHORD_NS - 1)];
    uint32_t n = 0, i, k, lo;
    while (n < 4u && c[n] >= 0) {
        o[n] = c[n];
        n++;
    }
    for (k = 0; k < (uint32_t)clamp(inv, 0, 3); k++) {
        for (lo = 0, i = 1; i < n; i++)
            if (o[i] < o[lo])
                lo = i;
        o[lo] += 12;
    }
    for (i = 1; i < n; i++)                             /* (lowest first: ARP1's UP, MIDI OUT) */
        for (k = i; k > 0 && o[k] < o[k - 1u]; k--) {
            int32_t x = o[k];
            o[k] = o[k - 1u];
            o[k - 1u] = x;
        }
    return n;
}

/* CHORD: TUNE CHORD INV DECAY, WAVE TONE ATTACK DETUNE */
static void chord_trigger(track_t *t, dvoice_t *v)
{
    const int16_t *p = &t->p[P_E0];
    chord_t *c = &v->ms.ch;
    int32_t o[4], a = clamp(p[6], 0, 127), tn = clamp(p[5], 0, 127);
    uint32_t i, n, base = inc_tune(CHORD_C4, dm_tune(t)), w;
    int live = t >= trk && t < trk + NTRK;
    if (live && chord_mono) {
        n = 1;                                          /* ARP1's note: the roll's own (seq.c roll_note) */
        o[0] = 0;
    } else {
        n = chord_notes(p[1], p[2], o);
    }
    c->n = (uint8_t)n;
    c->wave = (uint8_t)clamp(p[4], 0, 2);
    for (i = 0; i < n; i++) {
        uint32_t inc = inc_tune(base, o[i]);
        int32_t d = clamp(p[7], 0, 127) * CHORD_DSPR[i] * CHORD_DET / 254;
        c->inc[i] = (uint32_t)(((uint64_t)inc * (uint32_t)(65536 + d)) >> 16);
        c->ph[i] = i * 0x3C000000u;                     /* (spread: no comb at the start) */
    }
    /* TONE: a 1-pole low-pass 100 Hz .. ~18 kHz (2 ^ (7.5 TONE / 127)); 127 open. a = w / (1 + w), Q12 */
    w = (pow2_q16(tn * 1440 / 127) * 934u) >> 16;
    c->a = tn >= 127 ? 4096 : (int32_t)((w << 12) / (65536u + w));
    v->env[0] = 0;
    v->k[0] = dk(p[3]);                                 /* DECAY: 5 ms .. 4 s (after the gate) */
    v->x[2] = (int32_t)smpl_atk_len(a ? a : 1);         /* ATTACK: 1 ms (0 and 1) .. 2 s */
    v->x[0] = ENV1 / v->x[2];
    v->x[1] = (int32_t)smpl_gate_len(t);
    v->k[2] = SE_ON | SE_SUS;
    v->f[0] = 0;
    if (live)
        chord_claim(t, v);
}

static inline int32_t chord_osc(uint32_t wave, uint32_t ph, uint32_t inc)
{
    return wave == 0u ? osc_saw(ph, inc) : wave == 1u ? osc_pulse(ph, inc, 0x80000000u) : sine_i(ph);
}

static void chord_render(track_t *t, dvoice_t *v, int32_t *out, uint32_t n)
{
    chord_t *c = &v->ms.ch;
    int32_t b[CTL], env = v->env[0], lp = v->f[0], a = c->a, g = CHORD_GAIN[c->n & 7u], st = v->x[0];
    uint32_t i, j, f = v->k[2], k = v->k[0], wave = c->wave, atk = (uint32_t)v->x[2];
    (void)t;
    if (n > CTL)
        n = CTL;
    for (i = 0; i < n; i++)
        b[i] = 0;
    for (j = 0; j < c->n; j++) {                        /* (the wave fixed over the loop) */
        uint32_t ph = c->ph[j], inc = c->inc[j];
        if (wave == 0u)
            for (i = 0; i < n; i++, ph += inc)
                b[i] += osc_saw(ph, inc);
        else if (wave == 1u)
            for (i = 0; i < n; i++, ph += inc)
                b[i] += osc_pulse(ph, inc, 0x80000000u);
        else
            for (i = 0; i < n; i++, ph += inc)
                b[i] += sine_i(ph);
        c->ph[j] = ph;
    }
    if ((f & (SE_SUS | SE_REL)) == SE_SUS) {            /* the gate counted (a held one: smpl_hold's) */
        if ((uint32_t)v->x[1] > n)
            v->x[1] -= (int32_t)n;
        else
            f |= SE_REL;
    }
    for (i = 0; i < n; i++) {
        int32_t x = (b[i] * g) >> 5;                    /* (<= 49152) */
        lp += ((x - lp) * a) >> 12;
        if (atk) {
            env += st;
            if (!--atk)
                env = ENV1;
        } else if (f & SE_REL) {
            env = mulq16(env, k);
        }
        dm_put(v, out, i, mulq15(lp, env_q15(env)));
    }
    v->env[0] = env;
    v->f[0] = lp;
    v->x[2] = (int32_t)atk;
    v->k[2] = f;
    if (!atk && (f & SE_REL) && env < ENV_END)
        dm_end(v);
}

#define DM_CHORD_DEF {"CHORD", 1, 0, {PD("PITCH", F_SEMI, -24, 24, 0), PE("CHORD", N_CHSHAPE, 1), \
    PD("INV", F_INT, 0, 3, 0), \
    PD("DECAY", F_INT, 0, 127, 40), PE("WAVE", N_CHWAVE, 0), PD("TONE", F_INT, 0, 127, 80), \
    PD("ATTACK", F_INT, 0, 127, 0), PD("DETUNE", F_INT, 0, 127, 20)}, chord_trigger, chord_render, 2}
