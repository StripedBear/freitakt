/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* H1 (spec docs/superpowers/specs/2026-10-05-device-readiness-design.md §3): the drum firmware's boot path,
 * main.c's order, with our real code, against hostile flash and .noinit contents, built with ASan + UBSan.
 * H5 in the boot: the same cases with SEQ held. A sanitizer report aborts the run (nonzero exit). */
#define FELUCCA_FLASH 1
#include <stdint.h>
static uint8_t hflash[0x100000];
#define SMP_USER_XIP(k) ((const uint8_t *)hflash + 0xA0000u + (k) * 0x14000u)
#include "ui_host.h"

static int fails;
static int check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        fails++;
    return ok;
}

static uint32_t rs = 1;
static uint32_t rnd(void) { return rs = rs * 1664525u + 1013904223u; }
static void rfill(void *p, uint32_t n)
{
    uint8_t *b = p;
    while (n--)
        *b++ = (uint8_t)(rnd() >> 24);
}

static int bank_events_eq(const lk_bank_t *a, const lk_bank_t *b)   /* the same events, kinds and PLAY bits */
{
    uint32_t i;
    if (a->count != b->count || a->on != b->on)
        return 0;
    for (i = 0; i < a->count; i++)
        if (a->key[i] != b->key[i] || a->val[i] != b->val[i] || lk_is_lock(a, i) != lk_is_lock(b, i))
            return 0;
    return 1;
}

/* a sample slot k with a valid header and random (or plausible) zones */
static void slot_header(uint32_t k, int plausible)
{
    smp_user_hdr_t h;
    uint32_t i;
    rfill(&h, sizeof h);
    h.magic = SMP_USER_MAGIC;
    h.version = 1;
    h.nz = (uint8_t)(1u + rnd() % 16u);
    h.data_len = plausible ? 0x8000u : rnd();
    for (i = 0; plausible && i < h.nz; i++) {
        smp_zone_t *z = &h.zone[i];
        z->off = 0;
        z->n = 1000u + rnd() % 30000u;
        z->idx = 0;
        z->rate = 1u << 16;
        z->ls = 0;
        z->le = z->n - 1u;
        z->lo = 0;
        z->hi = 127;
    }
    memcpy(hflash + 0xA0000u + k * 0x14000u, &h, sizeof h);
}

/* an old record (another magic, size n): random fields, a valid size field and sum, saved as object obj */
static void old_record(uint32_t obj, uint32_t magic, uint32_t n)
{
    static uint8_t buf[4096];
    uint32_t h;
    rfill(buf, n);
    memcpy(buf, &magic, 4);
    memcpy(buf + 4, &n, 4);
    h = proj_hash(buf, n - 4u);
    memcpy(buf + n - 4u, &h, 4);
    st_save(obj, buf, n);
}

enum { F_ERASED, F_ZERO, F_RANDOM, F_HEADERS, F_FELUCCA, F_KINDS };
static const char *const KIND[F_KINDS] = {"erased", "zeros", "random", "valid headers, garbage fields", "felucca records"};

static void flash_image(int kind, uint32_t seed)
{
    rs = seed * 2654435761u + 7u;
    memset(hflash, kind == F_ZERO ? 0x00 : 0xFF, sizeof hflash);
    if (kind == F_RANDOM)
        rfill(hflash + 0x93000u, sizeof hflash - 0x93000u);   /* the data area (the app area is ours) */
    if (kind == F_HEADERS || kind == F_FELUCCA) {
        persist_t p;
        project_t pr;
        uint32_t k;
        for (k = 0; k < SMP_USER_SLOTS; k++)
            slot_header(k, kind == F_FELUCCA);
        rfill(&p, sizeof p);
        p.magic = PERSIST_MAGIC;
        if (kind == F_FELUCCA)
            p.panel = PANEL_DEFAULT;
        else
            p.panel.magic = PANEL_MAGIC;              /* a valid panel magic over random ids */
        st_save(OBJ_SETTINGS, &p, sizeof p);
        for (k = 0; k < 4; k++) {
            if (kind == F_HEADERS && k == 1u) {      /* Hortator's 8-track "FDRG": valid sum over random fields */
                old_record(OBJ_PROJECT0 + k, 0x47524446u, 3840u);
                continue;
            }
            if (kind == F_HEADERS && k == 2u) {      /* an M1 project ("FDR1"), an old size */
                old_record(OBJ_PROJECT0 + k, 0x31524446u, 3208u);
                continue;
            }
            if (kind == F_HEADERS && k == 3u) {      /* "FRT6" magic over a wrong size, random rest */
                rfill(&pr, sizeof pr);
                pr.magic = PROJ_MAGIC;
                pr.size = sizeof pr - 4u;
                st_save(OBJ_PROJECT0 + k, &pr, sizeof pr);
                continue;
            }
            rfill(&pr, sizeof pr);
            pr.magic = kind == F_FELUCCA ? 0x334E5546u : PROJ_MAGIC;   /* Felucca's "FUN3", or ours */
            pr.size = sizeof pr;
            pr.sum = proj_sum(&pr);                   /* ours: valid sum over random tracks */
            st_save(OBJ_PROJECT0 + k, &pr, sizeof pr);
        }
    }
}

static int32_t L[CTL], R[CTL];

/* main.c's order (fm1_main): persist_boot, settings_init, (LCD), input / ADC init, panel_init,
 * felucca_init = drum_boot_init, audio, USB, IRQs; then the main loop (UI + the update service) */
static int boot(int kind, uint32_t seed, int seq)
{
    uint32_t f, i, bounded = 1, peak = 0;
    flash_image(kind, seed);
    rfill(&settings, sizeof settings);               /* .noinit RAM after the stock firmware: anything */
    rfill(proj_slot, sizeof proj_slot);
    host_init();
    memset(&ui, 0, sizeof ui);
    persist_boot();
    if (kind == F_FELUCCA && !usr_nz[0])               /* the glue reads the image: valid slots are found */
        return 0;
    settings_init();
    panel_init();
    fm1_in.buttons = seq ? 1u << panel.btn[B_SEQ] : 0u;
    drum_boot_init();
    fm1_in.buttons = 0;
    for (f = 0; f < 2u * FS; f += CTL) {             /* 2 s of the audio ISR */
        render_mix(L, R, CTL);
        for (i = 0; i < CTL; i++) {
            bounded &= L[i] <= 32767 && L[i] >= -32768 && R[i] <= 32767 && R[i] >= -32768;
            peak |= (uint32_t)abs(L[i]) | (uint32_t)abs(R[i]);
        }
    }
    for (i = 0; i < 30; i++)                         /* the main loop's first UI frames */
        ui_frame();
    for (i = 0; !seq && i < 4; i++) {                /* the first hands-on minutes: load each slot, play */
        project_load(i);
        transport_req = 1;
        render_mix(0, 0, CTL * 64);
        ui_frame();
    }
    return bounded && settings.zoom <= 1u && settings.mutebar <= 1u && (seq ? safe_start && peak == 0 : !safe_start);
}

/* MUTE NEXT BAR (TRACKS) survives a power cycle beside ZOOM, in Felucca's settings format (zoom's bit 1) */
static int mutebar_persists(void)
{
    int ok = 1;
    uint32_t z, m;
    for (z = 0; z < 2u; z++)
        for (m = 0; m < 2u; m++) {
            memset(hflash, 0xFF, sizeof hflash);
            host_init();
            persist_boot();                          /* finds the flash */
            settings.magic = SETTINGS_MAGIC;
            settings.palette = THEME_TAG | 1u;
            settings.lowcut = 0;
            settings.zoom = z;
            settings.mutebar = m;
            settings.accel = (z ^ m) & 1u;
            settings_save();
            rfill(&settings, sizeof settings);       /* power off: .noinit is anything */
            persist_boot();
            settings_init();
            ok &= settings.zoom == z && settings.mutebar == m && settings.accel == ((z ^ m) & 1u);
        }
    return ok;
}

/* USB LEVEL (MENU): FIXED survives a power cycle in bit 3 of the stored zoom word; MASTER when it is clear (a 0.7.0
 * save); the other settings read back unchanged */
static int usb_level_persists(void)
{
    int ok = 1;
    uint32_t u;
    for (u = 0; u < 2u; u++) {
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        settings.magic = SETTINGS_MAGIC;
        settings.palette = THEME_TAG | 2u;
        settings.lowcut = 2;
        settings.zoom = 1;
        settings.mutebar = 1;
        settings.accel = 0;
        settings.usbfix = u;
        settings.perfpage = u;
        settings_save();
        rfill(&settings, sizeof settings);           /* power off: .noinit is anything */
        persist_boot();
        settings_init();
        ok &= settings.usbfix == u && settings.perfpage == u && fx_usb_fixed == u && settings.palette == (THEME_TAG | 2u) && settings.lowcut == 2 &&
              settings.zoom == 1 && settings.mutebar == 1 && settings.accel == 0;
    }
    return ok;
}

/* MENU > USB AUDIO 10 CH (removed in FREITAKT): bits 12..13 of the stored palette word are reserved: written 0, and
 * a record that has them set (an older Hortator's 10 CH MIX / FX) loads the rest as before, the bits ignored */
static int usb_audio_persists(void)
{
    int ok = 1;
    uint32_t u, want[6] = {0};
    {
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        settings.magic = SETTINGS_MAGIC;
        settings.palette = THEME_TAG | 2u;
        settings.lowcut = 1;
        settings.usbfix = 1;
        btset.bt_auto = 1;
        backlight = BL_DIM_HI;
        settings_save();
        st_load(OBJ_SETTINGS, &p, sizeof p);
        ok &= ((p.palette >> 12) & 3u) == 0u && p.palette >> 14 == 0u;
    }
    for (u = 0; u < 4u; u++) {                       /* an older record: 2 CH, 10 CH MIX, 10 CH FX, none */
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        memset(&p, 0, sizeof p);
        p.magic = PERSIST_MAGIC;
        p.palette = 2u | 1u << 8 | (uint32_t)BL_DIM_LO << 9 | u << 12;
        p.panel = PANEL_DEFAULT;
        st_save(OBJ_SETTINGS, &p, sizeof p);
        rfill(&settings, sizeof settings);           /* power off */
        persist_boot();
        settings_init();
        {
            uint32_t got[6] = {settings.palette, btset.bt_auto, backlight, midi_thru, solo_mom, rec_quant};
            if (!u)
                memcpy(want, got, sizeof want);      /* bits 12..13 clear: the reference */
            ok &= !memcmp(want, got, sizeof want) && btset.bt_auto == 1u;
        }
    }
    return ok;
}

/* GLOBAL 3/3 THRU (midi_out.c midi_thru) survives a power cycle in bit 14 of the stored palette word, the other bits as
 * they were; a record without it (every older one) and none read OFF */
static int thru_persists(void)
{
    int ok = 1;
    uint32_t t, low = 0;
    for (t = 0; t < 2u; t++) {
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        settings.magic = SETTINGS_MAGIC;
        settings.palette = THEME_TAG | 2u;
        btset.bt_auto = 1;
        backlight = BL_DIM_HI;
        fx_metro = 2;
        midi_thru = (uint8_t)t;
        settings_save();
        st_load(OBJ_SETTINGS, &p, sizeof p);
        ok &= ((p.palette >> 14) & 1u) == t && p.palette >> 15 == 0u;
        if (!t)
            low = p.palette & 0x3FFFu;
        else
            ok &= (p.palette & 0x3FFFu) == low;
        rfill(&settings, sizeof settings);           /* power off */
        midi_thru = 7;
        fx_metro = 0;
        persist_boot();
        settings_init();
        ok &= midi_thru == t && fx_metro == 2u && btset.bt_auto == 1u && backlight == BL_DIM_HI &&
              settings.palette == (THEME_TAG | 2u);
    }
    {   /* an older record: OFF; no record: OFF */
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        memset(&p, 0, sizeof p);
        p.magic = PERSIST_MAGIC;
        p.palette = 2u | 1u << 8 | (uint32_t)BL_DIM_LO << 9 | 1u << 12;
        p.panel = PANEL_DEFAULT;
        st_save(OBJ_SETTINGS, &p, sizeof p);
        midi_thru = 1;
        persist_boot();
        ok &= midi_thru == 0u;
        memset(hflash, 0xFF, sizeof hflash);
        midi_thru = 1;
        persist_boot();
        ok &= midi_thru == 0u;
    }
    return ok;
}

