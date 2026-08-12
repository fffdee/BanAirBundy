/**
 **************************************************************************************
 * @file    SpiMasterExample.c
 * @brief   Spi master
 * 
 * @author  messi
 * @version V1.0.0 	initial release
 * 
 * $Id$
 * $Created: 2019-5-20 17:31:10$
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
#include "irqn.h"
#include "delay.h"
#include "rtc.h"
#include "spim.h"
#include "watchdog.h"
#include "spi_flash.h"
#include "sys.h"
#include "remap.h"
#include "chip_info.h"
#include "spim_interface.h"

#define LIMIT(Val,lmt)   (((Val) > (lmt)) ? (lmt):(Val) )


#define  MAX_BUF_LEN   4096

uint8_t spimPort = SPIM_PORT1_A20_A21_A22_A28;
uint8_t spimRate  = SPIM_CLK_DIV_6M;
uint8_t spimMode = 0;
uint8_t SpimBuf_TX[MAX_BUF_LEN];
uint8_t SpimBuf_RX[MAX_BUF_LEN];

//SPIM CS pin有以下两种使用方式，任选其一
/********************************方式1-任意指定IO*******************************/
//#define SPIM_CS_INIT()         GPIO_RegOneBitClear(GPIO_A_IE, GPIOA9);\
//		                       GPIO_RegOneBitSet(GPIO_A_OE, GPIOA9);\
//		                       GPIO_RegOneBitSet(GPIO_A_OUT, GPIOA9);
//
//#define SPIM_CS_ENABLE()     GPIO_RegOneBitClear(GPIO_A_OUT, GPIOA9);
//#define SPIM_CS_DISABLE()    GPIO_RegOneBitSet(GPIO_A_OUT, GPIOA9);

/********************************方式2-专用CS pin*******************************/
#define SPIM_CS_INIT()		SPIM_CS_Init(spimPort);
#define SPIM_CS_ENABLE()	SPIM_CS_Enable();
#define SPIM_CS_DISABLE()	SPIM_CS_Disable();

