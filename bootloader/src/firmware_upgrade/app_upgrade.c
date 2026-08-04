/**
 * @file  app_upgrade.c
 * @brief Firmware upgrade engine — USB CDC only (bootloader port).
 */
#include "app_upgrade.h"
#include "dual_partition.h"
#include "spi_flash.h"
#include "debug.h"
#include "reset.h"
#include "otg_device_standard_request.h"
#include "otg_device_cdc.h"
#include <string.h>

/* ── CRC32 ───────────────────────────────────────────────────────────────── */
static uint32_t crc32_table[256];
static int crc32_table_init = 0;

static void crc32_init_table(void)
{
    uint32_t i, j, crc;
    if (crc32_table_init) return;
    crc32_table_init = 1;
    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320UL) : (crc >> 1);
        }
        crc32_table[i] = crc;
    }
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *buf, uint16_t len)
{
    uint16_t i;
    if (!crc32_table_init) crc32_init_table();
    crc ^= 0xFFFFFFFFUL;
    for (i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ buf[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFFUL;
}

/* ── Flash helpers (avoid FlashErase capacity trap in bootloader) ────────── */
#define FLASH_BLOCK_SZ  0x10000UL

static void flash_service_usb(void)
{
    OTG_DeviceRequestProcess();
    OTG_DeviceCDC_Task();
}

static int bl_flash_erase(uint32_t offset, uint32_t size)
{
    uint32_t cur = offset & ~(FLASH_SECTOR_SZ - 1u);
    uint32_t end = (offset + size + FLASH_SECTOR_SZ - 1u) & ~(FLASH_SECTOR_SZ - 1u);
    volatile uint32_t d;

    SpiFlashIOCtrl(IOCTL_FLASH_UNPROTECT, "\x35\xBA\x69", 3);
    while (cur < end) {
        if (((cur & (FLASH_BLOCK_SZ - 1u)) == 0u) &&
            ((end - cur) >= FLASH_BLOCK_SZ)) {
            SpiFlashErase(BLOCK_ERASE, cur / FLASH_BLOCK_SZ, 1);
            cur += FLASH_BLOCK_SZ;
        } else {
            SpiFlashErase(SECTOR_ERASE, cur / FLASH_SECTOR_SZ, 1);
            cur += FLASH_SECTOR_SZ;
        }
        flash_service_usb();
        d = 10000UL; while (d--) { ; }
    }
    return 1;
}

static int bl_flash_write(uint32_t addr, const uint8_t *data, uint16_t len)
{
    return (SpiFlashWrite(addr, (uint8_t *)data, len, 1) == FLASH_NONE_ERR) ? 1 : 0;
}

/* ── Engine state ────────────────────────────────────────────────────────── */
typedef enum {
    STATE_IDLE = 0,
    STATE_SYNC,
    STATE_QUERY,
    STATE_START,
    STATE_WRITING,
    STATE_FINISH
} UpgradeState_t;

typedef struct {
    UpgradeState_t  state;
    uint8_t         rx_buf[256];
    uint32_t        write_addr;
    uint32_t        total_size;
    uint32_t        written_size;
    uint32_t        data_crc;
    const UpgradeChannel_t *active_ch;
} UpgradeEngine_t;

static UpgradeEngine_t g_engine;
static uint32_t g_upg_base;
static uint32_t g_upg_max;
static uint8_t  g_upg_part;

static void upgrade_compute_target(void)
{
    const DualPart_Layout_t *layout = DualPart_GetLayout();

    if (layout->is_dual) {
        PartFlag_t flags;
        if (PartFlag_Read(&flags) && flags.active_part == 1u) {
            g_upg_part = 0;
            g_upg_base = PART_A_BASE;
            g_upg_max  = layout->part_a_usable;
        } else {
            g_upg_part = 1;
            g_upg_base = PART_B_BASE;
            g_upg_max  = layout->part_b_usable;
        }
    } else {
        g_upg_part = 0;
        g_upg_base = PART_A_BASE;
        g_upg_max  = layout->part_a_usable;
    }

    DBG("[UPG] Target Part %c @ 0x%08X max=%u KB\n",
        g_upg_part ? 'B' : 'A', (unsigned)g_upg_base,
        (unsigned)(g_upg_max / 1024));
}

static uint8_t get_active_partition(void)
{
    PartFlag_t flags;
    if (PartFlag_Read(&flags) != 0) {
        return flags.active_part;
    }
    return 0;
}

static int is_target_firmware_valid(void)
{
    volatile const uint32_t *magic_ptr =
        (volatile const uint32_t *)(g_upg_base + FW_VALID_MAGIC_OFFSET);
    return (*magic_ptr == FW_VALID_MAGIC) ? 1 : 0;
}

static void send_ack(const UpgradeChannel_t *ch, uint8_t data)
{
    uint8_t buf[3];
    buf[0] = UPG_SOF;
    buf[1] = RSP_ACK;
    buf[2] = data;
    ch->tx_write(buf, 3);
}

static void send_nack(const UpgradeChannel_t *ch, uint8_t error_code)
{
    uint8_t buf[3];
    buf[0] = UPG_SOF;
    buf[1] = RSP_NACK;
    buf[2] = error_code;
    ch->tx_write(buf, 3);
}

static void handle_sync(const UpgradeChannel_t *ch)
{
    g_engine.state = STATE_SYNC;
    send_ack(ch, UPG_VERSION);
}

static void handle_query(const UpgradeChannel_t *ch)
{
    DevInfo_t info;
    uint8_t buf[sizeof(info) + 3];
    PartFlag_t flags;
    const DualPart_Layout_t *layout = DualPart_GetLayout();

    memset(&info, 0, sizeof(info));
    info.boot_mode = layout->is_dual ? BOOT_MODE_DUAL_AB : BOOT_MODE_SINGLE;
    info.active_part = get_active_partition();
    if (PartFlag_Read(&flags) != 0) {
        info.boot_fail_cnt = flags.boot_fail_cnt;
    }
    info.protocol_ver = UPG_VERSION;
    info.part_a_base = PART_A_BASE;
    info.part_a_size = layout->part_a_usable;
    info.part_b_base = PART_B_BASE;
    info.part_b_size = layout->part_b_usable;

    buf[0] = UPG_SOF;
    buf[1] = RSP_ACK;
    memcpy(&buf[2], &info, sizeof(info));
    ch->tx_write(buf, 2 + sizeof(info));
    g_engine.state = STATE_QUERY;
}

static void handle_start(const UpgradeChannel_t *ch, const uint8_t *pkt, uint16_t len)
{
    uint32_t fw_size;

    if (len < 6) {
        send_nack(ch, UPG_ERR_PARAM);
        return;
    }

    fw_size = ((uint32_t)pkt[2] << 24) |
              ((uint32_t)pkt[3] << 16) |
              ((uint32_t)pkt[4] << 8) |
              ((uint32_t)pkt[5]);

    upgrade_compute_target();

    if (fw_size == 0 || fw_size > g_upg_max) {
        send_nack(ch, UPG_ERR_SIZE);
        return;
    }

    if (!bl_flash_erase(g_upg_base, fw_size)) {
        send_nack(ch, UPG_ERR_FLASH);
        return;
    }

    g_engine.state = STATE_WRITING;
    g_engine.active_ch = ch;
    g_engine.write_addr = g_upg_base;
    g_engine.total_size = fw_size;
    g_engine.written_size = 0;
    g_engine.data_crc = 0;

    send_ack(ch, 0);
}

static void handle_data(const UpgradeChannel_t *ch, const uint8_t *pkt, uint16_t len)
{
    uint16_t chunk_len;

    if (g_engine.state != STATE_WRITING) {
        send_nack(ch, UPG_ERR_STATE);
        return;
    }
    if (len < 3) {
        send_nack(ch, UPG_ERR_PARAM);
        return;
    }

    chunk_len = len - 2;
    if (g_engine.written_size + chunk_len > g_engine.total_size) {
        send_nack(ch, UPG_ERR_SIZE);
        return;
    }

    if (!bl_flash_write(g_engine.write_addr, &pkt[2], chunk_len)) {
        send_nack(ch, UPG_ERR_FLASH);
        return;
    }

    g_engine.write_addr += chunk_len;
    g_engine.written_size += chunk_len;
    g_engine.data_crc = crc32_update(g_engine.data_crc, &pkt[2], chunk_len);
    flash_service_usb();
    send_ack(ch, 0);
}

static void handle_finish(const UpgradeChannel_t *ch, const uint8_t *pkt, uint16_t len)
{
    uint32_t received_crc;
    PartFlag_t flags;

    if (g_engine.state != STATE_WRITING) {
        send_nack(ch, UPG_ERR_STATE);
        return;
    }
    if (len < 6) {
        send_nack(ch, UPG_ERR_PARAM);
        return;
    }

    received_crc = ((uint32_t)pkt[2] << 24) |
                   ((uint32_t)pkt[3] << 16) |
                   ((uint32_t)pkt[4] << 8) |
                   ((uint32_t)pkt[5]);

    if (g_engine.data_crc != received_crc) {
        send_nack(ch, UPG_ERR_CRC);
        return;
    }
    if (!is_target_firmware_valid()) {
        send_nack(ch, UPG_ERR_CRC);
        return;
    }

    if (DualPart_GetLayout()->is_dual) {
        if (PartFlag_Read(&flags) == 0) {
            PartFlag_Default(&flags);
        }
        flags.active_part   = g_upg_part;
        flags.reserved1     = 0;
        flags.boot_fail_cnt = 1;
        if (PartFlag_Write(&flags) == 0) {
            send_nack(ch, UPG_ERR_FLASH);
            return;
        }
    }

    send_ack(ch, 0);
    g_engine.state = STATE_FINISH;
    g_engine.active_ch = NULL;
}

static void handle_enter_boot(const UpgradeChannel_t *ch)
{
    /* Already in bootloader — ACK only */
    send_ack(ch, 0);
}

static void handle_reboot(const UpgradeChannel_t *ch)
{
    send_ack(ch, 0);
    {
        volatile uint32_t delay;
        for (delay = 0; delay < 50000; delay++) { ; }
    }
    Reset_McuSystem();
}

static void handle_jump(const UpgradeChannel_t *ch)
{
    send_ack(ch, 0);
    {
        volatile uint32_t delay;
        for (delay = 0; delay < 50000; delay++) { ; }
    }
    Boot_CheckAndJumpIfNeeded();
}

static void dispatch_command(const UpgradeChannel_t *ch, const uint8_t *pkt, uint16_t len)
{
    uint8_t cmd;

    if (len < 2 || pkt[0] != UPG_SOF) {
        return;
    }

    cmd = pkt[1];
    DBG("[UPG] cmd=0x%02X len=%u\n", cmd, (unsigned)len);

    switch (cmd) {
    case CMD_SYNC:       handle_sync(ch); break;
    case CMD_QUERY_INFO: handle_query(ch); break;
    case CMD_START:      handle_start(ch, pkt, len); break;
    case CMD_DATA:       handle_data(ch, pkt, len); break;
    case CMD_FINISH:     handle_finish(ch, pkt, len); break;
    case CMD_ENTER_BOOT: handle_enter_boot(ch); break;
    case CMD_REBOOT:     handle_reboot(ch); break;
    case CMD_JUMP:       handle_jump(ch); break;
    default:             send_nack(ch, UPG_ERR_PARAM); break;
    }
}

void App_Upgrade_Init(void)
{
    memset(&g_engine, 0, sizeof(g_engine));
    g_engine.state = STATE_IDLE;
    crc32_init_table();
}

void App_Upgrade_ProcessChannel(const UpgradeChannel_t *ch)
{
    uint16_t available;
    uint16_t rx_len;

    if (!ch) return;

    available = (uint16_t)ch->rx_available();
    if (available == 0) return;

    if (available > (uint16_t)sizeof(g_engine.rx_buf)) {
        available = (uint16_t)sizeof(g_engine.rx_buf);
    }

    rx_len = ch->rx_read(g_engine.rx_buf, available);
    if (rx_len == 0) return;

    dispatch_command(ch, g_engine.rx_buf, rx_len);
}

void App_Upgrade_InjectRaw(uint8_t ch_id, const uint8_t *buf, uint16_t len,
                           void (*tx_fn)(const uint8_t *data, uint16_t len))
{
    static UpgradeChannel_t s_inject_ch;
    memset(&s_inject_ch, 0, sizeof(s_inject_ch));
    s_inject_ch.id       = ch_id;
    s_inject_ch.tx_write = tx_fn;
    dispatch_command(&s_inject_ch, buf, len);
}

int App_Upgrade_IsActive(void)
{
    return (g_engine.state != STATE_IDLE && g_engine.active_ch != NULL);
}

int App_Upgrade_IsFinished(void)
{
    return (g_engine.state == STATE_FINISH);
}
