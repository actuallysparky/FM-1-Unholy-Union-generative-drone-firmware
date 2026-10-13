/* SPDX-License-Identifier: GPL-3.0-only */
/* HolyRND v1: main-loop project composition, never called from the ISR.
 * No saved IDs or project layouts added. Private streams make replay independent
 * of the instrument's random oscillators, keyboard activity and audio timing. */
typedef struct { uint8_t key, scale, rite, stir, tempo; uint32_t seed; } holy_recipe_t;
static uint32_t holy_next(uint32_t *s) {
    uint32_t x = *s; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return *s = x;
}
static uint32_t holy_bound(uint32_t *s, uint32_t n) {
    uint32_t x, cut = (uint32_t)(0u - n) % n;
    do { x = holy_next(s); } while (x < cut); return x % n;
}
static uint32_t holy_stream(uint32_t seed, uint32_t salt) {
    uint32_t s = seed ^ salt; return s ? s : 0x9e3779b9u;
}
static int holy_recipe_ok(const holy_recipe_t *r) {
    return r->key <= 12 && r->scale <= 4 && r->rite <= 2 && r->stir <= 127 &&
        (!r->tempo || (r->tempo >= 40 && r->tempo <= 110));
}
static const uint8_t HOLY_SCALE[4][7] = {{0,2,3,5,7,8,10},{0,2,3,5,7,9,10},{0,3,5,7,10,0,0},{0,2,4,7,9,0,0}};
static const uint8_t HOLY_SCALE_ID[4] = {2,3,6,5};
static const uint8_t HOLY_ENGINE[7] = {0,2,12,7,5,8,9};
static const uint8_t HOLY_WEIGHT[4][7] = {{30,20,10,15,15,5,5},{20,15,20,10,10,10,15},{15,20,20,10,10,5,20},{20,20,20,10,10,5,15}};
/* Compact role banks: every listed preset is a pitched factory sound. PHYS
 * percussion is excluded. Exact factory patches accompany FM6 projects. */
static const uint8_t HOLY_PAD[7][5] = {{1,3,11,255,255},{1,2,255,255,255},{4,6,255,255,255},{0,1,2,3,4},{0,255,255,255,255},{0,2,3,255,255},{3,7,255,255,255}};
static const uint8_t HOLY_PREF[7][5] = {{5,8,11,1,255},{4,2,1,5,255},{0,1,5,7,6},{3,1,2,255,255},{0,1,255,255,255},{3,2,0,255,255},{0,1,2,4,8}};
static uint32_t holy_preset(uint32_t *s, uint32_t family, uint32_t role, uint32_t rite) {
    const engine_t *e = ENGINES[HOLY_ENGINE[family]];
    const uint8_t *pref = role ? HOLY_PREF[family] : HOLY_PAD[family];
    uint32_t w[16], total=0, i, j, pick;
    for(i=0;i<e->npresets;i++) {
        w[i]=role ? 1u:0u;
        for(j=0;j<5;j++) if(pref[j]==i) w[i]=4;
        if(family==6 && (i==5 || i==6)) w[i]=0;
        if(rite==1 && (family==1 || family==2) && (i==1 || i==4)) w[i]*=2;
        total+=w[i];
    }
    pick=holy_bound(s,total);
    for(i=0;i<e->npresets;i++) { if(pick<w[i]) return i; pick-=w[i]; }
    return 0;
}
static uint8_t holy_near(uint32_t root, uint32_t scale, int32_t wanted, int32_t lo, int32_t hi) {
    int32_t best=lo, dist=999, n; uint32_t j, count=scale<2?7:5;
    for(n=lo;n<=hi;n++) for(j=0;j<count;j++) if((uint32_t)(n+12-root)%12==HOLY_SCALE[scale][j]) {
        int32_t d=n-wanted; if(d<0)d=-d; if(d<dist){dist=d;best=n;}
    }
    return (uint8_t)best;
}
#ifndef HOLY_CHOIR_MEASURE
#include "holy_mix.c"
#endif

