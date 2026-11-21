/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* GLO MIDI OUT (midi_out.c mon_*): the sequencer's hits as Note On / Off, on the host with the FM-1's timing modelled
 * as tests/sync_test.c does (TIMER4 ticks, the audio IRQ every half with its render, TIMER5 every 100 us polling USB
 * every 5th tick outside the render). A hit's time: the block its voice starts in, heard one half after its IRQ.
 *   channels   track n on channel n by default; a track's channel set; a track OFF; OUT OFF: nothing
 *   notes      a drum: the track's NOTE (the kit's GM notes) whatever the step's NOTE; a pitched model (FM, ACID):
 *              60 + the step's note
 *   arp        ARP / RTRG: every repeat a note, ARP's chord notes in order, as the engine plays them
 *   timing     each Note On within 1 ms of its hit's heard time; swing, MICRO, FIT (LEN 5 FIT vs LEN 16) kept
 *   off        the Off after the step's length (RATCH: its share); ACID: when its gate closes; a slide legato
 *   hanging    STOP, PLAY again, a mute: the Off at once; every On has its Off
 *   update     an update's SysEx: nothing sent for 3 s, then the notes go on
 *   overflow   no TIMER5 (a host not reading): the queue fills, Note Ons dropped and counted, the sound unchanged,
 *              nothing blocks; STOP then still ends every note (All Notes Off for a lost Off)
 *   clock      SYNC and 6 tracks hitting together: no Clock lost
 *   ble        (the Bluetooth build) OUT BLE / BOTH: the device's queue
 *   cost       host instructions: the render with 6 tracks + RTRG 1/32, OUT USB vs OFF; TIMER5's share */
#define FELUCCA_OTA 1
#include "drum_host.h"
#include <math.h>

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

/* ---- the FM-1's timing (tests/sync_test.c) ---- */
#define T_TICK 2400u
#define T_HALF ((double)HALF_FRAMES * 24000000.0 / FS)
static uint32_t now_t, audio_end, main_sub, main_owed, samp, no_timer5;
static double half_at, load;
typedef struct { uint32_t pkt, t; } sent_t;
#define MAXS 60000u
static sent_t sent[MAXS];
static uint32_t nsent;
typedef struct { uint32_t s; uint8_t k; int8_t nt; } hit_t;   /* an engine hit: its block's first sample, track, note */
#define MAXH 40000u
static hit_t hit[MAXH];
static uint32_t nhit;
static uint64_t isr_instr, t5_instr;
static int count_instr;

