/**
 * @file  boot_decision.c
 * @brief Bootloader partition layout + jump-to-APP decision.
 */
#include <string.h>
#include <nds32_intrinsic.h>
#include "dual_partition.h"
#include "spi_flash.h"
#include "watchdog.h"
#include "remap.h"
#include "core_d1088.h"
#include "debug.h"

static DualPart_Layout_t g_layout;

const DualPart_Layout_t *DualPart_GetLayout(void)
{
    return &g_layout;
}

void DualPart_Init(void)
{
    g_layout.flash_capacity = INTERNAL_ROM_CAPACITY;

    if (g_layout.flash_capacity >= PART_FLAG_ADDR_DEFAULT + FLASH_SECTOR_SZ) {
        g_layout.part_flag_addr = PART_FLAG_ADDR_DEFAULT;
    } else {
        g_layout.part_flag_addr = (g_layout.flash_capacity - FLASH_SECTOR_SZ)
                                  & ~(FLASH_SECTOR_SZ - 1u);
    }

    if (g_layout.flash_capacity >= PART_A_BASE + PART_A_SIZE &&
        g_layout.part_flag_addr >= PART_A_BASE + PART_A_SIZE) {
        g_layout.part_a_usable = PART_A_SIZE;
    } else {
        uint32_t a_end = g_layout.flash_capacity;
        if (g_layout.part_flag_addr >= PART_A_BASE &&
            g_layout.part_flag_addr < a_end) {
            a_end = g_layout.part_flag_addr;
        }
        g_layout.part_a_usable = (a_end > PART_A_BASE) ? a_end - PART_A_BASE : 0;
    }

    if (g_layout.flash_capacity >= PART_B_BASE + PART_B_SIZE &&
        g_layout.part_flag_addr >= PART_B_BASE + PART_B_SIZE) {
        g_layout.part_b_usable = PART_B_SIZE;
    } else if (g_layout.flash_capacity > PART_B_BASE) {
        uint32_t b_end = g_layout.flash_capacity;
        if (g_layout.part_flag_addr >= PART_B_BASE &&
            g_layout.part_flag_addr < b_end) {
            b_end = g_layout.part_flag_addr;
        }
        g_layout.part_b_usable = b_end - PART_B_BASE;
    } else {
        g_layout.part_b_usable = 0;
    }

    g_layout.is_dual = (g_layout.part_a_usable > 0 && g_layout.part_b_usable > 0) ? 1 : 0;

    DBG("[BOOT] Layout: %s A=%uKB B=%uKB flags@0x%08X\n",
        g_layout.is_dual ? "DUAL" : "SINGLE",
        (unsigned)(g_layout.part_a_usable / 1024),
        (unsigned)(g_layout.part_b_usable / 1024),
        (unsigned)g_layout.part_flag_addr);
}

static uint32_t crc32_calc(const uint8_t *buf, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= (uint32_t)buf[i];
        for (j = 0; j < 8u; j++)
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320UL) : (crc >> 1);
    }
    return crc ^ 0xFFFFFFFFUL;
}

static void part_flag_seal(PartFlag_t *f)
{
    f->magic = PART_FLAG_MAGIC;
    f->crc32 = crc32_calc((const uint8_t *)f,
                          sizeof(PartFlag_t) - sizeof(uint32_t));
}

static int part_flag_valid(const PartFlag_t *f)
{
    if (f->magic != PART_FLAG_MAGIC) return 0;
    return (crc32_calc((const uint8_t *)f,
                       sizeof(PartFlag_t) - sizeof(uint32_t))
            == f->crc32) ? 1 : 0;
}

int PartFlag_Read(PartFlag_t *flag)
{
    memcpy(flag, (const void *)g_layout.part_flag_addr, sizeof(PartFlag_t));
    return part_flag_valid(flag) ? 1 : 0;
}

void PartFlag_Default(PartFlag_t *flag)
{
    memset(flag, 0, sizeof(PartFlag_t));
    flag->active_part   = 0;
    flag->reserved1     = 0;
    flag->boot_fail_cnt = 0;
    flag->reserved2     = 0;
    part_flag_seal(flag);
}

int PartFlag_Write(const PartFlag_t *flag)
{
    PartFlag_t tmp;
    memcpy(&tmp, flag, sizeof(PartFlag_t));
    part_flag_seal(&tmp);

    /* Use SpiFlashErase directly — FlashErase capacity check is unreliable in BL */
    SpiFlashIOCtrl(IOCTL_FLASH_UNPROTECT, "\x35\xBA\x69", 3);
    SpiFlashErase(SECTOR_ERASE, g_layout.part_flag_addr / FLASH_SECTOR_SZ, 0);
    return (SpiFlashWrite(g_layout.part_flag_addr, (uint8_t *)&tmp,
                          sizeof(PartFlag_t), 0) == FLASH_NONE_ERR) ? 1 : 0;
}

