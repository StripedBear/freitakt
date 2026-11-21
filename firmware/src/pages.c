/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Drum machine fork: 2026 DEADACTIVE */
/* Drum UI pages: a family per page button (pressing it again steps through its pages), and what
 * the four knobs edit on each page. */
enum { FAM_HOME, FAM_SND, FAM_FX, FAM_SEQ, FAM_GLO, FAM_SAVE, FAM_LFO, FAM_MIX, FAM_EDT, FAM_COUNT };
/* FAM_EDT: no button, never shown on the FM-1: the web editor's own pages (editor.c ed_pages: GLOBAL, SYSTEM) */
enum { SC_TRACK, SC_GLOBAL, SC_GRID, SC_MIX };   /* knobs edit: the selected track, song.g, the STEP grid, the mixer */
enum { GR_NONE, GR_MODEL, GR_FX, GR_GRID, GR_STEPS, GR_SLOTS, GR_MIX, GR_COMP, GR_LFO, GR_MOTION, GR_FILTER,
       GR_LDEST, GR_INSERT, GR_DLY, GR_REV, GR_CHO };

#define SND_SLOT 0xC0u                            /* EDIT page ids: SND_SLOT + n = the track's n-th sound knob */
#define SND_N 20u                                 /* .. at most: 5 SOUND pages (the fifth the SAMPLE model's only) */
#define PG_MLEN 0xFEu                             /* PATTERN's MLEN: MASTER LENGTH (song.mlen, the slot's; not a track's) */

typedef struct {
    const char *title;
    uint8_t fam, scope, graph;
    uint8_t id[4];               /* parameter ids; 0xFF = empty column */
} page_t;

