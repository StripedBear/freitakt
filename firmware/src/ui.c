/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Drum machine fork: 2026 DEADACTIVE */
/* Drum machine user interface: state, page navigation, track / model selection, the STEP grid.
 * Four columns map to KNOB 1..4; rendering (ui_draw.c) is lazy: every element remembers what it
 * last drew and is redrawn only on change. */
#ifndef FELUCCA_VERSION
#define FELUCCA_VERSION "DRUM-DEV"         /* host tests; builds pass DRUM-<VERSION.txt> (tools/build.py) */
#endif
static void project_save(uint32_t slot);
static void project_load(uint32_t slot);
static void project_boot(void);
static int project_used(uint32_t slot);
static int project_name(uint32_t slot, char *b);
static void project_rename(uint32_t slot, const char *name);
static int slot_dirty(uint32_t s);         /* project.c: slot s has unsaved edits (spec 2026-10-26) */
static void project_discard(uint32_t s);   /* .. its RAM image back to its flash version */
static void proj_load_from(uint32_t slot); /* .. LOAD without the DISCARD EDITS? question */
static void project_save_go(uint32_t slot);   /* .. SAVE without the OVERWRITE question */
static void cue_map_refresh(void);         /* .. the cue map's per-slot state, once a frame */
static void cue_poll(void);                /* .. a cue's switch finished (the ISR's: cue.c) */
static int cue_slot(uint32_t s);           /* .. queue slot s / variation v (SAVE + a white key / a black key) */
static int cue_var(uint32_t v);
static int chain_set(const uint8_t *slots, uint32_t n);   /* .. the pattern chain (GLO + 2 or more white keys) */
static uint8_t chain_ent(uint32_t s);      /* .. a chain entry: pattern s, its default variation */
static int cue_led(uint32_t n);            /* .. the cue map's key n (1..4 slots, 5..12 variations): CM_* */
static int32_t proj_loaded(void);          /* .. the slot loaded, -1 none */
static void project_copy(uint32_t s);      /* .. COPY / PASTE of a slot (the COPY page) */
static void project_paste(uint32_t t);
static uint8_t copy_src;                   /* .. the slot COPY took + 1, 0 none (SAVE's COPY shows it) */
enum { CM_OFF, CM_ON, CM_WINK, CM_BLINK, CM_BREATHE };   /* a cue map key: dark, lit, winking, blinking, breathing */
static int transport_busy(void);
static int name_on(void);                  /* ui_name.c (NAME) */
static void draw_name(void);
static void panel_setup(void);
static void learn_save(void);              /* project.c: MIDI LEARN's map to flash (deferred while playing) */
static void ml_line(char *b);              /* ui_learn.c: MIDI LEARN's footer line */
static int pp_on(void);                    /* ui_preset.c: the PRESETS popup shows */
static int pl_on(void);                    /* .. the PRESETS list shows */
static void draw_popup(void);              /* .. the popup: its own screen */
static void draw_plist(void);
static int splash_draw(void);             /* ui_menu.c: the FREITAKT boot splash (1: a frame of it drawn) */
static int preset_user_save(uint32_t slot, const track_t *t);   /* project.c: the user presets' flash record */
static void preset_user_delete(uint32_t slot);

#define CONFIRM_DISCARD 7u             /* ui.confirm: DISCARD EDITS? (LOAD of a slot with unsaved edits) */
#define CONFIRM_OVERWRITE 8u           /* .. OVERWRITE B*? (SAVE onto a slot, not the loaded one, with unsaved edits) */
enum { CV_NORM, CV_HOT, CV_DIM };              /* a knob cell's value: plain, touched (inverted), inactive (dithered) */
#define HOT(c) ((c) == ui.hot_col && ui.hot_t ? CV_HOT : CV_NORM)
#define RATIO(d, v) ((d)->max > (d)->min ? (enum_rank((d), (int32_t)(v)) - (d)->min) * 1000 / ((d)->max - (d)->min) : -1)
/* layout A (spec 2026-10-19-md-ui §5), what is left of it since FREITAKT's LIST pages: top bar 0..15, the old knob
 * row 17..59 (PERFORM's columns), the graph band 61..180 (PERFORM's map, the PRESETS popup) */
#define Y_HEAD 0
#define H_HEAD 16
#define Y_KNOB 17
#define H_KNOB 43
#define Y_GRAPH 61
#define H_GRAPH 120

