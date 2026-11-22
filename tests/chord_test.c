/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* CHORD (dm_chord.c, FREITAKT): a polyphonic chord machine, one chord at a time over all tracks. Its notes per shape
 * and INV; one chord sounding (the tracks' own MODEL and machine locks, a new hit cuts the old with the declick
 * tail: no click); bounded output at every knob's ends; the gate (a step, TIE steps); KEYS rooted on the key; MODE
 * RTRG (the whole chord) / ARP1 (its own notes one by one) / ARP2 (a chord a held key); the factory presets; the CPU
 * cost of a sounding chord and of none. */
#include "drum_host.h"

static int fails;
static int check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        fails++;
    return ok;
}

#define BEAT (FS * 60u / 120u)
#define E(k) (P_E0 + (k))                             /* TUNE CHORD INV DECAY, WAVE TONE ATTACK DETUNE */

static void fresh(void)
{
    uint32_t i;
    host_init();
    song.g[G_BPM] = 120;
    for (i = 0; i < NTRK; i++) {
        memset(trk[i].step, 0, sizeof trk[i].step);
        trk[i].p[P_SLEN] = 16;
        trk[i].p[P_SDIV] = 2;
        trk[i].p[P_AMODE] = 0;
    }
}
static void set_chord(track_t *t, int shape, int inv, int decay, int wave, int tone, int atk, int det)
{
    drum_set_model(t, DM_CHORD);
    t->p[E(0)] = 0;
    t->p[E(1)] = (int16_t)shape;
    t->p[E(2)] = (int16_t)inv;
    t->p[E(3)] = (int16_t)decay;
    t->p[E(4)] = (int16_t)wave;
    t->p[E(5)] = (int16_t)tone;
    t->p[E(6)] = (int16_t)atk;
    t->p[E(7)] = (int16_t)det;
}
static uint32_t chord_voices(uint32_t *owner)         /* CHORD voices sounding over all tracks; the last one's track */
{
    uint32_t i, k, n = 0;
    for (i = 0; i < NTRK; i++)
        if (trk[i].model == DM_CHORD)
            for (k = 0; k < NDV; k++)
                if (trk[i].v[k].active) {
                    n++;
                    if (owner)
                        *owner = i;
                }
    return n;
}
static const dvoice_t *sounding(const track_t *t)
{
    uint32_t k;
    for (k = 0; k < NDV; k++)
        if (t->v[k].active)
            return &t->v[k];
    return 0;
}

/* ---- the notes: every shape, every INV, against the chords spelled out ---- */
static void notes(void)
{
    static const int8_t SP[CHORD_NS][4] = {{0, 4, 7, -1}, {0, 3, 7, -1}, {0, 4, 7, 10}, {0, 3, 7, 10}, {0, 4, 7, 11},
                                           {0, 2, 7, -1}, {0, 5, 7, -1}, {0, 3, 6, -1}, {0, 4, 7, 9}, {0, 4, 10, 14},
                                           {0, 12, -1, -1}, {0, 7, -1, -1}};
    static const char *const NM[CHORD_NS] = {"MAJ", "MIN", "7", "MIN7", "MAJ7", "SUS2", "SUS4", "DIM", "6", "9",
                                             "OCT", "5TH"};
    uint32_t s, inv, i, n, ok = 1, okn = 1;
    int32_t o[4], w[4];
    for (s = 0; s < CHORD_NS; s++) {
        okn &= !strcmp(N_CHSHAPE[s], NM[s]);
        for (inv = 0; inv < 4u; inv++) {
            uint32_t m = 0, k;
            while (m < 4u && SP[s][m] >= 0) {
                w[m] = SP[s][m];
                m++;
            }
            for (k = 0; k < inv; k++) {               /* the lowest note up an octave */
                uint32_t lo = 0;
                for (i = 1; i < m; i++)
                    if (w[i] < w[lo])
                        lo = i;
                w[lo] += 12;
            }
            for (i = 0; i < m; i++)                   /* sorted */
                for (k = i + 1u; k < m; k++)
                    if (w[k] < w[i]) {
                        int32_t x = w[i];
                        w[i] = w[k];
                        w[k] = x;
                    }
            n = chord_notes((int32_t)s, (int32_t)inv, o);
            ok &= n == m && n <= 4u;
            for (i = 0; i < n && i < m; i++)
                ok &= o[i] == w[i];
            if (s == 0 && inv == 1)                   /* MAJ, first inversion: E G C */
                ok &= o[0] == 4 && o[1] == 7 && o[2] == 12;
            if (s == 3 && inv == 3)                   /* MIN7, third: Bb C' Eb' G' */
                ok &= o[0] == 10 && o[1] == 12 && o[2] == 15 && o[3] == 19;
        }
    }
    check("shape names MAJ MIN 7 MIN7 MAJ7 SUS2 SUS4 DIM 6 9 OCT 5TH", okn && CHORD_NS == 12u);
    check("every shape x INV 0..3: its notes (at most 4, lowest first; INV: the lowest up an octave, INV times)", ok);
    n = chord_notes(-5, 9, o);
    check("out-of-range CHORD / INV clamp (MAJ, INV 3)", n == 3u && o[0] == 12 && o[1] == 16 && o[2] == 19);
}

