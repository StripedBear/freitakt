/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* Live cue (spec docs/superpowers/specs/2026-10-26-cue-variations-fill-copy-design.md): the next slot (A..D) or a
 * variation (1..8) of the playing slot queued while it plays, switched exactly where the pattern ends (the longest
 * track's), every track then from its step 0 on the grid a plain loop would be on (seq_restart); the same key again: at the next step. The behaviour follows mono2k by Iñigo Hernández (an FM-1
 * firmware, GPL-3.0-only: its queued pattern switch); no code of it is copied.
 * This file is the ISR's part (seq.c events_block): the boundary, the switch, a slot switch's 5 ms fade (fx.c
 * mix_part). The main loop stages a queued switch beforehand and files the outgoing state after it; the slots' data
 * are project.c's: proj_cue_apply, the ISR's swap of the live state with the staged one (host tests without project.c:
 * tests/drum_host.h's stub). */
#define CUE_SW_SLOT 1u                     /* cue.done: a slot switch / a variation switch happened */
#define CUE_SW_VAR 2u
enum { CUE_E_FULL = 1, CUE_E_NOSLOT = 3, CUE_E_EMPTY, CUE_E_SAME, CUE_E_PARK = 8 };   /* + c: SAVE C FIRST (2: reserved,
                                                   * was the song's STOP SONG TO CUE) */
#define CUE_FADE (FS * 5u / 1000u)         /* a slot switch's fade out: 5 ms */
#define CUE_FSTEP (32767u / CUE_FADE + 1u) /* .. its gain step a sample (Q15) */
#define CUE_LEAD (CUE_FADE + 2u * CTL)     /* own clock: it starts this far ahead of the boundary (done before the
                                            * boundary's block, whatever the phase of the blocks) */
/* a pattern's timing: LEN DIV SWG (a variation's, the editor's 4th byte: Grids' SRC, gone: 0) */
#define PAT_NT 3u
static const uint8_t PAT_TIMING[PAT_NT] = {P_SLEN, P_SDIV, P_SSWING};
/* the grid a plain loop is on */
#define B24_WRAP 768u                              /* a multiple of 24 and of every division's two steps in 1/24 beats
                                                    * (FIT: 32 beats are an even number of its steps too) */
/* n steps of track t in 1/24 beats: every fixed division's step is a whole number of them (1/32: 3 .. 4BAR: 384);
 * a FIT step mostly not (FBAR, LEN 5: 96/5), so its whole patterns exact, a step into one floored */
static uint32_t steps_b24(const track_t *t, uint32_t n)
{
    uint32_t den, num = trk_num_den(t, &den) * 24u;
    return num % den ? n / den * num + n % den * num / den : n * (num / den);
}
static uint32_t pattern_beats(const track_t *t, uint32_t *den)   /* t's pattern: num / den beats (FIT: LEN x num /
                                                                   * LEN, its span exactly) */
{
    uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP);
    return len * trk_num_den(t, den);
}
/* MASTER LENGTH (FREITAKT): song.mlen 1/16s after the last restart (PLAY, a cue's switch, its own) every track
 * restarts as a cue's switch restarts them; a cue (and a chain) then switches there, not at the longest track's end.
 * Its 1/16s are counted exactly as a 1/16 track without swing counts its steps (seq_tick), so its boundary is that
 * track's step to the sample */
static struct { uint32_t pos, cnt; uint8_t rem; } ml;
static uint32_t cue_mlen(void) { return song.mlen >= 2 ? (uint32_t)(song.mlen < 1024 ? song.mlen : 1024) : 0u; }
static void ml_restart(uint32_t x, uint32_t b24)   /* (as seq_restart a track's: step 0 x samples into the block) */
{
    ml.pos = 0x7FFFFFFFu - x;
    ml.cnt = 0;
    ml.rem = (uint8_t)(b24 * beat_samples() % 24u * 4u / 24u);
}
/* every track at its step 0, x samples into this block (seq_tick: cue.cut keeps the block's rest), on the grid a
 * plain loop would be on: each track's step remainder and swing pair phase from b24 (1/24 beats since PLAY: every
 * fixed division's step is a whole number of them; FIT's pair phase: the steps whole in b24, num / den exactly).
 * Returns the reference: the longest track (LEN x DIV; ties: the lowest) */
