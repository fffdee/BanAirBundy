/**
 * @file  app_upgrade.c
 * @brief Firmware upgrade engine — USB CDC framed protocol (bootloader).
 *
 * Frame (big-endian multi-byte), matches update_tool / wireless_mic_bootloader:
 *   [SOF:1][CMD:1][SEQ:2][LEN:2][DATA:len][CRC16:2]
 *   CRC16-CCITT (poly=0x1021, init=0xFFFF) over CMD+SEQ+LEN+DATA
 */
#include "app_upgrade.h"
#include "dual_partition.h"
#include "spi_flash.h"
#include "debug.h"
#include "reset.h"
#include "otg_device_standard_request.h"
#include "otg_device_cdc.h"
#include "core_d1088.h"
#include <nds32_intrinsic.h>
#include <string.h>

/* ── CRC32 (firmware image integrity at FINISH) ──────────────────────────── */
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

/* ── CRC16-CCITT (frame) ─────────────────────────────────────────────────── */
static uint16_t calc_crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    uint8_t i;
    while (len--) {
        crc ^= (uint16_t)(*buf++) << 8;
        for (i = 0; i < 8; i++) {
            crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* ── Flash helpers ───────────────────────────────────────────────────────── */
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

/* ── Engine / framing ────────────────────────────────────────────────────── */
#define PKT_DATA_MAX  (UPG_MAX_CHUNK + 4u)
#define TX_BUF_MAX    (1u + 1u + 2u + 2u + PKT_DATA_MAX + 2u)

typedef enum {
    STATE_IDLE = 0,
    STATE_WRITING,
    STATE_FINISH
} UpgradeState_t;

typedef struct {
    uint8_t  cmd;
    uint16_t seq;
    uint16_t len;
    uint8_t  data[PKT_DATA_MAX];
} UpgPkt_t;

typedef enum {
    PS_SOF, PS_CMD, PS_SEQ_H, PS_SEQ_L,
    PS_LEN_H, PS_LEN_L, PS_DATA, PS_CRC_H, PS_CRC_L
} ParserSt_t;

typedef struct {
    ParserSt_t st;
    uint16_t   di;
    uint8_t    crc_hi;
    UpgPkt_t   pkt;
} Parser_t;

typedef struct {
    UpgradeState_t state;
    uint32_t       write_base;
    uint32_t       total_size;
    uint32_t       written_size;
    uint32_t       data_crc;
    uint8_t        target_part;
    const UpgradeChannel_t *tx_ch;
} UpgradeEngine_t;

static UpgradeEngine_t g_engine;
static Parser_t        g_parser;
static uint32_t        g_upg_base;
static uint32_t        g_upg_max;
static uint8_t         g_upg_part;

static void parser_reset(void)
{
    g_parser.st = PS_SOF;
    g_parser.di = 0;
    g_parser.crc_hi = 0;
}

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
}

static void send_pkt(uint8_t cmd, uint16_t seq, const uint8_t *data, uint16_t dlen)
{
    uint8_t buf[TX_BUF_MAX];
    uint16_t n = 0, crc;

    if (!g_engine.tx_ch || !g_engine.tx_ch->tx_write)
        return;
    if (dlen > PKT_DATA_MAX)
        dlen = PKT_DATA_MAX;

    buf[n++] = UPG_SOF;
    buf[n++] = cmd;
    buf[n++] = (uint8_t)(seq >> 8);
    buf[n++] = (uint8_t)(seq);
    buf[n++] = (uint8_t)(dlen >> 8);
    buf[n++] = (uint8_t)(dlen);
    if (dlen && data) {
        memcpy(buf + n, data, dlen);
        n = (uint16_t)(n + dlen);
    }
    crc = calc_crc16(buf + 1, (uint16_t)(5u + dlen));
    buf[n++] = (uint8_t)(crc >> 8);
    buf[n++] = (uint8_t)(crc);
    g_engine.tx_ch->tx_write(buf, n);
}

#define SEND_ACK(seq)          send_pkt(RSP_ACK,  (seq), NULL, 0)
#define SEND_ACKD(seq, d, l)   send_pkt(RSP_ACK,  (seq), (d), (l))
#define SEND_NACK(seq, err)    do { uint8_t _e = (err); send_pkt(RSP_NACK, (seq), &_e, 1); } while (0)

static int parser_verify(const UpgPkt_t *pkt, uint16_t recv_crc)
{
    uint8_t tmp[5u + PKT_DATA_MAX];
    tmp[0] = pkt->cmd;
    tmp[1] = (uint8_t)(pkt->seq >> 8);
    tmp[2] = (uint8_t)(pkt->seq);
    tmp[3] = (uint8_t)(pkt->len >> 8);
    tmp[4] = (uint8_t)(pkt->len);
    if (pkt->len)
        memcpy(tmp + 5, pkt->data, pkt->len);
    return (calc_crc16(tmp, (uint16_t)(5u + pkt->len)) == recv_crc) ? 1 : -1;
}

/* Feed one byte. 1=packet ready, -1=CRC error, 0=need more */
static int parser_feed(uint8_t b)
{
    switch (g_parser.st) {
    case PS_SOF:
        if (b == UPG_SOF)
            g_parser.st = PS_CMD;
        break;
    case PS_CMD:
        g_parser.pkt.cmd = b;
        g_parser.st = PS_SEQ_H;
        break;
    case PS_SEQ_H:
        g_parser.pkt.seq = (uint16_t)b << 8;
        g_parser.st = PS_SEQ_L;
        break;
    case PS_SEQ_L:
        g_parser.pkt.seq |= b;
        g_parser.st = PS_LEN_H;
        break;
    case PS_LEN_H:
        g_parser.pkt.len = (uint16_t)b << 8;
        g_parser.st = PS_LEN_L;
        break;
    case PS_LEN_L:
        g_parser.pkt.len |= b;
        g_parser.di = 0;
        if (g_parser.pkt.len > PKT_DATA_MAX) {
            g_parser.st = PS_SOF;
            break;
        }
        g_parser.st = (g_parser.pkt.len == 0) ? PS_CRC_H : PS_DATA;
        break;
    case PS_DATA:
        g_parser.pkt.data[g_parser.di++] = b;
        if (g_parser.di >= g_parser.pkt.len)
            g_parser.st = PS_CRC_H;
        break;
    case PS_CRC_H:
        g_parser.crc_hi = b;
        g_parser.st = PS_CRC_L;
        break;
    case PS_CRC_L: {
        uint16_t recv = ((uint16_t)g_parser.crc_hi << 8) | b;
        g_parser.st = PS_SOF;
        return parser_verify(&g_parser.pkt, recv);
    }
    default:
        g_parser.st = PS_SOF;
        break;
    }
    return 0;
}

/* Match wireless_mic_bootloader / boot_decision fw_looks_valid:
 * accept BGPF magic at 0xA4, OR a non-erased / non-flag first word. */
static int is_target_firmware_valid(void)
{
    volatile const uint32_t *magic_ptr;
    volatile const uint32_t *first_word;
    uint32_t magic, first;
    int ok;

    DataCacheInvalidAll();
    __nds32__dsb();

    magic_ptr = (volatile const uint32_t *)(g_engine.write_base + FW_VALID_MAGIC_OFFSET);
    first_word = (volatile const uint32_t *)g_engine.write_base;
    magic = *magic_ptr;
    first = *first_word;
    ok = (magic == FW_VALID_MAGIC) ||
         (first != 0xFFFFFFFFu && first != PART_FLAG_MAGIC);

    DBG("[UPG] FINISH verify base=0x%X first=0x%X magic=0x%X written=%u/%u → %s\n",
        (unsigned)g_engine.write_base, (unsigned)first, (unsigned)magic,
        (unsigned)g_engine.written_size, (unsigned)g_engine.total_size,
        ok ? "OK" : "FAIL");

    return ok ? 1 : 0;
}

static void dispatch_packet(const UpgPkt_t *pkt)
{
    uint32_t offset;
    uint16_t dlen;

    /* DATA is high-rate; logging every chunk stalls USB CDC. */
    if (pkt->cmd != CMD_DATA) {
        DBG("[UPG] cmd=0x%02X seq=%u len=%u\n",
            pkt->cmd, (unsigned)pkt->seq, (unsigned)pkt->len);
    }

    switch (pkt->cmd) {
    case CMD_SYNC: {
        uint8_t ver = UPG_VERSION;
        g_engine.state = STATE_IDLE;
        g_engine.written_size = 0;
        g_engine.total_size = 0;
        upgrade_compute_target();
        parser_reset();
        SEND_ACKD(pkt->seq, &ver, 1);
        break;
    }

    case CMD_QUERY_INFO: {
        DevInfo_t info;
        PartFlag_t flags;
        const DualPart_Layout_t *layout = DualPart_GetLayout();

        memset(&info, 0, sizeof(info));
        info.boot_mode = layout->is_dual ? BOOT_MODE_DUAL_AB : BOOT_MODE_SINGLE;
        info.protocol_ver = UPG_VERSION;
        info.part_a_base = PART_A_BASE;
        info.part_a_size = layout->part_a_usable;
        info.part_b_base = PART_B_BASE;
        info.part_b_size = layout->part_b_usable;
        if (PartFlag_Read(&flags)) {
            info.active_part = flags.active_part;
            info.boot_fail_cnt = flags.boot_fail_cnt;
        }
        SEND_ACKD(pkt->seq, (uint8_t *)&info, (uint16_t)sizeof(info));
        break;
    }

    case CMD_START: {
        uint32_t fw_size;
        if (pkt->len < 4) {
            SEND_NACK(pkt->seq, UPG_ERR_PARAM);
            break;
        }
        fw_size = ((uint32_t)pkt->data[0] << 24) |
                  ((uint32_t)pkt->data[1] << 16) |
                  ((uint32_t)pkt->data[2] << 8) |
                  ((uint32_t)pkt->data[3]);
        upgrade_compute_target();
        if (fw_size == 0 || fw_size > g_upg_max) {
            SEND_NACK(pkt->seq, UPG_ERR_SIZE);
            break;
        }
        if (!bl_flash_erase(g_upg_base, fw_size)) {
            SEND_NACK(pkt->seq, UPG_ERR_FLASH);
            break;
        }
        g_engine.state = STATE_WRITING;
        g_engine.write_base = g_upg_base;
        g_engine.total_size = fw_size;
        g_engine.written_size = 0;
        g_engine.data_crc = 0;
        g_engine.target_part = g_upg_part;
        SEND_ACK(pkt->seq);
        break;
    }

    case CMD_DATA:
        if (g_engine.state != STATE_WRITING) {
            SEND_NACK(pkt->seq, UPG_ERR_STATE);
            break;
        }
        if (pkt->len < 5) {
            SEND_NACK(pkt->seq, UPG_ERR_PARAM);
            break;
        }
        offset = ((uint32_t)pkt->data[0] << 24) |
                 ((uint32_t)pkt->data[1] << 16) |
                 ((uint32_t)pkt->data[2] << 8) |
                 ((uint32_t)pkt->data[3]);
        dlen = (uint16_t)(pkt->len - 4u);
        if ((offset + dlen) > g_engine.total_size) {
            SEND_NACK(pkt->seq, UPG_ERR_SIZE);
            break;
        }
        flash_service_usb();
        if (!bl_flash_write(g_engine.write_base + offset, pkt->data + 4, dlen)) {
            SEND_NACK(pkt->seq, UPG_ERR_FLASH);
            break;
        }
        g_engine.written_size += dlen;
        g_engine.data_crc = crc32_update(g_engine.data_crc, pkt->data + 4, dlen);
        flash_service_usb();
        SEND_ACK(pkt->seq);
        flash_service_usb();
        break;

    case CMD_FINISH: {
        PartFlag_t flags;
        if (g_engine.state != STATE_WRITING) {
            SEND_NACK(pkt->seq, UPG_ERR_STATE);
            break;
        }
        if (!is_target_firmware_valid()) {
            SEND_NACK(pkt->seq, UPG_ERR_CRC);
            break;
        }
        if (DualPart_GetLayout()->is_dual) {
            if (PartFlag_Read(&flags) == 0) {
                PartFlag_Default(&flags);
            }
            flags.active_part = g_engine.target_part;
            flags.reserved1 = 0;
            flags.boot_fail_cnt = 1;
            if (PartFlag_Write(&flags) == 0) {
                SEND_NACK(pkt->seq, UPG_ERR_FLASH);
                break;
            }
        }
        SEND_ACK(pkt->seq);
        g_engine.state = STATE_FINISH;
        break;
    }

    case CMD_ENTER_BOOT:
        /* Already in bootloader */
        SEND_ACK(pkt->seq);
        break;

    case CMD_REBOOT:
        SEND_ACK(pkt->seq);
        {
            volatile uint32_t delay;
            for (delay = 0; delay < 50000; delay++) { ; }
        }
        Reset_McuSystem();
        break;

    case CMD_JUMP:
        SEND_ACK(pkt->seq);
        {
            volatile uint32_t delay;
            for (delay = 0; delay < 50000; delay++) { ; }
        }
        Boot_CheckAndJumpIfNeeded();
        break;

    case CMD_ERASE:
        upgrade_compute_target();
        if (!bl_flash_erase(g_upg_base, g_upg_max)) {
            SEND_NACK(pkt->seq, UPG_ERR_FLASH);
            break;
        }
        g_engine.state = STATE_IDLE;
        SEND_ACK(pkt->seq);
        break;

    default:
        SEND_NACK(pkt->seq, UPG_ERR_PARAM);
        break;
    }
}

void App_Upgrade_Init(void)
{
    memset(&g_engine, 0, sizeof(g_engine));
    g_engine.state = STATE_IDLE;
    parser_reset();
    crc32_init_table();
    upgrade_compute_target();
}

void App_Upgrade_ProcessChannel(const UpgradeChannel_t *ch)
{
    uint8_t b;
    int rc;

    if (!ch || !ch->rx_available || !ch->rx_read)
        return;

    g_engine.tx_ch = ch;

    while (ch->rx_available() > 0) {
        if (ch->rx_read(&b, 1) != 1)
            break;
        rc = parser_feed(b);
        if (rc == 0)
            continue;
        if (rc < 0) {
            DBG("[UPG] CRC err\n");
            SEND_NACK(g_parser.pkt.seq, UPG_ERR_CRC);
            parser_reset();
            continue;
        }
        dispatch_packet(&g_parser.pkt);
        parser_reset();
    }
}

/* InjectRaw helpers (feed a memory buffer through the same parser) */
static const uint8_t *s_inj_buf;
static uint16_t s_inj_len;
static uint16_t s_inj_pos;

static uint16_t inj_read(uint8_t *out, uint16_t maxLen)
{
    uint16_t n = 0;
    while (n < maxLen && s_inj_pos < s_inj_len) {
        out[n++] = s_inj_buf[s_inj_pos++];
    }
    return n;
}

static int inj_avail(void)
{
    return (s_inj_pos < s_inj_len) ? (int)(s_inj_len - s_inj_pos) : 0;
}

void App_Upgrade_InjectRaw(uint8_t ch_id, const uint8_t *buf, uint16_t len,
                           void (*tx_fn)(const uint8_t *data, uint16_t len))
{
    static UpgradeChannel_t s_inject_ch;

    (void)ch_id;
    if (!buf || !len || !tx_fn)
        return;

    s_inj_buf = buf;
    s_inj_len = len;
    s_inj_pos = 0;
    memset(&s_inject_ch, 0, sizeof(s_inject_ch));
    s_inject_ch.tx_write = tx_fn;
    s_inject_ch.rx_read = inj_read;
    s_inject_ch.rx_available = inj_avail;
    App_Upgrade_ProcessChannel(&s_inject_ch);
}

int App_Upgrade_IsActive(void)
{
    return (g_engine.state == STATE_WRITING) ? 1 : 0;
}

int App_Upgrade_IsFinished(void)
{
    return (g_engine.state == STATE_FINISH) ? 1 : 0;
}
