/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* ARP / RTRG (FREITAKT, seq.c roll_*): a track's MODE (OFF RTRG ARP; a step's lock too) turns a step's hit into a
 * roll over the step and the TIE steps after it: RTRG repeats it every RATE, FADE ramping the velocity; ARP plays the
 * step's note spread by CHORD over RANGE octaves in DIR's order (a pitched model's only: a drum plays RTRG). The
 * repeats sit on the step's own (swung, FIT) grid: no drift. Also the CPU cost with every track rolling at 1/32. */
#include "drum_host.h"

static int fails;
static int check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        fails++;
    return ok;
}

#define SECS(s) ((uint32_t)((s) * FS) / CTL * CTL)
#define SECS_MAX SECS(2)
#define CEILB(x) (((x) + CTL - 1u) / CTL)            /* a hit due at sample x plays in the first block at or after it */
#define MAXH 40000u
static uint32_t H_at[MAXH], H_n;                    /* the hits of track 0: block, note, velocity */
static int8_t H_nt[MAXH];
static uint8_t H_vel[MAXH];

static const dvoice_t *newest(const track_t *t)
{
    uint32_t i, a = 0, k = 0;
    for (i = 0; i < NDV; i++)
        if (t->v[i].age > a) {
            a = t->v[i].age;
            k = i;
        }
    return &t->v[k];
}
/* the sequencer alone (no audio: fast) for frames samples from PLAY, track 0's hits recorded */
static void run(uint32_t frames)
{
    uint32_t f, a;
    H_n = 0;
    transport_req = 1;
    a = hit_age(&trk[0]);
    for (f = 0; f < frames; f += CTL) {
        events_block(CTL);
        if (hit_age(&trk[0]) != a) {
            a = hit_age(&trk[0]);
            if (H_n < MAXH) {
                H_at[H_n] = f / CTL;
                H_nt[H_n] = trk[0].nt;
                H_vel[H_n] = newest(&trk[0])->vel;
            }
            H_n++;
        }
    }
}

static void fresh(uint32_t model)
{
    uint32_t i;
    host_init();
    song.g[G_BPM] = 120;
    for (i = 0; i < NTRK; i++) {
        memset(trk[i].step, 0, sizeof trk[i].step);
        trk[i].p[P_SLEN] = 16;
        trk[i].p[P_SDIV] = 2;                          /* 1/16 */
    }
    drum_set_model(&trk[0], model);
    trk[0].p[P_AMODE] = 1;
    trk[0].p[P_AFADE] = 0;
}
/* step 0 on, the rest of the 16 TIE: one roll over the bar */
static void bar_roll(void)
{
    uint32_t k;
    trk[0].step[0].on = 1;
    for (k = 1; k < 16u; k++)
        trk[0].step[k].tie = 1;
}

#define BEAT (FS * 60u / 120u)                       /* 22050 */
static const uint32_t RATE_D[7] = {4, 6, 8, 12, 16, 24, 32};   /* N_ARATE: repeats a whole note */

/* RTRG: every rate, one roll over a bar of TIE steps: the hit count, each on its place (a whole note / D apart) */
static void rtrg_rates(void)
{
    uint32_t r, k, ok = 1;
    for (r = 0; r < 7u; r++) {
        uint32_t want = RATE_D[r], okr = H_n == want;
        fresh(DM_K808);
        bar_roll();
        trk[0].p[P_ARATE] = (int16_t)r;
        run(4u * BEAT - CTL);
        okr = H_n == want;
        for (k = 0; k < H_n && k < want; k++) {
            uint32_t at = (uint32_t)((uint64_t)k * 4u * BEAT / want), b = CEILB(at);
            okr &= H_at[k] + 1u >= b && H_at[k] <= b + 1u;
        }
        if (!okr)
            printf("     rate %s: %u hits (want %u)\n", TP[P_ARATE].names[r], (unsigned)H_n, (unsigned)want);
        ok &= okr;
    }
    check("RTRG: 1/4 4T 1/8 8T 1/16 16T 1/32 over a tied bar: 4 6 8 12 16 24 32 hits, each on its grid place", ok);
    fresh(DM_K808);
    bar_roll();
    trk[0].p[P_AMODE] = 0;
    run(4u * BEAT - CTL);
    check("MODE OFF: the tied bar plays its one hit", H_n == 1u);
    fresh(DM_K808);
    trk[0].step[0].on = 1;                            /* no TIE: the roll ends with the step */
    trk[0].step[4].on = 1;
    trk[0].p[P_ARATE] = 6;                            /* 1/32: 2 a 1/16 step */
    run(4u * BEAT - CTL);
    check("RTRG: a plain step's roll ends with it (2 steps at 1/32: 4 hits)", H_n == 4u);
    fresh(DM_K808);
    trk[0].step[0].on = 1;
    trk[0].step[0].rat = 3;                           /* RATCH 4 */
    trk[0].p[P_AMODE] = 0;
    run(BEAT - CTL);
    ok = H_n == 4u;
    fresh(DM_K808);
    trk[0].step[0].on = 1;
    trk[0].step[0].rat = 3;
    trk[0].p[P_ARATE] = 6;
    run(BEAT - CTL);
    check("RATCH: plays as before with MODE OFF; a roll replaces it (1/32: 2 hits a 1/16 step)", ok && H_n == 2u);
}