static uint8_t DmaChannelMap_SPIM[] =
{
	PERIPHERAL_ID_SPIM_TX,
	PERIPHERAL_ID_SPIM_RX,
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

const char* spimIO[][4] =
{
	//    cs      miso     clk      mosi
		{"A9",    "A7",    "A6",     "A5"},
		{"A28",    "A22",   "A21",    "A20"},
};


#define     REG_SPIM_CLK_DIV_NUM           (*(volatile unsigned long *) 0x40021014)
void SpimSetting(void)
{
	DBG("\nSPIM init setting...\n");
	SPIM_IoConfig(spimPort);

	SPIM_CS_INIT();

	if(SPIM_Init(spimMode,spimRate))
	{
		DBG("SPI init success!\n");
		DBG("spim mode:%d\n",spimMode);//
		DBG("spim rate:%d\n",spimRate);//
		DBG("spim_cs  :%s\n",spimIO[spimPort][0]);
		DBG("spim_miso:%s\n",spimIO[spimPort][1]);
		DBG("spim_clk :%s\n",spimIO[spimPort][2]);
		DBG("spim_mosi:%s\n",spimIO[spimPort][3]);
	}
	else
	{
		DBG("****** Err: SPI init fail ******\n");
	}
}
void SpimDataSendByMcu()
{
	uint32_t i;

	DBG("write by mcu\n");

	memset(SpimBuf_TX, 0, MAX_BUF_LEN);

	for(i = 0; i< MAX_BUF_LEN;i++)
	{
		SpimBuf_TX[i]=(uint8_t)i;
	}
	SPIM_CS_ENABLE();
	SPIM_Send(SpimBuf_TX, MAX_BUF_LEN);
	SPIM_CS_DISABLE();
	WaitMs(10);

	DBG("Data Send:\n");
	for(i = 0;i < LIMIT(MAX_BUF_LEN,128);i++)//最多只打印128个值
	{
		DBG("[%02x]  ", SpimBuf_TX[i]);
	}
	DBG("\n\n");
}
void SpimDataRevByMcu()
{
	uint32_t i;

	DBG("read by mcu\n");

	memset(SpimBuf_RX, 0, MAX_BUF_LEN);

	SPIM_CS_ENABLE();
	SPIM_Recv(SpimBuf_RX,MAX_BUF_LEN);
	SPIM_CS_DISABLE();


	DBG("Data Rev:\n");
	for(i = 0;i < LIMIT(MAX_BUF_LEN,128);i++)//最多只打印128个值
	{
		DBG("[%02x]  ", SpimBuf_RX[i]);
	}
	DBG("\n\n");
}

void SpimDataSendByDMA()
{
	uint32_t i;

	DBG("write by dma\n");
	DMA_ChannelAllocTableSet(DmaChannelMap_SPIM);

	memset(SpimBuf_TX, 0, MAX_BUF_LEN);

	for(i = 0; i< MAX_BUF_LEN;i++)
	{
		SpimBuf_TX[i]=(uint8_t)i;
	}
	SPIM_CS_ENABLE();
	SPIM_DMA_Send_Start(SpimBuf_TX, MAX_BUF_LEN);
	while(!SPIM_DMA_HalfDone(PERIPHERAL_ID_SPIM_TX));
	SPIM_CS_DISABLE();
	WaitMs(10);

	DBG("Data Send:\n");
	for(i = 0;i < LIMIT(MAX_BUF_LEN,128);i++)//最多只打印128个值
	{
		DBG("[%02x]  ", SpimBuf_TX[i]);
	}
	DBG("\n\n");
}

void SpimDataRevByDMA()
{
	uint32_t i;

	DBG("read by dma\n");

	DMA_ChannelAllocTableSet(DmaChannelMap_SPIM);
	memset(SpimBuf_RX, 0, MAX_BUF_LEN);

	SPIM_CS_ENABLE();
	SPIM_DMA_Recv_Start(SpimBuf_RX,MAX_BUF_LEN);
	while(!SPIM_DMA_HalfDone(PERIPHERAL_ID_SPIM_RX));
	SPIM_CS_DISABLE();

	DBG("Data Rev:\n");
	for(i = 0;i < LIMIT(MAX_BUF_LEN,128);i++)//最多只打印128个值
	{
		DBG("[%02x]  ", SpimBuf_RX[i]);
	}
	DBG("\n\n");
}
void SpimDataTxAndRxTest(void)//需要SPIS程序配合
{
	uint32_t i;

	DBG("TX & RX Test\n");

//	while(1)
	{
		for(i = 0; i< MAX_BUF_LEN;i++)
		{
			SpimBuf_TX[i]=(uint8_t)i;
		}
		memset(SpimBuf_RX, 0, MAX_BUF_LEN);
		SPIM_CS_ENABLE();
		SPIM_DMA_Send_Recive_Start(SpimBuf_TX,SpimBuf_RX,MAX_BUF_LEN);
		while(!SPIM_DMA_FullDone());
		SPIM_CS_DISABLE();

		DBG("Data Send:\n");
		for(i = 0;i < LIMIT(MAX_BUF_LEN,128);i++)//最多只打印128个值
		{
			DBG("[%02x]  ", SpimBuf_RX[i]);
		}
		DBG("\n\n");
		WaitMs(10);
	}
}

int main(void)
{
    uint8_t Temp = 0;

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
    DBG("|                    SPIM Example                     |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");
    DMA_ChannelAllocTableSet(DmaChannelMap_SPIM);//初始化DMA通道列表
	SpimSetting();//spim
	while(1)
	{
		DBG("\n=============================================\n");
		DBG("\n SPIM Example...\n");
		DBG("1. SPIM Send by MCU\n");
		DBG("2. SPIM Receive by MCU\n");
		DBG("3. SPIM Send by DMA\n");
		DBG("4. SPIM Receive by DMA\n");
		DBG("5. SPIM TX&RX Test\n");
		DBG("\n=============================================\n");
		Temp = Wait4Datum4Ever();//注意UART端口号
		switch(Temp)
		{
			case 1:
				DBG("TX MCU\n");
				SpimDataSendByMcu();
				break;

			case 2:
				DBG("RX MCU\n");
				SpimDataRevByMcu();
				break;

			case 3:
				DBG("TX DMA\n");
				SpimDataSendByDMA();
				break;

			case 4:
				DBG("RX DMA\n");
				SpimDataRevByDMA();
				break;

			case 5:
				DBG("TX&RX DMA\n");
				SpimDataTxAndRxTest();
				break;
		}
	}

	DBG("END\n");

}

