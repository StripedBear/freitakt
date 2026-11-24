/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Drum machine fork: 2026 DEADACTIVE */
/* Drum UI pages: a family per page button (pressing it again steps through its pages), and what
 * the four knobs edit on each page. */
enum { FAM_HOME, FAM_SND, FAM_FX, FAM_SEQ, FAM_GLO, FAM_SAVE, FAM_LFO, FAM_MIX, FAM_EDT, FAM_ENV, FAM_ARP, FAM_COUNT };
/* FAM_EDT: no button, never shown on the FM-1: the web editor's own pages (editor.c ed_pages: GLOBAL, SYSTEM) */
enum { SC_TRACK, SC_GLOBAL, SC_MIX };   /* knobs edit: the selected track, song.g, the mixer */
enum { GR_NONE, GR_MODEL, GR_FX, GR_GRID, GR_STEPS, GR_SLOTS, GR_MIX, GR_COMP, GR_LFO, GR_MOTION, GR_FILTER,
       GR_LDEST, GR_INSERT, GR_DLY, GR_REV, GR_CHO };   /* (the editor's PAGES reply sends these: GR_GRID, the
                                                          * removed STEP page's, keeps its number) */

#define SND_SLOT 0xC0u                            /* EDIT page ids: SND_SLOT + n = the track's n-th sound knob */
#define SND_N 20u                                 /* .. at most: 5 SOUND pages (the fifth the SAMPLE model's) */
#define ENV_SLOT (SND_SLOT + SND_N)               /* ENV page ids: ENV_SLOT + n = the track's n-th envelope knob (env_list) */
#define ENV_N 8u                                  /* .. at most: 2 ENV pages */
#define PG_MLEN 0xFEu                             /* PATTERN's MLEN: MASTER LENGTH (song.mlen, the slot's; not a track's) */
#define PG_NOTE 0xFDu                             /* SOUND 1's NOTE (KNOB 2): the track's note for new steps (key_nt;
                                                   * a held step's own note: ui_input.c note_turn) */

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
                                                                                * PLAY CUT LOOP */
    /* ARP / RTRG (seq.c roll_*): the ARP button's own pages (2026-11: a tap opens page 1, again page 2, ui_input.c
     * arp_tap; no longer in EDIT's cycle); the second a pitched model's only (hidden on a drum: page_hidden) */
    {"ARP", FAM_ARP, SC_TRACK, GR_NONE, {P_AMODE, P_ARATE, P_AGATE, P_AFADE}},
    {"ARP", FAM_ARP, SC_TRACK, GR_NONE, {P_ADIR, P_ACHORD, P_ARANGE, 0xFF}},
    /* ENV (2026-11): the track's envelope knobs gathered (env_list: its machine's ATTACK / DECAY, a SAMPLE's ATK DECAY
     * SUS REL, the filter's DECAY last), 4 a page; a page past the list's end is not shown */
    {"ENV", FAM_ENV, SC_TRACK, GR_NONE, {ENV_SLOT + 0, ENV_SLOT + 1, ENV_SLOT + 2, ENV_SLOT + 3}},
    {"ENV", FAM_ENV, SC_TRACK, GR_NONE, {ENV_SLOT + 4, ENV_SLOT + 5, ENV_SLOT + 6, ENV_SLOT + 7}},
    {"FILTER", FAM_FX, SC_TRACK, GR_FILTER, {P_FTYPE, P_FCUT, P_FRESO, P_FENV}},   /* filter.c (its DECAY: ENV's) */
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
    /* LFO: one LFO on the FM-1 (FREITAKT redesign; LFO 2's knobs stay in the record, never computed: SRC is LFO1 or OFF): its
     * wave (the schematic over page 1), then TRIG MODE; then DEST, the track's 4 slots: KNOB 1 the slot
     * (song.lslot), 2-4 its SRC DEST DEPTH (page_id; lfo.c LS_ID) */
    {"LFO", FAM_LFO, SC_TRACK, GR_LFO, {P_LFO1 + LF_WAVE, P_LFO1 + LF_RATE, P_LFO1 + LF_MORPH, P_LFO1 + LF_PHASE}},
    {"LFO", FAM_LFO, SC_TRACK, GR_NONE, {P_LFO1 + LF_TRIG, P_LFO1 + LF_MODE, 0xFF, 0xFF}},
    {"DEST", FAM_LFO, SC_TRACK, GR_LDEST, {0xFF, 0xFF, 0xFF, 0xFF}},
    /* HOME's TRACKS mode (spec §3: a HOME tap on HOME; no button of its own): TRACK LEVEL LEN PAN */
    {"TRACKS", FAM_MIX, SC_MIX, GR_MIX, {0xFF, 0xFF, 0xFF, 0xFF}},
};
#define NPAGES (sizeof(PAGES) / sizeof(PAGES[0]))

/* the button (and LED) of each family; TRACKS (HOME's TRACKS mode) HOME's; SCL is free; the editor's pages none (0xFF:
 * no button's); ARP's open on its tap (ui_input.c mod_input), not on the press */
