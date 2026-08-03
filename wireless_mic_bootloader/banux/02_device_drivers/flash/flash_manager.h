/**
 * flash_manager.h - Flash管理层
 * 
 * 管理多颗Flash芯片的分区和访问：
 * - Flash #0 (W25Q64): 前1MB系统设置 + 后7MB给Looper
 * - Flash #1 (W25Q64): 8MB纯存储
 */

#ifndef __FLASH_MANAGER_H__
#define __FLASH_MANAGER_H__

#include <stdint.h>
#include <stdbool.h>
#include "flash_driver.h"

/*===========================================================================
 * 分区定义
 *===========================================================================*/

/* Flash设备ID */
#define FLASH_DEV_0             0   /* 主Flash (系统+Looper) */
#define FLASH_DEV_1             1   /* 存储Flash */
#define FLASH_DEV_MAX           2

/* Flash #0 分区布局 (W25Q64 = 8MB) */
#define PARTITION_SYSTEM_START      0x000000    /* 系统设置起始地址 */
#define PARTITION_SYSTEM_SIZE       0x100000    /* 系统设置大小: 1MB */
#define PARTITION_LOOPER_START      0x100000    /* Looper起始地址 */
#define PARTITION_LOOPER_SIZE       0x700000    /* Looper大小: 7MB */

/* Flash #1 分区布局 (W25Q64 = 8MB) */
#define PARTITION_STORAGE_START     0x000000    /* 存储起始地址 */
#define PARTITION_STORAGE_SIZE      0x800000    /* 存储大小: 8MB */

/* 系统设置分区内部布局 */
#define SETTINGS_MAGIC_ADDR         0x000000    /* 魔术字地址 */
#define SETTINGS_VERSION_ADDR       0x000004    /* 版本号地址 */
#define SETTINGS_DATA_ADDR          0x000100    /* 数据起始地址 */
#define SETTINGS_BACKUP_ADDR        0x080000    /* 备份区起始地址 (512KB) */
#define SETTINGS_MAGIC_VALUE        0x42475346  /* "BGSF" */

/*===========================================================================
 * 分区类型
 *===========================================================================*/

typedef enum {
    PARTITION_TYPE_SYSTEM = 0,  /* 系统设置分区 */
    PARTITION_TYPE_LOOPER,      /* Looper分区 */
    PARTITION_TYPE_STORAGE,     /* 通用存储分区 */
    PARTITION_TYPE_MAX
} PartitionType_t;

/*===========================================================================
 * 分区信息结构
 *===========================================================================*/

typedef struct {
    PartitionType_t type;       /* 分区类型 */
    uint8_t flash_id;           /* 所属Flash设备ID */
    uint32_t start_addr;        /* 分区起始地址 */
    uint32_t size;              /* 分区大小 */
    const char *name;           /* 分区名称 */
} PartitionInfo_t;

/*===========================================================================
 * Flash管理器状态
 *===========================================================================*/

typedef struct {
    bool initialized;                       /* 是否已初始化 */
    FlashDriver_t *flash[FLASH_DEV_MAX];    /* Flash驱动实例 */
    PartitionInfo_t partitions[PARTITION_TYPE_MAX]; /* 分区信息 */
} FlashManager_t;

/*===========================================================================
 * API函数
 *===========================================================================*/

FlashStatus_t FlashManager_Init(void);
void FlashManager_DeInit(void);
FlashManager_t* FlashManager_GetInstance(void);
FlashDriver_t* FlashManager_GetFlash(uint8_t flash_id);
const PartitionInfo_t* FlashManager_GetPartition(PartitionType_t type);

FlashStatus_t FlashManager_Read(PartitionType_t type, uint32_t offset, uint8_t *buf, uint32_t len);
FlashStatus_t FlashManager_Write(PartitionType_t type, uint32_t offset, const uint8_t *buf, uint32_t len);
FlashStatus_t FlashManager_EraseSector(PartitionType_t type, uint32_t offset);
FlashStatus_t FlashManager_ErasePartition(PartitionType_t type);

FlashStatus_t FlashManager_ReadSettings(uint32_t key, uint8_t *buf, uint32_t len);
FlashStatus_t FlashManager_WriteSettings(uint32_t key, const uint8_t *buf, uint32_t len);
bool FlashManager_IsSettingsValid(void);
FlashStatus_t FlashManager_InitSettings(void);

FlashStatus_t FlashManager_LooperRead(uint32_t offset, uint8_t *buf, uint32_t len);
FlashStatus_t FlashManager_LooperWrite(uint32_t offset, const uint8_t *buf, uint32_t len);
FlashStatus_t FlashManager_LooperEraseSector(uint32_t offset);
uint32_t FlashManager_LooperGetSize(void);

FlashStatus_t FlashManager_StorageRead(uint32_t offset, uint8_t *buf, uint32_t len);
FlashStatus_t FlashManager_StorageWrite(uint32_t offset, const uint8_t *buf, uint32_t len);
FlashStatus_t FlashManager_StorageEraseSector(uint32_t offset);
uint32_t FlashManager_StorageGetSize(void);

void FlashManager_PrintInfo(void);
FlashStatus_t FlashManager_Test(uint8_t flash_id);

#endif /* __FLASH_MANAGER_H__ */
