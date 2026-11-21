/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* The MIDI KEYBOARD (GLO MIDI OUT 4/8 KBD, LOCL; midi_out.c mkb_*) on the host, with the UI harness, the audio
 * interrupt's render and TIMER5's sends modelled as tests/midiout_seq_test.c does (a half: its render, then its
 * 5 ms of TIMER5 ticks outside it). Plain build: USB; -DFELUCCA_BLE=1: the Bluetooth device too, and its link.
 *   toggle     KBD on GLO MIDI OUT 4/8: OFF TRK 1..16, turned on: KEYS on; KBD, LOCL saved in the settings
 *              record (palette bits 18..23; the host flash)
 *   notes      every key its own Note On / Off, F3 = 53, velocity 100, on KBD's channel (TRK: the selected track's)
 *   octave     OCT- / OCT+ -4..+4 (notes 5..127); a key held across an octave change: its Off is the note sent
 *   local      LOCL OFF: the keys play no internal sound (the sequencer plays on); LOCL ON: they do
 *   steps      no step entry: no live record, no STEP REC, no TOOLS screen (SEQ held); SCL still leaves KEYS
 *   release    held keys' Offs on leaving the mode (KBD OFF, KEYS off), on a dropped Bluetooth link
 *   update     an update's SysEx: nothing sent for 3 s; the Offs after it (All Notes Off)
 *   home       HOME's status line "MIDI KBD CH1 OCT+1", the top bar's "MIDI KBD F3" */
#define FELUCCA_OTA 1
#define FELUCCA_FLASH 1
#include <stdint.h>
static uint8_t hflash[0x100000];
#define SMP_USER_XIP(k) ((const uint8_t *)hflash + 0xA0000u + (k) * 0x14000u)
#include "ui_host.h"
#if FELUCCA_BLE
#include "ble_ui_stub.h"
uint32_t ble_midi_room(void) { return 64u; }
int ble_midi_send(const uint8_t *m, uint32_t n) { (void)m; (void)n; return 1; }
#include "../firmware/src/bt_set.c"
#include "../firmware/src/smc.c"
#include "../firmware/src/bt_midi.c"
#include "../firmware/src/bt_fml.c"
#include "../firmware/src/bt_devs.c"
#include "../firmware/src/bt_page.c"
#endif
static uint32_t ota_now_ms(void) { return fm1_ms; }
static void ota_idle(void) {}

static int fails;
static int check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        fails++;
    return ok;
}
static int same(const char *a, const char *b) { return !strcmp(a, b); }

