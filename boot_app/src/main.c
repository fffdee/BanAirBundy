/**
 * @file    main.c
 * @brief   boot_app — BARE-METAL APP with BanUX + USB CDC/Audio + wireless.
 *
 * No RTOS scheduler. This mirrors the vendor wireless_mic_tx_sdk / rx_sdk super
 * loop, whose whole "USB does not disturb RF" trick is interrupt priority
 * layering plus a cooperative main loop:
 *
 *   BT_IRQn  (RF 2T1R state machine) priority 0  <- highest, never preempted
 *   Usb_IRQn (USB device)            priority 1
 *   Timer2_IRQn (1ms tick)           priority 2  <- lowest, carries USB-audio
 *                                                   1ms framing in its ISR
 *
 * The hard-real-time radio work runs inside the BT_IRQn ISR, so however long
 * OTG_DeviceRequestProcess()/Banux_Process() take in the loop below, the RF
 * still fires on time. while(1) only does the cooperative slow path.
 *
 * BanUX: event bus, VFS/driver framework, CDC Shell (help/boot), fw upgrade.
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
#include "timer.h"
#include "uarts_interface.h"
#include "sram_config.h"
#include "clock_config.h"
#include "app_config.h"
#include "spi_flash.h"
#include "Banux.h"
#include "fw_upgrade.h"
#include "dual_partition.h"
#include "app_version.h"

#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "otg_device_cdc.h"
#include "usb_audio_api.h"
#include "delay.h"
#include "shell_io_cdc.h"
#include "wireless_app.h"

extern int BanuxDriver_RegisterAll(void);

/*
 * FreeRTOS is still linked (its kernel sources are in the build) but the
 * scheduler is NEVER started here, so the kernel is inert dead code. The
 * headers + application hooks below are kept only so the kernel's hook
 * references still resolve at link time. No task/queue/semaphore is created.
 */
#include "FreeRTOS.h"
#include "task.h"

extern void OTG_DeviceFifoInit(void);
extern void SysTickInit(void);
extern void UsbAudioTimer1msProcess(void);
extern volatile uint32_t gSysTick;

/* Millisecond tick for super-loop RF diagnostics; incremented in Timer2 ISR. */
static volatile uint32_t s_app_ms;

/*
 * BT_IRQn (RF 2T1R ISR) entry counter. ISR_TABLE[15] is repointed to
 * BtIrqProbe() (see crt0.S + below), which bumps this then tail-calls the
 * vendor handler rwbt_isr(). Wireless_DiagReport() prints it: a climbing
 * bt_irq proves the radio baseband is generating interrupts (RF alive); a
 * frozen 0 means the RF ISR never fires (baseband never started). Defined
 * here (always compiled), externed by wireless_lib/mvwire_stack.c.
 */
volatile uint32_t g_bt_irq_count;
extern void rwbt_isr(void);	/* real vendor RF ISR, libwirelessStack.a */

/* APP USB identity (distinct from bootloader 0x8888/0x1722) */
#define APP_USB_VID   0x8888
#define APP_USB_PID   USBPID(CFG_PARA_USB_MODE)

static uint8_t DmaChannelMap[] =
{
	PERIPHERAL_ID_AUDIO_ADC1_RX,
	PERIPHERAL_ID_AUDIO_DAC0_TX,
	255, 255, 255, 255,
};

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
 * USB device bring-up (clock + FIFO + OTG device). Enumeration is NOT waited
 * on here: EP0/CDC are pumped cooperatively in the super loop so the radio is
 * never blocked by a host that is slow (or absent). Usb_IRQn is priority 1 —
 * strictly below the RF (BT_IRQn priority 0).
 */
static void App_UsbInit(void)
{
	/*
	 * USB clock: DPLL/5 = 48MHz (BL-proven).
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
	/* RF owns priority 0; USB sits at 1 so it can never preempt the radio. */
	NVIC_SetPriority(Usb_IRQn, 1);
	NVIC_EnableIRQ(Usb_IRQn);
}

/**
 * Timer2: 1ms periodic tick at the LOWEST interrupt priority (2). Its ISR
 * carries UsbAudioTimer1msProcess() so USB-audio framing is time-driven yet
 * can never disturb the RF. Matches the vendor SDK exactly.
 */
static void App_Timer2Init(void)
{
	Timer_Config(TIMER2, 1000, 0);	/* 1000us = 1ms, periodic */
	Timer_Start(TIMER2);
	NVIC_SetPriority(Timer2_IRQn, 2);
	NVIC_EnableIRQ(Timer2_IRQn);
}

