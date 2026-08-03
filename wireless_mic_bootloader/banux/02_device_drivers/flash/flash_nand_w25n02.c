/**
 *****************************************************************************
 * @file     flash_nand_w25n02.c
 * @author   BG Card Team
 * @version  V1.0.0
 * @date     03-April-2026
 * @brief    W25N02xx SPI NAND Flash 驱动实现 (含坏块管理)
 *
 * SPI 通信使用与 flash_nor_w25qxx.c 相同的 SPIM DMA 方式。
 *****************************************************************************
 */

#include "flash_nand_w25n02.h"
#include "spim.h"
#include "spim_interface.h"
#include "dma.h"
#include "gpio.h"
#include "debug.h"
#include "rtos_api.h"
#include <string.h>
#include "rtos_api.h"

/*===========================================================================
 * 调试宏
 *===========================================================================*/

#define W25N02_DEBUG    1

#if W25N02_DEBUG
    #define NAND_LOG(fmt, ...)  DBG("[W25N02] " fmt, ##__VA_ARGS__)
#else
    #define NAND_LOG(...)
#endif

/*===========================================================================
 * 私有数据结构
 *===========================================================================*/

typedef struct {
    W25N02_BBM_t bbm;       /* 坏块管理 */
    bool         ecc_on;    /* ECC 是否使能 */
} W25N02Priv_t;

/*===========================================================================
 * 函数前向声明
 *===========================================================================*/

static FlashStatus_t W25N02_Init(FlashDevice_t *dev);
static FlashStatus_t W25N02_DeInit(FlashDevice_t *dev);
static FlashStatus_t W25N02_Read(FlashDevice_t *dev, uint32_t addr, uint8_t *buf, uint32_t len);
static FlashStatus_t W25N02_Write(FlashDevice_t *dev, uint32_t addr, const uint8_t *buf, uint32_t len);
static FlashStatus_t W25N02_EraseSector(FlashDevice_t *dev, uint32_t addr);
static FlashStatus_t W25N02_EraseBlockByAddr(FlashDevice_t *dev, uint32_t addr);
static FlashStatus_t W25N02_EraseChip(FlashDevice_t *dev);
static FlashStatus_t W25N02_GetStatus(FlashDevice_t *dev, uint8_t *status);
static FlashStatus_t W25N02_WaitReady(FlashDevice_t *dev, uint32_t timeout_ms);
static FlashStatus_t W25N02_ReadID(FlashDevice_t *dev);
static FlashStatus_t W25N02_GetInfo(FlashDevice_t *dev, FlashDevInfo_t *info);

/*===========================================================================
 * 驱动操作表
 *===========================================================================*/

static const FlashOps_t g_w25n02_ops = {
    .init         = W25N02_Init,
    .deinit       = W25N02_DeInit,
    .read         = W25N02_Read,
    .write        = W25N02_Write,
    .erase_sector = W25N02_EraseSector,      /* NAND 无 sector, 映射到 block erase */
    .erase_block  = W25N02_EraseBlockByAddr,
    .erase_chip   = W25N02_EraseChip,
    .get_status   = W25N02_GetStatus,
    .wait_ready   = W25N02_WaitReady,
    .read_id      = W25N02_ReadID,
    .get_info     = W25N02_GetInfo
};

const FlashOps_t* W25N02_GetOps(void)
{
    return &g_w25n02_ops;
}

/*===========================================================================
 * SPI 底层操作 (复用 SPIM DMA 接口)
 *===========================================================================*/

static void spi_write_byte(uint8_t data)
{
    SPIM_DMA_Send_Start(&data, 1);
    while (!SPIM_DMA_HalfDone(PERIPHERAL_ID_SPIM_TX));
}

static uint8_t spi_read_byte(void)
{
    uint8_t data = 0;
    SPIM_DMA_Recv_Start(&data, 1);
    while (!SPIM_DMA_HalfDone(PERIPHERAL_ID_SPIM_RX));
    return data;
}