static const page_t PAGES[] = {
    /* EDIT: the selected track's sound list (snd_list), 4 a page; a page past its end is not shown */
    {"SOUND", FAM_SND, SC_TRACK, GR_MODEL, {SND_SLOT + 0, SND_SLOT + 1, SND_SLOT + 2, SND_SLOT + 3}},
    {"SOUND", FAM_SND, SC_TRACK, GR_MODEL, {SND_SLOT + 4, SND_SLOT + 5, SND_SLOT + 6, SND_SLOT + 7}},
    {"SOUND", FAM_SND, SC_TRACK, GR_MODEL, {SND_SLOT + 8, SND_SLOT + 9, SND_SLOT + 10, SND_SLOT + 11}},
    {"SOUND", FAM_SND, SC_TRACK, GR_MODEL, {SND_SLOT + 12, SND_SLOT + 13, SND_SLOT + 14, SND_SLOT + 15}},
    {"SOUND", FAM_SND, SC_TRACK, GR_MODEL, {SND_SLOT + 16, SND_SLOT + 17, SND_SLOT + 18, SND_SLOT + 19}},   /* SAMPLE:
                                                                                * ATK SUS REL LOOP */
    /* ARP / RTRG (seq.c roll_*): after the SOUND pages in EDIT's cycle (ARP + EDIT: here at once, ui_input.c); the
     * second ARP's only (hidden on a drum: page_hidden) */
    {"ARP", FAM_SND, SC_TRACK, GR_NONE, {P_AMODE, P_ARATE, P_AGATE, P_AFADE}},
    {"ARP", FAM_SND, SC_TRACK, GR_NONE, {P_ADIR, P_ACHORD, P_ARANGE, 0xFF}},
    {"FILTER", FAM_FX, SC_TRACK, GR_FILTER, {P_FTYPE, P_FCUT, P_FRESO, P_FENV}},   /* filter.c */
    {"FILTER", FAM_FX, SC_TRACK, GR_FILTER, {P_FDEC, 0xFF, 0xFF, 0xFF}},
    {"FX", FAM_FX, SC_TRACK, GR_FX, {P_DIST, P_CHOR, P_DLY, P_REV}},
    {"INSERT", FAM_FX, SC_TRACK, GR_INSERT, {P_ITYPE, P_IA, P_IB, P_IC}},   /* insert.c: after DIST; A B C by the TYPE */
    {"INSERT", FAM_FX, SC_TRACK, GR_INSERT, {P_IMIX, P_SRR, 0xFF, 0xFF}},   /* its dry / wet; SRR (before DIST) */
    {"COMP", FAM_FX, SC_GLOBAL, GR_COMP, {G_CSRC, G_CTHR, G_CRAT, G_CREL}},   /* FX after INSERT; keys: DUCK per track */
    {"COMP", FAM_FX, SC_GLOBAL, GR_COMP, {G_CATK, G_CKNEE, G_CMKUP, G_CGHOST}},
    /* SEQ: PATTERN alone (FREITAKT redesign: the steps are the keys' and the LEDs'; no STEP grid, no MOTION page) */
    {"PATTERN", FAM_SEQ, SC_TRACK, GR_STEPS, {P_SLEN, P_SDIV, P_SSWING, PG_MLEN}},
    /* the web editor's (FAM_EDT): BPM and SWING are HOME's knobs, the rest MENU's rows (ui_menu.c) */
    {"GLOBAL", FAM_EDT, SC_GLOBAL, GR_NONE, {G_BPM, G_SWING, G_CLOCK, G_DRCH}},
    {"SYSTEM", FAM_EDT, SC_GLOBAL, GR_NONE, {G_MIDI, G_SYNC, G_ROUTE, G_INFO}},
    {"MIDI OUT", FAM_GLO, SC_GLOBAL, GR_NONE, {G_MOUT, G_MCH1, G_KBD, G_LOCAL}},   /* the sequencer's notes (midi_out.c
                                                                       * mon_*): OUT, the selected track's channel; the
                                                                       * MIDI KEYBOARD (mkb_*): KBD, LOCL */
    {"DLY", FAM_GLO, SC_GLOBAL, GR_DLY, {G_DTIME, G_DFDBK, G_DCOLOR, G_DMIX}},   /* the send effects' buses */
    {"DLY", FAM_GLO, SC_GLOBAL, GR_DLY, {G_DMODE, 0xFF, 0xFF, 0xFF}},   /* MODE: NORMAL / PING-PONG */
    {"REVERB", FAM_GLO, SC_GLOBAL, GR_REV, {G_RTYPE, G_RSIZE, G_RDAMP, 0xFF}},   /* TYPE: ROOM / SPRING / HALL */
    {"CHORUS", FAM_GLO, SC_GLOBAL, GR_CHO, {G_CRATE, G_CDEPTH, 0xFF, 0xFF}},   /* GLO 5/5: RATE DEPTH */
    {"PROJECT", FAM_SAVE, SC_GLOBAL, GR_SLOTS, {G_SLOT, G_NAME, G_LOAD, G_SAVE}},   /* NAME: rename (ui_name.c) */
    {"TOOLS", FAM_SAVE, SC_GLOBAL, GR_NONE, {G_CLRSEQ, G_INITSND, G_CLRALL, G_INITALL}},   /* track; all */
    {"COPY", FAM_SAVE, SC_GLOBAL, GR_SLOTS, {G_SLOT, G_COPY, G_PASTE, G_LEARN}},   /* a slot's COPY / PASTE (project.c);
                                                                                 * KNOB 4: MIDI LEARN on / off */
    /* LFO: one LFO on the FM-1 (FREITAKT redesign; LFO 2 stays in the record, off unless a DEST slot names it): its
     * wave (the schematic over page 1), then TRIG MODE; then DEST, the track's 4 slots: KNOB 1 the slot
     * (song.lslot), 2-4 its SRC DEST DEPTH (page_id; lfo.c LS_ID) */
    {"LFO", FAM_LFO, SC_TRACK, GR_LFO, {P_LFO1 + LF_WAVE, P_LFO1 + LF_RATE, P_LFO1 + LF_MORPH, P_LFO1 + LF_PHASE}},
    {"LFO", FAM_LFO, SC_TRACK, GR_NONE, {P_LFO1 + LF_TRIG, P_LFO1 + LF_MODE, 0xFF, 0xFF}},
    {"DEST", FAM_LFO, SC_TRACK, GR_LDEST, {0xFF, 0xFF, 0xFF, 0xFF}},
    /* HOME's TRACKS mode (spec §3: a HOME tap on HOME; no button of its own): TRACK LEVEL LEN PAN */
    {"TRACKS", FAM_MIX, SC_MIX, GR_MIX, {0xFF, 0xFF, 0xFF, 0xFF}},
};
#define NPAGES (sizeof(PAGES) / sizeof(PAGES[0]))