static struct {
    uint8_t home;                /* HOME's second mode shown: the HOME screen (spec §3) */
    uint8_t hmode;               /* HOME's mode last shown: 0 TRACKS (the TRACKS page), 1 HOME (the HOME screen, COMP) */
    uint8_t page;                /* index into PAGES */
    uint8_t fam_last[FAM_COUNT]; /* last page used per family */
    uint8_t bank;                /* STEP grid: 16-step bank shown on the keys */
    uint8_t hot_col, hot_t;      /* column whose knob was just turned (its value inverted): hot_t on */
    uint32_t hot_t0;             /* .. the last turn (fm1_ticks: HOT_MS after it, off) */
    uint8_t menu;                /* 0 off, 1 the MENU's carousel (HOME held), 3 Bluetooth (BLE build: bt_page.c) */
    uint8_t splash;              /* the boot splash (ui_menu.c): 0 off, 1 to start, 2 running, 3 skipped (the keys
                                  * let go first) */
    uint8_t sp_sub, sp_bar;      /* .. the subtitle's glyphs and cursor shown, the bar's width shown */
    uint32_t sp_t0;              /* .. its start (fm1_ms) */
    uint8_t menu_sel;            /* the MENU's item shown (ui_menu.c) */
    uint8_t tool;                /* SAVE's TOOLS: the action shown (0..3: KNOB 1..4's; ui_menu.c draw_save) */
    uint32_t home_t0;
    uint8_t force;               /* full redraw pending */
    uint8_t msg_t;               /* transient message frames */
    uint8_t bpm_t;
    uint8_t arm, arm_t;          /* destructive action armed: param id, frames left to confirm */
    uint32_t rec_t0;             /* REC press time (btn_hold) */
    uint8_t rec_all, rec_keep;   /* REC ALL (a REC tap off TRACKS, ui_input.c rec_tap): on; the arm mask before it (back
                                  * when it is left). Never stored, as the arm state */
    uint32_t step_t0[16];        /* STEP grid: a white key's press time | 1; bit 1: no tap on its release (held
                                  * STEP_HOLD_MS, edited, accented or let go of: grid_drop_holds), bit 2: edited */
    uint16_t step_si[16];        /* the step it pressed, 0xFFFF none */
    uint8_t held;                /* the grid key pressed last (valid while step_t0[held] is set) */
    uint16_t step_lit;           /* the grid keys whose hold turned an off step on (nothing edited since): a copy /
                                  * paste combo (ui_input.c step_combo) puts it back as it was, step_was */
    step_t step_was[16];
    uint8_t confirm;             /* (1: the removed CLEAR TRACK n?, now REC + SEL held; 2: the removed MOTION page's, 3, 4: the removed SONG's), 5 the last project
                                  * failed to load (START NEW PROJECT?; confirm_trk its slot), 6 CLEAR ALL LEARNED?
                                  * (MIDI LEARN: ui_learn.c), 7 DISCARD EDITS? (a LOAD of slot confirm_trk with unsaved
                                  * edits: CONFIRM_DISCARD), 8 OVERWRITE B*? (a SAVE onto it: CONFIRM_OVERWRITE) */
    uint8_t confirm_trk;
    char msg[24];
    uint32_t enc_t[NE];
    uint8_t enc_c[NE];           /* knob acceleration: the turn's carried sixteenths of a step (ui_input.c accel) */
    char col[4][32];
    uint32_t graph_sig, head_sig, msg_sig, tempo_sig, col_sig, frame;
    uint8_t lfo_l;               /* LFO pages: the LFO whose knob was turned last (OCT+ cycles its RATE MODE) */
    uint8_t persist_pending;     /* a settings save asked for while playing: written once stopped (project.c) */
    uint32_t fx_t0;              /* PERFORM: FX's press time | FX_DOWN / FX_OPEN / FX_DEAD (ui_input.c fx_layer) */
    uint8_t layer;               /* PERFORM: the layer's map shows (FX held open, or a layer key still held) */
    uint8_t pg_open, pg_page, pg_home;   /* PERFORM PAGE: the screen open; the page / HOME under it when it opened */
    uint32_t glo_t0, sel_t0, arp_t0;   /* GLO's, SEL's, ARP's press time | MOD_DOWN KEY DEAD PLAIN (ui_input.c
                                        * mod_input: the combos) */
    uint32_t cue_t0;             /* SAVE's press time | CUE_DOWN OPEN DEAD PLAIN (ui_input.c cue_input) */
    uint8_t cue_map;             /* SAVE held: the cue map (white keys the slots, black keys the variations, 2026-10-26) */
    uint32_t seq_t0;             /* SEQ's press time | SQ_DOWN DEAD PLAIN KEY (ui_tools.c seq_input) */
    uint32_t clr_t0;             /* REC + SEL held: the time both were | 1; bit 1: done or dead (ui_input.c clr_input) */
    uint8_t clr, clr_trk;        /* .. the countdown's digit shown (5..1, 0: none), the track it clears */
    uint8_t keys;                /* KEYS (spec §5): an SCL tap; ON at power-on (draw_boot_title), never stored */
    uint8_t env;                 /* ENV (spec 2026-10-24 parameter locks): 1 STEPS (the white keys the selected track's
                                  * steps on every page), 0 TRACKS (as before); off at power-on, never stored */
    uint32_t env_t0;             /* ENV's press time | 1; bit 1: used (ENV + OCT+-, or pressed under a dialog): its
                                  * release toggles nothing */
    uint32_t ocd_t0;             /* locks: OCT- pressed with a step held: its time | 1; bit 1: the 0.6 s clear done */
    uint8_t ocd_si;              /* .. the step held then */
    struct {                     /* MIDI LEARN (ui_learn.c; GLO + G4): never stored */
        uint8_t on;              /* LEARN is on */
        uint8_t pick, trk, id;   /* something picked: track trk's P_* id (a CC is awaited: midi_learn.c ml_arm) .. */
        uint8_t act;             /* .. or, not 0, an action (midi_learn.c ML_A_*: a CC or a note awaited) */
        uint8_t save;            /* the map changed while playing: saved once stopped (project.c learn_save) */
    } ml;
    struct {                     /* PRESETS (ui_preset.c): never stored */
        uint8_t pop, pop_trk;    /* the popup shows, for that track */
        uint8_t lst;             /* the list shows */
        uint8_t arm;             /* the list: 1 SAVE over / 2 DELETE armed, on arm_id (the button again acts) */
        uint8_t e_st;            /* EDIT: 1 pressed (may become a hold), 2 the hold acted / dead until let go */
        uint8_t e_page, e_home, e_hmode, e_snd;   /* the page EDIT's press left (the hold puts it back) */
        uint16_t pop_id, cur, arm_id;   /* the popup's preset; the list's cursor */
        uint16_t shown[7];       /* the popup's rows, top to bottom (the preset in the middle) */
        uint32_t rsig[8];        /* .. what its bands show (the bank's, the 7 rows; 0: to draw) */
        uint32_t now, pop_t0, arm_t0, e_t0, sig;
        uint8_t pv, pv_trk;      /* the list: track pv_trk auditions the cursor's preset (its own sound in sv_*) */
        uint8_t sv_model, sv_ed; /* .. that sound: MODEL, the 8 SOUND knobs (motion's base while it plays), the preset */
        uint16_t sv_pre;         /*    and its edited mark, put back unless LOAD keeps the audition */
        int16_t sv_k[14];        /* (P_E0..P_E7, then ATK SUS REL LOOP PLAY CUT: presets.c preset_put) */
        int8_t sv_f[UPR_NFB];    /* .. its FX (presets.c UPR_FX, as a slot's bytes: upr_fb), put back when pv_fx: an
                                  *    audition changed them */
        uint8_t pv_fx;
    } pr;
} ui;
/* a knob turned on column c: its value inverted (a cell, a LIST row) until HOT_MS after the last turn (real time, not
 * frames: ui_draw) */
