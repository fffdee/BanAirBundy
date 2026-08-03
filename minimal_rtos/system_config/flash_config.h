#ifndef __FLASH_CONFIG_H__
#define __FLASH_CONFIG_H__

#include "chip_config.h"

#if (CHIP_FLASH_CAPACTITY == 16)
    #define FLASH_CAPACTITY_TYPE    0
    #define FLASH_ADDR              0x00000000
    #define BOOT_ADDR               0x00000000
    #define FLASH_BOOT_ADDR         0x00000000
    #define WIRELEDD_CONFIG_ADDR    0x0005C000
    #define SOC_CAP_CAL_ADDR        0x0005E000
    #define SOC_CAP_CAL_ADDR_1      0x0005E100
    #define USER_DATA_ADDR          0x0005F000
    #define CONST_DATA_ADDR         0x00058000
    #define AUDIO_EFFECT_ADDR       0x00040000
    #define FLASH_BOOT_EN           1
    #define FLASH_BOOT_LEN          0x10000
    #define APP_CODE_ADDR           (FLASH_BOOT_ADDR + FLASH_BOOT_LEN)
#endif

#endif
