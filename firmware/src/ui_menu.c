/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca menu (HOME held; FREITAKT redesign: a carousel, one item at a time): COLOR, SPEAKER EQ (FLAT LOWCUT BASS+),
 * USB LEVEL (MASTER FIXED), PERFORM (HOLD PAGE), KNOB ACCEL, BACKLIGHT (OFF DIM LO DIM HI INV INV LO INV HI), the
 * device settings, HARDWARE CALIBRATION, (BLE build: BLUETOOTH). */
/* ------------------------------------------------------------ menu --- */
/* the device settings the GLOBAL / SYSTEM pages had (FREITAKT redesign: those pages went; BPM and SWING are HOME's
 * knobs): an item each, KNOB 1 turns it, OCT+ steps it (wrapping); USB MIDI only shows the USB state. Their values
 * where they always were (song.g, the device settings: pages.c glob_desc, ui_input.c glob_set) */
#define MI_SETTINGS_ROWS MI_CLOCK, MI_SYNC, MI_DRCH, MI_ROUTE, MI_USBMIDI, MI_METRO, MI_MUTE, MI_THRU, MI_QUANT
#define MI_SETTINGS_NAMES "CLOCK", "MIDI SYNC", "MIDI CHANNEL", "MIDI ROUTE", "USB MIDI", "METRONOME", "MUTE MODE", \
                          "MIDI THRU", "REC QUANT"
static const uint8_t MI_GLOB[9] = {G_CLOCK, G_SYNC, G_DRCH, G_ROUTE, G_MIDI, G_METRO, G_MUTEBAR, G_THRU, G_QUANT};
static int glob_set(uint32_t id, int32_t v);        /* ui_input.c */
static int32_t knob_turn(uint32_t role, const param_desc_t *d, int32_t v, int32_t s);
#if FELUCCA_BLE                                     /* the Bluetooth page's item (bt_page.c), the last */
enum { MI_COLOR, MI_SPKEQ, MI_USB, MI_PERF, MI_ACCEL, MI_BACKLIGHT, MI_SETTINGS_ROWS, MI_PANEL, MI_BT, MI_COUNT };
static const char *const MI_NAME[MI_COUNT] = {"COLOR", "SPEAKER EQ", "USB LEVEL", "PERFORM", "ACCEL",
                                              "BACKLIGHT", MI_SETTINGS_NAMES, "HARDWARE CALIBRATION", "BLUETOOTH"};
static void bt_page_open(void);                     /* bt_page.c (after ble/ble_core.h): ui.menu 3 */
static void bt_page_close(void);
static void bt_page_draw(void);
static void bt_page_input(uint32_t pressed);
static void bt_autostart(void);                     /* AUTOSTART: once, 5 s after power-on (ui_input only, gate 3) */
#else
enum { MI_COLOR, MI_SPKEQ, MI_USB, MI_PERF, MI_ACCEL, MI_BACKLIGHT, MI_SETTINGS_ROWS, MI_PANEL, MI_COUNT };
static const char *const MI_NAME[MI_COUNT] = {"COLOR", "SPEAKER EQ", "USB LEVEL", "PERFORM", "ACCEL",
                                              "BACKLIGHT", MI_SETTINGS_NAMES, "HARDWARE CALIBRATION"};
