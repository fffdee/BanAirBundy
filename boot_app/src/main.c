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
 * reserved RAM corrupt the freelist; crash at prvInsertBlockIntoFreeList
 * (PC=0x41eaa, ABError). Use a fixed mid-SRAM window above BSS (~0x20006Cxx)
 * and below TCM remap (0x20037000), same pattern as wireless_mic bootloader.
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
	// #region agent log
	extern volatile uint32_t g_d503_cdc_evt;
	extern volatile uint32_t g_d503_cdc_baud;
	extern volatile uint32_t g_d503_cdc_dtr_rts;
	extern volatile uint32_t g_d503_cdc_serr;
	extern volatile uint32_t g_d503_cdc_last_req;
	uint32_t last_cdc_evt = 0;
	diag_putc('P');
	// #endregion agent log
	for (;;) {
		DBG("boot_app running... count=%d usb_cfg=%u cdc=%u dtr=%u\n",
		    count++, (unsigned)g_usb_configured, (unsigned)UsbCDC.InitOk,
		    (unsigned)UsbCDC.ControlLineState.DTR);
		// #region agent log
		/*
		 * H51 REJECTED / H52 REJECTED (evt=7,serr=0). H54 CONFIRMED:
		 * OTG_DeviceDebugDump() from this task overflowed stack
		 * (MINIMAL*2) and hung in overflow hook → COM open aborted.
		 * Keep one short NDJSON line only — no DebugDump here.
		 */
		if (g_d503_cdc_evt != last_cdc_evt) {
			last_cdc_evt = g_d503_cdc_evt;
			DBG("{\"sessionId\":\"5032d7\",\"runId\":\"cdc-open\",\"hypothesisId\":\"H54\","
			    "\"location\":\"main.c:vPrintTask\",\"message\":\"cdc_open_evt\","
			    "\"data\":{\"evt\":%u,\"req\":%u,\"baud\":%u,\"dtrRts\":%u,"
			    "\"serr\":%u,\"dtr\":%u,\"rts\":%u,\"conn\":%u},"
			    "\"timestamp\":%u}\n",
			    (unsigned)g_d503_cdc_evt, (unsigned)g_d503_cdc_last_req,
			    (unsigned)g_d503_cdc_baud, (unsigned)g_d503_cdc_dtr_rts,
			    (unsigned)g_d503_cdc_serr,
			    (unsigned)UsbCDC.ControlLineState.DTR,
			    (unsigned)UsbCDC.ControlLineState.RTS,
			    (unsigned)UsbCDC.IsConnected,
			    (unsigned)gSysTick);
		}
		// #endregion agent log
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
	// #region agent log
	TickType_t t0 = xTaskGetTickCount();
	uint8_t dumped = 0;
	/* H45/H48: prove first task entered before any USB work. */
	diag_putc('T');
	DBG("{\"sessionId\":\"5032d7\",\"runId\":\"post-fix\",\"hypothesisId\":\"H45\","
	    "\"location\":\"main.c:vUsbTask\",\"message\":\"usb_task_entry\","
	    "\"data\":{\"cfg\":%u,\"tick\":%u},\"timestamp\":%u}\n",
	    (unsigned)g_usb_configured, (unsigned)gSysTick, (unsigned)gSysTick);
	// #endregion agent log

	for (;;) {
		OTG_DeviceRequestProcess();
		// #region agent log
		if (!dumped)
			diag_putc('1');
		// #endregion agent log

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

		// #region agent log
		if (!dumped &&
		    (g_usb_configured ||
		     (xTaskGetTickCount() - t0) >= pdMS_TO_TICKS(800))) {
			dumped = 1;
			diag_putc('2');
			DBG("{\"sessionId\":\"5032d7\",\"runId\":\"post-fix\",\"hypothesisId\":\"H45,H48\","
			    "\"location\":\"main.c:vUsbTask\",\"message\":\"usb_task_status\","
			    "\"data\":{\"mode\":%u,\"cfg\":%u,\"cdc\":%u,\"spk\":%u,\"mic\":%u},"
			    "\"timestamp\":%u}\n",
			    (unsigned)CFG_PARA_USB_MODE,
			    (unsigned)g_usb_configured, (unsigned)UsbCDC.InitOk,
			    (unsigned)UsbAudioSpeaker.InitOk,
			    (unsigned)UsbAudioMic.InitOk,
			    (unsigned)gSysTick);
			/* Skip OTG_DeviceDebugDump here — previously stalled EP0; re-enable after stable. */
		}
		// #endregion agent log

		/* EP0 SETUP is polled: stay responsive until configured. */
		if (g_usb_configured)
			vTaskDelay(pdMS_TO_TICKS(1));
		else
			taskYIELD();
	}
}