/* QUANT (spec 2026-10-31 §2: seq.c rec_quant) survives a power cycle in bits 16..17 of the stored palette word, the
 * other bits as they were; a record without it (every older one) and none read 1/16 */
static int quant_persists(void)
{
    int ok = 1;
    uint32_t q, low = 0;
    for (q = 0; q < QUANT_N; q++) {
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        settings.magic = SETTINGS_MAGIC;
        settings.palette = THEME_TAG | 2u;
        midi_thru = 1;
        solo_mom = 1;
        rec_quant = (uint8_t)q;
        settings_save();
        st_load(OBJ_SETTINGS, &p, sizeof p);
        ok &= ((p.palette >> 16) & 3u) == q && p.palette >> 18 == 0u;
        if (!q)
            low = p.palette & 0xFFFFu;
        else
            ok &= (p.palette & 0xFFFFu) == low;
        rfill(&settings, sizeof settings);           /* power off */
        rec_quant = 9;
        midi_thru = 0;
        persist_boot();
        settings_init();
        ok &= rec_quant == q && midi_thru == 1u && solo_mom == 1u && settings.palette == (THEME_TAG | 2u);
    }
    {   /* an older record: 1/16; no record: 1/16 */
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        memset(&p, 0, sizeof p);
        p.magic = PERSIST_MAGIC;
        p.palette = 2u | 1u << 8 | 1u << 14 | 1u << 15;
        p.panel = PANEL_DEFAULT;
        st_save(OBJ_SETTINGS, &p, sizeof p);
        rec_quant = QUANT_OFF;
        persist_boot();
        ok &= rec_quant == QUANT_16;
        memset(hflash, 0xFF, sizeof hflash);
        rec_quant = QUANT_OFF;
        persist_boot();
        ok &= rec_quant == QUANT_16;
    }
    solo_mom = 0;
    midi_thru = 0;
    return ok;
}

/* GLO SYNC / ROUT are project globals: a project saved before them (0) loads SYNC OFF, ROUT NOTE; ROUT kept in its
 * range; SYNC as this build has it (without Bluetooth: BLE OFF, BOTH USB) and a loaded slot shows no edit for it */
static int sync_rout_load(void)
{
    project_t *p = &proj_slot[0];
    uint32_t ok = 1;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    project_save(0);
    ok &= p->g[PR_G(G_SYNC)] == 0 && p->g[PR_G(G_ROUTE)] == 0;
    song.g[G_SYNC] = 1;
    song.g[G_ROUTE] = 2;
    (void)proj_load(0, 1);
    ok &= song.g[G_SYNC] == 0 && song.g[G_ROUTE] == 0;
    p->g[PR_G(G_ROUTE)] = 2;
    p->g[PR_G(G_SYNC)] = 1;
    p->sum = proj_sum(p);
    (void)proj_load(0, 1);
    ok &= song.g[G_SYNC] == 1 && song.g[G_ROUTE] == 2;
    p->g[PR_G(G_ROUTE)] = 9;
    p->g[PR_G(G_SYNC)] = 3;                                /* BOTH (the Bluetooth build's): USB */
    p->sum = proj_sum(p);
    (void)proj_load(0, 1);
    ok &= song.g[G_SYNC] == 1 && song.g[G_ROUTE] == 2 && !live_globals_edited();
    p->g[PR_G(G_SYNC)] = 2;                                /* BLE: OFF */
    p->sum = proj_sum(p);
    (void)proj_load(0, 1);
    ok &= song.g[G_SYNC] == 0 && !live_globals_edited();
    p->g[PR_G(G_SYNC)] = -1;
    p->sum = proj_sum(p);
    (void)proj_load(0, 1);
    return (int)(ok & (song.g[G_SYNC] == 0));
}

/* a settings record longer than ours (another firmware's, same marker): the defaults, not a half-read record */
static int settings_oversize_refused(void)
{
    struct {
        persist_t p;
        uint8_t more[16];
    } r;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();                                  /* finds the flash */
    memset(&r, 0, sizeof r);
    r.p.magic = PERSIST_MAGIC;
    r.p.palette = 2;                                 /* an old id (CYAN), a record that is rejected */
    r.p.panel = PANEL_DEFAULT;
    st_save(OBJ_SETTINGS, &r, sizeof r);
    memset(&settings, 0, sizeof settings);           /* power off */
    persist_boot();
    settings_init();
    return settings.palette == (THEME_TAG | THEME_DEFAULT);   /* the default: ORANGE */
}

/* spec 2026-10-10 §8: an older record (no Bluetooth bits) loads exactly as before: SPEAKER EQ BASS+ stays, no device,
 * MODE AUTO */
static int settings_old_record(void)
{
    persist_t p;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();                                  /* finds the flash */
    memset(&p, 0, sizeof p);
    p.magic = PERSIST_MAGIC;
    p.palette = 2;
    p.lowcut = 2;                                    /* BASS+ */
    p.zoom = 1u | 1u << 1 | 1u << 3 | 1u << 4;       /* ZOOM, NEXT BAR, USB FIXED, PERFORM PAGE (bits 0..4) */
    p.panel = PANEL_DEFAULT;
    st_save(OBJ_SETTINGS, &p, sizeof p);
    rfill(&settings, sizeof settings);               /* power off: .noinit is anything */
    rfill(&btset, sizeof btset);                     /* (the load replaces whatever was there) */
    rfill(&lastp, sizeof lastp);
    persist_boot();
    settings_init();
    return lastp.set == 0u && settings.lowcut == 2u && fx_lowcut == 2u && settings.zoom == 1u && settings.mutebar == 1u && settings.accel == 1u &&
           settings.usbfix == 1u && settings.perfpage == 1u && settings.palette == (THEME_TAG | 2u) && btset.btmode == 0u &&
           btset.bt_type == 0u && btset.bt_lo == 0u && btset.bt_hi == 0u;
}

/* the remembered device (address, type) and MODE survive a save and a load in zoom's and lowcut's spare bits; lowcut
 * and the zoom flags keep their values */
static int settings_bt_round_trip(void)
{
    uint32_t m, ok = 1;
    for (m = 0; m < 3u; m++) {
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        settings.magic = SETTINGS_MAGIC;
        settings.palette = THEME_TAG | 3u;
        settings.lowcut = m;                         /* FLAT LOWCUT BASS+ */
        settings.zoom = 1;
        settings.mutebar = 0;
        settings.accel = 1;
        settings.usbfix = 1;
        settings.perfpage = 0;
        btset.btmode = m;                            /* AUTO SMC MIDI */
        btset.bt_type = m & 1u;
        btset.bt_lo = 0xA1B2C3u + m;                 /* address bytes 0..2 */
        btset.bt_hi = 0xFFEEDDu - m;                 /* .. 3..5 */
        settings_save();
        st_load(OBJ_SETTINGS, &p, sizeof p);
        ok &= p.zoom >> 8 == 0xA1B2C3u + m && p.lowcut >> 8 == 0xFFEEDDu - m && (p.lowcut & 0xFFu) == m &&
              ((p.zoom >> 5) & 3u) == m && ((p.zoom >> 7) & 1u) == (m & 1u) && (p.zoom & 0x1Fu) == (1u | 1u << 3);
        rfill(&settings, sizeof settings);           /* power off */
        memset(&btset, 0, sizeof btset);             /* (.bss) */
        persist_boot();
        settings_init();
        ok &= btset.btmode == m && btset.bt_type == (m & 1u) && btset.bt_lo == 0xA1B2C3u + m &&
              btset.bt_hi == 0xFFEEDDu - m && settings.lowcut == m && fx_lowcut == m && settings.zoom == 1u &&
              settings.mutebar == 0u && settings.accel == 1u && settings.usbfix == 1u && settings.perfpage == 0u &&
              settings.palette == (THEME_TAG | 3u);
    }
    return (int)ok;
}

/* an update keeps .noinit where it was: `settings` stays Felucca's 32 B (the RAM project slots after it are found
 * again); the Bluetooth fields are outside it, from the flash record only: none -> no device, MODE AUTO, the .noinit
 * settings kept */
static int settings_bt_after_update(void)
{
    memset(hflash, 0xFF, sizeof hflash);             /* no settings record */
    host_init();
    settings.magic = SETTINGS_MAGIC;
    settings.palette = THEME_TAG | 2u;
    settings.lowcut = 1;
    settings.zoom = 1;
    settings.mutebar = 1;
    settings.accel = 1;
    settings.usbfix = 0;
    settings.perfpage = 0;
    rfill(&btset, sizeof btset);                     /* (anything before: a record-less boot clears it) */
    persist_boot();
    settings_init();
    return sizeof settings == 32u && btset.btmode == 0u && btset.bt_type == 0u && btset.bt_lo == 0u &&
           btset.bt_hi == 0u && settings.palette == (THEME_TAG | 2u) && settings.lowcut == 1u && settings.zoom == 1u &&
           settings.mutebar == 1u;
}

/* a record whose MODE bits read 3 (no such MODE): AUTO; the address kept */
static int settings_bt_mode_range(void)
{
    persist_t p;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    memset(&p, 0, sizeof p);
    p.magic = PERSIST_MAGIC;
    p.palette = 2;
    p.zoom = 3u << 5 | 1u << 7 | 0x123456u << 8;
    p.lowcut = 0x654321u << 8;
    p.panel = PANEL_DEFAULT;
    st_save(OBJ_SETTINGS, &p, sizeof p);
    persist_boot();
    settings_init();
    return btset.btmode == 0u && btset.bt_type == 1u && btset.bt_lo == 0x123456u && btset.bt_hi == 0x654321u &&
           settings.lowcut == 0u;
}

/* spec 2026-10-11-fixes §9: METRO in lowcut's bits 5..6 survives a power cycle beside SPEAKER EQ (bits 0..1) and the
 * device's address bytes 3..5 (bits 8..31); a record without them (0.16.0's) and no record at all load OFF */
static int settings_metro_round_trip(void)
{
    uint32_t m, ok = 1;
    for (m = 0; m < 4u; m++) {
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        settings.magic = SETTINGS_MAGIC;
        settings.palette = THEME_TAG | 3u;
        settings.lowcut = m % 3u;
        settings.zoom = 1;
        btset.btmode = 1;
        btset.bt_lo = 0x112233u;
        btset.bt_hi = 0xFFEEDDu;
        fx_metro = (uint8_t)m;
        settings_save();
        st_load(OBJ_SETTINGS, &p, sizeof p);
        ok &= ((p.lowcut >> 5) & 3u) == m && (p.lowcut & 3u) == m % 3u && p.lowcut >> 8 == 0xFFEEDDu &&
              !(p.lowcut & 0x9Cu);
        rfill(&settings, sizeof settings);           /* power off */
        memset(&btset, 0, sizeof btset);             /* (.bss) */
        fx_metro = 0;
        persist_boot();
        settings_init();
        ok &= fx_metro == m && settings.lowcut == m % 3u && fx_lowcut == m % 3u && btset.bt_hi == 0xFFEEDDu &&
              btset.btmode == 1u && settings.palette == (THEME_TAG | 3u) && settings.zoom == 1u;
    }
    {                                                /* 0.16.0's record: BASS+, no METRO bits */
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        memset(&p, 0, sizeof p);
        p.magic = PERSIST_MAGIC;
        p.palette = 2;
        p.lowcut = 2u | 0x654321u << 8;
        p.panel = PANEL_DEFAULT;
        st_save(OBJ_SETTINGS, &p, sizeof p);
        fx_metro = 3;
        persist_boot();
        settings_init();
        ok &= fx_metro == 0u && settings.lowcut == 2u && btset.bt_hi == 0x654321u;
    }
    memset(hflash, 0xFF, sizeof hflash);             /* no record (an update from the stock firmware) */
    host_init();
    fx_metro = 2;
    persist_boot();
    settings_init();
    ok &= fx_metro == 0u;
    return (int)ok;
}

