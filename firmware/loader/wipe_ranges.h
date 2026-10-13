/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit user-store erase allowlist. Never includes app, boot head or OTA staging/records. */
static uint8_t ldr_wipe_enabled;
#define LDR_WIPE_OK(off,n) (FL_IN(off,n,0x97000u,0xE0000u) || FL_IN(off,n,0xE5000u,0xE8000u) || FL_IN(off,n,0xFC000u,0xFF000u))