static void usb_poll_model(void)
{
    while (mo_r != mo_w) {
        if (nsent < MAXS)
            sent[nsent++] = (sent_t){midi_out_q[mo_r % MQ], now_t};
        mo_r++;
    }
}
static void render_half(void)                          /* audio.c: a half, block by block, the hits recorded */
{
    static int32_t mixo[2 * CTL];
    uint32_t b, k, a[NTRK];
    uint64_t i0 = count_instr ? instr_now() : 0;
    for (b = 0; b < HALF_FRAMES; b += CTL) {
        for (k = 0; k < NTRK; k++)
            a[k] = hit_age(&trk[k]);
        mix_block(mixo, CTL);
        for (k = 0; k < NTRK; k++)
            if (hit_age(&trk[k]) != a[k] && nhit < MAXH)
                hit[nhit++] = (hit_t){samp, (uint8_t)k, trk[k].nt};
        samp += CTL;
    }
    if (count_instr)
        isr_instr += instr_now() - i0;
}
static void hw_tick(void)
{
    uint32_t in_audio;
    now_t += T_TICK;
    while ((double)now_t >= half_at) {
        uint32_t t0 = (uint32_t)half_at;
        fm1_ms = t0 / 24000u;
        mout_half(t0);
        render_half();
        audio_end = t0 + (uint32_t)(load * T_HALF);
        mout_half_done(audio_end);
        half_at += T_HALF;
    }
    fm1_ms = now_t / 24000u;
    in_audio = (int32_t)(audio_end - now_t) > 0;
    if (!no_timer5) {
        uint64_t i0 = count_instr ? instr_now() : 0;
        mout_tick(now_t, in_audio);
        if (count_instr)
            t5_instr += instr_now() - i0;
    }
    if (main_sub % 5u == 0u)
        main_owed |= 1u;
    if (++main_sub == 10u)
        main_sub = 0;
    if (!in_audio) {
        if ((main_owed & 1u) && !no_timer5)
            usb_poll_model();
        main_owed = 0;
    }
}
static void hw_run_ms(uint32_t ms)
{
    uint32_t i;
    for (i = 0; i < ms * 10u; i++)
        hw_tick();
}
#if FELUCCA_BLE
static uint32_t bq[256], bq_w, bq_r;
static int bt_sync_out(uint32_t pkt, int rt)
{
    (void)rt;
    if (bq_w - bq_r >= 256u)
        return 0;
    bq[bq_w++ % 256u] = pkt;
    return 1;
}
static void bt_midi_out_isr(uint32_t pkt) { (void)pkt; }
static void bt_fml_block(uint32_t n, uint32_t *seq_n)
{
    (void)n;
    (void)seq_n;
}
static void bt_fml_adv(void) {}
static uint32_t bt_link_downs(void) { return 0; }
#endif
/* a kit as the firmware's power-on (the GM notes), the steps cleared, 1/16, LEN 16, MIDI OUT USB */
static void hw_reset(void)
{
    uint32_t k;
    host_init_live();
    memset(&mout, 0, sizeof mout);
    memset(&mon, 0, sizeof mon);
    midi_thru = 0;
    mo_r = mo_w = 0;
    usb.config = 1;
    now_t = audio_end = main_sub = main_owed = samp = no_timer5 = 0;
    half_at = T_HALF;
    load = 0.05;
    nsent = nhit = 0;
    sx_collect = sx_ready = 0;
    so_r = so_w = 0;
    usb.sx_on = 0;
    usb.ota_req = 0;
    song.g[G_BPM] = 120;
    for (k = 0; k < NTRK; k++) {
        memset(trk[k].step, 0, sizeof trk[k].step);
        trk[k].p[P_SLEN] = 16;
        trk[k].p[P_SDIV] = 2;
        trk[k].p[P_SSWING] = 0;
    }
#if FELUCCA_BLE
    bq_w = bq_r = 0;
#endif
}

/* ---- what was sent ---- */
static uint32_t st_of(uint32_t pkt) { return (pkt >> 8) & 0xFFu; }
static int is_on(uint32_t pkt) { return (pkt & 0xFu) == 9u && (st_of(pkt) & 0xF0u) == 0x90u && (pkt >> 24) > 0u; }
static int is_off(uint32_t pkt) { return (pkt & 0xFu) == 8u && (st_of(pkt) & 0xF0u) == 0x80u; }
static uint32_t ch_of(uint32_t pkt) { return (st_of(pkt) & 0xFu) + 1u; }
static uint32_t nt_of(uint32_t pkt) { return (pkt >> 16) & 0x7Fu; }
static uint32_t ons(uint32_t ch)                       /* Note Ons on channel ch (0: any) */
{
    uint32_t i, n = 0;
    for (i = 0; i < nsent; i++)
        n += is_on(sent[i].pkt) && (!ch || ch_of(sent[i].pkt) == ch);
    return n;
}
static uint32_t notes_sent(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < nsent; i++)
        n += is_on(sent[i].pkt) || is_off(sent[i].pkt);
    return n;
}
/* every On has its Off (channel and note), no note left sounding; an All Notes Off on a channel settles it */
static int balanced(void)
{
    static int32_t open[16][128];
    uint32_t i, c, n;
    memset(open, 0, sizeof open);
    for (i = 0; i < nsent; i++) {
        uint32_t p = sent[i].pkt;
        if (is_on(p))
            open[ch_of(p) - 1u][nt_of(p)]++;
        else if (is_off(p) && open[ch_of(p) - 1u][nt_of(p)] > 0)
            open[ch_of(p) - 1u][nt_of(p)]--;   /* (an Off without its On: harmless, an update swallowed the On) */
        else if ((p & 0xFu) == 0xBu && nt_of(p) == 123u)
            memset(open[st_of(p) & 0xFu], 0, sizeof open[0]);
    }
    for (c = 0; c < 16u; c++)
        for (n = 0; n < 128u; n++)
            if (open[c][n] != 0)
                return 0;
    return 1;
}
static double heard_tick(uint32_t s) { return 2.0 * T_HALF + (double)s * 24000000.0 / FS; }   /* sample s heard */
static double ms_of(double ticks) { return ticks / 24000.0; }