/* spec 2026-10-10 §2 amendment 2026-10-12: AUTOSTART (BLE build) in the palette word's bit 8, round-tripped by both
 * builds as the device is; the palette itself in bits 0..7 (kept, not reset by settings_init); an older record and
 * none load OFF */
static int settings_autostart_round_trip(void)
{
    uint32_t a, ok = 1;
    for (a = 0; a < 2u; a++) {
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        settings.magic = SETTINGS_MAGIC;
        settings.palette = THEME_TAG | 3u;
        settings.lowcut = 2;
        settings.zoom = 1;
        btset.btmode = 2;
        btset.bt_lo = 0x123456u;
        btset.bt_hi = 0xABCDEFu;
        btset.bt_auto = a;
        backlight = 1;
        settings_save();
        st_load(OBJ_SETTINGS, &p, sizeof p);
        ok &= p.palette == (THEME_TAG | 3u | a << 8 | 1u << 9) &&   /* (bit 9: BACKLIGHT's level DIM LO, item 9) */
              (p.lowcut & 0xFFu) == (2u | 1u << 7) && p.lowcut >> 8 == 0xABCDEFu && p.zoom >> 8 == 0x123456u &&
              ((p.zoom >> 5) & 3u) == 2u;
        rfill(&settings, sizeof settings);           /* power off */
        memset(&btset, 0, sizeof btset);             /* (.bss) */
        btset.bt_auto = !a;
        backlight = 0;
        persist_boot();
        settings_init();
        ok &= btset.bt_auto == a && settings.palette == (THEME_TAG | 3u) && settings.lowcut == 2u && settings.zoom == 1u &&
              btset.btmode == 2u && btset.bt_lo == 0x123456u && btset.bt_hi == 0xABCDEFu && backlight == 1u;
    }
    {                                                /* an older record: palette 2, no bit 8 */
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        memset(&p, 0, sizeof p);
        p.magic = PERSIST_MAGIC;
        p.palette = 2;
        p.panel = PANEL_DEFAULT;
        st_save(OBJ_SETTINGS, &p, sizeof p);
        btset.bt_auto = 1;
        persist_boot();
        settings_init();
        ok &= btset.bt_auto == 0u && settings.palette == (THEME_TAG | 2u);
    }
    {                                                /* spec 2026-10-19-md-ui §2: an old palette id (MONO) with AUTOSTART */
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        memset(&p, 0, sizeof p);
        p.magic = PERSIST_MAGIC;
        p.palette = 4u | 1u << 8;
        p.panel = PANEL_DEFAULT;
        st_save(OBJ_SETTINGS, &p, sizeof p);
        persist_boot();
        settings_init();
        ok &= btset.bt_auto == 1u && settings.palette == (THEME_TAG | 1u) && C_BG == THEMES[1].bg;   /* RED ON BLACK */
    }
    memset(hflash, 0xFF, sizeof hflash);             /* no record */
    host_init();
    btset.bt_auto = 1;
    persist_boot();
    settings_init();
    ok &= btset.bt_auto == 0u;
    return (int)ok;
}

/* spec 2026-10-11-fixes §12: BACKLIGHT in lowcut's bit 7 survives a power cycle beside SPEAKER EQ, the last project,
 * METRO and the device's address bytes 3..5; a record without it (0.16.0's, this branch's before it) and no record at
 * all load OFF */
static int settings_backlight_round_trip(void)
{
    uint32_t b, ok = 1;
    for (b = 0; b < 2u; b++) {
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        settings.magic = SETTINGS_MAGIC;
        settings.lowcut = 2;
        fx_metro = 3;
        lastp.set = 1;
        lastp.slot = 2;
        btset.bt_hi = 0xABCDEFu;
        backlight = b;
        settings_save();
        st_load(OBJ_SETTINGS, &p, sizeof p);
        ok &= ((p.lowcut >> 7) & 1u) == b && (p.lowcut & 0x7Fu) == (2u | 1u << 2 | 2u << 3 | 3u << 5) &&
              p.lowcut >> 8 == 0xABCDEFu;
        rfill(&settings, sizeof settings);           /* power off */
        memset(&btset, 0, sizeof btset);             /* (.bss) */
        backlight = !b;
        fx_metro = 0;
        persist_boot();
        settings_init();
        ok &= backlight == b && settings.lowcut == 2u && fx_metro == 3u && lastp.set == 1u && lastp.slot == 2u &&
              btset.bt_hi == 0xABCDEFu;
    }
    {                                                /* an older record: BASS+, METRO MID, no bit 7 */
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        memset(&p, 0, sizeof p);
        p.magic = PERSIST_MAGIC;
        p.lowcut = 2u | 2u << 5 | 0x654321u << 8;
        p.panel = PANEL_DEFAULT;
        st_save(OBJ_SETTINGS, &p, sizeof p);
        backlight = 1;
        persist_boot();
        settings_init();
        ok &= backlight == 0u && settings.lowcut == 2u && fx_metro == 2u;
    }
    memset(hflash, 0xFF, sizeof hflash);             /* no record (an update from the stock firmware) */
    host_init();
    backlight = 1;
    persist_boot();
    settings_init();
    ok &= backlight == 0u;
    for (b = 0; b < BL_COUNT; b++) {                 /* fixes 2026-10-21 item 9: every level survives, in the palette
                                                      * word's bits 9..11, lowcut's bit 7 "not OFF" for an older one */
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        settings.magic = SETTINGS_MAGIC;
        settings.palette = THEME_TAG | 2u;
        btset.bt_auto = 1;
        backlight = b;
        settings_save();
        st_load(OBJ_SETTINGS, &p, sizeof p);
        ok &= ((p.lowcut >> 7) & 1u) == (b != 0u) && ((p.palette >> 9) & 7u) == b && (p.palette & 0x1FFu) ==
              ((THEME_TAG | 2u) & 0xFFu) + 0x100u;
        backlight = 0;
        btset.bt_auto = 0;
        persist_boot();
        settings_init();
        ok &= backlight == b && btset.bt_auto == 1u && theme_index(settings.palette) == 2u;
    }
    {                                                /* an older Hortator's record: ON (bit 7), no level: DIM LO */
        persist_t p;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        persist_boot();
        memset(&p, 0, sizeof p);
        p.magic = PERSIST_MAGIC;
        p.lowcut = 1u << 7;
        p.panel = PANEL_DEFAULT;
        st_save(OBJ_SETTINGS, &p, sizeof p);
        persist_boot();
        settings_init();
        ok &= backlight == BL_DIM_LO;
    }
    return (int)ok;
}

/* spec 2026-10-10 §4: a project saved with CLK BLE (3) or LINK (4) loads as CLK INT in a build whose CLK ends at TRS (2), not as
 * TRS; a value inside the range is kept */
static int clock_beyond_range_is_int(void)
{
    project_t *p = &proj_slot[0];
    uint32_t ok;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    project_save(0);
    p->g[PR_G(G_CLOCK)] = 3;
    p->sum = proj_sum(p);
    song.g[G_CLOCK] = 1;
    (void)proj_load(0, 1);                          /* (the edited slot: as DISCARD EDITS? YES) */
    ok = GP[G_CLOCK].max == 2 && song.g[G_CLOCK] == 0;
    p->g[PR_G(G_CLOCK)] = 4;                              /* (CLK LINK, the Bluetooth build's FM-Link follower) */
    p->sum = proj_sum(p);
    song.g[G_CLOCK] = 1;
    (void)proj_load(0, 1);                          /* (the edited slot: as DISCARD EDITS? YES) */
    ok &= song.g[G_CLOCK] == 0;
    p->g[PR_G(G_CLOCK)] = 2;
    p->sum = proj_sum(p);
    (void)proj_load(0, 1);                          /* (the edited slot: as DISCARD EDITS? YES) */
    ok &= song.g[G_CLOCK] == 2;
    p->g[PR_G(G_CLOCK)] = -1;
    p->sum = proj_sum(p);
    (void)proj_load(0, 1);                          /* (the edited slot: as DISCARD EDITS? YES) */
    return (int)(ok & (song.g[G_CLOCK] == 0));
}

/* (Skiff) HALL (TYPE 2) through a save and a load; ROOM / SPRING keep their ids (0 / 1, fdr7_round_trip) */
static int hall_round_trip(void)
{
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    song.g[G_RTYPE] = 2;
    project_save(1);
    song.g[G_RTYPE] = 0;
    memset(proj_slot, 0, sizeof proj_slot);
    project_load(1);
    return song.g[G_RTYPE] == 2;
}
/* FDR7 keeps TYPE through a save and a load */
static int fdr7_round_trip(void)
{
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    song.g[G_RTYPE] = 1;
    project_save(2);
    song.g[G_RTYPE] = 0;
    memset(proj_slot, 0, sizeof proj_slot);
    project_load(2);
    return song.g[G_RTYPE] == 1;
}
/* the delay's MODE (PING-PONG 1) rides in the record's reserved byte (rsv[0]): kept through a save and a load, the
 * record's size as before */
