/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Firmware save flows against NOR flash: deferred settings, retries, and failed-save rollback. */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int32_t fm1_enc_take(uint32_t e) { (void)e; return 0; }
static uint32_t irq_start_race, irq_races;
static void fm1_irq_off(void)
{
    if (irq_start_race) {                         /* audio serviced PLAY immediately before main masks it */
        irq_start_race = 0;
        events_block(CTL);
        irq_races++;
    }
}
static void fm1_irq_on(void) {}
static void lcd_sync(void) {}
static void lcd_power(uint32_t s) { (void)s; }   /* (MENU > SCREEN OFF: lcd.c) */
static void lcd_wake_now(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#define FELUCCA_FLASH 1
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/menu_items.c"
static void panel_setup(void) {}

static uint8_t nor[0x100000], flash_ok = 1;
static int erase_error, fail_after = -1;
static uint32_t erases, as_erases;               /* (as_erases: the autosave's two sectors, 0xE5000 / 0xE6000) */
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off)
{
    erases++;
    as_erases += off >= 0xE5000u && off < 0xE7000u;
    if (erase_error) return -8;
    memset(nor + off, 0xFF, 4096);
    return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    if (fail_after == 0) return -9;
    if (fail_after > 0) fail_after--;
    for (i = 0; i < n; i++) nor[off + i] &= s[i];
    return 0;
}
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0; }
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"


#include "../firmware/src/starter.c"
static int bad;
static void check(const char *what, int ok) { printf("starter: %s %s\n",what,ok?"ok":"FAIL"); bad += !ok; }
static void starter(void) {
 memset(&song,0,sizeof song); memset(trk,0,sizeof trk); memset(&chain,0,sizeof chain);
 chain_defaults(&chain_config); host_tracks_init(); fm6_init();
 for(uint32_t i=0;i<NTRK;i++) {track_defaults(&trk[i]);host_preset(&trk[i],UNHOLY_STARTER_SOUND[i][0],UNHOLY_STARTER_SOUND[i][1]);track_defaults_steps(&trk[i]);}
 unholy_starter_song(); str_cpy(proj_name,"EVENING VOW",sizeof proj_name);
 song.master_q12=2048; transport_req=panic_req=0;
}

static project_t candidate, repeated, evolved;

