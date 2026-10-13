/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline calibration/A-B probe. Real synth DSP; no MIDI/device or firmware writes. */
#define HOLY_TEST_NO_MAIN 1
#define HOLY_CHOIR_MEASURE 1
#include "holy_test.c"
#ifdef CHOIR_APPLY
#include "../firmware/src/holy_mix.c"
#endif

typedef struct { double b[3],a[3],z[2]; } choir_filter_t;
typedef struct { choir_filter_t f[2][2]; double chunk[4],sum,maximum; uint32_t pos,slot,filled; int64_t peak; } choir_meter_t;
/* BS.1770-4 Annex 1 tables 1/2, official ITU PDF, 48 kHz coefficients.
 * Bilinear rate conversion below retains the analogue transfer function at FS.
 * Relative maximum 400 ms energy only: not integrated/gated LUFS or true peak. */
static void choir_rate(choir_filter_t *f,const double *b,const double *a) {
    double c[2][3];
    for(uint32_t k=0;k<2;k++) {
        const double *v=k?a:b; double r=(double)FS/48000.;
        double v0=v[0]+v[1]+v[2],v1=2*(v[0]-v[2])*r,v2=(v[0]-v[1]+v[2])*r*r;
        c[k][0]=v0+v1+v2;c[k][1]=2*v0-2*v2;c[k][2]=v0-v1+v2;
    }
    for(uint32_t i=0;i<3;i++){f->b[i]=c[0][i]/c[1][0];f->a[i]=c[1][i]/c[1][0];}
}
static void choir_meter_init(choir_meter_t *m) {
    static const double b[2][3]={{1.53512485958697,-2.69169618940638,1.19839281085285},{1,-2,1}};
    static const double a[2][3]={{1,-1.69065929318241,.73248077421585},{1,-1.99004745483398,.99007225036621}};
    memset(m,0,sizeof *m);
    for(uint32_t ch=0;ch<2;ch++)for(uint32_t k=0;k<2;k++)choir_rate(&m->f[ch][k],b[k],a[k]);
}
static double choir_filter(choir_filter_t *f,double x) {
    double y=f->b[0]*x+f->z[0];f->z[0]=f->b[1]*x-f->a[1]*y+f->z[1];f->z[1]=f->b[2]*x-f->a[2]*y;return y;
}
static void choir_meter(choir_meter_t *m,int32_t l,int32_t r) {
    int64_t al=l<0?-(int64_t)l:l,ar=r<0?-(int64_t)r:r;
    if(al>m->peak)m->peak=al;if(ar>m->peak)m->peak=ar;
    double x[2]={(double)l/32768.,(double)r/32768.};
    for(uint32_t ch=0;ch<2;ch++)for(uint32_t k=0;k<2;k++)x[ch]=choir_filter(&m->f[ch][k],x[ch]);
    m->sum+=(x[0]*x[0]+x[1]*x[1])*.5;
    if(++m->pos==FS/10u) {
        m->chunk[m->slot]=m->sum;m->slot=(m->slot+1u)&3u;if(m->filled<4)m->filled++;
        if(m->filled==4){double w=0;for(uint32_t i=0;i<4;i++)w+=m->chunk[i];w/=(FS*4u/10u);if(w>m->maximum)m->maximum=w;}
        m->pos=0;m->sum=0;
    }
}
static double choir_db(const choir_meter_t *m) {return 10*log10(m->maximum+1e-12);}
static double choir_pk(const choir_meter_t *m) {return 20*log10((double)m->peak/32768.+1e-12);}
static void choir_load(uint32_t family,uint32_t preset,uint32_t mode) {
    starter();holy_factory_project(&candidate);proj_trk_t *t=&candidate.t[0];uint32_t e=HOLY_ENGINE[family];const preset_t *pr=&ENGINES[e]->presets[preset];
    t->engine=e;t->preset=preset;
    for(uint32_t i=0;i<P_COUNT;i++)t->p[i]=param_desc_of(e,i)->def;
    for(uint32_t i=0;i<8;i++)t->p[P_E0+i]=pr->e[i];
    t->p[P_ATK]=mode==0&&e!=ENGI_FM6?48:pr->env[0];t->p[P_DEC]=pr->env[1];t->p[P_SUS]=mode==0?112:pr->env[2];
    t->p[P_REL]=pr->env[3]>60?60:pr->env[3];t->p[P_ED_FLT]=pr->fenv;
    if(e==5)t->p[P_E3]=0;if(e==9&&t->p[P_E0]==3)t->p[P_E1]=127;
    if(e==8){t->p[P_E0]=0;t->p[P_E4]=0;t->p[P_E3]=60;}
    memcpy(candidate.fm6[0],e==ENGI_FM6?FM6_FACTORY[pr->e[7]]:FM6_INIT,FM6_PACKED);
    t->p[P_LEVEL]=112;t->p[P_VOICE]=mode==0 && family!=2 && family<5?V_POLY:V_MONO;
    for(uint32_t k=0;k<4;k++){memset(candidate.t[k].step,0,sizeof candidate.t[k].step);for(uint32_t i=0;i<NSTEP;i++)candidate.t[k].step[i].time=ST_REST;}
    candidate.sum=proj_sum(&candidate);project_restore_runtime(&candidate);song.master_q12=4096;
    int32_t out[CTL*2];for(uint32_t i=0;i<16;i++)mix_block(out,CTL);
}
int main(int argc,char **argv) {
    memset(nor,0xff,sizeof nor);
    if(argc==2&&!strcmp(argv[1],"inventory")) {
        for(uint32_t family=0;family<7;family++)for(uint32_t p=0;p<ENGINES[HOLY_ENGINE[family]]->npresets;p++)
            if(!(family==6&&(p==5||p==6)))printf("%u %u\n",family,p);
        return 0;
    }
    if(argc==2&&!strcmp(argv[1],"filter")) {
        for(uint32_t hz=50;hz<=10000;hz*=2){choir_meter_t m;choir_meter_init(&m);for(uint32_t i=0;i<FS;i++){int32_t x=(int32_t)(10000*sin(2*3.141592653589793*hz*i/FS));choir_meter(&m,x,x);}printf("%u %.6f\n",hz,choir_db(&m));}return 0;
    }
    if(argc==7&&!strcmp(argv[1],"profile")) {
        uint32_t family=atoi(argv[2]),preset=atoi(argv[3]),mode=atoi(argv[4]),note=atoi(argv[5]),vel=atoi(argv[6]);
        choir_load(family,preset,mode);choir_meter_t m;choir_meter_init(&m);
        uint32_t total=(mode==0?9:4)*FS,gate=mode==0?7*FS:mode==1?FS*12/100:FS*12/10;
        for(uint32_t f=0;f<total;f+=CTL) {
            uint32_t cycle=mode==1?f%(FS/2u):f;
            if(cycle<CTL){input_on(&trk[0],note,vel);if(mode==0&&trk[0].p[P_VOICE]==V_POLY){input_on(&trk[0],note+3,vel);input_on(&trk[0],note+7,vel);}}
            if(cycle>=gate&&cycle<gate+CTL){input_off(&trk[0],note);input_off(&trk[0],note+3);input_off(&trk[0],note+7);}
            memset(mix_l,0,sizeof mix_l);memset(mix_r,0,sizeof mix_r);memset(send_c,0,sizeof send_c);memset(send_d,0,sizeof send_d);memset(send_r,0,sizeof send_r);
            mix_part(&trk[0],CTL);for(uint32_t j=0;j<CTL;j++)choir_meter(&m,mix_l[j],mix_r[j]);
        }
        printf("{\"family\":%u,\"preset\":%u,\"mode\":%u,\"note\":%u,\"velocity\":%u,\"energy_db\":%.6f,\"peak_db\":%.6f}\n",family,preset,mode,note,vel,choir_db(&m),choir_pk(&m));return 0;
    }
#ifdef CHOIR_APPLY
    if(argc>=5&&!strcmp(argv[1],"mix")) {
        uint32_t seed=strtoul(argv[2],NULL,10),stir=atoi(argv[3]),balanced=atoi(argv[4]);
        starter();project_capture(&candidate);holy_recipe_t recipe={seed%13,seed%5,seed%3,stir,0,seed};uint8_t root,scale;
        if(holy_generate(&candidate,&recipe,&root,&scale))return 2;
        if(balanced)holy_balance(&candidate,&recipe);
        candidate.sum=proj_sum(&candidate);project_restore_runtime(&candidate);song.master_q12=4096;rng_state=0x1234567u;
        choir_meter_t m[5];for(uint32_t i=0;i<5;i++)choir_meter_init(&m[i]);
        int32_t out[CTL*2];uint32_t note=holy_near(root,scale==2?0:scale==3?1:scale==6?2:3,72,60,84);
        uint32_t live_velocity=argc>7?(uint32_t)atoi(argv[7]):96;if(argc>6)note=(uint32_t)atoi(argv[6]);
        FILE *wav=argc>5&&strcmp(argv[5],"-")?fopen(argv[5],"wb"):NULL;uint32_t total=(FS*30u+CTL-1u)/CTL*CTL;if(wav)wav_hdr(wav,total);
        uint32_t limiter_samples=0,maxvoices=0;transport_req=1;
        for(uint32_t f=0;f<total;f+=CTL) {
            if(f%(FS*5u)<CTL)input_on(&trk[3],note,live_velocity);
            if(f%(FS*5u)>=FS*2u&&f%(FS*5u)<FS*2u+CTL)input_off(&trk[3],note);
            memset(send_c,0,sizeof send_c);memset(send_d,0,sizeof send_d);memset(send_r,0,sizeof send_r);memset(mix_l,0,sizeof mix_l);memset(mix_r,0,sizeof mix_r);
            events_block(CTL);perf_begin(CTL);
            for(uint32_t k=0;k<4;k++){
                int32_t l[CTL],r[CTL];memcpy(l,mix_l,sizeof l);memcpy(r,mix_r,sizeof r);mix_part(&trk[k],CTL);
                for(uint32_t j=0;j<CTL;j++)choir_meter(&m[k],mix_l[j]-l[j],mix_r[j]-r[j]);
            }
            fx_buses(send_c,send_d,send_r,wet,CTL);if(hl.side)rev_side_mix(mix_l,mix_r,CTL);
            uint32_t voices=0;for(uint32_t k=0;k<4;k++)for(uint32_t i=0;i<NVOICE;i++)voices+=trk[k].v[i].active!=0;if(voices>maxvoices)maxvoices=voices;
            for(uint32_t j=0;j<CTL;j++){
                int32_t l=mix_l[j]+wet[j],r=mix_r[j]+wet[j];choir_meter(&m[4],l,r);master_out(&l,&r);
                limiter_samples+=lim_env>LIM_T;out[2*j]=l;out[2*j+1]=r;
                if(wav)wav_put(wav,l,r);
            }
        }
        if(wav)fclose(wav);
        printf("{\"seed\":%u,\"stir\":%u,\"balanced\":%u,\"levels\":[",seed,stir,balanced);
        for(uint32_t k=0;k<4;k++)printf("%s%d",k?",":"",candidate.t[k].p[P_LEVEL]);
        printf("],\"energy_db\":[");for(uint32_t k=0;k<5;k++)printf("%s%.6f",k?",":"",choir_db(&m[k]));
        printf("],\"peak_db\":[");for(uint32_t k=0;k<5;k++)printf("%s%.6f",k?",":"",choir_pk(&m[k]));
        printf("],\"limiter_samples\":%u,\"maxvoices\":%u}\n",limiter_samples,maxvoices);return 0;
    }
#endif
    fprintf(stderr,"choir_probe inventory | filter | profile FAMILY PRESET MODE NOTE VEL | mix SEED STIR BALANCED [WAV]\n");return 2;
}