#define HOT_MS 3000u
#define HOT_TICKS (HOT_MS * 1000u * FM1_TICKS_PER_US)
static void hot_touch(uint32_t c)
{
    ui.hot_col = (uint8_t)c;
    ui.hot_t = 1;
    ui.hot_t0 = ui.pr.now;
}
/* the knob popup (FREITAKT redesign, rule 5; it replaces ZOOM): a parameter knob turned on a LIST page, TRACKS (LEVEL
 * LEN PAN) or HOME (BPM SWING): the screen cleared to the parameter's name and value, large (ui_draw.c draw_kpop),
 * until HOT_MS after the last turn; then the screen under it whole, once. A session is one knob's turns on one thing
 * (a parameter, MODEL, a held step's lock or its PROB RATCH ACC NUDGE): what it was before the first turn is kept here
 * (kp_snap), and OCT- while the popup is up puts it back (kp_undo, ui_input.c) instead of its own job */
enum { KP_NONE, KP_PAR, KP_MODEL, KP_LOCK, KP_STEP, KP_NOTE };
static struct {
    uint8_t on;                  /* 0 off, 1 up (the screen still to clear), 2 up and drawn */
    uint8_t kind, col;           /* KP_*; the knob */
    uint8_t trk, st;             /* the track; KP_LOCK KP_STEP: the step */
    uint8_t had;                 /* KP_LOCK: the step locked the knob before (at v) */
    uint8_t ed, mute;            /* KP_PAR (a track's), KP_MODEL: the preset's edited mark, MUTE before */
    uint8_t model, undone;       /* KP_MODEL: the model before; OCT- done (again: nothing, until another turn) */
    uint16_t id, gid;            /* KP_PAR: the P_* id (P_COUNT none: a global, MLEN); KP_LOCK: the P_*; KP_STEP: the
                                  * field (0 PROB, 1 RATCH, 2 ACC, 3 NUDGE); KP_NOTE: the held keys (bit k: key k, its
                                  * step nsi[k]; 0: the track's NOTE, st 0); gid: a global's G_* (0xFFFF none) */
    uint16_t pre;                /* KP_MODEL: the preset before */
    int16_t v, v2;               /* the value before (KP_LOCK: the lock's); P_E4 (a SAMPLE's SET): its SLOT before */
    int16_t *vp;                 /* KP_PAR: the value */
    const param_desc_t *d;       /* KP_PAR KP_LOCK: its description (the name, the format) */
    step_t step;                 /* KP_STEP KP_LOCK: the step before */
    step_t steps[16];            /* KP_NOTE: the held steps before (KP_NOTE's v: the track's NOTE before; col 4:
                                  * ALGORITHM's) */
    uint8_t nsi[16];             /* KP_NOTE: their step numbers */
    int16_t k[8u + SENV_N];      /* KP_MODEL: the SOUND knobs before (as the PRESETS audition keeps them) */
    uint32_t t0;                 /* the last turn (or OCT-), as hot_t0 */
    uint32_t nsig, vsig, msig;   /* the name's, the value's and the message's rows shown */
} kp;
static void kp_close(void)                           /* the popup gone: the screen under it whole */
{
    if (kp.on)
        ui.force = 1;
    kp.on = 0;
}

static const page_t *cur_page(void) { return &PAGES[ui.page]; }

static uint32_t page_first(uint32_t fam)
{
    uint32_t i;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == fam)
            return i;
    return 0;
}

static void ui_say(const char *a, const char *b)     /* transient message in the top bar: a + b */
{
    uint32_t n;
    str_cpy(ui.msg, a, sizeof ui.msg);
    n = str_len(ui.msg);
    str_cpy(ui.msg + n, b, sizeof ui.msg - n);
    ui.msg_t = 40;
}

static void ui_message(const char *s) { ui_say(s, ""); }

/* the web editor restores a project or the user presets (editor_backup.c ed_restoring: from BEGIN to COMMIT / ABORT /
 * the idle timeout / update traffic): the device's own PLAY, cue keys, LOAD, SAVE, PASTE, NAME and preset save /
 * delete are refused meanwhile, as other refusals are (a message), so nothing touches the RAM homes the restore stages
 * in. ED_RESTORE_LOCK (felucca.c, the editor's host tests): the editor's restore is built in; else never locked */
