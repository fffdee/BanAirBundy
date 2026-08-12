/**
 * @file  sys_nv.c
 * @brief System NVM load/save for boot_app.
 */
#include "sys_nv.h"
#include "cdc_debug.h"
#include "otg_device_cdc.h"
#include "spi_flash.h"
#include "debug.h"
#include "type.h"
#include <string.h>

#ifndef FLASH_SECTOR_SZ
#define FLASH_SECTOR_SZ  0x1000UL
#endif

static SysNv_t g_nv;
static uint8_t g_nv_ready;
static uint8_t g_log_applied; /* deferred apply done */

static uint32_t sys_nv_crc(const SysNv_t *nv)
{
    const uint8_t *p = (const uint8_t *)nv;
    uint32_t crc = 0xFFFFFFFFu;
    unsigned i;
    unsigned n = (unsigned)(sizeof(SysNv_t) - sizeof(uint32_t));

    for (i = 0; i < n; i++) {
        unsigned b;
        crc ^= p[i];
        for (b = 0; b < 8u; b++)
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
    }
    return ~crc;
}

static void sys_nv_defaults(void)
{
    memset(&g_nv, 0, sizeof(g_nv));
    g_nv.magic = SYS_NV_MAGIC;
    g_nv.version = SYS_NV_VERSION;
    g_nv.size = (uint16_t)sizeof(SysNv_t);
    g_nv.log_global = 0;
    g_nv.log_mod_mask = (uint8_t)((1u << (unsigned)CDC_DBG_MOD_COUNT) - 1u);
    g_nv.crc32 = sys_nv_crc(&g_nv);
}

static int sys_nv_valid(const SysNv_t *nv)
{
    if (!nv)
        return 0;
    if (nv->magic != SYS_NV_MAGIC)
        return 0;
    if (nv->version != SYS_NV_VERSION)
        return 0;
    if (nv->size != (uint16_t)sizeof(SysNv_t))
        return 0;
    if (sys_nv_crc(nv) != nv->crc32)
        return 0;
    return 1;
}

static int sys_nv_flash_load(SysNv_t *out)
{
    if (SpiFlashRead(SYS_NV_FLASH_ADDR, (uint8_t *)out,
                     sizeof(*out), 100) != FLASH_NONE_ERR)
        return -1;
    return 0;
}

static int sys_nv_flash_save(const SysNv_t *nv)
{
    SysNv_t tmp = *nv;

    tmp.magic = SYS_NV_MAGIC;
    tmp.version = SYS_NV_VERSION;
    tmp.size = (uint16_t)sizeof(SysNv_t);
    tmp.crc32 = sys_nv_crc(&tmp);

    SpiFlashIOCtrl(IOCTL_FLASH_UNPROTECT, "\x35\xBA\x69", 3);
    if (FlashErase(SYS_NV_FLASH_ADDR, FLASH_SECTOR_SZ) != FLASH_NONE_ERR)
        return -1;
    if (SpiFlashWrite(SYS_NV_FLASH_ADDR, (uint8_t *)&tmp,
                      sizeof(tmp), 0) != FLASH_NONE_ERR)
        return -1;

    g_nv = tmp;
    return 0;
}

void SysNv_ApplyLog(void)
{
#if CDC_DEBUG_EN
    CdcDbg_SetModMask(g_nv.log_mod_mask);
    CdcDbg_SetGlobal(g_nv.log_global ? 1 : 0);
#else
    (void)g_nv;
#endif
    g_log_applied = 1;
}

void SysNv_ApplyLogDeferred(void)
{
    if (g_log_applied || !g_nv_ready)
        return;

    if (!UsbCDC.InitOk)
        return;

    /*
     * If persisted log is ON, wait until the host asserts DTR (COM open).
     * Enabling mirror during enum / before open floods Bulk-IN and can
     * prevent the CDC device from opening on Windows.
     */
    if (g_nv.log_global && !UsbCDC.ControlLineState.DTR)
        return;

    SysNv_ApplyLog();
    DBG("[SYS] NVM log applied global=%u mask=0x%02X\n",
        (unsigned)g_nv.log_global,
        (unsigned)g_nv.log_mod_mask);
}

int SysNv_Init(void)
{
    SysNv_t loaded;

    g_nv_ready = 0;
    g_log_applied = 0;
    sys_nv_defaults();

    if (sys_nv_flash_load(&loaded) == 0 && sys_nv_valid(&loaded)) {
        g_nv = loaded;
        DBG("[SYS] NVM loaded @0x%08lX log_global=%u mask=0x%02X (deferred)\n",
            (unsigned long)SYS_NV_FLASH_ADDR,
            (unsigned)g_nv.log_global,
            (unsigned)g_nv.log_mod_mask);
    } else {
        DBG("[SYS] NVM invalid/empty @0x%08lX — defaults\n",
            (unsigned long)SYS_NV_FLASH_ADDR);
        sys_nv_defaults();
    }

    /* Keep CdcDbg global OFF through USB enum; ApplyLogDeferred later. */
    g_nv_ready = 1;
    return 0;
}

int SysNv_Save(void)
{
    int rc;

#if CDC_DEBUG_EN
    g_nv.log_global = CdcDbg_GetGlobal() ? 1u : 0u;
    g_nv.log_mod_mask = CdcDbg_GetModMask();
#endif

    rc = sys_nv_flash_save(&g_nv);
    if (rc == 0) {
        DBG("[SYS] NVM saved log_global=%u mask=0x%02X\n",
            (unsigned)g_nv.log_global,
            (unsigned)g_nv.log_mod_mask);
    } else {
        DBG("[SYS] NVM save FAILED\n");
    }
    return rc;
}

int SysNv_SaveLog(void)
{
    return SysNv_Save();
}

const SysNv_t *SysNv_Get(void)
{
    return &g_nv;
}
