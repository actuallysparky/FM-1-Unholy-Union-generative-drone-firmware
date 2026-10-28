/* SPDX-License-Identifier: GPL-3.0-only */
/* Focused actual firmware layout lint and visual output for Immaculate Permutation. */
#define UI_RENDER_NO_MAIN 1
#include "ui_render.c"
static int led_of(const uint8_t *l,uint32_t id){uint32_t q=led_pos[id];return q!=0xFF && ((l[q>>3]>>(q&7u))&1u);}
static int has_text(const char *text) {
    for(uint32_t i=0;i<nscr;i++)if(str_eq(scr[i].s,text))return 1;
    return 0;
}
int main(void) {
    int navigation_ok=1;
    rep=fopen("build/host/permutation-ui-lint.txt","w");if(!rep)return 2;
    for(uint32_t p=0;p<NPALETTES;p++)for(uint32_t scale=0;scale<5;scale++) {
        state();pal(p);holy.screen=1;holy.key=12;holy.scale=scale;holy.rite=2;holy.stir=127;
        holy.has_seed=1;holy.root=11;holy.scale_id=5;holy.seed=0xffffffffu;holy.tempo=110;
        cur_name="permutation";nscr=npend=nrules=ncells=0;ui.force=1;ui_draw();lint();
        navigation_ok &= has_text("TAP OCT-: FULL EDITOR");
        if(p==0 && scale==4)write_ppm("build/host/permutation-ui","GREY","screen");
        for(uint32_t prayer=0;prayer<16;prayer++) {
            nscr=npend=nrules=ncells=0;holy.prayer=prayer;holy.pulse=1;holy.pulse_ms=fm1_ms;ui.force=1;ui_draw();lint();
            navigation_ok &= has_text("TAP OCT-: FULL EDITOR");
        }
    }
    /* A shimmer frame writes only the cross canvas; composition paints only a small status line. */
    state();holy.screen=1;ui.force=1;ui_draw();
    nscr=npend=nrules=ncells=0;blit_px=0;ui.force=0;
    holy.pulse=1;holy.pulse_ms=fm1_ms-180;holy.pulse_frame=0;ui_draw();
    int paint_ok=blit_px==40u*66u;
    nscr=npend=nrules=ncells=0;blit_px=0;holy.phase=2;ui.force=1;ui_draw();
    paint_ok &= blit_px<=240u*20u;holy.phase=0;
    state();song.playing=0;holy.screen=1;holy.welcome=1;holy.welcome_ms=fm1_ms;pal(UI_DEFAULT_INDEX);
    for(uint32_t age=0;age<=2600;age+=40u) {
        fm1_ms=holy.welcome_ms+age;nscr=npend=nrules=ncells=0;ui.force=!age;blit_px=0;ui_draw();
        if(age>=1800u && age<2600u)paint_ok &= blit_px<240u*120u;
        /* Fading intentionally covers the previous page: lint the prompt at rest. */
        if(!age){navigation_ok &= has_text("TAP OCT- FOR FULL EDITOR");lint();write_ppm("build/host/permutation-welcome","GILT SIN","screen");}
    }
    paint_ok &= !holy.welcome;
    /* Wall-clock feedback must outlive the generic 40-frame message countdown. */
    state();holy.screen=1;holy.save_state=1;ui_message("SAVING SESSION");
    nscr=npend=nrules=ncells=0;ui_draw();lint();
    navigation_ok &= has_text("TAP OCT-: FULL EDITOR");
    write_ppm("build/host/permutation-saving","GILT SIN","screen");
    for(uint32_t i=0;i<100;i++){fm1_ms+=16u;ui_draw();}
    paint_ok &= str_eq(ui.msg,"SAVING SESSION") && ui.msg_t && holy.save_state==1;
    holy.save_state=2;holy.save_slot=2;holy.save_until=fm1_ms+3000u;ui.force=1;
    for(uint32_t i=0;i<175;i++){fm1_ms+=16u;ui_draw();}
    paint_ok &= str_eq(ui.msg,"SESSION SAVED") && ui.msg_t && holy.save_state==2;
    fm1_ms=holy.save_until;ui_draw();paint_ok &= !holy.save_state && !ui.msg_t;
    const uint32_t layout_findings=nfind;
    /* Interactive flow uses incremental draws; layout lint above covers complete controlled frames. */
    FILE *layout_report=rep;rep=fopen("/dev/null","w");
    nscr=npend=nrules=ncells=0;
    state();holy.screen=1;hold(B_HOME);ui.menu_sel=MI_FACTORY_RESET;press(B_OCTUP);
    int ok=navigation_ok && paint_ok && !ui.menu && ui.confirm==CF_FACTORY_RESET && !holy.request;
    press(B_OCTDN);ok &= !ui.confirm && !holy.request;
    hold(B_HOME);ui.menu_sel=MI_FACTORY_RESET;press(B_OCTUP);press(B_OCTUP);
    ok &= !ui.confirm && holy.request==3;
    state();holy.screen=1;holy.request=0;btn_down(B_SAVE);frames(800);
    ok &= !holy.request;frames(500);ok &= holy.request==6;btn_up(B_SAVE);frame();
    ok &= holy.screen && holy.request==6;
    /* Exit is navigation only: playing/stopped state, selected track and project survive. */
    for(uint32_t playing=0;playing<2;playing++) {
        state();holy.screen=1;holy.request=0;song.playing=playing;song.sel=3;
        uint32_t before=autosave_sig();transport_req=0;
        press(B_OCTDN);
        ok &= !holy.screen && !holy.request && ui.home && song.playing==playing;
        ok &= song.sel==3 && autosave_sig()==before && !transport_req;
    }
    state();holy.screen=1;holy.request=0;btn_down(B_OCTDN);frames(800);
    ok &= holy.screen && holy.request==2;btn_up(B_OCTDN);frame();ok &= holy.screen;
    state();holy.screen=1;btn_down(B_ENV);press(B_OCTUP);btn_up(B_ENV);ok &= holy.request==4;
    state();holy.screen=1;btn_down(B_LFO);press(B_OCTUP);btn_up(B_LFO);ok &= holy.request==5;
    state();go_home();int octave=song.octave;btn_down(B_ENV);press(B_OCTUP);btn_up(B_ENV);frame();
    ok &= holy.request==4 && ui.home && song.octave==octave;
    state();go_home();octave=song.octave;btn_down(B_LFO);press(B_OCTUP);btn_up(B_LFO);frame();
    ok &= holy.request==5 && ui.home && song.octave==octave;
    state();go_home();btn_down(B_LFO);turn(EN_K1,1);press(B_OCTUP);btn_up(B_LFO);frame();
    ok &= holy.request==5 && !mom.n;
    state();hold(B_HOME);ui.menu_sel=MI_COLOR;btn_down(B_ENV);press(B_OCTUP);btn_up(B_ENV);
    ok &= ui.menu && !holy.request;
    state();ui.confirm=CF_CLEAR_SEQ;btn_down(B_ENV);press(B_OCTUP);btn_up(B_ENV);
    ok &= !holy.request;
    for(uint32_t on_screen=0;on_screen<2;on_screen++) {
        state();open_family(FAM_LFO);btn_down(B_LFO);turn(EN_K1,1);
        ok &= mom.n>0;holy.screen=on_screen;press(B_OCTUP);holy_poll();
        int16_t rolled=trk[0].p[P_LRATE];
        ok &= !mom.n && !holy.request && song.playing && rolled>=8 && rolled<=32;
        btn_up(B_LFO);frame();ok &= trk[0].p[P_LRATE]==rolled && song.playing;
    }
    state();holy.screen=1;holy.welcome=1;holy.welcome_ms=fm1_ms;holy.request=0;song.playing=0;
    press(B_PLAY);ok &= !holy.welcome && transport_req==1;
    holy.request=0;transport_req=0;
    state();holy.screen=1;settings_leds=LEDS_OFF;song.playing=0;ui_leds();
    /* Same four-LED-column mock as the existing UI tests; real HAL map is unchanged. */
    for(uint32_t id=0;id<41u;id++)led_pos[id]=(uint8_t)((id/4u)<<3 | (1u+id%4u));
    ui_leds();
    const uint8_t actions[]={B_HOME,B_ENV,B_LFO,B_SAVE,B_OCTDN,B_OCTUP,B_PLAY};
    for(uint32_t i=0;i<sizeof actions;i++)ok &= led_of(fm1_led,panel.btn[actions[i]]);
    song.playing=1;ui_leds();ok &= !led_of(fm1_led,panel.btn[B_PLAY]);
    settings.magic=0;settings_init();ok &= settings.palette==UI_DEFAULT_INDEX;
    settings.palette=2;settings_init();ok &= settings.palette==2;
    state();go_home();song.sel=0;press(B_OCTUP);
    ok &= song.octave==1 && song.track_octave[0]==1 && !song.track_octave[1];
    track_select(1);ok &= song.octave==0;press(B_OCTDN);
    ok &= song.octave==-1 && song.track_octave[1]==-1 && song.track_octave[0]==1;
    track_select(0);ok &= song.octave==1;
    ui.menu=1;ui.menu_sel=MI_AUTO_EVOLVE;press(B_OCTUP);
    ok &= menu_get(MI_AUTO_EVOLVE)==1 && ui.menu==1 && song.octave==1;
    ui.force=1;ui_draw();uint32_t auto_sig=ui.menu_sig;ui.force=0;
    menu_put(MI_AUTO_EVOLVE,2);ui_draw();ok &= ui.menu_sig!=auto_sig && ui.menu==1;
    state();open_family(FAM_SEQ);song.track_octave[song.sel]=1;song.octave=1;song.rec=1;song.playing=0;
    trk[song.sel].p[P_CHRD]=0;ui.cursor=0;ui.entry_open=0;
    seq_entry(1);ok &= trk[song.sel].step[0].note[0]==53;
    const uint8_t *d;uint16_t n;
    for(uint32_t i=2;i<=4;i++)ok &= get_desc(0x0300+i,&d,&n) && n==d[0] && d[1]==3;
    ok &= get_desc(0x0303,&d,&n) && n==26 && d[2]=='B' && d[16]=='A';
    ok &= get_desc(0x0304,&d,&n) && n==24 && d[2]=='B' && d[16]=='M';
    fclose(rep);rep=layout_report;
    fprintf(rep,"findings=%u controls=%d\n",layout_findings,ok);fclose(rep);
    return layout_findings || !ok;
}
