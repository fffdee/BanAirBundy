/**
 * @file shell_cmd_log.c
 * @brief Shell `log` command — enable/disable CDC debug by module.
 *
 * Usage:
 *   log              show status
 *   log -e           global enable (also mirrors DBG/printf to CDC)
 *   log -d           global disable
 *   log -a on|off    all modules on/off
 *   log -m name on|off
 *   log -l           list modules
 */
#include "shell_cmd_log.h"
#include "cdc_debug.h"
#include "sys_nv.h"
#include "banux_config.h"
#include <string.h>

#if !CDC_DEBUG_EN

static int ShellCmdLog_Disabled(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    Shell_Print("CDC debug compiled out (CDC_DEBUG_EN=0)\r\n");
    return 0;
}

static const ShellOpt_t g_shell_log_opts[] = {
    OPT("", "", NULL, "CDC debug disabled at compile time", ShellCmdLog_Disabled),
    OPT_END()
};

const ShellModule_t g_ShellCmdLogModule = {
    "log",
    "CDC debug log (compiled out)",
    MOD_CAT_DEBUG,
    g_shell_log_opts,
    OPT_COUNT(g_shell_log_opts)
};

#else /* CDC_DEBUG_EN */

static int parse_on_off(const char *s, int *out)
{
    if (!s || !out)
        return -1;
    if (!strcmp(s, "on") || !strcmp(s, "1") || !strcmp(s, "enable")) {
        *out = 1;
        return 0;
    }
    if (!strcmp(s, "off") || !strcmp(s, "0") || !strcmp(s, "disable")) {
        *out = 0;
        return 0;
    }
    return -1;
}

static void ShellCmdLog_PrintStatus(void)
{
    unsigned i;

    Shell_Printf("CDC log: global=%s\r\n",
                 CdcDbg_GetGlobal() ? "ON" : "OFF");
    for (i = 0; i < (unsigned)CDC_DBG_MOD_COUNT; i++) {
        Shell_Printf("  %-6s %s\r\n",
                     CdcDbg_ModuleName((CdcDbgModule_t)i),
                     CdcDbg_IsModuleEnabled((CdcDbgModule_t)i) ? "on" : "off");
    }
}

static int ShellCmdLog_Status(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    ShellCmdLog_PrintStatus();
    return 0;
}

static void ShellCmdLog_Persist(void)
{
    if (SysNv_SaveLog() == 0)
        Shell_Print("CDC log: saved to NVM\r\n");
    else
        Shell_Print("CDC log: NVM save failed\r\n");
}

static int ShellCmdLog_Enable(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    CdcDbg_SetGlobal(1);
    Shell_Print("CDC log: global ON (DBG/printf mirrored to CDC)\r\n");
    ShellCmdLog_Persist();
    return 0;
}

static int ShellCmdLog_Disable(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    CdcDbg_SetGlobal(0);
    Shell_Print("CDC log: global OFF\r\n");
    ShellCmdLog_Persist();
    return 0;
}

static int ShellCmdLog_All(int argc, char *argv[])
{
    int on;

    if (argc < 1 || parse_on_off(argv[0], &on) != 0) {
        Shell_Print("Usage: log -a on|off\r\n");
        return -1;
    }
    CdcDbg_SetAllModules(on);
    Shell_Printf("CDC log: all modules %s\r\n", on ? "on" : "off");
    ShellCmdLog_Persist();
    return 0;
}

static int ShellCmdLog_Module(int argc, char *argv[])
{
    CdcDbgModule_t mod;
    int on;

    if (argc < 2 || parse_on_off(argv[1], &on) != 0) {
        Shell_Print("Usage: log -m <name> on|off\r\n");
        Shell_Print("Names: sys usb banux shell audio upg\r\n");
        return -1;
    }

    mod = CdcDbg_FindModule(argv[0]);
    if (mod >= CDC_DBG_MOD_COUNT) {
        Shell_Printf("Unknown module: %s\r\n", argv[0]);
        return -1;
    }

    CdcDbg_SetModule(mod, on);
    Shell_Printf("CDC log: %s %s\r\n",
                 CdcDbg_ModuleName(mod), on ? "on" : "off");
    ShellCmdLog_Persist();
    return 0;
}

static int ShellCmdLog_List(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    ShellCmdLog_PrintStatus();
    return 0;
}

static const ShellOpt_t g_shell_log_opts[] = {
    OPT("",  "",       NULL,            "Show CDC log status",     ShellCmdLog_Status),
    OPT("e", "enable", NULL,            "Global ON + mirror DBG to CDC", ShellCmdLog_Enable),
    OPT("d", "disable",NULL,            "Global disable",          ShellCmdLog_Disable),
    OPT("a", "all",    "<on|off>",      "All modules on/off",      ShellCmdLog_All),
    OPT("m", "module", "<name> <on|off>","Per-module on/off",      ShellCmdLog_Module),
    OPT("l", "list",   NULL,            "List modules",            ShellCmdLog_List),
    OPT_END()
};

const ShellModule_t g_ShellCmdLogModule = {
    "log",
    "CDC debug log control",
    MOD_CAT_DEBUG,
    g_shell_log_opts,
    OPT_COUNT(g_shell_log_opts)
};

#endif /* CDC_DEBUG_EN */