/*
 * Load boot_app's own RF firmware (.tcm_section) into TCM and address-remap it,
 * so the BB MPU can execute the 2T1R radio state machine (SWBB_2t1r_*).
 *
 * The bootloader's Remap_InitTcm(0, 0, 12) is a DUMMY: it copies/remaps flash
 * 0x0 (the bootloader region), NOT boot_app's RF image. Left as-is, the main
 * CPU Wireless_common_init() still builds the sync table (that is why the boot
 * log prints the sync words), but the radio state machine -- which lives in
 * .tcm_section and must run from TCM -- never executes, so TX/RX never connect.
 *
 * Layout (Debug/output/readelf.txt):
 *   .tcm_section  flash 0x00042044  size 0x4660 (~18KB)  ends 0x000466A4
 * remap.h: src/dst/size need only 1KB alignment. We remap a WIRELESS_TCM_SIZE
 * (20KB) window from a 1KB-aligned base at/below the section start so the whole
 * section is covered:  [0x42000,0x47000) contains [0x42044,0x466A4).
 *
 * If a future boot log prints "remap ret=-1", the HW granularity is coarser
 * than 1KB; then move .tcm_section to the 0x40000 page base in
 * nds32-ae210p.ld/.sag (as the vendor SDK does) and set rf_src = PART_A_BASE.
 */
static REMAP_ERROR App_LoadRfFirmwareToTcm(void)
{
	const uint32_t rf_src = 0x00042000u;	/* 1KB-aligned, <= .tcm_section start */

	memcpy((void*)TCM_SRAM_START_ADDR_1, (const void*)rf_src, WIRELESS_TCM_SIZE * 1024);
	return Remap_AddrRemapSet(ADDR_REMAP0, rf_src, TCM_SRAM_START_ADDR_1, WIRELESS_TCM_SIZE);
}

/*
 * Configure the RF analog power rails exactly like the vendor SDK main()
 * (wireless_mic_tx_sdk/main.c L510-519). The bootloader only runs Chip_Init()
 * + clock setup; it never enables LDO16 (1.6V RF/core analog) nor lowers the
 * 3.3V supply to the 2.9V LDO33D rail. Without these the 2.4G radio front-end
 * is mis-biased: Wireless_common_init() (main-CPU code) still builds the sync
 * table so the boot log looks healthy, but the radio never actually
 * transmits/receives -> TX/RX never connect (s_pair_st=0, dev id=-1, no
 * device1_conn). boot_app defines CFG_VDD3V3_3V and not CFG_DCDC_EN (same as
 * the reference), so this resolves to Power_LDO16Config(1)+Power_LDO33DConfig(2).
 */
static void App_RfPowerConfig(void)
{
#ifdef CFG_DCDC_EN
	ldo_switch_to_dcdc(5);
#else
	Power_LDO16Config(1);
#endif
#ifdef CFG_VDD3V3_3V
	Power_LDO33DConfig(2);	/* 3.3V supply -> 2.9V rail */
#endif
}