static int dmode_round_trip(void)
{
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    song.g[G_DMODE] = 1;
    project_save(2);
    if (proj_slot[2].rsv[0] != 1u || sizeof(project_t) != 3044u)
        return 0;
    song.g[G_DMODE] = 0;
    memset(proj_slot, 0, sizeof proj_slot);
    project_load(2);
    return song.g[G_DMODE] == 1;
}
/* a record from before MODE (its reserved byte 0) loads NORMAL, PING-PONG running before; a stray value is cut */
static int dmode_old_normal(void)
{
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    project_save(1);                                 /* (NORMAL: rsv[0] 0, as every earlier firmware wrote it) */
    song.g[G_DMODE] = 1;
    memset(proj_slot, 0, sizeof proj_slot);
    project_load(1);
    if (song.g[G_DMODE] != 0)
        return 0;
    proj_slot[1].rsv[0] = 9;
    proj_slot[1].sum = proj_sum(&proj_slot[1]);
    song.g[G_DMODE] = 0;
    (void)proj_load(1, 0);                           /* (the RAM record as it is: no DISCARD question) */
    return song.g[G_DMODE] == 1;
}
/* FDR8: the motion goes through a save and a load (flash) */
static int fdr8_motion_round_trip(void)
{
    static lk_bank_t want;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    motion_add(0, 4, P_E1, 100);
    motion_add(5, 63, P_PAN, -64);
    motion_add(4, 0, P_LFO2 + LF_DEPTH, 127);
    mo.b.on = 0x21u;
    want = mo.b;
    project_save(2);
    memset(&mo, 0, sizeof mo);
    memset(proj_slot, 0, sizeof proj_slot);
    project_load(2);
    return bank_events_eq(&mo.b, &want);
}
/* a broken motion block: the project loads, the motion dropped */
static int fdr8_bad_motion_dropped(void)
{
    project_t *p = &proj_slot[0];
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    trk[3].step[5].on = 1;
    motion_add(0, 4, P_E1, 100);
    project_save(0);
    (void)p;
    lk_slot[0].key[0] = (uint16_t)lk_key(3, 3, P_MODEL);   /* broken: not a recordable knob */
    memset(&mo, 0, sizeof mo);
    trk[3].step[5].on = 0;
    (void)proj_load(0, 1);                          /* (the edited slot: as DISCARD EDITS? YES) */
    return trk[3].step[5].on && mo.b.count == 0u;
}
/* Review Focus 3: a load while motion plays: the stop after it puts nothing of the old project back */
static int fdr8_load_while_playing(void)
{
    uint32_t b;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    trk[0].p[P_E1] = 11;
    project_save(1);                                 /* the project: E1 11 */
    trk[0].p[P_E1] = 64;
    motion_add(0, 0, P_E1, 100);
    transport_req = 1;
    for (b = 0; b < 8u; b++)
        render_mix(L, R, CTL);                       /* motion plays: E1 100, the base 64 */
    (void)proj_load(1, 1);                          /* (the edited slot: as DISCARD EDITS? YES) */
    for (b = 0; b < 8u; b++)
        render_mix(L, R, CTL);                       /* the stop the load asked for */
    return !song.playing && trk[0].p[P_E1] == 11;
}
/* the name goes through a save and a load (flash; FDRA); the removed song's bytes stored as 0 */
static int fdr9_round_trip(void)
{
    uint32_t i, z = 0;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    name_set(pname.name, "BREAK 2");
    project_save(2);
    for (i = 0; i < sizeof proj_slot[2].rsv_song; i++)
        z |= proj_slot[2].rsv_song[i];
    memset(&pname, 0, sizeof pname);
    memset(proj_slot, 0, sizeof proj_slot);
    persist_boot();
    project_load(2);
    return !z && str_eq(pname.name, "BREAK 2") && pname.from == 3u && sizeof(project_t) == 3044u;   /* (FRT6) */
}
/* FDRA: the five filter knobs through a save and a load (flash) */
static int fdra_round_trip(void)
{
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    trk[5].p[P_FTYPE] = FT_NOTCH;
    trk[5].p[P_FCUT] = 33;
    trk[5].p[P_FRESO] = 101;
    trk[5].p[P_FENV] = -17;
    trk[5].p[P_FDEC] = 90;
    project_save(3);
    host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    persist_boot();
    project_load(3);
    return trk[5].p[P_FTYPE] == FT_NOTCH && trk[5].p[P_FCUT] == 33 && trk[5].p[P_FRESO] == 101 &&
           trk[5].p[P_FENV] == -17 && trk[5].p[P_FDEC] == 90;
}
/* Review Focus 4: out-of-range filter values in a stored project: clamped on load */
static int fdra_clamps(void)
{
    project_t *p = &proj_slot[0];
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    project_save(0);
    p->t[4].p[PR_P(P_FTYPE)] = 9;
    p->t[4].p[PR_P(P_FCUT)] = 300;
    p->t[4].p[PR_P(P_FENV)] = -200;
    p->sum = proj_sum(p);
    (void)proj_load(0, 1);                          /* (the edited slot: as DISCARD EDITS? YES) */
    return trk[4].p[P_FTYPE] == FT_N - 1 && trk[4].p[P_FCUT] == 127 && trk[4].p[P_FENV] == -64;
}
/* FDRB: SLIDE, TIE and the note through a save and a load (flash) */
static int fdrb_round_trip(void)
{
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    trk[2].step[4].on = 1;
    trk[2].step[4].slide = 1;
    trk[2].step[4].nt = -12;
    trk[2].step[5].tie = 1;
    project_save(0);
    host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    persist_boot();
    project_load(0);
    return trk[2].step[4].on && trk[2].step[4].slide && trk[2].step[4].nt == -12 && trk[2].step[5].tie &&
           !trk[2].step[5].on;
}
/* Review Focus 5: a stored step's reserved bits and an out-of-range note and MICRO (its byte, FRT6): cleared and
 * clamped on load */
static int fdrb_clamps(void)
{
    project_t *p = &proj_slot[0];
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    project_save(0);
    ((uint8_t *)&p->t[4].step[2])[0] = 0xFFu;        /* every flag, the reserved bits too */
    ((uint8_t *)&p->t[4].step[2])[3] = 0x5Au;        /* MICRO +90: +50 */
    p->t[4].step[3].nt = 100;                        /* past NT_MAX (C4 + 67 = G9) */
    p->t[4].step[5].nt = -100;
    p->sum = proj_sum(p);
    (void)proj_load(0, 1);                          /* (the edited slot: as DISCARD EDITS? YES) */
    return trk[4].step[2].on && trk[4].step[2].acc && trk[4].step[2].slide && trk[4].step[2].tie &&
           trk[4].step[2].rat == 3u && ((const uint8_t *)&trk[4].step[2])[0] == 0x3Fu &&
           trk[4].step[2].micro == MICRO_MAX && trk[4].step[3].nt == NT_MAX && trk[4].step[5].nt == NT_MIN;
}
/* a broken name: the project loads, the name is dropped */
static int fdr9_bad_name_dropped(void)
{
    project_t *p = &proj_slot[0];
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    trk[3].step[5].on = 1;
    name_set(pname.name, "OK");
    project_save(0);
    p->name[0] = 'a';                                /* outside the NAME set */
    p->sum = proj_sum(p);
    memset(&pname, 0, sizeof pname);
    trk[3].step[5].on = 0;
    (void)proj_load(0, 1);                          /* (the edited slot: as DISCARD EDITS? YES) */
    return trk[3].step[5].on && !pname.name[0];
}
/* a rename writes only the name (flash), and renames the current project when it is that slot */
static int rename_slot(void)
{
    char b[NAME_LEN + 1u];
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    trk[1].step[2].on = 1;
    name_set(pname.name, "A NAME");
    project_save(3);
    project_rename(3, "INTRO");
    memset(proj_slot, 0, sizeof proj_slot);
    persist_boot();
    return project_name(3, b) && str_eq(b, "INTRO") && proj_slot[3].t[1].step[2].on && str_eq(pname.name, "INTRO") &&
           !project_name(2, b) && !b[0];
}
/* ---- FDRC (spec 2026-10-11 §10): PRIO per track; the older formats load with PRIO HIGH */

static void all_prio(int16_t p)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        trk[k].p[P_PRIO] = p;
}
static int all_prio_is(int16_t p)
{
    uint32_t k, ok = 1;
    for (k = 0; k < NTRK; k++)
        ok &= trk[k].p[P_PRIO] == p;
    return (int)ok;
}

/* PRIO through a save and a load (flash; FDRC's, now FDRD's) */
static int fdrc_round_trip(void)
{
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    trk[1].p[P_PRIO] = 1;
    trk[5].p[P_PRIO] = 1;
    project_save(0);
    host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    persist_boot();
    project_load(0);
    return trk[1].p[P_PRIO] == 1 && trk[5].p[P_PRIO] == 1 && trk[0].p[P_PRIO] == 0 && sizeof(project_t) == 3044u &&
           proj_slot[0].magic == PROJ_MAGIC;        /* "FRT6" */
}

/* one pattern played 2 s from a loaded slot: a hash of the mix */
static uint32_t play_hash(uint32_t slot)
{
    uint32_t b, i, h = 2166136261u;
    (void)proj_load(slot, 1);                        /* (whatever the live state: as DISCARD EDITS? YES) */
    transport_req = 1;
    for (b = 0; b < 2u * FS / CTL; b++) {
        render_mix(L, R, CTL);
        for (i = 0; i < CTL; i++)
            h = (h ^ (uint32_t)L[i] ^ ((uint32_t)R[i] << 7)) * 16777619u;
    }
    transport_req = 2;
    render_mix(L, R, CTL);
    return h;
}
static void pattern_kit(void)                        /* the power-on kit, a groove of on / accent steps */
{
    uint32_t s;
    host_init();
    memset(&mo, 0, sizeof mo);
    memset(&pname, 0, sizeof pname);
    for (s = 0; s < 16u; s++) {
        trk[0].step[s].on = s % 4u == 0u;
        trk[1].step[s].on = s % 8u == 4u;
        trk[3].step[s].on = s % 2u == 0u;
        trk[3].step[s].acc = s % 4u == 2u;
        trk[5].step[s].on = s == 14u;
    }
}


/* FDRE round trip: slots 3 and 4 (sources, DEST, DEPTH) and changed sources of 1 and 2 */
static int fdre_slots_round_trip(void)
{
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    trk[5].p[P_LSRC1] = 1;
    trk[5].p[P_LSRC2] = 0;
    trk[5].p[P_LS3SRC] = 1;
    trk[5].p[P_LS3DEST] = 11;                        /* F.CUT */
    trk[5].p[P_LS3DEPTH] = -64;
    trk[5].p[P_LS4SRC] = 0;
    trk[5].p[P_LS4DEST] = 2;
    trk[5].p[P_LS4DEPTH] = 33;
    trk[3].p[P_LS3DEST] = LD_MAX;                    /* the INSERT's C: the last DEST */
    trk[3].p[P_LFO2 + LF_DEST] = 13;                 /* SRR */
    project_save(3);
    host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    persist_boot();
    project_load(3);
    return trk[5].p[P_LSRC1] == 1 && trk[5].p[P_LSRC2] == 0 && trk[5].p[P_LS3SRC] == 1 && trk[5].p[P_LS3DEST] == 11 &&
           trk[5].p[P_LS3DEPTH] == -64 && trk[5].p[P_LS4SRC] == 0 && trk[5].p[P_LS4DEST] == 2 &&
           trk[5].p[P_LS4DEPTH] == 33 && trk[3].p[P_LS3DEST] == LD_MAX && trk[3].p[P_LFO2 + LF_DEST] == 13 &&
           sizeof(project_t) == 3044u && sizeof(project_t) <= ST_PAYLOAD_MAX &&
           proj_slot[3].magic == PROJ_MAGIC;        /* "FRT6" */
}

/* SRR and the INSERT (P_SRR, P_ITYPE .. P_IMIX): past FDRE's block, so in the steps' spare bytes after KEYF's (one a
 * step: step 1 SRR, 2 TYPE, 3..5 A B C, 6 MIX), each stored as its offset from its power-on value (a byte 0 = the
 * power-on value: SRR 0, TYPE OFF, MIX 100 %, A B C 64 96 96). Kept through a save and a load (any value 0..127),
 * the record 3044 B, the live steps' spare bytes 0; a track at its power-on values stores 0 there */
static int fdre_inserts_round_trip(void)
{
    static const uint32_t ID[6] = {P_SRR, P_ITYPE, P_IA, P_IB, P_IC, P_IMIX};
    uint32_t k, i, j, ok = 1;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    pattern_kit();
    for (j = 0; j < 6u; j++) {
        trk[1].p[ID[j]] = (int16_t)(j == 1u ? IT_CRUSH : 127 - 3 * (int32_t)j);
        trk[4].p[ID[j]] = (int16_t)(j == 1u ? IT_CHORUS : (int32_t)j);   /* (0: below every A B C MIX default) */
    }
    trk[3].p[P_IMIX] = 0;
    project_save(2);
    for (k = 0; k < NTRK; k++)                         /* the tracks left at their power-on values: 0 stored */
        if (k != 1u && k != 4u && k != 3u)
            for (j = 0; j < 6u; j++)
                ok &= proj_slot[2].xp[k][PR_XP(ID[j])] == 0;
    host_init();
    memset(proj_slot, 0, sizeof proj_slot);
    persist_boot();
    project_load(2);
    for (k = 0; k < NTRK; k++) {
        for (j = 0; j < 6u; j++) {
            int32_t want = k == 1u ? (j == 1u ? IT_CRUSH : 127 - 3 * (int32_t)j)
                         : k == 4u ? (j == 1u ? IT_CHORUS : (int32_t)j)
                         : k == 3u && j == 5u ? 0 : TP[ID[j]].def;
            if (trk[k].p[ID[j]] != want) {
                printf("     T%u %s: %d, want %d\n", k + 1u, TP[ID[j]].label, trk[k].p[ID[j]], want);
                ok = 0;
            }
        }
        for (i = 0; i < NSTEP; i++)
            ok &= trk[k].step[i].rsv == 0;
    }
    return (int)ok && sizeof(project_t) == 3044u && proj_slot[2].magic == PROJ_MAGIC && proj_ok(&proj_slot[2]);
}
/* a record from before SRR and the INSERT (its steps' spare bytes 0, as 0.26.0 and every earlier firmware wrote
 * them) loads with SRR 0 and the INSERT OFF on every track (A B C MIX at their power-on values), and plays as it did */