static int holy_generate(project_t *p, const holy_recipe_t *r, uint8_t *root_out, uint8_t *scale_out) {
    uint32_t h=holy_stream(r->seed,0x4841524du), root, scale, section, k, i;
    uint32_t heavy=0,fms=0, progression=holy_bound(&h,3);
    if(!holy_recipe_ok(r)) return 2;
    root=r->key?r->key-1:holy_bound(&h,12);
    if(r->scale) scale=r->scale-1;
    else {i=holy_bound(&h,100);scale=i<50?0:i<70?1:i<85?2:3;}
    *root_out=(uint8_t)root; *scale_out=HOLY_SCALE_ID[scale];
    p->sel=3; memset(&p->motion,0,sizeof p->motion); memset(&p->chain,0,sizeof p->chain); memset(p->macro,0,4);
    if(!p->g[G_CLOCK]) {i=holy_bound(&h,100);p->g[G_BPM]=r->tempo?r->tempo:i<70?45+holy_bound(&h,31):i<95?76+holy_bound(&h,20):96+holy_bound(&h,15);}
    p->g[G_SWING]=0; p->g[G_RTYPE]=2;
    for(k=0;k<4;k++) {
        uint32_t s=holy_stream(r->seed,0x54524b31u+k*0x9e3779b9u), weights[7],total=0,family,e,pi;
        proj_trk_t *t=&p->t[k]; const preset_t *pr;
        for(i=0;i<7;i++){weights[i]=HOLY_WEIGHT[k][i]; if((heavy && i>=5)||(fms>=2 && i==2))weights[i]=0;
            if(r->rite==2 && (i==4 || i==6))weights[i]*=2;total+=weights[i];}
        i=holy_bound(&s,total);for(family=0;family<6 && i>=weights[family];family++)i-=weights[family];
        e=HOLY_ENGINE[family];pi=holy_preset(&s,family,k,r->rite);pr=&ENGINES[e]->presets[pi];
        if(e==8 && !SMP_ZONES[SMP_SETS[0].z0].n) return 5;
        heavy+=family>=5;fms+=family==2;t->engine=e;t->preset=pi;
        for(i=0;i<P_COUNT;i++)t->p[i]=param_desc_of(e,i)->def;
        for(i=0;i<8;i++)t->p[P_E0+i]=pr->e[i];
        t->p[P_ATK]=pr->env[0];t->p[P_DEC]=pr->env[1];t->p[P_SUS]=pr->env[2];t->p[P_REL]=pr->env[3]>60?60:pr->env[3];t->p[P_ED_FLT]=pr->fenv;
        memcpy(p->fm6[k],e==ENGI_FM6?FM6_FACTORY[pr->e[7]]:FM6_INIT,FM6_PACKED);
        if(e==5)t->p[P_E3]=0; /* VOICE SHIFT: concert pitch */
        if(e==9 && t->p[P_E0]==3)t->p[P_E1]=127; /* SYMP ROOT only */
        if(e==8){t->p[P_E0]=0;t->p[P_E4]=0;t->p[P_E3]=60;}
        t->p[P_ROOT]=root;t->p[P_SCALE]=HOLY_SCALE_ID[scale];t->p[P_QUANT]=1;
        t->p[P_SLEN]=64;t->p[P_SDIV]=1;t->p[P_SGATE]=k?68:100;
        t->p[P_VOICE]=k || family==2 || family>=5?V_MONO:V_POLY;
        t->p[P_LEVEL]=k==0?78+holy_bound(&s,11):k==1?64+holy_bound(&s,15):k==2?60+holy_bound(&s,13):78+holy_bound(&s,9);
        t->p[P_PAN]=k==1?8+holy_bound(&s,13):k==2?-8-(int32_t)holy_bound(&s,13):0;
        t->p[P_CHOR]=holy_bound(&s,41);t->p[P_DLY]=12+holy_bound(&s,21);t->p[P_REV]=35+holy_bound(&s,31);
        t->p[P_EVOL]=k==0?12+holy_bound(&s,21):k==3?0:holy_bound(&s,19);
        t->p[P_TENS]=(r->rite==2?8+holy_bound(&s,25):holy_bound(&s,r->rite==1?29:21))/(k?2:1);t->p[P_TRAMP]=4+holy_bound(&s,2);
        for(i=0;i<4;i++){t->p[P_M1SRC+i*3]=MS_MAC1+i;t->p[P_M1DST+i*3]=i==0?MD_REV:i==1?MD_DLY:i==2?MD_CHO:MD_PAN;t->p[P_M1AMT+i*3]=i==0?20:i==1?16:i==2?12:k==1?12:k==2?-12:0;}
        memset(t->step,0,sizeof t->step);for(i=0;i<64;i++)t->step[i].time=ST_REST;
        if(k==0) {
            step_t *st=&t->step[0];st->time=ST_NOTE;st->vel=82;st->n=t->p[P_VOICE]==V_POLY?3:1;
            st->note[0]=36+root;st->note[1]=holy_near(root,scale,40+root,36,59);st->note[2]=43+root;
            t->p[P_SUS]=112; if(e!=ENGI_FM6)t->p[P_ATK]=48;
            for(i=1;i<64;i++)t->step[i].time=ST_TIE;
        } else if(k<3) {
            static const uint8_t progress[3][4]={{0,1,2,3},{0,2,1,0},{0,0,3,0}};
            /* Chord pitch classes, already within the selected scale. Pentatonic
             * rites use dyads, avoiding thirds outside their five-note palette. */
            static const uint8_t chords[4][4][3]={
                {{0,3,7},{8,0,3},{5,8,0},{10,2,5}},
                {{0,3,7},{5,9,0},{7,10,2},{10,2,5}},
                {{0,7,0},{3,10,3},{5,0,5},{10,5,10}},
                {{0,7,0},{9,4,9},{2,9,2},{7,2,7}}
            };
            s=holy_stream(r->seed,0x50485231u+k*0x9e3779b9u);
            uint32_t count=scale<2?7:5, tones=scale<2?3:2;
            uint32_t motif[5], length=3+holy_bound(&s,3), shape=holy_bound(&s,4);
            int32_t prev=72+root;
            for(i=0;i<5;i++)motif[i]=holy_bound(&s,count);
            for(section=0;section<4;section++) {
                uint32_t chord=progress[progression][section],onsets=(k==1?2:3)+r->stir*(k==1?4:5)/127;
                for(i=0;i<onsets;i++) {
                    uint32_t lead=section==0?(k==1?4u:8u):0u;
                    uint32_t pos=section*16+lead+i*(section==0?16u-lead:14u)/onsets,degree;
                    int32_t want; step_t *st=&t->step[pos];
                    if(k==1)degree=(i%length==0 || holy_bound(&s,10)<7)?chords[scale][chord][i%tones]:HOLY_SCALE[scale][motif[i%length]];
                    else {uint32_t ix=shape==0?i%tones:shape==1?tones-1-i%tones:shape==2?(i%(tones*2-2)<tones?i%(tones*2-2):tones*2-2-i%(tones*2-2)):i%2?tones-1:0;degree=chords[scale][chord][ix];}
                    want=(k==1?60:48)+root+degree;
                    if(k==1){while(want<prev-6)want+=12;while(want>prev+6)want-=12;want=holy_near(root,scale,want,60,84);prev=want;}
                    st->time=ST_NOTE;st->n=1;st->note[0]=want;st->vel=64+holy_bound(&s,17);
                }
            }
        }
        if(r->rite==2)for(i=0;i<8;i++) {
            const param_desc_t *d=param_desc_of(e,P_E0+i);
            if(str_eq(d->label,"CUT") || str_eq(d->label,"TONE") || str_eq(d->label,"BRIT"))t->p[P_E0+i]=t->p[P_E0+i]*3/4;
        }
        for(i=0;i<P_COUNT;i++)t->p[i]=param_fit(param_desc_of(e,i),t->p[i]);
    }
#ifndef HOLY_CHOIR_MEASURE
    holy_balance(p,r);
#endif
    {static const char hex[]="0123456789ABCDEF";memset(p->name,0,sizeof p->name);p->name[0]='H';p->name[1]='R';p->name[2]='-';for(i=0;i<8;i++)p->name[3+i]=hex[(r->seed>>(28-i*4))&15];}
    p->sum=proj_sum(p);return proj_ok(p)&&proj_engines_ok(p)?0:4;
}