#ifdef ED_RESTORE_LOCK
static int ed_restoring(void);                      /* editor_backup.c (edb.on: no copy of it here) */
#define UI_RESTORING() ed_restoring()
#else
#define UI_RESTORING() 0
#endif
static int ui_locked(void)
{
    if (!UI_RESTORING())
        return 0;
    ui_message("RESTORING");
    return 1;
}


static int mix_mode(void) { return !ui.home && !ui.menu && cur_page()->scope == SC_MIX; }   /* HOME's TRACKS mode */
static int comp_mode(void) { return !ui.home && !ui.menu && cur_page()->graph == GR_COMP; }   /* COMP: keys DUCK */
/* KEYS (spec §5): the keys a chromatic keyboard of the selected track on every page, TRACKS and COMP too (FREITAKT,
 * owner: SEL lit = KEYS, always); NAME and the menu keep theirs, and FX held takes its keys first (seq.c
 * keyboard_block) */
static int keys_mode(void)
{
    return ui.keys && !ui.menu && !name_on();
}
/* the white keys are the selected track's steps (the bank's 16): every SEQ page while KEYS is off, and TRACKS while
 * nothing is armed (user, 2026-10-08) */
static int seq_keys(void)
{
    return !ui.home && !ui.menu &&
           !keys_mode() &&
           (cur_page()->fam == FAM_SEQ || (cur_page()->scope == SC_MIX && !song.rec));
}
/* ENV's STEPS mode (spec 2026-10-24 parameter locks): the white keys the selected track's steps on every page (HOME,
 * TRACKS armed, SOUND, FX, LFO, COMP ...); not in the menu or NAME */
static int env_steps(void) { return ui.env && !ui.menu && !name_on(); }
static int step_keys(void) { return seq_keys() || env_steps(); }
/* seq.c keyboard_block: whose the keys' presses are: 0 the drum keys (hits, MIDI), 1 the UI's (the STEP grid, COMP,
 * TRACKS, NAME), 2 TRACKS armed (hits and recording, as 0), 3 KEYS */
static uint8_t seq_mode_now(void)
{
    if (pl_on())                                       /* the PRESETS list: the keys play (and pick the track) */
        return 0u;
    if (keys_mode())
        return 3u;
    return (uint8_t)(step_keys() || comp_mode() ? 1u : mix_mode() ? (song.rec ? 2u : 1u) : 0u);
}

/* a held grid key lets go of its step (another track or page): keys still held edit nothing */
static void grid_drop_holds(void)
{
    uint32_t k;
    for (k = 0; k < 16u; k++) {
        ui.step_si[k] = 0xFFFFu;
        if (ui.step_t0[k])
            ui.step_t0[k] |= 6u;
    }
}

static void page_entered(void)
{
    grid_drop_holds();
    song.seq_mode = seq_mode_now();                    /* seq.c: whose the keys are */
    ui.hot_t = 0;
    kp.on = 0;                                         /* (the knob popup gone) */
    ui.force = 1;
}

/* SCL: KEYS on / off (a key still held: its release still ends its note, seq.c keyboard_block) */
static void keys_toggle(void)
{
    ui.keys ^= 1u;
    if (ui.keys)
        ui.env = 0;                                    /* one key role at a time: STEPS off */
    grid_drop_holds();                                 /* a grid key held edits nothing more */
    song.seq_mode = seq_mode_now();
    ui.force = 1;
}
/* ENV (its release): STEPS <-> TRACKS (KEYS off for STEPS: one key role at a time); a held key lets go of its step */
static void env_toggle(void)
{
    ui.env ^= 1u;
    if (ui.env)
        ui.keys = 0;
    grid_drop_holds();
    song.seq_mode = seq_mode_now();
    /* (no full repaint: the top bar's padlock, its KEYS line and the strip's marks are in their signatures) */
}
/* the MIDI KEYBOARD (MIDI OUT 4/8 KBD not OFF): KEYS has the keys and they send MIDI notes (midi_out.c mkb_keys) */
static int kbd_mode(void) { return mkb_kbd && keys_mode(); }
/* OCT- / OCT+ while KEYS has the keys: the keyboard an octave down / up, -2..+2 (seq.c keys_nt); as the MIDI
 * KEYBOARD -4..+4 (notes 5..127) */
static void keys_octave(int32_t d)
{
    int32_t r = mkb_kbd ? 4 : 2;
    song.octave = (int8_t)clamp(song.octave + d, -r, r);
}
/* a note (MIDI, 60 = C4) by name: "C4", "F#3", "C-1" */
static void note_text(char *b, int32_t n)
{
    n = clamp(n, 0, 127);
    str_cpy(b, N_NOTE[n % 12], 4);
    fmt_int(b + str_len(b), n / 12 - 1);
}

/* a page with nothing to edit for the selected track: an EDIT page past the end of its engine's sound list */
static int page_hidden(uint32_t i)
{
    uint8_t ids[SND_N];
    if (i < NPAGES && PAGES[i].id[0] == P_ADIR)        /* ARP's DIR CHORD RANGE: a pitched model's only */
        return !arp_model((uint32_t)TSEL->p[P_MODEL] % NMODELS);
    if (i >= NPAGES || PAGES[i].fam != FAM_SND || PAGES[i].id[0] < SND_SLOT)
        return 0;
    return PAGES[i].id[0] - SND_SLOT >= snd_list(TSEL, ids);
}

/* the pages a family shows (hidden ones left out), and where the current page is among them (1-based, 0 = not) */
static uint32_t fam_pages(uint32_t fam, uint32_t *pos)
{
    uint32_t i, n = 0;
    *pos = 0;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == fam && !page_hidden(i)) {
            n++;
            if (i == ui.page)
                *pos = n;
        }
    return n;
}

