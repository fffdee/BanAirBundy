/**
 * @file    main.c
 * @brief   boot_app — FreeRTOS APP with USB CDC + Audio composite
 *
 * USB mode: AUDIO_CDC (Speaker + CDC ACM), see usb_audio_api.h.
 * Boot path mirrors BanBox HAS_BOOTLOADER (skip Chip_Init/PLL when from BL).
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

#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "otg_device_cdc.h"
#include "usb_audio_api.h"
#include "delay.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*
 * heap_5s requires vPortDefineHeapRegions() before any pvPortMalloc /
 * xTaskCreate. Do NOT use _end..BP15_HEAP_END (~197KB): USB enum + TCM/BB
 * reserved RAM corrupt the freelist. Use a fixed mid-SRAM window above BSS
 * and below TCM remap (0x20037000).
 */
#define BOOT_APP_HEAP_START  0x20010000UL
#define BOOT_APP_HEAP_SIZE   0x20000UL   /* 128KB -> ends 0x20030000 */

static void prvInitialiseHeap(void)
{
	static HeapRegion_t xHeapRegions[2];

	xHeapRegions[0].pucStartAddress = (uint8_t *)BOOT_APP_HEAP_START;
	xHeapRegions[0].xSizeInBytes = (size_t)BOOT_APP_HEAP_SIZE;
	xHeapRegions[1].pucStartAddress = NULL;
	xHeapRegions[1].xSizeInBytes = 0;
	vPortDefineHeapRegions(xHeapRegions);
}

extern void OTG_DeviceFifoInit(void);
extern void SysTickInit(void);
extern volatile uint32_t gSysTick;

/* APP USB identity (distinct from bootloader 0x8888/0x1722) */
#define APP_USB_VID   0x8888
#define APP_USB_PID   USBPID(CFG_PARA_USB_MODE)

static uint8_t DmaChannelMap[] =
{
	255, 255, 255, 255, 255, 255,
};

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) \
	((TickType_t)(((TickType_t)(xTimeInMs) * (TickType_t)configTICK_RATE_HZ) / (TickType_t)1000))
#endif

static TaskHandle_t xPrintTaskHandle = NULL;
static TaskHandle_t xUsbTaskHandle = NULL;

/*
 * Host update_tool sends ASCII "boot\r\n" from "Enter Boot mode".
 * Consume that command from the APP CDC stream and reboot via burn-flag.
 */
static void App_CdcBootCommandProcess(void)
{
	static const char boot_cmd[] = "boot";
	static uint8_t match;
	uint8_t byte;

	while (OTG_DeviceCDC_Receive(&byte, 1) == 1u) {
		if (byte == '\r' || byte == '\n') {
			if (match == (sizeof(boot_cmd) - 1u)) {
				static const uint8_t reply[] = "[BOOT] entering bootloader\r\n";

				OTG_DeviceCDC_Send((uint8_t *)reply,
						  (uint16_t)(sizeof(reply) - 1u));
				FwUpgrade_RebootToBootloader();
			}
			match = 0;
			continue;
		}

		if (byte == (uint8_t)boot_cmd[match]) {
			match++;
			if (match >= sizeof(boot_cmd))
				match = 0;
		} else {
			match = (byte == (uint8_t)boot_cmd[0]) ? 1u : 0u;
		}
	}
}