#include "starter.c"
static void holy_factory_project(project_t *p) {
    uint32_t k,i; memset(p,0,sizeof *p);
    p->magic=PROJ_MAGIC;p->size=sizeof *p;p->parts=NPART;p->phys=PROJ_PHYS;p->sel=3;
    chain_defaults(&p->chain);
    for(i=0;i<G_COUNT;i++)p->g[i]=GP[i].def;
    p->g[G_BPM]=72;p->g[G_CLOCK]=0;
    for(k=0;k<NTRK;k++){
        proj_trk_t *t=&p->t[k];
        const uint32_t e=UNHOLY_STARTER_SOUND[k][0],pi=UNHOLY_STARTER_SOUND[k][1];
        const preset_t *pr=&ENGINES[e]->presets[pi];
        static const uint8_t fx[4]={0,24,28,36};
        t->engine=e;t->preset=pi;
        for(i=0;i<P_COUNT;i++)t->p[i]=param_desc_of(e,i)->def;
        for(i=0;i<8;i++)t->p[P_E0+i]=pr->e[i];
        t->p[P_ATK]=pr->env[0];t->p[P_DEC]=pr->env[1];t->p[P_SUS]=pr->env[2];t->p[P_REL]=pr->env[3];t->p[P_ED_FLT]=pr->fenv;
        t->p[P_VOICE]=pr->mono?V_LEGATO:V_POLY;
        for(i=0;i<4;i++)t->p[P_DIST+i]=pr->fx[i]?pr->fx[i]-1:fx[i];
        memcpy(p->fm6[k],e==ENGI_FM6?FM6_FACTORY[pr->e[7]]:FM6_INIT,FM6_PACKED);
        unholy_starter_track(t->p,t->step,k);
    }
    str_cpy(p->name,"EVENING VOW",sizeof p->name);p->sum=proj_sum(p);
}
/* On-demand harmony: retain patches, parameters, rhythms and the live track. */
static int holy_evolve(project_t *p, uint32_t seed, uint8_t *root_out, uint8_t *scale_out) {
    static const uint8_t chords[4][4][3]={
        {{0,3,7},{8,0,3},{5,8,0},{10,2,5}},
        {{0,3,7},{5,9,0},{7,10,2},{10,2,5}},
        {{0,7,0},{3,10,3},{5,0,5},{10,5,10}},
        {{0,7,0},{9,4,9},{2,9,2},{7,2,7}}
    };
    uint32_t root=p->t[0].p[P_ROOT],sid=p->t[0].p[P_SCALE];
    uint32_t scale=sid==2?0:sid==3?1:sid==6?2:sid==5?3:4;
    uint32_t s=holy_stream(seed,0x45564f4cu),progress[4],k,i,n;
    if(scale==4 || root>11)return 2;
    *root_out=root;*scale_out=sid;
    progress[0]=holy_bound(&s,4);for(i=1;i<4;i++)progress[i]=holy_bound(&s,4);
    for(k=0;k<3;k++) {
        if(p->t[k].p[P_ROOT]!=root || p->t[k].p[P_SCALE]!=sid)return 2;
        for(i=0;i<NSTEP;i++) {
            step_t *st=&p->t[k].step[i];
            if(st->time!=ST_NOTE || !st->n)continue;
            for(n=0;n<st->n;n++) {
                /* Keep the drone bass and half the melody as familiar anchors. */
                if((k==0 && n==0) || (k==1 && holy_bound(&s,100)<50))continue;
                uint32_t chord=progress[i/16],tones=scale<2?3:2;
                uint32_t degree=chords[scale][chord][(i+n+holy_bound(&s,tones))%tones];
                int32_t before=st->note[n],want=root+degree;
                while(want<before-6)want+=12;
                while(want>before+6)want-=12;
                st->note[n]=holy_near(root,scale,want,0,127);
            }
        }
    }
    p->sel=3;p->sum=proj_sum(p);return 0;
}