/* ---- the audio interrupt and TIMER5 (USB: midi_out_q; the Bluetooth device: bt_out_q) ---- */
static uint32_t clk_t, sent[512], nsent, hits;
#if FELUCCA_BLE
static uint32_t bsent[512], nbsent;
#endif
static void half_run(void)
{
    static int32_t mixo[2 * CTL];
    uint32_t b, k, t0 = clk_t, a[NTRK];
    fm1_ms = t0 / 24000u;
    mout_half(t0);
    for (b = 0; b < HALF_FRAMES; b += CTL) {
        for (k = 0; k < NTRK; k++)
            a[k] = hit_age(&trk[k]);
        mix_block(mixo, CTL);
        for (k = 0; k < NTRK; k++)
            hits += hit_age(&trk[k]) != a[k];
    }
    mout_half_done(t0);
    for (k = 1; k <= 58u; k++)
        mout_tick(t0 + k * MOUT_TICK, 0);
    clk_t = t0 + MOUT_HALF_TICKS;
    fm1_ms = clk_t / 24000u;
    for (; mo_r != mo_w; mo_r++)
        if (nsent < 512u)
            sent[nsent++] = midi_out_q[mo_r % MQ];
#if FELUCCA_BLE
    for (; bt_out_r != bt_out_w; bt_out_r++)
        if (nbsent < 512u)
            bsent[nbsent++] = bt_out_q[bt_out_r % BT_OUT_Q];
#endif
}
static void run_ms(uint32_t ms)
{
    uint32_t i, n = ms * FS / 1000u / HALF_FRAMES + 1u;
    for (i = 0; i < n; i++)
        half_run();
}
static void kh(uint32_t notes)                     /* the keys held now, then 10 ms of audio */
{
    fm1_in.notes = notes;
    run_ms(10);
}
static void frame(void)
{
    ui_frame();
    run_ms(10);
}
static void tap_b(uint32_t b)
{
    press(b);
    frame();
    release_all();
    frame();
}
static uint32_t st_of(uint32_t p) { return (p >> 8) & 0xFFu; }
static int is_on(uint32_t p) { return (p & 0xFu) == 9u && (st_of(p) & 0xF0u) == 0x90u && (p >> 24); }
static int is_off(uint32_t p) { return (p & 0xFu) == 8u && (st_of(p) & 0xF0u) == 0x80u; }
static uint32_t ch_of(uint32_t p) { return (st_of(p) & 0xFu) + 1u; }
static uint32_t nt_of(uint32_t p) { return (p >> 16) & 0x7Fu; }
static uint32_t vel_of(uint32_t p) { return p >> 24; }
static int has(uint32_t on, uint32_t ch, uint32_t nt)   /* sent: a Note On (1) / Off (0) */
{
    uint32_t i;
    for (i = 0; i < nsent; i++)
        if ((on ? is_on(sent[i]) : is_off(sent[i])) && ch_of(sent[i]) == ch && nt_of(sent[i]) == nt)
            return 1;
    return 0;
}
static uint32_t count(int (*f)(uint32_t))
{
    uint32_t i, n = 0;
    for (i = 0; i < nsent; i++)
        n += f(sent[i]) != 0;
    return n;
}
static int balanced(void)                            /* every On its Off (an All Notes Off settles its channel) */
{
    static int32_t open[16][128];
    uint32_t i, c, n;
    memset(open, 0, sizeof open);
    for (i = 0; i < nsent; i++) {
        uint32_t p = sent[i];
        if (is_on(p))
            open[ch_of(p) - 1u][nt_of(p)]++;
        else if (is_off(p) && open[ch_of(p) - 1u][nt_of(p)] > 0)
            open[ch_of(p) - 1u][nt_of(p)]--;
        else if ((p & 0xFu) == 0xBu && nt_of(p) == 123u)
            memset(open[st_of(p) & 0xFu], 0, sizeof open[0]);
    }
    for (c = 0; c < 16u; c++)
        for (n = 0; n < 128u; n++)
            if (open[c][n])
                return 0;
    return 1;
}
static void clear_sent(void)
{
    nsent = 0;
#if FELUCCA_BLE
    nbsent = 0;
#endif
}

/* GLO's MIDI OUT page (4/8), KNOB 3 KBD */
static void to_midi_out(void)
{
    uint32_t k;
    for (k = 0; k < 10u && (ui.home || !same(cur_page()->title, "MIDI OUT")); k++)
        tap_b(B_GLO);
}
static void fresh(void)
{
    memset(hflash, 0xFF, sizeof hflash);
    host_jedec = 0x856014u;
    ui_host_init();
    persist_boot();
    settings_init();
    memset(&mout, 0, sizeof mout);
    memset(&mon, 0, sizeof mon);
    memset(&mkb, 0, sizeof mkb);
    mkb_kbd = 0;
    mkb_local_off = 0;
    midi_thru = 0;
    mo_r = mo_w = 0;
    usb.config = 1;
    usb.ota_req = 0;
    usb.sx_on = 0;
    sx_collect = sx_ready = 0;
    so_r = so_w = 0;
    fm1_in.notes = 0;
    kb_prev = 0;
    clk_t = 0;
#if FELUCCA_BLE
    bt_out_r = bt_out_w = 0;
    mkb.downs = (uint8_t)bt_link_downs();
#endif
    clear_sent();
}
static void kbd_on(int32_t clicks)                   /* KBD: OFF + clicks (1 TRK, 2 channel 1, ...) */
{
    to_midi_out();
    turn(EN_K3, clicks);
    frame();
}

