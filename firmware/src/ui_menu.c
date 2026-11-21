/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca menu (HOME held): COLOR, SPEAKER EQ (FLAT LOWCUT BASS+), USB LEVEL (MASTER FIXED), PERFORM (HOLD PAGE), KNOB ACCEL, BACKLIGHT (OFF DIM LO DIM HI INV INV LO INV HI), HARDWARE CALIBRATION, (BLE build: BLUETOOTH,) ABOUT. */
/* ------------------------------------------------------------ menu --- */
/* the device settings the GLOBAL / SYSTEM pages had (FREITAKT redesign: those pages went; BPM and SWING are HOME's
 * knobs, CPU HOME's top line): a row each, KNOB 1 turns it, OCT+ steps it (wrapping); USB MIDI only shows the USB
 * state. Their values where they always were (song.g, the device settings: pages.c glob_desc, ui_input.c glob_set) */
#define MI_SETTINGS_ROWS MI_CLOCK, MI_SYNC, MI_DRCH, MI_ROUTE, MI_USBMIDI, MI_METRO, MI_MUTE, MI_THRU, MI_QUANT
#define MI_SETTINGS_NAMES "CLOCK", "MIDI SYNC", "MIDI CHANNEL", "MIDI ROUTE", "USB MIDI", "METRONOME", "MUTE MODE", \
                          "MIDI THRU", "REC QUANT"
static const uint8_t MI_GLOB[9] = {G_CLOCK, G_SYNC, G_DRCH, G_ROUTE, G_MIDI, G_METRO, G_MUTEBAR, G_THRU, G_QUANT};
static int glob_set(uint32_t id, int32_t v);        /* ui_input.c */
static int32_t knob_turn(uint32_t role, const param_desc_t *d, int32_t v, int32_t s);
#if FELUCCA_BLE                                     /* the Bluetooth page's row (bt_page.c) before ABOUT */
enum { MI_COLOR, MI_SPKEQ, MI_USB, MI_PERF, MI_ACCEL, MI_BACKLIGHT, MI_SETTINGS_ROWS, MI_PANEL, MI_BT, MI_ABOUT,
       MI_BACK, MI_COUNT };
static const char *const MI_NAME[MI_COUNT] = {"COLOR", "SPEAKER EQ", "USB LEVEL", "PERFORM", "ACCEL",
                                              "BACKLIGHT", MI_SETTINGS_NAMES, "HARDWARE CALIBRATION", "BLUETOOTH",
                                              "ABOUT", "BACK"};
#define MENU_DY 14u                                 /* eleven rows shown above the hints (screen y), scrolled */
#define MENU_HY 196u
static void bt_page_open(void);                     /* bt_page.c (after ble/ble_core.h): ui.menu 3 */
static void bt_page_close(void);
static void bt_page_draw(void);
static void bt_page_input(uint32_t pressed);
static void bt_autostart(void);                     /* AUTOSTART: once, 5 s after power-on (ui_input only, gate 3) */
#else
enum { MI_COLOR, MI_SPKEQ, MI_USB, MI_PERF, MI_ACCEL, MI_BACKLIGHT, MI_SETTINGS_ROWS, MI_PANEL, MI_ABOUT,
       MI_BACK, MI_COUNT };
static const char *const MI_NAME[MI_COUNT] = {"COLOR", "SPEAKER EQ", "USB LEVEL", "PERFORM", "ACCEL",
                                              "BACKLIGHT", MI_SETTINGS_NAMES, "HARDWARE CALIBRATION", "ABOUT", "BACK"};