#endif
static const char *const SPK_EQ[3] = {"FLAT", "LOWCUT", "BASS+"};   /* settings.lowcut (fx_lowcut) */
static uint32_t *menu_onoff(uint32_t i)            /* an ON / OFF item's setting, 0 if none */
{
    return i == MI_USB ? &settings.usbfix : i == MI_PERF ? &settings.perfpage :
           i == MI_ACCEL ? &settings.accel : 0;
}
static uint32_t menu_glob(uint32_t i)               /* a settings item's global id, 0xFF if none */
{
    return i >= MI_CLOCK && i <= MI_QUANT ? MI_GLOB[i - MI_CLOCK] : 0xFFu;
}
static void menu_glob_text(uint32_t id, char *b)    /* its value as the item shows it (b: 16 bytes) */
{
    int16_t *vp;
    const param_desc_t *d;
    const char *unit = "";
    if (id == G_MIDI) {                             /* (as SYSTEM's USB showed it) */
        str_cpy(b, !usb.up ? "OFF" : usb.config ? "MIDI" : usb.setups ? "ENUM" : usb.sof_seen ? "BUS" : "WAIT", 16);
        return;
    }
    d = glob_desc(id, &vp);
    param_format(d, *vp, b, &unit);
    if (unit[0] && str_len(b) + str_len(unit) + 2u <= 16u) {
        str_cpy(b + str_len(b), " ", 2);
        str_cpy(b + str_len(b), unit, 16 - str_len(b));
    }
}
static void menu_value(uint32_t i, char *b)         /* item i's value (b: 16 bytes; "": none) */
{
    uint32_t *o = menu_onoff(i);
    b[0] = 0;
    if (menu_glob(i) != 0xFFu)
        menu_glob_text(menu_glob(i), b);
    else if (o)
        str_cpy(b, i == MI_USB ? (*o ? "FIXED" : "MASTER") : i == MI_PERF ? (*o ? "PAGE" : "HOLD") : *o ? "ON" : "OFF",
                16);
    else if (i == MI_SPKEQ)
        str_cpy(b, SPK_EQ[settings.lowcut % 3u], 16);
    else if (i == MI_BACKLIGHT)                     /* (item 9: upstream's LEDS levels) */
        str_cpy(b, BL_NAME[backlight % BL_COUNT], 16);
    else if (i == MI_COLOR)                         /* the theme's name (owner 2026-10-23) */
        str_cpy(b, THEMES[theme_index(settings.palette)].name, 16);
}
static int menu_ok_job(uint32_t i) { return menu_glob(i) != G_MIDI; }   /* OCT+ does something on item i */

/* ---- CAROUSEL (FREITAKT redesign, phase D): the MENU and SAVE's PROJECT / TOOLS / COPY show one item at a time
 * under the top bar. Bands (screen rows; owner 2026-11-22: the title 3x, the hints 2x what they were): 16..59 the
 * section's title (F3 at 6x; a page count after it at 3x) and a dot an item (the item's a big one); 60..129 the
 * item's name, big and centred between a < and a > (the largest MD_F3 that fits, up to 5x; on two lines at a space
 * when one line would be under 3x); 130..161 its value (up to 4x); 162..181 a note (a state, or the message ui_say
 * put up; 2x); 182..239 the keys' hints under a dotted rule, two rows of up to two (hint_row: 4x, the left one at
 * the left edge, the right one at the right). SELECT scrolls the items (the input's; the < > and the dots say so).
 * A band is drawn only when what it shows changed (car_sig), all of them on ui.force: a value's change sends its
 * band, a SELECT detent the item's few */
typedef struct {
    const char *title, *count, *item, *val, *note; /* (count: "1/3", 0 none) */
    const char *hk[4], *ha[4];                      /* the hints: key, action; [0] [1] the first row, [2] [3] the
                                                     * second (hk 0: none) */
    uint32_t pos, n;                                /* the item's place, the items' count */
} car_t;
#define CAR_BANDS 5u
#define CAR_TSC 6u                                  /* the title's scale (3x the 2x it was) */
static const uint8_t CAR_Y[CAR_BANDS + 1u] = {16, 60, 130, 162, 182, 240};
#define CAR_MID 35                                  /* the item band's middle row */
static uint32_t car_sig[CAR_BANDS];

