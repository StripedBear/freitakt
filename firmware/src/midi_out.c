/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* MIDI out beyond the keys' notes: GLO SYNC (MIDI clock out), GLOBAL 3/8 THRU, GLO MIDI OUT (the sequencer's
 * notes) and the MIDI KEYBOARD (KEYS' notes as Note On / Off: mkb_*). Included from seq.c.
 *
 * SYNC (song.g[G_SYNC], a project global: SY_ bits, OFF 0, USB 1, and in the Bluetooth build BLE 2, BOTH 3; the FM-1's
 * TRS jack is MIDI IN only, so there is no TRS out). On the own clock (CLK INT): Start on PLAY (PLAY while playing:
 * Stop, Start; Hortator has no resume, so never Continue), 24 Clocks a beat while playing, Stop on STOP. The Clocks
 * come from the sequencer's time: the global 1/16 clock (seq.c gclk), whose 1/8s (an even 1/16 and the odd one after it) never
 * swing: 12 Clocks evenly over each 1/8, the 1/8's first on its start, so they stay on the steps' grid through tempo
 * changes and do not swing. Following a clock (CLK USB / TRS / BLE): that input's Clock, Start, Continue and Stop are
 * forwarded as they are read (mout_scan, on receipt: nothing regenerated) to the SYNC outputs, never back to the port
 * they came in on.
 * THRU (midi_thru, the device setting): the channel messages of each input to every other output, never back.
 *
 * Timing. The audio ISR renders a half (256 samples) at a time; a Clock's sample in it gives its due time in TIMER4
 * ticks: the half's IRQ + one half (it is heard then) + the sample (mout_push). The USB driver (usb_app.c, frozen) sends
 * from its poll, main.c's TIMER5 handler every 5th tick (0.5 ms), never during a render. mout_tick runs on every
 * TIMER5 tick before that handler (felucca.c) and mirrors its poll counter: at a poll, the Clocks due nearer to this
 * poll than to the next send are put into the USB ring (the next send: the next poll, or after the coming render when
 * it falls inside one), so a Clock leaves within a quarter of a millisecond of its time, half a render near one.
 * Contexts: the audio ISR fills the schedule (mout.due / mout.st); TIMER5 outside the render (in_audio 0) empties it,
 * scans the input ring and writes the USB out ring. The audio ISR never preempts TIMER5 and TIMER5 does none of this
 * while nested in the render, so the USB out ring (whose other writer is the audio ISR's keys) has one writer at a time.
 * Bluetooth: the same packets go to bt_midi.c's out queue (bt_sync_out, the keys' notes' queue too), which the main
 * loop hands to the BLE unit: one clock for USB and Bluetooth, no second Bluetooth clock sender. MODE FM-LINK, SMC
 * DAW or no link: nothing to Bluetooth.
 * An update (SysEx on USB, ota.c) pauses all of it: nothing is sent while its SysEx moves and for 3 s after. The web
 * editor's frames (F0 7D ..., editor.c) do not pause it (web/EDITOR_PROTOCOL.md rule 8): a frame whose first data byte
 * is 7D, and the out ring while it holds only the editor's replies (mout.ed_end / ed_tx), are not update traffic.
 *
 * MIDI OUT (G_MOUT, a project global: SY_ bits as SYNC's, USB by default; G_MCH1.. each track's channel, OFF or 1..16,
 * track n on channel n by default): the sequencer's hits (seq_tick: the steps, RATCH, MICRO, ARP / RTRG's repeats; not
 * REPEAT STEP, the keys (keyboard_block sends theirs), MIDI in) as Note On / Off, scheduled in the Clocks' queue from
 * the block the voice starts in (drum_play's mon_hit: mon_seq), timed as a Clock is. The note: a pitched model's
 * (arp_model: ACID SAMPL FM SKIN, the hit's machine) the hit's own (60 + its offset from C4: the step's NOTE, ARP's
 * chord and octaves); a drum's the track's NOTE (P_NOTE, the kit's GM note). The velocity: the hit's (ACC 127, else 96,
 * RTRG / ARP's FADE). One note a track at a time (a new hit ends the sounding one first; an ACID slide sends the new
 * note before the old one's Off: legato). Its Off: a gated model's (ACID) when its gate closes; any other's after
 * its length (a roll's hit: GATE of its repeat; else the step, swung, / RATCH), counted in blocks (mon_post). STOP,
 * PLAY again, the track muted (or another soloed): the Off at once. The Off goes to the channel and ports its On went
 * to. A Note On needs MON_ROOM free places in the queue (the Offs and the Clocks keep theirs); a full queue drops and
 * counts (mon.drops), never waits; a lost Off: STOP adds All Notes Off (CC 123) on the channels played. An update's
 * pause swallows the queue: All Notes Off on the channels it swallowed notes of, once the pause is over. */
#define MOUT_Q 64u                                  /* the scheduled Start / Clock / Stop and MIDI OUT's notes */
#define MON_ROOM 8u                                 /* a Note On only while this many places are free */
#define MOUT_TPS_Q16 35665605u                      /* TIMER4 ticks a sample (24 MHz / 44.1 kHz), Q16 */
#define MOUT_HALF_TICKS ((uint32_t)(((uint64_t)HALF_FRAMES * MOUT_TPS_Q16) >> 16))
#define MOUT_TICK 2400u                             /* TIMER5: 100 us */
#define MOUT_PAUSE_MS 3000u                         /* after an update's last SysEx */
#define MOUT_CLOCK_ROOM 8u                          /* a Clock only while the USB ring holds fewer packets: a computer not
                                                     * reading gets no backlog of stale Clocks, the keys' notes keep room */
#if FELUCCA_BLE
#define SY_ALL (SY_USB | SY_BLE)
static int bt_sync_out(uint32_t pkt, int rt);       /* bt_midi.c: to the BLE device's out queue (MIDI mode, linked; not
                                                     * SMC DAW nor FM-LINK), the one Bluetooth MIDI sender */
#else
#define SY_ALL SY_USB
#endif

static volatile uint8_t midi_thru;                  /* GLOBAL 3/7 THRU (settings: project.c, the palette word's bit 14) */
static struct {
    uint32_t due[MOUT_Q];                           /* TIMER4 tick */
    uint32_t pk[MOUT_Q];                            /* the USB-MIDI packet: a Clock's 0F F8 / FA / FC, MIDI OUT's */
    uint8_t po[MOUT_Q];                             /* its ports (SY_ bits); 0: a Clock's (SYNC's when it leaves) */
    volatile uint32_t w, r;                         /* the audio ISR writes w, TIMER5 r */
    uint32_t t0, blk, r_ticks;                      /* the half's IRQ, its samples before this block, the last render */
    uint32_t pos, pair, pulses;                     /* samples into the 1/8 (gclk.cnt / 2: pair), Clocks counted */
    uint8_t j, run;                                 /* the 1/8's next Clock (0..12); the own clock's transport running */
    uint8_t sub, owed;                              /* main.c fm1_timer5_irq's sub and owed (USB), mirrored */
    uint8_t sx_seen;
    uint32_t fw, sx_ms;                             /* the input ring read up to here; the last SysEx seen */
    uint32_t sent, drops, fwd;                      /* console: packets out, refused, forwarded */
    volatile uint32_t ed_end;                       /* editor.c: the SysEx out ring's write index after its last frame */
    volatile uint8_t ed_tx;                         /* editor.c: writing a frame into that ring now */
    uint16_t ano_ch;                                /* MIDI OUT: an update swallowed notes on these channels .. */
    uint8_t ano_po;                                 /* .. to these ports: All Notes Off there once it is over */
} mout;
/* MIDI OUT's notes (the audio ISR only) */
static uint8_t mon_seq;                             /* seq_tick running: drum_play's hits are the sequencer's */
static struct {
    uint8_t nt[NTRK], ch[NTRK], po[NTRK];           /* the sounding note, its channel 0..15, its ports (0: none) */
    uint8_t gate[NTRK];                             /* .. a gated model's: its Off when the gate closes */
    uint32_t left[NTRK];                            /* .. else the samples to its Off */
    uint32_t on;                                    /* bit per track: a note sounding */
    uint32_t used;                                  /* bit per channel: notes since PLAY (All Notes Off when lost) */
    uint32_t notes, drops, lost;                    /* console: Note Ons queued, refused (no room), Offs lost */
    uint8_t lostf;                                  /* an Off lost since PLAY */
} mon;

static uint32_t mout_port(uint32_t src)            /* the output an input's port is (midi_in_source) */
{
    return src == 1u ? SY_USB : src == 3u ? (uint32_t)SY_ALL & 2u : 0u;
}

/* one packet to ports (SY_ bits); a Clock (rt) only where the USB ring has room for it */
static void mout_send(uint32_t pkt, uint32_t ports, int rt)
{
    if (ports & SY_USB) {
        if (rt ? mo_w - mo_r < MOUT_CLOCK_ROOM : mo_w - mo_r < MQ)
            midi_out_event(pkt);                    /* (usb_app.c: dropped while no host has configured us) */
        else
            mout.drops++;
    }
#if FELUCCA_BLE
    if (ports & SY_BLE)
        (void)bt_sync_out(pkt, rt);                 /* (its own room rule for a Clock, as the USB ring's) */
#endif
    mout.sent++;
}

/* ---- the audio ISR ---- */
static void mout_half(uint32_t t0)                  /* audio.c: a half's render begins (its IRQ's tick) */
{
    mout.t0 = t0;
    mout.blk = 0;
}
static void mout_half_done(uint32_t t1)             /* .. and ends */
{
    mout.r_ticks = t1 - mout.t0;
}
static int mout_put(uint32_t pkt, uint32_t po, uint32_t off)   /* pkt to ports po (0: a Clock) at sample off */
{
    uint32_t w = mout.w;
    if (w - mout.r >= MOUT_Q) {
        mout.drops++;
        return 0;
    }
    mout.due[w % MOUT_Q] = mout.t0 + (uint32_t)(((uint64_t)(HALF_FRAMES + mout.blk + off) * MOUT_TPS_Q16) >> 16);
    mout.pk[w % MOUT_Q] = pkt;
    mout.po[w % MOUT_Q] = (uint8_t)po;
    RING_PUBLISH();
    mout.w = w + 1u;
    return 1;
}
static void mout_push(uint32_t st, uint32_t off)    /* the Clock's st at sample off of this block */
{
    (void)mout_put(0x0Fu | st << 8, 0, off);
}

/* ---- MIDI OUT: the sequencer's notes (the audio ISR) ---- */
static void mon_off(uint32_t k)                     /* track k's sounding note ends (now: this block's start) */
{
    if (!((mon.on >> k) & 1u))
        return;
    mon.on &= ~(1u << k);
    if (!mout_put(0x08u | (0x80u | mon.ch[k]) << 8 | (uint32_t)mon.nt[k] << 16 | 0x40u << 24, mon.po[k], 0))
        mon.lost++, mon.lostf = 1;
}
static void mon_all_off(void)                       /* STOP, PLAY: every Off; a lost one: All Notes Off */
{
    uint32_t k, c;
    for (k = 0; k < NTRK; k++)
        mon_off(k);
    if (mon.lostf && mon.used) {
        uint32_t po = (uint32_t)song.g[G_MOUT] & SY_ALL;
        for (c = 0; c < 16u && po; c++)
            if ((mon.used >> c) & 1u)
                (void)mout_put(0x0Bu | (0xB0u | c) << 8 | 123u << 16, po, 0);
    }
    mon.used = 0;
    mon.lostf = 0;
}
/* drum_play: track t's hit at velocity vel, seq_tick's (mon_seq); legato: an ACID slide (the old note's Off after) */
static __attribute__((noinline)) void mon_note(track_t *t, uint32_t vel, int legato)
{
    uint32_t k = trk_index(t), po = (uint32_t)song.g[G_MOUT] & SY_ALL, ch = (uint32_t)song.g[G_MCH1 + k], nt, len;
    uint32_t was = (mon.on >> k) & 1u, ont = mon.nt[k];
    if (!po || ch - 1u > 15u || trk_silent(t)) {
        mon_off(k);
        return;
    }
    nt = arp_model(voice_model(t)) ? (uint32_t)clamp(60 + t->nt, 0, 127) : (uint32_t)t->p[P_NOTE] & 127u;
    if (legato && was && ont == nt && mon.ch[k] == ch - 1u)
        return;                                     /* (slid into the same note: it goes on) */
    if (!legato)
        mon_off(k);                                 /* one note a track: the sounding one ends first */
    if (mout.w - mout.r > MOUT_Q - MON_ROOM) {      /* (the Offs and the Clocks keep their room) */
        mon.drops++;
        if (legato)
            mon_off(k);
        return;
    }
    vel = vel < 1u ? 1u : vel > 127u ? 127u : vel;
    (void)mout_put(0x09u | (0x90u | (ch - 1u)) << 8 | nt << 16 | vel << 24, po, 0);
    mon.notes++;
    if (legato && was && (ont != nt || mon.ch[k] != ch - 1u))
        (void)mout_put(0x08u | (0x80u | mon.ch[k]) << 8 | ont << 16 | 0x40u << 24, mon.po[k], 0);
    mon.on |= 1u << k;
    mon.used |= 1u << (ch - 1u);
    mon.nt[k] = (uint8_t)nt;
    mon.ch[k] = (uint8_t)(ch - 1u);
    mon.po[k] = (uint8_t)po;
    mon.gate[k] = (uint8_t)(trk_model(t)->gate != 0);
    if (!mon.gate[k]) {
        if (ar_glen) {
            len = ar_glen;                          /* a roll's hit: GATE of its repeat */
        } else {
            uint32_t r = t->step[t->seq_idx].rat;
            len = step_samples(t, trk_period(t, t->seq_rem), t->seq_cnt) / (r < 3u ? r + 1u : 4u);
        }
        mon.left[k] = len > 2u * CTL ? len : 2u * CTL;
    }
}
static void mon_hit(track_t *t, uint32_t vel)       /* drum_core.c drum_play (the hit plays: not muted) */
{
    if (mon_seq)
        mon_note(t, vel, 0);
}
/* events_block, after the steps (n samples): the Offs due in this block, a gated model's when its gate closed, a muted
 * track's */
static __attribute__((noinline)) void mon_post(uint32_t n)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++) {
        const track_t *t = &trk[k];
        if (!((mon.on >> k) & 1u))
            continue;
        if (trk_silent(t) || (mon.gate[k] ? !t->gate_n : mon.left[k] <= n))
            mon_off(k);
        else if (!mon.gate[k])
            mon.left[k] -= n;
    }
}
/* ---- the MIDI KEYBOARD (the audio ISR: seq.c keyboard_block) ----
 * KEYS with KBD not OFF (MIDI OUT 4/8; mkb_kbd, mkb_local_off: seq.c): each of the 27 keys sends its own Note On /
 * Note Off (several keys at once: several notes), velocity 100 (the keys are switches, as the keys' hits), F3 = note
 * 53 + 12 x the octave (OCT- / OCT+, -4..+4: notes 5..127), on KBD's channel (TRK: the selected track's MIDI OUT
 * channel, nothing while it is OFF; or 1..16), to every output as the keys' hits go (USB, and the Bluetooth device
 * when linked in MIDI mode: SY_ALL, not OUT's setting), through MIDI OUT's queue: each leaves when it is heard, nothing
 * during an update (All Notes Off after it: mout_tick), a Note On only with MON_ROOM free (the Offs keep their room).
 * A key's Off is the note and channel its On went out with (the octave, the track or KBD changed while it was held).
 * The keys not the MIDI KEYBOARD's any more (KEYS off, a page whose keys are not KEYS', the menu, KBD OFF) or the
 * Bluetooth link dropped (bt_link_downs): every held key's Off at once; that key then sends nothing until pressed
 * again (its release not even the pad's Off). An Off lost to a full queue: All Notes Off on its channel once no key is held */
static struct {
    uint32_t held;                                  /* bit per key: its Note On went out, its Off not yet */
    uint32_t gone;                                  /* .. its Off sent before its release (the mode left, the link
                                                     * dropped): that release sends nothing */
    uint8_t nt[27], ch[27];                         /* .. that note and channel 0..15 */
    uint16_t lost;                                  /* channels an Off was lost on */
    uint8_t downs;                                  /* bt_link_downs seen */
    uint32_t notes, drops;                          /* console: Note Ons queued, refused (no room) */
} mkb;
static uint32_t mkb_chan(void)                      /* the channel 1..16 the keys send on now; 0: none */
{
    uint32_t k = mkb_kbd;
    if (k == 1u)
        k = (uint32_t)song.g[G_MCH1 + song.sel % NTRK] + 1u;   /* TRK: the selected track's (0: OFF) */
    return k >= 2u && k <= 17u ? k - 1u : 0u;
}
static void mkb_off(uint32_t key)
{
    if (!((mkb.held >> key) & 1u))
        return;
    mkb.held &= ~(1u << key);
    if (!mout_put(0x08u | (0x80u | mkb.ch[key]) << 8 | (uint32_t)mkb.nt[key] << 16 | 0x40u << 24, SY_ALL, 0))
        mkb.lost |= (uint16_t)(1u << mkb.ch[key]);
}
/* keyboard_block: this block's key edges not taken by a combo or PERFORM (press, rel); on: the keys are the MIDI
 * KEYBOARD's now. Returns the releases it took (their Offs sent: no pad note-off for them) */
static __attribute__((noinline)) uint32_t mkb_keys(uint32_t press, uint32_t rel, int on)
{
    uint32_t took = rel & (mkb.held | mkb.gone), k, c;
#if FELUCCA_BLE
    if ((uint8_t)bt_link_downs() != mkb.downs) {   /* (a link dropped: what was held is ended) */
        mkb.downs = (uint8_t)bt_link_downs();
        on = 0;
        press = 0;
    }
#endif
    mkb.gone &= ~(rel | press);
    if (!mkb.held && !mkb.lost && !(on && press))
        return took;
    if (!on)
        mkb.gone |= mkb.held & ~rel;
    for (k = 0; k < 27u && mkb.held; k++)
        if (!on || ((took >> k) & 1u))
            mkb_off(k);
    c = on ? mkb_chan() : 0u;
    for (k = 0; c && press && k < 27u; k++) {
        int32_t nt;
        if (!((press >> k) & 1u))
            continue;
        mkb_off(k);                                 /* (a release this block did not see: its Off first) */
        if (mout.w - mout.r > MOUT_Q - MON_ROOM) {
            mkb.drops++;
            continue;
        }
        nt = clamp(53 + (int32_t)k + 12 * song.octave, 0, 127);
        (void)mout_put(0x09u | (0x8Fu + c) << 8 | (uint32_t)nt << 16 | 100u << 24, SY_ALL, 0);
        mkb.held |= 1u << k;
        mkb.nt[k] = (uint8_t)nt;
        mkb.ch[k] = (uint8_t)(c - 1u);
        mkb.notes++;
    }
    for (c = 0; mkb.lost && !mkb.held && c < 16u; c++)
        if (((mkb.lost >> c) & 1u) && mout_put(0x0Bu | (0xB0u | c) << 8 | 123u << 16, SY_ALL, 0))
            mkb.lost &= (uint16_t)~(1u << c);
    return took;
}
static void mout_clock(uint32_t off)
{
    mout.pulses++;
    if (song.g[G_SYNC])
        mout_push(0xF8u, off);
}
/* the Clocks of this block (adv samples, after gclk_tick): the 1/8's j-th at j / 24 of a beat into it */
static void mout_pulses(uint32_t adv)
{
    uint32_t beat = beat_samples(), pair = gclk.cnt >> 1, base = 0, p, x;
    if (pair != mout.pair) {                        /* a 1/8 began in this block, gclk.pos samples before its end: the
                                                     * last 1/8's Clocks not sent yet first (a tempo change) */
        uint32_t e = adv > gclk.pos ? adv - gclk.pos : 0u;
        for (; mout.j < 12u; mout.j++) {
            x = mout.j * beat / 24u;
            x = x > mout.pos ? x - mout.pos : 0u;
            mout_clock(x < e ? x : e);
        }
        mout.pair = pair;
        mout.j = 0;
        mout.pos = 0;
        base = e;
        p = gclk.pos;
    } else {
        p = mout.pos + adv;
    }
    for (; mout.j < 12u; mout.j++) {
        x = mout.j * beat / 24u;
        if (x >= p)
            break;
        mout_clock(base + (x > mout.pos ? x - mout.pos : 0u));
    }
    mout.pos = p;
}
/* events_block, every block (n samples); run: the steps moved (seq_n = n on the own clock) */
static __attribute__((noinline)) void mout_block(uint32_t n, uint32_t run)
{
    if (run && mout.run && song.playing && !song.g[G_CLOCK])
        mout_pulses(n);
    mout.blk += n;
}
static __attribute__((noinline)) void mout_start(void)   /* seq_start (block start) */
{
    mon_all_off();                                  /* (PLAY while playing: the sounding notes end) */
    if (song.g[G_CLOCK]) {                          /* following: the source's Start goes through as it is */
        mout.run = 0;
        return;
    }
    if (song.g[G_SYNC]) {
        if (mout.run)
            mout_push(0xFCu, 0);                    /* PLAY while playing: from the top */
        mout_push(0xFAu, 0);
    }
    mout.run = 1;
    mout.pair = 0xFFFFFFFFu;                        /* the first 1/8 begins with the first block */
    mout.j = 12;
    mout.pos = 0;
}
static __attribute__((noinline)) void mout_stop(void)   /* seq_stop */
{
    if (mout.run && song.g[G_SYNC])
        mout_push(0xFCu, 0);
    mout.run = 0;
    mon_all_off();                                  /* MIDI OUT: no note left hanging */
}

/* ---- TIMER5, outside the render ---- */
static int mout_paused(void)                        /* an update: its SysEx moving, or within 3 s of it */
{
#if FELUCCA_OTA
    /* update traffic: the upgrade command; a frame (arriving, or waiting for ota_take) whose first data byte is not
     * the editor's 7D (not known yet: as an update's); the out ring holding anything but the editor's replies */
    if (usb.ota_req || (usb.sx_on && (usb.sx_len < 2u || usb.sysex[1] != 0x7Du)) ||
        (sx_collect && (!sx_pos || sx_frame[0] != 0x7Du)) || (sx_ready && sx_frame[0] != 0x7Du) ||
        (sx_busy && !mout.ed_tx) || (so_w != so_r && so_w != mout.ed_end && !mout.ed_tx)) {
        mout.sx_ms = fm1_ms;
        mout.sx_seen = 1;
        return 1;
    }
    return mout.sx_seen && fm1_ms - mout.sx_ms < MOUT_PAUSE_MS;
#else
    return 0;
#endif
}
/* the input ring's new packets (USB, TRS, BLE: usb_app.c, midi_uart.c, bt_midi.c fill it), as they are read: the
 * followed source's realtime to the SYNC outputs, THRU's channel messages to the other outputs. Never back to their
 * port. (The ring's writers run in TIMER5 or with the IRQs off: none runs during this.) */
static void mout_scan(void)
{
    uint32_t sync = (uint32_t)song.g[G_SYNC] & SY_ALL, clk = (uint32_t)song.g[G_CLOCK];
    while (mout.fw != mi_w) {
        uint32_t at, pkt, src;
        if (mi_w - mout.fw > MQ)
            mout.fw = mi_w - MQ;                    /* (overwritten: lost) */
        at = mout.fw % MQ;
        pkt = midi_in_q[at];
        src = midi_in_source[at] ? midi_in_source[at] : 1u;
        mout.fw++;
        if (((pkt >> 8) & 0xFFu) >= 0xF8u) {
            if (clk && clk == src && sync) {
                mout_send(pkt, sync & ~mout_port(src), 1);
                mout.fwd++;
            }
        } else if (midi_thru) {
            mout_send(pkt, SY_ALL & ~mout_port(src), 0);
            mout.fwd++;
        }
    }
}
/* the Clocks due nearer to this USB poll than to the next send (the next poll, or the first after the coming render) */
static void mout_release(uint32_t now)
{
    uint32_t q = now + (1u + (5u - mout.sub % 5u) % 5u) * MOUT_TICK, t_next = mout.t0 + MOUT_HALF_TICKS, lim;
    uint32_t sync = (uint32_t)song.g[G_SYNC] & SY_ALL, e, i;
    if ((int32_t)(t_next - now) > 0 && (int32_t)(q - t_next) >= 0)
        q = t_next + mout.r_ticks + MOUT_TICK;
    lim = now + (q - now) / 2u;
    for (e = mout.r; e != mout.w && (int32_t)(mout.due[e % MOUT_Q] - lim) <= 0; e++)
        if (!mout.po[e % MOUT_Q] && sync)           /* the Clocks first: their room rule before this poll's notes */
            mout_send(mout.pk[e % MOUT_Q], sync, 1);
    for (; mout.r != e; mout.r++) {
        i = mout.r % MOUT_Q;
        if (mout.po[i])
            mout_send(mout.pk[i], mout.po[i], 0);   /* MIDI OUT's notes */
        RING_PUBLISH();
    }
}
/* every TIMER5 tick, before main.c's handler (felucca.c); in_audio: nested in the render */
static void mout_tick(uint32_t now, uint32_t in_audio)
{
    uint32_t poll;
    if (mout.sub % 5u == 0u)                        /* as main.c: USB every 5th tick, owed while the render runs */
        mout.owed = 1;
    if (++mout.sub == 10u)
        mout.sub = 0;
    if (in_audio)
        return;
    poll = mout.owed;
    mout.owed = 0;
    if (mout_paused()) {
        for (; mout.r != mout.w; mout.r++) {        /* (MIDI OUT's notes swallowed: All Notes Off after it) */
            uint32_t i = mout.r % MOUT_Q;
            if (mout.po[i]) {
                mout.ano_ch |= 1u << ((mout.pk[i] >> 8) & 15u);
                mout.ano_po |= mout.po[i];
            }
        }
        mout.fw = mi_w;
        return;
    }
    if (mout.ano_ch) {
        uint32_t c;
        for (c = 0; c < 16u; c++)
            if ((mout.ano_ch >> c) & 1u)
                mout_send(0x0Bu | (0xB0u | c) << 8 | 123u << 16, mout.ano_po, 0);
        mout.ano_ch = 0;
        mout.ano_po = 0;
    }
    mout_scan();
    if (poll && mout.r != mout.w)
        mout_release(now);
}