static void spi_write_buf(const uint8_t *data, uint32_t size)
{
    SPIM_DMA_Send_Start((uint8_t *)data, (uint16_t)size);
    while (!SPIM_DMA_HalfDone(PERIPHERAL_ID_SPIM_TX));
}

static void spi_read_buf(uint8_t *data, uint32_t size)
{
    SPIM_DMA_Recv_Start(data, (uint16_t)size);
    while (!SPIM_DMA_HalfDone(PERIPHERAL_ID_SPIM_RX));
}

/*===========================================================================
 * W25N02 寄存器操作
 *===========================================================================*/

static uint8_t nand_get_feature(FlashDevice_t *dev, uint8_t reg_addr)
{
    uint8_t val;
    dev->cs.select();
    spi_write_byte(W25N02_CMD_GET_FEATURE);
    spi_write_byte(reg_addr);
    val = spi_read_byte();
    dev->cs.deselect();
    return val;
}

static void nand_set_feature(FlashDevice_t *dev, uint8_t reg_addr, uint8_t val)
{
    dev->cs.select();
    spi_write_byte(W25N02_CMD_SET_FEATURE);
    spi_write_byte(reg_addr);
    spi_write_byte(val);
    dev->cs.deselect();
}

static void nand_write_enable(FlashDevice_t *dev)
{
    dev->cs.select();
    spi_write_byte(W25N02_CMD_WRITE_ENABLE);
    dev->cs.deselect();
}

static uint8_t nand_wait_busy(FlashDevice_t *dev, uint32_t timeout_ms)
{
    uint8_t sr;
    uint32_t start_tick, elapsed_ms;
    int rtos_running;
    
    rtos_running = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
    
    if (rtos_running && timeout_ms > 10u) {
        start_tick = (uint32_t)xTaskGetTickCount();
        do {
            sr = nand_get_feature(dev, W25N02_REG_STATUS);
            if (!(sr & W25N02_SR_OIP)) {
                break;
            }
            vTaskDelay(1);
            elapsed_ms = ((uint32_t)xTaskGetTickCount() - start_tick) * portTICK_PERIOD_MS;
        } while (elapsed_ms < timeout_ms);
    } else {
        uint32_t poll_count = 0;
        uint32_t max_polls = timeout_ms * 1000;
        do {
            sr = nand_get_feature(dev, W25N02_REG_STATUS);
            if (!(sr & W25N02_SR_OIP)) {
                break;
            }
            poll_count++;
        } while (poll_count < max_polls);
    }
    
    return sr;
}

/*===========================================================================
 * W25N02 页操作
 *===========================================================================*/

static void nand_reset(FlashDevice_t *dev)
{
    dev->cs.select();
    spi_write_byte(W25N02_CMD_RESET);
    dev->cs.deselect();
    nand_wait_busy(dev, W25N02_TIMEOUT_RESET + 2);
}

static FlashStatus_t nand_load_page_to_cache(FlashDevice_t *dev, uint32_t page_addr)
{
    uint8_t sr;

    dev->cs.select();
    spi_write_byte(W25N02_CMD_PAGE_READ);
    spi_write_byte((uint8_t)((page_addr >> 16) & 0xFF));
    spi_write_byte((uint8_t)((page_addr >>  8) & 0xFF));
    spi_write_byte((uint8_t)( page_addr        & 0xFF));
    dev->cs.deselect();

    sr = nand_wait_busy(dev, W25N02_TIMEOUT_PAGE_READ + 2);

    if ((sr & W25N02_ECC_MASK) == W25N02_ECC_UNCORRECTABLE) {
        NAND_LOG("ECC uncorrectable error at page 0x%04X\n", page_addr);
        return FLASH_ERR_ECC;
    }
    return FLASH_OK;
}

