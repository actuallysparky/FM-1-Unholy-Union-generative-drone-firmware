/* SPDX-License-Identifier: GPL-3.0-only */
/* P25Q80H live project writes. Execute ONLY from RAM, with IRQs masked.
 * Datasheet v1.7 sections 5.4/10.36: resume->suspend >=20us, suspend
 * latency <=30us. Notes 4/5 require >=100us active windows for forward
 * progress under repeated suspension, beyond the 20us command minimum. Both SUS bits are checked because the datasheet swaps
 * their names between sections. XIP resumes only when ready or suspended.
 * Only project sectors are allowed: never code, samples, OTA or settings.
 * Return 1 suspended, 0 finished, negative failed. Caller retries at most
 * once per main-loop iteration, keeping IRQ/UI/USB service between slices.
 */
#define FL_LIVE_US 120u
static RAMFN int fl_live_step_ram(uint32_t off, const uint8_t *src,
                                  uint32_t n, uint32_t resume)
{
    fl_saved_t s;
    uint32_t t0, sr, sus;
    int failed = 0;
    if (!FL_IN(off, n ? n : 4096u, 0x97000u, 0x9F000u) ||
        (n ? n > 128u || (off & 255u) + n > 256u : (off & 4095u))) return -1;
    if (fl_enter(&s)) return -2;
    if (resume) {
        fl_cs(0); fl_tx(0x7A); fl_cs(1);
    } else {
        sr = fl_rdsr(0x05); sus = fl_rdsr(0x35);
        if ((sr & 1u) || (sus & 0x84u)) {
            if (sus & 0x84u) {fl_cs(0);fl_tx(0x7A);fl_cs(1);
                t0=T4_CNT;while((T4_CNT-t0)/24u<FL_LIVE_US);
            }
            while (fl_rdsr(0x05) & 1u) ;
            fl_exit(&s); return -3;
        }
        fl_wren();
        if (!(fl_rdsr(0x05) & 2u)) { fl_exit(&s); return -3; }
        fl_cs(0); fl_tx(n ? 0x02 : 0x20); fl_addr(off);
        for (uint32_t i = 0; i < n; i++) fl_tx(src[i]);
        fl_cs(1);
    }
    t0 = T4_CNT;
    /* Always respect minimum resume time even if WIP has not asserted yet. */
    while ((T4_CNT - t0) / 24u < FL_LIVE_US) ;
    if (resume==2u) {
        while (fl_rdsr(0x05) & 1u) ;
        fl_inval(off,n?n:4096u);fl_exit(&s);return -5;
    }
    if (fl_rdsr(0x05) & 1u) {
        fl_cs(0); fl_tx(0x75); fl_cs(1);
        t0 = T4_CNT;
        while ((T4_CNT-t0)/24u < 30u) ; /* full tESL/tPSL before array reads */
        do {
            sr = fl_rdsr(0x05); sus = fl_rdsr(0x35);
            if (!(sr & 1u) && (sus & 0x84u)) { fl_exit(&s); return 1; }
            if (!(sr & 1u) && !(sus & 0x84u)) break; /* completed during suspend */
        } while ((T4_CNT - t0) / 24u <= 60u);
        if (sr & 1u) {
            /* Unsupported/failed suspend: drain in RAM, report failure. Audio
             * can gap on this fault; never return to unreadable code. A stuck
             * chip deliberately leaves watchdog enabled, rather than jump XIP. */
            failed = 1;
            while (fl_rdsr(0x05) & 1u) ;
            if (fl_rdsr(0x35) & 0x84u) { fl_exit(&s); return -4; }
        }
    }
    fl_inval(off, n ? n : 4096u);
    fl_exit(&s);
    return failed ? -4 : 0;
}