static void vPrintTask(void *pvParameters)
{
	(void)pvParameters;
	int count = 0;

	for (;;) {
		DBG("boot_app running... count=%d mode=%u cfg=%u cdc=%u dtr=%u spk=%u\n",
		    count++, (unsigned)CFG_PARA_USB_MODE,
		    (unsigned)g_usb_configured, (unsigned)UsbCDC.InitOk,
		    (unsigned)UsbCDC.ControlLineState.DTR,
		    (unsigned)UsbAudioSpeaker.InitOk);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

/**
 * USB task: EP0 / CDC / audio stream maintenance.
 * ISO RX/TX runs in UsbInterrupt callbacks.
 */
static void vUsbTask(void *pvParameters)
{
	(void)pvParameters;

	for (;;) {
		OTG_DeviceRequestProcess();

		if (g_usb_configured && !UsbCDC.InitOk) {
			OTG_DeviceCDC_Init();
			DBG("[USB] CDC InitOk\n");
		}

		if (UsbCDC.InitOk) {
			OTG_DeviceCDC_Task();
			App_CdcBootCommandProcess();
		}

		if (CFG_PARA_USB_MODE != CDC_ONLY) {
			UsbAudioSpeakerStreamProcess();
			UsbAudioMicStreamProcess();
			UsbAudioTimer1msProcess();
		}

		/* EP0 SETUP is polled: stay responsive until configured. */
		if (g_usb_configured)
			vTaskDelay(pdMS_TO_TICKS(1));
		else
			taskYIELD();
	}
}

static void App_UsbInit(void)
{
	uint32_t t_deadline;

	/*
	 * USB clock: DPLL/5 = 48MHz (BL-proven).
	 * Host first SETUP ~200ms after connect; do NOT block EP0 with UART
	 * dumps during enumeration.
	 */
	Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
	Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
	Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);
	Clock_USBClkDivSet(5);
	Clock_USBClkSelect(PLL_CLK_MODE);

	if (CFG_PARA_USB_MODE != CDC_ONLY) {
		UsbDevicePlayInit();
		UsbDevicePlayResMalloc();
	}

	SysTickInit();

	DBG("[USB] Mode=%u VID=0x%04X PID=0x%04X\n",
	    (unsigned)CFG_PARA_USB_MODE, APP_USB_VID, APP_USB_PID);

	DelayMs(20);
	Clock_USBClkDivSet(5);
	Clock_USBClkSelect(PLL_CLK_MODE);

	OTG_DeviceModeSel(CFG_PARA_USB_MODE, APP_USB_VID, APP_USB_PID);
	OTG_DeviceFifoInit();
	OTG_DeviceInit();
	NVIC_SetPriority(Usb_IRQn, 0);
	NVIC_EnableIRQ(Usb_IRQn);

	/* Poll up to 500ms for SET_CONFIG — no DBG inside (EP0 timing-critical). */
	t_deadline = gSysTick + 500u;
	while ((int32_t)(gSysTick - t_deadline) < 0) {
		OTG_DeviceRequestProcess();
		if (g_usb_configured && !UsbCDC.InitOk)
			OTG_DeviceCDC_Init();
		if (UsbCDC.InitOk)
			OTG_DeviceCDC_Task();
		if (g_usb_configured)
			break;
		DelayMs(1);
	}

	/* Drain a few more EP0 packets before any UART (avoid ep0csr stuck). */
	{
		int d;
		for (d = 0; d < 20; d++) {
			OTG_DeviceRequestProcess();
			if (UsbCDC.InitOk)
				OTG_DeviceCDC_Task();
		}
	}
}

int main(void)
{
#if HAS_BOOTLOADER
	WDG_Disable();
	/* Keep IRQs masked until FreeRTOS owns SysTick (BL left tick running). */
	__nds32__setgie_dis();

	DbgUartInit(1, 115200, 8, 0, 1);

	Remap_InitTcm(0, 0, 12);
	SpiFlashInit(80000000, MODE_4BIT, 0, 1);
	DMA_ChannelAllocTableSet(DmaChannelMap);

	DBG("\n\n=== boot_app @ 0x%08X (from bootloader) ===\n",
	    (unsigned)PART_A_BASE);
#else
	Remap_InitTcm(0, 0, 12);

	Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
	Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
	Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);

	Clock_Config(TRUE, SYS_CORE_DPLL_FREQ);
	Clock_SysClkSelect(PLL_CLK_MODE);
	Clock_UARTClkSelect(PLL_CLK_MODE);

	WDG_Disable();

	GPIO_PortAModeSet(GPIOA10, 5);
	GPIO_PortAModeSet(GPIOA9, 1);
	DbgUartInit(1, 115200, 8, 0, 1);

	SpiFlashInit(80000000, MODE_4BIT, 0, 1);
	DMA_ChannelAllocTableSet(DmaChannelMap);

	DBG("\n\n=== boot_app @ 0x%08X (standalone) ===\n",
	    (unsigned)PART_A_BASE);
#endif

	DBG("DPLL: %d kHz, APLL: %d kHz\n", SYS_CORE_DPLL_FREQ, SYS_CORE_APLL_FREQ);
	DBG("FreeRTOS heap: %d bytes\n", (int)configTOTAL_HEAP_SIZE);

	FwUpgrade_BootInit();
	FwUpgrade_ConfirmBootSuccess();

	/* Must precede xTaskCreate — heap_5s assert/hang otherwise. */
	prvInitialiseHeap();

	/* USB enum first, then tasks — heap now valid. */
	App_UsbInit();

	GIE_ENABLE();

	xTaskCreate(vUsbTask,
		    "USB",
		    configMINIMAL_STACK_SIZE * 4,
		    NULL,
		    tskIDLE_PRIORITY + 3,
		    &xUsbTaskHandle);

	xTaskCreate(vPrintTask,
		    "Print",
		    configMINIMAL_STACK_SIZE * 4,
		    NULL,
		    tskIDLE_PRIORITY + 1,
		    &xPrintTaskHandle);

	/* FreeRTOS yield/tick switch via OS_Trap_Interrupt_SWI. */
	NVIC_EnableIRQ(SWI_IRQn);
	DBG("Starting FreeRTOS scheduler...\n");
	vTaskStartScheduler();

	for (;;) {
		DBG("Error: scheduler failed!\n");
	}

	return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	DBG("STACK OVERFLOW in %s\n", pcTaskName);
	for (;;)
		;
}

void vApplicationMallocFailedHook(void)
{
	DBG("MALLOC FAILED\n");
	for (;;)
		;
}

void vApplicationIdleHook(void)
{
}
