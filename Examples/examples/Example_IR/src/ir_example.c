/**
 **************************************************************************************
 * @file    ir_example.c
 * @brief   ir example
 *
 * @author  Taowen
 * @version V1.0.0
 *
 * $Created: 2019-06-03 19:17:00$
 *
 * @Copyright (C) 2019, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <stdlib.h>
#include <nds32_intrinsic.h>
#include <string.h>
#include "gpio.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "type.h"
#include "debug.h"
#include "timeout.h"
#include "clk.h"
#include "dma.h"
#include "timer.h"
#include "spi_flash.h"
#include "remap.h"
#include "irqn.h"
#include "sys.h"
#include "chip_info.h"
#include "watchdog.h"
#include "reset.h"
#include <string.h>
#include "type.h"
#include "irqn.h"
#include "gpio.h"
#include "debug.h"
#include "timer.h"
#include "dma.h"
#include "uarts_interface.h"
#include "clk.h"
#include "watchdog.h"
#include "spi_flash.h"
#include "remap.h"
#include "chip_info.h"
#include "ir.h"
#include "delay.h"
#include "powercontroller.h"

//注意使用的UART端口
#define UART_TX_IO	1
#define UART_RX_IO  1

void NEC_IR_RecieveCmdExample(void)
{
	bool CmdOK = FALSE;
	uint32_t Cmd = 0;
	uint8_t RepeatCount = 0;
	uint8_t Last_RepeatCount = 0xff;

	DBG("enter NEC IR---B6 Recieve Command Example\n\n");

	GPIO_RegOneBitSet(GPIO_B_IE, GPIO_INDEX6);
	GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX6);

	IR_Config(IR_MODE_NEC, IR_GPIOB6, IR_NEC_32BITS);
	IR_Enable();

	while(1)
	{
		CmdOK = IR_CommandFlagGet();
		if(CmdOK == TRUE)
		{
			DBG("\n查询方式\n");
			Cmd = IR_CommandDataGet();
			DBG("\n---IR_Cmd:%08lx\n",Cmd);
			IR_IntFlagClear();

			RepeatCount = IR_RepeatCountGet();
			DBG("\n---RepeatCount:%0x\n",RepeatCount);
			while(1)
			{
				DelayMs(120);//NEC IR repeat code is transmited every 110mS

				Last_RepeatCount = RepeatCount;

				RepeatCount = IR_RepeatCountGet();
				DBG("\n*********RepeatCount:%0x\n",RepeatCount);

				if( (Last_RepeatCount - RepeatCount) > 0 )
				{
					Cmd = IR_CommandDataGet();
					DBG("\n--------There is a New IR_Command:%08lx------\n",Cmd);
					DBG("\n*****************IR_Cmd:%08lx\n",Cmd);
					IR_IntFlagClear();
				}

				if(RepeatCount == Last_RepeatCount)
				{
					IR_CommandFlagClear();
					break;
				}
			}
		}
	}
}

void NEC_IR_InterruptExample(void)
{
	DBG("enter NEC IR---B6 Interrupt Example\n\n");

	GPIO_RegOneBitSet(GPIO_B_IE, GPIO_INDEX6);
	GPIO_RegOneBitClear(GPIO_B_IE, GPIO_INDEX6);
	IR_Config(IR_MODE_NEC, IR_GPIOB6, IR_NEC_32BITS);

	NVIC_EnableIRQ(IR_IRQn);
	NVIC_SetPriority(IR_IRQn, 1);
	GIE_ENABLE();

	IR_Enable();//开启IR功能
	IR_InterruptEnable();//开启IR中断
}


void SONY_IR_RecieveCmdExample(void)
{
	bool CmdOK = FALSE;
	uint32_t Cmd = 0;
	uint8_t RepeatCount = 0;
	uint8_t Last_RepeatCount = 0xff;

	DBG("enter SONY IR---B6 Recieve Command Example\n\n");

	GPIO_RegOneBitSet(GPIO_B_IE, GPIO_INDEX6);
	GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX6);

	IR_Config(IR_MODE_SONY, IR_GPIOB6, IR_SONY_12BITS);
	IR_Enable();

	while(1)
	{
		CmdOK = IR_CommandFlagGet();
		if(CmdOK == TRUE)
		{
			DBG("\n查询方式\n");
			Cmd = IR_CommandDataGet();
			DBG("\n---IR_Cmd:%08lx\n",Cmd);
			IR_IntFlagClear();

			RepeatCount = IR_RepeatCountGet();
			DBG("\n---RepeatCount:%0x\n",RepeatCount);
			while(1)
			{
				DelayMs(45);//NEC IR repeat code is transmited every 110mS

				Last_RepeatCount = RepeatCount;

				RepeatCount = IR_RepeatCountGet();
				DBG("\n*********RepeatCount:%0x\n",RepeatCount);

				if( (Last_RepeatCount - RepeatCount) > 0 )
				{
					Cmd = IR_CommandDataGet();
					DBG("\n--------There is a New IR_Command:%08lx------\n",Cmd);
					DBG("\n*****************IR_Cmd:%08lx\n",Cmd);
					IR_IntFlagClear();
				}

				if(RepeatCount == Last_RepeatCount)
				{
					IR_CommandFlagClear();
					break;
				}
			}
		}
	}
}

void SONY_IR_InterruptExample(void)
{
	DBG("enter SONY IR---B6 Interrupt Example\n\n");

	GPIO_RegOneBitSet(GPIO_B_IE, GPIO_INDEX6);
	GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX6);

	NVIC_EnableIRQ(IR_IRQn);
	NVIC_SetPriority(IR_IRQn, 1);
	GIE_ENABLE();

	IR_Config(IR_MODE_SONY, IR_GPIOB6, IR_SONY_12BITS);
	IR_Enable();//开启IR功能
	IR_InterruptEnable();//开启IR中断
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



static void SystermIRWakeupConfig(IR_MODE_SEL ModeSel, IR_IO_SEL GpioSel, IR_CMD_LEN_SEL CMDLenSel)
{
#if 1
	if(ModeSel == IR_MODE_NEC)
	{
		DBG("enter NEC IR---B6 Wakeup Example\n\n");
	}else{
		DBG("enter SONY IR---B6 Wakeup Example\n\n");
	}
	
	IR_Config(ModeSel, GpioSel, CMDLenSel);
	IR_Enable();
	IR_IntFlagClear();
	IR_InterruptEnable();

	Clock_BTDMClkSelect(RC_CLK32_MODE);//sniff开启时使用影响蓝牙功能呢
	Reset_FunctionReset(IR_FUNC_SEPA);
	IR_WakeupEnable();


	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	GIE_ENABLE();

	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE11_IR, 0, 0);
	Power_WakeupEnable(SYSWAKEUP_SOURCE11_IR);
#endif
}


int main(void)
{
	uint32_t recvBuf = 0;

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

    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
    DbgUartInit(1, 2000000, 8, 0, 1);

	SysTickInit();

	DBG("\n");
    DBG("/-----------------------------------------------------\\\n");
    DBG("|                     IR Example                     |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");

	DBG("-------------------------------------------------------\n");
	DBG("Input '1' to enter NEC IR Recieve Cmd example\n\n");
	DBG("Input '2' to enter NEC IR Interrupt example\n\n");
	DBG("Input '3' to enter NEC IR WakeupFromDeepSleep example\n\n");
	DBG("Input '4' to enter SONY IR Recieve Cmd example\n\n");
	DBG("Input '5' to enter SONY IR Interrupt example\n\n");
	DBG("Input '6' to enter SONY IR WakeupFromDeepSleep example\n\n");
	DBG("-------------------------------------------------------\n");
	recvBuf = 0;
	recvBuf = WaitDatum1Ever();

    switch (recvBuf)
		{
    		case 1:
    		{
    			//NEC  IR RecieveCmdExample
    			NEC_IR_RecieveCmdExample();
    		}
			break;

    		case 2:
    		{
    			//NEC IR Interrupt example
    			NEC_IR_InterruptExample();
    		}
			break;

    		case 3:
    		{
    			//NEC IR WakeupFromDeepSleep example
    			SystermIRWakeupConfig(IR_MODE_NEC,IR_GPIOB6,IR_NEC_32BITS);
    		}
			break;

    		case 4:
    		{
    			//SONY IR RecieveCmdExample
    			SONY_IR_RecieveCmdExample();
    		}
			break;

    		case 5:
    		{
    			//SONY IR Interrupt example
    			SONY_IR_InterruptExample();
    		}
			break;

    		case 6:
    		{
    			// SONY IR WakeupFromDeepSleep example
    			SystermIRWakeupConfig(IR_MODE_SONY,IR_GPIOB6,IR_SONY_12BITS);
    		}
			break;
    	}

    	return 0;
}

static bool INT_Flag = FALSE;
__attribute__((section(".driver.isr"))) void IR_Interrupt(void)
{
	uint32_t Cmd = 0;

	if( IR_IntFlagGet() == TRUE )
	{
		DBG("\n**********中断方式**********\n");
		INT_Flag = FALSE;

		Cmd = IR_CommandDataGet();
		DBG("\n---IR_Cmd:%08lx\n",Cmd);
	}
	IR_IntFlagClear();
//	IR_CommandFlagClear();
}

static uint32_t sources;

__attribute__((section(".driver.isr")))void WakeupInterrupt(void)
{

	sources |= Power_WakeupSourceGet();

	DBG("wakeUp sources is: 0x%x\n",sources);

	Power_WakeupSourceClear();
}