static uint32_t seq_restart(uint32_t x, uint32_t b24)
{
    uint32_t k, best = 0, bn = 0, bd = 1;
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        uint32_t n, d;
        t->seq_idx = (uint16_t)(t->p[P_SLEN] - 1);
        t->seq_pos = 0x7FFFFFFFu - x;              /* step 0, x samples into the block (seq_tick, cue.cut) */
        t->seq_cnt = 0xFFFFFFFFu;
        {                                          /* the grid since PLAY: its remainder, its swing pair phase */
            uint32_t den, num = trk_num_den(t, &den);
            t->seq_par = (uint8_t)((b24 * den / (24u * num)) & 1u);
            t->seq_rem = (uint8_t)(b24 * beat_samples() % 24u * den / 24u);
        }
        t->rskip = 0;
        t->rat_n = 0;
        t->ar_on = 0;                              /* (ARP / RTRG: the roll ends, seq.c) */
        t->mic = 0;                                /* (MICRO: no early hit pending, seq.c) */
        n = pattern_beats(t, &d);
        if (n * bd > bn * d) {                     /* the longest; ties: the lowest track */
            best = k;
            bn = n;
            bd = d;
        }
    }
    return best;
}

static uint8_t var_cur;                    /* the variation playing: 0 = variation 1 (the slot's record's steps) */
static struct {
    volatile uint8_t q_slot, q_var;        /* queued: slot + 1 / variation + 1, 0 none (main writes, the ISR takes) */
    volatile uint8_t now;                  /* the same key again: switch at the next step */
    uint8_t last[4];                       /* each slot's last-played variation (RAM only; power-on 0) */
    uint16_t b24;                          /* 1/24 beats since PLAY at the last restart, mod B24_WRAP */
    volatile uint8_t done, err;            /* ISR -> main: CUE_SW_SLOT | CUE_SW_VAR; a refusal CUE_E_* (said once) */
    uint8_t stg, stg_slot, stg_var;        /* what the main loop staged (project.c STG_*), for slot / variation */
    uint8_t out, out_var, from;            /* the switch's outgoing state, in the staging area for cue_poll to file:
                                            * its kind, its variation, (a slot switch) the slot it left */
    uint16_t stg_pre[NTRK], out_pre[NTRK]; /* the tracks' presets: the staged slot's, the outgoing one's */
    uint8_t stg_pre_ed[NTRK], out_pre_ed[NTRK];
    uint8_t fade;                          /* 0 none, 1 fading out to the boundary, 2 ramping back up (a cancel) */
    uint16_t fg;                           /* the fade's gain at the next block's start (Q15) */
    uint8_t cut;                           /* a switch restarted the tracks this block (seq_tick) */
    volatile uint8_t chain;                /* main -> ISR: a chain runs (project.c chain_*): every pattern end restarts
                                            * the tracks, a switch queued or not */
    volatile uint8_t ends;                 /* ISR -> main: pattern ends taken while it ran (counts, wraps) */
    int16_t stg_mlen, out_mlen;            /* MASTER LENGTH: the staged slot's, the outgoing one's (project.c) */
} cue;
/* the chain (Elektron's; project.c chain_*: the main loop's): up to CHAIN_N slots played in turn, a pattern each, then
 * from the first again; RAM only. pos: the entry playing (CH_NONE: none yet), nx: the one after it (an empty slot's
 * entries skipped), qd: nx queued (or tried), run: seen playing (STOP ends the chain), ends: cue.ends taken */
#define CHAIN_N 16u
#define CH_NONE 0xFFu
static struct { uint8_t s[CHAIN_N], n, pos, nx, qd, run, ends; } chain;

static void ml_tick(uint32_t n)                    /* after cue_tick, with the steps' advance n (seq_tick's loop) */
{
    ml.pos += n;
    for (;;) {
        uint32_t cur = div_period(2u, ml.rem);
        if (ml.pos < cur && ml.pos != 0x7FFFFFFFu + n)
            break;
        if (ml.pos >= 0x7FFFFFFFu) {
            ml.pos = CLK_FOLLOWS() || cue.cut ? ml.pos - 0x7FFFFFFFu : 0;
        } else {
            ml.pos -= cur;
            ml.rem = (uint8_t)div_rem_next(2u, ml.rem);
            ml.cnt++;
        }
    }
}
/* the samples from the block's start to MASTER LENGTH's boundary (0xFFFFFFFF: not in its last 1/16); *b24 the grid's
 * time there */