static void open_family(uint32_t fam)
{
    if (!ui.home && cur_page()->fam == fam) {          /* same button again: next page */
        uint32_t i = ui.page + 1u;
        while (i < NPAGES && PAGES[i].fam == fam && page_hidden(i))
            i++;
        if (i >= NPAGES || PAGES[i].fam != fam)
            i = page_first(fam);
        ui.page = (uint8_t)i;
    } else {
        ui.page = ui.fam_last[fam] && PAGES[ui.fam_last[fam]].fam == fam && !page_hidden(ui.fam_last[fam])
                      ? ui.fam_last[fam]
                      : (uint8_t)page_first(fam);
    }
    ui.fam_last[fam] = ui.page;
    ui.home = 0;
    page_entered();
}

/* HOME (spec §3): two modes, 1 TRACKS (the TRACKS page: the mixer, TRACK LEVEL LEN PAN, the keys as on TRACKS) and 2
 * HOME (the HOME screen: the status line, the BPM; KNOB 1 BPM, KNOB 2 SWING). ui.hmode: the one last shown */
static void home_screen(void)                         /* HOME's second mode */
{
    ui.home = 1;
    ui.hmode = 1;
    ui.hot_t = 0;
    kp.on = 0;
    song.seq_mode = seq_mode_now();
    ui.force = 1;
}

static void home_tracks(void)                         /* HOME's TRACKS mode: the TRACKS page (FAM_MIX: HOME's LED) */
{
    ui.page = (uint8_t)page_first(FAM_MIX);
    ui.home = 0;
    ui.hmode = 0;
    page_entered();
}

static void go_home(void)                             /* HOME in the mode last shown */
{
    if (ui.hmode)
        home_screen();
    else
        home_tracks();
}

static void home_tap(void)                            /* on HOME: TRACKS <-> HOME (COMP: HOME); elsewhere: HOME */
{
    if (ui.home)
        home_tracks();
    else if (cur_page()->fam == FAM_MIX || cur_page()->fam == FAM_HOME)
        home_screen();
    else
        go_home();
}

/* SELECT: the current section's next / previous page (its empty pages skipped), stopping at the ends; HOME: nothing.
 * (HOME's TRACKS mode: BPM, ui_input.c) */
static void page_turn(int32_t dir)
{
    uint32_t fam, i;
    if (ui.home)                                      /* HOME: no pages of its own (COMP is FX's now) */
        return;
    fam = cur_page()->fam;
    i = ui.page;
    do
        i = dir > 0 ? i + 1u : i - 1u;                /* (below 0: wraps above NPAGES, out of the section) */
    while (i < NPAGES && PAGES[i].fam == fam && page_hidden(i));
    if (i < NPAGES && PAGES[i].fam == fam) {
        ui.page = (uint8_t)i;
        ui.fam_last[fam] = ui.page;
        page_entered();
    }
}

/* --------------------------------------------- what a track shows --- */
/* the STEP grid, PATTERN, the footer, TRACKS and the keys show a track's steps */
static uint32_t view_len(const track_t *t) { return (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP); }
static step_t view_step(const track_t *t, uint32_t si) { return t->step[si % NSTEP]; }
static uint32_t view_idx(const track_t *t) { return t->seq_idx; }

/* ------------------------------------------------------ STEP grid --- */
static uint32_t bank_count(void) { return (view_len(TSEL) + 15u) / 16u; }
static void bank_set(int32_t b) { ui.bank = (uint8_t)clamp(b, 0, (int32_t)bank_count() - 1); }
static void bank_fix(void)                             /* LEN shortened (knob, project): onto the last bank */
{
    if (ui.bank >= bank_count())
        bank_set((int32_t)bank_count() - 1);
}
/* the shown bank's locked steps (bit j: step ui.bank * 16 + j has a parameter or machine lock, lockbank.c
 * lk_step_mask) for the step LEDs (ui_input.c ui_leds: they blink). Kept, and found again only when the bank's locks
 * may have changed (lk_gen; its count: a bank cleared in place) or another track / bank / variation is shown (a
 * variation swaps mo.b: lk_gen), not each frame */
static struct {
    uint32_t gen;
    uint16_t mask, count;
    uint8_t trk, bank, ok;
} lkv;
static uint32_t lk_view_mask(void)
{
    if (!lkv.ok || lkv.gen != lk_gen || lkv.count != mo.b.count || lkv.trk != song.sel || lkv.bank != ui.bank) {
        lkv.mask = (uint16_t)lk_step_mask(&mo.b, song.sel, ui.bank * 16u);
        lkv.gen = lk_gen;
        lkv.count = mo.b.count;
        lkv.trk = (uint8_t)song.sel;
        lkv.bank = ui.bank;
        lkv.ok = 1;
    }
    return lkv.mask;
}

/* the NOTE of a step the grid keys turn on (spec §2): the track's NOTE (key_nt: SOUND 1's NOTE / ALGORITHM with no
 * step held, else the last KEYS / recorded note), every machine's (a drum plays it transposed from C4) */
static int8_t new_step_nt(const track_t *t) { return (int8_t)clamp(t->key_nt, NT_MIN, NT_MAX); }

/* NOTE (SOUND 1's KNOB 2; ALGORITHM with KEYS off): the step keys down now whose steps a turn writes (bit k: key k,
 * its step ui.step_si[k]): where the keys are the steps (the grid, STEPS), every grid key down inside LEN; 0 none:
 * the turn sets the track's NOTE (key_nt) */
