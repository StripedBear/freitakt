/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* FREITAKT's polyrhythm and pattern flow, with the firmware's real code on a host flash (tests/flash_host.h), built
 * with ASan + UBSan: FIT (a track's LEN steps spread over F1/2 FBAR F2BR F4BR, seq.c trk_num_den), MASTER LENGTH
 * (cue.c: every track restarted after MLEN 1/16s), the chain (project.c chain_*: slots in turn, a pattern each). */
#define FELUCCA_FLASH 1
#include <stdint.h>
static uint8_t hflash[0x100000];
#define SMP_USER_XIP(k) ((const uint8_t *)hflash + 0xA0000u + (k) * 0x14000u)
#define VAR_XIP(off) ((const void *)(hflash + (off)))
#include "ui_host.h"

static int fails;
static int fail_line;
static int check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) {
        fails++;
        if (fail_line)
            printf("     (first failed at line %d)\n", fail_line);
    }
    fail_line = 0;
    return ok;
}
#define OK(e) do { uint32_t ok_ = (e) ? 1u : 0u; if (!ok_ && !fail_line) fail_line = __LINE__; ok &= ok_; } while (0)

enum { F12 = DIV_FIT, FBAR, F2BR, F4BR };

static void wipe_ram(void)
{
    memset(&mo, 0, sizeof mo);
    memset(proj_slot, 0, sizeof proj_slot);
    memset(lk_slot, 0, sizeof lk_slot);
    memset(var_buf, 0, sizeof var_buf);
    memset(var_who, 0, sizeof var_who);
    var_play = 0;
    var_dirty = 0;
}
static void fresh(void)                               /* nothing in flash, booted, stopped */
{
    uint32_t k;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    memset(&ui, 0, sizeof ui);
    wipe_ram();
    persist_boot();
    for (k = 0; k < NTRK; k++) {
        memset(trk[k].step, 0, sizeof trk[k].step);
        trk[k].p[P_SSWING] = 0;
    }
    song.g[G_SWING] = 0;
    song.g[G_BPM] = 120;
    song.mlen = 0;
}
static void timing(uint32_t k, uint32_t len, uint32_t div)
{
    trk[k].p[P_SLEN] = (int16_t)len;
    trk[k].p[P_SDIV] = (int16_t)div;
}

/* the sequencer alone (events_block): each track's step starts (samples since PLAY, as now - seq_pos) */
static uint32_t now_s;
static uint32_t ppos[NTRK];
static void play_seq(void)
{
    uint32_t k;
    transport_req = 1;
    now_s = 0;
    events_block(CTL);
    now_s = CTL;
    for (k = 0; k < NTRK; k++)
        ppos[k] = trk[k].seq_pos;
}
/* one block; bit k of the result: track k's step began in it (*st: its start) */
static uint32_t seq_blk(uint32_t *st)
{
    uint32_t k, b = 0;
    events_block(CTL);
    now_s += CTL;
    for (k = 0; k < NTRK; k++) {
        if (trk[k].seq_pos != ppos[k] + CTL) {
            b |= 1u << k;
            st[k] = now_s - trk[k].seq_pos;
        }
        ppos[k] = trk[k].seq_pos;
    }
    return b;
}
static void stop_seq(void)
{
    transport_req = 2;
    events_block(CTL);
}

/* track 0 FIT (len over div), track 1 1/4 LEN 4: every FIT step starts at floor(cnt x num x beat / len) since step 0,
 * every bar's first step together with the 1/4 track's, for `bars` of the FIT span */
