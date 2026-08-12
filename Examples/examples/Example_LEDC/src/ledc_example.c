/**
 **************************************************************************************
 * @file    i2s_example.c
 * @brief   i2s example
 *
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * $Created: 2019-05-28 11:30:00$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
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
#include "watchdog.h"
#include "remap.h"
#include "spi_flash.h"
#include "chip_info.h"
#include "irqn.h"
#include "pwm.h"
#include "pwc.h"
#include "ledc.h"
#include "ledc_test.h"

#define     REG_GPIO_A_REG_IE              (*(volatile unsigned long *) 0x40040014)
#define     REG_GPIO_A_REG_OE              (*(volatile unsigned long *) 0x40040018)
#define     REG_GPIO_A_REG_O               (*(volatile unsigned long *) 0x40040004)
#define     ADR_LEDC_OUT                                               (0x400400EC)

static uint8_t DmaChannelMap[6] =
{
	PERIPHERAL_ID_TIMER6,//stream 0 channel
	PERIPHERAL_ID_TIMER7,//stream 1 channel
	255,//stream 2 channel
	255,//stream 3 channel
	255,//stream 4 channel
	255,//stream 5 channel
};
uint8_t DmaLineLen = 16*8;
uint16_t DmaOneLen = 8*4;
uint8_t DmaLineBuf[16*8]={
		0X00,0X00,0X00,0X00,0X00,0X00,0X00,0x01,
		0X01,0X01,0X01,0X01,0X01,0X01,0X01,0x02,
		0X02,0X02,0X02,0X02,0X02,0X02,0X02,0x03,
		0X03,0X03,0X03,0X03,0X03,0X03,0X03,0x04,
		0X04,0X04,0X04,0X04,0X04,0X04,0X04,0x05,
		0X05,0X05,0X05,0X05,0X05,0X05,0X05,0x06,
		0X06,0X06,0X06,0X06,0X06,0X06,0X06,0x07,
		0X07,0X07,0X07,0X07,0X07,0X07,0X07,0X08,
		0X08,0X08,0X08,0X08,0X08,0X08,0X08,0X09,
		0X09,0X09,0X09,0X09,0X09,0X09,0X09,0X0A,
		0X0A,0X0A,0X0A,0X0A,0X0A,0X0A,0X0A,0X0B,
		0X0B,0X0B,0X0B,0X0B,0X0B,0X0B,0X0B,0X0C,
		0X0C,0X0C,0X0C,0X0C,0X0C,0X0C,0X0C,0X0D,
		0X0D,0X0D,0X0D,0X0D,0X0D,0X0D,0X0D,0X0E,
		0X0E,0X0E,0X0E,0X0E,0X0E,0X0E,0X0E,0X0F,
		0X0F,0X0F,0X0F,0X0F,0X0F,0X0F,0X0F,0X00
	};
//uint32_t DmaOneBuf[8]={LEDC_OEN_CNT/128,LEDC_OEN_CNT/64,LEDC_OEN_CNT/32,LEDC_OEN_CNT/16,LEDC_OEN_CNT/8,LEDC_OEN_CNT/4,LEDC_OEN_CNT/2,LEDC_OEN_CNT};
uint32_t DmaOneBuf[8]={LEDOUT_DATAOUT_LONG,LEDOUT_DATAOUT_LONG/128,LEDOUT_DATAOUT_LONG/64,LEDOUT_DATAOUT_LONG/32,LEDOUT_DATAOUT_LONG/16,LEDOUT_DATAOUT_LONG/8,LEDOUT_DATAOUT_LONG/4,LEDOUT_DATAOUT_LONG/2};
uint8_t LED_BUFA[LED_BUF_SIZE];
uint8_t LED_BUFB[LED_BUF_SIZE];
static void LedcDmaDoneInterrupt(void)//3
{
	static uint8_t FRAME_COUNT=1;
	static uint8_t LINE_COUNT=0;
//	static uint8_t OEN_COUNT=0;

	GPIO_RegOneBitSet(GPIO_B_OUT,GPIO_INDEX4);//SET STB

//	Timer_Config(TIMER3, 1, 1);
//	Timer_TimerClkFreqUpdate(TIMER3,0,DmaOneBuf[OEN_COUNT++]);
	GPIO_RegOneBitClear(GPIO_B_OUT,GPIO_INDEX4);//clear STB
//	#if(LEDC_OEN_POLARITY)
//		GPIO_RegOneBitSet(GPIO_B_OUT,GPIO_INDEX5);
//	#else
//		GPIO_RegOneBitClear(GPIO_B_OUT,GPIO_INDEX5);
//	#endif
//	if(OEN_COUNT>=8)
//	{
//		OEN_COUNT=0;
//	}
	if(LINE_COUNT==LED_LINE_NUM)
	{
	  LINE_COUNT=0;
	}

//	Timer_Start(TIMER3);
	//发送新的length/2数据
	DMA_BlockBufSet(PERIPHERAL_ID_TIMER6, LED_BUFA+(LED_SIZE_WIDTH*LED_SIZE_HIGH/2)*FRAME_COUNT+LINE_COUNT*(LED_SIZE_WIDTH), LEDOUT_DATAOUT_LONG);
//	DMA_BlockBufSet(PERIPHERAL_ID_TIMER4, LED_BUFA+LINE_COUNT*LEDOUT_DATAOUT_LONG, LEDOUT_DATAOUT_LONG);
	DMA_InterruptFlagClear(PERIPHERAL_ID_TIMER6, DMA_DONE_INT);
	FRAME_COUNT++;
	if(FRAME_COUNT==8)
	{
		FRAME_COUNT=0;
		LINE_COUNT++;
	}
//	GPIO_RegOneBitClear(GPIO_B_OUT,GPIO_INDEX4);//clear STB
	DMA_ChannelEnable(PERIPHERAL_ID_TIMER6);
}
void PwcDma_Init(void)
{
	PWC_StructInit  PWCParam1,PWCParam2;
	TIMER_INDEX TimerIdx = TIMER7;
	DMA_CONFIG DMAConfig1;
	//配置第一个PWMC以及DMA
	DMAConfig1.Dir = DMA_CHANNEL_DIR_MEM2PERI;		//配置为内存传输到外设模式
	DMAConfig1.Mode = DMA_CIRCULAR_MODE;			//DMA设置为循环传输模式
	DMAConfig1.SrcAddress = DmaLineBuf;	//设置源数据缓存地址
	DMAConfig1.DataWidth = DMA_DWIDTH_BYTE;	//设置源数据的传输宽度
	DMAConfig1.SrcAddrIncremental = DMA_SRC_AINCR_SRC_WIDTH;//源数据地址累增宽度
	DMAConfig1.DstAddress = 0x40040004+2;//GPIOA16-A19      //目标数据缓存地址
//	DMAConfig1.DstDataWidth = DMA_DST_DWIDTH_BYTE;			//目标数据读入数据宽度
	DMAConfig1.DstAddrIncremental = DMA_DST_AINCR_NO;		//目标缓存区地址累加宽度，此处设置为地址不增加，即每次来的数据都放于此地址，即覆盖掉
	DMAConfig1.BufferLen = DmaLineLen;						//数据传输的总长度
	DMA_TimerConfig(PERIPHERAL_ID_TIMER7, &DMAConfig1);		//DMA触发源，此为定时器7为触发源

	DMA_CircularWritePtrSet(PERIPHERAL_ID_TIMER7,DmaLineLen+4);//将写指针指向buffer外
	DMA_ChannelEnable(PERIPHERAL_ID_TIMER7);

	GPIO_RegOneBitClear(GPIO_A_OE,GPIO_INDEX28);
	GPIO_RegOneBitSet(GPIO_A_IE,GPIO_INDEX28);
	//PWC复用GPIO配置
	PWC_GpioConfig(TimerIdx,28);  //配置TIMER_PWC1的IO口，此处为GPIOC4

	memset(&PWCParam1,0,sizeof(PWCParam1));
	//PWC参数配置
	PWCParam1.Polarity  = PWC_POLARITY_RAISING;//设置极性为上升沿触发
	PWCParam1.SingleGet       = 0;					//设置为连续触发模式
	PWCParam1.DMAReqEnable    = 1;					//打开中断触发DMA请求
	PWCParam1.TimeScale       = 100;				//设置PWC测量的量程
	PWCParam1.FilterTime      = 0;					//设置滤波时间

	PWC_Config(TimerIdx, &PWCParam1);
	PWC_Enable(TimerIdx);//开始采样

}
void rgb_to_buf(uint8_t *outbuf,uint8_t *inbuf,uint32_t image_width,uint32_t image_height,uint32_t led_width,uint32_t led_height)
{
	uint32_t i,j;
	uint8_t temp=0,b=1;
	uint32_t filter;
	int32_t oencount=0;
	for(b=0;b<8;b++)
	{
		filter = 0x01 << b;
		for(i=0;i<16;i++)
		{
			oencount=DmaOneBuf[b];
			for(j=0;j<image_width;j++)
			{
				if(image_height == led_height)
				{
					temp=((inbuf[i*image_width*3+j*3]&filter)>>b) | (((inbuf[i*image_width*3+j*3+1]&filter)>>b)<<1) | (((inbuf[i*image_width*3+j*3+2]&filter)>>b)<<2)
						 | (((inbuf[(i+16)*image_width*3+j*3]&filter)>>b)<<3) | (((inbuf[(i+16)*image_width*3+j*3+1]&filter)>>b)<<4) | (((inbuf[(i+16)*image_width*3+j*3+2]&filter)>>b)<<5);
#if LEDC_OEN_POLARITY
					if((oencount--)>0)
						outbuf[i*led_width+(b)*led_width*led_height/2+j]=temp|(1<<6);
					else
						outbuf[i*led_width+(b)*led_width*led_height/2+j]=temp;
#else
					if((oencount--)>0)
						outbuf[i*led_width+(b)*led_width*led_height/2+j]=temp;
					else
						outbuf[i*led_width+(b)*led_width*led_height/2+j]=temp|(1<<6);
#endif
					temp = 0;
				}
			}
#if LEDC_OEN_POLARITY
			outbuf[i*led_width+(b)*led_width*led_height/2+j-1] &=(~(1<<6));
#else
			outbuf[i*led_width+(b)*led_width*led_height/2+j-1] |=(1<<6);
#endif
		}

	}
}
//__attribute__((section(".driver.isr"))) void Timer3Interrupt(void)
//{
//	if(Timer_InterruptFlagGet(TIMER3,UPDATE_INTERRUPT_SRC))
//	{
//		Timer_InterruptFlagClear(TIMER3,UPDATE_INTERRUPT_SRC);
//#if(LEDC_OEN_POLARITY)
//		GPIO_RegOneBitClear(GPIO_B_OUT,GPIO_INDEX5);
//#else
//		GPIO_RegOneBitSet(GPIO_B_OUT,GPIO_INDEX5);
//#endif
//	}
//}
int main(void)
{
	DMA_CONFIG DMAConfig;

    Chip_Init(1);
    WDG_Disable();
    Clock_Config(1, 24000000);
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

//    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
//    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
//    DbgUartInit(1, 2000000, 8, 0, 1);

    SysTickInit();
	GIE_ENABLE();

	DMA_ChannelAllocTableSet(DmaChannelMap);

	LEDC_DataIOConfig();
	LEDC_CLKIOConfig();
//	GPIO_RegOneBitClear(GPIO_A_IE,GPIO_INDEX24);//clk
//	GPIO_RegOneBitSet(GPIO_A_OE,GPIO_INDEX24);
//	GPIO_RegOneBitClear(GPIO_A_OUT,GPIO_INDEX24);
	GPIO_RegOneBitClear(GPIO_A_IE,GPIO_INDEX16);//A
	GPIO_RegOneBitSet(GPIO_A_OE,GPIO_INDEX16);
	GPIO_RegOneBitClear(GPIO_A_OUT,GPIO_INDEX16);
	GPIO_RegOneBitClear(GPIO_A_IE,GPIO_INDEX17);//B
	GPIO_RegOneBitSet(GPIO_A_OE,GPIO_INDEX17);
	GPIO_RegOneBitClear(GPIO_A_OUT,GPIO_INDEX17);
	GPIO_RegOneBitClear(GPIO_A_IE,GPIO_INDEX18);//C
	GPIO_RegOneBitSet(GPIO_A_OE,GPIO_INDEX18);
	GPIO_RegOneBitClear(GPIO_A_OUT,GPIO_INDEX18);
	GPIO_RegOneBitClear(GPIO_A_IE,GPIO_INDEX19);//D
	GPIO_RegOneBitSet(GPIO_A_OE,GPIO_INDEX19);
	GPIO_RegOneBitClear(GPIO_A_OUT,GPIO_INDEX19);
	GPIO_RegOneBitClear(GPIO_B_IE,GPIO_INDEX4);//STB
	GPIO_RegOneBitSet(GPIO_B_OE,GPIO_INDEX4);
	GPIO_RegOneBitClear(GPIO_B_OUT,GPIO_INDEX4);
//	GPIO_RegOneBitClear(GPIO_B_IE,GPIO_INDEX5);//OEN
//	GPIO_RegOneBitSet(GPIO_B_OE,GPIO_INDEX5);
//#if (LEDC_OEN_POLARITY)
//	GPIO_RegOneBitClear(GPIO_B_OUT,GPIO_INDEX5);
//#else
//	GPIO_RegOneBitSet(GPIO_B_OUT,GPIO_INDEX5);
//#endif
	PwcDma_Init();

//	REG_GPIO_A_REG_IE &= ~(0xFF<<3);
//	REG_GPIO_A_REG_OE |=  (0xFF<<3);
//	REG_GPIO_A_REG_O &= ~(0xFF<<3);

	rgb_to_buf(LED_BUFA,gImage_rgb,64,32,64,32);

	DMAConfig.Dir = DMA_CHANNEL_DIR_MEM2PERI;
	DMAConfig.Mode = DMA_BLOCK_MODE;
	DMAConfig.SrcAddress = LED_BUFA;
	DMAConfig.DataWidth = DMA_DWIDTH_BYTE;
	DMAConfig.SrcAddrIncremental = DMA_SRC_AINCR_SRC_WIDTH;//DMA_SRC_AINCR_NO;

	DMAConfig.DstAddress = ADR_LEDC_OUT;
	DMAConfig.DstAddrIncremental = DMA_DST_AINCR_NO;//DMA_DST_AINCR_DST_WIDTH;//2测试间隔为不跟随目的宽度
	DMAConfig.BufferLen = LEDOUT_DATAOUT_LONG;
	DMA_TimerConfig(PERIPHERAL_ID_TIMER6, &DMAConfig);
	DMA_BlockBufSet(PERIPHERAL_ID_TIMER6, LED_BUFA, LEDOUT_DATAOUT_LONG);
	DMA_InterruptFunSet(PERIPHERAL_ID_TIMER6, DMA_DONE_INT, LedcDmaDoneInterrupt);
	DMA_InterruptEnable(PERIPHERAL_ID_TIMER6, DMA_DONE_INT, 1);

	Timer_Config(TIMER6, 1, 0);
	Timer_TimerClkFreqUpdate(TIMER6,0,Clock_SysClockFreqGet()/LEDC_CLK_TIME);

	LEDC_CtrlInit(TIMER6,0,0,1);

//	NVIC_EnableIRQ(Timer3_IRQn);
//	Timer_InterruptFlagClear(TIMER3, UPDATE_INTERRUPT_SRC);
//	Timer_Config(TIMER3, 1, 1);

	DMA_ChannelEnable(PERIPHERAL_ID_TIMER6);
	Timer_Start(TIMER6);
	while(1);

}