static void car_text(int32_t y, const char *s, uint32_t sc)    /* centred at band row y */
{
    md_text(120 - md_text_w(&MD_F3, sc, s) / 2, y, &MD_F3, sc, s, C_INK);
}
static void car_arrow(int32_t x, int32_t dir)       /* a solid triangle 6 px wide, its tip at x, the band's middle */
{
    int32_t i;
    for (i = 0; i < 6; i++)
        cv_rect(x + dir * i, CAR_MID - i, 1, 2 * i + 1, C_INK);
}
static void car_name(const char *s)                 /* band 1: the item's name */
{
    char a[24];
    uint32_t sc = md_fit_sc(s, 196, 5u), n = str_len(s), i, cut = 0, best = sc;
    if (sc < 3u)                                    /* two lines: at the space that lets them be biggest */
        for (i = 1; i + 1u < n && i < sizeof a; i++) {
            uint32_t k, k2;
            if (s[i] != ' ')
                continue;
            str_cpy(a, s, i + 1u);
            k = md_fit_sc(a, 196, 4u);
            k2 = md_fit_sc(s + i + 1u, 196, 4u);
            k = k2 < k ? k2 : k;
            if (k > best) {
                best = k;
                cut = i;
            }
        }
    if (!cut) {
        str_cpy(a, s, sizeof a);
        while (a[0] && md_text_w(&MD_F3, sc, a) > 196)
            a[str_len(a) - 1u] = 0;
        car_text(CAR_MID - (int32_t)(5u * sc) / 2, a, sc);
        return;
    }
    str_cpy(a, s, cut + 1u);                        /* (two rows of 5 x sc, 2 x sc apart, about the middle) */
    car_text(CAR_MID - (int32_t)(6u * best), a, best);
    car_text(CAR_MID + (int32_t)best, s + cut + 1u, best);
}
/* band 0's title: the name at CAR_TSC (less only if it would not fit), the page count after it at half that, on
 * the same baseline; the pair centred */
static void car_title(const char *t, const char *c)
{
    uint32_t cs = CAR_TSC / 2u, sc;
    int32_t cw = c ? md_text_w(&MD_F3, cs, c) + 4 * (int32_t)cs : 0, x;
    sc = md_fit_sc(t, 236 - cw, CAR_TSC);
    x = 120 - (md_text_w(&MD_F3, sc, t) + cw) / 2;
    x = md_text(x, 2, &MD_F3, sc, t, C_INK);
    if (c)
        md_text(x - (int32_t)sc + 4 * (int32_t)cs, 2 + 5 * (int32_t)(sc - cs), &MD_F3, cs, c, C_INK);
}
/* the carousel v: the bands whose content changed (all on ui.force), the top bar as everywhere; 1 when any band was
 * sent */
static int car_draw(const car_t *v)
{
    uint32_t s[CAR_BANDS], i, any = 0;
    s[0] = str_hash(str_hash(v->n * 131u + v->pos * 7u + 1u, v->title), v->count ? v->count : "");
    s[1] = str_hash(2u, v->item);
    s[2] = str_hash(3u, v->val);
    s[3] = str_hash(4u, v->note);
    s[4] = 5u;
    for (i = 0; i < 4u; i++)
        s[4] = str_hash(str_hash(s[4] * 31u + i, v->hk[i] ? v->hk[i] : ""), v->ha[i] ? v->ha[i] : "");
    draw_head();
    for (i = 0; i < CAR_BANDS; i++) {
        int32_t h = CAR_Y[i + 1u] - CAR_Y[i];
        if (!ui.force && s[i] == car_sig[i])
            continue;
        car_sig[i] = s[i];
        any = 1;
        cv_begin_at(240, (uint32_t)h, CAR_Y[i], C_BG);
        cv_oy = 0;
        switch (i) {
        case 0: {                                   /* the title, the dots (the item's 6 x 6, the others 2 x 2) */
            int32_t p = v->n > 1u ? (int32_t)(200u / (v->n - 1u)) : 0, x0;
            uint32_t d;
            p = p > 12 ? 12 : p;
            x0 = 120 - p * (int32_t)(v->n - 1u) / 2;
            car_title(v->title, v->count);
            for (d = 0; d < v->n && v->n > 1u; d++)
                if (d == v->pos)
                    cv_rect(x0 + p * (int32_t)d - 3, 36, 6, 6, C_INK);
                else
                    cv_rect(x0 + p * (int32_t)d - 1, 38, 2, 2, C_INK);
            break;
        }
        case 1:                                     /* the item, < > when there are others */
            if (v->n > 1u) {
                car_arrow(4, 1);
                car_arrow(235, -1);
            }
            car_name(v->item);
            break;
        case 2:                                     /* its value */
            if (v->val[0]) {
                uint32_t sc = md_fit_sc(v->val, 224, 4u);
                char b[24];
                str_cpy(b, v->val, sizeof b);
                while (b[0] && md_text_w(&MD_F3, sc, b) > 224)
                    b[str_len(b) - 1u] = 0;
                car_text(16 - (int32_t)(5u * sc) / 2, b, sc);
            }
            break;
        case 3: {                                   /* the note */
            char b[32];
            md_fit(b, v->note, sizeof b, 228);
            car_text(5, b, 2);
            break;
        }
        default:                                    /* the hints: two rows of 4x keycaps */
            cv_hdot(4, 235, 0, C_INK);
            hint_row(v->hk[0], v->ha[0], v->hk[1], v->ha[1], 4);
            hint_row(v->hk[2], v->ha[2], v->hk[3], v->ha[3], 31);
            break;
        }
        cv_blit(0, CAR_Y[i]);
    }
    return (int)any;
}

