/**
 * @file  flash_config.h
 * @brief Bootloader minimal flash_config — provides only the address
 *        macros required by chip_config.h and USB driver.
 *
 * This header OVERRIDES the full flash_config.h from unified_sdk.
 * The bootloader uses its own dual-partition layout defined in upgrade.h.
 */

#ifndef __FLASH_CONFIG_H__
#define __FLASH_CONFIG_H__

/* Bootloader flash layout (2 MB internal flash) */
#define CODE_ADDR               0x000000
#define CONST_DATA_ADDR         0x130000
#define AUDIO_EFFECT_ADDR       0x1C8000
#define FLASHFS_ADDR            0x1D0000

/* User data areas */
#define USER_DATA_ADDR          0x1F0000
#define BP_DATA_ADDR            0x1F3000
#define BT_DATA_ADDR            0x1FB000

/* Config areas */
#define USER_CONFIG_ADDR        0x1FE000
#define BT_CONFIG_ADDR          0x1FF000

/* Wireless config addresses (referenced by app_config.h) */
#define WIRELEDD_CONFIG_ADDR    0x1FF000
#define SOC_CAP_CAL_ADDR        0x1FE000
#define SOC_CAP_CAL_ADDR_1      0x1FE800

/* SDK version */
#define CFG_SDK_VER_CHIPID      (0x01)
#define CFG_SDK_MAJOR_VERSION   (0x0)
#define CFG_SDK_MINOR_VERSION   (0x0)
#define CFG_SDK_PATCH_VERSION   (0x3)

#endif /* __FLASH_CONFIG_H__ */
