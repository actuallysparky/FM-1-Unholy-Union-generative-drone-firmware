/* SPDX-License-Identifier: GPL-3.0-only */
/* Real checkpoint/job + commit-last persistence, with delayed NOR operations.
 * Models callback separation and fault/torn writes, not target suspend timing. */
#include <stdint.h>
#define ST_LIVE_SAVE 1
static int st_live_step(uint32_t off,const uint8_t *src,uint32_t n,uint32_t resume);
#define HOLY_TEST_NO_MAIN 1
#include "holy_test.c"
static uint32_t slices, remain, target, size;
static uint8_t pending_page[128];
static int live_failure;
static int st_live_step(uint32_t off,const uint8_t *src,uint32_t n,uint32_t resume)
{
    slices++;
    if(live_failure){live_failure=0;return -4;}
    if(resume==2u)return -5;
    if(!resume){target=off;size=n;remain=((n?3000u:20000u)+119u)/120u;if(n)memcpy(pending_page,src,n);}
    if(--remain)return 1;
    if(size)return st_prog(target,pending_page,size);
    return st_erase(target);
}
static int wait_save(void)
{
    uint32_t turns=0;
    while(checkpoint_pending && turns++<10000u) {
        int32_t audio[CTL*2];events_block(CTL);mix_block(audio,CTL);
        fm1_ms++;project_checkpoint_poll();
    }
    return !checkpoint_pending && turns>900u && turns<1300u;
}
int main(void)
{
    memset(nor,255,sizeof nor);starter();song.playing=1;holy.screen=1;
    project_t snapshot, after, restored;
    project_capture(&snapshot);
    holy.request=6;holy_poll();
    check("checkpoint starts with transport/voices untouched", checkpoint_pending && st_live_busy && song.playing && !transport_req);
    project_capture(&after);check("snapshot capture leaves current project intact",!memcmp(&snapshot,&after,sizeof after));
    check("ordinary writes cannot clobber live job",st_save(OBJ_SETTINGS,&snapshot,32)==-10);
    project_checkpoint_poll();uint32_t same_ms=slices;
    for(uint32_t i=0;i<100;i++)project_checkpoint_poll();
    check("one flash slice at most per millisecond",slices==same_ms);
    /* Edit after capture: the saved sermon must still contain the captured notes. */
    trk[1].step[2].note[0]=12;trk[0].p[P_LEVEL]=17;
    check("save completes over separate audio-service turns",wait_save() && song.playing && !transport_req);
    int n=st_load(OBJ_PROJECT0,&proj_wire,sizeof proj_wire);
    check("durable verified checkpoint survives simulated reload",n==sizeof proj_wire && proj_import(&restored,&proj_wire,n));
    check("immutable snapshot retains original notes and fader",restored.t[1].step[2].note[0]==snapshot.t[1].step[2].note[0] && restored.t[0].p[P_LEVEL]==snapshot.t[0].p[P_LEVEL] && !strcmp((char*)restored.name,"SERMON A"));
    for(uint32_t i=1;i<4;i++){project_checkpoint();check("next empty chosen",checkpoint_slot==i && wait_save());}
    uint32_t before=slices;project_checkpoint();
    check("full bank asks for choice without overwrite",ui.checkpoint_choose && !checkpoint_pending && before==slices && cur_page()->graph==GR_SLOTS && ui.act==4);
    /* The selected overwrite must preserve old data on errors/torn writes. */
    n=st_load(OBJ_PROJECT0,&proj_wire,sizeof proj_wire);project_store_t old=proj_wire;
    project_checkpoint_to(0);live_failure=1;project_checkpoint_poll();
    check("failed save reports failure and retains old copy",!checkpoint_pending && !strcmp(ui.msg,"SESSION NOT SAVED") && holy.save_state==3 && holy.save_error==-4 && st_load(OBJ_PROJECT0,&proj_wire,sizeof proj_wire)==sizeof proj_wire && !memcmp(&old,&proj_wire,sizeof old));
    project_checkpoint_to(0);
    for(uint32_t i=0;i<20;i++)project_checkpoint_poll();
    check("power cut before commit keeps old checkpoint",st_load(OBJ_PROJECT0,&proj_wire,sizeof proj_wire)==sizeof proj_wire && !memcmp(&old,&proj_wire,sizeof old));
    check("eventual overwrite verifies new A/B generation",wait_save());
    /* Reproduce the former 15ms LCD cadence with realistic 20ms erase/3ms page timing. */
    n=st_load(OBJ_PROJECT0,&proj_wire,sizeof proj_wire);old=proj_wire;
    project_checkpoint_to(0);
    for(uint32_t i=0;checkpoint_pending && i<1000u;i++){fm1_ms+=15u;project_checkpoint_poll();}
    check("old frame-only cadence times out without a false saved message",!checkpoint_pending && holy.save_state==3 && holy.save_error==-5);
    check("timed out save preserves prior durable copy",st_load(OBJ_PROJECT0,&proj_wire,sizeof proj_wire)==sizeof proj_wire && !memcmp(&old,&proj_wire,sizeof old));
    printf("%s\n",bad?"LIVE SAVE FAILED":"live save test passed");return !!bad;
}