static int fit_exact(uint32_t len, uint32_t div, uint32_t bars, uint32_t bpm)
{
    uint32_t ok = 1, st[NTRK], t0 = 0, n = 0, num = 2u << (div - DIV_FIT), bs, b, last_q = 0, nq = 0;
    fresh();
    song.g[G_BPM] = (int16_t)bpm;
    timing(0, len, div);
    timing(1, 4, 0);
    play_seq();
    bs = beat_samples();
    t0 = CTL;                                         /* (PLAY's block: every step 0 begins at its end) */
    while (n < bars * len) {
        b = seq_blk(st);
        if (b & 2u) {
            nq++;
            last_q = st[1];
            OK(st[1] - t0 == nq * bs);
        }
        if (b & 1u) {
            n++;
            OK(st[0] - t0 == (uint32_t)((uint64_t)n * num * bs / len));
            OK(trk[0].seq_idx == n % len);
            if (trk[0].seq_idx == 0u && num >= 4u)
                OK(last_q == st[0]);                  /* (the 1/4 track's step began there too: a beat's start) */
        }
        if (!ok)
            break;
    }
    stop_seq();
    if (!ok)
        printf("     LEN %u DIV %u: step %u at %u, want %u\n", len, div, n, st[0] - t0,
               (uint32_t)((uint64_t)n * num * bs / len));
    return (int)ok;
}
static void fit_timing(void)
{
    uint32_t ok = 1;
    OK(fit_exact(5, FBAR, 1000, 123));                /* (123 BPM: 21512 samples a beat, 4 beats / 5 not whole) */
    check("FIT: LEN 5 over FBAR, 5 steps in exactly 4 beats (to the sample), 1000 bars, a 1/4 track's beat with every "
          "bar's first", (int)ok);
    ok = 1;
    OK(fit_exact(7, F2BR, 200, 123));
    OK(fit_exact(3, F12, 300, 97));
    OK(fit_exact(64, F4BR, 20, 133));
    OK(fit_exact(1, FBAR, 50, 120));
    check("FIT: LEN 7 over F2BR, 3 over F1/2, 64 over F4BR, 1 over FBAR: exact, no drift", (int)ok);
}

/* 5 hits in 4 beats, rendered (the voices: a hit counter) */
static void fit_hits(void)
{
    uint32_t ok = 1, a, e, hits = 0;
    int32_t L[CTL], R[CTL];
    fresh();
    timing(0, 5, FBAR);
    for (a = 0; a < 5u; a++)
        trk[0].step[a].on = 1;
    trk[0].step[7].on = 1;                            /* (past LEN: never) */
    transport_req = 1;
    render_mix(L, R, CTL);
    a = dvage;
    for (e = CTL; e + CTL <= CTL + 4u * beat_samples() - CTL; e += CTL) {   /* the first 4 beats, short of the bar */
        render_mix(L, R, CTL);
        hits += dvage - a;
        a = dvage;
    }
    hits++;                                           /* (step 0: PLAY's block) */
    OK(hits == 5u);
    transport_req = 2;
    render_mix(L, R, CTL);
    check("FIT: 5 steps on, LEN 5 over FBAR: 5 hits in the bar", (int)ok);
}

/* LEN changed while a FIT track plays: from the next step on its steps are the new LEN's (a sample apart at most),
 * its index stays inside LEN, its remainder below the new LEN after a step */
static void fit_len_change(void)
{
    uint32_t ok = 1, st[NTRK], prev = 0, n = 0, i, bs, have = 0;
    static const uint8_t L2[4] = {3, 9, 16, 2};
    fresh();
    song.g[G_BPM] = 123;
    timing(0, 5, FBAR);
    play_seq();
    bs = beat_samples();
    for (i = 0; i < 4u; i++) {
        uint32_t lo, steps = 0, first = 1;
        while (now_s < (i + 1u) * 3u * bs + bs / 3u)  /* (mid step) */
            if (seq_blk(st) & 1u)
                prev = st[0];
        trk[0].p[P_SLEN] = L2[i];
        lo = 4u * bs / L2[i];
        while (steps < 2u * L2[i] + 2u) {
            if (seq_blk(st) & 1u) {
                uint32_t d = st[0] - prev;
                if (!first || have)
                    OK(first ? d <= (lo > 4u * bs / 5u ? lo : 4u * bs / 5u) + 1u + CTL : d >= lo && d <= lo + 1u);
                                                          /* (the step under way: the new length, or (past it) ends in
                                                           * the next block) */
                first = 0;
                have = 1;
                prev = st[0];
                steps++;
                n++;
                OK(trk[0].seq_idx < L2[i] && trk[0].seq_rem < L2[i]);
            }
        }
        trk[0].p[P_SLEN] = 5;                         /* (back: the next change from LEN 5) */
    }
    stop_seq();
    (void)n;
    check("FIT: LEN 5 -> 3, 9, 16, 2 while playing (FBAR): the next steps the new LEN's, index and remainder inside it",
          (int)ok);
}