#ifndef HOLY_TEST_NO_MAIN
int main(void) {
    int ok=1; uint32_t families=0; uint8_t root,scale;
    memset(nor,0xff,sizeof nor);starter();project_capture(&holy_undo);
    holy_factory_project(&candidate);repeated=holy_undo;
    /* host_tracks_init intentionally uses legacy ROOM for DSP goldens; real boot uses GP defaults. */
    repeated.g[G_RTYPE]=GP[G_RTYPE].def;repeated.sum=proj_sum(&repeated);
    check("factory reset project matches first-run Evening Vow",!memcmp(&candidate,&repeated,sizeof candidate));
    for(uint32_t seed=0;seed<1000;seed++) {
        holy_recipe_t r={seed%13,seed%5,seed%3,seed%2?127:0,0,seed};
        candidate=holy_undo;repeated=holy_undo;
        ok &= !holy_generate(&candidate,&r,&root,&scale);
        ok &= !holy_generate(&repeated,&r,&root,&scale) && !memcmp(&candidate,&repeated,sizeof candidate);
        evolved=candidate;
        ok &= !holy_evolve(&evolved,seed+1234,&root,&scale);
        ok &= !memcmp(&evolved.t[3],&candidate.t[3],sizeof evolved.t[3]);
        for(uint32_t k=0;k<3;k++) {
            ok &= !memcmp(evolved.t[k].p,candidate.t[k].p,sizeof evolved.t[k].p);
            for(uint32_t i=0;i<64;i++)for(uint32_t n=0;n<evolved.t[k].step[i].n;n++) {
                int in=0;uint32_t count=(scale==5||scale==6)?5:7;
                for(uint32_t j=0;j<count;j++)in|=(evolved.t[k].step[i].note[n]+12-root)%12==HOLY_SCALE[r.scale?r.scale-1:scale==2?0:scale==3?1:scale==6?2:3][j];
                ok &= in;
            }
        }
        uint32_t heavy=0,fm=0;
        for(uint32_t k=0;k<4;k++) {
            proj_trk_t *t=&candidate.t[k];families|=1u<<t->engine;heavy+=t->engine==8||t->engine==9;fm+=t->engine==12;
            ok &= t->p[P_ROOT]==root && t->p[P_SCALE]==scale && !t->p[P_AMODE] && !t->p[P_AHOLD];
            for(uint32_t i=0;i<64;i++) {
                step_t *st=&t->step[i];if(k==3)ok &= st->time==ST_REST && !st->n && !st->hit;
                for(uint32_t n=0;n<st->n;n++){int in=0;for(uint32_t j=0;j<(scale==5||scale==6?5:7);j++)in|=((st->note[n]+12-root)%12)==HOLY_SCALE[r.scale?r.scale-1:scale==2?0:scale==3?1:scale==6?2:3][j];ok &= in;}
            }
        }
        ok &= heavy<=1 && fm<=2 && proj_pack(&proj_wire,&candidate) && proj_import(&repeated,&proj_wire,sizeof proj_wire);
    }
    check("1000 seeds deterministic, valid, in key, silent live track and bounded engine mix",ok);
    check("all seven engine families represented",families==((1u<<0)|(1u<<2)|(1u<<5)|(1u<<7)|(1u<<8)|(1u<<9)|(1u<<12)));
    starter();song.track_octave[0]=1;step_t octave_step={0};
    octave_step.time=ST_NOTE;octave_step.n=1;octave_step.note[0]=60;octave_step.vel=80;
    seq_step(&trk[0],&octave_step,0,1000,0);seq_step(&trk[1],&octave_step,0,1000,0);
    int octave_ok=trk[0].seq_notes[0]==72 && trk[1].seq_notes[0]==60 && octave_step.note[0]==60;
    seq_release(&trk[0]);seq_release(&trk[1]);
    song.track_octave[0]=0;song.octave=0;in_key=1;kb_note[0]=53;arp_add(&trk[0],53);
    uint32_t octave_list[64],octave_count;
    song.track_octave[0]=1;arp_list(&trk[0],octave_list,&octave_count);octave_ok &= octave_list[0]==65;
    trk[0].nheld=0;song.octave=1;kb_note[0]=65;arp_add(&trk[0],65);
    arp_list(&trk[0],octave_list,&octave_count);octave_ok &= octave_list[0]==65; /* no double shift */
    song.track_octave[0]=-1;arp_list(&trk[0],octave_list,&octave_count);octave_ok &= octave_list[0]==41;
    in_key=0;trk[0].nheld=0;arp_add(&trk[0],60);arp_list(&trk[0],octave_list,&octave_count);octave_ok &= octave_list[0]==48;
    song.track_octave[0]=3;octave_step.note[0]=120;seq_step(&trk[0],&octave_step,0,1000,0);
    octave_ok &= trk[0].seq_notes[0]==127;
    check("selected-track sequence and panel/MIDI arp octaves, no double shift, untouched pattern and pitch clamp",octave_ok);
    starter();song.track_octave[0]=1;song.rec=1;song.playing=1;trk[0].seq_idx=0;trk[0].seq_pos=0;trk[0].step[0].n=0;trk[0].step[0].time=ST_REST;
    rec_note(&trk[0],72,80);
    check("live recording normalizes octave once and retains sounding-note release ownership",trk[0].step[0].note[0]==60 && trk[0].rh_note[0]==72);
    /* Bar scheduling follows the transport clock, never elapsed wall time. */
    {holy_recipe_t intro={10,1,0,32,72,1234};uint8_t root,scale;starter();project_capture(&candidate);holy_generate(&candidate,&intro,&root,&scale);
     int lead_ok=1;for(uint32_t i=0;i<4;i++)lead_ok &= candidate.t[1].step[i].time==ST_REST;
     for(uint32_t i=0;i<8;i++)lead_ok &= candidate.t[2].step[i].time==ST_REST;
     check("generated pad precedes melody by two beats and arp by four",lead_ok&&candidate.t[0].step[0].time==ST_NOTE&&candidate.t[1].step[4].time==ST_NOTE&&candidate.t[2].step[8].time==ST_NOTE);}
    int auto_ok=1;
    for(uint32_t setting=0;setting<8;setting++) {
        static const uint8_t bars[8]={0,2,4,8,12,16,24,32};
        starter();memset(&ui,0,sizeof ui);holy.request=holy.phase=holy.transfer=0;
        menu_put(MI_AUTO_EVOLVE,setting);auto_ok &= menu_get(MI_AUTO_EVOLVE)==setting;
        seq_start();click_tick(0);song.sel=1;holy_poll();
        project_capture(&candidate);
        uint32_t wait=bars[setting]?bars[setting]:32;
        for(uint32_t bar=0;bar<wait;bar++) {
            click_tick(div_samples(2)*16u);
            holy_auto_poll();
            auto_ok &= (holy.request==4)==(setting && bar+1==wait);
        }
        if(setting) {
            uint32_t tick=song.tick,pos=trk[0].seq_pos;
            holy_poll();project_capture(&evolved);
            auto_ok &= song.playing && song.sel==1 && song.tick==tick && trk[0].seq_pos==pos;
            auto_ok &= !memcmp(candidate.t[3].step,evolved.t[3].step,sizeof candidate.t[3].step);
            for(uint32_t k=0;k<4;k++)auto_ok &= !memcmp(candidate.t[k].p,evolved.t[k].p,sizeof candidate.t[k].p);
            seq_stop();holy_auto_poll();seq_bars+=64;holy_auto_poll();auto_ok &= !holy.request;
        }
    }
    check("all auto-evolve intervals, OFF, clock bars, stop and live-focus continuity",auto_ok);
    starter();memset(&ui,0,sizeof ui);holy.request=holy.phase=0;menu_put(MI_AUTO_EVOLVE,1);
    seq_start();click_tick(0);holy_poll();song.rec=1;click_tick(div_samples(2)*32u);holy_poll();
    auto_ok=!holy.request && song.rec==1;song.rec=0;holy_poll();auto_ok &= !holy.request;
    ui.menu=1;click_tick(div_samples(2)*32u);holy_poll();auto_ok &= !holy.request;
    ui.menu=0;holy_poll();auto_ok &= !holy.request;
    menu_put(MI_AUTO_EVOLVE,0);holy_poll();click_tick(div_samples(2)*512u);holy_poll();auto_ok &= !holy.request;
    check("recording and menus skip intervals without catch-up; OFF cancels",auto_ok);
    menu_put(MI_AUTO_EVOLVE,5);persist_t auto_saved={0};settings_export(&auto_saved);
    menu_put(MI_AUTO_EVOLVE,0);settings_import(&auto_saved,sizeof auto_saved);
    check("auto-evolve interval survives settings export/import",menu_get(MI_AUTO_EVOLVE)==5);
    ui_auto_evolve=5;check("legacy stored 32 bars retains its interval",menu_get(MI_AUTO_EVOLVE)==7);
    starter();memset(&ui,0,sizeof ui);holy.request=holy.phase=holy.transfer=0;
    ui.menu=1;ui.menu_sel=MI_AUTO_EVOLVE;menu_put(MI_AUTO_EVOLVE,1);
    seq_start();click_tick(0);holy_auto_poll();
    click_tick(div_samples(2)*32u);holy_auto_poll();
    check("auto evolution runs while its own field stays open",holy.request==4 && ui.menu==1);
    holy_poll();check("live menu evolution preserves playback and menu",song.playing && ui.menu==1 && !holy.phase);
    menu_put(MI_AUTO_EVOLVE,0);holy_auto_poll();click_tick(div_samples(2)*512u);holy_auto_poll();
    check("OFF takes effect without leaving its field",!holy.request && ui.menu==1);
    holy_factory_settings();check("factory reset defaults auto evolution to 16 bars",menu_get(MI_AUTO_EVOLVE)==5);
    memset(nor,0xFF,sizeof nor);ui_auto_evolve=0;persist_boot();
    check("fresh storage defaults auto evolution to 16 bars",menu_get(MI_AUTO_EVOLVE)==5);
    menu_put(MI_AUTO_EVOLVE,0);settings_export(&auto_saved);
    ui_auto_evolve=6;settings_import(&auto_saved,sizeof auto_saved);flash_ok=1;
    check("saved OFF still overrides the new-install default",menu_get(MI_AUTO_EVOLVE)==0);
    starter();holy.key=10;holy.scale=4;holy.rite=1;holy.seed=12345;holy.request=1;
    int32_t out[CTL*2];
    for(uint32_t i=0;i<80;i++){holy_poll();mix_block(out,CTL);fm1_ms++;}
    check("native reveal applies major pentatonic without starting transport",!holy.phase&&!holy.status&&holy.undo_valid&&!song.playing&&song.sel==3&&trk[0].p[P_SCALE]==5);
    holy.request=2;for(uint32_t i=0;i<80;i++){holy_poll();mix_block(out,CTL);fm1_ms++;}
    check("whole project undo restores Evening Vow",!holy.phase&&!holy.status&&!strcmp(proj_name,"EVENING VOW"));
    uint32_t before=erases;starter();transport_req=1;mix_block(out,CTL);holy.request=1;
    for(uint32_t i=0;i<80;i++){holy_poll();mix_block(out,CTL);fm1_ms++;}
    check("reveal while playing resumes with recording disarmed",song.playing&&!song.rec&&!holy.phase);
    check("reveal and undo never write flash or slots",erases==before);
    starter();holy.undo_valid=0;fm1_in.notes=1;holy.request=1;holy_poll();
    check("held physical key refuses revelation without changing current project",holy.status==1 && !strcmp(proj_name,"EVENING VOW"));fm1_in.notes=0;
    midi_owners[0]=1;holy.request=1;holy_poll();check("held MIDI ownership refuses revelation",holy.status==1);midi_owners[0]=0;
    project_capture(&candidate);candidate.g[G_CLOCK]=1;candidate.g[G_BPM]=103;
    holy_recipe_t ext={0,4,0,32,44,42};holy_generate(&candidate,&ext,&root,&scale);
    check("external clock preserves configured clock and tempo",candidate.g[G_CLOCK]==1 && candidate.g[G_BPM]==103);
    project_capture(&candidate);holy_recipe_t invalid={0,5,0,32,0,0};repeated=candidate;
    check("invalid recipe leaves project untouched",holy_generate(&candidate,&invalid,&root,&scale)==2 && !memcmp(&candidate,&repeated,sizeof candidate));
    starter();project_capture(&candidate);repeated=candidate;
    holy_lfo_roll(&candidate,54321);
    int lfo_ok=1;
    for(uint32_t k=0;k<4;k++) {
        int16_t *v=candidate.t[k].p;
        lfo_ok &= !memcmp(candidate.t[k].step,repeated.t[k].step,sizeof candidate.t[k].step);
        lfo_ok &= candidate.t[k].engine==repeated.t[k].engine && candidate.t[k].preset==repeated.t[k].preset;
        lfo_ok &= v[P_LRATE]>=8 && v[P_LRATE]<=32 && v[P_LWAVE]<=1 && v[P_LTRIG]==1 && !v[P_LSYNC] && !v[P_LD_PIT] && !v[P_LD_AMP];
        lfo_ok &= v[P_M4SRC]==MS_LFO && v[P_M4AMT]>=4 && v[P_M4AMT]<=12 && (v[P_M4DST]==MD_PAN||v[P_M4DST]==MD_REV||v[P_M4DST]==MD_CHO);
    }
    check("optional LFO roll preserves notes/presets and uses bounded slow safe modulation",lfo_ok);
    repeated=candidate;check("evolve accepts shared minor scale",!holy_evolve(&candidate,67890,&root,&scale));
    int evolve_ok=!memcmp(&candidate.t[3],&repeated.t[3],sizeof candidate.t[3]);
    for(uint32_t k=0;k<3;k++) {
        evolve_ok &= !memcmp(candidate.t[k].p,repeated.t[k].p,sizeof candidate.t[k].p);
        evolve_ok &= candidate.t[k].engine==repeated.t[k].engine && candidate.t[k].preset==repeated.t[k].preset;
        for(uint32_t i=0;i<64;i++) {
            step_t x=candidate.t[k].step[i],y=repeated.t[k].step[i];
            memset(x.note,0,sizeof x.note);memset(y.note,0,sizeof y.note);evolve_ok &= !memcmp(&x,&y,sizeof x);
        }
    }
    check("evolve retains LFOs, parameters, presets, rhythms and whole live track",evolve_ok);
    check("evolve changes backing notes",memcmp(candidate.t[2].step,repeated.t[2].step,sizeof candidate.t[2].step)!=0);
    starter();ui_prefs=PREF_RESTORE_OFF|PREF_ANIM_OFF;settings_hold=3;favorites.factory[0][1]=1;
    uint32_t preserved=0x424c4553u,readback=0;
    st_save(OBJ_PROJECT0,&preserved,sizeof preserved);st_save(OBJ_UPRESET0,&preserved,sizeof preserved);
    uint32_t reset_erases=erases;holy.request=3;
    for(uint32_t i=0;i<80;i++){holy_poll();mix_block(out,CTL);fm1_ms++;}
    check("reset returns stopped to Evening Vow, default settings, live T4 and Permutation screen",!holy.phase&&!holy.status&&!song.playing&&song.sel==3&&holy.screen&&!ui_prefs&&settings_hold==HOLD_DEF&&!strcmp(proj_name,"EVENING VOW"));
    check("reset preserves factory favorites and saved project/preset sectors",favorites.factory[0][1]==1 && st_load(OBJ_PROJECT0,&readback,sizeof readback)==sizeof readback && readback==preserved && st_load(OBJ_UPRESET0,&readback,sizeof readback)==sizeof readback && readback==preserved);
    fm1_ms+=AS_IDLE_MS+AS_POLL_MS+1;autosave_poll();
    check("reset replaces autosave for next power-on",as.saved==autosave_sig()&&erases>reset_erases);
    /* Live actions keep the clock, sequencer gates, voice ownership and transport intact. */
    for(uint32_t action=4;action<=5;action++) {
        starter();holy.status=0;transport_req=1;mix_block(out,CTL);
        for(uint32_t i=0;i<12;i++)mix_block(out,CTL);
        uint32_t step_before=trk[0].seq_pos;
        project_capture(&repeated);fm1_in.notes=1;holy.seed=2468+action;holy.request=action;
        holy_poll();
        check(action==4?"live evolve never requests STOP or resets sequencer while live key is held":"live LFO roll never requests STOP or resets sequencer while live key is held",song.playing && !transport_req && !holy.phase && !holy.status && trk[0].seq_pos==step_before && !trk[0].xf_on);
        fm1_in.notes=0;
        project_capture(&candidate);
        check("live actions retain presets and current track-4 note sequence",candidate.t[0].engine==repeated.t[0].engine && candidate.t[0].preset==repeated.t[0].preset && !memcmp(candidate.t[3].step,repeated.t[3].step,sizeof candidate.t[3].step));
        uint64_t live_energy=0;for(uint32_t i=0;i<80;i++){mix_block(out,CTL);for(uint32_t j=0;j<CTL*2;j++)live_energy+=abs(out[j]);}
        check("live action emits continuous audio",live_energy>0);
        check("live action stays playing after subsequent audio blocks",song.playing && !transport_req);
    }
    starter();transport_req=1;mix_block(out,CTL);holy.request=1;holy.seed=13579;
    for(uint32_t frame=0;frame<8;frame++) {
        holy_poll();
        /* Model a slow frame/display while the audio ISR continues serving blocks. */
        for(uint32_t block=0;block<350;block++)mix_block(out,CTL);
        fm1_ms+=250;
    }
    check("complete revelation resumes playback despite slow main-loop frames",song.playing && !holy.phase && !holy.status);
    int render_ok=1; uint32_t covered=0;
    for(uint32_t n=0;n<60;n++) {
        starter();project_capture(&candidate);holy_recipe_t r={0,(n%4)+1,n%3,n%2?127:0,0,0x100000u+n};
        holy_generate(&candidate,&r,&root,&scale);
        if(n%3==1)holy_lfo_roll(&candidate,r.seed+77);
        if(n%3==2){holy_lfo_roll(&candidate,r.seed+77);holy_evolve(&candidate,r.seed+123,&root,&scale);}
        project_restore_runtime(&candidate);song.master_q12=2048;
        for(uint32_t k=0;k<4;k++)covered|=1u<<trk[k].eng_req;
        uint64_t energy=0;int32_t peak=0;uint32_t clips=0,maxvoice=0;
        FILE *w=NULL;char path[80];if(n<4){snprintf(path,sizeof path,"build/host/HolyRND-%u.wav",n);w=fopen(path,"wb");wav_hdr(w,FS*90);}
        transport_req=1;
        for(uint32_t f=0;f<FS*90;f+=CTL) {
            if(f==FS*30)input_on(&trk[3],holy_near(root,n%4,72,60,84),90);
            if(f==FS*35)input_off(&trk[3],holy_near(root,n%4,72,60,84));
            mix_block(out,CTL);uint32_t voices=0;
            for(uint32_t k=0;k<4;k++)for(uint32_t v=0;v<NVOICE;v++)voices+=trk[k].v[v].active!=0;
            if(voices>maxvoice)maxvoice=voices;
            for(uint32_t j=0;j<CTL*2;j++){int32_t a=abs(out[j]);if(a>peak)peak=a;clips+=a>=32767;if(f<FS*10)energy+=a;}
            if(w)for(uint32_t j=0;j<CTL;j++)wav_put(w,out[j*2],out[j*2+1]);
        }
        if(w)fclose(w);transport_req=2;mix_block(out,CTL);
        render_ok &= energy>FS && clips==0 && maxvoice<=8 && !song.playing;
        for(uint32_t k=0;k<4;k++)render_ok &= !trk[k].seq_n;
        printf("holy render seed=%u scale=%u rite=%u peak=%d clips=%u maxvoices=%u\n",r.seed,scale,r.rite,peak,clips,maxvoice);
    }
    check("60 ninety-second DSP renders: early audio, live input, no clipping, <=8 voices, clean STOP",render_ok);
    check("render corpus exercises every engine family",covered==families);
    return bad?1:0;
}

#endif /* HOLY_TEST_NO_MAIN */
