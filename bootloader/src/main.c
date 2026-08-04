/**
 **************************************************************************************
 * @file    main.c
 * @brief   Bootloader — USB CDC firmware upgrade only
 *
 * Flow:
 *   1. Chip / clock / UART / SPI flash init
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
#include "timer.h"
#include "remap.h"

#include "app_config.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "otg_device_cdc.h"
#include "otg_detect.h"

#include "dual_partition.h"
#include "app_upgrade.h"
#include "cdc_upgrade.h"

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

void Timer2Interrupt(void)
{
    Timer_InterruptFlagClear(TIMER2, UPDATE_INTERRUPT_SRC);
    OTG_PortLinkCheck();
}

extern void OTG_DeviceFifoInit(void);

static void usb_cdc_upgrade_loop(void)
{
    OTG_DeviceModeSel(CDC_ONLY, BL_USB_VID, BL_USB_PID);
    OTG_DeviceFifoInit();
    OTG_DeviceInit();
    NVIC_EnableIRQ(Usb_IRQn);
    NVIC_SetPriority(Usb_IRQn, 0);

    App_Upgrade_Init();
    CDC_Upgrade_Init();
    CDC_Upgrade_EnterMode();

    DBG("[BOOT] USB CDC upgrade ready VID=0x%04X PID=0x%04X\n",
        BL_USB_VID, BL_USB_PID);

    while (1) {
        OTG_DeviceRequestProcess();
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

    Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
    Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
    Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);

    Clock_Config(1, 24000000);
    Clock_HOSCCurrentSet(15);
    Clock_PllLock(240 * 1000);
    Clock_APllLock(240 * 1000);
    Clock_SysClkSelect(PLL_CLK_MODE);
    Clock_UARTClkSelect(PLL_CLK_MODE);
    Clock_USBClkDivSet(4);
    Clock_USBClkSelect(APLL_CLK_MODE);
    Clock_HOSCCurrentSet(5);

    /* Clear stale partition-B remap before any jump decision */
    Remap_AddrRemapDisable(ADDR_REMAP0);
    Remap_AddrRemapDisable(ADDR_REMAP1);
    Remap_AddrRemapDisable(ADDR_REMAP2);
    Remap_InitTcm(0, 0, 12);

    SpiFlashInit(80000000, MODE_4BIT, 0, 1);
    DMA_ChannelAllocTableSet(DmaChannelMap);

    GPIO_PortAModeSet(GPIOA10, 5); /* UART1 TX */
    GPIO_PortAModeSet(GPIOA9, 1);  /* UART1 RX */
    DbgUartInit(1, 115200, 8, 0, 1);

    GIE_ENABLE();

    Timer_Config(TIMER2, 1000, 0);
    Timer_Start(TIMER2);
    NVIC_EnableIRQ(Timer2_IRQn);

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
