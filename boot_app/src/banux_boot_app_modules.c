/**
 * @file banux_boot_app_modules.c
 * @brief boot_app-specific Banux Shell modules.
 */
#include <stdio.h>
#include <string.h>
#include "bg_shell.h"
#include "banux_component.h"
#include "banux_config.h"
#include "banux_io.h"
#include "command_parser.h"
#include "drv_device.h"
#include "drv_fs.h"
#include "vfs.h"
#include "fw_upgrade.h"

static void print_component_type(BanuxComponentType_t type, const char *title)
{
    uint8_t i;

    Shell_Printf("\r\n%s:\r\n", title);
    for (i = 0u; i < BanuxComponent_GetCount(); i++) {
        const BanuxComponentInfo_t *info = BanuxComponent_Get(i);
        const BanuxComponentDescriptor_t *component;

        if (!info || !info->descriptor || info->descriptor->type != type) {
            continue;
        }
        component = info->descriptor;
        Shell_Printf("  %-18s %-10s v%-7s %s\r\n",
                     component->name,
                     BanuxComponent_StateName(info->state),
                     component->version,
                     component->description);
    }
}

static int banux_info(int argc, char *argv[])
{
    uint8_t i;
    uint8_t enabled = 0u;
    uint8_t ready = 0u;

    (void)argc;
    (void)argv;

    for (i = 0u; i < BanuxComponent_GetCount(); i++) {
        const BanuxComponentInfo_t *info = BanuxComponent_Get(i);
        if (!info || !info->descriptor) {
            continue;
        }
        if (info->descriptor->enabled) {
            enabled++;
        }
        if (info->state == BANUX_COMPONENT_READY) {
            ready++;
        }
    }

    Shell_Print("\r\nBanux Information:\r\n");
    Shell_Printf("  Framework version: %s\r\n", BANUX_VERSION_STRING);
    Shell_Printf("  Components:        %u total, %u enabled, %u ready\r\n",
                 (unsigned)BanuxComponent_GetCount(),
                 (unsigned)enabled,
                 (unsigned)ready);
    print_component_type(BANUX_COMPONENT_SYSTEM, "System components");
    print_component_type(BANUX_COMPONENT_APPLICATION, "Application components");
    Shell_Print("\r\n");
    return 0;
}

static const ShellOpt_t banux_opts[] = {
    OPT("i", "info", NULL, "Show Banux framework information", banux_info),
    OPT_END()
};

DEFINE_MODULE(banux, "Banux framework management", MOD_CAT_SYSTEM, banux_opts);

static int boot_enter(int argc, char *argv[])
{
    volatile uint32_t delay;

    (void)argc;
    (void)argv;

    Shell_Print("[BOOT] Writing burn flag and rebooting to bootloader ...\r\n");
    for (delay = 0u; delay < 200000u; delay++) {
        ;
    }
    FwUpgrade_RebootToBootloader();
    return 0;
}

static int boot_status(int argc, char *argv[])
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

static const ShellOpt_t boot_opts[] = {
    OPT("",  "",       NULL, "Reboot into bootloader", boot_enter),
    OPT("s", "status", NULL, "Show bootloader flag",   boot_status),
    OPT_END()
};

DEFINE_MODULE(boot, "Reboot to Bootloader", MOD_CAT_SYSTEM, boot_opts);

static int upg_enter(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    FwUpgrade_EnterCdcMode();
    return 0;
}

