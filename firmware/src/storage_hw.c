/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* The firmware side of storage.c (FELUCCA_FLASH): flash reads in short IRQ-off windows so the
 * audio keeps up, erases and writes only inside the storage areas, the audio buffer silenced
 * first. The host tests (storage_test.c) provide their own st_* on a simulated NOR. */
#define ST_LIVE_SAVE 1
#include "fm1_flash_live.h"
static uint8_t st_live_busy; /* storage.c job; blocks every other flash writer */
static uint32_t st_live_block_base;
static int st_live_step(uint32_t off, const uint8_t *src, uint32_t n, uint32_t resume)
{
    uint32_t f = irq_save();
    int rc = FL_FAR(fl_live_step_ram)(off, src, n, resume);
    st_live_block_base=rc==1 ? off & ~4095u : 0u;
    irq_restore(f);
    return rc;
}
static uint8_t flash_ok;                 /* JEDEC id matched at boot (persist_boot) */
static int st_read(uint32_t off, void *dst, uint32_t n)   /* 256-byte IRQ-off windows: audio keeps up */
{
    if(st_live_block_base && off < st_live_block_base+4096u && n && off+n>st_live_block_base)return -10;
    uint8_t *d = dst;
    while (n) {
        uint32_t k = n > 256u ? 256u : n, f = irq_save();
        int rc = FL_FAR(fl_read_ram)(off, d, k);
        irq_restore(f);
        if (rc)
            return rc;
        off += k;
        d += k;
        n -= k;
    }
    return 0;
}
static void audio_silence(void)                 /* IRQs off: the DMA would loop stale audio (buzz) */
{
    uint32_t i;
    for (i = 0; i < sizeof abuf / sizeof abuf[0]; i++)
        abuf[i] = 0;
}
static int st_erase(uint32_t off)
{
    uint32_t took, f;
    int rc;
    if (st_live_busy) return -10;
    if (!FL_STORE_OK(off, 0x1000u))
        return -8;
    f = irq_save();
    audio_silence();
    rc = FL_FAR(fl_erase4k_ram)(off, &took);
    irq_restore(f);
    return rc;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    if (st_live_busy) return -10;
    if (!FL_STORE_OK(off, n))                   /* only inside the storage areas */
        return -8;
    return fl_write(off, src, n);
}
