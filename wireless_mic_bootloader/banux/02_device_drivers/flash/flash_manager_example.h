/**
 * flash_manager_example.h - Flash管理器使用示例
 * 
 * 展示如何使用重构后的Flash驱动和管理层
 */

#ifndef __FLASH_MANAGER_EXAMPLE_H__
#define __FLASH_MANAGER_EXAMPLE_H__

#include "flash_manager.h"

/*===========================================================================
 * 使用示例
 *===========================================================================*/

/**
 * 示例1: 初始化Flash管理器
 */
static inline void example_init(void)
{
    FlashStatus_t ret = FlashManager_Init();
    if (ret != FLASH_OK) {
        DBG("Flash Manager init failed!\n");
        return;
    }
    
    if (!FlashManager_IsSettingsValid()) {
        DBG("Settings invalid, initializing...\n");
        FlashManager_InitSettings();
    }
    
    FlashManager_PrintInfo();
}

/**
 * 示例2: 系统设置读写
 */

#define SETTING_VOLUME          0x0000  /* 音量设置，4字节 */
#define SETTING_EQ_MODE         0x0004  /* EQ模式，4字节 */
#define SETTING_BLUETOOTH_NAME  0x0100  /* 蓝牙名称，32字节 */
#define SETTING_USER_DATA       0x0200  /* 用户数据区 */

static inline void example_settings(void)
{
    uint32_t volume;
    FlashManager_ReadSettings(SETTING_VOLUME, (uint8_t*)&volume, sizeof(volume));
    DBG("Current volume: %d\n", volume);
    
    volume = 80;
    FlashManager_EraseSector(PARTITION_TYPE_SYSTEM, 0);
    
    uint32_t magic = SETTINGS_MAGIC_VALUE;
    FlashManager_Write(PARTITION_TYPE_SYSTEM, SETTINGS_MAGIC_ADDR, (uint8_t*)&magic, sizeof(magic));
    FlashManager_WriteSettings(SETTING_VOLUME, (uint8_t*)&volume, sizeof(volume));
}

/**
 * 示例3: Looper分区读写
 */
static inline void example_looper(void)
{
    uint8_t audio_data[512];
    uint32_t offset = 0;
    
    uint32_t looper_size = FlashManager_LooperGetSize();
    DBG("Looper partition size: %d KB\n", looper_size / 1024);
    
    FlashManager_LooperEraseSector(offset);
    FlashManager_LooperWrite(offset, audio_data, sizeof(audio_data));
    FlashManager_LooperRead(offset, audio_data, sizeof(audio_data));
}

/**
 * 示例4: 存储分区读写 (第二颗Flash)
 */
static inline void example_storage(void)
{
    uint8_t data[256];
    uint32_t offset = 0;
    int i;
    
    uint32_t storage_size = FlashManager_StorageGetSize();
    DBG("Storage partition size: %d KB\n", storage_size / 1024);
    
    FlashManager_StorageEraseSector(offset);
    
    for (i = 0; i < 256; i++) {
        data[i] = i;
    }
    FlashManager_StorageWrite(offset, data, sizeof(data));
    FlashManager_StorageRead(offset, data, sizeof(data));
}

/**
 * 示例5: 直接访问底层驱动
 */
static inline void example_direct_access(void)
{
    FlashDriver_t *flash0 = FlashManager_GetFlash(FLASH_DEV_0);
    if (flash0) {
        DBG("Flash #0 size: %d MB\n", flash0->info.total_size / (1024*1024));
        
        uint8_t mfg, type, dev;
        flash0->read_id(flash0, &mfg, &type, &dev);
        DBG("Flash #0 ID: %02X %02X %02X\n", mfg, type, dev);
    }
    
    FlashDriver_t *flash1 = FlashManager_GetFlash(FLASH_DEV_1);
    if (flash1) {
        DBG("Flash #1 size: %d MB\n", flash1->info.total_size / (1024*1024));
    }
}

/**
 * 示例6: 测试Flash读写
 */
static inline void example_test(void)
{
    DBG("Testing Flash #0...\n");
    FlashManager_Test(FLASH_DEV_0);
    
    DBG("Testing Flash #1...\n");
    FlashManager_Test(FLASH_DEV_1);
}

/*===========================================================================
 * 分区布局说明
 *===========================================================================
 * 
 * Flash #0 (W25Q64 - 8MB) @ CS = GPIOA21
 * ┌─────────────────────────────────────────┐
 * │         系统设置分区 (1MB)               │  0x000000 - 0x0FFFFF
 * │  - 魔术字 (4B)                          │  0x000000
 * │  - 版本号 (4B)                          │  0x000004
 * │  - 设置数据                             │  0x000100 - 0x07FFFF
 * │  - 备份区                               │  0x080000 - 0x0FFFFF
 * ├─────────────────────────────────────────┤
 * │         Looper分区 (7MB)                │  0x100000 - 0x7FFFFF
 * │  - 音频录制数据                         │
 * │  - 循环播放数据                         │
 * └─────────────────────────────────────────┘
 * 
 * Flash #1 (W25Q64 - 8MB) @ CS = GPIOA23 (需根据实际硬件修改)
 * ┌─────────────────────────────────────────┐
 * │         存储分区 (8MB)                   │  0x000000 - 0x7FFFFF
 * │  - 通用数据存储                         │
 * │  - 预设/音色数据                        │
 * │  - 其他用户数据                         │
 * └─────────────────────────────────────────┘
 * 
 *===========================================================================*/

#endif /* __FLASH_MANAGER_EXAMPLE_H__ */
