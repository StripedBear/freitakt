/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Drum machine fork: 2026 DEADACTIVE */
/* Drum UI input: LEDs, knobs and buttons, the STEP grid on the keys, panel setup. */
/* ----------------------------------------------------------- LEDs --- */
/* The LED picture is built off-line and copied one byte per column: clearing
 * and relighting would let the 10 kHz scan catch the dark gap and flicker.
 * BACKLIGHT ON (spec 2026-10-11-fixes §12): one word per column instead, the full layer and the 25 % / 5 % ones,
 * which the scan's hook composes (led_dim.c). */
#include "led_dim.c"
static uint8_t led_pos[41];                        /* (col << 3) | row bit, 0xFF = none */

static void led_pos_init(void)
{
    uint32_t id, p, r;
    for (id = 0; id < 41u; id++) {
        led_pos[id] = 0xFF;
        for (p = 0; p < FM1_NCOL; p++)
            for (r = 1; r < 5u; r++)
                if (FM1_KEYMAP[r][p] == (int8_t)id)
                    led_pos[id] = (uint8_t)((p << 3) | r);
    }
}

static void led_put(uint8_t *nl, uint32_t id, int on)
{
    uint8_t q = led_pos[id];
    if (q != 0xFF && on)
        nl[q >> 3] |= (uint8_t)(1u << (q & 7u));
}

static uint32_t cur_fam(void) { return ui.home ? FAM_HOME : cur_page()->fam; }

/* fixes 2026-10-21 item 1: OCT+ where the keys are steps: 1 pressed (let go alone: the next bank), 2 a step key
 * accented with it (let go: nothing); 0 not held there (ui_input acc_oct_poll) */
static uint8_t acc_oct;
/* the black keys (key index from F3): black 1..8 F#3 G#3 A#3 C#4 D#4 F#4 G#4 A#4 queue variations 1..8 with SAVE held
 * (the cue map, every page it opens on: of the queued pattern if one is, cue_keys) or GLO held (mod_keys); black 1..6
 * are tracks 1..6 for the combos (SEL select, SEQ mute, ARP solo: mod_input, ui_tools.c) */
static const uint8_t CUE_BK[8] = {1, 3, 5, 8, 10, 13, 15, 17};
#define KEY_SOLO_MODE 25u                           /* F#5 (black 11): ARP + it, SOLO TOGGLE / MOMENTARY */
#define BLACK_MASK 0x252A52Au                       /* the 11 black keys' bits */
/* the combo buttons' state (GLO, SEL, ARP: mod_input; ui.glo_t0 ..): the press time | these */
#define MOD_DOWN 1u
#define MOD_KEY 2u                                  /* a key pressed with it: no tap */
#define MOD_DEAD 4u                                 /* pressed in the menu, a confirm, NAME, the PRESETS list, TOOLS */
#define MOD_PLAIN 8u                                /* pressed with the PERFORM layer up or FX held: a plain button */
#define GLO_SLOTS 4u                                /* the pattern slots A..D (project.c proj_slot): GLO + white 1..4 */
static int mod_live(uint32_t t0) { return t0 && !(t0 & (MOD_DEAD | MOD_PLAIN)); }   /* held, its keys its own */
/* things the boot found that the first frames say, once each (project.c persist_boot): USR2 / USR3 shortened to 48 KiB,
 * a lock bank lost (spec 2026-10-24 parameter locks), the variations' (spec 2026-10-26) */
#define PN_USR3 1u
#define PN_LOCKS 2u
#define PN_USR3F 4u                                 /* the USR3 clamp failed: no lock-bank writes this boot */
#define PN_USR2 8u                                  /* USR2 shortened to 48 KiB (spec 2026-10-26 cue / variations) */
#define PN_USR2F 16u                                /* the USR2 clamp failed: no variation writes this boot */
static uint8_t persist_note;
static uint8_t lk_lost;                             /* project.c: slots whose lock bank was lost (LOCKS LOST, once) */
#define PN_VARS 32u                                 /* a slot's variations lost (spec 2026-10-26 cue / variations) */
static uint8_t var_lost;                            /* project.c: slots whose variations were lost (VARIATIONS LOST) */
#define PN_UPR 64u                                  /* an older user bank moved into 96 slots dropped some (2026-11-01) */
static uint8_t upr_lost;                            /* project.c upr_migrate: how many (N PRESETS LOST, once) */
static void persist_note_show(void)                 /* every frame: with no message up, the lowest note's, once */
{
    if (!persist_note || ui.msg_t)
        return;
    if (persist_note & PN_USR2) {                   /* (USR2 before USR3: the order the boot shortened them) */
        persist_note &= (uint8_t)~PN_USR2;
        ui_message("USR2 SHORTENED TO 48K");
    } else if (persist_note & PN_USR2F) {
        persist_note &= (uint8_t)~PN_USR2F;
        ui_message("USR2 CLAMP FAILED");
    } else if (persist_note & PN_USR3) {
        persist_note &= (uint8_t)~PN_USR3;
        ui_message("USR3 SHORTENED TO 48K");
    } else if (persist_note & PN_LOCKS) {           /* (said once: not again at those slots' LOAD) */
        persist_note &= (uint8_t)~PN_LOCKS;
        if (lk_lost)
            ui_message("LOCKS LOST");
        lk_lost = 0;
    } else if (persist_note & PN_VARS) {            /* (said once: not again at those slots' LOAD) */
        persist_note &= (uint8_t)~PN_VARS;
        if (var_lost)
            ui_message("VARIATIONS LOST");
        var_lost = 0;
    } else if (persist_note & PN_UPR) {             /* "3 PRESETS LOST", "1 PRESET LOST" */
        char nb[4];
        uint32_t n = 0;
        persist_note &= (uint8_t)~PN_UPR;
        if (upr_lost >= 100u)
            nb[n++] = (char)('0' + upr_lost / 100u);
        if (upr_lost >= 10u)
            nb[n++] = (char)('0' + upr_lost / 10u % 10u);
        nb[n++] = (char)('0' + upr_lost % 10u);
        nb[n] = 0;
        ui_say(nb, upr_lost == 1u ? " PRESET LOST" : " PRESETS LOST");
        upr_lost = 0;
    } else {
        persist_note &= (uint8_t)~PN_USR3F;
        ui_message("USR3 CLAMP FAILED");
    }
}
/* fixes 2026-10-21 item 2: the keys show the selected track (its white key lit, as the keys select it there) on the
 * HOME screen and every page of the EDIT (SOUND), FX and LFO families; not under KEYS (the keyboard's), the
 * menu, NAME, the PRESETS list or PERFORM (ui_leds tests those first) */
static int track_keys(void)
{
    uint32_t f = cur_fam();
    return !ui.menu && !keys_mode() && (ui.home || f == FAM_SND || f == FAM_FX || f == FAM_LFO);
}

#include "ui_name.c"                               /* NAME: naming a project */
#include "ui_learn.c"                              /* MIDI LEARN: SAVE > COPY > LEARN, a knob, a CC */
#include "ui_tools.c"                              /* the step tools: SEQ held, the TOOLS screen */

/* fixes 2026-10-21 item 5 (upstream Felucca's breath, #119): the OCT LEDs, OCT_LIT_* lit, OCT_BR_* breathing (dark ..
 * ~60 % and back, led_dim.c: it would do something if pressed); a way back (NO, CANCEL, BACK) lit; elsewhere dark.
 * NAME: OCT- cancels, OCT+ writes; the PRESETS list: OCT+ LOAD, OCT- DELETE where it would (a used user slot); a
 * dialog: OCT- NO, OCT+ YES; the menu and the Bluetooth page: OCT- BACK, OCT+ OK (ABOUT: OCT- back alone); MIDI LEARN:
 * OCT- CLEAR, OCT+ DONE; KEYS: lit off octave 0, as before (where it is); the steps: each breathes while there is a
 * bank that way, OCT+ lit while held (its accent: item 1); the LFO pages: OCT+ (a RATE MODE) */
#define OCT_LIT_DN 1u
#define OCT_LIT_UP 2u
#define OCT_BR_UP 4u
#define OCT_BR_DN 8u
static uint32_t oct_leds(void)
{
    if (ui.layer)
        return 0;
    if (ui.tools)                                       /* TOOLS: the banks there are, either way */
        return (ui.bank + 1u < bank_count() ? OCT_BR_UP : 0u) | (ui.bank ? OCT_BR_DN : 0u);
    if (name_on())
        return OCT_LIT_DN | OCT_BR_UP;
    if (pl_on())
        return OCT_BR_UP | (preset_bank_of(ui.pr.cur) == PB_USER && upr_used(ui.pr.cur - PRE_NFACT) ? OCT_BR_DN
                                                                                                          : 0u);
    if (ui.confirm)
        return OCT_LIT_DN | OCT_BR_UP;
    if (ui.menu)
        return ui.menu == 2u ? OCT_LIT_DN : OCT_LIT_DN | OCT_BR_UP;
    if (ui.ml.on)
        return OCT_LIT_DN | OCT_BR_UP;
    if (keys_mode())
        return (song.octave < 0 ? OCT_LIT_DN : 0u) | (song.octave > 0 ? OCT_LIT_UP : 0u);
    if (seq_keys())
        return (ui.bank > 0u ? OCT_BR_DN : 0u) |
               (acc_oct ? OCT_LIT_UP : ui.bank + 1u < bank_count() ? OCT_BR_UP : 0u);
    {
        uint32_t r = 0;
        if (!ui.home && cur_fam() == FAM_LFO)    /* LFO: OCT+ the RATE MODE of the LFO turned last (item 3); */
            r = OCT_BR_UP;                            /* OCT- does nothing there */
        if (env_steps() && acc_oct)                   /* STEPS: OCT+ held (its accent) lit, as on the SEQ pages */
            r = (r & ~OCT_BR_UP) | OCT_LIT_UP;
        return r;
    }
}

/* fixes 2026-10-21 item 9 (upstream Felucca's LEDS levels; MENU BACKLIGHT, panel.c BL_*): the picture nl (lit), nb
 * (breathing), beats (the STEP keys' beats, bit k = key k) out to the LEDs. OFF and nothing breathing: fm1_led as ever,
 * the scan's hook idle; else the hook's layers (led_dim.c), one store per column: DIM LO / HI the unlit keys and the
 * idle buttons in the glow, the unlit beats in the mid layer; INV / INV LO / INV HI the idle ones lit / in the mid
 * layer and the lit ones dark; a breathing LED only breathes (neither lit nor in the glow). The levels: the layers'
 * frames of 20, the glow's pulse (led_dim.c) */
