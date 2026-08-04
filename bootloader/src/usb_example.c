/**
 **************************************************************************************
 * @file    usb_example.c
 * @brief   usb example
 *
 * @author  Shanks
 * @version V1.0.0
 *
 * $Created: 2025-1-22 10:16:10$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
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
#include "sd_card.h"
#include "spi_flash.h"
#include "watchdog.h"
#include "otg_detect.h"
#include "otg_host_udisk.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "otg_device_stor.h"
#include "otg_device_audio.h"
#include "usb_audio_api.h"

extern void __c_init_rom();
extern void audio_init(uint32_t SampleRate);
extern uint8_t OtgPortLinkState;

static uint8_t DmaChannelMap[] =
{
    PERIPHERAL_ID_AUDIO_ADC0_RX,
    PERIPHERAL_ID_AUDIO_ADC1_RX,
    PERIPHERAL_ID_AUDIO_DAC0_TX,
	PERIPHERAL_ID_SDIO_TX,
	PERIPHERAL_ID_SDIO_RX,
    255,
};

void Timer2Interrupt(void)
{
	Timer_InterruptFlagClear(TIMER2, UPDATE_INTERRUPT_SRC);
	UsbAudioTimer1msProcess(); //声卡1ms中断监控
}

void Timer6Interrupt(void)
{
	Timer_InterruptFlagClear(TIMER6, UPDATE_INTERRUPT_SRC);
	OTG_PortLinkCheck();	//检测函数
}

uint8_t read_buf[4096];
uint8_t write_buf[4096];
void usb_host_example(void)
{
	DBG("usb host example\n");

	//1：host模式配置
	OTG_HostFifoInit();
	OTG_HostControlInit();

	//2：host枚举设备
	OTG_HostEnumDevice();
	if(UDiskInit() == 1)
	{
		DBG("枚举MASSSTORAGE接口OK\n");
		UDiskReadBlock(0,write_buf,1);
		UDiskWriteBlock(0,write_buf,1);
		UDiskReadBlock(0,read_buf,1);
		if(memcmp(write_buf,read_buf,512) == 0)
		{
			DBG("读写U盘数据OK\n");
		}
		else
		{
			DBG("读写U盘数据错误\n");
		}
	}
	else
	{
		DBG("UDisk Init err\n");
	}
	while(1);
}

void usb_device_example(void)
{
	printf("usb_device_example\n");
	DMA_ChannelAllocTableSet(DmaChannelMap);
#if (CFG_PARA_USB_MODE >= READER)
	CardPortInit(SDIO_A15_A16_A17);
	if(SDCard_Init() != NONE_ERR)
	{
		DBG("sd card error\n");
		while(1);
	}
#else
	audio_init(CFG_PARA_SAMPLE_RATE);
	UsbDevicePlayInit();
	UsbDevicePlayResMalloc();
#endif
	OTG_DeviceModeSel(CFG_PARA_USB_MODE,USB_VID,USB_PID_BASE);
	OTG_DeviceFifoInit();
	OTG_DeviceInit();
	NVIC_EnableIRQ(Usb_IRQn);

#if (CFG_PARA_USB_MODE < READER)
 	Timer_Config(TIMER2,1000,0);
 	Timer_Start(TIMER2);
 	NVIC_EnableIRQ(Timer2_IRQn);
#endif

	while(1)
	{
		OTG_DeviceRequestProcess();
#if (CFG_PARA_USB_MODE >= READER)
		OTG_DeviceStorProcess();
#else
		UsbAudioSpeakerStreamProcess();
		UsbAudioMicStreamProcess();
#endif
	}
}


void OTG_DeviceSuspendProcess(void)
{
	printf("Suspend\n");
}
void OTG_DeviceResumeProcess(void)
{
	printf("Resume\n");
}
void OTG_DeviceResetProcess(void)
{
	printf("Reset\n");
}

void usb_device_Interrupt_example(void)
{
	DBG("usb_device_Interrupt_example\n");
	DMA_ChannelAllocTableSet(DmaChannelMap);
#if (CFG_PARA_USB_MODE >= READER)
	CardPortInit(SDIO_A15_A16_A17);
	if(SDCard_Init() != NONE_ERR)
	{
		DBG("sd card error\n");
		while(1);
	}
#else
	audio_init(CFG_PARA_SAMPLE_RATE);
	UsbDevicePlayInit();
	UsbDevicePlayResMalloc();
#endif
	OTG_DeviceModeSel(CFG_PARA_USB_MODE,USB_VID,USB_PID_BASE);
	OTG_DeviceFifoInit();
	OTG_DeviceInit();

	OTG_EndpointInterruptEnable(DEVICE_CONTROL_EP,OTG_DeviceRequestProcess);
	OTG_BusEventInterruptEnable(0,OTG_DeviceSuspendProcess);
	OTG_BusEventInterruptEnable(1,OTG_DeviceResumeProcess);
	OTG_BusEventInterruptEnable(2,OTG_DeviceResetProcess);

	NVIC_EnableIRQ(Usb_IRQn);

#if (CFG_PARA_USB_MODE < READER)
 	Timer_Config(TIMER2,1000,0);
 	Timer_Start(TIMER2);
 	NVIC_EnableIRQ(Timer2_IRQn);
#endif

	while(1)
	{
#if (CFG_PARA_USB_MODE >= READER)
		OTG_DeviceStorProcess();
#else
		UsbAudioSpeakerStreamProcess();
		UsbAudioMicStreamProcess();
#endif
	}
}

void usb_otg_example(void)
{
 	Timer_Config(TIMER6,1000,0);
 	Timer_Start(TIMER6);
 	NVIC_EnableIRQ(Timer6_IRQn);
 	OTG_PortSetDetectMode(1,1);
 	while(1)
 	{
 		if(OTG_PortHostIsLink())
 		{
 			usb_host_example();
 		}
 		else if(OTG_PortDeviceIsLink())
 		{
 			usb_device_example();
 		}
 	}
}

int main(void)
{
    uint8_t Key = 0;

	Chip_Init(1);
	WDG_Disable();
	__c_init_rom();
    Clock_Config(1, 24000000);
    Clock_HOSCCurrentSet(15);  // 加大了晶体的偏置电流
    Clock_PllLock(240 * 1000); // 240M频率
    Clock_APllLock(240 * 1000);
    Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
    Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
    Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);
    Clock_SysClkSelect(PLL_CLK_MODE);
    Clock_UARTClkSelect(PLL_CLK_MODE);
    Clock_HOSCCurrentSet(5);

	SpiFlashInit(80000000, MODE_4BIT, 0, 1);

    // BP15系列开发板启用串口，默认使用
    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
    DbgUartInit(1, 2000000, 8, 0, 1);

	GIE_ENABLE();
	SysTickInit();

	DBG("\n");
	DBG("/-----------------------------------------------------\\\n");
	DBG("|                     USB Example                     |\n");
	DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
	DBG("\\-----------------------------------------------------/\n");
	DBG("\n");

	DBG("Select an example:\n");
	DBG("0: USB OTG link check\n");
	DBG("1: USB Only host mode no link check\n");
	DBG("2: USB Only device mode no link check\n");
	DBG("3: USB Only device Interrupt mode no link check\n");
	while(1)
	{
		if(UARTS_RecvByte(UART_PORT1, &Key))
		{
			switch(Key)
			{
			case '0':
				DBG("USB OTG link check\n");
				usb_otg_example();
				break;
			case '1':
				DBG("USB Only host mode\n");
				OtgPortLinkState = 0;
				usb_host_example();
				break;
			case '2':
				DBG("USB Only device mode\n");
				usb_device_example();
				break;
			case '3':
				DBG("USB Only device Interrupt mode\n");
				usb_device_Interrupt_example();
				break;

			default:
				break;
			}
		}
	}
	while(1);
}
