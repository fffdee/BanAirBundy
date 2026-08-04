/**
 * @file  dual_partition.h
 * @brief Bootloader + dual-partition layout (ported from Banux firmware_upgrade).
 */
#ifndef __DUAL_PARTITION_H__
#define __DUAL_PARTITION_H__

#include "type.h"

#define BOOTLOADER_SIZE         0x00040000UL
#define INTERNAL_ROM_CAPACITY   0x00200000UL

#define PART_A_BASE             0x00040000UL
#define PART_A_SIZE             0x00200000UL
#define PART_B_BASE             0x00240000UL
#define PART_B_SIZE             0x00200000UL

#define PART_FLAG_ADDR_DEFAULT  0x00440000UL
#define PART_FLAG_MAGIC         0x42475057UL
#define FLASH_SECTOR_SZ         0x1000UL

typedef struct {
    uint32_t flash_capacity;
    uint32_t part_a_usable;
    uint32_t part_b_usable;
    uint32_t part_flag_addr;
    uint8_t  is_dual;
} DualPart_Layout_t;

const DualPart_Layout_t *DualPart_GetLayout(void);
void DualPart_Init(void);

#define FW_VALID_MAGIC          0x42475046UL
#define FW_VALID_MAGIC_OFFSET   0x000000A4UL
#define BOOT_FAIL_MAX           3

typedef struct {
    uint32_t magic;
    uint8_t  active_part;
    uint8_t  reserved1;
    uint8_t  boot_fail_cnt;
    uint8_t  reserved2;
    uint32_t crc32;
} PartFlag_t;

#define UPG_SOF             0xAAU
#define UPG_VERSION         0x04U
#define UPG_MAX_CHUNK       256U

#define CMD_SYNC            0x01U
#define CMD_START           0x02U
#define CMD_DATA            0x03U
#define CMD_FINISH          0x04U
#define CMD_JUMP            0x05U
#define CMD_ERASE           0x06U
#define CMD_QUERY_INFO      0x07U
#define CMD_SET_PART        0x08U
#define CMD_REBOOT          0x09U
#define CMD_ENTER_BOOT      0x0BU

#define RSP_ACK             0xA1U
#define RSP_NACK            0xA2U

#define UPG_ERR_CRC         0x01U
#define UPG_ERR_FLASH       0x02U
#define UPG_ERR_SIZE        0x03U
#define UPG_ERR_STATE       0x04U
#define UPG_ERR_PARAM       0x05U
#define UPG_ERR_WRONG_PART  0x06U

typedef struct {
    uint8_t  boot_mode;
    uint8_t  active_part;
    uint8_t  boot_fail_cnt;
    uint8_t  protocol_ver;
    uint32_t part_a_base;
    uint32_t part_a_size;
    uint32_t part_b_base;
    uint32_t part_b_size;
} DevInfo_t;

#define BOOT_MODE_DUAL_AB  1
#define BOOT_MODE_SINGLE   0

#define BURN_FLAG_ADDR      0x0003F000UL
#define BURN_FLAG_MAGIC     0x4F4F5442UL
#define BURN_FLAG_SECTOR    (BURN_FLAG_ADDR / 4096U)

#define PART_FLAG_ADDR  (DualPart_GetLayout()->part_flag_addr)

int  PartFlag_Read(PartFlag_t *flag);
int  PartFlag_Write(const PartFlag_t *flag);
void PartFlag_Default(PartFlag_t *flag);

/**
 * @brief Check burn flag / partition flags and jump to APP if valid.
 *        Returns only when bootloader should stay for USB CDC upgrade.
 */
void Boot_CheckAndJumpIfNeeded(void);

void Boot_JumpTo(uint32_t addr);

#endif /* __DUAL_PARTITION_H__ */
