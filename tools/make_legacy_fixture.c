/* SPDX-License-Identifier: GPL-3.0-only
 * Compile with -I<unaltered Felucca>/tests -I<unaltered Felucca>/firmware/src -Ibuild/gen.
 * This generates a test record, never an application or flash package. */
#define main hostsim_main
#include <hostsim.c>
#undef main
#define PROJ_HOST 1
#include <project.c>
int main(int argc,char **argv) {
 project_t p={0};project_store_t b;unsigned i,k;
 p.magic=PROJ_MAGIC;p.size=sizeof p;p.parts=NPART;p.phys=PROJ_PHYS;
 chain_defaults(&p.chain);
 for(i=0;i<G_COUNT;i++)p.g[i]=GP[i].def;
 for(k=0;k<NTRK;k++) {p.t[k].engine=k==0?ENGI_FM6:k==1?ENGI_PHYS:k==2?11u:ENGI_DRUM;for(i=0;i<P_E0;i++)p.t[k].p[i]=TP[i].def;for(i=0;i<8;i++)p.t[k].p[P_E0+i]=i+3;}
 p.t[0].p[P_M1SRC]=MS_SH;p.t[1].p[P_M1SRC]=MS_SLEW;
 p.motion.count=2;p.motion.on=1;
 p.motion.event[0]=(motion_event_t){0,P_E3|MOTION_LOCK,-3};p.motion.event[1]=(motion_event_t){64,P_REV,55};
 memcpy(p.name,"FELUCCA OLD",11);p.sum=proj_sum(&p);if(!proj_pack(&b,&p))return 1;
 FILE*f=fopen(argv[1],"wb");if(!f)return 2;fwrite(&b,1,sizeof b,f);fclose(f);return 0;
}