static int fdre_old_inserts_off(void)
{
    project_t *p = &proj_slot[1];
    uint32_t k, i, ok = 1, ref, h;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    persist_boot();
    pattern_kit();
    trk[3].p[P_DIST] = 70;                            /* (a tail the INSERT's would lengthen) */
    project_save(1);
    for (k = 0; k < NTRK; k++)                         /* (what 0.26.0 wrote: no byte of it set) */
        for (i = 0; i < NSTEP; i++)
            ok &= p->t[k].step[i].rsv == 0;
    host_init();
    ref = play_hash(1);
    host_init();
    for (k = 0; k < NTRK; k++) {                       /* the live values: replaced by the load */
        trk[k].p[P_SRR] = 99;
        trk[k].p[P_ITYPE] = IT_FUZZ;
        trk[k].p[P_IA] = trk[k].p[P_IB] = trk[k].p[P_IC] = trk[k].p[P_IMIX] = 5;
    }
    h = play_hash(1);
    for (k = 0; k < NTRK; k++)
        ok &= trk[k].p[P_SRR] == 0 && trk[k].p[P_ITYPE] == IT_OFF && trk[k].p[P_IA] == 64 && trk[k].p[P_IB] == 96 &&
              trk[k].p[P_IC] == 96 && trk[k].p[P_IMIX] == 127;
    return (int)ok && h == ref;
}

/* ---- the last project at power-on (spec 2026-10-11 §6, §9: lowcut's bits 2..4) */

static uint32_t host_jedec_keep;
/* main.c's boot after a power cycle: .noinit anything, .bss zero (song, the tracks, ui), the flash kept; SEQ held:
 * safe start. Returns the IRQ enables drum_boot_init did (main.c turns them on after audio_init: none) */
static uint32_t power_on(int seq)
{
    uint32_t irq;
    rfill(&settings, sizeof settings);
    rfill(proj_slot, sizeof proj_slot);
    memset(&song, 0, sizeof song);
    memset(trk, 0, sizeof trk);
    memset(&ui, 0, sizeof ui);
    memset(&mo, 0, sizeof mo);
    memset(&pname, 0, sizeof pname);
    rfill(&lastp, sizeof lastp);                     /* (persist_boot sets it: from the flash record or 0) */
    transport_req = 0;
    persist_boot();
    settings_init();
    panel_init();
    fm1_in.buttons = seq ? 1u << panel.btn[B_SEQ] : 0u;
    irq = host_irq_on;
    drum_boot_init();
    fm1_in.buttons = 0;
    return host_irq_on - irq;
}
static uint32_t lc_bits(void)                         /* the stored record's lowcut bits 0..7 */
{
    persist_t p;
    memset(&p, 0, sizeof p);
    st_load(OBJ_SETTINGS, &p, sizeof p);
    return p.lowcut & 0xFFu;
}
static int power_on_kit(void)                        /* the tracks and globals as drum_tracks_init sets them */
{
    static track_t want[NTRK];
    static int16_t g[G_COUNT];
    uint32_t k, ok = 1;
    track_t keep[NTRK];
    int16_t gk[G_COUNT];
    memcpy(keep, trk, sizeof trk);
    memcpy(gk, song.g, sizeof gk);
    drum_tracks_init();
    memcpy(want, trk, sizeof trk);
    memcpy(g, song.g, sizeof g);
    memcpy(trk, keep, sizeof trk);
    memcpy(song.g, gk, sizeof gk);
    for (k = 0; k < NTRK; k++)
        ok &= !memcmp(trk[k].p, want[k].p, sizeof want[k].p) && !memcmp(trk[k].step, want[k].step, sizeof want[k].step);
    return (int)ok && !memcmp(song.g, g, sizeof g);
}
static void fresh_flash(void)                        /* a device with nothing saved: SPEAKER EQ BASS+ */
{
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    memset(&ui, 0, sizeof ui);
    persist_boot();
    settings.magic = 0;
    settings_init();
    settings.lowcut = 2;
    fx_lowcut = 2;
}

/* a flash SAVE remembers its slot (settings saved); the next power-on loads it, stopped, LOADED, without enabling
 * the IRQs; SPEAKER EQ keeps its value beside the bits */
static int last_project_loads(void)
{
    uint32_t ok, irq;
    fresh_flash();
    trk[3].p[P_E1] = 11;
    trk[3].step[6].on = 1;
    trk[2].p[P_PRIO] = 1;
    song.g[G_BPM] = 97;
    name_set(pname.name, "LAST ONE");
    project_save(1);
    project_save(3);                                 /* the last one wins */
    ok = str_eq(ui.msg, "SAVED") && lastp.set == 1u && lastp.slot == 3u && lc_bits() == (2u | 1u << 2 | 3u << 3);
    irq = power_on(0);
    ok &= trk[3].p[P_E1] == 11 && trk[3].step[6].on && trk[2].p[P_PRIO] == 1 && song.g[G_BPM] == 97 &&
          str_eq(pname.name, "LAST ONE") && pname.from == 4u && song.g[G_SLOT] == 4 && !song.playing &&
          str_eq(ui.msg, "LOADED") && !ui.confirm && irq == 0u && settings.lowcut == 2u && fx_lowcut == 2u &&
          lastp.set == 1u && lastp.slot == 3u;
    transport_req = 1;                               /* it plays after the boot */
    render_mix(0, 0, CTL * 64);
    return (int)ok && song.playing;
}

/* the remembered slot empty: confirm 5 over the power-on kit; NO keeps the slot (asked again at the next power-on),
 * YES forgets it (saved: the next power-on is the power-on kit, no question) */
static int last_project_empty(void)
{
    uint32_t ok, i, lit = 0;
    fresh_flash();
    project_save(1);                                 /* remembered: B */
    memset(hflash + 0x97000u + 1u * 2u * 4096u, 0xFF, 2u * 4096u);   /* slot B's flash erased */
    ok = power_on(0) == 0u && ui.confirm == 5u && ui.confirm_trk == 1u && power_on_kit() && !song.playing &&
         !str_eq(ui.msg, "EMPTY SLOT") && lastp.set == 1u;
    ui_frame();
    for (i = 70u * 240u; i < 150u * 240u; i++)
        lit += fb[i] != 0;
    ok &= lit > 300u;
    shot("build/ui_shots/fixes/load_failed.ppm");
    press(B_OCTDN);                                  /* NO */
    ui_frame();
    release_all();
    ok &= !ui.confirm && lastp.set == 1u && lastp.slot == 1u && lc_bits() == (2u | 1u << 2 | 1u << 3) && power_on_kit();
    ok &= power_on(0) == 0u && ui.confirm == 5u && ui.confirm_trk == 1u;   /* asked again */
    for (i = 0; i < 5u; i++)                         /* nothing else reacts meanwhile */
        ui_frame();
    ok &= ui.confirm == 5u;
    press(B_OCTUP);                                  /* YES */
    ui_frame();
    release_all();
    ok &= !ui.confirm && lastp.set == 0u && lc_bits() == 2u && power_on_kit() && settings.lowcut == 2u;
    ok &= power_on(0) == 0u && !ui.confirm && power_on_kit() && !str_eq(ui.msg, "LOADED") && settings.lowcut == 2u;
    return (int)ok;
}

/* the remembered slot unreadable (a record whose sum fails): the same question */
static int last_project_corrupt(void)
{
    project_t c;
    fresh_flash();
    trk[4].p[P_E1] = 5;
    project_save(2);
    c = proj_slot[2];
    c.sum ^= 0x10u;
    st_save(OBJ_PROJECT0 + 2u, &c, sizeof c);
    return power_on(0) == 0u && ui.confirm == 5u && ui.confirm_trk == 2u && power_on_kit();
}

/* SEQ held at power-on (safe start): no load, no question; the slot stays remembered */
static int last_project_safe_start(void)
{
    fresh_flash();
    trk[3].p[P_E1] = 11;
    project_save(0);
    power_on(1);
    return safe_start && !ui.confirm && power_on_kit() && !str_eq(ui.msg, "LOADED") && lastp.set == 1u &&
           lastp.slot == 0u;
}

/* an older settings record (no last-project bits): the power-on kit as before, SPEAKER EQ kept; a project saved
 * in RAM only (no flash) remembers nothing */
static int last_project_old_record_no_flash(void)
{
    persist_t p;
    uint32_t ok;
    fresh_flash();
    trk[3].p[P_E1] = 11;
    project_save(0);                                 /* a slot, but the record does not name it */
    memset(&p, 0, sizeof p);
    p.magic = PERSIST_MAGIC;
    p.palette = 2;
    p.lowcut = 1;                                    /* LOWCUT */
    p.panel = PANEL_DEFAULT;
    st_save(OBJ_SETTINGS, &p, sizeof p);
    ok = power_on(0) == 0u && !ui.confirm && power_on_kit() && settings.lowcut == 1u && lastp.set == 0u;
    host_jedec = 0;                                  /* no flash */
    fresh_flash();
    trk[3].p[P_E1] = 11;
    project_save(0);
    ok &= !flash_ok && str_eq(ui.msg, "SAVED (RAM)") && lastp.set == 0u;
    ok &= power_on(0) == 0u && !ui.confirm && !flash_ok && lastp.set == 0u;
    host_jedec = 0xB36014u;                          /* the UC25HQ80 of newer FM-1s: the same 1 MiB SPI NOR, accepted */
    fresh_flash();
    ok &= power_on(0) == 0u && flash_ok;
    project_save(0);
    ok &= flash_ok && str_eq(ui.msg, "SAVED");
    host_jedec = 0x123456u;                          /* any other chip: RAM only, as before */
    fresh_flash();
    ok &= power_on(0) == 0u && !flash_ok;
    host_jedec = host_jedec_keep;
    return (int)ok;
}

/* Hortator's 8-track records (FDRG, 3840 B; a "VAR1" variation bank) in flash: no record (empty slots); the
 * remembered slot asks as an empty one over the power-on kit, nothing loaded; a SAVE there then loads */
static int old_8track_record_boots_empty(void)
{
    uint32_t ok;
    fresh_flash();
    project_save(0);                                 /* remembered: A */
    rs = 99;
    old_record(OBJ_PROJECT0 + 0u, 0x47524446u, 3840u);   /* "FDRG" over A, and in C: valid sums, 8 tracks */
    old_record(OBJ_PROJECT0 + 2u, 0x47524446u, 3840u);
    old_record(OBJ_VAR0 + 0u, 0x31524156u, 600u);   /* "VAR1": an old variation bank */
    ok = power_on(0) == 0u && ui.confirm == 5u && ui.confirm_trk == 0u && power_on_kit() && !song.playing;
    ok &= !project_used(0) && !project_used(2) && proj_loaded() < 0 && !str_eq(ui.msg, "LOADED");
    press(B_OCTUP);                                  /* YES: forgotten */
    ui_frame();
    release_all();
    ok &= !ui.confirm && lastp.set == 0u && power_on_kit();
    trk[3].p[P_E1] = 11;
    project_save(0);
    ok &= str_eq(ui.msg, "SAVED") && proj_ok(&proj_slot[0]);
    ok &= power_on(0) == 0u && str_eq(ui.msg, "LOADED") && trk[3].p[P_E1] == 11 && proj_loaded() == 0 &&
          proj_ok(&proj_slot[0]) && !project_used(2);
    return (int)ok;
}