void Boot_JumpTo(uint32_t addr)
{
    typedef void (*Entry_t)(void);
    Entry_t entry;

    WDG_Disable();
    __nds32__setgie_dis();
    entry = (Entry_t)addr;
    entry();
    while (1);
}

static int fw_looks_valid(uint32_t base)
{
    volatile const uint32_t *first_word = (volatile const uint32_t *)base;
    volatile const uint32_t *fw_magic =
        (volatile const uint32_t *)(base + FW_VALID_MAGIC_OFFSET);

    if (*fw_magic == FW_VALID_MAGIC) {
        return 1;
    }
    if (*first_word != 0xFFFFFFFFu && *first_word != PART_FLAG_MAGIC) {
        return 1;
    }
    return 0;
}

void Boot_CheckAndJumpIfNeeded(void)
{
    uint32_t flag_val = *(volatile const uint32_t *)BURN_FLAG_ADDR;
    if (flag_val == BURN_FLAG_MAGIC) {
        DBG("[BOOT] Burn flag set — stay in upgrade mode\n");
        SpiFlashErase(SECTOR_ERASE, BURN_FLAG_SECTOR, 0);
        return;
    }

    DataCacheInvalidAll();
    __nds32__dsb();

    if (!g_layout.is_dual) {
        if (!fw_looks_valid(PART_A_BASE)) {
            DBG("[BOOT] No valid firmware — stay for CDC upgrade\n");
            return;
        }
        DBG("[BOOT] Jumping to Part A @ 0x%08X\n", (unsigned)PART_A_BASE);
        Boot_JumpTo(PART_A_BASE);
        return;
    }

    {
        PartFlag_t flag;
        uint32_t jump_addr;

        if (!PartFlag_Read(&flag)) {
            PartFlag_Default(&flag);
            jump_addr = PART_A_BASE;
        } else {
            jump_addr = (flag.active_part == 1) ? PART_B_BASE : PART_A_BASE;

            if (flag.active_part == 1 && g_layout.part_b_usable == 0) {
                flag.active_part   = 0;
                flag.boot_fail_cnt = 0;
                PartFlag_Write(&flag);
                jump_addr = PART_A_BASE;
            }

            if (flag.boot_fail_cnt >= BOOT_FAIL_MAX) {
                uint8_t other = (flag.active_part == 0) ? 1u : 0u;
                uint32_t other_sz = (other == 0) ? g_layout.part_a_usable
                                                 : g_layout.part_b_usable;
                if (other_sz > 0) {
                    flag.active_part   = other;
                    flag.boot_fail_cnt = 1;
                    PartFlag_Write(&flag);
                    jump_addr = (flag.active_part == 1) ? PART_B_BASE : PART_A_BASE;
                } else {
                    DBG("[BOOT] Boot fail limit, no backup — stay in BL\n");
                    return;
                }
            } else {
                flag.boot_fail_cnt++;
                PartFlag_Write(&flag);
            }
        }

        if (!fw_looks_valid(jump_addr)) {
            uint32_t other = (jump_addr == PART_A_BASE) ? PART_B_BASE : PART_A_BASE;
            if (other == PART_B_BASE && g_layout.part_b_usable == 0) {
                DBG("[BOOT] No valid firmware — stay for CDC upgrade\n");
                return;
            }
            if (!fw_looks_valid(other)) {
                DBG("[BOOT] No valid firmware — stay for CDC upgrade\n");
                return;
            }
            flag.active_part   = (other == PART_B_BASE) ? 1u : 0u;
            flag.boot_fail_cnt = 0;
            PartFlag_Write(&flag);
            jump_addr = other;
        }

        if (jump_addr == PART_B_BASE) {
            Remap_AddrRemapSet(ADDR_REMAP0, PART_A_BASE, PART_B_BASE,
                               (uint32_t)(PART_A_SIZE / 1024UL));
            jump_addr = PART_A_BASE;
            DBG("[BOOT] Remap B->A window, jump 0x040000\n");
        }

        DBG("[BOOT] Jumping to 0x%08X\n", (unsigned)jump_addr);
        Boot_JumpTo(jump_addr);
    }
}