static void nand_read_from_cache(FlashDevice_t *dev, uint16_t col_addr,
                                  uint8_t *buf, uint32_t len)
{
    uint32_t remaining = len;
    uint32_t offset    = 0;
    uint32_t chunk;

    while (remaining > 0) {
        chunk = (remaining > 0xFFFFu) ? 0xFFFFu : remaining;

        dev->cs.select();
        spi_write_byte(W25N02_CMD_READ_CACHE_FAST);
        spi_write_byte((uint8_t)((col_addr >> 8) & 0x0F));
        spi_write_byte((uint8_t)(col_addr & 0xFF));
        spi_write_byte(0x00);
        spi_read_buf(buf + offset, chunk);
        dev->cs.deselect();

        offset    += chunk;
        remaining -= chunk;
        col_addr  += (uint16_t)chunk;
    }
}

static void nand_load_to_cache(FlashDevice_t *dev, uint16_t col_addr,
                                const uint8_t *buf, uint32_t len)
{
    dev->cs.select();
    spi_write_byte(W25N02_CMD_LOAD_PROG_DATA);
    spi_write_byte((uint8_t)((col_addr >> 8) & 0x0F));
    spi_write_byte((uint8_t)(col_addr & 0xFF));
    spi_write_buf(buf, len);
    dev->cs.deselect();
}

static FlashStatus_t nand_program_execute(FlashDevice_t *dev, uint32_t page_addr)
{
    uint8_t sr;

    dev->cs.select();
    spi_write_byte(W25N02_CMD_PROG_EXECUTE);
    spi_write_byte((uint8_t)((page_addr >> 16) & 0xFF));
    spi_write_byte((uint8_t)((page_addr >>  8) & 0xFF));
    spi_write_byte((uint8_t)( page_addr        & 0xFF));
    dev->cs.deselect();

    sr = nand_wait_busy(dev, W25N02_TIMEOUT_PAGE_PROG + 2);

    if (sr & W25N02_SR_PROG_FAIL) {
        NAND_LOG("Program failed at page 0x%04X\n", page_addr);
        return FLASH_ERR_PROGRAM;
    }
    return FLASH_OK;
}

/*===========================================================================
 * 坏块管理内部函数
 *===========================================================================*/

static uint8_t nand_read_oob0(FlashDevice_t *dev, uint16_t block_addr)
{
    uint8_t oob0 = 0xFF;
    uint32_t page_addr = (uint32_t)block_addr * W25N02_PAGES_PER_BLOCK;
    FlashStatus_t ret;

    ret = nand_load_page_to_cache(dev, page_addr);
    if (ret == FLASH_OK || ret == FLASH_ERR_ECC) {
        nand_read_from_cache(dev, W25N02_PAGE_SIZE, &oob0, 1);
    }
    return oob0;
}

static void bbm_set_bad(W25N02_BBM_t *bbm, uint16_t block_addr)
{
    if (block_addr < W25N02_BLOCK_COUNT) {
        uint16_t byte_idx = block_addr / 8u;
        uint8_t  bit_idx  = (uint8_t)(block_addr % 8u);
        if (!(bbm->bbt[byte_idx] & (1u << bit_idx))) {
            bbm->bbt[byte_idx] |= (1u << bit_idx);
            bbm->bad_count++;
        }
    }
}

static bool bbm_is_bad(const W25N02_BBM_t *bbm, uint16_t block_addr)
{
    if (block_addr >= W25N02_BLOCK_COUNT) {
        return true;
    }
    uint16_t byte_idx = block_addr / 8u;
    uint8_t  bit_idx  = (uint8_t)(block_addr % 8u);
    return (bbm->bbt[byte_idx] & (1u << bit_idx)) != 0u;
}

/*===========================================================================
 * FlashOps_t 接口实现
 *===========================================================================*/

