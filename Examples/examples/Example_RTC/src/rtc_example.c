/**
 **************************************************************************************
 * @file    rtc_example.c
 * @brief   rtc example
 *
 * @author  Tony
 * @version V1.0.1
 *
 * $Created: 2019-08-08 11:25:00$
 *
 * @Copyright (C) 2019, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <stdlib.h>
#include <nds32_intrinsic.h>
#include "gpio.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "type.h"
#include "debug.h"
#include "timeout.h"
#include "clk.h"
#include "dma.h"
#include "timer.h"
#include "watchdog.h"
#include "spi_flash.h"
#include "remap.h"
#include "rtc.h"
#include "irqn.h"
#include "chip_info.h"


uint32_t SecLast = 0;
uint32_t time = 0;
uint32_t alarm =0;

bool alarmFlag = FALSE;

void SysTickInit(void);

void OSC32K_Example(void)
{
	Clock_EnableLOSC32K();
	RTC_ClockSrcSel(OSC_32K);
	RTC_IntDisable();
	RTC_IntFlagClear();
	RTC_WakeupDisable();

	time = 1;
	RTC_SecSet(time);
	alarm = 10;
	RTC_SecAlarmSet(alarm);

	RTC_IntEnable();
	NVIC_EnableIRQ(RTC_IRQn);
	GIE_ENABLE();
	while(1)
	{
		time = RTC_SecGet();
		if(SecLast != time)
		{
			SecLast = time;
			DBG("rtc:%lds\n",time);

			if(TRUE == alarmFlag)
			{
				alarmFlag = FALSE;
				DBG("there is an alarm(int mode)\n");
				alarm = time+10;
				RTC_SecAlarmSet(alarm);
			}
		}
	}
}

void OSC24M_Example(void)
{
	RTC_ClockSrcSel(OSC_24M);
	RTC_IntDisable();
	RTC_IntFlagClear();
	RTC_WakeupDisable();

	time = 1;
	RTC_SecSet(time);
	alarm = 10;
	RTC_SecAlarmSet(alarm);

	RTC_IntEnable();
	NVIC_EnableIRQ(RTC_IRQn);
	GIE_ENABLE();

	while(1)
	{
		time = RTC_SecGet();
		if(SecLast != time)
		{
			SecLast = time;
			DBG("rtc:%lds\n",time);

			if(TRUE == alarmFlag)
			{
				alarmFlag = FALSE;
				DBG("there is an alarm(int mode)\n");
				alarm = time+10;
				RTC_SecAlarmSet(alarm);
			}
		}
	}
}


static int32_t WaitDatum1Ever(void)//从串口等待接收一个字节
{
	uint8_t Data;
	DBG("*******************Please input a num (0~9)*********************\n");
	do{
		if(0 < UARTS_Recv(1,&Data, 1,10))
        {
			break;
		}
	}while(1);
	return Data-0x30;
}

int main(void)
{
	uint8_t  recvBuf;

	Chip_Init(1);
	__c_init_rom();
	WDG_Disable();

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
    GPIO_PortAModeSet(GPIOA9, 1);  // Rx, A9:uart1_rxd_1
    GPIO_PortAModeSet(GPIOA10, 5); // Tx, A10:uart1_txd_1
    DbgUartInit(1, 2000000, 8, 0, 1);

	SysTickInit();

	DBG("\n");
    DBG("/-----------------------------------------------------\\\n");
    DBG("|                    RTC Example                    |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n\n");
	DBG("-------------------------------------------------------\n");
	DBG("1:  Enter OSC_32K Example\n");
	DBG("2:  Enter OSC_24M Example\n");
	DBG("-------------------------------------------------------\n");
    GIE_ENABLE();	//开启总中断
	recvBuf = WaitDatum1Ever();
	switch(recvBuf)
	{
		case 1:
			DBG(">>>>>>>>>>>>>>>>>>OSC32K_Example<<<<<<<<<<<<<<<<<<<<<<<\n\n");
			OSC32K_Example();
			break;

		case 2:
			DBG(">>>>>>>>>>>>>>>>>>OSC24M_Example<<<<<<<<<<<<<<<<<<<<<<<\n\n");
			OSC24M_Example();
			break;

		default:
			break;
	}
	return 0;
}

__attribute__((section(".driver.isr")))void RtcInterrupt(void)//RTC唤醒 并不会进入RTC中断
{
	if(RTC_IntFlagGet() == TRUE)
	{
		alarmFlag = TRUE;
		RTC_IntFlagClear();
	}
}