/* the button (and LED) of each family; TRACKS (HOME's TRACKS mode) HOME's; ENV and SCL are free; the editor's pages
 * none (0xFF: no button's) */
static const uint8_t FAM_BTN[FAM_COUNT] = {B_HOME, B_EDIT, B_FX, B_SEQ, B_GLO, B_SAVE, B_LFO, B_HOME, 0xFFu};

/* the EDIT list of track t: MODEL, its model's TUNE DECAY TONE and 4th knob, its extra knobs, then LVL PAN NOTE
 * CHOKE PRIO (at most 16: 14); the SAMPLE model's then PLAY CUT, and ATK SUS REL LOOP on a page of their own (the 5th:
 * 0xFF, an empty column, up to it); returns how many (SND_N at most) */
static uint32_t snd_list(const track_t *t, uint8_t *ids)
{
    const dmodel_t *m = &DMODELS[(uint32_t)t->p[P_MODEL] % NMODELS];
    uint32_t n = 0, k;
    ids[n++] = P_MODEL;
    for (k = 0; k < 8u; k++)
        if (k < 4u || (m->edit[k].label && m->edit[k].label[0] != '-'))
            ids[n++] = (uint8_t)(P_E0 + k);
    ids[n++] = P_LEVEL;
    ids[n++] = P_PAN;
    ids[n++] = P_NOTE;
    ids[n++] = P_CHOKE;
    ids[n++] = P_PRIO;
    if ((uint32_t)t->p[P_MODEL] % NMODELS == DM_SMPL) {
        ids[n++] = P_SPLAY;                         /* (SOUND 4/5: CHOKE PRIO PLAY CUT) */
        ids[n++] = P_SCUT;
        while (n < 16u)
            ids[n++] = 0xFFu;
        for (k = 0; k < 4u; k++)
            ids[n++] = (uint8_t)(P_SATK + k);
    }
    return n;
}

/* the parameter of a column: an EDIT slot is the selected track's sound list; LFO DEST: KNOB 2-4 the selected slot's SRC DEST DEPTH (KNOB 1 the slot:
 * no parameter); 0xFF = empty */
static uint32_t page_id(const page_t *pg, uint32_t slot)
{
    uint32_t id = pg->id[slot & 3u];
    if (id >= SND_SLOT && id < SND_SLOT + SND_N) {
        uint8_t ids[SND_N];
        uint32_t n = snd_list(TSEL, ids);
        return id - SND_SLOT < n ? ids[id - SND_SLOT] : 0xFFu;
    }
    if (pg->graph == GR_LDEST)
        return slot & 3u ? LS_ID[song.lslot & 3u][(slot & 3u) - 1u] : 0xFFu;
    if (id == G_MCH1 && pg->scope == SC_GLOBAL)
        return G_MCH1 + song.sel % NTRK;            /* MIDI OUT's CH: the selected track's */
    return id;
}

/* a global's: song.g[id] (< G_COUNT); past it the TOOLS actions, MUTE, METRO, COPY / PASTE, THRU, QUANT, LEARN, KBD,
 * LOCL (song.act: the device settings' mirror, ui_input.c) */
static const param_desc_t *glob_desc(uint32_t id, int16_t **valp)
{
    if (id >= G_COUNT) {
        id = id - G_COUNT < 11u ? id - G_COUNT : 0u;
        *valp = &song.act[id < 4u ? id : id < 6u ? id - 4u : id - 2u];   /* (COPY / PASTE: GO buttons, 0 at rest:
                                                                         * CLR* / INIT*'s act) */
        return &GP_ACT[id];
    }
    *valp = &song.g[id];
    return &GP[id];
}

static const param_desc_t *page_desc(const page_t *pg, uint32_t slot, int16_t **valp)
{
    uint32_t id = page_id(pg, slot);
    *valp = 0;
    if (id == 0xFFu || pg->scope == SC_GRID || pg->scope == SC_MIX)
        return 0;
    if (pg->scope == SC_GLOBAL)
        return glob_desc(id, valp);
    if (id == PG_MLEN) {
        *valp = &song.mlen;
        return &MLEN_DESC;
    }
    *valp = &TSEL->p[id];
    return track_desc(TSEL, id);
}