/* the MENU's carousel: the item's name, its value, OCT+'s and KNOB 1's jobs (none on USB MIDI: shown only) */
static void draw_menu(void)
{
    char v[16];
    car_t c;
    uint32_t i = ui.menu_sel % MI_COUNT;
#if FELUCCA_BLE
    if (ui.menu == 3u) {                            /* the Bluetooth page: its own drawing and signature */
        bt_page_draw();
        return;
    }
#endif
    memset(&c, 0, sizeof c);
    menu_value(i, v);
    c.title = "MENU";
    c.item = MI_NAME[i];
    c.val = v;
    c.note = ui.msg_t ? ui.msg : i == MI_PANEL ? "TEACH BUTTONS AND KNOBS" : "";
    c.pos = i;
    c.n = MI_COUNT;
    c.hk[2] = "OCT-";                              /* (SELECT: the < > and the dots say it) */
    c.ha[2] = "BACK";
    if (i == MI_PANEL) {
        c.hk[0] = "OCT+";
        c.ha[0] = "START";
#if FELUCCA_BLE
    } else if (i == MI_BT) {
        c.hk[0] = "OCT+";
        c.ha[0] = "OPEN";
#endif
    } else if (menu_ok_job(i)) {
        c.hk[0] = "K1";
        c.ha[0] = "SET";
        c.hk[1] = "OCT+";
        c.ha[1] = "NEXT";
    }
    car_draw(&c);
    if (ui.msg_t)
        ui.msg_t--;
}

/* SAVE's pages as carousels (FREITAKT redesign, phase D): PROJECT and COPY a project slot an item (SELECT, or KNOB 1:
 * SLOT, song.g[G_SLOT]; wrapping), TOOLS an action an item (ui.tool; a knob turned shows its action). The knobs do
 * what they did (ui_input.c edit_param): PROJECT K2 NAME, K3 LOAD, K4 SAVE (LOAD and SAVE turned twice: AGAIN: ...);
 * TOOLS K1 CLEAR TRACK, K2 INIT SOUND, K3 CLEAR ALL, K4 INIT ALL (each turned twice); COPY K2 COPY, K3 PASTE (twice),
 * K4 MIDI LEARN on / off. The PRESETS popup (the PRESET knob) over it as on the other pages */
static const char *const TOOL_NAME[4] = {"CLEAR TRACK", "INIT SOUND", "CLEAR ALL", "INIT ALL"};
static const char *const TOOL_NOTE[4] = {"STEPS, MOTION, LOCKS", "MODEL DEFAULT, FILTER", "STEPS, LEN, DIV, SWG",
                                         "PROJECTS, SETTINGS KEPT"};
