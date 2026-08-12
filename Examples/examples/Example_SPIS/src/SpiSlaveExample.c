/**
 **************************************************************************************
 * @file    SpiSlaveExample.c
 * @brief   Spi Slave
 * 
 * @author  grayson
 * @version V1.0.0 	initial release
 * 
 * $Id$
 * $Created: 2017-10-29 17:31:10$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <nds32_intrinsic.h>
#include "gpio.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "type.h"
#include "debug.h"
#include "spi_flash.h"
#include "timeout.h"
#include "clk.h"
#include "pwm.h"
#include "delay.h"
#include "rtc.h"
#include "spis.h"
#include "watchdog.h"
#include "irqn.h"
#include "spi_flash.h"
#include "remap.h"
#include "chip_info.h"

extern void SysTickInit(void);

#define LIMIT(Val,lmt)   (((Val) > (lmt)) ? (lmt):(Val) )

#define MAX_BUF_LEN   4096

uint8_t spisPort = SPIS_PORT1_A20A21A22A23;
uint8_t spisMode = 0;

uint8_t SpisSendBuf[MAX_BUF_LEN];
uint8_t SpisRecvBuf[MAX_BUF_LEN];



uint8_t DmaChannelMapSPIS[28] =
{
	PERIPHERAL_ID_SPIS_TX,
	PERIPHERAL_ID_SPIS_RX,
	255,
	255,
	255,
	255,
};


static int32_t Wait4Datum4Ever(void)//从串口等待接收一个字节
{
	uint8_t Data;
	do{
		if(0 < UARTS_Recv(UART_PORT1,&Data, 1,10))
        {
			break;
		}
	}while(1);
	return Data - '0';
}


const char* spisIO[][4] =
{
	//    cs      miso     clk      mosi
		{"A1",    "A0",    "B5",     "B4"},
		{"A23",   "A22",   "A21",    "A20"},
};

void SpisSetting(void)
{
	DBG("\nSPIS init setting...\n");

	SPIS_IoConfig(spisPort);
	SPIS_Init(spisMode);
	SPIS_BlockDMA_Init();
	DBG("spis mode: %d\n",spisMode);//
	DBG("spis_cs  : %s\n",spisIO[spisPort][0]);
	DBG("spis_miso: %s\n",spisIO[spisPort][1]);
	DBG("spis_clk : %s\n",spisIO[spisPort][2]);
	DBG("spis_mosi: %s\n",spisIO[spisPort][3]);
}




int main(void)
{
    uint8_t Temp = 0;
	uint32_t i;

    Chip_Init(1);
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

    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
    DbgUartInit(1, 2000000, 8, 0, 1);

	DBG("\n");
    DBG("/-----------------------------------------------------\\\n");
    DBG("|                    SPIS Example                     |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");

	for(i = 0; i< MAX_BUF_LEN;i++)
	{
		SpisSendBuf[i]=(uint8_t)i;
	}
	DMA_ChannelAllocTableSet(DmaChannelMapSPIS);
	SpisSetting();//spis配置
	while(1)
	{
		DBG("\n=============================================\n");
		DBG("\n SPIS Example...\n");
		DBG(" 1. SPIS TX by MCU\n");
		DBG(" 2. SPIS RX by MCU\n");
		DBG(" 3. SPIS TX by DMA\n");
		DBG(" 4. SPIS RX by DMA\n");
		DBG(" 5. SPIS TX&RX by DMA\n");
		DBG("\n=============================================\n");
		Temp = Wait4Datum4Ever();//注意UART端口号
		switch(Temp)
		{
			case 1:
				DBG("TX MCU\n");
				 SPIS_ClrTxFIFO();
				SPIS_Send(SpisSendBuf,4096,4000);
				DBG("send ok\n");
				break;
			case 2:
				DBG("RX MCU\n");
				 SPIS_ClrRxFIFO();
				SPIS_Recv(SpisRecvBuf,4096,4000);
				if(memcmp(SpisSendBuf,SpisRecvBuf,4096)==0)
				{
					DBG("recv ok\n");
				}
				for(i = 0;i < LIMIT(MAX_BUF_LEN,128);i++)//最多只打印128个值
				{
					DBG("[%02x]  ", SpisRecvBuf[i]);
				}
				break;
				break;
			case 3:
				DBG("TX DMA\n");
				 SPIS_ClrTxFIFO();
				SPIS_DMA_StartSend(SpisSendBuf,4096);
				while(!SPIS_DMA_SendState());
				DBG("send ok\n");
				break;

			case 4:
				DBG("RX DMA\n");
				 SPIS_ClrRxFIFO();
				SPIS_DMA_StartRecv(SpisRecvBuf,4096);
				while(!SPIS_DMA_RecvState());
				if(memcmp(SpisSendBuf,SpisRecvBuf,4096)==0)
				{
					DBG("recv ok\n");
				}
				for(i = 0;i < LIMIT(MAX_BUF_LEN,128);i++)//最多只打印128个值
				{
					DBG("[%02x]  ", SpisRecvBuf[i]);
				}
				break;
			case 5:
				DBG("TX&RX DMA\n");
				 SPIS_ClrRxFIFO();
				 SPIS_ClrTxFIFO();
				SPIS_DMA_StartSendRecv(SpisSendBuf,SpisRecvBuf,4096);
				while(!SPIS_DMA_RecvState());
				if(memcmp(SpisSendBuf,SpisRecvBuf,4096)==0)
				{
					DBG("recv ok\n");
				}
				for(i = 0;i < LIMIT(MAX_BUF_LEN,128);i++)//最多只打印128个值
				{
					DBG("[%02x]  ", SpisRecvBuf[i]);
				}
				break;
			default :break;
		}
	}
}

