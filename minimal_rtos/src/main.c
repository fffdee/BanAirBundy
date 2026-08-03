/**
 * @file    main.c
 * @brief   Minimal FreeRTOS + UART printf demo
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
#include "flash_boot.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* pdMS_TO_TICKS may not exist in older FreeRTOS */
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(((TickType_t)(xTimeInMs) * (TickType_t)configTICK_RATE_HZ) / (TickType_t)1000))
#endif

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
    /* TCM 初始化 */
    Remap_InitTcm(0, 0, WIRELESS_TCM_SIZE);

    /* 时钟配置: DPLL 288MHz, APLL 240MHz */
    Clock_Config(TRUE, SYS_CORE_DPLL_FREQ);

    /* UART 时钟选择 */
    Clock_UARTClkSelect(PLL_CLK_MODE);

    /* 模块时钟使能: UART0 */
    Clock_Module1Enable(UART0_CLK_EN);

    /* 关看门狗 */
    WDG_Disable();

    /* UART 初始化: UART0, 115200, 8N1 */
    UARTS_Init(UART_PORT1, 115200, 8, 0, 1);

    DBG("\n\n=== Minimal FreeRTOS Demo (BP1540A2) ===\n");
    DBG("DPLL: %d kHz, APLL: %d kHz\n", SYS_CORE_DPLL_FREQ, SYS_CORE_APLL_FREQ);
    DBG("FreeRTOS heap: %d bytes\n", (int)configTOTAL_HEAP_SIZE);

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