static void play_ms(uint32_t ms)
{
    transport_req = 1;
    hw_run_ms(ms);
    transport_req = 2;
    hw_run_ms(50);
}

/* ---- channels, OFF, the notes ---- */
static void test_channels_notes(void)
{
    static const uint8_t GM[NTRK] = {36, 38, 39, 42, 46, 45};
    uint32_t k, i, ok = 1, seen = 0;
    hw_reset();
    check("defaults: OUT USB, track n on channel n", song.g[G_MOUT] == 1 && song.g[G_MCH1] == 1 &&
                                                         song.g[G_MCH6] == 6);
    for (k = 0; k < NTRK; k++) {
        trk[k].step[0].on = 1;
        trk[k].step[0].nt = 5;                          /* (a drum's step NOTE: its pitch, not its MIDI note) */
    }
    play_ms(400);
    for (i = 0; i < nsent; i++)
        if (is_on(sent[i].pkt)) {
            k = ch_of(sent[i].pkt) - 1u;
            ok &= k < NTRK && nt_of(sent[i].pkt) == GM[k] && (sent[i].pkt >> 24) == 96u;
            seen |= 1u << k;
        }
    check("each track its own channel 1..6, a drum's note the track's NOTE (GM 36 38 39 42 46 45), velocity 96",
          ok && seen == 0x3Fu && ons(0) == 6u);
    check("STOP: every On has its Off", balanced());

    hw_reset();
    for (k = 0; k < NTRK; k++)
        trk[k].step[0].on = 1;
    trk[0].step[0].acc = 1;
    song.g[G_MCH1 + 1] = 10;
    song.g[G_MCH1 + 2] = 0;                             /* track 3 OFF */
    trk[3].p[P_NOTE] = 50;
    play_ms(300);
    check("a track's channel set (track 2 on 10), track 3 OFF: nothing from it; NOTE 50 on track 4",
          ons(2) == 0u && ons(10) == 1u && ons(3) == 0u && ons(4) == 1u && ons(0) == 5u);
    for (i = 0, ok = 0; i < nsent; i++)
        ok |= is_on(sent[i].pkt) && ch_of(sent[i].pkt) == 4u && nt_of(sent[i].pkt) == 50u;
    check("NOTE knob is the drum's MIDI note", ok);
    for (i = 0, ok = 0; i < nsent; i++)
        ok |= is_on(sent[i].pkt) && ch_of(sent[i].pkt) == 1u && (sent[i].pkt >> 24) == 127u;
    check("ACC: velocity 127", ok);

    hw_reset();
    for (k = 0; k < NTRK; k++)
        trk[k].step[0].on = 1;
    song.g[G_MOUT] = 0;
    play_ms(300);
    check("OUT OFF: no note at all", notes_sent() == 0u && nhit == 6u);

    hw_reset();
    drum_set_model(&trk[0], DM_FM);
    drum_set_model(&trk[1], DM_ACID);
    trk[0].step[0].on = 1;
    trk[0].step[0].nt = 7;
    trk[1].step[0].on = 1;
    trk[1].step[0].nt = -12;
    play_ms(300);
    for (i = 0, ok = 0; i < nsent; i++)
        if (is_on(sent[i].pkt))
            ok |= (ch_of(sent[i].pkt) == 1u && nt_of(sent[i].pkt) == 67u) << 0 |
                  (ch_of(sent[i].pkt) == 2u && nt_of(sent[i].pkt) == 48u) << 1;
    check("pitched (FM, ACID): 60 + the step's NOTE (67, 48)", ok == 3u && ons(0) == 2u && balanced());
}

/* ---- the hits and their notes, one to one; timing ---- */
static int match_hits(const char *what, uint32_t only, double *worst_ms)
{
    uint32_t i, j = 0, ok = 1, n = 0;
    double w = 0;
    for (i = 0; i < nhit; i++) {
        uint32_t k = hit[i].k, want_nt;
        if (!((only >> k) & 1u))
            continue;
        n++;
        want_nt = arp_model(voice_model(&trk[k])) ? (uint32_t)(60 + hit[i].nt) : (uint32_t)trk[k].p[P_NOTE];
        while (j < nsent && !(is_on(sent[j].pkt) && ch_of(sent[j].pkt) == k + 1u))
            j++;
        if (j == nsent) {
            ok = 0;
            break;
        }
        if (nt_of(sent[j].pkt) != want_nt)
            ok = 0;
        {
            double d = fabs((double)sent[j].t - heard_tick(hit[i].s));
            w = d > w ? d : w;
        }
        j++;
    }
    for (; j < nsent; j++)
        if (is_on(sent[j].pkt) && ((only >> (ch_of(sent[j].pkt) - 1u)) & 1u))
            ok = 0;                                    /* a note without its hit */
    if (worst_ms)
        *worst_ms = ms_of(w);
    if (!n)
        ok = 0;
    (void)what;
    return (int)ok;
}