static const uint32_t BL_MID_PH[BL_COUNT] = {0u, LED_PH_25, 0x22489u, 0u, 0x401u, LED_PH_25};   /* 25 30 - 10 25 % */
static const uint32_t BL_GLOW_W[BL_COUNT] = {0u, LED_GLOW_LO, LED_GLOW_HI, 0u, 0u, 0u};       /* the glow's pulse */
static void leds_out(uint8_t *nl, const uint8_t *nb, uint32_t beats, uint32_t dimk)   /* dimk: keys dim (unlit: the
                                                       * mid layer in every mode; GLO held, the used variations) */
{
    uint32_t lay[FM1_NCOL] = {0}, k, c, br = 0, m = backlight < BL_COUNT ? backlight : BL_OFF;
    for (c = 0; c < FM1_NCOL; c++) {
        nl[c] &= (uint8_t)~nb[c];
        br |= nb[c];
    }
    if (m == BL_OFF && !br && !dimk) {
        led_dim_on = 0;                               /* (before fm1_led: the hook stops composing) */
        led_br_on = 0;
        for (c = 0; c < FM1_NCOL; c++)
            fm1_led[c] = nl[c];
        return;
    }
    for (k = 0; k < 41u; k++) {                       /* the buttons (0..13) and the keys (14..40) */
        uint8_t q = led_pos[k];
        uint32_t bit, lit;
        if (q == 0xFF)
            continue;
        bit = 1u << (q & 7u);
        lit = nl[q >> 3] & bit;
        if (nb[q >> 3] & bit)
            lay[q >> 3] |= bit << 24;                 /* breathing */
        else if (!lit && k >= 14u && ((dimk >> (k - 14u)) & 1u))
            lay[q >> 3] |= bit << 8;                  /* dim */
        else if (m == BL_DIM_LO || m == BL_DIM_HI)
            lay[q >> 3] |= lit ? bit : k >= 14u && ((beats >> (k - 14u)) & 1u) ? bit << 8 : bit << 16;
        else if (m == BL_INV)
            lay[q >> 3] |= lit ? 0u : bit;
        else if (m >= BL_INV_LO)
            lay[q >> 3] |= lit ? 0u : bit << 8;
        else
            lay[q >> 3] |= lit;
    }
    led_mid_ph = !BL_MID_PH[m] && dimk ? LED_PH_25 : BL_MID_PH[m];
    led_glow_w = BL_GLOW_W[m];
    led_br_on = br != 0u;
    for (c = 0; c < FM1_NCOL; c++)
        led_lay[c] = lay[c];                          /* one store per column: the scan sees all of it or none */
    led_dim_on = 1;                                   /* (after the layers: the hook composes from them) */
}

static uint32_t in_now;                               /* ui_input's fm1_ticks (the step LEDs' blinks: no read of
                                                       * their own) */
static void ui_leds(void)
{
    uint8_t nl[FM1_NCOL] = {0}, nb[FM1_NCOL] = {0};
    uint32_t k, fam = cur_fam(), beats = 0, dimk = 0; /* beats: the keys of steps 1 5 9 13 (step keys); dimk: the
                                                       * keys dim (leds_out) */
    static uint8_t ready;
    if (safe_start) {                                 /* safe start: LEDs off */
        uint32_t c0;
        led_dim_on = 0;
        for (c0 = 0; c0 < FM1_NCOL; c0++)
            fm1_led[c0] = 0;
        return;
    }
    if (!ready) {
        led_pos_init();
        ready = 1;
    }
    led_put(nl, panel.btn[FAM_BTN[fam]], 1);
    led_put(nl, panel.btn[B_PLAY], song.playing && ((song.tick / 64u) & 1u) == 0u);   /* blinks: intended */
    led_put(nl, panel.btn[B_REC], song.rec != 0u || ui.srec);
    led_put(nl, panel.btn[B_SCL], ui.keys);           /* KEYS (spec §5) */
    led_put(nl, panel.btn[B_ENV], ui.env);            /* STEPS (spec 2026-10-24) */
    if (ui.cue_map) {                                 /* SAVE held: the cue map (project.c cue_led): white keys
                                                       * 1..4 the slots, the black keys (CUE_BK) the variations */
        uint32_t ms = fm1_ticks() / (1000u * FM1_TICKS_PER_US);
        cue_map_refresh();
        int blink = (int)((ms / 250u) & 1u), wink = (ms % 1000u) >= 125u;
        for (k = 0; k < 12u; k++) {
            int c = cue_led(k + 1u);
            uint32_t key = k < 4u ? STEP_KEY[k] : CUE_BK[k - 4u];
            led_put(nl, 14u + key, c == CM_ON || (c == CM_WINK && wink) || (c == CM_BLINK && blink));
            led_put(nb, 14u + key, c == CM_BREATHE);
        }
        led_put(nl, panel.btn[B_SAVE], 1);
    } else if (ui.tools) {                            /* TOOLS screen (SEQ held): ui_tools.c */
        tools_leds(nl);
    } else if (mod_live(ui.glo_t0)) {                 /* GLO held: the playing pattern's white key lit, a queued
                                                       * one blinking; black 1..8 the variations (project.c cue_led:
                                                       * of the queued pattern if one is): the playing one lit, a
                                                       * queued one blinking, the other used ones dim, unused dark */
        int blink = (int)((fm1_ticks() / (250u * 1000u * FM1_TICKS_PER_US)) & 1u);
        int32_t ld = proj_loaded();
        for (k = 0; k < GLO_SLOTS; k++)
            led_put(nl, 14u + STEP_KEY[k], cue.q_slot == k + 1u ? blink : (int32_t)k == ld);
        cue_map_refresh();
        for (k = 0; k < 8u; k++) {
            int c = cue_led(5u + k);
            led_put(nl, 14u + CUE_BK[k], c == CM_WINK || (c == CM_BLINK && blink));
            if (c == CM_ON)
                dimk |= 1u << CUE_BK[k];
        }
    } else if (mod_live(ui.arp_t0) || mod_live(ui.sel_t0) || (ui.seq_t0 && !(ui.seq_t0 & (TL_DEAD | TL_PLAIN)))) {
        int blink = (int)((fm1_ticks() / (250u * 1000u * FM1_TICKS_PER_US)) & 1u);   /* (SEQ: before TOOLS opens) */
        for (k = 0; k < NTRK; k++)                    /* ARP: the soloed tracks; SEL: the selected one; SEQ: the
                                                       * audible ones (a mute waiting for the bar blinks) */
            led_put(nl, 14u + CUE_BK[k], mod_live(ui.arp_t0) ? (int)((song.solo >> k) & 1u)
                                         : mod_live(ui.sel_t0) ? k == song.sel
                                         : !trk[k].p[P_MUTE] ^ (((song.mute_q >> k) & 1u) && blink));
        if (mod_live(ui.arp_t0))
            led_put(nl, 14u + KEY_SOLO_MODE, solo_mom);   /* F#5: lit while MOMENTARY */
    } else if (name_on()) {                           /* NAME: the keys that type */
        uint32_t m = name_leds(fm1_ticks());
        for (k = 0; k < 27u; k++)
            led_put(nl, 14u + k, (int)((m >> k) & 1u));
    } else if (pl_on()) {                             /* the PRESETS list: the keys play (lit while held) */
        for (k = 0; k < 27u; k++)
            led_put(nl, 14u + k, (int)((fm1_in.notes >> k) & 1u));
    } else if (ui.layer) {                            /* PERFORM: effects and REPEAT STEP's track keys blink, held
                                                       * lit, unavailable dark; the rate keys: the selected one lit */
        int blink = (int)((fm1_ticks() / (250u * 1000u * FM1_TICKS_PER_US)) & 1u);
        uint32_t held = perf_kill ? 0u : perf_held, ok = perf_avail();
        for (k = 0; k < 27u; k++) {
            uint32_t e = perf_key(k), r = perf_rate_key(k);
            led_put(nl, 14u + k, e < PF_N ? (int)(((ok >> e) & 1u) && (((held >> e) & 1u) || blink))
                                          : (int)(r == perf_rate));
        }
    } else if (step_keys()) {                         /* the bank's steps (Model:Cycles style): a trig lit, a
                                                       * locked step (parameter / machine lock) blinking, the
                                                       * selected track's playhead inverted; black 1..6 the tracks */
        const track_t *t = TSEL;
        uint32_t ms = in_now / (1000u * FM1_TICKS_PER_US), len = view_len(t);   /* (ui_input's time) */
        uint32_t ph = song.playing ? view_idx(t) : NSTEP;   /* (the selected track's own step: its LEN, DIV, FIT) */
        uint32_t lm = acc_oct ? 0u : lk_view_mask(); /* (cached: ui.c lk_view_mask) */
        int blink = (int)((ms / 250u) & 1u) == 0, wink = (ms % 1000u) >= 125u;
        for (k = 0; k < 16u; k++) {
            uint32_t si = ui.bank * 16u + k;
            int lk = si < len && ((lm >> k) & 1u), on = si < len && view_step(t, si).on &&
                                                       (!acc_oct || view_step(t, si).acc);   /* OCT+ held: the accents */
            if (si == ph)
                on = !(on || lk);                     /* the playhead: the key's state toggled */
            else if (lk)
                on = blink;
            led_put(nl, 14u + STEP_KEY[k], on);
            if ((k & 3u) == 0u)
                beats |= 1u << STEP_KEY[k];
        }
        for (k = 0; k < NTRK; k++) {                  /* the tracks' black keys (SEL + one selects): the selected
                                                       * lit, an armed one (REC) breathing, both: lit, winking */
            int arm = (int)((song.rec >> k) & 1u);
            if (k == song.sel)
                led_put(nl, 14u + CUE_BK[k], !arm || wink);
            else
                led_put(nb, 14u + CUE_BK[k], arm);
        }
    } else if (comp_mode()) {                         /* COMP: the ducked tracks' keys */
        for (k = 0; k < NTRK; k++)
            led_put(nl, 14u + KEY_TRK_KEY[k], trk[k].p[P_DUCK] && k != comp_src());
    } else if (track_keys()) {                        /* HOME, EDIT, FX, LFO: the selected track's key (item 2) */
        led_put(nl, 14u + KEY_TRK_KEY[song.sel % NTRK], 1);
        for (k = 0; k < 27u; k++)
            led_put(nl, 14u + k, (int)((fm1_in.notes >> k) & 1u));
    } else if (keys_mode()) {                         /* KEYS: the keys held lit (OCT- / OCT+: oct_leds) */
        for (k = 0; k < 27u; k++)
            led_put(nl, 14u + k, (int)((fm1_in.notes >> k) & 1u));
    } else {
        for (k = 0; k < 27u; k++)
            led_put(nl, 14u + k, (int)((fm1_in.notes >> k) & 1u));
    }
    k = oct_leds();                                   /* OCT- / OCT+: lit, or breathing (item 5) */
    led_put(nl, panel.btn[B_OCTDN], (int)(k & OCT_LIT_DN));
    led_put(nl, panel.btn[B_OCTUP], (k & OCT_LIT_UP) != 0u);
    led_put(nb, panel.btn[B_OCTUP], (k & OCT_BR_UP) != 0u);
    led_put(nb, panel.btn[B_OCTDN], (k & OCT_BR_DN) != 0u);
    leds_out(nl, nb, beats, dimk);
}