int main(void)
{
	uint8_t last_dtr = 0u;
	uint32_t last_diag_ms = 0u;

#if HAS_BOOTLOADER
	WDG_Disable();
	/* Keep IRQs masked during early init (BL left the tick running). */
	__nds32__setgie_dis();

	DbgUartInit(1, 115200, 8, 0, 1);

	App_RfPowerConfig();

	{
		REMAP_ERROR rf_remap_ret = App_LoadRfFirmwareToTcm();
		DBG("[TCM] RF fw -> TCM 0x%08X, %dKB, remap ret=%d, word0=0x%08X\n",
		    (unsigned)TCM_SRAM_START_ADDR_1, WIRELESS_TCM_SIZE, (int)rf_remap_ret,
		    (unsigned)*(volatile uint32_t*)(TCM_SRAM_START_ADDR_1 + 0x44));
	}
	SpiFlashInit(80000000, MODE_4BIT, 0, 1);
	DMA_ChannelAllocTableSet(DmaChannelMap);

	DBG("\n\n=== boot_app @ 0x%08X (from bootloader, bare-metal) ===\n",
	    (unsigned)PART_A_BASE);
#else
	App_LoadRfFirmwareToTcm();

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

	App_RfPowerConfig();

	SpiFlashInit(80000000, MODE_4BIT, 0, 1);
	DMA_ChannelAllocTableSet(DmaChannelMap);

	DBG("\n\n=== boot_app @ 0x%08X (standalone, bare-metal) ===\n",
	    (unsigned)PART_A_BASE);
#endif

	DBG("[APP] BanAirBundy boot_app v%s (%s %s)\n",
	    APP_VERSION_STRING, __DATE__, __TIME__);
	DBG("DPLL: %d kHz, APLL: %d kHz\n", SYS_CORE_DPLL_FREQ, SYS_CORE_APLL_FREQ);

	FwUpgrade_BootInit();
	FwUpgrade_ConfirmBootSuccess();

	/* Bare-metal heap over [_end .. BP15_HEAP_END). Replaces heap_5s and
	 * backs wireless_lib allocations (WL_USE_FREERTOS_HEAP=0 -> T_PortMalloc). */
	T_HeapInit();
	DBG("T_Heap: free=%lu bytes\n", (unsigned long)T_PortGetFreeHeapSize());

	/* Peripheral init under masked IRQs; interrupts are enabled just below. */
	App_UsbInit();
	App_Timer2Init();
	App_BanuxInit();

	GIE_ENABLE();

	/*
	 * RF frequency/osc calibration lives in the PMU NVM. The vendor SDK calls
	 * PMU_NVMInit() right before WIRELESS_FUNCTION()/Wireless_common_init() so
	 * the radio can read its per-chip freq trim. boot_app inherits the
	 * bootloader's Chip_Init() but never enabled NVM access, so the RF
	 * bring-up's PMU_NvmRead() fails and the radio runs uncalibrated -- TX and
	 * RX then sit on slightly different frequencies and never sync (the sync
	 * table is built, but s_pair_st stays 0 / dev id stays -1 / no device1_conn).
	 */
	PMU_NVMInit();

	/* Bring up the RF last: BT_IRQn is enabled at priority 0 inside the stack. */
	if (App_WirelessStart() != 0)
		DBG("[WL] App_WirelessStart failed — continue without RF\n");

	/* Reaffirm RF as the highest-priority IRQ (vendor SDK sets this last). */
	NVIC_SetPriority(BT_IRQn, 0);

	DBG("Entering bare-metal super loop\n");

	for (;;) {
		WDG_Feed();

		/* RF cooperative glue first; real-time state machine is in BT_IRQn. */
		App_WirelessSchedule();

		/* Read-only RF liveness report ~every 2s: shows whether the radio
		 * state machine is advancing (conn/sync/TX scan state). */
		if ((uint32_t)(s_app_ms - last_diag_ms) >= 2000u) {
			last_diag_ms = s_app_ms;
			App_WirelessDiag();
		}

		/* USB EP0 / CDC / audio bulk pump. */
		OTG_DeviceRequestProcess();

		if (g_usb_configured && !UsbCDC.InitOk) {
			OTG_DeviceCDC_Init();
			DBG("[USB] CDC InitOk\n");
		}

		if (UsbCDC.InitOk) {
			/*
			 * Shell welcome/TX needs host DTR (COM open). Sending Bulk-IN
			 * before that makes libDriver print "SEND ERROR" and can
			 * disturb the Windows CDC open.
			 */
			if (UsbCDC.ControlLineState.DTR && !last_dtr) {
				OTG_DeviceCDC_FlushRxBuffer();
				Shell_ResetInputLine();
			}
			last_dtr = UsbCDC.ControlLineState.DTR;

			/* CDC data is valid independently of DTR. */
			OTG_DeviceCDC_Task();
			Banux_Process();	/* shell + event bus + fw upgrade */
		}

		if (CFG_PARA_USB_MODE != CDC_ONLY) {
			UsbAudioSpeakerStreamProcess();
			UsbAudioMicStreamProcess();
		}
	}
}

/**
 * Timer2 (IRQ7) 1ms ISR — lowest priority. Carries the USB-audio 1ms framing
 * exactly like the vendor SDK, keeping USB timing off the RF's critical path.
 */
void Timer2Interrupt(void)
{
	Timer_InterruptFlagClear(TIMER2, UPDATE_INTERRUPT_SRC);
	s_app_ms++;	/* 1ms tick for super-loop RF diagnostics */
	if (CFG_PARA_USB_MODE != CDC_ONLY)
		UsbAudioTimer1msProcess();	/* 1ms interrupt-level */
}

/**
 * BT_IRQn (IRQ15) probe. crt0.S repoints ISR_TABLE[15] here. The dispatcher
 * OS_Trap_Int_Comm does SAVE_ALL_HW -> `jral ISR_TABLE[15]` -> RESTORE_ALL_HW
 * -> iret, i.e. it calls this as an ordinary void(void) C function with the
 * interrupt context already saved, so wrapping is safe. Count the entry (the
 * decisive RF-liveness proof) then tail-call the real vendor handler. This
 * line runs at priority 0 (above USB=1 / Timer2=2) and is never preempted;
 * the extra call costs only a few cycles, negligible against the RF frame
 * period. rwbt_isr stays strongly linked because libwirelessStack.a is pulled
 * in for Wireless_common_init() regardless of this reference.
 */
void BtIrqProbe(void)
{
	g_bt_irq_count++;
	rwbt_isr();
}

/* ---- FreeRTOS application hooks (kernel linked but scheduler never run) ---- */

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