static uint8_t car_pp;                              /* the PRESETS popup was up (its end: all again) */
static int save_car_on(void) { return !ui.home && !ui.layer && !ui.menu && cur_page()->fam == FAM_SAVE; }
static int save_slots(void) { return cur_page()->id[0] == G_SLOT; }   /* PROJECT, COPY (not TOOLS) */
static void save_select(int32_t s)                  /* SELECT: the next / previous item, wrapping */
{
    if (save_slots())
        song.g[G_SLOT] = (int16_t)(((uint32_t)(song.g[G_SLOT] - 1) + (s > 0 ? 1u : 3u)) % 4u + 1u);
    else
        ui.tool = (uint8_t)((ui.tool + (s > 0 ? 1u : 3u)) % 4u);
}
static void car_word(char *n, const char *w)        /* the note n (28 bytes): w after its words */
{
    uint32_t k = str_len(n);
    if (k)
        n[k++] = ' ';
    str_cpy(n + k, w, 28u - k);
}
static void draw_save(void)
{
    static const char *const KN[4] = {"K1", "K2", "K3", "K4"};
    const page_t *pg = cur_page();
    char t[20], it[2] = {0, 0}, v[NAME_LEN + 8u], n[28];
    car_t c;
    uint32_t pos, np = fam_pages(FAM_SAVE, &pos), k;
    memset(&c, 0, sizeof c);
    str_cpy(t, pg->title, 12);                      /* PROJECT, 1/3 */
    t[13] = (char)('0' + pos);
    t[14] = '/';
    t[15] = (char)('0' + np);
    t[16] = 0;
    c.title = t;
    c.count = t + 13;
    n[0] = 0;
    if (save_slots()) {                             /* a slot: its letter, its name, its state */
        uint32_t s = (uint32_t)(song.g[G_SLOT] - 1) & 3u;
        it[0] = (char)('A' + s);
        if (!project_name(s, v))
            str_cpy(v, "EMPTY", sizeof v);
        else if (!v[0])
            str_cpy(v, "USED", sizeof v);
        if (proj_loaded() == (int32_t)s)
            car_word(n, "LOADED");
        if (slot_dirty(s))
            car_word(n, "EDITED");
        if (copy_src == s + 1u)
            car_word(n, "COPIED");
        c.item = it;
        c.pos = s;
        if (pg->id[1] == G_NAME) {                  /* PROJECT: K2 NAME  K3 LOAD / K4 SAVE x2 */
            c.hk[0] = "K2";
            c.ha[0] = "NAME";
            c.hk[1] = "K3";
            c.ha[1] = "LOAD";
            c.hk[2] = "K4";
            c.ha[2] = "SAVE x2";
        } else {                                    /* COPY: K2 COPY  K3 PASTE / K4 LEARN OFF */
            c.hk[0] = "K2";
            c.ha[0] = "COPY";
            c.hk[1] = "K3";
            c.ha[1] = "PASTE";
            c.hk[2] = "K4";
            c.ha[2] = ui.ml.on ? "LEARN ON" : "LEARN OFF";
        }
    } else {                                        /* TOOLS: the action, whom it hits, what it does */
        k = ui.tool & 3u;
        c.item = TOOL_NAME[k];
        c.pos = k;
        if (k < 2u) {
            str_cpy(v, "TRACK 1", sizeof v);
            v[6] = (char)('1' + song.sel);
        } else {
            str_cpy(v, k == 2u ? "ALL TRACKS" : "EVERYTHING", sizeof v);
        }
        str_cpy(n, TOOL_NOTE[k], sizeof n);
        c.hk[0] = KN[k];
        c.ha[0] = "TURN TWICE";
    }
    c.n = 4;
    c.val = v;
    c.note = ui.msg_t ? ui.msg : n;
    if (car_pp != (uint8_t)pp_on()) {               /* the PRESETS popup came or went: all */
        car_pp = (uint8_t)pp_on();
        ui.force = 1;
    }
    if (car_draw(&c) && pp_on())                    /* (a band over the popup's place: the popup whole again) */
        ui.force = 1;
    if (pp_on())
        draw_popup();
}