static FlashStatus_t W25N02_Init(FlashDevice_t *dev)
{
    W25N02Priv_t *priv;
    uint8_t       id[3];
    uint8_t       cfg;

    if (!dev) {
        return FLASH_ERR_PARAM;
    }

    if (dev->cs.init) {
        dev->cs.init();
    }
    if (dev->cs.deselect) {
        dev->cs.deselect();
    }

    priv = (W25N02Priv_t *)pvPortMalloc(sizeof(W25N02Priv_t));
    if (!priv) {
        NAND_LOG("pvPortMalloc failed\n");
        return FLASH_ERR_NOMEM;
    }
    memset(priv, 0, sizeof(W25N02Priv_t));
    dev->priv = priv;

    nand_reset(dev);

    dev->cs.select();
    spi_write_byte(W25N02_CMD_READ_JEDEC_ID);
    spi_write_byte(0x00);
    id[0] = spi_read_byte();
    id[1] = spi_read_byte();
    id[2] = spi_read_byte();
    dev->cs.deselect();

    NAND_LOG("JEDEC ID: %02X %02X %02X\n", id[0], id[1], id[2]);

    if ((id[0] == 0x00u && id[1] == 0x00u && id[2] == 0x00u) ||
        (id[0] == 0xFFu && id[1] == 0xFFu && id[2] == 0xFFu) ||
        (id[2] != W25N02_DEV_ID && id[2] != 0xAAu)) {
        NAND_LOG("No valid W25N02 ID detected\n");
        vPortFree(priv);
        dev->priv = NULL;
        return FLASH_ERR_NOT_FOUND;
    }

    if (id[0] != W25N02_MFG_WINBOND) {
        NAND_LOG("Warning: unexpected manufacturer 0x%02X\n", id[0]);
    }

    dev->info.mfg_id    = id[0];
    dev->info.mem_type  = id[1];
    dev->info.dev_id    = id[2];
    dev->info.page_size   = W25N02_PAGE_SIZE;
    dev->info.sector_size = W25N02_BLOCK_SIZE;
    dev->info.block_size  = W25N02_BLOCK_SIZE;
    dev->info.block_count = W25N02_BLOCK_COUNT;
    dev->info.total_size  = W25N02_TOTAL_SIZE;
    dev->type = FLASH_TYPE_NAND;

    cfg = nand_get_feature(dev, W25N02_REG_CONFIG);
    cfg |= (W25N02_CFG_ECC_EN | W25N02_CFG_BUF_EN);
    nand_set_feature(dev, W25N02_REG_CONFIG, cfg);
    priv->ecc_on = true;

    {
        uint8_t prot;
        nand_write_enable(dev);
        nand_set_feature(dev, W25N02_REG_PROTECTION, 0x00);
        nand_wait_busy(dev, 5);
        prot = nand_get_feature(dev, W25N02_REG_PROTECTION);
        if (prot != 0x00) {
            NAND_LOG("Warn: PROT reg=0x%02X, retry unprotect\n", prot);
            nand_write_enable(dev);
            nand_set_feature(dev, W25N02_REG_PROTECTION, 0x00);
            nand_wait_busy(dev, 5);
            prot = nand_get_feature(dev, W25N02_REG_PROTECTION);
            NAND_LOG("PROT reg after 2nd attempt: 0x%02X\n", prot);
        } else {
            NAND_LOG("Write protection disabled (PROT=0x00)\n");
        }
    }

    dev->initialized = true;

    NAND_LOG("Init OK. Type=NAND, Size=%lu MB, Blocks=%u\n",
             (unsigned long)(W25N02_TOTAL_SIZE / 1024 / 1024),
             W25N02_BLOCK_COUNT);

    return FLASH_OK;
}

static FlashStatus_t W25N02_DeInit(FlashDevice_t *dev)
{
    if (!dev) {
        return FLASH_ERR_PARAM;
    }
    if (dev->priv) {
        vPortFree(dev->priv);
        dev->priv = NULL;
    }
    dev->initialized = false;
    return FLASH_OK;
}