/* lowcut's bits: SPEAKER EQ (0..2) and every slot round trip, the Bluetooth bytes (8..31) kept */
static int last_project_bits(void)
{
    uint32_t m, k, ok = 1;
    for (m = 0; m < 3u; m++)
        for (k = 0; k < 4u; k++) {
            persist_t p;
            fresh_flash();
            settings.lowcut = m;
            btset.bt_hi = 0x123456u;
            lastp.set = 1;
            lastp.slot = (uint8_t)k;
            settings_save();
            st_load(OBJ_SETTINGS, &p, sizeof p);
            ok &= p.lowcut == (m | 1u << 2 | k << 3 | 0x123456u << 8);
            rfill(&settings, sizeof settings);
            memset(&lastp, 0, sizeof lastp);
            persist_boot();
            settings_init();
            ok &= settings.lowcut == m && fx_lowcut == m && lastp.set == 1u && lastp.slot == k && btset.bt_hi == 0x123456u;
        }
    return (int)ok;
}
/* MEMB at every knob extreme (TUNE, DECAY, TONE, HEAD, POS, BEND, STICK) and both velocities: bounded, in memory */
static int memb_extremes(void)
{
    static const int16_t LO[8] = {-24, 0, 0, 0, 0, 0, 0, 0}, HI[8] = {24, 127, 127, 127, 127, 127, 127, 0};
    uint32_t c, k, i;
    int ok = 1;
    for (c = 0; c < 256u; c++) {
        host_init();
        drum_set_model(&trk[0], DM_MEMB);
        for (k = 0; k < 7u; k++)
            trk[0].p[P_E0 + k] = ((c >> k) & 1u) ? HI[k] : LO[k];
        drum_hit(&trk[0], (c & 128u) ? 127u : 96u);
        for (i = 0; i < 40u; i++) {
            render_mix(L, R, CTL);
            ok &= L[0] <= 32767 && L[0] >= -32768;
        }
    }
    return ok;
}
/* ---- USR3 at 48 KiB (spec 2026-10-24 parameter locks: the lock banks take its last 32 KiB) */
#define U3 0xC8000u
static uint32_t rd32(uint32_t off) { uint32_t v; memcpy(&v, hflash + off, 4); return v; }
static void usr_image(uint32_t at, uint32_t data_len) /* an upstream-style USR2 / USR3 at at: 3 zones A (inside 48 KiB),
                                                       * B (across 0xBE00, its loop start past the cut), C (past it);
                                                       * random data */
{
    smp_user_hdr_t h;
    smp_zone_t *z;
    uint32_t i;
    memset(&h, 0, sizeof h);
    h.magic = SMP_USER_MAGIC;
    h.version = 1;
    h.nz = 3;
    memcpy(h.name, "BIGLOOP", 7);
    h.data_len = data_len;
    for (i = 0; i < 3u; i++) {
        z = &h.zone[i];
        z->off = i == 0 ? 0x100u : i == 1 ? 0xB000u : 0xC000u;
        z->n = i == 0 ? 0x2000u : i == 1 ? 0x4000u : 0x1000u;
        z->ls = i == 1 ? 0x3000u : 0u;
        z->le = z->n - 1u;
        z->looped = 1;
        z->rate = 1u << 16;
        z->lo = (uint8_t)(i * 40u);
        z->hi = (uint8_t)(i * 40u + 39u);
    }
    if (data_len <= 0xBE00u) {                        /* a 48 KiB one: B ends inside, no C */
        h.nz = 2;
        h.zone[1].n = 0x1000u;
        h.zone[1].ls = 0x800u;
        h.zone[1].le = 0xFFFu;
    }
    rfill(hflash + at + 512u, 0x14000u - 512u);
    memcpy(hflash + at, &h, sizeof h);
}
static void usr3_image(uint32_t data_len) { usr_image(U3, data_len); }
static void usr3_boot(void)
{
    host_init();
    memset(&ui, 0, sizeof ui);
    host_nflog = 0;
    persist_note = 0;
    persist_boot();
}
static int flog_only_usr3_header(void)              /* every write so far: one erase of 0xC8000 and programs inside it */
{
    uint32_t i, erases = 0, ok = host_nflog <= HOST_NFLOG;
    for (i = 0; ok && i < host_nflog; i++) {
        ok &= host_flog[i].off >= U3 && host_flog[i].off + host_flog[i].n <= U3 + 0x1000u;
        erases += host_flog[i].erase;
        ok &= !host_flog[i].erase || host_flog[i].off == U3;
    }
    return ok && erases == 1u;
}
static int usr3_clamped_once(void)
{
    static uint8_t orig[0x1000];
    smp_user_hdr_t h;
    uint32_t ok;
    memset(hflash, 0xFF, sizeof hflash);
    rs = 77;
    usr3_image(0x13E00u);
    memcpy(orig, hflash + U3, sizeof orig);
    usr3_boot();
    memcpy(&h, hflash + U3, sizeof h);
    ok = h.magic == SMP_USER_MAGIC && h.version == 1 && h.data_len == 0xBE00u && h.nz == 2;
    ok &= h.zone[0].off == 0x100u && h.zone[0].n == 0x2000u && h.zone[0].looped == 1;   /* A as it was */
    ok &= h.zone[1].n == (0xBE00u - 0xB000u) * 2u && h.zone[1].le < h.zone[1].n && h.zone[1].ls <= h.zone[1].le &&
          h.zone[1].looped == 0;                                                        /* B cut, its loop off */
    ok &= h.crc == st_crc32(hflash + U3 + 512u, 0xBE00u);
    ok &= !memcmp(hflash + U3 + 512u, orig + 512u, 0x1000u - 512u);
    ok &= !memcmp(h.name, "BIGLOOP", 7);
    ok &= usr_nz[2] == 2 && (persist_note & PN_USR3) && flog_only_usr3_header();
    usr3_boot();                                       /* the next boot: nothing to do */
    ok &= host_nflog == 0 && persist_note == 0 && usr_nz[2] == 2 && rd32(U3 + 16u) == 0xBE00u;
    return (int)ok;
}
static int usr3_short_untouched(void)
{
    static uint8_t orig[0x14000];
    memset(hflash, 0xFF, sizeof hflash);
    rs = 78;
    usr3_image(0xBE00u);
    memcpy(orig, hflash + U3, sizeof orig);
    usr3_boot();
    return host_nflog == 0 && persist_note == 0 && usr_nz[2] == 2 && !memcmp(orig, hflash + U3, sizeof orig);
}
static int usr3_clamp_fails_safe(void)
{
    static uint8_t usr12[0x28000];
    uint32_t ok, i;
    memset(hflash, 0xFF, sizeof hflash);
    rs = 79;
    slot_header(0, 1);
    slot_header(1, 1);
    usr3_image(0x13E00u);
    memcpy(usr12, hflash + 0xA0000u, sizeof usr12);
    host_fail_at = 0;                                  /* the clamp's erase fails */
    usr3_boot();
    ok = usr_nz[2] == 0 && usr_nz[0] && usr_nz[1] && !memcmp(usr12, hflash + 0xA0000u, sizeof usr12);
    ok &= rd32(U3 + 16u) == 0x13E00u && !(persist_note & PN_USR3);
    for (i = 0; i < host_nflog && i < HOST_NFLOG; i++)
        ok &= host_flog[i].off >= U3 && host_flog[i].off + host_flog[i].n <= U3 + 0x1000u;
    host_fail_at = -1;
    return (int)ok;
}
static int usr3_note_shown(void)
{
    uint32_t i;
    memset(hflash, 0xFF, sizeof hflash);
    rs = 80;
    usr3_image(0x13E00u);
    persist_note = 0;
    power_on(0);
    for (i = 0; i < 3u; i++)
        ui_frame();
    return !strcmp(ui.msg, "USR3 SHORTENED TO 48K") && ui.msg_t && persist_note == 0;
}

/* ---- USR2 at 48 KiB (spec 2026-10-26 cue / variations: the variations take its last 32 KiB, 0xC0000..0xC7FFF) */
#define U2 0xB4000u
static int flog_only_header(uint32_t at, uint32_t from, uint32_t erases_want)   /* writes from..: inside at's sector */
{
    uint32_t i, erases = 0, ok = host_nflog <= HOST_NFLOG;
    for (i = from; ok && i < host_nflog; i++) {
        ok &= host_flog[i].off >= at && host_flog[i].off + host_flog[i].n <= at + 0x1000u;
        erases += host_flog[i].erase;
        ok &= !host_flog[i].erase || host_flog[i].off == at;
    }
    return ok && erases == erases_want;
}
static int usr2_clamped_once(void)
{
    static uint8_t orig[0x1000];
    smp_user_hdr_t h;
    uint32_t ok;
    memset(hflash, 0xFF, sizeof hflash);
    rs = 177;
    usr_image(U2, 0x13E00u);
    memcpy(orig, hflash + U2, sizeof orig);
    usr3_boot();
    memcpy(&h, hflash + U2, sizeof h);
    ok = h.magic == SMP_USER_MAGIC && h.version == 1 && h.data_len == 0xBE00u && h.nz == 2;
    ok &= h.zone[0].off == 0x100u && h.zone[0].n == 0x2000u && h.zone[0].looped == 1;   /* A as it was */
    ok &= h.zone[1].n == (0xBE00u - 0xB000u) * 2u && h.zone[1].le < h.zone[1].n && h.zone[1].ls <= h.zone[1].le &&
          h.zone[1].looped == 0;                                                        /* B cut, its loop off */
    ok &= h.crc == st_crc32(hflash + U2 + 512u, 0xBE00u);
    ok &= !memcmp(hflash + U2 + 512u, orig + 512u, 0x1000u - 512u);
    ok &= !memcmp(h.name, "BIGLOOP", 7);
    ok &= usr_nz[1] == 2 && (persist_note & PN_USR2) && !(persist_note & PN_USR3) && flog_only_header(U2, 0, 1u) &&
          !var_blocked;
    usr3_boot();                                       /* the next boot: nothing to do */
    ok &= host_nflog == 0 && persist_note == 0 && usr_nz[1] == 2 && rd32(U2 + 16u) == 0xBE00u;
    return (int)ok;
}
static int usr2_short_untouched(void)
{
    static uint8_t orig[0x14000];
    memset(hflash, 0xFF, sizeof hflash);
    rs = 178;
    usr_image(U2, 0xBE00u);
    memcpy(orig, hflash + U2, sizeof orig);
    usr3_boot();
    return host_nflog == 0 && persist_note == 0 && usr_nz[1] == 2 && !memcmp(orig, hflash + U2, sizeof orig);
}
static int usr2_clamp_fails_safe(void)
{
    static uint8_t usr13[0x14000];
    uint32_t ok, i;
    memset(hflash, 0xFF, sizeof hflash);
    rs = 179;
    slot_header(0, 1);
    usr_image(U2, 0x13E00u);
    memcpy(usr13, hflash + 0xA0000u, sizeof usr13);
    host_fail_at = 0;                                  /* the clamp's erase fails */
    usr3_boot();
    ok = usr_nz[1] == 0 && usr_nz[0] && !memcmp(usr13, hflash + 0xA0000u, sizeof usr13);
    ok &= rd32(U2 + 16u) == 0x13E00u && !(persist_note & PN_USR2) && (persist_note & PN_USR2F) && var_blocked;
    for (i = 0; i < host_nflog && i < HOST_NFLOG; i++)
        ok &= host_flog[i].off >= U2 && host_flog[i].off + host_flog[i].n <= U2 + 0x1000u;
    host_fail_at = -1;
    return (int)ok;
}
static int usr2_usr3_both(void)
{
    uint32_t ok, f, n2 = 0, n3 = 0, order = 1;
    char last[32] = "";
    memset(hflash, 0xFF, sizeof hflash);
    rs = 180;
    usr_image(U2, 0x13E00u);
    usr_image(U3, 0x13E00u);
    persist_note = 0;
    host_nflog = 0;
    power_on(0);
    ok = rd32(U2 + 16u) == 0xBE00u && rd32(U3 + 16u) == 0xBE00u && usr_nz[1] == 2 && usr_nz[2] == 2;
    ok &= host_nflog >= 2u && host_flog[0].erase && host_flog[0].off == U2;   /* USR2 first */
    for (f = 0; f < 200u; f++) {
        ui_frame();
        if (ui.msg_t && !str_eq(ui.msg, last)) {
            snprintf(last, sizeof last, "%s", ui.msg);
            if (str_eq(ui.msg, "USR2 SHORTENED TO 48K"))
                n2++;
            if (str_eq(ui.msg, "USR3 SHORTENED TO 48K")) {
                n3++;
                order &= n2 == 1u;
            }
        }
    }
    ok &= n2 == 1u && n3 == 1u && order && persist_note == 0;
    usr3_boot();                                       /* a second boot writes nothing */
    ok &= host_nflog == 0 && persist_note == 0;
    if (!ok)
        printf("     usr2+usr3: len %x %x nz %u %u notes %u %u order %u nflog %u\n", rd32(U2 + 16u), rd32(U3 + 16u),
               usr_nz[1], usr_nz[2], n2, n3, order, host_nflog);
    return (int)ok;
}