static void enc_drop(void)                             /* knob turns nobody takes */
{
    uint32_t k;
    for (k = 0; k < NE; k++)
        panel_enc(k);
}

static void menu_close(void)
{
#if FELUCCA_BLE
    if (ui.menu == 3u)                                 /* HOME held on the Bluetooth page: it closes too */
        bt_page_close();
#endif
    settings_save();                                   /* palette / panel table, if changed */
    ui.menu = 0;
    ui.force = 1;
    go_home();                                         /* HOME in the mode last shown */
}

#define SPLASH_MS 3200u                                /* FREITAKT (owner): 2 s longer; any key skips it */
#define SP_Y 76u                                       /* the name's band: y 76..135 */
#define SP_SUB "Form folgt Function"   /* (MD_F3: drawn in caps) */
/* the start screen (main.c fm1_main, right after lcd_init, while the rest starts): the theme reversed (ORANGE: orange
 * on black), the CRT's beam a line across the middle; the splash runs on from it in the first UI frames */
static void draw_boot_title(void)
{
    lcd_fill(0, 0, 240, 240, C_INK);
    lcd_fill(0, SP_Y + 29u, 240, 2, C_BG);
    ui.splash = 1;
    ui.keys = 1;                                       /* FREITAKT (owner): KEYS on at power-on, SEL lit */
}

/* the FREITAKT boot splash (FREITAKT: Model:Cycles style, light): SPLASH_MS of UI frames after boot, no blocking, no
 * bitmap: the beam sweeps down revealing FREITAKT through scanlines (the rows near it torn sideways), glitch slices
 * that settle, then the clean name, "Form folgt Function" typed under it and a boot bar filling. Any key, button or knob
 * skips it (and does nothing else: ui_input waits for everything let go). Drawn: the name's band (240 x 60) every
 * frame, the subtitle when it grows, the bar's new part */