/* the delay's TIME keeps its own ids (N_DTIME: 4T 1/4D 1/8D 1/16D 32T at 10..14), never FIT's; the knob order shows
 * every DIV once */
static void fit_ids(void)
{
    uint32_t ok = 1, i, seen = 0;
    OK(GP[G_DTIME].names == N_DTIME && GP[G_DTIME].max == 14);
    OK(!strcmp(N_DTIME[10], "4T") && !strcmp(N_DTIME[14], "32T") && !strcmp(N_DIV[DIV_FIT], "F1/2") &&
       !strcmp(N_DIV[DIV_MAX], "F4BR") && TP[P_SDIV].max == DIV_MAX);
    song.g[G_BPM] = 120;
    OK(div_samples(10) == 22050u * 2u / 3u && div_samples(13) == 22050u * 3u / 8u);   /* (4T, 1/16D: as before) */
    for (i = 0; i <= DIV_MAX; i++)
        seen |= 1u << DIV_ORDER[i];
    OK(seen == (1u << (DIV_MAX + 1)) - 1u);
    check("FIT: the delay's TIME ids untouched (FIT is N_DIV's only); DIV's knob order holds every id once", (int)ok);
}

/* ---- MASTER LENGTH */
/* tracks LEN 5 / 12 / 16 (1/16), MLEN 64: every 64 1/16s each is at its step 0, begun exactly there, one hit there */
static void mlen_restarts(void)
{
    uint32_t ok = 1, st[NTRK], k, b, m = 0, bs, len[3] = {5, 12, 16}, hitk[3] = {0, 0, 0};
    int32_t L[CTL], R[CTL];
    fresh();
    song.g[G_BPM] = 123;
    for (k = 0; k < 3u; k++) {
        timing(k, len[k], 2);
        trk[k].step[0].on = 1;
    }
    song.mlen = 64;
    transport_req = 1;
    render_mix(L, R, CTL);
    now_s = CTL;
    for (k = 0; k < NTRK; k++)
        ppos[k] = trk[k].seq_pos;
    bs = beat_samples();
    while (m < 12u) {
        uint32_t a = dvage;
        render_mix(L, R, CTL);
        now_s += CTL;
        b = 0;
        for (k = 0; k < 3u; k++) {
            if (trk[k].seq_pos != ppos[k] + CTL) {
                b |= 1u << k;
                st[k] = now_s - trk[k].seq_pos;
            }
            ppos[k] = trk[k].seq_pos;
        }
        for (k = 3u; k < NTRK; k++)
            ppos[k] = trk[k].seq_pos;
        if (b && st[0] - CTL == (m + 1u) * 16u * bs) {    /* an MLEN boundary in this block: 64 1/16s = 16 beats */
            m++;
            for (k = 0; k < 3u; k++) {
                OK((b >> k) & 1u);
                OK(st[k] == st[0] && trk[k].seq_idx == 0u && trk[k].seq_cnt == 0u);
            }
            OK(dvage - a == 3u);                          /* (each track's step 0 once) */
            for (k = 0; k < 3u; k++)
                hitk[k]++;
        } else {
            OK(dvage - a <= 2u);
        }
        if (now_s > (m + 2u) * 16u * bs)
            OK(0);                                        /* (a boundary missed) */
        if (!ok)
            break;
    }
    transport_req = 2;
    render_mix(L, R, CTL);
    check("MLEN 64: tracks LEN 5 / 12 / 16 at step 0 together every 64 1/16s (12 times), begun on the sample, one hit "
          "each", (int)ok);
    ok = 1;                                               /* OFF: the plain polymeter (LEN 5 at 64: step 4) */
    fresh();
    timing(0, 5, 2);
    timing(1, 16, 2);
    play_seq();
    while (now_s < 16u * beat_samples() + 2u * CTL)
        seq_blk(st);
    OK(trk[0].seq_idx == 4u && trk[0].seq_cnt == 64u);
    stop_seq();
    check("MLEN OFF: no restart (LEN 5 is at step 4 after 64 1/16s)", (int)ok);
}