/* ---------------------------------------------------------- input --- */
/* knob acceleration (MENU > KNOB ACCEL ON; after upstream Felucca 1.1.5's, 276f72a, #23 / #52 / #126): a turn going on
 * in one direction over a wide value (range > 32) speeds up with the time per click, the longer of this read's and the
 * previous read's: x ACC_RATE / ms, in sixteenths of a step carried through the turn (80 ms x1.25, 50 ms (20 clicks/s)
 * x2, 25 ms x4, 12.5 ms x8), at most x4, x8 over a range above 64 (upstream's caps). A click slower than ACC_GAP ms
 * (12.5 clicks/s and slower), the first two reads of a turn, a single quick click (a bounce) and a reversal are one
 * step a click, and the sign is always the clicks'. Faster than upstream's (x50 / ms from 40 ms: x1 to 25 clicks/s,
 * x2 .. x3 at 40 .. 60) at a medium turn; the same caps. A list of names (F_ENUM: MODEL, TYPE ..) never speeds up
 * (knob_turn; upstream's too). OFF: always one step a click. ui.enc_t[role]: bits 0..23 the ms of its last read, bit 24
 * its direction (+1), 25..31 its ms per click (127 slow); ui.enc_c[role] the carry. tests/accel_test.c: the clicks
 * to sweep 0..127 against upstream's */
#define ACC_GAP 80u
#define ACC_RATE 100u
static int32_t accel(uint32_t role, int32_t s, int32_t range)
{
    uint32_t now = (fm1_ticks() / (1000u * FM1_TICKS_PER_US)) & 0xFFFFFFu, st = ui.enc_t[role], up = s > 0, pi = st >> 25;
    uint32_t a, i, m = 16u, cap;
    if (!settings.accel || range <= 32 || !s)
        return s;
    a = (uint32_t)(s < 0 ? -s : s);
    i = ((now - st) & 0xFFFFFFu) / a;                   /* ms per click of this read */
    if (!st || ((st >> 24) & 1u) != up || i >= ACC_GAP) {
        i = 127u;                                       /* a new turn, or reversed */
        ui.enc_c[role] = 0;
    } else if (pi < ACC_GAP) {
        cap = range > 64 ? 8u * 16u : 4u * 16u;
        m = ACC_RATE * 16u / (i > pi ? i : pi ? pi : 1u);
        m = m < 16u ? 16u : m > cap ? cap : m;
    }
    ui.enc_t[role] = now | up << 24 | (i ? i : 1u) << 25;
    a = a * m + ui.enc_c[role];                         /* sixteenths: at least one step a click */
    ui.enc_c[role] = (uint8_t)(a & 15u);
    return up ? (int32_t)(a >> 4) : -(int32_t)(a >> 4);
}

/* a knob (role) turned s on the value v of d: the new value; a list of names one entry a click (upstream 1.1.5) */
static int32_t knob_turn(uint32_t role, const param_desc_t *d, int32_t v, int32_t s)
{
    return param_turn(d, v, accel(role, s, d->fmt == F_ENUM ? 0 : d->max - d->min));
}

/* a step key held + KNOB 1: its PROB, KNOB 2: its RATCH, KNOB 3: its accent (a detent right ACC on, left off),
 * KNOB 4: its MICRO (-50 .. +50, spec 2026-10-31 §1), from the first detent (the press changed nothing). An edit is no
 * tap: the keys down let go toggle nothing. An off step's first edit turns it on (step_hold_on, as the hold itself:
 * KNOB 3 right at once = on accented). A held key outside LEN takes the turn and does nothing, as does any while the
 * web editor restores (RESTORING: ui_locked) */
static int step_edit(uint32_t knob, int32_t steps)
{
    uint32_t k = ui.held, i;
    step_t *s;
    if (knob > 3u || k >= 16u || !ui.step_t0[k])
        return 0;
    for (i = 0; i < 16u; i++)                         /* edited: no key down is a tap any more */
        if (ui.step_t0[i])
            ui.step_t0[i] |= 2u;
    if (ui.step_si[k] >= (uint32_t)TSEL->p[P_SLEN] || ui_locked())   /* outside LEN, a restore */
        return 1;
    s = &TSEL->step[ui.step_si[k]];
    step_hold_on(k);
    if (knob == 2u) {
        s->acc = steps > 0;                           /* (absolute: a fast turn's detents never flicker it) */
        return 1;
    }
    if (knob == 3u) {
        s->micro = (int8_t)clamp(s->micro + accel(EN_K4, steps, 2 * MICRO_MAX), -MICRO_MAX, MICRO_MAX);
        return 1;
    }
    if (knob == 0u)
        s->cond = (uint8_t)cond_store((uint32_t)clamp((int32_t)cond_pos(s->cond) + accel(EN_K1, steps, COND_MAX), 0,
                                                      (int32_t)COND_MAX));
    else
        s->rat = (uint8_t)clamp((int32_t)s->rat + (steps > 0 ? 1 : -1), 0, 3);
    return 1;
}

/* TRACKS mixer: KNOB 1 TRACK, 2 LEVEL (a muted track: the first turn unmutes), 3 LEN, 4 PAN */
static void tracks_edit(uint32_t slot, int32_t steps)
{
    track_t *t = TSEL;
    uint32_t id;
    if (slot == 0u) {
        track_select((uint32_t)clamp((int32_t)song.sel + (steps > 0 ? 1 : -1), 0, NTRK - 1));
        return;
    }
    if (slot == 1u && t->p[P_MUTE]) {
        t->p[P_MUTE] = 0;
        ml_knob(song.sel, P_LEVEL);                 /* (MIDI LEARN: picked) */
        return;
    }
    id = slot == 1u ? P_LEVEL : slot == 2u ? P_SLEN : P_PAN;
    t->p[id] = (int16_t)clamp(t->p[id] + accel(EN_K1 + slot, steps, TP[id].max - TP[id].min), TP[id].min, TP[id].max);
    motion_capture(t, id);                          /* motion.c: LEVEL / PAN recorded (LEN never) */
    ml_knob(song.sel, id);                          /* (MIDI LEARN: picked) */
}

static void mute_bar_set(uint32_t on)                  /* the mutes: at once / on the next bar (device setting) */
{
    settings.mutebar = on & 1u;
    song.act[2] = (int16_t)settings.mutebar;
    settings_save();
    ui_message(settings.mutebar ? "MUTE: NEXT BAR" : "MUTE: NOW");
}

static void track_rec_toggle(uint32_t k)               /* arm / disarm track k (REC, every screen; the SMC-Mixer). It
                                                        * never starts play (owner, 2026-10-22): PLAY does; an armed
                                                        * track records once playing */
{
    song.rec ^= (uint8_t)(1u << k);
}

/* REC (owner, 2026-10-22): on TRACKS a tap arms / disarms the selected track ("T3 REC ARMED" / "T3 REC OFF"); on every
 * other page it turns REC ALL on or off. REC ALL: every track armed, so each white key that plays its track records it
 * (seq.c input_hit: HOME, the SOUND / FX pages, TRACKS ...; KEYS, step record and motion stay the selected track's);
 * off: the arm state from before it back. On TRACKS during REC ALL: it ends, the arm state from before it plus the
 * selected track. REC never starts play. Never stored */
#define REC_ALL_MASK ((uint8_t)((1u << NTRK) - 1u))
static void rec_all_drop(void) { ui.rec_all = 0; }    /* REC ALL off, the arm state as it is (SMC REC n) */
static void rec_say(uint32_t k, const char *what)      /* "T3 REC ARMED" */
{
    char m[4] = "T1 ";
    m[1] = (char)('1' + k);
    ui_say(m, what);
}
static void rec_tap(void)                              /* a REC tap (the button, a learned REC, the SMC's REC); step
                                                        * record already passed */
{
    uint32_t k = song.sel, bit = 1u << k;
    if (mix_mode()) {                                  /* TRACKS: the selected track */
        if (ui.rec_all) {
            ui.rec_all = 0;
            song.rec = (uint8_t)(ui.rec_keep | bit);
        } else {
            track_rec_toggle(k);
        }
        rec_say(k, (song.rec & bit) ? "REC ARMED" : "REC OFF");
        return;
    }
    if (ui.rec_all) {                                  /* off: the arm state from before REC ALL */
        ui.rec_all = 0;
        song.rec = ui.rec_keep;
        for (k = 0; k + 1u < NTRK && !((song.rec >> k) & 1u); k++)
            ;
        if (!song.rec)
            ui_message("REC OFF");
        else if (song.rec & (song.rec - 1u))
            ui_message("REC TRACKS");
        else
            rec_say(k, "REC ARMED");                   /* (one armed: its name) */
        return;
    }
    ui.rec_keep = song.rec;
    ui.rec_all = 1;
    song.rec = REC_ALL_MASK;                           /* (no play started) */
    ui_message("REC ALL");
}

/* MOTION page: KNOB 1 PLAY (right ON, left OFF), KNOB 4 CLEAR (a turn right asks: "CLEAR MOTION T<n>?") */
static void motion_page_edit(uint32_t slot, int32_t steps)
{
    if (slot == 0u) {
        motion_set_play(song.sel, steps > 0);
    } else if (slot == 3u && steps > 0 && motion_count(song.sel)) {
        ui.confirm = 2;
        ui.confirm_trk = song.sel;
        ui.force = 1;
    }
}

