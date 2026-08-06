/**
 * @file    main.c
 * @brief   Minimal FreeRTOS APP for BanAirBundy bootloader (BP1532/BP1540)
 *
 * Boot path mirrors BanBox HAS_BOOTLOADER:
 *   - BL already did Chip_Init / PLL / GPIO / UART hardware
 *   - APP only rebuilds driver software state after __c_init wiped .bss
 *   - Do NOT re-lock PLL (can hang) or break running UART
 */
#include <stdlib.h>
#include <string.h>
#include "nds32_intrinsic.h"
#include "debug.h"
#include "clk.h"
#include "watchdog.h"
#include "powercontroller.h"
#include "gpio.h"
#include "remap.h"
#include "sys.h"
#include "irqn.h"
#include "dma.h"
#include "pmu.h"
#include "heap.h"
#include "uarts_interface.h"
#include "sram_config.h"
#include "clock_config.h"
#include "app_config.h"
#include "spi_flash.h"
#include "fw_upgrade.h"
#include "dual_partition.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* DMA channel map for SPI flash (6 channels on this SoC) */
static uint8_t DmaChannelMap[] =
{
    255, 255, 255, 255, 255, 255,
};

/* pdMS_TO_TICKS may not exist in older FreeRTOS */
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(((TickType_t)(xTimeInMs) * (TickType_t)configTICK_RATE_HZ) / (TickType_t)1000))
#endif

/* Direct UART1 TX — works before printf retarget is ready (BanBox diag) */
#define DIAG_UART1_STATUS  (*(volatile uint32_t *)0x40006014UL)
#define DIAG_UART1_TX      (*(volatile uint32_t *)0x40006018UL)

static inline void diag_putc(char c)
{
    while (!(DIAG_UART1_STATUS & (1u << 9)))
        ;
    DIAG_UART1_TX = (uint32_t)(unsigned char)c;
}

/* Task handles */
static TaskHandle_t xPrintTaskHandle = NULL;

/*-----------------------------------------------------------*/
static void vPrintTask(void *pvParameters)
{
    (void)pvParameters;
    int count = 0;

    for (;;)
    {
        DBG("Minimal RTOS running... count=%d\n", count++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*-----------------------------------------------------------*/
int main(void)
{
#if HAS_BOOTLOADER
    /*
     * BL already configured: Chip_Init, PLL (240MHz), UART1 PA10/PA9,
     * SPI flash XIP, TCM. Re-locking PLL or Chip_Init can hang.
     * __c_init() cleared .bss — rebuild UART/SPI/DMA software state only.
     */
    diag_putc('M');
    WDG_Disable();
    diag_putc('1');

    DbgUartInit(1, 115200, 8, 0, 1);
    diag_putc('U');

    Remap_InitTcm(0, 0, 12);
    SpiFlashInit(80000000, MODE_4BIT, 0, 1);
    DMA_ChannelAllocTableSet(DmaChannelMap);
    diag_putc('R');

    DBG("\n\n=== Minimal FreeRTOS APP @ 0x%08X (from bootloader) ===\n",
        (unsigned)PART_A_BASE);
    diag_putc('3');
#else
    Remap_InitTcm(0, 0, 12);

    Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
    Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
    Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);

    Clock_Config(TRUE, SYS_CORE_DPLL_FREQ);
    Clock_SysClkSelect(PLL_CLK_MODE);
    Clock_UARTClkSelect(PLL_CLK_MODE);

    WDG_Disable();

    GPIO_PortAModeSet(GPIOA10, 5); /* UART1 TX — BP15 board */
    GPIO_PortAModeSet(GPIOA9, 1);  /* UART1 RX */
    DbgUartInit(1, 115200, 8, 0, 1);

    SpiFlashInit(80000000, MODE_4BIT, 0, 1);
    DMA_ChannelAllocTableSet(DmaChannelMap);

    DBG("\n\n=== Minimal FreeRTOS APP @ 0x%08X (standalone) ===\n",
        (unsigned)PART_A_BASE);
#endif

    DBG("DPLL: %d kHz, APLL: %d kHz\n", SYS_CORE_DPLL_FREQ, SYS_CORE_APLL_FREQ);
    DBG("FreeRTOS heap: %d bytes\n", (int)configTOTAL_HEAP_SIZE);

    /* Dual-partition boot hooks (BL already jumped here) */
    FwUpgrade_BootInit();
    FwUpgrade_ConfirmBootSuccess();

    /* 创建打印任务 */
    xTaskCreate(vPrintTask,
                "Print",
                configMINIMAL_STACK_SIZE * 2,
                NULL,
                tskIDLE_PRIORITY + 1,
                &xPrintTaskHandle);

    /* 启动调度器 */
    DBG("Starting FreeRTOS scheduler...\n");
    vTaskStartScheduler();

    /* 不应到达 */
    for (;;)
    {
        DBG("Error: scheduler failed!\n");
    }

    return 0;
}

/* FreeRTOS 钩子函数 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    DBG("STACK OVERFLOW in %s\n", pcTaskName);
    for (;;);
}

void vApplicationMallocFailedHook(void)
{
    DBG("MALLOC FAILED\n");
    for (;;);
}

void vApplicationIdleHook(void)
{
    /* 空闲钩子，可用于低功耗 */
}
