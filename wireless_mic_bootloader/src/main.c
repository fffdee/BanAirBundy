/**
 **************************************************************************************
 * @file    main.c
 * @brief   Bootloader – USB CDC firmware upgrade, FreeRTOS + CDC_ONLY
 *
 * FreeRTOS-based bootloader:
 *   1. Initialise hardware (chip, clock, UART, SPI flash, DMA, Timer2)
 *   2. Check partition flags; jump to application if valid firmware found
 *   3. If no valid firmware, start FreeRTOS and run USB CDC upgrade task
 *
 * USB mode: CDC_ONLY (no audio composite device)
 * USB driver: from wireless_mic_rx_sdk
 **************************************************************************************
 */

/* Version: increment on each release (format V<major>.<minor>.<patch>) */
#define BOOTLOADER_VERSION_MAJOR   0
#define BOOTLOADER_VERSION_MINOR   4
#define BOOTLOADER_VERSION_PATCH   0
#define BOOTLOADER_VERSION_STR     "V0.4.0"

#include <stdlib.h>
#include <stdbool.h>
#include <nds32_intrinsic.h>
#include "gpio.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "type.h"
#include "debug.h"
#include "clk.h"
#include "dma.h"
#include "timer.h"
#include "watchdog.h"
#include "spi_flash.h"
#include "remap.h"
#include "irqn.h"
#include "FreeRTOS.h"
#include "task.h"
#include "delay.h"

#include <string.h>
#include <stdio.h>

/* USB CDC (from rx_sdk) */
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "otg_device_cdc.h"
#include "otg_detect.h"

#include "upgrade.h"

/* ──────────────────────────────────────────────────────────────────────────
 * Timer2 ISR (1 ms tick)
 * Services USB port-link detection only (no audio in CDC_ONLY mode).
 * ────────────────────────────────────────────────────────────────────────── */
void Timer2Interrupt(void) {
	Timer_InterruptFlagClear(TIMER2, UPDATE_INTERRUPT_SRC);
	OTG_PortLinkCheck();
}

/* ──────────────────────────────────────────────────────────────────────────
 * DMA channel allocation table
 * UART1 RX/TX channels are assigned for debug output.
 * ────────────────────────────────────────────────────────────────────────── */
static uint8_t DmaChannelMap[29] = {
		255, //PERIPHERAL_ID_SPIS_RX = 0,
		255, //PERIPHERAL_ID_SPIS_TX,
		255, //PERIPHERAL_ID_TIMER3,
		255, //PERIPHERAL_ID_SDIO_RX,
		255, //PERIPHERAL_ID_SDIO_TX,
		255, //PERIPHERAL_ID_UART0_RX,
		255, //PERIPHERAL_ID_TIMER1,
		255, //PERIPHERAL_ID_TIMER2,
		255, //PERIPHERAL_ID_SDPIF_RX,
		255, //PERIPHERAL_ID_SDPIF_TX,
		255, //PERIPHERAL_ID_SPIM_RX,
		255, //PERIPHERAL_ID_SPIM_TX,
		255, //PERIPHERAL_ID_UART0_TX,
		7,   //PERIPHERAL_ID_UART1_RX,
		6,   //PERIPHERAL_ID_UART1_TX,
		255, //PERIPHERAL_ID_TIMER4,
		255, //PERIPHERAL_ID_TIMER5,
		255, //PERIPHERAL_ID_TIMER6,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255
		};

/* ──────────────────────────────────────────────────────────────────────────
 * USB CDC upgrade task
 * ────────────────────────────────────────────────────────────────────────── */