static void test_toggle(void)
{
    int16_t *vp;
    const param_desc_t *d;
    char v[12];
    const char *u;
    fresh();
    to_midi_out();
    d = page_desc(cur_page(), 2, &vp);
    param_format(d, *vp, v, &u);
    check("GLO MIDI OUT: KNOB 3 is KBD, OFF", d && same(d->label, "KBD") && same(v, "OFF") && !mkb_kbd);
    d = page_desc(cur_page(), 3, &vp);
    param_format(d, *vp, v, &u);
    check("GLO MIDI OUT: KNOB 4 is LOCL, ON", d && same(d->label, "LOCL") && same(v, "ON") && !mkb_local_off);
    check("KEYS off before", !ui.keys);
    turn(EN_K3, 1);
    frame();
    d = page_desc(cur_page(), 2, &vp);
    param_format(d, *vp, v, &u);
    check("KBD -> TRK: the MIDI KEYBOARD on, KEYS on with it", mkb_kbd == 1u && same(v, "TRK") && ui.keys &&
                                                                 song.seq_mode == 3u && kbd_mode());
    turn(EN_K3, 30);
    frame();
    param_format(d, *vp, v, &u);
    check("KBD: up to channel 16 at most", mkb_kbd == 17u && same(v, "16"));
    turn(EN_K3, -40);
    frame();
    check("KBD -> OFF: the mode off (KEYS stays on)", mkb_kbd == 0u && ui.keys && !kbd_mode());
    turn(EN_K3, 6);                                  /* channel 5 */
    turn(EN_K4, -1);                                 /* LOCL OFF */
    frame();
    mkb_kbd = 0;
    mkb_local_off = 0;
    persist_boot();
    check("stored: KBD 5 and LOCL OFF back after a power cycle (the settings record)", mkb_kbd == 6u && mkb_local_off);
    memset(hflash, 0xFF, sizeof hflash);
    persist_boot();
    check("a blank flash: KBD OFF, LOCL ON", !mkb_kbd && !mkb_local_off);
}

static void test_notes(void)
{
    uint32_t k, ok = 1;
    fresh();
    kbd_on(1);                                       /* TRK: track 1's MIDI OUT channel, 1 */
    for (k = 0; k < 27u; k++) {
        clear_sent();
        kh(1u << k);
        ok &= has(1, 1, 53u + k) && count(is_on) == 1u && vel_of(sent[0]) == 100u;
        clear_sent();
        kh(0);
        ok &= has(0, 1, 53u + k) && count(is_off) == 1u;
    }
    check("all 27 keys: Note On 53 + key (F3 the first), velocity 100, its Off on release, channel 1 (TRK, T1)", ok);
    clear_sent();
    kh(1u | 1u << 4 | 1u << 26);
    check("three keys at once: three Ons", count(is_on) == 3u && has(1, 1, 53) && has(1, 1, 57) && has(1, 1, 79));
    kh(1u << 4);
    check(".. two let go: their Offs only", count(is_off) == 2u && has(0, 1, 53) && has(0, 1, 79) && !has(0, 1, 57));
    kh(0);
    check(".. all let go: balanced", balanced());
    song.sel = 2;
    song.g[G_MCH1 + 2] = 3;
    clear_sent();
    kh(1);
    kh(0);
    check("TRK: track 3 selected -> channel 3", has(1, 3, 53) && has(0, 3, 53));
    song.g[G_MCH1 + 2] = 0;
    clear_sent();
    kh(1);
    kh(0);
    check("TRK on a track whose MIDI OUT is OFF: nothing", count(is_on) == 0u);
    song.sel = 0;
    to_midi_out();
    turn(EN_K3, 9);                                  /* TRK -> channel 9 */
    frame();
    clear_sent();
    kh(1u << 2);
    check("KBD 9: channel 9", mkb_kbd == 10u && has(1, 9, 55));
    to_midi_out();
    turn(EN_K3, 3);                                  /* .. changed while the key is held */
    frame();
    kh(0);
    check("KBD changed while a key is held: its Off on the channel its On went out on", has(0, 9, 55) &&
                                                                                         !has(0, 12, 55));
    check("the pad's own MIDI (the drum channel) not sent for the MIDI KEYBOARD's keys", count(is_on) == 1u);
}