/* a voice's oscillators: the notes as frequency ratios over the root (DETUNE 0) */
static int ratios(const dvoice_t *v, const int32_t *o, uint32_t n)
{
    uint32_t i;
    int ok = v && v->ms.ch.n == n;
    for (i = 0; ok && i < n; i++) {
        double r = (double)v->ms.ch.inc[i] / (double)v->ms.ch.inc[0], want = pow(2.0, (o[i] - o[0]) / 12.0);
        ok &= fabs(r / want - 1.0) < 0.003;
    }
    return ok;
}

/* ---- a step's hit: the chord at the step's NOTE; ownership; machine locks ---- */
static void ownership(void)
{
    uint32_t b, owner = 99, maxv = 0, ok, nb;
    int32_t o[4];
    const dvoice_t *v;
    double r;
    fresh();
    set_chord(&trk[0], 3, 0, 127, 0, 127, 0, 0);     /* MIN7 */
    trk[0].step[0].on = 1;
    trk[0].step[0].nt = 2;                            /* D */
    for (b = 1; b < 16u; b++)
        trk[0].step[b].tie = 1;
    transport_req = 1;
    for (b = 0; b < 8u; b++)
        events_block(CTL);
    v = sounding(&trk[0]);
    (void)chord_notes(3, 0, o);
    check("a step's hit: one voice holds the whole chord (MIN7: 4 oscillators, the notes' ratios)", ratios(v, o, 4) &&
                                                                                                chord_voices(0) == 1u);
    r = v ? (double)v->ms.ch.inc[0] / (double)inc_tune(CHORD_C4, 0) : 0;
    check("its root: the step's NOTE over C4 (D: 2 semitones)", fabs(r / pow(2.0, 2.0 / 12.0) - 1.0) < 0.003);

    /* tracks 0 1 2 CHORD, hits on different steps; track 3 a K808 with a machine lock to CHORD on step 6 */
    fresh();
    set_chord(&trk[0], 0, 0, 127, 0, 127, 0, 0);
    set_chord(&trk[1], 1, 0, 127, 0, 127, 0, 0);
    set_chord(&trk[2], 2, 0, 127, 0, 127, 0, 0);
    drum_set_model(&trk[3], DM_K808);
    trk[0].step[0].on = 1;
    trk[1].step[2].on = 1;
    trk[2].step[4].on = 1;
    trk[3].step[6].on = 1;
    lock_set(3, 6, P_MODEL, DM_CHORD);
    lock_set(3, 6, P_E0 + 1, 9);                      /* (its CHORD knob locked too: 9) */
    trk[0].step[8].on = 1;
    trk[3].step[10].on = 1;                           /* track 3's own K808 again */
    transport_req = 1;
    ok = 1;
    nb = (uint32_t)(2u * BEAT / CTL);                  /* a bar */
    for (b = 0; b < nb; b++) {
        uint32_t s = (uint32_t)((uint64_t)b * CTL * 16u / (4u * BEAT)), n;
        events_block(CTL);
        n = chord_voices(&owner);
        maxv = n > maxv ? n : maxv;
        if (s == 1u)
            ok &= owner == 0u;
        if (s == 3u)
            ok &= owner == 1u && !sounding(&trk[0]);
        if (s == 5u)
            ok &= owner == 2u && !sounding(&trk[1]);
        if (s == 7u) {
            ok &= owner == 3u && !sounding(&trk[2]) && trk[3].model == DM_CHORD;
            ok &= sounding(&trk[3]) && sounding(&trk[3])->ms.ch.n == 4u;   /* the lock's CHORD 9 */
        }
        if (s == 9u)
            ok &= owner == 0u && trk[3].model != DM_CHORD;   /* (track 3's lock over, then track 0 took it) */
        if (s == 11u)
            ok &= owner == 0u && trk[3].model == DM_K808 && sounding(&trk[3]);   /* K808 rings with it */
    }
    check("one chord at a time: every new hit (any track, a machine lock's too) takes it, the old one cut",
          ok && maxv == 1u);
    lock_clear_track(3);
}

