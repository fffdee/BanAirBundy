/**
 **************************************************************************************
 * @file    main.c
 * @brief   Bootloader — USB CDC firmware upgrade only
 *
 * Flow:
 *   1. Chip / clock / UART / SPI flash init (BP1540 Example_USB device path)
 *   2. DualPart_Init + Boot_CheckAndJumpIfNeeded (may jump to APP)
 *   3. Stay in bootloader: enumerate as CDC_ONLY, run upgrade protocol
 **************************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <nds32_intrinsic.h>
#include "chip_info.h"
#include "uarts_interface.h"
#include "gpio.h"
#include "type.h"
#include "irqn.h"
#include "debug.h"
#include "clk.h"
#include "sys.h"
#include "spi_flash.h"
#include "watchdog.h"
#include "dma.h"
#include "remap.h"

#include "app_config.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "otg_device_cdc.h"
#include "timeout.h"
#include "delay.h"

#include "dual_partition.h"
#include "app_upgrade.h"
#include "cdc_upgrade.h"

extern uint32_t gSysTick;

#define BOOTLOADER_VERSION_STR  "V1.0.0"

/* Bootloader CDC identity (host tools identify upgrade mode by VID/PID) */
#define BL_USB_VID   0x8888
#define BL_USB_PID   0x1722

/* 本平台仅 6 路 DMA：下标=通道号，值为外设 ID（255=空闲） */
static uint8_t DmaChannelMap[] =
{
    255,
    255,
    255,
    255,
    255,
    255,
};

/* ROM helper used by BP1540 Example_USB before clock bring-up */
extern void __c_init_rom(void);
extern void OTG_DeviceFifoInit(void);

static void usb_cdc_upgrade_loop(void)
{
    /* Prepare upgrade stack BEFORE D+ pull-up.
     * Host starts enumerating the moment DP is up; RequestProcess must run then. */
    App_Upgrade_Init();
    CDC_Upgrade_Init();
    CDC_Upgrade_EnterMode();

    DelayMs(20);

    DBG("[BOOT] USB CDC upgrade ready VID=0x%04X PID=0x%04X — attaching USB\n",
        BL_USB_VID, BL_USB_PID);

    OTG_DeviceModeSel(CDC_ONLY, BL_USB_VID, BL_USB_PID);
    OTG_DeviceFifoInit();
    OTG_DeviceInit(); /* enables DP pull-up — host may SETUP immediately */
    NVIC_EnableIRQ(Usb_IRQn);
    NVIC_SetPriority(Usb_IRQn, 0);

    while (1) {
        OTG_DeviceRequestProcess();

        /* Defer CDC_Init until SET_CONFIGURATION status is done. */
        if (g_usb_configured && !UsbCDC.InitOk) {
            OTG_DeviceCDC_Init();
        }

        OTG_DeviceCDC_Task();

        if (!CDC_Upgrade_InMode()) {
            CDC_Upgrade_CheckEnter();
        }
        if (CDC_Upgrade_InMode()) {
            CDC_Upgrade_Process();
        }
    }
}

int main(void)
{
    Chip_Init(1);
    WDG_Disable();
    __c_init_rom();

    /*
     * Clock sequence aligned with BP1540 Example_USB (device-only path):
     *   Config → HOSC↑ → DPLL/APLL lock → ModuleEnable → Sys/UART select → HOSC↓
     * USB: DPLL / 5 = 48MHz (SDK clock_config.h: default div=5 for 240M DPLL).
     * Evidence: apl 0/1 both failed with div=4 (60MHz); EP0 CSR never latched SETUP.
     */
    Clock_Config(1, 24000000);
    Clock_HOSCCurrentSet(15);
    Clock_PllLock(240 * 1000);
    Clock_APllLock(240 * 1000);

    Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
    Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
    Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);

    Clock_SysClkSelect(PLL_CLK_MODE);
    Clock_UARTClkSelect(PLL_CLK_MODE); /* Example: also leaves USB mux on DPLL */
    Clock_USBClkDivSet(5);             /* 240/5=48MHz — NOT div=4 (60MHz) */
    Clock_USBClkSelect(PLL_CLK_MODE);
    Clock_HOSCCurrentSet(5);

    /* Clear stale partition-B remap before any jump decision */
    Remap_AddrRemapDisable(ADDR_REMAP0);
    Remap_AddrRemapDisable(ADDR_REMAP1);
    Remap_AddrRemapDisable(ADDR_REMAP2);
    Remap_InitTcm(0, 0, 12);

    SpiFlashInit(80000000, MODE_4BIT, 0, 1);
    DMA_ChannelAllocTableSet(DmaChannelMap);

    GPIO_PortAModeSet(GPIOA10, 5); /* UART1 TX — BP15 board default */
    GPIO_PortAModeSet(GPIOA9, 1);  /* UART1 RX */
    DbgUartInit(1, 115200, 8, 0, 1);

    GIE_ENABLE();
    /* Required: OTG_DeviceControlSend/WaitEnd timeouts use GetSysTick1MsCnt().
     * Without this, tick stays 0 (log alive=0) and EP0 GET_DESCRIPTOR dies → Win Code 43. */
    SysTickInit();

    DualPart_Init();

    DBG("\n");
    DBG("/-----------------------------------------------------\\\n");
    DBG("|              Bootloader " BOOTLOADER_VERSION_STR "                     |\n");
    DBG("|           USB CDC Firmware Upgrade                  |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");

    /* May never return when a valid APP is found */
    Boot_CheckAndJumpIfNeeded();

    DBG("[BOOT] Entering USB CDC upgrade mode\n");
    usb_cdc_upgrade_loop();

    while (1) { ; }
}