static uint32_t note_keys(void)
{
    uint32_t k, m = 0;
    if (!step_keys())
        return 0;
    for (k = 0; k < 16u; k++)
        if (ui.step_t0[k] && ((fm1_in.notes >> STEP_KEY[k]) & 1u) && ui.step_si[k] < (uint32_t)TSEL->p[P_SLEN])
            m |= 1u << k;
    return m;
}
/* .. the key whose step NOTE shows (the last pressed if it is down, else the lowest); 16 none */
static uint32_t note_key(uint32_t m)
{
    uint32_t k;
    if (ui.held < 16u && ((m >> ui.held) & 1u))
        return ui.held;
    for (k = 0; k < 16u && !((m >> k) & 1u); k++)
        ;
    return k;
}

/* a step key down this long is a hold, not a tap (owner 2026-10-23: holding a step is for editing it, never toggles
 * it). 0.3 s: a deliberate tap on these keys is well under it (~0.1 s down), a hold to edit is felt as one past it,
 * and the STEP n PROB RATCH line comes up without a wait; under GLO / FX's 0.4 s and HOME / REC's 0.7 s holds */
#define STEP_HOLD_MS 300u

/* the step a grid key holds for editing: held STEP_HOLD_MS, or edited / accented while down (step_t0 bit 1, its
 * page and track unchanged); NSTEP none. A tap shows nothing (its line would only flash) */
static uint32_t held_step(void)
{
    uint32_t k = ui.held;
    return k < 16u && (ui.step_t0[k] & 2u) && ui.step_si[k] < NSTEP ? ui.step_si[k] : NSTEP;
}

/* key k of the grid pressed: the step it holds, bank * 16 + k, inside LEN only.
 * The press changes nothing: a short one toggles the step on the release (step_tap); a hold turns an off step on
 * (step_hold_on) and edits it (step_edit) */
static void step_grab(uint32_t k)
{
    uint32_t si = ui.bank * 16u + k;
    ui.step_si[k] = 0xFFFFu;
    if (k >= 16u || si >= (uint32_t)TSEL->p[P_SLEN])
        return;
    ui.step_si[k] = (uint16_t)si;
}

/* key k's step held for editing (STEP_HOLD_MS down, or a knob edit while down; owner 2026-10-23): an OFF step turns
 * on there and then (plain, the note a tap's new step gets), lit, its line up and its knobs live; an ON step stays
 * exactly as it is. Once a press (bit 2) */
static void step_hold_on(uint32_t k)
{
    step_t *s;
    if (k >= 16u || (ui.step_t0[k] & 4u))
        return;
    ui.step_t0[k] |= 4u;
    if (ui.step_si[k] >= (uint32_t)TSEL->p[P_SLEN])
        return;
    s = &TSEL->step[ui.step_si[k]];
    if (!s->on) {
        s->nt = new_step_nt(TSEL);                    /* spec §2: ACID's last played / recorded note, a drum's C4 */
        s->on = 1;
        s->acc = 0;
        s->tie = 0;                                   /* a TIE step turned on plays (acid_step reads TIE first) */
    }
}

/* key k let go after a short press (down less than STEP_HOLD_MS, no held-step edit, its page and track unchanged):
 * its step turns on, or off (then plain again: no accent, PROB 100 %, 1 hit) */
static void step_tap(uint32_t k)
{
    uint32_t si = ui.step_si[k];
    step_t *s;
    if (si >= (uint32_t)TSEL->p[P_SLEN])
        return;
    s = &TSEL->step[si];
    if (s->on) {
        memset(s, 0, sizeof *s);
    } else {
        s->on = 1;
        s->acc = 0;
        s->tie = 0;                                   /* a TIE step turned on plays (acid_step reads TIE first) */
        s->nt = new_step_nt(TSEL);                    /* spec §2: ACID's last played / recorded note, a drum's C4 */
    }
}

/* key k pressed with OCT+ held (fixes 2026-10-21 item 1: the accent's gesture; a held key is PROB / RATCH's): step
 * bank * 16 + k accented, an off one turned on with it (a TIE: plays), an accented one plain again (still on), inside
 * LEN only, at the press; held, the key's step (a knob turned edits it as it is now; its release toggles nothing) */
static void step_accent(uint32_t k)
{
    uint32_t si = ui.bank * 16u + k;
    step_t *s;
    ui.step_si[k] = 0xFFFFu;
    if (k >= 16u || si >= (uint32_t)TSEL->p[P_SLEN])
        return;
    s = &TSEL->step[si];
    if (!s->on) {
        s->nt = new_step_nt(TSEL);                    /* (as a tap's new step) */
        s->on = 1;
        s->tie = 0;
        s->acc = 1;
    } else {
        s->acc = (uint8_t)!s->acc;
    }
    ui.step_si[k] = (uint16_t)si;
}

static void track_clear(track_t *t)               /* its steps, its motion and its locks; its last KEYS note C4 again */
{
    memset(t->step, 0, sizeof t->step);
    t->key_nt = 0;
    motion_clear((uint32_t)(t - trk));
    lock_clear_track((uint32_t)(t - trk));
}

/* TOOLS CLR*: every track's pattern cleared and its pattern settings (LEN DIV SWG, MLEN) at default; sounds, FX,
 * COMP and the globals untouched */
static void seq_clear_all(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        track_clear(t);
        t->p[P_SLEN] = TP[P_SLEN].def;
        t->p[P_SDIV] = TP[P_SDIV].def;
        t->p[P_SSWING] = TP[P_SSWING].def;
    }
    song.mlen = 0;                                      /* (MASTER LENGTH: OFF) */
    ui.bank = 0;
    ui.force = 1;
}

