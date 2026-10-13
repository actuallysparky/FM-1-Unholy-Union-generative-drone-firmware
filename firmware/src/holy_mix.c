/* SPDX-License-Identifier: GPL-3.0-only */
/* The Choir Master: generation-time gain decisions, never audio ISR processing.
 * Tables measure the real pitched presets, including the generator's drone chord.
 * Pitch/velocity interpolation is a bounded perceptual estimate, not a loudness meter.
 * Existing preset/phrase RNG draws remain untouched; this uses its own mix stream. */
#include "holy_mix_profiles.h"
static int32_t holy_profile(const int8_t v[5],int32_t note) {
    note=clamp(note,36,84)-36;uint32_t i=(uint32_t)note/12u;
    if(i>=4)return v[4];
    return v[i]+((int32_t)v[i+1]-v[i])*(note%12)/12;
}
static void holy_balance(project_t *p,const holy_recipe_t *r) {
    /* Weighted active energy targets in half-dB; different jobs, not equal loudness. */
    static const int8_t goal[4]={-56,-60,-66,-54},base[4]={82,80,74,82};
    static const int8_t peak_ceiling[4]={-30,-32,-38,-28};
    int32_t peaks[4];uint32_t random=holy_stream(r->seed,0x43484f49u);
    for(uint32_t k=0;k<4;k++) {
        proj_trk_t *t=&p->t[k];uint32_t family=0,mode=k==0?0:k==3?2:1,count=0,total=0,vel=0;
        while(family<7&&HOLY_ENGINE[family]!=t->engine)family++;
        if(family==7 || t->preset>=16)return; /* generator invariant; never rebalance arbitrary projects */
        for(uint32_t i=0;i<NSTEP;i++)if(t->step[i].time==ST_NOTE&&t->step[i].n) {
            /* Sustained chord profile uses its bass/root; melody/arp average active notes. */
            uint32_t n=k==0?1:t->step[i].n;
            for(uint32_t j=0;j<n;j++){total+=t->step[i].note[j];vel+=t->step[i].vel;count++;}
        }
        int32_t note=count?(int32_t)(total/count):60+t->p[P_ROOT];
        int32_t velocity=count?(int32_t)(vel/count):80;
        int32_t a=holy_profile(HOLY_MIX_ENERGY64[family][t->preset][mode],note);
        int32_t b=holy_profile(HOLY_MIX_ENERGY96[family][t->preset][mode],note);
        int32_t energy=a+(b-a)*(clamp(velocity,64,96)-64)/32;
        int32_t density=k>0&&k<3&&count>16&&t->p[P_REL]>40?2:0;
        int32_t level=112+goal[k]-energy-density+(int32_t)holy_bound(&random,3)-1;
        level=clamp(level,base[k]-18,base[k]+6); /* nominal calibration -9/+3 dB */
        peaks[k]=holy_profile(HOLY_MIX_PEAK[family][t->preset][mode],note)+(k==3?6:0);
        /* Peak reserve may attenuate beyond the nominal trim; never increase a clipped voice. */
        if(level+peaks[k]-112>peak_ceiling[k])level=112+peak_ceiling[k]-peaks[k];
        t->p[P_LEVEL]=(int16_t)clamp(level,32,96);
    }
    /* Reserve aggregate room for live playing and FX. Coherent sample-peak bound,
     * not a post-limiter clipping check; conservative and bounded to six extra dB. */
    for(uint32_t pass=0;pass<12;pass++) {
        uint32_t sum=0;for(uint32_t k=0;k<4;k++)sum+=LEVEL_Q12[clamp(p->t[k].p[P_LEVEL]+peaks[k],1,127)];
        if(sum<=1536)break;
        for(uint32_t k=0;k<4;k++)if(p->t[k].p[P_LEVEL]>32)p->t[k].p[P_LEVEL]--;
    }
}