static void test_arp(void)
{
    uint32_t k, i, f[4], nf = 0;
    double w;
    char what[200];
    hw_reset();
    drum_set_model(&trk[0], DM_FM);
    trk[0].p[P_AMODE] = 2;                              /* ARP */
    trk[0].p[P_ARATE] = 6;                              /* 1/32 */
    trk[0].p[P_ACHORD] = 2;                             /* MAJ */
    trk[0].p[P_ARANGE] = 2;
    trk[0].p[P_AFADE] = 0;
    trk[0].step[0].on = 1;
    trk[0].step[0].nt = 2;
    for (k = 1; k < 16u; k++)
        trk[0].step[k].tie = 1;
    trk[1].p[P_AMODE] = 1;                              /* RTRG on a drum */
    trk[1].p[P_ARATE] = 6;
    trk[1].p[P_AFADE] = 0;
    trk[1].step[0].on = trk[1].step[8].on = 1;
    for (k = 1; k < 16u; k++)
        trk[1].step[k].tie = k != 8u;
    play_ms(2100);
    for (i = 0; i < nsent && nf < 4u; i++)
        if (is_on(sent[i].pkt) && ch_of(sent[i].pkt) == 1u)
            f[nf++] = nt_of(sent[i].pkt);
    snprintf(what, sizeof what, "ARP 1/32 MAJ x2 from D4: every repeat a note as the engine plays it (%u notes, first "
             "%u %u %u %u)", ons(1), f[0], f[1], f[2], f[3]);
    check(what, match_hits("arp", 1u, &w) && ons(1) >= 30u && nf == 4u && f[0] == 62u && f[1] == 66u && f[2] == 69u &&
                    f[3] == 74u);
    snprintf(what, sizeof what, "RTRG 1/32 on a drum: every repeat its NOTE (%u notes), each within 1 ms of its hit "
             "(worst %.2f ms)", ons(2), w);
    check(what, match_hits("rtrg", 2u, &w) && ons(2) >= 30u && w <= 1.0);
    check("ARP / RTRG: every On has its Off", balanced());
}

static void test_timing(void)
{
    uint32_t i, a = 0, b = 0;
    double w;
    char what[200];
    hw_reset();
    for (i = 0; i < 16u; i++)
        trk[0].step[i].on = 1;                          /* 1/16, swing 60 */
    trk[0].p[P_SSWING] = 60;
    trk[1].p[P_SLEN] = 5;                               /* LEN 5 over a bar (FIT FBAR) */
    trk[1].p[P_SDIV] = DIV_FIT + 1;
    for (i = 0; i < 5u; i++)
        trk[1].step[i].on = 1;
    trk[2].step[4].on = 1;                              /* MICRO +25: a quarter step late */
    trk[2].step[4].micro = 25;
    trk[2].step[12].on = 1;
    trk[2].step[12].micro = -40;
    song.g[G_BPM] = 97;
    load = 0.5;
    play_ms(4000);                                      /* ~1.6 bars at 97 */
    check("timing: swing 60, FIT LEN 5, MICRO: one note per hit", match_hits("t", 7u, &w));
    snprintf(what, sizeof what, "timing: every Note On within a USB poll of its hit's heard time, as SYNC's Clocks "
             "(worst %.2f ms <= 2.3: a render of 50 %% in the way)", w);
    check(what, w <= 2.3);
    hw_reset();
    for (i = 0; i < 16u; i++)
        trk[0].step[i].on = 1;
    trk[1].p[P_SLEN] = 5;
    trk[1].p[P_SDIV] = DIV_FIT + 1;
    for (i = 0; i < 5u; i++)
        trk[1].step[i].on = 1;
    transport_req = 1;
    hw_run_ms(4000 - 5);                                /* 2 bars at 120, less a hair */
    a = ons(1);
    b = ons(2);
    {
        double first = 0, last = 0, mx = 0, mn = 1e9;
        uint32_t n = 0;
        for (i = 0; i < nsent; i++)
            if (is_on(sent[i].pkt) && ch_of(sent[i].pkt) == 2u) {
                if (n) {
                    double d = sent[i].t - last;
                    mx = d > mx ? d : mx;
                    mn = d < mn ? d : mn;
                } else {
                    first = sent[i].t;
                }
                last = sent[i].t;
                n++;
            }
        (void)first;
        snprintf(what, sizeof what, "polymetry: 2 bars: LEN 16 1/16 %u notes, LEN 5 FIT %u notes, evenly %.2f..%.2f ms "
                 "apart (a bar / 5 = 400 ms)", a, b, ms_of(mn), ms_of(mx));
        check(what, a == 32u && b == 10u && fabs(ms_of(mn) - 400.0) < 1.5 && fabs(ms_of(mx) - 400.0) < 1.5);
    }
    transport_req = 2;
    hw_run_ms(50);
}