static void edit_param(uint32_t slot, int32_t steps)
{
    int16_t *vp;
    const page_t *pg = cur_page();
    const param_desc_t *d;
    uint32_t id = page_id(pg, slot);
    int32_t v;
    if (pg->scope == SC_GRID) {
        if (slot == 0u)
            bank_set((int32_t)ui.bank + (steps > 0 ? 1 : -1));
        return;
    }
    if (pg->graph == GR_MOTION) {                     /* the MOTION page's own knobs */
        motion_page_edit(slot, steps);
        return;
    }
    if (pg->graph == GR_LDEST && slot == 0u) {        /* LFO DEST: KNOB 1 the slot */
        song.lslot = (uint8_t)clamp((int32_t)(song.lslot & 3u) + steps, 0, (int32_t)LS_N - 1);
        return;
    }
    if (pg->graph == GR_LFO)
        ui.lfo_l = (uint8_t)(slot >> 1);              /* OCT+: this LFO's RATE MODE */
    if (pg->scope == SC_MIX) {
        tracks_edit(slot, steps);
        return;
    }
    if (pg->scope == SC_TRACK && id == P_MODEL) {     /* a model comes with its default sound */
        model_step(steps);
        ml_knob(song.sel, P_MODEL);                   /* (MIDI LEARN: NOT LEARNABLE) */
        return;
    }
    if (pg->graph == GR_SLOTS && id == G_NAME) {      /* PROJECT, NAME: rename the selected slot */
        if (steps > 0)
            name_rename();
        return;
    }
    d = page_desc(pg, slot, &vp);
    if (!d || !vp || d->max == d->min)
        return;
    if (pg->scope == SC_GLOBAL && id == G_BPM && song.g[G_CLOCK]) {
        ui_say("CLK ", N_CLOCK[song.g[G_CLOCK]]);
        return;
    }
    v = knob_turn(EN_K1 + slot, d, *vp, steps);
    if (d->fmt == F_SLOT && pg->scope == SC_TRACK) {   /* SLOT: its set's samples 1..N (a KEY
                                                                                * note from before SLOT: from its own) */
        int32_t i = smp_slot_of((uint32_t)TSEL->p[P_E4], *vp), n = (int32_t)smp_slot_count((uint32_t)TSEL->p[P_E4]);
        v = SMP_SLOT_V(clamp((i < 0 ? 0 : i) + steps, 0, n > 0 ? n - 1 : 0));   /* (no acceleration: <= 16) */
    }
    if (pg->scope == SC_TRACK && vp >= TSEL->p && vp < TSEL->p + P_COUNT && preset_marks(TSEL, (uint32_t)(vp - TSEL->p)) &&
        v != *vp)
        preset_touch(TSEL);                           /* presets.c: the preset edited (not a turn at its limit) */
    *vp = (int16_t)v;
    if (pg->scope == SC_TRACK && vp >= TSEL->p && vp < TSEL->p + P_COUNT) {
        fm1_irq_off();
        smp_set_moved(TSEL, (uint32_t)(vp - TSEL->p));   /* presets.c: a SET keeps its SLOT inside its samples */
        fm1_irq_on();
        motion_capture(TSEL, (uint32_t)(vp - TSEL->p));   /* motion.c: recorded when armed and playing */
        ml_knob(song.sel, (uint32_t)(vp - TSEL->p));      /* (MIDI LEARN: picked) */
    }
    if (pg->scope == SC_GLOBAL && id == G_MUTEBAR) {
        mute_bar_set((uint32_t)v);
        return;
    }
    if (pg->scope == SC_GLOBAL && id == G_METRO) {    /* METRO: the device setting (fx.c's click), saved */
        fx_metro = (uint8_t)v;
        settings_save();
        return;
    }
    if (pg->scope == SC_GLOBAL && id == G_THRU) {     /* THRU: the device setting (midi_out.c), saved */
        midi_thru = (uint8_t)v;
        settings_save();
        return;
    }
    if (pg->scope == SC_GLOBAL && id == G_LEARN) {    /* LEARN (the COPY page's KNOB 4): MIDI LEARN on / off */
        if ((uint32_t)v != ui.ml.on)
            ml_toggle();
        return;
    }
    if (pg->scope == SC_GLOBAL && id == G_KBD) {      /* KBD: the MIDI KEYBOARD's channel (midi_out.c mkb_*), saved;
                                                       * turned on: KEYS on too, OFF: KEYS' own octaves again */
        if (v && !mkb_kbd && !ui.keys)
            keys_toggle();
        mkb_kbd = (uint8_t)v;
        if (!v)
            keys_octave(0);
        ui_message(v ? "MIDI KEYBOARD" : "MIDI KEYBOARD OFF");
        settings_save();
        return;
    }
    if (pg->scope == SC_GLOBAL && id == G_LOCAL) {    /* LOCL: the MIDI KEYBOARD's keys sound here too (OFF: MIDI
                                                       * only), saved */
        mkb_local_off = (uint8_t)!v;
        settings_save();
        return;
    }
    if (pg->scope == SC_GLOBAL && id == G_QUANT) {    /* QUANT: the device setting (seq.c rec_hit), saved */
        rec_quant = (uint8_t)v;
        settings_save();
        return;
    }
    if (!v || pg->scope != SC_GLOBAL)
        return;
    if ((id == G_LOAD || id == G_SAVE || id == G_CLRSEQ || id == G_INITSND || id == G_CLRALL || id == G_INITALL ||
         id == G_PASTE) && ui.arm != id) {
        *vp = 0;                                      /* one detent arms, a second one within ~1.5 s acts */
        ui.arm = (uint8_t)id;
        ui.arm_t = 90;
        ui_say("AGAIN: ", d->label);
        return;
    }
    ui.arm = 0;
    switch (id) {                                     /* GO buttons: act, then back to 0 */
    case G_LOAD:
        *vp = 0;
        project_load((uint32_t)song.g[G_SLOT] - 1u);
        break;
    case G_SAVE:                                      /* (with the current name; NAME renames: ui_name.c) */
        *vp = 0;
        project_save((uint32_t)song.g[G_SLOT] - 1u);
        break;
    case G_COPY:                                      /* the COPY page (spec 2026-10-26): SLOT's slot remembered */
        *vp = 0;
        project_copy((uint32_t)song.g[G_SLOT] - 1u);
        break;
    case G_PASTE:                                     /* .. pasted onto SLOT's (again on it: undone) */
        *vp = 0;
        project_paste((uint32_t)song.g[G_SLOT] - 1u);
        break;
    case G_CLRSEQ:
        *vp = 0;
        track_clear(TSEL);
        ui_message("PATTERN CLEARED");
        break;
    case G_CLRALL:
        *vp = 0;
        seq_clear_all();
        ui_message("ALL PATTERNS CLEARED");
        break;
    case G_INITALL:
        *vp = 0;
        init_all();
        ui_message("ALL INIT");
        break;
    case G_INITSND:
        *vp = 0;
        fm1_irq_off();
        drum_set_model(TSEL, (uint32_t)TSEL->p[P_MODEL]);
        for (id = P_FTYPE; id <= P_FDEC; id++)
            TSEL->p[id] = TP[id].def;                   /* the FILTER too (spec 2026-10-08 §3) */
        motion_rebase(song.sel);
        fm1_irq_on();
        ui_message("SOUND INIT");
        ui.force = 1;
        break;
    default:
        break;
    }
}

/* HOME / REC: tap on release, hold 0.7 s fires once. t0 = press time | 1,
 * bit 1 = fired (or swallowed: then the release is no tap either) */
enum { BT_NONE, BT_TAP, BT_HOLD };
static uint32_t btn_hold(uint32_t *t0, uint32_t label, uint32_t now, int hold_ok)
{
    uint32_t tap;
    if ((fm1_in.buttons >> panel.btn[label]) & 1u) {
        if (!*t0)
            *t0 = (now | 1u) & ~2u;
        else if (hold_ok && !(*t0 & 2u) && now - (*t0 & ~3u) > 700u * 1000u * FM1_TICKS_PER_US) {
            *t0 |= 2u;
            return BT_HOLD;
        }
        return BT_NONE;
    }
    tap = *t0 && !(*t0 & 2u);
    *t0 = 0;
    return tap ? BT_TAP : BT_NONE;
}

/* PERFORM (perform.c): FX tapped = its pages, on the release (nothing else touched); FX held = the layer: keys
 * pressed with it are its own (seq.c keyboard_block), KNOB 1..4 its macros FILTER CRUSH THROW DEPTH (never saved;
 * spec §7: they stay on when the screen goes, until turned back to 0 on it, INIT ALL or BENCH). It opens at once
 * with a key or a knob, else after FX_HOLD_MS (then the release does nothing). No layer in the menu, a dialog or
 * TOOLS: a press there stays dead until let go, and every key effect is off until its keys are let go */
#define FX_HOLD_MS 400u
#define FX_DOWN 1u
#define FX_OPEN 2u
#define FX_DEAD 4u
static int fx_allowed(void) { return !ui.menu && !ui.confirm && !name_on() && !ui.tools && !pl_on(); }
/* fixes 2026-10-21 item 7: PERFORM has KNOB 1..4 (fx_layer reads them; set each pass): the page's loop leaves them
 * alone, so detents that come in after fx_layer's read wait for its next one instead of reaching the page under it.
 * 1 PERFORM's; 2 the pass it stops being (FX let go): the detents read then are still the macros', the page's loop
 * still leaves them (a knob turning while FX is let go moves nothing under it); 0 the page's */
static uint8_t fx_knobs;
/* PERFORM PAGE: the screen closed (the screen under it changed, HOME, the menu, a dialog): the macros stay on; keys
 * still held stay the layer's until let go */
static void perf_page_close(void)
{
    ui.pg_open = 0;
    perf_mask &= ~PERF_PAGE;
}
static void fx_layer(uint32_t now, int home_tap)
{
    uint32_t bit = 1u << panel.btn[B_FX], *t0 = &ui.fx_t0, k, show, down = (fm1_in.buttons & bit) != 0u;
    int32_t s;
    if (!fx_allowed()) {
        perf_kill = 1;                                  /* the buffer effects off (the macros stay on: spec §7) */
    } else if (!kb_layer) {
        perf_kill = 0;
    }
    if (ui.pg_open && (!fx_allowed() || home_tap || ui.page != ui.pg_page || ui.home != ui.pg_home))
        perf_page_close();
    if (down) {
        if (!*t0)
            *t0 = (now & ~7u) | FX_DOWN;
        if (!fx_allowed())
            *t0 = (*t0 | FX_DEAD) & ~FX_OPEN;           /* dead until let go: no map, no layer, no PAGE */
        if (!(*t0 & FX_DEAD) && (kb_layer || now - (*t0 & ~7u) >= FX_HOLD_MS * 1000u * FM1_TICKS_PER_US))
            *t0 |= FX_OPEN;
    } else if (*t0) {
        if (!(*t0 & (FX_OPEN | FX_DEAD)))
            open_family(FAM_FX);                        /* a tap: the FX pages, as the button always did */
        *t0 = 0;
    }
    fx_knobs = (down && !(*t0 & FX_DEAD)) || ui.pg_open || (kb_layer && fx_allowed()) ? 1u : fx_knobs == 1u ? 2u : 0u;
    if (fx_knobs)                                       /* the knobs follow the screen: */
        for (k = 0; k < 4u; k++)                        /* the macros (FILTER CRUSH THROW DEPTH) while it is PERFORM's */
            if ((s = panel_enc(EN_K1 + k)) != 0) {
                int32_t v = clamp(perf_k[k] + accel(EN_K1 + k, s, 200), k ? 0 : -100, 100);
                if (!k && perf_k[0] && (v < 0) != (perf_k[0] < 0))
                    v = 0;                              /* FILTER: a turn across 0 stops at 0, its off point */
                perf_k[k] = (int8_t)v;
                ml_pick_act(ML_A_MAC + k);              /* (MIDI LEARN: the macro picked, as a knob's parameter) */
                if (down && !(*t0 & FX_DEAD))
                    *t0 |= FX_OPEN;                     /* (a dead hold stays dead: a held layer key's turn
                                                         * opens no map that outlives the key) */
            }
    if (settings.perfpage && (*t0 & FX_OPEN) && !ui.pg_open && fx_allowed()) {   /* PAGE: the screen stays */
        ui.pg_open = 1;
        ui.pg_page = ui.page;
        ui.pg_home = ui.home;
    }
    perf_mask = (fx_allowed() && !(*t0 & FX_DEAD) ? bit : 0u) | (ui.pg_open ? PERF_PAGE : 0u);   /* armed before FX
                                                         * is pressed: a key struck with it is the layer's even before
                                                         * this pass has seen FX (keyboard_block tests the button) */
    show = fx_allowed() && ((*t0 & FX_OPEN) || kb_layer || ui.pg_open);   /* (the screen gone: the macros stay) */
    if (show != ui.layer) {
        ui.layer = (uint8_t)show;
        ui.force = 1;
    }
}