static FlashStatus_t W25N02_Read(FlashDevice_t *dev, uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t  end_addr    = addr + len;
    uint32_t  cur_addr    = addr;
    uint8_t  *dst         = buf;

    if (!dev || !buf || len == 0) {
        return FLASH_ERR_PARAM;
    }
    if (!dev->initialized) {
        return FLASH_ERR_NOT_INIT;
    }

    while (cur_addr < end_addr) {
        uint32_t page_addr  = cur_addr / W25N02_PAGE_SIZE;
        uint16_t col_addr   = (uint16_t)(cur_addr % W25N02_PAGE_SIZE);
        uint32_t page_remain = W25N02_PAGE_SIZE - col_addr;
        uint32_t read_size   = (end_addr - cur_addr);
        FlashStatus_t ret;

        if (read_size > page_remain) {
            read_size = page_remain;
        }

        {
            W25N02Priv_t *priv = (W25N02Priv_t *)dev->priv;
            if (bbm_is_bad(&priv->bbm, (uint16_t)(page_addr / W25N02_PAGES_PER_BLOCK))) {
                NAND_LOG("Read skip bad block %u\n",
                         (unsigned)(page_addr / W25N02_PAGES_PER_BLOCK));
                return FLASH_ERR_BAD_BLOCK;
            }
        }

        ret = nand_load_page_to_cache(dev, page_addr);
        if (ret != FLASH_OK) {
            return ret;
        }
        nand_read_from_cache(dev, col_addr, dst, read_size);

        cur_addr += read_size;
        dst      += read_size;
    }
    return FLASH_OK;
}

static FlashStatus_t W25N02_Write(FlashDevice_t *dev, uint32_t addr,
                                   const uint8_t *buf, uint32_t len)
{
    uint32_t        end_addr  = addr + len;
    uint32_t        cur_addr  = addr;
    const uint8_t  *src       = buf;
    FlashStatus_t   ret;

    if (!dev || !buf || len == 0) {
        return FLASH_ERR_PARAM;
    }
    if (!dev->initialized) {
        return FLASH_ERR_NOT_INIT;
    }

    while (cur_addr < end_addr) {
        uint32_t page_addr   = cur_addr / W25N02_PAGE_SIZE;
        uint16_t col_addr    = (uint16_t)(cur_addr % W25N02_PAGE_SIZE);
        uint32_t page_remain  = W25N02_PAGE_SIZE - col_addr;
        uint32_t write_size   = end_addr - cur_addr;
        uint16_t block_addr   = (uint16_t)(page_addr / W25N02_PAGES_PER_BLOCK);

        if (write_size > page_remain) {
            write_size = page_remain;
        }

        {
            W25N02Priv_t *priv = (W25N02Priv_t *)dev->priv;
            if (bbm_is_bad(&priv->bbm, block_addr)) {
                NAND_LOG("Write skip bad block %u\n", (unsigned)block_addr);
                return FLASH_ERR_BAD_BLOCK;
            }
        }

        nand_write_enable(dev);
        nand_load_to_cache(dev, col_addr, src, write_size);
        ret = nand_program_execute(dev, page_addr);
        if (ret != FLASH_OK) {
            W25N02_MarkBadBlock(dev, block_addr);
            return ret;
        }

        cur_addr += write_size;
        src      += write_size;
    }
    return FLASH_OK;
}

static FlashStatus_t W25N02_EraseSector(FlashDevice_t *dev, uint32_t addr)
{
    uint16_t block_addr = (uint16_t)(addr / W25N02_BLOCK_SIZE);
    return W25N02_EraseBlock(dev, block_addr);
}

static FlashStatus_t W25N02_EraseBlockByAddr(FlashDevice_t *dev, uint32_t addr)
{
    uint16_t block_addr = (uint16_t)(addr / W25N02_BLOCK_SIZE);
    return W25N02_EraseBlock(dev, block_addr);
}

