/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Drum machine fork: 2026 DEADACTIVE */
/* Drum UI drawing in the Machinedrum style (spec docs/superpowers/specs/2026-10-19-md-ui-design.md): the top bar,
 * the knob cells, the graphs, the popups, the trig strip and the breadcrumb; two colours (C_BG, C_INK) */
static void draw_menu(void);
static int save_car_on(void);                      /* ui_menu.c: SAVE's pages, carousels */
static void draw_save(void);

static uint32_t str_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}

/* src cut to n - 1 characters at most, and no wider than maxw in MD_F3 at 2x */
static void md_fit(char *d, const char *src, uint32_t n, int32_t maxw)
{
    str_cpy(d, src, n);
    while (d[0] && md_text_w(&MD_F3, 2, d) > maxw)
        d[str_len(d) - 1u] = 0;
}


/* ---- partial redraws (the LCD transfer is the cost: ~1.33 us a pixel at 12 MHz, the whole screen ~77 ms). A region
 * (a strip, a row, the graph) draws its canvas whole when its signature moves, else only windows of it (cv_begin_win):
 * the parts that changed, and the damage: what a popup left this frame (rg.dmg, less the popups still up, rg.ex) */
typedef struct {
    int16_t x, y, w, h;
} rc_t;
#define RG_WIN 40u
static struct {
    rc_t dmg;                    /* the screen rect a popup left / uncovered this frame (w 0: none) */
    rc_t ex[3];                  /* the popups up (drawn this frame, or there): no window covers them */
    uint8_t nex;
    uint8_t noex;                /* windows not less rg.ex (PRESETS' popup drawn whole: over the others, as before) */
    uint8_t thaw;                /* a popup came / went this frame: the graph draws as it is now (round the popups) */
    int16_t rx, ry, rw, rh;      /* the region planned (screen) */
    uint8_t n;                   /* its windows (its canvas's coordinates) */
    rc_t win[RG_WIN];
} rg;

static int rc_cut(rc_t *a, rc_t b)                   /* a = a & b; 0: empty */
{
    int32_t x0 = a->x > b.x ? a->x : b.x, y0 = a->y > b.y ? a->y : b.y;
    int32_t x1 = a->x + a->w < b.x + b.w ? a->x + a->w : b.x + b.w;
    int32_t y1 = a->y + a->h < b.y + b.h ? a->y + a->h : b.y + b.h;
    if (x1 <= x0 || y1 <= y0)
        return 0;
    a->x = (int16_t)x0, a->y = (int16_t)y0, a->w = (int16_t)(x1 - x0), a->h = (int16_t)(y1 - y0);
    return 1;
}

static void rg_damage(int32_t x, int32_t y, int32_t w, int32_t h)   /* dmg grows to hold this rect */
{
    if (rg.dmg.w) {
        int32_t x1 = rg.dmg.x + rg.dmg.w > x + w ? rg.dmg.x + rg.dmg.w : x + w;
        int32_t y1 = rg.dmg.y + rg.dmg.h > y + h ? rg.dmg.y + rg.dmg.h : y + h;
        x = rg.dmg.x < x ? rg.dmg.x : x;
        y = rg.dmg.y < y ? rg.dmg.y : y;
        w = x1 - x;
        h = y1 - y;
    }
    rg.dmg.x = (int16_t)x, rg.dmg.y = (int16_t)y, rg.dmg.w = (int16_t)w, rg.dmg.h = (int16_t)h;
}

static void rg_keep(int32_t x, int32_t y, int32_t w, int32_t h)    /* a popup still up: kept out of the damage */
{
    if (rg.nex < 3u) {
        rc_t *e = &rg.ex[rg.nex++];
        e->x = (int16_t)x, e->y = (int16_t)y, e->w = (int16_t)w, e->h = (int16_t)h;
    }
}