static void test_octave(void)
{
    uint32_t k;
    fresh();
    kbd_on(2);                                       /* channel 1 */
    tap_b(B_HOME);
    for (k = 0; k < 6u; k++)
        tap_b(B_OCTUP);
    check("OCT+: up to +4 as the MIDI KEYBOARD", song.octave == 4);
    clear_sent();
    kh(1u << 26);
    kh(0);
    check("OCT +4: the top key 127", has(1, 1, 127));
    for (k = 0; k < 10u; k++)
        tap_b(B_OCTDN);
    check("OCT-: down to -4", song.octave == -4);
    clear_sent();
    kh(1);
    kh(0);
    check("OCT -4: the first key 5", has(1, 1, 5));
    for (k = 0; k < 4u; k++)
        tap_b(B_OCTUP);
    clear_sent();
    kh(1u << 3);                                   /* G#3 = 56 */
    tap_b(B_OCTUP);
    check("OCT+ while a key is held", song.octave == 1);
    kh(0);
    check(".. its Off is the note sent (56), not the shifted one (68)", has(0, 1, 56) && !has(0, 1, 68) && balanced());
    clear_sent();
    kh(1u << 3);
    kh(0);
    check(".. pressed again: the new octave", has(1, 1, 68));
    to_midi_out();
    turn(EN_K3, -30);
    frame();
    check("KBD OFF: the octave back to KEYS' -2..+2", !mkb_kbd && song.octave == 1);
    tap_b(B_HOME);
    tap_b(B_OCTUP);
    tap_b(B_OCTUP);
    check("KEYS without the MIDI KEYBOARD: +2 at most", song.octave == 2);
}

static void test_local(void)
{
    uint32_t h0;
    fresh();
    kbd_on(2);
    hits = 0;
    kh(1);
    kh(0);
    run_ms(50);
    check("LOCL ON: the key plays the selected track too", hits >= 1u && has(1, 1, 53));
    to_midi_out();
    turn(EN_K4, -1);
    frame();
    check("LOCL -> OFF", mkb_local_off == 1u);
    hits = 0;
    clear_sent();
    kh(1);
    kh(0);
    run_ms(50);
    check("LOCL OFF: no internal sound, the MIDI still sent", hits == 0u && has(1, 1, 53) && has(0, 1, 53));
    trk[1].step[0].on = 1;
    trk[1].p[P_SLEN] = 1;
    transport_req = 1;
    h0 = hits;
    run_ms(300);
    check("LOCL OFF: the sequencer still plays", song.playing && hits > h0);
    transport_req = 2;
    run_ms(20);
}

static void test_steps(void)
{
    uint32_t k, s0 = 0;
    fresh();
    kbd_on(2);
    tap_b(B_HOME);
    song.rec = 1u;
    transport_req = 1;
    run_ms(20);
    kh(1u << 5);
    run_ms(100);
    kh(0);
    for (k = 0; k < 64u; k++)
        s0 += trk[0].step[k].on;
    check("armed and playing: the keys record nothing", s0 == 0u && has(1, 1, 58));
    transport_req = 2;
    run_ms(20);
    song.rec = 0;
    tap_b(B_SEQ);                                    /* a SEQ page */
    check("STEP REC not available (REC on the STEP page)", !srec_ok());
    press(B_SEQ);
    for (k = 0; k < 20u; k++) {
        host_ticks += 50u * 1000u * FM1_TICKS_PER_US;
        frame();
    }
    check("SEQ held: no TOOLS screen", !ui.tools);
    fm1_in.notes = 1u << 2;
    frame();
    fm1_in.notes = 0;
    frame();
    release_all();
    frame();
    check("SEQ + a white key: no TOOLS, no step", !ui.tools && !trk[0].step[0].on && !trk[0].step[1].on);
    clear_sent();
    kh(1u << 2);
    kh(0);
    check("the keys still play the MIDI KEYBOARD after", has(1, 1, 55));
}