/* TOOLS INIT*: everything at power-on (the default kit, sounds, patterns, FX, COMP, BPM): stopped, nothing
 * armed; saved projects and the device settings untouched */
static void init_all(void)
{
    uint32_t master = song.master_q12;                  /* the volume knob's level (main.c reads it), kept */
    transport_req = 2;
    fm1_irq_off();                                      /* the audio ISR must not see half a kit */
    drum_tracks_init();
    song.master_q12 = master;
    song.playing = 0;
    song.rec = 0;
    ui.rec_all = 0;                                     /* (no REC ALL: nothing armed) */
    song.solo = 0;
    memset(&mo, 0, sizeof mo);                     /* no motion */
    lk_gen++;
    memset(&pname, 0, sizeof pname);               /* no name */
    perf_k[0] = perf_k[1] = perf_k[2] = perf_k[3] = 0;   /* PERFORM's knob effects off (spec §7) */
    fm1_irq_on();
    ui.bank = 0;
    ui.force = 1;
}

/* track k muted / unmuted (MUTE: NOW, or NEXT BAR); a mute also cuts what it is playing (declicked, in the ISR) */
static void track_mute_toggle(uint32_t k)
{
    if (k >= NTRK)
        return;
    if (settings.mutebar) {                            /* on the next bar (seq.c; at once when stopped) */
        fm1_irq_off();
        song.mute_q ^= (uint8_t)(1u << k);
        fm1_irq_on();
        return;
    }
    trk[k].p[P_MUTE] = (int16_t)!trk[k].p[P_MUTE];
    if (trk[k].p[P_MUTE])
        panic_req |= (uint8_t)(1u << k);
}

/* SOLO (the SMC-Mixer; ARP + the black keys, ui_input.c): track k soloed (on 1) / not (0) / the other (2); the
 * tracks this silences cut what they play, as a mute does */
static uint8_t solo_mom;                               /* SOLO by ARP + a black key: 0 TOGGLE, 1 MOMENTARY (+ F#5; a device
                                                        * setting: project.c, the palette word's bit 15) */
static void track_solo_set(uint32_t k, uint32_t on)
{
    uint32_t i, was = 0;
    if (k >= NTRK)
        return;
    for (i = 0; i < NTRK; i++)
        was |= (uint32_t)trk_silent(&trk[i]) << i;
    if (on == 2u)
        song.solo ^= (uint8_t)(1u << k);
    else if (on)
        song.solo |= (uint8_t)(1u << k);
    else
        song.solo &= (uint8_t)~(1u << k);
    for (i = 0; i < NTRK; i++)
        if (trk_silent(&trk[i]) && !((was >> i) & 1u))
            panic_req |= (uint8_t)(1u << i);
}
static void track_solo_toggle(uint32_t k) { track_solo_set(k, 2u); }

/* COMP: white key k turns DUCK of track k on / off (not the source's: it is never ducked) */
static void track_duck_toggle(uint32_t k)
{
    if (k < NTRK && k != comp_src())
        trk[k].p[P_DUCK] = (int16_t)!trk[k].p[P_DUCK];
}

/* --------------------------------------------------- track, model --- */
static void track_select(uint32_t i)
{
    if (i >= NTRK || i == song.sel)
        return;
    song.sel = (uint8_t)i;
    grid_drop_holds();
    ui.bank = 0;
    kp.on = 0;                                         /* (the knob popup was the other track's) */
    ui.force = 1;
}

/* power-on, before audio_init (IRQs still off): SEQ held -> safe start. The HAL's polled scan
 * (TIMER4 waits, GPIO only) runs FM1_DEB_RELEASE + 4 frames (the longest debounce, upstream 1.0), ~7 ms */
static void safe_start_check(void)
{
    uint32_t k;
    for (k = 0; k < FM1_DEB_RELEASE + 4u; k++)
        fm1_input_scan();
    safe_start = (uint8_t)((fm1_in.buttons >> panel.btn[B_SEQ]) & 1u);
    if (safe_start)
        for (k = 0; k < SMP_USER_SLOTS; k++)
            usr_nz[k] = 0;
}

/* felucca_init (main.c): everything the drum firmware sets up before audio and USB start */
static void drum_boot_init(void)
{
    safe_start_check();
    drum_tracks_init();
    home_tracks();                                     /* HOME's TRACKS mode (spec §3) */
    ui.force = 1;
    if (!safe_start)
        project_boot();                                /* the last saved project (project.c; no IRQ enable) */
}

/* the MODEL knob's list (fix 8, owner 2026-10-21): MD -> MODERN -> CLASSIC, as the preset banks; within MD and CLASSIC
 * the enum's order, MODERN as its bank (ACID, CHORD, SMPL last). Only the order shown: the models keep their numbers
 * (projects store them, dmodels.c) */
static const uint8_t MODEL_LIST[NMODELS] = {
    DM_KTRX, DM_SDTRX, DM_XTTRX, DM_XCTRX, DM_CPTRX, DM_RSTRX, DM_CBTRX, DM_CHTRX, DM_OHTRX, DM_CYTRX, DM_MATRX, DM_CLTRX,
    DM_B2TRX, DM_BDEFM, DM_SDEFM, DM_XTEFM, DM_CPEFM, DM_RSEFM, DM_CBEFM, DM_HHEFM, DM_CYEFM,
    DM_KBOOM, DM_KPUNC, DM_SSNAP, DM_SCRAK, DM_HMETL, DM_HNOIS, DM_MEMB, DM_FM, DM_ACID, DM_CHORD, DM_SMPL,
    DM_K808, DM_K909, DM_S808, DM_S909, DM_C808, DM_C909, DM_HATC, DM_HATO, DM_CYMB, DM_COWB, DM_TOM, DM_CONGA, DM_RIM,
    DM_CLAVE,
};
static uint32_t model_list_idx(uint32_t m)           /* m's place in MODEL_LIST, from 0 (0: not in it) */
{
    uint32_t i;
    for (i = 0; i < NMODELS && MODEL_LIST[i] != m; i++)
        ;
    return i == NMODELS ? 0u : i;
}
static uint32_t model_list_next(uint32_t m, int32_t dir)   /* the model after / before m in MODEL_LIST, wrapping */
{
    return MODEL_LIST[(model_list_idx(m) + (dir > 0 ? 1u : NMODELS - 1u)) % NMODELS];
}