/* ---- the Off ---- */
static double off_after(uint32_t ch, uint32_t nth)    /* ms from the nth On on ch to its Off */
{
    uint32_t i, n = 0, nt = 0;
    double t = 0;
    for (i = 0; i < nsent; i++) {
        uint32_t p = sent[i].pkt;
        if (ch_of(p) != ch)
            continue;
        if (is_on(p) && n++ == nth) {
            t = sent[i].t;
            nt = nt_of(p);
        } else if (is_off(p) && n == nth + 1u && nt_of(p) == nt) {
            return ms_of(sent[i].t - t);
        }
    }
    return -1;
}
static void test_gate(void)
{
    double d, e;
    char what[200];
    uint32_t i, on65 = 0, off60 = 0, ord = 0;
    hw_reset();
    trk[0].step[0].on = 1;                              /* 1/16 at 120: 125 ms */
    trk[1].step[0].on = 1;
    trk[1].step[0].rat = 1;                             /* RATCH 2: 62.5 ms each */
    trk[2].p[P_SDIV] = 0;                               /* 1/4: 500 ms */
    trk[2].step[0].on = 1;
    play_ms(900);
    d = off_after(1, 0);
    snprintf(what, sizeof what, "Off after the step: 1/16 at 120 %.2f ms (125), RATCH 2 %.2f / %.2f (62.5), 1/4 %.2f (500)",
             d, off_after(2, 0), off_after(2, 1), off_after(3, 0));
    check(what, fabs(d - 125.0) <= 1.5 && fabs(off_after(2, 0) - 62.5) <= 1.5 && fabs(off_after(2, 1) - 62.5) <= 1.5 &&
                    fabs(off_after(3, 0) - 500.0) <= 1.5 && ons(2) == 2u);

    hw_reset();
    drum_set_model(&trk[0], DM_ACID);
    trk[0].step[0].on = 1;
    trk[0].step[4].on = 1;
    trk[0].step[4].slide = 1;                           /* slides into step 5 */
    trk[0].step[5].on = 1;
    trk[0].step[5].nt = 5;
    play_ms(900);
    e = ms_of((double)acid_gate_len(&trk[0], 5512) * 24000000.0 / FS);
    d = off_after(1, 0);
    snprintf(what, sizeof what, "ACID: the Off when its gate closes (%.2f ms, GATE %.2f)", d, e);
    check(what, fabs(d - e) <= 1.5);
    for (i = 0; i < nsent; i++) {
        if (is_on(sent[i].pkt) && nt_of(sent[i].pkt) == 65u)
            on65 = i + 1u;
        if (is_off(sent[i].pkt) && nt_of(sent[i].pkt) == 60u)
            off60 = i + 1u;
    }
    ord = on65 && off60 && on65 < off60 && sent[on65 - 1u].t == sent[off60 - 1u].t;
    check("ACID slide: the new note's On, then the old one's Off (legato), together", ord && ons(1) == 3u && balanced());
}