static void sp_row_fill(int32_t y, uint16_t c)
{
    uint32_t x;
    if (y >= 0 && y < (int32_t)cv_h)
        for (x = 0; x < 240u; x++)
            cv_px[(uint32_t)y * 240u + x] = swap16(c);
}
static void sp_row_shift(int32_t y, int32_t dx, uint16_t g)   /* row y dx px to the right (left: < 0), g in the gap */
{
    uint16_t *r = cv_px + (uint32_t)y * 240u;
    int32_t x;
    if (y < 0 || y >= (int32_t)cv_h || !dx)
        return;
    if (dx > 0)
        for (x = 239; x >= 0; x--)
            r[x] = x >= dx ? r[x - dx] : swap16(g);
    else
        for (x = 0; x < 240; x++)
            r[x] = x - dx < 240 ? r[x - dx] : swap16(g);
}
static void sp_row_inv(int32_t y, uint16_t a, uint16_t b)
{
    uint32_t x;
    uint16_t sa = swap16(a), sb = swap16(b);
    if (y >= 0 && y < (int32_t)cv_h)
        for (x = 0; x < 240u; x++)
            cv_px[(uint32_t)y * 240u + x] = cv_px[(uint32_t)y * 240u + x] == sa ? sb : sa;
}
static uint32_t sp_rnd(uint32_t *r)
{
    *r ^= *r << 13;
    *r ^= *r >> 17;
    *r ^= *r << 5;
    return *r;
}
static int splash_draw(void)                         /* 0: over (the page drawn whole this frame) */
{
    const uint16_t g = C_INK, k = C_BG;               /* the theme reversed */
    uint32_t t, sc, n, cur, w, r;
    int32_t lx, y;
    if (ui.splash == 1) {
        ui.splash = 2;
        ui.sp_t0 = fm1_ms;
        ui.sp_sub = ui.sp_bar = 0;
        lcd_fill(0, 0, 240, 240, g);
    }
    t = fm1_ms - ui.sp_t0;
    if (t >= SPLASH_MS) {
        ui.splash = 0;
        ui.force = 1;
        return 0;
    }
    r = (t / 40u + 1u) * 2654435761u;                 /* (the tears change 25 times a second) */
    sc = md_fit_sc("FREITAKT", 208, 7u);
    lx = 120 - md_text_w(&MD_F3, sc, "FREITAKT") / 2;
    cv_begin(240, 60, g);
    md_text(lx, 30 - 5 * (int32_t)sc / 2, &MD_F3, sc, "FREITAKT", k);
    cv_rect(lx - 10, 4, 12, 2, k);                    /* the corner marks */
    cv_rect(lx - 10, 4, 2, 12, k);
    cv_rect(240 - lx - 2, 54, 12, 2, k);
    cv_rect(240 - lx + 8, 44, 2, 12, k);
    if (t < 500u) {                                   /* the beam's sweep: rows above it through scanlines */
        int32_t b = (int32_t)(t * 60u / 500u);
        for (y = 0; y < 60; y++)
            if (y > b + 1 || (y < b && (y & 1)))
                sp_row_fill(y, g);
        for (y = b - 10; y < b; y++)                  /* (the rows just under the beam torn) */
            sp_row_shift(y, (int32_t)(sp_rnd(&r) % 9u) * 2 - 8, g);
        sp_row_fill(b, k);
        sp_row_fill(b + 1, k);
    } else if (t < 900u) {                            /* glitch slices, settling */
        int32_t amp = (int32_t)(900u - t) * 24 / 400, i;
        for (i = 0; i < 3; i++) {
            int32_t y0 = (int32_t)(sp_rnd(&r) % 54u), h = 2 + (int32_t)(sp_rnd(&r) % 6u);
            int32_t dx = (int32_t)(sp_rnd(&r) % (uint32_t)(2 * amp + 1)) - amp;
            int inv = amp > 8 && (sp_rnd(&r) & 1u);
            for (y = y0; y < y0 + h; y++) {
                sp_row_shift(y, dx, g);
                if (inv)
                    sp_row_inv(y, g, k);
            }
        }
        if (t < 700u)
            for (y = 1; y < 60; y += 2)
                sp_row_fill(y, g);
    }
    cv_blit(0, SP_Y);
    n = t < 600u ? 0u : (t - 600u) / 20u;             /* the subtitle typed, a block cursor after it */
    if (n > sizeof SP_SUB - 1u)
        n = sizeof SP_SUB - 1u;
    cur = n < sizeof SP_SUB - 1u || ((t / 120u) & 1u);
    if (t >= 600u && n * 2u + cur != ui.sp_sub) {
        char b[sizeof SP_SUB];
        int32_t x0 = 120 - md_text_w(&MD_F3, 2, SP_SUB) / 2, x;
        ui.sp_sub = (uint8_t)(n * 2u + cur);
        str_cpy(b, SP_SUB, n + 1u);
        cv_begin(240, 12, g);
        x = md_text(x0, 1, &MD_F3, 2, b, k);
        if (cur)
            cv_rect(n ? x + 2 : x0, 0, 8, 12, k);
        cv_blit(0, SP_Y + 72u);
    }
    w = t * 200u / SPLASH_MS;                         /* the boot bar (x 20..219, y 214) */
    if (w > ui.sp_bar) {
        lcd_fill(20u + ui.sp_bar, 214, w - ui.sp_bar, 3, k);
        ui.sp_bar = (uint8_t)w;
    }
    return 1;
}
/* the splash's input (ui_input): anything pressed or turned skips it; then nothing acts until all is let go */
static void splash_input(uint32_t pressed, uint32_t notes)
{
    uint32_t k, any = pressed | notes;
    for (k = 0; k < NE; k++)
        any |= panel_enc(k) != 0;
    if (ui.splash == 3u) {
        if (!fm1_in.buttons && !fm1_in.notes)
            ui.splash = 0;
    } else if (any) {
        ui.splash = 3;
        ui.force = 1;
    }
}