/* ---- FDRF: the motion and the locks in per-slot lock banks (spec 2026-10-24 parameter locks) */
static void wipe_ram(void)                            /* a power cycle's RAM: .noinit slots gone, .bss banks zero */
{
    memset(&mo, 0, sizeof mo);
    memset(proj_slot, 0, sizeof proj_slot);
    memset(lk_slot, 0, sizeof lk_slot);
    lk_lost = 0;
    persist_note = 0;
}
static int fdrf_round_trip(void)
{
    static lk_bank_t want;
    uint32_t i, ok = 1;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    wipe_ram();
    persist_boot();
    motion_add(0, 4, P_E1, 100);
    motion_add(3, 10, P_PAN, -64);
    motion_add(4, 63, P_LFO2 + LF_DEPTH, 127);
    lk_put(&mo.b, lk_key(0, 4, P_E1), 30, 1);         /* a lock on motion's knob and step */
    lk_put(&mo.b, lk_key(5, 2, P_FCUT), -10, 1);
    mo.b.on = 0x21u;
    want = mo.b;
    project_save(2);
    wipe_ram();
    persist_boot();
    project_load(2);
    for (i = 0; i < sizeof proj_slot[2].rsv_mo; i++)
        ok &= proj_slot[2].rsv_mo[i] == 0;
    return (int)ok && bank_events_eq(&mo.b, &want) && proj_slot[2].magic == PROJ_MAGIC && proj_slot[2].lk_n == 5u &&
           sizeof(project_t) == 3044u && sizeof proj_slot[2].rsv_mo + sizeof proj_slot[2].xp + sizeof proj_slot[2].xp2 +
                   sizeof proj_slot[2].xp3 == 504u &&
           sizeof proj_slot[2].xp == 42u && sizeof proj_slot[2].xp2 == 24u && sizeof proj_slot[2].xp3 == 12u && !proj_slot[2].var_len &&
           !proj_slot[2].var_link && str_eq(ui.msg, "LOADED");
}
/* MICRO (spec 2026-10-31 §1): "FRT6" keeps every step's micro in its spare byte, and the 7 parameters past the block
 * (KEYF, SRR, the INSERT's TYPE A B C MIX) in the reserved area (7 B a track; FREITAKT removed RESON: KEYF's 0) */
static int fdrg_micro_round_trip(void)
{
    static step_t want[NTRK][NSTEP];
    uint32_t k, ok = 1;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    wipe_ram();
    persist_boot();
    pattern_kit();
    trk[0].step[0].micro = -50;
    trk[0].step[4].micro = 50;
    trk[3].step[6].micro = 17;
    trk[4].step[63].micro = -1;                      /* (an off step's micro: kept) */
    trk[2].p[P_SRR] = 40;
    trk[5].p[P_ITYPE] = IT_CRUSH;
    trk[5].p[P_IMIX] = 0;
    trk[1].p[P_IA] = 3;
    for (k = 0; k < NTRK; k++)
        memcpy(want[k], trk[k].step, sizeof want[k]);
    project_save(2);
    ok &= proj_slot[2].magic == PROJ_MAGIC && proj_slot[2].t[0].step[4].micro == 50 &&
          proj_slot[2].t[0].step[0].micro == -50 && proj_slot[2].xp[2][0] == 0u && proj_slot[2].xp[2][PR_XP(P_SRR)] == 40u &&
          proj_slot[2].xp[5][PR_XP(P_IMIX)] == (uint8_t)(0 - TP[P_IMIX].def);
    host_init();
    wipe_ram();
    persist_boot();
    project_load(2);
    for (k = 0; k < NTRK; k++)
        ok &= !memcmp(trk[k].step, want[k], sizeof want[k]);
    ok &= trk[2].p[P_SRR] == 40 && trk[5].p[P_ITYPE] == IT_CRUSH && trk[5].p[P_IMIX] == 0 &&
          trk[1].p[P_IA] == 3 && trk[0].p[P_SRR] == 0 && trk[0].p[P_IMIX] == TP[P_IMIX].def;
    return (int)ok && proj_ok(&proj_slot[2]) && sizeof(project_t) == 3044u && str_eq(ui.msg, "LOADED");
}
static void fill_bank(int lock)                      /* mo.b: 1024 events */
{
    uint32_t i;
    for (i = 0; mo.b.count < LK_MAX; i++)
        lk_put(&mo.b, lk_key(i / 192u, i % 64u, P_E0 + (i / 64u) % 3u), (int32_t)(i % 100u), lock && (i & 1u));
}
static int bank_scan_offset_erased(void)
{
    uint32_t s, c, i, ok = 1;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    wipe_ram();
    persist_boot();
    fill_bank(1);
    project_save(0);
    project_save(0);
    project_save(3);
    for (s = 0; s < 4u; s++)
        for (c = 0; c < 2u; c++)
            for (i = 0xF00u; i < 0xF50u; i++)
                ok &= hflash[st_sector(OBJ_LOCKS0 + s, c) + i] == 0xFFu;
    ok &= hflash[st_sector(OBJ_LOCKS0 + 3u, 0)] != 0xFFu && hflash[st_sector(OBJ_LOCKS0, 1)] != 0xFFu;   /* (written) */
    return (int)ok;
}
/* Review Focus 1: a save torn by a power cut (the project record's first page, the bank's commit record, the bank's
 * payload): the previous project and its bank load together */
static int bank_torn_keeps_pair(void)
{
    static const int AT[3] = {16, 14, 6};            /* ops: bank erase 0, payload 1..13, header 14; project erase 15,
                                                      * its first page 16 */
    uint32_t c, ok = 1;
    for (c = 0; c < 3u; c++) {
        int i;
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        wipe_ram();
        persist_boot();
        trk[0].p[P_E1] = 11;
        lk_put(&mo.b, lk_key(0, 4, P_E1), 100, 1);
        project_save(1);                             /* pair A */
        trk[0].p[P_E1] = 22;
        lk_put(&mo.b, lk_key(0, 4, P_E1), 50, 1);
        host_nflog = 0;
        host_fail_at = AT[c];
        project_save(1);                             /* pair B, torn */
        host_fail_at = -1;
        ok &= str_eq(ui.msg, "SAVE ERROR") && host_nflog == (uint32_t)AT[c] + 1u;
        wipe_ram();
        host_init();
        persist_boot();
        project_load(1);
        i = lk_find(&mo.b, lk_key(0, 4, P_E1), 1);
        ok &= trk[0].p[P_E1] == 11 && i >= 0 && mo.b.val[i] == 100 && mo.b.count == 1u && !lk_lost &&
              !(persist_note & PN_LOCKS) && str_eq(ui.msg, "LOADED");
        if (!ok)
            printf("     torn at op %d: E1 %d, lock %d, count %u\n", AT[c], trk[0].p[P_E1], i >= 0 ? mo.b.val[i] : -1,
                   mo.b.count);
    }
    return (int)ok;
}
static int all_writes_in_map(void)
{
    uint32_t i, ok = host_nflog <= HOST_NFLOG;
    for (i = 0; ok && i < host_nflog; i++)
        ok &= host_fl_in_map(host_flog[i].off, host_flog[i].n);
    return (int)ok;
}
/* Review Focus 2: an 80 KiB USR3 uploaded (upstream Felucca) over the banks: USR3 shortened, every project loads with
 * an empty bank, LOCKS LOST said once after the USR3 note, later loads say LOADED */
static int usr3_upload_over_banks(void)
{
    uint32_t s, f, ok = 1, n_usr3 = 0, n_lost = 0, order = 1;
    char last[32] = "";
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    wipe_ram();
    persist_boot();
    for (s = 0; s < 4u; s++) {
        memset(&mo.b, 0, sizeof mo.b);
        lk_put(&mo.b, lk_key(s, 4, P_E1), (int32_t)(10u + s), 1);
        motion_add(s, 5, P_E2, 7);
        project_save(s);
    }
    rs = 81;
    usr3_image(0x13E00u);                            /* over 0xC8000..0xDBFFF: the banks too */
    host_nflog = 0;
    power_on(0);                                     /* (the last project, D, loads: LOADED) */
    ok &= rd32(U3 + 16u) == 0xBE00u && usr_nz[2] == 2 && (persist_note & PN_LOCKS) && lk_lost == 0x0Fu &&
          mo.b.count == 0u && str_eq(ui.msg, "LOADED") && all_writes_in_map();
    if (!ok)
        printf("     usr3 over banks at boot: len %x nz %u note %u lost %x count %u msg %s map %d\n", rd32(U3 + 16u),
               usr_nz[2], persist_note, lk_lost, mo.b.count, ui.msg, all_writes_in_map());
    for (f = 0; f < 200u; f++) {
        ui_frame();
        if (ui.msg_t && !str_eq(ui.msg, last)) {
            snprintf(last, sizeof last, "%s", ui.msg);
            if (str_eq(ui.msg, "USR3 SHORTENED TO 48K"))
                n_usr3++;
            if (str_eq(ui.msg, "LOCKS LOST")) {
                n_lost++;
                order &= n_usr3 == 1u;
            }
        }
    }
    ok &= n_usr3 == 1u && n_lost == 1u && order && persist_note == 0 && lk_lost == 0;
    for (s = 0; s < 4u; s++) {
        project_load(s);
        ok &= mo.b.count == 0u && str_eq(ui.msg, "LOADED") && project_used(s);
    }
    if (!ok)
        printf("     usr3 over banks: notes usr3 %u lost %u order %u\n", n_usr3, n_lost, order);
    return (int)ok && all_writes_in_map();
}
/* a bank with a valid storage record but wrong inside (both copies) loads empty: LOCKS LOST, never misread */
static int bad_bank_never_misread(void)
{
    static lk_bank_t bad;
    uint32_t c, ok = 1;
    for (c = 0; c < 5u; c++) {
        memset(hflash, 0xFF, sizeof hflash);
        host_init();
        wipe_ram();
        persist_boot();
        lk_put(&mo.b, lk_key(1, 4, P_E1), 100, 1);
        lk_put(&mo.b, lk_key(1, 5, P_E1), 90, 1);
        motion_add(1, 6, P_E2, 7);
        project_save(1);
        bad = lk_slot[1];
        if (c == 0u)
            bad.magic ^= 0x100u;                     /* (a) the inner magic */
        else if (c == 1u)
            lk_seal(&bad, 2);                        /* (b) slot 2's bank */
        else if (c == 2u) {                          /* (c) unsorted */
            uint16_t x = bad.key[0];
            bad.key[0] = bad.key[1];
            bad.key[1] = x;
        } else if (c == 3u)
            bad.count = 1025;                        /* (d) */
        else
            bad.val[0] = 101;                        /* (e) valid, but not the bank the project links */
        st_save(OBJ_LOCKS0 + 1u, &bad, sizeof bad);
        st_save(OBJ_LOCKS0 + 1u, &bad, sizeof bad);
        wipe_ram();
        host_init();
        persist_boot();
        ok &= (persist_note & PN_LOCKS) && lk_lost == 2u;
        project_load(1);
        ok &= mo.b.count == 0u && str_eq(ui.msg, "LOCKS LOST") && lk_lost == 0u && project_used(1);
        project_load(1);
        ok &= str_eq(ui.msg, "LOADED");
        if (!ok)
            printf("     bad bank case %u: count %u msg %s\n", c, mo.b.count, ui.msg);
    }
    return (int)ok;
}
static int writes_stay_in_map(void)
{
    uint32_t s;
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    wipe_ram();
    host_nflog = 0;
    persist_boot();
    fill_bank(1);
    for (s = 0; s < 4u; s++)
        project_save(s);
    project_rename(2, "MAPPED");
    return host_nflog > 60u && all_writes_in_map() && str_eq(ui.msg, "RENAMED");
}