/* FADE: each repeat's velocity, - down by f / 4 a repeat to silence, + up from quiet to the hit's, 0 flat */
static void fade(void)
{
    uint32_t k, ok = 1;
    fresh(DM_K808);
    bar_roll();
    trk[0].p[P_ARATE] = 4;                            /* 1/16 */
    run(4u * BEAT - CTL);
    for (k = 0; k < H_n; k++)
        ok &= H_vel[k] == 96u;
    check("FADE 0: every repeat at the hit's velocity", ok && H_n == 16u);
    fresh(DM_K808);
    bar_roll();
    trk[0].p[P_ARATE] = 4;
    trk[0].p[P_AFADE] = -32;
    run(4u * BEAT - CTL);
    ok = H_n == 12u;                                  /* 96, 88 .. 8: then silent, the roll over */
    for (k = 0; k < H_n; k++)
        ok &= H_vel[k] == 96u - 8u * k;
    check("FADE -32: 96 88 80 .. 8, then the roll ends (12 hits)", ok);
    fresh(DM_K808);
    bar_roll();
    trk[0].step[0].acc = 1;
    trk[0].p[P_ARATE] = 4;
    trk[0].p[P_AFADE] = 32;
    run(4u * BEAT - CTL);
    ok = H_n == 16u;                                  /* 127 - 63 = 64, 72 .. 127 */
    for (k = 0; k < H_n; k++)
        ok &= H_vel[k] == (64u + 8u * k < 127u ? 64u + 8u * k : 127u);
    check("FADE +32 (accent): 64 72 80 .. up to the hit's 127, then flat", ok);
}