#define MENU_DY 14u                                 /* eleven rows shown above the hints (screen y), scrolled */
#define MENU_HY 196u
#endif
#define MENU_Y0 40u                                 /* the first row (screen y), inside the MENU window */
#define MENU_ROWS ((MENU_HY - MENU_Y0) / MENU_DY)   /* the rows shown at once (11) */
static const char *const SPK_EQ[3] = {"FLAT", "LOWCUT", "BASS+"};   /* settings.lowcut (fx_lowcut) */
static uint32_t *menu_onoff(uint32_t i)            /* an ON / OFF row's setting, 0 if none */
{
    return i == MI_USB ? &settings.usbfix : i == MI_PERF ? &settings.perfpage :
           i == MI_ACCEL ? &settings.accel : 0;
}
static uint32_t menu_glob(uint32_t i)               /* a settings row's global id, 0xFF if none */
{
    return i >= MI_CLOCK && i <= MI_QUANT ? MI_GLOB[i - MI_CLOCK] : 0xFFu;
}
static void menu_glob_text(uint32_t id, char *b)    /* its value as the row shows it (b: 16 bytes) */
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
static uint32_t menu_glob_sig(void)                 /* what the settings rows show */
{
    uint32_t i, h = 2166136261u;
    char b[16];
    for (i = 0; i < 9u; i++) {
        menu_glob_text(MI_GLOB[i], b);
        h = str_hash(h, b) * 31u + i;
    }
    return h;
}

/* ABOUT (owner 2026-10-22): the credits under the fixed logo, one string a credit, wrapped at its spaces to the text's
 * width (a credit's further lines indented); SELECT scrolls them a row a detent (ui.about_top), clamped at both ends;
 * a thumb at the right shows where. LICENSING.md and README's Credits say the same in full. */
static const char *const AB[] = {
    "DRUM MACHINE ON FELUCCA", FELUCCA_VERSION, "FORK: DEADACTIVE",
    "FELUCCA: LEO KUROSHITA, H\xDCGELTON INSTRUMENTS", "GITHUB.COM/HUGELTON/FELUCCA",
    "MUTABLE INSTRUMENTS, EMILIE GILLET: PLAITS + STMLIB (DRUM ALGORITHMS, MIT), "
    "STREAMS (COMPRESSOR, MIT)",
    "DAISYSP, ELECTROSMITH + EMILIE GILLET: MODAL / MEMBRANE (SKIN; MIT)",
    "SAMPLES: VERSILIAN STUDIOS VSCO-2 CE / VCSL (CC0) + H\xDCGELTON SAMPLE PACK",
    "PIXEL FONT: DEADACTIVE",
    "MD VOICES MEASURED FROM JOE LANDERS' MACHINEDRUM EMULATOR, A FORK OF GEARMULATOR BY THE USUAL SUSPECTS (DSP56300 "
    "PROJECT): MEASUREMENTS ONLY, NO CODE OR DATA FROM IT",
    "MACHINEDRUM IS A TRADEMARK OF ELEKTRON. NOT AFFILIATED WITH ELEKTRON, JOE LANDERS OR THE USUAL SUSPECTS",
    "JIELI AC79 SDK (APACHE-2.0) IN THE PACKAGE",
    "RECOVERY TOOL: JL-UBOOT-TOOL BY KAGAIMIQ (ANDREY GRIGORYEV), MIT",
    "WEB FONT: TERMINUS (DIMITAR ZHEKOV, OFL)", "WEB ICONS: FUKIAI (H\xDCGELTON INSTRUMENTS, MIT)",
    "BOOT SCREEN STUDY: UNIFRAKTUR FONTS (J. 'MACH' WUST, PETER WIEGEL, OFL)",
    "GPL-3.0, NO WARRANTY"};
#define AB_Y0 78                                    /* the first row (under the logo), 14 px rows, eleven shown */
#define AB_DY 14
#define AB_ROWS 11u
#define AB_X 6
#define AB_IND 12                                   /* a credit's further lines */
#define AB_R 228                                    /* the text's right edge (exclusive) */
#define AB_BX 232                                   /* the thumb: x 232..234 in rows 78..227 */
#define AB_BH (int32_t)(AB_ROWS * AB_DY - 4u)
#define AB_MAXL 64u
static struct { uint8_t c, at, n; } ab_line[AB_MAXL];   /* the wrapped rows: credit, first character, length */
static uint32_t ab_lines;

static void ab_str(char *b, uint32_t r)              /* row r's text */
{
    memcpy(b, AB[ab_line[r].c] + ab_line[r].at, ab_line[r].n);
    b[ab_line[r].n] = 0;
}

