/**
 *****************************************************************************
 * @file     flash_nand_w25n02.h
 * @author   BG Card Team
 * @version  V1.0.0
 * @date     03-April-2026
 * @brief    W25N02xx SPI NAND Flash 驱动 (含坏块管理)
 *
 * 支持型号: W25N02KV (2Gbit / 256MB)
 *
 * 规格:
 *   - 容量      : 2Gbit = 256MB
 *   - 页大小    : 2048 bytes (数据) + 64 bytes (spare/OOB)
 *   - 块大小    : 64 pages = 128KB
 *   - 块数量    : 2048 blocks
 *   - 页地址    : 16-bit (0x0000 ~ 0x07FF)
 *   - 接口      : Standard/Dual/Quad SPI
 *   - JEDEC ID  : EF AA 22
 *
 * 坏块管理 (BBM):
 *   - 初始化时扫描所有块的 OOB[0]，0xFF=好块，否则为坏块
 *   - RAM 中维护 2048-bit 位图 (256 bytes) 标记坏块
 *   - 擦除/编程失败时自动标记坏块
 *   - 提供 W25N02_IsBadBlock() / W25N02_MarkBadBlock() API
 *****************************************************************************
 */

#ifndef __FLASH_NAND_W25N02_H__
#define __FLASH_NAND_W25N02_H__

#include "flash_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * W25N02 规格常量
 *===========================================================================*/

#define W25N02_PAGE_SIZE            2048U       /* 数据页大小 (bytes) */
#define W25N02_OOB_SIZE             64U         /* spare/OOB 大小 (bytes) */
#define W25N02_PAGES_PER_BLOCK      64U         /* 每块页数 */
#define W25N02_BLOCK_COUNT          2048U       /* 总块数 */
#define W25N02_BLOCK_SIZE           (W25N02_PAGE_SIZE * W25N02_PAGES_PER_BLOCK)  /* 128KB */
#define W25N02_TOTAL_SIZE           ((uint32_t)W25N02_BLOCK_COUNT * W25N02_BLOCK_SIZE) /* 256MB */
#define W25N02_BBT_SIZE_BYTES       (W25N02_BLOCK_COUNT / 8U)  /* 坏块位图大小 (256 bytes) */

/*===========================================================================
 * W25N02 SPI 命令集
 *===========================================================================*/

#define W25N02_CMD_RESET            0xFF    /* 设备复位 */
#define W25N02_CMD_READ_JEDEC_ID    0x9F    /* 读取 JEDEC ID */
#define W25N02_CMD_READ_ID          0x90    /* 读取制造商/设备 ID */
#define W25N02_CMD_GET_FEATURE      0x0F    /* 读取特性寄存器 */
#define W25N02_CMD_SET_FEATURE      0x1F    /* 写特性寄存器 */
#define W25N02_CMD_WRITE_ENABLE     0x06    /* 写使能 */
#define W25N02_CMD_WRITE_DISABLE    0x04    /* 写禁止 */
#define W25N02_CMD_PAGE_READ        0x13    /* 读页到缓存 (page addr → cache) */
#define W25N02_CMD_READ_CACHE       0x03    /* 从缓存读数据 (slow) */
#define W25N02_CMD_READ_CACHE_FAST  0x0B    /* 从缓存快速读数据 */
#define W25N02_CMD_READ_CACHE_X2    0x3B    /* 双线缓存读 */
#define W25N02_CMD_READ_CACHE_X4    0x6B    /* 四线缓存读 */
#define W25N02_CMD_LOAD_PROG_DATA   0x02    /* 加载数据到缓存 (写入) */
#define W25N02_CMD_PROG_EXECUTE     0x10    /* 编程执行 (缓存 → Flash) */
#define W25N02_CMD_BLOCK_ERASE      0xD8    /* 块擦除 */
#define W25N02_CMD_READ_BBM_LUT     0xA5    /* 读取内置 BBM 查找表 */
#define W25N02_CMD_LAST_ECC_FAIL    0xA9    /* 获取最后 ECC 失败的页地址 */
#define W25N02_CMD_PROG_LOAD_RANDOM 0x84    /* 随机加载数据到缓存 */

/*===========================================================================
 * 特性寄存器地址
 *===========================================================================*/

