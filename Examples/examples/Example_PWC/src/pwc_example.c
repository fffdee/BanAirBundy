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
static uint8_t DmaChannelMap[29] = {
		PERIPHERAL_ID_TIMER5,
		PERIPHERAL_ID_AUDIO_DAC0_TX,
		PERIPHERAL_ID_I2S0_RX,
		PERIPHERAL_ID_I2S0_TX,
		PERIPHERAL_ID_I2S1_RX,
		PERIPHERAL_ID_I2S1_TX,
};
static char GetChar(void)
{
    uint8_t     ch = 0;
    uint8_t     ret=0;
    while(ret == 0)
    {
    	if(UART_Recv(0,&ch, 1,1000) > 0)
        {
            ret=1;
        }
    }
    return ch;
}

__attribute__((section(".driver.isr"))) void Timer7Interrupt(void)
{
	uint32_t PWCValue;
	uint32_t ret;

	ret = Timer_InterruptFlagGet(TIMER7,PWC1_CAP_DATA_INTERRUPT_SRC|\
										PWC_OVER_RUN_INTERRUPT_SRC  | UPDATE_INTERRUPT_SRC);

	if(ret & PWC1_CAP_DATA_INTERRUPT_SRC)
	{
		PWCValue = PWC_IOCTRL(TIMER7, PWC_DATA_GET, NULL);//读比较值，自动清CAP中断
		DBG("\n> Get value1 : %d\n", (int)PWCValue);
	}

	if(ret & PWC_OVER_RUN_INTERRUPT_SRC)//捕获定时器溢出
	{
		Timer_InterruptFlagClear(TIMER7,PWC_OVER_RUN_INTERRUPT_SRC);	  //清中断标志
	//		DBG("\nTimer3 PWC_OVER_RUN\n");
	}
}

static void PWC_GpioConfigInit(TIMER_INDEX TimerIdx)
{
    if(TimerIdx == TIMER5)
    {

        GPIO_RegOneBitSet(GPIO_A_IE,GPIO_INDEX18);
        PWC_GpioConfig(TIMER5,18);
        DBG("PWC INPUT: A18\n");
    }

    if(TimerIdx == TIMER6)
    {

        GPIO_RegOneBitSet(GPIO_A_IE,GPIO_INDEX18);
        PWC_GpioConfig(TIMER6,18);
        DBG("PWC INPUT: A18\n");
    }

    if(TimerIdx == TIMER7)
	{
		GPIO_RegOneBitSet(GPIO_A_IE,GPIO_INDEX18);
		PWC_GpioConfig(TIMER7,18);

		DBG("PWC INPUT: A18\n");
	}

	if(TimerIdx == TIMER8)
	{

		GPIO_RegOneBitSet(GPIO_A_IE,GPIO_INDEX18);
		PWC_GpioConfig(TIMER8,18);
		DBG("PWC INPUT: A18\n");
	}
}