/* Optional ambient LFO blessing. Never changes notes, presets, key or tempo. */
static int holy_lfo_roll(project_t *p, uint32_t seed) {
    uint32_t k,i,s=holy_stream(seed,0x4c464f31u);
    static const uint8_t targets[3]={MD_PAN,MD_REV,MD_CHO};
    for(k=0;k<NTRK;k++) {
        int16_t *v=p->t[k].p;
        v[P_LRATE]=8+holy_bound(&s,25);v[P_LWAVE]=holy_bound(&s,2);
        v[P_LSYNC]=0;v[P_LTRIG]=1;v[P_LPOL]=0;
        v[P_LPHASE]=holy_bound(&s,128);v[P_LFADE]=20+holy_bound(&s,25);
        v[P_LD_PIT]=v[P_LD_FLT]=v[P_LD_SHP]=v[P_LD_AMP]=0;
        for(i=0;i<4;i++)if(v[P_M1SRC+3*i]==MS_LFO){v[P_M1SRC+3*i]=v[P_M1DST+3*i]=v[P_M1AMT+3*i]=0;}
        v[P_M4SRC]=MS_LFO;v[P_M4DST]=targets[holy_bound(&s,3)];v[P_M4AMT]=4+holy_bound(&s,9);
    }
    p->sel=3;p->sum=proj_sum(p);return 0;
}

