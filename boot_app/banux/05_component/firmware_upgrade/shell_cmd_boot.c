/**
 * @file shell_cmd_boot.c
 * @brief USB CDC Shell commands that control bootloader entry.
 */
#include "bg_shell.h"
#include "shell_cmd_boot.h"
#include "fw_upgrade.h"

static int ShellCmdBoot_Enter(int argc, char *argv[])
{
    volatile uint32_t delay;

    (void)argc;
    (void)argv;

    Shell_Print("[BOOT] Writing burn flag and rebooting to bootloader ...\r\n");

    /* Give the CDC TX state machine time to send the status line. */
    for (delay = 0; delay < 200000u; ++delay)
        ;

    FwUpgrade_RebootToBootloader();
    return 0;
}

static int ShellCmdBoot_Status(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    Shell_Printf("Burn flag = 0x%08X\r\n",
                 (unsigned)FwUpgrade_GetBootloaderFlag());
    Shell_Printf("Status: %s\r\n",
                 FwUpgrade_IsBootloaderFlagSet()
                     ? "SET (bootloader on next reboot)"
                     : "CLEARED (normal boot)");
    return 0;
}

static const ShellOpt_t g_shell_boot_opts[] = {
    OPT("",  "",       NULL, "Reboot into bootloader", ShellCmdBoot_Enter),
    OPT("s", "status", NULL, "Show bootloader flag",   ShellCmdBoot_Status),
    OPT_END()
};

const ShellModule_t g_ShellCmdBootModule = {
    "boot",
    "Reboot to Bootloader",
    MOD_CAT_SYSTEM,
    g_shell_boot_opts,
    OPT_COUNT(g_shell_boot_opts)
};
