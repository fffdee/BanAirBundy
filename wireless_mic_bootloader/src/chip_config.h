/**
 * @file  chip_config.h
 * @brief Bootloader minimal chip_config — provides only chip type and
 *        flash capacity macros. No audio/analog dependencies.
 */

#ifndef _CHIP_CONFIG_H_
#define _CHIP_CONFIG_H_

/* Chip package types */
#define CFG_CHIP_BP1532B2       7

/* Chip selection — BP1532B2, 16MB flash */
#define CFG_CHIP_SEL            CFG_CHIP_BP1532B2

/* Flash capacity (MB) */
#define CHIP_FLASH_CAPACTITY    16
#define CHIP_NAME               "BP1532B2"

/* DCDC enabled */
#define CHIP_USE_DCDC

/* RAM */
#define CFG_D16K_MEM16K_EN      0
#define CFG_D16KMEM16K_RAM_SIZE (CFG_D16K_MEM16K_EN*16*1024)
#define CFG_CHIP_RAM_SIZE       (256*1024 + CFG_D16KMEM16K_RAM_SIZE)

#endif /* _CHIP_CONFIG_H_ */