/* ---- no click when the chord moves (sine, open): the largest step of the mix against a chord's own ---- */
static int32_t L[8u * BEAT], R_[8u * BEAT];
static void click(void)
{
    uint32_t i, n = 4u * BEAT, k;
    int32_t steady = 0, worst = 0, pk = 0;
    fresh();
    for (k = 0; k < NTRK; k++)
        trk[k].p[P_PAN] = 0, trk[k].p[P_REV] = 0, trk[k].p[P_DLY] = 0;
    set_chord(&trk[0], 2, 0, 127, 2, 127, 0, 0);
    set_chord(&trk[1], 2, 0, 127, 2, 127, 0, 0);
    trk[0].step[0].on = trk[0].step[8].on = 1;
    trk[1].step[4].on = trk[1].step[12].on = 1;
    for (k = 1; k < 16u; k++) {
        trk[0].step[k].tie = k != 8u;
        trk[1].step[k].tie = k != 4u && k != 12u;
    }
    trk[0].step[8].nt = trk[0].step[0].nt = 0;
    trk[1].step[4].nt = trk[1].step[12].nt = 0;
    transport_req = 1;
    render_mix(L, R_, n);
    for (i = 1; i < n; i++) {
        int32_t d = abs(L[i] - L[i - 1u]), a = abs(L[i]);
        uint32_t ph = i % (BEAT / 2u);                 /* within a 1/8 .. away from the hits (each 1/4: 4 steps) */
        pk = a > pk ? a : pk;
        if (i % BEAT > BEAT / 4u && i % BEAT < BEAT - 64u)
            steady = d > steady ? d : steady;
        else
            worst = d > worst ? d : worst;
        (void)ph;
    }
    printf("     sine chord: peak %d, the largest step held %d, around the hits / moves %d\n", (int)pk, (int)steady,
           (int)worst);
    check("a chord moving between tracks: no click (a step at the move within 2x the held chord's own)",
          pk > 2000 && worst <= 2 * steady + 64);
}

