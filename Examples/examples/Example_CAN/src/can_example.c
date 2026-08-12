/**
 **************************************************************************************
 * @file    can_example.c
 * @brief   can example
 *
 * @author  liangzx
 * @version V1.0.0
 *
 * $Created: 2024-01-22$
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
#include "efuse.h"
#include "can_interface.h"
#include "timer.h"
#include "spi_flash.h"
#include "remap.h"
#include "irqn.h"
#include "sys.h"
#include "chip_info.h"
#include "watchdog.h"

#define MAX_RX_LENGTH  32
typedef struct _BYTES_FIFO
{
 	uint32_t		Head;
	uint32_t		Count;
	CAN_DATA_MSG	Msg[MAX_RX_LENGTH];
}BYTES_RX_FIFO;

static volatile BYTES_RX_FIFO  RxMsg;

void can_msg_printf(CAN_DATA_MSG Msg)
{
	uint8_t len,i;

	DBG("ID:%X ,%s%s\n",Msg.Id,Msg.EFF?"Extend Frame":"Standard Frame",Msg.RTR?",Remote Frame":"\0");

	len = Msg.DATALENGTH;
	if(Msg.RTR == 0)
	{
		DBG("DATA(%d): ",len);
		i = 0;
		while(len-- > 0)
		{
			DBG("%02X ",Msg.Data[i++]);
		}
		DBG("\n");
	}
}

void CanRxLoopTest(void)
{
	CAN_DATA_MSG	Msg;
	uint8_t c;

	DBG("CanRxLoopTest\n");
	DBG("  E: Exit!\n");

	while(1)
	{
		if(UART_RecvByte(UART_PORT1,(uint8_t*)&c))
		{
			if(c == 'E')
			{
				DBG("Exit CanRxLoopTest!\n");
				return;
			}
		}

		if(CAN_GetStatus() & CAN_RX_RDY)
		{
			DBG("RxMsgCnt: %d\n",CAN_GetRxMsgCnt());
			CAN_Recv(&Msg,100);
			can_msg_printf(Msg);
		}
	}
}


void CanTxLoopTest(void)
{
	CAN_DATA_MSG Msg;
	uint8_t c;

	DBG("CanTxLoopTest\n");
	DBG("  S:Send Data!\n");
	DBG("  1:Extend Frame!\n");
	DBG("  2:Standard Frame!\n");
	DBG("  3:Remote Frame!\n");
	DBG("  4:Normal frame!\n");
	DBG("  E:Exit!\n");

	Msg.Id		= 0x55;
	Msg.EFF 	= 0;
	Msg.RTR 	= 0;
	Msg.Data[0]	= 0x10;
	Msg.Data[1]	= 0x20;
	Msg.Data[2]	= 0x30;
	Msg.Data[3]	= 0x40;
	Msg.Data[4]	= 0x50;
	Msg.Data[5]	= 0x60;
	Msg.Data[6]	= 0x70;
	Msg.Data[7]	= 0x80;
	Msg.DATALENGTH = 8;

	while(1)
	{
		if(UART_RecvByte(UART_PORT1,(uint8_t*)&c))
		{
			switch(c)
			{
			case 'E':
				DBG("Exit CanTxLoopTest!\n");
				return;
			case 'S':
				Msg.Id++;
				Msg.Data[0]++;
				Msg.Data[1]++;
				Msg.Data[2]++;
				Msg.Data[3]++;
				Msg.Data[4]++;
				Msg.Data[5]++;
				Msg.Data[6]++;
				Msg.Data[7]++;
				CAN_Send(&Msg,100);
				can_msg_printf(Msg);
				break;
			case '1':
				Msg.EFF 	= 1;
				Msg.Id		= 0x55555;
				CAN_Send(&Msg,100);
				can_msg_printf(Msg);
				break;
			case '2':
				Msg.EFF 	= 0;
				Msg.Id		= 0x55;
				CAN_Send(&Msg,100);
				can_msg_printf(Msg);
				break;
			case '3':
				Msg.RTR 	= 1;
				CAN_Send(&Msg,100);
				can_msg_printf(Msg);
				break;
			case '4':
				Msg.RTR 	= 0;
				CAN_Send(&Msg,100);
				can_msg_printf(Msg);
				break;
			default:
				break;
			}
		}
	}
}


void CanTxRxLoopTest(void)
{
	CAN_DATA_MSG Msg;
	uint8_t c;

	DBG("CanTxRxLoopTest\n");
	DBG("  E:Exit!\n");

	while(1)
	{
		if(UART_RecvByte(UART_PORT1,(uint8_t*)&c))
		{
			if(c == 'E')
			{
				DBG("Exit CanRxLoopTest!\n");
				return;
			}
		}

		if(CAN_GetRxMsgCnt() > 0)
		{
			CAN_RecvISR(&Msg);
			can_msg_printf(Msg);
			CAN_Send(&Msg,100);
		}
	}
}

void CanIntLoopTest(void)
{
	CAN_DATA_MSG	Msg;
	uint8_t c;

	DBG("CanIntLoopTest\n");
	DBG("  E: Exit!\n");

	memset(&RxMsg,0,sizeof(RxMsg));

	CAN_IntTypeEnable(CAN_INT_OR_EN | CAN_INT_RX_EN);
	//CAN中断和SPDIF复用一个中断号 22
	NVIC_EnableIRQ(SPDIF_IRQn);//22
	GIE_ENABLE();

	while(1)
	{
		if(UART_RecvByte(UART_PORT1,(uint8_t*)&c))
		{
			if(c == 'E')
			{
				CAN_IntTypeDisable();
				//CAN中断和SPDIF复用一个中断号 22
				NVIC_DisableIRQ(SPDIF_IRQn);//22
				DBG("Exit CanIntLoopTest!\n");
				return;
			}
		}

		if(RxMsg.Count > 0)	//fifo is not empty
		{
			can_msg_printf(RxMsg.Msg[RxMsg.Head]);
			RxMsg.Head = (RxMsg.Head + 1) % MAX_RX_LENGTH;
			RxMsg.Count--;
		}
	}
}

//CAN中断和SPDIF复用一个中断号 22
void SPDIF0_Interrupt(void)
{
	uint8_t index;
	CAN_BIT_INTSTATUS int_flag = CAN_GetIntStatus();

	if(int_flag & CAN_INT_RX_FLAG)
	{
		DBG("CAN_INT_RX_FLAG!\n");
		if(RxMsg.Count < MAX_RX_LENGTH)
		{
			index = (RxMsg.Head + RxMsg.Count) % MAX_RX_LENGTH;
			CAN_RecvISR(&RxMsg.Msg[index]);
			RxMsg.Count++;
		}
		CAN_ClrIntStatus(CAN_INT_RX_FLAG);
	}

	if(int_flag & CAN_INT_DATA_OR)
	{
		uint8_t cnt;

		cnt = CAN_GetRxMsgCnt();
		DBG("Overrun Int! %d\n",cnt);

		while(cnt--)
		{
			if(RxMsg.Count < MAX_RX_LENGTH)
			{
				index = (RxMsg.Head + RxMsg.Count) % MAX_RX_LENGTH;
				CAN_RecvISR(&RxMsg.Msg[index]);
				RxMsg.Count++;
			}
		}
		CAN_SetModeCmd(CAN_MODE_RST_SELECT);
		CAN_SetModeCmd(CAN_MODE_RST_DISABLE);
	}

	if(int_flag & CAN_INT_WAKEUP)
	{
		DBG("CAN_INT_WAKEUP! %x\n",int_flag);
		CAN_ClrIntStatus(CAN_INT_WAKEUP);
	}

	if(int_flag & CAN_INT_TX_FLAG)
	{
		DBG("CAN_INT_TX_FLAG!\n");
		CAN_ClrIntStatus(CAN_INT_TX_FLAG);
	}

	if(int_flag & CAN_INT_BERR)
	{
		DBG("CAN_INT_BERR! %X\n",CAN_GetStatus());
		CAN_ClrIntStatus(CAN_INT_BERR);
	}

	if(int_flag & CAN_INT_ERR)
	{
		DBG("CAN_INT_ERR!\n");
		CAN_ClrIntStatus(CAN_INT_ERR);
	}

	if(int_flag & CAN_INT_ERR_PASSIVE)
	{
		DBG("CAN_INT_ERR_PASSIVE!\n");
		CAN_ClrIntStatus(CAN_INT_ERR_PASSIVE);
	}

	if(int_flag & CAN_INT_ARB_LOST)
	{
		DBG("CAN_INT_ARB_LOST!\n");
		CAN_ClrIntStatus(CAN_INT_ARB_LOST);
	}
}

int main(void)
{
	uint32_t Key=0;

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
    DBG("|                     CAN Example                     |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");

	DBG("0 ---> 循环接收测试\n");
	DBG("1 ---> 中断接收测试\n");
	DBG("2 ---> 发送测试\n");
	DBG("3 ---> 转发测试\n");

	CAN_ModuleInit(RATE_500KBPS,CAN_PORT_A3_A4);

	while(1)
	{
		if(UART_RecvByte(UART_PORT1,(uint8_t*)&Key))
		{
			switch(Key-'0')
			{
				case 0:
					CanRxLoopTest();
					break;
				case 1:
					CanIntLoopTest();
					break;
				case 2:
					CanTxLoopTest();
					break;
				case 3:
					CanTxRxLoopTest();
					break;
				default:
					break;
			}
		}
	}
}
