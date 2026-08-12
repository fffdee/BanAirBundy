/**
 **************************************************************************************
 * @file    gpio_example.c
 * @brief   gpio example
 *
 * @author  Taowen
 * @version V1.0.0
 *
 * $Created: 2019-05-31 19:17:00$
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
#include "delay.h"
#include "chip_info.h"

//注意使用的UART端口
#define UART_TX_IO	1
#define UART_RX_IO  1

static uint8_t DmaChannelMap[6] = {
	PERIPHERAL_ID_UART1_TX,
	PERIPHERAL_ID_TIMER5,
	PERIPHERAL_ID_SOFTWARE,
	255,
	255,
	255
};

void OutputExample(void)
{
	uint8_t					recvBuffer;

	DBG("\nOutputExample\n");
	DBG("Input 'x' to exit this example\n");
	DBG("Input 'h' to output high \n");
	DBG("Input 'l' to output low \n");
	DBG("Input '0' to A2 output Driving Strength Select 4mA\n");
	DBG("Input '1' to A2 output Driving Strength Select 8mA\n");
	DBG("Input '2' to A2 output Driving Strength Select 12mA\n");
	DBG("Input '3' to A2 output Driving Strength Select 24mA\n");


	while(1)
	{
		recvBuffer = 0;
		UARTS_Recv(UART_RX_IO, &recvBuffer, 1,	10);

		if(recvBuffer == 'x')
		{
			break;
		}
		else if(recvBuffer == 'h')
		{
			DBG("A2 output High\n");
			GPIO_RegOneBitClear(GPIO_A_IE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OUT, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OUT, GPIO_INDEX2);
		}
		else if(recvBuffer == 'l')
		{
			DBG("A2  first output high then output Low\n");
			GPIO_RegOneBitClear(GPIO_A_IE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OUT, GPIO_INDEX2);
			GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX2);
		}
		else if(recvBuffer == '0')
		{
			DBG("A2 has outds control;Driving Strength Select 4mA\n");
			GPIO_RegOneBitClear(GPIO_A_IE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OUT, GPIO_INDEX2);
			GPIO_PortAOutDsSet(GPIO_INDEX2, GPIO_OUTDS_4MA);
		}
		else if(recvBuffer == '1')
		{
			//建议使用8mA驱动力
			DBG("A2 has outds control;Driving Strength Select 8mA\n");
			GPIO_RegOneBitClear(GPIO_A_IE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OUT, GPIO_INDEX2);
			GPIO_PortAOutDsSet(GPIO_INDEX2, GPIO_OUTDS_8MA);
		}
		else if(recvBuffer == '2')
		{
			DBG("A2 has outds control;Driving Strength Select 12mA\n");
			GPIO_RegOneBitClear(GPIO_A_IE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OUT, GPIO_INDEX2);
			GPIO_PortAOutDsSet(GPIO_INDEX2, GPIO_OUTDS_12MA);
		}
		else if(recvBuffer == '3')
		{
			DBG("A2 has outds control;Driving Strength Select 24mA\n");
			GPIO_RegOneBitClear(GPIO_A_IE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_OUT, GPIO_INDEX2);
			GPIO_PortAOutDsSet(GPIO_INDEX2, GPIO_OUTDS_24MA);
		}
	}
}

void InputExample(void)
{
	uint8_t					recvBuffer;
	uint8_t					newVal = 0;

	DBG("\nInputExample\n");
	DBG("Input 'x' to exit this example\n");
	DBG("Input 'h' to output A2 high and input A3 high \n");
	DBG("Input 'l' to output A2 low and input A3 low \n");
	// A3 是 Input 脚
	GPIO_RegOneBitSet(GPIO_A_IE, GPIO_INDEX3);

	GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX3);

	// A2 是 Output 脚
	GPIO_RegOneBitClear(GPIO_A_IE, GPIO_INDEX2);

	GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX2);

	while(1)
	{
		recvBuffer = 0;
		UARTS_Recv(UART_RX_IO, &recvBuffer, 1,	1000);

		if(recvBuffer == 'x')
		{
			break;
		}
		else if(recvBuffer == 'h')
		{
			DBG("A2 High\n");
			GPIO_RegOneBitSet(GPIO_A_OUT, GPIO_INDEX2);
			break;
		}
		else if(recvBuffer == 'l')
		{
			DBG("A2 Low\n");
			GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX2);
			break;
		}

	}

	while(1)
	{
		DelayMs(200);
		newVal = GPIO_RegOneBitGet(GPIO_A_IN, GPIO_INDEX3);
		if(newVal == 1)
		{
			DBG("A3 Get value is High\n");
		}
		else
		{
			DBG("A3 Get value is Low\n");
		}
	}
}

void InterruptExample(void)
{
	uint8_t				recvBuffer;
	uint8_t				triggerType = GPIO_NEG_EDGE_TRIGGER;

	DBG("\nInterruptExample\n");
	DBG("Input 'x' to exit this example\n");
	DBG("Input 'p' to posedge trigger A4  \n");
	DBG("Input 'n' to negedge trigger A4  \n");
	DBG("Input 'e' to edge trigger A4  \n");

	NVIC_EnableIRQ(Gpio_IRQn);

	while(1)
	{
		recvBuffer = 0;
		UARTS_Recv(UART_RX_IO, &recvBuffer, 1,	10);

		if(recvBuffer == 'x')
		{
			NVIC_DisableIRQ(Gpio_IRQn);
			break;
		}
		else if(recvBuffer == 'p')
		{
			// posedge trigger
			DBG("\nposedge trigger ,  must connect the GPIO A4 with the GND \n");
			triggerType = GPIO_POS_EDGE_TRIGGER;
			// A4 为中断检测脚
			GPIO_RegOneBitSet(GPIO_A_IE, GPIO_INDEX4);
			GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX4);
			GPIO_RegOneBitSet(GPIO_A_PD, GPIO_INDEX4);
			GPIO_RegOneBitClear(GPIO_A_PU, GPIO_INDEX4);

			GPIO_INTEnable(GPIO_A_INT, GPIO_INDEX4, triggerType);
		}
		else if(recvBuffer == 'n')
		{
			//negedge trigger
			DBG("\nnegedge trigger ,  must connect the GPIO A4 with the 3V3\n");
			triggerType = GPIO_NEG_EDGE_TRIGGER;
			// A4 为中断检测脚
			GPIO_RegOneBitSet(GPIO_A_IE, GPIO_INDEX4);
			GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX4);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX4);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX4);

			GPIO_INTEnable(GPIO_A_INT, GPIO_INDEX4, triggerType);
		}
		else if(recvBuffer == 'e')
		{
			//edge trigger
			DBG("\edge trigger ,  must connect the GPIO A4 with the 3V3 \n");
			triggerType = GPIO_EDGE_TRIGGER;
			// A4 为中断检测脚
			GPIO_RegOneBitSet(GPIO_A_IE, GPIO_INDEX4);
			GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX4);
			GPIO_RegOneBitSet(GPIO_A_PD, GPIO_INDEX4);
			GPIO_RegOneBitClear(GPIO_A_PU, GPIO_INDEX4);

			GPIO_INTEnable(GPIO_A_INT, GPIO_INDEX4, triggerType);
		}
	}
}

void PullUpDownExample(void)
{
	uint8_t					recvBuffer;

	DBG("\nPullUpDownExample\n");
	DBG("Input 'x' to exit this example\n");
	DBG("Input '1' to A3 Pull up 20uA\n");
	DBG("Input '2' to A3 Pull down 20uA\n");
	DBG("Input '3' to A2 Pull up 20uA\n");
	DBG("Input '4' to A2 Pull down 20uA\n");

	// A3 是 Input 脚
	GPIO_RegBitsSet(GPIO_A_IE, GPIO_INDEX3);
	GPIO_RegBitsClear(GPIO_A_OE, GPIO_INDEX3);
	GPIO_RegBitsSet(GPIO_A_IN, GPIO_INDEX3);
	GPIO_RegBitsClear(GPIO_A_OUT, GPIO_INDEX3);

	// A2 是 Output 脚
	GPIO_RegBitsClear(GPIO_A_IE, GPIO_INDEX2);
	GPIO_RegBitsSet(GPIO_A_OE, GPIO_INDEX2);
	GPIO_RegBitsClear(GPIO_A_IN, GPIO_INDEX2);
	GPIO_RegBitsSet(GPIO_A_OUT, GPIO_INDEX2);

	while(1)
	{
		recvBuffer = 0;
		UARTS_Recv(UART_RX_IO, &recvBuffer, 1,	10);

		if(recvBuffer == 'x')
		{
			break;
		}
		else if(recvBuffer == '1')
		{
			DBG("A3  Pull up 20uA\n");
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX3);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX3);
		}
		else if(recvBuffer == '2')
		{
			DBG("A3 Pull down 20uA\n");
			GPIO_RegOneBitClear(GPIO_A_PU, GPIO_INDEX3);
			GPIO_RegOneBitSet(GPIO_A_PD, GPIO_INDEX3);
		}
		else if(recvBuffer == '3')
		{
			DBG("A2 Pull up 20uA\n");
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX2);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX2);
		}
		else if(recvBuffer == '4')
		{
			DBG("A2 Pull down 20uA\n");
			GPIO_RegOneBitClear(GPIO_A_PU, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_PD, GPIO_INDEX2);
		}
	}
}

void PullDownCurrentExample(void)
{
	uint8_t					recvBuffer;

	DBG("\nPullDownCurrentExample\n");
	DBG("Input 'x' to exit this example\n");
	DBG("Input '1' to A5 Pull down current 0mA\n");
	DBG("Input '2' to A5 Pull down current 1.6mA\n");
	DBG("Input '3' to A5 Pull down current 3.2mA\n");
	DBG("Input '4' to A5 Pull down current 4.8mA\n");

	while(1)
	{
		recvBuffer = 0;
		UARTS_Recv(UART_RX_IO, &recvBuffer, 1,	10);

		if(recvBuffer == 'x')
		{
			break;
		}
		else if(recvBuffer == '1')
		{
			DBG("A5 Pull down current 0mA\n");
			GPIO_RegOneBitSet(GPIO_A_IE, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX5);
			GPIO_RegOneBitSet(GPIO_A_IN, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_PU, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX5);
			GPIO_PortAPulldownSet(GPIOA5, GPIO_PULLDOWN_CLOSE);

		}
		else if(recvBuffer == '2')
		{
			DBG("A5 Pull down current 1.6mA\n");
			GPIO_RegOneBitSet(GPIO_A_IE, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX5);
			GPIO_RegOneBitSet(GPIO_A_IN, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_PU, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX5);
			GPIO_PortAPulldownSet(GPIOA5, GPIO_PULLDOWN_1MA6);

		}
		else if(recvBuffer == '3')
		{
			DBG("A5 Pull down current 3.2mA\n");
			GPIO_RegOneBitSet(GPIO_A_IE, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX5);
			GPIO_RegOneBitSet(GPIO_A_IN, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_PU, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX5);
			GPIO_PortAPulldownSet(GPIOA5, GPIO_PULLDOWN_3MA2);

		}
		else if(recvBuffer == '4')
		{
			DBG("A5 Pull down current 4.8mA\n");
			GPIO_RegOneBitSet(GPIO_A_IE, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX5);
			GPIO_RegOneBitSet(GPIO_A_IN, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_PU, GPIO_INDEX5);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX5);
			GPIO_PortAPulldownSet(GPIOA5, GPIO_PULLDOWN_4MA8);

		}
	}
}

void RegisterResetMaskExample(void)
{
	uint8_t num;
	uint32_t RegGpioAOut_val = 0;

	RegGpioAOut_val = GPIO_RegGet(GPIO_A_OUT);
	DBG("REG_GPIO_A_REG_OUT: %08lx\n",RegGpioAOut_val);
	DBG("\nInput any of char (a~z) to go on\n");
	while(1)
	{
		UARTS_Recv(UART_RX_IO, &num, 1,	1000);
		if((num >= 'a')&&(num <= 'z'))
		{
			break;
		}
	}

	GPIO_RegisterResetMask();

	GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX14);
	GPIO_RegOneBitClear(GPIO_A_IE, GPIO_INDEX14);
	GPIO_RegOneBitSet(GPIO_A_OUT, GPIO_INDEX14);

	RegGpioAOut_val = GPIO_RegGet(GPIO_A_OUT);
	DBG("REG_GPIO_A_REG_OUT: %08lx\n",RegGpioAOut_val);

	Reset_McuSystem();
}

#define REG_GPIO_A_TOG  (*(volatile unsigned *)0x40010010)
void GpioTogglebyMcuExample(void)
{
	uint32_t GpioToggleTestPin = 0;

	DBG("\nGpioTogglebyMcuExample\n");
	DBG("A1 Toggle by Mcu\n");

	GpioToggleTestPin = GPIO_INDEX1;
 	GPIO_RegBitsClear(GPIO_A_IE, GpioToggleTestPin);
 	GPIO_RegBitsSet(GPIO_A_OE, GpioToggleTestPin);

   while(1)
	{
	   /////////调用API,最大18MHz左右///////
		   GPIO_RegSet(GPIO_A_TGL, GpioToggleTestPin);
		   GPIO_RegSet(GPIO_A_TGL, GpioToggleTestPin);
		   GPIO_RegSet(GPIO_A_TGL, GpioToggleTestPin);
		   GPIO_RegSet(GPIO_A_TGL, GpioToggleTestPin);
		   GPIO_RegSet(GPIO_A_TGL, GpioToggleTestPin);

		   GPIO_RegSet(GPIO_A_TGL, GpioToggleTestPin);
		   GPIO_RegSet(GPIO_A_TGL, GpioToggleTestPin);
		   GPIO_RegSet(GPIO_A_TGL, GpioToggleTestPin);
		   GPIO_RegSet(GPIO_A_TGL, GpioToggleTestPin);
		   GPIO_RegSet(GPIO_A_TGL, GpioToggleTestPin);
	}

//   while(1)
//	{
//	   /////////直接操作寄存器,最大72MHz左右///////
//	   REG_GPIO_A_TOG = GpioToggleTestPin;
//	   REG_GPIO_A_TOG = GpioToggleTestPin;
//	   REG_GPIO_A_TOG = GpioToggleTestPin;
//	   REG_GPIO_A_TOG = GpioToggleTestPin;
//	   REG_GPIO_A_TOG = GpioToggleTestPin;
//
//	   REG_GPIO_A_TOG = GpioToggleTestPin;
//	   REG_GPIO_A_TOG = GpioToggleTestPin;
//	   REG_GPIO_A_TOG = GpioToggleTestPin;
//	   REG_GPIO_A_TOG = GpioToggleTestPin;
//	   REG_GPIO_A_TOG = GpioToggleTestPin;
//	}
}

#define BUFFER_LEN    1024
#define     ADR_GPIO_A_REG_O                                           (0x40040004)
#define     ADR_GPIO_B_REG_O                                           (0x40040034)
#define     REG_TIMER5_CTRL                (*(volatile unsigned long *) 0x4002C000)
void GpioTogglebyDmaExample(void)
{
	static uint32_t  DmaTestBuffer[BUFFER_LEN];
	//Timer5触发DMA Request， 将memory数据toggle到A0~A31
	//可以从示波器上A0~A31上看到方波，周期为TIMER5定时的2倍
		uint32_t i;
		DMA_CONFIG DMAConfig;
		uint32_t DmaTestLen;

		DBG("\nGpioTogglebyDmaExample\n");
		DBG("GPIO PortA bit[0xffff7917] Toggle by Dma\n");

		DMA_ChannelAllocTableSet(DmaChannelMap);

		for(i = 0; i < BUFFER_LEN ; i+=2)
		{
			DmaTestBuffer[i]   = 0x55555555;
			DmaTestBuffer[i+1] = 0xaaaaaaaa;
		}

		GPIO_RegBitsClear(GPIO_A_IE, 0xffff7917);
		GPIO_RegBitsSet(GPIO_A_OE, 0xffff7917);
		GPIO_OutMaskSet(GPIO_A_OUT_MASK, GPIO_OUTMASK_DMA, GPIO_INDEX3|GPIO_INDEX5|GPIO_INDEX6|GPIO_INDEX7|GPIO_INDEX9|GPIO_INDEX10);//DMA MASK
		GPIO_OutMaskSet(GPIO_A_OUT_MASK, GPIO_OUTMASK_DMA, GPIO_INDEX30|GPIO_INDEX31);//DMA MASK

		DmaTestLen = BUFFER_LEN*4;

		DMAConfig.Dir = DMA_CHANNEL_DIR_MEM2PERI;
		DMAConfig.Mode = DMA_CIRCULAR_MODE;
		DMAConfig.SrcAddress = (uint32_t)DmaTestBuffer;
		DMAConfig.DataWidth = DMA_DWIDTH_WORD;
		DMAConfig.SrcAddrIncremental = DMA_SRC_AINCR_SRC_WIDTH;
		DMAConfig.DstAddress = ADR_GPIO_A_REG_O;
		DMAConfig.DstAddrIncremental = DMA_DST_AINCR_NO;
		DMAConfig.BufferLen = DmaTestLen;
		DMA_TimerConfig(PERIPHERAL_ID_TIMER5, &DMAConfig);

		DMA_CircularWritePtrSet(PERIPHERAL_ID_TIMER5, DmaTestLen+16);//将写指针指向buffer外
		DMA_ChannelEnable(PERIPHERAL_ID_TIMER5);

		/////////////此时GPIO翻转频率可达2*500kHz
		Timer_Config(TIMER5, 1, 0);
		Timer_Start(TIMER5);
		REG_TIMER5_CTRL |= (1<<8);
		DMA_ChannelEnable(PERIPHERAL_ID_TIMER5);
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
    DBG("|                     GPIO Example                     |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");

    RESTART:
    	DBG("-------------------------------------------------------\n");
    	DBG("GPIO Example:\n");
    	DBG("o:  Output Example\n");
    	DBG("i:  Input Example\n");
    	DBG("t:  Interrupt Example\n");
    	DBG("p:  Pull up , Pull down  Example\n");
    	DBG("d:  Pull-Down-current  Example\n");
    	DBG("3:  Gpio Register Reset Mask  Example\n");
    	DBG("4:  Gpio Toggle by MCU  Example\n");
    	DBG("5:  Gpio Toggle by DMA  Example\n");
    	DBG("-------------------------------------------------------\n");

    	while(1)
    	{
    		recvBuf = 0;
    		UARTS_Recv(UART_RX_IO, (uint8_t *)&recvBuf, 1,	1000);

    		if(recvBuf == 'o')
    		{
    			// output example
    			OutputExample();
    			goto RESTART;
    		}
    		else if(recvBuf == 'i')
    		{
    			// input example
    			InputExample();
    			goto RESTART;
    		}
    		else if(recvBuf == 't')
    		{
    			// interrupt example
    			InterruptExample();
    			goto RESTART;
    		}
    		else if(recvBuf == 'p')
    		{
    			// pull up / pu down example
    			PullUpDownExample();
    			goto RESTART;
    		}
    		else if(recvBuf == 'd')
    		{
    			// pull-Down-current example
    			PullDownCurrentExample();
    			goto RESTART;
    		}
    		else if(recvBuf == '3')
    		{
    			// Gpio Register Reset Mask  Example
    			RegisterResetMaskExample();
    			goto RESTART;
    		}
    		else if(recvBuf == '4')
    		{
    			// Gpio Toggle by MCU  Example
    			GpioTogglebyMcuExample();
    			goto RESTART;
    		}
    		else if(recvBuf == '5')
    		{
    			// Gpio Toggle by DMA  Example
    			DMA_ChannelAllocTableSet(DmaChannelMap);
    			GpioTogglebyDmaExample();
    			goto RESTART;
    		}
    	}

    	return 0;
}

uint32_t gpioPinIndex[2] = {0,0};
__attribute__((section(".driver.isr"))) void GpioInterrupt(void)
{

	DBG("---Enter GpioInterrupt---\n");
	gpioPinIndex[0] = GPIO_INTFlagGet(GPIO_A_INT);
	if(gpioPinIndex[0])
	{
		DBG("GpioA Int Index:0x%08lx\n",gpioPinIndex[0]);
		GPIO_INTFlagClear(GPIO_A_INT,gpioPinIndex[0]);
	}

	gpioPinIndex[1] = GPIO_INTFlagGet(GPIO_B_INT);
	if(gpioPinIndex[1])
	{
		DBG("GpioB Int Index:0x%02lx\n",gpioPinIndex[1]);
		GPIO_INTFlagClear(GPIO_B_INT,gpioPinIndex[1]);
	}

}