static int upg_info(int argc, char *argv[])
{
    FwUpgradeInfo_t info;

    (void)argc;
    (void)argv;

    (void)FwUpgrade_GetInfo(&info);
    Shell_Print("\r\nFirmware Upgrade:\r\n");
    Shell_Printf("  Part A:      0x%08X, %u KB\r\n",
                 (unsigned)info.part_a_base,
                 (unsigned)(info.part_a_size / 1024u));
    Shell_Printf("  Part B:      0x%08X, %u KB\r\n",
                 (unsigned)info.part_b_base,
                 (unsigned)(info.part_b_size / 1024u));
    Shell_Printf("  Flags:       0x%08X (%s)\r\n",
                 (unsigned)info.flags_addr,
                 info.flags_valid ? "valid" : "invalid");
    Shell_Printf("  Active:      %c\r\n", info.active_part ? 'B' : 'A');
    Shell_Printf("  Running B:   %s\r\n", info.running_part_b ? "yes" : "no");
    Shell_Printf("  Fail count:  %u/%u\r\n",
                 (unsigned)info.boot_fail_cnt,
                 (unsigned)info.boot_fail_max);
    Shell_Print("\r\n");
    return 0;
}

static const ShellOpt_t upg_opts[] = {
    OPT("",  "",     NULL, "Enter CDC firmware upgrade mode", upg_enter),
    OPT("i", "info", NULL, "Show firmware upgrade partition info", upg_info),
    OPT_END()
};

DEFINE_MODULE(upg, "CDC firmware upgrade", MOD_CAT_SYSTEM, upg_opts);

#if VFS_EN
static int cmd_ls(int argc, char *argv[])
{
    FsNode_t *node;
    int i;

    node = argc > 0 ? DrvFs_FindNode(argv[0]) : DrvFs_GetCwd();
    if (!node) {
        Shell_Print("ls: path not found\r\n");
        return -1;
    }
    if (node->type != FS_NODE_DIR && node->type != FS_NODE_DEV) {
        Shell_Print("ls: not a directory\r\n");
        return -2;
    }

    DrvFs_RefreshDir(node);
    for (i = 0; i < node->childCount; i++) {
        FsNode_t *child = node->children[i];
        if (!child) continue;
        Shell_Printf("%-16s %s\r\n", child->name, DrvFs_GetTypeName(child->type));
    }
    return 0;
}

static const ShellOpt_t ls_opts[] = {
    OPT("", "", "[path]", "List directory contents", cmd_ls),
    OPT_END()
};

DEFINE_MODULE(ls, "List directory contents", MOD_CAT_SYSTEM, ls_opts);

static int cmd_pwd(int argc, char *argv[])
{
    char path[VFS_MAX_PATH_LEN];

    (void)argc;
    (void)argv;
    if (DrvFs_GetCwdPath(path, sizeof(path)) != FS_OK) {
        return -1;
    }
    Shell_Printf("%s\r\n", path);
    return 0;
}

static const ShellOpt_t pwd_opts[] = {
    OPT("", "", NULL, "Print working directory", cmd_pwd),
    OPT_END()
};

DEFINE_MODULE(pwd, "Print working directory", MOD_CAT_SYSTEM, pwd_opts);

static int cmd_cd(int argc, char *argv[])
{
    const char *path = argc > 0 ? argv[0] : "/";

    if (DrvFs_Cd(path) != FS_OK) {
        Shell_Printf("cd: %s: not a directory\r\n", path);
        return -1;
    }
    return 0;
}

static const ShellOpt_t cd_opts[] = {
    OPT("", "", "[path]", "Change directory", cmd_cd),
    OPT_END()
};

DEFINE_MODULE(cd, "Change directory", MOD_CAT_SYSTEM, cd_opts);

static int cmd_cat(int argc, char *argv[])
{
    FsNode_t *node;
    char buf[128];
    int ret;

    if (argc < 1) {
        Shell_Print("Usage: cat <path>\r\n");
        return -1;
    }

    node = DrvFs_FindNode(argv[0]);
    if (!node) {
        Shell_Printf("cat: %s: no such path\r\n", argv[0]);
        return -2;
    }

    if (node->type == FS_NODE_FILE) {
        uint32_t offset = 0u;
        while (offset < node->fileSize) {
            int n = DrvFs_ReadFile(node, buf, sizeof(buf) - 1u, offset);
            if (n <= 0) return -3;
            buf[n] = '\0';
            Shell_Print(buf);
            offset += (uint32_t)n;
        }
        Shell_Print("\r\n");
        return 0;
    }

    if (node->type != FS_NODE_PARAM) {
        Shell_Print("cat: not a readable parameter/file\r\n");
        return -4;
    }

    ret = banux_read(argv[0], buf, sizeof(buf));
    if (ret < 0) {
        Shell_Print("cat: read failed\r\n");
        return ret;
    }
    Shell_Printf("%s\r\n", buf);
    return 0;
}