static FlashStatus_t W25N02_EraseChip(FlashDevice_t *dev)
{
    uint16_t blk;
    FlashStatus_t ret = FLASH_OK;

    if (!dev || !dev->initialized) {
        return FLASH_ERR_NOT_INIT;
    }
    NAND_LOG("Chip erase started (%u blocks)...\n", W25N02_BLOCK_COUNT);
    for (blk = 0; blk < W25N02_BLOCK_COUNT; blk++) {
        W25N02Priv_t *priv = (W25N02Priv_t *)dev->priv;
        if (bbm_is_bad(&priv->bbm, blk)) {
            continue;
        }
        ret = W25N02_EraseBlock(dev, blk);
        if (ret != FLASH_OK) {
            NAND_LOG("Chip erase: block %u failed\n", blk);
        }
    }
    NAND_LOG("Chip erase done\n");
    return FLASH_OK;
}

static FlashStatus_t W25N02_GetStatus(FlashDevice_t *dev, uint8_t *status)
{
    if (!dev || !status) {
        return FLASH_ERR_PARAM;
    }
    *status = nand_get_feature(dev, W25N02_REG_STATUS);
    return FLASH_OK;
}

static FlashStatus_t W25N02_WaitReady(FlashDevice_t *dev, uint32_t timeout_ms)
{
    uint8_t sr = nand_wait_busy(dev, timeout_ms);
    if (sr & W25N02_SR_OIP) {
        return FLASH_ERR_TIMEOUT;
    }
    return FLASH_OK;
}

static FlashStatus_t W25N02_ReadID(FlashDevice_t *dev)
{
    uint8_t id[3];

    if (!dev) {
        return FLASH_ERR_PARAM;
    }
    dev->cs.select();
    spi_write_byte(W25N02_CMD_READ_JEDEC_ID);
    spi_write_byte(0x00);
    id[0] = spi_read_byte();
    id[1] = spi_read_byte();
    id[2] = spi_read_byte();
    dev->cs.deselect();

    dev->info.mfg_id   = id[0];
    dev->info.mem_type = id[1];
    dev->info.dev_id   = id[2];
    return FLASH_OK;
}

static FlashStatus_t W25N02_GetInfo(FlashDevice_t *dev, FlashDevInfo_t *info)
{
    if (!dev || !info) {
        return FLASH_ERR_PARAM;
    }
    *info = dev->info;
    return FLASH_OK;
}

/*===========================================================================
 * 公共接口实现
 *===========================================================================*/

FlashDevice_t* W25N02_Create(const char *name,
                             void (*cs_select)(void),
                             void (*cs_deselect)(void),
                             void (*cs_init)(void))
{
    FlashDevice_t *dev;

    if (!name || !cs_select || !cs_deselect) {
        return NULL;
    }

    dev = (FlashDevice_t *)pvPortMalloc(sizeof(FlashDevice_t));
    if (!dev) {
        NAND_LOG("Create failed: out of memory\n");
        return NULL;
    }
    memset(dev, 0, sizeof(FlashDevice_t));

    strncpy(dev->name, name, FLASH_NAME_MAX_LEN - 1);
    dev->name[FLASH_NAME_MAX_LEN - 1] = '\0';
    dev->type        = FLASH_TYPE_NAND;
    dev->ops         = &g_w25n02_ops;
    dev->cs.select   = cs_select;
    dev->cs.deselect = cs_deselect;
    dev->cs.init     = cs_init;

    return dev;
}

void W25N02_Destroy(FlashDevice_t *dev)
{
    if (!dev) {
        return;
    }
    if (dev->initialized) {
        W25N02_DeInit(dev);
    }
    vPortFree(dev);
}

/*===========================================================================
 * 坏块管理公共 API
 *===========================================================================*/