/* the combos (FREITAKT, Elektron Model:Cycles style): hold GLO, SEL or ARP and press a key. Every key pressed with
 * one of them is its own until let go (seq.c keyboard_block, mute_mask: no note, no MIDI, nothing recorded). Pressed in
 * the menu, a confirm, NAME, the PRESETS list or TOOLS: dead until let go; with the PERFORM layer up or FX held: a plain
 * button. Two held: GLO's keys, then ARP's, then SEL's.
 * GLO: a tap (let go within GLO_HOLD_MS, no key pressed meanwhile) = the GLOBAL pages, on the release; + white key n
 * queues pattern n (A..D; the first of a hold, at once: cue_slot; whites past GLO_SLOTS: nothing); + black key n (1..8)
 * queues variation n (cue_var: of the queued pattern if one is, else the playing one). 2 or more whites: on the
 * release, the chain of their entries in press order (chain_set; repeats allowed, CHAIN_N at most): each white an
 * entry, its pattern and that pattern's last-played variation (chain_ent), a black key after it that entry's
 * variation. A black key before any white of the hold: a plain variation cue; after the first white (still one): its
 * variation, cued too (a plain cue so far); black 9 / 10 (C#5 / D#5): nothing. SEL: + black 1..6 selects track 1..6; let go with no key pressed: KEYS on /
 * off. ARP: + black 1..6 SOLO of tracks 1..6 (TOGGLE, or MOMENTARY: while the key is held), + F#5 the SOLO mode.
 * SEQ + black 1..6 (mutes): ui_tools.c tools_input */
#define GLO_HOLD_MS 400u
static uint8_t glo_ch[CHAIN_N], glo_chn, glo_wh;       /* GLO held: the chain entries its white keys picked, in press
                                                        * order (project.c chain.s); glo_wh the whites pressed */
static uint8_t solo_held;                              /* the tracks an ARP black key soloed, the key still down
                                                        * (mute_ups) */
static int mod_dead(void) { return ui.menu || ui.confirm || name_on() || pl_on() || ui.tools; }
static int cue_owns_keys(void);
static int mod_owns_keys(void)                         /* a combo button held: the keys pressed now are its own */
{
    return mod_live(ui.glo_t0) ||
           ((mod_live(ui.arp_t0) || mod_live(ui.sel_t0)) && !cue_owns_keys() && !tools_owns_keys());
}
static void mod_hold(uint32_t b, uint32_t *t0, uint32_t now, uint32_t keyed)   /* button b held: its state */
{
    if (!((fm1_in.buttons >> panel.btn[b]) & 1u))
        return;
    if (!*t0)
        *t0 = (now & ~15u) | MOD_DOWN;
    if (!(*t0 & MOD_KEY) && mod_dead())
        *t0 |= MOD_DEAD;
    else if (!(*t0 & MOD_KEY) && (ui.layer || ui.fx_t0))
        *t0 |= MOD_PLAIN;
    if (keyed && !(*t0 & (MOD_DEAD | MOD_PLAIN)))
        *t0 |= MOD_KEY;
}
static uint32_t mod_up(uint32_t b, uint32_t *t0)       /* button b let go: its state (then 0), else 0 */
{
    uint32_t bit = 1u << panel.btn[b], f = *t0;
    if (f && !(fm1_in.buttons & bit))
        *t0 = 0;
    else
        f = 0;
    mute_mask = (mute_mask & ~bit) |                   /* armed before the press: a key struck with it is its own */
                (mod_live(*t0) || (!*t0 && !mod_dead() && !ui.layer && !ui.fx_t0) ? bit : 0u);
    return f;
}
static void mod_keys(uint32_t hits)                    /* the keys the ISR gave the combo button held */
{
    uint32_t k;
    if (mod_live(ui.glo_t0)) {
        if (ui_locked())
            return;
        for (k = 0; k < GLO_SLOTS; k++)                 /* white key k + 1: pattern k */
            if ((hits >> STEP_KEY[k]) & 1u) {
                uint8_t e = chain_ent(k);               /* (before the cue: its default variation) */
                if (!glo_wh)
                    (void)cue_slot(k);                  /* (a chain ends) */
                if (glo_wh < 255u)
                    glo_wh++;
                if (glo_chn < CHAIN_N)
                    glo_ch[glo_chn++] = e;
            }
        for (k = 0; k < 8u; k++)                        /* black key k + 1: variation k */
            if ((hits >> CUE_BK[k]) & 1u) {
                if (glo_wh && glo_wh == glo_chn)        /* (an entry past CHAIN_N: none to set) */
                    glo_ch[glo_chn - 1u] = (uint8_t)((glo_ch[glo_chn - 1u] & 3u) | k << 2);
                if (glo_wh <= 1u)
                    (void)cue_var(k);                   /* none yet, or the plain cue's */
            }
        return;
    }
    for (k = 0; k < NTRK; k++) {
        if (!((hits >> CUE_BK[k]) & 1u))
            continue;
        if (mod_live(ui.arp_t0)) {
            track_solo_set(k, solo_mom ? 1u : 2u);
            solo_held |= (uint8_t)(1u << k);
        } else {
            track_select(k);
        }
    }
    if (mod_live(ui.arp_t0) && ((hits >> KEY_SOLO_MODE) & 1u)) {   /* F#5: the SOLO mode flips */
        solo_mom ^= 1u;
        settings_save();
        ui_message(solo_mom ? "SOLO: MOMENTARY" : "SOLO: TOGGLE");
    }
}
/* ARP + EDIT (ARP held, its keys its own: not in the menu, a confirm, NAME, the PRESETS list, TOOLS, with the PERFORM
 * layer up or FX held): the selected track's ARP page, again ARP 2/2 (a pitched model's) and back; returns 1 if so */
static int arp_edit(void)
{
    uint32_t i = page_first(FAM_SND);
    if (!((fm1_in.buttons >> panel.btn[B_ARP]) & 1u) || (ui.arp_t0 & (MOD_DEAD | MOD_PLAIN)) || mod_dead() ||
        ui.layer || ui.fx_t0)
        return 0;
    while (i < NPAGES && PAGES[i].id[0] != P_AMODE)
        i++;
    if (!ui.home && ui.page == i && !page_hidden(i + 1u))
        i++;
    ui.arp_t0 |= ui.arp_t0 ? MOD_KEY : 0u;
    ui.page = (uint8_t)i;
    ui.fam_last[FAM_SND] = ui.page;
    ui.home = 0;
    page_entered();
    return 1;
}
static void mute_ups(void)                             /* SOLO MOMENTARY: an ARP solo key let go (ARP held or not)
                                                        * unsolos */
{
    uint32_t ups, k;
    if (!kb_mute_up)
        return;
    fm1_irq_off();
    ups = kb_mute_up;
    kb_mute_up = 0;
    fm1_irq_on();
    for (k = 0; k < NTRK; k++)
        if (((ups >> CUE_BK[k]) & 1u) && ((solo_held >> k) & 1u)) {
            solo_held &= (uint8_t)~(1u << k);
            if (solo_mom)
                track_solo_set(k, 0u);
        }
}
static void mod_input(uint32_t now, uint32_t notes)
{
    uint32_t hits = 0, f;
    if (kb_mute_hit && !cue_owns_keys() && !tools_owns_keys()) {   /* (SAVE held: the cue map's, cue_input; SEQ
                                                         * held: the TOOLS screen's, tools_input) */
        fm1_irq_off();
        hits = kb_mute_hit;
        kb_mute_hit = 0;
        fm1_irq_on();
    }
    mod_hold(B_GLO, &ui.glo_t0, now, notes | hits);
    mod_hold(B_ARP, &ui.arp_t0, now, notes | hits);
    mod_hold(B_SCL, &ui.sel_t0, now, notes | hits);
    if (hits && !mod_dead())
        mod_keys(hits);
    f = mod_up(B_GLO, &ui.glo_t0);
    if (f) {
        if (glo_chn >= 2u && !ui_locked())
            (void)chain_set(glo_ch, glo_chn);           /* 2 or more whites: their chain */
        else if (!(f & (MOD_KEY | MOD_DEAD)) && !mod_dead() &&
                 ((f & MOD_PLAIN) || now - (f & ~15u) < GLO_HOLD_MS * 1000u * FM1_TICKS_PER_US))
            open_family(FAM_GLO);                       /* a tap: the GLOBAL pages */
        glo_chn = glo_wh = 0;
    }
    (void)mod_up(B_ARP, &ui.arp_t0);
    f = mod_up(B_SCL, &ui.sel_t0);
    if (f && !(f & (MOD_KEY | MOD_DEAD)) && !mod_dead())
        keys_toggle();                                  /* a SEL tap: KEYS on / off (spec §5) */
    mute_ups();
}

/* SAVE (spec 2026-10-26, the live cue): a tap opens the projects page on its release (as FX / GLO); held 0.4 s, or
 * pressed with a key, the cue map: white keys 1..4 queue slots A..D, the black keys F#3 .. A#4 (CUE_BK) variations 1..8
 * (project.c cue_slot / cue_var; 2026-10-27: no longer white keys 5..12), the top line CUE A1 / CUE>B3; its release
 * then does nothing else. Every key pressed with SAVE is the map's until let go (seq.c keyboard_block, mute_mask: no note, no MIDI, nothing recorded). Pressed in the menu, a
 * confirm, NAME, the PRESETS list or TOOLS: dead until let go; with the PERFORM layer up or FX held: a plain
 * button (the release opens the projects page) */
/* FILL (spec 2026-10-26): REC held FILL_HOLD_MS on any page but SEQ's (STEP, PATTERN, MOTION: there REC held is
 * their confirm) plays a fill while held (seq.c fill_on: FILL steps play, !FILL ones not), the top line FILL; its
 * release then is no tap (rec_t0's bit 1: btn_hold). A shorter press is the tap, on the release, as before. Not in the
 * menu, a confirm, NAME, the PRESETS list, TOOLS or MIDI LEARN. A learned REC and the SMC's REC are taps */
#define FILL_HOLD_MS 400u
static void rec_fill(uint32_t now)
{
    int held = (fm1_in.buttons >> panel.btn[B_REC]) & 1u;
    if (!held) {
        fill_on = 0;
        return;
    }
    if (!fill_on && ui.rec_t0 && !(ui.rec_t0 & 2u) && cur_fam() != FAM_SEQ && !ui.menu && !ui.confirm &&
        !name_on() && !pl_on() && !ui.tools && !ui.ml.on &&
        now - (ui.rec_t0 & ~3u) >= FILL_HOLD_MS * 1000u * FM1_TICKS_PER_US) {
        fill_on = 1;
        ui.rec_t0 |= 2u;                                /* (no tap on the release) */
    }
}

