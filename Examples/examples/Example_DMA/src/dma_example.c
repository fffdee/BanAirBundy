/**
 **************************************************************************************
 * @file    dma_example.c
 * @brief   dma example
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
	PERIPHERAL_ID_UART0_TX,
	PERIPHERAL_ID_UART0_RX,
	PERIPHERAL_ID_TIMER3,
	PERIPHERAL_ID_SOFTWARE,
	255,
	255
};

static uint8_t DmaTestBuf[512];
uint32_t  DmaTestLen;

static void DmaBlockMem2Mem(void)              //0
{
	//BLOCK MEM2MEM
	uint32_t i, n, error_flag = 0;
	uint32_t DmaTestSrcAddr,DmaTestDstAddr;
	DBG("---------DMA BLOCK MEM2MEM TEST-----------\n");
	DBG("先赋值00~FF到源内存块，再使用DMA将源内存内容传输到目的内存块\n");
	DmaTestSrcAddr = 0x20008000;
	DmaTestDstAddr = 0x20018000;
	n = 65532;
	//n = 1024 * 62 - 4;
	DBG("按照BYTE单位增加\n");
	for(i = 0; i < n; i++)
	{
		((uint8_t *)DmaTestSrcAddr)[i] = (uint8_t)i;
	}

	for(i = 0; i < n; i++)
	{
		((uint8_t *)DmaTestDstAddr)[i] = (uint8_t)0;
	}

	DMA_MemCopy(DmaTestSrcAddr, DmaTestDstAddr, n, DMA_DWIDTH_BYTE, DMA_SRC_AINCR_SRC_WIDTH, DMA_DST_AINCR_DST_WIDTH);
	while(!DMA_InterruptFlagGet(PERIPHERAL_ID_SOFTWARE, DMA_DONE_INT));

	DMA_InterruptFlagClear(PERIPHERAL_ID_SOFTWARE, DMA_DONE_INT);
	for(i = 0; i < n; i++)
	{
		if(((uint8_t *)DmaTestSrcAddr)[i] != ((uint8_t *)DmaTestDstAddr)[i])
		{
			DBG("Error Happend %d != %d\n", ((uint8_t *)DmaTestSrcAddr)[i], ((uint8_t *)DmaTestDstAddr)[i]);
			error_flag = 1;
		}
	}
	DBG("按照WORD单位增加\n");

	for(i = 0; i < n; i++)
	{
		((uint8_t *)DmaTestDstAddr)[i] = (uint8_t)0;
	}
	DMA_MemCopy(DmaTestSrcAddr, DmaTestDstAddr, n, DMA_DWIDTH_WORD, DMA_SRC_AINCR_SRC_WIDTH, DMA_DST_AINCR_DST_WIDTH);
	while(!DMA_InterruptFlagGet(PERIPHERAL_ID_SOFTWARE, DMA_DONE_INT));
	DMA_InterruptFlagClear(PERIPHERAL_ID_SOFTWARE, DMA_DONE_INT);
	for(i = 0; i < n; i++)
	{
		if(((uint8_t *)DmaTestSrcAddr)[i] != ((uint8_t *)DmaTestDstAddr)[i])
		{
			DBG("Error Happend %d != %d\n", ((uint8_t *)DmaTestSrcAddr)[i], ((uint8_t *)DmaTestDstAddr)[i]);
			error_flag = 1;
		}
	}

	if(!error_flag)
	{
		DBG("success! no error happen!\n");
	}
	DBG("test end\n");
	while(1);
}

static void DmaBlockMem2Peri(void)
{
	uint32_t n, i,cnt;
	DBG("---------DMA Block模式下 Memory到外设示例，使用查询模式-----------\n");
	DBG("Uart0 Tx配成了DMA Block模式\n");
	DBG("uart0在dma模式下循环向外发送字符\n");

	n = 10;
	cnt =0;

	GPIO_PortAModeSet(GPIOA0, 1);
	GPIO_PortAModeSet(GPIOA1, 5);
	UARTS_Init(0, 115200, 8,  0,  1);
	for(i=0;i<n;i++)
	{
		DmaTestBuf[i] = i+'0';
	}

	DMA_BlockConfig(PERIPHERAL_ID_UART0_TX);
	UART_IOCtl(UART_PORT0,UART_IOCTL_DMA_TX_EN, 1);

	DMA_BlockBufSet(PERIPHERAL_ID_UART0_TX, DmaTestBuf, n);
	DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);

	while(1)
	{
		while(!DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_TX, DMA_DONE_INT));
		DMA_InterruptFlagClear(PERIPHERAL_ID_UART0_TX, DMA_DONE_INT);

		cnt++;
		for(i=0;i<n;i++)
		{
			DBG("cnt = %d, i = %d, recive = %c\n",(int)cnt, (int)i, DmaTestBuf[i]);
		}

		DMA_BlockBufSet(PERIPHERAL_ID_UART0_TX, DmaTestBuf, n);
		DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);
	}
}

void InterruptBlockPeri2Mem(void)
{
	uint32_t n=10;
	uint32_t i;
	DBG("Enter UART0 DMA接收中断\n");
	DBG("UART0 DMA收到的数据为：\n");

	DMA_InterruptFlagClear(PERIPHERAL_ID_UART0_RX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_UART0_RX, DMA_THRESHOLD_INT);

	for(i=0;i<n;i++)
	{
		DBG("i = %d,recive = %c\n",(int)i,DmaTestBuf[i]);
	}

	memset(DmaTestBuf, 0, n+1);

	DMA_BlockBufSet(PERIPHERAL_ID_UART0_RX, DmaTestBuf, n);
	DMA_ChannelEnable(PERIPHERAL_ID_UART0_RX);
}

static void DmaBlockPeri2Mem(void)
{
	uint32_t n;
	DBG("---------DMA BLOCK模式下外设到Memory，使用中断模式-----------\n");
	DBG("Uart0 Rx配制成了DMA Block模式\n");

	n = 10;

	GPIO_PortAModeSet(GPIOA0, 1);
	GPIO_PortAModeSet(GPIOA1, 5);
	UARTS_Init(0, 115200, 8,  0,  1);

	DMA_BlockConfig(PERIPHERAL_ID_UART0_RX);
	UART_IOCtl(UART_PORT0,UART_IOCTL_DMA_RX_EN, 1);

	GIE_ENABLE();
	DMA_InterruptFunSet(PERIPHERAL_ID_UART0_RX, DMA_DONE_INT, InterruptBlockPeri2Mem);
	DMA_InterruptEnable(PERIPHERAL_ID_UART0_RX, DMA_DONE_INT, 1);

	DMA_BlockBufSet(PERIPHERAL_ID_UART0_RX, DmaTestBuf, n);
	DMA_ChannelEnable(PERIPHERAL_ID_UART0_RX);

	while(1);
}

static void InterruptCirclePeri2Mem(void)
{
	uint32_t buf[10];
	uint8_t i;
	static uint8_t cbt_n = 0;

	DBG("---------DMA CIRCULAR模式下外设到Memory，使用中断模式-----------\n");
	DBG("DMA收到的数据为：\n");

	memset(buf, 0, 20);
	DMA_CircularDataGet(PERIPHERAL_ID_UART0_RX, DmaTestBuf, DMA_CircularThresholdLenGet(PERIPHERAL_ID_UART0_RX));

	for(i=0;i<DmaTestLen;i++)
	{
		DBG("i = %d,recive = %c\n",i,DmaTestBuf[i]);
	}
	cbt_n++;
	if(cbt_n == 10)
	{
		cbt_n = 1;
	}
	DBG("\n水位设置为:%d\n",cbt_n);
	DMA_CircularThresholdLenSet(PERIPHERAL_ID_UART0_RX, cbt_n);
	DMA_InterruptFlagClear(PERIPHERAL_ID_UART0_RX, DMA_THRESHOLD_INT);
}

static void DmaCirclePeri2Mem(void)
{
	DBG("---------DMA CIRCULAR模式下外设到Memory-----------\n");
	DBG("将uart0 rx配置成dma circular模式，1/2水位，从uart0输入n/2个字节后会从uart0打印出来\n");
	DmaTestLen = 10;

	GPIO_PortAModeSet(GPIOA0, 1);
	GPIO_PortAModeSet(GPIOA1, 5);
	UARTS_Init(0, 115200, 8,  0,  1);

	GIE_ENABLE();
	NVIC_EnableIRQ(UART0_IRQn);
	UART_IOCtl(UART_PORT0,UART_IOCTL_DMA_RX_EN, 1);

	DMA_CircularConfig(PERIPHERAL_ID_UART0_RX, DmaTestLen/2, DmaTestBuf, DmaTestLen);
	DMA_InterruptFunSet(PERIPHERAL_ID_UART0_RX, DMA_THRESHOLD_INT, InterruptCirclePeri2Mem);
	DMA_InterruptEnable(PERIPHERAL_ID_UART0_RX, DMA_THRESHOLD_INT, 1);
	DMA_ChannelEnable(PERIPHERAL_ID_UART0_RX);

	while(1);
}

static void DmaCircleMem2Peri(void)
{
	uint8_t i;

	DBG("---------DMA CIRCULAR MEM2PERI TEST-----------\n");
	DBG("将uart1 tx配制成DMA Circular模式，1/2水位，测试开始后将不断从uart1打印00~FF\n");
	DmaTestLen = 10;

	for(i=0;i<DmaTestLen;i++)
	{
		DmaTestBuf[i] = i+'0';
	}
	GPIO_PortAModeSet(GPIOA0, 1);
	GPIO_PortAModeSet(GPIOA1, 5);
	UARTS_Init(0, 115200, 8,  0,  1);

	UART_IOCtl(UART_PORT0,UART_IOCTL_DMA_TX_EN, 1);
	DMA_CircularConfig(PERIPHERAL_ID_UART0_TX, DmaTestLen/2, DmaTestBuf, DmaTestLen);

	DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);
	DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, DmaTestBuf, DmaTestLen);
	while(1)
	{
		while(!DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_TX, DMA_THRESHOLD_INT));

		DMA_InterruptFlagClear(PERIPHERAL_ID_UART0_TX, DMA_THRESHOLD_INT);
		for(i=0;i<DmaTestLen;i++)
		{
			DBG("i = %d, recive = %c\n",i, DmaTestBuf[i]);
		}

		DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, DmaTestBuf, DmaTestLen);
		DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);
	}
}

#define BUFFER_LEN    1024
#define     ADR_GPIO_A_REG_O                                           (0x40040004)
#define     REG_TIMER3_CTRL                (*(volatile unsigned long *) 0x40035000)
static uint32_t  DmaTestBuffer[BUFFER_LEN];
static void DmaTimerCircleMode(void)
{
	//Timer3触发DMA Request， 将memory数据toggle到A0~A31
	//可以从示波器上A0~A31上看到方波，周期为TIMER3定时的2倍
	uint32_t i;
	DMA_CONFIG DMAConfig;
	uint32_t DmaTestLen;

	DBG("DmaTimerCircleMode\n");

	DMA_ChannelAllocTableSet(DmaChannelMap);

	for(i = 0; i < BUFFER_LEN ; i+=2)
	{
		DmaTestBuffer[i]   = 0x55555555;
		DmaTestBuffer[i+1] = 0xaaaaaaaa;
	}

	GPIO_RegBitsClear(GPIO_A_IE, 0xffffffff);
	GPIO_RegBitsSet(GPIO_A_OE, 0xffffffff);

	DmaTestLen = BUFFER_LEN;

	DMAConfig.Dir = DMA_CHANNEL_DIR_MEM2PERI;
	DMAConfig.Mode = DMA_CIRCULAR_MODE;
	DMAConfig.SrcAddress = (uint32_t)DmaTestBuffer;
	DMAConfig.DataWidth = DMA_DWIDTH_WORD;
	DMAConfig.SrcAddrIncremental = DMA_SRC_AINCR_SRC_WIDTH;
	DMAConfig.DstAddress = ADR_GPIO_A_REG_O;
	DMAConfig.DstAddrIncremental = DMA_DST_AINCR_NO;
	DMAConfig.BufferLen = DmaTestLen;
	DMA_TimerConfig(PERIPHERAL_ID_TIMER3, &DMAConfig);

	DMA_CircularWritePtrSet(PERIPHERAL_ID_TIMER3, DmaTestLen+16);//将写指针指向buffer外
	DMA_ChannelEnable(PERIPHERAL_ID_TIMER3);

	Timer_Config(TIMER3, 1, 0);
	Timer_Start(TIMER3);
	REG_TIMER3_CTRL |= (1<<8);
	while(1);
}

static void DmaTimerBlockMode(void)
{
	//Timer3触发DMA Request， 将memory数据toggle到A0~A31
	//可以从示波器上A0~A31上看到方波，周期为TIMER3定时的2倍
	uint32_t i;
	DMA_CONFIG DMAConfig;

	DBG("DmaTimerBlockMode\n");

	for(i = 0; i < BUFFER_LEN ; i+=2)
	{
		DmaTestBuffer[i]   = 0x55555555;
		DmaTestBuffer[i + 1]   = 0xaaaaaaaa;
	}

	GPIO_RegBitsClear(GPIO_A_IE, 0xffffffff);
	GPIO_RegBitsSet(GPIO_A_OE, 0xffffffff);
	DmaTestLen = BUFFER_LEN;

	DMAConfig.Dir = DMA_CHANNEL_DIR_MEM2PERI;
	DMAConfig.Mode = DMA_BLOCK_MODE;
	DMAConfig.SrcAddress = (uint32_t)DmaTestBuffer;
	DMAConfig.DataWidth = DMA_DWIDTH_WORD;
	DMAConfig.SrcAddrIncremental = DMA_SRC_AINCR_SRC_WIDTH;
	DMAConfig.DstAddress = ADR_GPIO_A_REG_O;
	DMAConfig.DstAddrIncremental = DMA_DST_AINCR_NO;
	DMAConfig.BufferLen = DmaTestLen;
	DMA_TimerConfig(PERIPHERAL_ID_TIMER3, &DMAConfig);

	Timer_Config(TIMER3, 1, 0);
	REG_TIMER3_CTRL |= (1<<8);

	DMA_BlockBufSet(PERIPHERAL_ID_TIMER3, DmaTestBuffer, DmaTestLen);
	DMA_ChannelEnable(PERIPHERAL_ID_TIMER3);
	Timer_Start(TIMER3);

	while(1)
	{
		while(!DMA_InterruptFlagGet(PERIPHERAL_ID_TIMER3, DMA_DONE_INT));
		DMA_InterruptFlagClear(PERIPHERAL_ID_TIMER3, DMA_DONE_INT);

		DMA_BlockBufSet(PERIPHERAL_ID_TIMER3, DmaTestBuffer, DmaTestLen);
		DMA_ChannelEnable(PERIPHERAL_ID_TIMER3);
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
    DBG("|                     DMA Example                     |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");
	DMA_ChannelAllocTableSet(DmaChannelMap);

	DBG("0 ---> DmaBlockMem2Mem\n");
	DBG("1 ---> DmaBlockMem2Peri\n");
	DBG("2 ---> DmaBlockPeri2Mem\n");
	DBG("3 ---> DmaCirclePeri2Mem\n");
	DBG("4 ---> DmaCircleMem2Peri\n");
	DBG("5 ---> DmaTimerCircleMode\n");
	DBG("6 ---> DmaTimerBlockMode\n");

	while(1)
	{
		if(UART_RecvByte(UART_PORT1,(uint8_t*)&Key))
		{
			switch(Key-'0')
			{
				case 0:
					//Dma block memory to memory
					DmaBlockMem2Mem();
					break;
				case 1:
					// Dma block memory to peripheral and peripheral to memory
					DmaBlockMem2Peri();
					break;
				case 2:
					DmaBlockPeri2Mem();
					break;
				case 3:
					// Dma circular peripheral to memory
					DmaCirclePeri2Mem();
					break;
				case 4:
					// Dma circular memory to peripheral
					DmaCircleMem2Peri();
					break;
				case 5:
					// Timer trigger DMA toggle GPIO
					DmaTimerCircleMode();
					break;
				case 6:
					// Timer trigger DMA toggle GPIO
					DmaTimerBlockMode();
					break;
				default:
					break;
			}
		}
	}
	while(1);
}