static void PWM_GpioConfigInit(TIMER_INDEX TimerIdx)
{

	if(TimerIdx == TIMER5)
	{
		PWM_GpioConfig(TIMER5_PWM_A0_A7_A10_A22_A24_B0_B1, 1, PWM_IO_MODE_OUT);
        DBG("PWM Init OUTPUT: A7\n");
	}

	if(TimerIdx == TIMER6)
	{
		PWM_GpioConfig(TIMER6_PWM_A1_A9_A10_A23_A24_A28_B0_B1, 1, PWM_IO_MODE_OUT);
		DBG("PWM Init OUTPUT: A9\n");
	}

	if(TimerIdx == TIMER7)
	{
		PWM_GpioConfig(TIMER7_PWM_A3_A5_A20_B4, 1, PWM_IO_MODE_OUT);
		DBG("PWM Init OUTPUT: A5\n");
	}
	if(TimerIdx == TIMER8)
	{
		PWM_GpioConfig(TIMER8_PWM_A4_A6_A21_B5, 0, PWM_IO_MODE_OUT);
   		DBG("PWM Init OUTPUT: A4\n");
	}
}
void PwcSingleOrContinueMode(bool IsSingle)
{
    TIMER_INDEX TimerIdx = TIMER5;
    uint8_t     PolarIdx = PWC_POLARITY_RAISING;
    uint8_t     TimeScaleIdx = 2;
    uint8_t     PWMFreqIdx   = 2;
    TIMER_INDEX PWMIdx;

    uint32_t    TimeScaleTbl[]  = {1, 10, 100, 1000};
    uint32_t    PWMFreqTbl[4][3] = {
                                    {20,       800,       4000},
                                    {400,      4000,      80000},
                                    {100000,   4000000,   800000},
                                    {200000,   8000000,   50000000},
                                   };

    PWC_StructInit  PWCParam;
    PWM_StructInit  PWMParam;

    uint32_t PWCValue;
    uint8_t  Buf[10];

    //查询方式
	DBG("\n> Timer%d, PolarIdx: %d,Time Scale: %d,FreqDiv: %d\n", TimerIdx+1,PolarIdx, (int)TimeScaleTbl[TimeScaleIdx],(int)PWMFreqTbl[TimeScaleIdx][PWMFreqIdx]);
	PWMIdx = (TimerIdx == TIMER5? TIMER6:TIMER5);

	//PWC复用GPIO配置
	PWC_GpioConfigInit(TimerIdx);

	//PWM复用GPIO配置
	PWM_GpioConfigInit(PWMIdx);

	//PWC参数配置
	PWCParam.Polarity        = PolarIdx;
	PWCParam.SingleGet       = IsSingle;
	PWCParam.DMAReqEnable    = 0;
	PWCParam.TimeScale       = TimeScaleTbl[TimeScaleIdx];
	PWCParam.FilterTime      = 3;
	PWCParam.PwcSourceSelect      = 0;

	//PWM参数配置
	PWMParam.CounterMode        = PWM_COUNTER_MODE_UP;
	PWMParam.OutputType         = PWM_OUTPUT_SINGLE_1;
	PWMParam.DMAReqEnable       = 0;
	PWMParam.FreqDiv            = PWMFreqTbl[TimeScaleIdx][PWMFreqIdx];
	PWMParam.Duty               = 50;

	PWM_Config(PWMIdx,&PWMParam);
	PWM_Enable(PWMIdx);

	PWC_Config(TimerIdx, &PWCParam);
	PWC_Enable(TimerIdx);

	DBG("-----------------PWC Mode:Check------------------------\n");
	DBG("Examp Menu:\n");
	DBG("x: exit \n");
	DBG("s: Start\n");
	DBG("p: Pause\n");
	DBG("r: Resume\n");
	DBG("-------------------------------------------------------\n");

	//将PWM输出口与PWC捕获口相联接，PWC就可捕获PWM的脉冲宽度
	while(1)
	{
		if(PWC_IOCTRL(TimerIdx,PWC_DONE_STATUS_GET,NULL))
		{
			PWCValue = PWC_IOCTRL(TimerIdx, PWC_DATA_GET, NULL);
			DBG("\n> Get value : %d\n", (int)PWCValue);
		}
		Buf[0] = 0x0;
		UARTS_Recv(0, Buf, 1,0);
		if(Buf[0] == 'x')
		{
			DBG("x\n");
			PWC_Disable(TimerIdx);
			break;
		}
		if(Buf[0] == 's')
		{
			DBG("Start\n");
			PWC_Enable(TimerIdx);
		}
		if(Buf[0] == 'p')
		{
			DBG("Pause\n");
			Timer_Pause(TimerIdx, 1);
		}
		if(Buf[0] == 'r')
		{
			DBG("Resume\n");
			Timer_Pause(TimerIdx, 0);
		}
	}
}
void PwcInterruptMode(bool IsSingle)
{
    TIMER_INDEX TimerIdx = TIMER7;
    uint8_t     PolarIdx = PWC_POLARITY_BOTH;

    TIMER_INDEX PWMIdx;

    uint32_t    PwcTimeScale  = 12000;
    uint32_t    PWMFreqDiv = 120000000;


    PWC_StructInit  PWCParam;
    PWM_StructInit  PWMParam;

    uint8_t IrqTbl[8] = {0, 0, 0, 0, Timer5_IRQn, Timer6_IRQn, Timer7_IRQn, Timer8_IRQn};

    uint8_t  Buf[10];

	DBG("\n> Timer%d, PolarIdx: %d,Time Scale: %d,FreqDiv: %d\n", TimerIdx+1, PolarIdx,(int)PwcTimeScale,(int)PWMFreqDiv);
	DBG("-----------------PWC Mode:Interrupt--------------------\n");
	DBG("Examp Menu:\n");
	DBG("x: exit \n");
	DBG("s: Start\n");
	DBG("p: Pause\n");
	DBG("r: Resume\n");
	DBG("-------------------------------------------------------\n");

    //中断方式

	NVIC_DisableIRQ(Timer7_IRQn);
	NVIC_DisableIRQ(Timer8_IRQn);

	NVIC_EnableIRQ(IrqTbl[TimerIdx]);
	PWMIdx = (TimerIdx == TIMER7? TIMER8:TIMER7);

	PWC_GpioConfigInit(TimerIdx);//PWC复用GPIO配置
	PWM_GpioConfigInit(PWMIdx);//PWM复用GPIO配置

	//PWC参数配置
	PWCParam.Polarity        = PolarIdx;
	PWCParam.SingleGet       = IsSingle;//IsSingle;
	PWCParam.DMAReqEnable    = 0;
	PWCParam.TimeScale       = PwcTimeScale;//PWC的捕获理论值 = PWMFreqDiv / PwcTimeScale
	PWCParam.FilterTime      = 0;

	//PWM参数配置
	PWMParam.CounterMode       = PWM_COUNTER_MODE_UP;
	PWMParam.OutputType        = PWM_OUTPUT_SINGLE_1;
	PWMParam.DMAReqEnable      = 0;
	PWMParam.FreqDiv           = PWMFreqDiv;//1s
	PWMParam.Duty              = 50;

	PWM_Config(PWMIdx, &PWMParam);
	PWM_Enable(PWMIdx);

	PWC_Config(TimerIdx, &PWCParam);
	Timer_InterrputSrcEnable(TimerIdx, PWC1_CAP_DATA_INTERRUPT_SRC);
//	Timer_InterrputSrcEnable(TimerIdx, PWC_OVER_RUN_INTERRUPT_SRC);
//	Timer_InterrputSrcEnable(TimerIdx, UPDATE_INTERRUPT_SRC);
	PWC_Enable(TimerIdx);

	//将PWM输出口与PWC捕获输入口联接，PWC就可捕获PWM的脉冲宽度
	while(1)
	{
		Buf[0] = 0x0;
		UARTS_Recv(0, Buf, 1,100);
		if(Buf[0] == 'x')
		{
			DBG("x\n");
			PWC_Disable(TimerIdx);
			break;
		}
		if(Buf[0] == 's')
		{
			DBG("Start\n");
			PWC_Enable(TimerIdx);
		}
		if(Buf[0] == 'p')
		{
			DBG("Pause\n");
			Timer_Pause(TimerIdx, 1);
		}
		if(Buf[0] == 'r')
		{
			DBG("Resume\n");
			Timer_Pause(TimerIdx, 0);
		}
	}
}

