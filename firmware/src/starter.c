#ifndef UNHOLY_STARTER_INCLUDED
#define UNHOLY_STARTER_INCLUDED
/* SPDX-License-Identifier: GPL-3.0-only */
/* EVENING VOW: a self-contained, editable three-part A-minor demonstration.
 * Loaded before normal autosave restoration; never saved over a project slot.
 * The chord arpeggio is recorded as steps so PLAY needs no held keyboard chord. */
static const uint8_t UNHOLY_STARTER_SOUND[NTRK][2] = {
    {0, 1}, {ENGI_FM6, 1}, {2, 2}, {ENGI_PHYS, 3}
}; /* ANALOG SOFT PAD, FM6 BELL, PHASE STRING, live PHYS BOWED METAL */

static void unholy_starter_track(int16_t *p, step_t *step, uint32_t k)
{
    static const uint8_t BED[3] = {45, 48, 52}; /* A2 C3 E3 */
    /* User-supplied backup's eight harmonic events, developed into sixteen bars.
     * First half retains its pitches; second half adds open voicings and a homecoming. */
    static const uint8_t VOW[16][3] = {
        {64,52,0},{64,52,0},{52,64,69},{52,64,69},
        {48,40,0},{40,48,0},{60,52,69},{60,52,69},
        {52,64,76},{52,64,67},{52,64,69},{52,69,76},
        {40,48,64},{48,52,60},{52,60,72},{45,60,69}
    };
    static const uint8_t ARP[8] = {57, 60, 64, 69, 72, 69, 64, 60};
    uint32_t i;
        memset(step, 0, sizeof(step_t) * NSTEP);
        for (i = 0; i < NSTEP; i++) step[i].time = ST_REST;
        p[P_AMODE] = p[P_AHOLD] = 0;
        p[P_ROOT] = 9; p[P_SCALE] = 2; /* A minor */
        p[P_SLEN] = 16; p[P_SDIV] = 1; /* eighths, two-bar phrase */
        p[P_SGATE] = 90;
        p[P_LEVEL] = k == 0 ? 86 : k == 1 ? 77 : k == 2 ? 68 : 80;
        p[P_PAN] = k == 1 ? 12 : k == 2 ? -12 : 0;
        if (k < 3u) {
            p[P_M1SRC] = MS_DRIFT; p[P_M1DST] = MD_PAN; p[P_M1AMT] = 8;
            p[P_M2SRC] = MS_MAC1; p[P_M2DST] = MD_CUT; p[P_M2AMT] = 22;
            p[P_M3SRC] = MS_MAC2; p[P_M3DST] = MD_REV; p[P_M3AMT] = 25;
            p[P_M4SRC] = MS_MAC3; p[P_M4DST] = MD_DLY; p[P_M4AMT] = 20;
        }
    if (k == 0) {
    p[P_SLEN] = 32;
    p[P_SUS] = 112;
    p[P_EVOL] = 27; p[P_TENS] = 12; p[P_TRAMP] = 4; /* 8 bars */
    step[0].n = 3; step[0].time = ST_NOTE; step[0].vel = 90;
    memcpy(step[0].note, BED, sizeof BED);
    for (i = 1; i < 32u; i++) step[i].time = ST_TIE;
    }
    if (k == 1) {
    p[P_EVOL] = 10;
    p[P_SLEN] = 64; p[P_SDIV] = 0; /* 64 quarter notes: sixteen bars */
    p[P_VOICE] = V_POLY; p[P_LEVEL] = 70; /* room for the supplied chord sequence */
    p[P_M1SRC] = MS_MAC4; p[P_M1DST] = MD_PAN; p[P_M1AMT] = 20;
    }
    if (k == 2) {
    p[P_ATK] = 16; p[P_DEC] = 55; p[P_SUS] = 60; p[P_REL] = 38;
    p[P_EVOL] = 18;
    p[P_SLEN] = 64; /* eight-bar arp loop with a one-bar entrance */
    }
    if (k == 3) {
    p[P_VOICE] = V_MONO; /* one live note alongside the backing's shared voices */
    p[P_DLY] = 24; p[P_REV] = 55;
    }
    for (i = 0; i < NSTEP; i++) {
        step_t *s = &step[i];
        if (k == 2 && i >= 8u) {
        s->note[0] = ARP[i % 8u]; s->n = 1; s->time = ST_NOTE; s->vel = i % 4u ? 66 : 78;
        }
    }
    if (k == 1) for (i = 0; i < 16u; i++) {
        uint32_t at=i*4u+(i==0?2u:0u);step_t *s=&step[at];
        s->time=ST_NOTE;s->n=VOW[i][2]?3:2;s->vel=i>=8u?(i%4u?72:78):74;
        memcpy(s->note,VOW[i],s->n);
        for(uint32_t j=at+1u;j<(i+1u)*4u;j++)step[j].time=ST_TIE;
    }
}

static void unholy_starter_song(void)
{
    song.g[G_BPM] = 72; song.g[G_CLOCK] = 0;
    song.sel = 3; song.rec = song.playing = 0;
    for (uint32_t i = 0; i < 4; i++) song.macro[i] = 0;
    for (uint32_t k = 0; k < NTRK; k++) unholy_starter_track(trk[k].p, trk[k].step, k);
}
#endif
