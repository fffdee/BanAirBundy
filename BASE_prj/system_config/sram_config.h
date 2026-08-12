#ifndef __SRAM_CONFIG_H__
#define __SRAM_CONFIG_H__

#include "app_config.h"
#include "remap.h"
#include "bb_api.h"

#define WIRELESS_TCM_SIZE   		20
#define TCM_SRAM_START_ADDR_1		(BB_MPU_START_ADDR - WIRELESS_TCM_SIZE * 1024)

#ifdef TCM_EN
#define BP15_HEAP_END				(TCM_SRAM_START_ADDR_1)
#else
#define BP15_HEAP_END				(BB_MPU_START_ADDR)
#endif

#define DUT_MODE_CMD	0x2

#define CFG_D16K_MEM16K_EN		0
#define CFG_D16KMEM16K_RAM_SIZE	0
#define CFG_CHIP_RAM_SIZE		(256*1024)

#endif
