/* Drum firmware UI checks on the host. */
#include "ui_host.h"

static int fails;
static int check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        fails++;
    return ok;
}

static void snap_page(const char *name);
static void rec_hold_ms(uint32_t ms);
static void seq_play_to(uint32_t ti, uint32_t step);
static void seq_open(const char *title);
static uint32_t fb_lit(uint32_t y0, uint32_t y1);
static int list_rows_drawn(void);
static int menu_value_is(uint32_t item, const char *s);
static uint32_t fb_count(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint16_t c);
static void tap(uint32_t b);
#define OK0(e) do { if (!(e)) { printf("     (line %d)\n", __LINE__); ok = 0; } } while (0)
static void hold(uint32_t label);
static void safe_boot(int seq_held);
static void kit_session(void);
static void fx_hold_frames(uint32_t n);
static void vg_settle(void);
static int led_on(uint32_t id)                       /* the LED of button / key id lit (ui_leds' last frame: fm1_led,
                                                      * or the full layer while the scan's hook composes them) */
{
    uint32_t q = led_pos[id];
    return q != 0xFF && (((led_dim_on ? led_lay[q >> 3] : fm1_led[q >> 3]) >> (q & 7u)) & 1u);
}
static int led_breathes(uint32_t id)                 /* .. breathing (led_dim.c's breath layer; item 5) */
{
    uint32_t q = led_pos[id];
    return q != 0xFF && led_dim_on && ((led_lay[q >> 3] >> (24u + (q & 7u))) & 1u);
}

/* SELECT (spec §2): one detent; the pages, BPM on HOME's TRACKS mode */
static void sel(int32_t d)
{
    turn(EN_SELECT, d);
    ui_frame();
}
/* HOME's TRACKS mode (spec §3): HOME taps until it shows (on HOME a tap toggles TRACKS / HOME, elsewhere it opens
 * HOME in the mode last shown) */
static void open_tracks(void)
{
    uint32_t k;
    for (k = 0; k < 3u && !mix_mode(); k++)
        tap(B_HOME);
}

static void test_families(void)
{
    static const struct { uint32_t btn; uint32_t fam; } MAP[] = {
        {B_EDIT, FAM_SND}, {B_FX, FAM_FX},
        {B_SEQ, FAM_SEQ}, {B_GLO, FAM_GLO}, {B_SAVE, FAM_SAVE}, {B_LFO, FAM_LFO},
    };
    uint32_t i, ok = 1;
    ui_host_init();
    for (i = 0; i < sizeof MAP / sizeof MAP[0]; i++) {
        press(MAP[i].btn);
        ui_frame();
        release_all();
        ui_frame();                                  /* (FX opens its pages on the release) */
        ok &= !ui.home && cur_page()->fam == MAP[i].fam;
    }
    check("buttons open their page families (EDIT SOUND, FX, SEQ, GLO, SAVE, LFO LFO)", ok);
    press(B_ENV);
    ui_frame();
    release_all();
    press(B_SCL);
    ui_frame();
    check("SEL pressed: KEYS waits for the release (SEL + a black key selects a track)", !ui.keys);
    release_all();
    ui_frame();
    check("ENV (STEPS, 2026-10-24) then SCL: KEYS on (STEPS off), the page stays", !ui.home && cur_page()->fam == FAM_LFO && ui.keys);
    press(B_SCL);                                    /* KEYS off again */
    ui_frame();
    release_all();
    ui_frame();
    press(B_EDIT);
    ui_frame();
    release_all();
    press(B_EDIT);
    ui_frame();
    release_all();
    check("EDIT again: the second SOUND page", ui.page == page_first(FAM_SND) + 1u && str_eq(cur_page()->title, "SOUND"));
    {
        uint32_t i, ok = 1;
        const char *c;
        for (i = 0; i < NPAGES; i++)                 /* the footer numbers pages ("2/2"): titles carry none */
            for (c = PAGES[i].title; *c; c++)
                ok &= *c < '0' || *c > '9';
        check("page titles have no page number in them", ok);
    }
}

/* EDIT: one list per engine, 4 knobs a page: MODEL, its TUNE DECAY TONE and 4th knob, its extras, then
 * LVL PAN NOTE CHOKE (no MUTE: the TRACKS quick mutes) */
static void edit_next(void)
{
    press(B_EDIT);
    ui_frame();
    release_all();
}

static void test_sound_pages(void)
{
    uint32_t n, k;
    const page_t *pg;
    ui_host_init();
    old_set_model(&trk[0], DM_C808);
    edit_next();
    pg = cur_page();
    n = fam_pages(FAM_SND, &k);
    check("EDIT page 1 (C808): MODEL TUNE DECAY TONE; 4 pages (FREITAKT: ARP's first, a drum's)",
          n == 4u && k == 1u && page_id(pg, 0) == P_MODEL && page_id(pg, 1) == P_E0 && page_id(pg, 3) == P_E2);
    snap_page("engines/zz_c808_edit_1");
    edit_next();
    pg = cur_page();
    check("EDIT page 2 (C808): TAIL LVL PAN NOTE",
          page_id(pg, 0) == P_E3 && page_id(pg, 1) == P_LEVEL && page_id(pg, 2) == P_PAN && page_id(pg, 3) == P_NOTE);
    snap_page("engines/zz_c808_edit_2");
    edit_next();
    pg = cur_page();
    n = fam_pages(FAM_SND, &k);
    check("EDIT page 3: CHOKE PRIO, nothing else (no MUTE)", page_id(pg, 0) == P_CHOKE && page_id(pg, 1) == P_PRIO &&
                                                               page_id(pg, 2) == 0xFFu && k == 3u && n == 4u);
    snap_page("engines/zz_c808_edit_3");
    edit_next();
    pg = cur_page();
    n = fam_pages(FAM_SND, &k);
    check("EDIT page 4 (FREITAKT): ARP MODE RATE GATE FADE (a drum: its DIR CHORD RANGE page hidden)",
          str_eq(pg->title, "ARP") && page_id(pg, 0) == P_AMODE && page_id(pg, 3) == P_AFADE && k == 4u && n == 4u);
    edit_next();
    n = fam_pages(FAM_SND, &k);
    check("EDIT again: back to page 1", k == 1u);
    old_set_model(&trk[0], DM_K909);
    edit_next();
    pg = cur_page();
    check("K909 page 2: CLICK SWPT DRIVE LVL (its extras before LVL)",
          page_id(pg, 0) == P_E3 && page_id(pg, 1) == P_E4 && page_id(pg, 2) == P_E5 && page_id(pg, 3) == P_LEVEL);
    edit_next();
    edit_next();
    edit_next();
    turn(EN_K1, 1);
    ui_frame();
    check("EDIT page 1: KNOB 1 is MODEL (next engine, its default sound)", (uint32_t)trk[0].p[P_MODEL] == DM_K909 + 1u);
}

static void test_track_select(void)
{
    ui_host_init();
    turn(EN_ALGO, 3);
    ui_frame();
    check("ALGORITHM selects tracks (1 -> 2 per detent)", song.sel == 1);
    for (int i = 0; i < 20; i++) {
        turn(EN_ALGO, 1);
        ui_frame();
    }
    check("ALGORITHM stops at track 6 (FREITAKT: the last)", song.sel == NTRK - 1);
}

static void test_model_swap(void)
{
    uint32_t m0;
    ui_host_init();
    m0 = (uint32_t)trk[0].p[P_MODEL];
    trk[0].p[P_E1] = 3;                              /* an edited DECAY */
    drum_hit(&trk[0], 127);
    press(B_EDIT);
    ui_frame();
    release_all();
    turn(EN_K1, 1);
    ui_frame();
    check("MODEL knob on EDIT (page 1): next model with its default sound, voices stopped",
          (uint32_t)trk[0].p[P_MODEL] == (m0 + 1u) % NMODELS && trk[0].model == trk[0].p[P_MODEL] &&
              trk[0].p[P_E1] == DMODELS[(m0 + 1u) % NMODELS].edit[1].def && !trk[0].v[0].active);
    turn(EN_K1, -1);
    ui_frame();
    check("MODEL knob on EDIT (page 1): back to the first model, defaults loaded",
          (uint32_t)trk[0].p[P_MODEL] == m0 && trk[0].p[P_E1] == DMODELS[m0].edit[1].def);
    press(B_FX);
    ui_frame();
    release_all();
    ui_frame();
    sel(1);
    check("SELECT elsewhere (FX page) leaves the model alone", (uint32_t)trk[0].p[P_MODEL] == m0);
}

/* SELECT turns the current section's pages (spec §2), both ways, stopping at the ends (HOME's second mode: HOME 1/3
 * <-> COMP 2/3 <-> 3/3); on HOME's TRACKS mode it is BPM; PRESETS does nothing; it never changes the engine */
static void test_select_pages(void)
{
    uint32_t m0, pos, k;
    int16_t bpm;
    ui_host_init();
    m0 = (uint32_t)TSEL->p[P_MODEL];
    bpm = song.g[G_BPM];
    sel(1);
    check("SELECT on HOME (mode 2): nothing (COMP is FX's since FREITAKT), the engine and BPM unchanged",
          ui.home && (uint32_t)TSEL->p[P_MODEL] == m0 && song.g[G_BPM] == bpm);
    sel(-1);
    check("SELECT back on HOME: stays, the engine unchanged", ui.home && (uint32_t)TSEL->p[P_MODEL] == m0);
    tap(B_FX);
    check("FX opens on FILTER 1/2", str_eq(cur_page()->title, "FILTER") && page_id(cur_page(), 0) == P_FTYPE);
    sel(1);                                          /* FILTER 2/2 */
    sel(1);                                          /* FX */
    sel(1);
    check("SELECT on FX: INSERT 1/2 (TYPE A B C)", str_eq(cur_page()->title, "INSERT") && page_id(cur_page(), 0) == P_ITYPE);
    sel(1);                                          /* INSERT 2/2 */
    check("SELECT on INSERT 1/2: INSERT 2/2", str_eq(cur_page()->title, "INSERT") && page_id(cur_page(), 0) == P_IMIX);
    sel(1);
    sel(1);
    sel(1);
    check("SELECT on: COMP 1/2, COMP 2/2, FX's last page, it stays", comp_mode() && page_id(cur_page(), 0) == G_CATK);
    sel(-1);
    sel(-1);
    check("SELECT back: INSERT 2/2", str_eq(cur_page()->title, "INSERT") && page_id(cur_page(), 0) == P_IMIX);
    k = ui.page;
    turn(EN_PRESET, 3);
    ui_frame();
    turn(EN_PRESET, -3);
    ui_frame();
    check("PRESETS loads presets only (the page, the BPM, the track stay; 3 right, 3 left: the engine back)",
          ui.page == k && song.g[G_BPM] == bpm && song.sel == 0 && (uint32_t)TSEL->p[P_MODEL] == m0);
    tap(B_SEQ);
    tap(B_FX);
    check("FX again later: the page SELECT left it on (INSERT 2/2)",
          str_eq(cur_page()->title, "INSERT") && page_id(cur_page(), 0) == P_IMIX);
    tap(B_EDIT);                                     /* SOUND 1/3 .. 3/3; page 4 is empty for every engine: skipped */
    for (k = 0; k < 6u; k++)
        sel(1);
    check("SELECT on EDIT: stops on the family's last page, ARP (FREITAKT; empty SOUND pages skipped)",
          str_eq(cur_page()->title, "ARP") && fam_pages(FAM_SND, &pos) == pos && !page_hidden(ui.page));
    tap(B_OCTUP);                                    /* (no LAYER since FREITAKT: OCT+ does nothing on EDIT) */
    check("OCT+ on EDIT: the page stays (no LAYER)", str_eq(cur_page()->title, "ARP"));
    tap(B_HOME);                                     /* HOME as last shown (mode 2), then its TRACKS mode */
    tap(B_HOME);
    k = ui.page;
    host_ticks += 300u * 1000u * FM1_TICKS_PER_US;   /* (a new turn: no acceleration) */
    sel(1);
    check("SELECT on HOME's TRACKS mode: BPM +1, the page stays", mix_mode() && ui.page == k && song.g[G_BPM] == bpm + 1);
    host_ticks += 300u * 1000u * FM1_TICKS_PER_US;
    sel(-2);
    check("SELECT left on TRACKS: BPM -2", mix_mode() && song.g[G_BPM] == bpm - 1);
    turn(EN_PRESET, 2);
    ui_frame();
    check("PRESETS on TRACKS: no BPM, the page stays", mix_mode() && song.g[G_BPM] == bpm - 1);
    turn(EN_ALGO, 2);
    ui_frame();
    check("ALGORITHM on TRACKS: the track (1 -> 2 per read)", song.sel == 1);
}

static void test_home_macros(void)
{
    ui_host_init();
    trk[0].p[P_E1] = 10;
    song.g[G_SWING] = 0;
    turn(EN_K2, 5);
    ui_frame();
    check("HOME: KNOB 2 edits the global SWING (FREITAKT; not the track's DECAY)", song.g[G_SWING] > 0 &&
          trk[0].p[P_E1] == 10);
    song.g[G_SWING] = 0;
}

static void open_step_page(void)
{
    press(B_SEQ);
    ui_frame();
    release_all();
    ui_frame();                                      /* (SEQ turns its pages on the release: SEQ held is TOOLS) */
}

/* the STEP grid's keys: the 16 white keys F3 G3 A3 B3 C4 D4 E4 F4 G4 A4 B4 C5 D5 E5 F5 G5 (key index from F3) */
static const uint8_t WHITE[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};

static int led_lit(uint32_t id) { return led_on(id); }   /* the LED of button / key id, as ui_leds left it */

/* a key held on the grid for ms (frames advance the host clock), then let go */
static void grid_hold(uint32_t key, uint32_t ms)
{
    uint32_t t;
    keys(1u << key);
    ui_frame();
    for (t = 0; t < ms; t += 50u) {
        host_ticks += 50u * 1000u * FM1_TICKS_PER_US;
        ui_frame();
    }
    keys(0);
    ui_frame();
}

/* fixes 2026-10-21 item 1: OCT+ held, a step key pressed, let go, OCT+ let go: the step's accent */
static void grid_acc(uint32_t key)
{
    press(B_OCTUP);
    ui_frame();
    keys(1u << key);
    ui_frame();
    keys(0);
    ui_frame();
    release_all();
    ui_frame();
}

/* a tap turns a step on / off; a hold is a tap (fixes 2026-10-21 item 1: a held key is PROB / RATCH's, no accent);
 * OCT+ held + a step key flips its accent (an off step on and accented); OCT+ let go alone: the next bank, with a
 * step accented: none; while OCT+ is held the keys show the bank's accents */
static void test_grid_hold_accent(void)
{
    step_t *s = &trk[0].step[4];
    ui_host_init();
    open_step_page();
    grid_hold(WHITE[4], 0);
    check("grid: a tap turns step 5 on, no accent", s->on && !s->acc);
    grid_hold(WHITE[4], 0);
    check("grid: a tap on an on step turns it off", !s->on && !s->acc);
    grid_hold(WHITE[4], 200);
    check("grid: a 0.2 s press is still a tap (on, no accent)", s->on && !s->acc);
    grid_hold(WHITE[4], 600);
    check("grid: a 0.6 s hold is no tap: the step stays on, no accent", s->on && !s->acc);
    grid_acc(WHITE[4]);
    check("grid: OCT+ held + an on step: accented (it stays on), the bank unchanged", s->on && s->acc && ui.bank == 0u);
    grid_acc(WHITE[4]);
    check("grid: OCT+ held + an accented step: the accent off (it stays on)", s->on && !s->acc);
    s->on = 0;
    s->acc = 0;
    grid_acc(WHITE[4]);
    check("grid: OCT+ held + an off step: on with accent", s->on && s->acc);
    grid_hold(WHITE[4], 0);
    check("grid: a tap on an accented step turns it off", !s->on && !s->acc);
    trk[0].p[P_SLEN] = 32;                           /* two banks */
    trk[0].step[4].on = trk[0].step[4].acc = 1;
    trk[0].step[6].on = 1;
    press(B_OCTUP);
    ui_frame();
    check("OCT+ pressed: no bank yet (it waits for the release); the keys show the accents (step 5 lit, step 7 dark)",
          ui.bank == 0u && led_on(14u + WHITE[4]) && !led_on(14u + WHITE[6]));
    release_all();
    ui_frame();
    check("OCT+ let go alone: the next bank, the keys the steps again", ui.bank == 1u && !led_on(14u + WHITE[4]));
    tap(B_OCTDN);
    grid_acc(WHITE[2]);
    check("OCT+ let go after a step accented: the bank stays", ui.bank == 0u && trk[0].step[2].on && trk[0].step[2].acc);
    trk[0].step[2].on = trk[0].step[2].acc = 0;
    {
        uint32_t ms;
        keys(1u << WHITE[6]);                        /* a step key held long: no tap, no accent */
        ui_frame();
        for (ms = 0; ms < 800u; ms += 50u) {
            host_ticks += 50u * 1000u * FM1_TICKS_PER_US;
            ui_frame();
        }
        check("grid: a key held 0.8 s: step 7 stays on, no accent", trk[0].step[6].on && !trk[0].step[6].acc);
        keys(0);
        ui_frame();
        check("grid: .. and let go: still on (a hold never toggles)", trk[0].step[6].on && !trk[0].step[6].acc);
    }
    trk[0].step[4].on = 1;
    trk[0].step[4].acc = 1;
    trk[0].step[0].on = 1;
    ui.force = 1;
    snap_page("fixes/step_oct_accent_hint");
    trk[0].p[P_SLEN] = 16;
}


/* a step key held + KNOB n (n = 0 PROB, 1 RATCH) turned `steps` detents, slowly, then let go */
static void grid_hold_turn(uint32_t key, uint32_t knob, int32_t steps)
{
    int32_t i;
    keys(1u << key);
    ui_frame();
    for (i = 0; i < (steps < 0 ? -steps : steps); i++) {
        turn(EN_K1 + knob, steps < 0 ? -1 : 1);
        host_ticks += 100u * 1000u * FM1_TICKS_PER_US;   /* 0.1 s apart: no acceleration */
        ui_frame();
    }
}

static void test_grid_step_edit(void)
{
    step_t *s = &trk[0].step[4];
    ui_host_init();
    open_step_page();
    grid_acc(WHITE[4]);                              /* on, accented (OCT+ held) */
    grid_hold_turn(WHITE[4], 0, -5);                 /* 100 % -> 75 % */
    check("hold + KNOB 1: PROB of the held step (100 % -> 75 %); its accent stays, still on",
          s->on && s->acc && s->cond == cond_store(15) && ui.bank == 0);
    snap_page("seq/20_step_held_prob");
    keys(0);
    ui_frame();
    grid_hold_turn(WHITE[4], 1, 2);
    keys(0);
    ui_frame();
    check("hold + KNOB 2: RATCH of the held step (1 -> 3 hits)", s->rat == 2 && s->on && s->acc);
    grid_hold_turn(WHITE[6], 0, 3);                  /* an off step: on, 100 % -> 1-SHOT -> 1/2 -> 2/2 */
    keys(0);
    ui_frame();
    check("hold + KNOB 1 on an off step turns it on (no accent): PROB past 100 % gives 1-SHOT, 1/2, 2/2",
          trk[0].step[6].on && !trk[0].step[6].acc && trk[0].step[6].cond == 23);
    grid_hold(WHITE[6], 0);
    check("a tap turning a step off resets its PROB / RATCH", !trk[0].step[6].on && trk[0].step[6].cond == 0);
    grid_acc(WHITE[4]);                              /* accent off by OCT+ + the key: PROB / RATCH kept */
    check("OCT+ + the key flips the accent and keeps PROB / RATCH", s->on && !s->acc && s->cond == cond_store(15) && s->rat == 2);
    trk[0].p[P_SLEN] = 24;                           /* two banks; on bank 2 keys 9..16 are outside LEN */
    ui_frame();
    press(B_OCTUP);
    ui_frame();
    release_all();
    grid_hold_turn(WHITE[12], 0, -2);                /* step 29: outside LEN */
    keys(0);
    ui_frame();
    check("hold + KNOB 1 on a key outside LEN edits nothing and does not turn the bank",
          ui.bank == 1 && trk[0].step[28].cond == 0 && !trk[0].step[28].on);
    press(B_OCTDN);
    ui_frame();
    release_all();
    trk[0].p[P_SLEN] = 16;
    trk[0].step[0].on = 1;
    trk[0].step[0].cond = 33;                        /* 3/5 */
    trk[0].step[8].on = trk[0].step[8].acc = 1;
    trk[0].step[8].rat = 3;
    trk[0].step[12].on = 1;
    trk[0].step[12].cond = (uint8_t)cond_store(10);
    trk[0].step[12].rat = 1;
    ui.force = 1;
    snap_page("seq/21_step_prob_ratch");
    open_step_page();                                /* PATTERN */
    snap_page("seq/22_pattern_prob_ratch");
}

/* owner 2026-10-23: a step key toggles its step only on a SHORT press, decided on the release (down less than
 * STEP_HOLD_MS, no held-step edit meanwhile); a long press or a KNOB 1 / 2 edit while held never toggles (holding is
 * for editing; the STEP n PROB RATCH line once held past STEP_HOLD_MS). Every page whose white keys are the steps. */
static void grid_down(uint32_t key) { keys(fm1_in.notes | 1u << key); ui_frame(); }
static void grid_up(uint32_t key) { keys(fm1_in.notes & ~(1u << key)); ui_frame(); }
static void grid_wait(uint32_t ms)
{
    uint32_t t;
    for (t = 0; t < ms; t += 50u) {
        host_ticks += 50u * 1000u * FM1_TICKS_PER_US;
        ui_frame();
    }
}

static void step_hold_on_page(const char *what)
{
    step_t *s = &trk[0].step[3];
    char b[160];
    memset(s, 0, sizeof *s);
    grid_down(WHITE[3]);
    snprintf(b, sizeof b, "%s: a step key pressed: nothing yet (the step decided on the release)", what);
    check(b, !s->on);
    grid_wait(100);
    grid_up(WHITE[3]);
    snprintf(b, sizeof b, "%s: a short press (0.1 s) let go: the step on", what);
    check(b, s->on && !s->acc);
    grid_down(WHITE[3]);
    grid_wait(STEP_HOLD_MS + 100u);
    snprintf(b, sizeof b, "%s: an on step held past STEP_HOLD_MS: still on", what);
    check(b, s->on);
    grid_up(WHITE[3]);
    snprintf(b, sizeof b, "%s: .. let go: still on (a hold never toggles)", what);
    check(b, s->on);
    grid_down(WHITE[3]);
    grid_wait(50);
    grid_up(WHITE[3]);
    snprintf(b, sizeof b, "%s: a short press on an on step: off", what);
    check(b, !s->on);
    grid_down(WHITE[3]);
    grid_wait(STEP_HOLD_MS - 100u);
    snprintf(b, sizeof b, "%s: an off step held under STEP_HOLD_MS: still off", what);
    check(b, !s->on);
    grid_wait(150u);
    snprintf(b, sizeof b, "%s: .. held past STEP_HOLD_MS: on at once, still held (no accent)", what);
    check(b, s->on && !s->acc);
    grid_up(WHITE[3]);
    snprintf(b, sizeof b, "%s: .. let go: stays on", what);
    check(b, s->on);
}

static void test_step_hold(void)
{
    step_t *s = &trk[0].step[4];
    ui_host_init();
    open_step_page();
    check("STEP_HOLD_MS is about 0.3 s", STEP_HOLD_MS >= 200u && STEP_HOLD_MS <= 400u);
    s->on = 1;
    grid_down(WHITE[4]);
    check("STEP: an on step pressed stays on (no toggle on the press)", s->on);
    check("STEP: no held-step line before STEP_HOLD_MS (a tap does not flash it)", held_step() == NSTEP);
    grid_wait(STEP_HOLD_MS + 50u);
    check("STEP: held past STEP_HOLD_MS: still on; the held-step line shows step 5", s->on && held_step() == 4u);
    snap_page("seq/23_step_held_long");
    grid_up(WHITE[4]);
    check("STEP: let go after a hold: still on; no held step", s->on && held_step() == NSTEP);
    grid_down(WHITE[4]);                             /* a quick press + KNOB 1: an edit, no toggle */
    turn(EN_K1, -1);
    ui_frame();
    check("STEP: a quick KNOB 1 turn while pressed edits PROB at once; the line shows", s->on &&
          s->cond == cond_store(19) && held_step() == 4u);
    grid_up(WHITE[4]);
    check("STEP: .. let go within STEP_HOLD_MS: no toggle (edited), PROB kept", s->on && s->cond == cond_store(19));
    memset(&trk[0].step[6], 0, sizeof trk[0].step[6]);
    grid_down(WHITE[6]);                             /* an off step + KNOB 2: on (an off step's edit is no use) */
    turn(EN_K2, 1);
    ui_frame();
    check("STEP: an off step held + KNOB 2: on (no accent) with RATCH 2", trk[0].step[6].on && !trk[0].step[6].acc &&
          trk[0].step[6].rat == 1);
    grid_up(WHITE[6]);
    check("STEP: .. let go: stays on (an edit is no tap)", trk[0].step[6].on && trk[0].step[6].rat == 1);
    s->acc = 0;                                      /* accent: OCT+ held + a key, at the press, as before */
    press(B_OCTUP);
    ui_frame();
    grid_down(WHITE[4]);
    check("OCT+ held + a step key: the accent at the press (still on)", s->on && s->acc);
    grid_up(WHITE[4]);
    release_all();
    ui_frame();
    check("OCT+ + key let go quickly: no tap on the release (on, accented); the bank stays",
          s->on && s->acc && ui.bank == 0u);
    memset(&trk[0].step[8], 0, sizeof trk[0].step[8]);
    trk[0].step[9].on = 1;
    grid_down(WHITE[8]);                             /* two keys: each its own press */
    grid_down(WHITE[9]);
    grid_wait(STEP_HOLD_MS + 50u);
    grid_down(WHITE[10]);
    grid_up(WHITE[10]);
    check("a key tapped while two are held: it toggles (step 11 on)", trk[0].step[10].on);
    grid_up(WHITE[8]);
    grid_up(WHITE[9]);
    check("the two held long: step 9 (off) on by the hold, step 10 (on) unchanged", trk[0].step[8].on &&
          trk[0].step[9].on);
    trk[0].step[10].on = 0;
    grid_down(WHITE[10]);                            /* the page changes under a quick press: no tap */
    press(B_SEQ);
    ui_frame();
    release_all();
    grid_up(WHITE[10]);
    check("a quick press whose page changes before the release: no toggle", !trk[0].step[10].on);
    ui_host_init();
    seq_open("PATTERN");
    step_hold_on_page("PATTERN");
    open_tracks();
    check("TRACKS open (keys = the steps)", mix_mode() && step_keys());
    step_hold_on_page("TRACKS");
    seq_open("PATTERN");                                /* KEYS: the keys are notes, no step changes */
    tap(B_SCL);
    memset(&trk[0].step[3], 0, sizeof trk[0].step[3]);
    grid_down(WHITE[3]);
    grid_wait(50);
    grid_up(WHITE[3]);
    check("KEYS on: a key tap changes no step", ui.keys && !trk[0].step[3].on);
    tap(B_SCL);
}

/* owner 2026-10-23, follow-up: an OFF step held STEP_HOLD_MS turns on there and then (lit, its line), KNOB 1 / 2 its
 * PROB / RATCH; KNOB 3 its accent (right: ACC on, left: off), shown in the held-step line and the knob columns; an
 * ON step's hold changes nothing; OCT+ + a key still flips the accent */
static void test_step_hold_on_acc(void)
{
    step_t *s = &trk[0].step[5];
    ui_host_init();
    open_step_page();
    memset(s, 0, sizeof *s);
    grid_down(WHITE[5]);
    grid_wait(STEP_HOLD_MS - 100u);
    check("off step held under STEP_HOLD_MS: still off, its key dark, no line", !s->on && !led_on(14u + WHITE[5]) &&
          held_step() == NSTEP);
    grid_wait(150u);
    check("off step held past STEP_HOLD_MS: on (no accent, plain) while held, its key lit, its line",
          s->on && !s->acc && s->cond == 0 && s->rat == 0 && led_on(14u + WHITE[5]) && held_step() == 5u);
    turn(EN_K1, -1);
    host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    turn(EN_K2, 1);
    host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    check("held (turned on by the hold): KNOB 1 PROB, KNOB 2 RATCH", s->on && s->cond == cond_store(19) && s->rat == 1);
    turn(EN_K3, 1);
    host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    check("held: KNOB 3 right: ACC on; PROB / RATCH kept", s->on && s->acc && s->cond == cond_store(19) && s->rat == 1);
    check("held: step 6 (the popup: ACC ON)", held_step() == 5u && kp.on && kp.kind == KP_STEP && kp.id == 2u);
    snap_page("seq/24_step_held_acc");
    turn(EN_K3, 3);
    host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    check("held: KNOB 3 further right: ACC stays on", s->acc);
    turn(EN_K3, -1);
    host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    check("held: KNOB 3 left: ACC off, still on", s->on && !s->acc && held_step() == 5u);
    turn(EN_K4, 2);
    host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    check("held: KNOB 4 is the step's MICRO (+2; step, bank, accent unchanged)", s->on && !s->acc && ui.bank == 0u &&
          s->micro == 2);
    grid_up(WHITE[5]);
    check("let go: on, PROB / RATCH kept, no accent", s->on && !s->acc && s->cond == cond_store(19) && s->rat == 1);
    memset(&trk[0].step[7], 0, sizeof trk[0].step[7]);
    grid_down(WHITE[7]);                             /* KNOB 3 before STEP_HOLD_MS on an off step: on, accented */
    turn(EN_K3, 1);
    ui_frame();
    check("off step, KNOB 3 right at once: on and accented", trk[0].step[7].on && trk[0].step[7].acc);
    grid_up(WHITE[7]);
    check(".. let go quickly: stays on, accented (an edit is no tap)", trk[0].step[7].on && trk[0].step[7].acc);
    trk[0].step[2].on = 1;                           /* an ON step held: nothing changes (no accent, no plain-again) */
    trk[0].step[2].acc = 1;
    trk[0].step[2].rat = 2;
    grid_down(WHITE[2]);
    grid_wait(STEP_HOLD_MS + 200u);
    grid_up(WHITE[2]);
    check("an on (accented, RATCH 3) step held and let go: unchanged", trk[0].step[2].on && trk[0].step[2].acc &&
          trk[0].step[2].rat == 2);
    grid_acc(WHITE[2]);
    check("OCT+ + the key: the accent off as before (still on)", trk[0].step[2].on && !trk[0].step[2].acc);
    grid_acc(WHITE[2]);
    check("OCT+ + the key again: accented", trk[0].step[2].on && trk[0].step[2].acc);
    seq_open("PATTERN");                             /* (owner 2026-11-01) a held step's KNOB 3 is its accent on every
                                                      * steps page: SWING rests */
    {
        int16_t sw = trk[0].p[P_SSWING];
        trk[0].step[2].acc = 0;
        grid_down(WHITE[2]);
        grid_wait(STEP_HOLD_MS + 50u);
        turn(EN_K3, 2);
        host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
        ui_frame();
        grid_up(WHITE[2]);
        check("PATTERN: a step held + KNOB 3: the accent, SWING untouched", trk[0].p[P_SSWING] == sw && trk[0].step[2].acc &&
              trk[0].step[2].on);
    }
}

/* review #2: a step key held while the track changes (ALGO, or a page change) writes nothing into the new track */
static void test_grid_hold_track_change(void)
{
    step_t before;
    ui_host_init();
    trk[0].p[P_SLEN] = 64;
    trk[1].p[P_SLEN] = 16;
    open_step_page();
    press(B_OCTUP);                                  /* bank 2 of T1: steps 17..32 */
    ui_frame();
    release_all();
    trk[0].step[20].on = 1;
    trk[0].step[20].cond = 33;
    before = trk[1].step[20];
    keys(1u << WHITE[4]);                            /* hold T1 step 21 */
    ui_frame();
    turn(EN_ALGO, 1);                                /* T2 while holding */
    ui_frame();
    turn(EN_K1, 2);
    host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    host_ticks += 600u * 1000u * FM1_TICKS_PER_US;   /* and held past the accent time */
    ui_frame();
    keys(0);
    ui_frame();
    check("a held step key, then another track (ALGO): the new track's steps are untouched (also outside its LEN), "
          "the held one too (no tap on the release)",
          song.sel == 1 && !memcmp(&trk[1].step[20], &before, sizeof before) && trk[0].step[20].cond == 33 &&
              trk[0].step[20].on);
}

static void open_comp(int page2)                    /* FX until COMP (FREITAKT: FX's pages after INSERT), then
                                                      * SELECT to its page 1 or 2 */
{
    uint32_t k;
    for (k = 0; k < 12u && !comp_mode(); k++)
        tap(B_FX);
    for (k = 0; k < 4u && comp_mode() && (cur_page()->id[0] == G_CATK) != page2; k++)
        sel(page2 ? 1 : -1);
}

static void test_comp_pages(void)
{
    uint32_t k, pos, fx_comp = 0;
    ui_host_init();
    for (k = 0; k < 12u && !fx_comp; k++) {          /* FX steps through its pages: COMP after INSERT */
        press(B_FX);
        ui_frame();
        release_all();
        ui_frame();
        fx_comp = !ui.home && cur_page()->graph == GR_COMP;
    }
    check("FX reaches COMP (SRC THRSH RATIO REL) after INSERT 2/2, FX lit",
          fx_comp && cur_page()->fam == FAM_FX && cur_page()->id[0] == G_CSRC && PAGES[ui.page - 1u].id[0] == P_IMIX &&
              led_lit(panel.btn[B_FX]));
    sel(1);
    check("SELECT again: COMP 2/2 (ATK KNEE MKUP GHOST), FX's last page", !ui.home && cur_page()->id[0] == G_CATK &&
          fam_pages(FAM_FX, &pos) == pos);
    tap(B_HOME);
    check("a HOME tap on COMP: the HOME screen", ui.home);
    sel(1);
    check("SELECT on HOME: no COMP there any more (HOME stays)", ui.home);
    open_comp(0);
    snap_page("comp/01_off");
    turn(EN_K1, 1);
    ui_frame();
    turn(EN_K1 + 1, -2);
    ui_frame();
    check("COMP: KNOB 1 picks the source (T1), KNOB 2 the threshold", song.g[G_CSRC] == 1 && song.g[G_CTHR] == 24);
    open_comp(1);
    turn(EN_K1 + 1, -1);
    ui_frame();
    check("COMP 2/2 (ATK KNEE MKUP); KNOB 2 sets the knee HARD",
          cur_page()->id[0] == G_CATK && cur_page()->id[2] == G_CMKUP && song.g[G_CKNEE] == 0);
    snap_page("comp/03_page2_curve_hard");
}

/* COMP 3/3's KNOB 4: GHOST (MUTE KEEP HIDE, KEEP at start); the picture shows the source HEARD / GHOST / MUTED */
static void test_comp_ghost_knob(void)
{
    char v[12];
    const char *u;
    int ok = 1;
    ui_host_init();
    song.g[G_CSRC] = 1;
    trk[1].p[P_DUCK] = 1;
    open_comp(1);
    check("COMP 3/3: KNOB 4 is GHOST, KEEP at start, the source HEARD",
          cur_page()->id[3] == G_CGHOST && str_eq(GP[G_CGHOST].label, "GHOST") && song.g[G_CGHOST] == CG_KEEP &&
              str_eq(comp_src_state(), "HEARD"));
    param_format(&GP[G_CGHOST], CG_MUTE, v, &u);
    ok &= str_eq(v, "MUTE");
    param_format(&GP[G_CGHOST], CG_HIDE, v, &u);
    ok &= str_eq(v, "HIDE");
    check("GHOST values: MUTE KEEP HIDE", ok);
    trk[0].p[P_MUTE] = 1;
    ui_frame();
    check("GHOST KEEP, the source muted: GHOST", str_eq(comp_src_state(), "GHOST"));
    snap_page("comp/05_ghost_keep_muted");
    turn(EN_K1 + 3, -1);
    ui_frame();
    check("KNOB 4 down: MUTE; the muted source MUTED", song.g[G_CGHOST] == CG_MUTE && str_eq(comp_src_state(), "MUTED"));
    snap_page("comp/06_ghost_mute_muted");
    trk[0].p[P_MUTE] = 0;
    turn(EN_K1 + 3, 2);
    ui_frame();
    check("KNOB 4 up twice: HIDE; the unmuted source GHOST", song.g[G_CGHOST] == CG_HIDE && str_eq(comp_src_state(), "GHOST"));
    snap_page("comp/07_ghost_hide");
    open_comp(0);
    snap_page("comp/08_ghost_hide_page1");
    song.g[G_CSRC] = 0;
    check("SRC OFF: no source state", str_eq(comp_src_state(), ""));
    song.g[G_CSRC] = 1;
    project_save(0);
    song.g[G_CGHOST] = CG_MUTE;
    (void)proj_load(0, 1);                           /* (a global edit: as DISCARD EDITS? YES) */
    check("GHOST is saved with the project (FDR6)", song.g[G_CGHOST] == CG_HIDE);
    memset(&proj_slot[0], 0, sizeof proj_slot[0]);  /* the slot empty again for the project tests */
}

/* ACID on 2 tracks at most: a 3rd track's MODEL knob skips ACID with the message; a project
 * with 4 ACID tracks loads with the first 2, the others back to their power-on kit model */
static void test_acid_cap(void)
{
    uint32_t k, n;
    ui_host_init();
    old_set_model(&trk[0], DM_ACID);
    old_set_model(&trk[1], DM_ACID);
    song.sel = 2;
    old_set_model(&trk[2], DM_FM);
    model_step(1);
    check("ACID cap: a 3rd track's MODEL knob goes past ACID (full) to the next model in the list (SMPL)",
          trk[2].p[P_MODEL] == DM_SMPL && str_eq(ui.msg, "ACID: 2 TRACKS MAX"));
    model_step(-1);
    check("ACID cap: back down, past ACID (full) to FM", trk[2].p[P_MODEL] == DM_FM);
    old_set_model(&trk[1], DM_K808);                /* a place frees */
    model_step(1);
    check("ACID cap: once a place frees, the 3rd track gets ACID", trk[2].p[P_MODEL] == DM_ACID);
    ui_host_init();
    for (k = 0; k < 4u; k++)
        old_set_model(&trk[k], DM_ACID);
    project_save(0);
    project_load(0);
    for (k = 0, n = 0; k < NTRK; k++)
        n += trk[k].p[P_MODEL] == DM_ACID;
    check("ACID cap on load: the first two ACID tracks keep it, the others fall back to their kit model",
          n == 2u && trk[0].p[P_MODEL] == DM_ACID && trk[1].p[P_MODEL] == DM_ACID &&
              trk[2].p[P_MODEL] == KIT_DEF[2][0] && trk[3].p[P_MODEL] == KIT_DEF[3][0] &&
              trk[2].model == KIT_DEF[2][0] && trk[2].p[P_E0] == DMODELS[KIT_DEF[2][0]].edit[0].def);
    memset(&proj_slot[0], 0, sizeof proj_slot[0]);
}

/* COMP pages: white keys toggle DUCK (lit = ducked), not on the source; with SRC OFF too; elsewhere they play */
static void test_comp_keys(void)
{
    uint32_t a;
    ui_host_init();
    open_comp(0);
    a = hit_age(&trk[1]);
    keys(1u << KEY_TRK_KEY[1]);
    ui_frame();
    keys(0);
    ui_frame();
    check("COMP (SRC OFF): key 2 sets DUCK on T2, no hit, its LED lit",
          trk[1].p[P_DUCK] == 1 && hit_age(&trk[1]) == a && led_lit(14u + KEY_TRK_KEY[1]));
    song.g[G_CSRC] = 1;
    keys(1u << KEY_TRK_KEY[0]);
    ui_frame();
    keys(0);
    ui_frame();
    check("COMP: the source's key (T1) does not toggle DUCK", trk[0].p[P_DUCK] == 0 && !led_lit(14u + KEY_TRK_KEY[0]));
    keys(1u << KEY_TRK_KEY[1]);
    ui_frame();
    keys(0);
    ui_frame();
    check("COMP: key 2 again: DUCK off", trk[1].p[P_DUCK] == 0 && !led_lit(14u + KEY_TRK_KEY[1]));
    trk[1].p[P_DUCK] = trk[3].p[P_DUCK] = trk[4].p[P_DUCK] = 1;
    old_set_model(&trk[1], DM_HATO);
    trk[0].step[0].on = trk[0].step[4].on = trk[0].step[8].on = trk[0].step[12].on = 1;
    trk[1].step[2].on = trk[1].step[6].on = trk[1].step[10].on = trk[1].step[14].on = 1;
    transport_req = 1;
    seq_play_to(0, 1);
    snap_page("comp/02_pumping");
    song.g[G_CMKUP] = 127;                           /* MKUP at the end: Streams' limiter */
    seq_play_to(0, 5);
    snap_page("comp/04_limiter");
    tap(B_HOME);                                     /* a HOME tap on COMP: the HOME screen */
    a = hit_age(&trk[2]);
    keys(1u << KEY_TRK_KEY[2]);
    ui_frame();
    keys(0);
    check("leaving COMP: the keys play again", hit_age(&trk[2]) != a);
}

/* TOOLS: CLR* clears every pattern and its settings (sounds untouched); INIT* puts everything at power-on;
 * both arm on the first detent and act on the second */
static void test_tools_all(void)
{
    uint32_t k;
    ui_host_init();
    old_set_model(&trk[2], DM_CONGA);
    trk[2].p[P_E0] = 7;
    trk[2].p[P_LEVEL] = 90;
    trk[2].p[P_DIST] = 50;
    trk[2].step[3].on = 1;
    trk[2].step[3].cond = 33;
    trk[2].p[P_SLEN] = 12;
    trk[2].p[P_SDIV] = 1;
    trk[2].p[P_SSWING] = 40;
    trk[5].step[0].on = 1;
    song.g[G_BPM] = 140;
    song.g[G_RSIZE] = 99;
    song.g[G_CSRC] = 3;
    for (k = 0; k < NPAGES && (ui.home || !str_eq(cur_page()->title, "TOOLS")); k++) {
        tap(B_SAVE);                                 /* (SAVE opens its pages on the release: 2026-10-26) */
    }
    turn(EN_K1 + 2, 1);
    ui_frame();
    check("TOOLS CLR*: the first detent only arms", trk[2].step[3].on && trk[5].step[0].on);
    turn(EN_K1 + 2, 1);
    ui_frame();
    check("TOOLS CLR*: every pattern cleared, LEN 16 / DIV 1/16 / SWG 0",
          !trk[2].step[3].on && trk[2].step[3].cond == 0 && !trk[5].step[0].on && trk[2].p[P_SLEN] == 16 &&
              trk[2].p[P_SDIV] == TP[P_SDIV].def && trk[2].p[P_SSWING] == 0);
    check("TOOLS CLR*: sounds, FX, COMP and BPM untouched",
          trk[2].p[P_MODEL] == DM_CONGA && trk[2].p[P_E0] == 7 && trk[2].p[P_LEVEL] == 90 && trk[2].p[P_DIST] == 50 &&
              song.g[G_BPM] == 140 && song.g[G_RSIZE] == 99 && song.g[G_CSRC] == 3);
    snap_page("tools_all");
    transport_req = 1;
    ui_frame();
    song.rec = 4u;
    song.master_q12 = 900;
    turn(EN_K1 + 3, 1);
    ui_frame();
    check("TOOLS INIT*: the first detent only arms", trk[2].p[P_MODEL] == DM_CONGA);
    turn(EN_K1 + 3, 1);
    ui_frame();
    ui_frame();
    check("TOOLS INIT*: everything at power-on (kit, sounds, BPM, FX, COMP), stopped, nothing armed",
          (uint32_t)trk[2].p[P_MODEL] == KIT_DEF[2][0] && trk[2].p[P_LEVEL] == TP[P_LEVEL].def && trk[2].p[P_DIST] == 0 &&
              song.g[G_BPM] == GP[G_BPM].def && song.g[G_RSIZE] == GP[G_RSIZE].def && song.g[G_CSRC] == 0 &&
              !song.playing && song.rec == 0 && song.sel == 0);
    check("TOOLS INIT*: the volume knob's level is kept", song.master_q12 == 900u);
}

static void tap(uint32_t b)
{
    press(b);
    ui_frame();
    release_all();
    ui_frame();
}

static void test_layer_in_edit(void)                 /* FREITAKT removed the sample LAYER: no page under SOUND */
{
    uint32_t k;
    ui_host_init();
    tap(B_EDIT);
    tap(B_EDIT);                                     /* SOUND 2/3 */
    k = ui.page;
    tap(B_OCTUP);
    check("EDIT: OCT+ does nothing (no LAYER)", ui.page == k && str_eq(cur_page()->title, "SOUND"));
    tap(B_OCTDN);
    check("EDIT: OCT- does nothing either", ui.page == k && str_eq(cur_page()->title, "SOUND"));
    ui_frame();
    check("SOUND: the EDIT LED is steady", led_lit(panel.btn[B_EDIT]));
    for (k = 0; k < NPAGES; k++)
        if (str_eq(PAGES[k].title, "LAYER"))
            break;
    check("no LAYER page left", k == NPAGES);
}

/* the LFO button (fixes 2026-10-21 item 3): one page set with both LFOs, LFO 1 on KNOB 1-2 and LFO 2 on KNOB 3-4
 * (WAVE RATE, MORPH PHASE, TRIG MODE), then DEST: the track's 4 slots (KNOB 1 the slot, 2 SRC, 3 DEST, 4 DEPTH). OCT-
 * does nothing there any more; OCT+ the RATE MODE of the LFO turned last (DEST: the slot's source) */
static void test_lfo_pages(void)                     /* FREITAKT: one LFO in the UI, 3 LIST pages */
{
    uint32_t k, pos;
    int16_t w2, m2;
    ui_host_init();
    w2 = TSEL->p[P_LFO2 + LF_WAVE];
    m2 = TSEL->p[P_LFO2 + LF_MODE];
    tap(B_LFO);
    check("LFO opens LFO 1/3: WAVE RATE MORPH PHASE of LFO 1",
          str_eq(cur_page()->title, "LFO") && page_id(cur_page(), 0) == P_LFO1 + LF_WAVE &&
              page_id(cur_page(), 1) == P_LFO1 + LF_RATE && page_id(cur_page(), 2) == P_LFO1 + LF_MORPH &&
              page_id(cur_page(), 3) == P_LFO1 + LF_PHASE && fam_pages(FAM_LFO, &pos) == 3u && pos == 1u);
    snap_page("lfo/page1_wave");
    turn(EN_K1, 1);
    ui_frame();
    check("KNOB 1 turns LFO 1's WAVE (LFO 2 untouched)", TSEL->p[P_LFO1 + LF_WAVE] != 0 && TSEL->p[P_LFO2 + LF_WAVE] == w2);
    k = (uint32_t)TSEL->p[P_LFO1 + LF_MODE];
    tap(B_OCTUP);
    check("OCT+ on an LFO page: LFO 1's RATE MODE steps (LFO 2's untouched)",
          (uint32_t)TSEL->p[P_LFO1 + LF_MODE] == (k + 1u) % 3u && TSEL->p[P_LFO2 + LF_MODE] == m2);
    tap(B_OCTDN);
    check("OCT- on an LFO page: nothing (the same page)", str_eq(cur_page()->title, "LFO") && ui.page == page_first(FAM_LFO));
    tap(B_LFO);
    check("LFO again: 2/3 TRIG MODE of LFO 1", page_id(cur_page(), 0) == P_LFO1 + LF_TRIG &&
          page_id(cur_page(), 1) == P_LFO1 + LF_MODE && fam_pages(FAM_LFO, &pos) == 3u && pos == 2u);
    snap_page("lfo/page2_trig_mode");
    tap(B_LFO);
    check("3/3 DEST: KNOB 1 the slot (1), the slot's SRC DEST DEPTH on KNOB 2-4",
          str_eq(cur_page()->title, "DEST") && song.lslot == 0u && fam_pages(FAM_LFO, &pos) == 3u && pos == 3u);
    TSEL->p[P_LFO1 + LF_DEST] = 3;
    TSEL->p[P_LFO1 + LF_DEPTH] = 40;
    ui.force = 1;
    ui_frame();
    snap_page("lfo/dest_page");
    turn(EN_K1, 2);
    ui_frame();
    turn(EN_K1 + 2, 2);
    ui_frame();
    turn(EN_K1 + 3, 8);
    ui_frame();
    check("KNOB 1: slot 3; KNOB 3 its DEST, KNOB 4 its DEPTH; slot 1 kept", song.lslot == 2u &&
          TSEL->p[P_LS3DEST] == 2 && TSEL->p[P_LS3DEPTH] == 8 && TSEL->p[P_LFO1 + LF_DEST] == 3 &&
          TSEL->p[P_LFO1 + LF_DEPTH] == 40);
    turn(EN_K1, 5);
    ui_frame();
    check("KNOB 1 stops at slot 4", song.lslot == 3u);
    keys(1u << KEY_TRK_KEY[3]);
    ui_frame();
    keys(0);
    ui_frame();
    check("DEST: a white key selects its track (the slot stays)", song.sel == 3u && song.lslot == 3u);
    tap(B_LFO);
    check("LFO on DEST: back to 1/3", ui.page == page_first(FAM_LFO));
    render_mix(0, 0, CTL);
}

/* the EDIT gauge marker: a modulated knob shows its live value */
static void test_lfo_edit_marker(void)
{
    int32_t mv = -1;
    ui_host_init();
    TSEL->p[P_LFO1 + LF_WAVE] = LW_SQUARE;
    TSEL->p[P_LFO1 + LF_MODE] = LM_HZ;
    TSEL->p[P_LFO1 + LF_RATE] = 60;
    TSEL->p[P_LFO1 + LF_DEST] = 2;                   /* DECAY */
    TSEL->p[P_LFO1 + LF_DEPTH] = 40;
    tap(B_EDIT);
    render_mix(0, 0, CTL * 50);
    check("EDIT: a modulated knob reports its live value (for the gauge marker)",
          lfo_live(TSEL, P_E1, &mv) && mv != TSEL->p[P_E1]);
    snap_page("lfo/90_edit_marker");
}

static void test_grid_keys(void)
{
    uint32_t a;
    ui_host_init();
    open_step_page();
    check("SEQ opens PATTERN (the keys its steps)", str_eq(cur_page()->title, "PATTERN") && song.seq_mode);
    a = dvage;
    keys(1u << WHITE[2]);                            /* the third white key, A3 */
    ui_frame();
    keys(0);
    ui_frame();
    check("grid: the third white key (A3) sets step 3 on, no drum hit",
          trk[0].step[2].on && !trk[0].step[2].acc && dvage == a);
    {
        uint32_t i, others = 0;
        for (i = 0; i < NSTEP; i++)                  /* only step 3 on: only its key is lit */
            trk[0].step[i].on = i == 2;
        ui_frame();
        for (i = 0; i < 27u; i++)
            others += i != WHITE[2] && i != CUE_BK[0] && led_lit(14u + i);   /* (black 1: the selected track) */
        check("grid: step 3's LED is the third white key's (A3), no other step key lit (black 1: track 1, selected)",
              led_lit(14u + WHITE[2]) && !others && led_lit(14u + CUE_BK[0]));
    }
    keys(1u << WHITE[2]);
    ui_frame();
    keys(0);
    ui_frame();
    check("grid: a second tap turns the step off", !trk[0].step[2].on && !trk[0].step[2].acc);
    trk[0].p[P_SLEN] = 64;
    press(B_OCTUP);
    ui_frame();
    release_all();
    keys(1u << 0);
    ui_frame();
    keys(0);
    ui_frame();
    check("grid: OCT+ moves to bank 2 (white key 1 = step 17)", ui.bank == 1 && trk[0].step[16].on);
    {
        uint32_t i, on0 = 0, on1 = 0;
        for (i = 0; i < NSTEP; i++)
            on0 += trk[0].step[i].on + trk[0].step[i].acc;
        keys(1u << 1 | 1u << 3 | 1u << 20 | 1u << 25);  /* black keys: F#3 G#3 C#5 F#5 */
        ui_frame();
        keys(0);
        ui_frame();
        for (i = 0; i < NSTEP; i++)
            on1 += trk[0].step[i].on + trk[0].step[i].acc;
        check("grid: black keys change no step", on0 == on1);
    }
    keys(1u << WHITE[15]);                           /* G5, the 16th white key: step 32 of bank 2 */
    ui_frame();
    keys(0);
    ui_frame();
    check("grid: the 16th white key (G5) is step 16 of the bank", trk[0].step[31].on);
    press(B_SEQ);                                    /* PATTERN page: the keys stay the steps (user, 2026-10-08) */
    ui_frame();
    release_all();
    ui_frame();                                      /* (the page on SEQ's release) */
    check("PATTERN page: the keys stay the steps", str_eq(cur_page()->title, "PATTERN") && song.seq_mode == 1u);
}

static void test_bank_follows_len(void)
{
    ui_host_init();
    trk[0].p[P_SLEN] = 64;
    open_step_page();
    tap(B_OCTUP);                                    /* (the bank on OCT+'s release: item 1) */
    tap(B_OCTUP);
    tap(B_OCTUP);
    check("bank 4 of 64 steps", ui.bank == 3);
    trk[0].p[P_SLEN] = 20;                           /* LEN shortened elsewhere (knob, project) */
    ui_frame();
    check("shorter LEN: the bank follows (20 steps = banks 1..2)", ui.bank == 1);
    keys(1u << WHITE[10]);                           /* step 27 > LEN: ignored */
    ui_frame();
    keys(0);
    ui_frame();
    check("grid: a key past LEN changes nothing", !trk[0].step[26].on);
}

/* HOME's two modes (spec §3): 1 TRACKS (the mixer: TRACK LEVEL LEN PAN, the keys as on TRACKS), 2 HOME (the scope,
 * the selected track's macros); a HOME tap on HOME toggles them, elsewhere it opens the mode last shown; power-on:
 * TRACKS */
static void test_home_modes(void)
{
    uint32_t k;
    safe_boot(0);                                    /* power-on */
    ui_frame();
    check("power-on: HOME's TRACKS mode (the TRACKS mixer, the footer TRACKS), HOME lit, REC dark",
          mix_mode() && !ui.home && str_eq(cur_page()->title, "TRACKS") && fam_pages(cur_page()->fam, &k) == 1u &&
              led_lit(panel.btn[B_HOME]) && !led_lit(panel.btn[B_REC]) && song.seq_mode == 1u);
    snap_page("fixes/home_tracks");
    turn(EN_K1, 1);
    ui_frame();
    check("TRACKS mode: KNOB 1 selects the track", song.sel == 1);
    turn(EN_K1, -1);
    ui_frame();
    tap(B_HOME);
    check("a HOME tap on TRACKS: HOME's second mode (the HOME screen), HOME lit", ui.home && !mix_mode() &&
          led_lit(panel.btn[B_HOME]) && song.seq_mode == 0u);
    snap_page("fixes/home_mode2");
    tap(B_HOME);
    check("a HOME tap on the HOME screen: TRACKS again", mix_mode());
    tap(B_EDIT);
    tap(B_HOME);
    check("from EDIT a HOME tap opens HOME in the mode last shown (TRACKS)", mix_mode());
    tap(B_HOME);
    tap(B_GLO);
    tap(B_HOME);
    check("from GLO a HOME tap opens HOME in the mode last shown (the HOME screen)", ui.home);
    sel(1);
    sel(1);
    tap(B_SEQ);
    tap(B_HOME);
    check("COMP 3/3, then SEQ, then HOME: the HOME screen (COMP is the second mode's)", ui.home);
    tap(B_HOME);
    hold(B_HOME);                                    /* the menu over TRACKS, and closed again */
    check("HOME held on TRACKS: the menu", ui.menu == 1u);
    hold(B_HOME);
    check("the menu closed: HOME in the mode last shown (TRACKS)", !ui.menu && mix_mode());
    {
        static const uint8_t B[] = {B_EDIT, B_FX, B_SEQ, B_GLO, B_SAVE, B_LFO, B_ENV, B_SCL, B_REC};
        uint32_t b, any = 0;
        for (b = 0; b < sizeof B; b++) {             /* each button tapped on and on: never TRACKS */
            ui_host_init();
            for (k = 0; k < 4u; k++) {
                tap(B[b]);
                any |= (uint32_t)mix_mode();
            }
            transport_req = 2;
            ui_frame();
        }
        check("no page button and no REC opens TRACKS (HOME's)", !any);
    }
    ui_host_init();                                  /* the screens with a pattern: TRACKS, then HOME's second mode */
    kit_session();
    open_tracks();
    snap_page("fixes/home_tracks_kit");
    tap(B_HOME);
    snap_page("fixes/home_mode2_kit");
    transport_req = 2;
    ui_frame();
}

/* REC (spec §4): a tap arms / disarms the selected track on every screen (it never starts play: PLAY does); never
 * opens TRACKS; the menu, NAME and a confirm keep it. On TRACKS the selected track, elsewhere REC ALL (test_rec_all) */
static void test_rec_everywhere(void)
{
    static const struct { const char *what; uint32_t btn, n; } S[] = {
        {"HOME (mode 2)", B_HOME, 0}, {"HOME's TRACKS mode", B_HOME, 1}, {"EDIT", B_EDIT, 1}, {"FX", B_FX, 1},
        {"GLO", B_GLO, 1}, {"SEQ PATTERN", B_SEQ, 2}, {"LFO", B_LFO, 1}, {"SAVE", B_SAVE, 1},
    };
    uint32_t i, j, pg, home;
    char what[96];
    for (i = 0; i < sizeof S / sizeof S[0]; i++) {
        ui_host_init();
        for (j = 0; j < S[i].n; j++)
            tap(S[i].btn);
        turn(EN_ALGO, 1);                            /* track 2 */
        ui_frame();
        pg = ui.page;
        home = ui.home;
        tap(B_REC);
        ui_frame();
        if (mix_mode()) {                            /* TRACKS: the selected track */
            snprintf(what, sizeof what, "%s: a REC tap arms track 2 (T2 REC ARMED), no play, the screen stays, REC lit",
                     S[i].what);
            check(what, song.rec == 2u && !ui.rec_all && str_eq(ui.msg, "T2 REC ARMED") && !song.playing &&
                        !transport_req && ui.page == pg && ui.home == home && led_lit(panel.btn[B_REC]));
        } else {                                     /* elsewhere: REC ALL */
            snprintf(what, sizeof what, "%s: a REC tap: REC ALL, no play, the screen stays, REC lit", S[i].what);
            check(what, song.rec == REC_ALL_MASK && ui.rec_all && !song.playing && !transport_req && ui.page == pg &&
                        ui.home == home && led_lit(panel.btn[B_REC]));
        }
        tap(B_REC);
        snprintf(what, sizeof what, "%s: a REC tap again disarms (still stopped), REC dark", S[i].what);
        check(what, song.rec == 0u && !ui.rec_all && !song.playing && ui.page == pg && ui.home == home &&
                    !led_lit(panel.btn[B_REC]));
        transport_req = 2;
        ui_frame();
    }
    ui_host_init();
    tap(B_EDIT);
    pg = ui.page;
    hold(B_REC);
    check("REC held long off the SEQ pages: no dialog; a fill while held, no tap on the release (spec 2026-10-26: was "
          "REC ALL)", !ui.confirm && song.rec == 0u && !ui.rec_all && !fill_on && ui.page == pg);
    transport_req = 2;
    ui_frame();
    ui_host_init();
    hold(B_HOME);                                    /* the menu */
    tap(B_REC);
    check("REC in the menu: nothing", ui.menu == 1u && song.rec == 0u && !song.playing);
    hold(B_HOME);
    ui_host_init();
    trk[0].step[0].on = 1;
    seq_open("PATTERN");
    hold(B_REC);                                     /* REC held on SEQ: FILL (phase C: no CLEAR TRACK? dialog) */
    check("REC held on SEQ: no dialog (the clear is REC + SEL since phase C), no tap, the steps kept", !ui.confirm &&
          song.rec == 0u && !ui.rec_all && !fill_on && trk[0].step[0].on);
}

/* upstream 1.0 (project.c "STOP TO SAVE"): a flash erase silences the audio and stalls the sequencer, so no project
 * is saved while playing, and a settings save waits until the transport stops */
static void test_no_save_while_playing(void)
{
    uint32_t k;
    ui_host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    transport_req = 1;
    ui_frame();
    for (k = 0; k < 8u && (ui.home || page_id(cur_page(), 0) != G_SLOT); k++)
        tap(B_SAVE);
    turn(EN_K1 + 3, 1);                              /* SAVE: GO, and GO again */
    ui_frame();
    turn(EN_K1 + 3, 1);
    ui_frame();
    check("SAVE while playing: refused (STOP TO SAVE), the slot stays empty",
          song.playing && str_eq(ui.msg, "STOP TO SAVE") && !project_used(0));
    settings_save();
    check("a settings save while playing waits", ui.persist_pending == 1u);
    transport_req = 2;
    ui_frame();
    ui_frame();
    check("... and is written once stopped", !song.playing && ui.persist_pending == 0u);
    turn(EN_K1 + 3, 1);
    ui_frame();
    turn(EN_K1 + 3, 1);
    ui_frame();
    check("SAVE stopped: saved", project_used(0));
    memset(proj_slot, 0, sizeof proj_slot);
}

/* upstream 1.0 / 1.1.5 (#23 / #52): knob acceleration only for a turn going on in one direction over a wide range; the
 * first detents, a single quick detent (a bounce) and a reversal are one step each; at most x8 (x4 over a range of 64 or
 * less); MENU switches it off (the curve against upstream's: tests/accel_test.c) */
static int32_t acc_at(int32_t s, int32_t range, uint32_t gap_ms)
{
    host_ticks += gap_ms * 1000u * FM1_TICKS_PER_US;
    return accel(EN_K1, s, range);
}

static void test_knob_accel(void)
{
    int32_t r[6];
    uint32_t k, ok = 1;
    ui_host_init();
    settings.accel = 1;
    for (k = 0; k < 4u; k++)
        ok &= acc_at(1, 127, 100) == 1;
    check("accel: a slow turn (100 ms a detent): one step per detent", ok);
    for (k = 0; k < 6u; k++)                         /* a new turn (after a pause), fast */
        r[k] = acc_at(1, 127, k ? 8u : 300u);
    printf("     accel, a new fast turn (8 ms a detent): %d %d %d %d %d %d\n", r[0], r[1], r[2], r[3], r[4], r[5]);
    check("accel: a fast turn speeds up after its first detents, at most x8",
          r[0] == 1 && r[1] == 1 && r[5] > 1 && r[5] <= 8);
    for (k = 0; k < 6u; k++)                         /* the same over a range of 64: at most x4 */
        r[k] = acc_at(1, 64, k ? 8u : 300u);
    check("accel: over a range of 64 or less, at most x4", r[0] == 1 && r[5] > 1 && r[5] <= 4);
    check("accel: a reversal is one step", acc_at(-1, 127, 8) == -1);
    check("accel: a single quick detent after a pause (a bounce) is one step", acc_at(1, 127, 300) == 1 && acc_at(1, 127, 8) == 1);
    for (k = 0, ok = 1; k < 6u; k++)
        ok &= acc_at(1, 32, 8) == 1;
    check("accel: a small range (32 or less) is never accelerated", ok);
    settings.accel = 0;
    for (k = 0, ok = 1; k < 6u; k++)
        ok &= acc_at(1, 127, 8) == 1;
    check("accel: KNOB ACCEL OFF: one step per detent", ok);
    settings.accel = 1;
    {
        const panel_t keep = panel;
        panel.btn[1] = panel.btn[0];                 /* a calibration with one button learned twice */
        panel_init();
        check("calibration: a button learned twice falls back to the default table", !memcmp(&panel, &PANEL_DEFAULT, sizeof panel));
        panel = keep;
        panel.enc[2] = panel.enc[1];
        panel_init();
        check("calibration: a knob learned twice falls back to the default table", !memcmp(&panel, &PANEL_DEFAULT, sizeof panel));
        panel = keep;
    }
}

/* HOME's TRACKS mode: a white key is a step of the selected track (nothing armed); the mutes themselves
 * (track_mute_toggle: a mute cuts what rings; NEXT BAR waits for the bar, at once when stopped) until the MUTE screen
 * drives them; leaving TRACKS the keys play again and the mutes stay */
static void test_quick_mute(void)
{
    uint32_t a, k, ok = 1;
    ui_host_init();
    settings.mutebar = 0;
    open_tracks();
    trk[5].p[P_MUTE] = 1;
    a = hit_age(&trk[2]);
    keys(1u << KEY_TRK_KEY[2]);
    ui_frame();
    keys(0);
    ui_frame();
    check("TRACKS: white key 3 is step 3 of the selected track: no select, no mute, no sound",
          song.sel == 0 && trk[2].p[P_MUTE] == 0 && hit_age(&trk[2]) == a && trk[0].step[2].on);
    for (k = 0; k < NTRK; k++)
        ok &= led_lit(14u + KEY_TRK_KEY[k]) == trk[0].step[k].on;
    check("TRACKS: the keys show the steps, not the mutes", ok);
    drum_hit(&trk[2], 127);                          /* track 3 ringing */
    track_mute_toggle(2);
    for (k = 0; k < 5u; k++)
        ui_frame();
    check("a mute: track 3 muted, its ringing voice cut", trk[2].p[P_MUTE] == 1 && !trk[2].v[0].active &&
          !trk[2].v[1].active);
    track_mute_toggle(2);
    check("again: unmuted", trk[2].p[P_MUTE] == 0);
    settings.mutebar = 1;                            /* MUTE: NEXT BAR */
    transport_req = 1;
    seq_play_to(0, 2);
    track_mute_toggle(0);
    ui_frame();
    check("NEXT BAR while playing: the mute waits (track 1 not muted yet)", trk[0].p[P_MUTE] == 0 && (song.mute_q & 1u));
    seq_play_to(0, 15);
    check("still waiting on the bar's last step", trk[0].p[P_MUTE] == 0);
    seq_play_to(0, 0);
    check("at the bar (track 1's step 1): track 1 muted", trk[0].p[P_MUTE] == 1 && !song.mute_q);
    settings.mutebar = 0;
    track_mute_toggle(0);
    check("NOW while playing: unmuted at once", trk[0].p[P_MUTE] == 0);
    transport_req = 2;
    ui_frame();
    settings.mutebar = 1;
    track_mute_toggle(3);
    ui_frame();
    check("NEXT BAR but stopped: a mute is at once", trk[3].p[P_MUTE] == 1);
    settings.mutebar = 0;
    tap(B_HOME);                                     /* HOME's second mode: keys play again; track 6 stays muted */
    a = hit_age(&trk[0]);
    keys(1u << KEY_TRK_KEY[0]);
    ui_frame();
    keys(0);
    ui_frame();
    check("leaving TRACKS: the keys play again, the mutes stay", ui.home && hit_age(&trk[0]) != a &&
          trk[5].p[P_MUTE] == 1 && trk[0].p[P_MUTE] == 0);
}

/* FREITAKT combos (Elektron Model:Cycles style): hold a button, press a key. GLO tapped (let go within 0.4 s, no key
 * meanwhile): the GLOBAL pages, on the release; GLO + white n: pattern n, + black n (1..8) variation n (cue_test);
 * SEL + black 1..6: select; SEQ + black 1..6: mute; ARP + black 1..6: solo, + F#5 the SOLO mode. The keys pressed
 * with one of them play nothing, record nothing, toggle no step */
static uint32_t glo_bit(void) { return 1u << panel.btn[B_GLO]; }
static void btn_hold_frames(uint32_t b, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        fm1_in.buttons |= 1u << panel.btn[b];
        ui_frame();
    }
}
static void btn_key(uint32_t b, uint32_t key)          /* a key tapped while button b is held */
{
    keys(1u << key);
    btn_hold_frames(b, 2);                           /* (the render hands the key over after the UI's pass) */
    keys(0);
    btn_hold_frames(b, 1);
}
static void glo_hold_frames(uint32_t n) { btn_hold_frames(B_GLO, n); }
static void glo_key(uint32_t key) { btn_key(B_GLO, key); }
static const uint8_t SOLO_BK[NTRK] = {1, 3, 5, 8, 10, 13};   /* black keys 1..6 */
/* ARP + black 1..6: SOLO tracks 1..6 (TOGGLE: a tap on / off; MOMENTARY: while held), ARP + F#5 flips the mode (a
 * device setting, its key lit while ARP is held and MOMENTARY); the soloed keys lit while ARP is held; no sound */
static void test_arp_solo(void)
{
    uint32_t a, k, ok;
    ui_host_init();
    solo_mom = 0;
    song.solo = 0;
    press(B_ARP);
    btn_hold_frames(B_ARP, 50);
    check("ARP held: nothing soloed, its keys dark, F#5 dark (TOGGLE)", !song.solo && !led_lit(14u + 25u) &&
          !led_lit(14u + SOLO_BK[0]));
    a = hit_age(&trk[2]);
    btn_key(B_ARP, SOLO_BK[2]);
    check("SOLO TOGGLE: ARP + black key 3 solos track 3, its key lit, no sound, no select",
          song.solo == 1u << 2 && led_lit(14u + SOLO_BK[2]) && hit_age(&trk[2]) == a && song.sel == 0);
    btn_key(B_ARP, SOLO_BK[5]);
    check("SOLO TOGGLE: ARP + black key 6 adds track 6", song.solo == (1u << 2 | 1u << 5));
    ok = 1;
    for (k = 0; k < NTRK; k++)
        ok &= trk_silent(&trk[k]) == !(k == 2u || k == 5u);
    check("SOLO: only the soloed tracks are heard", ok);
    btn_key(B_ARP, SOLO_BK[2]);
    btn_key(B_ARP, SOLO_BK[5]);
    check("SOLO TOGGLE: the same keys again unsolo", song.solo == 0u && !led_lit(14u + SOLO_BK[2]));
    btn_key(B_ARP, 25u);                             /* F#5 */
    check("ARP + F#5 flips the mode to MOMENTARY (saved), its key lit, the top line says so",
          solo_mom == 1u && led_lit(14u + 25u) && str_eq(ui.msg, "SOLO: MOMENTARY"));
    keys(1u << SOLO_BK[0]);
    btn_hold_frames(B_ARP, 3);
    check("SOLO MOMENTARY: black key 1 held = track 1 soloed", song.solo == 1u);
    keys(0);
    btn_hold_frames(B_ARP, 2);
    check("SOLO MOMENTARY: let go = unsoloed", song.solo == 0u);
    keys(1u << SOLO_BK[5]);
    btn_hold_frames(B_ARP, 3);
    release_all();                                   /* ARP let go first, the key still held */
    ui_frame();
    ui_frame();
    check("SOLO MOMENTARY: ARP let go with the key held: still soloed, its LED back to the page's",
          song.solo == 1u << 5 && !led_lit(14u + 25u));
    keys(0);
    ui_frame();
    ui_frame();
    check("SOLO MOMENTARY: the key let go later: unsoloed", song.solo == 0u);
    press(B_SAVE);                                   /* (a black key with SAVE: a variation, no unsolo) */
    song.solo = 1u;
    keys(1u << SOLO_BK[0]);
    btn_hold_frames(B_SAVE, 3);
    keys(0);
    btn_hold_frames(B_SAVE, 2);
    release_all();
    ui_frame();
    check("SOLO MOMENTARY: a black key let go that ARP did not hold unsolos nothing", song.solo == 1u);
    song.solo = 0;
    press(B_ARP);
    btn_key(B_ARP, 25u);
    check("ARP + F#5 again: TOGGLE", solo_mom == 0u && !led_lit(14u + 25u) && str_eq(ui.msg, "SOLO: TOGGLE"));
    release_all();
    ui_frame();
    check("ARP let go: no page change", ui.home);
}
/* FREITAKT ARP + EDIT: ARP held, EDIT: the selected track's ARP page (MODE RATE GATE FADE), again: ARP 2/2 (DIR CHORD
 * RANGE, a pitched model's), again: back to 1/2; a drum's stays on 1/2; the SOLO keys still work with it */
static void test_arp_edit_page(void)
{
    const page_t *pg;
    uint32_t k, n;
    ui_host_init();
    old_set_model(&trk[0], DM_ACID);
    song.sel = 0;
    press(B_ARP);
    btn_hold_frames(B_ARP, 3);
    press(B_EDIT);
    fm1_in.buttons |= 1u << panel.btn[B_ARP];
    ui_frame();
    fm1_in.buttons = 1u << panel.btn[B_ARP];
    ui_frame();
    pg = cur_page();
    check("ARP + EDIT: the ARP page (MODE RATE GATE FADE)", !ui.home && str_eq(pg->title, "ARP") &&
          page_id(pg, 0) == P_AMODE && page_id(pg, 1) == P_ARATE && page_id(pg, 2) == P_AGATE && page_id(pg, 3) == P_AFADE);
    {
        static const char *const MV[4] = {"OFF", "RTRG", "ARP1", "ARP2"};
        char l[16], v[16], pv[16];
        const char *u;
        uint32_t i, ok = 1;
        for (i = 1; i < 6u; i++) {                   /* MODE: OFF RTRG ARP1 ARP2, at its end stays */
            uint32_t w = i < 4u ? i : 3u;
            turn(EN_K1, 1);
            ui_frame();
            kp_text(l, v, &u);
            param_format(track_desc(&trk[0], P_AMODE), trk[0].p[P_AMODE], pv, &u);
            ok &= kp.on && str_eq(l, "MODE") && str_eq(v, MV[w]) && str_eq(pv, MV[w]) && trk[0].p[P_AMODE] == (int16_t)w;
        }
        check("ARP page KNOB 1 (ACID): MODE OFF RTRG ARP1 ARP2 on the knob's value and its popup", ok);
        trk[0].p[P_AMODE] = 2;                       /* the stored 2 (the ARP before): ARP1 */
        param_format(track_desc(&trk[0], P_AMODE), 2, pv, &u);
        check("MODE's stored 2 (ARP before ARP2) shows ARP1", str_eq(pv, "ARP1"));
        trk[0].p[P_AMODE] = 0;
        release_all();
        ui_frame();
    }
    press(B_EDIT);
    fm1_in.buttons |= 1u << panel.btn[B_ARP];
    ui_frame();
    fm1_in.buttons = 1u << panel.btn[B_ARP];
    ui_frame();
    pg = cur_page();
    n = fam_pages(FAM_SND, &k);
    check("ARP + EDIT again (ACID): ARP 2/2, DIR CHORD RANGE", page_id(pg, 0) == P_ADIR && page_id(pg, 1) == P_ACHORD &&
          page_id(pg, 2) == P_ARANGE && k == n);
    btn_key(B_ARP, SOLO_BK[1]);
    check("ARP + black key 2 on the ARP page: still SOLO (track 2), the page stays", song.solo == 1u << 1 &&
          page_id(cur_page(), 0) == P_ADIR);
    btn_key(B_ARP, SOLO_BK[1]);
    release_all();
    ui_frame();
    ui_frame();
    check("ARP let go after ARP + EDIT: the page stays, nothing soloed", page_id(cur_page(), 0) == P_ADIR && !song.solo);
    turn(EN_K1, 2);
    ui_frame();
    check("ARP page KNOB 1 (DIR) turns the track's DIR", trk[0].p[P_ADIR] == 2);
    old_set_model(&trk[0], DM_K808);
    ui_frame();
    check("a drum: ARP 2/2 hidden, the ARP page 1/2 shown", page_id(cur_page(), 0) == P_AMODE);
    turn(EN_K1, 5);
    ui_frame();
    check("a drum: MODE goes OFF / RTRG only", trk[0].p[P_AMODE] == 1);
    press(B_EDIT);
    fm1_in.buttons |= 1u << panel.btn[B_ARP];
    ui_frame();
    release_all();
    ui_frame();
    check("ARP + EDIT on a drum: stays on ARP 1/2", page_id(cur_page(), 0) == P_AMODE);
}
/* SEL + black 1..6: track 1..6 selected (on every page), the selected one's black key lit while SEL is held; a SEL
 * tap with no key: KEYS on / off, on the release */
static void test_sel_select(void)
{
    uint32_t a, k, ok = 1;
    ui_host_init();
    tap(B_EDIT);                                     /* (any page: SOUND) */
    a = hit_age(&trk[3]);
    press(B_SCL);
    btn_hold_frames(B_SCL, 2);
    check("SEL held: track 1's black key lit", led_lit(14u + SOLO_BK[0]) && !led_lit(14u + SOLO_BK[3]));
    btn_key(B_SCL, SOLO_BK[3]);
    check("SEL + black key 4: track 4 selected, its key lit, no sound", song.sel == 3u && led_lit(14u + SOLO_BK[3]) &&
          !led_lit(14u + SOLO_BK[0]) && hit_age(&trk[3]) == a);
    for (k = 0; k < NTRK; k++) {
        btn_key(B_SCL, SOLO_BK[k]);
        ok &= song.sel == k;
    }
    btn_key(B_SCL, 20u);                             /* (C#5: nothing) */
    check("SEL + black keys 1..6 select tracks 1..6; other keys nothing", ok && song.sel == 5u);
    release_all();
    ui_frame();
    check("SEL let go after a key: KEYS stays off, the page stays", !ui.keys && !ui.home && cur_fam() == FAM_SND);
    tap(B_SCL);
    check("SEL tapped alone: KEYS on", ui.keys);
    a = hit_age(&trk[5]);
    press(B_SCL);
    btn_key(B_SCL, SOLO_BK[1]);
    release_all();
    ui_frame();
    check("KEYS: SEL + black key 2 selects track 2, plays no note, KEYS stays on", song.sel == 1u && ui.keys &&
          !kz.key && hit_age(&trk[5]) == a);
    tap(B_SCL);
    check("SEL tapped again: KEYS off", !ui.keys);
    press(B_SCL);
    btn_hold_frames(B_SCL, 2000);
    release_all();
    ui_frame();
    check("SEL held 2 s alone, let go: still a KEYS toggle (no key pressed)", ui.keys);
    tap(B_SCL);
    ui.menu = 1;
    ui.force = 1;
    press(B_SCL);
    btn_key(B_SCL, SOLO_BK[2]);
    release_all();
    ui_frame();
    check("menu: SEL dead (no select, no KEYS)", song.sel == 1u && !ui.keys);
    ui.menu = 0;
    ui.force = 1;
    ui_frame();
}
/* SEQ + black 1..6: tracks 1..6 muted / unmuted (MUTE NOW / NEXT BAR), no TOOLS, no page turn; the audible tracks'
 * black keys lit while SEQ is held (a mute waiting for the bar blinks); SEQ + a white key, or held 0.4 s: TOOLS */
static void test_seq_mute(void)
{
    uint32_t a, k, ok, l1, l2;
    step_t s0;
    ui_host_init();
    settings.mutebar = 0;
    kit_session();                                   /* track 6 muted */
    song.rec = 0;
    a = hit_age(&trk[2]);
    press(B_SEQ);
    btn_hold_frames(B_SEQ, 2);
    ok = 1;
    for (k = 0; k < NTRK; k++)
        ok &= led_lit(14u + SOLO_BK[k]) == !trk[k].p[P_MUTE];
    check("SEQ held: the audible tracks' black keys lit (track 6 muted: dark)", ok);
    btn_key(B_SEQ, SOLO_BK[2]);
    check("SEQ + black key 3 mutes track 3; no sound, no select", trk[2].p[P_MUTE] == 1 &&
          hit_age(&trk[2]) == a && song.sel == 0 && !led_lit(14u + SOLO_BK[2]));
    btn_key(B_SEQ, SOLO_BK[5]);
    check("SEQ + black key 6 unmutes track 6", trk[5].p[P_MUTE] == 0 && led_lit(14u + SOLO_BK[5]));
    btn_hold_frames(B_SEQ, 600);
    s0 = trk[0].step[0];
    btn_key(B_SEQ, STEP_KEY[0]);
    check("SEQ held past 0.4 s after a black key, a white key: nothing (no step, no mute)",
          !memcmp(&trk[0].step[0], &s0, sizeof s0) && trk[0].p[P_MUTE] == 0);
    release_all();
    ui_frame();
    check("SEQ let go: no page turn (HOME still)", ui.home);
    settings.mutebar = 1;                            /* NEXT BAR */
    transport_req = 1;
    seq_play_to(0, 2);
    press(B_SEQ);
    btn_key(B_SEQ, SOLO_BK[0]);
    check("NEXT BAR while playing: track 1's mute waits for the bar", trk[0].p[P_MUTE] == 0 && (song.mute_q & 1u));
    host_ticks += 250u * 1000u * FM1_TICKS_PER_US;
    btn_hold_frames(B_SEQ, 1);
    l1 = led_lit(14u + SOLO_BK[0]);
    host_ticks += 250u * 1000u * FM1_TICKS_PER_US;
    btn_hold_frames(B_SEQ, 1);
    l2 = led_lit(14u + SOLO_BK[0]);
    check("the waiting mute's key blinks (250 ms)", l1 != l2);
    seq_play_to(0, 0);
    check("at the bar: track 1 muted", trk[0].p[P_MUTE] == 1 && !song.mute_q);
    release_all();
    ui_frame();
    transport_req = 2;
    ui_frame();
    settings.mutebar = 0;
    trk[0].p[P_MUTE] = trk[2].p[P_MUTE] = 0;
    press(B_SEQ);                                    /* the black key as SEQ is let go: still the mutes */
    keys(1u << SOLO_BK[1]);
    ui_frame();
    release_all();
    ui_frame();
    keys(0);
    ui_frame();
    check("a black key struck as SEQ went: track 2 muted, no page turn", trk[1].p[P_MUTE] == 1 && ui.home);
    trk[1].p[P_MUTE] = 0;
    press(B_SEQ);
    btn_key(B_SEQ, STEP_KEY[0]);
    check("SEQ + a white key: nothing (no TOOLS screen since phase C, no step, no note)", ui.home &&
          !memcmp(&trk[0].step[0], &s0, sizeof s0) && trk[0].p[P_MUTE] == 0);
    btn_key(B_SEQ, SOLO_BK[0]);
    check("then black key 1 still mutes track 1", trk[0].p[P_MUTE] == 1);
    release_all();
    ui_frame();
    check("SEQ let go after its keys: no page turn", ui.home);
    trk[0].p[P_MUTE] = 0;
    press(B_SEQ);
    btn_hold_frames(B_SEQ, 500);
    check("SEQ held 0.5 s alone: nothing while held", ui.home);
    btn_key(B_SEQ, SOLO_BK[3]);
    check("SEQ held 0.5 s, then black key 4: track 4 muted", trk[3].p[P_MUTE] == 1);
    release_all();
    ui_frame();
    trk[3].p[P_MUTE] = 0;
    press(B_SEQ);
    btn_hold_frames(B_SEQ, 800);
    release_all();
    ui_frame();
    check("SEQ held 0.8 s alone, let go: the SEQ pages (the hold is a tap now)", !ui.home && cur_fam() == FAM_SEQ);
    tap(B_HOME);
    tap(B_SEQ);
    check("a SEQ tap still turns the SEQ pages", !ui.home && cur_fam() == FAM_SEQ);
}
/* GLO: a tap opens GLOBAL; held alone or with a key: no page; the keys with GLO never play or record or edit a step;
 * GLO let go with a key held leaves nothing stuck */
/* GLO + black n (1..8): variation n of the playing pattern queued; C#5 / D#5 (black 9 / 10) nothing; a black key with
 * no white before it: no chain. GLO held: black 1..8 the playing variation lit, a queued one blinking, a used one dim
 * (the mid layer, BACKLIGHT OFF too), an unused one dark (cue_test: the timing, the chain's entries, A1 B3 A2 played) */
static uint32_t gv_lay(uint32_t key)                  /* 0 dark, 1 lit, 2 dim */
{
    uint8_t q = led_pos[14u + key];
    if (q == 0xFF)
        return 0;
    if (!led_dim_on)
        return (fm1_led[q >> 3] >> (q & 7u)) & 1u;
    return (led_lay[q >> 3] >> (q & 7u)) & 1u ? 1u : (led_lay[q >> 3] >> (8u + (q & 7u))) & 1u ? 2u : 0u;
}
static int home_line_is(const char *s);
static void test_glo_vars(void)
{
    static const uint8_t BK8[8] = {1, 3, 5, 8, 10, 13, 15, 17};
    static step_t vs[NTRK][NSTEP];
    static int16_t vt[NTRK][4];
    const step_t *vp[NTRK];
    uint32_t k, j, i, on = 0, off = 0;
    int ok = 1;
    ui_host_init();
    backlight = BL_OFF;
    trk[0].step[0].on = 1;
    project_save(0);
    project_save(1);
    project_load(0);
    for (k = 0; k < NTRK; k++) {                     /* variation 4 of A: used */
        memcpy(vs[k], trk[k].step, sizeof vs[k]);
        for (j = 0; j < PAT_NT; j++)
            vt[k][j] = trk[k].p[PAT_TIMING[j]];
        vp[k] = vs[k];
    }
    vs[0][5].on = 1;
    OK0(var_put(&var_buf[var_play], 3, vp, vt) == 0);
    ui.home = 1;
    transport_req = 1;
    seq_play_to(0, 2);
    press(B_GLO);
    glo_hold_frames(2);
    OK0(gv_lay(BK8[0]) == 1u && gv_lay(BK8[3]) == 2u && gv_lay(BK8[1]) == 0u && gv_lay(BK8[7]) == 0u);
    OK0(led_lit(14u + STEP_KEY[0]) && !led_lit(14u + STEP_KEY[1]));
    check("GLO held: black 1 (the playing variation) lit, black 4 (used) dim, the unused ones dark; white 1 (A) lit",
          ok);
    ok = 1;
    glo_key(22u);                                     /* D#5, C#5: nothing */
    glo_key(20u);
    OK0(!cue.q_var && !cue.q_slot && var_cur == 0u);
    glo_key(BK8[5]);                                  /* black 6: variation 6 queued */
    OK0(cue.q_var == 6u && !cue.q_slot);
    for (i = 0; i < 12u; i++) {                       /* .. its key blinks, black 1 stays lit, black 4 dim */
        host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
        glo_hold_frames(1);
        on += gv_lay(BK8[5]) == 1u;
        off += gv_lay(BK8[5]) == 0u;
        OK0(gv_lay(BK8[0]) == 1u && gv_lay(BK8[3]) == 2u);
    }
    OK0(on && off);
    release_all();
    ui_frame();
    ui_frame();
    OK0(!chain.n && cue.q_var == 6u && ui.home);
    OK0(gv_lay(BK8[3]) == 0u && gv_lay(BK8[5]) == 0u);   /* GLO let go: not shown */
    check("GLO + C#5 / D#5: nothing; GLO + black 6: variation 6 queued (its key blinking), no chain, no GLOBAL page",
          ok);
    ok = 1;
    for (k = 0; k < 4000u && cue.q_var; k++)
        ui_frame();
    OK0(var_cur == 5u);
    ui_frame();
    check("HOME: the queued variation plays at the pattern end, V6 on the status line", ok && home_line_is("P1 V6 T1 L1"));
    transport_req = 2;
    ui_frame();
    ui_frame();
}
static void test_glo_keys(void)
{
    uint32_t a, sel, n, k;
    step_t s0;
    ui_host_init();
    press(B_GLO);
    ui_frame();
    check("GLO pressed: nothing yet (GLOBAL waits for the release)", ui.home);
    release_all();
    ui_frame();
    check("GLO tapped: GLOBAL 1/3 on the release", !ui.home && cur_fam() == FAM_GLO && ui.page == page_first(FAM_GLO));
    tap(B_GLO);
    tap(B_GLO);
    check("GLO taps go on: SYSTEM 2/3, GLOBAL 3/3", !ui.home && ui.page == page_first(FAM_GLO) + 2u);
    tap(B_HOME);
    press(B_GLO);
    glo_hold_frames(600);
    release_all();
    ui_frame();
    check("GLO held 0.6 s alone: no GLOBAL page (not a tap)", ui.home);
    a = hit_age(&trk[4]);
    sel = song.sel;
    press(B_GLO);
    keys(1u << KEY_TRK_KEY[4]);                      /* GLO and a white key in the same frame */
    glo_hold_frames(2);
    check("GLO + a key in the same frame: no sound, no select", hit_age(&trk[4]) == a && song.sel == sel);
    release_all();                                   /* GLO let go, the key still held */
    ui_frame();
    ui_frame();
    check("GLO let go with the key held: no GLOBAL page, still no sound",
          ui.home && hit_age(&trk[4]) == a && (kb_mute >> KEY_TRK_KEY[4]) & 1u);
    keys(0);
    ui_frame();
    check("the key let go: nothing held, no sound", !kb_mute && hit_age(&trk[4]) == a);
    ui_frame();
    a = hit_age(&trk[0]);                            /* the ISR before the UI's pass sees GLO */
    fm1_in.buttons |= glo_bit();
    keys(1u << KEY_TRK_KEY[0]);
    render_mix(0, 0, CTL);
    check("GLO + a key before the UI's pass: the key is GLO's, no note", (kb_mute & 1u) && hit_age(&trk[0]) == a);
    keys(0);
    release_all();
    ui_frame();
    ui_frame();
    tap(B_SCL);                                      /* KEYS on: the keys play the selected track */
    a = hit_age(&trk[0]);
    press(B_GLO);
    keys(1u << 3);
    glo_hold_frames(2);
    keys(0);
    glo_hold_frames(1);
    check("KEYS: GLO + a key plays no note", hit_age(&trk[0]) == a && !kz.key);
    release_all();
    ui_frame();
    tap(B_SCL);                                      /* KEYS off */
    open_tracks();
    tap(B_REC);                                      /* track 1 armed, playing: the keys play and record */
    seq_play_to(0, 3);
    for (k = 0, n = 0; k < 16u; k++)
        n += trk[0].step[k].on;
    a = hit_age(&trk[0]);
    press(B_GLO);
    glo_key(KEY_TRK_KEY[0]);
    for (k = 0, sel = 0; k < 16u; k++)
        sel += trk[0].step[k].on;
    check("TRACKS armed: GLO + key 1 records nothing, plays nothing", sel == n && hit_age(&trk[0]) == a);
    release_all();
    ui_frame();
    check("GLO let go: TRACKS again, still armed", mix_mode() && song.rec == 1u);
    tap(B_REC);
    transport_req = 2;
    ui_frame();
    seq_open("PATTERN");
    s0 = TSEL->step[0];
    press(B_GLO);
    glo_key(0);
    release_all();
    ui_frame();
    check("PATTERN: GLO + key 1 toggles no step", TSEL->step[0].on == s0.on);
    seq_open("PATTERN");
    s0 = TSEL->step[1];
    press(B_ARP);
    btn_key(B_ARP, STEP_KEY[1]);
    release_all();
    ui_frame();
    press(B_SCL);
    btn_key(B_SCL, STEP_KEY[1]);
    release_all();
    ui_frame();
    check("PATTERN: ARP / SEL + key 2 toggle no step", TSEL->step[1].on == s0.on && !ui.keys);
}

/* ---- SEQ (FREITAKT phase C: the TOOLS screen gone): a tap or a hold turns the SEQ pages on the release; SEQ + black
 * 1..6 the mutes (test_seq_mute); the keys pressed with SEQ are its own until let go: no note, no MIDI, nothing
 * recorded, a white key nothing */
static uint32_t seq_bit(void) { return 1u << panel.btn[B_SEQ]; }
static void seq_hold_frames(uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        fm1_in.buttons |= seq_bit();
        ui_frame();
    }
}
static void test_seq_button(void)
{
    uint32_t a, mw, k, on = 0;
    ui_host_init();
    press(B_SEQ);
    ui_frame();
    check("SEQ pressed: nothing yet (the SEQ pages wait for the release)", ui.home);
    release_all();
    ui_frame();
    check("SEQ tapped: PATTERN on the release", !ui.home && str_eq(cur_page()->title, "PATTERN"));
    tap(B_HOME);
    press(B_SEQ);
    seq_hold_frames(600);
    check("SEQ held 0.6 s: no screen over HOME (no TOOLS)", ui.home);
    release_all();
    ui_frame();
    check("SEQ held 0.6 s, let go: PATTERN (a hold is a tap)", !ui.home && str_eq(cur_page()->title, "PATTERN"));
    tap(B_HOME);
    song.rec = REC_ALL_MASK;                         /* REC ALL, playing: the keys would play and record */
    transport_req = 1;
    ui_frame();
    a = hit_age(&trk[0]);
    mw = mo_w;
    press(B_SEQ);
    keys(1u << KEY_TRK_KEY[0]);                      /* SEQ and white key 1 in the same frame */
    seq_hold_frames(2);
    release_all();                                   /* SEQ let go, the key still held */
    ui_frame();
    ui_frame();
    check("SEQ + a white key, SEQ let go first: no SEQ page (not a tap), the key still SEQ's", ui.home &&
          (kb_mute & 1u));
    keys(0);
    ui_frame();
    for (k = 0; k < NSTEP; k++)
        on |= trk[0].step[k].on;
    check("SEQ + a white key: no hit, no MIDI note out, nothing recorded, no step",
          !kb_mute && hit_age(&trk[0]) == a && mo_w == mw && !on);
    a = hit_age(&trk[1]);                            /* the ISR before the UI's pass sees SEQ */
    fm1_in.buttons |= seq_bit();
    keys(1u << KEY_TRK_KEY[1]);
    render_mix(0, 0, CTL);
    check("SEQ + a key before the UI's pass: the key is SEQ's, no note", (kb_mute >> KEY_TRK_KEY[1]) & 1u &&
          hit_age(&trk[1]) == a && mo_w == mw);
    keys(0);
    release_all();
    ui_frame();
    ui_frame();
    transport_req = 2;
    ui_frame();
    song.rec = 0;
}

/* steps and their locks: a track's whole pattern compared, a step filled */
static uint32_t step_locks(uint32_t k, uint32_t s, uint8_t *id, int8_t *v)   /* track k's locks at step s, in order */
{
    uint32_t i = lk_seek(&mo.b, k, s), n = 0;
    for (; i < mo.b.count && (uint32_t)(mo.b.key[i] >> 7) == (k << 6 | s); i++)
        if (lk_is_lock(&mo.b, i)) {
            if (id)
                id[n] = (uint8_t)(mo.b.key[i] & 127u);
            if (v)
                v[n] = mo.b.val[i];
            n++;
        }
    return n;
}
static int same_step(uint32_t ka, uint32_t sa, uint32_t kb, uint32_t sb)   /* the steps (all 4 B) and their locks */
{
    uint8_t ia[P_COUNT], ib[P_COUNT];
    int8_t va[P_COUNT], vb[P_COUNT];
    uint32_t na = step_locks(ka, sa, ia, va), nb = step_locks(kb, sb, ib, vb);
    return !memcmp(&trk[ka].step[sa], &trk[kb].step[sb], sizeof(step_t)) && na == nb && !memcmp(ia, ib, na) &&
           !memcmp(va, vb, na);
}
static void rich_step(uint32_t k, uint32_t s, uint32_t seed)   /* a step with everything set, two locks */
{
    step_t *p = &trk[k].step[s];
    memset(p, 0, sizeof *p);
    p->on = 1;
    p->acc = seed & 1u;
    p->slide = (seed >> 1) & 1u;
    p->tie = (seed >> 2) & 1u;
    p->rat = seed % 4u;
    p->cond = (uint8_t)(seed % 57u);
    p->nt = (int8_t)(seed % 20u) - 5;
    ((uint8_t *)p)[3] = (uint8_t)(seed * 7u + 1u);     /* the step's spare byte: the micro (spec §1) */
    lock_set(k, s, P_E1, (int32_t)(seed % 100u));
    lock_set(k, s, P_FCUT, (int32_t)(seed % 120u) + 1);
}
static int track_same(uint32_t ka, uint32_t kb)      /* every step and lock, LEN DIV SWG */
{
    uint32_t s;
    int ok = trk[ka].p[P_SLEN] == trk[kb].p[P_SLEN] && trk[ka].p[P_SDIV] == trk[kb].p[P_SDIV] &&
             trk[ka].p[P_SSWING] == trk[kb].p[P_SSWING];
    for (s = 0; s < NSTEP; s++)
        ok &= same_step(ka, s, kb, s);
    return ok;
}
static int track_empty(uint32_t k)                   /* no step set, no lock */
{
    static const step_t z;
    uint32_t s;
    int ok = lock_count(k) == 0u;
    for (s = 0; s < NSTEP; s++)
        ok &= !memcmp(&trk[k].step[s], &z, sizeof z);
    return ok;
}
static void rich_session(void)                       /* tracks 1, 3, 5: rich steps with locks, LEN DIV SWG set */
{
    uint32_t k, s;
    memset(&mo, 0, sizeof mo);
    for (k = 0; k < NTRK; k++)
        memset(trk[k].step, 0, sizeof trk[k].step);
    for (k = 0; k < NTRK; k += 2u) {
        for (s = k; s < NSTEP; s += 5u + k)
            rich_step(k, s, k * 64u + s + 1u);
        trk[k].p[P_SLEN] = (int16_t)(24 + 8 * (int32_t)k);
        trk[k].p[P_SDIV] = (int16_t)(1 + k / 2u);
        trk[k].p[P_SSWING] = (int16_t)(10 + 5 * (int32_t)k);
    }
}

/* ---- REC + SEL held 5 s (FREITAKT phase C; the CLEAR TRACK dialog gone): the selected track cleared (steps, locks),
 * the warning CLEAR T3 with its countdown 5..1 while held; either let go early: nothing changed; neither button acts on
 * its own (no REC tap / REC ALL / FILL / arm, no KEYS toggle, no track select), whichever was pressed first */
static uint32_t rs_bits(void) { return 1u << panel.btn[B_REC] | 1u << panel.btn[B_SCL]; }
static void rs_hold_ms(uint32_t held, uint32_t ms)    /* the buttons in held down for ms (frames of 50 ms) */
{
    uint32_t t;
    for (t = 0; t < ms; t += 50u) {
        host_ticks += 50u * 1000u * FM1_TICKS_PER_US;
        fm1_in.buttons |= held;
        ui_frame();
    }
}
static void test_clr_combo(void)
{
    static track_t snap[NTRK];
    static lk_bank_t bsnap;
    uint32_t k, ok, n, last, seq[8], lc[NTRK], t0, t1 = 0;
    uint8_t keys0;
    /* SEL first, then REC: the countdown, then the clear */
    ui_host_init();
    rich_session();
    song.sel = 2;
    trk[2].step[1].on = 1;
    lock_set(2, 1, P_E2, 9);                         /* (track 3: a step with a lock) */
    memcpy(snap, trk, sizeof snap);
    for (k = 0; k < NTRK; k++)
        lc[k] = lock_count(k);
    keys0 = ui.keys;
    press(B_SCL);
    ui_frame();
    rs_hold_ms(1u << panel.btn[B_SCL], 300);
    press(B_REC);
    fm1_in.buttons |= rs_bits();
    ui_frame();
    check("REC + SEL (SEL first): the warning at once, 5", ui.clr == 5u && ui.clr_trk == 2u && !kp.on);
    t0 = host_ticks;
    for (n = 0, last = 0, ok = 1, k = 0; k < 200u && ui.clr; k++) {
        if (ui.clr != last && n < 8u)
            seq[n++] = last = ui.clr;
        ok &= !fill_on && song.rec == 0u && !ui.rec_all && !track_empty(2);
        t1 = host_ticks;
        rs_hold_ms(rs_bits(), 50);
    }
    check("REC + SEL: the countdown 5 4 3 2 1, never FILL, REC ALL or an arm, the track whole until then",
          ok && n == 5u && seq[0] == 5u && seq[1] == 4u && seq[2] == 3u && seq[3] == 2u && seq[4] == 1u);
    check("REC + SEL: cleared after 5 s (not before 4.9 s)", t1 - t0 >= 4900u * 1000u * FM1_TICKS_PER_US &&
          t1 - t0 < 5100u * 1000u * FM1_TICKS_PER_US);
    ok = track_empty(2) && !ui.clr && str_eq(ui.msg, "CLEARED T3");
    for (k = 0; k < NTRK; k++)
        if (k != 2u)
            ok &= !memcmp(trk[k].step, snap[k].step, sizeof snap[k].step) && lock_count(k) == lc[k];
    check("REC + SEL held 5 s: track 3's steps and locks gone, CLEARED T3, the other tracks untouched",
          ok && lc[2] > 0u && lc[0] > 0u);
    n = lock_count(0);
    rs_hold_ms(rs_bits(), 1000);
    release_all();
    ui_frame();
    ui_frame();
    check("REC + SEL let go after the clear: no KEYS toggle, no REC ALL, no arm, no select, nothing else cleared",
          ui.keys == keys0 && !ui.rec_all && song.rec == 0u && song.sel == 2u && lock_count(0) == n && n > 0u);

    /* REC first (0.2 s), then SEL; REC let go at 2.5 s: cancelled */
    ui_host_init();
    rich_session();
    song.sel = 4;
    memcpy(snap, trk, sizeof snap);
    bsnap = mo.b;
    tap(B_FX);                                       /* (off SEQ: a REC hold would FILL) */
    rec_hold_ms(200);
    press(B_SCL);
    fm1_in.buttons |= rs_bits();
    ui_frame();
    check("REC first, then SEL: the warning (CLEAR T5)", ui.clr == 5u && ui.clr_trk == 4u);
    rs_hold_ms(rs_bits(), 2500);
    check("REC + SEL 2.5 s: 3 left, no FILL", ui.clr == 3u && !fill_on);
    fm1_in.buttons = 1u << panel.btn[B_SCL];         /* REC let go */
    ui_frame();
    check("REC let go early: cancelled, the screen back, nothing changed", !ui.clr &&
          !memcmp(trk, snap, sizeof snap) && !memcmp(&mo.b, &bsnap, sizeof bsnap) && !str_eq(ui.msg, "CLEARED T5"));
    rs_hold_ms(1u << panel.btn[B_SCL], 3000);
    release_all();
    ui_frame();
    check("REC first: REC's release no tap (no REC ALL), SEL's no KEYS toggle, no FILL",
          !ui.rec_all && song.rec == 0u && !ui.keys && !fill_on && !memcmp(trk, snap, sizeof snap));

    /* REC held 0.6 s off SEQ (FILL), then SEL: the FILL stops; SEL let go at 1 s: cancelled */
    rec_hold_ms(600);
    check("(REC held 0.6 s: FILL)", fill_on);
    press(B_SCL);
    fm1_in.buttons |= rs_bits();
    ui_frame();
    check("SEL pressed during a FILL: the FILL stops, the warning", !fill_on && ui.clr == 5u);
    rs_hold_ms(rs_bits(), 1000);
    fm1_in.buttons = 1u << panel.btn[B_REC];         /* SEL let go */
    ui_frame();
    check("SEL let go early: cancelled, nothing changed", !ui.clr && !memcmp(trk, snap, sizeof snap) &&
          !memcmp(&mo.b, &bsnap, sizeof bsnap));
    rec_hold_ms(1000);
    check("REC still held after SEL went: no FILL again", !fill_on);
    release_all();
    ui_frame();
    check("REC let go: no tap (no REC ALL), no KEYS toggle", !ui.rec_all && song.rec == 0u && !ui.keys);

    /* SEL held with a black key first (a select), then REC: the second press takes both */
    track_select(0);
    press(B_SCL);
    ui_frame();
    btn_key(B_SCL, SOLO_BK[1]);
    check("(SEL + black 2: track 2 selected)", song.sel == 1u);
    press(B_REC);
    fm1_in.buttons |= rs_bits();
    ui_frame();
    keys(1u << SOLO_BK[3]);
    rs_hold_ms(rs_bits(), 100);
    keys(0);
    rs_hold_ms(rs_bits(), 100);
    check("REC + SEL: a black key under the warning selects nothing", song.sel == 1u && ui.clr == 5u &&
          ui.clr_trk == 1u);
    release_all();
    ui_frame();
    check("let go: nothing armed, nothing cleared", !ui.rec_all && song.rec == 0u && !memcmp(trk, snap, sizeof snap));

    /* the knob popup up: the warning closes it, OCT- under the warning undoes nothing */
    tap(B_EDIT);
    k = (uint32_t)TSEL->p[page_id(cur_page(), 1)];
    turn(EN_K2, 3);
    ui_frame();
    n = (uint32_t)TSEL->p[page_id(cur_page(), 1)];
    check("(the knob popup up, the value moved)", kp_live() && n != k);
    press(B_REC);
    ui_frame();
    press(B_SCL);
    fm1_in.buttons |= rs_bits();
    ui_frame();
    check("REC + SEL: the knob popup closed, the warning up", !kp.on && ui.clr == 5u);
    press(B_OCTDN);
    fm1_in.buttons |= rs_bits();
    ui_frame();
    rs_hold_ms(rs_bits(), 100);
    check("OCT- under the warning: no undo", (uint32_t)TSEL->p[page_id(cur_page(), 1)] == n && ui.clr == 5u);
    release_all();
    ui_frame();
    check("let go: cancelled, the page back", !ui.clr && !kp.on);

    /* in the menu: dead */
    ui_host_init();
    rich_session();
    memcpy(snap, trk, sizeof snap);
    hold(B_HOME);
    check("(the menu)", ui.menu);
    press(B_REC);
    press(B_SCL);
    ui_frame();
    rs_hold_ms(rs_bits(), 5500);
    check("REC + SEL in the menu: no warning, nothing cleared", !ui.clr && !memcmp(trk, snap, sizeof snap));
    release_all();
    ui_frame();
}

/* ---- copy / paste (FREITAKT phase C: Model:Cycles style combos, no screen): SAVE + REC copies the selected track,
 * SAVE + PLAY pastes it; GLO + REC copies the pattern, GLO + PLAY pastes it. No transport, no REC (tap / FILL / arm),
 * no projects page, no GLOBAL page, no cue map */
static void mod_combo(uint32_t mod, uint32_t b)       /* mod held, b tapped under it, then mod let go */
{
    press(mod);
    ui_frame();
    press(b);
    ui_frame();
    fm1_in.buttons = 1u << panel.btn[mod];
    ui_frame();
    release_all();
    ui_frame();
}
static void test_copy_paste_combos(void)
{
    static track_t snap[NTRK];
    static lk_bank_t bsnap;
    uint32_t k, ok, pg;
    ui_host_init();
    rich_session();
    clip_kind = CB_NONE;
    track_select(3);
    memcpy(snap, trk, sizeof snap);
    mod_combo(B_SAVE, B_PLAY);
    check("SAVE + PLAY with nothing copied: NO TRACK COPIED, nothing changed, no start",
          str_eq(ui.msg, "NO TRACK COPIED") && !memcmp(trk, snap, sizeof snap) && !song.playing && !transport_req);
    track_select(0);
    mod_combo(B_SAVE, B_REC);
    check("SAVE + REC: COPIED T1; no REC ALL, no arm, no FILL, no projects page, no cue map",
          str_eq(ui.msg, "COPIED T1") && !ui.rec_all && song.rec == 0u && !fill_on && ui.home && !ui.cue_map &&
              !memcmp(trk, snap, sizeof snap));
    track_select(3);
    mod_combo(B_SAVE, B_PLAY);
    ok = track_same(3, 0);
    for (k = 0; k < NTRK; k++)
        if (k != 3u)
            ok &= !memcmp(trk[k].step, snap[k].step, sizeof snap[k].step);
    check("SAVE + PLAY: track 1 pasted onto track 4 (steps, locks, LEN DIV SWG), PASTED T4, the others untouched",
          ok && str_eq(ui.msg, "PASTED T4"));
    check("SAVE + PLAY: no start, no projects page", !song.playing && !transport_req && ui.home && !ui.cue_map);
    transport_req = 1;                               /* playing: a paste neither stops it */
    ui_frame();
    track_select(5);
    mod_combo(B_SAVE, B_PLAY);
    check("SAVE + PLAY while playing: pasted onto track 6, still playing", track_same(5, 0) && song.playing &&
          transport_req != 2u);
    transport_req = 2;
    ui_frame();
    press(B_SAVE);                                   /* SAVE held 0.5 s (the cue map), then REC */
    btn_hold_frames(B_SAVE, 1);
    host_ticks += 500u * 1000u * FM1_TICKS_PER_US;
    btn_hold_frames(B_SAVE, 1);
    check("(SAVE held 0.5 s: the cue map)", ui.cue_map);
    track_select(2);
    press(B_REC);
    ui_frame();
    fm1_in.buttons = 1u << panel.btn[B_SAVE];
    ui_frame();
    release_all();
    ui_frame();
    check("SAVE held (the cue map) + REC: COPIED T3, no REC ALL; let go: the map gone, no projects page",
          str_eq(ui.msg, "COPIED T3") && !ui.rec_all && song.rec == 0u && !ui.cue_map && ui.home);

    /* the pattern */
    ui_host_init();
    rich_session();
    memcpy(snap, trk, sizeof snap);
    bsnap = mo.b;
    mod_combo(B_GLO, B_REC);
    check("GLO + REC: COPIED PATTERN; no REC ALL, no FILL, no GLOBAL page", str_eq(ui.msg, "COPIED PATTERN") &&
          !ui.rec_all && song.rec == 0u && !fill_on && ui.home);
    for (k = 0; k < NTRK; k++)
        track_clear(&trk[k]);
    trk[1].step[7].on = 1;
    trk[0].p[P_SLEN] = 64;
    pg = ui.page;
    mod_combo(B_GLO, B_PLAY);
    ok = 1;
    for (k = 0; k < NTRK; k++)
        ok &= !memcmp(trk[k].step, snap[k].step, sizeof snap[k].step) && trk[k].p[P_SLEN] == snap[k].p[P_SLEN] &&
              trk[k].p[P_SDIV] == snap[k].p[P_SDIV] && trk[k].p[P_SSWING] == snap[k].p[P_SSWING];
    ok &= mo.b.count == bsnap.count && !memcmp(mo.b.key, bsnap.key, sizeof bsnap.key) &&
          !memcmp(mo.b.val, bsnap.val, sizeof bsnap.val);
    check("GLO + PLAY: the pattern back (every track's steps, locks, LEN DIV SWG), PASTED PATTERN",
          ok && str_eq(ui.msg, "PASTED PATTERN"));
    check("GLO + PLAY: no start, no GLOBAL page", !song.playing && !transport_req && ui.home && ui.page == pg);
    mod_combo(B_SAVE, B_PLAY);
    check("SAVE + PLAY with a pattern copied: NO TRACK COPIED", str_eq(ui.msg, "NO TRACK COPIED"));
    track_select(0);
    mod_combo(B_SAVE, B_REC);
    mod_combo(B_GLO, B_PLAY);
    check("GLO + PLAY with a track copied: NO PATTERN COPIED, no start", str_eq(ui.msg, "NO PATTERN COPIED") &&
          !song.playing && !transport_req);
    tap(B_GLO);
    check("a GLO tap still opens GLOBAL", !ui.home && cur_fam() == FAM_GLO);
    tap(B_SAVE);
    check("a SAVE tap still opens the projects page", !ui.home && cur_fam() == FAM_SAVE);
    tap(B_PLAY);
    check("PLAY alone still starts", song.playing || transport_req == 1u);
    transport_req = 2;
    ui_frame();
    tap(B_REC);
    check("REC alone still taps (REC ALL)", ui.rec_all);
    tap(B_REC);
}

/* the clipboard's locks: a paste into a full bank: as many as fit, LOCKS FULL */
static void test_paste_locks_full(void)
{
    uint32_t id, s;
    ui_host_init();
    memset(&mo, 0, sizeof mo);
    for (s = 0; s < NSTEP; s++)
        memset(&trk[0].step[s], 0, sizeof(step_t));
    rich_step(0, 0, 3);
    lock_set(0, 0, P_LEVEL, 5);
    lock_set(0, 0, P_PAN, 6);
    lock_set(0, 0, P_E2, 7);                         /* 5 locks */
    track_select(0);
    mod_combo(B_SAVE, B_REC);
    for (s = 0; s < NSTEP && mo.b.count < LK_MAX - 3u; s++)   /* the bank full but 3 places, with track 6's */
        for (id = 0; id < P_COUNT && mo.b.count < LK_MAX - 3u; id++)
            if (motion_param(id))
                lk_put(&mo.b, lk_key(5, s, id), 1, 1);
    track_select(1);
    mod_combo(B_SAVE, B_PLAY);
    check("LOCKS FULL: the track pasted with as many of its locks as fit (3 of 5), says so",
          !memcmp(&trk[1].step[0], &trk[0].step[0], sizeof(step_t)) && step_locks(1, 0, 0, 0) == 3u &&
              mo.b.count == LK_MAX && str_eq(ui.msg, "LOCKS FULL"));
}

/* spec §5: not in the menu, NAME, a confirm or the PERFORM layer */

/* ---- steps copy / paste (owner 2026-11-22; ui_input.c step_combo): in the step views, step keys held + REC copies
 * the lowest .. the highest held step, held + PLAY pastes from the held step (one) or over the held range (several).
 * No transport, no arm, no FILL, no tap toggled on the release, no hold's step left on */
static uint32_t sk(uint32_t k) { return 1u << STEP_KEY[k]; }
static void steps_combo(uint32_t mask, uint32_t b, uint32_t hold_ms)   /* the keys down, b tapped, all let go */
{
    uint32_t t;
    keys(mask);
    ui_frame();
    for (t = 0; t < hold_ms; t += 50u) {
        host_ticks += 50u * 1000u * FM1_TICKS_PER_US;
        ui_frame();
    }
    press(b);
    ui_frame();
    release_all();
    ui_frame();
    keys(0);
    ui_frame();
}
static int steps_quiet(void) { return !song.playing && !transport_req && song.rec == 0u && !ui.rec_all && !fill_on; }
static void test_steps_copy_paste(void)
{
    static track_t snap[NTRK];
    static lk_bank_t bsnap;
    uint32_t s, ok, id;
    int16_t v;
    ui_host_init();
    rich_session();
    open_step_page();
    track_select(0);
    ui_frame();
    clip_kind = CB_NONE;
    memcpy(snap, trk, sizeof snap);
    bsnap = mo.b;
    steps_combo(sk(3), B_PLAY, 0);
    check("steps: held + PLAY with nothing copied: NO STEPS COPIED, nothing changed, no start",
          str_eq(ui.msg, "NO STEPS COPIED") && !memcmp(trk, snap, sizeof snap) && !memcmp(&mo.b, &bsnap, sizeof bsnap) &&
              steps_quiet());
    steps_combo(sk(5), B_REC, 0);
    check("steps: step 6 held + REC: COPIED 6; nothing changed (no tap), no arm, no FILL, no start",
          str_eq(ui.msg, "COPIED 6") && !memcmp(trk, snap, sizeof snap) && !memcmp(&mo.b, &bsnap, sizeof bsnap) &&
              steps_quiet() && step_keys());
    lock_set(0, 3, P_MODEL, 1);                       /* step 4 (no trig): a machine lock */
    memcpy(snap, trk, sizeof snap);
    bsnap = mo.b;
    steps_combo(sk(2) | sk(7), B_REC, 0);
    check("steps: steps 3 and 8 held + REC: COPIED 3-8, nothing changed", str_eq(ui.msg, "COPIED 3-8") &&
          !memcmp(trk, snap, sizeof snap) && !memcmp(&mo.b, &bsnap, sizeof bsnap) && steps_quiet());
    track_select(1);
    trk[1].p[P_SLEN] = 16;
    ui_frame();
    steps_combo(sk(9), B_PLAY, 0);
    ok = 1;
    for (s = 9; s < 15u; s++)
        ok &= same_step(1, s, 0, s - 7u);
    check("steps: onto track 2 from step 10: PASTED 10-15, the steps whole with their locks", ok &&
          str_eq(ui.msg, "PASTED 10-15") && steps_quiet());
    check("steps: the machine lock travelled (step 4's MODEL lock now on track 2's step 11)",
          lock_get(1, 10, P_MODEL, &v) && v == 1);
    ok = 1;
    for (s = 0; s < NSTEP; s++)
        if (s < 9u || s >= 15u)
            ok &= !memcmp(&trk[1].step[s], &snap[1].step[s], sizeof(step_t)) && step_locks(1, s, 0, 0) == 0u;
    check("steps: track 2's other steps untouched, no stray lock; the other tracks untouched", ok &&
          !memcmp(trk[0].step, snap[0].step, sizeof snap[0].step) && !memcmp(trk[2].step, snap[2].step, sizeof snap[2].step));
    steps_combo(sk(12), B_PLAY, 0);
    ok = trk[1].p[P_SLEN] == 16;
    for (s = 12; s < 16u; s++)
        ok &= same_step(1, s, 0, s - 10u);
    for (s = 16; s < NSTEP; s++)
        ok &= !memcmp(&trk[1].step[s], &snap[1].step[s], sizeof(step_t)) && step_locks(1, s, 0, 0) == 0u;
    check("steps: from step 13 with LEN 16: PASTED 13-16, the rest dropped (nothing past LEN)", ok &&
          str_eq(ui.msg, "PASTED 13-16"));
    track_select(0);
    ui_frame();
    steps_combo(sk(5), B_REC, 0);
    track_select(3);
    ui_frame();
    memcpy(snap, trk, sizeof snap);
    steps_combo(sk(0) | sk(3), B_PLAY, 0);
    ok = 1;
    for (s = 0; s < 4u; s++)
        ok &= same_step(3, s, 0, 5);
    for (s = 4; s < NSTEP; s++)
        ok &= !memcmp(&trk[3].step[s], &snap[3].step[s], sizeof(step_t));
    check("steps: one step copied, steps 1 and 4 held + PLAY: 1..4 filled with it, PASTED 1-4", ok &&
          str_eq(ui.msg, "PASTED 1-4") && steps_quiet());
    track_select(4);
    ui_frame();
    for (s = 0; s < NSTEP; s++)
        memset(&trk[4].step[s], 0, sizeof(step_t));
    lock_clear_track(4);
    memcpy(snap, trk, sizeof snap);
    bsnap = mo.b;
    steps_combo(sk(2), B_REC, 1000u);                  /* held long: the hold turns the off step on .. */
    check("steps: an off step held long + REC: COPIED 3, the step off again on the release (no toggle, no lock)",
          str_eq(ui.msg, "COPIED 3") && !memcmp(trk, snap, sizeof snap) && !memcmp(&mo.b, &bsnap, sizeof bsnap) &&
              steps_quiet());
    check("steps: .. the copy is the off step", clip_kind == CB_STEPS && clip_sn == 1u && !clip.st[0][0].on);
    track_select(0);
    ui_frame();
    steps_combo(sk(5), B_REC, 0);                     /* (a rich step: 2 locks) */
    for (s = 0; s < NSTEP && mo.b.count < LK_MAX - 1u; s++)   /* the bank full but 1 place */
        for (id = 0; id < P_COUNT && mo.b.count < LK_MAX - 1u; id++)
            if (motion_param(id))
                lk_put(&mo.b, lk_key(5, s, id), 1, 1);
    track_select(2);
    ui_frame();
    steps_combo(sk(1), B_PLAY, 0);
    check("steps: LOCKS FULL: the step pasted, as many locks as fit (1 of 2), says so",
          !memcmp(&trk[2].step[1], &trk[0].step[5], sizeof(step_t)) && step_locks(2, 1, 0, 0) == 1u &&
              mo.b.count == LK_MAX && str_eq(ui.msg, "LOCKS FULL") && steps_quiet());
}

/* ---- the cue map on SAVE held (spec 2026-10-26): SAVE's tap opens the projects page on its release; held (or with a
 * key) the white keys are the cue's: no note, no MIDI, nothing recorded */
static uint32_t save_bit(void) { return 1u << panel.btn[B_SAVE]; }
static void test_save_tap_opens_projects(void)
{
    uint32_t pg;
    ui_host_init();
    press(B_SAVE);
    ui_frame();
    check("SAVE pressed: no page yet (it may be held for the cue map)", ui.home && !ui.cue_map);
    release_all();
    ui_frame();
    check("SAVE let go within 0.4 s: the projects page", !ui.home && cur_fam() == FAM_SAVE &&
          str_eq(cur_page()->title, "PROJECT"));
    tap(B_SAVE);
    check("a second SAVE tap: TOOLS", !ui.home && str_eq(cur_page()->title, "TOOLS"));
    pg = ui.page;
    press(B_SAVE);
    ui_frame();
    host_ticks += 500u * 1000u * FM1_TICKS_PER_US;
    fm1_in.buttons |= save_bit();
    ui_frame();
    check("SAVE held 0.5 s: the cue map (top line CUE)", ui.cue_map);
    release_all();
    ui_frame();
    check("SAVE let go after the map: no page change, the map gone", !ui.cue_map && ui.page == pg && !ui.home);
}
static void test_cue_keys_no_notes(void)
{
    uint32_t a, k, mw, on = 0;
    ui_host_init();
    song.rec = REC_ALL_MASK;                                /* REC ALL: every track armed */
    transport_req = 1;
    ui_frame();
    a = hit_age(&trk[0]);
    mw = mo_w;
    press(B_SAVE);
    keys(1u << STEP_KEY[0]);                         /* SAVE and white key 1 in the same frame */
    ui_frame();
    fm1_in.buttons |= save_bit();
    ui_frame();
    keys(0);
    ui_frame();
    for (k = 0; k < NSTEP; k++)
        on |= trk[0].step[k].on;
    check("SAVE held + white key 1: the cue map's, no hit on track 1, no MIDI note out, nothing recorded",
          ui.cue_map && hit_age(&trk[0]) == a && mo_w == mw && !on);
    release_all();
    ui_frame();
    transport_req = 2;
    ui_frame();
    song.rec = 0;
}
static void test_save_dead(void)
{
    uint32_t a;
    ui_host_init();
    ui.confirm = 1;                                  /* a dialog: SAVE is dead, the keys play as before */
    ui.confirm_trk = 0;
    ui.force = 1;
    ui_frame();
    a = hit_age(&trk[1]);
    press(B_SAVE);
    keys(1u << KEY_TRK_KEY[1]);
    ui_frame();
    fm1_in.buttons |= save_bit();
    host_ticks += 500u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    check("SAVE in a dialog: dead (no map); a key plays its track as before", !ui.cue_map && hit_age(&trk[1]) != a &&
          ui.confirm == 1u);
    keys(0);
    release_all();
    ui_frame();
    ui.confirm = 0;
    ui.force = 1;
    ui_frame();
    check("SAVE let go in the dialog: no page", ui.home);
    ui.menu = 1;
    press(B_SAVE);
    ui_frame();
    host_ticks += 500u * 1000u * FM1_TICKS_PER_US;
    fm1_in.buttons |= save_bit();
    ui_frame();
    release_all();
    ui_frame();
    check("SAVE in the menu: no map, no page", !ui.cue_map && ui.menu);
    ui.menu = 0;
    ui.force = 1;
    ui_frame();
    settings.mutebar = 0;
    press(B_SEQ);
    keys(1u << SOLO_BK[4]);
    btn_hold_frames(B_SEQ, 2);
    check("SEQ held: black key 5 still mutes track 5", trk[4].p[P_MUTE] == 1);
    keys(0);
    release_all();
    ui_frame();
    trk[4].p[P_MUTE] = 0;
}
static void test_combo_refused(void)
{
    uint32_t i;
    ui_host_init();
    ui.menu = 1;
    ui.force = 1;
    press(B_ARP);
    btn_key(B_ARP, SOLO_BK[0]);
    check("menu: ARP + black key 1 dead, no solo", !song.solo);
    menu_close();
    btn_key(B_ARP, SOLO_BK[0]);
    check("ARP held out of the menu: dead until let go", !song.solo);
    release_all();
    ui_frame();
    check("..and its release does nothing", ui.home);
    ui.confirm = 1;
    ui.force = 1;
    press(B_GLO);
    glo_hold_frames(5);
    release_all();
    ui_frame();
    check("confirm: GLO no GLOBAL page", ui.home && ui.confirm);
    ui.confirm = 0;
    ui.force = 1;
    ui_frame();
    project_save(0);
    name_open(NK_RENAME, 0);
    press(B_SCL);
    btn_key(B_SCL, SOLO_BK[2]);
    release_all();
    ui_frame();
    check("NAME: SEL + black key 3 selects nothing", song.sel == 0u && name_on() && !ui.keys);
    name_close();
    ui_frame();
    press(B_FX);
    for (i = 0; i < 500u; i++) {
        fm1_in.buttons |= 1u << panel.btn[B_FX];
        ui_frame();
    }
    press(B_ARP);
    keys(1u << SOLO_BK[1]);
    for (i = 0; i < 5u; i++) {
        fm1_in.buttons |= 1u << panel.btn[B_FX] | 1u << panel.btn[B_ARP];
        ui_frame();
    }
    keys(0);
    check("PERFORM layer: ARP + a black key is the layer's (no solo)", ui.layer && !song.solo);
    press(B_GLO);
    for (i = 0; i < 450u; i++) {
        fm1_in.buttons |= 1u << panel.btn[B_FX] | glo_bit();
        ui_frame();
    }
    release_all();
    ui_frame();
    check("PERFORM layer: GLO a plain button there (its release opens GLOBAL)", !ui.home && cur_fam() == FAM_GLO);
}

/* HOME's TRACKS mode while a track is armed: the white keys play (and record) their tracks instead of the steps; REC
 * held is no mute key (its release is a REC tap: disarms); disarmed: the steps again */
static void test_tracks_rec_keys(void)
{
    uint32_t a, k, n = 0;
    ui_host_init();
    settings.mutebar = 0;
    for (k = 0; k < 16u; k++)
        trk[1].step[k].on = 0;
    open_tracks();
    turn(EN_K1, 1);                                  /* track 2 */
    ui_frame();
    tap(B_REC);                                      /* arms track 2 */
    tap(B_PLAY);                                     /* (REC starts nothing: PLAY) */
    seq_play_to(1, 3);
    a = hit_age(&trk[1]);
    keys(1u << KEY_TRK_KEY[1]);
    ui_frame();
    keys(0);
    ui_frame();
    for (k = 0; k < 16u; k++)
        n += trk[1].step[k].on;
    check("TRACKS, armed: key 2 plays track 2 and records it", (song.rec & 2u) && hit_age(&trk[1]) != a && n == 1u);
    a = hit_age(&trk[3]);
    keys(1u << KEY_TRK_KEY[3]);
    ui_frame();
    keys(0);
    ui_frame();
    check("TRACKS, armed: key 4 plays track 4, the selection stays", song.sel == 1 && hit_age(&trk[3]) != a);
    press(B_REC);                                    /* REC held: no longer the mute key */
    ui_frame();
    a = hit_age(&trk[3]);
    keys(1u << KEY_TRK_KEY[3]);
    press(B_REC);
    ui_frame();
    keys(0);
    press(B_REC);
    ui_frame();
    check("TRACKS, armed, REC held: key 4 plays track 4 (no mute)", trk[3].p[P_MUTE] == 0 && hit_age(&trk[3]) != a &&
          (song.rec & 2u));
    release_all();                                   /* REC let go: a tap, track 2 disarmed */
    ui_frame();
    a = hit_age(&trk[5]);
    keys(1u << KEY_TRK_KEY[5]);
    ui_frame();
    keys(0);
    ui_frame();
    check("TRACKS, disarmed: key 6 is step 6 of track 2, no select, no sound",
          song.rec == 0 && song.sel == 1 && hit_age(&trk[5]) == a && song.seq_mode == 1u);
    transport_req = 2;
    ui_frame();
}

/* REC (owner, 2026-10-22): on TRACKS a tap arms / disarms the selected track ("T3 REC ARMED" / "T3 REC OFF"); on every
 * other page a tap turns REC ALL on (every track armed, each white key plays and records its own track wherever the
 * keys play tracks) or off (the arm state from before it back). REC never starts play */
static uint32_t steps_on(uint32_t t)
{
    uint32_t k, n = 0;
    for (k = 0; k < NSTEP; k++)
        n += trk[t].step[k].on;
    return n;
}
static void steps_clear(void)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++)
        memset(trk[i].step, 0, sizeof trk[i].step);
}
static void key_hit(uint32_t t)                      /* track t's white key struck */
{
    keys(1u << KEY_TRK_KEY[t]);
    ui_frame();
    keys(0);
    ui_frame();
}
static int only_steps(uint32_t mask)                 /* exactly one recorded step on each track of mask, none elsewhere */
{
    uint32_t i, ok = 1;
    for (i = 0; i < NTRK; i++)
        ok &= steps_on(i) == ((mask >> i) & 1u);
    return (int)ok;
}

/* ---- FILL (spec 2026-10-26): REC held 0.4 s off SEQ plays a fill while held; a shorter press is the tap as
 * before (arm on TRACKS, REC ALL elsewhere, on the release); on SEQ REC held is the confirm as before */
static void rec_hold_ms(uint32_t ms)                 /* REC down for ms (frames of 10 ms), not let go */
{
    uint32_t t;
    press(B_REC);
    ui_frame();
    for (t = 10; t < ms; t += 10) {
        host_ticks += 10u * 1000u * FM1_TICKS_PER_US;
        fm1_in.buttons |= 1u << panel.btn[B_REC];
        ui_frame();
    }
}
static void test_rec_fill_hold(void)
{
    static const struct { const char *what; uint32_t btn, n; } S[] = {
        {"HOME", B_HOME, 0}, {"SOUND", B_EDIT, 1}, {"FX", B_FX, 1}, {"LFO", B_LFO, 1}, {"TRACKS", B_HOME, 1},
    };
    uint32_t i, j, ok = 1;
    char hm[24];
    for (i = 0; i < sizeof S / sizeof S[0]; i++) {
        uint8_t rec;
        ui_host_init();
        for (j = 0; j < S[i].n; j++)
            tap(S[i].btn);
        rec = song.rec;
        rec_hold_ms(300);
        ok &= !fill_on;
        rec_hold_ms(450);
        head_mode(hm);
        ok &= fill_on && str_eq(hm, "FILL");
        release_all();
        ui_frame();
        ok &= !fill_on && song.rec == rec && !ui.rec_all;
        if (!ok) {
            printf("     fill on %s: fill %u rec %u\n", S[i].what, fill_on, song.rec);
            break;
        }
    }
    check("REC held 0.4 s on HOME, SOUND, FX, LFO, TRACKS: FILL while held (top line FILL), off on the release, the "
          "arm state and REC ALL untouched", (int)ok);
}
static void test_rec_hold_vs_rec_all(void)       /* Review Focus 3 */
{
    uint32_t ok = 1;
    ui_host_init();
    tap(B_FX);
    rec_hold_ms(300);
    OK0(!ui.rec_all);
    release_all();
    ui_frame();
    OK0(ui.rec_all && song.rec == REC_ALL_MASK && !fill_on);  /* a 0.3 s press: REC ALL on the release */
    rec_hold_ms(500);
    OK0(fill_on && ui.rec_all);
    release_all();
    ui_frame();
    OK0(!fill_on && ui.rec_all && song.rec == REC_ALL_MASK);  /* a 0.5 s hold: the fill, REC ALL still on */
    tap(B_REC);                                      /* (REC ALL off) */
    open_tracks();
    track_select(2);
    ui_frame();
    rec_hold_ms(300);
    release_all();
    ui_frame();
    OK0(song.rec == 4u && !fill_on);                  /* TRACKS: a 0.3 s press arms the selected track */
    check("REC: a 0.3 s press off SEQ is the tap (REC ALL, TRACKS' arm) on the release; a 0.5 s hold is FILL and "
          "leaves REC ALL on", (int)ok);
}
static void test_rec_seq_unchanged(void)           /* (phase C: the PATTERN page's CLEAR TRACK? hold gone) */
{
    uint32_t k, ok = 1;
    ui_host_init();
    for (k = 0; k < NPAGES && (ui.home || !str_eq(cur_page()->title, "PATTERN")); k++)
        tap(B_SEQ);
    OK0(str_eq(cur_page()->title, "PATTERN"));
    trk[0].step[0].on = 1;
    rec_hold_ms(750);
    OK0(fill_on && !ui.confirm);
    release_all();
    ui_frame();
    OK0(!fill_on && !ui.rec_all && song.rec == 0u && trk[0].step[0].on);
    check("REC held 0.7 s on PATTERN: FILL as on every page (no CLEAR TRACK? dialog), no tap on the release", (int)ok);
}
static void test_rec_learned_short(void)
{
    uint32_t ok = 1, f;
    ui_host_init();
    tap(B_FX);
    for (f = 0; f < 60u; f++) {                       /* a learned REC (as ml_drain gives it), every frame for 0.6 s */
        ml_q[ml_qw % ML_QN] = ML_A_REC;
        ml_qw++;
        host_ticks += 10u * 1000u * FM1_TICKS_PER_US;
        ui_frame();
        OK0(!fill_on);
    }
    check("a learned REC is a tap, never FILL (as the SMC's REC: rec_tap)", (int)ok);
}

static void test_copy_page(void)
{
    uint32_t ok = 1;
    int16_t *v;
    const param_desc_t *d;
    ui_host_init();
    tap(B_SAVE);
    OK0(str_eq(cur_page()->title, "PROJECT"));
    tap(B_SAVE);
    OK0(str_eq(cur_page()->title, "TOOLS"));
    tap(B_SAVE);
    OK0(str_eq(cur_page()->title, "COPY") && cur_page()->graph == GR_SLOTS);
    d = page_desc(cur_page(), 0, &v);
    OK0(d && str_eq(d->label, "SLOT"));
    d = page_desc(cur_page(), 1, &v);
    OK0(d && str_eq(d->label, "COPY") && v && *v == 0);
    d = page_desc(cur_page(), 2, &v);
    OK0(d && str_eq(d->label, "PASTE") && v && *v == 0);
    snap_page("fixes/save_copy_page");
    tap(B_SAVE);
    OK0(str_eq(cur_page()->title, "PROJECT"));
    check("SAVE taps: PROJECT, TOOLS, COPY (SLOT, COPY, PASTE), PROJECT again", (int)ok);
}
static void test_rec_all(void)
{
    uint32_t i, ok;
    ui_host_init();
    settings.mutebar = 0;
    open_tracks();
    track_select(2);
    ui_frame();
    press(B_REC);
    ui_frame();
    check("TRACKS: REC pressed (not let go): nothing yet", song.rec == 0u && !song.playing);
    release_all();
    ui_input();
    check("TRACKS: a REC tap arms the selected track at once (the release's pass): T3 REC ARMED, no play",
          song.rec == 4u && !ui.rec_all && str_eq(ui.msg, "T3 REC ARMED") && !transport_req && !song.playing);
    snap_page("fixes/rec_tracks_armed");
    tap(B_REC);
    check("TRACKS: a REC tap again disarms it: T3 REC OFF, REC dark", song.rec == 0u && !ui.rec_all &&
          str_eq(ui.msg, "T3 REC OFF") && !led_lit(panel.btn[B_REC]));
    tap(B_REC);                                      /* track 3 armed */
    tap(B_FX);                                       /* FX: the keys play their drums */
    tap(B_REC);
    check("FX: a REC tap: REC ALL, every track armed, REC ALL said, REC lit, no play", song.rec == REC_ALL_MASK &&
          ui.rec_all && str_eq(ui.msg, "REC ALL") && led_lit(panel.btn[B_REC]) && !transport_req && !song.playing);
    snap_page("fixes/rec_all");
    tap(B_PLAY);
    steps_clear();
    seq_play_to(0, 3);
    key_hit(0);
    key_hit(3);
    key_hit(5);
    check("REC ALL on FX: keys 1, 4, 6 each record into their own track", only_steps(1u | 8u | 32u));
    trk[4].p[P_MUTE] = 1;
    key_hit(4);
    check("REC ALL: a muted track's key records too (as an armed muted track)", only_steps(1u | 8u | 16u | 32u));
    trk[4].p[P_MUTE] = 0;
    check("REC ALL on FX: a key selects its track, as before (track 5 last)", song.sel == 4u);
    track_select(2);
    open_tracks();
    steps_clear();
    key_hit(1);
    key_hit(5);
    check("REC ALL on TRACKS: keys 2 and 6 record into their tracks, the selection stays",
          only_steps(2u | 32u) && song.sel == 2u && song.seq_mode == 2u && ui.rec_all);
    snap_page("fixes/rec_all_tracks");
    tap(B_HOME);                                     /* the HOME screen */
    steps_clear();
    key_hit(4);
    check("REC ALL on HOME: key 5 records into track 5 (and selects it, as before)", ui.home && only_steps(16u) &&
          song.sel == 4u);
    tap(B_EDIT);                                     /* (HOME's knobs: BPM, SWING since FREITAKT: SOUND's) */
    turn(EN_K2, 3);
    ui_frame();
    for (ok = motion_count(4) >= 1u, i = 0; i < NTRK; i++)
        ok &= i == 4u || motion_count(i) == 0u;
    check("REC ALL: a knob's motion records on the selected track only", ok);
    tap(B_HOME);
    tap(B_SCL);                                      /* KEYS: the selected track's keyboard */
    steps_clear();
    keys(1u << 0);
    ui_frame();
    keys(0);
    ui_frame();
    check("REC ALL + KEYS: the key records into the selected track only", only_steps(16u));
    tap(B_SCL);
    track_select(2);
    tap(B_REC);
    check("HOME: a REC tap leaves REC ALL: track 3 armed again, as before it (T3 REC ARMED), still playing",
          song.rec == 4u && !ui.rec_all && str_eq(ui.msg, "T3 REC ARMED") && led_lit(panel.btn[B_REC]) && song.playing);
    tap(B_REC);
    check("HOME: a REC tap again: REC ALL", song.rec == REC_ALL_MASK && ui.rec_all);
    track_select(1);
    open_tracks();
    tap(B_REC);
    check("TRACKS during REC ALL: a REC tap ends it, the tracks from before and the selected one armed (T2 REC ARMED)",
          song.rec == 6u && !ui.rec_all && str_eq(ui.msg, "T2 REC ARMED"));
    tap(B_REC);
    track_select(2);
    tap(B_REC);
    check("TRACKS: T2, then T3 disarmed: nothing armed", song.rec == 0u && str_eq(ui.msg, "T3 REC OFF"));
    tap(B_HOME);
    tap(B_REC);
    tap(B_REC);
    check("HOME: REC ALL on and off with nothing armed before: REC OFF, REC dark, still playing", song.rec == 0u &&
          !ui.rec_all && str_eq(ui.msg, "REC OFF") && !led_lit(panel.btn[B_REC]) && song.playing);
    track_select(1);
    tap(B_REC);
    tap(B_REC);
    steps_clear();
    key_hit(1);
    check(".. the keys only play again", only_steps(0u));
    transport_req = 2;
    ui_frame();
    ui_frame();
    tap(B_REC);
    check("HOME, stopped: a REC tap: REC ALL, play not started", ui.rec_all && song.rec == REC_ALL_MASK && !transport_req &&
          !song.playing);
    ui_frame();
    init_all();
    check("INIT ALL: REC ALL off, nothing armed", !ui.rec_all && song.rec == 0u);

    ui_host_init();
    trk[0].step[0].on = 1;
    seq_open("PATTERN");
    tap(B_REC);
    check("STEP, KEYS off: a REC tap: REC ALL (no play)", ui.rec_all && song.rec == REC_ALL_MASK && !song.playing);
    hold(B_REC);
    check("STEP: REC held: no dialog, no tap (REC ALL as it was)", !ui.confirm && song.rec == REC_ALL_MASK && ui.rec_all);
    tap(B_REC);
    check(".. a tap after: REC ALL off", song.rec == 0u && !ui.rec_all);
}

/* text s in F3 at sc drawn in ink at x, y on the screen (every pixel of its box: ink where the glyphs are, else not) */
static int region_is_text(int32_t x, int32_t y, uint32_t sc, const char *s)
{
    uint32_t w = (uint32_t)md_text_w(&MD_F3, sc, s), h = 5u * sc, i, j;
    cv_begin(w, h, C_BG);
    md_text(0, 0, &MD_F3, sc, s, C_INK);
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if ((fb[((uint32_t)y + j) * 240u + (uint32_t)x + i] == C_INK) != (cv_px[j * w + i] == swap16(C_INK)))
                return 0;
    return 1;
}
/* HOME's status line (ui_draw.c draw_home): s centred, as large as fits (F3 at 5x at most) */
static int home_line_is(const char *s)
{
    uint32_t sc = md_fit_sc(s, 228, 5u);
    return region_is_text(120 - md_text_w(&MD_F3, sc, s) / 2, HM_Y_MID + 48 - 5 * (int32_t)sc / 2, sc, s);
}

/* HOME: a white key selects its track and plays it (the status line's T) */
static void test_home_keys_and_lfo(void)
{
    uint32_t a;
    ui_host_init();
    a = hit_age(&trk[3]);
    keys(1u << KEY_TRK_KEY[3]);
    ui_frame();
    keys(0);
    ui_frame();
    check("HOME: white key 4 selects track 4 and plays it", ui.home && song.sel == 3 && hit_age(&trk[3]) != a);
    check("HOME: the status line shows the track (T4)", home_line_is("P- V1 T4 L1"));
}

/* GLOBAL 3/3: MUTE NOW / BAR, the device setting the mutes follow (track_mute_toggle) */
static void menu_row(uint32_t mi)                    /* the device MENU open on row mi (FREITAKT: GLOBAL / SYSTEM's
                                                      * settings live there) */
{
    ui.menu = 1;
    ui.menu_sel = (uint8_t)mi;
    ui.force = 1;
    ui_frame();
}
static void menu_turn(int32_t n)
{
    turn(EN_K1, n);
    ui_frame();
}
static void menu_shut(void)
{
    ui.menu = 0;
    ui.force = 1;
    ui_frame();
}
static void test_global_mute_setting(void)
{
    ui_host_init();
    settings.mutebar = 0;
    menu_row(MI_MUTE);
    menu_turn(1);
    check("MENU MUTE MODE: KNOB 1 -> BAR (the setting)", settings.mutebar == 1u);
    snap_page("menu_mute_mode");
    menu_turn(-1);
    check("MENU MUTE MODE: KNOB 1 back -> NOW", settings.mutebar == 0u);
    menu_shut();
}

/* GLOBAL 3/3, KNOB 2: METRO OFF / LOW / MID / HIGH (spec 2026-10-11-fixes §8), the device setting fx_metro (fx.c's
 * click); not in projects */
static void test_global_metro_setting(void)
{
    uint32_t k, ok = 1;
    int16_t *vp;
    const param_desc_t *d;
    char v[12];
    const char *u;
    static const char *const NAMES[4] = {"OFF", "LOW", "MID", "HIGH"};
    ui_host_init();
    fx_metro = 0;
    d = glob_desc(G_METRO, &vp);
    for (k = 0; k < 4u; k++) {
        param_format(d, (int32_t)k, v, &u);
        ok &= str_eq(v, NAMES[k]);
    }
    check("METRO values: OFF LOW MID HIGH", ok && d->max == 3);
    menu_row(MI_METRO);
    menu_turn(2);
    check("MENU METRONOME: KNOB 1 -> MID (the setting)", fx_metro == 2u);
    snap_page("menu_metronome");
    menu_turn(5);
    check("KNOB 1 up: HIGH at the end", fx_metro == 3u);
    menu_turn(-9);
    check("KNOB 1 down: OFF", fx_metro == 0u);
    menu_shut();
}

static int col_has(uint32_t c, const char *t) { return strstr(ui.col[c], t) != 0; }   /* KNOB c+1's column shows t */
/* (owner 2026-11-01) a step held on any page whose white keys are the steps (TRACKS unarmed, STEP, PATTERN, MOTION):
 * KNOB 1-4 its PROB RATCH ACC NUDGE, shown in the knob columns, and the page's own values untouched */
static void test_step_hold_every_seq_page(void)
{
    static const char *const PG[] = {"TRACKS", "PATTERN"};
    uint32_t i;
    for (i = 0; i < sizeof PG / sizeof PG[0]; i++) {
        int16_t p0[P_COUNT], g0[G_COUNT];
        step_t *st = &trk[0].step[3];
        int ok, cols;
        char what[96];
        ui_host_init();
        song.rec = 0;
        if (i == 0u)
            open_tracks();
        else
            seq_open(PG[i]);
        memset(trk[0].step, 0, sizeof trk[0].step);
        memcpy(p0, trk[0].p, sizeof p0);
        memcpy(g0, song.g, sizeof g0);
        grid_down(WHITE[3]);
        grid_wait(400);                              /* held: the step's edit */
        turn(EN_K1, 3);
        ui_frame();
        turn(EN_K2, 1);
        ui_frame();
        turn(EN_K3, 1);
        ui_frame();
        turn(EN_K4, 7);
        ui_frame();
        cols = kp.on && kp.kind == KP_STEP && kp.id == 3u && kp.st == 3u;   /* (the knob popup: the step's NUDGE) */
        grid_up(WHITE[3]);
        ui_frame();
        ok = st->on && st->cond != 0 && st->rat == 1u && st->acc && st->micro == 7 &&
             !memcmp(p0, trk[0].p, sizeof p0) && !memcmp(g0, song.g, sizeof g0) &&
             song.sel == 0;
        str_cpy(what, PG[i], sizeof what);
        str_cpy(what + str_len(what), ": a held step's KNOB 1-4 edit PROB RATCH ACC NUDGE, the page's values untouched",
                sizeof what - str_len(what));
        check(what, ok && (i == 0u || str_eq(cur_page()->title, PG[i])));
        str_cpy(what, PG[i], sizeof what);
        str_cpy(what + str_len(what), ": the knob popup shows the held step's field (KNOB 4: NUDGE)", sizeof what - str_len(what));
        check(what, cols);
    }
}

/* QUANT (spec 2026-10-31 §2): GLOBAL 3/7 KNOB 4, 1/16 (default) 1/8 1/32 OFF, the device setting rec_quant */
static void test_glo_quant(void)
{
    ui_host_init();
    rec_quant = 0;
    menu_row(MI_QUANT);
    menu_turn(1);
    check("MENU REC QUANT: KNOB 1 -> 1/8 (the setting)", rec_quant == QUANT_8);
    menu_turn(1);
    check("REC QUANT: -> 1/32", rec_quant == QUANT_32);
    menu_turn(5);
    check("REC QUANT: -> OFF at the end", rec_quant == QUANT_OFF);
    snap_page("menu_rec_quant");
    menu_turn(-5);
    check("REC QUANT: back to 1/16; MUTE METRO THRU untouched", rec_quant == QUANT_16 && settings.mutebar == 0u &&
          fx_metro == 0u && midi_thru == 0u);
    menu_shut();
}

/* GLO MIDI OUT 4/8: KNOB 1 OUT (OFF USB, + BLE BOTH in the Bluetooth build; USB by default), KNOB 2 the selected
 * track's channel (OFF, 1..16; track n on n by default), project globals (midi_out.c mon_*) */
static void test_glo_midi_out(void)
{
    uint32_t k;
    int16_t *vp;
    const param_desc_t *d;
    char v[12];
    const char *u;
    ui_host_init();
    for (k = 0; k < 10u && (ui.home || !str_eq(cur_page()->title, "MIDI OUT")); k++)
        tap(B_GLO);
    d = page_desc(cur_page(), 0, &vp);
    param_format(d, *vp, v, &u);
    check("GLO MIDI OUT: KNOB 1 is OUT, USB", d && str_eq(d->label, "OUT") && vp == &song.g[G_MOUT] && str_eq(v, "USB"));
    d = page_desc(cur_page(), 1, &vp);
    param_format(d, *vp, v, &u);
    check("GLO MIDI OUT: KNOB 2 the selected track's channel (T1CH 1)", d && str_eq(d->label, "T1CH") &&
                                                                          vp == &song.g[G_MCH1] && str_eq(v, "1"));
    song.sel = 2;
    ui_frame();
    d = page_desc(cur_page(), 1, &vp);
    check("GLO MIDI OUT: track 3 selected: T3CH 3", d && str_eq(d->label, "T3CH") && vp == &song.g[G_MCH1 + 2] &&
                                                       *vp == 3);
    turn(EN_K2, -3);
    ui_frame();
    param_format(d, song.g[G_MCH1 + 2], v, &u);
    check("T3CH: KNOB 2 down to OFF, track 1's untouched", song.g[G_MCH1 + 2] == 0 && str_eq(v, "OFF") &&
                                                             song.g[G_MCH1] == 1);
    turn(EN_K2, 20);
    ui_frame();
    check("T3CH: up to 16 at most", song.g[G_MCH1 + 2] == 16);
    turn(EN_K1, -1);
    ui_frame();
    check("OUT: KNOB 1 -> OFF", song.g[G_MOUT] == 0);
    song.sel = 0;
}

/* GLO SYSTEM 2/3: KNOB 2 SYNC (MIDI clock out: OFF USB, + BLE BOTH in the Bluetooth build), KNOB 3 ROUT (MIDI in:
 * NOTE SEL TRACK), project globals; GLOBAL 3/3 KNOB 3: THRU OFF / ON, the device setting midi_thru (midi_out.c) */
static void test_glo_midi_settings(void)
{
    uint32_t ok = 1;
    int16_t *vp;
    const param_desc_t *d;
    char v[12];
    const char *u;
    ui_host_init();
    midi_thru = 0;
    d = glob_desc(G_SYNC, &vp);
    param_format(d, 0, v, &u);
    ok &= str_eq(v, "OFF");
    param_format(d, 1, v, &u);
    ok &= str_eq(v, "USB");
    check("SYNC values: OFF USB (no TRS out on the FM-1)", ok && d->max == 1);
    menu_row(MI_SYNC);
    menu_turn(1);
    check("MENU MIDI SYNC: KNOB 1 -> USB", song.g[G_SYNC] == 1);
    menu_row(MI_ROUTE);
    d = glob_desc(G_ROUTE, &vp);
    menu_turn(1);
    param_format(d, song.g[G_ROUTE], v, &u);
    check("MENU MIDI ROUTE: KNOB 1 -> SEL", song.g[G_ROUTE] == 1 && str_eq(v, "SEL"));
    menu_turn(5);
    param_format(d, song.g[G_ROUTE], v, &u);
    check("MIDI ROUTE: up to TRACK at the end", song.g[G_ROUTE] == 2 && str_eq(v, "TRACK"));
    snap_page("menu_midi_route");
    menu_row(MI_THRU);
    menu_turn(1);
    check("MENU MIDI THRU: KNOB 1 -> ON (the setting)", midi_thru == 1u);
    menu_turn(-1);
    check("MIDI THRU: back OFF; MUTE and METRO untouched", midi_thru == 0u && settings.mutebar == 0u && fx_metro == 0u);
    menu_shut();
    song.g[G_SYNC] = song.g[G_ROUTE] = 0;
}

static void test_oct_both_reaches_main(void)
{
    ui_host_init();
    open_step_page();
    fm1_in.buttons = (1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP]);
    ui_input();
    check("OCT- + OCT+ held: buttons stay visible to main.c (UBOOT countdown)",
          (fm1_in.buttons & ((1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP]))) ==
              ((1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP])));
    release_all();
}

static uint32_t fb_lit(uint32_t y0, uint32_t y1)    /* drawn pixels in rows y0..y1: neither the theme's background
                                                     * nor black (the screens not redrawn yet), the frame's edge
                                                     * columns x 0 / 239 left out */
{
    uint32_t n = 0, i;
    for (i = y0 * 240u; i < y1 * 240u; i++)
        n += fb[i] != C_BG && fb[i] != 0 && i % 240u != 0 && i % 240u != 239u;
    return n;
}

/* spec 2026-10-11-fixes §12: the key backlight. The level ui_leds gave key / button id: 100 full, 25, 5 or 0 (the
 * dimmed layers in led_lay while BACKLIGHT is ON, else fm1_led) */
static uint32_t led_level(uint32_t id)
{
    uint8_t q = led_pos[id];
    uint32_t lay, bit;
    if (q == 0xFF)
        return 0;
    bit = 1u << (q & 7u);
    if (!led_dim_on)
        return fm1_led[q >> 3] & bit ? 100u : 0u;
    lay = led_lay[q >> 3];
    return lay & bit ? 100u : (lay >> 8) & bit ? 25u : (lay >> 16) & bit ? 5u : (lay >> 24) & bit ? 60u : 0u;
}
static int keys_dim_except(uint32_t mask)          /* every key not in mask at 5 % */
{
    uint32_t k, ok = 1;
    for (k = 0; k < 27u; k++)
        if (!((mask >> k) & 1u))
            ok &= led_level(14u + k) == 5u;
    return (int)ok;
}
static void test_backlight_leds(void)
{
    uint32_t k, ok, full[FM1_NCOL], c;
    ui_host_init();
    backlight = 0;
    for (k = 0; k < NSTEP; k++)
        trk[0].step[k].on = 0;
    trk[0].p[P_SLEN] = 32;
    trk[0].step[1].on = trk[0].step[4].on = 1;      /* steps 2 and 5 (5: a beat) */
    ui_frame();                                      /* (the HOME screen: no OCT LED breathes) */
    check("BACKLIGHT OFF, nothing breathing: ui_leds writes fm1_led, no dimming (the hook idle)",
          !led_dim_on && led_level(14u + KEY_TRK_KEY[0]) == 100u && led_level(14u + 1u) == 0u);
    seq_open("PATTERN");
    ui_frame();
    ok = 1;
    for (c = 0; c < FM1_NCOL; c++)
        ok &= (led_lay[c] & 0xFFFF00u) == 0u;
    check("BACKLIGHT OFF, STEP (2 banks): the hook on for OCT+'s breath alone (item 5): the keys full or dark, no dim "
          "(black 1: the selected track, lit)",
          led_dim_on && ok && led_breathes(panel.btn[B_OCTUP]) && led_level(14u + STEP_KEY[1]) == 100u &&
          led_level(14u + STEP_KEY[0]) == 0u && led_level(14u + 1u) == 100u && led_level(14u + 3u) == 0u);
    backlight = 1;
    ui_frame();
    ok = led_dim_on && led_level(14u + STEP_KEY[1]) == 100u && led_level(14u + STEP_KEY[4]) == 100u;
    ok &= led_level(14u + STEP_KEY[0]) == 25u && led_level(14u + STEP_KEY[8]) == 25u &&
          led_level(14u + STEP_KEY[12]) == 25u;
    for (k = 0; k < 16u; k++)
        if (k != 0u && k != 1u && k != 4u && k != 8u && k != 12u)
            ok &= led_level(14u + STEP_KEY[k]) == 5u;
    for (k = 0; k < 27u; k++)                        /* the black keys */
        if (k != 0u && k != 2u && k != 4u && k != 6u && k != 7u && k != 9u && k != 11u && k != 12u && k != 14u &&
            k != 16u && k != 18u && k != 19u && k != 21u && k != 23u && k != 24u && k != 26u)
            ok &= led_level(14u + k) == (k == 1u ? 100u : 5u);   /* (black 1: the selected track) */
    check("BACKLIGHT ON, STEP: on steps full, off beats 1 5 9 13 at 25 %, the other keys at 5 % (black 1, the "
          "selected track, full)", ok);
    check("BACKLIGHT ON (DIM LO): the buttons: SEQ lit, the idle ones at 5 % (SAVE, OCT-: no bank before), OCT+ breathing",
          led_level(panel.btn[B_SEQ]) == 100u && led_level(panel.btn[B_SAVE]) == 5u &&
          led_level(panel.btn[B_OCTUP]) == 60u && led_level(panel.btn[B_OCTDN]) == 5u);
    for (c = 0; c < FM1_NCOL; c++)
        full[c] = led_lay[c] & 0xFF0000FFu;          /* (the full and breath layers) */
    tap(B_OCTUP);                                    /* bank 2: steps 17..32, step 21 on */
    trk[0].step[20].on = 1;
    ui_frame();
    check("BACKLIGHT ON, bank 2: the beats are 17 21 25 29 (keys 1 5 9 13); step 21 on: full",
          ui.bank == 1u && led_level(14u + STEP_KEY[0]) == 25u && led_level(14u + STEP_KEY[4]) == 100u &&
          led_level(14u + STEP_KEY[8]) == 25u && led_level(14u + STEP_KEY[12]) == 25u &&
          led_level(14u + STEP_KEY[1]) == 5u);
    tap(B_OCTDN);
    backlight = 0;
    ui_frame();
    ok = led_dim_on;
    for (c = 0; c < FM1_NCOL; c++)
        ok &= led_lay[c] == full[c];
    check("BACKLIGHT OFF again: the full and breath layers as with it ON, nothing dim", ok);
    backlight = 1;
    transport_req = 1;                               /* the playhead: on step 2 (on) it goes out, so it dims; on
                                                      * step 3 (off) it lights full */
    seq_play_to(0, 1);
    ui_leds();                                       /* (the frame lit the LEDs before the step moved) */
    ok = led_level(14u + STEP_KEY[1]) == 5u;
    seq_play_to(0, 2);
    ui_leds();
    ok &= led_level(14u + STEP_KEY[2]) == 100u;
    seq_play_to(0, 4);
    ui_leds();
    ok &= led_level(14u + STEP_KEY[4]) == 25u;
    check("BACKLIGHT ON, playing: the playhead lights an off step full; on an on step it dims (5 %, a beat 25 %)", ok);
    transport_req = 2;
    ui_frame();
    ui_frame();
    ui.home = 1;                                     /* HOME's second mode: the keys play */
    ui.hmode = 1;
    ui.force = 1;
    ui_frame();
    keys(1u << 20);                                  /* (C#5: G#3 .. the first 8 black keys show the variations) */
    ui_frame();
    check("BACKLIGHT ON, HOME (the keys play): a held key and the selected track's (item 2) full, every other key 5 %",
          !step_keys() && led_level(14u + 20u) == 100u && led_level(14u + KEY_TRK_KEY[0]) == 100u &&
          keys_dim_except(1u << 20 | 1u << KEY_TRK_KEY[0]));
    keys(0);
    ui_frame();
    {                                                /* SEQ held: audible tracks' black keys full, the rest 5 % */
        uint32_t m = 0;
        trk[5].p[P_MUTE] = 1;
        press(B_SEQ);
        btn_hold_frames(B_SEQ, 2);
        for (k = 0; k < NTRK; k++)
            if (!trk[k].p[P_MUTE])
                m |= 1u << SOLO_BK[k];
        check("BACKLIGHT ON, SEQ held: audible tracks' black keys full, every other key 5 %",
              led_level(14u + SOLO_BK[5]) == 5u && led_level(14u + SOLO_BK[0]) == 100u &&
              keys_dim_except(m));
        release_all();
        ui_frame();
        trk[5].p[P_MUTE] = 0;
    }
    {                                                /* PERFORM: lit keys full, dark ones 5 % */
        uint32_t m = 0, dim = 0;
        press(B_FX);
        fx_hold_frames(500);
        for (k = 0; k < 27u; k++) {
            uint32_t l = led_level(14u + k);
            m += l == 100u;
            dim += l == 5u;
        }
        check("BACKLIGHT ON, PERFORM: its lit keys full, every other key 5 %", ui.layer && m > 0u && m + dim == 27u);
        release_all();
        ui_frame();
    }
    safe_start = 1;
    ui_frame();
    ok = !led_dim_on;
    for (c = 0; c < FM1_NCOL; c++)
        ok &= fm1_led[c] == 0u;
    check("BACKLIGHT ON in safe start: all LEDs off, the hook idle", ok);
    safe_start = 0;
    backlight = 0;
    ui_frame();
}

/* the beat marks on every screen where the keys are steps (user, 2026-10-12: "the divisions must be visible on every
 * screen when sequencer on keys available"): every page of the table and the HOME screen, KEYS off / on, the track
 * armed or not. Where step_keys(): the bank's on steps full, its unlit steps 1 5 9 13 at 25 %, the rest 5 %, in bank 1
 * and bank 2 (OCT+, as the keys' own bank); elsewhere no key at 25 %. */
static int beats_shown(uint32_t bank)               /* track 0's pattern below; every other key 5 % (the selected
                                                      * track's black key full; armed: winking) */
{
    uint32_t k, ok = 1, steps = 0;
    for (k = 0; k < 16u; k++) {
        uint32_t si = bank * 16u + k, l = led_level(14u + STEP_KEY[k]);
        steps |= 1u << STEP_KEY[k];
        ok &= trk[0].step[si].on ? l == 100u : (k & 3u) == 0u ? l == 25u : l == 5u;
    }
    for (k = 0; k < 27u; k++)
        if (k == CUE_BK[song.sel % NTRK])
            ok &= led_level(14u + k) == 100u || ((song.rec >> song.sel) & 1u);
        else if (!((steps >> k) & 1u))
            ok &= led_level(14u + k) == 5u;
    return (int)ok;
}
static int no_beat_layer(void)
{
    uint32_t k, ok = 1;
    for (k = 0; k < 27u; k++)
        ok &= led_level(14u + k) != 25u;
    return (int)ok;
}
static void test_backlight_beats_everywhere(void)
{
    uint32_t i, kys, arm, n_steps = 0, n_other = 0, seq_seen = 0, mix_seen = 0, ok_steps = 1, ok_other = 1;
    char bad[160] = "";
    ui_host_init();
    for (i = 0; i < NSTEP; i++)
        trk[0].step[i].on = 0;
    trk[0].p[P_SLEN] = 32;
    trk[0].step[1].on = trk[0].step[4].on = 1;       /* bank 1: steps 2, 5 (a beat) */
    trk[0].step[17].on = trk[0].step[28].on = 1;     /* bank 2: steps 18, 29 (a beat) */
    backlight = 1;
    for (kys = 0; kys < 2u; kys++)
        for (arm = 0; arm < 2u; arm++)
            for (i = 0; i <= NPAGES; i++) {          /* (NPAGES: the HOME screen) */
                ui.home = i == NPAGES;
                ui.hmode = ui.home;
                if (i < NPAGES)
                    ui.page = (uint8_t)i;
                ui.keys = (uint8_t)kys;
                song.rec = (uint8_t)arm;
                ui.bank = 0;
                page_entered();
                ui_frame();
                if (step_keys()) {
                    int ok = ui.bank == 0u && beats_shown(0);
                    tap(B_OCTUP);
                    ok &= ui.bank == 1u && beats_shown(1);
                    tap(B_OCTDN);
                    ok &= ui.bank == 0u && beats_shown(0);
                    if (!ok && str_len(bad) < 120u) {
                        str_cpy(bad + str_len(bad), " ", sizeof bad - str_len(bad));
                        str_cpy(bad + str_len(bad), cur_page()->title, sizeof bad - str_len(bad));
                    }
                    ok_steps &= (uint32_t)ok;
                    n_steps++;
                    seq_seen |= cur_page()->fam == FAM_SEQ ? 1u << (ui.page & 31u) : 0u;
                    mix_seen |= cur_page()->scope == SC_MIX;
                } else {
                    ok_other &= (uint32_t)no_beat_layer();
                    n_other++;
                }
            }
    for (i = 0; i < NPAGES; i++)                     /* every SEQ page among them */
        if (PAGES[i].fam == FAM_SEQ)
            ok_steps &= (seq_seen >> i) & 1u;
    if (!ok_steps)
        printf("      (beats missing on:%s)\n", bad);
    check("BACKLIGHT ON, every page and the HOME screen, KEYS off / on, armed or not: where the keys are steps (every "
          "SEQ page, TRACKS unarmed) on steps full, the unlit beats 1 5 9 13 at 25 % in bank 1 and bank 2, the rest 5 %",
          ok_steps && mix_seen && n_steps == 3u);    /* PATTERN x armed / not, TRACKS unarmed; KEYS off only (SEL lit:
                                                      * every key KEYS') */
    check("BACKLIGHT ON: where the keys are not steps (EDIT FX LFO GLOBAL SAVE COMP, the HOME screen, KEYS, "
          "TRACKS armed) no key at 25 %", ok_other && n_other > 0u);
    ui.home = 0;                                     /* a SEQ page under the menu, the MUTE screen, PERFORM: their
                                                      * keys are not steps */
    ui.keys = 0;
    song.rec = 0;
    seq_open("PATTERN");
    ui_frame();
    i = beats_shown(0);
    ui.menu = 1;
    ui.force = 1;
    ui_frame();
    i &= (uint32_t)no_beat_layer();
    ui.menu = 0;
    ui_frame();
    press(B_ARP);
    btn_hold_frames(B_ARP, 500);
    i &= mod_live(ui.arp_t0) && no_beat_layer();
    release_all();
    ui_frame();
    press(B_FX);
    fx_hold_frames(500);
    i &= ui.layer && no_beat_layer();
    release_all();
    ui_frame();
    i &= beats_shown(0);
    check("BACKLIGHT ON, over STEP: the menu, the MUTE screen and PERFORM show no beat marks; back on STEP they do", i);
    backlight = 0;
    ui_frame();
}

/* MENU > BACKLIGHT (fixes 2026-10-21 item 9: upstream Felucca's LEDS levels): KNOB 1 steps OFF DIM LO DIM HI INV INV LO
 * INV HI, stopping at the ends; OCT+ steps and wraps; the keys follow at once; the item drawn (the carousel) */
static void test_backlight_menu(void)
{
    uint32_t ok = 1, i;
    ui_host_init();
    backlight = 0;
    check("menu: BACKLIGHT is a row (after ACCEL), a list of six levels (not ON / OFF)",
          str_eq(MI_NAME[MI_BACKLIGHT], "BACKLIGHT") && MI_BACKLIGHT == MI_ACCEL + 1 && !menu_onoff(MI_BACKLIGHT) &&
          BL_COUNT == 6u && str_eq(BL_NAME[BL_DIM_LO], "DIM LO") && str_eq(BL_NAME[BL_INV_HI], "INV HI"));
    ui.menu = 1;
    ui.menu_sel = MI_BACKLIGHT;
    ui.force = 1;
    for (i = 1; i <= 6u; i++) {
        host_ticks += 300u * 1000u * FM1_TICKS_PER_US;
        turn(EN_K1, 1);
        ui_frame();
        ok &= backlight == (i < 6u ? i : 5u) && led_dim_on;
    }
    for (i = 0; i < 7u; i++) {
        host_ticks += 300u * 1000u * FM1_TICKS_PER_US;
        turn(EN_K1, -1);
        ui_frame();
    }
    ok &= backlight == BL_OFF;
    press(B_OCTUP);
    ui_frame();
    release_all();
    ui_frame();
    ok &= backlight == BL_DIM_LO && ui.menu == 1u;
    check("BACKLIGHT: KNOB 1 steps OFF .. INV HI and stops at the ends, OCT+ steps on; the LEDs follow at once", ok);
    backlight = BL_INV_HI;
    press(B_OCTUP);
    ui_frame();
    release_all();
    ui_frame();
    check("BACKLIGHT: OCT+ from INV HI wraps to OFF", backlight == BL_OFF);
    backlight = BL_DIM_HI;
    ui.force = 1;
    ui_frame();
    snap_page("fixes/menu_backlight");
    check("menu: BACKLIGHT's carousel item, its level under it", menu_value_is(MI_BACKLIGHT, "DIM HI"));
    backlight = 0;
    menu_close();
    ui_frame();
}

/* the start screen (main.c fm1_main, FREITAKT): the theme reversed, the CRT beam across the middle; it starts the
 * splash (ui.splash 1) */
static void test_boot_title(void)
{
    uint32_t t, i, ok = 1;
    for (t = 0; t < NTHEMES; t++) {
        uint32_t stray = 0;
        theme_set(THEME_TAG | t);
        memset(fb, 0, sizeof fb);
        draw_boot_title();
        if (t == 0)
            shot("build/ui_shots/00_boot.ppm");
        for (i = 0; i < 240u * 240u; i++)
            stray += fb[i] != C_BG && fb[i] != C_INK;
        ok &= stray == 0u;
    }
    check("boot: two tones only, in every theme", ok);
    theme_set(THEME_TAG | THEME_DEFAULT);
    memset(fb, 0, sizeof fb);
    draw_boot_title();
    check("boot: the ground the theme's ink (ORANGE: black), the beam a 2 px orange line at y 105..106, the splash next",
          fb_count(0, 0, 240, 105, C_INK) == 240u * 105u && fb_count(0, 105, 240, 2, C_BG) == 480u &&
          fb_count(0, 107, 240, 133, C_INK) == 240u * 133u && ui.splash == 1u);
    ui.splash = 0;
    theme_set(settings.palette);
}

/* the FREITAKT boot splash (ui_menu.c splash_draw): SPLASH_MS of UI frames, two tones, FREITAKT revealed, MASCHINE //
 * TAKT typed, the boot bar; then the page drawn whole; any key skips it and does nothing else */
static void test_boot_splash(void)
{
    uint32_t i, stray = 0, k, ms, play;
    ui_host_init();
    draw_boot_title();
    fm1_ms = 10000;
    for (ms = 0; ms < SPLASH_MS; ms += 40u) {
        fm1_ms = 10000u + ms;
        ui_frame();
        for (i = 0; i < 240u * 240u; i++)
            stray += fb[i] != C_BG && fb[i] != C_INK;
        if (ms == 240u)
            shot("build/ui_shots/fixes/splash_1_scan.ppm");
        if (ms == 600u)
            shot("build/ui_shots/fixes/splash_2_glitch.ppm");
    }
    shot("build/ui_shots/fixes/splash_3_name.ppm");
    {
        uint32_t sc = md_fit_sc("FREITAKT", 208, 7u);
        int32_t lx = 120 - md_text_w(&MD_F3, sc, "FREITAKT") / 2;
        uint16_t ink = C_INK, bg = C_BG;
        C_INK = bg, C_BG = ink;                      /* (the theme reversed: region_is_text's ink) */
        k = region_is_text(lx, (int32_t)SP_Y + 30 - 5 * (int32_t)sc / 2, sc, "FREITAKT") &&
            region_is_text(120 - md_text_w(&MD_F3, 2, SP_SUB) / 2, (int32_t)SP_Y + 73, 2, SP_SUB);
        C_INK = ink, C_BG = bg;
    }
    check("splash: two tones only; at its end FREITAKT clean, MASCHINE // TAKT under it, the bar (nearly) full",
          stray == 0u && ui.splash == 2u && k && fb_count(20, 214, 190, 3, C_BG) == 190u * 3u);
    fm1_ms = 10000u + SPLASH_MS;
    ui_frame();
    check("splash: over after SPLASH_MS, HOME drawn whole", ui.splash == 0u && home_line_is("P- V1 T1 L1"));
    ui_host_init();                                  /* a key skips it, and only that */
    draw_boot_title();
    fm1_ms = 20000;
    ui_frame();
    fm1_ms += 200u;
    ui_frame();
    play = song.playing;
    press(B_PLAY);
    ui_frame();
    check("splash: a button skips it (the page at once), the button does nothing", ui.splash == 3u && !play &&
          !song.playing && home_line_is("P- V1 T1 L1"));
    ui_frame();
    check("splash: .. still held: nothing", ui.splash == 3u && !song.playing);
    release_all();
    ui_frame();
    ui_frame();
    check("splash: .. let go: the panel works again, no play", ui.splash == 0u && !song.playing);
    ui_host_init();
    draw_boot_title();
    ui_frame();
    turn(EN_K1, 3);
    ui_frame();
    check("splash: a knob skips it, and edits nothing", ui.splash == 3u && !ui.hot_t);
    ui_frame();
    check("splash: .. nothing held: over", ui.splash == 0u);
    fm1_ms = 0;
}

/* owner 2026-10-23: every colour in both polarities, the COLOR row in the order RED, RED INV, BLUE, BLUE INV, GREEN,
 * GREEN INV (each colour, then its inverse); the stored index is the one before (the old four keep 0..3, the new
 * inverses take 4, 5) */
static const char *const THEME_SHOWN[8] = {"ORANGE", "ORANGE INV", "RED", "RED INV", "BLUE", "BLUE INV", "GREEN",
                                           "GREEN INV"};

static uint32_t c8(uint16_t c, uint32_t ch)          /* RGB565 channel ch (0 r, 1 g, 2 b) as 8 bits */
{
    return ch == 0 ? (c >> 11) << 3 : ch == 1 ? ((c >> 5) & 63u) << 2 : (c & 31u) << 3;
}
static uint32_t lum(uint16_t c) { return 2u * c8(c, 0) + 7u * c8(c, 1) + c8(c, 2); }
static int near_c(uint16_t a, uint16_t b)             /* the same colour, retuned a little (as RED's pair) */
{
    uint32_t ch, d, ok = 1;
    for (ch = 0; ch < 3u; ch++) {
        d = c8(a, ch) > c8(b, ch) ? c8(a, ch) - c8(b, ch) : c8(b, ch) - c8(a, ch);
        ok &= d <= 0x30u;
    }
    return (int)ok;
}

static void test_theme_list(void)
{
    uint32_t i, j, pairs = 0, ok = NTHEMES == 8u;
    char inv[24];
    for (i = 0; i < NTHEMES; i++) {
        size_t n = strlen(THEMES[i].name);
        ok &= n <= 10u;                              /* "ORANGE INV" the longest */
        if (n > 4u && str_eq(THEMES[i].name + n - 4u, " INV"))
            continue;
        snprintf(inv, sizeof inv, "%s INV", THEMES[i].name);
        for (j = 0; j < NTHEMES; j++)
            if (str_eq(THEMES[j].name, inv)) {
                const theme_t *a = &THEMES[i], *b = &THEMES[j];
                int swapped = (lum(a->bg) > lum(a->ink)) != (lum(b->bg) > lum(b->ink));
                if (swapped && near_c(a->bg, b->ink) && near_c(a->ink, b->bg))
                    pairs++;
                else
                    printf("      %s / %s: ink and ground not swapped\n", a->name, b->name);
            }
    }
    check("themes: 8, ORANGE RED BLUE GREEN each with its INV (ink and ground swapped, the colours as the base's)",
          ok && pairs == 4u);
    for (ok = 1, i = 0; i < NTHEMES; i++)
        ok &= THEMES[theme_index(THEME_TAG | THEME_ORDER[i])].bg == THEMES[THEME_ORDER[i]].bg &&
              str_eq(THEMES[THEME_ORDER[i]].name, THEME_SHOWN[i]);
    check("themes: shown ORANGE, ORANGE INV, RED, RED INV, BLUE, BLUE INV, GREEN, GREEN INV", ok);
}

/* a stored COLOR from any older Hortator loads as the theme it was: the palette ids 0..4 (GREEN AMBER CYAN RED MONO)
 * and THEME_TAG | 0..3 (MD RED, RED ON BLACK, WHITE ON BLUE, BLACK ON GREEN, md-ui) keep their colours; the new
 * inverses are 4, 5; anything else MD RED */
static void test_theme_stored(void)
{
    static const struct { uint32_t stored; uint16_t bg, ink; } C[] = {
        {THEME_TAG | 0u, RGB(0xe0, 0x47, 0x2b), RGB(0x38, 0x10, 0x0a)},
        {THEME_TAG | 1u, RGB(0x17, 0x06, 0x05), RGB(0xf0, 0x53, 0x2f)},
        {THEME_TAG | 2u, RGB(0x24, 0x45, 0xc8), RGB(0xea, 0xf1, 0xff)},
        {THEME_TAG | 3u, RGB(0xa3, 0xbf, 0x3c), RGB(0x1c, 0x2a, 0x08)},
        {0u, RGB(0xa3, 0xbf, 0x3c), RGB(0x1c, 0x2a, 0x08)},              /* GREEN */
        {1u, RGB(0xe0, 0x47, 0x2b), RGB(0x38, 0x10, 0x0a)},              /* AMBER */
        {2u, RGB(0x24, 0x45, 0xc8), RGB(0xea, 0xf1, 0xff)},              /* CYAN */
        {3u, RGB(0xe0, 0x47, 0x2b), RGB(0x38, 0x10, 0x0a)},              /* RED */
        {4u, RGB(0x17, 0x06, 0x05), RGB(0xf0, 0x53, 0x2f)},              /* MONO */
        {THEME_TAG | 6u, RGB(0xff, 0x60, 0x00), RGB(0x10, 0x08, 0x00)},  /* ORANGE (FREITAKT) */
        {THEME_TAG | 7u, RGB(0x10, 0x08, 0x00), RGB(0xff, 0x60, 0x00)},  /* ORANGE INV */
        {THEME_TAG | 15u, RGB(0xff, 0x60, 0x00), RGB(0x10, 0x08, 0x00)},
        {0x2Au, RGB(0xff, 0x60, 0x00), RGB(0x10, 0x08, 0x00)},
    };
    uint32_t i, ok = 1, keep = settings.palette;
    for (i = 0; i < sizeof C / sizeof C[0]; i++) {
        settings.palette = C[i].stored;
        settings_init();
        if (C_BG != C[i].bg || C_INK != C[i].ink) {
            ok = 0;
            printf("      stored 0x%02x: the wrong theme\n", (unsigned)C[i].stored);
        }
    }
    check("COLOR: every stored value of an older Hortator loads as the theme it was (0..4, 0x10..0x17; else ORANGE)",
          ok);
    for (ok = 1, i = 4u; i < 6u; i++) {
        settings.palette = THEME_TAG | i;
        settings_init();
        ok &= settings.palette == (THEME_TAG | i) && C_BG == THEMES[i].bg && C_INK == THEMES[i].ink;
    }
    check("COLOR: the new inverses stored as 0x14, 0x15 (BLUE INV, GREEN INV), kept across a restart",
          ok && str_eq(THEMES[4].name, "BLUE INV") && str_eq(THEMES[5].name, "GREEN INV"));
    settings.palette = keep;
    settings_init();
}

/* MENU > COLOR steps the 8 themes in their shown order (KNOB 1 and OCT+) and wraps */
static void test_theme_menu(void)
{
    uint32_t k, ok, seen = 0;
    ui_host_init();
    ok = settings.palette == (THEME_TAG | THEME_DEFAULT);
    ui.menu = 1;
    ui.menu_sel = MI_COLOR;
    ui.force = 1;
    for (k = 1; k <= NTHEMES; k++) {
        press(B_OCTUP);
        ui_frame();
        release_all();
        ui_frame();
        ok &= str_eq(THEMES[theme_index(settings.palette)].name, THEME_SHOWN[k % NTHEMES]) &&
              C_BG == THEMES[theme_index(settings.palette)].bg;
        seen |= 1u << theme_index(settings.palette);
    }
    turn(EN_K1, -1);
    ui_frame();
    ok &= str_eq(THEMES[theme_index(settings.palette)].name, "GREEN INV");
    turn(EN_K1, -1);
    ui_frame();
    ok &= str_eq(THEMES[theme_index(settings.palette)].name, "GREEN");
    turn(EN_K1, 1);
    ui_frame();
    turn(EN_K1, 1);
    ui_frame();
    ok &= settings.palette == (THEME_TAG | THEME_DEFAULT);
    check("COLOR: a new device is ORANGE; OCT+ steps ORANGE > ORANGE INV > RED > .. > GREEN INV > ORANGE (every "
          "theme once); KNOB 1 left steps back, right on, both wrap", ok && seen == (1u << NTHEMES) - 1u);
    menu_close();
    ui_frame();
}

/* ---- the carousel (FREITAKT redesign phase D, ui_menu.c car_draw): the MENU and SAVE's PROJECT / TOOLS / COPY, one
 * item at a time (owner 2026-11-22: the title 3x, the hints 2x): the title (F3 at 6x, a page count after it at 3x)
 * and a dot an item (rows 16..59), the item's name centred about row 95 (F3 at 5x at most; on two lines at a space
 * when one would be under 3x), its value about row 146 (4x at most), a note at row 167 (2x), the hints' keycaps at
 * 4x in rows 182..239 (their text at row 188 / 215) */
#define CAR_IMID 95
static int car_item_is(const char *s)
{
    char a[24];
    uint32_t sc = md_fit_sc(s, 196, 5u), n = (uint32_t)strlen(s), i, cut = 0, best = sc;
    if (sc < 3u)
        for (i = 1; i + 1u < n && i < sizeof a; i++) {
            uint32_t k, k2;
            if (s[i] != ' ')
                continue;
            snprintf(a, sizeof a, "%.*s", (int)i, s);
            k = md_fit_sc(a, 196, 4u);
            k2 = md_fit_sc(s + i + 1u, 196, 4u);
            k = k2 < k ? k2 : k;
            if (k > best) {
                best = k;
                cut = i;
            }
        }
    if (!cut)
        return region_is_text(120 - md_text_w(&MD_F3, sc, s) / 2, CAR_IMID - 5 * (int32_t)sc / 2, sc, s);
    snprintf(a, sizeof a, "%.*s", (int)cut, s);
    return region_is_text(120 - md_text_w(&MD_F3, best, a) / 2, CAR_IMID - 6 * (int32_t)best, best, a) &&
           region_is_text(120 - md_text_w(&MD_F3, best, s + cut + 1u) / 2, CAR_IMID + (int32_t)best, best,
                          s + cut + 1u);
}
static int car_val_is(const char *s)
{
    uint32_t sc = md_fit_sc(s, 224, 4u);
    return region_is_text(120 - md_text_w(&MD_F3, sc, s) / 2, 130 + 16 - 5 * (int32_t)sc / 2, sc, s);
}
static int car_note_is(const char *s) { return region_is_text(120 - md_text_w(&MD_F3, 2, s) / 2, 167, 2, s); }
/* the title s ("PROJECT 1/3": the name at 6x, "1/3" after it at 3x on its baseline; "MENU": no count) */
static int car_title_is(const char *s)
{
    char t[24];
    const char *c = strrchr(s, ' ');
    int32_t cw, x;
    uint32_t sc;
    if (c && strchr(c, '/')) {
        snprintf(t, sizeof t, "%.*s", (int)(c - s), s);
        c++;
    } else {
        snprintf(t, sizeof t, "%s", s);
        c = 0;
    }
    cw = c ? md_text_w(&MD_F3, 3, c) + 12 : 0;
    sc = md_fit_sc(t, 236 - cw, 6u);
    x = 120 - (md_text_w(&MD_F3, sc, t) + cw) / 2;
    return sc == 6u && region_is_text(x, 18, sc, t) &&
           (!c || region_is_text(x + md_text_w(&MD_F3, sc, t) + 12, 18 + 5 * (int32_t)(sc - 3u), 3, c));
}
/* s in F3 at 4x in the hints' rows (a keycap's key at row 188 / 215, its action beside it), whole, anywhere along */
static int car_hint_has(const char *s)
{
    uint32_t y, x, w = (uint32_t)md_text_w(&MD_F3, 4, s);
    for (y = 188; y <= 215u; y += 27u)
        for (x = 0; x + w <= 240u; x++)
            if (region_is_text((int32_t)x, (int32_t)y, 4, s))
                return 1;
    return 0;
}
static uint32_t car_dot(uint32_t n, uint32_t i)       /* dot i of n: its centre x */
{
    int32_t p = n > 1u ? (int32_t)(200u / (n - 1u)) : 0;
    p = p > 12 ? 12 : p;
    return (uint32_t)(120 - p * (int32_t)(n - 1u) / 2 + p * (int32_t)i);
}
static int car_pos_is(uint32_t n, uint32_t i)        /* the dots: n of them, the i-th the big one */
{
    uint32_t d, ok = fb_count(car_dot(n, i) - 3u, 52, 6, 6, C_INK) == 36u;
    for (d = 0; d < n; d++)
        if (d != i)
            ok &= fb_count(car_dot(n, d) - 3u, 52, 6, 6, C_INK) == 4u;
    return (int)ok;
}
static int menu_value_is(uint32_t item, const char *s)   /* the MENU's carousel on item, its value s */
{
    return car_item_is(MI_NAME[item]) && car_val_is(s);
}

/* owner 2026-10-23: COLOR shows the theme's name as its value (as SPEAKER EQ's FLAT); since the carousel (phase D) the
 * value under the item's name */
static void test_theme_row_name(void)
{
    uint32_t k, ok = 1;
    ui_host_init();
    ui.menu = 1;
    for (k = 0; k < NTHEMES; k++) {
        settings.palette = THEME_TAG | THEME_ORDER[k];
        theme_set(settings.palette);
        ui.menu_sel = MI_COLOR;
        ui.force = 1;
        ui_frame();
        if (!menu_value_is(MI_COLOR, THEME_SHOWN[k])) {
            ok = 0;
            printf("      COLOR %s: no name\n", THEME_SHOWN[k]);
        }
    }
    check("MENU: COLOR shows the theme's name (ORANGE .. GREEN INV) under the item, centred", ok);
    settings.palette = THEME_TAG | THEME_DEFAULT;
    theme_set(settings.palette);
    menu_close();
    ui_frame();
}

/* Review Focus 1: after a theme change every region repaints (nothing keeps the old theme's colours) */
static void test_theme_repaint(void)
{
    uint32_t i, stale = 0;
    ui_host_init();
    tap(B_EDIT);
    ui_frame();
    ui_frame();
    ui.menu = 1;
    ui.menu_sel = MI_COLOR;
    ui.force = 1;
    turn(EN_K1, 1);                                  /* ORANGE -> ORANGE INV -> RED (a detent per frame) */
    ui_frame();
    turn(EN_K1, 1);
    ui_frame();
    menu_close();
    ui_frame();
    ui_frame();
    for (i = 0; i < 240u * 240u; i++)
        stale += fb[i] == THEMES[THEME_DEFAULT].bg || fb[i] == THEMES[THEME_DEFAULT].ink;
    check("theme change: the whole screen repaints in the new theme (no ORANGE pixel left)",
          settings.palette == (THEME_TAG | 0u) && stale == 0);
    theme_set(THEME_TAG | THEME_DEFAULT);
}

/* spec 2026-10-19-md-ui §5: the pixels of colour c in a screen rectangle */
static uint32_t fb_count(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t n = 0, x, y;
    for (y = y0; y < y0 + h; y++)
        for (x = x0; x < x0 + w; x++)
            n += fb[y * 240u + x] == c;
    return n;
}

/* §5 top bar: ink bar 0..15, the state glyph in BG at 4,3: ■ stopped, ▶ playing, ● recording, outlined ● armed;
 * the FX tag (BG box) after the BPM while a PERFORM knob effect is on */
static void test_topbar(void)                         /* FREITAKT: the theme's ground, the battery top right */
{
    ui_host_init();
    tap(B_EDIT);
    song.playing = 0;
    song.rec = 0;
    ui.force = 1;
    ui_frame();
    check("top bar: the theme's ground (rows 0..15 >= 80 % BG), no transport icon",
          fb_count(0, 0, 240, 16, C_BG) > 240u * 16u * 8u / 10u && fb_count(0, 0, 200, 16, C_INK) == 0u);
    check("top bar: the battery's outline top right (x 220..233, y 4..11)", fb_count(220, 4, 14, 1, C_INK) == 14u &&
          fb_count(220, 11, 14, 1, C_INK) == 14u);
    song.playing = 1;
    ui_frame();
    check("top bar: playing: still nothing left of the battery", fb_count(0, 0, 200, 16, C_INK) == 0u);
    song.rec = 1u << song.sel;
    ui_frame();
    check("top bar: the selected track armed: the record icon left", fb_count(0, 0, 40, 16, C_INK) > 10u);
    song.rec = 0;
    song.playing = 0;
    ui_frame();
}

/* §7 messages: a small popup over the graph area; the top bar keeps its BPM */
static void test_msg_popup(void)
{
    uint32_t k, edge = 0;
    ui_host_init();
    tap(B_EDIT);
    ui.force = 1;
    ui_frame();
    ui_message("SAVED A1");
    ui_frame();
    for (k = 0; k < 240u; k++)                     /* a 2 px right edge: two ink columns with BG left of them */
        if (k > 2u && fb[120u * 240u + k] == C_INK && fb[120u * 240u + k - 1u] == C_INK &&
            fb[120u * 240u + k - 2u] == C_BG && k < 239u)
            edge = k;
    check("message: a popup window over the graph area, its 2 px shadow edge", edge > 150u && edge < 239u);
    check("message: the top bar stays (the battery drawn)", fb_count(220, 4, 14, 1, C_INK) == 14u);
    for (k = 0; k < 45u; k++)
        ui_frame();
    {                                                /* (the LIST rows may have ink there: as a full repaint) */
        static uint16_t was[240u * 240u];
        memcpy(was, fb, sizeof was);
        ui.force = 1;
        ui_frame();
        check("message: gone after its time (the page as a full repaint draws it)", !ui.msg_t &&
              !memcmp(was, fb, sizeof was));
    }
}


/* spec 2026-10-19-md-ui §8, graphs I (two tones): the model's name in F3 4x, left (owner 2026-10-22: every engine
 * view as FM's); STEP's bars; PROJECT's cursor rows inverted */
static void test_graph_model_name(void)              /* FREITAKT LIST: the MODEL row, the hit's band above the rows */
{
    ui_host_init();
    tap(B_EDIT);
    ui.force = 1;
    ui_frame();
    check("SOUND: row 1 the machine (K909) and its preset, row 2 TUNE; the hit waveform's band above (y 16..63)",
          col_has(0, "K909|") && col_has(1, "TUNE|") && fb_count(0, 16, 240, 48, C_INK) > 50u &&
              fb_count(0, 16, 240, 48, C_BG) > 240u * 48u / 2u);
}

static void test_perform_buttons(void)
{
    uint32_t x = 4u + 47u, y = Y_GRAPH + 3u, on, off;
    ui_host_init();
    press(B_FX);
    fx_hold_frames(500);
    off = fb_count(x + 2u, y + 2u, 40, 23, C_INK);
    perf_held |= 1u << PF_BLACK[1];
    ui.force = 1;
    fx_hold_frames(1);
    on = fb_count(x + 2u, y + 2u, 40, 23, C_INK);
    check("PERFORM: a button an outline, held: solid (ink with its name in BG)", ui.layer && off < 200u && on > 690u);
    perf_held = 0;
    release_all();
    ui_frame();
}

/* FREITAKT HOME (Model:Cycles style): only the status line "P1 V2 T6 L1" large (y 72..167), the CPU load and the
 * battery small (y 0..23); nothing else (no bars, no strip, no scope, no breadcrumb); an idle HOME sends nothing */
static void test_home_strip(void)
{
    uint32_t y, ink = 0, px;
    ui_host_init();
    TSEL->p[P_SLEN] = 32;
    ui.bank = 1;
    song.cpu_q8 = 128;
    ui.force = 1;
    ui_frame();
    check("HOME: the status line P- V1 T1 L2 (no slot loaded, variation 1, track 1, step page 2), large", ui.home &&
          home_line_is("P- V1 T1 L2"));
    check("HOME: CPU 50% small at the top left", region_is_text(8, 7, 2, "CPU 50%"));
    {
        char bb[12];
        str_cpy(bb, "BPM ", sizeof bb);
        fmt_int(bb + 4, song.g[G_BPM]);
        check("HOME: the BPM large in y 24..71 (FREITAKT; KNOB 1 turns it)",
              region_is_text(120 - md_text_w(&MD_F3, 4, bb) / 2, 24 + 14, 4, bb));
    }
    for (y = 208; y < 240u; y++)
        ink += fb_count(0, y, 240, 1, C_INK);
    check("HOME: nothing else (y 208..239 plain; no top bar, no breadcrumb)", ink == 0u &&
          fb_count(0, 0, 4, 24, C_INK) == 0u);
    ui_frame();
    px = host_blit_px;
    ui_frame();
    ui_frame();
    check("HOME: idle, nothing redrawn", host_blit_px == px);
    var_cur = 1;
    ui_frame();
    check("HOME: variation 2 shows (V2)", home_line_is("P- V2 T1 L2"));
    var_cur = 0;
    song.cpu_q8 = 0;
    ui_frame();
    {                                                /* a chain A1 A3 C2 B1: its line under the status line */
        static const uint8_t CH[4] = {0, 0 | 2u << 2, 2 | 1u << 2, 1};
        uint32_t k;
        int32_t x0;
        for (k = 0; k < 3u; k++)
            project_save(k);
        project_load(0);
        ui.bank = 0;
        ui.msg_t = 0;                                /* (LOADED) */
        ui_frame();
        check("HOME: slot A loaded: P1", home_line_is("P1 V1 T1 L1"));
        check("HOME: no chain, no chain line", fb_count(0, HM_Y_CH, 240, HM_H_CH, C_INK) == 0u);
        chain_set(CH, 4);
        ui_frame();
        shot("build/ui_shots/fixes/home_chain.ppm");
        x0 = 120 - (md_text_w(&MD_F3, 3, "CH") + 6 + 4 * 33) / 2;
        check("HOME: a chain runs: CH and its entries (pattern and variation: A1 A3 C2 B1) under the status line, the "
              "one playing inverted",
              region_is_text(x0, HM_Y_CH + 12, 3, "CH") && fb_count(0, HM_Y_CH, 240, HM_H_CH, C_INK) > 300u &&
              region_is_text(x0 + md_text_w(&MD_F3, 3, "CH") + 15 + 33, HM_Y_CH + 12, 3, "A3") &&
              region_is_text(x0 + md_text_w(&MD_F3, 3, "CH") + 15 + 66, HM_Y_CH + 12, 3, "C2") &&
              region_is_text(x0 + md_text_w(&MD_F3, 3, "CH") + 15 + 99, HM_Y_CH + 12, 3, "B1"));
        chain_clear();
        ui_frame();
        check("HOME: the chain cleared, its line gone", fb_count(0, HM_Y_CH, 240, HM_H_CH, C_INK) == 0u);
    }
}

/* spec 2026-10-19-md-ui §7: confirms and TEMPO as MD popups over the live page; MENU a popup window (the cursor an
 * inverted bar); NAME's block cursor */
static void test_confirm_popup(void)
{
    uint32_t i;
    ui_host_init();
    trk[0].step[0].on = 1;
    open_step_page();
    ui.confirm = 6;                                  /* (CLEAR ALL LEARNED?: phase C took the REC hold's CLEAR TRACK?) */
    for (i = 0; i < 2; i++)
        ui_input();
    ui.force = 1;
    ui_frame();
    check("confirm: a popup over the page (the battery and the page's rows still drawn)", ui.confirm != 0u &&
          fb_count(220, 4, 14, 1, C_INK) == 14u && fb_count(0, 200, 240, 40, C_INK) > 100u);
    check("confirm: the popup's 2 px shadow edge at x 210..211, its title tab over the top edge",
          fb[120u * 240u + 210u] == C_INK && fb[120u * 240u + 211u] == C_INK && fb[120u * 240u + 209u] == C_BG &&
          fb_count(80, 76, 80, 6, C_INK) > 100u);
    press(B_OCTDN);
    ui_frame();
    release_all();
    ui_frame();
}

static void test_tempo_popup(void)
{
    uint32_t k;
    ui_host_init();
    open_tracks();
    ui.force = 1;
    ui_frame();
    sel(1);                                          /* BPM + 1 */
    ui_frame();
    check("TEMPO: changing BPM opens the TEMPO popup (its shadow edge at x 208..209)", ui.bpm_t &&
          fb[120u * 240u + 208u] == C_INK && fb[120u * 240u + 209u] == C_INK && fb[120u * 240u + 207u] == C_BG);
    for (k = 0; k < 45u; k++)
        ui_frame();
    check("TEMPO: gone when bpm_t runs out", !ui.bpm_t &&
          !(fb[120u * 240u + 208u] == C_INK && fb[120u * 240u + 209u] == C_INK && fb[120u * 240u + 207u] == C_BG));
}

static void test_name_block_cursor(void)
{
    ui_host_init();
    name_open(NK_RENAME, 0);
    ui_frame();
    check("NAME: an ink title bar; the cursor a solid block under its slot (after PROJECT A: slot 10)",
          name_on() && fb_count(0, 0, 240, 16, C_INK) > 240u * 16u * 7u / 10u &&
          fb_count(12u + 18u * 9u, 70, 18, 3, C_INK) == 54u && fb_count(12u + 18u * 8u, 70, 18, 3, C_INK) == 0u);
    nm.kind = NK_NONE;
    ui.force = 1;
    ui_frame();
}

/* spec 2026-10-19-md-ui §10: the review sheet: the key pages in each theme into build/ui_shots/themes/<theme>/, every
 * one in two tones only (the theme's BG and INK) */
static const char *const THEME_DIR[8] = {"md-red", "red-on-black", "white-on-blue", "black-on-green", "blue-on-white",
                                         "green-on-black", "black-on-orange", "orange-on-black"};   /* by stored index */
static uint32_t sheet_t;
static uint32_t sheet_stray;
static void sheet_snap(const char *page)
{
    char path[128];
    uint32_t i, k;
    for (k = 0; k < 3u; k++)
        ui_frame();
    snprintf(path, sizeof path, "build/ui_shots/themes/%s/%s.ppm", THEME_DIR[sheet_t], page);
    shot(path);
    for (i = 0; i < 240u * 240u; i++)
        if (fb[i] != C_BG && fb[i] != C_INK) {
            if (!sheet_stray)
                printf("     two tones: %s/%s has 0x%04x at %u,%u\n", THEME_DIR[sheet_t], page, fb[i], i % 240u, i / 240u);
            sheet_stray++;
        }
}
static void sheet_open(void)                          /* a fresh device in theme sheet_t */
{
    ui_host_init();
    settings.palette = THEME_TAG | sheet_t;
    theme_set(settings.palette);
    ui.force = 1;
}
static void test_theme_sheet(void)
{
    uint32_t i;
    sheet_stray = 0;
    for (sheet_t = 0; sheet_t < NTHEMES; sheet_t++) {
        sheet_open();
        draw_boot_title();
        shot("build/ui_shots/themes/boot.ppm");
        ui.splash = 0;
        sheet_snap("00_home");
        sheet_open();
        tap(B_EDIT);
        sheet_snap("01_sound");
        sheet_open();
        tap(B_FX);
        sheet_snap("02_filter");
        sheet_open();
        trk[0].step[0].on = trk[0].step[0].acc = trk[0].step[4].on = 1;
        open_step_page();
        sheet_snap("03_step");
        sheet_open();
        tap(B_LFO);
        sheet_snap("04_lfo");
        sheet_open();
        sel(1);
        sel(1);
        sheet_snap("05_comp");
        sheet_open();
        tap(B_SAVE);
        sheet_snap("06_project");
        sheet_open();
        open_tracks();
        sheet_snap("07_tracks");
        sheet_open();
        press(B_FX);
        fx_hold_frames(500);
        sheet_snap("08_perform");
        release_all();
        sheet_open();
        ui.menu = 1;
        sheet_snap("09_menu");
        menu_close();
        sheet_open();
        trk[0].step[0].on = 1;
        open_step_page();
        fm1_in.buttons |= 1u << panel.btn[B_REC];
        for (i = 0; i < 800; i++)
            ui_input();
        release_all();
        ui.force = 1;
        sheet_snap("10_confirm");
        press(B_OCTDN);
        ui_frame();
        release_all();
        sheet_open();
        name_open(NK_RENAME, 0);
        sheet_snap("11_name");
        nm.kind = NK_NONE;
        sheet_open();
        press(B_SEQ);
        btn_hold_frames(B_SEQ, 500);
        sheet_snap("12_tools");
        release_all();
        sheet_open();
        safe_start = 1;
        sheet_snap("13_safe_start");
        safe_start = 0;
        sheet_open();
        tap(B_EDIT);
        turn(EN_PRESET, 1);                          /* the PRESETS popup */
        ui_frame();
        sheet_snap("14_preset_popup");
        sheet_open();
        hold(B_EDIT);                                /* the PRESETS list */
        sheet_snap("15_preset_list");
        release_all();
        tap(B_EDIT);
    }
    sheet_t = 0;
    theme_set(THEME_TAG | THEME_DEFAULT);
    check("theme sheet: 16 screens in each of the 8 themes, every pixel the theme's BG or INK", sheet_stray == 0u);
}

/* final review fixes (2026-10-19-md-ui): a 4-glyph value whole; ZOOM on HOME; a shorter message repaints; popups do
 * not let the graph under them redraw */
static void test_fix_value_4_glyphs(void)
{
    char l[16], v[16];
    const char *u;
    ui_host_init();
    open_tracks();
    TSEL->p[P_PAN] = -64;                            /* PAN fully left: -100 */
    turn(EN_K4, -1);                                 /* (at its end: stays) */
    ui_frame();
    kp_text(l, v, &u);
    check("popup: PAN fully left shows -100 whole (4 glyphs), not -10", kp.on && str_eq(l, "PAN") && str_eq(v, "-100") &&
          fb_count(0, KP_VY, 240, KP_VH, C_INK) > 0u);
    TSEL->p[P_PAN] = 0;
}

/* the knob popup (FREITAKT redesign, rule 5; ui.c kp): a parameter knob's first turn clears the screen to its name
 * and value, large (the top bar gone too) */
static void test_kpop_first_turn(void)
{
    ui_host_init();
    tap(B_EDIT);                                     /* SOUND 1/n: MODEL, then the machine's knobs */
    ui.force = 1;
    ui_frame();
    check("popup: none before a turn", !kp.on && list_rows_drawn());
    turn(EN_K2, 1);
    ui_frame();
    check("popup: up on the first turn of a parameter knob (KNOB 2: the SOUND page's second row)",
          kp.on == 2u && kp.kind == KP_PAR && kp.col == 1u && kp.vp == &TSEL->p[page_id(cur_page(), 1)]);
    check("popup: the screen cleared (no top bar, no rows), the name and the value large",
          fb_count(0, 0, 240, KP_NY, C_INK) == 0u && fb_count(0, KP_NY, 240, KP_NH, C_INK) > 100u &&
          fb_count(0, KP_VY, 240, KP_VH, C_INK) > 600u && fb_count(0, KP_VY + KP_VH, 240, 240 - KP_VY - KP_VH, C_INK) == 0u);
    shot("build/ui_shots/popup.ppm");
}

/* the popup's later turns send its value's row only (no fill, one blit of 240 x KP_VH) */
static void test_kpop_value_row_only(void)
{
    ui_host_init();
    tap(B_EDIT);
    ui_frame();
    turn(EN_K2, 1);
    ui_frame();
    ui_frame();
    host_fill_px = host_blit_px = host_wins = 0;
    ui_frame();
    check("popup: an idle frame sends nothing", host_fill_px == 0u && host_blit_px == 0u);
    host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
    turn(EN_K2, 1);
    ui_frame();
    check("popup: a later turn sends its value's row only (one window, 240 x 80, no fill)",
          host_fill_px == 0u && host_wins == 1u && host_blit_px == 240u * KP_VH);
}

/* the popup goes HOT_MS (3 s) of real time after the last turn, frames or not; each turn starts the 3 s again; then the
 * page under it whole once */
static void test_kpop_time(void)
{
    uint32_t t0, ms, k, at2;
    ui_host_init();
    tap(B_EDIT);
    ui_frame();
    turn(EN_K2, 1);
    ui_frame();
    t0 = host_ticks;
    for (k = 0; k < 400u && kp.on; k++)
        grid_wait(50);
    ms = (host_ticks - t0) / (1000u * FM1_TICKS_PER_US);
    check("popup: gone 3 s after the turn (real time)", !kp.on && ms >= 2950u && ms <= 3150u);
    check("popup: gone, the page whole again (its rows)", list_rows_drawn() && fb_count(0, KP_VY, 240, KP_VH, C_INK) > 0u);
    turn(EN_K2, 1);
    ui_frame();
    grid_wait(2000);
    turn(EN_K2, 1);                                  /* another turn 2 s on: 3 s from this one */
    ui_frame();
    grid_wait(2000);
    at2 = kp.on;
    grid_wait(1200);
    check("popup: each turn starts the 3 s again", at2 && !kp.on);
}

/* OCT- while the popup is up: the parameter back to its value before the session's first turn, the popup stays and
 * shows it; OCT-'s own job (KEYS: the octave down) not done. A second OCT-: nothing more */
static void test_kpop_undo(void)
{
    int16_t v0, *vp;
    char l[16], v[16], w[16];
    const char *u;
    ui_host_init();
    tap(B_SCL);                                      /* KEYS: OCT- would be the keyboard's octave down */
    tap(B_EDIT);
    ui_frame();
    vp = &TSEL->p[page_id(cur_page(), 1)];
    v0 = *vp;
    turn(EN_K2, 1);
    ui_frame();
    host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
    turn(EN_K2, 2);
    ui_frame();
    check("undo: the turns moved the value", *vp != v0 && kp.on);
    tap(B_OCTDN);
    check("undo: OCT- puts the value back as before the session", *vp == v0);
    check("undo: OCT-'s own job not done (KEYS' octave unchanged)", song.octave == 0 && ui.keys);
    kp_text(l, v, &u);
    param_format(track_desc(TSEL, page_id(cur_page(), 1)), v0, w, &u);
    check("undo: the popup stays, showing the value back", kp.on && str_eq(v, w));
    *vp = (int16_t)(v0 + 1);
    tap(B_OCTDN);
    check("undo: once (a second OCT- puts nothing back)", *vp == v0 + 1 && song.octave == 0);
    *vp = v0;
    grid_wait(3200);
    tap(B_OCTDN);
    check("undo: the popup gone, OCT- is OCT- again (the octave down)", !kp.on && song.octave == -1);
    tap(B_OCTUP);
    tap(B_SCL);
}

/* undo of a session on TRACKS (a muted track's LEVEL: the first turn unmutes; OCT-: LEVEL and MUTE back), MODEL (the
 * sound back), HOME's BPM; TRACKS' TRACK and an unassigned HOME knob open no popup */
static void test_kpop_undo_kinds(void)
{
    track_t *t;
    int16_t lvl, bpm;
    uint32_t m0, pre0;
    ui_host_init();
    open_tracks();
    t = TSEL;
    t->p[P_MUTE] = 1;
    lvl = t->p[P_LEVEL];
    turn(EN_K2, 3);
    ui_frame();
    check("TRACKS LEVEL: popup up, the first turn unmuted", kp.on && !t->p[P_MUTE]);
    tap(B_OCTDN);
    check("TRACKS LEVEL: OCT- mutes again, LEVEL as before", t->p[P_MUTE] == 1 && t->p[P_LEVEL] == lvl);
    t->p[P_MUTE] = 0;
    grid_wait(3200);
    turn(EN_K1, 1);
    ui_frame();
    check("TRACKS TRACK: no popup, the next track", !kp.on && song.sel == 1u);
    track_select(0);
    ui_frame();
    tap(B_EDIT);
    ui_frame();
    m0 = (uint32_t)TSEL->p[P_MODEL];
    pre0 = TSEL->pre;
    turn(EN_K1, 1);
    ui_frame();
    check("MODEL: popup up, another machine", kp.on && kp.kind == KP_MODEL && (uint32_t)TSEL->p[P_MODEL] != m0);
    tap(B_OCTDN);
    check("MODEL: OCT- the machine and its preset back", (uint32_t)TSEL->p[P_MODEL] == m0 && TSEL->pre == pre0);
    grid_wait(3200);
    tap(B_HOME);
    while (!ui.home)
        tap(B_HOME);
    bpm = song.g[G_BPM];
    turn(EN_K1, 4);
    ui_frame();
    check("HOME BPM: popup up (BPM)", kp.on && kp.vp == &song.g[G_BPM] && song.g[G_BPM] != bpm);
    shot("build/ui_shots/popup_home_bpm.ppm");
    tap(B_OCTDN);
    check("HOME BPM: OCT- the tempo back", song.g[G_BPM] == bpm);
    grid_wait(3200);
    turn(EN_K3, 1);
    ui_frame();
    check("HOME KNOB 3 (nothing): no popup", !kp.on);
}

/* ENV STEPS, a step held: a knob's turn locks it (the popup shows the lock, inverted); OCT- takes the lock away (none
 * before) or puts the old one back, and does not clear the step's locks (OCT-'s job with a step held) */
static void test_kpop_undo_lock(void)
{
    int16_t v;
    uint32_t id;
    ui_host_init();
    tap(B_EDIT);
    tap(B_ENV);                                      /* STEPS */
    ui_frame();
    id = lock_id(1);
    check("lock undo: KNOB 2 here can lock", id < P_COUNT && env_steps());
    lock_set(song.sel, 3, P_PAN, 10);                /* (another lock on the step: OCT- would clear it) */
    keys(1u << STEP_KEY[3]);
    ui_frame();
    turn(EN_K2, 2);
    ui_frame();
    check("lock undo: the turn locked step 4, the popup a lock's", lock_get(song.sel, 3, id, &v) && kp.on &&
          kp.kind == KP_LOCK);
    shot("build/ui_shots/popup_lock.ppm");
    tap(B_OCTDN);
    check("lock undo: OCT- takes the new lock away, the step's other lock kept", !lock_get(song.sel, 3, id, &v) &&
          lock_get(song.sel, 3, P_PAN, &v) && v == 10);
    grid_wait(3200);                                 /* (the popup gone: the next turn a new session) */
    lock_set(song.sel, 3, id, 20);
    turn(EN_K2, 3);
    ui_frame();
    check("lock undo: a new session from the old lock (20)", lock_get(song.sel, 3, id, &v) && v != 20);
    tap(B_OCTDN);
    check("lock undo: OCT- the old lock back", lock_get(song.sel, 3, id, &v) && v == 20);
    keys(0);
    ui_frame();
    lock_clear_track(song.sel);
    tap(B_ENV);
}

static void test_fix_msg_shrink(void)
{
    uint32_t k, e = 0;
    ui_host_init();
    tap(B_EDIT);
    ui.force = 1;
    ui_frame();
    ui_message("A MUCH LONGER MESSAGE");
    ui_frame();
    for (k = 3; k < 239u; k++)
        if (fb[120u * 240u + k] == C_INK && fb[120u * 240u + k - 1u] == C_INK && fb[120u * 240u + k - 2u] == C_BG)
            e = k;
    ui_message("OK");
    ui_frame();
    check("message: a shorter one replacing it leaves no edge of the longer popup", e > 150u &&
          !(fb[120u * 240u + e] == C_INK && fb[120u * 240u + e - 1u] == C_INK && fb[120u * 240u + e - 2u] == C_BG));
}

static void test_fix_popup_holds_graph(void)
{
    uint32_t k, during;
    ui_host_init();
    open_tracks();
    transport_req = 1;
    for (k = 0; k < 5u; k++)
        ui_frame();
    sel(1);                                          /* BPM + 1: TEMPO */
    ui_frame();
    host_graph_blits = 0;
    for (k = 0; k < 6u; k++) {
        trk[k].peak = (int16_t)(8000 + 3000 * (int32_t)k);   /* the meters move */
        ui_frame();
    }
    during = host_graph_blits;
    check("TEMPO over a playing TRACKS: the rows under it are not redrawn (no flicker), the popup stays",
          ui.bpm_t && during == 0u && fb[120u * 240u + 208u] == C_INK && fb[120u * 240u + 209u] == C_INK);
    for (k = 0; k < 45u; k++)
        ui_frame();
    check("TEMPO gone: the rows redrawn again", !ui.bpm_t && host_graph_blits > 0u);
    transport_req = 2;
    ui_frame();
}

static void snap_page(const char *name)
{
    char path[96];
    uint32_t i;
    for (i = 0; i < 3; i++) {
        if (i == 2u && !ui.home && !ui.layer && cur_page()->graph == GR_MODEL)
            vg_settle();                             /* (the engine view's rendered hit whole: the frames' budgets at
                                                      * once) */
        ui_frame();
    }
    snprintf(path, sizeof path, "build/ui_shots/%s.ppm", name);
    shot(path);
}

/* a drum session for the screens: four on the floor, snare and clap on 2 / 4, hats, a tom fill (muted);
 * track 3 armed (FREITAKT: 6 tracks, Hortator's rim and crash gone) */
static void kit_session(void)
{
    static const char *const PAT[NTRK] = {
        "X...x...x...x...",   /* K909: four on the floor, accent on 1 */
        "....x.......X...",   /* S808 */
        "............x...",   /* C808 */
        "x.x.x...x.x.x...",   /* HATC */
        "......x.......x.",   /* HATO */
        ".............xxX",   /* TOM: fill (muted) */
    };
    uint32_t i, k;
    for (i = 0; i < NTRK; i++)
        for (k = 0; k < 16; k++) {
            trk[i].step[k].on = PAT[i][k] != '.';
            trk[i].step[k].acc = PAT[i][k] == 'X';
        }
    trk[5].p[P_MUTE] = 1;
    song.rec = 1u << 2;
}

/* TRACKS: one row per track; a step's cell is lit exactly when the step is on */
/* TRACKS (FREITAKT redesign): the 6 tracks a list, a row each (number, machine); a muted track's row inverted, the
 * selected one a pointer at the left */
static void test_tracks_rows(void)
{
    uint32_t r, ok = 1;
    ui_host_init();
    kit_session();
    open_tracks();
    for (r = 0; r < NTRK; r++)
        trk[r].p[P_MUTE] = 0;
    trk[2].p[P_MUTE] = 1;
    track_select(1);
    ui.force = 1;
    ui_frame();
    for (r = 0; r < NTRK; r++) {
        uint32_t y = TK_Y0 + r * TK_H, ink = fb_count(0, y, 240, TK_H - 1u, C_INK);
        if (r == 2u)
            ok &= ink > 240u * (TK_H - 1u) * 7u / 10u;   /* muted: inverted */
        else
            ok &= ink > 60u && ink < 240u * (TK_H - 1u) * 3u / 10u;
        ok &= (fb_count(3, y + 9u, 7, 13, r == 2u ? C_BG : C_INK) > 40u) == (r == 1u);   /* the pointer: the
                                                                                    * selected track's */
    }
    check("TRACKS: 6 rows (number + machine), the muted one inverted, the selected one pointed at", ok);
    snap_page("tracks_muted");
    turn(EN_K1, 1);
    ui_frame();
    check("TRACKS: KNOB 1 moves the pointer (no popup)", !kp.on && song.sel == 2u &&
          fb_count(3, TK_Y0 + 2u * TK_H + 9u, 7, 13, C_BG) > 40u);
    trk[2].p[P_MUTE] = 0;
}

static void hold(uint32_t label)                    /* a button held 0.8 s (1 ms per tick), then let go */
{
    uint32_t k;
    press(label);
    for (k = 0; k < 800; k++)
        ui_input();
    release_all();
    ui_frame();
}

/* a LIST page (ui_draw.c lrow_draw): its first row's label drawn, and a row in all with ink at the left */
static int list_rows_drawn(void)
{
    int32_t h = list_h(), y = list_y0();
    return fb_count(6, (uint32_t)y + 2u, 110, (uint32_t)h - 4u, C_INK) > 40u;
}

/* a screenshot of HOME, every page of PAGES[] (reached with its own button; SAVE's carousels), the menu (carousel) and
 * the clear's warning: build/ui_shots/NN_title[_n].png */
static void test_screens(void)
{
    uint32_t i, k, n = 0, ok = 1, reach = 1;
    char name[40];
    ui_host_init();
    kit_session();                                   /* a pattern to show */
    transport_req = 1;
    snap_page("00_home");
    ok &= fb_lit(0, 24) > 30 && fb_lit(72, 168) > 200;   /* HOME: CPU / battery, the status line */
    for (i = 0; i < NPAGES; i++) {
        const page_t *pg = &PAGES[i];
        uint32_t nth = i - page_first(pg->fam), j = 0;
        if (page_hidden(i) || pg->fam == FAM_EDT)   /* EDIT page 4: no engine has that many knobs yet; the web
                                                      * editor's own pages */
            continue;
        char *d;
        const char *c;
        for (k = 0; k < NPAGES && (cur_page() != pg || ui.home); k++) {   /* its button until it shows */
            if (pg->fam == FAM_MIX) {                /* HOME's TRACKS mode: HOME (toggles) */
                tap(B_HOME);
                continue;
            } else {
                press(FAM_BTN[pg->fam]);
            }
            ui_frame();
            release_all();
            ui_frame();                              /* (FX opens its pages on the release) */
        }
        reach &= cur_page() == pg && !ui.home;
        snprintf(name, sizeof name, "%02u_", (unsigned)++n);
        for (d = name + 3, c = pg->title; *c && j < 20u; c++, j++)
            *d++ = *c == '/' ? '-' : (char)(*c >= 'A' && *c <= 'Z' ? *c + 32 : *c);
        *d = 0;
        if (nth && str_eq(PAGES[i - 1u].title, pg->title)) {   /* SOUND 2/3, COMP 2/2: the n-th of its title */
            uint32_t same = 1, q;
            for (q = page_first(pg->fam); q < i; q++)
                same += str_eq(PAGES[q].title, pg->title);
            snprintf(d, sizeof name - (size_t)(d - name), "_%u", (unsigned)same);
        }
        snap_page(name);
        if (!(fb_count(220, 4, 16, 8, C_INK) > 20u &&                   /* the battery */
              (list_on() ? list_rows_drawn() : save_car_on() ? fb_lit(44, 131) > 100 && fb_lit(200, 239) > 100 :
                                                fb_lit(26, 70) > 100 && fb_lit(182, 240) == 0u) &&   /* the LIST
                                                      * rows, SAVE's carousel, else the knob cells (no step strip,
                                                      * no crumb) */
              (pg->graph != GR_MIX || fb_lit(74, 198) > 300))) {   /* (TRACKS: 8 rows) */
            ok = 0;
            printf("      %s: not drawn whole\n", pg->title);
        }
    }
    check("every page of PAGES[] is reached with its button (not the editor's)", reach);
    check("every page draws the battery and its rows / cells; nothing at the bottom but LIST rows", ok);
    press(B_HOME);
    ui_frame();
    release_all();
    ui_frame();
    hold(B_HOME);                                    /* HOME held: the menu */
    snap_page("90_menu");
    check("HOME held: the menu draws", ui.menu == 1 && fb_lit(20, 230) > 300);
    for (k = 0; k < MI_CLOCK; k++)                   /* SELECT: one item per detent (the carousel) */
        sel(1);
    snap_page("91_menu_clock");
    check("menu: SELECT to CLOCK (a former GLOBAL setting) draws it", ui.menu == 1 && car_item_is("CLOCK"));
    press(B_OCTDN);
    ui_frame();
    release_all();
    seq_open("PATTERN");                             /* (SEQ: PATTERN) */
    press(B_REC);                                    /* REC + SEL held: the clear's warning (phase C) */
    press(B_SCL);
    ui_frame();
    host_ticks += 2100u * 1000u * FM1_TICKS_PER_US;
    snap_page("92_clear_countdown");
    check("REC + SEL held 2 s: the warning CLEAR T1 3 draws, the screen inverted", ui.clr == 3u &&
          fb_count(0, 0, 240, 240, C_INK) > 240u * 240u * 3u / 4u);
    release_all();
    ui_frame();
    check("let go: the warning gone, nothing cleared (screens in build/ui_shots)", !ui.clr && !ui.confirm);
    press(B_HOME);
    ui_frame();
    release_all();
    ui_frame();
    fm1_irq_off();
    old_set_model(TSEL, DM_KBOOM);
    fm1_irq_on();
    for (k = 0; k < NPAGES && (cur_page() != &PAGES[0] || ui.home); k++) {   /* SOUND 1/2: its knobs */
        press(B_EDIT);
        ui_frame();
        release_all();
        ui_frame();
    }
    snap_page("93_sound_kboom");
    check("an M1-C model's SOUND page draws (KBOOM: TUNE DECAY TONE PUNCH)", TSEL->model == DM_KBOOM &&
          cur_page() == &PAGES[0] && str_eq(PAGES[0].title, "SOUND") && fb_lit(26, 70) > 100 && fb_lit(202, 240) > 100);
}

/* every engine's SOUND pages, chosen and edited through the UI, for looking at: build/ui_shots/engines/
 * NN_name_P_default / _min / _max (page P = 1 or 2; every knob of the page turned to its end) */
static void engine_page_shots(uint32_t mi, uint32_t pg)
{
    char name[64], low[8];
    uint32_t k, j;
    for (j = 0; N_MODEL[mi][j] && j < sizeof low - 1; j++)
        low[j] = (char)(N_MODEL[mi][j] >= 'A' && N_MODEL[mi][j] <= 'Z' ? N_MODEL[mi][j] + 32 : N_MODEL[mi][j]);
    low[j] = 0;
    for (k = 0; k < NPAGES && (cur_page() != &PAGES[pg] || ui.home); k++) {
        press(B_EDIT);
        ui_frame();
        release_all();
        ui_frame();
    }
    snprintf(name, sizeof name, "engines/%02u_%s_%u_default", (unsigned)mi, low, (unsigned)pg + 1u);
    snap_page(name);
    for (k = 0; k < 4; k++)                          /* (not MODEL: it would change the engine) */
        if (page_id(cur_page(), k) != P_MODEL)
            turn(EN_K1 + k, -999);
    ui_frame();
    snprintf(name, sizeof name, "engines/%02u_%s_%u_min", (unsigned)mi, low, (unsigned)pg + 1u);
    snap_page(name);
    for (k = 0; k < 4; k++)
        if (page_id(cur_page(), k) != P_MODEL)
            turn(EN_K1 + k, 999);
    ui_frame();
    snprintf(name, sizeof name, "engines/%02u_%s_%u_max", (unsigned)mi, low, (unsigned)pg + 1u);
    snap_page(name);
}

static void test_engine_screens(void)
{
    uint32_t mi, k, n = 0;
    for (mi = 0; mi < NMODELS; mi++) {
        ui_host_init();
        for (k = 0; k < 2u * NMODELS && (uint32_t)TSEL->p[P_MODEL] != mi; k++)   /* the next engine */
            model_step(1);
        for (k = 0; k < 4u; k++)                     /* every EDIT page this engine shows */
            if (!page_hidden(page_first(FAM_SND) + k))
                engine_page_shots(mi, page_first(FAM_SND) + k);
        n++;
    }
    printf("     engines: %u engines, SOUND pages at default / min / max in build/ui_shots/engines\n", (unsigned)n);
}

/* ---- the sequencer at work, for looking at: build/ui_shots/seq/ (no checks: the user looks) */
static void seq_play_to(uint32_t ti, uint32_t step)  /* play until track ti's playhead is on step */
{
    uint32_t k;
    for (k = 0; k < 4000u && !(song.playing && trk[ti].seq_idx == step); k++)
        ui_frame();
}

static void seq_open(const char *title)              /* SEQ until the page shows (PATTERN: SEQ's only page since
                                                      * FREITAKT's redesign, the white keys its steps) */
{
    uint32_t k;
    for (k = 0; k < NPAGES && (!str_eq(cur_page()->title, title) || ui.home); k++) {
        press(B_SEQ);
        ui_frame();
        release_all();
        ui_frame();
    }
}

static void seq_select(uint32_t ti)                  /* the track encoder, one detent at a time */
{
    uint32_t k;
    for (k = 0; k < NTRK && song.sel != ti; k++) {
        turn(EN_ALGO, ti > song.sel ? 1 : -1);
        ui_frame();
    }
}

static void seq_snap_at(uint32_t ti, uint32_t step, const char *name)
{
    char path[64];
    seq_play_to(ti, step);
    snprintf(path, sizeof path, "seq/%s", name);
    snap_page(path);
}

static void test_seq_screens(void)
{
    static const uint32_t S[4] = {0, 4, 8, 12};
    char name[48];
    uint32_t i, k;
    ui_host_init();
    kit_session();
    song.rec = 0;
    trk[5].p[P_SLEN] = 32;                           /* TOM: two banks, a second-bar fill */
    for (k = 16; k < 32; k++)
        trk[5].step[k].on = k == 20 || k == 26 || k >= 29;
    trk[5].step[31].acc = 1;
    trk[4].p[P_SLEN] = 12;                           /* HATO: a 12-step loop, swung */
    trk[4].p[P_SSWING] = 60;
    seq_open("PATTERN");
    snap_page("seq/01_step_t1_stopped");
    transport_req = 1;
    for (i = 0; i < 4; i++) {                        /* the playhead walks the kick's bar */
        snprintf(name, sizeof name, "%02u_step_t1_play_step%02u", (unsigned)(2 + i), (unsigned)S[i] + 1u);
        seq_snap_at(0, S[i], name);
    }
    seq_select(3);                                   /* HATC: 8ths with gaps */
    seq_snap_at(3, 6, "06_step_t4_hats_step07");
    seq_select(5);                                   /* TOM, 32 steps: bank 2 while its playhead is there */
    press(B_OCTUP);
    ui_frame();
    release_all();
    seq_snap_at(5, 26, "07_step_t6_bank2_step27");
    press(B_OCTDN);
    ui_frame();
    release_all();
    seq_open("PATTERN");
    seq_snap_at(5, 29, "08_pattern_t6_len32_step30");
    seq_select(4);                                   /* HATO: LEN 12, swing 60 */
    seq_snap_at(4, 9, "09_pattern_t5_len12_swing_step10");
    seq_select(0);
    seq_snap_at(0, 4, "10_pattern_t1_step05");
    open_tracks();                                   /* HOME's TRACKS mode: the mixer */
    seq_snap_at(0, 2, "11_tracks_step03");
    seq_snap_at(0, 13, "12_tracks_step14");
    press(B_HOME);
    ui_frame();
    release_all();
    ui_frame();
    seq_snap_at(0, 8, "13_home_play_step09");
    for (k = 0; k < 16; k++)                         /* live recording: the snare's steps cleared, track 2 armed */
        trk[1].step[k].on = trk[1].step[k].acc = 0;
    seq_select(1);
    seq_open("PATTERN");
    seq_snap_at(1, 0, "14_rec_t2_before");
    seq_open("PATTERN");                             /* off the grid: the keys play (and record) the drums */
    song.rec = 1u << 1;
    for (i = 0; i < 4; i++) {                        /* a player hits the snare key on steps 3, 7, 11, 15 */
        seq_play_to(1, 2 + 4 * i);
        keys(1u << KEY_TRK_KEY[1]);
        ui_frame();
        keys(0);
        ui_frame();
    }
    seq_snap_at(1, 0, "15_rec_t2_pattern_after");
    seq_open("PATTERN");
    seq_snap_at(1, 1, "16_rec_t2_step_after");
    transport_req = 2;
    for (k = 0; k < 8; k++)
        ui_frame();
    snap_page("seq/17_step_t2_stopped_after");
    printf("     sequencer: screens in build/ui_shots/seq\n");
}

static void test_project_roundtrip(void)
{
    ui_host_init();
    old_set_model(&trk[2], DM_CONGA);
    trk[2].p[P_E0] = -5;
    trk[2].p[P_DUCK] = 1;
    trk[2].step[7].on = trk[2].step[7].acc = 1;
    trk[2].p[P_SLEN] = 23;
    trk[2].step[7].cond = 33;                        /* 3/5 */
    trk[2].step[7].rat = 2;                          /* 3 hits */
    song.g[G_DRCH] = 5;
    trk[3].p[P_DUCK] = 1;
    song.g[G_CSRC] = 1;
    song.g[G_CRAT] = 99;
    song.g[G_CMKUP] = 50;
    song.g[G_CKNEE] = 0;
    trk[4].p[P_LFO2 + LF_WAVE] = LW_RWALK;
    trk[4].p[P_LFO2 + LF_DEST] = 9;
    trk[4].p[P_LFO2 + LF_DEPTH] = -20;
    trk[4].p[P_LFO2 + LF_MODE] = LM_TIME;
    song.g[G_BPM] = 133;
    song.sel = 2;
    project_save(1);
    ui_host_init();
    check("project: a saved slot reads as used", project_used(1) && !project_used(0));
    project_load(1);
    check("project: load restores models, params, steps, globals, selection",
          trk[2].p[P_MODEL] == DM_CONGA && trk[2].model == DM_CONGA && trk[2].p[P_E0] == -5 &&
              trk[2].p[P_DUCK] == 1 && trk[2].step[7].on && trk[2].step[7].acc && trk[2].p[P_SLEN] == 23 &&
              song.g[G_BPM] == 133 && song.sel == 2);
    check("project: load restores PROB / RATCH and the drum channel",
          trk[2].step[7].cond == 33 && trk[2].step[7].rat == 2 && song.g[G_DRCH] == 5);
    check("project: load restores DUCK and the COMP settings",
          trk[3].p[P_DUCK] == 1 && song.g[G_CSRC] == 1 && song.g[G_CRAT] == 99 && song.g[G_CMKUP] == 50 &&
              song.g[G_CKNEE] == 0);
    check("project: load restores the LFOs",
          trk[4].p[P_LFO2 + LF_WAVE] == LW_RWALK && trk[4].p[P_LFO2 + LF_DEST] == 9 &&
              trk[4].p[P_LFO2 + LF_DEPTH] == -20 && trk[4].p[P_LFO2 + LF_MODE] == LM_TIME);
}

/* LFO DEST's SRC (owner 2026-11-22): no LFO 2 (it has no page): the choices LFO1 and OFF, OFF the value LFO 2 had, so
 * a stored LFO 2 loads as OFF and routes nothing; LFO 2 never computed */
static void test_lfo_src_no_lfo2(void)
{
    uint32_t ok = 1, k;
    char v[16];
    const char *u;
    ui_host_init();
    OK0(TP[P_LSRC1].max == 1 && TP[P_LS4SRC].max == 1 && str_eq(TP[P_LSRC2].names[0], "LFO1") &&
        str_eq(TP[P_LSRC2].names[1], "OFF"));
    while (cur_fam() != FAM_LFO || !str_eq(cur_page()->title, "DEST"))
        tap(B_LFO);
    song.lslot = 0;
    TSEL->p[P_LSRC1] = 0;
    turn(EN_K1 + 1, 5);                              /* KNOB 2: SRC, past its end */
    ui_frame();
    param_format(&TP[P_LSRC1], TSEL->p[P_LSRC1], v, &u);
    OK0(TSEL->p[P_LSRC1] == 1 && str_eq(v, "OFF"));
    check("LFO DEST: SRC is LFO1 or OFF (no LFO2: there is no LFO 2 page)", ok);
    ok = 1;
    trk[2].p[P_LSRC2] = 1;                           /* slot 2 / 4 from LFO 2, as a project stored it */
    trk[2].p[P_LFO2 + LF_DEST] = 9;
    trk[2].p[P_LFO2 + LF_DEPTH] = 64;
    trk[2].p[P_LS4SRC] = 1;
    trk[2].p[P_LS4DEST] = 10;
    trk[2].p[P_LS4DEPTH] = -64;
    trk[2].p[P_LFO2 + LF_WAVE] = LW_SQUARE;
    trk[2].p[P_LFO2 + LF_MODE] = LM_HZ;
    trk[2].p[P_LFO2 + LF_RATE] = 120;
    trk[2].p[P_LEVEL] = 100;
    trk[2].p[P_PAN] = 0;
    project_save(0);
    trk[2].p[P_LSRC2] = 0;
    trk[2].p[P_LFO2 + LF_DEST] = 0;
    (void)proj_load(0, 1);
    OK0(trk[2].p[P_LSRC2] == 1 && trk[2].p[P_LS4SRC] == 1 && trk[2].p[P_LFO2 + LF_DEST] == 9);
    param_format(&TP[P_LSRC2], trk[2].p[P_LSRC2], v, &u);
    OK0(str_eq(v, "OFF"));
    for (k = 0; k < 200u; k++) {
        lfo_apply(CTL);
        OK0(trk[2].p[P_LEVEL] == 100 && trk[2].p[P_PAN] == 0 && trk[2].lnum == 0u);
        lfo_restore();
        if (!ok)
            break;
    }
    OK0(trk[2].lfo[1].ph == 0u && trk[2].lfo[0].ph == 0u);
    check("LFO DEST: a project's slots stored from LFO 2 load as SRC OFF: nothing routed (LVL, PAN untouched), no LFO "
          "computed", ok);
    memset(proj_slot, 0, sizeof proj_slot);
}

static void test_project_rejects(void)
{
    ui_host_init();
    project_save(0);
    proj_slot[0].t[3].p[P_MODEL] = 99;               /* corrupt the stored data, fix the checksum */
    proj_slot[0].t[3].p[P_E1] = 30000;
    ((uint8_t *)&proj_slot[0].t[3].step[0])[0] = 0xC1u;   /* on + both reserved bits */
    proj_slot[0].t[3].step[1].cond = 200;
    ((uint8_t *)&proj_slot[0].t[3].step[1])[0] = 0x30u;   /* rat 3 */
    proj_slot[0].t[3].p[PR_P(P_SLEN)] = 400;
    proj_slot[0].g[PR_G(G_DRCH)] = -5;
    proj_slot[0].g[PR_G(G_CSRC)] = 77;
    proj_slot[0].t[3].p[PR_P(P_DUCK)] = 5;
    proj_slot[0].t[3].p[PR_P(P_LFO1 + LF_DEST)] = 99;
    proj_slot[0].t[3].p[PR_P(P_LFO1 + LF_WAVE)] = -4;
    proj_slot[0].t[3].p[PR_P(P_FTYPE)] = 99;
    proj_slot[0].t[3].p[PR_P(P_FCUT)] = 500;
    proj_slot[0].t[3].p[PR_P(P_FDEC)] = -9;
    proj_slot[0].sum = proj_sum(&proj_slot[0]);
    (void)proj_load(0, 1);                           /* (the edited slot: as DISCARD EDITS? YES) */
    check("project: out-of-range values are clamped on load",
          trk[3].p[P_MODEL] < NMODELS && trk[3].p[P_E1] <= DMODELS[trk[3].p[P_MODEL]].edit[1].max &&
              trk[3].step[0].on == 1 && trk[3].step[1].cond == COND_MAX && trk[3].step[1].rat == 3 &&
              trk[3].p[P_SLEN] == NSTEP && song.g[G_DRCH] == 1 &&
              song.g[G_CSRC] == NTRK && trk[3].p[P_DUCK] == 1 &&
              trk[3].p[P_LFO1 + LF_DEST] == TP[P_LFO1 + LF_DEST].max && trk[3].p[P_LFO1 + LF_WAVE] == 0 &&
              trk[3].p[P_FTYPE] == FT_N - 1 && trk[3].p[P_FCUT] == 127 && trk[3].p[P_FDEC] == 0);
    proj_slot[1].magic = 0x46554E33u;                /* an old Felucca project ("FUN3") */
    check("project: Felucca projects are not used", !project_used(1));
    project_save(2);
    proj_slot[2].sum ^= 1;
    check("project: a bad checksum is not used", !project_used(2));
}

/* SEQ held at power-on: a silent start with USB intact (spec §4) */
static void safe_boot(int seq_held)
{
    ui_host_init();
    fm1_in.buttons = seq_held ? 1u << panel.btn[B_SEQ] : 0u;
    host_scans = 0;
    drum_boot_init();
    fm1_in.buttons = 0;
}

static void test_safe_start(void)
{
    uint32_t i, peak = 0, a;
    safe_boot(1);
    check("safe start: SEQ held through the boot's polled scan (FM1_DEB_RELEASE + 4 scans)",
          safe_start && host_scans == FM1_DEB_RELEASE + 4u);
    a = dvage;
    keys(1u << KEY_TRK_KEY[0]);                      /* a white key */
    ui_frame();
    keys(0);
    midi_in_q[mi_w % MQ] = 0x09u | (0x90u | 9u) << 8 | (uint32_t)trk[1].p[P_NOTE] << 16 | 100u << 24;
    mi_w++;                                          /* a USB-MIDI note on the drum channel (as drum_test's midi_in) */
    drum_hit(&trk[2], 127);                          /* the editor's audition path */
    for (i = 0; i < 40; i++) {
        ui_frame();
        peak |= (uint32_t)abs(mixo[0]) | (uint32_t)abs(mixo[1]);
    }
    check("safe start: keys, MIDI and direct hits start no voice; the mix is silent", dvage == a && peak == 0);
    snap_page("safe_start");
    check("safe start: the SAFE START screen draws", fb_lit(90, 160) > 200);
    for (i = 0; i < SMP_USER_SLOTS; i++)
        peak |= usr_nz[i];
    check("safe start: user sample slots unusable", peak == 0);
    safe_boot(0);
    check("normal start: SEQ not held: no safe mode, the same scans", !safe_start && host_scans == FM1_DEB_RELEASE + 4u);
    {
        panel_t keep = panel;                        /* a recalibrated panel: SEQ on another matrix id */
        uint8_t t = panel.btn[B_SEQ];
        panel.btn[B_SEQ] = panel.btn[B_PLAY];
        panel.btn[B_PLAY] = t;
        safe_boot(1);
        check("safe start: SEQ found through a learned panel map", safe_start);
        panel = keep;
    }
    safe_boot(1);                                    /* an update cancelled in safe start: main.c clears the */
    ui_frame();                                      /* screen and sets ui.force; the safe screen must come back */
    memset(fb, 0, sizeof fb);
    ui.force = 1;
    ui_frame();
    check("safe start: the screen is redrawn after an update session (ui.force)", fb_lit(90, 160) > 200 && !ui.force);
    safe_boot(0);                                    /* leave the other tests a normal start */
}

/* H4: the heaviest page's frame (TRACKS while playing, 8 rows) is far inside the 8 s watchdog */
static void test_ui_frame_cost(void)
{
    uint64_t i0, worst = 0, c;
    uint32_t i;
    ui_host_init();
    kit_session();
    transport_req = 1;
    open_tracks();                                   /* HOME's TRACKS mode: the mixer */
    for (i = 0; i < 60; i++) {
        i0 = instr_now();
        ui_input();
        ui_leds();
        ui_draw();
        c = instr_now() - i0;
        worst = c > worst ? c : worst;
        render_mix(0, 0, CTL);
    }
    printf("     ui: heaviest frame %llu host instructions (TRACKS, playing)\n", (unsigned long long)worst);
    check("ui frame cost: under 4,000,000 host instructions (8 s watchdog, 100x margin)", !i0 || worst < 4000000u);
}

/* a white key on a per-track page (SOUND, TRACK, MIDI, FX, INSERT) selects its track and still plays it;
 * HOME, the STEP grid, PATTERN and the global pages keep the selection */
static int key_selects(const char *title, uint32_t fam_btn, uint32_t presses)
{
    uint32_t a, i;
    ui_host_init();
    for (i = 0; i < presses; i++) {
        press(fam_btn);
        ui_frame();
        release_all();
        ui_frame();
    }
    if (!str_eq(cur_page()->title, title)) {
        printf("     (could not open %s, on %s)\n", title, cur_page()->title);
        return -1;
    }
    a = hit_age(&trk[2]);
    keys(1u << KEY_TRK_KEY[2]);                      /* the third white key: track 3 */
    ui_frame();
    keys(0);
    ui_frame();
    return song.sel == 2 && hit_age(&trk[2]) != a ? 1 : song.sel == 0 ? 0 : -1;
}

static void test_key_selects_track(void)
{
    static const struct { const char *title; uint32_t btn, presses; int want; } C[] = {
        {"SOUND", B_EDIT, 1, 1},
        {"FILTER", B_FX, 1, 1}, {"FX", B_FX, 3, 1}, {"INSERT", B_FX, 4, 1}, {"INSERT", B_FX, 5, 1},
        {"COMP", B_FX, 6, 0},
        {"MIDI OUT", B_GLO, 1, 0}, {"DLY", B_GLO, 2, 0}, {"CHORUS", B_GLO, 5, 0},
        {"PATTERN", B_SEQ, 1, 0},
    };
    uint32_t i, ok = 1;
    for (i = 0; i < sizeof C / sizeof C[0]; i++) {
        int got = key_selects(C[i].title, C[i].btn, C[i].presses);
        if (got != C[i].want) {
            printf("     %s: key 3 %s\n", C[i].title, got == 1 ? "selected T3" : got == 0 ? "kept T1" : "unexpected");
            ok = 0;
        }
    }
    check("a white key on a per-track page selects its track (and plays it); global / sequencer pages keep it", ok);
    ui_host_init();                                  /* HOME: keys select (and play), as SOUND / FX */
    keys(1u << KEY_TRK_KEY[2]);
    ui_frame();
    keys(0);
    check("HOME: a white key selects its track", song.sel == 2);
}

/* MENU > SPEAKER EQ (sound pack): KNOB 1 steps FLAT LOWCUT BASS+ and stops at the ends, OCT+ steps and wraps;
 * the master follows (fx_lowcut); a stored value past BASS+ loads as FLAT */
static void test_speaker_eq(void)
{
    static const int8_t TURN[6] = {1, 1, 1, -1, -1, -1};
    static const uint8_t WANT[6] = {1, 2, 2, 1, 0, 0};
    uint32_t ok = 1, i;
    ui_host_init();
    settings.lowcut = 0;
    fx_lowcut = 0;
    ui.menu = 1;
    ui.menu_sel = MI_SPKEQ;
    ui.force = 1;
    for (i = 0; i < 6u; i++) {
        turn(EN_K1, TURN[i]);
        ui_frame();
        ok &= settings.lowcut == WANT[i] && fx_lowcut == WANT[i];
    }
    check("SPEAKER EQ: KNOB 1 steps FLAT LOWCUT BASS+, stops at the ends; the master follows", ok);
    ok = 1;
    for (i = 0; i < 4u; i++) {
        press(B_OCTUP);
        ui_frame();
        release_all();
        ui_frame();
        ok &= settings.lowcut == (i + 1u) % 3u && fx_lowcut == (i + 1u) % 3u;
    }
    check("SPEAKER EQ: OCT+ steps and wraps", ok);
    settings.lowcut = 2;
    fx_lowcut = 2;
    ui.force = 1;
    snap_page("menu_speaker_eq");
    settings.lowcut = 3;
    settings_init();
    check("SPEAKER EQ: a stored value past BASS+ loads as FLAT", settings.lowcut == 0 && fx_lowcut == 0);
    ui.menu = 0;
    ui.force = 1;
}


static void glo_open(const char *title)             /* GLO until the page shows */
{
    uint32_t k;
    for (k = 0; k < NPAGES && (!str_eq(cur_page()->title, title) || ui.home); k++) {
        press(B_GLO);
        ui_frame();
        release_all();
        ui_frame();
    }
}
/* the delay's, the reverb's and the chorus' pages are GLO's, after its own (GLOBAL, SYSTEM, GLOBAL 3/8, MIDI OUT); FX
 * keeps the per-track pages */
static void test_bus_pages_on_glo(void)
{
    uint32_t k, seen = 0;
    static const char *const WANT[] = {"MIDI OUT", "DLY", "DLY", "REVERB", "CHORUS", "MIDI OUT"};
    ui_host_init();
    glo_open("MIDI OUT");
    for (k = 0; k < 6u; k++) {
        seen += str_eq(cur_page()->title, WANT[k]) && cur_page()->fam == FAM_GLO;
        press(B_GLO);
        ui_frame();
        release_all();
        ui_frame();
    }
    check("GLO: MIDI OUT 1/5, DLY 2/5, DLY 3/5, REVERB 4/5, CHORUS 5/5, then round to MIDI OUT (FREITAKT: no GLOBAL / "
          "SYSTEM pages)", seen == 6u);
    for (k = 0, seen = 0; k < NPAGES; k++)
        seen += PAGES[k].fam == FAM_FX && (str_eq(PAGES[k].title, "DLY") || str_eq(PAGES[k].title, "REVERB") ||
                                           str_eq(PAGES[k].title, "CHORUS"));
    check("FX: no DLY, REVERB or CHORUS page any more", seen == 0u);
    for (k = 0, seen = 0; k < NPAGES; k++)
        seen += PAGES[k].fam == FAM_GLO;
    check("GLO: 5 pages", seen == 5u);
}
static void test_reverb_pages(void)
{
    ui_host_init();
    glo_open("REVERB");
    check("GLO reaches REVERB: TYPE SIZE DAMP", str_eq(cur_page()->title, "REVERB") && cur_page()->id[0] == G_RTYPE &&
          cur_page()->id[1] == G_RSIZE && cur_page()->id[2] == G_RDAMP && cur_page()->id[3] == 0xFF);
    turn(EN_K1, 1);
    ui_frame();
    check("REVERB: KNOB 1 turns TYPE to SPRING", song.g[G_RTYPE] == 1);
    check("REVERB: the TYPE names fit the 5-character value (ROOM, SPRNG, HALL)",
          strlen(N_RTYPE[0]) <= 5u && strlen(N_RTYPE[1]) <= 5u && str_eq(N_RTYPE[1], "SPRNG") &&
          strlen(N_RTYPE[2]) <= 5u && str_eq(N_RTYPE[2], "HALL"));
    snap_page("glo_reverb_spring");
    turn(EN_K1, 1);
    ui_frame();
    check("REVERB: KNOB 1 on to HALL (TYPE's third value)", song.g[G_RTYPE] == 2);
    turn(EN_K1, 1);
    ui_frame();
    check("REVERB: TYPE stops at HALL", song.g[G_RTYPE] == 2);
    snap_page("glo_reverb_hall");
    turn(EN_K1, -2);
    ui_frame();
    glo_open("CHORUS");
    check("GLO reaches CHORUS: RATE DEPTH", str_eq(cur_page()->title, "CHORUS") && cur_page()->fam == FAM_GLO &&
          cur_page()->id[0] == G_CRATE && cur_page()->id[1] == G_CDEPTH && cur_page()->id[2] == 0xFF &&
          cur_page()->id[3] == 0xFF);
    snap_page("glo_chorus");
}

/* the delay's MODE: DLY's second page (after TIME FDBK COLR MIX), KNOB 1 NORM / PING; screens for the user (TIME
 * at a dotted value, MODE PING-PONG) */
static void test_delay_mode_page(void)
{
    ui_host_init();
    glo_open("DLY");
    check("GLO reaches DLY: TIME FDBK COLR MIX", str_eq(cur_page()->title, "DLY") && cur_page()->id[0] == G_DTIME);
    song.g[G_DTIME] = 12;                            /* 1/8D */
    snap_page("glo_delay_time_dotted");
    press(B_GLO);
    ui_frame();
    release_all();
    ui_frame();
    check("GLO: DLY's second page has MODE (KNOB 1), nothing else", str_eq(cur_page()->title, "DLY") &&
          cur_page()->id[0] == G_DMODE && cur_page()->id[1] == 0xFF && cur_page()->id[2] == 0xFF &&
          cur_page()->id[3] == 0xFF);
    check("DLY MODE: NORM at power-on", song.g[G_DMODE] == 0);
    turn(EN_K1, 1);
    ui_frame();
    check("DLY MODE: KNOB 1 turns it to PING, and stops there", song.g[G_DMODE] == 1);
    turn(EN_K1, 1);
    ui_frame();
    check("DLY MODE: (the end)", song.g[G_DMODE] == 1);
    check("DLY MODE: the names fit the value box whole (4 characters: NORM, PING)",
          strlen(N_DMODE[0]) <= 4u && strlen(N_DMODE[1]) <= 4u && str_eq(N_DMODE[1], "PING"));
    snap_page("glo_delay_mode_pingpong");
    glo_open("REVERB");
    check("GLO: REVERB after the two DLY pages", str_eq(cur_page()->title, "REVERB"));
}

/* sound pack: division knobs run by length (4BAR .. 1/32), then FIT (F4BR F2BR FBAR F1/2: LEN steps over 16 .. 2
 * beats); the stored value stays the id; the gauge follows */
static void test_div_order(void)
{
    static const int16_t WANT[DIV_MAX + 1] = {9, 8, 7, 6, 0, 1, 4, 2, 5, 3, 13, 12, 11, 10};
    const param_desc_t *d = &TP[P_SDIV];
    uint32_t i, ok = 1;
    for (i = 0; i < DIV_MAX; i++)
        ok &= param_turn(d, WANT[i], 1) == WANT[i + 1] && param_turn(d, WANT[i + 1], -1) == WANT[i];
    ok &= param_turn(d, 10, 1) == 10 && param_turn(d, 9, -1) == 9 && param_turn(d, 9, 30) == 10;
    check("DIV: the knob steps 4BAR 2BAR 1/1 1/2 1/4 1/8 8T 1/16 16T 1/32 F4BR F2BR FBAR F1/2, stopping at the ends", ok);
    check("DLY TIME: the same order, its dotted values between (1/2 1/4D 1/4 1/8D 4T 1/8 ..)",
          param_turn(&GP[G_DTIME], 6, 1) == 11 && param_turn(&GP[G_DTIME], 11, 1) == 0 &&
          param_turn(&GP[G_DTIME], 0, -1) == 11 && param_turn(&GP[G_DTIME], 0, 1) == 12);
    check("DIV gauge: in the knob's order (4BAR empty, F1/2 full, 1/4 at 4/13, 1/32 at 9/13)",
          RATIO(d, 9) == 0 && RATIO(d, 10) == 1000 && RATIO(d, 0) == 4000 / 13 && RATIO(d, 3) == 9000 / 13);
    check("other knobs keep their order (INSERT TYPE, ids as shown)", param_turn(&TP[P_ITYPE], 1, 1) == 2);
    ui_host_init();
    settings.accel = 0;
    seq_open("PATTERN");
    trk[song.sel].p[P_SDIV] = 9;
    ok = 1;
    for (i = 1; i < 10u; i++) {
        turn(EN_K2, 1);
        ui_frame();
        ok &= trk[song.sel].p[P_SDIV] == WANT[i];
    }
    check("PATTERN DIV: turning right walks the length order", ok);
    trk[song.sel].p[P_SDIV] = 8;
    snap_page("seq/pattern_div_2bar");
    settings.accel = 1;
}

/* MENU > USB LEVEL (USB audio): KNOB 1 right FIXED / left MASTER, OCT+ toggles; the mix follows (fx_usb_fixed); the
 * menu's eight rows for the user's eye */
static void test_usb_level(void)
{
    uint32_t ok = 1;
    ui_host_init();
    settings.usbfix = 0;
    fx_usb_fixed = 0;
    ui.menu = 1;
    ui.menu_sel = MI_USB;
    ui.force = 1;
    turn(EN_K1, 1);
    ui_frame();
    ok &= settings.usbfix == 1 && fx_usb_fixed == 1;
    turn(EN_K1, -1);
    ui_frame();
    ok &= settings.usbfix == 0 && fx_usb_fixed == 0;
    press(B_OCTUP);
    ui_frame();
    release_all();
    ui_frame();
    ok &= settings.usbfix == 1 && fx_usb_fixed == 1;
    check("USB LEVEL: KNOB 1 right FIXED / left MASTER, OCT+ toggles; the mix follows", ok);
    ui.force = 1;
    snap_page("menu_usb_level");
    settings.usbfix = 0;
    fx_usb_fixed = 0;
    ui.menu = 0;
    ui.force = 1;
}

/* MIDI clock: CLK USB (MENU's CLOCK row), and HOME's BPM knob while following (a message, the tempo kept) */
static void test_clock_screens(void)
{
    int16_t bpm;
    ui_host_init();
    home_screen();
    song.g[G_CLOCK] = 1;
    ui.force = 1;
    ui_frame();
    snap_page("home_clk_usb");
    bpm = song.g[G_BPM];
    turn(EN_K1, 3);
    ui_frame();
    check("CLK USB: HOME's BPM knob keeps the tempo (the clock's) and says so", song.g[G_BPM] == bpm && ui.msg_t &&
          str_eq(ui.msg, "CLK USB"));
    open_tracks();
    ui.msg_t = 0;
    ui.msg[0] = 0;
    sel(3);
    check("CLK USB: SELECT on HOME's TRACKS mode keeps the tempo and says CLK USB", song.g[G_BPM] == bpm && mix_mode() &&
          str_eq(ui.msg, "CLK USB"));
    song.g[G_CLOCK] = 0;
    render_mix(0, 0, CTL);
}

/* PERFORM: FX tapped opens its pages on the release; held, the layer (keys, knobs, LEDs) */
static uint32_t fx_bit(void) { return 1u << panel.btn[B_FX]; }
static void fx_hold_frames(uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        fm1_in.buttons |= fx_bit();
        ui_frame();
    }
}
static void test_perform_gesture(void)
{
    uint32_t fam;
    ui_host_init();
    press(B_FX);
    ui_frame();
    check("FX pressed: nothing yet (the page waits for the release)", ui.home);
    release_all();
    ui_frame();
    check("FX tapped: the FX pages on the release", !ui.home && cur_fam() == FAM_FX && !ui.layer);
    fam = cur_page() - PAGES;
    press(B_FX);
    fx_hold_frames(500);                              /* well past 0.4 s (1 ms a frame) */
    check("FX held alone: the layer's map", ui.layer == 1u);
    release_all();
    ui_frame();
    check("FX let go after the hold: no page change, the map gone", (uint32_t)(cur_page() - PAGES) == fam && !ui.layer);
    press(B_FX);
    ui_frame();
    turn(EN_K2, 5);
    fx_hold_frames(1);
    check("FX + KNOB 2: CRUSH, the layer open at once", perf_k[1] > 0 && ui.layer == 1u);
    release_all();
    ui_frame();
    check("FX let go (HOLD): CRUSH stays on, no page change", perf_k[1] > 0 && (uint32_t)(cur_page() - PAGES) == fam);
}

static void test_perform_keys(void)
{
    uint32_t sel;
    ui_host_init();
    sel = song.sel;
    press(B_FX);
    ui_frame();
    keys(1u << 4);                                    /* A3: track 3's key */
    fx_hold_frames(2);                                /* (the render takes the key after the UI's pass) */
    check("FX + a track key: no track select, the layer open, track 3 repeating",
          song.sel == sel && ui.layer == 1u && ((perf_held >> (PF_T1 + 2u)) & 1u));
    check("LEDs: the repeating track's key lit; the rate keys: F5 (1/16) lit, D5 E5 G5 dark", led_on(14u + 4u) &&
          led_on(14u + 24u) && !led_on(14u + 21u) && !led_on(14u + 23u) && !led_on(14u + 26u));
    keys(1u << 4 | 1u << 23);                         /* E5: 1/8, the track key still held */
    fx_hold_frames(2);
    check("FX + E5: the rate 1/8, its key lit, F5's dark; the track still repeating", perf_rate == PR_8 &&
          led_on(14u + 23u) && !led_on(14u + 24u) && ((perf_held >> (PF_T1 + 2u)) & 1u));
    keys(1u << 4);
    fx_hold_frames(2);
    check("E5 let go: the rate stays 1/8", perf_rate == PR_8 && led_on(14u + 23u));
    keys(0);
    fx_hold_frames(1);
    release_all();
    ui_frame();
    check("all let go: nothing held, the map gone", !perf_held && !ui.layer);
    seq_open("PATTERN");                              /* PATTERN: FX + a white key toggles no step */
    {
        step_t s0 = TSEL->step[0];
        press(B_FX);
        ui_frame();
        keys(1u << 0);
        fx_hold_frames(1);
        keys(0);
        fx_hold_frames(1);
        release_all();
        ui_frame();
        check("PATTERN: FX + a white key toggles no step", TSEL->step[0].on == s0.on);
    }
}

/* Review Focus 3 */
static void test_perform_menu_kills(void)
{
    uint32_t i;
    ui_host_init();
    press(B_FX);
    ui_frame();
    keys(1u << 3);                                    /* G#3: REPEAT 1/16 */
    fx_hold_frames(1);
    check("REPEAT held in the layer", (perf_held >> PF_R16) & 1u);
    release_all();
    for (i = 0; i < 1000u && !ui.menu; i++) {         /* HOME held: the menu */
        fm1_in.buttons |= 1u << panel.btn[B_HOME];
        ui_frame();
    }
    release_all();
    ui_frame();                                       /* (the next pass sees the menu: 1 ms) */
    check("the menu opened with an effect key held: effects off, the key still the layer's",
          ui.menu && perf_kill && !perf_act && (kb_layer >> 3) & 1u);
    keys(0);
    ui_frame();
    check("the key let go: nothing held", !perf_held && !kb_layer);
    release_all();
    ui_frame();
    menu_close();
    ui_frame();
    ui_frame();
    check("menu closed, no key held: effects allowed again", !perf_kill);
}

/* MENU > PERFORM: HOLD / PAGE; PAGE: the PERFORM screen stays after FX is let go, until the screen changes */
static void test_perform_page(void)
{
    ui_host_init();
    ui.menu = 1;
    ui.menu_sel = MI_PERF;
    ui.force = 1;
    turn(EN_K1, 1);
    ui_frame();
    check("MENU PERFORM: KNOB 1 right = PAGE", settings.perfpage == 1u);
    snap_page("perform/menu_page");
    turn(EN_K1, -1);
    ui_frame();
    check("MENU PERFORM: KNOB 1 left = HOLD", settings.perfpage == 0u);
    menu_close();
    ui_frame();
    settings.perfpage = 1;
    tap(B_SEQ);                                       /* a page under it */
    press(B_FX);
    fx_hold_frames(500);
    release_all();
    ui_frame();
    check("PAGE: FX held then let go, the PERFORM screen stays", ui.layer && ui.pg_open);
    keys(1u << 3);                                    /* G#3 without FX */
    ui_frame();
    check("PAGE: a black key plays its effect with no button held", ((perf_held >> PF_R16) & 1u) && (kb_layer >> 3) & 1u);
    keys(0);
    ui_frame();
    turn(EN_K2, 10);
    ui_frame();
    check("PAGE: the knobs are the macros without FX, and keep their values", perf_k[1] > 0);
    press(B_PLAY);
    ui_frame();
    release_all();
    ui_frame();
    press(B_OCTUP);
    ui_frame();
    release_all();
    ui_frame();
    check("PAGE: PLAY and OCT+ leave it open", ui.pg_open && perf_k[1] > 0);
    tap(B_GLO);
    check("PAGE: another page button closes it, the macros stay on", !ui.pg_open && !ui.layer && perf_k[1] > 0 &&
          cur_fam() == FAM_GLO);
    press(B_FX);
    fx_hold_frames(500);
    release_all();
    ui_frame();
    tap(B_FX);
    ui_frame();
    check("PAGE: an FX tap closes it and opens the FX pages", !ui.pg_open && cur_fam() == FAM_FX);
    press(B_FX);
    fx_hold_frames(500);
    release_all();
    ui_frame();
    tap(B_HOME);
    ui_frame();
    check("PAGE: HOME closes it", !ui.pg_open && !ui.layer);
    settings.perfpage = 0;
    press(B_FX);
    fx_hold_frames(500);
    release_all();
    ui_frame();
    check("HOLD: the map goes with FX", !ui.layer && !ui.pg_open);
    transport_req = 2;                                /* (PLAY above started the transport: stopped for the next test) */
    render_mix(0, 0, CTL);
}

/* PERFORM screens: the map held alone, with keys held (REPEAT 1/16 running, track 2 repeating), an unavailable REPEAT
 * (40 BPM), OCT UP with the SHIMMER knob */
static void test_perform_screens(void)
{
    ui_host_init();
    press(B_FX);
    fx_hold_frames(500);
    snap_page("perform/map");
    check("the map: FX held shows it (no footer since FREITAKT)", ui.layer);
    keys(1u << 3 | 1u << 2);                          /* G#3 REPEAT 1/16, G3 track 2 */
    fx_hold_frames(2);
    snap_page("perform/keys");
    keys(1u << 26);                                   /* G5: REPEAT STEP 1/32 */
    fx_hold_frames(2);
    keys(1u << 4 | 1u << 7);                          /* A3 C4: tracks 3 and 5 repeating */
    fx_hold_frames(2);
    snap_page("perform/repeat_step");
    check("the map: G5 selects 1/32, tracks 3 and 5 held", perf_rate == PR_32 &&
          perf_held == (PF_BIT(PF_T1 + 2u) | PF_BIT(PF_T1 + 4u)));
    keys(0);
    fx_hold_frames(2);
    song.g[G_BPM] = 40;
    ui.force = 1;
    snap_page("perform/bpm40");
    song.g[G_BPM] = 120;
    keys(1u << 20);                                   /* C#5 OCT UP */
    turn(EN_K4, 40);
    fx_hold_frames(2);
    snap_page("perform/oct_shimmer");
    check("OCT UP playing: KNOB 4 is SHIMMER", perf_harm_on() && perf_k[3] > 0);
    keys(0);
    release_all();
    ui_frame();
}

/* spec §7: the knob effects (FILTER CRUSH THROW DEPTH) keep their values and keep working after the PERFORM screen
 * closes (FX let go, PAGE closed, the menu, a confirm, NAME); they change only on the PERFORM screen; INIT ALL resets
 * them; the top bar shows FX while any is on. The buffer effects (black keys) still stop as before */
static void test_perform_knobs_stay(void)
{
    uint32_t mode;
    int16_t e1;
    ui_host_init();
    tap(B_EDIT);                                     /* (HOME has no top bar: SOUND's, its knobs HOME's four) */
    ui_frame();
    press(B_FX);
    turn(EN_K1, -40);                                 /* FILTER: LPF */
    fx_hold_frames(2);
    release_all();
    ui_frame();
    render_mix(0, 0, CTL);
    check("HOLD: FX let go, FILTER stays on and keeps working", perf_k[0] < 0 && !ui.layer && pf.busy && pf.lc < PF_TOP);
    ui_frame();
    home_screen();                                   /* (HOME: its knobs the macros) */
    ui.force = 1;
    ui_frame();
    e1 = song.g[G_SWING];
    turn(EN_K1 + 1, 3);
    ui_frame();
    check("the screen gone: the knobs edit the page again (HOME: SWING), the macros untouched",
          song.g[G_SWING] != e1 && perf_k[1] == 0 && perf_k[0] < 0);
    song.g[G_SWING] = e1;
    for (mode = 0; mode < 3u; mode++) {               /* the menu, a confirm, NAME: the macros stay; effects stop */
        press(B_FX);
        keys(1u << 3);                                /* G#3: REPEAT 1/16 */
        turn(EN_K2, 10);                              /* CRUSH */
        fx_hold_frames(2);
        release_all();
        if (mode == 0u) {
            ui.menu = 1;
            ui.force = 1;
        } else if (mode == 1u) {
            ui.confirm = 1;
            ui.force = 1;
        } else {
            project_save(0);
            name_open(NK_RENAME, 0);
        }
        fm1_in.buttons = 0;
        ui_frame();
        render_mix(0, 0, CTL);
        check(mode == 0u ? "the menu: CRUSH and FILTER stay on, the held REPEAT stops"
              : mode == 1u ? "a confirm: CRUSH and FILTER stay on, the held REPEAT stops"
                           : "NAME: CRUSH and FILTER stay on, the held REPEAT stops",
              perf_k[1] > 0 && perf_k[0] < 0 && perf_kill && !perf_act && pf.busy);
        keys(0);
        ui_frame();
        if (mode == 0u)
            menu_close();
        else if (mode == 1u)
            ui.confirm = 0;
        else
            name_close();
        ui.force = 1;
        ui_frame();
        perf_k[1] = 0;
    }
    settings.perfpage = 1;                            /* PAGE: closed by another page */
    press(B_FX);
    turn(EN_K3, 20);                                  /* THROW */
    fx_hold_frames(2);
    release_all();
    ui_frame();
    check("PAGE: the screen stays after FX", ui.pg_open && perf_k[2] > 0);
    tap(B_EDIT);
    check("PAGE closed by another page: THROW stays on", !ui.pg_open && !ui.layer && perf_k[2] > 0);
    settings.perfpage = 0;
    press(B_FX);
    turn(EN_K1, 60);                                  /* FILTER past 0 (-40 + 60): it stops at 0, the off point */
    turn(EN_K3, -40);                                 /* THROW back to 0 */
    fx_hold_frames(2);
    check("FILTER turned across 0 stops at 0 (off), not at the far side", perf_k[0] == 0);
    release_all();
    ui_frame();
    ui_frame();
    check("on the PERFORM screen, turned back to 0: FILTER and THROW off", !perf_k[0] && !perf_k[2]);
    press(B_FX);
    turn(EN_K1, 10);                                  /* from 0 a turn goes on as before */
    fx_hold_frames(2);
    check("FILTER from 0: a turn moves it (HPF side)", perf_k[0] > 0);
    turn(EN_K1, -30);
    fx_hold_frames(2);
    check("FILTER back across 0: stops at 0 again", perf_k[0] == 0);
    turn(EN_K4, 20);                                  /* DEPTH alone: nothing sounds without a black key */
    fx_hold_frames(2);
    release_all();
    ui_frame();
    ui_frame();
    check("DEPTH alone: set (it acts only with a held black key)", perf_k[3] > 0);
    perf_k[1] = 30;
    perf_k[3] = 20;
    init_all();
    render_mix(0, 0, CTL);
    check("INIT ALL: the knob effects off", !perf_k[0] && !perf_k[1] && !perf_k[2] && !perf_k[3]);
    ui_frame();
}

/* review: FX pressed and a key at once, before the UI's pass has seen FX: still the layer's (no note) */
static void test_perform_fast_key(void)
{
    uint32_t mo;
    ui_host_init();
    ui_frame();
    usb.config = 1;
    mo = mo_w;
    fm1_in.buttons |= fx_bit();                       /* FX down, the UI has not run since */
    keys(1u << 0);                                    /* F3 in the same moment */
    render_mix(0, 0, CTL);                            /* the audio render first */
    check("FX + a key before the UI's pass: the key is the layer's (track 1's REPEAT STEP), no note (no MIDI)",
          (kb_layer & 1u) && ((perf_held >> PF_T1) & 1u) && mo_w == mo);
    usb.config = 0;
    keys(0);
    release_all();
    ui_frame();
    ui_frame();
}

/* review: FX held into the menu and out again: the press is dead (no map, no layer, no PAGE), the macros off */
static void test_perform_dead_hold(void)
{
    uint32_t p, i;
    for (p = 0; p < 2u; p++) {
        ui_host_init();
        settings.perfpage = p;
        press(B_FX);
        turn(EN_K2, 10);                              /* CRUSH: the layer open at once */
        fx_hold_frames(2);
        for (i = 0; i < 1000u && !ui.menu; i++) {     /* HOME held too: the menu */
            fm1_in.buttons |= 1u << panel.btn[B_HOME];
            fx_hold_frames(1);
        }
        fm1_in.buttons &= ~(1u << panel.btn[B_HOME]);
        fx_hold_frames(2);
        check(p ? "PAGE: FX held into the menu: the CRUSH macro stays on there" : "HOLD: FX held into the menu: the CRUSH macro stays on there",
              ui.menu && perf_k[1] > 0);
        menu_close();
        fx_hold_frames(3);                            /* FX still held, the menu gone */
        check(p ? "PAGE: FX held out of the menu: dead (no PERFORM page, keys not the layer's)"
                : "HOLD: FX held out of the menu: dead (no map, keys not the layer's)",
              !ui.layer && !ui.pg_open && !perf_mask);
        release_all();
        ui_frame();
        settings.perfpage = 0;
    }
}

/* MOTION: knob turns of the armed, selected track record while playing (SOUND, HOME, TRACKS); clears */
static void motion_armed_play(void)
{
    ui_host_init();
    song.rec = 1u;                                    /* track 1 armed */
    transport_req = 1;
    ui_frame();
    ui_frame();
}
static void test_motion_recording(void)
{
    uint32_t i;
    motion_armed_play();
    tap(B_EDIT);                                      /* SOUND 1: MODEL, the model's 1st, 2nd, 3rd knob */
    turn(EN_K3, 4);
    ui_frame();
    check("MOTION: a SOUND knob of the armed track records while playing", motion_count(0) == 1u);
    ui.home = 1;
    ui.force = 1;
    ui_frame();
    turn(EN_K2, 3);
    ui_frame();
    check("MOTION: HOME's knobs (BPM, SWING: global since FREITAKT) record nothing", motion_count(0) == 1u);
    song.g[G_SWING] = 0;
    open_family(FAM_MIX);
    ui_frame();
    turn(EN_K2, 3);                                   /* TRACKS: LEVEL */
    ui_frame();
    check("MOTION: TRACKS' LEVEL records", motion_count(0) >= 2u);
    for (i = 0; mo.b.count < LK_MAX; i++)             /* the store full with tracks 2 and 3's */
        motion_add(1u + i / 512u, i % 64u, P_E0 + (i / 64u) % 8u, 1);
    tap(B_EDIT);
    turn(EN_K4, -4);                                  /* the model's 3rd knob: a new place (E1's is taken) */
    ui_frame();
    check("MOTION FULL: said when a turn finds no free place", str_eq(ui.msg, "MOTION FULL") && !mo.full);
    track_clear(&trk[1]);
    check("CLEAR TRACK (and CLR SEQ / CLR ALL): its motion cleared too", motion_count(1) == 0u && motion_count(0) >= 1u);
    transport_req = 2;
    ui_frame();
}

/* Review Focus 4: a model change while motion plays re-takes the base */
static void test_motion_model_change(void)
{
    int16_t def;
    uint32_t i;
    ui_host_init();
    trk[0].p[P_E1] = 7;                               /* the old sound's DECAY, not a default */
    trk[0].p[P_LEVEL] = 104;                          /* a knob the model change leaves alone */
    motion_add(0, 0, P_E1, 120);                      /* plays at step 0: E1 moved off the base 7 */
    motion_add(0, 0, P_LEVEL, 20);                    /* .. and LEVEL off 104 */
    transport_req = 1;
    for (i = 0; i < 4u; i++)
        ui_frame();
    tap(B_EDIT);
    turn(EN_K1, 1);                                   /* SOUND 1 KNOB 1: the next model, its default sound */
    ui_frame();
    def = trk[0].p[P_E1];
    check("MOTION: a model change while playing is the new base", mo.base[0][P_E1] == def && def != 7);
    transport_req = 2;
    ui_frame();
    check("MOTION: STOP after a model change keeps the new model's values (not the old 7)", trk[0].p[P_E1] == def);
    check("MOTION: STOP after a model change puts the other knobs' patch values back (LEVEL 104, not 20)",
          trk[0].p[P_LEVEL] == 104);
    init_all();
    check("INIT ALL: no motion left", mo.b.count == 0u);
}

/* parameter locks (2026-10-24): CLEAR TRACK and TOOLS CLR* remove a track's locks with its motion; MOTION CLEAR
 * keeps them; INIT ALL leaves an empty bank; other tracks' locks are kept */
static void test_clears_and_locks(void)
{
    uint32_t ok;
    ui_host_init();
    lock_set(2, 4, P_E1, 100);
    lock_set(5, 6, P_FCUT, 20);
    motion_add(2, 1, P_E2, 3);
    motion_add(5, 1, P_E2, 3);
    track_clear(&trk[2]);                             /* CLEAR TRACK 3 (REC + SEL held 5 s) */
    ok = lock_count(2) == 0u && motion_count(2) == 0u && lock_count(5) == 1u && motion_count(5) == 1u;
    check("CLEAR TRACK: the track's locks and motion gone, the other tracks' kept", ok);
    motion_clear(5);                                  /* MOTION's CLEAR */
    check("MOTION CLEAR: the track's motion gone, its locks kept", motion_count(5) == 0u && lock_count(5) == 1u);
    lock_set(1, 0, P_E3, 9);
    seq_clear_all();                                  /* TOOLS CLR* */
    check("TOOLS CLR*: every track's locks gone", mo.b.count == 0u);
    lock_set(1, 0, P_E3, 9);
    init_all();                                       /* TOOLS INIT* */
    check("INIT ALL: an empty bank", mo.b.count == 0u && lock_count(1) == 0u);
}

static void name_type(uint32_t key)                  /* one key tap (key index from F3) */
{
    keys(1u << key);
    ui_frame();
    keys(0);
    ui_frame();
}
static void project_page(void)
{
    uint32_t k;
    for (k = 0; k < 8u && (ui.home || page_id(cur_page(), 0) != G_SLOT); k++)
        tap(B_SAVE);
}
static void save_knob(void)                          /* SAVE: GO, and GO again */
{
    turn(EN_K1 + 3, 1);
    ui_frame();
    turn(EN_K1 + 3, 1);
    ui_frame();
}
static void test_name_save(void)
{
    char b[NAME_LEN + 1u];
    ui_host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    project_page();
    save_knob();
    check("SAVE saves at once (no NAME screen), the project unnamed", !name_on() && project_used(0) &&
          project_name(0, b) && !b[0]);
    turn(EN_K1 + 1, 1);
    ui_frame();
    check("NAME knob on an unnamed slot: NAME opens, prefilled PROJECT A", name_on() && str_eq(nm.s, "PROJECT A"));
    snap_page("name/abc");
    while (nm.len) {                                 /* C# (key 8): DELETE */
        nm.cur = nm.len;
        name_type(8);
    }
    name_type(0);                                    /* F3: AB -> A */
    name_type(0);                                    /* again within 0.8 s: B */
    host_ticks += 900u * 1000u * FM1_TICKS_PER_US;   /* 0.9 s: kept */
    ui_frame();
    name_type(2);                                    /* G3: CD -> C */
    host_ticks += 900u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    name_type(3);                                    /* G#3: SPACE */
    name_type(10);                                   /* D#4: 123 */
    snap_page("name/123");
    name_type(0);                                    /* 1 */
    name_type(2);                                    /* 2 */
    check("NAME: multi-tap, the timeout, SPACE, 123", str_eq(nm.s, "BC 12") && nm.cur == 5u);
    name_type(1);                                    /* F#3: left */
    name_type(8);                                    /* DELETE: the 1 */
    check("NAME: cursor left, DELETE", str_eq(nm.s, "BC 2") && nm.cur == 3u);
    turn(EN_K1, -3);
    ui_frame();
    turn(EN_K1 + 1, 1);
    ui_frame();
    check("NAME: KNOB 1 the cursor, KNOB 2 the character", nm.cur == 0u && nm.s[0] == 'C');
    turn(EN_K1, 10);
    ui_frame();
    name_type(3);                                    /* a trailing space: dropped when written */
    press(B_OCTUP);
    ui_frame();
    release_all();
    ui_frame();
    check("NAME OCT+: the slot renamed (ends trimmed), the current project's name too", !name_on() &&
          project_name(0, b) && str_eq(b, "CC 2") && str_eq(pname.name, "CC 2"));
    project_page();
    ui.force = 1;
    snap_page("name/project");
    turn(EN_K1 + 1, 1);
    ui_frame();
    check("NAME knob: renames the selected slot, prefilled with its name", name_on() && str_eq(nm.s, "CC 2"));
    press(B_OCTDN);
    ui_frame();
    release_all();
    ui_frame();
    check("NAME OCT-: cancelled, nothing written", !name_on() && str_eq(pname.name, "CC 2"));
    turn(EN_K1, 1);                                  /* slot B: empty */
    ui_frame();
    turn(EN_K1 + 1, 1);
    ui_frame();
    check("NAME knob on an empty slot: EMPTY SLOT", !name_on() && str_eq(ui.msg, "EMPTY SLOT"));
}
static void test_name_keys_silent(void)
{
    uint32_t a;
    ui_host_init();
    project_page();
    project_save(0);
    transport_req = 1;
    ui_frame();
    transport_req = 0;
    name_open(NK_RENAME, 0);
    song.rec = 1u;
    a = hit_age(&trk[0]);
    name_type(0);                                    /* F3: track 1's key */
    press(B_FX);
    ui_frame();
    name_type(1);                                    /* F#3 with FX held: not PERFORM */
    release_all();
    ui_frame();
    check("NAME: the keys never sound, record or start PERFORM", hit_age(&trk[0]) == a && !perf_held &&
          !trk[0].step[0].on && !trk[0].step[1].on && str_eq(nm.s, "PROJECT AA"));
}
static void test_name_stop_to_save(void)
{
    char b[NAME_LEN + 1u];
    ui_host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    project_page();
    project_save(0);
    name_open(NK_RENAME, 0);
    transport_req = 1;
    ui_frame();
    press(B_OCTUP);
    ui_frame();
    release_all();
    ui_frame();
    check("NAME OCT+ while playing: STOP TO SAVE, the screen stays", name_on() && project_name(0, b) && !b[0] &&
          str_eq(ui.msg, "STOP TO SAVE"));
    tap(B_PLAY);
    ui_frame();
    press(B_OCTUP);
    ui_frame();
    release_all();
    ui_frame();
    check("... stopped (PLAY works in NAME): renamed", !name_on() && project_name(0, b) && str_eq(b, "PROJECT A"));
    name_open(NK_RENAME, 0);
    name_type(0);
    tap(B_HOME);
    ui_frame();
    check("NAME: HOME cancels", !name_on() && project_name(0, b) && str_eq(b, "PROJECT A"));
}

static void test_name_message_ends(void)
{
    uint32_t i;
    ui_host_init();
    project_page();
    project_save(0);
    name_open(NK_RENAME, 0);
    ui_message("STOP TO SAVE");
    for (i = 0; i < 60u; i++)
        ui_frame();
    check("NAME: a message in its title goes after a while", ui.msg_t == 0u);
    name_close();
}

static void test_phys_screens(void)
{
    uint32_t k;
    ui_host_init();
    old_set_model(TSEL, DM_MEMB);
    for (k = 0; k < NPAGES && (ui.home || cur_page()->fam != FAM_SND); k++)
        tap(B_EDIT);
    ui.force = 1;
    snap_page("phys/memb_1");
    page_turn(1);
    ui.force = 1;
    snap_page("phys/memb_2");
    {
        int16_t *vp;
        const param_desc_t *h = page_desc(cur_page(), 0, &vp), *b = page_desc(cur_page(), 2, &vp);
        check("MEMB: SOUND 2/3 holds HEAD .. BEND", h && b && str_eq(h->label, "HEAD") && str_eq(b->label, "BEND"));
    }
}

/* FILTER: two pages first in FX (TYPE CUT RESO ENV, DECAY), the graph per TYPE */
static void test_filter_screens(void)
{
    static const char *const SHOT[FT_N] = {"filter/off", "filter/lp", "filter/bp", "filter/hp", "filter/notch"};
    uint32_t k, ty;
    int16_t *vp;
    ui_host_init();
    for (k = 0; k < NPAGES && (ui.home || cur_page()->fam != FAM_FX); k++)
        tap(B_FX);
    check("FX: the first page is FILTER 1/2 (TYPE CUT RESO ENV)",
          str_eq(cur_page()->title, "FILTER") && str_eq(page_desc(cur_page(), 0, &vp)->label, "TYPE") &&
          str_eq(page_desc(cur_page(), 3, &vp)->label, "ENV"));
    TSEL->p[P_FCUT] = 70;
    TSEL->p[P_FRESO] = 90;
    TSEL->p[P_FENV] = 30;
    for (ty = 0; ty < FT_N; ty++) {
        TSEL->p[P_FTYPE] = (int16_t)ty;
        ui.force = 1;
        snap_page(SHOT[ty]);
    }
    page_turn(1);
    ui.force = 1;
    snap_page("filter/decay");
    check("FILTER 2/2 holds DECAY", str_eq(cur_page()->title, "FILTER") && str_eq(page_desc(cur_page(), 0, &vp)->label, "DECAY"));
    TSEL->p[P_LFO1 + LF_DEST] = 11;
    for (k = 0; k < NPAGES && (ui.home || cur_page()->fam != FAM_LFO); k++)
        tap(B_LFO);
    page_turn(1);
    ui.force = 1;
    snap_page("filter/lfo_fcut");
    {
        char v[8];
        const char *u;
        param_format(&TP[P_LFO1 + LF_DEST], 11, v, &u);
        check("LFO DEST 11 reads F.CUT", str_eq(v, "F.CUT"));
    }
}

/* INSERT (insert.c, from Skiff / Felucca 1.5): two pages after FX (DIST and the sends), FX's last: TYPE A B
 * C (A B C named by the TYPE), then MIX and SRR; the graph per TYPE */
static void test_insert_screens(void)
{
    static const char *const SHOT[IT_N] = {"insert/off", "insert/soft", "insert/hard", "insert/fold", "insert/fuzz",
                                           "insert/crush", "insert/phasr", "insert/flang", "insert/chor"};
    uint32_t k, ty;
    int16_t *vp;
    ui_host_init();
    for (k = 0; k < NPAGES && (ui.home || page_id(cur_page(), 0) != P_ITYPE); k++)
        tap(B_FX);
    check("FX: INSERT 1/2 after FX (TYPE A B C), OFF by default",
          str_eq(cur_page()->title, "INSERT") && page_id(cur_page(), 0) == P_ITYPE && page_id(cur_page(), 3) == P_IC &&
              str_eq(page_desc(cur_page(), 1, &vp)->label, "A") && TSEL->p[P_ITYPE] == IT_OFF);
    turn(EN_K1, 1);
    ui_frame();
    check("INSERT TYPE: one step right is SOFT; A B C read DRIVE TONE LEVEL", TSEL->p[P_ITYPE] == IT_SOFT &&
          str_eq(page_desc(cur_page(), 1, &vp)->label, "DRIVE") && str_eq(page_desc(cur_page(), 2, &vp)->label, "TONE") &&
          str_eq(page_desc(cur_page(), 3, &vp)->label, "LEVEL"));
    TSEL->p[P_SRR] = 30;
    for (ty = 0; ty < IT_N; ty++) {
        TSEL->p[P_ITYPE] = (int16_t)ty;
        ui.force = 1;
        snap_page(SHOT[ty]);
    }
    TSEL->p[P_ITYPE] = IT_CRUSH;
    ui_frame();
    check("INSERT on CRUSH: A B C read BITS RATE LPF", str_eq(page_desc(cur_page(), 1, &vp)->label, "BITS") &&
          str_eq(page_desc(cur_page(), 2, &vp)->label, "RATE") && str_eq(page_desc(cur_page(), 3, &vp)->label, "LPF"));
    page_turn(1);
    ui.force = 1;
    snap_page("insert/mix_srr");
    check("INSERT 2/2: MIX (100 % by default) and SRR (OFF by default)",
          str_eq(cur_page()->title, "INSERT") && page_id(cur_page(), 0) == P_IMIX && page_id(cur_page(), 1) == P_SRR &&
              TP[P_IMIX].def == 127 && TP[P_SRR].def == 0);
    turn(EN_K2, -200);
    ui_frame();
    {
        char v[12];
        const char *u = 0;
        param_format(&TP[P_SRR], TSEL->p[P_SRR], v, &u);
        check("SRR turned down to 0 reads OFF", TSEL->p[P_SRR] == 0 && str_eq(v, "OFF"));
    }
    TSEL->p[P_ITYPE] = IT_OFF;
}

/* INIT SOUND resets the FILTER (spec §3); a model change keeps it (as DIST) */
static void test_initsnd_filter(void)
{
    uint32_t k;
    ui_host_init();
    TSEL->p[P_FTYPE] = FT_HP;
    TSEL->p[P_FCUT] = 40;
    TSEL->p[P_FENV] = -20;
    model_step(1);
    check("a model change keeps the FILTER", TSEL->p[P_FTYPE] == FT_HP && TSEL->p[P_FCUT] == 40);
    for (k = 0; k < NPAGES && (ui.home || !str_eq(cur_page()->title, "TOOLS")); k++) {
        tap(B_SAVE);                                 /* (SAVE opens its pages on the release: 2026-10-26) */
    }
    turn(EN_K1 + 1, 1);                              /* INIT SOUND: one detent arms, */
    ui_frame();
    turn(EN_K1 + 1, 1);                              /* a second acts */
    ui_frame();
    check("INIT SOUND resets the FILTER (TYPE OFF, CUT 127, ENV 0)",
          TSEL->p[P_FTYPE] == FT_OFF && TSEL->p[P_FCUT] == 127 && TSEL->p[P_FENV] == 0 && TSEL->p[P_FDEC] == 40);
}

/* the knobs follow the screen: while the PERFORM screen shows (FX held, a layer key held, or the PAGE), they are
 * its macros, never the page under it; when it goes, the macros snap back (user report 2026-10-08, PAGE mode) */
static void test_perform_knobs_follow_screen(void)
{
    uint32_t f, i, ch = 0, mode;
    for (mode = 0; mode < 2u; mode++) {              /* 0: HOLD, 1: PAGE */
        int16_t tb[P_COUNT];
        ui_host_init();
        settings.perfpage = (uint32_t)mode;
        tap(B_EDIT);                                 /* SOUND under it */
        for (f = 0; f < 600u; f++) {                 /* FX held past the hold time: the PERFORM screen */
            press(B_FX);
            ui_frame();
        }
        keys(1u << 1);                               /* a black key: the layer's (REPEAT 1/8) */
        press(B_FX);
        ui_frame();
        release_all();                               /* FX let go, the key still held */
        keys(1u << 1);
        ui_frame();
        if (mode) {
            press(B_SEQ);                            /* PAGE: another page under it (a page button) closes the PAGE
                                                      * (SELECT turns none under PERFORM: fixes 2026-10-21 item 7) */
            keys(1u << 1);
            ui_frame();
            release_all();
            keys(1u << 1);
            ui_frame();                              /* (SEQ: its page on the release; a plain button under the
                                                      * layer, spec 2026-10-31 §3) */
        }
        memcpy(tb, TSEL->p, sizeof tb);
        turn(EN_K1, 3);
        turn(EN_K1 + 2, 3);
        keys(1u << 1);
        ui_frame();
        for (i = 0; i < P_COUNT; i++)
            ch |= TSEL->p[i] != tb[i] && ui.layer;
        keys(0);                                     /* the key let go: the screen goes, the macros stay on */
        ui_frame();
        ui_frame();
        ch |= ui.layer || !perf_k[0] || !perf_k[2];
    }
    check("PERFORM shown (FX or a layer key held, or PAGE): the knobs are its macros, not the page under it", !ch);
}

/* bug 2026-10-10 ("the PERFORM knobs change the page's knobs"): FX and a layer key held into the menu, the menu
 * closed with both still held: FX is dead (no map), the key keeps the screen and the knobs. A knob turn then must not
 * mark the dead FX open: once the key is let go the screen goes with it, and the knobs edit the page again with the
 * page shown (never the page under a PERFORM screen) */
static void test_perform_dead_knob(void)
{
    uint32_t mode, i, bad = 0;
    for (mode = 0; mode < 2u; mode++) {
        int16_t tb[P_COUNT];
        ui_host_init();
        settings.perfpage = mode;
        tap(B_EDIT);                                 /* SOUND under it */
        keys(1u << 1);                               /* FX + a black key: the layer's (REPEAT 1/8) */
        fx_hold_frames(3);
        for (i = 0; i < 1000u && !ui.menu; i++) {    /* HOME held too: the menu */
            fm1_in.buttons |= 1u << panel.btn[B_HOME];
            fx_hold_frames(1);
        }
        fm1_in.buttons &= ~(1u << panel.btn[B_HOME]);
        fx_hold_frames(2);
        menu_close();
        fx_hold_frames(3);                           /* FX still held (dead), the key still held */
        turn(EN_K2, 3);                              /* the key's screen: CRUSH */
        fx_hold_frames(2);
        keys(0);                                     /* the key let go, FX still held */
        fx_hold_frames(3);
        memcpy(tb, TSEL->p, sizeof tb);
        turn(EN_K1, 3);
        fx_hold_frames(2);
        for (i = 0; i < P_COUNT; i++)
            bad |= ui.layer && TSEL->p[i] != tb[i];
        bad |= ui.layer;                             /* a dead FX hold shows no map */
        release_all();
        ui_frame();
    }
    check("FX dead (held through the menu), its layer key let go: no PERFORM screen over the page the knobs edit", !bad);
}

/* user 2026-10-08: on SEQ's page (PATTERN, its only one since the redesign) and on HOME's TRACKS mode (not armed) the 16 white
 * keys are the selected track's steps in the bank (a tap: on / off, OCT+ held + a key: accent); they play nothing and select
 * nothing; OCT+ / OCT- change the bank; the key LEDs show the steps. The track: ALGO (or TRACKS' KNOB 1) */
static void test_step_keys_everywhere(void)
{
    uint32_t p, k;
    for (p = 0; p < 2u; p++) {
        uint32_t a2, a3, was;
        const char *name = p < 1u ? "PATTERN" : "TRACKS";
        char what[96];
        ui_host_init();
        for (k = 0; k < NSTEP; k++)
            trk[1].step[k].on = trk[1].step[k].acc = 0;
        trk[1].p[P_SLEN] = 32;
        if (p < 1u) {
            seq_open("PATTERN");
        } else {
            open_tracks();                           /* HOME's TRACKS mode */
        }
        turn(EN_ALGO, 1);                            /* track 2, nothing played */
        ui_frame();
        a2 = hit_age(&trk[1]);
        a3 = hit_age(&trk[2]);
        was = trk[1].step[2].on;
        keys(1u << STEP_KEY[2]);                     /* the third white key: step 3 */
        ui_frame();
        keys(0);
        ui_frame();
        snprintf(what, sizeof what, "%s: white key 3 toggles step 3 of the selected track, plays / selects nothing", name);
        check(what, trk[1].step[2].on == !was && song.sel == 1 && hit_age(&trk[1]) == a2 && hit_age(&trk[2]) == a3 &&
              song.seq_mode == 1u && led_lit(14u + STEP_KEY[2]));
        tap(B_OCTUP);                                /* bank 2: steps 17..32 */
        grid_acc(STEP_KEY[0]);                       /* OCT+ held + the key: accent (item 1) */
        snprintf(what, sizeof what, "%s: OCT+ alone is the bank; OCT+ held + a key accents the step", name);
        check(what, ui.bank == 1u && trk[1].step[16].on && trk[1].step[16].acc);
    }
}

/* REC is no mute key (spec §4): on HOME's TRACKS mode REC held + a white key toggles a step (nothing armed), REC
 * + top C# / D# change no MUTE setting; the release is a REC tap (arms); a long REC hold there asks nothing */
static void test_tracks_rec_mutes(void)
{
    uint32_t k;
    ui_host_init();
    settings.mutebar = 0;
    open_tracks();
    press(B_REC);
    ui_frame();
    keys(1u << KEY_TRK_KEY[3]);
    press(B_REC);
    ui_frame();
    keys(0);
    press(B_REC);
    ui_frame();
    keys(1u << 22);                                  /* top D# */
    press(B_REC);
    ui_frame();
    keys(0);
    press(B_REC);
    ui_frame();
    check("TRACKS: REC held + key 4 mutes nothing (step 4 toggled), + top D# sets no MUTE mode",
          trk[3].p[P_MUTE] == 0 && settings.mutebar == 0u && trk[0].step[3].on && !ui.confirm && mix_mode());
    release_all();
    ui_frame();
    check("TRACKS: REC let go: a tap (track 1 armed)", song.rec == 1u);
    transport_req = 2;
    ui_frame();
    for (k = 0; k < 60u; k++) {                      /* REC held 1.2 s alone */
        host_ticks += 20u * 1000u * FM1_TICKS_PER_US;
        press(B_REC);
        ui_frame();
    }
    release_all();
    ui_frame();
    check("TRACKS: a long REC hold asks nothing", !ui.confirm);
    transport_req = 2;
    ui_frame();
}

/* FM: SOUND 1/4 MODEL TUNE DECAY INDEX, 2/4 RATIO MDEC SWEEP FBK, 3/4 VEL LVL PAN NOTE, 4/4 CHOKE; the graph shows the
 * ratio and its sidebands */
static uint32_t fm_graph_lit(void)               /* lit pixels right of the model's name: the sideband bars */
{
    uint32_t n = 0, x, y;
    for (y = Y_GRAPH + 30u; y < Y_GRAPH + 100u; y++)
        for (x = 100u; x < 236u; x++)
            n += fb[y * 240u + x] != 0;
    return n;
}

/* review fix: the FM graph keeps clear of CHOKE, and INDEX 0 (a pure sine) shows no sidebands */
static uint32_t fm_graph_area(uint32_t y0, uint32_t y1)   /* lit pixels at x 100..235 of rows y0..y1 (graph rows) */
{
    uint32_t n = 0, x, y;
    for (y = Y_GRAPH + y0; y < Y_GRAPH + y1; y++)
        for (x = 100u; x < 236u; x++)
            n += fb[y * 240u + x] == C_INK;
    return n;
}


static void test_fm_screens(void)
{
    uint32_t k;
    int16_t *vp;
    ui_host_init();
    old_set_model(TSEL, DM_FM);
    for (k = 0; k < NPAGES && (ui.home || cur_page()->fam != FAM_SND); k++)
        tap(B_EDIT);
    ui.force = 1;
    snap_page("fm/sound_1");
    check("FM: SOUND 1/4 holds MODEL TUNE DECAY INDEX; the graph draws the sidebands",
          str_eq(page_desc(cur_page(), 1, &vp)->label, "TUNE") && str_eq(page_desc(cur_page(), 3, &vp)->label, "INDEX") &&
          str_eq(N_FMRATIO[TSEL->p[P_E3]], "1.41") && fm_graph_lit() > 150u);
    page_turn(1);
    ui.force = 1;
    snap_page("fm/sound_2");
    check("FM: SOUND 2/4 holds RATIO MDEC SWEEP FBK",
          str_eq(page_desc(cur_page(), 0, &vp)->label, "RATIO") && str_eq(page_desc(cur_page(), 3, &vp)->label, "FBK"));
}

/* KTRX: SOUND 1/4 holds MODEL TUNE DECAY SWEEP, 2/4 SDEC PUNCH NOISE SUB; the graph draws the pitch and the level,
 * SWEEP raising the pitch curve (more lit rows high in the graph); the CHOKE label stays out of it */
static uint32_t ktrx_high_lit(void)                  /* lit pixels in the graph's upper half (the pitch above ~2 oct) */
{
    return fm_graph_area(60u, 92u);                   /* (screen rows 134..165: under the title) */
}

static void test_ktrx_screens(void)
{
    uint32_t k, flat, swept, plain, labels;
    int16_t *vp;
    ui_host_init();
    old_set_model(TSEL, DM_KTRX);
    for (k = 0; k < NPAGES && (ui.home || cur_page()->fam != FAM_SND); k++)
        tap(B_EDIT);
    TSEL->p[P_E3] = 96;                              /* SDEC 96: a sweep the graph's 300 ms shows */
    TSEL->p[P_E2] = 0;
    ui.force = 1;
    snap_page("ktrx/sweep0");
    flat = ktrx_high_lit();
    {                                                /* review fix: no spike at the level curve's start (its fade-in
                                                      * starts at 0: the first column stays low) */
        uint32_t x, y, n = 0;
        for (y = Y_GRAPH + 66u; y < Y_GRAPH + 80u; y++)
            for (x = 100u; x < 102u; x++)
                n += fb[y * 240u + x] == C_INK;
        check("KTRX graph: SWEEP 0: nothing at the top of its first columns (no spike from the level's start)", n == 0u);
    }
    TSEL->p[P_E2] = 127;
    ui.force = 1;
    snap_page("ktrx/sweep127");
    swept = ktrx_high_lit();
    TSEL->p[P_E2] = 64;
    TSEL->p[P_E3] = 32;
    ui.force = 1;
    snap_page("ktrx/sound_1");
    plain = fm_graph_area(0u, 110u);
    printf("     KTRX graph: upper half lit %u (SWEEP 0) / %u (SWEEP 127, SDEC 96)\n", flat, swept);
    check("KTRX: SOUND 1/4 holds MODEL TUNE DECAY SWEEP (FREITAKT LIST: the hit's band, no pitch graph)",
          str_eq(page_desc(cur_page(), 1, &vp)->label, "TUNE") && str_eq(page_desc(cur_page(), 3, &vp)->label, "SWEEP"));
    TSEL->p[P_CHOKE] = 2;
    ui.force = 1;
    ui_frame();
    labels = fm_graph_area(0u, 110u);
    check("KTRX graph: the CHOKE label stays out of it", labels == plain);
    TSEL->p[P_CHOKE] = 0;
    page_turn(1);
    ui.force = 1;
    snap_page("ktrx/sound_2");
    check("KTRX: SOUND 2/4 holds SDEC PUNCH NOISE SUB",
          str_eq(page_desc(cur_page(), 0, &vp)->label, "SDEC") && str_eq(page_desc(cur_page(), 3, &vp)->label, "SUB"));
}

/* SDTRX, B2TRX: their SOUND pages (MODEL + the first three knobs, then the next four) */
static void test_trx_drum_screens(void)
{
    static const struct { uint32_t m; const char *dir, *l1, *l3, *l4, *l7; } S[] = {
        {DM_SDTRX, "trxdrums/sdtrx", "TUNE", "SNAP", "TONE", "CLIP"},
        {DM_B2TRX, "trxdrums/b2trx", "TUNE", "RAMP", "HOLD", "DIRT"}};
    uint32_t i, k, ok = 1;
    int16_t *vp;
    for (i = 0; i < 2u; i++) {
        char name[48];
        ui_host_init();
        old_set_model(TSEL, S[i].m);
        for (k = 0; k < NPAGES && (ui.home || cur_page()->fam != FAM_SND); k++)
            tap(B_EDIT);
        ui.force = 1;
        snprintf(name, sizeof name, "%s_sound_1", S[i].dir);
        snap_page(name);
        ok &= str_eq(page_desc(cur_page(), 1, &vp)->label, S[i].l1) && str_eq(page_desc(cur_page(), 3, &vp)->label, S[i].l3);
        page_turn(1);
        ui.force = 1;
        snprintf(name, sizeof name, "%s_sound_2", S[i].dir);
        snap_page(name);
        ok &= str_eq(page_desc(cur_page(), 0, &vp)->label, S[i].l4) && str_eq(page_desc(cur_page(), 3, &vp)->label, S[i].l7);
    }
    check("SDTRX, B2TRX: SOUND 1/4 holds MODEL TUNE DECAY SNAP / RAMP, 2/4 TONE .. CLIP / HOLD .. DIRT", ok);
}

/* CPTRX / MATRX: their SOUND pages hold the knobs in order (1/4 MODEL TUNE DECAY + the third, 2/4 the next four, 3/4
 * the last); screenshots build/ui_shots/engines/trxn_*.png */
static void test_trxn_screens(void)
{
    static const struct { uint32_t m; const char *k3, *k4, *k7, *s1, *s2, *s3; } M[2] = {
        {DM_CPTRX, "CLPY", "HARD", "RTUN", "engines/trxn_cptrx_1", "engines/trxn_cptrx_2", "engines/trxn_cptrx_3"},
        {DM_MATRX, "ATT", "REV", "HARD", "engines/trxn_matrx_1", "engines/trxn_matrx_2", "engines/trxn_matrx_3"}};
    uint32_t i, k, ok = 1;
    int16_t *vp;
    for (i = 0; i < 2u; i++) {
        ui_host_init();
        old_set_model(TSEL, M[i].m);
        for (k = 0; k < NPAGES && (ui.home || cur_page()->fam != FAM_SND); k++)
            tap(B_EDIT);
        ui.force = 1;
        snap_page(M[i].s1);
        ok &= str_eq(page_desc(cur_page(), 1, &vp)->label, "TUNE") && str_eq(page_desc(cur_page(), 2, &vp)->label, "DECAY") &&
              str_eq(page_desc(cur_page(), 3, &vp)->label, M[i].k3);
        page_turn(1);
        ui.force = 1;
        snap_page(M[i].s2);
        ok &= str_eq(page_desc(cur_page(), 0, &vp)->label, M[i].k4);
        page_turn(1);
        ui.force = 1;
        snap_page(M[i].s3);
        ok &= str_eq(page_desc(cur_page(), 0, &vp)->label, M[i].k7);
    }
    check("CPTRX, MATRX: SOUND 1/4 MODEL TUNE DECAY CLPY / ATT, 2/4 from HARD / REV, 3/4 RTUN / HARD", ok);
}

/* SDEFM / CPEFM / RSEFM: their SOUND pages hold the knobs in order (1/4 MODEL TUNE DECAY + the third, 2/4 the next
 * four, 3/4 the last); screenshots build/ui_shots/engines/efmn_*.png */
static void test_efmn_screens(void)
{
    static const struct { uint32_t m; const char *k3, *k4, *k8, *s1, *s2, *s3; } M[3] = {
        {DM_SDEFM, "NOIS", "NDEC", "HPF", "engines/efmn_sdefm_1", "engines/efmn_sdefm_2", "engines/efmn_sdefm_3"},
        {DM_CPEFM, "CLPS", "CDEC", "HPF", "engines/efmn_cpefm_1", "engines/efmn_cpefm_2", "engines/efmn_cpefm_3"},
        {DM_RSEFM, "MOD", "HPF", "SMOD", "engines/efmn_rsefm_1", "engines/efmn_rsefm_2", "engines/efmn_rsefm_3"}};
    uint32_t i, k, ok = 1;
    int16_t *vp;
    for (i = 0; i < 3u; i++) {
        ui_host_init();
        old_set_model(TSEL, M[i].m);
        for (k = 0; k < NPAGES && (ui.home || cur_page()->fam != FAM_SND); k++)
            tap(B_EDIT);
        ui.force = 1;
        snap_page(M[i].s1);
        ok &= str_eq(page_desc(cur_page(), 1, &vp)->label, "TUNE") && str_eq(page_desc(cur_page(), 2, &vp)->label, "DECAY") &&
              str_eq(page_desc(cur_page(), 3, &vp)->label, M[i].k3);
        page_turn(1);
        ui.force = 1;
        snap_page(M[i].s2);
        ok &= str_eq(page_desc(cur_page(), 0, &vp)->label, M[i].k4);
        page_turn(1);
        ui.force = 1;
        snap_page(M[i].s3);
        ok &= str_eq(page_desc(cur_page(), 0, &vp)->label, M[i].k8);
    }
    check("SDEFM, CPEFM, RSEFM: SOUND 1/4 MODEL TUNE DECAY NOIS / CLPS / MOD, 2/4 from NDEC / CDEC / HPF, 3/4 HPF / HPF / "
          "SMOD", ok);
}

/* ---- ACID plan B: KEYS (spec §5) */
/* an SCL tap turns KEYS on (its LED, the top bar KEYS F3) and off; the 27 keys play the selected track chromatically
 * (a drum transposed: nt = NOTE - 60, F3 = 53); OCT- / OCT+ shift by an octave, -2..+2; KEYS selects no track */
static void test_keys_map_oct(void)
{
    char b[16];
    uint32_t k, a, ok = 1;
    ui_host_init();
    tap(B_SCL);
    check("KEYS: an SCL tap turns it on: the SCL LED lit, the top bar KEYS F3, the keys KEYS' (seq_mode 3)",
          ui.keys && led_on(panel.btn[B_SCL]) && head_mode(b) && str_eq(b, "KEYS F3") && song.seq_mode == 3u);
    for (k = 0; k < 27u; k++) {
        a = hit_age(&trk[0]);
        keys(1u << k);
        ui_frame();
        keys(0);
        ui_frame();
        ok &= hit_age(&trk[0]) != a && trk[0].nt == (int32_t)k - 7;
    }
    check("KEYS: key k (0 = F3) hits the selected drum transposed to NOTE 53 + k (nt k - 7), every key", ok);
    check("KEYS: the keys select no track (HOME)", song.sel == 0);
    tap(B_OCTUP);
    tap(B_OCTUP);
    tap(B_OCTUP);
    keys(1u);
    ui_frame();
    keys(0);
    ui_frame();
    check("KEYS: OCT+ three times: +2 octaves at most (F3's key plays F5, nt +17), KEYS F5, OCT+ lit",
          song.octave == 2 && trk[0].nt == 17 && head_mode(b) && str_eq(b, "KEYS F5") && led_on(panel.btn[B_OCTUP]));
    for (k = 0; k < 5u; k++)
        tap(B_OCTDN);
    keys(1u);
    ui_frame();
    keys(0);
    ui_frame();
    check("KEYS: OCT- five times: -2 octaves at most (F1, nt -31), KEYS F1",
          song.octave == -2 && trk[0].nt == -31 && head_mode(b) && str_eq(b, "KEYS F1"));
    tap(B_SCL);
    keys(1u << KEY_TRK_KEY[3]);
    ui_frame();
    keys(0);
    ui_frame();
    check("KEYS off: the LED dark, no KEYS in the top bar, the keys select and hit tracks again",
          !ui.keys && !led_on(panel.btn[B_SCL]) && !head_mode(b) && song.sel == 3 && song.seq_mode == 0u);
}

/* ACID under KEYS: mono, the last key wins; held: the gate stays open past GATE; a key over the sounding one slides
 * (same voice); the sounding key's release closes the gate, an earlier key's does nothing */
static void test_keys_acid_mono(void)
{
    track_t *t = &trk[0];
    uint32_t age, k;
    ui_host_init();
    old_set_model(t, DM_ACID);
    tap(B_SCL);
    keys(1u << 7);                                   /* C4 */
    ui_frame();
    age = t->v[0].age;
    check("KEYS, ACID: a key starts its note (C4), the gate open",
          t->v[0].active && (t->v[0].x[2] & 1) && t->v[0].x[1] == 60 * 16);
    for (k = 0; k < 400u; k++)                       /* 400 blocks, 290 ms: well past GATE (half a 1/16) */
        ui_frame();
    check("KEYS, ACID: held past GATE the gate stays open (the key holds it)", (t->v[0].x[2] & 1) != 0);
    keys(1u << 7 | 1u << 12);                        /* F4 over it */
    ui_frame();
    check("KEYS, ACID: a key over the sounding one slides to it (same voice, no restart)",
          t->v[0].age == age && t->v[0].x[1] == 65 * 16 && (t->v[0].x[2] & 1));
    keys(1u << 12);                                  /* C4 (no longer sounding) let go */
    ui_frame();
    check("KEYS, ACID: an earlier key let go: the note goes on", (t->v[0].x[2] & 1) != 0);
    keys(0);
    ui_frame();
    check("KEYS, ACID: the sounding key let go: the gate closes", !(t->v[0].x[2] & 1));
    keys(1u << 9);                                   /* D4 after all were let go */
    ui_frame();
    check("KEYS, ACID: a key after all were let go starts a new note", t->v[0].age != age && t->v[0].x[1] == 62 * 16);
    keys(0);
    ui_frame();
}

/* Review Focus 1: a key's ACID note ends on its release wherever the keys went meanwhile */
static void test_keys_release_anywhere(void)
{
    ui_host_init();
    old_set_model(&trk[0], DM_ACID);
    old_set_model(&trk[1], DM_ACID);
    tap(B_SCL);
    keys(1u << 7);                                   /* C4 on track 1 */
    ui_frame();
    tap(B_SCL);                                      /* KEYS off, C4 still held */
    keys(0);
    ui_frame();
    check("KEYS turned off while a key holds an ACID note: its release still ends it",
          !(trk[0].v[0].x[2] & 1) && song.seq_mode == 0u);
    tap(B_SCL);
    keys(1u << 7);
    ui_frame();
    turn(EN_ALGO, 1);                                /* track 2 selected while C4 holds track 1's note */
    ui_frame();
    keys(0);
    ui_frame();
    check("KEYS: the track changed while held: the release ends track 1's note", song.sel == 1 && !(trk[0].v[0].x[2] & 1));
    keys(1u << 7);                                   /* C4 on track 2 */
    ui_frame();
    tap(B_HOME);                                     /* a HOME tap: HOME's TRACKS mode (KEYS still on: SEL lit) */
    keys(0);
    ui_frame();
    check("KEYS: TRACKS opened while held: the release ends the note, the keys stay KEYS'", mix_mode() &&
          keys_mode() && song.seq_mode == 3u && !(trk[1].v[0].x[2] & 1));
    tap(B_HOME);
    keys(1u << 7);                                   /* C4 on track 2, held */
    ui_frame();
    turn(EN_ALGO, -1);                               /* track 1 */
    ui_frame();
    keys(1u << 7 | 1u << 9);                         /* D4 on track 1 while C4 still holds track 2's note */
    ui_frame();
    check("KEYS: a key on another track ends the held note (mono over the keys)",
          !(trk[1].v[0].x[2] & 1) && (trk[0].v[0].x[2] & 1) && trk[0].v[0].x[1] == 62 * 16);
    keys(0);
    ui_frame();
    check("KEYS: ... and its own release ends the new one", !(trk[0].v[0].x[2] & 1));
}

/* Review Focus 3: with KEYS on, FX held (PERFORM), TRACKS, COMP, the menu and NAME keep their keys */
static void test_keys_owners(void)
{
    uint32_t a;
    int16_t duck;
    ui_host_init();
    old_set_model(&trk[0], DM_ACID);
    tap(B_SCL);
    press(B_FX);
    ui_frame();
    keys(1u << 4);                                   /* A3 with FX held */
    fx_hold_frames(2);
    check("KEYS + FX held: the key is PERFORM's (track 3 repeating while held), no note",
          ((perf_held >> (PF_T1 + 2u)) & 1u) && !trk[0].v[0].active);
    keys(0);
    fx_hold_frames(1);
    release_all();
    ui_frame();
    open_tracks();                                   /* HOME's TRACKS mode, nothing armed (FREITAKT: SEL lit = KEYS) */
    keys(1u << STEP_KEY[2]);
    ui_frame();
    a = trk[0].v[0].active;
    keys(0);
    ui_frame();
    check("KEYS + TRACKS: a white key plays the track (KEYS wins, SEL lit), no step",
          mix_mode() && keys_mode() && song.seq_mode == 3u && !trk[0].step[2].on && a);
    tap(B_SCL);                                      /* KEYS off: TRACKS' step grid again */
    keys(1u << STEP_KEY[2]);
    ui_frame();
    keys(0);
    ui_frame();
    check("TRACKS, KEYS off: a white key is the track's step (seq_mode 1)",
          mix_mode() && song.seq_mode == 1u && trk[0].step[2].on);
    tap(B_SCL);                                      /* KEYS on again */
    open_comp(0);
    duck = trk[3].p[P_DUCK];
    keys(1u << KEY_TRK_KEY[3]);
    ui_frame();
    keys(0);
    ui_frame();
    check("KEYS + COMP: a white key plays the track, no duck", comp_mode() && keys_mode() &&
          trk[3].p[P_DUCK] == duck);
    tap(B_SCL);                                      /* KEYS off: COMP's duck keys */
    keys(1u << KEY_TRK_KEY[3]);
    ui_frame();
    keys(0);
    ui_frame();
    check("COMP, KEYS off: a white key ducks its track", comp_mode() && trk[3].p[P_DUCK] != duck);
    tap(B_SCL);
    hold(B_HOME);                                    /* the menu */
    check("KEYS + the menu: the keys are not KEYS' (as without it)", ui.menu && !keys_mode() && song.seq_mode == 0u);
    hold(B_HOME);
    project_page();
    project_save(0);
    name_open(NK_RENAME, 0);
    a = trk[0].v[0].age;
    name_type(7);                                    /* C4 */
    check("KEYS + NAME: the keys type, no note", name_on() && song.seq_mode == 1u && trk[0].v[0].age == a);
    name_close();
}

/* main.c's UBOOT (OCT- + OCT+ held 5 s): under KEYS both buttons stay visible; pressed together the octave is back */
static void test_keys_oct_uboot(void)
{
    uint32_t both;
    ui_host_init();
    tap(B_SCL);
    seq_open("PATTERN");
    both = (1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP]);
    press(B_OCTDN);
    press(B_OCTUP);
    ui_input();
    check("KEYS: OCT- + OCT+ together: the octave back where it was, both buttons visible to main.c (UBOOT), no bank",
          song.octave == 0 && (fm1_in.buttons & both) == both && ui.bank == 0);
    release_all();
}

/* ---- ACID plan B: live record (spec §6) */
/* armed and playing, KEYS on ACID: a key writes the nearest step (ON + NOTE), TIE on each later step it is held past
 * the middle of, a legato key SLIDE on the held note's last step */
static void test_keys_live_rec(void)
{
    track_t *t = &trk[0];
    ui_host_init();
    old_set_model(t, DM_ACID);
    memset(t->step, 0, sizeof t->step);
    tap(B_SCL);                                      /* KEYS, HOME */
    song.rec = 1u;                                   /* track 1 armed (as TRACKS' REC tap) */
    transport_req = 1;
    seq_play_to(0, 2);                               /* step 3 just started */
    keys(1u << 7);                                   /* C4 */
    ui_frame();
    seq_play_to(0, 5);                               /* held through steps 4 and 5's middles */
    keys(1u << 7 | 1u << 11);                        /* E4 over it: legato */
    ui_frame();
    keys(1u << 11);                                  /* C4 let go */
    ui_frame();
    keys(0);                                         /* E4 let go at once */
    ui_frame();
    seq_play_to(0, 7);
    check("KEYS live record: the key's note on the nearest step (ON + C4)", t->step[2].on && !t->step[2].tie &&
          t->step[2].nt == 0);
    check("KEYS live record: held into the later steps: TIE on them", t->step[3].tie && !t->step[3].on &&
          t->step[4].tie && !t->step[4].on);
    check("KEYS live record: a legato key: SLIDE on the held note's last step, its own note (E4) next",
          t->step[4].slide && !t->step[2].slide && !t->step[3].slide && t->step[5].on && t->step[5].nt == 4 &&
          !t->step[5].slide);
    check("KEYS live record: let go before the next step's middle: nothing there", !t->step[6].tie && !t->step[6].on);
    transport_req = 2;
    ui_frame();
}

/* Review Focus 4: pressed past the middle of the last step: the next one across the wrap (LEN 4), which then does not
 * hit a second time; the TIE follows across it */
static void test_keys_live_rec_wrap(void)
{
    track_t *t = &trk[0];
    uint32_t k, age;
    ui_host_init();
    old_set_model(t, DM_ACID);
    memset(t->step, 0, sizeof t->step);
    t->p[P_SLEN] = 4;
    tap(B_SCL);
    song.rec = 1u;
    transport_req = 1;
    seq_play_to(0, 3);
    for (k = 0; k < 120u; k++)                       /* 70 % into step 4 (172 blocks a 1/16 at 120 BPM) */
        ui_frame();
    keys(1u << 9);                                   /* D4 */
    ui_frame();
    age = t->v[0].age;
    check("KEYS live record past mid-step: the next step across the wrap (step 1: ON + D4), step 4 untouched",
          t->step[0].on && t->step[0].nt == 2 && !t->step[3].on && !t->step[3].tie && t->rskip && t->rskip_idx == 0u);
    seq_play_to(0, 2);                               /* held through step 1 and step 2's middle */
    check("KEYS live record: the written step does not hit again, the note holds (same voice); TIE on step 2",
          t->v[0].age == age && (t->v[0].x[2] & 1) && t->step[1].tie && t->step[0].on && !t->step[0].tie);
    keys(0);
    ui_frame();
    seq_play_to(0, 3);
    check("KEYS live record: let go at step 3's start: no TIE there", !t->step[2].tie && !t->step[2].on);
    transport_req = 2;
    ui_frame();
}

/* Review Focus 4, the block a held note passes a step's middle in: a legato key pressed in that very block still finds
 * the TIE there (keys_tie before the presses), so the SLIDE goes on it and no rest is left between the two notes */
static void test_keys_live_rec_mid_block(void)
{
    track_t *t = &trk[0];
    uint32_t k, half;
    ui_host_init();
    old_set_model(t, DM_ACID);
    memset(t->step, 0, sizeof t->step);
    tap(B_SCL);
    song.rec = 1u;
    transport_req = 1;
    seq_play_to(0, 2);
    keys(1u << 7);                                   /* C4 on step 3 */
    ui_frame();
    seq_play_to(0, 3);
    half = step_samples(t, div_period((uint32_t)t->p[P_SDIV], t->seq_rem), t->seq_cnt) / 2u;
    for (k = 0; k < 400u && t->seq_pos <= half; k++)  /* the block that passes step 4's middle has just run */
        ui_frame();
    keys(1u << 7 | 1u << 11);                        /* E4 over it in the next block */
    ui_frame();
    keys(0);
    ui_frame();
    seq_play_to(0, 6);
    check("KEYS live record: a legato key in the block that passes the middle: TIE + SLIDE on step 4, E4 on step 5",
          t->step[2].on && t->step[2].nt == 0 && t->step[3].tie && t->step[3].slide && t->step[4].on &&
          t->step[4].nt == 4);
    transport_req = 2;
    ui_frame();
}

/* Review fix: a note held longer than LEN stops recording at the phrase's start: its TIEs never reach the step the key
 * wrote (it stays ON with its note); a later legato note does not move that start */
static void test_keys_live_rec_long_hold(void)
{
    track_t *t = &trk[0];
    uint32_t k, ok = 1;
    ui_host_init();
    old_set_model(t, DM_ACID);
    memset(t->step, 0, sizeof t->step);
    t->p[P_SLEN] = 4;
    tap(B_SCL);
    song.rec = 1u;
    transport_req = 1;
    seq_play_to(0, 1);                               /* step 2 just started */
    keys(1u << 7);                                   /* C4 on step 2 */
    ui_frame();
    seq_play_to(0, 3);
    seq_play_to(0, 1);                               /* across the wrap */
    seq_play_to(0, 3);                               /* held through 5 steps' middles */
    keys(0);
    ui_frame();
    for (k = 0; k < 4u; k++)
        ok &= k == 1u || (t->step[k].tie && !t->step[k].on && t->step[k].nt == 0);
    check("KEYS live record, held longer than LEN: the start step stays ON with its note (C4), TIEs on the others",
          t->step[1].on && !t->step[1].tie && t->step[1].nt == 0 && ok);
    transport_req = 2;
    ui_frame();
    ui_host_init();
    old_set_model(t, DM_ACID);
    memset(t->step, 0, sizeof t->step);
    t->p[P_SLEN] = 4;
    tap(B_SCL);
    song.rec = 1u;
    transport_req = 1;
    seq_play_to(0, 1);
    keys(1u << 7);                                   /* C4 on step 2 */
    ui_frame();
    seq_play_to(0, 3);                               /* TIE on step 3 */
    keys(1u << 7 | 1u << 11);                        /* E4 over it on step 4: legato */
    ui_frame();
    keys(1u << 11);
    ui_frame();
    seq_play_to(0, 1);
    seq_play_to(0, 3);                               /* E4 held round to the phrase's start and past it */
    keys(0);
    ui_frame();
    check("KEYS live record, a legato note held round the loop: the phrase's start (C4) kept, step 3 TIE + SLIDE kept",
          t->step[1].on && !t->step[1].tie && t->step[1].nt == 0 && t->step[2].tie && t->step[2].slide &&
          t->step[2].nt == 0 && t->step[3].on && t->step[3].nt == 4 && t->step[0].tie && t->step[0].nt == 4);
    transport_req = 2;
    ui_frame();
}

/* Review fix: a step held through becomes a plain TIE: its PROB, RATCH, ACC (and SLIDE) cleared, as rec_hit's step */
static void test_keys_live_rec_tie_clean(void)
{
    track_t *t = &trk[0];
    step_t *s = &t->step[3];
    ui_host_init();
    old_set_model(t, DM_ACID);
    memset(t->step, 0, sizeof t->step);
    s->on = 1;
    s->acc = 1;
    s->slide = 1;
    s->cond = 5;
    s->rat = 2;
    s->nt = -7;
    tap(B_SCL);
    song.rec = 1u;
    transport_req = 1;
    seq_play_to(0, 2);
    keys(1u << 7);                                   /* C4 on step 3, held through step 4 */
    ui_frame();
    seq_play_to(0, 4);
    keys(0);
    ui_frame();
    check("KEYS live record: a step held through is a plain TIE (no ACC, PROB, RATCH, SLIDE; the held note)",
          s->tie && !s->on && !s->acc && !s->slide && s->cond == 0 && s->rat == 0 && s->nt == 0);
    transport_req = 2;
    ui_frame();
}

/* a drum track records ON + NOTE only: holding its key writes no TIE */
static void test_keys_live_rec_drum(void)
{
    track_t *t = &trk[0];
    ui_host_init();
    memset(t->step, 0, sizeof t->step);
    tap(B_SCL);
    song.rec = 1u;
    transport_req = 1;
    seq_play_to(0, 2);
    keys(1u << 9);                                   /* D4, held into step 6 */
    ui_frame();
    seq_play_to(0, 5);
    keys(0);
    ui_frame();
    check("KEYS live record, a drum: ON + NOTE (D4: nt +2) on the nearest step, no TIE on the steps held through",
          t->step[2].on && t->step[2].nt == 2 && !t->step[3].tie && !t->step[3].on && !t->step[4].tie &&
          !t->step[4].on);
    transport_req = 2;
    ui_frame();
}

/* spec §2: a step the grid keys turn on gets the track's last played / recorded note (C4 at first); a TIE step turned
 * on plays (its TIE cleared) */
static void test_step_key_note(void)
{
    track_t *t;
    ui_host_init();
    t = TSEL;
    old_set_model(t, DM_ACID);
    memset(t->step, 0, sizeof t->step);
    open_step_page();
    grid_hold(WHITE[0], 0);
    check("grid: a new step, nothing played yet: C4", t->step[0].on && t->step[0].nt == 0);
    tap(B_SCL);                                      /* KEYS: F4 played */
    keys(1u << 12);
    ui_frame();
    keys(0);
    ui_frame();
    tap(B_SCL);
    check("KEYS: the track's last key note kept (F4, nt +5)", t->key_nt == 5);
    grid_hold(WHITE[1], 0);
    check("grid: a new step gets the track's last key note (F4)", t->step[1].on && t->step[1].nt == 5);
    t->step[2].tie = 1;
    t->step[2].nt = -3;
    grid_hold(WHITE[2], 0);
    check("grid: a TIE step turned on plays: ON, TIE cleared, the last key note", t->step[2].on && !t->step[2].tie &&
          t->step[2].nt == 5);
    t->step[3].tie = 1;
    grid_acc(WHITE[3]);
    check("grid: a TIE step accented (OCT+ held): ON + ACC, TIE cleared", t->step[3].on && t->step[3].acc &&
          !t->step[3].tie && t->step[3].nt == 5);
}

/* bug 2026-10-10 (user: "ACID only"): the last KEYS note goes to the steps the grid keys turn on only on a gated
 * (ACID) track; on a drum the new step is C4 (tap, hold, hold + knob) */
static void test_step_key_note_drum(void)
{
    track_t *t;
    uint32_t pass;
    for (pass = 0; pass < 2u; pass++) {              /* 0: a drum (the kick), 1: ACID */
        int8_t want;
        ui_host_init();
        t = TSEL;
        if (pass)
            old_set_model(t, DM_ACID);
        memset(t->step, 0, sizeof t->step);
        tap(B_SCL);                                  /* KEYS: D4 played */
        keys(1u << 9);
        ui_frame();
        keys(0);
        ui_frame();
        tap(B_SCL);
        open_step_page();
        grid_hold(WHITE[0], 0);                      /* tap */
        grid_acc(WHITE[1]);                          /* OCT+ held: an accent */
        grid_hold_turn(WHITE[2], 0, -2);             /* hold + KNOB 1 */
        keys(0);
        ui_frame();
        want = pass ? 2 : 0;
        check(pass ? "ACID: a step the grid keys turn on (tap, hold, hold + knob) gets the last KEYS note (D4)"
                   : "drum: a step the grid keys turn on (tap, hold, hold + knob) is C4 after KEYS played D4",
              t->step[0].on && t->step[1].on && t->step[2].on && t->step[0].nt == want && t->step[1].nt == want &&
                  t->step[2].nt == want);
    }
}

/* bug 2026-10-10: no last KEYS note carries over a LOAD or a cleared pattern */
static void test_key_note_reset(void)
{
    uint32_t k, ok = 1;
    ui_host_init();
    project_save(0);
    for (k = 0; k < NTRK; k++)
        trk[k].key_nt = 5;
    project_load(0);
    for (k = 0; k < NTRK; k++)
        ok &= trk[k].key_nt == 0;
    check("LOAD: every track's last KEYS note back to C4", ok);
    trk[2].key_nt = 5;
    track_clear(&trk[2]);
    check("a track's pattern cleared (CLEAR TRACK, CLR PATTERN): its last KEYS note back to C4", trk[2].key_nt == 0);
    for (k = 0; k < NTRK; k++)
        trk[k].key_nt = 5;
    seq_clear_all();
    for (ok = 1, k = 0; k < NTRK; k++)
        ok &= trk[k].key_nt == 0;
    check("CLR*: every track's last KEYS note back to C4", ok);
}

/* bug 2026-10-10: live record of a drum key (or MIDI) writes the step's NOTE as C4 (the hit sounded at C4): a NOTE
 * left on an off step (a step-record REST, TIME -> REST) does not come back */
static void test_rec_hit_drum_note(void)
{
    track_t *t = &trk[0];
    ui_host_init();
    memset(t->step, 0, sizeof t->step);
    t->step[3].nt = 7;                               /* off, a NOTE left on it */
    song.rec = 1u;
    transport_req = 1;
    seq_play_to(0, 3);
    keys(1u << 0);                                   /* F3: track 1's key (HOME, KEYS off) */
    ui_frame();
    keys(0);
    ui_frame();
    check("drum live record onto a step holding a NOTE: ON at C4", t->step[3].on && t->step[3].nt == 0);
    transport_req = 2;
    ui_frame();
}

static void srec_turn(uint32_t knob, int32_t d)      /* one detent, 0.1 s after the last (no acceleration; the name
                                                     * from the removed step record's tests) */
{
    turn(EN_K1 + knob, d);
    host_ticks += 100u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
}

static void acid_line(track_t *t)                    /* C4 ACC, D4 SLIDE, E4, TIE, G4 RATCH 2, a rest */
{
    memset(t->step, 0, sizeof t->step);
    t->step[0].on = 1;
    t->step[0].acc = 1;
    t->step[1].on = 1;
    t->step[1].nt = 2;
    t->step[1].slide = 1;
    t->step[2].on = 1;
    t->step[2].nt = 4;
    t->step[3].tie = 1;
    t->step[4].on = 1;
    t->step[4].nt = 7;
    t->step[4].rat = 1;
}

/* ---- ACID plan B: the final review's fixes */
/* final review 2: KEYS off, armed: a hit recorded onto a TIE step plays (on, no TIE: acid_step reads TIE first) */
static void test_rec_hit_on_tie(void)
{
    track_t *t = &trk[0];
    ui_host_init();
    old_set_model(t, DM_ACID);
    memset(t->step, 0, sizeof t->step);
    t->step[3].tie = 1;
    t->step[3].nt = 7;
    song.rec = 1u;
    transport_req = 1;
    seq_play_to(0, 3);
    keys(1u << 0);                                   /* F3: track 1's key (HOME, KEYS off) */
    ui_frame();
    keys(0);
    ui_frame();
    check("live record onto a TIE step: ON, the TIE cleared", t->step[3].on && !t->step[3].tie);
    transport_req = 2;
    ui_frame();
}

/* final review 3: LEN cut under a recording key's phrase start (8 -> 4, the key from step 7): the recording ends, the
 * loop's notes stay (no TIEs round and round) */
static void test_keys_live_rec_len_cut(void)
{
    track_t *t = &trk[0];
    uint32_t k, ok = 1;
    ui_host_init();
    old_set_model(t, DM_ACID);
    memset(t->step, 0, sizeof t->step);
    t->p[P_SLEN] = 8;
    for (k = 0; k < 4u; k++) {
        t->step[k].on = 1;
        t->step[k].nt = (int8_t)k;
    }
    tap(B_SCL);
    song.rec = 1u;
    transport_req = 1;
    seq_play_to(0, 6);
    keys(1u << 7);                                   /* C4 on step 7, held */
    ui_frame();
    t->p[P_SLEN] = 4;
    for (k = 0; k < 1200u; k++)                      /* held over the 4 steps twice */
        ui_frame();
    keys(0);
    ui_frame();
    for (k = 0; k < 4u; k++)
        ok &= t->step[k].on && !t->step[k].tie && t->step[k].nt == (int8_t)k;
    check("KEYS live record: LEN cut under the held note's start: the loop's notes stay", ok);
    check("KEYS live record: ... the key's own note on step 7", t->step[6].on && t->step[6].nt == 0);
    t->p[P_SLEN] = 16;
    transport_req = 2;
    ui_frame();
}


/* final review 8 (plan rulings: the sequencer and a key on one ACID voice): rests leave a key's gate open; an ON step
 * takes the voice over (its gate counts); the sequencer's gate closes while the key is still held; the release then
 * does nothing */
static void test_keys_seq_takeover(void)
{
    track_t *t = &trk[0];
    uint32_t age;
    ui_host_init();
    old_set_model(t, DM_ACID);
    memset(t->step, 0, sizeof t->step);
    t->step[4].on = 1;                               /* one ON step (5: G4), rests elsewhere */
    t->step[4].nt = 7;
    tap(B_SCL);
    transport_req = 1;
    seq_play_to(0, 1);
    keys(1u << 7);                                   /* C4 held */
    ui_frame();
    seq_play_to(0, 3);
    check("KEYS + sequencer: rests leave the key's gate open", (t->v[0].x[2] & 1) && t->gate_n == 0u);
    age = t->v[0].age;
    seq_play_to(0, 4);
    ui_frame();
    check("KEYS + sequencer: an ON step takes the voice over: restarted (no glide from the key's note), G4, its gate counting",
          t->v[0].age != age && t->v[0].x[1] == 67 * 16 && t->gate_n != 0u);
    seq_play_to(0, 6);
    check("KEYS + sequencer: the step's gate closes, the key still held", !(t->v[0].x[2] & 1));
    keys(0);
    ui_frame();
    check("KEYS + sequencer: the key let go then: nothing (gate closed, nothing counting)",
          !(t->v[0].x[2] & 1) && t->gate_n == 0u);
    transport_req = 2;
    ui_frame();
}

/* the screens, for the user's eye: build/ui_shots/acid/ */
static void test_acid_screens(void)
{
    track_t *t;
    uint32_t k;
    ui_host_init();
    t = TSEL;
    old_set_model(t, DM_ACID);
    acid_line(t);
    t->step[6].on = 1;
    t->step[6].nt = -5;
    t->step[7].on = 1;
    t->step[7].nt = 7;
    t->step[7].acc = 1;
    t->step[7].slide = 1;
    t->step[8].on = 1;
    t->step[8].nt = 12;
    t->step[9].tie = 1;
    t->step[10].tie = 1;
    t->step[12].on = 1;
    t->step[12].nt = -12;
    t->step[12].rat = 2;
    t->step[14].on = 1;
    t->step[14].nt = 3;
    t->step[14].acc = 1;
    tap(B_SCL);
    snap_page("acid/01_home_keys_f3");
    tap(B_OCTUP);
    snap_page("acid/02_home_keys_f4");
    tap(B_OCTDN);
    seq_open("PATTERN");                             /* (SEQ: PATTERN; the ACID roll went with the STEP page) */
    snap_page("acid/03_pattern_keys");
    keys(1u << 11);                                  /* E4 held: played (KEYS) */
    snap_page("acid/04_pattern_key_held");
    keys(0);
    ui_frame();
    transport_req = 1;
    seq_play_to(0, 7);
    snap_page("acid/06_pattern_play_step08");
    seq_play_to(0, 12);
    snap_page("acid/07_pattern_play_step13");
    transport_req = 2;
    ui_frame();
    tap(B_SCL);                                      /* KEYS off */
    seq_select(1);                                   /* track 2, a drum: its transposed steps marked */
    for (k = 0; k < 16u; k++) {
        trk[1].step[k].on = k % 4u == 0u || k == 6u || k == 14u;
        trk[1].step[k].nt = (int8_t)(k == 4u ? 5 : k == 6u ? -7 : k == 14u ? 12 : 0);
    }
    snap_page("acid/08_drum_pattern_transposed");
    trk[1].key_nt = 3;
    press(B_OCTUP);                                  /* step 2 accented (OCT+ held) and held             */
    ui_frame();
    keys(1u << WHITE[1]);
    for (k = 0; k < 10u; k++) {
        host_ticks += 50u * 1000u * FM1_TICKS_PER_US;
        ui_frame();
    }
    snap_page("acid/09_pattern_held_step");
    keys(0);
    ui_frame();
    release_all();
    ui_frame();
    seq_select(0);
    for (k = 0; k < NPAGES && (ui.home || cur_page()->fam != FAM_SND); k++)
        tap(B_EDIT);
    snap_page("acid/10_edit_acid");
    t->p[P_E0 + 3] = 127;
    t->p[P_E0 + 2] = 70;
    snap_page("acid/11_edit_acid_reso127");
    t->p[P_E0 + 1] = 127;
    t->p[P_E0 + 7] = 127;
    snap_page("acid/12_edit_acid_decay_gate");
    seq_select(1);                                   /* track 2, a drum, on PATTERN: KEYS on */
    seq_open("PATTERN");
    tap(B_SCL);
    snap_page("acid/13_drum_pattern_keys");
    seq_select(0);                                   /* ACID: a KEYS key held (G7) */
    seq_open("PATTERN");
    tap(B_OCTUP);
    tap(B_OCTUP);
    keys(1u << 26);                                  /* G7 */
    snap_page("acid/15_pattern_key_g7");
    keys(0);
    tap(B_OCTDN);
    tap(B_OCTDN);
    tap(B_SCL);
    ui_frame();
    t->step[8].cond = 3;                             /* KEYS off: C5 (tied) at PROB 10 %; step 5 (G4, RATCH 2) held, */
    keys(1u << STEP_KEY[4]);                         /* KNOB 1: its PROB (85 %) */
    ui_frame();
    for (k = 0; k < 3u; k++)
        srec_turn(0, -1);
    snap_page("acid/16_pattern_held_prob");
    keys(0);
    ui_frame();
    t->step[8].cond = t->step[4].cond = 0;
    printf("     acid: screens in build/ui_shots/acid\n");
}

/* ---- CPU (spec 2026-10-11 §10): PRIO on the SOUND list after CHOKE; the CPU icon after an overload shed */

static void test_prio_sound_list(void)
{
    uint8_t ids[SND_N];
    uint32_t m, n, ok = 1;
    ui_host_init();
    for (m = 0; m < NMODELS; m++) {
        old_set_model(TSEL, m);
        n = snd_list(TSEL, ids);
        if (m == DM_SMPL)                            /* (SAMPLE: then its own page, ATK SUS REL LOOP) */
            ok &= n == SND_N && ids[13] == P_PRIO && ids[12] == P_CHOKE && ids[16] == P_SATK;
        else
            ok &= n <= 16u && ids[n - 1u] == P_PRIO && ids[n - 2u] == P_CHOKE;
    }
    check("PRIO: every model's SOUND list ends CHOKE, PRIO (SAMPLE's before its ATK SUS REL LOOP page)", ok);
    old_set_model(TSEL, DM_CYMB);
    n = snd_list(TSEL, ids);
    ui.home = 0;                                     /* the SOUND page with PRIO */
    ui.page = (uint8_t)(page_first(FAM_SND) + (n - 1u) / 4u);
    ui.force = 1;
    ui_frame();
    turn(EN_K1 + (n - 1u) % 4u, 1);
    ui_frame();
    check("PRIO: its knob sets LOW (the selected track), HIGH back", TSEL->p[P_PRIO] == 1 && trk[1].p[P_PRIO] == 0);
    snap_page("fixes/sound_prio");
    turn(EN_K1 + (n - 1u) % 4u, -1);
    ui_frame();
    check("PRIO: ... and HIGH again", TSEL->p[P_PRIO] == 0);
}


/* ---- PRESETS (plan 2026-10-19-presets Task 3): the encoder's popup, the list on a long EDIT, the preset's name and
 * the edited mark; the screens in build/ui_shots/presets/ */
static int voice_is(const track_t *t, uint32_t id)    /* t's MODEL and 8 SOUND knobs = preset id's */
{
    preset_t p;
    uint32_t i;
    if (!preset_get(id, &p) || (uint32_t)t->p[P_MODEL] != p.model || t->model != p.model)
        return 0;
    for (i = 0; i < 8u; i++)
        if (t->p[P_E0 + i] != p.k[i])
            return 0;
    return 1;
}
static void pre_turn(int32_t d)                       /* PRESETS, d detents, one frame */
{
    turn(EN_PRESET, d);
    ui_frame();
}
static void pre_wait(uint32_t ms)                     /* ms later, one frame */
{
    host_ticks += ms * 1000u * FM1_TICKS_PER_US;
    ui_frame();
}
static uint32_t lit_box(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)   /* ink pixels in the box (the frame's
                                                                                  * edge columns left out) */
{
    uint32_t n = 0, x, y;
    for (y = y0; y < y1; y++)
        for (x = x0; x < x1; x++)
            n += fb[y * 240u + x] == C_INK && x != 0u && x != 239u;
    return n;
}
static int as_full(void)                               /* the screen = what a full repaint draws now (nothing stale) */
{
    static uint16_t was[240 * 240];
    memcpy(was, fb, sizeof was);
    ui.force = 1;
    ui_draw();
    return !memcmp(was, fb, sizeof was);
}
static void users_clear(void)                          /* no user preset, nothing left to write */
{
    memset(&upr, 0, sizeof upr);
    upr_pending = 0;
}

static void test_popup_shows_and_loads(void)
{
    uint32_t id0, id1, m;
    int16_t bpm0;
    ui_host_init();
    users_clear();
    ui_frame();
    id0 = TSEL->pre;
    id1 = preset_step(TSEL, id0, 1);
    check("PRESETS: the power-on track's voice is its first preset", id0 == preset_first_of(DM_K909) && !TSEL->pre_ed);
    pre_turn(1);
    check("PRESETS one detent right: the next preset loaded at once (MODEL and the 8 knobs), not edited; the popup up",
          TSEL->pre == id1 && voice_is(TSEL, id1) && !TSEL->pre_ed && pp_on());
    check("PRESETS popup: the preset in the middle, the three loadable ones before and after around it",
          ui.pr.shown[3] == id1 && ui.pr.shown[2] == id0 && ui.pr.shown[1] == preset_step(TSEL, id0, -1) &&
              ui.pr.shown[0] == preset_step(TSEL, ui.pr.shown[1], -1) && ui.pr.shown[4] == preset_step(TSEL, id1, 1) &&
              ui.pr.shown[5] == preset_step(TSEL, ui.pr.shown[4], 1) &&
              ui.pr.shown[6] == preset_step(TSEL, ui.pr.shown[5], 1));
    check("PRESETS popup: a screen of its own, nothing of HOME under it (no CPU, no BPM: the header's middle and the "
          "screen's last rows bare; the preset's bar lit)",
          lit_box(0, 112, 240, 152) > 4000u && lit_box(110, 0, 200, 32) == 0u && lit_box(0, 230, 240, 240) == 0u &&
              as_full());
    snap_page("presets/01_popup");
    pre_wait(2900);
    check("PRESETS popup: still up 2.9 s after the detent", pp_on());
    pre_turn(1);
    pre_wait(2900);
    check("PRESETS popup: another detent keeps it up 3 s more", pp_on() && TSEL->pre == preset_step(TSEL, id1, 1));
    pre_turn(-1);
    pre_wait(3100);
    check("PRESETS popup: gone 3 s after the last detent, HOME drawn whole again (= a full repaint)",
          !pp_on() && ui.home && lit_box(0, 112, 240, 152) < 4000u && as_full());
    snap_page("presets/02_popup_closed");
    bpm0 = song.g[G_BPM];
    pre_turn(1);
    turn(EN_K1, 1);                                     /* a knob over the PRESETS screen: the knob's screen */
    ui_frame();
    ui_frame();
    check("PRESETS screen, a knob turned: the knob's screen instead (the later one), drawn whole",
          kp.on && !pp_on() && as_full());
    pre_turn(1);
    check("knob screen, PRESETS turned: the PRESETS screen instead, drawn whole", pp_on() && !kp.on && as_full());
    pre_wait(3100);
    check("PRESETS screen after the knob's: gone 3 s after, HOME whole", !pp_on() && !kp.on && ui.home && as_full());
    song.g[G_BPM] = bpm0;                               /* (the tempo and the preset as they were) */
    pre_turn(-1);
    pre_turn(-1);
    pre_wait(3100);
    pre_turn(-1);
    check("PRESETS one detent left: the one before (the first again)", TSEL->pre == id0 && voice_is(TSEL, id0));
    TSEL->pre = (uint16_t)PRE_NONE;                    /* (a converted project: no preset) */
    m = (uint32_t)TSEL->p[P_MODEL];
    pre_turn(-1);
    check("PRESETS with no preset: the first detent (either way) loads the voice's first preset",
          TSEL->pre == preset_first_of(m) && voice_is(TSEL, preset_first_of(m)));
    while (TSEL->pre != PRE_BANK_START[PB_MODERN] - 1u)    /* the MACHINEDRUM bank's last (fix 8: MODERN next) */
        pre_turn(TSEL->pre < PRE_BANK_START[PB_MODERN] ? 1 : -1);
    pre_turn(1);
    check("PRESETS across banks: MACHINEDRUM's last -> MODERN's first", TSEL->pre == PRE_BANK_START[PB_MODERN] &&
                                                                          preset_bank_of(ui.pr.shown[2]) == PB_MD);
    check("PRESETS popup at a bank's end: a dotted rule over the preset's bar, the screen a full repaint's",
          lit_box(8, 110, 232, 111) > 100u && lit_box(8, 82, 232, 83) == 0u && as_full());
    snap_page("presets/03_popup_bank_edge");
    pre_wait(3100);
    open_tracks();
    pre_turn(1);
    check("PRESETS on TRACKS: loads, the popup a screen of its own, TRACKS stays under it", pp_on() && mix_mode() &&
              TSEL->pre == PRE_BANK_START[PB_MODERN] + 1u && lit_box(0, 112, 240, 152) > 4000u && as_full());
    snap_page("presets/04_popup_tracks");
    pre_wait(3100);
    check("PRESETS on TRACKS: the rows back, whole", !pp_on() && mix_mode() && as_full());
}

/* Review Focus 1: turning PRESETS fast while a dense pattern plays: every detent's preset loads, the output stays
 * bounded, no voice is left stuck */
static void test_popup_fast_while_playing(void)
{
    static int32_t l[2205], r[2205];                    /* 50 ms: 40 detents in 2 s */
    uint32_t k, i, ok = 1, stuck = 0;
    int32_t peak = 0;
    ui_host_init();
    users_clear();
    kit_session();
    trk[5].p[P_MUTE] = 0;
    song.rec = 0;
    for (k = 0; k < NTRK; k++)
        for (i = 0; i < 16u; i++)
            trk[k].step[i].on = 1;                      /* every track on every step */
    transport_req = 1;
    ui_frame();
    for (k = 0; k < 40u; k++) {
        uint32_t want = preset_step(TSEL, TSEL->pre, 1);
        pre_turn(1);
        ok &= TSEL->pre == want && voice_is(TSEL, want) && pp_on();
        render_mix(l, r, 2205);
        for (i = 0; i < 2205u; i++) {
            int32_t a = l[i] < 0 ? -l[i] : l[i], b = r[i] < 0 ? -r[i] : r[i];
            peak = a > peak ? a : peak;
            peak = b > peak ? b : peak;
        }
    }
    check("PRESETS: 40 detents in 2 s while 8 tracks play every step: each detent's preset loaded at once",
          ok && song.playing);
    check("PRESETS fast while playing: the output sounds and stays inside 16 bits", peak > 1000 && peak <= 32767);
    transport_req = 2;
    ui_frame();
    for (i = 0; i < 160u; i++)                          /* 8 s */
        render_mix(0, 0, 2205);
    for (k = 0; k < NTRK; k++)
        for (i = 0; i < NDV; i++)
            stuck += trk[k].v[i].active;
    check("PRESETS fast while playing: stopped, every voice ends (none stuck)", !song.playing && !stuck);
}

/* Review Focus 2: ACID on 2 tracks: the popup skips ACID's presets, the list refuses them; never a third ACID track */
static void test_popup_skips_capped_acid(void)
{
    uint32_t a = preset_first_of(DM_ACID), z = a, k, n;
    while (z < PRE_NFACT && PRE_FACTORY[z].model == DM_ACID)
        z++;                                            /* z: the preset after ACID's (a .. z - 1) */
    ui_host_init();
    users_clear();
    drum_set_model(&trk[0], DM_ACID);
    drum_set_model(&trk[1], DM_ACID);
    song.sel = 2;
    preset_load(&trk[2], a - 1u);                       /* the preset before ACID's */
    ui_frame();
    pre_turn(1);
    for (k = 0, n = 0; k < NTRK; k++)
        n += trk[k].p[P_MODEL] == DM_ACID;
    check("PRESETS with ACID on 2 tracks: the detent past ACID's presets to the one after them",
          z > a + 1u && trk[2].pre == z && trk[2].p[P_MODEL] != DM_ACID && n == 2u);
    for (k = 0, n = 0; k < 7u; k++)
        n += ui.pr.shown[k] >= a && ui.pr.shown[k] < z;
    check("PRESETS with ACID on 2 tracks: the popup leaves them out", ui.pr.shown[2] == a - 1u && n == 0u);
    snap_page("presets/05_popup_acid_capped");
    pre_turn(-1);
    check("PRESETS with ACID on 2 tracks: back past it too", trk[2].pre == a - 1u);
    pre_wait(3100);
    preset_list_open();
    pre_turn(1);                                        /* the cursor onto ACID's first: no audition */
    check("list with ACID on 2 tracks: the cursor on ACID's preset auditions nothing (ACID: 2 TRACKS MAX)",
          ui.pr.cur == a && trk[2].pre == a - 1u && trk[2].p[P_MODEL] != DM_ACID &&
              str_eq(ui.msg, "ACID: 2 TRACKS MAX"));
    ui.msg[0] = 0;
    tap(B_OCTUP);
    check("list with ACID on 2 tracks: LOAD on ACID's preset refused (ACID: 2 TRACKS MAX), the voice kept, the list too",
          trk[2].pre == a - 1u && trk[2].p[P_MODEL] != DM_ACID && str_eq(ui.msg, "ACID: 2 TRACKS MAX") && pl_on());
    drum_set_model(&trk[1], DM_K808);                   /* a place frees */
    tap(B_OCTUP);
    check("list: once a place frees, LOAD gives the 3rd track ACID (the list closed)",
          trk[2].pre == a && trk[2].p[P_MODEL] == DM_ACID && !pl_on());
}

/* the list: SAVE into the first free user slot or the one under the cursor (a used one: SAVE twice), DELETE (OCT-
 * twice) on a used user slot, LOAD (OCT+), the banks (KNOB 2), the cursor (PRESETS, KNOB 1); a SAVE while playing is
 * in the list at once and in flash at STOP */
static void test_list_save_delete_overwrite(void)
{
    uint32_t u0 = PRE_NFACT, kb = PRE_BANK_START[PB_CLASSIC];   /* (fix 8: CLASSIC the last factory bank) */
    char nb[9];
    ui_host_init();
    users_clear();
    ui_frame();
    TSEL->p[P_E1] = 10;                                 /* an edited DECAY */
    preset_touch(TSEL);
    hold(B_EDIT);
    check("EDIT held: the PRESETS list, its cursor on the track's preset", pl_on() && ui.pr.cur == TSEL->pre);
    snap_page("presets/06_list");
    tap(B_SAVE);
    check("list SAVE on a factory preset: the voice into the first free user slot, now the track's (not edited)",
          upr.s[0].used == 1u && upr.s[0].model == DM_K909 && upr.s[0].k[1] == 10 && TSEL->pre == u0 &&
              !TSEL->pre_ed && ui.pr.cur == u0 && pl_on());
    preset_name_of(u0, nb);
    check("list SAVE: auto-named (K909 01), said in the title", str_eq(nb, "K909 01") && str_eq(ui.msg, "SAVED K909 01"));
    snap_page("presets/07_list_saved");
    TSEL->p[P_E1] = 20;
    tap(B_SAVE);
    check("list SAVE on a used user slot: armed only (SAVE AGAIN), the slot as it was", upr.s[0].k[1] == 10 && ui.pr.arm == 1u);
    snap_page("presets/08_list_save_over_armed");
    tap(B_SAVE);
    check("list SAVE again: saved over it", upr.s[0].k[1] == 20 && !ui.pr.arm);
    pre_turn(3);
    check("PRESETS in the list: the cursor, one per detent (an empty slot: nothing auditioned)",
          ui.pr.cur == u0 + 3u && TSEL->pre == u0);
    tap(B_SAVE);
    check("list SAVE on an empty user slot: saved there at once", upr.s[3].used == 1u && TSEL->pre == u0 + 3u && !ui.pr.arm);
    tap(B_OCTDN);
    check("list OCT- on a used user slot: armed only (DELETE AGAIN)", upr.s[3].used == 1u && ui.pr.arm == 2u);
    snap_page("presets/09_list_delete_armed");
    tap(B_OCTDN);
    preset_name_of(TSEL->pre, nb);
    check("list OCT- again: deleted; the track keeps its knobs and its reference (shown as -)",
          upr.s[3].used != 1u && TSEL->pre == u0 + 3u && TSEL->p[P_E1] == 20 && str_eq(nb, "-"));
    turn(EN_K2, -1);
    ui_frame();
    check("list KNOB 2 left: the bank before, on its first (CLASSIC)", ui.pr.cur == kb);
    tap(B_SAVE);
    check("list SAVE off the user bank: the first free slot (2: 1 used, 4 deleted)", upr.s[1].used == 1u && TSEL->pre == u0 + 1u);
    turn(EN_K2, -1);
    ui_frame();
    tap(B_OCTUP);
    check("list OCT+ (LOAD): the preset under the cursor on the track, the list closed",
          TSEL->pre == kb && voice_is(TSEL, kb) && !pl_on());
    hold(B_EDIT);
    check("(the list again, on it)", pl_on() && ui.pr.cur == kb);
    tap(B_OCTDN);
    check("list OCT- on a factory preset: nothing deleted", !ui.pr.arm && TSEL->pre == kb);
    host_ticks += 300u * 1000u * FM1_TICKS_PER_US;      /* (a new turn: no acceleration) */
    turn(EN_K1, 1);
    ui_frame();
    check("list KNOB 1: the cursor", ui.pr.cur == kb + 1u);
    transport_req = 1;
    ui_frame();
    ui_frame();
    tap(B_SAVE);
    check("list SAVE while playing: in the list at once (slot 3), flash at the next STOP; the sound saved the track's, "
          "not the auditioned one",
          song.playing && upr.s[2].used == 1u && TSEL->pre == u0 + 2u && upr_pending &&
              upr.s[2].model == PRE_FACTORY[kb].model && upr.s[2].k[0] == PRE_FACTORY[kb].k[0] && voice_is(TSEL, u0 + 2u));
    snap_page("presets/10_list_saved_playing");
    transport_req = 2;
    ui_frame();
    ui_frame();
    check("list: STOP writes it (nothing left pending)", !song.playing && !upr_pending && pl_on());
    tap(B_EDIT);
    check("list EDIT: closed, back where the hold began (HOME)", !pl_on() && ui.home);
    hold(B_EDIT);
    tap(B_HOME);
    check("list HOME: closed", !pl_on());
}

/* EDIT: a tap as always (SOUND, its next page), held GLO_HOLD_MS the list over the page the press left; step record's
 * EDIT a rest, held too */
static void test_long_edit_opens_list(void)
{
    uint32_t k, i, cur;
    ui_host_init();
    users_clear();
    tap(B_EDIT);
    check("EDIT tap: SOUND at once, no list", !pl_on() && !ui.home && cur_page()->fam == FAM_SND);
    k = ui.page;
    press(B_EDIT);
    for (i = 0; i < 300u; i++)                          /* 0.3 s */
        ui_input();
    release_all();
    ui_frame();
    check("EDIT held 0.3 s: a tap (the next SOUND page), no list", !pl_on() && ui.page == k + 1u);
    k = ui.page;
    hold(B_EDIT);
    check("EDIT held 0.8 s: the list, the page under it the one before the press", pl_on() && ui.page == k);
    tap(B_EDIT);
    check("EDIT in the list: back to that page", !pl_on() && ui.page == k && cur_page()->fam == FAM_SND && !ui.home);
}

/* review fix 5: MIDI LEARN on, EDIT held does nothing (no list: LEARN stays on); the tap as always */
static void test_edit_hold_in_learn(void)
{
    ui_host_init();
    users_clear();
    ml_toggle();
    check("(MIDI LEARN on)", ui.ml.on);
    tap(B_EDIT);
    check("LEARN: an EDIT tap opens SOUND as always, LEARN stays on", ui.ml.on && !pl_on() && !ui.home &&
          cur_page()->fam == FAM_SND);
    hold(B_EDIT);
    check("LEARN: EDIT held opens no list (the press a tap), LEARN stays on", ui.ml.on && !pl_on() &&
          cur_page()->fam == FAM_SND);
    ml_toggle();
    hold(B_EDIT);
    check("LEARN off: EDIT held opens the list again", !ui.ml.on && pl_on());
    tap(B_EDIT);
}

/* the track's preset on its SOUND page ("PRESET K909", "*" once a SOUND knob moved) and on its TRACKS row */
static void test_name_and_edited_mark(void)
{
    uint32_t a, b;
    ui_host_init();
    users_clear();
    tap(B_EDIT);
    snap_page("presets/11_sound_preset");
    a = 0;
    check("SOUND: the track's preset named on the MODEL row, not edited", col_has(0, "K909|") && !strchr(ui.col[0], '*') &&
          !TSEL->pre_ed);
    host_ticks += 300u * 1000u * FM1_TICKS_PER_US;
    turn(EN_K2, 1);                                     /* TUNE */
    ui_frame();
    host_ticks += 3100u * 1000u * FM1_TICKS_PER_US;
    for (b = 0; b < 50u; b++)                           /* (the knob popup gone) */
        ui_frame();
    snap_page("presets/12_sound_edited");
    b = 0;
    check("SOUND: a SOUND knob turned marks the preset edited (*)", TSEL->pre_ed && strchr(ui.col[0], '*') && a == b);
    open_tracks();
    snap_page("presets/13_tracks_preset");
    TSEL->pre = (uint16_t)PRE_NONE;
    tap(B_EDIT);
    snap_page("presets/14_sound_no_preset");
    check("SOUND with no preset: the MODEL row's value -", col_has(0, "|-"));                               /* ("-" or "-*") */
}

/* ---- the list auditions (owner, 2026-10-19): every cursor move puts the highlighted preset on the track for now (its
 * MODEL and 8 knobs) and hits it once; the track's key plays it; LOAD keeps it and closes the list; any other way out
 * (EDIT, HOME) puts the track's sound from before back, as another track picked in the list does; SAVE stores that
 * sound, not the auditioned one; seven rows, no K1 / K2 hint */
static uint32_t voices_on(const track_t *t)
{
    uint32_t i, n = 0;
    for (i = 0; i < NDV; i++)
        n += t->v[i].active;
    return n;
}
typedef struct { int16_t p[P_COUNT]; uint16_t pre; uint8_t ed, model; } snd_t;   /* a track's whole sound */
static void snd_take(const track_t *t, snd_t *s)
{
    memcpy(s->p, t->p, sizeof s->p);
    s->pre = t->pre;
    s->ed = t->pre_ed;
    s->model = t->model;
}
static int snd_same(const track_t *t, const snd_t *s)
{
    return !memcmp(s->p, t->p, sizeof s->p) && s->pre == t->pre && s->ed == t->pre_ed && s->model == t->model;
}
static int voice_kept(const track_t *t, const snd_t *s)   /* everything but MODEL and the 8 SOUND knobs as in s */
{
    uint32_t id;
    for (id = 0; id < P_COUNT; id++)
        if (id != P_MODEL && !PRE_SOUND(id) && t->p[id] != s->p[id])
            return 0;
    return 1;
}

static void test_list_preview_scrub(void)
{
    uint32_t id0, want;
    snd_t s;
    ui_host_init();
    users_clear();
    ui_frame();
    TSEL->p[P_CHOKE] = 2;                               /* (a CHOKE, a LEVEL, a FILTER the audition leaves alone) */
    TSEL->p[P_LEVEL] = 77;
    TSEL->p[P_FCUT] = 50;
    snd_take(TSEL, &s);
    hold(B_EDIT);
    id0 = ui.pr.cur;
    drum_cut(TSEL);
    ui_frame();
    check("list open: nothing auditioned yet (the track as it was, nothing hit)", pl_on() && snd_same(TSEL, &s) &&
                                                                                   voices_on(TSEL) == 0u);
    pre_turn(1);
    check("list PRESETS one detent: the highlighted preset on the track at once (MODEL, 8 knobs), not edited, hit once",
          ui.pr.cur == id0 + 1u && voice_is(TSEL, id0 + 1u) && !TSEL->pre_ed && voices_on(TSEL) == 1u && pl_on());
    check("list audition: CHOKE, LEVEL, FILTER and the rest untouched", voice_kept(TSEL, &s));
    snap_page("presets/15_list_preview");
    drum_cut(TSEL);
    host_ticks += 300u * 1000u * FM1_TICKS_PER_US;      /* (a new turn: no acceleration) */
    turn(EN_K1, 1);
    ui_frame();
    check("list KNOB 1: the cursor, auditioned (one hit)", ui.pr.cur == id0 + 2u && voice_is(TSEL, id0 + 2u) &&
                                                          voices_on(TSEL) == 1u);
    drum_cut(TSEL);
    want = PRE_BANK_START[(preset_bank_of(ui.pr.cur) + PB_N - 1u) % PB_N];   /* (fix 8: CLASSIC is the last factory
                                                                              * bank, USER after it: back to MODERN) */
    turn(EN_SELECT, -1);
    ui_frame();
    check("list SELECT: the bank before's first, auditioned (another voice: one hit)", ui.pr.cur == want &&
          voice_is(TSEL, want) && voices_on(TSEL) == 1u && voice_kept(TSEL, &s));
    drum_cut(TSEL);
    turn(EN_K2, 1);
    ui_frame();
    want = PRE_BANK_START[(preset_bank_of(want) + 1u) % PB_N];
    check("list KNOB 2: the next bank's first, auditioned", ui.pr.cur == want && voice_is(TSEL, want) &&
                                                              voices_on(TSEL) == 1u);
    drum_cut(TSEL);
    keys(1u << KEY_TRK_KEY[0]);
    ui_frame();
    keys(0);
    ui_frame();
    check("list: the track's own key plays the auditioned sound (the track stays)", song.sel == 0u &&
          voice_is(TSEL, want) && voices_on(TSEL) == 1u && pl_on());
    while (preset_bank_of(ui.pr.cur) != PB_CLASSIC) {         /* the last factory bank (fix 8) */
        turn(EN_SELECT, 1);
        ui_frame();
    }
    want = ui.pr.cur;
    drum_cut(TSEL);
    turn(EN_SELECT, 1);                                 /* USER: its first slot empty */
    ui_frame();
    check("list on an empty user slot: no audition (the last one kept, nothing hit), EMPTY SLOT said",
          ui.pr.cur == PRE_NFACT && voice_is(TSEL, want) && voices_on(TSEL) == 0u && str_eq(ui.msg, "EMPTY SLOT"));
    tap(B_EDIT);
    check("list EDIT: the track's sound back (CHOKE, LEVEL, FILTER too)", !pl_on() && snd_same(TSEL, &s));
}

static void test_list_cancel_restores(void)
{
    uint32_t c;
    snd_t s, s1;
    ui_host_init();
    users_clear();
    ui_frame();
    TSEL->p[P_E1] = 10;                                 /* an edited sound */
    preset_touch(TSEL);
    snd_take(TSEL, &s);
    hold(B_EDIT);
    pre_turn(5);
    turn(EN_SELECT, -1);                                /* another voice (MODERN: CLASSIC's next is USER, fix 8) */
    ui_frame();
    check("(list: another voice auditioned)", (uint32_t)TSEL->p[P_MODEL] != s.model && pl_on());
    drum_cut(TSEL);
    tap(B_EDIT);
    check("list EDIT without LOAD: closed, MODEL, knobs, preset and * exactly as before, nothing hit",
          !pl_on() && snd_same(TSEL, &s) && voices_on(TSEL) == 0u);
    hold(B_EDIT);
    pre_turn(-3);
    check("(list: the one 3 before auditioned)", !snd_same(TSEL, &s));
    tap(B_HOME);
    check("list HOME without LOAD: closed, the sound back exactly", !pl_on() && snd_same(TSEL, &s));
    snd_take(&trk[1], &s1);
    hold(B_EDIT);
    pre_turn(2);
    c = ui.pr.cur;
    drum_cut(&trk[1]);
    turn(EN_ALGO, 1);
    ui_frame();
    check("list ALGORITHM: track 1's sound back, track 2 auditions the highlighted preset (one hit)",
          song.sel == 1u && snd_same(&trk[0], &s) && voice_is(&trk[1], c) && voices_on(&trk[1]) == 1u && pl_on());
    drum_cut(&trk[0]);
    keys(1u << KEY_TRK_KEY[0]);
    ui_frame();
    keys(0);
    ui_frame();
    check("list: track 1's white key: track 2's sound back, track 1 auditions it (the key's one hit)",
          song.sel == 0u && snd_same(&trk[1], &s1) && voice_is(&trk[0], c) && voices_on(&trk[0]) == 1u && pl_on());
    tap(B_EDIT);
    check("list EDIT: both tracks as before", !pl_on() && snd_same(&trk[0], &s) && snd_same(&trk[1], &s1));
}

static void test_list_load_closes(void)
{
    uint32_t c;
    ui_host_init();
    users_clear();
    ui_frame();
    TSEL->p[P_E1] = 10;
    preset_touch(TSEL);
    hold(B_EDIT);
    turn(EN_SELECT, -1);                                /* (MODERN: CLASSIC's next is USER, fix 8) */
    ui_frame();
    c = ui.pr.cur;
    tap(B_OCTUP);
    check("list LOAD (OCT+): the highlighted preset stays on the track (its preset, not edited), the list closed",
          !pl_on() && TSEL->pre == c && voice_is(TSEL, c) && !TSEL->pre_ed && ui.home);
    snap_page("presets/16_list_loaded");
}

static void test_list_save_saves_real_sound(void)
{
    uint32_t m;
    ui_host_init();
    users_clear();
    ui_frame();
    m = (uint32_t)TSEL->p[P_MODEL];
    TSEL->p[P_E1] = 10;
    preset_touch(TSEL);
    hold(B_EDIT);
    turn(EN_SELECT, -1);                                /* (MODERN: CLASSIC's next is USER, fix 8) */
    ui_frame();
    check("(list: another voice auditioned)", (uint32_t)TSEL->p[P_MODEL] != m);
    tap(B_SAVE);
    check("list SAVE while auditioning: the sound from before saved (its voice, DECAY 10), the track back to it, now "
          "its preset (not edited)",
          upr.s[0].used == 1u && upr.s[0].model == m && upr.s[0].k[1] == 10 && (uint32_t)TSEL->p[P_MODEL] == m &&
              TSEL->p[P_E1] == 10 && TSEL->pre == PRE_NFACT && !TSEL->pre_ed && ui.pr.cur == PRE_NFACT && pl_on());
    tap(B_EDIT);
    check("list EDIT after SAVE: the saved sound stays", !pl_on() && (uint32_t)TSEL->p[P_MODEL] == m &&
                                                         TSEL->p[P_E1] == 10 && TSEL->pre == PRE_NFACT);
}

static void test_list_seven_rows(void)
{
    uint32_t i, rows = 0, bands = 0, prev = 0, y;
    ui_host_init();
    users_clear();
    ui_frame();
    hold(B_EDIT);
    ui.force = 1;
    ui_frame();
    for (i = 0; i < 7u; i++)
        rows += lit_box(16, 48u + i * 20u + 2u, 120, 48u + i * 20u + 18u) > 0u;
    check("list: 7 preset rows", rows == 7u);
    check("list: the controls start under the 7th row (their dotted rule)", lit_box(0, 188, 240, 189) >= 119u);
    for (y = 189; y < 239u; y++) {                      /* (row 239: the frame) */
        uint32_t on = lit_box(0, y, 240, y + 1u) > 0u;
        bands += on && !prev;
        prev = on;
    }
    check("list: two hint lines (OCT+ / SAVE, OCT- / EDIT), no K1 / K2 line", bands == 2u);
}

/* the md-ui final review's deferred minors (2026-10-19) */
static uint32_t fb_diff(const uint16_t *a, uint32_t x0, uint32_t y0, uint32_t w, uint32_t h)   /* pixels changed */
{
    uint32_t x, y, n = 0;
    for (y = y0; y < y0 + h; y++)
        for (x = x0; x < x0 + w; x++)
            n += fb[y * 240u + x] != a[y * 240u + x];
    return n;
}

static void test_popups_keep_their_margins(void)
{
    static uint16_t a[240u * 240u];
    uint32_t i;
    ui_host_init();                                  /* a CLEAR confirm: box x 28..211, y 82..156; its tab x 95..144 */
    trk[0].step[0].on = 1;
    open_step_page();
    ui.force = 1;
    ui_frame();
    memcpy(a, fb, sizeof a);
    ui.confirm = 6;                                  /* (CLEAR ALL LEARNED?: phase C took CLEAR TRACK?) */
    for (i = 0; i < 2; i++)
        ui_input();
    ui.force = 1;
    ui_frame();
    check("confirm: nothing beside its tab, left of it or under its shadow is wiped",
          ui.confirm && fb_diff(a, 28, 76, 66, 6) + fb_diff(a, 146, 76, 66, 6) + fb_diff(a, 27, 76, 1, 84) +
          fb_diff(a, 27, 157, 186, 3) + fb_diff(a, 212, 76, 1, 84) == 0u);
    press(B_OCTDN);
    ui_frame();
    release_all();
    ui_frame();
    ui_host_init();                                  /* TEMPO: box x 30..209, y 76..166; its tab x 95..144 */
    open_tracks();
    ui.force = 1;
    ui_frame();
    memcpy(a, fb, sizeof a);
    sel(1);
    ui_frame();
    check("TEMPO: nothing beside its tab, left of it or under its shadow is wiped",
          ui.bpm_t && fb_diff(a, 30, 70, 64, 6) + fb_diff(a, 146, 70, 64, 6) + fb_diff(a, 29, 70, 1, 98) +
          fb_diff(a, 29, 167, 182, 1) + fb_diff(a, 210, 70, 1, 98) == 0u);
    for (i = 0; i < 45u; i++)
        ui_frame();
}

/* fix 8 (owner, 2026-10-21): the MODEL knob's list runs MD -> MODERN -> CLASSIC (the models keep their numbers: projects
 * store them); each voice with its first preset, wrapping both ways */
static void test_model_list_order(void)
{
    static const char *const WANT[NMODELS] = {
        "BDTRX", "SDTRX", "TMTRX", "CGTRX", "CPTRX", "RSTRX", "CBTRX", "CHTRX", "OHTRX", "CYTRX", "SHTRX", "CLTRX",
        "B2TRX", "BDEFM", "SDEFM", "TMEFM", "CPEFM", "RSEFM", "CBEFM", "HHEFM", "CYEFM",
        "BOOM", "PUNCH", "SNAP", "CRACK", "METAL", "HISS", "SKIN", "FM", "ACID", "SAMPL",
        "K808", "K909", "S808", "S909", "C808", "C909", "HATC", "HATO", "CYMB", "COWB", "TOM", "CONGA", "RIM", "CLAVE"};
    uint32_t i, ok = 1, okb = 1, okp = 1;
    ui_host_init();
    song.sel = 0;
    old_set_model(TSEL, DM_KTRX);
    for (i = 1; i <= NMODELS; i++) {
        model_step(1);
        ok &= str_eq(N_MODEL[TSEL->p[P_MODEL]], WANT[i % NMODELS]);
        okp &= TSEL->pre == preset_first_of((uint32_t)TSEL->p[P_MODEL]) && !TSEL->pre_ed;
        if (!str_eq(N_MODEL[TSEL->p[P_MODEL]], WANT[i % NMODELS]))
            printf("      step %u: %s, want %s\n", i, N_MODEL[TSEL->p[P_MODEL]], WANT[i % NMODELS]);
    }
    for (i = NMODELS; i-- > 0;) {
        model_step(-1);
        okb &= str_eq(N_MODEL[TSEL->p[P_MODEL]], WANT[i]);
    }
    check("MODEL list: MD (BDTRX .. CYEFM), MODERN (BOOM .. SAMPL), CLASSIC (K808 .. CLAVE), CLAVE wraps to BDTRX", ok);
    check("MODEL list: the same order backwards (BDTRX back to CLAVE)", okb);
    check("MODEL list: each voice on its first preset, not edited", okp);
    check("models keep their numbers (projects store them): K808 0, ACID 23, BDTRX 24, CYEFM 44",
          DM_K808 == 0 && DM_ACID == 23 && DM_KTRX == 24 && DM_CYEFM == 44 && NMODELS == 45);
}

/* the renamed engines (owner, fixes 9): the MODEL message, the engine view's title (DMODELS' name) and N_MODEL agree,
 * every name 1..5 characters */
static void test_model_names(void)
{
    static const struct { uint32_t m; const char *s; } C[] = {
        {DM_KTRX, "BDTRX"}, {DM_XTTRX, "TMTRX"}, {DM_MATRX, "SHTRX"}, {DM_XTEFM, "TMEFM"},
        {DM_KBOOM, "BOOM"}, {DM_SCRAK, "CRACK"}, {DM_HNOIS, "HISS"}, {DM_MEMB, "SKIN"}, {DM_SMPL, "SAMPL"}};
    uint32_t i, ok = 1, oks = 1, okl = 1;
    char want[16], wv[16];
    const char *wu;
    for (i = 0; i < NMODELS; i++) {
        size_t n = strlen(N_MODEL[i]);
        okl &= n >= 1u && n <= 5u && str_eq(DMODELS[i].name, N_MODEL[i]);
        if (!str_eq(DMODELS[i].name, N_MODEL[i]))
            printf("      %u: DMODELS %s, N_MODEL %s\n", (unsigned)i, DMODELS[i].name, N_MODEL[i]);
    }
    for (i = 0; i < sizeof C / sizeof C[0]; i++) {
        ui_host_init();
        song.sel = 0;
        old_set_model(TSEL, model_list_next(C[i].m, -1));
        tap(B_EDIT);
        ui_frame();
        turn(EN_K1, 1);                              /* MODEL: the knob popup "MODEL" + the machine */
        ui_frame();
        kp_text(want, wv, &wu);
        ok &= TSEL->p[P_MODEL] == (int16_t)C[i].m && str_eq(want, "MODEL") && str_eq(wv, C[i].s);
        oks &= str_eq(N_MODEL[C[i].m], C[i].s);
        if (!str_eq(wv, C[i].s))
            printf("      %s: popup %s %s\n", C[i].s, want, wv);
    }
    check("renamed engines: the MODEL popup says BDTRX TMTRX SHTRX TMEFM BOOM CRACK HISS SKIN SAMPL", ok);
    check("renamed engines: N_MODEL has the new names", oks);
    check("every engine's name: 1..5 characters, the engine view's title (DMODELS) = N_MODEL", okl);
}


/* ------------------------------------------------------------------ voicegraph (owner 2026-10-22) --- */
/* every engine view as FM's / ACID's / BDTRX's: the name left (F3 4x), "#NN/45" under it, VOICES / CHOKE under
 * that, the graph right (x 100..235); a model with no graph of its own draws its rendered hit (ui_draw.c vg_step): the
 * voice's real sound, offline, on a scratch track, as min / max columns */


static int vg_drawn(uint32_t mi) { return mi != DM_FM && mi != DM_ACID && mi != DM_KTRX; }   /* (the rendered hit's) */

static void vg_settle(void)                          /* the rendered hit to its end: the frames' budgets at once */
{
    uint32_t k;
    for (k = 0; k < 4096u; k++) {
        vg_step(TSEL);
        if (!vg.run)
            break;
    }
}

static void engine_view_ready(void)                  /* a frame (the render starts), the render done, a frame */
{
    ui.force = 1;
    ui_frame();
    vg_settle();
    ui_frame();
}


static void graph_copy(uint16_t *d) { memcpy(d, fb + Y_GRAPH * 240u, 240u * H_GRAPH * sizeof fb[0]); }

/* the same sound draws the same pixels: again after a knob went and came back, after the live state moved on (hits on
 * every track: the metal bank, SKIN's bodies, the voices' ages and seeds), on a forced redraw; another knob value
 * draws another hit */
static void test_vg_deterministic(void)
{
    static const uint8_t M[] = {DM_HATC, DM_CYMB, DM_MEMB, DM_SMPL, DM_CYTRX, DM_KPUNC, DM_CPTRX, DM_SSNAP, DM_HNOIS};
    static uint16_t a[240 * H_GRAPH], b[240 * H_GRAPH], c[240 * H_GRAPH];
    uint32_t i, same = 1, differs = 0;
    for (i = 0; i < sizeof M; i++) {
        const param_desc_t *d = &DMODELS[M[i]].edit[1];
        int16_t old;
        ui_host_init();
        song.sel = 0;
        tap(B_EDIT);
        old_set_model(TSEL, M[i]);
        vg.sig = 0;
        engine_view_ready();
        graph_copy(a);
        old = TSEL->p[P_E1];
        TSEL->p[P_E1] = (int16_t)(old != d->max ? d->max : d->min);
        engine_view_ready();
        graph_copy(b);
        TSEL->p[P_E1] = old;
        engine_view_ready();
        graph_copy(c);
        same &= !memcmp(a, c, sizeof a);
        differs += memcmp(a, b, sizeof a) != 0;
        old_set_model(&trk[1], M[i]);
        old_set_model(&trk[2], DM_HATO);
        drum_hit(&trk[0], 127);
        drum_hit(&trk[1], 90);
        drum_hit(&trk[2], 127);
        render_mix(0, 0, 3000);
        vg.sig = 0;
        engine_view_ready();
        graph_copy(c);
        same &= !memcmp(a, c, sizeof a);
        ui.force = 1;
        ui_frame();
        graph_copy(c);
        same &= !memcmp(a, c, sizeof a);
        if (M[i] == DM_CYMB)
            snap_page("voicegraph/cymb_decay");
    }
    check("rendered hit: the same sound the same pixels (after a knob's round trip, live hits, a forced redraw)", same);
    check("rendered hit: another DECAY another picture", differs == sizeof M);
}

/* the render touches nothing live: every track, the song, the voices' age counter, the block counter, the metal bank,
 * SKIN's bodies, the RNG are the same after a whole render of each model: hits sounding on every track, and again once
 * they have all ended (the metal bank then idle, blocks behind: a render must not make it) */
static void test_vg_state_untouched(void)
{
    static track_t t0[NTRK];
    static song_t s0;
    static memb_body_t mb0[NTRK];
    static int32_t mbuf0[CTL];
    uint32_t i, k, ok = 1, ph0[6], blk0, db0, age0, rng0, j;
    for (i = 0; i < 2u * NMODELS; i++) {
        uint32_t mi = MODEL_LIST[i / 2u];
        ui_host_init();
        song.sel = 0;
        tap(B_EDIT);
        for (k = 1; k < NTRK; k++)
            old_set_model(&trk[k], k == 1u ? DM_MEMB : k == 2u ? mi : trk[k].p[P_MODEL]);
        old_set_model(TSEL, mi);
        for (k = 0; k < NTRK; k++)
            drum_hit(&trk[k], 100);
        render_mix(0, 0, 1000);
        for (j = 0; i % 2u && j < 200u; j++)          /* (all ended: 6 s) */
            render_mix(0, 0, 1323);
        memcpy(t0, trk, sizeof t0);
        memcpy(&s0, &song, sizeof s0);
        memcpy(mb0, memb_body, sizeof mb0);
        memcpy(mbuf0, metal_buf, sizeof mbuf0);
        memcpy(ph0, metal_ph, sizeof ph0);
        blk0 = metal_blk;
        db0 = dblock;
        age0 = dvage;
        rng0 = rng_state;
        vg.sig = 0;
        vg_settle();
        if (memcmp(t0, trk, sizeof t0) || memcmp(&s0, &song, sizeof s0) || memcmp(mb0, memb_body, sizeof mb0) ||
            memcmp(mbuf0, metal_buf, sizeof mbuf0) || memcmp(ph0, metal_ph, sizeof ph0) || blk0 != metal_blk ||
            db0 != dblock || age0 != dvage || rng0 != rng_state) {
            ok = 0;
            printf("      %s: the render changed live state\n", N_MODEL[mi]);
        }
    }
    check("rendered hit: no live state touched (tracks, voices, song, ages, blocks, metal bank, SKIN bodies, RNG)", ok);
}

/* the audio is bit for bit the same with the renders as without: a pattern on 5 tracks (hats, cymbal, SKIN, SAMPL;
 * FREITAKT: 6 tracks, the TRX and EFM metals dropped; sparse: the metal bank idle between) while the selected track (muted, no steps) runs through
 * every model, its engine view rendering each (the renders interleaved with the audio blocks, as the ISR preempts the
 * UI) */
static uint32_t vg_audio_run(int draw, uint32_t *rendered)
{
    static const uint8_t M[NTRK - 1] = {DM_HATC, DM_HATO, DM_CYMB, DM_MEMB, DM_SMPL};
    static const uint16_t STEPS[NTRK - 1] = {0x0101, 0x0010, 0x0001, 0x0404, 0x4040};   /* (bit = step) */
    static int32_t l[256], r[256];
    uint32_t h = 2166136261u, f, k, i, w0;
    ui_host_init();
    for (k = 0; k < NTRK - 1u; k++) {
        old_set_model(&trk[k], M[k]);
        memset(trk[k].step, 0, sizeof trk[k].step);
        for (i = 0; i < 16u; i++)
            trk[k].step[i].on = (STEPS[k] >> i) & 1u;
        trk[k].p[P_MUTE] = 0;
    }
    memset(trk[NTRK - 1].step, 0, sizeof trk[NTRK - 1].step);
    trk[NTRK - 1].p[P_MUTE] = 1;
    song.sel = NTRK - 1;
    tap(B_EDIT);
    vg.sig = 0;
    w0 = vg.work;
    transport_req = 1;
    for (f = 0; f < 700u; f++) {
        if (f % 15u == 0u)
            old_set_model(&trk[NTRK - 1], MODEL_LIST[(f / 15u) % NMODELS]);
        ui_input();
        ui_leds();
        render_mix(l, r, 256);
        for (i = 0; i < 256u; i++)
            h = ((h ^ (uint32_t)l[i]) * 16777619u ^ (uint32_t)r[i]) * 16777619u;
        if (draw)
            ui_draw();
        render_mix(l, r, 256);
        for (i = 0; i < 256u; i++)
            h = ((h ^ (uint32_t)l[i]) * 16777619u ^ (uint32_t)r[i]) * 16777619u;
    }
    *rendered = vg.work - w0;
    return h;
}

static void test_vg_live_audio(void)
{
    uint32_t w1, w0, h1 = vg_audio_run(1, &w1), h0 = vg_audio_run(0, &w0);
    printf("     rendered hit: %u samples rendered beside the pattern (none without the view)\n", w1);
    check("rendered hit: the audio bit for bit the same with the engine view's renders as without", h1 == h0 &&
          w1 > 100000u && w0 == 0u);
}

/* progressive: at most VG_STEP samples a frame, a long voice over several frames, the graph as it was meanwhile (no
 * redraw until the render is done); idle frames render nothing and redraw nothing. The cost of each model's hit
 * (host instructions; the FM-1 at ~6.86 ns each, docs/PERFORMANCE.md) */
static void test_vg_budget(void)
{
    uint32_t f, w, maxw = 0, done = 0, blit_frames = 0, b, i, worst = 0;
    uint64_t i0, d, ins_max = 0, frame_max = 0;
    ui_host_init();
    song.sel = 0;
    tap(B_EDIT);
    old_set_model(TSEL, DM_CYTRX);
    engine_view_ready();
    TSEL->p[P_E1] = (int16_t)(TSEL->p[P_E1] > 64 ? 20 : 120);   /* another DECAY: a new render, the old graph shown */
    vg.sig = 0;
    for (f = 0; f < 200u; f++) {
        w = vg.work;
        b = host_graph_blits;
        ui_frame();
        maxw = vg.work - w > maxw ? vg.work - w : maxw;
        blit_frames += host_graph_blits != b;
        if (!vg.run && vg.ok == vg.sig) {
            done = f + 1u;
            break;
        }
    }
    printf("     rendered hit: CYTRX done in %u frames, %u samples a frame at most, the graph drawn in %u of them\n",
           done, maxw, blit_frames);
    check("rendered hit: progressive, at most VG_STEP samples a frame (a long voice over several frames)",
          maxw <= VG_STEP && done > 3u && done < 60u);
    check("rendered hit: the graph redrawn at the knob's frame and when the render is done, not while it runs",
          blit_frames <= 2u);
    w = vg.work;
    b = host_graph_blits;
    for (f = 0; f < 30u; f++)
        ui_frame();
    check("rendered hit: idle frames render nothing, redraw nothing", vg.work == w && host_graph_blits == b);
    for (i = 0; i < NMODELS; i++) {
        uint32_t mi = MODEL_LIST[i], n;
        if (!vg_drawn(mi))
            continue;
        old_set_model(TSEL, mi);
        vg.sig = 0;
        w = vg.work;
        i0 = instr_now();
        vg_settle();
        d = instr_now() - i0;
        n = vg.work - w;
        printf("     %-5s %4u MS span, %6u samples, %8llu host instructions (%3llu a sample, ~%5.1f ms on the FM-1), "
               "%2u frames\n", N_MODEL[mi], (unsigned)vg.span, n, (unsigned long long)d,
               (unsigned long long)(n ? d / n : 0), (double)d * 6.86e-6, (n + VG_STEP - 1u) / VG_STEP);
        if (d > ins_max) {
            ins_max = d;
            worst = mi;
        }
        if (n && d * VG_STEP / n > frame_max)
            frame_max = d * VG_STEP / n;
    }
    printf("     rendered hit: the dearest hit %s %llu host instructions (~%.1f ms on the FM-1, over the frames); a "
           "frame's share at most ~%llu (~%.2f ms)\n", N_MODEL[worst], (unsigned long long)ins_max, (double)ins_max * 6.86e-6,
           (unsigned long long)frame_max, (double)frame_max * 6.86e-6);
    check("rendered hit: a frame's share of any model's render under 3 ms on the FM-1 (~437000 host instructions)",
          frame_max < 437000u);
}

/* fix 4: TOOLS INIT ALL (a new project) gives the TRX classic kit, each on its first preset, no choke; TRACKS and HOME
 * shown with it (build/ui_shots/fixes/trx_kit_*) */
static void test_init_all_trx_kit(void)
{
    static const uint32_t M[NTRK] = {DM_KTRX, DM_SDTRX, DM_CPTRX, DM_CHTRX, DM_OHTRX, DM_XTTRX, DM_RSTRX, DM_CYTRX};
    uint32_t k, ok = 1;
    ui_host_init();
    init_all();
    for (k = 0; k < NTRK; k++)
        ok &= (uint32_t)trk[k].p[P_MODEL] == M[k] && trk[k].p[P_CHOKE] == 0 &&
              trk[k].pre == preset_first_of(M[k]) && !trk[k].pre_ed;
    check("TOOLS INIT ALL: the TRX classic kit, first presets, no choke", ok);
    ui.force = 1;
    snap_page("fixes/trx_kit_home");
    open_tracks();
    ui.force = 1;
    snap_page("fixes/trx_kit_tracks");
    check("(TRACKS: the kit's models listed)", mix_mode());
}

/* ------------------------------------------------- fixes 2026-10-21 (fix-keys: items 1 2 5 7 9) --- */
/* item 7: PERFORM has its own knobs: while it has them (FX held, the PAGE screen, a layer key held) a turn of KNOB 1..4
 * is a macro's, never the page's under it, even when the detents come in between the pass's two reads (the scan ISR
 * runs while ui_input does: before, the page's loop took the late ones); SELECT turns no page under it */
static void test_fk_perform_own_knobs(void)
{
    int16_t e1, e0;
    uint32_t pg, r;
    ui_host_init();
    e1 = trk[0].p[P_E1];
    press(B_FX);
    ui_frame();
    host_ticks += 300u * 1000u * FM1_TICKS_PER_US;
    turn(EN_K2, 2);
    host_enc_late[panel.enc[EN_K2]] = 3 * panel.dir[EN_K2];   /* 3 more detents after fx_layer's read */
    fx_hold_frames(1);
    check("PERFORM (FX held, HOME under it): late KNOB 2 detents in the same pass leave DECAY alone",
          trk[0].p[P_E1] == e1 && perf_k[1] > 0);
    fx_hold_frames(1);
    check("..and are CRUSH's on the next pass (none lost)", perf_k[1] == 5);
    release_all();
    ui_frame();
    perf_k[1] = 0;
    tap(B_FX);                                       /* FILTER 1/2: KNOB 1 TYPE */
    e0 = TSEL->p[P_FTYPE];
    pg = ui.page;
    settings.perfpage = 1;                           /* PAGE: the screen stays after FX */
    press(B_FX);
    fx_hold_frames(500);
    release_all();
    ui_frame();
    for (r = 0; r < 4u; r++) {
        host_ticks += 300u * 1000u * FM1_TICKS_PER_US;   /* (slow turns: no acceleration) */
        turn(EN_K1, 1);
        host_enc_late[panel.enc[EN_K1]] = 2 * panel.dir[EN_K1];
        ui_frame();
    }
    sel(1);
    check("PERFORM PAGE over FILTER: KNOB 1 is FILTER's macro only (TYPE untouched), SELECT turns no page",
          ui.pg_open && ui.layer && TSEL->p[P_FTYPE] == e0 && ui.page == pg && perf_k[0] == 12);
    tap(B_HOME);
    settings.perfpage = 0;
    perf_k[0] = 0;
    ui_frame();
}

/* review of fix-keys (item 7: PERFORM is its own page, never the page under it; item 1: OCT+ alone, the next bank on
 * its release): OCT+ let go under the clear's warning (REC + SEL; TOOLS before phase C) or PERFORM, or held across
 * either, moves no bank under it */
static void test_fk_oct_under_mute_perform(void)
{
    uint32_t octb = 1u << panel.btn[B_OCTUP], glob = 1u << panel.btn[B_REC] | 1u << panel.btn[B_SCL];
    int was;
    ui_host_init();
    trk[0].p[P_SLEN] = 32;                           /* two banks */
    open_step_page();
    press(B_OCTUP);
    ui_frame();
    press(B_REC);
    press(B_SCL);
    ui_frame();
    host_ticks += 500u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    was = ui.clr;
    fm1_in.buttons &= ~octb;                         /* OCT+ let go under the clear's warning */
    ui_frame();
    check("OCT+ held, REC + SEL (the clear's warning), OCT+ let go under it: no bank change", was && ui.bank == 0u);
    release_all();
    ui_frame();
    check("..REC + SEL let go: the STEP page, bank 1", !ui.clr && step_keys() && ui.bank == 0u);
    press(B_OCTUP);
    ui_frame();
    press(B_REC);
    press(B_SCL);
    ui_frame();
    host_ticks += 500u * 1000u * FM1_TICKS_PER_US;
    ui_frame();
    was = ui.clr;
    fm1_in.buttons &= ~glob;                         /* the warning gone (cancelled), OCT+ still held */
    ui_frame();
    release_all();                                   /* OCT+ let go on the steps */
    ui_frame();
    check("OCT+ held across the clear's warning, let go on the steps: no bank change", was && ui.bank == 0u);
    press(B_OCTUP);
    ui_frame();
    press(B_FX);
    fx_hold_frames(500);
    was = ui.layer;
    fm1_in.buttons &= ~octb;                         /* OCT+ let go under PERFORM */
    fx_hold_frames(1);
    check("OCT+ held, FX held into PERFORM, OCT+ let go under it: no bank change", was && ui.bank == 0u);
    release_all();
    ui_frame();
    press(B_FX);
    fx_hold_frames(500);
    press(B_OCTUP);                                  /* OCT+ tapped under PERFORM */
    fx_hold_frames(1);
    fm1_in.buttons &= ~octb;
    fx_hold_frames(1);
    check("OCT+ tapped under PERFORM: no bank change", ui.layer && ui.bank == 0u);
    release_all();
    ui_frame();
    tap(B_OCTUP);
    check("(OCT+ tapped alone on the steps: still the next bank)", step_keys() && ui.bank == 1u);
}

/* FX let go while KNOB 2 turns: the detents read on the pass FX is let go are PERFORM's (CRUSH), never HOME's DECAY */
static void test_fk_fx_release_knobs(void)
{
    int16_t e1;
    ui_host_init();
    e1 = trk[0].p[P_E1];
    press(B_FX);
    fx_hold_frames(500);
    check("(FX held: PERFORM up over HOME)", ui.layer && ui.home);
    host_ticks += 300u * 1000u * FM1_TICKS_PER_US;
    release_all();                                   /* FX let go .. */
    turn(EN_K2, 2);                                  /* .. while KNOB 2 turns */
    host_enc_late[panel.enc[EN_K2]] = 3 * panel.dir[EN_K2];
    ui_frame();
    check("FX let go during a KNOB 2 turn: HOME's DECAY untouched on that pass", ui.home && trk[0].p[P_E1] == e1);
    ui_frame();
    perf_k[1] = 0;
}

/* PRESETS and ALGORITHM turn nothing under PERFORM (FX held, the PAGE screen): no preset loaded, no track change */
static void test_fk_perform_preset_algo(void)
{
    uint16_t pre;
    int16_t p[P_COUNT];
    uint32_t sel0;
    ui_host_init();
    pre = TSEL->pre;
    sel0 = song.sel;
    memcpy(p, TSEL->p, sizeof p);
    press(B_FX);
    fx_hold_frames(500);
    turn(EN_PRESET, 1);
    turn(EN_ALGO, 1);
    fx_hold_frames(1);
    check("PERFORM (FX held): PRESETS loads no preset, ALGORITHM keeps the track",
          ui.layer && song.sel == sel0 && TSEL->pre == pre && !memcmp(p, TSEL->p, sizeof p) && !ui.pr.pop);
    release_all();
    ui_frame();
    tap(B_FX);
    settings.perfpage = 1;                           /* PAGE: the screen stays after FX */
    press(B_FX);
    fx_hold_frames(500);
    release_all();
    ui_frame();
    turn(EN_PRESET, -1);
    turn(EN_ALGO, 1);
    ui_frame();
    check("PERFORM PAGE: PRESETS loads no preset, ALGORITHM keeps the track",
          ui.pg_open && song.sel == sel0 && TSEL->pre == pre && !memcmp(p, TSEL->p, sizeof p) && !ui.pr.pop);
    tap(B_HOME);
    settings.perfpage = 0;
    ui_frame();
}

/* item 2: the HOME screen and the EDIT (SOUND), FX and LFO pages show the selected track on the keys: its
 * white key (KEY_TRK_KEY) lit, the other keys dark unless held; it follows ALGORITHM and a key's select; KEYS (the
 * keyboard), COMP (DUCK), the step pages and GLOBAL / SAVE keep their own; the menu none */
static int fk_keys_show_only(uint32_t mask)          /* the 27 key LEDs: lit exactly where mask says */
{
    uint32_t k, ok = 1;
    for (k = 0; k < 27u; k++)
        ok &= led_on(14u + k) == (int)((mask >> k) & 1u);
    return (int)ok;
}
static void test_fk_track_keys(void)
{
    uint32_t i, n = 0, ok = 1, other = 1, fams = 0;
    ui_host_init();
    for (i = 0; i <= NPAGES; i++) {                  /* (NPAGES: the HOME screen) */
        uint32_t fam;
        ui.home = i == NPAGES;
        ui.hmode = ui.home;
        if (i < NPAGES)
            ui.page = (uint8_t)i;
        page_entered();
        song.sel = (uint8_t)(i % NTRK);
        ui_frame();
        fam = cur_fam();
        if ((ui.home || fam == FAM_SND || fam == FAM_FX || fam == FAM_LFO) && !comp_mode()) {   /* (COMP: FX's, DUCK) */
            ok &= fk_keys_show_only(1u << KEY_TRK_KEY[song.sel]);
            fams |= 1u << fam;
            n++;
        } else if (!step_keys() && !comp_mode()) {
            other &= fk_keys_show_only(0);           /* GLOBAL SAVE: as before, the keys held only */
        }
    }
    check("item 2: HOME, EDIT (SOUND), FX, LFO: the selected track's key lit, every other key dark",
          ok && n > 10u && fams == (1u << FAM_HOME | 1u << FAM_SND | 1u << FAM_FX | 1u << FAM_LFO));
    check("item 2: GLOBAL, SAVE: no track key (the keys held only, as before)", other);
    tap(B_FX);
    song.sel = 0;
    turn(EN_ALGO, 2);
    ui_frame();
    check("item 2: FX, ALGORITHM to track 2: its key (G3) lit, F3 dark", song.sel == 1u &&
          fk_keys_show_only(1u << KEY_TRK_KEY[1]));
    tap(B_EDIT);
    keys(1u << KEY_TRK_KEY[4]);                      /* C4: track 5, selected by its key */
    ui_frame();
    check("item 2: EDIT, track 5's key pressed: selected, lit (held)", song.sel == 4u &&
          fk_keys_show_only(1u << KEY_TRK_KEY[4]));
    keys(1u << KEY_TRK_KEY[4] | 1u << 1);            /* a black key held too: lit while held */
    ui_frame();
    check("item 2: a key held is lit beside the track's", fk_keys_show_only(1u << KEY_TRK_KEY[4] | 1u << 1));
    keys(0);
    ui_frame();
    check("item 2: let go: track 5's key stays lit", fk_keys_show_only(1u << KEY_TRK_KEY[4]));
    tap(B_SCL);                                      /* KEYS: the keyboard's picture (held keys only) */
    check("item 2: KEYS on: no track key (the keyboard)", fk_keys_show_only(0));
    tap(B_SCL);
    tap(B_LFO);
    check("item 2: LFO: track 5's key", fk_keys_show_only(1u << KEY_TRK_KEY[4]));
    ui.menu = 1;
    ui.force = 1;
    ui_frame();
    check("item 2: the menu over it: no track key", fk_keys_show_only(0));
    ui.menu = 0;
    ui_frame();
}

/* item 5: OCT- / OCT+ breathe (upstream Felucca's #119, led_dim.c) where pressing them would do something, a way back
 * lit: the steps (a bank that way; OCT+ lit while held, its accent), a dialog
 * (NO lit, YES breathing), the menu (BACK lit, OK breathing; USB MIDI: back), NAME, MIDI LEARN, the PRESETS list (LOAD;
 * DELETE on a used user slot); KEYS: lit off octave 0 as before; HOME, GLOBAL: dark */
static uint32_t fk_oct(void)                         /* 1 OCT- lit, 2 OCT+ lit, 4 OCT+ breathing, 8 OCT- breathing */
{
    return (uint32_t)led_on(panel.btn[B_OCTDN]) | (uint32_t)led_on(panel.btn[B_OCTUP]) << 1 |
           (uint32_t)led_breathes(panel.btn[B_OCTUP]) << 2 | (uint32_t)led_breathes(panel.btn[B_OCTDN]) << 3;
}
static void test_fk_oct_breath(void)
{
    uint32_t k;
    ui_host_init();
    ui_frame();
    check("item 5: the HOME screen: OCT- / OCT+ dark, the hook idle (BACKLIGHT OFF)", fk_oct() == 0u && !led_dim_on);
    trk[0].p[P_SLEN] = 48;
    seq_open("PATTERN");
    ui_frame();
    check("item 5: STEP, bank 1 of 3: OCT+ breathes (a bank after), OCT- dark", fk_oct() == 4u && led_br_on);
    tap(B_OCTUP);
    check("item 5: bank 2 of 3: both breathe", ui.bank == 1u && fk_oct() == 12u);
    tap(B_OCTUP);
    check("item 5: bank 3 of 3: OCT- breathes, OCT+ dark", ui.bank == 2u && fk_oct() == 8u);
    press(B_OCTUP);
    ui_frame();
    check("item 5: OCT+ held (the accent): lit", fk_oct() == (2u | 8u));
    release_all();
    ui_frame();
    tap(B_OCTDN);
    tap(B_OCTDN);
    trk[0].p[P_SLEN] = 16;
    tap(B_SCL);                                      /* KEYS: the octave, lit off 0 */
    check("item 5: KEYS at octave 0: dark", fk_oct() == 0u);
    tap(B_OCTUP);
    check("item 5: KEYS at octave +1: OCT+ lit (not breathing)", song.octave == 1 && fk_oct() == 2u);
    tap(B_OCTDN);
    tap(B_SCL);
    tap(B_EDIT);
    check("item 5: SOUND: dark (no LAYER pages since FREITAKT)", fk_oct() == 0u);
    tap(B_GLO);
    check("item 5: GLOBAL: dark", fk_oct() == 0u);
    ui.confirm = 1;
    ui.confirm_trk = 0;
    ui.force = 1;
    ui_frame();
    check("item 5: a dialog: OCT- (NO) lit, OCT+ (YES) breathes", fk_oct() == 5u);
    tap(B_OCTDN);
    for (k = 0; k < 1000u && !ui.menu; k++) {         /* HOME held: the menu */
        fm1_in.buttons |= 1u << panel.btn[B_HOME];
        ui_frame();
    }
    release_all();
    ui_frame();
    check("item 5: the menu: OCT- (BACK) lit, OCT+ (OK) breathes", ui.menu == 1u && fk_oct() == 5u);
    ui.menu_sel = MI_USBMIDI;
    ui_frame();
    check("item 5: the menu on USB MIDI (shown only): OCT- (BACK) lit alone", ui.menu == 1u && fk_oct() == 1u);
    menu_close();
    ui_frame();
    ui.ml.on = 1;
    ui_frame();
    check("item 5: MIDI LEARN: OCT- (CLEAR) lit, OCT+ (DONE) breathes", fk_oct() == 5u);
    ui.ml.on = 0;
    ui_frame();
    project_save(0);
    name_open(NK_RENAME, 0);
    ui_frame();
    check("item 5: NAME: OCT- (CANCEL) lit, OCT+ (RENAME) breathes", name_on() && fk_oct() == 5u);
    name_close();
    ui_frame();
    users_clear();
    tap(B_EDIT);
    hold(B_EDIT);
    check("item 5: the PRESETS list on a factory preset: OCT+ (LOAD) breathes, OCT- (DELETE) dark", pl_on() &&
          fk_oct() == 4u);
    upr.s[0].used = 1u;
    upr.s[0].model = 0;
    ui.pr.cur = PRE_NFACT;                           /* a used user slot under the cursor */
    ui_frame();
    check("item 5: .. on a used user slot: OCT- (DELETE) breathes too", fk_oct() == 12u);
    tap(B_EDIT);
    users_clear();
    ui_frame();
}

/* item 9: BACKLIGHT's levels (upstream Felucca's LEDS: OFF DIM LO DIM HI INV, 1.5.6's INV LO INV HI) on STEP with
 * steps 2 and 5 on: the layers each mode gives the keys and the buttons, and the frames of 20 the hook lights them in */
static uint32_t fk_layer(uint32_t id)                /* 0 none, 1 full, 2 mid, 3 glow, 4 breath */
{
    uint8_t q = led_pos[id];
    uint32_t lay, bit;
    if (q == 0xFF || !led_dim_on)
        return q != 0xFF && ((fm1_led[q >> 3] >> (q & 7u)) & 1u);
    bit = 1u << (q & 7u);
    lay = led_lay[q >> 3];
    return lay & bit ? 1u : (lay >> 8) & bit ? 2u : (lay >> 16) & bit ? 3u : (lay >> 24) & bit ? 4u : 0u;
}
static uint32_t fk_frames(uint32_t ph)               /* frames of 20 in a phase mask */
{
    uint32_t f, n = 0;
    for (f = 0; f < 20u; f++)
        n += (ph >> f) & 1u;
    return n;
}
static void test_fk_led_levels(void)
{
    static const char *const WHAT[BL_COUNT] = {
        "OFF: on steps full, the rest dark",
        "DIM LO (the old ON): on steps full, the unlit beats mid (5 of 20 frames), the other keys and the idle buttons "
        "glow (a short pulse every frame)",
        "DIM HI: as DIM LO, brighter: the beats 6 of 20, the glow's pulse longer",
        "INV: the active ones dark, the idle keys and buttons lit",
        "INV LO: the active ones dark, the idle ones mid (2 of 20)",
        "INV HI: the active ones dark, the idle ones mid (5 of 20)"};
    static const uint32_t MID_N[BL_COUNT] = {0, 5, 6, 0, 2, 5}, GLOW_W[BL_COUNT] = {0, LED_GLOW_LO, LED_GLOW_HI, 0, 0, 0};
    uint32_t m;
    ui_host_init();
    trk[0].p[P_SLEN] = 32;
    trk[0].step[1].on = trk[0].step[4].on = 1;       /* step 2; step 5 a beat */
    seq_open("PATTERN");
    for (m = 0; m < BL_COUNT; m++) {
        uint32_t on = 1u, idle = m == BL_OFF ? 0u : m == BL_INV ? 1u : m >= BL_INV_LO ? 2u : 3u, ok;
        backlight = m;
        ui_frame();
        if (m == BL_INV || m >= BL_INV_LO)
            on = 0u;
        ok = fk_layer(14u + STEP_KEY[1]) == on && fk_layer(14u + STEP_KEY[4]) == on &&
             fk_layer(14u + STEP_KEY[2]) == idle && fk_layer(14u + 3u) == idle &&   /* an off step, a black key */
             fk_layer(14u + 1u) == on &&     /* (black 1: the selected track) */
             fk_layer(14u + STEP_KEY[0]) == (m == BL_DIM_LO || m == BL_DIM_HI ? 2u : idle) &&   /* an unlit beat */
             fk_layer(panel.btn[B_SEQ]) == on && fk_layer(panel.btn[B_SAVE]) == idle &&
             fk_layer(panel.btn[B_OCTUP]) == 4u &&   /* breathing in every mode */
             fk_frames(led_mid_ph) == MID_N[m] && led_glow_w == GLOW_W[m];
        check(WHAT[m], ok);
    }
    backlight = BL_OFF;
    trk[0].p[P_SLEN] = 16;
    ui_frame();
}

/* ---- ENV: the white keys as the selected track's steps on every page (STEPS), or as today (TRACKS) (spec
 * 2026-10-24 parameter locks) */
static void btn_up(uint32_t label) { fm1_in.buttons &= ~(1u << panel.btn[label]); }
static void env_tap(void) { tap(B_ENV); }
static void key_tap(uint32_t key) { grid_down(key); grid_wait(50); grid_up(key); }
static void test_env_toggle(void)
{
    uint32_t ok;
    ui_host_init();
    tap(B_EDIT);                                     /* SOUND */
    song.sel = 0;
    memset(trk[0].step, 0, sizeof trk[0].step);
    memset(trk[2].step, 0, sizeof trk[2].step);
    env_tap();
    ok = ui.env == 1u && led_lit(panel.btn[B_ENV]) && song.seq_mode == 1u;
    key_tap(WHITE[2]);
    ok &= trk[0].step[2].on && song.sel == 0u && !trk[2].step[2].on && str_eq(cur_page()->title, "SOUND");
    check("ENV: STEPS mode (LED lit): on SOUND a white key toggles the selected track's step, selects no track, plays "
          "nothing", ok);
    env_tap();
    ok = ui.env == 0u && !led_lit(panel.btn[B_ENV]) && song.seq_mode == 0u;
    key_tap(WHITE[2]);
    check("ENV again: TRACKS mode: the key selects its track as before", ok && song.sel == 2u && trk[0].step[2].on);
}
static void test_env_tracks_unchanged(void)
{
    static const uint32_t FAMS[5] = {FAM_SND, FAM_FX, FAM_LFO, FAM_SEQ, FAM_MIX};
    uint32_t f, ok = 1, seqm, sel;
    for (f = 0; f < 5u; f++) {
        uint32_t lv[2];
        ui_host_init();
        open_family(FAMS[f]);
        ui_frame();
        seqm = song.seq_mode;
        lv[0] = led_lit(panel.btn[B_OCTUP]) | led_lit(14u + WHITE[0]) << 1 | led_lit(14u + WHITE[3]) << 2;
        env_tap();
        env_tap();
        lv[1] = led_lit(panel.btn[B_OCTUP]) | led_lit(14u + WHITE[0]) << 1 | led_lit(14u + WHITE[3]) << 2;
        ok &= ui.env == 0u && song.seq_mode == seqm && lv[0] == lv[1];
        sel = song.sel;
        key_tap(WHITE[3]);
        ok &= FAMS[f] == FAM_SEQ || FAMS[f] == FAM_MIX ? trk[sel].step[3].on : song.sel == 3u;
    }
    check("ENV toggled twice: every family's keys, LEDs and key mode as before", ok);
}
static void test_env_banks(void)
{
    uint32_t ok;
    ui_host_init();
    tap(B_EDIT);
    TSEL->p[P_SLEN] = 64;
    env_tap();
    press(B_ENV);                                    /* ENV held + OCT+: the next bank */
    ui_frame();
    press(B_OCTUP);
    ui_frame();
    btn_up(B_OCTUP);
    ui_frame();
    ok = ui.bank == 1u && str_eq(cur_page()->title, "SOUND");
    btn_up(B_ENV);
    ui_frame();
    ok &= ui.env == 1u;
    press(B_ENV);
    ui_frame();
    press(B_OCTDN);
    ui_frame();
    release_all();
    ui_frame();
    ok &= ui.bank == 0u && ui.env == 1u;
    check("ENV held + OCT+ / OCT-: the next / previous bank; ENV let go after them: still STEPS", ok);
    env_tap();
    press(B_ENV);
    ui_frame();
    btn_up(B_ENV);
    ui_frame();
    check("(ENV tapped back to STEPS)", ui.env == 1u);
    env_tap();
    tap(B_OCTUP);
    check("ENV off on SOUND: OCT+ no bank (and no page job: no LAYER)", ui.bank == 0u && str_eq(cur_page()->title, "SOUND"));
}
static void test_env_oct_jobs(void)
{
    uint32_t ok;
    int16_t m;
    ui_host_init();
    tap(B_EDIT);
    TSEL->p[P_SLEN] = 32;
    env_tap();
    press(B_OCTUP);
    ui_frame();
    ok = str_eq(cur_page()->title, "SOUND") && led_lit(panel.btn[B_OCTUP]);
    release_all();
    ui_frame();
    ok &= str_eq(cur_page()->title, "SOUND") && ui.bank == 0u;
    check("STEPS on SOUND: OCT+ alone does nothing on its release (no bank, no LAYER)", ok);
    tap(B_OCTDN);
    check("STEPS on SOUND: OCT- does nothing either", str_eq(cur_page()->title, "SOUND") && ui.bank == 0u);
    TSEL->step[5].acc = 0;
    TSEL->step[5].on = 1;
    press(B_OCTUP);
    ui_frame();
    grid_down(WHITE[5]);
    btn_up(B_OCTUP);
    ui_frame();
    grid_up(WHITE[5]);
    check("STEPS: OCT+ held + a step key: its accent, OCT+'s release no page job",
          TSEL->step[5].acc && TSEL->step[5].on && str_eq(cur_page()->title, "SOUND"));
    tap(B_LFO);
    ui.lfo_l = 0;
    m = TSEL->p[P_LFO1 + LF_MODE];
    tap(B_OCTUP);
    check("STEPS on an LFO page: OCT+ alone cycles the RATE MODE", TSEL->p[P_LFO1 + LF_MODE] == (m + 1) % 3);
    open_step_page();
    ui_frame();
    tap(B_OCTUP);
    check("STEPS on SEQ STEP: OCT+ alone is the next bank, as today", ui.bank == 1u && ui.env == 1u);
}
static void test_env_keys_exclusive(void)
{
    ui_host_init();
    tap(B_EDIT);
    tap(B_SCL);
    check("(KEYS on)", ui.keys == 1u);
    env_tap();
    check("ENV with KEYS on: KEYS off, STEPS on", ui.keys == 0u && ui.env == 1u && song.seq_mode == 1u);
    tap(B_SCL);
    check("SCL in STEPS: STEPS off, KEYS on", ui.keys == 1u && ui.env == 0u && song.seq_mode == 3u);
}
static void test_env_not_saved(void)
{
    static uint8_t a[sizeof settings];
    ui_host_init();
    memcpy(a, &settings, sizeof a);
    env_tap();
    check("ENV: not in the settings (not saved), off at power-on",
          ui.env == 1u && !memcmp(a, &settings, sizeof a) && (ui_host_init(), ui.env == 0u));
}
static void test_env_home_and_tracks(void)
{
    uint32_t ok, k;
    ui_host_init();                                  /* the HOME screen */
    song.sel = 1;
    for (k = 0; k < NTRK; k++)
        memset(trk[k].step, 0, sizeof trk[k].step);
    env_tap();
    key_tap(WHITE[6]);
    ok = ui.home && trk[1].step[6].on && song.sel == 1u;
    home_tracks();
    song.rec = 1u << 1;                              /* TRACKS, track 2 armed */
    ui_frame();
    key_tap(WHITE[7]);
    ok &= trk[1].step[7].on && song.seq_mode == 1u;
    check("STEPS: the HOME screen and TRACKS (armed) keys are the selected track's steps", ok);
    song.rec = 0;
    tap(B_EDIT);
    transport_req = 1;
    ui_frame();
    tap(B_REC);                                      /* off TRACKS: REC ALL */
    ok = ui.rec_all && song.playing;
    key_tap(WHITE[9]);
    key_tap(WHITE[10]);
    for (k = 0; k < NTRK; k++)
        if (k != 1u)
            ok &= !trk[k].step[0].on && !trk[k].step[4].on && !trk[k].step[8].on && !trk[k].step[12].on;
    ok &= trk[1].step[9].on && trk[1].step[10].on;
    transport_req = 2;
    ui_frame();
    check("STEPS + REC ALL playing: the keys toggle steps, record no hits", ok);
    rec_all_drop();
    song.rec = 0;
}
/* Review Focus 5: ENV while a step key is held: the hold drops, its release toggles nothing, no lock after */
static void test_env_while_step_held(void)
{
    uint32_t ok;
    int16_t v;
    const page_t *pg;
    int16_t *vp;
    ui_host_init();
    tap(B_EDIT);
    song.sel = 0;
    memset(trk[0].step, 0, sizeof trk[0].step);
    env_tap();
    grid_down(WHITE[4]);
    grid_wait(400);
    ok = trk[0].step[4].on;                          /* an off step held 0.3 s: on */
    press(B_ENV);
    ui_frame();
    btn_up(B_ENV);
    ui_frame();
    ok &= ui.env == 0u;
    grid_up(WHITE[4]);
    ok &= trk[0].step[4].on;
    pg = cur_page();
    page_desc(pg, 1, &vp);
    v = *vp;
    host_ticks += 300u * 1000u * FM1_TICKS_PER_US;
    turn(EN_K2, 1);
    ui_frame();
    ok &= *vp == v + 1 && lock_count(0) == 0u && mo.b.count == 0u;
    check("ENV while a step key is held: TRACKS mode, the release leaves the step on, a turn edits the knob (no lock)", ok);
}

/* ---- parameter locks: hold a step in STEPS and turn a knob (spec 2026-10-24) */
static void lock_fresh(void)                         /* STEPS on, track 1, its steps off, an empty bank */
{
    ui_host_init();
    song.sel = 0;
    memset(trk[0].step, 0, sizeof trk[0].step);
    memset(&mo.b, 0, sizeof mo.b);
    env_tap();
}
static void open_page_at(uint32_t i)
{
    ui.page = (uint8_t)i;
    ui.home = 0;
    page_entered();
    ui_frame();
}
/* step 5 held 0.35 s, KNOB c+1 one detent right, let go; the knob's id here (P_COUNT none) */
static uint32_t held_turn(uint32_t c, int32_t dir)
{
    grid_down(WHITE[4]);
    grid_wait(350);
    host_ticks += 300u * 1000u * FM1_TICKS_PER_US;
    turn(EN_K1 + c, dir);
    ui_frame();
    return lock_id(c);
}
static int lock_page_ok(const char *what)            /* every knob of this screen, step 5 held */
{
    uint32_t c, ok = 1;
    for (c = 0; c < 4u; c++) {
        static int16_t before[P_COUNT], base[P_COUNT];
        uint32_t id, nlocks;
        int16_t lv = 0, want;
        int16_t *vp = 0;
        const param_desc_t *d;
        if (ui.home)
            d = home_param(c, &vp);
        else if (cur_page()->scope == SC_MIX)
            d = c == 1u ? &TP[P_LEVEL] : c == 3u ? &TP[P_PAN] : 0, vp = c == 1u ? &TSEL->p[P_LEVEL] : c == 3u ? &TSEL->p[P_PAN] : 0;
        else
            d = page_desc(cur_page(), c, &vp);
        memcpy(before, trk[0].p, sizeof before);
        memcpy(base, mo.base[0], sizeof base);
        nlocks = mo.b.count;
        id = held_turn(c, 1);
        grid_up(WHITE[4]);
        if (id < P_COUNT) {
            want = (int16_t)(id == P_MODEL ? model_list_next((uint32_t)*vp, 1) :   /* MODEL: the next model (a machine lock) */
                             vp && *vp < d->max ? *vp + 1 : vp ? *vp : 0);
            ok &= lock_get(0, 4, id, &lv) && lv == want && mo.b.count == nlocks + 1u &&
                  !memcmp(before, trk[0].p, sizeof before) && (motion_param(id) || id == P_MODEL) && trk[0].step[4].on;
            if (!(lock_get(0, 4, id, &lv) && lv == want))
                printf("     %s KNOB %u (id %u): lock %d, want %d\n", what, c + 1u, id, lock_get(0, 4, id, &lv) ? lv : -999, want);
        } else {
            ok &= mo.b.count == nlocks && !memcmp(before, trk[0].p, sizeof before) &&
                  !(vp && vp >= TSEL->p && vp < TSEL->p + P_COUNT && lock_param((uint32_t)(vp - TSEL->p)) && d &&
                    d->label && d->label[0] != '-' && d->max > d->min);
            if (mo.b.count != nlocks || memcmp(before, trk[0].p, sizeof before))
                printf("     %s KNOB %u: not lockable, but changed\n", what, c + 1u);
        }
        (void)base;
        memset(&mo.b, 0, sizeof mo.b);
        memset(trk[0].step, 0, sizeof trk[0].step);
    }
    return (int)ok;
}
static void test_lock_entry_every_page(void)
{
    uint32_t i, l, ok = 1, pages = 0;
    char b[96];
    lock_fresh();
    for (i = 0; i < NPAGES; i++) {
        uint32_t f = PAGES[i].fam;
        if (PAGES[i].scope != SC_TRACK || (f != FAM_SND && f != FAM_FX && f != FAM_LFO) ||
            page_hidden(i))
            continue;
        for (l = 0; l < (PAGES[i].graph == GR_LDEST ? 4u : 1u); l++) {
            song.lslot = (uint8_t)l;
            open_page_at(i);
            snprintf(b, sizeof b, "%s %u (slot %u)", PAGES[i].title, i, l);
            ok &= (uint32_t)lock_page_ok(b);
            pages++;
        }
    }
    song.lslot = 0;
    home_screen();
    ui_frame();
    ok &= (uint32_t)lock_page_ok("HOME");
    home_tracks();
    ui_frame();
    ok &= (uint32_t)lock_page_ok("TRACKS");
    check("locks: on every SOUND, FX, INSERT, FILTER, LFO, LFO DEST page, HOME and TRACKS a held step + "
          "a recordable knob writes a lock (its own value + 1; MODEL the next model, a machine lock), the knob and the "
          "base untouched; TYPE, WAVE, DEST, SRC and the rest change nothing", ok && pages > 12u);
}
static void test_lock_before_hold(void)
{
    int16_t v = 0;
    uint32_t ok;
    lock_fresh();
    tap(B_EDIT);
    grid_down(WHITE[4]);
    grid_wait(100);
    host_ticks += 300u * 1000u * FM1_TICKS_PER_US;
    turn(EN_K2, 1);
    ui_frame();
    ok = lock_get(0, 4, lock_id(1), &v) && trk[0].step[4].on;
    grid_up(WHITE[4]);
    check("locks: a turn 0.1 s after the press writes the lock, turns the off step on; the release toggles nothing",
          ok && trk[0].step[4].on);
}
static void test_lock_cells(void)
{
    char want[16], val[12], m[16];
    const char *unit;
    uint32_t ok, id;
    int16_t lv = 0;
    lock_fresh();
    tap(B_EDIT);
    id = held_turn(1, 1);                            /* KNOB 2 locked on step 5 */
    ok = lock_get(0, 4, id, &lv);
    grid_up(WHITE[4]);
    grid_wait(3200);                                 /* (the touched mark gone: HOT_MS) */
    grid_down(WHITE[4]);
    grid_wait(350);
    param_format(track_desc(TSEL, id), lv, val, &unit);
    snprintf(want, sizeof want, "|%sB", val);
    ok &= strstr(ui.col[1], want) != 0;
    param_format(track_desc(TSEL, lock_id(2)), TSEL->p[lock_id(2)], val, &unit);
    snprintf(want, sizeof want, "|%sA", val);
    ok &= strstr(ui.col[2], want) != 0;
    ok &= head_mode(m) && str_eq(m, "LOCK STEP 5");
    snap_page("locks/sound_held");
    grid_up(WHITE[4]);
    param_format(track_desc(TSEL, id), TSEL->p[id], val, &unit);
    snprintf(want, sizeof want, "|%sA", val);
    ok &= strstr(ui.col[1], want) != 0 && !head_mode(m);
    check("locks: step held: a locked cell shows the lock inverted, the others their value; LOCK STEP 5; let go: the "
          "knobs' values", ok);
}
/* while playing (Model:Cycles: locks are entered with the pattern running): the held step + a knob locks it, the
 * knob popup shows the lock, and the step plays it on the next pass (the knob's own value on the others) */
static void test_lock_while_playing(void)
{
    char want[16], val[12];
    const char *unit;
    uint32_t ok, id, k;
    int16_t lv = 0, own;
    lock_fresh();
    tap(B_EDIT);
    for (k = 0; k < 16u; k++)
        trk[0].step[k].on = 1;
    trk[0].p[P_SLEN] = 16;
    transport_req = 1;
    seq_play_to(0, 1);
    id = lock_id(1);
    own = trk[0].p[id];
    grid_down(WHITE[4]);
    grid_wait(350);
    turn(EN_K2, 5);
    ui_frame();
    ok = song.playing && id < P_COUNT && lock_get(0, 4, id, &lv) && lv != own;
    param_format(track_desc(TSEL, id), lv, val, &unit);
    {                                                  /* the knob popup: the lock (inverted) */
        char l[16], v[16];
        kp_text(l, v, &unit);
        ok &= kp.on && kp.kind == KP_LOCK && kp.st == 4u && strncmp(v, val, strlen(v)) == 0;
    }
    (void)want;
    grid_up(WHITE[4]);
    seq_play_to(0, 4);
    ok &= trk[0].seq_idx == 4u && trk[0].p[id] == lv;  /* step 5 plays its lock */
    seq_play_to(0, 5);
    ok &= trk[0].p[id] == own;                         /* the next step the knob's own value */
    transport_req = 2;
    ui_frame();
    check("locks while playing: a held step + KNOB 2 locks it, the popup shows the lock, step 5 plays it and "
          "step 6 the knob's own value", ok);
}
static void test_lock_full(void)
{
    uint32_t i;
    lock_fresh();
    tap(B_EDIT);
    for (i = 0; mo.b.count < LK_MAX; i++)
        motion_add(1u + i / 512u, i % 64u, P_E0 + (i / 64u) % 8u, 1);
    held_turn(1, 1);
    grid_up(WHITE[4]);
    check("locks: a held turn with 1024 events: LOCKS FULL, nothing added", str_eq(ui.msg, "LOCKS FULL") &&
          mo.b.count == LK_MAX && lock_count(0) == 0u);
}
static void oct_dn_for(uint32_t ms)                  /* OCT- held ms, let go (the step key stays down) */
{
    press(B_OCTDN);
    ui_frame();
    grid_wait(ms);
    btn_up(B_OCTDN);
    ui_frame();
}
static void two_locks(void)                          /* E1 (SOUND 1) and CUT (FILTER) locked at step 5, motion E1 too */
{
    lock_set(0, 4, P_E1, 100);
    lock_set(0, 4, P_FCUT, 20);
    lock_set(0, 5, P_E1, 90);
    motion_add(0, 4, P_E1, 33);
}
static void test_lock_clear_page(void)
{
    uint32_t ok;
    int16_t v;
    lock_fresh();
    tap(B_EDIT);
    two_locks();
    grid_down(WHITE[4]);
    grid_wait(350);
    oct_dn_for(100);
    ok = !lock_get(0, 4, P_E1, &v) && lock_get(0, 4, P_FCUT, &v) && lock_get(0, 5, P_E1, &v) && motion_count(0) == 1u;
    ok &= str_eq(ui.msg, "STEP 5 LOCKS CLEARED") && str_eq(cur_page()->title, "SOUND");
    grid_up(WHITE[4]);
    check("locks: step held + OCT- tap on SOUND clears that step's SOUND locks only (FILTER's, the next step's and "
          "motion kept)", ok && trk[0].step[4].on);
}
static void test_lock_clear_all(void)
{
    uint32_t ok;
    int16_t v;
    lock_fresh();
    tap(B_EDIT);
    two_locks();
    grid_down(WHITE[4]);
    grid_wait(350);
    press(B_OCTDN);
    ui_frame();
    grid_wait(700);
    ok = !lock_get(0, 4, P_E1, &v) && !lock_get(0, 4, P_FCUT, &v) && lock_get(0, 5, P_E1, &v) && motion_count(0) == 1u &&
         str_eq(ui.msg, "STEP 5 ALL CLEARED");
    btn_up(B_OCTDN);
    ui_frame();
    grid_up(WHITE[4]);
    check("locks: step held + OCT- held 0.6 s clears all its locks; the release after does nothing (still SOUND)",
          ok && str_eq(cur_page()->title, "SOUND"));
}
static void test_lock_oct_dn_alone(void)
{
    uint32_t k;
    lock_fresh();
    tap(B_EDIT);
    k = ui.page;
    tap(B_OCTDN);
    check("locks: OCT- with no step held on SOUND: nothing (the page stays)", ui.page == k && str_eq(cur_page()->title, "SOUND"));
}
static void test_lock_step_page(void)
{
    step_t *s = &trk[0].step[4];
    lock_fresh();
    open_step_page();
    ui_frame();
    held_turn(0, -1);
    held_turn(1, 1);
    held_turn(2, 1);
    grid_up(WHITE[4]);
    check("locks: SEQ STEP in STEPS: held + KNOB 1 / 2 / 3 are PROB / RATCH / ACC, no lock",
          s->on && s->cond == cond_store(19) && s->rat == 1u && s->acc && mo.b.count == 0u);
}

   /* (draw_foot: cv_trigs at x 6, y 5) */

/* device test 2026-10-25 ("ENV acts only from the second push"): the FM-1's tick counter has any low bits at a press
 * (the host's advance in steps of 24000: bit 1 always 0); ENV, and OCT- clearing a held step's locks, act on one
 * press at every phase of the clock */
static void test_env_any_tick_phase(void)
{
    uint32_t ph, ok = 1, n = 0;
    int16_t v;
    ui_host_init();
    tap(B_EDIT);
    for (ph = 0; ph < 8u; ph++) {
        uint32_t was = ui.env;
        host_ticks = (host_ticks & ~7u) + ph;
        env_tap();
        ok &= ui.env == (was ^ 1u);
        n += ui.env != was;
    }
    check("ENV: one press toggles STEPS at every phase of the tick counter (8 presses, 8 toggles)", ok && n == 8u);
    for (ph = 0; ph < 8u; ph++) {                    /* in STEPS: hold step 5, OCT- tap clears its SOUND locks */
        if (!ui.env)
            env_tap();
        lock_set(0, 4, P_E1, 100);
        grid_down(WHITE[4]);
        grid_wait(350);
        host_ticks = (host_ticks & ~7u) + ph;
        press(B_OCTDN);
        ui_frame();
        grid_wait(100);
        btn_up(B_OCTDN);
        ui_frame();
        grid_up(WHITE[4]);
        ok &= !lock_get(0, 4, P_E1, &v);
    }
    check("locks: step held + OCT- tap clears at every phase of the tick counter", ok);
    memset(&mo.b, 0, sizeof mo.b);
}
/* .. one press after: a bank change with ENV held, a held step, a page change, the menu, power-on */
static int env_one_press(void)
{
    uint32_t was = ui.env;
    host_ticks = (host_ticks & ~7u) + 6u;
    env_tap();
    if (ui.env == was)
        printf("     ENV press did not toggle (env %u, page %s, fx_knobs %u, menu %u)\n", ui.env, cur_page()->title, fx_knobs, ui.menu);
    return ui.env == (was ^ 1u);
}
static void test_env_one_press_after_states(void)
{
    uint32_t ok;
    ui_host_init();                                  /* (power-on) */
    tap(B_EDIT);
    TSEL->p[P_SLEN] = 64;
    ok = env_one_press();                            /* on */
    press(B_ENV);
    ui_frame();
    press(B_OCTUP);
    ui_frame();
    release_all();
    ui_frame();
    ok &= ui.bank == 1u && ui.env == 1u && env_one_press();   /* after ENV + OCT+: off */
    ok &= env_one_press();                           /* on */
    grid_down(WHITE[2]);
    grid_wait(400);
    grid_up(WHITE[2]);
    ok &= env_one_press();                           /* after a held step: off */
    tap(B_FX);
    grid_wait(100);                                  /* (a hand's gap: FX's release frame is PERFORM's) */
    ok &= env_one_press();                           /* after a page change: on */
    hold(B_HOME);
    hold(B_HOME);                                    /* the menu, closed */
    ok &= !ui.menu && env_one_press();               /* after the menu: off */
    check("ENV: one press toggles after ENV + OCT+, a held step, a page change, the menu, power-on", ok);
}
/* .. and lock entry after one ENV press, through the panel's own path (ENV, a step key, KNOB 2) */
static void test_env_one_press_then_lock(void)
{
    int16_t v = 0;
    uint32_t id;
    ui_host_init();
    tap(B_EDIT);
    song.sel = 0;
    memset(&mo.b, 0, sizeof mo.b);
    host_ticks = (host_ticks & ~7u) + 2u;
    env_tap();
    id = held_turn(1, 2);
    grid_up(WHITE[4]);
    check("one ENV press (tick bit 1 set), step 5 held + KNOB 2: a lock on step 5", ui.env == 1u && id < P_COUNT &&
          lock_get(0, 4, id, &v) && lock_count(0) == 1u);
    memset(&mo.b, 0, sizeof mo.b);
}

/* device test 2026-10-25: STEPS shows a padlock in the top bar (x 207..216, rows 3..12, the icons' 5 x 5 at 2x), on
 * every page; TRACKS none */
static uint32_t head_lock_px(void)                   /* the padlock's square (FREITAKT: ink on the ground, x 205..214) */
{
    uint32_t x, y, n = 0;
    for (y = 3; y < 13u; y++)
        for (x = 205; x < 215u; x++)
            n += fb[(Y_HEAD + y) * 240u + x] == C_INK;
    return n;
}
static void test_head_lock_icon(void)
{
    static const uint32_t FAMS[4] = {FAM_SND, FAM_FX, FAM_LFO, FAM_SEQ};
    uint32_t f, ok;
    ui_host_init();
    tap(B_EDIT);
    ui_frame();
    ok = head_lock_px() == 0u;
    env_tap();
    ui_frame();
    ok &= head_lock_px() == 76u;
    snap_page("locks/head_sound");
    for (f = 0; f < 4u; f++) {
        open_family(FAMS[f]);
        ui_frame();
        ok &= head_lock_px() == 76u;
    }
    snap_page("locks/head_seq");
    env_tap();
    ui_frame();
    ok &= head_lock_px() == 0u;
    check("STEPS: a padlock in the top bar on every page (SOUND, FX, LFO, SEQ; HOME has no top bar); TRACKS: none", ok);
}

/* release check 2026-10-25: the held step's "LOCK STEP n" line drawn whole (no glyph cut by a neighbour's paint), on
 * the top bar of every page where a step can be held in STEPS, and in HOME's strip */
static uint16_t ref_px[16 * 240];
static int text_whole(const char *t, uint32_t y0, uint32_t y1, uint16_t fg, uint16_t bg)   /* t somewhere in rows y0..y1 */
{
    uint32_t w = (uint32_t)md_text_w(&MD_F3, 2, t), h = 10u, x, y, i, j, good;
    cv_begin(w, h, bg);
    md_text(0, 0, &MD_F3, 2, t, fg);
    for (i = 0; i < w * h; i++)
        ref_px[i] = (uint16_t)((cv_px[i] >> 8) | (cv_px[i] << 8));
    for (y = y0; y + h <= y1 + 1u; y++)
        for (x = 0; x + w <= 240u; x++) {
            for (j = 0, good = 1; good && j < h; j++)
                for (i = 0; good && i < w; i++)
                    good = fb[(y + j) * 240u + x + i] == ref_px[j * w + i];
            if (good)
                return 1;
        }
    return 0;
}
static int lock_line_whole_now(const char *where);
static int lock_line_whole(const char *where)    /* as the frames left it, and after a full repaint */
{
    int ok = lock_line_whole_now(where);
    ui.force = 1;
    ui_frame();
    return ok && lock_line_whole_now(where);
}
static int lock_line_whole_now(const char *where)
{
    char t[16];
    int ok;
    snprintf(t, sizeof t, "LOCK STEP %u", (unsigned)lock_view_step() + 1u);
    ok = lock_view_step() < NSTEP && text_whole(t, 0, 15, C_INK, C_BG);   /* the top bar: ink on the ground */
    if (!ok)
        printf("     LOCK STEP line not whole: %s\n", where);
    return ok;
}
static void test_lock_line_whole_every_page(void)
{
    uint32_t i, ok = 1, n = 0;
    lock_fresh();
    for (i = 0; i < NPAGES; i++) {
        uint32_t f = PAGES[i].fam;
        if (f == FAM_SEQ || page_hidden(i))
            continue;
        open_page_at(i);
        grid_down(WHITE[4]);
        grid_wait(400);
        if (lock_page()) {
            ok &= (uint32_t)lock_line_whole(PAGES[i].title);
            n++;
        }
        grid_up(WHITE[4]);
        grid_wait(100);
    }
    home_tracks();                                   /* (FREITAKT's HOME has no top bar: TRACKS) */
    ui_frame();
    grid_down(WHITE[4]);
    grid_wait(400);
    ok &= (uint32_t)lock_line_whole("TRACKS");
    grid_up(WHITE[4]);
    check("locks: the LOCK STEP 5 line whole on every page a step can be held in STEPS (top bar)",
          ok && n > 15u);
}

/* the step LEDs (Model:Cycles style, ui_input.c ui_leds' step branch): a trig lit, a locked step (parameter or
 * machine lock) blinking, the selected track's playhead toggling its key (on its own LEN / FIT, its own page), black
 * 1..6 the tracks: the selected lit, an armed one breathing, the selected armed winking; GLO held over it all. The
 * lock mask cached (ui.c lk_view_mask), found again when a lock comes or goes */
static int sk_blinks(uint32_t key, uint32_t frames, int *always_on, int *always_off)
{
    uint32_t i, on = 0, off = 0;
    for (i = 0; i < frames; i++) {
        host_ticks += 90u * 1000u * FM1_TICKS_PER_US;
        ui_input();                                  /* (the LEDs' time: ui_input's) */
        ui_leds();
        if (led_on(14u + key))
            on++;
        else
            off++;
    }
    if (always_on)
        *always_on = !off;
    if (always_off)
        *always_off = !on;
    return on && off;
}
static int sk_ph_key(uint32_t want)                  /* the 16 keys: only key want (16: none) differs from the
                                                      * steps (trigs; a locked step other than want blinks: either) */
{
    uint32_t k, si;
    int ok = 1;
    const track_t *t = TSEL;
    ui_leds();
    for (k = 0; k < 16u; k++) {
        si = ui.bank * 16u + k;
        if (k != want && ((lkv.mask >> k) & 1u))
            continue;
        int trig = si < view_len(t) && t->step[si].on;
        ok &= led_on(14u + STEP_KEY[k]) == (k == want ? !trig : trig);
    }
    return ok;
}
static void test_step_leds(void)
{
    uint32_t ok = 1, k, id;
    int on1, off1;
    lock_fresh();
    tap(B_EDIT);
    trk[0].p[P_SLEN] = 16;
    trk[0].step[0].on = trk[0].step[2].on = 1;
    ui_frame();
    OK0(step_keys() && led_on(14u + STEP_KEY[0]) && led_on(14u + STEP_KEY[2]) && !led_on(14u + STEP_KEY[1]));
    OK0(led_on(14u + CUE_BK[0]) && !led_on(14u + CUE_BK[1]) && !led_breathes(14u + CUE_BK[1]));
    check("step LEDs: the trigs lit, the empty steps dark; black 1 (the selected track) lit, the others dark", ok);

    ok = 1;
    id = lock_id(1);
    lock_set(0, 2, id, 3);                            /* step 3: a parameter lock */
    lock_set(0, 5, P_MODEL, 1);                       /* step 6 (no trig): a machine lock */
    ui_frame();
    OK0(lkv.ok && lkv.gen == lk_gen && lkv.mask == ((1u << 2) | (1u << 5)));
    OK0(sk_blinks(STEP_KEY[2], 12, 0, 0) && sk_blinks(STEP_KEY[5], 12, 0, 0));
    sk_blinks(STEP_KEY[0], 12, &on1, 0);
    OK0(on1);
    check("step LEDs: a locked step blinks (a parameter lock on a trig, a machine lock), the plain trig steady", ok);

    ok = 1;
    lk_set_lock(&mo.b, 0, 0);                         /* (behind the bank's back: no lk_gen, the same count) */
    ui_leds();
    OK0(lkv.mask == ((1u << 2) | (1u << 5)));          /* cached: not found again */
    lk_set_lock(&mo.b, 0, 1);
    OK0(lock_clear_step(0, 2, 0) == 1u);
    ui_leds();
    OK0(lkv.mask == (1u << 5));
    sk_blinks(STEP_KEY[2], 12, &on1, 0);
    OK0(on1);
    lock_set(0, 9, id, 4);
    ui_leds();
    OK0(lkv.mask == ((1u << 5) | (1u << 9)));
    lock_clear_track(0);
    ui_leds();
    OK0(lkv.mask == 0u);
    sk_blinks(STEP_KEY[5], 12, 0, &off1);
    OK0(off1);
    check("step LEDs: the lock mask kept between frames, found again after a lock is added or cleared", ok);

    ok = 1;
    lock_set(0, 2, id, 3);
    transport_req = 1;
    seq_play_to(0, 1);
    OK0(sk_ph_key(1));                                /* empty step 2: lit */
    seq_play_to(0, 2);
    for (k = 0; k < 8u; k++) {                        /* locked trig step 3: dark (every blink phase) */
        host_ticks += 90u * 1000u * FM1_TICKS_PER_US;
        ui_input();
        ui_leds();
        OK0(!led_on(14u + STEP_KEY[2]));
    }
    seq_play_to(0, 3);
    OK0(sk_blinks(STEP_KEY[2], 12, 0, 0));            /* (blinking again once the cursor moved on) */
    seq_play_to(0, 0);
    ui_leds();
    OK0(!led_on(14u + STEP_KEY[0]));                  /* trig step 1: dark */
    check("step LEDs: the running cursor toggles its key (empty lit, a trig or a locked step dark)", ok);

    ok = 1;
    lock_clear_track(0);
    trk[1].p[P_SLEN] = 5;                             /* track 2: LEN 5, FIT (its 5 steps over the bar) */
    trk[1].p[P_SDIV] = DIV_FIT + 1;
    memset(trk[1].step, 0, sizeof trk[1].step);
    trk[1].step[0].on = 1;
    track_select(1);
    ui_frame();
    OK0(song.sel == 1u && led_on(14u + CUE_BK[1]) && !led_on(14u + CUE_BK[0]));
    for (k = 0; k < 5u; k++) {
        seq_play_to(1, k);
        OK0(sk_ph_key(k));                            /* the cursor on track 2's own step, keys 6..16 dark */
    }
    seq_play_to(1, 0);
    OK0(sk_ph_key(0) && !led_on(14u + STEP_KEY[5]));   /* round again after 5 */
    trk[1].p[P_SLEN] = 20;                            /* LEN 20: page 2 holds steps 17..20 */
    trk[1].p[P_SDIV] = TP[P_SDIV].def;
    trk[1].step[17].on = 1;
    ui.bank = 1;
    seq_play_to(1, 3);
    OK0(sk_ph_key(16));                               /* step 4: on page 1, none here */
    seq_play_to(1, 18);
    OK0(sk_ph_key(2));                                /* step 19: key 3 */
    seq_play_to(1, 17);
    OK0(sk_ph_key(1));                                /* step 18 (a trig): key 2 dark */
    ui.bank = 0;
    check("step LEDs: the cursor follows the selected track (LEN 5 + FIT, LEN 20 over two pages: its page only)", ok);

    ok = 1;
    song.rec = (uint8_t)(1u << 3);                    /* track 4 armed, track 2 selected */
    ui_frame();
    OK0(led_breathes(14u + CUE_BK[3]) && !led_breathes(14u + CUE_BK[1]) && led_on(14u + CUE_BK[1]));
    OK0(!led_on(14u + CUE_BK[0]) && !led_breathes(14u + CUE_BK[0]));
    song.rec = (uint8_t)(1u << 1);                    /* the selected armed: lit, winking */
    OK0(sk_blinks(CUE_BK[1], 24, 0, 0));
    OK0(!led_breathes(14u + CUE_BK[1]));
    check("step LEDs: an armed track's black key breathes; the selected one lit, winking when armed too", ok);

    ok = 1;
    song.rec = (uint8_t)(1u << 3);
    trk[1].step[2].on = 1;
    press(B_GLO);
    glo_hold_frames(2);
    OK0(!led_breathes(14u + CUE_BK[3]) && !led_on(14u + STEP_KEY[2]));
    release_all();
    ui_frame();
    ui_frame();
    OK0(led_breathes(14u + CUE_BK[3]));
    transport_req = 2;
    ui_frame();
    song.rec = 0;
    check("step LEDs: GLO held shows the cue map over them (no steps, no armed breathing), let go: back", ok);
}

/* ---- the carousels (FREITAKT redesign phase D) ---- */
/* the MENU (HOME held): one item at a time; SELECT scrolls both ways and wraps; KNOB 1 sets, OCT+ steps on, OCT- closes;
 * every item reachable, the former GLOBAL / SYSTEM settings among them; a frame sends only what changed */
static void test_menu_carousel(void)
{
    static const char *const MOVED[9] = {"CLOCK", "MIDI SYNC", "MIDI CHANNEL", "MIDI ROUTE", "USB MIDI", "METRONOME",
                                         "MUTE MODE", "MIDI THRU", "REC QUANT"};
    uint32_t i, k, ok, seen = 0, idle, px;
    char v[16];
    int16_t ch;
    ui_host_init();
    hold(B_HOME);
    check("MENU (HOME held): a carousel on COLOR: its value ORANGE under it, the title MENU, a dot an item (the first "
          "big), the hints K1 SET  OCT+ NEXT / OCT- BACK (no SEL MOVE: the < > and the dots say it)",
          ui.menu == 1u && ui.menu_sel == MI_COLOR && car_title_is("MENU") && menu_value_is(MI_COLOR, "ORANGE") &&
              car_pos_is(MI_COUNT, 0) && car_hint_has("K1") && car_hint_has("SET") && car_hint_has("OCT+") &&
              car_hint_has("NEXT") && !car_hint_has("SEL") && !car_hint_has("MOVE") && car_hint_has("OCT-") &&
              car_hint_has("BACK"));
    snap_page("menu_carousel_color");
    sel(1);
    ok = ui.menu_sel == MI_SPKEQ && menu_value_is(MI_SPKEQ, "FLAT") && car_pos_is(MI_COUNT, 1);
    sel(-1);
    ok &= ui.menu_sel == MI_COLOR && car_item_is("COLOR") && car_pos_is(MI_COUNT, 0);
    sel(-1);
    ok &= ui.menu_sel == MI_COUNT - 1u && car_item_is(MI_NAME[MI_COUNT - 1u]) && car_pos_is(MI_COUNT, MI_COUNT - 1u);
    sel(1);
    ok &= ui.menu_sel == MI_COLOR && car_item_is("COLOR");
    check("MENU: SELECT right the next item, left the one before; past either end it wraps (COLOR <-> the last)", ok);
    for (ok = 1, i = 0; i < MI_COUNT; i++) {
        menu_value(ui.menu_sel, v);
        if (!car_item_is(MI_NAME[ui.menu_sel]) || (v[0] && !car_val_is(v)) || !car_pos_is(MI_COUNT, ui.menu_sel)) {
            ok = 0;
            printf("      MENU item %u (%s = %s) not drawn\n", (unsigned)ui.menu_sel, MI_NAME[ui.menu_sel], v);
        }
        for (k = 0; k < 9u; k++)
            if (str_eq(MI_NAME[ui.menu_sel], MOVED[k]))
                seen |= 1u << k;
        ok &= !str_eq(MI_NAME[ui.menu_sel], "ABOUT") && !str_eq(MI_NAME[ui.menu_sel], "BACK");
        sel(1);
    }
    check("MENU: SELECT reaches every item, each drawn with its value (HARDWARE CALIBRATION on two lines); CLOCK, MIDI "
          "SYNC, MIDI CHANNEL, MIDI ROUTE, USB MIDI, METRONOME, MUTE MODE, MIDI THRU, REC QUANT among them; no ABOUT, "
          "no BACK", ok && seen == 0x1FFu && ui.menu_sel == MI_COLOR);
    check("MENU: the items (default build): COLOR .. BACKLIGHT, the nine settings, HARDWARE CALIBRATION",
          MI_COUNT == 16u && str_eq(MI_NAME[MI_PANEL], "HARDWARE CALIBRATION") && MI_PANEL == MI_COUNT - 1u);
    while (ui.menu_sel != MI_DRCH)
        sel(1);
    ui_frame();
    host_blit_px = 0;
    ui_frame();
    idle = host_blit_px;
    ch = song.g[G_DRCH];
    turn(EN_K1, 1);
    host_blit_px = 0;
    ui_frame();
    px = host_blit_px;
    menu_glob_text(G_DRCH, v);
    check("MENU: KNOB 1 sets the item's value (MIDI CHANNEL + 1), drawn under it; that frame sends the value's band alone "
          "(240 x 32), an idle frame nothing", song.g[G_DRCH] == ch + 1 && car_val_is(v) && px == 240u * 32u &&
                                                    idle == 0u);
    snap_page("menu_carousel_midi_channel");
    press(B_OCTUP);
    ui_frame();
    release_all();
    ui_frame();
    menu_glob_text(G_DRCH, v);
    check("MENU: OCT+ steps the value on", song.g[G_DRCH] == ch + 2 && car_val_is(v) && ui.menu == 1u);
    song.g[G_DRCH] = ch;
    while (ui.menu_sel != MI_USBMIDI)
        sel(1);
    menu_value(MI_USBMIDI, v);
    press(B_OCTUP);
    ui_frame();
    release_all();
    ui_frame();
    check("MENU: USB MIDI shows the USB state, no K1 / OCT+ hint (only shown); OCT+ does nothing there",
          car_item_is("USB MIDI") && car_val_is(v) && !car_hint_has("SET") && !car_hint_has("NEXT") &&
              car_hint_has("BACK") && ui.menu == 1u && ui.menu_sel == MI_USBMIDI);
    while (ui.menu_sel != MI_PANEL)
        sel(1);
    check("MENU: HARDWARE CALIBRATION: OCT+ START, a note what it does",
          car_item_is("HARDWARE CALIBRATION") && car_hint_has("START") && car_note_is("TEACH BUTTONS AND KNOBS"));
    snap_page("menu_carousel_calibration");
    press(B_OCTDN);
    ui_frame();
    release_all();
    ui_frame();
    check("MENU: OCT- closes it (HOME as last shown)", ui.menu == 0u);
}

static void save_open(const char *title)              /* SAVE taps until its page title shows */
{
    uint32_t k;
    for (k = 0; k < 4u && (ui.home || !str_eq(cur_page()->title, title)); k++)
        tap(B_SAVE);
}
static void save_turn(uint32_t k, int32_t d)           /* KNOB k+1, d detents (one a frame) */
{
    for (; d; d += d > 0 ? -1 : 1) {
        turn(EN_K1 + k, d > 0 ? 1 : -1);
        ui_frame();
    }
}
static void msg_wait(void)                             /* the message's frames out */
{
    uint32_t k;
    for (k = 0; k < 400u && ui.msg_t; k++)
        ui_frame();
    ui_frame();
}
/* SAVE > PROJECT: a slot at a time (A..D, its name, its state), SELECT and KNOB 1 scroll, K4 SAVE, K3 LOAD */
static void test_save_carousel(void)
{
    uint32_t ok = 1;
    ui_host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    kit_session();
    save_open("PROJECT");
    song.g[G_SLOT] = 1;
    ui_frame();
    OK0(car_title_is("PROJECT 1/3"));
    OK0(car_item_is("A") && car_val_is("EMPTY") && car_pos_is(4, 0));
    OK0(car_hint_has("K3") && car_hint_has("LOAD") && car_hint_has("K4") && car_hint_has("SAVE x2"));
    OK0(car_hint_has("K2") && car_hint_has("NAME") && !car_hint_has("SEL"));
    check("SAVE > PROJECT: a carousel of the slots: A, EMPTY under it, the title PROJECT 1/3, four dots (A's big), the "
          "hints K2 NAME  K3 LOAD / K4 SAVE x2", ok);
    {                                                /* owner 2026-11-22: the title 3x, the hints 2x, apart */
        int32_t w2 = md_text_w(&MD_F3, 4, "K4") + 6 + 8 + md_text_w(&MD_F3, 4, "SAVE x2"), y, x, top = 240, bot = 0;
        ok = region_is_text(7, 188, 4, "K2") && region_is_text(236 - md_text_w(&MD_F3, 4, "LOAD"), 188, 4, "LOAD") &&
             region_is_text(120 - w2 / 2 + 3, 215, 4, "K4");
        for (y = 16; y < 52; y++)                    /* the title's ink: rows 18..47 (the dots: 52..57) */
            for (x = 0; x < 240; x++)
                if (fb_count((uint32_t)x, (uint32_t)y, 1, 1, C_INK)) {
                    top = y < top ? y : top;
                    bot = y > bot ? y : bot;
                }
        ok &= top == 18 && bot == 47 && fb_count(0, 60, 240, 4, C_INK) == 0u && fb_count(0, 178, 240, 4, C_INK) == 0u;
        ok &= md_text_w(&MD_F3, 6, "PROJECT") + 12 + md_text_w(&MD_F3, 3, "1/3") <= 236;
        check("SAVE > PROJECT: the title 3x (PROJECT at 6x, 30 px tall, 1/3 at 3x), the hints 2x (4x keycaps, K2 NAME at "
              "the left edge, K3 LOAD at the right, K4 SAVE x2 centred under them); nothing between the bands", ok);
        ok = 1;
    }
    ok = 1;
    sel(1);
    ok &= song.g[G_SLOT] == 2 && car_item_is("B") && car_pos_is(4, 1);
    sel(-1);
    ok &= song.g[G_SLOT] == 1 && car_item_is("A");
    sel(-1);
    ok &= song.g[G_SLOT] == 4 && car_item_is("D") && car_pos_is(4, 3);
    sel(1);
    ok &= song.g[G_SLOT] == 1 && car_item_is("A");
    save_turn(0, 2);
    ok &= song.g[G_SLOT] == 3 && car_item_is("C");
    save_turn(0, -2);
    ok &= song.g[G_SLOT] == 1 && car_item_is("A") && !kp.on;
    check("PROJECT: SELECT steps A B C D, wrapping both ways; KNOB 1 (SLOT) too; no knob popup", ok);
    save_turn(3, 1);
    ok = ui.arm == G_SAVE && car_note_is("AGAIN: SAVE") && !project_used(0);
    save_turn(3, 1);
    ok &= project_used(0) && proj_loaded() == 0 && !strncmp(ui.msg, "SAVED", 5) && car_note_is(ui.msg);
    msg_wait();
    ok &= car_note_is("LOADED") && car_val_is("USED") && car_item_is("A");
    check("PROJECT: K4 once arms (AGAIN: SAVE in the note), again saves the slot shown (SAVED, then LOADED; USED: no "
          "name)", ok);
    snap_page("save_project_slot");
    sel(1);
    save_turn(3, 2);
    msg_wait();
    ok = project_used(1) && proj_loaded() == 1 && car_item_is("B") && car_note_is("LOADED");
    trk[5].step[3].on ^= 1;                          /* B edited */
    ui_frame();
    ok &= slot_dirty(1) && car_note_is("LOADED EDITED");
    sel(-1);
    save_turn(2, 1);
    ok &= ui.arm == G_LOAD && car_note_is("AGAIN: LOAD");
    save_turn(2, 1);
    msg_wait();
    ok &= !ui.confirm && proj_loaded() == 0 && car_item_is("A") && car_note_is("LOADED");
    sel(1);
    ok &= car_item_is("B") && car_note_is("EDITED");
    check("PROJECT: B saved, edited (LOADED EDITED); K3 twice on A loads A (B keeps its edits: EDITED)", ok);
    memset(proj_slot, 0, sizeof proj_slot);
}
/* OVERWRITE B*? / DISCARD EDITS? (owner 2026-11-22): a screen of their own, nothing of the page under them: the
 * question at 4x about row 86, what it means at 3x at row 124, OCT- NO at the left edge and OCT+ YES at the right
 * one as the carousels' 4x hints (row 188), a dotted rule over them; the rest of the screen ground */
static int confirm_full_is(const char *q, const char *q2)
{
    uint32_t ink = fb_count(0, 0, 240, 240, C_INK), want;
    int32_t qw = md_text_w(&MD_F3, 4, q), q2w = md_text_w(&MD_F3, 3, q2);
    if (!region_is_text(120 - qw / 2, 76, 4, q) || !region_is_text(120 - q2w / 2, 124, 3, q2) ||
        !region_is_text(7, 188, 4, "OCT-") || !region_is_text(236 - md_text_w(&MD_F3, 4, "YES"), 188, 4, "YES") ||
        !region_is_text(236 - md_text_w(&MD_F3, 4, "YES") - 8 - 6 - md_text_w(&MD_F3, 4, "OCT+") + 3, 188, 4, "OCT+"))
        return 0;
    want = fb_count(0, 76, 240, 20, C_INK) + fb_count(0, 124, 240, 15, C_INK) + fb_count(0, 182, 240, 58, C_INK);
    return ink == want && fb_count(0, 0, 240, 76, C_INK) == 0u;   /* (no top bar, no title: a screen of its own) */
}
/* SAVE > PROJECT's questions: OVERWRITE B*? (SAVE onto an edited slot not loaded), DISCARD EDITS? (LOAD of an
 * edited slot), each a screen of its own; the carousel whole again after */
static void test_save_carousel_confirms(void)
{
    uint32_t ok;
    ui_host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    kit_session();
    save_open("PROJECT");
    song.g[G_SLOT] = 1;
    save_turn(3, 2);                                 /* A saved */
    sel(1);
    save_turn(3, 2);                                 /* B saved (B loaded) */
    trk[5].step[3].on ^= 1;
    sel(-1);
    save_turn(2, 2);                                 /* A loaded, B keeps its edits */
    msg_wait();
    sel(1);
    save_turn(3, 2);
    ok = 1;
    OK0(ui.confirm == CONFIRM_OVERWRITE && ui.confirm_trk == 1u);
    OK0(confirm_full_is("OVERWRITE B*?", "ITS EDITS ARE LOST"));
    OK0(!car_title_is("PROJECT 1/3"));
    snap_page("save_project_overwrite");
    host_blit_px = 0;
    ui_frame();
    OK0(host_blit_px == 0u);                         /* (an idle frame under it: nothing sent) */
    press(B_OCTDN);
    ui_frame();
    release_all();
    ui_frame();
    OK0(!ui.confirm && proj_loaded() == 0 && slot_dirty(1));
    msg_wait();                                      /* (AGAIN: SAVE's time out) */
    OK0(car_item_is("B") && car_val_is("USED") && car_note_is("EDITED") && car_title_is("PROJECT 1/3") &&
        !confirm_full_is("OVERWRITE B*?", "ITS EDITS ARE LOST"));
    check("PROJECT: SAVE onto B (edited, not loaded) asks OVERWRITE B*? on a screen of its own (the question big, OCT- "
          "NO / OCT+ YES as 4x hints, no carousel under it; an idle frame sends nothing); OCT- (NO) keeps it, the "
          "carousel whole again", ok);
    save_turn(2, 2);
    ok = ui.confirm == CONFIRM_DISCARD && confirm_full_is("DISCARD EDITS?", "LOAD SLOT B SAVED") &&
         !car_title_is("PROJECT 1/3");
    snap_page("save_project_discard");
    press(B_OCTUP);
    ui_frame();
    release_all();
    msg_wait();
    ok &= !ui.confirm && proj_loaded() == 1 && !slot_dirty(1) && car_item_is("B") && car_note_is("LOADED");
    check("PROJECT: LOAD of B (edited) asks DISCARD EDITS? on a screen of its own; OCT+ (YES) loads it as saved (LOADED)",
          ok);
    memset(proj_slot, 0, sizeof proj_slot);
}
/* SAVE > TOOLS and SAVE > COPY as carousels; the PRESETS popup over them */
static void test_tools_copy_carousels(void)
{
    uint32_t ok, k;
    char n[32];
    ui_host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    kit_session();
    save_open("TOOLS");
    ui.tool = 0;
    ui.force = 1;
    ui_frame();
    check("SAVE > TOOLS: a carousel of its four actions: CLEAR TRACK (TRACK n, STEPS, MOTION, LOCKS), the title TOOLS "
          "2/3, the hints K1 TURN TWICE",
          car_title_is("TOOLS 2/3") && car_item_is("CLEAR TRACK") && car_val_is("TRACK 1") &&
              car_note_is("STEPS, MOTION, LOCKS") && car_pos_is(4, 0) && car_hint_has("K1") &&
              car_hint_has("TURN TWICE") && !car_hint_has("MOVE"));
    sel(1);
    ok = ui.tool == 1u && car_item_is("INIT SOUND") && car_hint_has("K2");
    sel(-1);
    sel(-1);
    ok &= ui.tool == 3u && car_item_is("INIT ALL") && car_val_is("EVERYTHING") && car_hint_has("K4");
    sel(1);
    ok &= ui.tool == 0u && car_item_is("CLEAR TRACK");
    check("TOOLS: SELECT steps the actions, wrapping both ways", ok);
    save_turn(2, 1);
    ok = ui.tool == 2u && car_item_is("CLEAR ALL") && car_note_is("AGAIN: CLEAR ALL") && trk[0].step[0].on;
    snap_page("save_tools_clear_all");
    save_turn(2, 1);
    for (k = 0; k < NTRK; k++)
        ok &= !trk[k].step[0].on && !trk[k].step[4].on;
    ok &= car_note_is("ALL PATTERNS CLEARED");
    check("TOOLS: a knob shows its action (K3: CLEAR ALL, AGAIN: CLEAR ALL in the note); again it acts", ok);
    save_open("COPY");
    song.g[G_SLOT] = 1;
    ui_frame();
    ok = car_title_is("COPY 3/3") && car_item_is("A") && car_val_is("EMPTY") && car_hint_has("COPY") &&
         car_hint_has("PASTE") && car_hint_has("LEARN OFF") && !car_hint_has("SLOT");
    project_save(0);
    msg_wait();
    save_turn(1, 1);
    ok &= copy_src == 1u && car_note_is(ui.msg);
    msg_wait();
    snprintf(n, sizeof n, "%s%s%s", proj_loaded() == 0 ? "LOADED " : "", slot_dirty(0) ? "EDITED " : "", "COPIED");
    ok &= car_note_is(n);
    snap_page("save_copy_slot");
    save_turn(3, 1);
    ok &= ui.ml.on && car_hint_has("LEARN ON");
    save_turn(3, -1);
    ok &= !ui.ml.on && car_hint_has("LEARN OFF");
    sel(1);
    ok &= song.g[G_SLOT] == 2 && car_item_is("B") && car_note_is("");
    check("SAVE > COPY: a carousel of the slots (COPY 3/3); K2 copies the one shown (COPIED in its note), K4 MIDI "
          "LEARN ON / OFF in its hint", ok);
    pre_turn(1);
    ok = pp_on() && save_car_on() && lit_box(0, 112, 240, 152) > 4000u && as_full();
    pre_wait(3100);
    ui_frame();
    ok &= !pp_on() && car_item_is("B") && car_val_is("EMPTY") && car_hint_has("PASTE") && as_full();
    check("SAVE's carousel: the PRESETS popup a screen of its own over it; gone, the carousel whole again", ok);
    memset(proj_slot, 0, sizeof proj_slot);
}

int main(void)
{
    test_safe_start();
    test_key_selects_track();
    test_grid_hold_accent();
    test_grid_step_edit();
    test_step_hold();
    test_step_hold_on_acc();
    test_step_hold_every_seq_page();
    test_grid_hold_track_change();
    test_engine_screens();
    test_seq_screens();
    test_families();
    test_sound_pages();
    test_track_select();
    test_model_swap();
    test_home_macros();
    test_grid_keys();
    test_bank_follows_len();
    test_home_modes();
    test_rec_everywhere();
    test_quick_mute();
    test_glo_keys();
    test_glo_vars();
    test_arp_solo();
    test_arp_edit_page();
    test_sel_select();
    test_seq_mute();
    test_combo_refused();
    test_seq_button();                               /* FREITAKT phase C: SEQ, REC + SEL, the copy / paste combos */
    test_clr_combo();
    test_copy_paste_combos();
    test_paste_locks_full();
    test_steps_copy_paste();
    test_save_tap_opens_projects();
    test_cue_keys_no_notes();
    test_save_dead();
    test_no_save_while_playing();
    test_knob_accel();
    test_home_keys_and_lfo();
    test_tracks_rec_keys();
    test_rec_all();
    test_rec_fill_hold();
    test_rec_hold_vs_rec_all();
    test_rec_seq_unchanged();
    test_rec_learned_short();
    test_copy_page();
    test_acid_cap();
    test_select_pages();
    test_global_mute_setting();
    test_global_metro_setting();
    test_glo_midi_settings();
    test_glo_midi_out();
    test_glo_quant();
    test_comp_pages();
    test_comp_ghost_knob();
    test_comp_keys();
    test_tools_all();
    test_layer_in_edit();
    test_lfo_pages();
    test_lfo_edit_marker();
    test_oct_both_reaches_main();
    test_tracks_rows();
    test_screens();
    test_project_roundtrip();
    test_project_rejects();
    test_lfo_src_no_lfo2();
    test_ui_frame_cost();
    test_speaker_eq();
    test_bus_pages_on_glo();
    test_reverb_pages();
    test_delay_mode_page();
    test_div_order();
    test_usb_level();
    test_clock_screens();
    test_perform_gesture();
    test_perform_keys();
    test_perform_menu_kills();
    test_perform_page();
    test_perform_screens();
    test_perform_fast_key();
    test_perform_knobs_stay();
    test_perform_dead_hold();
    test_motion_recording();
    test_motion_model_change();
    test_clears_and_locks();
    test_name_save();
    test_name_keys_silent();
    test_name_stop_to_save();
    test_name_message_ends();
    test_phys_screens();
    test_filter_screens();
    test_insert_screens();
    test_initsnd_filter();
    test_perform_knobs_follow_screen();
    test_perform_dead_knob();
    test_step_keys_everywhere();
    test_tracks_rec_mutes();
    test_fm_screens();
    test_ktrx_screens();
    test_trxn_screens();
    test_efmn_screens();
    test_trx_drum_screens();
    test_keys_map_oct();
    test_keys_acid_mono();
    test_keys_release_anywhere();
    test_keys_owners();
    test_keys_oct_uboot();
    test_keys_live_rec();
    test_keys_live_rec_wrap();
    test_keys_live_rec_mid_block();
    test_keys_live_rec_long_hold();
    test_keys_live_rec_tie_clean();
    test_keys_live_rec_drum();
    test_step_key_note();
    test_step_key_note_drum();
    test_key_note_reset();
    test_rec_hit_drum_note();
    test_rec_hit_on_tie();
    test_keys_live_rec_len_cut();
    test_keys_seq_takeover();
    test_acid_screens();
    test_prio_sound_list();
    test_backlight_leds();
    test_backlight_beats_everywhere();
    test_backlight_menu();
    test_boot_title();
    test_boot_splash();
    test_theme_list();
    test_theme_stored();
    test_theme_menu();
    test_theme_row_name();
    test_theme_repaint();
    test_topbar();
    test_msg_popup();
    test_graph_model_name();
    test_save_carousel();
    test_save_carousel_confirms();
    test_tools_copy_carousels();
    test_perform_buttons();
    test_home_strip();
    test_confirm_popup();
    test_tempo_popup();
    test_menu_carousel();
    test_name_block_cursor();
    test_theme_sheet();
    test_fix_value_4_glyphs();
    test_kpop_first_turn();
    test_kpop_value_row_only();
    test_kpop_time();
    test_kpop_undo();
    test_kpop_undo_kinds();
    test_kpop_undo_lock();
    test_fix_msg_shrink();
    test_fix_popup_holds_graph();
    test_popup_shows_and_loads();
    test_popup_fast_while_playing();
    test_popup_skips_capped_acid();
    test_list_save_delete_overwrite();
    test_long_edit_opens_list();
    test_edit_hold_in_learn();
    test_name_and_edited_mark();
    test_list_preview_scrub();
    test_list_cancel_restores();
    test_list_load_closes();
    test_list_save_saves_real_sound();
    test_list_seven_rows();
    test_popups_keep_their_margins();
    test_model_list_order();
    test_model_names();
    test_vg_deterministic();
    test_vg_state_untouched();
    test_vg_live_audio();
    test_vg_budget();
    test_init_all_trx_kit();
    test_fk_perform_own_knobs();                     /* fixes 2026-10-21 (fix-keys) */
    test_fk_oct_under_mute_perform();                /* (fix-keys review) */
    test_fk_fx_release_knobs();
    test_fk_perform_preset_algo();
    test_fk_track_keys();
    test_fk_oct_breath();
    test_fk_led_levels();
    test_env_toggle();                               /* parameter locks (2026-10-24): ENV */
    test_env_tracks_unchanged();
    test_env_banks();
    test_env_oct_jobs();
    test_env_keys_exclusive();
    test_env_not_saved();
    test_env_home_and_tracks();
    test_env_while_step_held();
    test_lock_entry_every_page();                    /* parameter locks: entry and clearing */
    test_lock_before_hold();
    test_lock_cells();
    test_lock_while_playing();
    test_lock_full();
    test_lock_clear_page();
    test_lock_clear_all();
    test_lock_oct_dn_alone();
    test_lock_step_page();
    test_env_any_tick_phase();                       /* device test 2026-10-25 */
    test_env_one_press_then_lock();
    test_env_one_press_after_states();
    test_head_lock_icon();
    test_lock_line_whole_every_page();
    test_step_leds();                                /* the step LEDs: trigs, locks, cursor, tracks */
    printf(fails ? "ui_test: %d FAILED\n" : "ui_test: all passed\n", fails);
    return fails ? 1 : 0;
}
