/**
 * @file  cdc_debug.h
 * @brief USB CDC debug log with compile-time and per-module runtime gates.
 *
 * Master switch: CDC_DEBUG_EN (banux_config.h).
 * Runtime: CdcDbg_SetGlobal / CdcDbg_SetModule, or Shell `log` command.
 *
 * `log -e` also mirrors platform DBG()/printf (via retarget putchar) to CDC,
 * so existing DBG logs become usable over the same serial terminal as Shell.
 */
#ifndef __CDC_DEBUG_H__
#define __CDC_DEBUG_H__

#include <stdint.h>
#include "banux_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CDC_DBG_MOD_SYS = 0,
    CDC_DBG_MOD_USB,
    CDC_DBG_MOD_BANUX,
    CDC_DBG_MOD_SHELL,
    CDC_DBG_MOD_AUDIO,
    CDC_DBG_MOD_UPG,
    CDC_DBG_MOD_COUNT
} CdcDbgModule_t;

#ifndef CDC_DEBUG_EN
#define CDC_DEBUG_EN  0
#endif

#if CDC_DEBUG_EN

void CdcDbg_Init(void);
void CdcDbg_SetGlobal(int enable);
int  CdcDbg_GetGlobal(void);
void CdcDbg_SetModule(CdcDbgModule_t mod, int enable);
int  CdcDbg_IsModuleEnabled(CdcDbgModule_t mod);
int  CdcDbg_IsEnabled(CdcDbgModule_t mod);
void CdcDbg_SetAllModules(int enable);
uint8_t CdcDbg_GetModMask(void);
void CdcDbg_SetModMask(uint8_t mask);
const char *CdcDbg_ModuleName(CdcDbgModule_t mod);
CdcDbgModule_t CdcDbg_FindModule(const char *name);
void CdcDbg_Printf(CdcDbgModule_t mod, const char *fmt, ...);
void CdcDbg_Write(CdcDbgModule_t mod, const uint8_t *data, uint16_t len);
/** Mirror one DBG/printf byte to CDC when global log is ON (used by retarget putchar). */
void CdcDbg_MirrorChar(int c);

#define CDC_DBG(mod, fmt, ...)      CdcDbg_Printf((mod), (fmt), ##__VA_ARGS__)
#define CDC_DBG_SYS(fmt, ...)       CDC_DBG(CDC_DBG_MOD_SYS,   fmt, ##__VA_ARGS__)
#define CDC_DBG_USB(fmt, ...)       CDC_DBG(CDC_DBG_MOD_USB,   fmt, ##__VA_ARGS__)
#define CDC_DBG_BANUX(fmt, ...)     CDC_DBG(CDC_DBG_MOD_BANUX, fmt, ##__VA_ARGS__)
#define CDC_DBG_SHELL(fmt, ...)     CDC_DBG(CDC_DBG_MOD_SHELL, fmt, ##__VA_ARGS__)
#define CDC_DBG_AUDIO(fmt, ...)     CDC_DBG(CDC_DBG_MOD_AUDIO, fmt, ##__VA_ARGS__)
#define CDC_DBG_UPG(fmt, ...)       CDC_DBG(CDC_DBG_MOD_UPG,   fmt, ##__VA_ARGS__)

#else /* !CDC_DEBUG_EN */

#define CdcDbg_Init()                           ((void)0)
#define CdcDbg_SetGlobal(e)                     ((void)(e))
#define CdcDbg_GetGlobal()                      (0)
#define CdcDbg_SetModule(m, e)                  ((void)(m), (void)(e))
#define CdcDbg_IsModuleEnabled(m)               ((void)(m), 0)
#define CdcDbg_IsEnabled(m)                     ((void)(m), 0)
#define CdcDbg_SetAllModules(e)                 ((void)(e))
#define CdcDbg_GetModMask()                     (0)
#define CdcDbg_SetModMask(m)                    ((void)(m))
#define CdcDbg_ModuleName(m)                    ((void)(m), (const char *)"")
#define CdcDbg_FindModule(n)                    ((void)(n), CDC_DBG_MOD_COUNT)
#define CdcDbg_Printf(m, fmt, ...)              ((void)(m), (void)(fmt))
#define CdcDbg_Write(m, d, l)                   ((void)(m), (void)(d), (void)(l))
#define CdcDbg_MirrorChar(c)                    ((void)(c))

#define CDC_DBG(mod, fmt, ...)                  ((void)0)
#define CDC_DBG_SYS(fmt, ...)                   ((void)0)
#define CDC_DBG_USB(fmt, ...)                   ((void)0)
#define CDC_DBG_BANUX(fmt, ...)                 ((void)0)
#define CDC_DBG_SHELL(fmt, ...)                 ((void)0)
#define CDC_DBG_AUDIO(fmt, ...)                 ((void)0)
#define CDC_DBG_UPG(fmt, ...)                   ((void)0)

#endif /* CDC_DEBUG_EN */

#ifdef __cplusplus
}
#endif

#endif /* __CDC_DEBUG_H__ */