#define W25N02_REG_PROTECTION       0xA0    /* 保护寄存器 */
#define W25N02_REG_CONFIG           0xB0    /* 配置寄存器 */
#define W25N02_REG_STATUS           0xC0    /* 状态寄存器 */

/*===========================================================================
 * 状态寄存器位定义 (addr 0xC0)
 *===========================================================================*/

#define W25N02_SR_OIP               (1 << 0)    /* 操作进行中 (BUSY) */
#define W25N02_SR_WEL               (1 << 1)    /* 写使能锁存 */
#define W25N02_SR_ERASE_FAIL        (1 << 2)    /* 擦除失败 */
#define W25N02_SR_PROG_FAIL         (1 << 3)    /* 编程失败 */
#define W25N02_SR_ECC_S0            (1 << 4)    /* ECC 状态位 0 */
#define W25N02_SR_ECC_S1            (1 << 5)    /* ECC 状态位 1 */

#define W25N02_ECC_OK               (0x00)
#define W25N02_ECC_CORRECTED        (W25N02_SR_ECC_S0)
#define W25N02_ECC_UNCORRECTABLE    (W25N02_SR_ECC_S1)
#define W25N02_ECC_MASK             (W25N02_SR_ECC_S0 | W25N02_SR_ECC_S1)

/*===========================================================================
 * 配置寄存器位定义 (addr 0xB0)
 *===========================================================================*/

#define W25N02_CFG_ECC_EN           (1 << 4)    /* ECC 使能 (默认 1) */
#define W25N02_CFG_BUF_EN           (1 << 3)    /* 缓冲读模式 (1=buffer, 0=continuous) */

/*===========================================================================
 * 厂商 ID
 *===========================================================================*/

#define W25N02_MFG_WINBOND          0xEF        /* Winbond 厂商 ID */
#define W25N02_MEM_TYPE             0xAA        /* NAND Flash 类型 */
#define W25N02_DEV_ID               0x22        /* W25N02KV 设备 ID */

/*===========================================================================
 * 超时设置 (ms)
 *===========================================================================*/

#define W25N02_TIMEOUT_PAGE_READ    1           /* 页读取超时 (典型 <60us, 最大 115us) */
#define W25N02_TIMEOUT_PAGE_PROG    5           /* 页编程超时 (典型 300us, 最大 700us) */
#define W25N02_TIMEOUT_BLOCK_ERASE  10          /* 块擦除超时 (典型 2ms, 最大 5ms) */
#define W25N02_TIMEOUT_RESET        1           /* 复位超时 */

/*===========================================================================
 * 坏块管理 (BBM) 数据结构
 *===========================================================================*/

typedef struct {
    uint8_t  bbt[W25N02_BBT_SIZE_BYTES];    /* 坏块位图: bit=1 表示坏块 */
    uint16_t bad_count;                      /* 坏块总数 */
    bool     scanned;                        /* 是否已完成扫描 */
} W25N02_BBM_t;

/*===========================================================================
 * W25N02 驱动接口
 *===========================================================================*/

FlashDevice_t* W25N02_Create(const char *name,
                             void (*cs_select)(void),
                             void (*cs_deselect)(void),
                             void (*cs_init)(void));

void W25N02_Destroy(FlashDevice_t *dev);

const FlashOps_t* W25N02_GetOps(void);

FlashStatus_t W25N02_ResetDevice(FlashDevice_t *dev);

FlashStatus_t W25N02_ScanBBT(FlashDevice_t *dev);

bool W25N02_IsBadBlock(FlashDevice_t *dev, uint16_t block_addr);

FlashStatus_t W25N02_MarkBadBlock(FlashDevice_t *dev, uint16_t block_addr);

FlashStatus_t W25N02_GetBBM(FlashDevice_t *dev, const W25N02_BBM_t **out_bbm);

FlashStatus_t W25N02_ReadPage(FlashDevice_t *dev, uint32_t page_addr,
                              uint16_t col_addr, uint8_t *buf, uint32_t len);

FlashStatus_t W25N02_ProgramPage(FlashDevice_t *dev, uint32_t page_addr,
                                 uint16_t col_addr, const uint8_t *buf, uint32_t len);

FlashStatus_t W25N02_EraseBlock(FlashDevice_t *dev, uint16_t block_addr);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_NAND_W25N02_H__ */