static void holy_factory_settings(void) {
    /* Preserve calibration, favorites, saved projects/presets and MIDI learn. */
    ui_prefs=ui_rec_prefs=ui_prefs2=ui_home_view=ui_style=ui_scr=ui_idle=0;
    ui_auto_evolve=UI_AUTO_EVOLVE_DEFAULT;
    settings.magic=0;settings_init();settings_leds=LEDS_DIM;settings_hold=HOLD_DEF;rp_apply();
    favorites.factory[15][31]=0;ui_lcat=0;
    holy.key=holy.scale=holy.rite=holy.tempo=holy.confirm=holy.has_seed=0;holy.stir=32;
    holy.screen=1;ui.home=1;ui.menu=ui.layer=ui.ly=0;ui.force=1;
    settings_save();
    /* Force a new autosave even if this happens to match the current music. */
    as.saved=~autosave_sig();as.seen=autosave_sig();as.t=fm1_ms;
}

/* One full-project undo; scratch is owned only while phase != IDLE. */
static project_t holy_undo __attribute__((section(".pool")));
static uint32_t holy_started, holy_sig;
static uint8_t holy_resume, holy_is_undo, holy_is_reset, holy_is_evolve, holy_is_lfo;
static int holy_keys_down(void) {
    uint32_t k;if(fm1_in.notes || mom.n)return 1;
    for(k=0;k<4;k++)if(trk[k].arp_phys || trk[k].nheld || midi_owners[k])return 1;
    return 0;
}
/* Clock observation is O(1); composition still runs only in the main loop.
 * Missed intervals coalesce into one evolution, never a burst after editing/transfer.
 * Stop/start or interval changes begin a fresh wait. Count-in is not playback. */
