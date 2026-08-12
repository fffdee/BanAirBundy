/**
 * @file shell_cmd_sys.c
 * @brief System Shell module — reset, NVM save/status.
 *
 * Usage:
 *   sys / sys -r     reboot MCU
 *   sys -s           save NVM (log settings, …)
 *   sys -n           show NVM status
 */
#include "shell_cmd_sys.h"
#include "sys_nv.h"
#include "cdc_debug.h"
#include "reset.h"

static int ShellCmdSys_Reset(int argc, char *argv[])
{
    volatile uint32_t delay;

    (void)argc;
    (void)argv;

    Shell_Print("[SYS] Resetting MCU ...\r\n");

    /* Allow CDC TX to flush the status line. */
    for (delay = 0; delay < 200000u; ++delay)
        ;

    Reset_McuSystem();
    return 0; /* never reached */
}

static int ShellCmdSys_Save(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (SysNv_Save() == 0) {
        Shell_Print("[SYS] NVM saved\r\n");
        return 0;
    }
    Shell_Print("[SYS] NVM save failed\r\n");
    return -1;
}

static int ShellCmdSys_NvStatus(int argc, char *argv[])
{
    const SysNv_t *nv = SysNv_Get();

    (void)argc;
    (void)argv;

    Shell_Printf("[SYS] NVM addr=0x%08lX ver=0x%04X\r\n",
                 (unsigned long)SYS_NV_FLASH_ADDR,
                 (unsigned)nv->version);
    Shell_Printf("  log_global=%u  log_mask=0x%02X\r\n",
                 (unsigned)nv->log_global,
                 (unsigned)nv->log_mod_mask);
#if CDC_DEBUG_EN
    Shell_Printf("  runtime: global=%s mask=0x%02X\r\n",
                 CdcDbg_GetGlobal() ? "ON" : "OFF",
                 (unsigned)CdcDbg_GetModMask());
#endif
    return 0;
}

static const ShellOpt_t g_shell_sys_opts[] = {
    OPT("",  "",      NULL, "Reset MCU",            ShellCmdSys_Reset),
    OPT("r", "reset", NULL, "Reset MCU",            ShellCmdSys_Reset),
    OPT("s", "save",  NULL, "Save NVM to flash",    ShellCmdSys_Save),
    OPT("n", "nv",    NULL, "Show NVM / log state", ShellCmdSys_NvStatus),
    OPT_END()
};

const ShellModule_t g_ShellCmdSysModule = {
    "sys",
    "System control / NVM",
    MOD_CAT_SYSTEM,
    g_shell_sys_opts,
    OPT_COUNT(g_shell_sys_opts)
};
