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
int main(void) {
 int32_t out[2*CTL],peak=0; uint32_t clipped=0,maxvoices=0; uint64_t energy=0;
 memset(nor,0xFF,sizeof nor);starter();
 check("fresh boot is stopped and recording disarmed",!song.playing&&!song.rec);
 check("A-minor drone is three notes followed by ties",trk[0].step[0].n==3&&trk[0].step[0].note[0]==45&&trk[0].step[31].time==ST_TIE);
 check("melody and recorded chord arpeggio need no keyboard latch",trk[1].step[2].note[0]==64&&trk[2].step[8].note[0]==57&&!trk[2].p[P_AMODE]&&!trk[2].p[P_AHOLD]);
 check("user motif develops over sixteen quarter-note bars",trk[1].p[P_SLEN]==64&&trk[1].p[P_SDIV]==0&&trk[1].step[28].note[0]==60&&trk[1].step[60].note[0]==45);
 check("pad leads sequence by two beats and arp by four",trk[1].step[0].time==ST_REST&&trk[1].step[1].time==ST_REST&&trk[1].step[2].time==ST_NOTE&&trk[2].step[7].time==ST_REST&&trk[2].step[8].time==ST_NOTE);
 check("track four is an audible live PHYS BOWED METAL with an empty sequence",trk[3].eng_req==ENGI_PHYS&&trk[3].preset==3&&trk[3].p[P_LEVEL]>0&&!trk[3].p[P_MUTE]&&trk[3].p[P_VOICE]==V_MONO);
 {int empty=1;for(uint32_t i=0;i<NSTEP;i++)empty&=trk[3].step[i].time==ST_REST&&!trk[3].step[i].n&&!trk[3].step[i].hit;check("every live-track step stays empty",empty);}
 for(uint32_t i=0;i<NTRK;i++)for(uint32_t k=0;k<P_COUNT;k++){const param_desc_t*d=param_desc_of(trk[i].eng_req,k);if(trk[i].p[k]<d->min||trk[i].p[k]>d->max)bad++;}
 for(uint32_t i=0;i<FS;i+=CTL){mix_block(out,CTL);for(uint32_t j=0;j<2*CTL;j++)energy+=(uint64_t)abs(out[j]);}
 check("no sound before PLAY",energy==0);
 project_capture(&proj_scratch);check("starter project packs as current FUNB",proj_pack(&proj_wire,&proj_scratch));
 FILE*p=fopen("build/host/Evening-Vow.fun","wb");if(!p)return 2;fwrite(&proj_wire,1,sizeof proj_wire,p);fclose(p);
 check("wire project imports",proj_import(&proj_scratch,&proj_wire,sizeof proj_wire));
 check("import retains three parts and project name",!memcmp(proj_scratch.name,"EVENING VOW",11)&&proj_scratch.t[2].step[15].note[0]==60);
 FILE*w=fopen("build/host/Evening-Vow.wav","wb");if(!w)return 2;wav_hdr(w,FS*60);transport_req=1;energy=0;
 for(uint32_t f=0;f<FS*60;f+=CTL){mix_block(out,CTL);uint32_t voices=0;
  for(uint32_t t=0;t<NTRK;t++)for(uint32_t v=0;v<NVOICE;v++)voices+=trk[t].v[v].active!=0;
  if(voices>maxvoices)maxvoices=voices;
  for(uint32_t j=0;j<2*CTL;j++){int32_t a=abs(out[j]);if(a>peak)peak=a;clipped+=a>=32767;energy+=a;}
  for(uint32_t j=0;j<CTL;j++)wav_put(w,out[2*j],out[2*j+1]);
 }fclose(w);
 printf("starter: 60-second DSP render peak=%d clipped=%u max-active-voices=%u\n",peak,clipped,maxvoices);
 check("PLAY produces audible audio",energy>FS*60u);
 check("shared eight-voice budget retained",maxvoices<=8);
 check("draft has no clipped output samples",clipped==0);
 transport_req=2;mix_block(out,CTL);
 check("STOP releases every sequenced gate",!song.playing&&!trk[0].seq_n&&!trk[1].seq_n&&!trk[2].seq_n);
 starter();song.sel=3;input_on(&trk[3],76,90);energy=0;
 for(uint32_t f=0;f<FS;f+=CTL){mix_block(out,CTL);for(uint32_t j=0;j<2*CTL;j++)energy+=(uint64_t)abs(out[j]);}
 check("live track sounds from key input without transport",energy>FS&&!song.playing);
 input_off(&trk[3],76);
 starter();song.g[G_BPM]=101;trk[0].step[0].note[0]=50;str_cpy(proj_name,"USER MUSIC",sizeof proj_name);song.macro[0]=37;
 project_capture(&proj_scratch);proj_pack(&proj_wire,&proj_scratch);
 check("user autosave stored in simulated NOR",st_save(OBJ_AUTOSAVE,&proj_wire,sizeof proj_wire)==0);
 starter();ui_prefs=0;check("valid autosave overrides the starter",autosave_boot(1)==1&&song.g[G_BPM]==101&&trk[0].step[0].note[0]==50&&song.macro[0]==37&&!strcmp(proj_name,"USER MUSIC"));
 starter();check("crash recovery keeps starter and skips autosave",autosave_boot(0)==0&&song.g[G_BPM]==72);
 starter();ui_prefs=PREF_RESTORE_OFF;check("restore-last OFF keeps starter",autosave_boot(1)==0&&song.g[G_BPM]==72);
 return bad?1:0;
}
