/**
 *****************************************************************************
 * @file     flash_test.h
 * @brief    Flash 驱动框架测试头文件 (NOR + NAND)
 *****************************************************************************
 */

#ifndef __FLASH_TEST_H__
#define __FLASH_TEST_H__

#include <stdint.h>
#include <stdbool.h>
#include "banux_config.h"

/*===========================================================================
 * NOR Flash 测试
 *===========================================================================*/
#if FLASH_TEST_EN
#define NOR_FLASH_TEST
#endif

#ifdef NOR_FLASH_TEST
void FlashNewDriver_Test(void);
void FlashNewDriver_QuickTest(void);

typedef struct {
    bool     test_passed;
    float    pure_write_speed_kbs;
    float    write_with_erase_speed_kbs;
    float    seq_read_speed_kbs;
    uint32_t pure_write_time_ms;
    uint32_t write_with_erase_time_ms;
    uint32_t read_time_ms;
    uint32_t test_size_bytes;
} NorTestResult_t;

void NorFlash_SpeedTest(uint32_t test_size_kb, NorTestResult_t *result);
#endif /* NOR_FLASH_TEST */

/*===========================================================================
 * NAND Flash 测试
 *===========================================================================*/
#if FLASH_TEST_EN
#define NAND_FLASH_TEST
#endif

#ifdef NAND_FLASH_TEST

typedef struct {
    bool     test_passed;
    float    seq_write_speed_kbs;
    float    seq_read_speed_kbs;
    uint32_t write_time_ms;
    uint32_t read_time_ms;
    uint16_t bad_block_count;
    uint32_t test_size_bytes;
} NandTestResult_t;

void NandFlash_Test(void);
void NandFlash_SpeedTest(uint8_t test_blocks, NandTestResult_t *result);
void NandFlash_BBMTest(void);

#endif /* NAND_FLASH_TEST */

/*===========================================================================
 * PSRAM 测试
 *===========================================================================*/
#if FLASH_TEST_EN
#define PSRAM_TEST
#endif

#ifdef PSRAM_TEST

typedef struct {
    bool     test_passed;
    float    seq_write_speed_kbs;
    float    seq_read_speed_kbs;
    uint32_t write_time_ms;
    uint32_t read_time_ms;
    uint32_t test_size_bytes;
} PsramTestResult_t;

void PsramFlash_Test(void);
void PsramFlash_SpeedTest(uint32_t test_size_kb, PsramTestResult_t *result);

#endif /* PSRAM_TEST */

/*===========================================================================
 * SD Card 测试
 *===========================================================================*/
#if FLASH_TEST_EN
#define SDCARD_TEST
#endif

#ifdef SDCARD_TEST

typedef struct {
    bool     test_passed;
    float    seq_write_speed_kbs;
    float    seq_read_speed_kbs;
    uint32_t write_time_ms;
    uint32_t read_time_ms;
    uint32_t test_size_bytes;
    uint32_t card_capacity_mb;
} SDCardTestResult_t;

void SDCardFlash_Test(void);
void SDCardFlash_SpeedTest(uint32_t test_blocks, SDCardTestResult_t *result);

#endif /* SDCARD_TEST */

#endif /* __FLASH_TEST_H__ */