static const uint8_t FAM_BTN[FAM_COUNT] = {B_HOME, B_EDIT, B_FX, B_SEQ, B_GLO, B_SAVE, B_LFO, B_HOME, 0xFFu, B_ENV, B_ARP};

/* a model's envelope knob (its ATTACK, DECAY: ENV's page, 2026-11; not SOUND's) */
static int env_knob(const dmodel_t *m, uint32_t k)
{
    const char *l = m->edit[k].label;
    return l && (str_eq(l, "DECAY") || str_eq(l, "ATTACK"));
}
/* the EDIT list of track t: MODEL, NOTE (the sequencer's: PG_NOTE), NLEN (ACID SAMPL CHORD: nlen_model), its model's PITCH, TONE and the rest of its knobs
 * but the envelope ones (ENV's), then LVL PAN MNOTE CHOKE PRIO (at most 16); the SAMPLE model's then PLAY CUT LOOP on
 * the 5th page (0xFF, an empty column, up to it); returns how many (SND_N at most) */
static uint32_t snd_list(const track_t *t, uint8_t *ids)
{
    const dmodel_t *m = &DMODELS[(uint32_t)t->p[P_MODEL] % NMODELS];
    uint32_t n = 0, k;
    ids[n++] = P_MODEL;
    ids[n++] = PG_NOTE;
    if (nlen_model((uint32_t)t->p[P_MODEL] % NMODELS))
        ids[n++] = P_NLEN;                          /* NLEN: SOUND 1's third knob (a gated model's; the others' PITCH) */
    for (k = 0; k < 8u; k++)
        if ((k < 4u || (m->edit[k].label && m->edit[k].label[0] != '-')) && !env_knob(m, k))
            ids[n++] = (uint8_t)(P_E0 + k);
    ids[n++] = P_LEVEL;
    ids[n++] = P_PAN;
    ids[n++] = P_NOTE;
    ids[n++] = P_CHOKE;
    ids[n++] = P_PRIO;
    if ((uint32_t)t->p[P_MODEL] % NMODELS == DM_SMPL) {
        while (n < 16u)                             /* (SOUND 4/5: CHOKE PRIO) */
            ids[n++] = 0xFFu;
        ids[n++] = P_SPLAY;                         /* SOUND 5/5: PLAY CUT LOOP */
        ids[n++] = P_SCUT;
        ids[n++] = P_SLOOP;
    }
    return n;
}
/* the ENV list of track t (2026-11, option 1: the knobs it has, gathered; no new sound): a SAMPLE's ATK DECAY SUS REL;
 * another machine's ATTACK and DECAY as its knobs order them (CHORD: ATTACK, DECAY; a drum: DECAY); the filter's DECAY
 * last; returns how many (ENV_N at most) */
static uint32_t env_list(const track_t *t, uint8_t *ids)
{
    const dmodel_t *m = &DMODELS[(uint32_t)t->p[P_MODEL] % NMODELS];
    uint32_t n = 0, k;
    if ((uint32_t)t->p[P_MODEL] % NMODELS == DM_SMPL) {
        ids[n++] = P_SATK;
        for (k = 0; k < 8u; k++)
            if (env_knob(m, k))
                ids[n++] = (uint8_t)(P_E0 + k);
        ids[n++] = P_SSUS;
        ids[n++] = P_SREL;
    } else {
        for (k = 0; k < 8u && n < ENV_N - 1u; k++)  /* (ATTACK first: the knob before DECAY in time) */
            if (env_knob(m, k) && str_eq(m->edit[k].label, "ATTACK"))
                ids[n++] = (uint8_t)(P_E0 + k);
        for (k = 0; k < 8u && n < ENV_N - 1u; k++)
            if (env_knob(m, k) && !str_eq(m->edit[k].label, "ATTACK"))
                ids[n++] = (uint8_t)(P_E0 + k);
    }
    ids[n++] = P_FDEC;
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
    if (id >= ENV_SLOT && id < ENV_SLOT + ENV_N) {
        uint8_t ids[ENV_N];
        uint32_t n = env_list(TSEL, ids);
        return id - ENV_SLOT < n ? ids[id - ENV_SLOT] : 0xFFu;
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
    if (id == 0xFFu || pg->scope == SC_MIX)
        return 0;
    if (pg->scope == SC_GLOBAL)
        return glob_desc(id, valp);
    if (id == PG_MLEN) {
        *valp = &song.mlen;
        return &MLEN_DESC;
    }
    if (id == PG_NOTE) {
        *valp = &TSEL->key_nt;
        return &NOTE_DESC;
    }
    *valp = &TSEL->p[id];
    if (id == P_FDEC && pg->fam == FAM_ENV) {        /* (beside the machine's DECAY: the filter's, by its own name) */
        static const param_desc_t FDEC_ENV = PD("FDEC", F_FDEC, 0, 127, 40);
        return &FDEC_ENV;
    }
    return track_desc(TSEL, id);
}