/* a variation queued with MLEN 64 (the longest track LEN 16): it switches at MLEN's boundary, not the track's end,
 * and the tracks are where MLEN alone puts them (one restart) */
static void mlen_cue(void)
{
    uint32_t ok = 1, k, sw_at = 0, bs, t_end;
    int32_t L[CTL], R[CTL];
    uint32_t pos[NTRK], cnt[NTRK], rem[NTRK];
    static step_t VS[NTRK][NSTEP];
    static int16_t VTM[NTRK][4];
    const step_t *vp[NTRK];
    fresh();
    timing(0, 16, 2);
    timing(1, 5, 2);
    timing(2, 3, FBAR);
    trk[0].step[0].on = 1;
    song.mlen = 64;
    project_save(0);
    memset(VS, 0, sizeof VS);
    for (k = 0; k < NTRK; k++) {
        vp[k] = VS[k];
        VTM[k][0] = trk[k].p[P_SLEN];
        VTM[k][1] = trk[k].p[P_SDIV];
    }
    VS[0][0].on = 1;
    var_put(&var_buf[var_play], 1, vp, VTM);
    project_save(0);
    bs = 22050;
    t_end = 16u * bs + 5u * bs + 77u;
    transport_req = 1;                                    /* MLEN alone */
    for (now_s = 0; now_s < t_end; now_s += CTL)
        render_mix(L, R, CTL);
    for (k = 0; k < NTRK; k++) {
        pos[k] = trk[k].seq_pos;
        cnt[k] = trk[k].seq_cnt;
        rem[k] = trk[k].seq_rem;
    }
    transport_req = 2;
    render_mix(L, R, CTL);
    transport_req = 1;
    for (now_s = 0; now_s < t_end; now_s += CTL) {
        uint32_t was = var_cur;
        if (now_s == 50u * CTL)
            OK(cue_var(1) == 0 && cue.q_var == 2u);
        render_mix(L, R, CTL);
        cue_poll();
        if (!was && var_cur)
            sw_at = now_s + CTL;
    }
    OK(var_cur == 1u && sw_at - CTL < CTL + 16u * bs && sw_at >= CTL + 16u * bs);   /* (at 64 1/16s, not 16) */
    for (k = 0; k < NTRK; k++)
        OK(trk[k].seq_pos == pos[k] && trk[k].seq_cnt == cnt[k] && trk[k].seq_rem == rem[k]);
    transport_req = 2;
    render_mix(L, R, CTL);
    check("MLEN 64 + a queued variation: it switches at MLEN's boundary (not the longest track's end), one restart (the "
          "tracks where MLEN alone puts them)", (int)ok);
}

/* MLEN: a slot's own, saved in its record (rsv_song[0..1]), loaded with it, switched with a slot's cue */
static void mlen_saved(void)
{
    uint32_t ok = 1;
    fresh();
    song.mlen = 48;
    project_save(0);
    OK(proj_slot[0].rsv_song[0] == 48u && proj_slot[0].rsv_song[1] == 0u);
    song.mlen = 1;                                        /* (OFF) */
    project_save(1);
    OK(proj_slot[1].rsv_song[0] == 0u && proj_slot[1].rsv_song[1] == 0u);
    song.mlen = 1000;
    project_save(2);
    OK(proj_slot[2].rsv_song[0] == (1000u & 255u) && proj_slot[2].rsv_song[1] == 1000u >> 8);
    project_load(0);
    OK(song.mlen == 48);
    project_load(1);
    OK(cue_mlen() == 0u);
    project_load(2);
    OK(song.mlen == 1000);
    OK(cue_slot(0) == 0 && song.mlen == 48);              /* (stopped: at once) */
    OK(proj_mlen(&proj_slot[2]) == 1000);
    OK(sizeof(project_t) == 3044u);
    check("MLEN: saved in the record's reserved bytes (OFF: 0), loaded, a slot cue brings its own", (int)ok);
}