/* ARP: the notes in DIR's order over CHORD x RANGE */
static int arp_seq(uint32_t model, int dir, int chord, int range, const int8_t *want, uint32_t n)
{
    uint32_t k, ok;
    fresh(model);
    bar_roll();
    trk[0].step[0].nt = -12;
    trk[0].p[P_AMODE] = 2;
    trk[0].p[P_ARATE] = 4;
    trk[0].p[P_ADIR] = (int16_t)dir;
    trk[0].p[P_ACHORD] = (int16_t)chord;
    trk[0].p[P_ARANGE] = (int16_t)range;
    run(4u * BEAT - CTL);
    ok = H_n == 16u;
    for (k = 0; k < H_n && k < 16u; k++)
        ok &= H_nt[k] == -12 + want[k % n];
    if (!ok && dir != 3) {
        printf("     model %s dir %d chord %d range %d:", N_MODEL[model], dir, chord, range);
        for (k = 0; k < H_n && k < 16u; k++)
            printf(" %d", H_nt[k] + 12);
        printf("\n");
    }
    return (int)ok;
}
static void arp_order(void)
{
    static const int8_t UP[6] = {0, 4, 7, 12, 16, 19}, DN[6] = {19, 16, 12, 7, 4, 0};
    static const int8_t UD[10] = {0, 4, 7, 12, 16, 19, 16, 12, 7, 4};
    static const int8_t M7[4] = {0, 3, 7, 10}, OCT4[4] = {0, 12, 24, 36}, FIFTH3[6] = {0, 7, 12, 19, 24, 31};
    static const int8_t ROOT[1] = {0};
    static const uint8_t PITCHED[4] = {DM_ACID, DM_SMPL, DM_FM, DM_MEMB};
    uint32_t k, ok = 1, seen = 0, inset = 1;
    int8_t first[16];
    check("ARP UP: MAJ over 2 octaves: 0 4 7 12 16 19, again", arp_seq(DM_FM, 0, 2, 2, UP, 6));
    check("ARP DOWN: 19 16 12 7 4 0", arp_seq(DM_FM, 1, 2, 2, DN, 6));
    check("ARP UPDN: the ends once (0 4 7 12 16 19 16 12 7 4, again)", arp_seq(DM_FM, 2, 2, 2, UD, 10));
    check("ARP CHORD MIN7, RANGE 1: 0 3 7 10", arp_seq(DM_FM, 0, 6, 1, M7, 4));
    check("ARP CHORD OCT, RANGE 4: 0 12 24 36", arp_seq(DM_FM, 0, 0, 4, OCT4, 4));
    check("ARP CHORD 5TH, RANGE 3: 0 7 12 19 24 31", arp_seq(DM_FM, 0, 1, 3, FIFTH3, 6));
    for (k = 0; k < 4u; k++)
        ok &= arp_seq(PITCHED[k], 0, 2, 2, UP, 6);
    check("ARP plays on the pitched models: ACID, SAMPL, FM, SKIN", ok);
    (void)arp_seq(DM_FM, 3, 2, 2, UP, 6);            /* RND: notes of the set, varied, the same after PLAY */
    for (k = 0; k < 16u && k < H_n; k++) {
        int d = H_nt[k] + 12, j, in = 0;
        for (j = 0; j < 6; j++)
            in |= d == UP[j];
        inset &= in;
        seen |= 1u << (d % 32);
        first[k] = H_nt[k];
    }
    run(4u * BEAT - CTL);
    ok = H_n == 16u;
    for (k = 0; k < 16u && k < H_n; k++)
        ok &= H_nt[k] == first[k];
    check("ARP RND: the set's notes, at least 4 different, the same sequence after every PLAY",
          inset && ok && __builtin_popcount(seen) >= 4);
    check("ARP on a drum (K808): RTRG (the step's note repeated)", arp_seq(DM_K808, 0, 2, 2, ROOT, 1));
    fresh(DM_K808);
    check("MODE on a drum: OFF RTRG only (its knob's top RTRG)", track_desc(&trk[0], P_AMODE)->max == 1 &&
          track_desc(&trk[1], P_AMODE) != 0);
    fresh(DM_FM);
    check("MODE on a pitched model: OFF RTRG ARP", track_desc(&trk[0], P_AMODE)->max == 2);
}

/* a step's lock: MODE (and RATE) per step */
static void step_lock(void)
{
    uint32_t ok;
    fresh(DM_FM);
    trk[0].p[P_AMODE] = 0;
    trk[0].step[0].on = 1;
    trk[0].step[4].on = 1;
    trk[0].step[8].on = 1;
    lock_set(0, 4, P_AMODE, 1);                       /* step 4: RTRG 1/32 (2 hits) */
    lock_set(0, 4, P_ARATE, 6);
    lock_set(0, 8, P_AMODE, 2);                       /* step 8: ARP, 1/16's notes of its own step: 1 */
    lock_set(0, 8, P_ARATE, 6);
    lock_set(0, 8, P_ACHORD, 2);
    run(4u * BEAT - CTL);
    ok = H_n == 5u && H_nt[3] == 0 && H_nt[4] == 4;
    check("a step's lock: MODE RTRG on step 4 (2 hits), ARP on step 8 (0 then 4), the others plain (5 hits)", ok);
    lock_clear_track(0);
}

