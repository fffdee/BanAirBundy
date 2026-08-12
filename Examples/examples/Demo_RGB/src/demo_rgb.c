/**
 **************************************************************************************
 * @file    demo_rgb.c
 * @brief   demo rgb
 *
 * @author  Peter
 * @version V1.0.0
 *
 * $Created: 2019-05-22 19:17:00$
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

static uint8_t DmaChannelMap[6] = {
		PERIPHERAL_ID_TIMER4,
		255,
		255,
		255,
		255,
		255,
};

static uint8_t DmaTestBuf[512];
uint32_t  DmaTestLen;


//#define		Dma_Test
#define		BUFFER_LEN    420 + 980//
#define     ADR_GPIO_A_REG_O                                           (0x40040004)
#define     REG_TIMER3_CTRL                (*(volatile unsigned long *) 0x40035000)
#define     REG_TIMER4_CTRL                (*(volatile unsigned long *) 0x40035800)
#define     REG_TIMER4_NUM                (*(volatile unsigned long *) 0x4003580C)
#ifdef Dma_Test
static uint32_t  DmaTestBuffer[BUFFER_LEN];
#else
static uint32_t  DmaTestBuffer[BUFFER_LEN] =
{
	GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,

	GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,

	GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,

	GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,

	GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,
	GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,GPIOA22,0,0,


	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

};
#endif
static void DmaTimerCircleMode(void)
{
	//Timer3触发DMA Request， 将memory数据toggle到A0~A31
	//可以从示波器上A0~A31上看到方波，周期为TIMER3定时的2倍
	uint32_t i;
	DMA_CONFIG DMAConfig;
	uint32_t DmaTestLen;
	DMA_ERROR ret;
	DBG("DmaTimerCircleMode\n");

	DMA_ChannelAllocTableSet(DmaChannelMap);

#ifdef Dma_Test
	for(i = 0; i < BUFFER_LEN ; i+=4)
	{
//		DmaTestBuffer[i]   = 0x55555555;
//		DmaTestBuffer[i+1] = 0xaaaaaaaa;
		DmaTestBuffer[i]   = 0;
		DmaTestBuffer[i+1] = GPIOA22;
		DmaTestBuffer[i+2] = GPIOA22;
		DmaTestBuffer[i+3] = GPIOA22;
	}
#endif

//	GPIO_RegBitsClear(GPIO_A_IE, 0xffffffff);
//	GPIO_RegBitsSet(GPIO_A_OE, 0xffffffff);
	GPIO_RegBitsClear(GPIO_A_IE, GPIOA22);
	GPIO_RegBitsSet(GPIO_A_OE, GPIOA22);
	//避免cpu写IO和DMA冲突引发闪烁
	GPIO_RegBitsSet(GPIO_A_DMA_OUT_MASK, ~(GPIOA22));
	//GPIO_RegBitsSet(GPIO_A_CORE_OUT_MASK, GPIOA22 | GPIOA2);

	DmaTestLen = BUFFER_LEN*4;

	DMAConfig.Dir = DMA_CHANNEL_DIR_MEM2PERI;
	DMAConfig.Mode = DMA_CIRCULAR_MODE;
	DMAConfig.SrcAddress = (uint32_t)DmaTestBuffer;
	//DMAConfig.SrcDataWidth = DMA_SRC_DWIDTH_WORD;
//	DMAConfig.SrcDataWidth = DMA_SRC_DWIDTH_BYTE;
	DMAConfig.SrcAddrIncremental = DMA_SRC_AINCR_SRC_WIDTH;
	DMAConfig.DstAddress = ADR_GPIO_A_REG_O;
	DMAConfig.DataWidth = DMA_DWIDTH_WORD;
//	DMAConfig.DstDataWidth = DMA_SRC_DWIDTH_BYTE;
	DMAConfig.DstAddrIncremental = DMA_DST_AINCR_NO;
	DMAConfig.BufferLen = DmaTestLen;
	ret = DMA_TimerConfig(PERIPHERAL_ID_TIMER4, &DMAConfig);
	DBG("ret:%d\n",ret);
	DMA_CircularWritePtrSet(PERIPHERAL_ID_TIMER4, DmaTestLen+16);//将写指针指向buffer外
	DMA_ChannelEnable(PERIPHERAL_ID_TIMER4);

	//Timer_Config(TIMER3, 1, 0);
	REG_TIMER4_NUM = 41;
	Timer_Start(TIMER4);
	REG_TIMER4_CTRL |= (1<<8);
	while(1);
}

static void DmaTimerBlockMode(void)
{
	//Timer3触发DMA Request， 将memory数据toggle到A0~A31
	//可以从示波器上A0~A31上看到方波，周期为TIMER3定时的2倍
	uint32_t i;
	DMA_CONFIG DMAConfig;

	DBG("DmaTimerBlockMode\n");

//	for(i = 0; i < BUFFER_LEN ; i+=2)
//	{
//		DmaTestBuffer[i]   = 0x55555555;
//		DmaTestBuffer[i + 1]   = 0xaaaaaaaa;
//	}

//	GPIO_RegBitsClear(GPIO_A_IE, 0xffffffff);
//	GPIO_RegBitsSet(GPIO_A_OE, 0xffffffff);
	GPIO_RegOneBitClear(GPIO_A_IE, GPIOA22);
	GPIO_RegOneBitSet(GPIO_A_OE, GPIOA22);
	//避免cpu写IO和DMA冲突引发闪烁
	GPIO_RegBitsSet(GPIO_A_DMA_OUT_MASK, ~(GPIOA22));
	GPIO_RegBitsSet(GPIO_A_CORE_OUT_MASK, GPIOA22);
	DmaTestLen = BUFFER_LEN;

	DMAConfig.Dir = DMA_CHANNEL_DIR_MEM2PERI;
	DMAConfig.Mode = DMA_BLOCK_MODE;
	DMAConfig.SrcAddress = (uint32_t)DmaTestBuffer;
	//DMAConfig.SrcDataWidth = DMA_SRC_DWIDTH_WORD;
	DMAConfig.SrcAddrIncremental = DMA_SRC_AINCR_SRC_WIDTH;
	DMAConfig.DstAddress = ADR_GPIO_A_REG_O;
	DMAConfig.DataWidth = DMA_DWIDTH_WORD;
	DMAConfig.DstAddrIncremental = DMA_DST_AINCR_NO;
	DMAConfig.BufferLen = DmaTestLen;
	DMA_TimerConfig(PERIPHERAL_ID_TIMER4, &DMAConfig);

//	Timer_Config(TIMER4, 1, 0);
	REG_TIMER4_NUM = 41;
	REG_TIMER4_CTRL |= (1<<8);

	DMA_BlockBufSet(PERIPHERAL_ID_TIMER4, DmaTestBuffer, DmaTestLen);
	DMA_ChannelEnable(PERIPHERAL_ID_TIMER4);
	Timer_Start(TIMER4);

	while(1)
	{
//		while(!DMA_InterruptFlagGet(PERIPHERAL_ID_TIMER4, DMA_DONE_INT));
//		DMA_InterruptFlagClear(PERIPHERAL_ID_TIMER4, DMA_DONE_INT);
//
//		DMA_BlockBufSet(PERIPHERAL_ID_TIMER4, DmaTestBuffer, DmaTestLen);
//		DMA_ChannelEnable(PERIPHERAL_ID_TIMER4);
		;
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

    // BP15系列开发板启用串口，默认使用
    GPIO_PortAModeSet(GPIOA9, 1);  // Rx, A9:uart1_rxd_1
    GPIO_PortAModeSet(GPIOA10, 5); // Tx, A10:uart1_txd_1
    DbgUartInit(1, 2000000, 8, 0, 1);

	SysTickInit();

	SpiFlashInit(80000000, MODE_4BIT, 0, 1);

	DBG("\n");
    DBG("/-----------------------------------------------------\\\n");
    DBG("|                    DMA RGB Example                     |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");
	DMA_ChannelAllocTableSet(DmaChannelMap);

	DmaTimerCircleMode();
	//DmaTimerBlockMode();
	while(1);
}