uint32_t pwc_dma_buf[100];
#define     ADR_GPIO_A_REG_O_TGL                                       (0x40040010)
void PwcDMAMode(void)
{
    TIMER_INDEX TimerIdx = TIMER5;
    uint8_t     PolarIdx = PWC_POLARITY_RAISING;
    uint8_t     TimeScaleIdx = 0;
    uint8_t     PWMFreqIdx   = 0;
    TIMER_INDEX PWMIdx;

    uint32_t    TimeScaleTbl[]  = {1, 10, 100, 1000};
    uint32_t    PWMFreqTbl[4][3] = {
                                    {20,       800,       4000},
                                    {400,      4000,      80000},
                                    {100000,   4000000,   800000},
                                    {200000,   8000000,   50000000},
                                   };

    PWC_StructInit  PWCParam;
    PWM_StructInit  PWMParam;
    DMA_CONFIG    DMAParam;

    //uint32_t PWCValue;
    uint8_t  Buf[10];

    //查询方式
	DBG("\n> Timer%d, PolarIdx: %d,Time Scale: %d,FreqDiv: %d\n", TimerIdx+1,PolarIdx, (int)TimeScaleTbl[TimeScaleIdx],(int)PWMFreqTbl[TimeScaleIdx][PWMFreqIdx]);
	PWMIdx = (TimerIdx == TIMER5? TIMER6:TIMER5);

	//PWC复用GPIO配置
	PWC_GpioConfigInit(TimerIdx);

	//PWM复用GPIO配置
	PWM_GpioConfigInit(PWMIdx);

	GPIO_PortAModeSet(GPIOA7,0);
	GPIO_RegOneBitSet(GPIO_A_OE,GPIO_INDEX7);
	GPIO_RegOneBitSet(GPIO_A_OUT,GPIO_INDEX7);

	//PWC参数配置
	PWCParam.Polarity        = PolarIdx;
	PWCParam.SingleGet       = 0;
	PWCParam.DMAReqEnable    = 1;
	PWCParam.TimeScale       = TimeScaleTbl[TimeScaleIdx];
	PWCParam.FilterTime      = 0;

	memset(pwc_dma_buf,0x00000080,sizeof(pwc_dma_buf));
	//PWC DMA
	DMAParam.Dir = DMA_CHANNEL_DIR_MEM2PERI;
	DMAParam.Mode = DMA_CIRCULAR_MODE;
	DMAParam.SrcAddress = pwc_dma_buf;
	DMAParam.DataWidth = DMA_DWIDTH_WORD;
	DMAParam.SrcAddrIncremental = DMA_DST_AINCR_DST_WIDTH;
	DMAParam.DstAddress = ADR_GPIO_A_REG_O_TGL;
//	DMAParam.DstDataWidth = DMA_SRC_DWIDTH_WORD;
	DMAParam.DstAddrIncremental = DMA_DST_AINCR_NO;
	DMAParam.BufferLen = sizeof(pwc_dma_buf);
	DMA_TimerConfig(PERIPHERAL_ID_TIMER5, &DMAParam);
	DMA_ChannelEnable(PERIPHERAL_ID_TIMER5);
	DMA_CircularWritePtrSet(PERIPHERAL_ID_TIMER5, sizeof(pwc_dma_buf)+2);//将读指针指向buffer外
	//PWM参数配置
	PWMParam.CounterMode        = PWM_COUNTER_MODE_UP;
	PWMParam.OutputType         = PWM_OUTPUT_SINGLE_1;
	PWMParam.DMAReqEnable       = 0;
	PWMParam.FreqDiv            = PWMFreqTbl[TimeScaleIdx][PWMFreqIdx];
	PWMParam.Duty               = 50;

	PWM_Config(PWMIdx,&PWMParam);
	PWM_Enable(PWMIdx);

	PWC_Config(TimerIdx, &PWCParam);
	PWC_Enable(TimerIdx);

	DBG("-----------------PWC Mode:Check------------------------\n");
	DBG("Examp Menu:\n");
	DBG("x: exit \n");
	DBG("s: Start\n");
	DBG("p: Pause\n");
	DBG("r: Resume\n");
	DBG("-------------------------------------------------------\n");

	//将PWM输出口与PWC捕获口相联接，PWC就可捕获PWM的脉冲宽度
	while(1)
	{
		Buf[0] = 0x0;
		UARTS_Recv(0, Buf, 1,100);
		if(Buf[0] == 'x')
		{
			DBG("x\n");
			PWC_Disable(TimerIdx);
			break;
		}
		if(Buf[0] == 's')
		{
			DBG("Start\n");
			PWC_Enable(TimerIdx);
		}
		if(Buf[0] == 'p')
		{
			DBG("Pause\n");
			Timer_Pause(TimerIdx, 1);
		}
		if(Buf[0] == 'r')
		{
			DBG("Resume\n");
			Timer_Pause(TimerIdx, 0);
		}
	}
}