/* swing and FIT: the repeats on the step's own (swung) grid */
static uint32_t S0[20000];
static void starts(uint32_t num, uint32_t den, int32_t sw, uint32_t n)
{
    uint32_t rem = 0, c, at = 0;
    int32_t s = sw * (int32_t)((BEAT * num) / den) / 250;
    for (c = 0; c < n; c++) {
        uint32_t per = (BEAT * num + rem) / den;
        S0[c] = at;
        rem = (BEAT * num + rem) % den;
        at += per + (uint32_t)((c & 1u) ? -s : s);
    }
}
static int on_grid(uint32_t nsteps, uint32_t per_step_num, uint32_t per_step_den)   /* each step's hits at its own
                                                                                       * share k * len * n / d */
{
    uint32_t c, k, h = 0, ok = 1;
    for (c = 0; c + 1u < nsteps; c++) {
        uint32_t len = S0[c + 1u] - S0[c];
        for (k = 0;; k++) {
            uint32_t at = S0[c] + (uint32_t)((uint64_t)k * len * per_step_den / per_step_num), b = CEILB(at);
            if ((uint64_t)k * per_step_den >= per_step_num)
                break;
            ok &= h < H_n && H_at[h] + 1u >= b && H_at[h] <= b + 1u;
            h++;
        }
    }
    return (int)ok;
}
static void swing_fit(void)
{
    uint32_t k;
    fresh(DM_K808);
    for (k = 0; k < 16u; k++)
        trk[0].step[k].on = 1;
    song.g[G_SWING] = 40;
    trk[0].p[P_ARATE] = 6;                            /* 1/32: 2 a step, halfway through each (swung) step */
    starts(1, 4, 40, 70);
    run(S0[64]);
    check("swing 40: every 1/16 step's 2 repeats at its start and its own half (long / short steps)",
          H_n == 128u && on_grid(64, 2, 1));
    fresh(DM_K808);
    for (k = 0; k < 5u; k++)
        trk[0].step[k].on = 1;
    trk[0].p[P_SLEN] = 5;
    trk[0].p[P_SDIV] = DIV_FIT + 1;                   /* FBAR: 5 steps a bar */
    trk[0].p[P_ARATE] = 6;                            /* 1/32: a step (4/5 beat) 6.4 of them: 7 hits */
    starts(4, 5, 0, 70);
    run(S0[60]);
    check("FIT (FBAR, LEN 5): each step's 1/32s from its start (7 a step: 0 .. 6/6.4 of it)", on_grid(60, 32, 5));
}

/* 1000 bars: the last bar's repeats on the clock's grid exactly (16T: 1.5 a step, the second 2/3 into the step) */
static void no_drift(void)
{
    uint32_t k, c = 16000u - 16u, ok;
    fresh(DM_K808);
    song.g[G_BPM] = 133;                              /* (a beat of no whole 1/16s: remainders) */
    for (k = 0; k < 16u; k++)
        trk[0].step[k].on = 1;
    trk[0].p[P_ARATE] = 5;
    run(16000u * (FS * 60u / 133u) / 4u + 4u * CTL);
    {
        uint32_t beat = FS * 60u / 133u, rem = 0, at = 0, i;
        for (i = 0; i < c; i++) {
            at += (beat + rem) / 4u;
            rem = (beat + rem) % 4u;
        }
        /* the hits: 2 a step; hit 2c at the step's start, 2c+1 2/3 into it */
        {
            uint32_t len = (beat + rem) / 4u, b0 = CEILB(at), b1 = CEILB(at + len * 2u / 3u);
            ok = H_n >= 2u * c + 2u && H_at[2u * c] + 1u >= b0 && H_at[2u * c] <= b0 + 1u &&
                 H_at[2u * c + 1u] + 1u >= b1 && H_at[2u * c + 1u] <= b1 + 1u;
        }
    }
    check("1000 bars at 133 BPM, 16T: the last bar's repeats on the step clock's grid (no drift)", ok);
}

/* a tempo change mid-roll: the repeats follow (none past the step's end, the count per step by the new tempo) */
static void tempo_change(void)
{
    uint32_t f, n0;
    fresh(DM_K808);
    bar_roll();
    trk[0].p[P_ARATE] = 6;                            /* 1/32 */
    run(2u * BEAT);                                   /* half the bar: 16 hits */
    n0 = H_n;
    song.g[G_BPM] = 240;                              /* the other half in 1 beat of 120: 16 more */
    for (f = 0; f < BEAT - 2u * CTL; f += CTL) {
        uint32_t a = hit_age(&trk[0]);
        events_block(CTL);
        n0 += hit_age(&trk[0]) != a;
    }
    check("tempo 120 -> 240 mid-roll: 16 + 16 repeats of 1/32 in the bar", H_n == 16u && n0 == 32u);
}