FlashStatus_t W25N02_ResetDevice(FlashDevice_t *dev)
{
    uint8_t cfg;
    uint8_t prot;

    if (!dev || !dev->initialized) {
        return FLASH_ERR_NOT_INIT;
    }

    NAND_LOG("Resetting device...\n");
    nand_reset(dev);

    cfg = nand_get_feature(dev, W25N02_REG_CONFIG);
    cfg |= (W25N02_CFG_ECC_EN | W25N02_CFG_BUF_EN);
    nand_set_feature(dev, W25N02_REG_CONFIG, cfg);

    nand_write_enable(dev);
    nand_set_feature(dev, W25N02_REG_PROTECTION, 0x00);
    nand_wait_busy(dev, 5);
    prot = nand_get_feature(dev, W25N02_REG_PROTECTION);
    if (prot != 0x00) {
        NAND_LOG("Warn: PROT=0x%02X after reset\n", prot);
    } else {
        NAND_LOG("Reset OK, write protection cleared\n");
    }

    return FLASH_OK;
}

FlashStatus_t W25N02_ScanBBT(FlashDevice_t *dev)
{
    W25N02Priv_t *priv;
    uint16_t      blk;
    uint8_t       sr;
    uint32_t      page_addr;

    if (!dev || !dev->initialized) {
        return FLASH_ERR_NOT_INIT;
    }
    priv = (W25N02Priv_t *)dev->priv;

    memset(priv->bbm.bbt, 0, sizeof(priv->bbm.bbt));
    priv->bbm.bad_count = 0;

    {
        uint8_t prot;
        prot = nand_get_feature(dev, W25N02_REG_PROTECTION);
        NAND_LOG("BBT scan: PROT=0x%02X before erase-verify\n", prot);
        if (prot != 0x00u) {
            NAND_LOG("PROT != 0, clearing write protection...\n");
            nand_write_enable(dev);
            nand_set_feature(dev, W25N02_REG_PROTECTION, 0x00);
            nand_wait_busy(dev, 5);
            prot = nand_get_feature(dev, W25N02_REG_PROTECTION);
            NAND_LOG("PROT after clear: 0x%02X\n", prot);
            if (prot != 0x00u) {
                NAND_LOG("ERROR: Cannot clear PROT, all erases will fail!\n");
            }
        }
    }

    NAND_LOG("Scanning bad blocks by erase-verify (%u blocks)...\n", W25N02_BLOCK_COUNT);

    for (blk = 0; blk < W25N02_BLOCK_COUNT; blk++) {
        page_addr = (uint32_t)blk * W25N02_PAGES_PER_BLOCK;

        nand_write_enable(dev);

        dev->cs.select();
        spi_write_byte(W25N02_CMD_BLOCK_ERASE);
        spi_write_byte((uint8_t)((page_addr >> 16) & 0xFF));
        spi_write_byte((uint8_t)((page_addr >>  8) & 0xFF));
        spi_write_byte((uint8_t)( page_addr        & 0xFF));
        dev->cs.deselect();

        sr = nand_wait_busy(dev, W25N02_TIMEOUT_BLOCK_ERASE + 5);

        if (sr & W25N02_SR_ERASE_FAIL) {
            bbm_set_bad(&priv->bbm, blk);
            NAND_LOG("  Bad block: %u (erase fail, SR=0x%02X)\n", (unsigned)blk, sr);
        }

        if (blk == 3u && priv->bbm.bad_count >= 4u) {
            NAND_LOG("ERROR: First 4 blocks ALL failed erase, PROT likely still active!\n");
            NAND_LOG("  Aborting scan. Check SPI bus / protection register.\n");
            priv->bbm.scanned = true;
            return FLASH_ERR_ERASE;
        }

        if (((blk + 1u) & 0xFFu) == 0u) {
            NAND_LOG("  Scanned %u / %u ...\n", (unsigned)(blk + 1u), W25N02_BLOCK_COUNT);
        }
    }

    priv->bbm.scanned = true;
    NAND_LOG("BBT scan done. Bad blocks: %u / %u (%.1f%%)\n",
             priv->bbm.bad_count, W25N02_BLOCK_COUNT,
             (float)priv->bbm.bad_count * 100.0f / W25N02_BLOCK_COUNT);

    return FLASH_OK;
}

