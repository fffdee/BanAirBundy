/**
 * @file  cdc_debug.c
 * @brief CDC debug logger implementation.
 */
#include "cdc_debug.h"

#if CDC_DEBUG_EN

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "otg_device_cdc.h"

#ifndef CDC_DBG_OUT_BUF_SIZE
#define CDC_DBG_OUT_BUF_SIZE  192
#endif

typedef struct {
    const char *name;
} CdcDbgModInfo_t;

static const CdcDbgModInfo_t g_mods[CDC_DBG_MOD_COUNT] = {
    { "sys" },
    { "usb" },
    { "banux" },
    { "shell" },
    { "audio" },
    { "upg" },
};

/* Default: global off (avoid flooding Shell), all modules armed. */
#ifndef CDC_DBG_MIRROR_BUF_SIZE
#define CDC_DBG_MIRROR_BUF_SIZE  160
#endif

static uint8_t  g_global_en = 0;
static uint8_t  g_mod_mask  = (uint8_t)((1u << CDC_DBG_MOD_COUNT) - 1u);
static char     g_out_buf[CDC_DBG_OUT_BUF_SIZE];
static uint8_t  g_mirror_buf[CDC_DBG_MIRROR_BUF_SIZE];
static uint16_t g_mirror_len = 0;

void CdcDbg_Init(void)
{
    g_global_en = 0;
    g_mod_mask  = (uint8_t)((1u << CDC_DBG_MOD_COUNT) - 1u);
    g_mirror_len = 0;
}

void CdcDbg_SetGlobal(int enable)
{
    g_global_en = enable ? 1u : 0u;
    if (!g_global_en)
        g_mirror_len = 0;
}

int CdcDbg_GetGlobal(void)
{
    return g_global_en ? 1 : 0;
}

void CdcDbg_SetModule(CdcDbgModule_t mod, int enable)
{
    if ((unsigned)mod >= (unsigned)CDC_DBG_MOD_COUNT)
        return;

    if (enable)
        g_mod_mask |= (uint8_t)(1u << (unsigned)mod);
    else
        g_mod_mask &= (uint8_t)~(1u << (unsigned)mod);
}

int CdcDbg_IsModuleEnabled(CdcDbgModule_t mod)
{
    if ((unsigned)mod >= (unsigned)CDC_DBG_MOD_COUNT)
        return 0;
    return (g_mod_mask & (uint8_t)(1u << (unsigned)mod)) ? 1 : 0;
}

int CdcDbg_IsEnabled(CdcDbgModule_t mod)
{
    return (g_global_en && CdcDbg_IsModuleEnabled(mod)) ? 1 : 0;
}

void CdcDbg_SetAllModules(int enable)
{
    g_mod_mask = enable
        ? (uint8_t)((1u << CDC_DBG_MOD_COUNT) - 1u)
        : 0u;
}

uint8_t CdcDbg_GetModMask(void)
{
    return g_mod_mask;
}

void CdcDbg_SetModMask(uint8_t mask)
{
    g_mod_mask = (uint8_t)(mask & (uint8_t)((1u << CDC_DBG_MOD_COUNT) - 1u));
}

const char *CdcDbg_ModuleName(CdcDbgModule_t mod)
{
    if ((unsigned)mod >= (unsigned)CDC_DBG_MOD_COUNT)
        return "?";
    return g_mods[mod].name;
}

CdcDbgModule_t CdcDbg_FindModule(const char *name)
{
    unsigned i;

    if (!name)
        return CDC_DBG_MOD_COUNT;

    for (i = 0; i < (unsigned)CDC_DBG_MOD_COUNT; i++) {
        if (strcmp(g_mods[i].name, name) == 0)
            return (CdcDbgModule_t)i;
    }
    return CDC_DBG_MOD_COUNT;
}

static void CdcDbg_SendRaw(const char *str, unsigned len)
{
    if (!str || !len || !UsbCDC.InitOk)
        return;
    /* Host must have opened the port — avoid Bulk-IN flood during enum. */
    if (!UsbCDC.ControlLineState.DTR)
        return;
    OTG_DeviceCDC_Send((uint8_t *)str, (uint16_t)len);
}

void CdcDbg_Write(CdcDbgModule_t mod, const uint8_t *data, uint16_t len)
{
    if (!CdcDbg_IsEnabled(mod) || !data || !len)
        return;
    if (!UsbCDC.InitOk || !UsbCDC.ControlLineState.DTR)
        return;
    OTG_DeviceCDC_Send((uint8_t *)data, len);
}

void CdcDbg_Printf(CdcDbgModule_t mod, const char *fmt, ...)
{
    va_list args;
    int n;
    unsigned prefix_len;
    const char *name;

    if (!CdcDbg_IsEnabled(mod) || !fmt)
        return;
    if (!UsbCDC.InitOk || !UsbCDC.ControlLineState.DTR)
        return;

    name = CdcDbg_ModuleName(mod);
    prefix_len = (unsigned)snprintf(g_out_buf, sizeof(g_out_buf), "[%s] ", name);
    if (prefix_len >= sizeof(g_out_buf))
        prefix_len = sizeof(g_out_buf) - 1u;

    va_start(args, fmt);
    n = vsnprintf(g_out_buf + prefix_len,
                  sizeof(g_out_buf) - prefix_len,
                  fmt, args);
    va_end(args);

    if (n < 0)
        return;

    if ((unsigned)n >= (sizeof(g_out_buf) - prefix_len))
        n = (int)(sizeof(g_out_buf) - prefix_len - 1u);

    CdcDbg_SendRaw(g_out_buf, prefix_len + (unsigned)n);
}

void CdcDbg_MirrorChar(int c)
{
    if (!g_global_en || !UsbCDC.InitOk || !UsbCDC.ControlLineState.DTR)
        return;

    /* putchar converts '\n' → UART \r\n in one call; mirror the same. */
    if (c == '\n') {
        if (g_mirror_len + 2u > sizeof(g_mirror_buf)) {
            CdcDbg_SendRaw((const char *)g_mirror_buf, g_mirror_len);
            g_mirror_len = 0;
        }
        g_mirror_buf[g_mirror_len++] = (uint8_t)'\r';
        g_mirror_buf[g_mirror_len++] = (uint8_t)'\n';
        CdcDbg_SendRaw((const char *)g_mirror_buf, g_mirror_len);
        g_mirror_len = 0;
        return;
    }

    if (c == '\r')
        return;

    if (g_mirror_len + 1u >= sizeof(g_mirror_buf)) {
        CdcDbg_SendRaw((const char *)g_mirror_buf, g_mirror_len);
        g_mirror_len = 0;
    }
    g_mirror_buf[g_mirror_len++] = (uint8_t)c;
}

#endif /* CDC_DEBUG_EN */
