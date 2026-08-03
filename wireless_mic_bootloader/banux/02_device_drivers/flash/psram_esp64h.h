/**
 * psram_esp64h.h - ESP-PSRAM64H SPI PSRAM 驱动
 */

#ifndef __PSRAM_ESP64H_H__
#define __PSRAM_ESP64H_H__

#include "flash_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * ESP-PSRAM64H 规格常量
 *===========================================================================*/

#define PSRAM64H_TOTAL_SIZE         (8u * 1024u * 1024u)  /* 8MB */
#define PSRAM64H_PAGE_SIZE          1024u                  /* 页边界: 1KB */
#define PSRAM64H_SECTOR_SIZE        PSRAM64H_PAGE_SIZE
#define PSRAM64H_BLOCK_SIZE         (64u * 1024u)          /* 虚拟块: 64KB */
#define PSRAM64H_BLOCK_COUNT        (PSRAM64H_TOTAL_SIZE / PSRAM64H_BLOCK_SIZE) /* 128 */

/*===========================================================================
 * ESP-PSRAM64H SPI 命令集
 *===========================================================================*/

#define PSRAM64H_CMD_READ           0x03
#define PSRAM64H_CMD_FAST_READ      0x0B
#define PSRAM64H_CMD_WRITE          0x02
#define PSRAM64H_CMD_QUAD_READ      0xEB
#define PSRAM64H_CMD_QUAD_WRITE     0x38
#define PSRAM64H_CMD_ENTER_QPI      0x35
#define PSRAM64H_CMD_EXIT_QPI       0xF5
#define PSRAM64H_CMD_RESET_ENABLE   0x66
#define PSRAM64H_CMD_RESET          0x99
#define PSRAM64H_CMD_READ_ID        0x9F
#define PSRAM64H_CMD_WRAP_TOGGLE    0xC0

/*===========================================================================
 * 厂商 / 设备 ID
 *===========================================================================*/

#define PSRAM64H_KNOWN_MFG_ID       0x0D
#define PSRAM64H_KNOWN_KGD          0x5D

/*===========================================================================
 * 超时设置
 *===========================================================================*/

#define PSRAM64H_TIMEOUT_RESET_US   150
#define PSRAM64H_DMA_MAX_CHUNK      2048u

/*===========================================================================
 * ESP-PSRAM64H 驱动接口
 *===========================================================================*/

FlashDevice_t* PSRAM64H_Create(const char *name,
                               void (*cs_select)(void),
                               void (*cs_deselect)(void),
                               void (*cs_init)(void));

void PSRAM64H_Destroy(FlashDevice_t *dev);

const FlashOps_t* PSRAM64H_GetOps(void);

FlashStatus_t PSRAM64H_DirectRead(FlashDevice_t *dev, uint32_t addr,
                                  uint8_t *buf, uint32_t len);

FlashStatus_t PSRAM64H_DirectWrite(FlashDevice_t *dev, uint32_t addr,
                                   const uint8_t *buf, uint32_t len);

FlashStatus_t PSRAM64H_ReadID(FlashDevice_t *dev, uint8_t *mfg_id, uint8_t *kgd);

void PSRAM64H_Reset(FlashDevice_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* __PSRAM_ESP64H_H__ */