/* the next / previous model on the selected track (MODEL_LIST's order), with its default sound; with the audio IRQ off,
 * so the ISR never renders a model with another model's parameters */
static void model_step(int32_t dir)
{
    uint32_t m = model_list_next((uint32_t)TSEL->p[P_MODEL] % NMODELS, dir);
    int capped = m == DM_ACID && acid_tracks(TSEL) >= ACID_MAXTRK;   /* ACID on 2 tracks at most: the list skips it */
    if (capped)
        m = model_list_next(m, dir);
    fm1_irq_off();
    drum_set_model(TSEL, m);
    motion_rebase(song.sel);                        /* motion.c: the new sound is the base */
    fm1_irq_on();
    if (capped)
        ui_message("ACID: 2 TRACKS MAX");
    /* (the knob popup shows the model; it repaints the page whole when it goes) */
}

/* HOME: KNOB k edits the selected model's macro k (TUNE DECAY TONE CHAR) */
static const param_desc_t *home_param(uint32_t k, int16_t **vp)
{
    static const param_desc_t NONE_DESC;             /* KNOB 3, 4: nothing (no label: no popup, no turn) */
    static int16_t none;
    k &= 3u;
    if (k < 2u) {                                     /* FREITAKT redesign: KNOB 1 BPM, KNOB 2 the global SWING */
        *vp = &song.g[k ? G_SWING : G_BPM];
        return &GP[k ? G_SWING : G_BPM];
    }
    *vp = &none;
    return &NONE_DESC;
}

/* ---- parameter locks (spec 2026-10-24): in STEPS, off the SEQ pages, a held step + a recordable knob locks it */
#define LOCK_CLR_MS 600u                               /* step held + OCT- held this long: all its locks cleared */
static int lock_page(void) { return env_steps() && (ui.home || cur_page()->fam != FAM_SEQ); }
static uint32_t lock_step(void);
static uint32_t held_step(void);
/* a step held on a page whose white keys are the steps (TRACKS unarmed, STEP, PATTERN, MOTION; owner
 * 2026-11-01): KNOB 1-4 are its PROB RATCH ACC NUDGE (ui_input.c step_edit), the page's own knobs rest; not where
 * the held step takes locks (ENV's STEPS on TRACKS: lock_step) */
static int step_edit_page(void) { return seq_keys() && lock_step() >= NSTEP; }   /* HOME TRACKS SOUND
                                                                                                * FX LFO COMP */
/* the step of the last-pressed key still down (from the press: a turn right after it locks), NSTEP none (not a lock
 * page, no key, outside LEN) */
static uint32_t lock_step(void)
{
    uint32_t k = ui.held;
    if (!lock_page() || k >= 16u || !ui.step_t0[k] || !((fm1_in.notes >> STEP_KEY[k]) & 1u) || ui.step_si[k] >= NSTEP)
        return NSTEP;
    return ui.step_si[k];
}
/* .. shown (cells, LOCK STEP n): once held STEP_HOLD_MS or turned (a tap shows nothing, as held_step) */
static uint32_t lock_view_step(void)
{
    uint32_t s = lock_step();
    return s < NSTEP && (ui.step_t0[ui.held] & 2u) ? s : NSTEP;
}
/* the P_* id KNOB slot + 1 edits here when a step can lock it (motion.c lock_param: MOTION's, and MODEL a machine
 * lock): a track page's, HOME's macros,
 * TRACKS' LEVEL and PAN; P_COUNT none (the model, TYPE, WAVE, DEST, SRC, the timing, a global page ...) */
static uint32_t lock_id(uint32_t slot)
{
    const page_t *pg = cur_page();
    const param_desc_t *d;
    int16_t *vp = 0;
    uint32_t id;
    if (slot > 3u)
        return P_COUNT;
    if (ui.home)
        d = home_param(slot, &vp);
    else if (pg->scope == SC_MIX)
        return slot == 1u ? (uint32_t)P_LEVEL : slot == 3u ? (uint32_t)P_PAN : (uint32_t)P_COUNT;
    else if (pg->scope == SC_TRACK)
        d = page_desc(pg, slot, &vp);
    else
        return P_COUNT;
    if (!d || !vp || vp < TSEL->p || vp >= TSEL->p + P_COUNT || !d->label || d->label[0] == '-' || d->max <= d->min)
        return P_COUNT;
    id = (uint32_t)(vp - TSEL->p);
    return lock_param(id) ? id : P_COUNT;
}
/* what a cell of knob id shows: with a step held, its lock there (*locked 1) or the knob's own value; else live */
static int16_t lock_shown(uint32_t id, int16_t live, int *locked)
{
    uint32_t s = lock_view_step();
    int16_t v;
    *locked = 0;
    if (s >= NSTEP || id >= P_COUNT)
        return live;
    if (lock_get(song.sel, s, id, &v)) {
        *locked = 1;
        return v;
    }
    return motion_rest(song.sel, id);
}