static void test_release(void)
{
    fresh();
    kbd_on(2);
    clear_sent();
    kh(1u | 1u << 7);
    to_midi_out();
    turn(EN_K3, -5);                                 /* KBD OFF with two keys held */
    frame();
    check("KBD OFF with keys held: their Offs at once", has(0, 1, 53) && has(0, 1, 60) && balanced());
    clear_sent();
    kh(0);
    check(".. their release later: nothing more", count(is_off) == 0u && count(is_on) == 0u);
    turn(EN_K3, 2);
    frame();
    clear_sent();
    kh(1u << 1);
    tap_b(B_SCL);                                    /* KEYS off */
    check("KEYS off (SCL) with a key held: its Off", !ui.keys && has(0, 1, 54) && balanced());
    kh(0);
    tap_b(B_SCL);
    clear_sent();
    kh(1u << 1);
    tap_b(B_GLO);
    tap_b(B_GLO);
    kh(0);
    check("a page change while held: the Off once, balanced", balanced() && count(is_off) == 1u);
#if FELUCCA_BLE
    {
        bt_set_t s;
        uint32_t i, on = 0, off = 0;
        memset(&bt_st, 0, sizeof bt_st);
        bt_set_get(&s);
        s.mode = 2;                                  /* MODE MIDI */
        bt_set_put(&s);
        memset(conn_name, 0, sizeof conn_name);
        str_cpy(conn_name, "nanoKEY", sizeof conn_name);
        conn_up = 1;
        bt_link(1);
        tap_b(B_HOME);
        check("(Bluetooth MIDI device linked)", bt_in.out_on && kbd_mode());
        clear_sent();
        kh(1u << 9);
        for (i = 0; i < nbsent; i++)
            on += is_on(bsent[i]) && nt_of(bsent[i]) == 62u;
        check("BLE build: the Note On to USB and to the Bluetooth device", on == 1u && has(1, 1, 62));
        conn_up = 0;
        bt_link(0);
        frame();
        check("the link drops with a key held: its Off (USB) at once", has(0, 1, 62));
        clear_sent();
        kh(0);
        for (i = 0; i < nbsent; i++)
            off += is_off(bsent[i]);
        check(".. its release later: nothing more", count(is_off) == 0u && off == 0u);
        clear_sent();
        kh(1u << 9);
        kh(0);
        check("unlinked: the keys still go to USB", has(1, 1, 62) && has(0, 1, 62) && nbsent == 0u);
    }
#endif
}

static void test_update(void)
{
    uint32_t n0;
    fresh();
    kbd_on(2);
    clear_sent();
    kh(1u << 4);
    usb.ota_req = 1;                                 /* an update starts with the key held */
    run_ms(10);
    usb.ota_req = 0;
    n0 = nsent;
    kh(0);
    kh(1u << 6);
    kh(0);
    run_ms(2900);
    check("update: nothing sent while it runs and for 3 s after", nsent == n0 && has(1, 1, 57) && !has(0, 1, 57));
    run_ms(300);
    check(".. then the Offs made good (All Notes Off on the channel)", balanced());
    clear_sent();
    kh(1);
    kh(0);
    check(".. and the keys play on", has(1, 1, 53) && has(0, 1, 53));
}

static void test_home(void)
{
    char s[24], b[16];
    fresh();
    kbd_on(2);
    tap_b(B_HOME);
    if (ui.home != 1u)
        tap_b(B_HOME);
    tap_b(B_OCTUP);
    home_status(s);
    check("HOME's status line: MIDI KBD CH1 OCT+1", same(s, "MIDI KBD CH1 OCT+1"));
    tap_b(B_OCTDN);
    tap_b(B_OCTDN);
    home_status(s);
    check(".. OCT-1", same(s, "MIDI KBD CH1 OCT-1"));
    tap_b(B_OCTUP);
    home_status(s);
    check(".. OCT0", same(s, "MIDI KBD CH1 OCT0"));
    head_mode(b);
    check("the top bar: MIDI KBD F3", same(b, "MIDI KBD F3"));
    mkb_kbd = 1;
    song.g[G_MCH1] = 0;
    home_status(s);
    check("TRK on a track with MIDI OUT OFF: CH-", same(s, "MIDI KBD CH- OCT0"));
    song.g[G_MCH1] = 1;
    mkb_kbd = 0;
    home_status(s);
    check("KBD OFF: the usual status line", s[0] == 'P');
    ui_draw();
}

int main(void)
{
    test_toggle();
    test_notes();
    test_octave();
    test_local();
    test_steps();
    test_release();
    test_update();
    test_home();
    printf("%s\n", fails ? "MIDI KEYBOARD TESTS FAILED" : "midikbd_test: all passed");
    return fails != 0;
}