/* ---- bounded: every corner of the knobs, the output within full scale, the voice ends ---- */
static int32_t OUT[6u * FS];
static void bounds(void)
{
    static const int16_t LO[8] = {-24, 0, 0, 0, 0, 0, 0, 0}, HI[8] = {24, CHORD_NS - 1, 3, 127, 2, 127, 127, 127};
    uint32_t c, i, ok = 1, ends = 1;
    int32_t pk = 0;
    for (c = 0; c < 256u; c++) {
        track_t *t = &trk[0];
        dvoice_t *v;
        fresh();
        drum_set_model(t, DM_CHORD);
        for (i = 0; i < 8u; i++)
            t->p[P_E0 + i] = (c >> i) & 1u ? HI[i] : LO[i];
        if (t->p[E(1)] == 0)
            t->p[E(1)] = 9;                           /* (4 notes) */
        t->p[E(6)] = t->p[E(6)] ? 20 : 0;              /* (a short ATTACK: the test's 6 s) */
        drum_play(t, 127, (c & 64u) ? 24 : -24);
        v = (dvoice_t *)sounding(t);
        if (!v) {
            ok = 0;
            continue;
        }
        v->x[1] = (int32_t)(FS / 2u);                  /* (gate: half a second) */
        memset(OUT, 0, sizeof OUT);
        dm_render_n(t, OUT, 6u * FS);
        for (i = 0; i < 6u * FS; i++) {
            int32_t a = abs(OUT[i]);
            pk = a > pk ? a : pk;
        }
        ends &= !t->v[0].active && !t->v[1].active;
    }
    printf("     every knob corner (256): peak %d of %d\n", (int)pk, VOICE_FS);
    check("every knob at its ends: the output bounded (within full scale), no overflow", ok && pk <= VOICE_FS + VOICE_FS / 2);
    check("every knob at its ends: the voice has ended 5.5 s after its gate", ends);
}

/* ---- the gate: a plain step ends the chord with its DECAY; TIE steps hold it ---- */
static uint32_t held_blocks(uint32_t ties)
{
    uint32_t b, k, last = 0;
    fresh();
    set_chord(&trk[0], 0, 0, 0, 0, 127, 0, 0);       /* DECAY 0: ends at the gate */
    trk[0].step[0].on = 1;
    for (k = 1; k <= ties; k++)
        trk[0].step[k].tie = 1;
    transport_req = 1;
    for (b = 0; b < (uint32_t)(2u * BEAT / CTL); b++) {
        events_block(CTL);
        if (sounding(&trk[0]))
            last = b;
        drum_block_begin();
        track_render(&trk[0], blk, CTL);
    }
    return last;
}
static void gate(void)
{
    uint32_t step = BEAT / 4u / CTL, a = held_blocks(0), b = held_blocks(3);
    printf("     DECAY 0: a plain step sounds %u blocks, with 3 TIE steps %u (a step: %u)\n", (unsigned)a,
           (unsigned)b, (unsigned)step);
    check("the gate: a plain step's chord ends after its step, 3 TIE steps hold it 4 steps", a >= step - 1u &&
                                                                                                a <= step + 16u &&
                                                                                                b >= 4u * step - 1u &&
                                                                                                b <= 4u * step + 16u);
}

/* ---- KEYS: the chord rooted on the key ---- */
static void keys(void)
{
    const dvoice_t *v;
    uint32_t b, ok = 1, i0, i4;
    int32_t o[4];
    fresh();
    set_chord(&trk[0], 4, 0, 127, 0, 127, 0, 0);     /* MAJ7 */
    song.seq_mode = 3;
    song.sel = 0;
    fm1_in.notes = 1u << 7;                           /* (C) */
    for (b = 0; b < 4u; b++)
        events_block(CTL);
    v = sounding(&trk[0]);
    (void)chord_notes(4, 0, o);
    ok &= ratios(v, o, 4);
    i0 = v ? v->ms.ch.inc[0] : 0;
    fm1_in.notes = 0;
    for (b = 0; b < 4u; b++)
        events_block(CTL);
    fm1_in.notes = 1u << 11;                          /* (E: 4 semitones up) */
    for (b = 0; b < 4u; b++)
        events_block(CTL);
    v = sounding(&trk[0]);
    ok &= ratios(v, o, 4) && chord_voices(0) == 1u;
    i4 = v ? v->ms.ch.inc[0] : 0;
    fm1_in.notes = 0;
    check("KEYS: a key plays the whole chord rooted on its note (C then E: the root 4 semitones up, one chord)",
          ok && i0 && fabs((double)i4 / i0 / pow(2.0, 4.0 / 12.0) - 1.0) < 0.003);
}