bool W25N02_IsBadBlock(FlashDevice_t *dev, uint16_t block_addr)
{
    if (!dev || !dev->priv) {
        return true;
    }
    return bbm_is_bad(&((W25N02Priv_t *)dev->priv)->bbm, block_addr);
}

FlashStatus_t W25N02_MarkBadBlock(FlashDevice_t *dev, uint16_t block_addr)
{
    W25N02Priv_t *priv;
    uint32_t      page_addr;
    uint8_t       mark = 0x00;

    if (!dev || !dev->initialized) {
        return FLASH_ERR_NOT_INIT;
    }
    priv = (W25N02Priv_t *)dev->priv;
    bbm_set_bad(&priv->bbm, block_addr);

    page_addr = (uint32_t)block_addr * W25N02_PAGES_PER_BLOCK;
    nand_write_enable(dev);
    nand_load_to_cache(dev, W25N02_PAGE_SIZE, &mark, 1);
    nand_program_execute(dev, page_addr);

    NAND_LOG("Block %u marked as bad\n", (unsigned)block_addr);
    return FLASH_OK;
}

FlashStatus_t W25N02_GetBBM(FlashDevice_t *dev, const W25N02_BBM_t **out_bbm)
{
    if (!dev || !dev->priv || !out_bbm) {
        return FLASH_ERR_PARAM;
    }
    *out_bbm = &((W25N02Priv_t *)dev->priv)->bbm;
    return FLASH_OK;
}

/*===========================================================================
 * 底层直接操作接口
 *===========================================================================*/

FlashStatus_t W25N02_ReadPage(FlashDevice_t *dev, uint32_t page_addr,
                              uint16_t col_addr, uint8_t *buf, uint32_t len)
{
    FlashStatus_t ret;

    if (!dev || !buf) {
        return FLASH_ERR_PARAM;
    }
    ret = nand_load_page_to_cache(dev, page_addr);
    if (ret != FLASH_OK) {
        return ret;
    }
    nand_read_from_cache(dev, col_addr, buf, len);
    return FLASH_OK;
}

FlashStatus_t W25N02_ProgramPage(FlashDevice_t *dev, uint32_t page_addr,
                                 uint16_t col_addr, const uint8_t *buf, uint32_t len)
{
    FlashStatus_t ret;

    if (!dev || !buf) {
        return FLASH_ERR_PARAM;
    }
    nand_write_enable(dev);
    nand_load_to_cache(dev, col_addr, buf, len);
    ret = nand_program_execute(dev, page_addr);
    if (ret != FLASH_OK) {
        uint16_t block = (uint16_t)(page_addr / W25N02_PAGES_PER_BLOCK);
        W25N02_MarkBadBlock(dev, block);
    }
    return ret;
}

FlashStatus_t W25N02_EraseBlock(FlashDevice_t *dev, uint16_t block_addr)
{
    uint8_t       sr;
    uint32_t      page_addr;

    if (!dev || !dev->initialized) {
        return FLASH_ERR_NOT_INIT;
    }
    if (block_addr >= W25N02_BLOCK_COUNT) {
        return FLASH_ERR_PARAM;
    }

    page_addr = (uint32_t)block_addr * W25N02_PAGES_PER_BLOCK;

    nand_write_enable(dev);

    dev->cs.select();
    spi_write_byte(W25N02_CMD_BLOCK_ERASE);
    spi_write_byte((uint8_t)((page_addr >> 16) & 0xFF));
    spi_write_byte((uint8_t)((page_addr >>  8) & 0xFF));
    spi_write_byte((uint8_t)( page_addr        & 0xFF));
    dev->cs.deselect();

    sr = nand_wait_busy(dev, W25N02_TIMEOUT_BLOCK_ERASE + 5);

    if (sr & W25N02_SR_ERASE_FAIL) {
        NAND_LOG("Erase failed: block %u\n", (unsigned)block_addr);
        W25N02_MarkBadBlock(dev, block_addr);
        return FLASH_ERR_ERASE;
    }
    return FLASH_OK;
}