static void UpgradeTask(void)
{
	/* CDC_ONLY: pure CDC device, no audio composite.
	 * Bootloader uses dedicated VID=0x8888/PID=0x1722, host identifies
	 * device in upgrade mode by this VID/PID. APP uses different ones. */
	OTG_DeviceModeSel(CDC_ONLY, 0x8888, 0x1722);
	OTG_DeviceInit();
	NVIC_EnableIRQ(Usb_IRQn);
	NVIC_SetPriority(Usb_IRQn, 0);
	Upgrade_Init();

	DBG("[BOOT] USB CDC upgrade ready (VID=0x%04X PID=0x%04X)\n", 0x8888, 0x1722);

	while (1) {
		/* Service USB enumeration and control requests */
		OTG_DeviceRequestProcess();

		/* Drive CDC RX/TX ring-buffers */
		OTG_DeviceCDC_Task();

		/* Firmware upgrade state machine */
		Upgrade_Process();
	}
}

/* ──────────────────────────────────────────────────────────────────────────
 * Entry point
 * ────────────────────────────────────────────────────────────────────────── */
void prvInitialiseHeap(void);

int main(void) {
	Chip_Init(1);
	WDG_Disable();

	Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
	Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
	Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);

	Clock_Config(1, 24000000);
	Clock_PllLock(288000);
	Clock_APllLock(240000);

	Clock_SysClkSelect(PLL_CLK_MODE);
	Clock_UARTClkSelect(APLL_CLK_MODE);

	Clock_USBClkDivSet(4);
	Clock_USBClkSelect(APLL_CLK_MODE);

	GPIO_PortAModeSet(GPIOA9, 1);    /* UART1 RX  */
	GPIO_PortAModeSet(GPIOA10, 3);   /* UART1 TX  */
	DbgUartInit(1, 115200, 8, 0, 1);

	/* CRITICAL: Disable any stale partition-B remap from a previous boot.
	 * Must happen BEFORE Boot_CheckAndJumpIfNeeded(). */
	Remap_AddrRemapDisable(ADDR_REMAP0);
	Remap_AddrRemapDisable(ADDR_REMAP1);
	Remap_AddrRemapDisable(ADDR_REMAP2);

	Remap_InitTcm(0, 0, 12);
	SpiFlashInit(80000000, MODE_4BIT, 0, 1);
	DMA_ChannelAllocTableSet(DmaChannelMap);

	/* Detect flash capacity and compute safe partition flag address.
	 * Must be called BEFORE Boot_CheckAndJumpIfNeeded(). */
	PartFlag_Init();

	GIE_ENABLE();

	Timer_Config(TIMER2, 1000, 0);
	Timer_Start(TIMER2);
	NVIC_EnableIRQ(Timer2_IRQn);

	DBG("****************************************************************\n");
	DBG("                    Wireless MIC Bootloader " BOOTLOADER_VERSION_STR "\n");
	DBG("****************************************************************\n");

	/* Check partition flags; jump to application if not in upgrade mode.
	 * This call may never return (when a valid application is found). */
	Boot_CheckAndJumpIfNeeded();

	/* No valid firmware — stay resident for USB CDC upgrade */
	DBG("[BOOT] No valid firmware — entering USB CDC upgrade mode\n");

	prvInitialiseHeap();
	NVIC_EnableIRQ(SWI_IRQn);

	xTaskCreate((TaskFunction_t)UpgradeTask, "UpgTask", 2048, NULL, 1, NULL);

	DBG("[Main] Starting FreeRTOS scheduler...\n");
	vTaskStartScheduler();

	while (1)
		;
}

/* ──────────────────────────────────────────────────────────────────────────
 * FreeRTOS heap initialisation
 * heap_5s.c needs an explicit init call before any dynamic allocation.
 * Bootloader uses a single region inside SRAM.
 * ────────────────────────────────────────────────────────────────────────── */
void prvInitialiseHeap(void)
{
	/* heap_5s.c requires vPortDefineHeapRegions() before any pvPortMalloc(). */
	static HeapRegion_t xHeapRegions[] = {
		{ (uint8_t *)0x20020000UL, 0x20000UL }, /* 128 KB heap in SRAM */
		{ NULL, 0 }
	};
	vPortDefineHeapRegions(xHeapRegions);
}

/* ──────────────────────────────────────────────────────────────────────────
 * FreeRTOS hook stubs
 * ────────────────────────────────────────────────────────────────────────── */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	(void)xTask;
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
	/* Nothing to do in idle */
}
