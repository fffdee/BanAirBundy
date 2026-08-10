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

#define DIAG_UART1_STATUS  (*(volatile uint32_t *)0x40006014UL)
#define DIAG_UART1_TX      (*(volatile uint32_t *)0x40006018UL)

static inline void diag_putc(char c)
{
	while (!(DIAG_UART1_STATUS & (1u << 9)))
		;
	DIAG_UART1_TX = (uint32_t)(unsigned char)c;
}

static TaskHandle_t xPrintTaskHandle = NULL;
static TaskHandle_t xUsbTaskHandle = NULL;

static void vPrintTask(void *pvParameters)
{
	(void)pvParameters;
	int count = 0;

	for (;;) {
		DBG("boot_app running... count=%d usb_cfg=%u cdc=%u\n",
		    count++, (unsigned)g_usb_configured, (unsigned)UsbCDC.InitOk);
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
	int i;

	/*
	 * USB clock: DPLL/5 = 48MHz (same proven path as bootloader).
	 * EP0 ControlSend timeouts use GetSysTick1MsCnt() — tick MUST run
	 * before D+ pull-up, or Windows shows "Unknown device".
	 * SystickInterrupt is guarded: only gSysTick++ until scheduler runs.
	 */
	Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
	Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
	Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);
	Clock_USBClkDivSet(5);
	Clock_USBClkSelect(PLL_CLK_MODE);

	UsbDevicePlayInit();
	UsbDevicePlayResMalloc();

	/* Starts Timer0 1ms + GIE + Systick IRQ (safe with scheduler guard). */
	SysTickInit();

	DBG("[USB] Mode=%u VID=0x%04X PID=0x%04X\n",
	    (unsigned)CFG_PARA_USB_MODE, APP_USB_VID, APP_USB_PID);

	/* Prepare stack, then attach — host may SETUP immediately after DP up. */
	OTG_DeviceModeSel(CFG_PARA_USB_MODE, APP_USB_VID, APP_USB_PID);
	OTG_DeviceFifoInit();
	OTG_DeviceInit();
	NVIC_SetPriority(Usb_IRQn, 0);
	NVIC_EnableIRQ(Usb_IRQn);

	/* Drain early control transfers before FreeRTOS takes the CPU. */
	for (i = 0; i < 100; i++) {
		OTG_DeviceRequestProcess();
		if (g_usb_configured && !UsbCDC.InitOk)
			OTG_DeviceCDC_Init();
		if (UsbCDC.InitOk)
			OTG_DeviceCDC_Task();
		DelayMs(1);
	}
	DBG("[USB] attach done tick=%u cfg=%u cdc=%u\n",
	    (unsigned)gSysTick, (unsigned)g_usb_configured, (unsigned)UsbCDC.InitOk);
	OTG_DeviceDebugDump();
}

int main(void)
{
#if HAS_BOOTLOADER
	diag_putc('M');
	WDG_Disable();
	/* Keep IRQs masked until FreeRTOS owns SysTick (BL left tick running). */
	__nds32__setgie_dis();
	diag_putc('1');

	DbgUartInit(1, 115200, 8, 0, 1);
	diag_putc('U');

	Remap_InitTcm(0, 0, 12);
	SpiFlashInit(80000000, MODE_4BIT, 0, 1);
	DMA_ChannelAllocTableSet(DmaChannelMap);
	diag_putc('R');

	DBG("\n\n=== boot_app @ 0x%08X (from bootloader) ===\n",
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

	App_UsbInit();

	xTaskCreate(vUsbTask,
		    "USB",
		    configMINIMAL_STACK_SIZE * 4,
		    NULL,
		    tskIDLE_PRIORITY + 3,
		    &xUsbTaskHandle);

	xTaskCreate(vPrintTask,
		    "Print",
		    configMINIMAL_STACK_SIZE * 2,
		    NULL,
		    tskIDLE_PRIORITY + 1,
		    &xPrintTaskHandle);

	// #region agent log
	DBG("{\"sessionId\":\"5032d7\",\"runId\":\"pre-fix\",\"hypothesisId\":\"H11\","
	    "\"location\":\"main.c:main\",\"message\":\"pre_scheduler\","
	    "\"data\":{\"tick\":%u},\"timestamp\":%u}\n",
	    (unsigned)gSysTick, (unsigned)gSysTick);
	// #endregion agent log
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