static const ShellOpt_t cat_opts[] = {
    OPT("", "", "<path>", "Display file or parameter", cmd_cat),
    OPT_END()
};

DEFINE_MODULE(cat, "Display file contents", MOD_CAT_SYSTEM, cat_opts);

static int cmd_echo(int argc, char *argv[])
{
    const char *path;
    const char *value;
    int ret;

    if (argc < 2) {
        Shell_Print("Usage: echo <path> <value>\r\n");
        return -1;
    }

    path = argv[0];
    value = argv[1];
    ret = banux_write(path, value, (uint32_t)strlen(value));
    if (ret < 0) {
        Shell_Print("echo: write failed\r\n");
        return ret;
    }
    Shell_Print("OK\r\n");
    return 0;
}

static const ShellOpt_t echo_opts[] = {
    OPT("", "", "<path> <value>", "Write parameter value", cmd_echo),
    OPT_END()
};

DEFINE_MODULE(echo, "Write to parameter", MOD_CAT_SYSTEM, echo_opts);

static void print_vfs_tree(VfsNode_t *node, uint8_t depth)
{
    int i;

    if (!node) return;
    for (i = 0; i < depth; i++) {
        Shell_Print("  ");
    }
    Shell_Printf("%s%s\r\n", node->name,
                 (node->type == VFS_NODE_DIR || node->type == VFS_NODE_DEV)
                     ? "/" : "");

    if (node->type != VFS_NODE_DIR && node->type != VFS_NODE_DEV) {
        return;
    }

    Vfs_RefreshDir(node);
    for (i = 0; i < node->childCount; i++) {
        print_vfs_tree(node->children[i], (uint8_t)(depth + 1u));
    }
}

static int cmd_tree(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    print_vfs_tree(Vfs_GetRoot(), 0u);
    return 0;
}

static const ShellOpt_t tree_opts[] = {
    OPT("", "", NULL, "Display VFS tree", cmd_tree),
    OPT_END()
};

DEFINE_MODULE(tree, "Display VFS tree", MOD_CAT_SYSTEM, tree_opts);
#endif

static int cmd_drivers(int argc, char *argv[])
{
    DrvDevice_t **devices;
    int count;
    int i;

    (void)argc;
    (void)argv;

    devices = DrvDevice_GetList(&count);
    Shell_Print("\r\nRegistered Drivers:\r\n");
    for (i = 0; i < count; i++) {
        Shell_Printf("  %-12s %-8s %s\r\n",
                     devices[i]->name,
                     DrvDevice_GetBusName(devices[i]->bus),
                     devices[i]->isRegistered ? "OK" : "FAIL");
    }
    Shell_Printf("Total: %d\r\n\r\n", count);
    return 0;
}

static const ShellOpt_t drivers_opts[] = {
    OPT("", "", NULL, "List registered drivers", cmd_drivers),
    OPT_END()
};

DEFINE_MODULE(drivers, "List device drivers", MOD_CAT_SYSTEM, drivers_opts);

void Shell_RegisterAllModules(void)
{
    REGISTER_MODULE(banux);
#if VFS_EN
    REGISTER_MODULE(ls);
    REGISTER_MODULE(pwd);
    REGISTER_MODULE(cd);
    REGISTER_MODULE(cat);
    REGISTER_MODULE(echo);
    REGISTER_MODULE(tree);
    CommandParser_RegisterCommands();
#endif
    REGISTER_MODULE(drivers);
    REGISTER_MODULE(boot);
    REGISTER_MODULE(upg);
}