static uint32_t holy_auto_bar, holy_auto_run;
static uint8_t holy_auto_setting, holy_auto_playing, holy_auto_request;
static void holy_auto_poll(void) {
    static const uint8_t bars[8] = {0,2,4,8,12,32,16,24};
    uint32_t now = seq_bars;
    uint8_t setting = ui_auto_evolve < NELEM(bars) ? ui_auto_evolve : 0;
    if (!song.playing || holy_auto_run != seq_run || !holy_auto_playing || setting != holy_auto_setting) {
        holy_auto_bar=now; holy_auto_run=seq_run; holy_auto_playing=song.playing; holy_auto_setting=setting;
        return;
    }
    if (!setting || now-holy_auto_bar < bars[setting]) return;
    if (holy.phase || holy.request || song.rec || (ui.menu && !(ui.menu==1 && ui.menu_sel==MI_AUTO_EVOLVE)) || ui.confirm || song.grid == 2u || ui.layer || ui.uboot || ui.ml.on ||
        (holy.transfer && fm1_ms-holy.transfer_ms<15000u)) {
        holy_auto_bar=now; return; /* skip busy interval; do not queue a surprise afterwards */
    }
    holy_auto_bar=now; holy_auto_request=1;
    holy.seed=holy.seed*1664525u+1013904223u+fm1_ms;holy.request=4;
}
static void holy_poll(void) {
    uint32_t k; holy_recipe_t recipe;
    project_checkpoint_poll();
    holy_auto_poll();
    if(!holy.phase && holy.request) {
        uint8_t request=holy.request, automatic=holy_auto_request;holy.request=0;holy_auto_request=0;
        if((request<=3 && holy_keys_down()) || (holy.transfer && fm1_ms-holy.transfer_ms<15000u)){holy.status=1;ui_message("RELEASE KEYS");return;}
        if(request==6){project_checkpoint();return;}
        if(request==7){project_checkpoint_to(ui.checkpoint_slot);return;}
        holy_is_undo=request==2;holy_is_reset=request==3;holy_is_evolve=request==4;holy_is_lfo=request==5;
        if(holy_is_undo && !holy.undo_valid){holy.status=3;ui_message("NO RITE TO UNDO");return;}
        if(holy_is_undo && autosave_sig()!=holy_sig && !holy.confirm){holy.status=7;ui_message("HOLD OCT- AGAIN");holy.confirm=1;return;}
        holy.confirm=0;holy_resume=holy_is_reset?0:song.playing;holy_started=fm1_ms;
        if(holy_is_undo){memcpy(&proj_scratch,&holy_undo,sizeof proj_scratch);project_capture(&holy_undo);}
        else if(holy_is_reset){holy_factory_project(&proj_scratch);project_capture(&holy_undo);}
        else {
            project_capture(&proj_scratch);
            recipe=(holy_recipe_t){holy.key,holy.scale,holy.rite,holy.stir,holy.tempo,holy.seed};
            holy.status=holy_is_lfo?holy_lfo_roll(&proj_scratch,holy.seed):holy_is_evolve?holy_evolve(&proj_scratch,holy.seed,&holy.root,&holy.scale_id):holy_generate(&proj_scratch,&recipe,&holy.root,&holy.scale_id);
            if(holy.status){if(!automatic)ui_message(holy_is_evolve?"SET SHARED SCALE":"RITE REFUSED");return;}
            project_capture(&holy_undo);
        }
        if(holy_is_evolve || holy_is_lfo) {
            /* Live changes: do not stop, reload engines, retrigger or reset clock phase. */
            fm1_irq_off();
            song.rec=0;
            for(k=0;k<NTRK;k++) {
                if(holy_is_evolve && k<3) {
                    for(uint32_t i=0;i<NSTEP;i++)
                        memcpy(trk[k].step[i].note,proj_scratch.t[k].step[i].note,sizeof trk[k].step[i].note);
                } else if(holy_is_lfo) {
                    for(uint32_t i=0;i<P_COUNT;i++)
                        if(proj_scratch.t[k].p[i]!=holy_undo.t[k].p[i])trk[k].p[i]=proj_scratch.t[k].p[i];
                }
            }
            if(!automatic){song.sel=3;song.octave=song.track_octave[3];}fm1_irq_on();
            holy_auto_bar=seq_bars;
            holy.status=0;holy.undo_valid=1;holy.has_seed=0;holy_sig=autosave_sig();
            holy.prayer=(holy.prayer+1u)%16u;holy.pulse=1;holy.pulse_ms=fm1_ms;
            sync_reload=1;ui.force=1;autosave_hold();ui_message(HOLY_PRAYERS[holy.prayer]);return;
        }
        holy.phase=2;transport_req=2;
        if(!holy_is_undo && !holy_is_reset){holy.prayer=(holy.prayer+1u)%16u;holy.pulse=1;holy.pulse_ms=fm1_ms;}
        ui.force=1;ui_message(holy_is_reset?"RETURNING...":holy_is_undo?"RESTORING...":HOLY_PRAYERS[holy.prayer]);
    }
    if(!holy.phase)return;
    autosave_hold();
    if(holy.phase==2 && !song.playing && !transport_req) {
        fm1_irq_off();song.rec=0;
        for(k=0;k<4;k++){trk[k].nheld=trk[k].arp_phys=trk[k].xp_n=0;trk[k].xf_on=1;trk[k].xf=XF_BLOCKS;}
        fm1_irq_on();holy.phase=3;holy_started=fm1_ms;return;
    }
    if(holy.phase==3) {
        for(k=0;k<4;k++)if(trk[k].xf_on)goto waiting;
        if(project_restore_runtime(&proj_scratch)){holy.status=4;holy.phase=0;return;}
        holy.phase=4;holy_started=fm1_ms;return;
    }
    if(holy.phase==4) {
        for(k=0;k<4;k++)if(trk[k].xf_on || trk[k].engine!=trk[k].eng_req)goto waiting;
        holy.phase=0;holy.status=0;holy.undo_valid=!holy_is_undo && !holy_is_reset;holy.has_seed=!holy_is_undo && !holy_is_reset && !holy_is_evolve && !holy_is_lfo;
        song.sel=3;if(holy_is_reset)holy_factory_settings();
        holy_sig=autosave_sig();if(holy_resume)transport_req=1;ui_message(holy_is_reset?"EVENING VOW":holy_is_undo?"RITE UNDONE":"REVELATION");ui.force=1;return;
    }
waiting:
    if(fm1_ms-holy_started>1000u){if(holy.phase==4)project_restore_runtime(&holy_undo);transport_req=2;holy.phase=0;holy.status=6;holy.undo_valid=0;ui_message("RITE TIMED OUT");ui.force=1;}
}
