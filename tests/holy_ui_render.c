/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_RENDER_NO_MAIN 1
#include "ui_render.c"
int main(void) {
 rep=fopen("build/host/holy-ui-lint.txt","w");
 setup(S_HOME_IDLE);holy.screen=1;holy.scale=4;holy.rite=2;holy.key=12;holy.stir=127;
 holy.seed=0xffffffff;holy.has_seed=1;ui.force=1;
 ui_draw();write_ppm("build/host/holy-ui","GREEN","HolyRND");
 printf("HolyRND UI: findings=%u inks=%u\n",nfind,nink);fclose(rep);return nfind||nink;
}