static void about_wrap(void)                         /* once: the credits as rows */
{
    uint32_t c, len, at, end, cut, fit, n = 0;
    char b[64];
    if (ab_lines)
        return;
    for (c = 0; c < sizeof AB / sizeof AB[0]; c++) {
        const char *s = AB[c];
        for (len = 0; s[len]; len++)
            ;
        for (at = 0; at < len && n < AB_MAXL;) {
            int32_t x = at ? AB_X + AB_IND : AB_X;
            for (cut = fit = 0, end = at + 1u; end <= len && end - at < sizeof b; end++) {
                memcpy(b, s + at, end - at);
                b[end - at] = 0;
                if (x + md_text_w(&MD_F3, 2, b) > AB_R)
                    break;
                fit = end;
                if (end == len || s[end] == ' ')
                    cut = end;                      /* the last word end that fits */
            }
            if (!cut)                               /* (a word wider than the row: as much as fits) */
                cut = fit > at ? fit : at + 1u;
            ab_line[n].c = (uint8_t)c;
            ab_line[n].at = (uint8_t)at;
            ab_line[n].n = (uint8_t)(cut - at);
            n++;
            for (at = cut; at < len && s[at] == ' '; at++)
                ;
        }
    }
    ab_lines = n;
}

static uint32_t about_max(void)                      /* the last first row: the last credit on the last row */
{
    about_wrap();
    return ab_lines > AB_ROWS ? ab_lines - AB_ROWS : 0u;
}

static void draw_about(void)                         /* the credits' rows and the thumb (into the band's canvas) */
{
    uint32_t i, r;
    char b[64];
    int32_t th, ty, mx = (int32_t)about_max();
    for (i = 0; i < AB_ROWS && (r = ui.about_top + i) < ab_lines; i++) {
        int32_t y = AB_Y0 + (int32_t)i * AB_DY;
        ab_str(b, r);
        md_text(ab_line[r].at ? AB_X + AB_IND : AB_X, y, &MD_F3, 2, b, C_INK);
        if (ab_line[r].c == 1u && !ab_line[r].at)   /* the version's row: the build date at the right */
            md_text(AB_R - md_text_w(&MD_F3, 2, __DATE__), y, &MD_F3, 2, __DATE__, C_INK);
    }
    th = mx ? AB_BH * (int32_t)AB_ROWS / (int32_t)ab_lines : AB_BH;
    th = th < 10 ? 10 : th;
    ty = AB_Y0 + (mx ? (AB_BH - th) * (int32_t)ui.about_top / mx : 0);
    cv_rect(AB_BX, ty, 3, th, C_INK);
}

