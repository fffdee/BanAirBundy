/**
 * @file    main.c
 * @brief   boot_app — FreeRTOS APP with BanUX + USB CDC/Audio
 *
 * BanUX: event bus, VFS/driver framework, CDC Shell (help/boot).
 * USB: AUDIO_CDC or CDC_ONLY via usb_audio_api.h / app_config.h.
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
#include "Banux.h"
#include "fw_upgrade.h"
#include "dual_partition.h"

#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "otg_device_cdc.h"
#include "usb_audio_api.h"
#include "delay.h"
#include "shell_io_cdc.h"
#include "wireless_app.h"

extern int BanuxDriver_RegisterAll(void);

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*
 * heap_5s: place FreeRTOS heap AFTER linker BSS (_end), below TCM/BB EM.
 * Fixed 0x20010000 overlapped wireless BSS (rwip_heap_* … _end≈0x20019ea4)
 * after linking libwireless*, which corrupted the freelist → crash in pvPortMalloc.
 */
extern char _end;
#define BOOT_APP_HEAP_START  ((((uint32_t)&_end) + 31u) & ~31u)
#define BOOT_APP_HEAP_END    ((uint32_t)BP15_HEAP_END)
#define BOOT_APP_HEAP_SIZE   (BOOT_APP_HEAP_END - BOOT_APP_HEAP_START)

static void prvInitialiseHeap(void)
{
	static HeapRegion_t xHeapRegions[2];

	configASSERT(BOOT_APP_HEAP_END > BOOT_APP_HEAP_START);
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
	PERIPHERAL_ID_AUDIO_ADC1_RX,
	PERIPHERAL_ID_AUDIO_DAC0_TX,
	255, 255, 255, 255,
};

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) \
	((TickType_t)(((TickType_t)(xTimeInMs) * (TickType_t)configTICK_RATE_HZ) / (TickType_t)1000))
#endif

static TaskHandle_t xUsbTaskHandle = NULL;

static void App_BanuxLog(const char *text)
{
	if (text)
		DBG("%s", text);
}

/**
 * BanUX framework bring-up for boot_app:
 * event bus + VFS/driver core + CDC Shell + firmware upgrade component.
 * Keep HW_DRV_USB_CDC_EN=0 so composite USB stays on otg_device_cdc.
 */
static void App_BanuxInit(void)
{
	int ret;
	const BanuxConfig_t config = {
		App_BanuxLog,
		ShellIO_CDC_Get(),
		NULL,
		BanuxDriver_RegisterAll,
		NULL,
		NULL,
	};

	ret = Banux_Init(&config);
	if (ret != 0) {
		DBG("[BanUX] Banux_Init failed: %d\n", ret);
	} else {
		DBG("[BanUX] framework ready, Shell IO = %s\n", Shell_GetIOName());
	}
}

/**
 * USB task: EP0 / CDC / audio stream maintenance.
 * ISO RX/TX runs in UsbInterrupt callbacks.
 */
static void vUsbTask(void *pvParameters)
{
	static uint8_t s_lastDtr = 0u;

	(void)pvParameters;
	DBG("[USB] task running\n");

	for (;;) {
		OTG_DeviceRequestProcess();

		if (g_usb_configured && !UsbCDC.InitOk) {
			OTG_DeviceCDC_Init();
			DBG("[USB] CDC InitOk\n");
		}

		if (UsbCDC.InitOk) {
			/*
			 * Shell welcome/TX needs host DTR (COM open). Sending Bulk-IN
			 * before that makes libDriver print "SEND ERROR" and can disturb
			 * Windows CDC open.
			 */
			if (UsbCDC.ControlLineState.DTR && !s_lastDtr) {
				OTG_DeviceCDC_FlushRxBuffer();
				Shell_ResetInputLine();
			}
			s_lastDtr = UsbCDC.ControlLineState.DTR;

			/* CDC data is valid independently of DTR.  Some host tools send
			 * bulk data before asserting the modem-control line. */
			OTG_DeviceCDC_Task();
			Banux_Process();
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
			/* Do not spin at the highest task priority before enumeration. */
			vTaskDelay(pdMS_TO_TICKS(1));
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
	DBG("FreeRTOS heap: %lu bytes @ 0x%08lX..0x%08lX (_end=0x%08lX)\n",
	    (unsigned long)BOOT_APP_HEAP_SIZE,
	    (unsigned long)BOOT_APP_HEAP_START,
	    (unsigned long)BOOT_APP_HEAP_END,
	    (unsigned long)(uint32_t)&_end);

	FwUpgrade_BootInit();
	FwUpgrade_ConfirmBootSuccess();

	/* Must precede xTaskCreate — heap_5s assert/hang otherwise. */
	prvInitialiseHeap();

	/* USB enum first, then BanUX + tasks — heap now valid. */
	App_UsbInit();
	App_BanuxInit();

	GIE_ENABLE();

	if (App_WirelessStart() != 0)
		DBG("[WL] App_WirelessStart failed — continue without RF\n");

	{
		BaseType_t task_ret;

		task_ret = xTaskCreate(vUsbTask,
				       "USB",
				       configMINIMAL_STACK_SIZE * 8,
				       NULL,
				       tskIDLE_PRIORITY + 3,
				       &xUsbTaskHandle);
		DBG("[USB] task create=%s handle=%p free_heap=%lu\n",
		    task_ret == pdPASS ? "ok" : "failed",
		    (void *)xUsbTaskHandle,
		    (unsigned long)xPortGetFreeHeapSize());
	}

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
	const unsigned char *p = (const unsigned char *)pcTaskName;
	DBG("STACK OVERFLOW task=%p name='%s' hex=%02X %02X %02X %02X\n",
	    (void *)xTask,
	    (pcTaskName && pcTaskName[0] >= 0x20 && pcTaskName[0] < 0x7F)
		    ? pcTaskName
		    : "?",
	    p ? p[0] : 0,
	    p ? p[1] : 0,
	    p ? p[2] : 0,
	    p ? p[3] : 0);
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