/* ---- the chain */
static int32_t L_[CTL], R_[CTL];
static void slots_abc(void)                              /* A B C saved (track 0 LEVEL 10 / 20 / 30, LEN 16 1/16), D
                                                          * empty; A loaded */
{
    uint32_t s;
    fresh();
    timing(0, 16, 2);
    timing(1, 12, 2);
    trk[0].step[0].on = 1;
    for (s = 0; s < 3u; s++) {
        trk[0].p[P_LEVEL] = (int16_t)(10 + 10 * (int32_t)s);
        project_save(s);
    }
    project_load(0);
}
static void chain_plays(void)
{
    static const uint8_t CH[5] = {0, 0, 3, 2, 1};       /* 1 1 4 3 2: 4 empty (skipped) */
    static const uint8_t WANT[10] = {0, 0, 2, 1, 0, 0, 2, 1, 0, 0};
    uint32_t ok = 1, e = 0, bs, ld = 0;
    uint8_t got[10];
    slots_abc();
    transport_req = 1;
    render_mix(L_, R_, CTL);
    bs = beat_samples();
    for (now_s = CTL; now_s < bs; now_s += CTL) {
        render_mix(L_, R_, CTL);
        cue_poll();
    }
    OK(chain_set(CH, 5) == 0 && chain.n == 5u && chain.pos == CH_NONE && chain.nx == 0u && !cue.q_slot);
    while (e < 10u) {
        uint32_t pc = trk[0].seq_cnt;
        render_mix(L_, R_, CTL);
        now_s += CTL;
        cue_poll();
        if (trk[0].seq_cnt < pc || (pc == 15u && trk[0].seq_cnt == 0u)) {   /* a pattern end: a restart */
            OK(now_s - trk[0].seq_pos - CTL == (e + 1u) * 4u * bs);   /* (on the longest track's end, exactly) */
            got[e] = (uint8_t)proj_loaded();
            OK(got[e] == WANT[e]);
            OK(trk[0].p[P_LEVEL] == 10 + 10 * (int)WANT[e]);
            OK(chain.pos == (uint8_t)(e % 4u == 0u ? 0u : e % 4u == 1u ? 1u : e % 4u == 2u ? 3u : 4u));
            e++;
        }
        if (now_s > (e + 2u) * 4u * bs)
            OK(0);
        if (!ok)
            break;
    }
    (void)ld;
    check("chain 1 1 4 3 2 (4 empty): at each pattern end (exactly) slot 1, 1, 3, 2, then from the first again; the empty "
          "one skipped", (int)ok);
    ok = 1;                                               /* STOP: the chain ends */
    transport_req = 2;
    render_mix(L_, R_, CTL);
    cue_poll();
    OK(chain.n == 0u && !cue.chain && !cue.q_slot && !cue.q_var);
    check("chain: STOP ends it (nothing queued, the ISR's flag off)", (int)ok);
}
static void chain_stopped(void)
{
    static const uint8_t CH[3] = {2, 1, 2};
    uint32_t ok = 1, bs, sw = 0;
    slots_abc();
    OK(chain_set(CH, 3) == 0 && proj_loaded() == 2 && chain.pos == 0u && chain.nx == 1u && !cue.q_slot);   /* (at once) */
    transport_req = 1;
    render_mix(L_, R_, CTL);
    cue_poll();
    OK(cue.q_slot == 2u);                                 /* (PLAY: the next one queued) */
    bs = beat_samples();
    for (now_s = CTL; now_s < 4u * bs + 2u * CTL; now_s += CTL) {
        render_mix(L_, R_, CTL);
        cue_poll();
        if (!sw && proj_loaded() == 1)
            sw = now_s + CTL;
    }
    OK(sw && chain.pos == 1u && chain.nx == 2u && cue.q_slot == 3u);
    OK(cue_slot(0) == 0 && chain.n == 0u && !cue.chain);  /* (a slot's own cue: the chain ends) */
    transport_req = 2;
    render_mix(L_, R_, CTL);
    cue_poll();
    OK(chain_set(CH, 1) == 0 && chain.n == 0u);            /* (one entry: no chain) */
    check("chain set while stopped: the first entry at once, PLAY queues the next; a cue key ends it; one entry: none",
          (int)ok);
}
/* a FIT track the longest (LEN 5 over F2BR: 8 beats): a queued variation switches at its pattern's end exactly, and
 * after it every track (another FIT one too) is where the never-switched loop has it */