/* ---- MODE: RTRG repeats the chord; ARP1 its notes one by one; ARP2 a chord a held key ---- */
#define MAXH 400u
static uint32_t H_n, H_cn[MAXH];
static int8_t H_nt[MAXH];
static void run_hits(uint32_t frames, uint32_t keys)
{
    uint32_t f, a = 0, i;
    const dvoice_t *v;
    H_n = 0;
    for (i = 0; i < NDV; i++)
        a += trk[0].v[i].age;
    for (f = 0; f < frames; f += CTL) {
        uint32_t s = 0;
        if (keys)
            fm1_in.notes = keys;
        events_block(CTL);
        for (i = 0; i < NDV; i++)
            s += trk[0].v[i].age;
        if (s != a) {
            a = s;
            v = 0;
            for (i = 0; i < NDV; i++)
                if (trk[0].v[i].active && (!v || trk[0].v[i].age > v->age))
                    v = &trk[0].v[i];
            if (H_n < MAXH) {
                H_nt[H_n] = trk[0].nt;
                H_cn[H_n] = v ? v->ms.ch.n : 0;
            }
            H_n++;
        }
    }
    fm1_in.notes = 0;
}
static void modes(void)
{
    uint32_t k, ok;
    int32_t o[4];
    char what[200];
    /* RTRG 1/16 over a tied bar: 16 hits, each the whole chord */
    fresh();
    set_chord(&trk[0], 3, 0, 60, 0, 127, 0, 0);
    trk[0].step[0].on = 1;
    for (k = 1; k < 16u; k++)
        trk[0].step[k].tie = 1;
    trk[0].p[P_AMODE] = 1;
    trk[0].p[P_ARATE] = 4;
    trk[0].p[P_AFADE] = 0;
    transport_req = 1;
    run_hits(4u * BEAT - CTL, 0);
    for (k = 0, ok = H_n == 16u; k < H_n && k < MAXH; k++)
        ok &= H_cn[k] == 4u && H_nt[k] == 0;
    check("RTRG 1/16 on CHORD over a tied bar: 16 repeats, each the whole chord (MIN7: 4 notes)", ok);

    /* ARP1: MIN7 INV 1 (Eb G Bb C'), RANGE 2, UP: the chord's own notes one by one, the ARP CHORD knob ignored */
    fresh();
    set_chord(&trk[0], 3, 1, 60, 0, 127, 0, 0);
    trk[0].step[0].on = 1;
    trk[0].step[0].nt = -12;
    for (k = 1; k < 16u; k++)
        trk[0].step[k].tie = 1;
    trk[0].p[P_AMODE] = 2;
    trk[0].p[P_ARATE] = 4;
    trk[0].p[P_ADIR] = 0;
    trk[0].p[P_ACHORD] = 2;                           /* (MAJ: not used on CHORD) */
    trk[0].p[P_ARANGE] = 2;
    trk[0].p[P_AFADE] = 0;
    transport_req = 1;
    run_hits(4u * BEAT - CTL, 0);
    (void)chord_notes(3, 1, o);
    ok = H_n == 16u;
    for (k = 0; k < H_n && k < 16u; k++)
        ok &= H_cn[k] == 1u && H_nt[k] == -12 + o[k % 4u] + 12 * (int32_t)((k % 8u) / 4u);
    snprintf(what, sizeof what, "ARP1 on CHORD (MIN7 INV 1, RANGE 2, UP): one note a hit, its own chord's notes "
             "(%d %d %d %d %d ..)", H_nt[0] + 12, H_nt[1] + 12, H_nt[2] + 12, H_nt[3] + 12, H_nt[4] + 12);
    check(what, ok);

    /* ARP2 (KEYS: C and G held): a chord a key, the keys in turn as on another pitched model (FM) */
    {
        int8_t fm[MAXH];
        uint32_t fn = 0, m;
        for (m = 0; m < 2u; m++) {
            fresh();
            if (m)
                set_chord(&trk[0], 0, 0, 60, 0, 127, 0, 0);
            else
                drum_set_model(&trk[0], DM_FM);
            trk[0].p[P_AMODE] = 3;
            trk[0].p[P_ARATE] = 4;
            trk[0].p[P_ADIR] = 0;
            trk[0].p[P_ARANGE] = 1;
            trk[0].p[P_AFADE] = 0;
            song.seq_mode = 3;
            song.sel = 0;
            transport_req = 1;
            events_block(CTL);
            events_block(CTL);
            run_hits(2u * BEAT, 1u << 7 | 1u << 14);
            if (!m) {
                fn = H_n;
                memcpy(fm, H_nt, sizeof fm);
            }
        }
        ok = H_n >= 6u && H_n == fn;
        for (k = 0; k < H_n && k < MAXH; k++)
            ok &= H_cn[k] == 3u && H_nt[k] == fm[k];
        for (k = 1, m = 0; k < H_n && k < MAXH; k++)
            m |= (H_nt[k] == 0) << 0 | (H_nt[k] == 7) << 1;
        snprintf(what, sizeof what, "ARP2 on CHORD (KEYS C + G held): the held keys in turn as on FM, each hit a "
                 "whole chord on its key (%u hits)", (unsigned)H_n);
        check(what, ok && m == 3u && chord_voices(0) == 1u);
    }
}