static int save_error_rename_writes_bank(void)
{
    int i;
    char b[NAME_LEN + 1u];
    memset(hflash, 0xFF, sizeof hflash);
    host_init();
    wipe_ram();
    persist_boot();
    lk_put(&mo.b, lk_key(0, 4, P_E1), 100, 1);
    project_save(1);                                 /* pair A in flash */
    lk_put(&mo.b, lk_key(0, 4, P_E1), 50, 1);
    host_fail_at = 0;                                /* B: the bank's erase fails, nothing written */
    project_save(1);
    host_fail_at = -1;
    if (!str_eq(ui.msg, "SAVE ERROR"))
        return 0;
    host_nflog = 0;
    project_rename(1, "AFTER ERR");                  /* RAM's B (unsaved since the error): renamed in RAM only
                                                      * (spec 2026-10-26: flash keeps its version whole) */
    if (host_nflog || !str_eq(ui.msg, "RENAMED (SAVE TO KEEP)"))
        return 0;
    wipe_ram();
    host_init();
    persist_boot();
    project_load(1);
    i = lk_find(&mo.b, lk_key(0, 4, P_E1), 1);
    return i >= 0 && mo.b.val[i] == 100 && str_eq(ui.msg, "LOADED") && project_name(1, b) && str_eq(b, "");
}
/* review fix 2: the USR3 clamp failed (USR3 still longer than 48 KiB in flash): no lock-bank write that boot (an older
 * Hortator would play them as USR3's sound), said once; SAVE and RENAME refuse to write */
static int usr3_clamp_fail_blocks_banks(void)
{
    uint32_t i, f, ok = 1, said = 0, bank_writes = 0;
    memset(hflash, 0xFF, sizeof hflash);
    rs = 82;
    usr3_image(0x13E00u);
    persist_note = 0;
    host_fail_at = 0;                                /* the clamp's erase fails */
    power_on(0);
    host_fail_at = -1;
    for (f = 0; f < 60u; f++) {
        ui_frame();
        said |= str_eq(ui.msg, "USR3 CLAMP FAILED") && ui.msg_t;
    }
    ok &= said >= 1u && rd32(U3 + 16u) == 0x13E00u && usr_nz[2] == 0;
    host_nflog = 0;
    lk_put(&mo.b, lk_key(0, 4, P_E1), 100, 1);
    project_save(0);
    ok &= str_eq(ui.msg, "USR3 CLAMP FAILED");
    project_rename(0, "NOPE");
    for (i = 0; i < host_nflog && i < HOST_NFLOG; i++)
        bank_writes += host_flog[i].off >= 0xC9000u && host_flog[i].off < 0xDC000u;
    ok &= bank_writes == 0u;
    if (!ok)
        printf("     clamp failed: said %u len %x usr_nz %u bank writes %u msg %s\n", said, rd32(U3 + 16u), usr_nz[2],
               bank_writes, ui.msg);
    return (int)ok;
}

int main(void)
{
    char what[96];
    host_jedec_keep = host_jedec;
    int k, s, seq;
    check("project: the reverb TYPE survives a save and a load", fdr7_round_trip());
    check("project: TYPE HALL (2) survives a save and a load", hall_round_trip());
    check("project: the delay MODE (PING-PONG) survives a save and a load, in the reserved byte", dmode_round_trip());
    check("project: a record from before the delay MODE loads NORMAL", dmode_old_normal());
    check("project: FDR8 keeps the motion through a save and a load", fdr8_motion_round_trip());
    check("project: a broken motion block is dropped, the project loads", fdr8_bad_motion_dropped());
    check("project: a load while motion plays: the old base is not put back", fdr8_load_while_playing());
    check("project: FDRA keeps the name through a save and a load, the song bytes 0", fdr9_round_trip());
    check("project: FDRA keeps the five filter knobs through a save and a load", fdra_round_trip());
    check("project: out-of-range filter values are clamped on load", fdra_clamps());
    check("project: FDRB keeps SLIDE, TIE and the note through a save and a load", fdrb_round_trip());
    check("project: a stored step's reserved bits are cleared, its note and MICRO clamped", fdrb_clamps());
    check("project: a broken name is dropped, the project loads", fdr9_bad_name_dropped());
    check("project: a rename writes the name only, the current project's too", rename_slot());
    check("MEMB at every extreme: bounded (ASan)", memb_extremes());
    check("settings: MUTE NEXT BAR, ZOOM and KNOB ACCEL survive a power cycle (Felucca's settings format)", mutebar_persists());
    check("settings: USB LEVEL FIXED survives a power cycle; a save without it reads MASTER", usb_level_persists());
    check("settings: palette bits 12..13 (the removed USB AUDIO 10 CH) written 0; an older record's ignored",
          usb_audio_persists());
    check("settings: a record longer than ours loads the defaults (not truncated)", settings_oversize_refused());
    check("settings: an older record (no Bluetooth bits) loads as before (SPEAKER EQ BASS+ stays, no device)",
          settings_old_record());
    check("settings: the remembered device and MODE survive a power cycle in the spare bits; SPEAKER EQ keeps its value",
          settings_bt_round_trip());
    check("settings: a record's MODE bits beyond MIDI load AUTO", settings_bt_mode_range());
    check("settings: .noinit's settings stay 32 B (an update finds the RAM projects); no record: no device",
          settings_bt_after_update());
    check("settings: METRO survives a power cycle in lowcut's bits 5..6 (SPEAKER EQ, the device kept); 0.16.0's record "
          "and none load OFF", settings_metro_round_trip());
    check("settings: BACKLIGHT survives a power cycle in lowcut's bit 7 (SPEAKER EQ, the last project, METRO, the "
          "device kept); an older record and none load OFF; every level (item 9) kept, an older ON is DIM LO",
          settings_backlight_round_trip());
    check("settings: AUTOSTART survives a power cycle in the palette word's bit 8 (the palette, SPEAKER EQ, BACKLIGHT, "
          "the device kept); an older record and none load OFF", settings_autostart_round_trip());
    check("project: CLK beyond this build's range (BLE, LINK) loads as INT, not TRS", clock_beyond_range_is_int());
    check("settings: MIDI THRU survives a power cycle (palette bit 14, the rest kept); older records, none: OFF",
          thru_persists());
    check("settings: QUANT survives a power cycle (palette bits 16..17, the rest kept); older records, none: 1/16",
          quant_persists());
    check("project: SYNC / ROUT load OFF / NOTE from older projects; ROUT in range; SYNC BLE / BOTH without Bluetooth: "
          "OFF / USB", sync_rout_load());
    check("project: PRIO kept through a save and a load (FDRE)", fdrc_round_trip());
    check("project: the DEST slots kept through a save and a load (FRT6, 3044 B: one flash sector)", fdre_slots_round_trip());
    check("project: SRR and the INSERT kept through a save and a load (3044 B: the steps' spare bytes), 0 stored "
          "at their power-on values", fdre_inserts_round_trip());
    check("project: a record from before SRR and the INSERT loads with both off, playing as before",
          fdre_old_inserts_off());
    check("last project: a flash SAVE remembers its slot; power-on loads it (stopped, LOADED, no IRQ enable)",
          last_project_loads());
    check("last project: an empty slot asks START NEW PROJECT? over the power-on kit; NO asks again, YES forgets",
          last_project_empty());
    check("last project: an unreadable slot asks the same", last_project_corrupt());
    check("last project: safe start loads nothing and asks nothing", last_project_safe_start());
    check("last project: an older record boots the power-on kit; a RAM-only save remembers nothing",
          last_project_old_record_no_flash());
    check("project: an old 8-track record (Hortator's FDRG, 3840 B) in flash boots as an empty slot, nothing loaded",
          old_8track_record_boots_empty());
    check("last project: lowcut's bits keep SPEAKER EQ, the slot and the Bluetooth bytes", last_project_bits());
    check("FDRF: motion, locks and PLAY bits through a save and a load (the project record 3044 B, 504 B reserved: xp, "
          "xp2, the rest 0)",
          fdrf_round_trip());
    check("FRT6: every step's MICRO and the 7 parameters past the block through a save and a load (3044 B)",
          fdrg_micro_round_trip());
    check("FDRF: 1024 events: every bank sector's +0xF00..+0xF4F stays erased (the loader's scan)", bank_scan_offset_erased());
    check("FDRF: a save torn in the project record, the bank's commit or its payload: the old pair loads together",
          bank_torn_keeps_pair());
    check("FDRF: an 80 KiB USR3 over the banks: shortened, banks empty, LOCKS LOST once after the USR3 note",
          usr3_upload_over_banks());
    check("FDRF: a bank wrong inside (magic, slot, order, count, not the linked one) loads empty, LOCKS LOST",
          bad_bank_never_misread());
    check("FDRF: boot, four saves of 1024 events and a rename write only inside the flash map", writes_stay_in_map());
    check("FDRF: a RENAME after a SAVE ERROR is RAM's only (unsaved edits); flash's pair loads whole after a reset",
          save_error_rename_writes_bank());
    check("USR3: a failed clamp: no lock-bank write that boot (SAVE / RENAME refuse), USR3 CLAMP FAILED said",
          usr3_clamp_fail_blocks_banks());
    check("USR3: an 80 KiB sample is shortened to 48 KiB once (zones past cut, CRC new, the data kept)", usr3_clamped_once());
    check("USR3: a 48 KiB sample is not touched", usr3_short_untouched());
    check("USR3: a failed clamp leaves USR3 unusable (never read past 0xD3FFF), USR1 / USR2 as they were", usr3_clamp_fails_safe());
    check("USR3: the boot says USR3 SHORTENED TO 48K once", usr3_note_shown());
    check("USR2: an 80 KiB sample is shortened to 48 KiB once (zones past cut, CRC new, the data kept)", usr2_clamped_once());
    check("USR2: a 48 KiB sample is not touched", usr2_short_untouched());
    check("USR2: a failed clamp leaves USR2 unusable, no variation write that boot, USR1 as it was", usr2_clamp_fails_safe());
    check("USR2 and USR3 both long: one boot shortens both (USR2 SHORTENED first, then USR3's), the next writes nothing",
          usr2_usr3_both());
    for (k = 0; k < F_KINDS; k++)
        for (s = 0; s < (k == F_RANDOM || k == F_HEADERS ? 8 : 1); s++)
            for (seq = 0; seq < 2; seq++) {
                snprintf(what, sizeof what, "boot: %s #%d (%s)", KIND[k], s, seq ? "safe" : "normal");
                check(what, boot(k, (uint32_t)s + 1u, seq));
            }
    check("flash: no erase or program in any test outside the map (projects, USR2's header, the variations, USR3's "
          "header, lock banks, user objects, settings)", host_fl_stray == 0);
    check("flash: nothing in the core writes the Bluetooth store's sectors (0x9F000 / 0xFE000)", host_fl_bt == 0);
    printf(fails ? "boot_test: %d FAILED\n" : "boot_test: all passed\n", fails);
    return fails ? 1 : 0;
}
