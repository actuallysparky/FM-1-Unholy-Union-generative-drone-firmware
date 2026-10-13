/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercises the exact RAM driver against a status/command/timing model.
 * No hardware timing claim: target MMIO/SPI latency needs instrument testing. */
#include <stdint.h>
#include <stdio.h>
#define RAMFN
#define FL_IN(off,n,lo,hi) ((off)>=(lo) && (off)<=(hi) && (n)<=(hi)-(off))
typedef struct {uint32_t unused;} fl_saved_t;
static uint32_t now,remaining,finish,started,op_started,cmd,bytes,op_us=500;
static int busy,sus,xip=1,wel,denied,ignored,exits,invalidates,bad;
static uint32_t ticks(void){now+=5;return now*24;}
#define T4_CNT ticks()
static int fl_enter(fl_saved_t *s){(void)s;xip=0;return 0;}
static void fl_exit(fl_saved_t *s){(void)s;if(busy && !sus)bad++;xip=1;exits++;}
static void update(void){now+=2;if(busy && !sus && now>=finish){busy=0;wel=0;}}
static uint8_t fl_rdsr(uint8_t c){update();return c==0x35?(sus?0x84:0):(busy && !sus?1:0)|(wel && !sus?2:0);}
static void fl_cs(int high){
 if(!high){cmd=0;bytes=0;return;}
 if(cmd==0x20 || cmd==0x02){busy=1;sus=0;finish=now+op_us;started=op_started=now;}
 if(cmd==0x75 && busy && !ignored){if(now-started<20)bad++;/* Below 100us, emulate a part that restarts its internal pulse and makes no progress. */
     remaining=now-started<100u?finish-op_started:(finish>now?finish-now:0);sus=1;}
 if(cmd==0x7A && sus){sus=0;busy=1;finish=now+remaining;started=op_started=now;}
}
static void fl_tx(uint8_t b){now+=1;if(!bytes++)cmd=b;}
static void fl_addr(uint32_t off){(void)off;now+=3;}
static void fl_wren(void){wel=!denied;}
static void fl_inval(uint32_t off,uint32_t n){(void)off;(void)n;invalidates++;}
#include "../firmware/hal/fm1_flash_live.h"
#define CHECK(x) do{if(!(x)){printf("FAIL line %d\n",__LINE__);bad++;}}while(0)
int main(void){
 uint8_t data[128]={0};int rc;
 rc=fl_live_step_ram(0x97000,0,0,0);CHECK(rc==1 && sus && xip);
 unsigned turns=0;
 while(rc==1 && turns++<100){now+=100;rc=fl_live_step_ram(0x97000,0,0,1);}
 CHECK(rc==0 && !busy && !sus && turns>1 && turns<10 && invalidates==1);
 rc=fl_live_step_ram(0x97100,data,128,0);CHECK(rc==1 && xip);
 rc=fl_live_step_ram(0x97100,data,128,2);CHECK(rc==-5 && !busy && !sus && xip);
 denied=1;CHECK(fl_live_step_ram(0x97100,data,128,0)==-3 && xip);denied=0;
 ignored=1;CHECK(fl_live_step_ram(0x97100,data,128,0)==-4 && !busy && xip);ignored=0;
 op_us=10;CHECK(fl_live_step_ram(0x97100,data,128,0)==0 && !busy && xip);
 int before=exits;
 CHECK(fl_live_step_ram(0xA0000,data,128,0)==-1 && exits==before);
 CHECK(fl_live_step_ram(0x97001,0,0,0)==-1 && exits==before);
 CHECK(fl_live_step_ram(0x971F0,data,128,0)==-1 && exits==before);
 CHECK(fl_live_step_ram(0x97100,data,256,0)==-1 && exits==before);
 printf("flash live driver %s\n",bad?"FAILED":"passed");return !!bad;
}