static void fit_longest_cue(void)
{
    uint32_t ok = 1, k, sw_at = 0, bs, t_end;
    uint32_t pos[NTRK], cnt[NTRK], rem[NTRK], par[NTRK];
    static step_t VS[NTRK][NSTEP];
    static int16_t VTM[NTRK][4];
    const step_t *vp[NTRK];
    fresh();
    song.g[G_BPM] = 123;
    timing(0, 5, F2BR);
    timing(1, 16, 2);
    timing(2, 3, FBAR);
    trk[0].step[0].on = 1;
    trk[1].p[P_SSWING] = 40;
    project_save(0);
    memset(VS, 0, sizeof VS);
    for (k = 0; k < NTRK; k++) {
        vp[k] = VS[k];
        VTM[k][0] = trk[k].p[P_SLEN];
        VTM[k][1] = trk[k].p[P_SDIV];
        VTM[k][2] = trk[k].p[P_SSWING];
    }
    VS[0][1].on = 1;
    var_put(&var_buf[var_play], 1, vp, VTM);
    project_save(0);
    bs = beat_samples();
    OK(cue_longest() == 0u);
    t_end = 8u * bs + 7u * bs + 333u;
    transport_req = 1;
    for (now_s = 0; now_s < t_end; now_s += CTL)
        render_mix(L_, R_, CTL);
    for (k = 0; k < NTRK; k++) {
        pos[k] = trk[k].seq_pos;
        cnt[k] = trk[k].seq_cnt % (uint32_t)trk[k].p[P_SLEN];
        rem[k] = trk[k].seq_rem;
        par[k] = (trk[k].seq_cnt ^ trk[k].seq_par) & 1u;
    }
    transport_req = 2;
    render_mix(L_, R_, CTL);
    transport_req = 1;
    for (now_s = 0; now_s < t_end; now_s += CTL) {
        uint32_t was = var_cur;
        if (now_s == 3u * bs / CTL * CTL)
            OK(cue_var(1) == 0 && cue.q_var == 2u);
        render_mix(L_, R_, CTL);
        cue_poll();
        if (!was && var_cur) {
            sw_at = now_s + CTL;
            OK(sw_at - trk[0].seq_pos == CTL + 8u * bs);  /* (step 0 again at 8 beats, exactly) */
            OK(cue.b24 == 192u);
        }
    }
    OK(var_cur == 1u && sw_at);
    for (k = 0; k < NTRK; k++)
        OK(trk[k].seq_pos == pos[k] && trk[k].seq_cnt % (uint32_t)trk[k].p[P_SLEN] == cnt[k] && trk[k].seq_rem == rem[k] &&
           ((trk[k].seq_cnt ^ trk[k].seq_par) & 1u) == par[k]);
    transport_req = 2;
    render_mix(L_, R_, CTL);
    check("cue: a FIT track the longest (LEN 5 F2BR) drives the boundary (8 beats exactly); after the switch every "
          "track (FIT, swung 1/16) sits where the never-switched loop has it", (int)ok);
}

/* HOME shows the chain: its band draws while one runs (and its line changes with the entry playing) */
static void chain_home(void)
{
    static const uint8_t CH[4] = {0, 0, 2, 1};
    uint32_t ok = 1, s0;
    slots_abc();
    ui.home = 1;
    ui.force = 1;
    ui_draw();
    s0 = hm.chain;
    OK(chain_set(CH, 4) == 0);
    ui_draw();
    OK(hm.chain != s0);
    s0 = hm.chain;
    ui_draw();
    OK(hm.chain == s0);
    chain.pos = 1;
    ui_draw();
    OK(hm.chain != s0);
    chain_clear();
    ui.home = 0;
    check("HOME: the chain line under the status line (its signature: the entries, the one playing, the next)",
          (int)ok);
}

int main(void)
{
    fit_ids();
    fit_timing();
    fit_hits();
    fit_len_change();
    fit_longest_cue();
    mlen_restarts();
    mlen_cue();
    mlen_saved();
    chain_plays();
    chain_stopped();
    chain_home();
    printf(fails ? "poly_test: %d FAILED\n" : "poly_test: all passed\n", fails);
    return fails ? 1 : 0;
}