/* ---- no hanging notes ---- */
static void test_hanging(void)
{
    uint32_t i, a, n_before, ok = 1;
    hw_reset();
    trk[0].p[P_SDIV] = 7;                               /* 1/1: a 2 s note */
    trk[0].p[P_SLEN] = 1;
    trk[0].step[0].on = 1;
    transport_req = 1;
    hw_run_ms(300);
    check("a long note sounding", ons(1) == 1u && !balanced());
    transport_req = 2;
    hw_run_ms(15);
    check("STOP: its Off at once (within 15 ms: the next block, heard a half later)", balanced());

    hw_reset();
    trk[0].p[P_SDIV] = 7;
    trk[0].p[P_SLEN] = 1;
    trk[0].step[0].on = 1;
    transport_req = 1;
    hw_run_ms(300);
    transport_req = 1;                                  /* PLAY again: from the top */
    hw_run_ms(15);
    for (i = 0, a = 0; i < nsent; i++)
        a += is_off(sent[i].pkt);
    check("PLAY while playing: the Off first, then the note again", a == 1u && ons(1) == 2u);
    transport_req = 2;
    hw_run_ms(20);
    check("..: balanced after STOP", balanced());

    hw_reset();
    for (i = 0; i < 16u; i++)
        trk[1].step[i].on = 1;
    trk[1].p[P_SDIV] = 7;
    trk[1].p[P_SLEN] = 1;
    trk[1].step[0].on = 1;
    trk[0].step[0].on = trk[0].step[8].on = 1;
    transport_req = 1;
    hw_run_ms(300);
    trk[1].p[P_MUTE] = 1;                               /* muted mid-note */
    hw_run_ms(15);
    for (i = 0, a = 0; i < nsent; i++)
        a += is_off(sent[i].pkt) && ch_of(sent[i].pkt) == 2u;
    check("a mute: its sounding note's Off at once", a == 1u);
    n_before = ons(2);
    hw_run_ms(2500);
    check("muted: no Note On from it", ons(2) == n_before && ons(1) > 2u);
    trk[1].p[P_MUTE] = 0;
    song.solo = 1u;                                     /* SOLO track 1: the others silent */
    hw_run_ms(2100);
    for (i = 0; i < nsent; i++)
        ok &= !(is_on(sent[i].pkt) && ch_of(sent[i].pkt) == 2u && sent[i].t > now_t - 2000u * 24000u);
    check("another track soloed: no Note On from this one", ok);
    song.solo = 0;
    transport_req = 2;
    hw_run_ms(20);
    check("STOP: every On has its Off", balanced());

    hw_reset();
    trk[0].p[P_SDIV] = 7;
    trk[0].p[P_SLEN] = 1;
    trk[0].step[0].on = 1;
    transport_req = 1;
    hw_run_ms(300);
    song.g[G_MOUT] = 0;                                 /* OUT OFF and the channel changed while the note sounds */
    song.g[G_MCH1] = 9;
    transport_req = 2;
    hw_run_ms(20);
    check("OUT OFF / a new channel while a note sounds: its Off still goes, on its channel", balanced() && ons(9) == 0u);
}

/* ---- an update ---- */
static void test_update(void)
{
    uint32_t n0, n1, n2, k;
    hw_reset();
    for (k = 0; k < NTRK; k++)
        trk[k].step[0].on = trk[k].step[4].on = trk[k].step[8].on = trk[k].step[12].on = 1;
    transport_req = 1;
    hw_run_ms(500);
    n0 = nsent;
    usb.ota_req = 1;
    hw_run_ms(10);
    usb.ota_req = 0;
    hw_run_ms(2890);
    n1 = nsent;
    hw_run_ms(600);
    n2 = nsent;
    check("update: nothing sent while it runs and for 3 s after, then the notes go on", n0 >= 12u && n1 == n0 &&
                                                                                           n2 - n1 >= 6u);
    transport_req = 2;
    hw_run_ms(20);
    check("update: the Offs it swallowed made good after it (All Notes Off on their channels)", balanced());
}

/* ---- a full queue ---- */
static void test_overflow(void)
{
    uint32_t k, s, h_on, h_off;
    char what[200];
    for (k = 0; k < 2u; k++) {
        hw_reset();
        for (s = 0; s < NTRK; s++) {
            uint32_t i;
            for (i = 0; i < 16u; i++)
                trk[s].step[i].on = 1;
            trk[s].p[P_AMODE] = 1;
            trk[s].p[P_ARATE] = 6;
        }
        song.g[G_MOUT] = (int16_t)k;
        no_timer5 = 1;                                  /* nobody empties the queue */
        transport_req = 1;
        hw_run_ms(2000);
        if (!k)
            h_off = nhit;
        else
            h_on = nhit;
    }
    snprintf(what, sizeof what, "overflow: the queue full (%u waiting of %u), Note Ons refused and counted (%u), the "
             "sound unchanged (%u hits, %u with OUT OFF), nothing blocked", mout.w - mout.r, MOUT_Q, mon.drops, h_on,
             h_off);
    check(what, mout.w - mout.r <= MOUT_Q && mout.w - mout.r >= MOUT_Q - MON_ROOM && mon.drops > 100u && h_on == h_off);
    no_timer5 = 0;
    hw_run_ms(20);                                      /* TIMER5 again: the backlog goes */
    transport_req = 2;
    hw_run_ms(50);
    check("overflow: after STOP no note left hanging (an Off lost: All Notes Off on its channel)", balanced());
}