/* the MENU's carousel: SELECT scrolls the items (wrapping), KNOB 1 sets the item's value, OCT+ steps it (wrapping) or
 * starts it (HARDWARE CALIBRATION, BLUETOOTH), OCT- closes the menu */
static void menu_input(uint32_t pressed)
{
    int32_t s;
    uint32_t ok = (pressed >> panel.btn[B_OCTUP]) & 1u, back = (pressed >> panel.btn[B_OCTDN]) & 1u;
#if FELUCCA_BLE
    if (ui.menu == 3u) {                               /* the Bluetooth page (OCT-: back to the carousel) */
        bt_page_input(pressed);
        enc_drop();
        return;
    }
#endif
    if (back) {
        menu_close();
        return;
    }
    if ((s = panel_enc(EN_SELECT)) != 0)
        ui.menu_sel = (uint8_t)((ui.menu_sel + (s > 0 ? 1u : MI_COUNT - 1u)) % MI_COUNT);
    s = panel_enc(EN_K1);
    if (s != 0 && ui.menu_sel == MI_COLOR) {
        settings.palette = theme_step(settings.palette, s);
        theme_set(settings.palette);
        ui.force = 1;                               /* every region in the new colours */
    }
    if ((s != 0 || ok) && ui.menu_sel == MI_SPKEQ) {
        /* KNOB 1 steps FLAT LOWCUT BASS+ and stops at the ends; OCT+ steps and wraps (upstream 1.0.2) */
        settings.lowcut = s > 0 ? (settings.lowcut < 2u ? settings.lowcut + 1u : 2u)
                        : s < 0 ? (settings.lowcut ? settings.lowcut - 1u : 0u) : (settings.lowcut + 1u) % 3u;
        fx_lowcut = (uint8_t)settings.lowcut;
        ok = 0;
        s = 0;
    }
    if ((s != 0 || ok) && ui.menu_sel == MI_BACKLIGHT) {
        /* BACKLIGHT (item 9): KNOB 1 steps OFF DIM LO DIM HI INV INV LO INV HI and stops at the ends; OCT+ steps and
         * wraps (as SPEAKER EQ) */
        backlight = s > 0 ? (backlight + 1u < BL_COUNT ? backlight + 1u : BL_COUNT - 1u)
                  : s < 0 ? (backlight ? backlight - 1u : 0u) : (backlight + 1u) % BL_COUNT;
        ok = 0;
        s = 0;
    }
    if ((s != 0 || ok) && menu_glob(ui.menu_sel) != 0xFFu) {
        /* a settings item: KNOB 1 turns it, OCT+ steps it and wraps; USB MIDI: only shown */
        uint32_t g = menu_glob(ui.menu_sel);
        if (g != G_MIDI) {
            int16_t *vp;
            const param_desc_t *d = glob_desc(g, &vp);
            int32_t v = s ? knob_turn(EN_K1, d, *vp, s) : *vp >= d->max ? d->min : *vp + 1;
            *vp = (int16_t)v;
            glob_set(g, v);
        }
        ok = 0;
        s = 0;
    }
    if ((s != 0 || ok) && menu_onoff(ui.menu_sel)) {
        /* KNOB 1: right = ON, left = OFF; OCT+ toggles */
        uint32_t *v = menu_onoff(ui.menu_sel);
        *v = s > 0 ? 1u : s < 0 ? 0u : !*v;
        fx_usb_fixed = (uint8_t)settings.usbfix;
        ok = 0;
    }
    if (ok) {
        switch (ui.menu_sel) {
        case MI_COLOR:                                 /* OCT+ steps through the themes too */
            settings.palette = theme_step(settings.palette, 1);
            theme_set(settings.palette);
            ui.force = 1;
            break;
        case MI_PANEL:
            panel_setup();
            ui.force = 1;
            break;
#if FELUCCA_BLE
        case MI_BT:
            bt_page_open();
            break;
#endif
        default:
            break;
        }
    }
    enc_drop();                                        /* swallow the rest while the menu is up */
}

