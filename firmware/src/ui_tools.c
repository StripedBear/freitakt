/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* FREITAKT (redesign phase C): the SEQ button, the clipboard and the track clear. The TOOLS screen (SEQ held, the step
 * copy tools of spec 2026-10-31 §3) is gone: copy / paste are button combos (ui_input.c combo_input: SAVE + REC copies
 * the selected track, SAVE + PLAY pastes it; GLO + REC copies the pattern, GLO + PLAY pastes it), the track clear is
 * REC + SEL held 5 s (ui_input.c clr_input), no screen but its countdown.
 *
 * SEQ: a tap, or a hold, turns the SEQ pages on its release. SEQ + black 1..6 mute / unmute tracks 1..6 (as MUTE NOW /
 * NEXT BAR says): then the release turns no page. Every key pressed with SEQ is its own until let go (seq.c
 * keyboard_block, mute_mask: no note, no MIDI, nothing recorded; a white key does nothing). Pressed in the menu, a
 * confirm, NAME, the PRESETS list, the cue map or the clear's countdown: dead until let go; with the PERFORM layer up or
 * FX held: a plain button (the release turns the pages). SEQ held at power-on is the safe start, as before (ui.c
 * safe_start_check) */
#define SQ_DOWN 1u
#define SQ_DEAD 4u
#define SQ_PLAIN 8u
#define SQ_KEY 16u                  /* a key struck with it: the mutes (no page on the release) */

static int cue_owns_keys(void);

static int seq_dead(void) { return ui.menu || ui.confirm || name_on() || pl_on() || ui.cue_map || ui.clr; }
static int seq_owns_keys(void)                         /* SEQ held (GLO not): the keys pressed now are its own */
{
    return (((fm1_in.buttons >> panel.btn[B_SEQ]) & 1u) || ui.seq_t0) &&
           !((fm1_in.buttons >> panel.btn[B_GLO]) & 1u) && !(ui.seq_t0 & (SQ_DEAD | SQ_PLAIN)) && !seq_dead() &&
           !ui.layer && !ui.fx_t0;
}
static void seq_mutes(uint32_t hits)                 /* SEQ + black 1..6: tracks 1..6 muted / unmuted */
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        if ((hits >> CUE_BK[k]) & 1u)
            track_mute_toggle(k);
}
static void seq_input(uint32_t now, uint32_t notes)
{
    uint32_t bit = 1u << panel.btn[B_SEQ], *t0 = &ui.seq_t0, hits = 0;
    if (kb_mute_hit && seq_owns_keys() && !cue_owns_keys()) {
        fm1_irq_off();
        hits = kb_mute_hit;
        kb_mute_hit = 0;
        fm1_irq_on();
    }
    if (fm1_in.buttons & bit) {
        if (!*t0)
            *t0 = (now & ~31u) | SQ_DOWN;
        if (!(*t0 & SQ_KEY) && seq_dead())
            *t0 |= SQ_DEAD;
        else if (!(*t0 & SQ_KEY) && (ui.layer || ui.fx_t0))
            *t0 |= SQ_PLAIN;
        if (!(*t0 & (SQ_DEAD | SQ_PLAIN)) && (notes | hits))
            *t0 |= SQ_KEY;                          /* (a key: no page on the release) */
        if (*t0 & SQ_KEY)
            seq_mutes(hits);
    } else if (*t0) {
        if (hits && !(*t0 & (SQ_DEAD | SQ_PLAIN))) {
            *t0 |= SQ_KEY;                          /* (keys struck as SEQ went: theirs; no tap) */
            seq_mutes(hits);
        }
        if (!(*t0 & (SQ_KEY | SQ_DEAD)) && !seq_dead())
            open_family(FAM_SEQ);                   /* a tap or a hold: the SEQ pages, as the button always did */
        *t0 = 0;
    }
    mute_mask = (mute_mask & ~bit) |
                ((*t0 & SQ_KEY) || (!(*t0 & (SQ_DEAD | SQ_PLAIN)) && !seq_dead() && !ui.layer && !ui.fx_t0) ? bit : 0u);
}