static void App_UsbInit(void)
{
	// #region agent log
	uint32_t div_m1, mux;
	uint32_t t_deadline;
	// #endregion agent log

	/*
	 * USB clock: DPLL/5 = 48MHz (BL-proven).
	 * Evidence: host first SETUP ~200ms after connect; do NOT block EP0
	 * with UART dumps during enumeration (prior run left ep0csr=1, cfg=0).
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
	// #region agent log
	div_m1 = *(volatile uint32_t *)0x40021004UL;
	mux = (uint32_t)(*(volatile uint16_t *)0x4002103CUL);
	DBG("{\"sessionId\":\"5032d7\",\"runId\":\"post-fix\",\"hypothesisId\":\"H36\","
	    "\"location\":\"main.c:App_UsbInit\",\"message\":\"usb_clk_pre_attach\","
	    "\"data\":{\"div\":%u,\"mhz\":%u,\"apl\":%u},"
	    "\"timestamp\":%u}\n",
	    (unsigned)(div_m1 + 1u), (unsigned)(240u / (div_m1 + 1u)),
	    (unsigned)((mux & 0x800u) ? 1u : 0u),
	    (unsigned)gSysTick);
	// #endregion agent log

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

	// #region agent log
	/* Short line only — full DebugDump here previously left ep0csr=1 and
	 * hung before xTaskCreate ('AB' then silence). Dump from vUsbTask. */
	DBG("{\"sessionId\":\"5032d7\",\"runId\":\"post-fix\",\"hypothesisId\":\"H41\","
	    "\"location\":\"main.c:App_UsbInit\",\"message\":\"attach_done\","
	    "\"data\":{\"mode\":%u,\"tick\":%u,\"cfg\":%u,\"cdc\":%u,\"spk\":%u,\"mic\":%u},"
	    "\"timestamp\":%u}\n",
	    (unsigned)CFG_PARA_USB_MODE, (unsigned)gSysTick,
	    (unsigned)g_usb_configured, (unsigned)UsbCDC.InitOk,
	    (unsigned)UsbAudioSpeaker.InitOk, (unsigned)UsbAudioMic.InitOk,
	    (unsigned)gSysTick);
	diag_putc('A');
	// #endregion agent log
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

	/* Must precede xTaskCreate — heap_5s assert/hang otherwise. */
	prvInitialiseHeap();
	// #region agent log
	{
		extern char _end;
		diag_putc('H');
		DBG("{\"sessionId\":\"5032d7\",\"runId\":\"post-fix\",\"hypothesisId\":\"H42\","
		    "\"location\":\"main.c:main\",\"message\":\"heap_init\","
		    "\"data\":{\"bssEnd\":%u,\"heapStart\":%u,\"heapBytes\":%u},"
		    "\"timestamp\":0}\n",
		    (unsigned)(uint32_t)&_end, (unsigned)BOOT_APP_HEAP_START,
		    (unsigned)BOOT_APP_HEAP_SIZE);
	}
	// #endregion agent log

	/* USB enum first (CDC proven path), then tasks — heap now valid. */
	App_UsbInit();
	// #region agent log
	diag_putc('B');
	// #endregion agent log

	/* H43 rejected: GIE_DISABLE around xTaskCreate was unproven and
	 * diverged from BanBox (GET_PSW() at create should see normal PSW). */
	GIE_ENABLE();

	xTaskCreate(vUsbTask,
		    "USB",
		    configMINIMAL_STACK_SIZE * 4,
		    NULL,
		    tskIDLE_PRIORITY + 3,
		    &xUsbTaskHandle);
	// #region agent log
	diag_putc('C');
	// #endregion agent log

	xTaskCreate(vPrintTask,
		    "Print",
		    configMINIMAL_STACK_SIZE * 4,
		    NULL,
		    tskIDLE_PRIORITY + 1,
		    &xPrintTaskHandle);

	// #region agent log
	diag_putc('D');
	DBG("{\"sessionId\":\"5032d7\",\"runId\":\"post-fix\",\"hypothesisId\":\"H42,H43\","
	    "\"location\":\"main.c:main\",\"message\":\"pre_scheduler\","
	    "\"data\":{\"tick\":%u,\"mode\":%u},\"timestamp\":%u}\n",
	    (unsigned)gSysTick, (unsigned)CFG_PARA_USB_MODE,
	    (unsigned)gSysTick);
	// #endregion agent log
	/*
	 * BanBox enables SWI before vTaskStartScheduler — FreeRTOS yield/tick
	 * switch via OS_Trap_Interrupt_SWI. Without it, first task may never
	 * run or Delay/YIELD hangs (H45: no usb_task_* after "Starting...").
	 */
	NVIC_EnableIRQ(SWI_IRQn);
	DBG("Starting FreeRTOS scheduler...\n");
	// #region agent log
	diag_putc('S');
	// #endregion agent log
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