/* ---- SYNC and the notes together ---- */
static void test_clock_room(void)
{
    uint32_t k, i, n = 0;
    hw_reset();
    song.g[G_SYNC] = 1;
    for (k = 0; k < NTRK; k++)
        for (i = 0; i < 16u; i++)
            trk[k].step[i].on = 1;
    transport_req = 1;
    hw_run_ms(2000);
    for (i = 0; i < nsent; i++)
        n += sent[i].pkt == 0xF80Fu;
    check("SYNC + 6 tracks hitting every 1/16 together: 24 Clocks a beat, none lost (4 beats: 96 +-1)",
          n >= 95u && n <= 97u && ons(0) >= 6u * 15u);
    transport_req = 2;
    hw_run_ms(20);
}

#if FELUCCA_BLE
static void test_ble(void)
{
    uint32_t i, n = 0;
    hw_reset();
    trk[0].step[0].on = 1;
    song.g[G_MOUT] = SY_BLE;
    play_ms(200);
    for (i = bq_r; i != bq_w; i++)
        n += is_on(bq[i % 256u]) || is_off(bq[i % 256u]);
    check("BLE: the notes to the device's queue, not to USB", n == 2u && notes_sent() == 0u);
    hw_reset();
    trk[0].step[0].on = 1;
    song.g[G_MOUT] = SY_USB | SY_BLE;
    play_ms(200);
    for (i = bq_r, n = 0; i != bq_w; i++)
        n += is_on(bq[i % 256u]) || is_off(bq[i % 256u]);
    check("BOTH: to each", n == 2u && notes_sent() == 2u);
}
#endif

/* ---- cost ---- */
static void test_cost(void)
{
    double isr[2], t5[2], per[2];
    uint32_t k, m, notes = 0;
    char what[240];
    for (m = 0; m < 2u; m++) {
        hw_reset();
        for (k = 0; k < NTRK; k++) {
            uint32_t i;
            for (i = 0; i < 16u; i++)
                trk[k].step[i].on = 1;
            trk[k].p[P_AMODE] = 1;                      /* RTRG 1/32 on every track */
            trk[k].p[P_ARATE] = 6;
            trk[k].p[P_AFADE] = 0;
        }
        song.g[G_MOUT] = (int16_t)m;
        transport_req = 1;
        hw_run_ms(200);
        isr_instr = t5_instr = 0;
        count_instr = 1;
        hw_run_ms(2000);
        count_instr = 0;
        isr[m] = (double)isr_instr / (FS * 2.0);
        t5[m] = (double)t5_instr / (FS * 2.0);
        per[m] = 0;
        if (m)
            notes = ons(0);
        transport_req = 2;
        hw_run_ms(20);
    }
    if (!isr[0]) {
        printf("     cost: no instruction counter on this host (perf_event): not measured\n");
        return;
    }
    (void)per;
    snprintf(what, sizeof what, "cost (host instructions / sample), 6 tracks RTRG 1/32 (%u notes in 2 s): render OUT OFF "
             "%.1f, OUT USB %.1f (+%.2f, +%.3f %%); TIMER5 %.2f -> %.2f", notes, isr[0], isr[1], isr[1] - isr[0],
             100.0 * (isr[1] - isr[0]) / isr[0], t5[0], t5[1]);
    check(what, isr[1] - isr[0] <= 0.01 * isr[0] + 2.0);
}

int main(void)
{
    setvbuf(stdout, 0, _IONBF, 0);
    test_channels_notes();
    test_arp();
    test_timing();
    test_gate();
    test_hanging();
    test_update();
    test_overflow();
    test_clock_room();
#if FELUCCA_BLE
    test_ble();
#endif
    test_cost();
    printf("%s\n", fails ? "MIDI OUT TESTS FAILED" : "midiout_seq_test: all passed");
    return fails ? 1 : 0;
}