/* ---- the clipboard (Model:Cycles style, no screen): a track (the selected one's steps, their locks and its LEN DIV
 * SWG; motion events stay where they are, as the old step tools' copies) or the pattern (every track's: the live
 * variation, as it plays). A copied step is its whole step_t (on, accent, PROB, ratchet, note, slide / tie, the micro);
 * a paste replaces the target's steps, locks and LEN DIV SWG; LOCKS FULL: as many as fit, and the message. One
 * clipboard (a copy replaces what it held), never stored (a clipboard across projects and patterns); no undo, as the
 * old tools; the edits are RAM edits as any (SAVE keeps them, LOAD asks DISCARD EDITS?) */
#define CB_NT 3u
static const uint8_t CB_TM[CB_NT] = {P_SLEN, P_SDIV, P_SSWING};   /* (cue.c PAT_TIMING: a variation's timing) */
enum { CB_NONE, CB_TRACK, CB_PAT };
typedef struct {
    step_t st[NTRK][NSTEP];         /* a track's: row 0 */
    int16_t tm[NTRK][CB_NT];
    uint16_t lk[LK_MAX];            /* the locks: track (a track's: 0) << 13 | step << 7 | knob, ascending */
    int8_t lv[LK_MAX];
} clip_t;
static clip_t clip __attribute__((section(".pool")));   /* ~4.7 KB: the pool's (not zeroed: clip_kind says) */
static uint8_t clip_kind;                               /* CB_*: what it holds */
static uint16_t clip_lkn;                               /* .. its locks */

static void clip_take(uint32_t row, uint32_t k)        /* track k -> the clipboard's row, its locks after the ones in */
{
    uint32_t j, i;
    memcpy(clip.st[row], trk[k].step, sizeof clip.st[row]);
    for (j = 0; j < CB_NT; j++)
        clip.tm[row][j] = trk[k].p[CB_TM[j]];
    for (i = lk_seek(&mo.b, k, 0); i < mo.b.count && (uint32_t)(mo.b.key[i] >> 13) == k; i++)
        if (lk_is_lock(&mo.b, i) && clip_lkn < LK_MAX) {
            clip.lk[clip_lkn] = (uint16_t)(row << 13 | (mo.b.key[i] & 0x1FFFu));
            clip.lv[clip_lkn++] = mo.b.val[i];
        }
}
/* the clipboard's row -> track k (its steps, LEN DIV SWG; its locks replaced); 1: the lock bank refused some */
static int clip_give(uint32_t row, uint32_t k)
{
    uint32_t j, i;
    int full = 0;
    lock_clear_track(k);
    fm1_irq_off();
    memcpy(trk[k].step, clip.st[row], sizeof trk[k].step);
    fm1_irq_on();
    for (j = 0; j < CB_NT; j++)
        trk[k].p[CB_TM[j]] = clip.tm[row][j];
    for (i = 0; i < clip_lkn; i++)
        if ((uint32_t)(clip.lk[i] >> 13) == row) {
            fm1_irq_off();
            if (lk_put(&mo.b, (uint32_t)k << 13 | (clip.lk[i] & 0x1FFFu), clip.lv[i], 1) < 0)
                full = 1;
            fm1_irq_on();
        }
    return full;
}
static void clip_copy(int pat)                          /* SAVE + REC: the selected track; GLO + REC: the pattern */
{
    uint32_t k;
    clip_lkn = 0;
    if (pat) {
        for (k = 0; k < NTRK; k++)
            clip_take(k, k);
        clip_kind = CB_PAT;
        ui_message("COPIED PATTERN");
    } else {
        char m[4] = "T1";
        clip_take(0, song.sel);
        clip_kind = CB_TRACK;
        m[1] = (char)('1' + song.sel);
        ui_say("COPIED ", m);
    }
}
static void clip_paste(int pat)                         /* SAVE + PLAY: onto the selected track; GLO + PLAY: the pattern */
{
    uint32_t k;
    int full = 0;
    if (clip_kind != (pat ? CB_PAT : CB_TRACK)) {
        ui_message(pat ? "NO PATTERN COPIED" : "NO TRACK COPIED");
        return;
    }
    if (ui_locked())                                    /* (a web editor restore: RESTORING) */
        return;
    if (pat)
        for (k = 0; k < NTRK; k++)
            full |= clip_give(k, k);
    else
        full = clip_give(0, song.sel);
    bank_fix();                                         /* (LEN may have changed) */
    ui.force = 1;
    if (full) {
        ui_message("LOCKS FULL");
    } else if (pat) {
        ui_message("PASTED PATTERN");
    } else {
        char m[4] = "T1";
        m[1] = (char)('1' + song.sel);
        ui_say("PASTED ", m);
    }
}