int main(void)
{
	TIMER_INDEX TimerId = TIMER2;
    uint8_t recvBuf = 0;

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

    // BP15系列开发板启用串口，默认使用
    GPIO_PortAModeSet(GPIOA9, 1);  // Rx, A9:uart1_rxd_1
    GPIO_PortAModeSet(GPIOA10, 5); // Tx, A10:uart1_txd_1
    DbgUartInit(1, 2000000, 8, 0, 1);
    SysTickInit();
	GIE_ENABLE();

	DBG("-------------------------------------------------------\n");
	DBG("\t\tPWC Example\n");
	DBG("Example Menu:\n");
	DBG("1: PWC Single mode example\n");
	DBG("2: PWC Continue mode example\n");
	DBG("3: PWC Interrupt example\n");
	DBG("4: PWC DMA example\n");
	DBG("-------------------------------------------------------\n");

	while(1)
	{
		recvBuf = 0;
		recvBuf = GetChar();

		if(recvBuf == '1')
		{
			PwcSingleOrContinueMode(1);//single mode
		}
		else if(recvBuf == '2')
		{
			PwcSingleOrContinueMode(0);//Continue mode
		}
		else if(recvBuf == '3')
		{
			PwcInterruptMode(0);//
		}
		else if(recvBuf == '4')
		{
			DMA_ChannelAllocTableSet(DmaChannelMap);
			PwcDMAMode();
		}
	}


}