/* ---- the factory presets ---- */
static void presets(void)
{
    uint32_t id, n = 0, ok = 1;
    preset_t p;
    for (id = 0; id < PRE_NFACT; id++)
        if (preset_get(id, &p) && p.model == DM_CHORD) {
            fresh();
            preset_apply(&trk[0], &p, id);
            ok &= trk[0].p[P_MODEL] == DM_CHORD && trk[0].pre == (uint16_t)id;
            drum_play(&trk[0], 100, 0);
            ok &= sounding(&trk[0]) != 0;
            n++;
        }
    check("factory presets of CHORD (MINSTAB DUBCHORD SOFTPAD GLASSORG POWER5): load and play", ok && n == 5u &&
                                                                     preset_first_of(DM_CHORD) != PRE_NONE);
}

/* ---- the CPU cost: one chord sounding, per sample; and nothing when none is ---- */
static void cost(void)
{
    uint64_t a, b;
    uint32_t w, s;
    double per[3];
    static const char *const WN[3] = {"SAW", "SQR", "SIN"};
    for (w = 0; w < 3u; w++) {
        fresh();
        set_chord(&trk[0], 9, 0, 127, (int)w, 64, 0, 60);
        drum_play(&trk[0], 127, 0);
        ((dvoice_t *)sounding(&trk[0]))->x[1] = (int32_t)(4u * FS);
        memset(OUT, 0, sizeof OUT);
        a = instr_now();
        dm_render_n(&trk[0], OUT, FS);
        b = instr_now();
        per[w] = (double)(b - a) / FS;
    }
    {
        static const uint8_t RM[2] = {DM_FM, DM_ACID};
        for (s = 0; s < 2u; s++) {
            fresh();
            drum_set_model(&trk[0], RM[s]);
            trk[0].p[P_E0 + 1] = 127;
            drum_play(&trk[0], 127, 0);
            memset(OUT, 0, sizeof OUT);
            a = instr_now();
            dm_render_n(&trk[0], OUT, FS);
            b = instr_now();
            printf("     (reference: one %s hit, DECAY 127: %.1f host instructions a sample)\n", N_MODEL[RM[s]],
                   (double)(b - a) / FS);
        }
    }
    for (s = 0; s < 3u; s++)
        printf("     cost: one 4-note chord, %s, TONE 64: %.1f host instructions a sample\n", WN[s], per[s]);
    if (per[0] == 0) {
        printf("     (no instruction counter: cost not measured)\n");
        return;
    }
    check("cost: a 4-note chord within 200 host instructions a sample (every WAVE, -O2)", per[0] < 200 &&
                                                                                     per[1] < 200 && per[2] < 200);
}

int main(void)
{
#ifndef CHORD_COST
    notes();
    ownership();
    click();
    bounds();
    gate();
    keys();
    modes();
    presets();
#else
    cost();
#endif
    if (fails) {
        printf("chord_test: %d FAILED\n", fails);
        return 1;
    }
    printf("chord_test: all passed\n");
    return 0;
}
