/* SPDX-License-Identifier: GPL-3.0-only */
/* Real generator invariants: balance changes gains only, never presets/notes/state. */
#define HOLY_TEST_NO_MAIN 1
#define HOLY_CHOIR_MEASURE 1
#include "holy_test.c"
#include "../firmware/src/holy_mix.c"
int main(void) {
    memset(nor,0xff,sizeof nor);starter();holy_factory_project(&candidate);
    int ok=1;uint8_t root,scale;uint32_t changed=0;
    for(uint32_t seed=0;seed<2000;seed++) {
        holy_recipe_t r={seed%13,seed%5,seed%3,seed%2?127:0,0,seed};
        holy_factory_project(&candidate);ok &= !holy_generate(&candidate,&r,&root,&scale);
        repeated=candidate;evolved=candidate;
        holy_balance(&repeated,&r);holy_balance(&evolved,&r);
        ok &= !memcmp(&repeated,&evolved,sizeof repeated);
        for(uint32_t k=0;k<4;k++) {
            ok &= repeated.t[k].p[P_LEVEL]>=32&&repeated.t[k].p[P_LEVEL]<=96;
            changed+=repeated.t[k].p[P_LEVEL]!=candidate.t[k].p[P_LEVEL];
            repeated.t[k].p[P_LEVEL]=candidate.t[k].p[P_LEVEL];
        }
        ok &= !memcmp(&repeated,&candidate,sizeof repeated);
        for(uint32_t k=0;k<4;k++)candidate.t[k].p[P_LEVEL]=evolved.t[k].p[P_LEVEL];
        candidate.sum=proj_sum(&candidate);ok &= proj_ok(&candidate)&&proj_engines_ok(&candidate);
        repeated=candidate;ok &= !holy_evolve(&repeated,seed+1,&root,&scale);
        for(uint32_t k=0;k<4;k++)ok &= repeated.t[k].p[P_LEVEL]==candidate.t[k].p[P_LEVEL];
        repeated=candidate;holy_lfo_roll(&repeated,seed+2);
        for(uint32_t k=0;k<4;k++)ok &= repeated.t[k].p[P_LEVEL]==candidate.t[k].p[P_LEVEL];
    }
    check("2000 identical recipes: deterministic bounded gains, only LEVEL changes, valid project, evolve/LFO preserve faders",ok&&changed>1000);
    check("five-register energy and peak profiles fit 5040 bytes",sizeof HOLY_MIX_ENERGY64+sizeof HOLY_MIX_ENERGY96+sizeof HOLY_MIX_PEAK==5040);
    check("register interpolation clamps edges and retains measured anchors",holy_profile(HOLY_MIX_ENERGY96[0][1][0],0)==HOLY_MIX_ENERGY96[0][1][0][0]&&holy_profile(HOLY_MIX_ENERGY96[0][1][0],127)==HOLY_MIX_ENERGY96[0][1][0][4]&&holy_profile(HOLY_MIX_ENERGY96[0][1][0],48)==HOLY_MIX_ENERGY96[0][1][0][1]);
    return bad?1:0;
}