static void draw_menu(void)
{
    static int16_t pk[8];                           /* ABOUT: the logo's peak dots */
    static uint32_t pf;                             /* the logo's frame shown */
    uint32_t i, pass, fr = fm1_ms / 66u,
                      sig = ui.menu * 7u + ui.menu_sel * 131u + settings.palette * 1009u + settings.lowcut * 7919u +
                            settings.accel * 15485863u + settings.usbfix * 32452843u +
                            settings.perfpage * 49979687u + backlight * 67867967u + ui.about_top * 86028121u +
                            (ui.menu == 1 ? menu_glob_sig() : 0u);
#if FELUCCA_BLE
    if (ui.menu == 3u) {                            /* the Bluetooth page: its own drawing and signature */
        bt_page_draw();
        return;
    }
#endif
    if (!ui.force && sig == ui.menu_sig) {
        if (ui.menu == 2 && fr != pf) {             /* ABOUT: the logo moves (15 fps), alone: rows 17..77, x 82..157
                                                     * (an even x: the dither keeps its phase) */
            pf = fr;
            cv_begin_at(76, 61, 17, C_BG);
            cv_oy = -17;
            cv_vu(2, 18, fr, 1, pk);
            cv_blit(82, 17);
        }
        return;
    }
    ui.menu_sig = sig;
    ui.force = 1;
    draw_head();                                    /* the top bar as everywhere */
    for (pass = 0; pass < 2u; pass++) {             /* the canvas holds 124 rows: two bands, screen rows 16..239 */
        uint32_t y0 = H_HEAD + pass * 124u, h = pass ? 240u - H_HEAD - 124u : 124u;
        cv_begin_at(240, h, y0, C_BG);
        cv_oy = -(int32_t)y0;                       /* (drawn in screen rows) */
        cv_rect(0, (int32_t)y0, 1, (int32_t)h, C_INK);
        cv_rect(239, (int32_t)y0, 1, (int32_t)h, C_INK);
        cv_rect(0, 239, 240, 1, C_INK);
        if (ui.menu == 2) {                         /* ABOUT: the logo, the credits (scrolled) */
            if (pass == 0u && fr != pf) {           /* (the peaks step once a frame, not once a band) */
                pf = fr;
                cv_vu(84, 18, fr, 1, pk);
            } else {
                int16_t keep[8];
                memcpy(keep, pk, sizeof keep);
                cv_vu(84, 18, fr, 1, keep);
            }
            draw_about();
        } else {                                    /* MENU: a window, the cursor an inverted bar */
            uint32_t top = ui.menu_sel >= MENU_ROWS ? ui.menu_sel + 1u - MENU_ROWS : 0u;   /* (scrolled: the cursor's
                                                                                    * row the last shown) */
            char gb[16];
            cv_popup(6, 30, 228, 184, "MENU", C_INK, C_BG);
            if (top)                                /* more above / below: a small arrow at the right edge */
                cv_rect(226, 36, 3, 2, C_INK), cv_rect(227, 35, 1, 1, C_INK);
            if (top + MENU_ROWS < MI_COUNT)
                cv_rect(226, (int32_t)(MENU_Y0 + MENU_ROWS * MENU_DY) - 4, 3, 2, C_INK),
                    cv_rect(227, (int32_t)(MENU_Y0 + MENU_ROWS * MENU_DY) - 2, 1, 1, C_INK);
            for (i = top; i < MI_COUNT && i < top + MENU_ROWS; i++) {
                int32_t y = (int32_t)(MENU_Y0 + (i - top) * MENU_DY);
                int sel = i == ui.menu_sel;
                const char *v = 0;
                if (menu_glob(i) != 0xFFu) {
                    menu_glob_text(menu_glob(i), gb);
                    v = gb;
                }
                if (menu_onoff(i))
                    v = i == MI_USB ? (*menu_onoff(i) ? "FIXED" : "MASTER") : i == MI_PERF ? (*menu_onoff(i) ? "PAGE" : "HOLD") :
                        *menu_onoff(i) ? "ON" : "OFF";
                if (i == MI_SPKEQ)
                    v = SPK_EQ[settings.lowcut % 3u];
                if (i == MI_BACKLIGHT)              /* (item 9: upstream's LEDS levels) */
                    v = BL_NAME[backlight % BL_COUNT];
                if (i == MI_COLOR)                  /* the theme's name (owner 2026-10-23) */
                    v = THEMES[theme_index(settings.palette)].name;
                list_row(10, 225, y, MI_NAME[i], v, sel);
            }
            i = (uint32_t)cv_keycap(12, (int32_t)MENU_HY, "OCT+", "OK", C_INK);
            i = (uint32_t)cv_keycap((int32_t)i + 8, (int32_t)MENU_HY, "OCT-", "BACK", C_INK);
            cv_keycap((int32_t)i + 8, (int32_t)MENU_HY, "SEL", "MOVE", C_INK);
        }
        cv_oy = 0;
        cv_blit(0, y0);
    }
    ui.force = 0;
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

#define SPLASH_MS 1200u
#define SP_Y 76u                                       /* the name's band: y 76..135 */
#define SP_SUB "Form folgt Function"   /* (MD_F3: drawn in caps) */
/* the start screen (main.c fm1_main, right after lcd_init, while the rest starts): the theme reversed (ORANGE: orange
 * on black), the CRT's beam a line across the middle; the splash runs on from it in the first UI frames */
static void draw_boot_title(void)
{
    lcd_fill(0, 0, 240, 240, C_INK);
    lcd_fill(0, SP_Y + 29u, 240, 2, C_BG);
    ui.splash = 1;
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

/* menu: SELECT moves, OCT+ confirms, OCT- cancels (ABOUT -> list -> close) */
static void menu_input(uint32_t pressed)
{
    int32_t s;
    uint32_t ok = (pressed >> panel.btn[B_OCTUP]) & 1u, back = (pressed >> panel.btn[B_OCTDN]) & 1u;
#if FELUCCA_BLE
    if (ui.menu == 3u) {                               /* the Bluetooth page (OCT-: back to the list) */
        bt_page_input(pressed);
        enc_drop();
        return;
    }
#endif
    if (back) {
        if (ui.menu == 2)
            ui.menu = 1, ui.force = 1;
        else
            menu_close();
        return;
    }
    if ((s = panel_enc(EN_SELECT)) != 0 && ui.menu == 1)
        ui.menu_sel = (uint8_t)((ui.menu_sel + (s > 0 ? 1u : MI_COUNT - 1u)) % MI_COUNT);
    else if (s != 0 && ui.menu == 2) {                 /* ABOUT: the credits a row a detent, clamped */
        int32_t t = (int32_t)ui.about_top + s, mx = (int32_t)about_max();
        ui.about_top = (uint8_t)(t < 0 ? 0 : t > mx ? mx : t);
    }
    s = panel_enc(EN_K1);
    if (s != 0 && ui.menu == 1 && ui.menu_sel == MI_COLOR) {
        settings.palette = theme_step(settings.palette, s);
        theme_set(settings.palette);
        ui.force = 1;                               /* every region in the new colours */
    }
    if ((s != 0 || ok) && ui.menu == 1 && ui.menu_sel == MI_SPKEQ) {
        /* KNOB 1 steps FLAT LOWCUT BASS+ and stops at the ends; OCT+ steps and wraps (upstream 1.0.2) */
        settings.lowcut = s > 0 ? (settings.lowcut < 2u ? settings.lowcut + 1u : 2u)
                        : s < 0 ? (settings.lowcut ? settings.lowcut - 1u : 0u) : (settings.lowcut + 1u) % 3u;
        fx_lowcut = (uint8_t)settings.lowcut;
        ok = 0;
        s = 0;
    }
    if ((s != 0 || ok) && ui.menu == 1 && ui.menu_sel == MI_BACKLIGHT) {
        /* BACKLIGHT (item 9): KNOB 1 steps OFF DIM LO DIM HI INV INV LO INV HI and stops at the ends; OCT+ steps and
         * wraps (as SPEAKER EQ) */
        backlight = s > 0 ? (backlight + 1u < BL_COUNT ? backlight + 1u : BL_COUNT - 1u)
                  : s < 0 ? (backlight ? backlight - 1u : 0u) : (backlight + 1u) % BL_COUNT;
        ok = 0;
        s = 0;
    }
    if ((s != 0 || ok) && ui.menu == 1 && menu_glob(ui.menu_sel) != 0xFFu) {
        /* a settings row: KNOB 1 turns it, OCT+ steps it and wraps; USB MIDI: only shown */
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
    if ((s != 0 || ok) && ui.menu == 1 && menu_onoff(ui.menu_sel)) {
        /* KNOB 1: right = ON, left = OFF; OCT+ toggles */
        uint32_t *v = menu_onoff(ui.menu_sel);
        *v = s > 0 ? 1u : s < 0 ? 0u : !*v;
        fx_usb_fixed = (uint8_t)settings.usbfix;
        ok = 0;
    }
    if (ok && ui.menu == 1) {
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
        case MI_ABOUT:
            ui.menu = 2;
            ui.about_top = 0;                          /* the credits from the top */
            ui.force = 1;
            break;
        default:
            menu_close();
            break;
        }
    }
    enc_drop();                                        /* swallow the rest while the menu is up */
}

