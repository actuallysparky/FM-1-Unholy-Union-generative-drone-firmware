/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
#include "../firmware/src/project.c"
static int bad;
static void uu_check(const char *s,int yes){printf("unholy: %-68s %s\n",s,yes?"ok":"FAIL");bad+=!yes;}
static void uu_fresh(unsigned e){memset(trk,0,sizeof trk);memset(&song,0,sizeof song);memset(drn,0,sizeof drn);drn[0].seed=0x6a09e667;host_tracks_init();host_preset(&trk[0],e,0);}
int main(void){
 unsigned e,i,k,ok;track_t*t=&trk[0];project_t a,b;project_store_t st;int16_t saved[P_COUNT];
 uu_check("preserved engine IDs and appended modulation IDs",ENGI_PHYS==9 && ENGI_DRUM==10 && ENGI_FM6==12 && MS_SH==9 && MS_SLEW==10 && MS_DRIFT==11 && MS_MAC1==12 && MS_MAC4==15 && P_E0==106);
 for(e=0;e<NENGINES;e++){
  if(e==1)continue;uu_fresh(e);t->p[P_EVOL]=127;t->p[P_TENS]=127;t->v[0].active=1;memcpy(saved,t->p,sizeof saved);
  for(k=0;k<1000;k++){drone_tick(t,1);for(i=0;i<drn[0].ne;i++){const param_desc_t*pd=&ENGINES[e]->edit[drn[0].ej[i]];if(pd->fmt==F_ENUM || t->p[P_E0+drn[0].ej[i]]<pd->min || t->p[P_E0+drn[0].ej[i]]>pd->max)bad++;}drone_restore(t);}
  char msg[80];snprintf(msg,sizeof msg,"engine %u targets continuous, bounded; parameters restored",e);uu_check(msg,!memcmp(saved,t->p,sizeof saved));
 }
 for(e=0;e<NENGINES;e++){
  if(e==1 || e==ENGI_DRUM)continue;uu_fresh(e);t->p[P_EVOL]=127;t->p[P_TENS]=127;
  t->p[P_SUS]=127;trk_note_on(t,48,96);trk_note_on(t,55,96);trk_note_on(t,60,96);
  int32_t samples[2*CTL];int64_t dc=0;unsigned peak=0,total=0;
  for(k=0;k<12000;k++){mix_block(samples,CTL);for(i=0;i<2*CTL;i++){int32_t v=samples[i];unsigned ab=v<0?-v:v;if(ab>peak)peak=ab;if(k>6000){dc+=v;total++;}}}
  char msg[80];snprintf(msg,sizeof msg,"engine %u active evolution/tension: bounded audio and DC",e);
  uu_check(msg,peak<=32767 && (dc<0?-dc:dc)<(int64_t)total*400);
  trk_all_off(t);for(k=0;k<20000;k++)mix_block(samples,CTL);
  unsigned active=0;for(i=0;i<NVOICE;i++)active|=t->v[i].active;
  snprintf(msg,sizeof msg,"engine %u drone note-off frees its voices",e);uu_check(msg,!active);
 }
 /* Exhaustive truth table: all four slots independently OFF, inactive destination,
  * zero amount, positive/negative active DRIFT, or another source. */
 uu_fresh(0);ok=1;
 for(unsigned pattern=0;pattern<1296;pattern++){
  unsigned n=pattern,expected=0;
  for(i=0;i<4;i++){unsigned state=n%6;n/=6;
   t->p[P_M1SRC+3*i]=state==0?0:state==5?MS_MAC1:MS_DRIFT;
   t->p[P_M1DST+3*i]=state==1?0:MD_CUT;
   t->p[P_M1AMT+3*i]=state==2?0:state==4?-31:27;
   expected|=state==3 || state==4;
  }
  if(mod_drift(t)!=expected)ok=0;
 }
 uu_check("fixed DRIFT scan equals original truth table (1296 combinations)",ok);
 uu_fresh(0);t->v[0].active=1;memcpy(saved,t->p,sizeof saved);uu_check("disabled drone returns without touching parameters",!drone_tick(t,0) && !memcmp(saved,t->p,sizeof saved));
 t->p[P_M1SRC]=MS_DRIFT;t->p[P_M1DST]=MD_CUT;t->p[P_M1AMT]=20;
 uint32_t seed=mod_seed;for(k=0;k<40000;k++){drone_tick(t,mod_drift(t));drone_restore(t);}uu_check("DRIFT moves with EVOL zero and leaves note RAND state alone",drn[0].w[0]!=0 && mod_seed==seed);
 uu_fresh(0);song.g[G_BPM]=120;t->v[0].active=1;t->p[P_TENS]=127;t->p[P_TRAMP]=1;
 for(k=0;k<FS*2u/CTL;k++){drone_tick(t,0);drone_restore(t);}uu_check("one-bar tension ramp reaches target at 120 BPM within one tick",((t->p[P_TENS]*258<<16)-drn[0].tens)<dr_ramp_step(t)*2);
 t->v[0].active=0;drone_tick(t,0);uu_check("silent ramp resets to zero",!drn[0].on && !drn[0].tens);
 t->v[0].active=1;t->xf_on=1;uu_check("engine fade excludes drone modulation",!drone_tick(t,1));
 for(i=0;i<4;i++){song.macro[i]=127;uu_check("macro is unipolar and full-scale",!MS_BIPOLAR(MS_MAC1+i) && mod_tsrc(t,MS_MAC1+i,0)==32766);song.macro[i]=0;uu_check("macro zero is zero",mod_tsrc(t,MS_MAC1+i,0)==0);}
 uu_fresh(0);t->p[P_AHOLD]=1;events_block(CTL);input_on(t,60,96);input_on(t,64,96);input_off(t,60);input_off(t,64);
 uu_check("HOLD with ARP OFF latches the chord after key-up",t->nheld==2 && !t->arp_phys && t->v[0].active);
 input_on(t,67,96);input_off(t,67);uu_check("a new chord replaces the released steady latch",t->nheld==1 && t->held[0]==67);
 t->p[P_AHOLD]=0;events_block(CTL);uu_check("HOLD off clears the steady latch",t->nheld==0);
 uu_fresh(ENGI_DRUM);t->p[P_AHOLD]=1;input_on(t,36,96);input_off(t,36);uu_check("HOLD does not latch drums",!t->nheld && !t->arp_phys);
 FILE*f=fopen("tests/fixtures/felucca-fun10.bin","rb");ok=f && fread(&st,1,sizeof st,f)==sizeof st;if(f)fclose(f);ok=ok && proj_import(&a,&st,sizeof st);
 uu_check("actual Felucca FUN10 fixture imports",ok);
 if(ok){uu_check("old PHYS/NOISE/FM6 IDs and SH/SLEW source meanings stay intact",a.t[0].engine==12 && a.t[1].engine==9 && a.t[2].engine==11 && a.t[0].p[P_M1SRC]==9 && a.t[1].p[P_M1SRC]==10);
 uu_check("legacy engine values and parameter locks migrate correctly",a.t[0].p[P_E3]==6 && MOTION_ID(&a.motion.event[0])==P_E3 && (a.motion.event[0].param&MOTION_LOCK));
 uu_check("legacy drone controls and macros default off",a.t[0].p[P_EVOL]==0 && a.t[0].p[P_TENS]==0 && a.t[0].p[P_TRAMP]==0 && !a.macro[0]);
 for(i=0;i<4;i++)a.macro[i]=i*40;a.t[0].p[P_EVOL]=77;a.sum=proj_sum(&a);
 uu_check("FUN11 round trip preserves drone, macros, FM6 and motion",proj_pack(&st,&a) && proj_import(&b,&st,sizeof st) && !memcmp(&a,&b,sizeof a));
 st.raw[PROJ_MACRO_OFF]=128;uint32_t hash=proj_hash(st.raw,sizeof st-4);memcpy(st.raw+sizeof st-4,&hash,4);uu_check("macro over range is rejected even with a valid checksum",!proj_import(&b,&st,sizeof st));
 }
 printf("resource: drone state %zu B, macro state %zu B, track %zu B (HOST ABI)\n",sizeof drn,sizeof song.macro,sizeof(track_t));
 return !!bad;
}