#define CUE_HOLD_MS 400u
#define CUE_DOWN 1u
#define CUE_OPEN 2u
#define CUE_DEAD 4u
#define CUE_PLAIN 8u
static int cue_dead(void) { return ui.menu || ui.confirm || name_on() || pl_on() || ui.tools; }
static int cue_owns_keys(void)                         /* SAVE held (GLO not): the keys pressed now are the map's */
{
    return ((fm1_in.buttons >> panel.btn[B_SAVE]) & 1u) && !((fm1_in.buttons >> panel.btn[B_GLO]) & 1u) &&
           !(ui.cue_t0 & (CUE_DEAD | CUE_PLAIN)) && !cue_dead() && !ui.layer && !ui.fx_t0;
}
static void cue_keys(uint32_t hits)                    /* white key n: STEP_KEY[n - 1]; black key v: CUE_BK[v - 1] */
{
    uint32_t n;
    if (ui_locked())
        return;
    for (n = 0; n < 4u; n++)
        if ((hits >> STEP_KEY[n]) & 1u)
            (void)cue_slot(n);
    for (n = 0; n < 8u; n++)
        if ((hits >> CUE_BK[n]) & 1u)
            (void)cue_var(n);
}
static void cue_input(uint32_t now, uint32_t notes)
{
    uint32_t bit = 1u << panel.btn[B_SAVE], *t0 = &ui.cue_t0, hits = 0;
    if (kb_mute_hit && cue_owns_keys()) {
        fm1_irq_off();
        hits = kb_mute_hit;
        kb_mute_hit = 0;
        fm1_irq_on();
    }
    if (fm1_in.buttons & bit) {
        if (!*t0)
            *t0 = (now & ~15u) | CUE_DOWN;
        if (!(*t0 & CUE_OPEN) && cue_dead())
            *t0 |= CUE_DEAD;
        else if (!(*t0 & CUE_OPEN) && (ui.layer || ui.fx_t0))
            *t0 |= CUE_PLAIN;
        if (!(*t0 & (CUE_OPEN | CUE_DEAD | CUE_PLAIN)) &&
            (notes || hits || now - (*t0 & ~15u) >= CUE_HOLD_MS * 1000u * FM1_TICKS_PER_US)) {
            *t0 |= CUE_OPEN;                            /* the cue map */
            ui.cue_map = 1;
            ui.force = 1;
            grid_drop_holds();                          /* a grid key held edits nothing under it */
        }
    } else if (*t0) {
        if (!(*t0 & (CUE_OPEN | CUE_DEAD)) && !cue_dead())
            open_family(FAM_SAVE);                      /* a tap: the projects page (then TOOLS, COPY) */
        *t0 = 0;
        if (ui.cue_map) {
            ui.cue_map = 0;                             /* let go: the screen before */
            ui.force = 1;
        }
    }
    if (hits && (*t0 & CUE_OPEN))
        cue_keys(hits);
    mute_mask = (mute_mask & ~bit) |
                (!(*t0 & (CUE_DEAD | CUE_PLAIN)) && !cue_dead() && !ui.layer && !ui.fx_t0 ? bit : 0u);
}

/* PLAY: start / stop; step record ends */
static void play_press(void)
{
    ui.srec = 0;
    if (song.playing)
        transport_req = 2;
    else if (ui_locked())
        return;                                     /* (a web editor restore: no start) */
    else
        transport_req = 1;
}

/* --------------------------------------------------- step record (spec §6) --- */
/* KEYS on the STEP page, stopped: a REC tap starts it at the bank's first step (top bar STEP REC nn). A key writes its
 * note to the cursor step (ON, NOTE; TIME = NOTE; ACC / SLIDE as FLAG left them) and the cursor moves on when it is
 * let go; a key pressed while the cursor's key is still held: SLIDE on that step, its note on the next one. KNOB 1 the
 * cursor, 2 NOTE, 3 TIME (NOTE / TIE / REST), 4 FLAG (- / ACC / SLD / A+S); an EDIT tap: a rest, the cursor on. A REC
 * tap, PLAY, leaving the page, KEYS off, another track, the transport starting, the menu, NAME or PERFORM's layer
 * (FX held) end it. The ISR sounds the keys. */
static int srec_ok(void)
{
    return keys_mode() && !mkb_kbd && !ui.home && !ui.layer && cur_page()->scope == SC_GRID && !song.playing;
}
static void srec_start(void)
{
    uint32_t len = view_len(TSEL);
    ui.srec = 1;
    ui.sr_cur = (uint8_t)(ui.bank * 16u < len ? ui.bank * 16u : len - 1u);
    ui.sr_key = 0;
    grid_drop_holds();
}
static void srec_next(void)                            /* the cursor on (LEN wraps), its bank shown */
{
    ui.sr_cur = (uint8_t)((ui.sr_cur + 1u) % view_len(TSEL));
    ui.bank = (uint8_t)(ui.sr_cur / 16u);
    ui.sr_key = 0;
}
static void srec_note(uint32_t key)                    /* a key pressed: its note on the cursor step */
{
    track_t *t = TSEL;
    step_t *s;
    int32_t nt;
    if (!keys_note(t, key, &nt))
        return;                                        /* (a black key on a CHOP track: nothing) */
    if (ui.sr_key) {                                   /* the cursor's key still held: its step slides into this one */
        t->step[ui.sr_cur].slide = 1;
        srec_next();
    }
    s = &t->step[ui.sr_cur];
    s->on = 1;
    s->tie = 0;
    s->micro = 0;                                      /* (step record has no timing: on the step) */
    s->nt = (int8_t)nt;                                /* (a CHOP track: the key's chop, + 0..15) */
    t->key_nt = s->nt;
    ui.sr_key = (uint8_t)(key + 1u);
}
static void srec_keys(uint32_t notes)                  /* the frame's key edges: one key, the highest (the ISR's
                                                        * keyboard_block sounds that one of a frame's chord) */
{
    uint32_t k;
    if (ui.sr_key && !((fm1_in.notes >> (ui.sr_key - 1u)) & 1u))
        srec_next();                                   /* the cursor's key let go */
    for (k = 27u; k-- > 0u;)
        if ((notes >> k) & 1u) {
            srec_note(k);
            break;
        }
}
static void srec_rest(void)                            /* EDIT: the cursor step a plain rest (no PROB RATCH ACC SLIDE
                                                        * MICRO left for a later note), the cursor on */
{
    step_t *s = &TSEL->step[ui.sr_cur];
    s->on = 0;
    s->micro = 0;
    s->tie = 0;
    s->cond = 0;
    s->rat = 0;
    s->acc = 0;
    s->slide = 0;
    srec_next();
}
static void srec_knob(uint32_t k, int32_t s)
{
    track_t *t = TSEL;
    step_t *st;
    int32_t d = s > 0 ? 1 : -1, v, len = (int32_t)view_len(t);
    if (k == 0u) {
        ui.sr_cur = (uint8_t)clamp((int32_t)ui.sr_cur + accel(EN_K1, s, len), 0, len - 1);
        ui.bank = (uint8_t)(ui.sr_cur / 16u);
        ui.sr_key = 0;                                 /* a key still held moves nothing on its release */
        return;
    }
    st = &t->step[ui.sr_cur];
    if (k == 1u) {
        st->nt = (int8_t)clamp(st->nt + accel(EN_K2, s, NT_MAX - NT_MIN), NT_MIN, NT_MAX);
    } else if (k == 2u) {
        v = clamp((int32_t)srec_time(st) + d, 0, 2);
        st->on = v == 0;
        st->tie = v == 1;
    } else {
        v = clamp((int32_t)(st->acc | st->slide << 1) + d, 0, 3);
        st->acc = (uint8_t)(v & 1);
        st->slide = (uint8_t)(v >> 1);
    }
}

#include "ui_preset.c"                             /* PRESETS: the encoder's popup, the list on a long EDIT */

/* fixes 2026-10-21 item 1: OCT+ let go where it was pressed on the steps: alone (no step key accented with it), and the
 * keys still the steps, the next bank; anything else (an accent, another page, the menu, a dialog, TOOLS or
 * PERFORM up while it was held: acc_oct 2, ui_input) nothing */
/* OCT+'s page job (an LFO page: the RATE MODE of the LFO turned last) */
static void oct_up_job(void)
{
    if (!ui.home && cur_fam() == FAM_LFO) {      /* the LFO turned last (DEST: the slot's source): SYNC -> Hz
                                                         * -> TIME */
        uint32_t l = cur_page()->graph == GR_LDEST ? (TSEL->p[LS_ID[song.lslot & 3u][LS_SRC]] ? 1u : 0u)
                                                   : ui.lfo_l & 1u;
        int16_t *mp = &TSEL->p[(l ? P_LFO2 : P_LFO1) + LF_MODE];
        *mp = (int16_t)((clamp(*mp, 0, 2) + 1) % 3);
        ui.force = 1;
    }
}
static void acc_oct_poll(void)
{
    if (!acc_oct || ((fm1_in.buttons >> panel.btn[B_OCTUP]) & 1u))
        return;
    if (acc_oct == 1u && step_keys() && !ui.confirm && !name_on() && !pl_on() && !ui.layer && !fx_knobs) {
        if (seq_keys())
            bank_set((int32_t)ui.bank + 1);
        else
            oct_up_job();                               /* STEPS elsewhere: OCT+ let go alone is the page's */
    }
    acc_oct = 0;
}
/* a machine lock (Model:Cycles): MODEL turned with step st held, from model cur: the step's hit plays the next / previous
 * model (MODEL_LIST's order, ACID skipped: no lock's) with the track's knobs; back at the track's own model, no lock */
static void mlock_knob(uint32_t st, uint32_t cur, int32_t s)
{
    uint32_t own = (uint32_t)motion_rest(song.sel, P_MODEL) % NMODELS, m = model_list_next(cur % NMODELS, s);
    char b[24];
    if (m == DM_ACID)
        m = model_list_next(m, s);
    if (m == own) {
        uint32_t ids[MO_W] = {0};
        ids[P_MODEL >> 5] = 1u << (P_MODEL & 31u);
        lock_clear_step(song.sel, st, ids);
    } else {
        lock_set(song.sel, st, P_MODEL, (int32_t)m);
    }
    str_cpy(b, "MLOCK STEP ", sizeof b);
    fmt_int(b + 11, (int32_t)st + 1);
    str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), m == own ? "OFF" : N_MODEL[m], sizeof b - str_len(b));   /* (OFF: the track's own) */
    ui_message(b);
}
/* locks (spec 2026-10-24): a held step + a knob: its lock, from the knob's value there (the lock's, else the knob's own),
 * as a held step's edit (no tap on the release, an off step on); the knob, the base and motion untouched. A knob MOTION
 * cannot record does nothing while a step is held (the held step is its own context) */
static void lock_knob(uint32_t c, int32_t s)
{
    uint32_t id = lock_id(c), st = lock_step(), i;
    int16_t cur;
    if (id >= P_COUNT || st >= NSTEP)
        return;
    for (i = 0; i < 16u; i++)                         /* edited: no key down is a tap any more */
        if (ui.step_t0[i])
            ui.step_t0[i] |= 2u;
    step_hold_on(ui.held);
    if (!lock_get(song.sel, st, id, &cur))
        cur = motion_rest(song.sel, id);
    if (id == P_MODEL)
        mlock_knob(st, (uint32_t)cur, s);
    else
        lock_set(song.sel, st, id, knob_turn(EN_K1 + c, track_desc(TSEL, id), cur, s));
    hot_touch(c);
}
/* OCT- pressed with a step held (ui.ocd_t0): let go before LOCK_CLR_MS, that step's locks on this page's knobs go;
 * held LOCK_CLR_MS, all its locks at once (its release then nothing) */