static double cost_key(const char *key)             /* tests/drum_cost_ref.txt "key value" */
{
    FILE *f = fopen("tests/drum_cost_ref.txt", "r");
    char line[160], k[32];
    double v, r = 0;
    while (f && fgets(line, sizeof line, f))
        if (line[0] != '#' && sscanf(line, "%31s %lf", k, &v) == 2 && !strcmp(k, key))
            r = v;
    if (f)
        fclose(f);
    return r;
}
/* CPU: the realistic heavy kit (drum_test test_cost) as is, with RTRG 1/32 on all 6 tracks, with ARP 1/32 on all 6
 * (pitched: FM SAMPL ACID SKIN FM FM). Host instructions / sample (0 where the counter is missing) */
static double heavy(int mode, int pitched, uint32_t ratch)
{
    static const char *const PAT[NTRK] = {
        "x...x...x...x..x", "....x.......x...", "....x.......x..x", "x.x.x.x.x.x.x.x.",
        "..x...x...x...x.", "......x....x....",
    };
    static const uint8_t PM[NTRK] = {DM_FM, DM_SMPL, DM_ACID, DM_MEMB, DM_FM, DM_FM};
    static int32_t wl[SECS_MAX], wr[SECS_MAX];
    uint32_t i, k;
    uint64_t i0;
    host_init();
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        if (pitched)
            drum_set_model(t, PM[i]);
        for (k = 0; k < 16; k++)
            t->step[k].on = PAT[i][k] == 'x', t->step[k].rat = ratch & 3u;
        t->p[P_DIST] = 40;
        t->p[P_CHOR] = t->p[P_DLY] = t->p[P_REV] = 40;
        t->p[P_AMODE] = (int16_t)mode;
        t->p[P_ARATE] = 6;
        t->p[P_ACHORD] = 5;
        t->p[P_ARANGE] = 2;
    }
    transport_req = 1;
    render_mix(0, 0, SECS(0.5));
    i0 = instr_now();
    render_mix(wl, wr, SECS(2));
    return i0 ? (double)(instr_now() - i0) / SECS(2) : 0;
}
static void cpu(void)
{
    double off = heavy(0, 0, 0), rt = heavy(1, 0, 0), rat = heavy(0, 0, 1), poff = heavy(0, 1, 0), arp = heavy(2, 1, 0);
    double prat = heavy(0, 1, 1), ref = cost_key("ref"), emax = cost_key("extreme_max");
    printf("     heavy kit (host instructions / sample): OFF %.0f, RTRG 1/32 x6 %.0f, RATCH 2 x6 %.0f; pitched kit OFF "
           "%.0f, ARP 1/32 x6 %.0f, RATCH 2 x6 %.0f (reference %.0f, extreme %.0f)\n", off, rt, rat, poff, arp, prat, ref,
           emax);
#ifndef ARP_COST
    printf("     (cost: measured by the -O2 build, arp_cost)\n");
#else
    check("cost: the realistic heavy kit (MODE OFF) within the stock reference", !off || ref == 0 || off <= ref);
    check("cost: RTRG 1/32 on all 6 tracks costs what RATCH 2 on every hit does (the voices; +2 % at most)",
          !off || rt <= rat * 1.02);
    check("cost: ARP 1/32 on all 6 (pitched) tracks costs what RATCH 2 on every hit does (+2 % at most)",
          !off || arp <= prat * 1.02);
    check("cost: either within the extreme kit's limit", !off || emax == 0 || (rt <= emax && arp <= emax));
#endif
}

int main(void)
{
    setvbuf(stdout, 0, _IONBF, 0);
    rtrg_rates();
    fade();
    arp_order();
    step_lock();
    swing_fit();
    no_drift();
    tempo_change();
    cpu();
    printf("%s\n", fails ? "ARP TESTS FAILED" : "all ARP tests passed");
    return fails ? 1 : 0;
}