static int rg_meets(int32_t x, int32_t y, int32_t w, int32_t h)    /* the damage touches this screen rect */
{
    rc_t a = {(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h};
    return rg.dmg.w && rc_cut(&a, rg.dmg);
}

static void rg_region(int32_t x, int32_t y, int32_t w, int32_t h)  /* plan a region's windows (none yet) */
{
    rg.rx = (int16_t)x, rg.ry = (int16_t)y, rg.rw = (int16_t)w, rg.rh = (int16_t)h;
    rg.n = 0;
}

/* a window, in the region's coordinates: less the popups up (rg.ex; unless rg.noex), each one splitting a piece in up
 * to four (above, below, left, right) */
static void rg_add(int32_t x, int32_t y, int32_t w, int32_t h)
{
    static rc_t p[64], q[64];                         /* (3 rects: 4 ^ 3 pieces at most) */
    rc_t a = {(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h}, r = {0, 0, rg.rw, rg.rh};
    uint32_t n = 1, m, i, k;
    if (!rc_cut(&a, r))
        return;
    a.x = (int16_t)(a.x + rg.rx), a.y = (int16_t)(a.y + rg.ry);
    p[0] = a;
    for (k = 0; k < (rg.noex ? 0u : rg.nex); k++) {
        rc_t e = rg.ex[k];
        for (i = m = 0; i < n; i++) {
            rc_t c = p[i], o = e;
            if (!rc_cut(&o, c)) {                     /* clear of it: whole */
                q[m++] = c;
                continue;
            }
            if (o.y > c.y)
                q[m++] = (rc_t){c.x, c.y, c.w, (int16_t)(o.y - c.y)};
            if (o.y + o.h < c.y + c.h)
                q[m++] = (rc_t){c.x, (int16_t)(o.y + o.h), c.w, (int16_t)(c.y + c.h - o.y - o.h)};
            if (o.x > c.x)
                q[m++] = (rc_t){c.x, o.y, (int16_t)(o.x - c.x), o.h};
            if (o.x + o.w < c.x + c.w)
                q[m++] = (rc_t){(int16_t)(o.x + o.w), o.y, (int16_t)(c.x + c.w - o.x - o.w), o.h};
        }
        for (i = 0; i < m; i++)
            p[i] = q[i];
        n = m;
    }
    for (i = 0; i < n && rg.n < RG_WIN; i++) {
        p[i].x = (int16_t)(p[i].x - rg.rx), p[i].y = (int16_t)(p[i].y - rg.ry);
        rg.win[rg.n++] = p[i];
    }
}

static void rg_whole(void) { rg_add(0, 0, rg.rw, rg.rh); }

static void rg_clip_h(int32_t h)                     /* the windows planned, less the region's rows from h */
{
    uint32_t i, n = 0;
    rc_t c = {0, 0, rg.rw, (int16_t)h};
    for (i = 0; i < rg.n; i++) {
        rc_t a = rg.win[i];
        if (rc_cut(&a, c))
            rg.win[n++] = a;
    }
    rg.n = (uint8_t)n;
}

static void rg_damaged(void)                         /* the region's part in the damage */
{
    rc_t a = {rg.rx, rg.ry, rg.rw, rg.rh};
    if (rg.dmg.w && rc_cut(&a, rg.dmg))
        rg_add(a.x - rg.rx, a.y - rg.ry, a.w, a.h);
}

/* a region with one signature (screen x, y, w x h; *st the one shown): whole when ui.force or it moved, else its
 * damage. The windows' count */
static uint32_t rg_plan(uint32_t *st, uint32_t sig, int32_t x, int32_t y, int32_t w, int32_t h)
{
    rg_region(x, y, w, h);
    if (ui.force || sig != *st) {
        *st = sig;
        rg_whole();
        return rg.n;
    }
    rg_damaged();
    return rg.n;
}

static void rg_win(uint32_t i, uint32_t sy, uint16_t bg)          /* begin window i (sy: the canvas's screen row) */
{
    cv_begin_win((uint32_t)rg.win[i].x, (uint32_t)rg.win[i].y, (uint32_t)rg.win[i].w, (uint32_t)rg.win[i].h, sy, bg);
}

static void rg_fill(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* a fill: whole on ui.force, else its
                                                                                * damage */
{
    uint32_t i;
    rg_region(x, y, w, h);
    if (ui.force)
        rg_whole();
    else
        rg_damaged();
    for (i = 0; i < rg.n; i++)
        lcd_fill((uint32_t)(x + rg.win[i].x), (uint32_t)(y + rg.win[i].y), (uint32_t)rg.win[i].w, (uint32_t)rg.win[i].h,
                 c);
}

static int32_t batt_level(void)
{
    return song.batt_raw >= 591 ? 3 : song.batt_raw >= 561 ? 2 : song.batt_raw >= 531 ? 1 : 0;
}
static int32_t batt_shown(void)
{
    if (usb.config && !usb.suspended)
        return 1 + (int32_t)((fm1_ms / 600u) % 3u);
    return batt_level();
}

/* the top bar's mode (spec §7): KEYS and its lowest key's note ("KEYS F3"); 0 (b "") when not */
static int head_text(char *b, int keys);
static int head_mode(char *b) { return head_text(b, 1); }
static int head_text(char *b, int keys)              /* keys 0: not KEYS (draw_head) */
{
    uint32_t ls = lock_view_step();
    b[0] = 0;
    if (ui.cue_map) {                                 /* SAVE held (spec 2026-10-26): CUE A1 (* edited), CUE>B3 queued */
        int32_t ld = proj_loaded();
        if (cue.q_slot || cue.q_var) {
            uint32_t s = cue.q_slot ? cue.q_slot - 1u : (uint32_t)(ld < 0 ? 0 : ld);
            str_cpy(b, "CUE>", 16);
            b[4] = (char)('A' + s);
            b[5] = cue.q_var ? (char)('0' + cue.q_var) : 0;
            b[6] = 0;
        } else if (ld < 0) {
            str_cpy(b, "CUE -", 16);
        } else {
            str_cpy(b, "CUE ", 16);
            b[4] = (char)('A' + ld);
            b[5] = (char)('1' + var_cur);
            b[6] = slot_dirty((uint32_t)ld) ? '*' : 0;
            b[7] = 0;
        }
        return 1;
    }
    if (fill_on) {                                    /* REC held off SEQ: a fill (spec 2026-10-26) */
        str_cpy(b, "FILL", 16);
        return 1;
    }
    if (ls < NSTEP) {                                 /* a step held in STEPS: its locks on the knobs */
        str_cpy(b, "LOCK STEP ", 16);
        fmt_int(b + 10, (int32_t)ls + 1);
        return 1;
    }
    if (!ui.keys || !keys)
        return 0;
    str_cpy(b, mkb_kbd ? "MIDI KBD " : "KEYS ", 16);   /* (the MIDI KEYBOARD: KBD not OFF) */
    note_text(b + str_len(b), 53 + 12 * song.octave);
    return 1;
}

/* the top bar (FREITAKT redesign): plain ground, the battery; the ENV STEPS padlock; on the left the passing states
 * (draw_head). Messages are a popup (draw_msg). */
static void draw_head(void)
{
    char m[32], t[32];
    uint32_t k, sig;
    int32_t x = 4, lvl;
    uint32_t rec = (song.rec >> song.sel) & 1u ? 2u : song.rec != 0u;
    /* FREITAKT redesign: the battery alone, top right; the ENV STEPS padlock left of it; on the left only what is
     * essential and passing: a track armed (the record icon), MIDI LEARN's line, the held modes (CUE, FILL, LOCK STEP,
     * STEP TOOLS, STEP REC; not KEYS: its LED says it) */
    if (ui.ml.on)
        ml_line(m);
    else if (!head_text(m, 0))
        m[0] = 0;
    lvl = batt_shown();
    sig = str_hash(rec * 5u + (uint32_t)lvl * 7777u + ui.env * 15485863u + 0x9E3779B9u, m);
    if (!ui.force && sig == ui.head_sig)
        return;
    ui.head_sig = sig;
    cv_begin(240, H_HEAD, C_BG);
    if (rec)
        x = md_text(4, 3, &MD_ICON, 2, rec == 2u ? "r" : "a", C_INK) + 6;
    if (m[0]) {
        md_fit(t, m, sizeof t, (ui.env ? 200 : 212) - x);
        md_text(x, 3, &MD_F3, 2, t, C_INK);
    }
    if (ui.env) {                                     /* STEPS (parameter locks): a padlock, the icons' 5 x 5 at 2x */
        static const uint8_t PADLOCK[5] = {0x0E, 0x11, 0x1F, 0x1B, 0x1F};   /* .###. #...# ##### ##.## ##### */
        uint32_t r, c;
        for (r = 0; r < 5u; r++)
            for (c = 0; c < 5u; c++)
                if ((PADLOCK[r] >> (4u - c)) & 1u)
                    cv_rect(205 + (int32_t)c * 2, 3 + (int32_t)r * 2, 2, 2, C_INK);
    }
    cv_frame(220, 4, 14, 8, C_INK);                   /* the battery: outline, nub, up to 3 cells */
    cv_rect(234, 6, 2, 4, C_INK);
    for (k = 0; k < (uint32_t)lvl && k < 3u; k++)
        cv_rect(222 + (int32_t)k * 4, 6, 3, 4, C_INK);
    cv_blit(0, Y_HEAD);
}

/* the fixed parts (ui.force): row 16, the knob row's ground, its dotted dividers at x 60 / 120 / 180, the rule at y 60;
 * the 1 px frame's edges where no region draws them */
static int list_on(void);
static int list_band_on(void);
static int32_t list_y0(void);
static void draw_frame(void)
{
    int32_t y;
    if (list_on()) {                                  /* LIST: the ground over the rows (no band: y 16..19) */
        if (!list_band_on())
            lcd_fill(0, H_HEAD, 240, (uint32_t)(list_y0() - H_HEAD), C_BG);
        return;
    }
    lcd_fill(1, H_HEAD, 238, 1, C_BG);                /* (only what no region covers: nothing sent twice) */
    lcd_fill(0, H_HEAD, 1, 1, C_INK);
    lcd_fill(239, H_HEAD, 1, 1, C_INK);
    if (ui.home && !ui.layer)                         /* the HOME strip: y 17..68 */
        return;
    lcd_fill(0, Y_KNOB, 1, H_KNOB, C_INK);           /* the knob cells: x 1..59, 61..119, 121..179, 181..238 */
    lcd_fill(239, Y_KNOB, 1, H_KNOB, C_INK);
    cv_begin(1, H_KNOB, C_BG);                        /* the dotted dividers */
    cv_oy = 0;
    for (y = 0; y < H_KNOB; y += 2)
        cv_pset(0, y, C_INK);
    cv_blit(60, Y_KNOB);
    cv_blit(120, Y_KNOB);
    cv_blit(180, Y_KNOB);
    lcd_fill(0, Y_GRAPH - 1, 240, 1, C_INK);
    lcd_fill(0, Y_GRAPH + H_GRAPH, 240, 240 - (Y_GRAPH + H_GRAPH), C_BG);   /* (no step strip, no breadcrumb) */
}

/* ---- LIST pages (FREITAKT redesign, rule 4): the parameter pages as a list, a row a knob ("NAME  value", the
 * largest MD_F3 that fits, up to 5x), under the top bar; the SOUND pages and LFO 1/3 a small schematic over the rows
 * (list_band: y 16..63). Each row a region of its own (rg_plan: whole when it changed, else what a popup left), so
 * a turn sends one row. TRACKS is a list too, of the tracks (draw_tracks); the SAVE pages and PERFORM keep their knob
 * cells and graph. */
#define LS_GY H_HEAD                                    /* the schematic's band */
#define LS_GH 48
static struct {
    uint32_t sig[4];                                    /* the rows' signatures shown */
    uint32_t g;                                         /* .. the band's */
} ls;
static int list_on(void)
{
    const page_t *pg = cur_page();
    return !ui.home && !ui.layer && (pg->scope == SC_TRACK || pg->scope == SC_MIX ||
                                     (pg->scope == SC_GLOBAL && pg->fam != FAM_SAVE));
}
static int list_band_on(void) { return cur_page()->graph == GR_MODEL || cur_page()->graph == GR_LFO; }
static int32_t list_y0(void) { return list_band_on() ? LS_GY + LS_GH : H_HEAD + 4; }   /* the first row: 64 / 20 */
static int32_t list_h(void) { return (240 - list_y0()) / 4; }                         /* .. a row: 44 / 55 */

/* row c: the label left, the value right (its unit small after it); st CV_HOT: the value inverted (touched, or a
 * step's lock), CV_DIM: dithered; no label: an empty row. A dotted rule under the first three */
static void lrow_draw(uint32_t c, const char *label, const char *val, const char *unit, uint32_t st)
{
    int32_t h = list_h(), y = list_y0() + (int32_t)c * h, lw, vw, uw, x, ty;
    uint32_t i, n, sc = 5u;
    char l[12], v[12];
    str_cpy(l, label, sizeof l);
    str_cpy(v, l[0] ? val : "", sizeof v);
    n = str_len(v);
    if (n > 1u && v[n - 1u] == '.')                  /* "-24." -> "-24" */
        v[n - 1u] = 0;
    if (!l[0])
        unit = "";
    n = rg_plan(&ls.sig[c], str_hash(str_hash(str_hash(0x51u + st * 977u + (uint32_t)h * 131u, l), v), unit), 0, y,
                240, h);
    if (!n)
        return;
    for (;;) {                                        /* the largest scale both fit at */
        lw = md_text_w(&MD_F3, sc, l);
        vw = md_text_w(&MD_F3, sc, v);
        uw = unit[0] ? md_text_w(&MD_F3, 2, unit) + 4 : 0;
        if (lw + 14 + vw + uw <= 228 || sc <= 2u)
            break;
        sc--;
    }
    ty = (h - 5 * (int32_t)sc) / 2;
    x = 234 - uw - vw;
    for (i = 0; i < n; i++) {
        rg_win(i, (uint32_t)y, C_BG);
        if (l[0]) {
            md_text(6, ty, &MD_F3, sc, l, C_INK);
            if (st == CV_HOT && v[0]) {
                cv_rect(x - 3, ty - 3, vw + 6, 5 * (int32_t)sc + 6, C_INK);
                md_text(x, ty, &MD_F3, sc, v, C_BG);
            } else {
                md_text(x, ty, &MD_F3, sc, v, C_INK);
                if (st == CV_DIM)                     /* half its pixels: the MD's grey */
                    cv_dither(x, ty, vw, 5 * (int32_t)sc, C_BG);
            }
            if (uw)
                md_text(238 - uw, ty + 5 * (int32_t)sc - 10, &MD_F3, 2, unit, C_INK);
        }
        if (c < 3u)
            cv_hdot(6, 233, h - 1, C_INK);
        cv_blit(0, (uint32_t)y);
    }
}

static uint32_t md_fit_sc(const char *s, int32_t w, uint32_t sc)   /* the largest scale <= sc that s fits w in */
{
    while (sc > 2u && md_text_w(&MD_F3, sc, s) > w)
        sc--;
    return sc;
}

/* TRACKS (FREITAKT redesign): the 6 tracks as a list under the top bar, a 36 px row each: the number, REC armed (a
 * dot: solid while playing, an outline stopped), the machine, its preset's name small at the right ("*": edited
 * since); a muted track's row inverted (MUTE, LEVEL 0 or another track's SOLO: trk_silent), the selected track a
 * pointer at the left. A row is sent when what it shows changes (rg_plan; round a popup up, its uncovered part) */
#define TK_Y0 (H_HEAD + 4)
#define TK_H 36
static uint32_t tk_sig[NTRK];
static void draw_tracks(void)
{
    uint32_t c, i, n, sc, sig;
    rg_fill(0, TK_Y0 + NTRK * TK_H, 240, 240 - (TK_Y0 + NTRK * TK_H), C_BG);   /* (y 236..239) */
    for (c = 0; c < NTRK; c++) {
        const track_t *t = &trk[c];
        uint32_t sel = c == song.sel, mute = !((uint32_t)t->p[P_LEVEL] & 127u) || trk_silent(t);
        uint32_t arm = (song.rec >> c) & 1u, pid = preset_of(t);
        int32_t y = TK_Y0 + (int32_t)c * TK_H, pw, r;
        uint16_t pen = mute ? C_BG : C_INK;
        const char *mn = N_MODEL[(uint32_t)t->p[P_MODEL] % NMODELS];
        char pn[12], nb[2] = {(char)('1' + c), 0};
        pn[0] = 0;
        if (pid != PRE_NONE) {
            preset_name_of(pid, pn);
            if (t->pre_ed)
                str_cpy(pn + str_len(pn), "*", 2);
        }
        sig = str_hash(str_hash(1u + sel + arm * 2u + mute * 4u + (arm && song.playing) * 8u, mn), pn);
        for (n = rg_plan(&tk_sig[c], sig, 0, y, 240, TK_H), i = 0; i < n; i++) {
            rg_win(i, (uint32_t)y, mute ? C_INK : C_BG);
            if (sel)                                  /* the pointer: a triangle 7 x 13 */
                for (r = 0; r < 13; r++)
                    cv_rect(3, 9 + r, r < 7 ? r + 1 : 13 - r, 1, pen);
            md_text(14, 8, &MD_F3, 4, nb, pen);
            if (arm) {
                if (song.playing)
                    cv_rect(32, 14, 7, 7, pen);
                else
                    cv_frame(32, 14, 7, 7, pen);
            }
            pw = pn[0] ? md_text_w(&MD_F3, 2, pn) : 0;
            sc = md_fit_sc(mn, 234 - 46 - (pw ? pw + 8 : 0), 4u);
            md_text(46, 28 - 5 * (int32_t)sc, &MD_F3, sc, mn, pen);
            if (pw)
                md_text(234 - pw, 18, &MD_F3, 2, pn, pen);
            if (mute)                                 /* (two muted rows apart) */
                cv_rect(0, TK_H - 1, 240, 1, C_BG);
            else if (c + 1u < NTRK)
                cv_hdot(6, 233, TK_H - 1, C_INK);
            cv_blit(0, (uint32_t)y);
        }
    }
}

/* a knob cell (spec 2026-10-19-md-ui §5, §6), 59 x 43 at x c*60+1: the label centred over a dotted rule; a
 * continuous parameter (ratio 0..1000): the MD knob, its LFO's live value (mod) a hollow dot outside it, the value
 * right of it; a discrete one (ratio < 0): its text centred, no knob. st CV_HOT: touched (the value inverted);
 * CV_DIM: inactive (the value dithered). At most 4 glyphs of the value. */
static void draw_column_m(uint32_t c, const char *label, const char *val, const char *unit, uint32_t st,
                          int32_t ratio, int32_t mod)
{
    char l[8], v[8], key[40];
    int32_t w = c == 3u ? 58 : 59, vw, vx;
    uint32_t touched = st == CV_HOT, dim = st == CV_DIM, n;
    md_fit(l, label, sizeof l, w - 2);
    md_fit(v, val, 5, ratio >= 0 ? 30 : w - 4);
    n = str_len(v);
    if (n > 1u && v[n - 1u] == '.')                  /* "-24." -> "-24" */
        v[n - 1u] = 0;
    str_cpy(key, l, 8);
    str_cpy(key + str_len(key), "|", 2);
    str_cpy(key + str_len(key), v, 8);
    n = str_len(key);
    key[n] = (char)('A' + touched + dim * 2u);
    key[n + 1] = (char)(' ' + (ratio < 0 ? 0 : 1 + ratio / 10));
    key[n + 2] = (char)(' ' + (mod < 0 ? 0 : 1 + mod / 10));
    key[n + 3] = 0;
    if (list_on()) {                                  /* a LIST page: a row (ui.col: what it shows, "NAME|value" and
                                                       * its state A plain, B touched, C dim, as a cell's) */
        str_cpy(ui.col[c], label, 12);
        str_cpy(ui.col[c] + str_len(ui.col[c]), "|", 2);
        str_cpy(ui.col[c] + str_len(ui.col[c]), val, 12);
        n = str_len(ui.col[c]);
        ui.col[c][n] = (char)('A' + touched + dim * 2u);
        ui.col[c][n + 1] = 0;
        lrow_draw(c, label, val, unit, st);
        return;
    }
    if (!ui.force && str_eq(key, ui.col[c]))
        return;
    str_cpy(ui.col[c], key, sizeof ui.col[c]);
    cv_begin_at((uint32_t)w, H_KNOB, Y_KNOB, C_BG);
    if (l[0]) {
        md_text((w - md_text_w(&MD_F3, 2, l)) / 2, 2, &MD_F3, 2, l, C_INK);
        cv_hdot(1, w - 2, 14, C_INK);
    }
    vw = md_text_w(&MD_F3, 2, v);
    if (ratio >= 0) {                                 /* the knob, the value right of it (4 glyphs fit, 3 px clear of
                                                       * the LFO dot) */
        cv_knob(14, 28, 8, ratio, mod, C_INK);
        vx = 57 - vw;
    } else {
        vx = (w - vw) / 2;
    }
    if (touched && v[0]) {
        cv_rect(vx - 2, 22, vw + 4, 13, C_INK);
        md_text(vx, 24, &MD_F3, 2, v, C_BG);
    } else {
        md_text(vx, 24, &MD_F3, 2, v, C_INK);
        if (dim)                                      /* half its pixels: the MD's grey */
            cv_dither(vx, 24, vw, 10, C_BG);
    }
    cv_blit(c * 60u + 1u, Y_KNOB);
}

static void draw_column(uint32_t c, const char *label, const char *val, const char *unit, uint32_t st, int32_t ratio)
{
    draw_column_m(c, label, val, unit, st, ratio, -1);
}

/* ACID's SOUND graph (spec §7): the filter's response at CUTOF / RESO (24 dB / octave, the RESO peak at the cutoff:
 * an approximation, as FILTER's), ENVMD's reach a dotted AMB line, 30 Hz .. 16 kHz across x 100..235 (15 px an
 * octave), +12 .. -32 dB (1 px a dB, 0 dB at y ACID_GY); under it the note's shape over 1 s (136 px): the amp gate
 * (DIM: GATE of a step at the tempo now) and the filter envelope falling over DECAY (HI) */
#define ACID_GY 14
#define ACID_EY 56                                      /* the shape's top; 36 px high */
static int32_t acid_cut_x(int32_t semis)                /* the cutoff MIDI 16 + semis -> px from 30 Hz (MIDI 23) */
{
    return clamp((16 + semis - 23) * 5 / 4, 0, 135);
}
/* KTRX: the pitch over TUNE (octaves, 12 px each: the drop and SWEEP over SDEC) and the level (the fade-in, DECAY)
 * over the first 300 ms, from the voice's own laws */
static int32_t ktrx_exp(uint32_t t, uint32_t tau)    /* e ^ (-t / tau), Q16 (t, tau in samples) */
{
    uint32_t e = pow2_q16(-(int32_t)clamp((int32_t)(t * 277u / tau), 0, 3072));
    return e > 65536u ? 65536 : (int32_t)e;          /* (pow2_q16(0) is 65549: 1 - e must not wrap) */
}
static void graph_ktrx(const track_t *t, uint16_t c)
{
    const int16_t *p = &t->p[P_E0];
    int32_t s = clamp(p[0], -24, 24), r = clamp(p[2], 0, 127), x, py = 0, pa = 0;
    uint32_t sc = pow2_q16(-s * 24);                     /* the drop re the pitch: 2 ^ (-s / 8) */
    uint32_t ta = ktrx_tau(2293u, (clamp(p[1], 0, 127) - 64) * 1518 / 100, 309u);
    uint32_t tr = ktrx_tau(4322u, (clamp(p[3], 0, 127) - 72) * 1427 / 100, 221u);
    uint32_t ramp = (uint32_t)(((uint64_t)(uint32_t)(r * r * r) * 23187u) >> 16);   /* 11.06 (r / 127) ^ 3, Q16 */
    uint32_t dq = (uint32_t)((34341ull * sc) >> 16), fq = (uint32_t)((173014ull * sc) >> 16);   /* 32.2, 162.1 / 61.4 */
    (void)c;
    md_text(100, 10, &MD_F3, 2, "PITCH / LEVEL", C_INK);
    cv_hdot(100, 235, 84, C_INK);
    for (x = 0; x < 136; x++) {
        uint32_t tt = (uint32_t)x * 97u;               /* 300 ms over 136 px */
        uint32_t q = 65536u + (uint32_t)(((uint64_t)dq * (uint32_t)ktrx_exp(tt, 213u)) >> 16) +
                     (uint32_t)(((uint64_t)fq * (uint32_t)ktrx_exp(tt, 158u)) >> 16) +
                     (uint32_t)(((uint64_t)ramp * (uint32_t)ktrx_exp(tt, tr)) >> 16);   /* the pitch re TUNE, Q16 */
        int32_t n = 0, oct, y, lv, ya;
        while (n < 15 && (q >> (n + 17)))
            n++;
        oct = n * 256 + (int32_t)(((q >> n) - 65536u) >> 8);   /* log2, Q8 (linear between octaves: a graph) */
        y = 84 - clamp(oct * 12 / 256, 0, 48);
        lv = (int32_t)(((uint32_t)ktrx_exp(tt, ta) * (uint32_t)(65536 - ktrx_exp(tt, 582u))) >> 16);
        ya = 84 - lv * 48 / 65536;
        if (x) {
            cv_line_dot(99 + x, pa, 100 + x, ya, C_INK);   /* the level: dotted */
            cv_line(99 + x, py, 100 + x, y, C_INK);        /* the pitch: solid */
        }
        py = y;
        pa = ya;
    }
}

static void graph_acid(const track_t *t, uint16_t c)
{
    const int16_t *p = &t->p[P_E0];
    int32_t cs = clamp(p[2], 0, 127) * 103 / 127, cx = acid_cut_x(cs), pk = -6 + clamp(p[3], 0, 127) * 18 / 127;
    int32_t x, y, py = 0, ex, gw, e = ENV1, kx = 65536;
    uint32_t k = acid_decay(p[1]), i;
    (void)c;
    cv_hdot(100, 235, ACID_GY, C_INK);                       /* 0 dB */
    for (x = 0; x < 136; x++) {
        int32_t dx = x - cx, db = dx <= -15 ? 0 : dx <= 0 ? pk * (15 + dx) / 15 : pk - dx * 24 / 15;
        y = ACID_GY - clamp(db, -32, 12);
        if (x)
            cv_line(99 + x, py, 100 + x, y, C_INK);
        py = y;
    }
    ex = acid_cut_x(cs + clamp(p[4], 0, 127) * 48 / 127);   /* ENVMD: the cutoff at the note's start */
    if (p[4] > 0)
        for (y = 2; y < 46; y += 4)
            cv_rect(100 + ex, y, 1, 2, C_INK);
    gw = clamp((int32_t)(acid_gate_len(t, trk_period(t, 0)) / 324u), 1, 135);   /* 324 samples a px */
    cv_hdot(100, 235, ACID_EY + 36, C_INK);
    cv_hdot(100, 100 + gw, ACID_EY, C_INK);                  /* the amp: open at once, GATE long, then closed */
    cv_vdot(100, ACID_EY, ACID_EY + 36, C_INK);
    cv_vdot(100 + gw, ACID_EY, ACID_EY + 36, C_INK);
    for (i = 0; i < 324u; i++)                               /* DECAY's factor per px */
        kx = mulq16(kx, k);
    for (x = 0; x < 136; x++) {
        y = ACID_EY + 36 - (int32_t)(((uint32_t)e >> 8) * 36u >> 16);
        if (x)
            cv_line(99 + x, py, 100 + x, y, C_INK);
        py = y;
        e = mulq16(e, (uint32_t)kx);
    }
}

/* the engine view's rendered hit (owner 2026-10-22: every model with no graph of its own, all but FM, ACID, BDTRX): the
 * voice's real sound, one hit at velocity 100 (as the keys play it) struck offline on a scratch track (drum_core.c
 * vg_hit: nothing live touched), its min / max over each block of CTL samples, then over each of the graph's 136
 * columns, normalised to its peak (a hat as tall as a kick: the shape, not the level; LEVEL is the mix's). The span
 * follows the sound: its length to -30 dB of its peak, + 1/4, rounded up to 100 150 200 300 400 500 750 1000 ms (a
 * rim's click and a cymbal's wash both fill the width; 1 s at most: the render stops there, or at the voice's end, or
 * after 100 ms under 1/64 of the peak). Rendered over the frames, VG_STEP samples each, while the engine view shows
 * (draw_graph), anew when the sound changes (vg_sig: MODEL, the 8 SOUND knobs, SAMPL's user samples); until it is done
 * the graph keeps the model's last one (no flicker while a knob turns); nothing on idle frames */
#define VG_NB (FS / CTL)                                /* blocks: 1 s at most */
#define VG_STEP 1024u                                   /* samples rendered a frame (~2.3 ms on the FM-1 at most) */
#define VG_VEL 100u
#define VG_QUIET (FS / 10u / CTL)                       /* 100 ms under 1/64 of the peak so far: the hit is over */
#define VG_CY 58                                        /* the centre line (graph y); the columns swing +- VG_A */
#define VG_A 28
static const uint16_t VG_SPAN[] = {100, 150, 200, 300, 400, 500, 750, 1000};
static struct {
    uint32_t sig, ok, shown;     /* the sound rendering; the sound the columns show (0: none); the one drawn */
    uint32_t n, work;            /* samples rendered: this hit; since power-on (the tests') */
    uint32_t loud;               /* the blocks up to the last at 1/64 of the peak so far */
    uint32_t span;               /* the columns' span, ms (0: the hit made no sound) */
    int32_t pk;                  /* the peak so far (>> 2, as the blocks) */
    uint8_t run, okm;            /* rendering; the columns' model */
    int16_t bmn[VG_NB], bmx[VG_NB];   /* each block's min / max (>> 2) */
    int8_t cmn[136], cmx[136];   /* the columns: px under / over the centre line */
} vg __attribute__((section(".pool")));

static int vg_model(uint32_t mi) { return mi != DM_FM && mi != DM_ACID && mi != DM_KTRX; }   /* (the others' own) */

static uint32_t vg_sig(const track_t *t)             /* what the hit is of: MODEL, the 8 SOUND knobs (+ SAMPL's samples) */
{
    uint32_t h = 2166136261u, i;
    for (i = P_MODEL; i <= P_E7; i++)
        h = (h ^ (uint32_t)(uint16_t)t->p[i]) * 16777619u;
    if ((uint32_t)t->p[P_MODEL] % NMODELS == DM_SMPL)
        h = (h ^ smp_user_gen) * 16777619u;
    return h | 1u;
}

static void vg_finish(uint32_t mi)                   /* the hit rendered: its span, its columns */
{
    uint32_t nb = vg.n / CTL, len = 0, b, c, k, nbs;
    int32_t pk = vg.pk;
    for (b = 0; b < nb; b++)
        if (pk && (vg.bmx[b] > -vg.bmn[b] ? vg.bmx[b] : -vg.bmn[b]) * 32 >= pk)
            len = b + 1u;
    len = len * CTL * 5u / 4u;
    for (k = 0; k + 1u < sizeof VG_SPAN / sizeof VG_SPAN[0] && VG_SPAN[k] * 441u / 10u < len; k++)
        ;
    nbs = VG_SPAN[k] * 441u / 10u / CTL;
    for (c = 0; c < 136u; c++) {
        uint32_t b0 = c * nbs / 136u, b1 = (c + 1u) * nbs / 136u;
        int32_t mn = 0, mx = 0;
        if (b1 <= b0)
            b1 = b0 + 1u;
        for (b = b0; b < b1 && b < nb; b++) {
            mn = vg.bmn[b] < mn ? vg.bmn[b] : mn;
            mx = vg.bmx[b] > mx ? vg.bmx[b] : mx;
        }
        vg.cmx[c] = (int8_t)(pk ? (mx * VG_A + pk / 2) / pk : 0);
        vg.cmn[c] = (int8_t)(pk ? -((-mn * VG_A + pk / 2) / pk) : 0);
    }
    vg.span = pk ? VG_SPAN[k] : 0u;
    vg.ok = vg.sig;
    vg.okm = (uint8_t)mi;
    vg.run = 0;
}

/* one frame's share of t's rendered hit (draw_graph, the engine view shown): a new sound starts it again; VG_STEP
 * samples at most; nothing once it is done */
static void vg_step(const track_t *t)
{
    uint32_t mi = (uint32_t)t->p[P_MODEL] % NMODELS, sig, k, i;
    int32_t b[CTL];
    if (!vg_model(mi)) {
        vg.run = 0;
        vg.sig = 0;
        return;
    }
    sig = vg_sig(t);
    if (sig != vg.sig) {
        vg.sig = sig;
        vg.n = 0;
        vg.pk = 0;
        vg.loud = 0;
        vg.run = 1;
        vg_hit(t, VG_VEL);
    }
    for (k = 0; vg.run && k < VG_STEP; k += CTL) {
        uint32_t on = vg_render(b, CTL), bi = vg.n / CTL;
        int32_t mn = 0, mx = 0, a;
        for (i = 0; i < CTL; i++) {
            mn = b[i] < mn ? b[i] : mn;
            mx = b[i] > mx ? b[i] : mx;
        }
        mn = clamp(mn >> 2, -32767, 0);
        mx = clamp(mx >> 2, 0, 32767);
        vg.bmn[bi] = (int16_t)mn;
        vg.bmx[bi] = (int16_t)mx;
        a = mx > -mn ? mx : -mn;
        vg.pk = a > vg.pk ? a : vg.pk;
        if (a && a * 64 >= vg.pk)
            vg.loud = bi + 1u;
        vg.n += CTL;
        vg.work += CTL;
        if (!on || !vg_trk.v[0].active || vg.n >= VG_NB * CTL || (vg.pk && bi + 1u >= vg.loud + VG_QUIET))
            vg_finish(mi);
    }
}

/* the rendered hit's graph (x 100..235): "HIT / 300 MS", a dotted centre line, each column's min .. max solid (a DAW's
 * waveform); a new model's first hit still rendering: the line only */
static void graph_wave(const track_t *t)
{
    uint32_t x, n;
    char b[20];
    cv_hdot(100, 235, VG_CY, C_INK);
    if (!vg.ok || vg.okm != (uint32_t)t->p[P_MODEL] % NMODELS)
        return;
    str_cpy(b, "HIT / ", sizeof b);
    if (vg.span) {
        fmt_int(b + 6, (int32_t)vg.span);
        n = str_len(b);
        str_cpy(b + n, " MS", sizeof b - n);
    } else {
        str_cpy(b + 6, "NO SOUND", sizeof b - 6);
    }
    md_text(100, 10, &MD_F3, 2, b, C_INK);
    vg.shown = vg.ok;
    for (x = 0; x < 136u; x++)
        if (vg.cmx[x] || vg.cmn[x])
            cv_rect(100 + (int32_t)x, VG_CY - vg.cmx[x], 1, vg.cmx[x] - vg.cmn[x] + 1, C_INK);
}

/* the track's preset (spec 2026-10-19 presets): "PRESET K909", "*" (amber) once a SOUND knob moved since; "PRESET -"
 * with none (or its user slot emptied, or holding another voice now: presets.c preset_of). At (x, y): an inverted
 * PRESET tab (label), the name in F3 at 2x, the mark after it */
static void preset_line(const track_t *t, int32_t x, int32_t y, int label)
{
    char nb[9];
    preset_name_of(preset_of(t), nb);
    if (label)
        x += cv_tab(x, y, "PRESET", C_INK, C_BG) + 4;
    x = md_text(x, y + 2, &MD_F3, 2, nb, C_INK);
    if (t->pre_ed)
        md_text(x + 2, y + 2, &MD_F3, 2, "*", C_INK);
}

/* the selected track's model (owner 2026-10-22: every model as FM, ACID, BDTRX): its preset in the band above
 * (preset_line); left, the name in F3 4x (5 glyphs, 76 px, end at x 80 at most), its place in the MODEL knob's list
 * under it ("#32/45", MODEL_LIST), voices, choke group, the layer under that; right (x 100..235) the graph: FM's
 * sidebands, ACID's filter, BDTRX's pitch and level, the others' rendered hit (graph_wave) */
static void graph_model(const track_t *t, uint16_t c)
{
    const dmodel_t *m = &DMODELS[(uint32_t)t->p[P_MODEL] % NMODELS];
    uint32_t mi = (uint32_t)t->p[P_MODEL] % NMODELS, n = model_list_idx(mi) + 1u;
    char b[16], nb[8];
    int32_t y = 50;
    preset_line(t, 4, 2 - G_OY, 1);
    nb[0] = '#';
    nb[1] = (char)('0' + n / 10u);
    nb[2] = (char)('0' + n % 10u);
    nb[3] = '/';
    nb[4] = (char)('0' + NMODELS / 10u);
    nb[5] = (char)('0' + NMODELS % 10u);
    nb[6] = 0;
    md_text(4, 10, &MD_F3, 4, m->name, C_INK);
    md_text(4, 34, &MD_F3, 2, nb, C_INK);
    md_text(4, y, &MD_F3, 2, m->voices > 1 ? "2 VOICES" : "1 VOICE", C_INK);
    if (t->p[P_CHOKE]) {
        str_cpy(b, "CHOKE 1", sizeof b);
        b[6] = (char)('0' + t->p[P_CHOKE]);
        y += 16;
        md_text(4, y, &MD_F3, 2, b, C_INK);
    }
    if (vg_model(mi))
        graph_wave(t);                               /* the rendered hit */
    if (mi == DM_FM) {                                 /* FM: the ratio, the carrier and its sidebands f +- k r f */
        uint32_t r8 = FM_RATIO_Q8[(uint32_t)clamp(t->p[P_E3], 0, 15)], k;
        int32_t h0 = clamp(t->p[P_E2], 0, 127) * 48 / 127;   /* INDEX 0: a pure sine, no sidebands */
        str_cpy(b, "RATIO ", sizeof b);
        str_cpy(b + 6, N_FMRATIO[(uint32_t)clamp(t->p[P_E3], 0, 15)], sizeof b - 6);
        md_text(100, 10, &MD_F3, 2, b, C_INK);
        cv_rect(100, 84, 136, 1, C_INK);                 /* the axis */
        cv_rect(100 + 17, 84 - 50, 2, 50, C_INK);        /* the carrier (x: 17 px per f, 0 .. 8 f) */
        for (k = 1; k <= 3u && h0 > 0; k++) {
            int32_t up = (int32_t)(256u + k * r8), dn = (int32_t)(256u) - (int32_t)(k * r8), hk = h0 / (int32_t)k;
            if (up * 17 / 256 < 136)
                cv_rect(100 + up * 17 / 256, 84 - hk, 2, hk, C_INK);
            dn = dn < 0 ? -dn : dn;
            if (dn * 17 / 256 < 136)                     /* the lower sidebands: dithered */
                cv_dither(100 + dn * 17 / 256, 84 - hk, 2, hk, C_INK);
        }
    }
    if (mi == DM_ACID)
        graph_acid(t, c);                            /* ACID: the filter and the note's shape */
    if (mi == DM_KTRX)
        graph_ktrx(t, c);                            /* KTRX: the pitch and the level */
}

/* OFF (a type / model switched off): F3 at 6x centred, a 3-row dither band above and below it (graph rows) */
static void off_word(void)
{
    int32_t w = md_text_w(&MD_F3, 6, "OFF"), x = (240 - w) / 2;
    md_text(x, 35, &MD_F3, 6, "OFF", C_INK);
    cv_dither(x - 6, 30, w + 12, 3, C_INK);
    cv_dither(x - 6, 67, w + 12, 3, C_INK);
}

/* a horizontal meter (the MD's LEV, lying down): v (0..1000) of w px solid, 9 px tall, 2 px tick dashes every 6 px
 * above (y - 3) and below (y + 10); dith: the fill dithered (a second meter beside a solid one) */
static void hmeter(int32_t x, int32_t y, int32_t w, int32_t v, int dith)
{
    int32_t t;
    v = clamp(v, 0, 1000);
    if (dith)
        cv_dither(x, y, w * v / 1000, 9, C_INK);
    else
        cv_rect(x, y, w * v / 1000, 9, C_INK);
    for (t = x; t <= x + w; t += 6) {
        cv_rect(t, y - 3, 1, 2, C_INK);
        cv_rect(t, y + 10, 1, 2, C_INK);
    }
}

static void graph_fx(const track_t *t, uint16_t c)
{
    uint32_t i;
    int32_t y;
    (void)c;
    for (i = 0; i < 4u; i++) {                       /* the sends as MD LEV bars: solid from the bottom, tick dashes */
        int32_t h = t->p[P_DIST + i] * 80 / 127, x = (int32_t)i * 60 + 28;
        cv_rect(x - 3, 90 - h, 7, h, C_INK);
        for (y = 10; y <= 90; y += 6) {
            cv_rect(x - 7, y, 2, 1, C_INK);
            cv_rect(x + 6, y, 2, 1, C_INK);
        }
    }
}

/* oscilloscope of the output, triggered on a rising zero crossing: a 1 px wave round cy (+- amp), a dotted zero
 * line */
static void graph_scope(int32_t cy, int32_t amp, int fresh)   /* fresh: a new snapshot (else the one shown again) */
{
    static int16_t snap[SCOPE_N];
    uint32_t w = scope_w, i, trig = 0;
    int32_t py = cy, x, peak = 1500;
    if (fresh)
        for (i = 0; i < SCOPE_N; i++)
            snap[i] = scope_buf[(w + i) & (SCOPE_N - 1u)];
    for (i = 0; i < SCOPE_N; i++) {
        if (snap[i] > peak)
            peak = snap[i];
        else if (-snap[i] > peak)
            peak = -snap[i];
    }
    for (i = 1; i < SCOPE_N - 240u; i++)
        if (snap[i - 1] < 0 && snap[i] >= 0) {
            trig = i;
            break;
        }
    cv_hdot(2, 237, cy, C_INK);
    for (x = 2; x < 238; x++) {
        int32_t y = cy - snap[trig + (uint32_t)x] * amp / peak;   /* auto-scaled */
        if (x > 2)
            cv_line(x - 1, py, x, y, C_INK);
        py = y;
    }
}


/* a list row (spec 2026-10-19-md-ui §8): l from x0 + 4, r right-aligned at x1 - 4; sel: an ink bar 14 px from y - 2,
 * the text in BG */
static void list_row(int32_t x0, int32_t x1, int32_t y, const char *l, const char *r, int sel)
{
    uint16_t pen = sel ? C_BG : C_INK;
    if (sel)
        cv_rect(x0, y - 2, x1 - x0 + 1, 14, C_INK);
    md_text(x0 + 4, y, &MD_F3, 2, l, pen);
    if (r && r[0])
        md_text(x1 - 4 - md_text_w(&MD_F3, 2, r), y, &MD_F3, 2, r, pen);
}

#define MX_LW 34                                        /* a level meter's width (COMP's IN) */
static int32_t meter_px(int32_t a)                   /* |sample| (Q15) -> px, 6 dB = MX_LW / 10 */
{
    int32_t lg = 0, v;
    if (a < 64)
        return 0;
    while ((a >> lg) > 1)
        lg++;
    v = lg * 8 + (((a << 3) >> lg) & 7);
    return clamp((v - 48) * MX_LW / 80, 0, MX_LW);
}

/* the COMP source in the mix: HEARD, GHOST (keys the compressor, not heard: muted with GHOST KEEP, or HIDE),
 * MUTED (GHOST MUTE: keys nothing); "" with SRC OFF */
static const char *comp_src_state(void)
{
    uint32_t src = comp_src();
    if (src >= NTRK)
        return "";
    if (trk_silent(&trk[src]))
        return song.g[G_CGHOST] == CG_MUTE ? "MUTED" : "GHOST";
    return song.g[G_CGHOST] == CG_HIDE ? "GHOST" : "HEARD";
}

/* COMP pages: the routing, the gain reduction now and the source's level; page 2 adds the curve (input -> output
 * level, -48..0 dB in, -48..+12 dB out) for the knobs' THRSH / RATIO / MKUP / KNEE */
static void graph_comp(uint16_t c, int curve)
{
    char r[48], v[12];
    uint32_t k, src = comp_src(), n, any = 0;
    int32_t dbx10 = comp_on() ? (-comp.gr) * 241 / 32768 : 0;
    str_cpy(r, "SRC: ", sizeof r);
    n = str_len(r);
    if (src < NTRK) {
        r[n++] = 'T';
        r[n++] = (char)('1' + src);
        r[n] = 0;
    } else {
        str_cpy(r + n, "OFF", sizeof r - n);
    }
    str_cpy(r + str_len(r), "  DUCK:", sizeof r - str_len(r));
    n = str_len(r);
    for (k = 0; k < NTRK && n + 4u < sizeof r; k++)
        if (trk[k].p[P_DUCK] && k != src) {
            r[n++] = ' ';
            r[n++] = 'T';
            r[n++] = (char)('1' + k);
            r[n] = 0;
            any = 1;
        }
    if (!any)
        str_cpy(r + n, " -", sizeof r - n);
    md_text(4, 2, &MD_F3, 2, r, C_INK);
    md_text(4, 27, &MD_F3, 2, "GR", C_INK);
    hmeter(36, 27, curve ? 70 : 120, clamp(dbx10, 0, 240) * 1000 / 240, 0);
    comp_fmt_db10(v, -dbx10);
    str_cpy(v + str_len(v), "DB", sizeof v - str_len(v));
    md_text(curve ? 36 : 162, curve ? 42 : 27, &MD_F3, 2, v, C_INK);
    md_text(4, curve ? 65 : 52, &MD_F3, 2, "IN", C_INK);
    hmeter(36, curve ? 65 : 52, curve ? 70 : 120, (comp_on() ? meter_px(comp.peak) : 0) * 1000 / MX_LW, 1);
    {                                                   /* the source in the mix, next to its level (IN) */
        const char *st = comp_src_state();
        md_text(curve ? 36 : 162, curve ? 84 : 52, &MD_F3, 2, st, C_INK);
    }
    if (curve) {                                        /* the curve in a box, a dotted 4 x 4 grid */
        comp_set_t s = comp_knobs();
        comp_cfg_t cf;
        comp_configure(&s, &cf);
        cv_frame(118, 22, 119, 77, C_INK);
        for (k = 1; k < 4u; k++) {
            cv_vdot(118 + (int32_t)k * 119 / 4, 24, 96, C_INK);
            cv_hdot(120, 235, 22 + (int32_t)k * 77 / 4, C_INK);
        }
        for (k = 0; k < 116u; k++) {
            int32_t in10 = -480 + (int32_t)k * 480 / 116, lvl = in10 * 65536 / 60;
            int32_t out10 = (lvl + comp_atten(&cf, lvl) + cf.makeup) * 60 / 65536;
            int32_t y = 98 - (out10 + 480) * 76 / 600;
            if (y >= 23 && y <= 96)
                cv_rect(120 + (int32_t)k, y, 1, 2, C_INK);
        }
    }
    (void)c;
}

static int lfo_random(uint32_t wave) { return wave == LW_SH || wave == LW_WANDER || wave == LW_RWALK; }

/* every frame on an LFO page: each LFO's trail gets a point each 1/LTR_N of its cycle (the width = one cycle; at most
 * one a frame); a trail starts over on another track or another wave of its LFO, or after frames away from the LFO
 * pages */
static void lfo_trail(void)
{
    const track_t *t = TSEL;
    uint32_t l, cont = ui.frame == ui.tr_frame + 1u;
    ui.tr_frame = ui.frame;
    for (l = 0; l < 2u; l++) {
        uint32_t ph = t->lfo[l].ph;
        uint32_t key = 1u << 16 | (uint32_t)song.sel << 8 | (uint32_t)clamp(t->p[(l ? P_LFO2 : P_LFO1) + LF_WAVE], 0, 15);
        if (key != ui.tr_key[l] || !cont) {
            ui.tr_key[l] = key;
            ui.tr_n[l] = ui.tr_h[l] = 0;
        }
        if (ui.tr_n[l] && ph - ui.tr_ph[l] < 0xFFFFFFFFu / LTR_N)
            continue;
        ui.tr_ph[l] = ph;
        ui.tr[l][ui.tr_h[l]] = (int16_t)(lfo_out(t, l) >> 1);
        ui.tr_h[l] = (uint8_t)((ui.tr_h[l] + 1u) % LTR_N);
        if (ui.tr_n[l] < LTR_N)
            ui.tr_n[l]++;
    }
}

/* the routing of LFO l under its picture: "> TONE" (one slot), "> 2 DEST" (more), "> OFF" (none) */
static void lfo_route_text(const track_t *t, uint32_t l, char *b, uint32_t n)
{
    uint32_t s, cnt = 0, first = 0;
    for (s = 0; s < LS_N; s++)
        if (t->p[LS_ID[s][LS_DEST]] && t->p[LS_ID[s][LS_DEPTH]] && (t->p[LS_ID[s][LS_SRC]] ? 1u : 0u) == l && !cnt++)
            first = s;
    str_cpy(b, "> ", n);
    if (cnt == 1u)
        str_cpy(b + 2, lfo_dest_label(t, t->p[LS_ID[first][LS_DEST]]), n - 2u);
    else if (cnt) {
        fmt_int(b + 2, (int32_t)cnt);
        str_cpy(b + str_len(b), " DEST", n - str_len(b));
    } else
        str_cpy(b + 2, "OFF", n - 2u);
}

/* LFO pages: both LFOs side by side, LFO 1 over KNOB 1-2, LFO 2 over KNOB 3-4: the name and the rate mode; one cycle
 * of the wave (as morphed, phase-shifted) with the live position, or for the random waves a scope (the live value at
 * the half's right edge, its trail to the left); TRIG and the routing (the DEST slots it feeds) */
static void graph_lfo_half(const track_t *t, uint32_t l)
{
    const int16_t *q = &t->p[l ? P_LFO2 : P_LFO1];
    uint32_t wave = (uint32_t)clamp(q[LF_WAVE], 0, LW_COUNT - 1), k, off = (uint32_t)clamp(q[LF_PHASE], 0, 127) << 25;
    int32_t m = clamp(q[LF_MORPH], 0, 127), x0 = l ? 120 : 0, gx = x0 + (int32_t)LG_X0, px, dx, dy;
    const char *mode = N_LMODE[clamp(q[LF_MODE], 0, 2)], *trig = N_LTRIG[clamp(q[LF_TRIG], 0, 2)];
    char b[16];
    md_text(x0 + 6, 2, &MD_F3, 2, l ? "LFO 2" : "LFO 1", C_INK);
    md_text(x0 + 114 - md_text_w(&MD_F3, 2, mode), 2, &MD_F3, 2, mode, C_INK);
    cv_hdot(gx, gx + (int32_t)LTR_N - 1, LG_CY, C_INK);
    if (lfo_random(wave)) {
        for (k = 0, px = 0; k < ui.tr_n[l]; k++) {   /* newest at the right edge, older to the left */
            int32_t py = LG_CY - ui.tr[l][(ui.tr_h[l] + LTR_N - 1u - k) % LTR_N] * (2 * LG_A) / 32767;
            if (!k)
                px = py;
            cv_rect(gx + (int32_t)LTR_N - 1 - (int32_t)k, py < px ? py : px, 1, (py < px ? px - py : py - px) + 2, C_INK);
            px = py;
        }
        dx = gx + (int32_t)LTR_N - 1;
    } else {
        for (k = 0, px = 0; k < LTR_N; k++) {        /* px: the previous point's y (joined: no gaps at edges); the
                                                       * area to the zero line dithered */
            int32_t py = LG_CY - lfo_shape(wave, m, k * (0xFFFFFFFFu / LTR_N) + off) * LG_A / 32767;
            if (!k)
                px = py;
            if (py < LG_CY - 1)
                cv_dither(gx + (int32_t)k, py + 2, 1, LG_CY - py - 2, C_INK);
            else if (py > LG_CY + 1)
                cv_dither(gx + (int32_t)k, LG_CY + 1, 1, py - LG_CY - 1, C_INK);
            cv_rect(gx + (int32_t)k, py < px ? py : px, 1, (py < px ? px - py : py - px) + 2, C_INK);
            px = py;
        }
        k = t->lfo[l].ph / (0xFFFFFFFFu / LTR_N);
        dx = gx + (int32_t)(k < LTR_N ? k : LTR_N - 1u);
    }
    dy = LG_CY - lfo_out(t, l) * LG_A / 32767;
    cv_rect(dx - 3, dy - 3, 7, 7, C_BG);             /* the live value: 5x5 solid, a clear ring round it */
    cv_rect(dx - 2, dy - 2, 5, 5, C_INK);
    md_text(x0 + 6, 86, &MD_F3, 2, trig, C_INK);
    lfo_route_text(t, l, b, sizeof b);
    md_text(x0 + 114 - md_text_w(&MD_F3, 2, b), 86, &MD_F3, 2, b, C_INK);
}
static void graph_lfo(uint16_t c)
{
    (void)c;
    graph_lfo_half(TSEL, 0);
    graph_lfo_half(TSEL, 1);
    cv_vdot(120, 2, 98, C_INK);                       /* between the halves (KNOB 2 | KNOB 3) */
}

/* a DEST slot's modulation now: its source's last value x DEPTH, Q15 (0: off) */
static int32_t lfo_slot_now(const track_t *t, uint32_t s)
{
    if (!t->p[LS_ID[s][LS_DEST]] || !t->p[LS_ID[s][LS_DEPTH]])
        return 0;
    return lfo_out(t, t->p[LS_ID[s][LS_SRC]] ? 1u : 0u) * clamp(t->p[LS_ID[s][LS_DEPTH]], -64, 64) / 64;
}

/* LFO DEST: the 4 slots as rows ("3  LFO2  > DECAY  -40%" and a meter of the slot's modulation now, from its middle);
 * the selected slot (KNOB 1) inverted */
static void graph_ldest(void)
{
    const track_t *t = TSEL;
    uint32_t s;
    char b[16], v[12];
    const char *u;
    for (s = 0; s < LS_N; s++) {
        int32_t y = 6 + (int32_t)s * 24, w = lfo_slot_now(t, s) * 20 / 32767;
        uint16_t pen = s == (song.lslot & 3u) ? C_BG : C_INK;
        if (s == (song.lslot & 3u))
            cv_rect(1, y - 3, 238, 18, C_INK);
        b[0] = (char)('1' + s);
        b[1] = 0;
        md_text(8, y, &MD_F3, 2, b, pen);
        md_text(28, y, &MD_F3, 2, N_LSRC[t->p[LS_ID[s][LS_SRC]] ? 1 : 0], pen);
        str_cpy(b, "> ", sizeof b);
        str_cpy(b + 2, lfo_dest_label(t, t->p[LS_ID[s][LS_DEST]]), sizeof b - 2u);
        md_text(76, y, &MD_F3, 2, b, pen);
        param_format(&TP[P_LS3DEPTH], t->p[LS_ID[s][LS_DEPTH]], v, &u);
        str_cpy(v + str_len(v), u, sizeof v - str_len(v));
        md_text(186 - md_text_w(&MD_F3, 2, v), y, &MD_F3, 2, v, pen);
        cv_rect(214, y, 1, 11, pen);                  /* the meter's middle; the modulation from it */
        if (w)
            cv_rect(w > 0 ? 215 : 214 + w, y + 2, w > 0 ? w : -w, 7, pen);
    }
    cv_vdot(22, 3, 97, C_INK);                        /* the column rules: slot | source | dest, depth | meter */
    cv_vdot(70, 3, 97, C_INK);
    cv_vdot(192, 3, 97, C_INK);
}

/* FILTER: TYPE's response at CUT / RESO (an approximation: flat, the RESO peak, 12 dB / octave; BP 6 dB; the
 * notch's width by RESO), 30 Hz .. 16 kHz across x 100..235 (15 px an octave), +18 .. -36 dB; ENV's reach a
 * dotted line at CUT + ENV */
static int32_t flt_resp_db(uint32_t ty, int32_t dx, int32_t reso)   /* dx: px from the cutoff (15 an octave) */
{
    int32_t pk = -6 + reso * 24 / 127, ad = dx < 0 ? -dx : dx, w = 4 + (127 - reso) / 8;
    if (ty == FT_HP)
        dx = -dx;
    if (ty == FT_LP || ty == FT_HP)
        return dx <= -15 ? 0 : dx <= 0 ? pk * (15 + dx) / 15 : pk - dx * 12 / 15;
    if (ty == FT_BP)
        return pk - ad * 6 / 15;
    return ad >= w ? 0 : -36 + ad * 36 / w;                    /* NOTCH */
}

static void graph_filter(uint16_t c)
{
    const track_t *t = TSEL;
    uint32_t ty = (uint32_t)clamp(t->p[P_FTYPE], 0, FT_N - 1);
    int32_t cx = clamp(t->p[P_FCUT], 0, 127) * 135 / 127, x, py = 0, ex;
    char b[16];
    const char *u;
    (void)c;
    if (!ty) {
        off_word();
        return;
    }
    md_text(4, 4, &MD_F3, 4, N_FTYPE[ty], C_INK);
    param_format(&TP[P_FCUT], t->p[P_FCUT], b, &u);
    str_cpy(b + str_len(b), u, sizeof b - str_len(b));    /* "1.0kHz" */
    md_text(4, 44, &MD_F3, 2, b, C_INK);
    cv_hdot(100, 235, 28, C_INK);                              /* 0 dB */
    for (x = 0; x < 136; x++) {
        int32_t db = clamp(flt_resp_db(ty, x - cx, clamp(t->p[P_FRESO], 0, 127)), -36, 18);
        int32_t y = 28 - db * 3 / 2;
        if (x)
            cv_line(99 + x, py, 100 + x, y, C_INK);
        py = y;
    }
    ex = clamp(t->p[P_FCUT] + t->p[P_FENV], 0, 127) * 135 / 127;   /* ENV: the cutoff at the hit */
    if (t->p[P_FENV])
        for (x = 10; x < 84; x += 4)
            cv_rect(100 + ex, x, 1, 2, C_INK);
}

/* GLO > DLY (both pages): the TIME large, its length in ms and the MODE under it; at the right the echo train over
 * 8 delay periods: the hit (dithered), then the echoes, the first at MIX, each next one FDBK times the last (fx.c
 * fx_buses' gains), dithered more the darker COLR leaves them; NORM from the floor, PING above (L) and below (R) the
 * middle line in turn */
static void graph_delay(void)
{
    uint32_t k, ms = delay_samples() * 1000u / FS, ping = song.g[G_DMODE] != 0;
    int32_t a = song.g[G_DMIX] * 258, fb = song.g[G_DFDBK] * 230, dark = 127 - song.g[G_DCOLOR], mid = 47;
    char b[16];
    md_text(4, 4, &MD_F3, 4, N_DTIME[(uint32_t)song.g[G_DTIME] % (sizeof N_DTIME / sizeof N_DTIME[0])], C_INK);
    fmt_int(b, (int32_t)ms);
    str_cpy(b + str_len(b), " MS", sizeof b - str_len(b));
    md_text(4, 44, &MD_F3, 2, b, C_INK);
    md_text(4, 60, &MD_F3, 2, ping ? "PING-PONG" : "NORMAL", C_INK);
    if (ping) {
        cv_hdot(100, 235, mid, C_INK);
        md_text(226, 4, &MD_F3, 2, "L", C_INK);
        md_text(226, 78, &MD_F3, 2, "R", C_INK);
    } else {
        cv_hdot(100, 235, 88, C_INK);
    }
    cv_dither(101, ping ? mid - 40 : 8, 3, ping ? 80 : 80, C_INK);   /* the hit */
    for (k = 1; k <= 8u; k++) {
        int32_t x = 100 + (int32_t)k * 16, h = (a >> 6) * (ping ? 40 : 80) / 512;   /* (a: Q15) */
        if (h < 1)
            break;
        if (ping) {
            if (k & 1u)
                cv_rect(x, mid - h, 3, h, C_INK);
            else
                cv_rect(x, mid + 1, 3, h, C_INK);
        } else {
            cv_rect(x, 88 - h, 3, h, C_INK);
        }
        if (k > 1u && dark * (int32_t)(k - 1u) > 96)   /* COLR: the later repeats duller */
            cv_dither(x - 1, ping ? ((k & 1u) ? mid - h : mid + 1) : 88 - h, 5, h, C_BG);
        a = mulq15(a, fb);
    }
}

/* GLO > REVERB: the TYPE large and its decay time (RT: to -60 dB) under it; at the right the tail over 4 s (a tick a
 * second) in dB, 60 dB tall: the line is the whole sound's decay (ROOM: the combs' loop gain from SIZE over their mean
 * length; SPRNG: the spring's, with its drips at every round trip; HALL: its lines' gain (fx.c hall_gain) over their
 * mean length, at half the rate), the solid under it its top end, which DAMP makes fade sooner. The loop's gain g over L samples: -60 dB after 6.9 L / -ln g samples, -ln g = d + d^2 / 2, d = 1 - g */
static int32_t rev_t60(int32_t g, int32_t len)           /* g Q15, len samples -> samples to -60 dB */
{
    int32_t d = 32768 - g, q = d + ((d * d) >> 16);         /* -ln g, Q15 */
    return q > 0 ? (int32_t)((uint32_t)len * 226099u / (uint32_t)q) : 1 << 24;   /* (6.9 in Q15) */
}
static void graph_reverb(void)
{
    uint32_t ty = (uint32_t)clamp(song.g[G_RTYPE], 0, 2), spring = ty == 1u, s = (uint32_t)song.g[G_RSIZE], x, drip;
    int32_t damp = song.g[G_RDAMP], g, len, t60, tb, px, pb, py = 0;
    char b[16];
    if (spring) {
        g = 19661 + (int32_t)s * 85;
        len = 1323 + (int32_t)((s * 1323u) >> 7);
    } else if (ty == 2u) {                                   /* HALL: a line of the mean length (609 at 22.05 kHz) */
        g = (int32_t)hall_gain(609u, exp2_neg(s * 65536u / 30u));
        len = 2 * 609;
    } else {
        g = 25000 + (int32_t)s * 50;
        len = (1116 + 1188 + 1277 + 1356) / 4;
    }
    t60 = rev_t60(g, len);
    tb = t60 * (170 - damp) / 170;                           /* DAMP: the top end fades sooner */
    px = t60 / 1297 > 1 ? t60 / 1297 : 1;                    /* (4 s = 176400 samples over 136 px) */
    pb = tb / 1297 > 1 ? tb / 1297 : 1;
    drip = (uint32_t)(len / 1297 + 1);
    md_text(4, 4, &MD_F3, 4, N_RTYPE[ty], C_INK);
    str_cpy(b, "RT ", sizeof b);
    fmt_int(b + 3, t60 / 44100);
    x = str_len(b);
    b[x] = '.';
    b[x + 1] = (char)('0' + t60 * 10 / 44100 % 10);
    str_cpy(b + x + 2, " S", sizeof b - x - 2);
    md_text(4, 44, &MD_F3, 2, b, C_INK);
    cv_hdot(100, 235, 88, C_INK);
    for (x = 0; x <= 4u; x++)
        cv_rect(100 + (int32_t)x * 34, 86, 1, 5, C_INK);
    for (x = 0; x < 136u; x++) {
        int32_t h = (int32_t)x < px ? 78 - 78 * (int32_t)x / px : 0, hb = (int32_t)x < pb ? 78 - 78 * (int32_t)x / pb : 0, y;
        if (spring && h > 6 && x % drip == 0u)
            h += 6;                                          /* a drip */
        y = 88 - h;
        if (x)
            cv_line(99 + (int32_t)x, py, 100 + (int32_t)x, y, C_INK);
        py = y;
        if (hb > 0)
            cv_rect(100 + (int32_t)x, 88 - hb, 1, hb, C_INK);
        if (h > hb + 1)
            cv_dither(100 + (int32_t)x, 89 - h, 1, h - hb - 1, C_INK);
    }
}

/* GLO > CHORUS: the RATE large and the delay's swing in ms under it; at the right the chorus' modulated delay over 4 s
 * (a tick a second): the sine fx.c cho_tick sweeps, from its shortest (the dotted line, DEPTH 0) up by DEPTH, 0 ..
 * 30 ms tall; past ~8 Hz (over 34 cycles in 136 px) the band it fills, dithered */
static void graph_chorus(void)
{
    uint32_t r = (uint32_t)song.g[G_CRATE] & 127u, hz = LFO_HZ_X100[r], x;
    int32_t lo = 400, hi = 400 + song.g[G_CDEPTH] * 6, py = 0;   /* (cho_tick: 400 + up to cdepth samples) */
    int32_t ylo = 88 - lo * 80 / 1323, yhi = 88 - hi * 80 / 1323;   /* (30 ms = 1323 samples over 80 px) */
    char b[16];
    const char *u;
    param_format(&GP[G_CRATE], song.g[G_CRATE], b, &u);
    x = (uint32_t)md_text(4, 4, &MD_F3, 4, b, C_INK);
    md_text((int32_t)x + 4, 30, &MD_F3, 2, "HZ", C_INK);
    fmt_int(b, lo * 1000 / FS);
    str_cpy(b + str_len(b), "-", sizeof b - str_len(b));
    fmt_int(b + str_len(b), hi * 1000 / FS);
    str_cpy(b + str_len(b), " MS", sizeof b - str_len(b));
    md_text(4, 44, &MD_F3, 2, b, C_INK);
    cv_hdot(100, 235, ylo, C_INK);
    cv_rect(100, 88, 136, 1, C_INK);
    for (x = 0; x <= 4u; x++)
        cv_rect(100 + (int32_t)x * 34, 86, 1, 5, C_INK);
    if (hz > 850u) {                                         /* too fast to draw: the band it sweeps */
        cv_dither(100, yhi, 136, ylo - yhi + 1, C_INK);
        cv_rect(100, yhi, 136, 1, C_INK);
        return;
    }
    for (x = 0; x < 136u; x++) {                             /* phase: x / 34 s at hz / 100, 2^32 a cycle */
        uint32_t ph = x * hz * 1263226u;                     /* (2^32 / 3400, wrapping) */
        int32_t y = ylo - (((osc_sine(ph - 0x40000000u) + 32768) * (ylo - yhi)) >> 16);
        if (x)
            cv_line(99 + (int32_t)x, py, 100 + (int32_t)x, y, C_INK);
        py = y;
    }
}

/* INSERT 1/2 and 2/2 (insert.c): the TYPE in large, MIX and SRR under it; at the right a sine's cycle as the track's
 * chain bends it (SRR's steps, then the drive's / CRUSH's curve at MIX, after Skiff's graph_insert), or for the swept
 * ones their sweep over two cycles (PHASR and FLANG a triangle, FLANG's squared, CHOR a sine, as deep as DEPTH). Both
 * off: OFF */
static void graph_insert(void)
{
    const track_t *t = TSEL;
    uint32_t ty = (uint32_t)clamp(t->p[P_ITYPE], 0, IT_N - 1), i;
    int32_t a = t->p[P_IA], bb = t->p[P_IB], cc = t->p[P_IC], mix = ins_mixq(t->p[P_IMIX]), srr = t->p[P_SRR];
    int32_t hold = 1 + srr_period(srr) / 1024, py = 0, h = 0;   /* (SRR's steps, drawn: 1 .. 32 px) */
    char b[16];
    const char *u;
    if (!ty && !srr) {
        off_word();
        return;
    }
    md_text(4, 4, &MD_F3, 4, N_ITYPE[ty], C_INK);
    str_cpy(b, "MIX ", sizeof b);
    param_format(&TP[P_IMIX], t->p[P_IMIX], b + 4, &u);
    str_cpy(b + str_len(b), u, sizeof b - str_len(b));
    if (ty)
        md_text(4, 30, &MD_F3, 2, b, C_INK);
    str_cpy(b, "SRR ", sizeof b);
    param_format(&TP[P_SRR], srr, b + 4, &u);
    str_cpy(b + str_len(b), u, sizeof b - str_len(b));
    md_text(4, 44, &MD_F3, 2, b, C_INK);
    cv_hdot(100, 235, 46, C_INK);                                /* 0 */
    for (i = 0; i < 136u; i++) {
        int32_t y, x = sine_i(i * 31580641u) * 3 / 4;        /* (one cycle across 136 px) */
        if (ty >= IT_PHASER) {
            uint32_t ph = (uint32_t)((i % 68u) * 65536u / 68u) << 16;
            int32_t sw = ty == IT_CHORUS ? osc_sine(ph) : (int32_t)ins_tri(ph) - 32768;
            if (ty == IT_FLANGER)
                sw = (int32_t)(((uint32_t)(sw + 32768) * (uint32_t)(sw + 32768)) >> 16) - 32768;
            y = sw * bb / 127 * 24000 / 32768;
        } else {
            if (!(i % (uint32_t)hold))
                h = x;                                       /* SRR: held */
            y = h;
            if (ty == IT_CRUSH)
                y = ins_shape(IT_CRUSH, h, ins_bits_sh(a));
            else if (ty)
                y = (ins_shape(ty, ((h >> 3) * (4096 + a * a * 3)) >> 9, 0) * LEVEL_Q12[cc & 127]) >> 12;
            y = h + (int32_t)(((int64_t)(clamp(y, -40000, 40000) - h) * mix) >> 15);
        }
        y = clamp(46 - y * 36 / 24000, 4, 88);
        if (i)
            cv_line(99 + (int32_t)i, py, 100 + (int32_t)i, y, C_INK);
        py = y;
    }
}

static uint32_t graph_signature(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint32_t h = 2166136261u, i;
    h ^= (uint32_t)pg->graph * 131u + song.sel * 7777u + ui.bank * 104729u;
    for (i = 0; i < P_COUNT; i++)
        h = (h ^ (uint32_t)t->p[i]) * 16777619u;
    h ^= (uint32_t)song.g[G_SLOT] * 13u;
    if (pg->graph == GR_CHO)                           /* the chorus' globals */
        h = (h ^ (uint32_t)song.g[G_CRATE] * 23u ^ (uint32_t)song.g[G_CDEPTH] * 7919u) * 16777619u;
    if (pg->graph == GR_DLY || pg->graph == GR_REV)    /* the buses' globals (and the tempo: TIME in ms) */
        for (i = G_DTIME; i <= G_DMIX; i++)
            h = (h ^ (uint32_t)song.g[i] + (uint32_t)song.g[G_DMODE] * 7u + (uint32_t)song.g[G_RTYPE] * 11u +
                 (uint32_t)song.g[G_RSIZE] * 13u + (uint32_t)song.g[G_RDAMP] * 17u + (uint32_t)song.g[G_BPM] * 19u) *
                16777619u;
    if (pg->graph == GR_MODEL) {
        char nb[9];
        h ^= (uint32_t)song.g[G_BPM] * 40503u + vg.ok * 2654435761u;   /* ACID's gate: GATE of a step at the tempo; the
                                                                          * rendered hit done */
        preset_name_of(preset_of(t), nb);             /* the preset's line (its user slot may change) */
        h = str_hash(h ^ t->pre_ed * 7919u, nb);
    }
    if (pg->graph == GR_COMP) {
        for (i = G_CSRC; i <= G_CGHOST; i++)
            h = (h ^ (uint32_t)song.g[i]) * 16777619u;
        for (i = 0; i < NTRK; i++)
            h = (h ^ (uint32_t)(trk[i].p[P_DUCK] | trk_silent(&trk[i]) << 1)) * 16777619u;
        h ^= ui.page * 389u;
        if (comp_on())
            h ^= ((uint32_t)(-comp.gr) >> 7) * 2654435761u + (uint32_t)meter_px(comp.peak) * 40503u;
    }
    if (pg->graph == GR_LFO || pg->graph == GR_LDEST) {
        uint32_t l;
        h ^= ui.page * 389u + song.lslot * 7919u;
        for (l = 0; l < 2u; l++) {
            h ^= ((t->lfo[l].ph >> 26) * 2654435761u + (uint32_t)(lfo_out(t, l) >> 10) * 40503u) * (l + 1u);
            if (lfo_random((uint32_t)t->p[(l ? P_LFO2 : P_LFO1) + LF_WAVE]))
                h ^= ((ui.tr_h[l] + 1u) * 97u + ui.tr_n[l] * 65537u) * (l + 3u);
        }
    }
    return h;
}

/* PERFORM (perform.c), while FX is held: the black keys' effects in two rows of five (as on the keys: 1/8 1/16 1/32
 * REV TAPE, LPF HPF FRZ OCT+ OCT-), the white track keys' REPEAT STEP (T1 .. T6), its rates (the keys D5 .. G5: 1/4
 * 1/8 1/16 1/32) beside RPT; held: lit, running: inverted, unavailable at the tempo: dim, the rate selected: lit */
static const char *const PF_NAME[PF_T1] = {"R1/8", "R1/16", "R1/32", "REV", "LPF", "HPF", "TAPE", "FRZ", "OCT+", "OCT-"};
static const char *const PR_NAME[PR_N] = {"1/4", "1/8", "1/16", "1/32"};
#define PF_CH 27                                      /* a cell's height; the rows 29 apart */
static void perf_cell(int32_t x, int32_t y, int32_t w, const char *s, uint32_t st)   /* st: 0 off 1 held 2 run 3 dim */
{
    int32_t tx = x + (w - md_text_w(&MD_F3, 2, s)) / 2, ty = y + (PF_CH - 10) / 2;
    if (st == 1u || st == 2u) {                      /* held / running: solid, the name in BG */
        cv_rect(x, y, w, PF_CH, C_INK);
        md_text(tx, ty, &MD_F3, 2, s, C_BG);
        return;
    }
    if (st == 3u) {                                  /* not available: a dotted outline */
        cv_hdot(x, x + w - 1, y, C_INK);
        cv_hdot(x, x + w - 1, y + PF_CH - 1, C_INK);
        cv_vdot(x, y, y + PF_CH - 1, C_INK);
        cv_vdot(x + w - 1, y, y + PF_CH - 1, C_INK);
    } else {
        cv_frame(x, y, w, PF_CH, C_INK);
    }
    md_text(tx, ty, &MD_F3, 2, s, C_INK);
}
static void graph_perf(void)
{
    uint32_t held = perf_kill ? 0u : perf_held, act = perf_act, ok = perf_avail(), i, nw, wi, rate = perf_rate;
    uint32_t sig = held * 31u + act * 7u + ok + rate * 0x9E3779B9u;
    for (nw = rg_plan(&ui.graph_sig, sig, 0, Y_GRAPH, 240, H_GRAPH), wi = 0; wi < nw; wi++) {
        rg_win(wi, Y_GRAPH, C_BG);
        cv_oy = 0;
        cv_rect(0, 0, 1, H_GRAPH, C_INK);
        cv_rect(239, 0, 1, H_GRAPH, C_INK);
        for (i = 0; i < 10u; i++) {
            uint32_t e = PF_BLACK[i];
            perf_cell(4 + (int32_t)(i % 5u) * 47, i < 5u ? 3 : 32, 44, PF_NAME[e],
                      !((ok >> e) & 1u) ? 3u : (act >> e) & 1u ? 2u : (held >> e) & 1u ? 1u : 0u);
        }
        for (i = 0; i < NTRK; i++) {
            char n[3] = {'T', (char)('1' + i), 0};
            perf_cell(4 + (int32_t)i * 29, 61, 27, n, (held >> (PF_T1 + i)) & 1u);
        }
        md_text(4 + (44 - md_text_w(&MD_F3, 2, "RPT")) / 2, 90 + (PF_CH - 10) / 2, &MD_F3, 2, "RPT", C_INK);
        for (i = 0; i < PR_N; i++)
            perf_cell(4 + (int32_t)(i + 1u) * 47, 90, 44, PR_NAME[i], i == rate);
        cv_blit(0, Y_GRAPH);
    }
}
/* PERFORM: KNOB 1..4 FILTER CRUSH THROW DEPTH (SHIMMER while OCT UP / DN plays) */
static void perf_columns(void)
{
    char val[12];
    int32_t m = perf_k[0];
    fmt_int(val, m < 0 ? -m : m);
    draw_column(0, "FILT", m ? val : "OFF", m < 0 ? "LPF" : m > 0 ? "HPF" : "", m ? HOT(0u) : CV_DIM, (m + 100) * 5);
    fmt_int(val, perf_k[1]);
    draw_column(1, "CRUSH", perf_k[1] ? val : "OFF", perf_k[1] ? "%" : "", perf_k[1] ? HOT(1u) : CV_DIM,
                perf_k[1] * 10);
    fmt_int(val, perf_k[2]);
    draw_column(2, "THROW", perf_k[2] ? val : "OFF", perf_k[2] ? "%" : "", perf_k[2] ? HOT(2u) : CV_DIM,
                perf_k[2] * 10);
    if (perf_harm_on()) {
        fmt_int(val, perf_k[3]);
        draw_column(3, "SHIMR", perf_k[3] ? val : "OFF", perf_k[3] ? "%" : "", perf_k[3] ? HOT(3u) : CV_DIM,
                    perf_k[3] * 10);
    } else {
        fmt_int(val, 100 - perf_k[3]);
        draw_column(3, "DEPTH", val, "%", HOT(3u), (100 - perf_k[3]) * 10);
    }
}

/* the page's graph in the canvas or a window of it, with the frame's edges; 1: something in the band above the graph
 * proper (the canvas from row 0) */
static uint32_t graph_paint(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint16_t c = C_INK;
    uint32_t top = 0;
    cv_oy = G_OY;
    switch (pg->graph) {
    case GR_MODEL:
        graph_model(t, c);
        top = 1;                                      /* (the preset's line in the band above) */
        break;
    case GR_FX:
        graph_fx(t, c);
        break;
    case GR_COMP:
        graph_comp(c, pg->id[0] == G_CATK);
        break;
    case GR_LFO:
        graph_lfo(c);
        break;
    case GR_LDEST:
        graph_ldest();
        break;
    case GR_FILTER:
        graph_filter(c);
        break;
    case GR_INSERT:
        graph_insert();
        break;
    case GR_DLY:
        graph_delay();
        break;
    case GR_REV:
        graph_reverb();
        break;
    case GR_CHO:
        graph_chorus();
        break;
    default:
        break;
    }
    cv_oy = 0;
    cv_rect(0, 0, 1, H_GRAPH, C_INK);                 /* the frame's edges */
    cv_rect(239, 0, 1, H_GRAPH, C_INK);
    return top;
}

/* the LIST band's picture (list_band): SOUND the rendered hit, as a DAW's waveform (136 columns about y 24; FM ACID
 * KTRX, whose hits it does not render: the centre line); LFO 1/3 one cycle of LFO 1's wave at its MORPH and PHASE (the
 * random ones: a fixed pattern of their kind). Static: no live dot, nothing moves */
static void band_paint(void)
{
    const track_t *t = TSEL;
    int32_t k, px = 0, py;
    if (cur_page()->graph == GR_MODEL) {
        cv_hdot(52, 187, 24, C_INK);
        if (!vg.ok || vg.okm != (uint32_t)t->p[P_MODEL] % NMODELS || !vg_model(vg.okm))
            return;
        vg.shown = vg.ok;
        for (k = 0; k < 136; k++)
            if (vg.cmx[k] || vg.cmn[k]) {
                int32_t a = vg.cmx[k] * 20 / VG_A, b = vg.cmn[k] * 20 / VG_A;
                cv_rect(52 + k, 24 - a, 1, a - b + 1, C_INK);
            }
        return;
    }
    {
        static const int8_t RND[8] = {12, -18, 20, -4, 16, -20, 4, 18};   /* the random waves' pattern */
        const int16_t *q = &t->p[P_LFO1];
        uint32_t wave = (uint32_t)clamp(q[LF_WAVE], 0, LW_COUNT - 1), off = (uint32_t)clamp(q[LF_PHASE], 0, 127) << 25;
        int32_t m = clamp(q[LF_MORPH], 0, 127);
        cv_hdot(40, 199, 24, C_INK);
        for (k = 0; k < 160; k++) {                   /* one cycle, x 40..199; joined to the point before */
            if (wave == LW_SH)
                py = 24 - RND[k / 20];
            else if (wave == LW_WANDER)
                py = 24 - (RND[k / 20] * (20 - k % 20) + RND[(k / 20 + 1) % 8] * (k % 20)) / 20;
            else if (wave == LW_RWALK)
                py = 24 - RND[k / 20] / 2 - RND[(k / 10) % 8] / 3;
            else
                py = 24 - lfo_shape(wave, m, (uint32_t)k * (0xFFFFFFFFu / 160u) + off) * 20 / 32767;
            if (!k)
                px = py;
            cv_rect(40 + k, py < px ? py : px, 1, (py < px ? px - py : py - px) + 2, C_INK);
            px = py;
        }
    }
}
static uint32_t band_sig(void)
{
    const track_t *t = TSEL;
    uint32_t mi = (uint32_t)t->p[P_MODEL] % NMODELS;
    if (cur_page()->graph == GR_MODEL)
        return 0x5A5A0001u ^ (vg.ok && vg.okm == mi && vg_model(mi) ? vg.ok : 0u);
    return 0x1F0u + (uint32_t)t->p[P_LFO1 + LF_WAVE] * 7919u + (uint32_t)t->p[P_LFO1 + LF_MORPH] * 104729u +
           (uint32_t)t->p[P_LFO1 + LF_PHASE] * 15485863u;
}
static void list_band(void)
{
    uint32_t i, n;
    if (cur_page()->graph == GR_MODEL)
        vg_step(TSEL);                                /* the hit rendered: a frame's share */
    n = rg_plan(&ls.g, band_sig(), 0, LS_GY, 240, LS_GH);
    for (i = 0; i < n; i++) {
        rg_win(i, LS_GY, C_BG);
        band_paint();
        cv_blit(0, LS_GY);
    }
}

static void draw_graph(void)
{
    const page_t *pg = cur_page();
    uint32_t sig, i, n, top;
    int whole;
    if (list_on()) {                                  /* a LIST page: its band (SOUND, LFO 1/3), no graph */
        if (list_band_on())
            list_band();
        return;
    }
    if (!ui.home && !ui.layer && pg->graph == GR_MODEL) {
        vg_step(TSEL);                                /* the engine view's rendered hit: a frame's share (under a popup
                                                       * too: MODEL's message) */
        if (vg.ok != vg.shown && vg.okm == (uint32_t)TSEL->p[P_MODEL] % NMODELS && vg_model(vg.okm))
            rg.thaw = 1;                              /* .. done, not shown yet: drawn now, round the popups */
    }
    if ((ui.msg_t || ui.bpm_t) && !ui.force && !rg.thaw)   /* a popup over the graph: the graph holds still under it
                                                       * (no redraw-then-popup flicker); a popup coming / going: the
                                                       * graph as it is now, round the popups up (rg.ex) */
        return;
    if (ui.layer) {                                     /* PERFORM: the map */
        graph_perf();
        ui.graph_top = 1;
        return;
    }
    if (!ui.home && pg->graph == GR_LFO)
        lfo_trail();
    sig = graph_signature();
    whole = ui.force || sig != ui.graph_sig;
    n = rg_plan(&ui.graph_sig, sig, 0, Y_GRAPH, 240, H_GRAPH);
    if (whole) {                                      /* (the band above the graph proper only when it holds
                                                       * something, or held something) */
        uint32_t r0;
        cv_begin_at(240, H_GRAPH, Y_GRAPH, C_BG);
        top = graph_paint();
        r0 = top || ui.graph_top || ui.force || rg.dmg.w ? 0u : G_OY;
        ui.graph_top = (uint8_t)top;
        rg_region(0, Y_GRAPH, 240, H_GRAPH);
        rg_add(0, (int32_t)r0, 240, H_GRAPH - (int32_t)r0);
        if (rg.n == 1u && rg.win[0].w == 240 && rg.win[0].h == H_GRAPH - (int32_t)r0) {
            cv_blit_from(0, Y_GRAPH, r0);
            return;
        }
        n = rg.n;                                     /* round a popup up: in windows */
    }
    for (i = 0; i < n; i++) {                         /* a popup's place */
        rg_win(i, Y_GRAPH, C_BG);
        top = graph_paint();
        cv_blit(0, Y_GRAPH);
        ui.graph_top |= (uint8_t)top;
    }
}

/* a message (ui_say): a small MD popup over the middle of the screen while ui.msg_t runs; drawn when it changes, or
 * a popup's end uncovered its place (rg.dmg), or TEMPO was drawn over it this frame (again: the message over TEMPO, as
 * a full repaint). Gone (or narrower), what it covered draws again (ui_draw: the damage) */
#define MSG_Y 106
#define MSG_H 31                                        /* (30 + the shadow's row) */
static struct {
    int16_t x, w;                /* the message's popup on the screen */
} msgp;
static int32_t msg_w(void)
{
    int32_t w = md_text_w(&MD_F3, 2, ui.msg) + 24;
    return w > 236 ? 236 : w;
}
static void draw_msg(int again)
{
    uint32_t sig = ui.msg_t ? str_hash(7u, ui.msg) : 0u;
    int32_t w = msg_w(), h = 30;
    if (!sig || (!again && !ui.force && sig == ui.msg_sig && !rg_meets(120 - w / 2, MSG_Y, w, MSG_H)))
        return;
    ui.msg_sig = sig;
    cv_begin((uint32_t)w, (uint32_t)h + 1u, C_BG);
    cv_popup(0, 0, w, h, "", C_INK, C_BG);
    md_text(12, 10, &MD_F3, 2, ui.msg, C_INK);
    cv_blit((uint32_t)(120 - w / 2), MSG_Y);
    msgp.x = (int16_t)(120 - w / 2);
    msgp.w = (int16_t)w;
}

/* a popup's title tab where cv_popup puts it (6 px over the box at x, y, w wide), as its own canvas: the box's canvas
 * starts at the box (cv_popup drew the tab's lower part in it), so nothing beside the tab is wiped */
static void popup_tab(int32_t x, int32_t y, int32_t w, const char *title)
{
    int32_t tw = md_text_w(&MD_F3, 2, title) + 12;
    cv_begin((uint32_t)tw, 6u, C_INK);
    md_text(6, 3, &MD_F3, 2, title, C_BG);
    cv_blit((uint32_t)(x + w / 2 - tw / 2), (uint32_t)(y - 6));
}

/* TEMPO (spec 2026-10-19-md-ui §7): while the BPM is being changed (ui.bpm_t), a popup with the BPM in the MD's tempo
 * digits at 3x (as messages: drawn when it changes, or uncovered; gone, what it covered draws again). x 30..209, y
 * 70..166 (the tab over the box) */
#define TEMPO_RC 30, 70, 180, 97
static int draw_tempo(void)                          /* 1: drawn */
{
    char b[8];
    int32_t w;
    if (!ui.bpm_t || (!ui.force && ui.tempo_sig == (uint32_t)song.g[G_BPM] + 1u && !rg_meets(TEMPO_RC)))
        return 0;
    ui.tempo_sig = (uint32_t)song.g[G_BPM] + 1u;
    cv_begin(180, 91, C_BG);
    cv_popup(0, 0, 180, 90, "TEMPO", C_INK, C_BG);
    fmt_int(b, song.g[G_BPM]);
    w = md_text_w(&MD_BIG, 3, b);
    md_text(90 - w / 2, 28, &MD_BIG, 3, b, C_INK);
    cv_blit(30, 76);
    popup_tab(30, 76, 180, "TEMPO");
    return 1;
}

/* a confirm (spec §7): a popup over the page, the question's first word its title, OCT- NO / OCT+ YES as keycaps */
static void draw_confirm(const char *q, const char *q2)
{
    char verb[12];
    uint32_t n = 0;
    while (q[n] && q[n] != ' ' && q[n] != ':' && n < sizeof verb - 1u) {
        verb[n] = q[n];
        n++;
    }
    verb[n] = 0;
    cv_begin(184, 75, C_BG);
    cv_popup(0, 0, 184, 74, verb, C_INK, C_BG);
    md_text(92 - md_text_w(&MD_F3, 2, q) / 2, 10, &MD_F3, 2, q, C_INK);
    if (q2)
        md_text(92 - md_text_w(&MD_F3, 2, q2) / 2, 24, &MD_F3, 2, q2, C_INK);
    cv_keycap(11, 40, "OCT-", "NO", C_INK);
    cv_keycap(99, 40, "OCT+", "YES", C_INK);
    cv_blit(28, 82);
    popup_tab(28, 82, 184, verb);
}

/* a parameter's label in at most 4 letters (the HOME strip's 38 px cells; the MD's own abbreviations) */
static const char *abbr4(const char *l, char *b)
{
    static const char *const AB[][2] = {{"DECAY", "DEC"}, {"SWEEP", "SWP"}, {"CLICK", "CLIK"}, {"PITCH", "PTCH"},
                                        {"DEPTH", "DPTH"}, {"NOISE", "NOIS"}, {"LEVEL", "LEV"}, {"SHIMR", "SHMR"},
                                        {"CHAOS", "CHAS"}, {"COLOR", "COLR"}, {"SPEED", "SPD"}, {"RANGE", "RNG"},
                                        {"PUNCH", "PNCH"}, {"SNAPY", "SNAP"}, {"SPRED", "SPRD"}, {"INDEX", "INDX"},
                                        {"RATIO", "RATI"}, {"ACCNT", "ACC"}, {"CUTOF", "CUT"}, {"ENVMD", "ENV"}};
    uint32_t i;
    for (i = 0; i < sizeof AB / sizeof AB[0]; i++)
        if (str_eq(l, AB[i][0]))
            return AB[i][1];
    str_cpy(b, l, 5);
    return b;
}

/* HOME (FREITAKT, Model:Cycles style): only the status line "P1 V2 T6 L1" (the pattern playing (1..4: A..D), its variation,
 * the selected track, the track's step page) large, the CPU load and the battery small, the chain's line while a
 * pattern chain runs (a knob turned: the knob popup, draw_kpop). Bands: CPU (0..23, x 0..149),
 * battery (x 150..239), the BPM (24..71), the status line (72..167), the chain (168..207), a plain ground
 * (208..239). Each band its own signature (rg_plan: whole when it moved, else the damage a popup left, round the
 * popups up): an idle HOME draws nothing */
#define HM_Y_MID 72
#define HM_BPM_X 24                                     /* the BPM's text box: x 24..215, y 36..59 */
#define HM_BPM_Y 36
#define HM_BPM_H 24
#define HM_H_MID 96
#define HM_Y_CH 168
#define HM_H_CH 40
static struct {
    uint32_t cpu, bat, bpm, mid, chain;  /* the bands' signatures shown */
    uint8_t on;                          /* HOME shown last frame (in or out: all drawn again) */
} hm;

/* the status line: "P1 V2 T6 L1" ("P-": no slot loaded, the power-on kit); as the MIDI KEYBOARD "MIDI KBD CH1 OCT+1"
 * (its channel now, "CH-" none: TRK on a track whose MIDI OUT channel is OFF) */
static void home_status(char *s)
{
    int32_t ld = proj_loaded();
    if (kbd_mode()) {
        uint32_t c = mkb_chan(), n;
        str_cpy(s, "MIDI KBD CH", 24);
        if (c)
            fmt_int(s + 11, (int32_t)c);
        else
            str_cpy(s + 11, "-", 2);
        str_cpy(s + str_len(s), " OCT", 8);
        n = str_len(s);
        if (song.octave > 0)
            s[n++] = '+';
        fmt_int(s + n, song.octave);
        return;
    }
    s[0] = 'P', s[1] = ld >= 0 ? (char)('1' + ld) : '-', s[2] = ' ';
    s[3] = 'V', s[4] = (char)('1' + var_cur % 9u), s[5] = ' ';
    s[6] = 'T', s[7] = (char)('1' + song.sel % 9u), s[8] = ' ';
    s[9] = 'L';
    fmt_int(s + 10, (int32_t)ui.bank + 1);
}

static void draw_home(void)
{
    char st[24], b[12];
    uint32_t i, n, sig, sc, cpu = song.cpu_q8 * 100u / 256u, cn = 0, cf = 0;
    int32_t lvl = batt_shown(), x, w;
    if (cpu > 999u)
        cpu = 999u;
    for (n = rg_plan(&hm.cpu, cpu + 1u, 0, 0, 150, 24), i = 0; i < n; i++) {   /* "CPU 23%" */
        rg_win(i, 0, C_BG);
        x = md_text(8, 7, &MD_F3, 2, "CPU ", C_INK);
        fmt_int(b, (int32_t)cpu);
        str_cpy(b + str_len(b), "%", 2);
        md_text(x, 7, &MD_F3, 2, b, C_INK);
        cv_blit(0, 0);
    }
    for (n = rg_plan(&hm.bat, (uint32_t)lvl + 1u, 150, 0, 90, 24), i = 0; i < n; i++) {   /* the battery: 3 cells */
        rg_win(i, 0, C_BG);
        cv_frame(54, 6, 22, 12, C_INK);
        cv_rect(76, 9, 2, 6, C_INK);
        for (x = 0; x < lvl && x < 3; x++)
            cv_rect(56 + x * 6, 8, 5, 8, C_INK);
        cv_blit(150, 0);
    }
    rg_fill(0, 24, 240, HM_BPM_Y - 24, C_BG);          /* the BPM's band round its text: drawn once */
    rg_fill(0, HM_BPM_Y + HM_BPM_H, 240, HM_Y_MID - HM_BPM_Y - HM_BPM_H, C_BG);
    rg_fill(0, HM_BPM_Y, HM_BPM_X, HM_BPM_H, C_BG);
    rg_fill(240 - HM_BPM_X, HM_BPM_Y, HM_BPM_X, HM_BPM_H, C_BG);
    for (n = rg_plan(&hm.bpm, (uint32_t)song.g[G_BPM] + 1u + (uint32_t)song.g[G_CLOCK] * 65536u, HM_BPM_X, HM_BPM_Y,
                     240 - 2 * HM_BPM_X, HM_BPM_H), i = 0;
         i < n; i++) {
        rg_win(i, HM_BPM_Y, C_BG);                    /* the BPM (FREITAKT redesign: KNOB 1 turns it): only its
                                                       * text's box sent on a change. Following a clock: its source
                                                       * and the tempo measured ("USB 120") */
        str_cpy(b, song.g[G_CLOCK] ? N_CLOCK[song.g[G_CLOCK]] : "BPM", sizeof b);
        str_cpy(b + str_len(b), " ", 2);
        fmt_int(b + str_len(b), song.g[G_BPM]);
        md_text(120 - HM_BPM_X - md_text_w(&MD_F3, 4, b) / 2, 38 - HM_BPM_Y, &MD_F3, 4, b, C_INK);
        cv_blit(HM_BPM_X, HM_BPM_Y);
    }
    rg_fill(0, HM_Y_CH + HM_H_CH, 240, 240 - HM_Y_CH - HM_H_CH, C_BG);
    if (pp_on()) {                                    /* PRESETS' popup over the middle (y 61..180; it repaints all
                                                       * when gone): the chain band's rows under it */
        rg_fill(0, Y_GRAPH + H_GRAPH, 240, HM_Y_CH + HM_H_CH - Y_GRAPH - H_GRAPH, C_BG);
        return;
    }
    home_status(st);
    for (n = rg_plan(&hm.mid, str_hash(0x811C9DC5u, st), 0, HM_Y_MID, 240, HM_H_MID), i = 0; i < n; i++) {
        rg_win(i, HM_Y_MID, C_BG);                    /* "P1 V2 T6 L1", centred */
        sc = md_fit_sc(st, 228, 5u);
        md_text(120 - md_text_w(&MD_F3, sc, st) / 2, 48 - 5 * (int32_t)sc / 2, &MD_F3, sc, st, C_INK);
        cv_blit(0, HM_Y_MID);
    }
    sig = 0;                                          /* the chain: up to 6 entries ("A1 B3": the pattern, its
                                                       * variation) from the one before the one playing; that one
                                                       * inverted, the next underlined */
    if (chain.n > 1u) {
        uint32_t p = chain.pos == CH_NONE ? 0u : chain.pos;
        cn = chain.n < 6u ? chain.n : 6u;
        cf = chain.n <= 6u ? 0u : (p + chain.n - 1u) % chain.n;
        sig = 1u + cn * 7u + cf * 31u + (chain.pos + 1u) * 7919u + (chain.nx + 1u) * 104729u;
        for (i = 0; i < cn; i++)
            sig = sig * 16777619u ^ chain.s[(cf + i) % chain.n];
    }
    for (n = rg_plan(&hm.chain, sig, 0, HM_Y_CH, 240, HM_H_CH), i = 0; i < n; i++) {
        rg_win(i, HM_Y_CH, C_BG);
        if (cn) {
            uint32_t c;
            x = md_text(120 - (md_text_w(&MD_F3, 3, "CH") + 6 + (int32_t)cn * 33) / 2, 12, &MD_F3, 3, "CH", C_INK) + 12;
            for (c = 0; c < cn; c++, x += 33) {
                uint32_t e = (cf + c) % chain.n;
                char d[3] = {(char)('A' + (chain.s[e] & 3u)), (char)('1' + ((chain.s[e] >> 2) & 7u)), 0};
                w = md_text_w(&MD_F3, 3, d);
                if (e == chain.pos) {
                    cv_rect(x - 3, 9, w + 6, 21, C_INK);
                    md_text(x, 12, &MD_F3, 3, d, C_BG);
                } else {
                    md_text(x, 12, &MD_F3, 3, d, C_INK);
                }
                if (e == chain.nx && e != chain.pos)
                    cv_rect(x, 31, w, 2, C_INK);
            }
        }
        cv_blit(0, HM_Y_CH);
    }
}

/* a step held on a steps page (step_edit_page): KNOB 1 PROB, 2 RATCH, 3 ACC, 4 NUDGE (step_edit); 1: drawn */
static const char *const STEP_FIELD[4] = {"PROB", "RATCH", "ACC", "NUDGE"};
static void step_field_text(uint32_t c, const step_t *st, char *val)   /* field c of st as shown (12 bytes) */
{
    if (c == 0u) {
        cond_format(st->cond, val);
    } else if (c == 1u) {
        fmt_int(val, (int32_t)st->rat + 1);
    } else if (c == 2u) {
        str_cpy(val, st->acc ? "ON" : "OFF", 12);
    } else {
        val[0] = st->micro > 0 ? '+' : 0;             /* -50 .. +50 */
        val[1] = 0;
        fmt_int(val + str_len(val), st->micro);
    }
}
static int held_step_columns(void)
{
    char val[12];
    uint32_t c;
    const step_t *st;
    if (ui.home || ui.menu || !step_edit_page() || held_step() >= NSTEP)
        return 0;
    st = &TSEL->step[held_step()];
    for (c = 0; c < 4u; c++) {
        step_field_text(c, st, val);
        draw_column(c, STEP_FIELD[c], val, "", HOT(c), -1);
    }
    return 1;
}
static void draw_columns(void)
{
    uint32_t c;
    char val[12];
    const char *unit;
    const page_t *pg = cur_page();
    if (ui.layer) {                                     /* PERFORM: the macros */
        perf_columns();
        return;
    }
    if (pg->scope == SC_MIX) {                          /* TRACKS: the tracks' list (the knobs: the popup) */
        draw_tracks();
        return;
    }
    if (held_step_columns())                          /* a step held on a steps page: its PROB RATCH ACC NUDGE */
        return;
    for (c = 0; c < 4u; c++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(pg, c, &vp);
        if (pg->graph == GR_LDEST && c == 0u) {        /* LFO DEST: KNOB 1 the slot */
            fmt_int(val, (int32_t)(song.lslot & 3u) + 1);
            draw_column(0, "SLOT", val, "/4", HOT(0u), -1);
            continue;
        }
        if (!d || !d->label || d->label[0] == '-') {
            draw_column(c, "", "", "", CV_NORM, -1);
            continue;
        }
        if (pg->id[c] == G_MIDI && pg->scope == SC_GLOBAL) {
            str_cpy(val, !usb.up ? "OFF" : usb.config ? "MIDI" : usb.setups ? "ENUM" : usb.sof_seen ? "BUS" : "WAIT", 12);
            draw_column(c, "USB", val, "USB", CV_NORM, -1);
            continue;
        }
        int lk = 0;
        int16_t sv = pg->scope == SC_TRACK ? lock_shown(lock_id(c), *vp, &lk) : *vp;   /* (a step held in STEPS: the
                                                                                          * knob's lock there) */
        if (pg->id[c] == G_INFO && pg->scope == SC_GLOBAL) {
            fmt_int(val, (int32_t)(song.cpu_q8 * 100u / 256u));
            unit = "%";
        } else {
            param_format(d, sv, val, &unit);
        }
        if (pg->graph == GR_MODEL && page_id(pg, c) == P_MODEL && list_on()) {   /* SOUND 1/n's first row: the machine,
                                                                                   * its preset ("*": edited since) */
            char pn[12];
            preset_name_of(preset_of(TSEL), pn);
            if (TSEL->pre_ed)
                str_cpy(pn + str_len(pn), "*", 2);
            draw_column_m(c, val, pn, "", lk ? CV_HOT : HOT(c), -1, -1);
            continue;
        }
        {
            int32_t mv, mod = -1;
            if (pg->scope == SC_TRACK && page_id(pg, c) < P_COUNT && lfo_live(TSEL, page_id(pg, c), &mv))
                mod = RATIO(d, mv);
            draw_column_m(c, d->label, val, unit, lk ? CV_HOT : HOT(c),
                          d->fmt == F_ENUM || d->fmt == F_LDEST ? -1 : RATIO(d, sv), mod);
        }
    }
}

/* ---- the knob popup (ui.c kp; FREITAKT redesign, rule 5) */
/* up and shown: nothing owns the screen over it (the menu, a confirm, NAME, the PRESETS list or popup, the clear's countdown, PERFORM) */
static int kp_live(void)
{
    return kp.on && !ui.menu && !ui.confirm && !name_on() && !pl_on() && !pp_on() && !ui.clr && !ui.layer;
}
/* the session's name and value now (16 bytes each; unit: the value's, small) */
static void kp_text(char *l, char *v, const char **unit)
{
    const track_t *t = &trk[kp.trk % NTRK];
    int16_t x;
    uint32_t n;
    *unit = "";
    l[0] = v[0] = 0;
    if (kp.kind == KP_STEP) {                          /* a held step's PROB RATCH ACC NUDGE */
        str_cpy(l, STEP_FIELD[kp.id & 3u], 16);
        step_field_text(kp.id & 3u, &t->step[kp.st % NSTEP], v);
    } else if (kp.kind == KP_MODEL || (kp.kind == KP_LOCK && kp.id == P_MODEL)) {   /* MODEL, a machine lock */
        x = t->p[P_MODEL];
        if (kp.kind == KP_LOCK && !lock_get(kp.trk, kp.st, P_MODEL, &x))
            x = motion_rest(kp.trk, P_MODEL);
        str_cpy(l, "MODEL", 16);
        str_cpy(v, N_MODEL[(uint32_t)x % NMODELS], 16);
    } else if (kp.kind == KP_LOCK) {                   /* the step's lock, else the knob's own value there */
        if (!lock_get(kp.trk, kp.st, kp.id, &x))
            x = motion_rest(kp.trk, kp.id);
        str_cpy(l, kp.d->label, 16);
        param_format(kp.d, x, v, unit);
    } else if (kp.kind == KP_PAR) {
        str_cpy(l, kp.d->label, 16);
        if (kp.id == P_LEVEL && t->p[P_MUTE])
            str_cpy(v, "MUTE", 16);
        else
            param_format(kp.d, *kp.vp, v, unit);
    }
    n = str_len(v);
    if (n > 1u && v[n - 1u] == '.')                  /* "-24." -> "-24" */
        v[n - 1u] = 0;
}
/* the screen cleared, the name (y 40..79) and the value (y 92..171; a step's lock inverted) as large as they fit, a
 * message small under them (y 196..215). Opened (or ui.force): the screen once (the ground round the rows, the rows);
 * then a row only when what it shows changed: a turn sends the value's row */
#define KP_NY 40
#define KP_NH 40
#define KP_VY 92
#define KP_VH 80
#define KP_MY 196
#define KP_MH 20
static void draw_kpop(void)
{
    char l[16], v[16];
    const char *u;
    uint32_t sc, sig;
    int32_t w, uw, x, ty;
    if (kp.on == 1u || ui.force) {                     /* the ground round the rows (they send their own) */
        lcd_fill(0, 0, 240, KP_NY, C_BG);
        lcd_fill(0, KP_NY + KP_NH, 240, KP_VY - KP_NY - KP_NH, C_BG);
        lcd_fill(0, KP_VY + KP_VH, 240, KP_MY - KP_VY - KP_VH, C_BG);
        lcd_fill(0, KP_MY + KP_MH, 240, 240 - KP_MY - KP_MH, C_BG);
        kp.on = 2;
        kp.nsig = kp.vsig = kp.msig = 0;
    }
    kp_text(l, v, &u);
    sig = str_hash(0x4B50u, l) | 1u;
    if (sig != kp.nsig) {
        kp.nsig = sig;
        sc = md_fit_sc(l, 228, 6u);
        cv_begin(240, KP_NH, C_BG);
        md_text(120 - md_text_w(&MD_F3, sc, l) / 2, (KP_NH - 5 * (int32_t)sc) / 2, &MD_F3, sc, l, C_INK);
        cv_blit(0, KP_NY);
    }
    sig = str_hash(str_hash(0x5650u + (kp.kind == KP_LOCK), v), u) | 1u;
    if (sig != kp.vsig) {
        kp.vsig = sig;
        uw = u[0] ? md_text_w(&MD_F3, 3, u) + 6 : 0;
        sc = md_fit_sc(v, 220 - uw, 14u);
        w = md_text_w(&MD_F3, sc, v);
        x = 120 - (w + uw) / 2;
        ty = (KP_VH - 5 * (int32_t)sc) / 2;
        cv_begin(240, KP_VH, C_BG);
        if (kp.kind == KP_LOCK) {                      /* a lock: the value inverted (as a LIST row's) */
            cv_rect(x - 5, ty - 5, w + 10, 5 * (int32_t)sc + 10, C_INK);
            md_text(x, ty, &MD_F3, sc, v, C_BG);
        } else {
            md_text(x, ty, &MD_F3, sc, v, C_INK);
        }
        if (uw)
            md_text(x + w + 6, ty + 5 * (int32_t)sc - 15, &MD_F3, 3, u, C_INK);
        cv_blit(0, KP_VY);
    }
    sig = ui.msg_t ? str_hash(0x4D53u, ui.msg) | 1u : 1u;
    if (sig != kp.msig) {                              /* a message (MODEL's ACID cap, LOCKS FULL ..): small, here */
        kp.msig = sig;
        cv_begin(240, KP_MH, C_BG);
        if (ui.msg_t)
            md_text(120 - md_text_w(&MD_F3, 2, ui.msg) / 2, 5, &MD_F3, 2, ui.msg, C_INK);
        cv_blit(0, KP_MY);
    }
}

/* REC + SEL held (ui_input.c clr_input): the warning, whole screen inverted: CLEAR T3, the countdown's digit (5..1)
 * very large, LET GO TO CANCEL; the digit's rows only when it changes */
#define CLR_DY 70
#define CLR_DH 100
static void draw_clr(void)
{
    static uint8_t shown;
    char t[12] = "CLEAR T1", d[2] = "5";
    if (ui.force) {
        uint32_t sc = md_fit_sc(t, 220, 7u);
        t[7] = (char)('1' + ui.clr_trk % NTRK);
        cv_begin(240, CLR_DY, C_INK);                 /* (three bands: the screen once) */
        md_text(120 - md_text_w(&MD_F3, sc, t) / 2, 14 + (50 - 5 * (int32_t)sc) / 2, &MD_F3, sc, t, C_BG);
        cv_blit(0, 0);
        cv_begin(240, 240 - CLR_DY - CLR_DH, C_INK);
        md_text(120 - md_text_w(&MD_F3, 2, "LET GO TO CANCEL") / 2, 26 + 5, &MD_F3, 2, "LET GO TO CANCEL", C_BG);
        cv_blit(0, CLR_DY + CLR_DH);
        shown = 0;
        ui.force = 0;
    }
    if (shown == ui.clr)
        return;
    shown = ui.clr;
    d[0] = (char)('0' + ui.clr % 10u);
    cv_begin(240, CLR_DH, C_INK);
    md_text(120 - md_text_w(&MD_F3, 18, d) / 2, (CLR_DH - 5 * 18) / 2, &MD_F3, 18, d, C_BG);
    cv_blit(0, CLR_DY);
}

/* inlined into fm1_main (main.c), as before M2: fm1_main then stays a function of its own, the shape H2 compares
 * with upstream (the boot sequence up to the fm1_main call) */
__attribute__((always_inline)) static inline void ui_draw(void)
{
    int home;
    if (safe_start) {                                 /* safe start: one static screen, drawn again after */
        static uint8_t drawn;                         /* an update session cleared it (main.c sets ui.force) */
        if (!drawn || ui.force) {
            lcd_fill(0, 0, 240, 240, C_BG);
            draw_text_box(0, 92, 240, &FONT_L, "SAFE START", C_INK, 1);
            draw_text_box(0, 128, 240, &FONT_S, "NO AUDIO - USB UPDATE READY", C_INK, 1);
            draw_text_box(0, 148, 240, &FONT_S, "POWER OFF TO LEAVE", C_INK, 1);
            drawn = 1;
            ui.force = 0;
        }
        return;
    }
    ui.frame++;
    if ((ui.splash == 1u || ui.splash == 2u) && splash_draw())   /* the boot splash (ui_menu.c) */
        return;
    if (kp.on && (!kp_live() || ui.pr.now - kp.t0 >= HOT_TICKS)) {   /* the knob popup: HOT_MS after the last turn, or
                                                                     * a screen over it: gone, the page whole again */
        kp_close();
        ui.hot_t = 0;
    }
    if (ui.menu) {
        draw_menu();
        ui.force = 0;
        return;
    }
    if (ui.clr) {                                     /* REC + SEL held: the clear's countdown */
        draw_clr();
        return;
    }
    if (ui.confirm) {
        if (ui.force) {
            char b[20] = "CONFIRM?";
            if (ui.confirm == 6u) {              /* MIDI LEARN (ui_learn.c ML_CONFIRM) */
                str_cpy(b, "CLEAR ALL LEARNED?", sizeof b);
            } else if (ui.confirm == 5u) {              /* the power-on load failed (project.c project_boot) */
                str_cpy(b, "LOAD FAILED: SLOT A", sizeof b);
                b[18] = (char)('A' + ui.confirm_trk % 4u);
            } else if (ui.confirm == CONFIRM_DISCARD) { /* LOAD of a slot with unsaved edits (project.c) */
                str_cpy(b, "DISCARD EDITS?", sizeof b);
            } else if (ui.confirm == CONFIRM_OVERWRITE) {   /* SAVE onto a slot with unsaved edits (project.c) */
                str_cpy(b, "OVERWRITE A*?", sizeof b);
                b[10] = (char)('A' + ui.confirm_trk % 4u);
            }
            bank_fix();                               /* the page under it, then the popup */
            if (save_car_on()) {                      /* (SAVE's carousel) */
                draw_save();
            } else if (ui.home && !ui.layer) {
                lcd_fill(0, 0, 240, 240, C_BG);
                draw_home();
            } else {
                draw_frame();
                draw_head();
                draw_columns();
                draw_graph();
            }
            {
                char q2[20] = "LOAD SLOT A SAVED";
                q2[10] = (char)('A' + ui.confirm_trk % 4u);
                draw_confirm(b, ui.confirm == 5u ? "START NEW PROJECT?" : ui.confirm == CONFIRM_DISCARD ? q2
                                : ui.confirm == CONFIRM_OVERWRITE ? "ITS EDITS ARE LOST" : 0);
            }
            ui.force = 0;
        }
        return;
    }
    if (name_on()) {                                  /* NAME: its own screen */
        draw_name();
        return;
    }
    if (pl_on()) {                                    /* the PRESETS list (ui_preset.c): its own screen */
        draw_plist();
        return;
    }
    if (kp.on) {                                      /* the knob popup: its own screen (the timers run on) */
        draw_kpop();
        if (ui.msg_t)
            ui.msg_t--;
        if (ui.bpm_t)
            ui.bpm_t--;
        if (ui.arm_t && !--ui.arm_t)
            ui.arm = 0;
        ui.force = 0;
        hm.on = 0;                                    /* (HOME under it: all again when it goes) */
        return;
    }
    if (save_car_on()) {                              /* SAVE's PROJECT / TOOLS / COPY: carousels (ui_menu.c), the
                                                       * messages in their note */
        draw_save();
        if (ui.msg_t)
            ui.msg_t--;
        if (ui.bpm_t)
            ui.bpm_t--;
        if (ui.arm_t && !--ui.arm_t)
            ui.arm = 0;
        ui.hot_t = 0;
        hm.on = 0;                                    /* (HOME after it: all again) */
        ui.force = 0;
        rg.dmg.w = 0;
        rg.nex = 0;
        rg.thaw = 0;
        return;
    }
    bank_fix();
    home = ui.home && !ui.layer;                      /* HOME: its own few bands (draw_home), no bars */
    if (home != hm.on)                                /* (in or out of HOME: the other screen's parts all again) */
        ui.force = 1;
    hm.on = (uint8_t)home;
    rg.dmg.w = 0;                                     /* a popup gone: its place is damage (what lies under it draws
                                                       * there again); another message: the graph as it is now round it
                                                       * (thaw), the old one's place damage (it may be narrower) */
    rg.nex = 0;
    rg.thaw = ui.msg_t && ui.msg_sig && str_hash(7u, ui.msg) != ui.msg_sig;   /* (another message) */
    if (!ui.msg_t && ui.msg_sig) {
        rg_damage(msgp.x, MSG_Y, msgp.w, MSG_H);
        ui.msg_sig = 0;
    }
    if (!ui.bpm_t && ui.tempo_sig) {
        rg_damage(TEMPO_RC);
        ui.tempo_sig = 0;
    }
    if (rg.thaw)
        rg_damage(msgp.x, MSG_Y, msgp.w, MSG_H);
    rg.thaw = rg.dmg.w != 0;                          /* (a popup came and went, another one still up: the graph as it
                                                       * is now round it, as the full repaint once did) */
    if (rg.dmg.w && pp_on())                          /* PRESETS' popup up (over the HOME strip's last rows): all, as
                                                       * before */
        ui.force = 1;
    if (ui.force)
        rg.dmg.w = 0, rg.thaw = 0;
    if (ui.msg_t)                                     /* the popups up: no window covers them */
        rg_keep(120 - msg_w() / 2, MSG_Y, msg_w(), MSG_H);
    if (ui.bpm_t) {                                   /* (TEMPO: its box, its tab) */
        rg_keep(30, 76, 180, 91);
        rg_keep(120 - (md_text_w(&MD_F3, 2, "TEMPO") + 12) / 2, 70, md_text_w(&MD_F3, 2, "TEMPO") + 12, 6);
    }
    if (ui.force && !home)
        draw_frame();
    felucca_dbg.stage = 3;
    if (home) {
        draw_home();
    } else {
        uint8_t nex = rg.nex;
        if (pp_on())                                  /* PRESETS' popup up: the LIST rows round it */
            rg_keep(0, Y_GRAPH, 240, H_GRAPH);
        draw_head();
        felucca_dbg.stage = 4;
        draw_columns();
        if (pp_on() && list_on() && list_band_on())   /* the LIST band (y 16..63) round the popup (draw_graph's
                                                       * place is the popup's) */
            list_band();
        rg.nex = nex;
    }
    felucca_dbg.stage = 5;
    if (pp_on())
        draw_popup();                                 /* PRESETS turned: the popup in the graph's place */
    else if (!home)
        draw_graph();
    draw_msg(draw_tempo());                           /* (the message over TEMPO) */
    if (ui.msg_t)
        ui.msg_t--;
    if (ui.bpm_t)
        ui.bpm_t--;
    if (ui.arm_t && !--ui.arm_t)
        ui.arm = 0;
    if (ui.hot_t && ui.pr.now - ui.hot_t0 >= HOT_TICKS)   /* (HOT_MS after the last turn) */
        ui.hot_t = 0;
    felucca_dbg.stage = 6;
    ui.force = 0;
    rg.dmg.w = 0;
    rg.nex = 0;
    rg.thaw = 0;
}