static uint32_t ml_dist(uint32_t mlen, uint32_t *b24)
{
    uint32_t cur;
    if (ml.pos >= 0x7FFFFFFFu || ml.cnt + 1u < mlen)
        return 0xFFFFFFFFu;
    cur = div_period(2u, ml.rem);
    *b24 = (cue.b24 + (ml.cnt + 1u) * 6u) % B24_WRAP;
    return cur > ml.pos ? cur - ml.pos : 0u;
}
static int proj_cue_apply(void);           /* project.c: the staged state in, the outgoing one out (0, or nothing
                                            * staged: CUE_E_SAME) */

static void cue_fade_set(uint32_t g, int32_t d, uint32_t at)   /* this block's mix gain (fx.c mix_part) */
{
    mix_fade = (int32_t)g;
    mix_fade_d = d;
    mix_fade_at = at;
}
static void cue_fade_up(void)              /* a fade under way, its switch gone (cancelled, refused, STOP): back up */
{
    if (cue.fade == 1u)
        cue.fade = 2;
}

/* every track restarted x samples into this block (the ISR), the grid's time there b24: a switch's, MASTER LENGTH's,
 * a chain's pattern end */
static void cue_restart(uint32_t x, uint32_t b24)
{
    cue.b24 = (uint16_t)b24;
    seq_restart(x, b24);
    ml_restart(x, b24);
    cue.cut = 1;                                   /* (seq_tick: the block's rest after x) */
    acid_gates_close();
}
/* the grid's time at the end of track k's playing step: + the steps it played since the last restart */
static uint32_t cue_b24_at(uint32_t k) { return (cue.b24 + steps_b24(&trk[k], trk[k].seq_cnt + 1u)) % B24_WRAP; }
/* the switch, x samples into this block (the ISR), or at once while stopped (the main loop, the IRQ off): checks
 * first, changes after. b24: the grid's time there (stopped: cue.b24) */
static __attribute__((noinline)) int cue_switch(uint32_t x, uint32_t b24)
{
    uint32_t k;
    int r = proj_cue_apply();
    if (r) {
        cue.q_slot = cue.q_var = cue.now = 0;
        if (r != CUE_E_SAME)
            cue.err = (uint8_t)r;
        cue_fade_up();
        return r;
    }
    if (song.playing)
        cue_restart(x, b24);
    else
        cue.b24 = (uint16_t)b24;
    if (cue.fade == 1u) {                          /* this block: the old voices were cut before it renders */
        if (cue.done & CUE_SW_SLOT && mix_fade == 0) {
            for (k = 0; k < NTRK; k++)
                trk[NTRK - 1u - k].dtail = 0;      /* (faded out: no declick tail of them at full gain) */
            cue_fade_set(32767u, 0, CTL);          /* the new steps from the block's start (as they fire) */
        } else {
            cue_fade_set((uint32_t)mix_fade, -(int32_t)CUE_FSTEP, x);   /* (not faded out: their declick ramps on) */
        }
        cue.fade = 0;
        cue.fg = 32767;
    }
    cue.q_slot = cue.q_var = cue.now = 0;
    return 0;
}

/* every block, first (events_block): the mix gain of this block (fx.c mix_part): a slot switch's fade on its way
 * down, or back up after a cancel; full otherwise. n: the block */
