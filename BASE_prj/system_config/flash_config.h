#ifndef __FLASH_CONFIG_H__
#define __FLASH_CONFIG_H__

#include "chip_config.h"

/*
 * BanAirBundy APP layout (bootloader at 0x0, size 256KB):
 *   Bootloader : 0x00000000 – 0x0003FFFF
 *   Part A APP : 0x00040000 – …
 * Vector table / IVB / link address all use APP_CODE_ADDR (= PART_A_BASE).
 */
#if (CHIP_FLASH_CAPACTITY == 16)
    #define FLASH_CAPACTITY_TYPE    0
    #define FLASH_ADDR              0x00040000
    #define BOOT_ADDR               0x00000000
    #define FLASH_BOOT_ADDR         0x00000000
    #define FLASH_BOOT_EN           0
    #define FLASH_BOOT_LEN          0x00000000
    #define APP_CODE_ADDR           0x00040000

    #define WIRELEDD_CONFIG_ADDR    0x001C0000
    #define SOC_CAP_CAL_ADDR        0x001C2000
    #define SOC_CAP_CAL_ADDR_1      0x001C2100
    #define USER_DATA_ADDR          0x001C3000
    #define CONST_DATA_ADDR         0x001C1000
    #define AUDIO_EFFECT_ADDR       0x001C4000
#endif

#endif