static void lock_clear_say(const char *what)
{
    char m[24];
    str_cpy(m, "STEP ", sizeof m);
    fmt_int(m + 5, (int32_t)ui.ocd_si + 1);
    str_cpy(m + str_len(m), what, sizeof m - str_len(m));
    ui_message(m);
}
static void ocd_poll(uint32_t now)
{
    if (!ui.ocd_t0)
        return;
    if ((fm1_in.buttons >> panel.btn[B_OCTDN]) & 1u) {
        if (!(ui.ocd_t0 & 2u) && now - (ui.ocd_t0 & ~3u) >= LOCK_CLR_MS * 1000u * FM1_TICKS_PER_US) {
            lock_clear_step(song.sel, ui.ocd_si, 0);
            lock_clear_say(" ALL CLEARED");
            ui.ocd_t0 |= 2u;
        }
        return;
    }
    if (!(ui.ocd_t0 & 2u)) {
        uint32_t ids[MO_W] = {0}, c, id;
        for (c = 0; c < 4u; c++)
            if ((id = lock_id(c)) < P_COUNT)
                ids[id >> 5] |= 1u << (id & 31u);
        lock_clear_step(song.sel, ui.ocd_si, ids);
        lock_clear_say(" LOCKS CLEARED");
    }
    ui.ocd_t0 = 0;
}
/* ENV (spec 2026-10-24): a tap toggles STEPS / TRACKS on its release, unless OCT+- changed the bank while it was held
 * or it was pressed under the menu, NAME, the PRESETS list, TOOLS, PERFORM or a dialog */
static void env_poll(uint32_t now, uint32_t pressed)
{
    int busy = ui.menu || name_on() || pl_on() || ui.tools || ui.layer || fx_knobs || ui.confirm;
    if ((pressed >> panel.btn[B_ENV]) & 1u)
        ui.env_t0 = ((now | 1u) & ~2u) | (busy ? 2u : 0u);   /* (the clock's own bit 1 cleared: it is the flag) */
    if (ui.env_t0 && !((fm1_in.buttons >> panel.btn[B_ENV]) & 1u)) {
        if (!(ui.env_t0 & 2u) && !busy)
            env_toggle();
        ui.env_t0 = 0;
    }
}

/* KEYS on a SAMPLE track on PLAY CHOP: the chop the last key played (seq.c keys_on), "CHOP 5/12" (of the set's) */
static void keys_chop_say(void)
{
    static uint8_t said;
    uint32_t n = keys_chop_n, c = keys_chop;
    char b[16];
    if (n == said)
        return;
    said = (uint8_t)n;
    str_cpy(b, "CHOP ", sizeof b);
    fmt_int(b + 5, (int32_t)c);
    n = str_len(b);
    b[n] = '/';
    fmt_int(b + n + 1u, (int32_t)smp_slot_count((uint32_t)TSEL->p[P_E4]));
    ui_message(b);
}
static void ui_input(void)
{
    uint32_t pressed, notes, now, id, b, k, fam;
    smp_slots_sync();                                   /* dm_sample.c: a user slot changed (an upload): its lists,
                                                         * START states (the main loop's) */
    keys_chop_say();                                    /* KEYS on a CHOP track: CHOP 5/12 */
    pressed = fm1_input_edges(0);
    notes = fm1_input_note_edges();
    now = in_now = fm1_ticks();
    fam = cur_fam();
    uint32_t home = btn_hold(&ui.home_t0, B_HOME, now, 1);
    uint32_t rec = btn_hold(&ui.rec_t0, B_REC, now, !ui.menu && fam == FAM_SEQ), acts;
    int32_t s, dt;
    if (safe_start)                                     /* safe start: no pages, no edits */
        return;
    if (ui.splash) {                                    /* the boot splash: a key skips it, and only that */
        splash_input(pressed, notes);
        return;
    }
    acc_oct_poll();                                     /* OCT+ let go on the steps: the next bank (item 1) */
    env_poll(now, pressed);                             /* ENV: STEPS / TRACKS */
    pre_frame(now);                                     /* PRESETS: EDIT's hold, the popup's time (ui_preset.c) */
#if FELUCCA_BLE
    bt_autostart();                                     /* AUTOSTART: BLUETOOTH ON once, 5 s after power-on (the
                                                         * main loop's UI only: gate 3 CALLERS) */
#endif
    mod_input(now, notes);                              /* GLO, SEL, ARP: taps, the combos */
    if (mod_owns_keys())
        notes = 0;                                      /* (the combo's keys: no step, no select, no record) */
    cue_poll();                                         /* project.c: a cue's switch finished (spec 2026-10-26) */
    cue_input(now, notes);                              /* SAVE: tap / hold, the cue map */
    if (cue_owns_keys())
        notes = 0;                                      /* (the map's keys: no step, no select, no record) */
    tools_input(now, notes);                            /* SEQ: tap / hold, the TOOLS screen (ui_tools.c) */
    if (tools_owns_keys())
        notes = 0;                                      /* (the screen's keys: no step, no select, no record) */
    rec_fill(now);                                      /* REC held off SEQ: FILL */
    fx_layer(now, home == BT_TAP);                      /* PERFORM: FX tap / hold, the PAGE screen */
    if (acc_oct && (ui.tools || ui.layer || fx_knobs))
        acc_oct = 2;                                    /* OCT+ held under MUTE / PERFORM: no bank on its release, there
                                                         * or after (they own the panel: item 7) */
    ml_guard();                                         /* MIDI LEARN: off in the menu, NAME, another confirm */
    ml_poll();                                          /* .. a CC / note came for the picked one */
    if (ui.ml.on && !ui.confirm) {                      /* .. PLAY / REC pick their action, and do nothing else */
        if ((pressed >> panel.btn[B_PLAY]) & 1u) {
            pressed &= ~(1u << panel.btn[B_PLAY]);
            ml_pick_act(ML_A_PLAY);
        }
        if ((pressed >> panel.btn[B_REC]) & 1u) {
            ui.rec_t0 |= 2u;                            /* (no tap, no hold) */
            ml_pick_act(ML_A_REC);
        }
    }
    acts = ml_drain(&dt);                               /* .. learned buttons pressed: PLAY and REC as their taps,
                                                         * NEXT / PREV TRACK as ALGORITHM (each where the button acts) */
    if (acts & 1u)
        pressed |= 1u << panel.btn[B_PLAY];
    if ((acts & 2u) && rec == BT_NONE)
        rec = BT_TAP;
    if (ui.tools) {                                     /* the TOOLS screen owns the panel (its keys: tools_input):
                                                         * PLAY, OCT- / OCT+ the bank, the selection's knobs */
        tools_panel(pressed);
        return;
    }
    if ((fm1_in.buttons & perf_mask) || (perf_mask & PERF_PAGE))   /* PERFORM: keys are the layer's */
        notes = 0;
    notes &= ~kb_layer;
    if (name_on()) {                                  /* NAME owns the panel: OCT+ / OCT- / HOME / PLAY, the keys, K1 K2 */
        ui.srec = 0;                                    /* (step record ends) */
        name_input(pressed, notes, now, home);
        return;
    }
    if (pl_on()) {                                    /* the PRESETS list owns the panel (ui_preset.c) */
        ui.srec = 0;
        preset_list_input(pressed, notes, home);
        return;
    }
    if (home == BT_HOLD) {                              /* HOME held: open the menu, or leave it */
        if (ui.menu) {
            menu_close();
        } else {
            ui.menu = 1;
            ui.srec = 0;                                /* step record ends */
            ui.menu_sel = 0;
            ui.confirm = 0;
            ui.force = 1;
            song.seq_mode = 0;
        }
    }
    if (ui.menu) {
        if (ui.rec_t0)
            ui.rec_t0 |= 2u;
        if (!ui.home_t0)
            menu_input(pressed);
        return;
    }
    if (rec == BT_HOLD) {                               /* REC held on SEQ: "clear track n?" */
        ui.confirm = 1;
        ui.confirm_trk = song.sel;
        ui.force = 1;
    } else if (rec == BT_TAP && !ui.confirm) {          /* spec §4: REC arms on every screen */
        if (ui.srec) {
            ui.srec = 0;                                /* step record ends */
        } else if (srec_ok()) {
            srec_start();                               /* KEYS, STEP, stopped: step record */
        } else {
            rec_tap();                                  /* TRACKS: the selected track armed / disarmed; else REC ALL */
        }
    }
    if (ui.confirm) {                                   /* OCT- cancels (NO), OCT+ clears (YES); nothing else reacts */
        if ((pressed >> panel.btn[B_OCTUP]) & 1u) {
            char m[12] = "1 CLEARED";
            m[0] = (char)('1' + ui.confirm_trk);
            if (ui.confirm == CONFIRM_OVERWRITE) {   /* SAVE onto a slot with unsaved edits: saved over them */
                ui.confirm = 0;
                project_save_go(ui.confirm_trk & 3u);
            } else if (ui.confirm == CONFIRM_DISCARD) { /* LOAD: the slot's unsaved edits dropped, its saved one */
                ui.confirm = 0;
                if (!ui_locked()) {
                    project_discard(ui.confirm_trk & 3u);
                    proj_load_from(ui.confirm_trk & 3u);
                }
            } else if (ui.confirm == 5u) {              /* the last project failed to load: forget it (saved) */
                lastp.set = 0;
                settings_save();
            } else if (ui.confirm == ML_CONFIRM) {      /* MIDI LEARN: CLEAR ALL LEARNED? (LEARN stays on) */
                ml_clear_all();
                ui_message("LEARN CLEARED");
            } else if (ui.confirm == 2u) {              /* the MOTION page's CLEAR */
                motion_clear(ui.confirm_trk % NTRK);
                ui_message("MOTION CLEARED");
            } else {
                track_clear(&trk[ui.confirm_trk % NTRK]);
                ui_say("TRACK ", m);
            }
            ui.confirm = 0;
            ui.force = 1;
        } else if ((pressed >> panel.btn[B_OCTDN]) & 1u) {
            ui.confirm = 0;
            ui.force = 1;
        }
        enc_drop();
        return;
    }
    if (home == BT_TAP)
        home_tap();                                     /* HOME: TRACKS <-> HOME; elsewhere HOME as last shown */
    settings_poll();
    persist_note_show();                                /* USR3 SHORTENED TO 48K, LOCKS LOST (persist_boot) */
    song.act[2] = (int16_t)settings.mutebar;
    song.act[3] = (int16_t)fx_metro;
    song.act[4] = (int16_t)midi_thru;
    song.act[5] = (int16_t)rec_quant;
    song.act[6] = (int16_t)ui.ml.on;
    song.act[7] = (int16_t)mkb_kbd;
    song.act[8] = (int16_t)!mkb_local_off;
    bank_fix();                                         /* LEN may have changed (knob, load) */
    if (!ui.home && page_hidden(ui.page)) {             /* the engine changed: its last EDIT page */
        while (ui.page > page_first(FAM_SND) && page_hidden(ui.page))
            ui.page--;
        ui.fam_last[FAM_SND] = ui.page;
        page_entered();
    }
    for (id = 0; id < 14u; id++) {
        if (!((pressed >> id) & 1u))
            continue;
        b = panel_btn_of(id);
        if (b == B_EDIT && arp_edit()) {                /* ARP held: the ARP page (no step record rest, no PRESETS) */
            ui.pr.e_st = 2;
            continue;
        }
        if (ui.srec && b == B_EDIT) {                   /* step record: a rest, the cursor on */
            srec_rest();
            ui.pr.e_st = 2;                             /* (held: no PRESETS list) */
            continue;
        }
        if (b == B_EDIT)
            edit_down(now);                             /* PRESETS: held, the list (ui_preset.c edit_hold) */
        switch (b) {
        case B_PLAY:
            play_press();
            break;
        case B_REC:
        case B_HOME:                                    /* tap / hold: above */
            break;
        case B_OCTDN:
        case B_OCTUP:                                   /* the STEP grid's bank; an LFO page: OCT+ its RATE MODE */
            if (ui.ml.on) {                             /* MIDI LEARN: OCT- clear, OCT+ done (the page's wait) */
                ml_oct(b == B_OCTUP);
            } else if (keys_mode()) {
                keys_octave(b == B_OCTUP ? 1 : -1);
            } else if (ui.env_t0) {                     /* ENV held: the bank (STEPS' banks; no toggle on its release) */
                bank_set((int32_t)ui.bank + (b == B_OCTUP ? 1 : -1));
                ui.env_t0 |= 2u;
            } else if (b == B_OCTDN && lock_step() < NSTEP) {   /* a step held: OCT- clears its locks (ocd_poll), no
                                                                 * page job */
                uint32_t i;
                ui.ocd_t0 = (now | 1u) & ~2u;           /* (the clock's own bit 1 cleared: the flag) */
                ui.ocd_si = (uint8_t)lock_step();
                for (i = 0; i < 16u; i++)               /* (the keys down: no tap) */
                    if (ui.step_t0[i])
                        ui.step_t0[i] |= 2u;
            } else if (seq_keys()) {                    /* OCT+: held + a step key its accent, let go alone the
                                                         * next bank (acc_oct_poll); OCT-: the bank before */
                if (b == B_OCTUP)
                    acc_oct = 1;
                else
                    bank_set((int32_t)ui.bank - 1);
            } else if (env_steps()) {                   /* STEPS elsewhere: OCT+ held + a step key its accent, let go
                                                         * alone the page's job */
                if (b == B_OCTUP)
                    acc_oct = 1;
            } else if (b == B_OCTUP) {
                oct_up_job();
            }
            break;
        default: {
            uint32_t f;
            for (f = FAM_HOME + 1u; f < FAM_COUNT; f++)
                if (FAM_BTN[f] == b && b != B_FX && b != B_GLO && b != B_SAVE && b != B_SEQ)
                                                        /* (FX, GLO, SAVE, SEQ: on the release) */
                    open_family(f);
            break;
        }
        }
    }
    ocd_poll(now);                                      /* locks: OCT- with a step held */
    edit_hold(now);                                     /* EDIT held: the PRESETS list */
    if (pl_on())
        return;
    song.seq_mode = seq_mode_now();                   /* seq.c: whose the keys are (KEYS may have changed) */
    if (ui.srec && !srec_ok())
        ui.srec = 0;                                    /* KEYS off, another page, the transport started */
    if (ui.srec) {
        srec_keys(notes);
    } else if (step_keys()) {
        for (k = 0; k < 16u; k++) {                     /* steps on the white keys (seq.c STEP_KEY): a short press */
            uint32_t *t0 = &ui.step_t0[k];               /* toggles the step on its release, a hold edits it (owner
                                                         * 2026-10-23); with OCT+ held, its accent at the press */
            uint32_t held_long = *t0 && now - (*t0 & ~7u) >= STEP_HOLD_MS * 1000u * FM1_TICKS_PER_US;
            if ((notes >> STEP_KEY[k]) & 1u) {
                if (acc_oct) {
                    step_accent(k);
                    acc_oct = 2;
                    *t0 = now | 1u | 2u | 4u;           /* (as it is now: a knob turned while held edits it) */
                } else {
                    step_grab(k);
                    *t0 = (now | 1u) & ~6u;
                }
                ui.held = (uint8_t)k;
            } else if (*t0 && !((fm1_in.notes >> STEP_KEY[k]) & 1u)) {
                if (!(*t0 & 2u) && !held_long)
                    step_tap(k);                        /* a short press: the step on / off */
                *t0 = 0;
            } else if (held_long && !(*t0 & 2u)) {
                *t0 |= 2u;                              /* a hold: no tap; the STEP n PROB RATCH line; */
                step_hold_on(k);                        /* an off step on now */
            }
        }
    } else if (comp_mode()) {
        for (k = 0; k < NTRK; k++)                      /* COMP: a white key ducks / unducks its track */
            if ((notes >> KEY_TRK_KEY[k]) & 1u)
                track_duck_toggle(k);
    } else if (!keys_mode() && (ui.home || (!ui.menu && cur_page()->scope == SC_TRACK && cur_fam() != FAM_SEQ))) {
        for (k = 0; k < NTRK; k++)                      /* HOME, a per-track page: a white key selects its track */
            if ((notes >> KEY_TRK_KEY[k]) & 1u)         /* (it plays it too: seq.c reads the keys itself) */
                track_select(k);
    }
    if ((s = panel_enc(EN_PRESET)) != 0 && (fx_knobs || ui.layer)) {
        /* PERFORM up: no preset loaded under it (item 7; the turn dropped) */
    } else if (s) {                                     /* PRESETS: each detent the next / previous preset, loaded */
        for (; s; s += s > 0 ? -1 : 1)                  /* (ui_preset.c: the popup) */
            preset_popup_turn(s > 0 ? 1 : -1);
    }
    if ((s = panel_enc(EN_ALGO)) != 0 && (fx_knobs || ui.layer)) {
        /* PERFORM up: no track change under it (item 7; the turn dropped) */
    } else if (s) {
        if (ui.ml.on)
            ml_pick_act(s > 0 ? ML_A_NEXT : ML_A_PREV);   /* MIDI LEARN: NEXT / PREV TRACK picked, no track change */
        else
            track_select((uint32_t)clamp((int32_t)song.sel + (s > 0 ? 1 : -1), 0, NTRK - 1));
    }
    if (dt)                                             /* NEXT / PREV TRACK learned (MIDI LEARN) */
        track_select((uint32_t)clamp((int32_t)song.sel + dt, 0, NTRK - 1));
    if ((s = panel_enc(EN_SELECT)) != 0 && (fx_knobs || ui.layer)) {
        /* PERFORM up: SELECT turns no page under it (item 7; the turn dropped) */
    } else if (s) {                                     /* SELECT: the section's pages; HOME's TRACKS mode: BPM */
        if (!mix_mode())
            page_turn(s > 0 ? 1 : -1);
        else if (song.g[G_CLOCK])                       /* following a clock: its tempo, not the knob */
            ui_say("CLK ", N_CLOCK[song.g[G_CLOCK]]);
        else {
            song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + accel(EN_SELECT, s, 200), GP[G_BPM].min, GP[G_BPM].max);
            ui.bpm_t = 40;
        }
    }
    for (k = 0; k < 4u && !fx_knobs; k++) {            /* (PERFORM's: fx_layer's alone, item 7) */
        const page_t *pg = cur_page();
        int16_t *hv;
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        if (ui.srec) {                                  /* step record: STEP NOTE TIME FLAG */
            srec_knob(k, s);
            hot_touch(k);
            continue;
        }
        if (step_edit_page() && step_edit(k, s))
            continue;
        if (lock_step() < NSTEP) {                      /* STEPS, a step held: a lock (or nothing) */
            lock_knob(k, s);
            continue;
        }
        if (ui.home || pg->scope == SC_GRID || pg->scope == SC_MIX || pg->graph == GR_LDEST ||
            page_desc(pg, k, &hv))
            hot_touch(k);
        if (ui.home) {
            int16_t *vp;
            const param_desc_t *d = home_param(k, &vp);
            if (d->max > d->min) {
                int32_t v = knob_turn(EN_K1 + k, d, *vp, s);
                if (v != *vp)
                    preset_touch(TSEL);                 /* (TUNE DECAY TONE CHAR: the voice edited; not at a limit) */
                *vp = (int16_t)v;
                if (vp >= TSEL->p && vp < TSEL->p + P_COUNT) {
                    motion_capture(TSEL, (uint32_t)(vp - TSEL->p));
                    ml_knob(song.sel, (uint32_t)(vp - TSEL->p));   /* (MIDI LEARN: picked) */
                }
            }
        } else {
            edit_param(k, s);
        }
    }
    if (mo.full) {                                      /* motion.c: a turn found no free place */
        mo.full = 0;
        ui_message("MOTION FULL");
    }
    if (mo.lfull) {                                     /* motion.c: a lock found the bank full */
        mo.lfull = 0;
        ui_message("LOCKS FULL");
    }
}