static void cue_fade_tick(uint32_t n)
{
    if (mix_fade != 32767 || mix_fade_d)
        cue_fade_set(32767u, 0, CTL);
    if (cue.fade == 1u && !cue.q_slot)
        cue.fade = 2;                              /* (its switch gone: cancelled, refused, STOP) */
    if (cue.fade == 1u) {                          /* fading out: this block from fg down */
        cue_fade_set(cue.fg, -(int32_t)CUE_FSTEP, CTL);
        cue.fg = (uint16_t)(cue.fg > n * CUE_FSTEP ? cue.fg - n * CUE_FSTEP : 0u);
    } else if (cue.fade == 2u) {                   /* back up */
        cue_fade_set(cue.fg, (int32_t)CUE_FSTEP, CTL);
        cue.fg = (uint16_t)(cue.fg + n * CUE_FSTEP < 32767u ? cue.fg + n * CUE_FSTEP : 32767u);
        if (cue.fg == 32767u)
            cue.fade = 0;
    }
}
/* the track whose pattern is the longest (LEN x DIV; ties: the lowest): a queued cue switches at its end */
static uint32_t cue_longest(void)
{
    uint32_t k, best = 0, bn = 0, bd = 1;
    for (k = 0; k < NTRK; k++) {
        uint32_t m, d;
        m = pattern_beats(&trk[k], &d);
        if (m * bd > bn * d) {
            best = k;
            bn = m;
            bd = d;
        }
    }
    return best;
}
/* before the block's seq_ticks (events_block), playing: the pattern end in this block -> the queued switch there (a
 * chain's or MASTER LENGTH's pattern end with none: the restart alone). The pattern end: MASTER LENGTH's boundary,
 * else the end of the longest track's pattern (FIT: its LEN steps, exactly its span); the same key again: the soonest
 * next step. adv: the sequencer's advance this block (following a clock: the clock's), lead: how far ahead a slot
 * switch's fade starts (own clock CUE_LEAD; following, a pulse at least: the boundary's last pulse), n: the block */
static __attribute__((noinline)) void cue_tick(uint32_t n, uint32_t adv, uint32_t lead)   /* (noinline: the
                                                   * audio ISR's loops stay as they were: target_budget.py) */
{
    uint32_t k, best = NTRK, dist = 0xFFFFFFFFu, b24 = 0, q = cue.q_slot || cue.q_var, mlen = cue_mlen();
    if (!q && !mlen && !cue.chain)
        return;
    if (!song.playing)
        return;
    if (q && cue.now) {                            /* the soonest next step of any track */
        for (k = 0; k < NTRK; k++) {
            track_t *t = &trk[k];
            uint32_t cur;
            if (t->seq_pos >= 0x7FFFFFFFu)
                return;                            /* (PLAY's first block: step 0 is now anyway) */
            cur = step_samples(t, trk_period(t, t->seq_rem), t->seq_cnt);
            if (cur - t->seq_pos < dist) {
                dist = cur - t->seq_pos;
                best = k;
            }
        }
        if (best < NTRK && dist <= adv)
            cue_switch(dist, cue_b24_at(best));
        return;
    }
    if (mlen) {                                    /* MASTER LENGTH's boundary */
        dist = ml_dist(mlen, &b24);
        if (dist == 0xFFFFFFFFu)
            return;
    } else {                                       /* the end of the longest track's pattern */
        uint32_t len, cur;
        track_t *t;
        best = cue_longest();
        t = &trk[best];
        len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP);
        if (t->seq_pos >= 0x7FFFFFFFu || t->seq_idx + 1u != len)
            return;
        cur = step_samples(t, trk_period(t, t->seq_rem), t->seq_cnt);
        dist = cur > t->seq_pos ? cur - t->seq_pos : 0u;
    }
    if (cue.q_slot && cue.fade != 1u && dist <= adv + lead) {
        cue.fade = 1;                              /* a slot switch: its fade starts (this block on) */
        cue.fg = (uint16_t)mix_fade;
        cue_fade_set(cue.fg, -(int32_t)CUE_FSTEP, CTL);
        cue.fg = (uint16_t)(cue.fg > n * CUE_FSTEP ? cue.fg - n * CUE_FSTEP : 0u);
    }
    if (dist > adv)
        return;
    if (!mlen)
        b24 = cue_b24_at(best);
    if (q) {
        if (cue_switch(dist, b24))
            return;                                /* (refused: nothing restarted) */
    } else {
        cue_restart(dist, b24);
    }
    if (cue.chain)
        cue.ends++;
}

static void cue_start(void)                        /* seq_start: the grid's time from PLAY */
{
    cue.b24 = 0;
    ml_restart(0, 0);
}
static void cue_stop(void)                         /* seq_stop: a queued cue dropped (a fade goes back up) */
{
    cue.q_slot = cue.q_var = cue.now = 0;
}