/* ---------------------------------------------------- panel setup --- */
/* 30 s without input: give up and keep the old table (a stuck key cannot hang the boot) */
#define SETUP_IDLE_MS 30000u
static void panel_setup(void)
{
    uint32_t i, used = 0, t0 = fm1_ms;
    const panel_t old = panel;
    lcd_fill(0, 0, 240, 240, C_BG);
    draw_text_box(0, 10, 240, &FONT_S, "HARDWARE CALIBRATION", C_INK, 1);
    draw_text_box(0, 30, 240, &FONT_S, "TEACH EACH BUTTON AND KNOB", C_INK, 1);
    while (fm1_in.buttons) {                             /* wait for OCT-/OCT+ release */
        fm1_wdt_feed();
        if (fm1_ms - t0 > SETUP_IDLE_MS)
            goto timeout;
    }
    fm1_input_edges(0);
    for (i = 0; i < NB; i++) {
        uint32_t p = 0, id;
        draw_text_box(0, 80, 240, &FONT_S, "PRESS", C_INK, 1);
        draw_text_box(0, 100, 240, &FONT_L, B_NAME[i], C_INK, 1);
        t0 = fm1_ms;
        while (!(p & ~used)) {
            fm1_wdt_feed();
            p |= fm1_input_edges(0);
            if (fm1_ms - t0 > SETUP_IDLE_MS)
                goto timeout;
        }
        for (id = 0; id < 14u; id++)
            if (((p & ~used) >> id) & 1u)
                break;
        panel.btn[i] = (uint8_t)id;
        used |= 1u << id;
    }
    used = 0;
    for (i = 0; i < NE; i++) {
        uint32_t e;
        int32_t st = 0;
        draw_text_box(0, 80, 240, &FONT_S, "TURN RIGHT", C_INK, 1);
        draw_text_box(0, 100, 240, &FONT_L, E_NAME[i], C_INK, 1);
        for (e = 0; e < 7u; e++)
            enc_take(e);
        t0 = fm1_ms;
        for (;;) {
            fm1_wdt_feed();
            if (fm1_ms - t0 > SETUP_IDLE_MS)
                goto timeout;
            for (e = 0; e < 7u; e++)
                if (!((used >> e) & 1u) && (st = enc_take(e)) != 0)
                    break;
            if (e < 7u)
                break;
        }
        panel.enc[i] = (uint8_t)e;
        panel.dir[i] = (int8_t)(st > 0 ? 1 : -1);
        used |= 1u << e;
        fm1_delay_ms(300);
        enc_take(e);
    }
    panel.magic = PANEL_MAGIC;
    lcd_fill(0, 0, 240, 240, C_BG);
    ui.force = 1;
    return;
timeout:
    panel = old;
    lcd_fill(0, 0, 240, 240, C_BG);
    ui.force = 1;
    ui_message("SETUP CANCELLED");
}

