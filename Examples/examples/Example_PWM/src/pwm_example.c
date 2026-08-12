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

#define FREQUENCY_1US				120

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
    	if(UART_Recv(1,&ch, 1,1000) > 0)
        {
            ret=1;
        }
    }
    return ch;
}
static char	tempString[20];
char * GetString(void)
{
	char ch;
	char *tempP;
	memset(tempString, 0, sizeof(tempString));
	tempP = tempString;

	ch = GetChar();
	while(ch != 0x0D)
	{
		DBG("%c", ch);
		*(tempP++) = ch;
		ch = GetChar();
	}
	ch = GetChar();
	return tempString;
}

uint32_t StrToI(char * str)
{
	uint32_t result;

	result = 0;
	while(*str)
	{
		result = result*10 + *(str++) - '0';
	}
	return result;
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

static void PWM_GpioConfigDeInit(TIMER_INDEX TimerIdx)
{
    if(TimerIdx == TIMER5)
    {
        PWM_GpioConfig(TIMER5_PWM_A0_A7_A10_A22_A24_B0_B1, 1, PWM_IO_MODE_NONE);
        DBG("PWM DeInit OUTPUT: A7\n");
    }

    if(TimerIdx == TIMER6)
    {
        PWM_GpioConfig(TIMER6_PWM_A1_A9_A10_A23_A24_A28_B0_B1, 1, PWM_IO_MODE_NONE);
        DBG("PWM DeInit OUTPUT: A9\n");
    }

    if(TimerIdx == TIMER7)
	 {
		 PWM_GpioConfig(TIMER7_PWM_A3_A5_A20_B4, 1, PWM_IO_MODE_NONE);
		 DBG("PWM DeInit OUTPUT: A5\n");
	 }

	 if(TimerIdx == TIMER8)
	 {
		 PWM_GpioConfig(TIMER8_PWM_A4_A6_A21_B5, 0, PWM_IO_MODE_NONE);
		 DBG("PWM DeInit OUTPUT: A4\n");
	 }

}

void PwmForceOutputExample(void)
{
	TIMER_INDEX 	TimerIndex = TIMER5;
	PWM_StructInit  PWMParam;
	uint8_t         type = PWM_OUTPUT_FORCE_LOW;
	uint8_t			recvBuf;

	DBG("PWM force output example\n");
	PWMParam.CounterMode   			= PWM_COUNTER_MODE_UP;
	PWMParam.OutputType    			= type;
	PWMParam.DMAReqEnable  			= PWM_REQ_INTERRUPT_MODE;
	PWMParam.FreqDiv				= 12000; //120M系统频率下 12000 = 100us
	PWMParam.Duty			        = 50;

	//GPIO Config
	PWM_GpioConfigInit(TimerIndex);

	//PWM Config
	PWM_Config(TimerIndex,&PWMParam);

	//PWM Start
	PWM_Enable(TimerIndex);
	DBG("'c' : change OutputType\n");
	DBG("'x' : exit this example\n\n");

	 while(1)
	 {
		 recvBuf = 0;
		 recvBuf = GetChar();

		 if(recvBuf == 'x')
		 {
			 PWM_Disable(TimerIndex);
			 break;
		 }
		 else if(recvBuf == 'c')
		 {
			 if(type == PWM_OUTPUT_FORCE_LOW)
			 {
				 type = PWM_OUTPUT_FORCE_HIGH;
				 DBG("OutputType: PWM_OUTPUT_FORCE_HIGH\n");
			 }
			 else
			 {
				 type = PWM_OUTPUT_FORCE_LOW;
				 DBG("OutputType: PWM_OUTPUT_FORCE_LOW\n");
			 }

			 PWMParam.OutputType  = type;
			 PWM_Config(TimerIndex,&PWMParam);
			 PWM_Enable(TimerIndex);
		 }
	 }
}
void PwmOnePulseOutputExample(void)
{
	TIMER_INDEX 	TimerIndex = TIMER5;
	PWM_StructInit  PWMParam;
	uint8_t         type = PWM_OUTPUT_SINGLE_1;
	uint8_t			recvBuf;

	DBG("PWM one pulse output example\n");
	PWMParam.CounterMode   			= PWM_COUNTER_MODE_DOWN;
	PWMParam.OutputType    			= PWM_OUTPUT_ONE_PULSE;//;
	PWMParam.DMAReqEnable  			= PWM_REQ_INTERRUPT_MODE;
	PWMParam.FreqDiv				= 120000; //120M / 120000 = 1000us
	PWMParam.Duty			        = 50;

	//GPIO Config
	PWM_GpioConfigInit(TimerIndex);

	//PWM Config
	PWM_Config(TimerIndex,&PWMParam);

	//PWM Start
	PWM_Enable(TimerIndex);
	DBG("'s' : Start One Pulse PWM\n");
	DBG("'c' : change OutputType\n");
	DBG("'x' : exit this example\n\n");
	while(1)
	{
		recvBuf = 0;
		recvBuf = GetChar();

		if(recvBuf == 'x')
		{
			break;
		}
		else if(recvBuf == 's')
		{
			DBG("Start\n");
			PWM_Enable(TimerIndex);
		}
		else if(recvBuf == 'c')
		{
			if(type == PWM_OUTPUT_SINGLE_1)
			{
				type = PWM_OUTPUT_SINGLE_2;
				DBG("change one pulse Type: PWM_OUTPUT_SINGLE_2\n");
			}
			else
			{
				type = PWM_OUTPUT_SINGLE_1;
				DBG("change one pulse Type: PWM_OUTPUT_SINGLE_1\n");
			}

			PWM_IOCTRL_ARG arg;
			arg.OutputType = type;

			PWM_IOCTRl(TimerIndex,OUTPUT_TYPE_UPDATE,&arg);

			PWM_Enable(TimerIndex);
		}
	}
}

void PwmContinuousOutputExample(void)
{
	uint8_t						recvBuf;

	TIMER_INDEX					timer = TIMER7;
	PWM_IOCTRL_ARG arg;


	PWM_StructInit				pwmParam;
	PWM_COUNTER_MODE			countMode = PWM_COUNTER_MODE_DOWN;
	PWM_OUTPUT_TYPE				outputType = PWM_OUTPUT_SINGLE_1;
	uint32_t					frequency = 100;
	uint8_t						duty = 50;

	DBG("TimerPWMExample:\n\n");

GOHERE:
	DBG("Timer%d, Count Mode = %d, Duty = %d, Output Type = %d, Frequency = %d us\n", timer+1, countMode, duty, outputType, (int)frequency);

	PWM_GpioConfigDeInit(timer);
	PWM_GpioConfigInit(timer);

	pwmParam.CounterMode		= countMode;
	pwmParam.OutputType			= outputType;
	pwmParam.DMAReqEnable		= PWM_REQ_INTERRUPT_MODE;
	pwmParam.FreqDiv			= frequency * FREQUENCY_1US;
	pwmParam.Duty				= duty;
	PWM_Config(timer,&pwmParam);
	PWM_Enable(timer);

	DBG("'s' : setting\n");
	DBG("'x' : exit this example\n");
	while(1)
	{
		recvBuf = GetChar();

		if(recvBuf == 'x')
		{
			PWM_Disable(timer);
			break;
		}
		else if(recvBuf == 's')
		{
			char	ch = 0;
			// enter setting menu
			DBG("Enter setting menu:\n");
			DBG("1. Timer\n");
			DBG("2. CountMode\n");
			DBG("3. Duty\n");
			DBG("4. OutputType\n");
			DBG("5. Frequency\n");
			DBG("p. Print Settings\n");
			DBG("g. go!\n");

			while(ch != 'g')
			{
				char			*str;

				ch = GetChar();
				DBG("%c\n", ch);
REINPUT:
				switch(ch)
				{
				case '1':
					{
						uint8_t		tempVal;
						DBG("1.Timer:");
						str = GetString();
						DBG("\n");
						tempVal = StrToI(str);
						if(tempVal < 5 || tempVal > 8)
						{
							DBG("Error input timer (5 ~ 8)\n");
							timer = timer;
							goto REINPUT;
						}
						else
						{
							timer = TIMER5 + tempVal - 5;
							goto GOHERE;
						}
					}
					break;
				case '2':
					{
						uint32_t		tempVal;
						DBG("2.CountMode: (0 ~ 5)");
						str = GetString();
						DBG("\n");
						tempVal = StrToI(str);
						if(tempVal > 5)
						{
							DBG("Error input CountMode\n");
							countMode = countMode;
							goto REINPUT;
						}
						else
						{
							countMode = tempVal;
							goto GOHERE;
						}
					}
					break;
				case '3':
					{
						uint32_t		tempVal;
						DBG("3.Duty:(0 ~ 100)");
						str = GetString();
						DBG("\n");
						tempVal = StrToI(str);
						if(tempVal > 100)
						{
							DBG("Error input Duty (0 ~ 100)\n");
							duty = duty;
							goto REINPUT;
						}
						else
						{
							duty = tempVal;

							arg.Duty = duty;
							PWM_IOCTRl(timer,OUTPUT_DUTY_UPDATE,&arg);
							DBG("DUTY:%d\n",duty);
							goto GOHERE;
						}
					}
					break;
				case '4':
					{
						uint32_t		tempVal;
						DBG("4.OutputType:(4 ~ 8)");
						str = GetString();
						DBG("\n");
						tempVal = StrToI(str);
						if(tempVal < 4 || tempVal > 8)
						{
							DBG("Error input OutputType (4 ~ 8)\n");
							outputType = outputType;
							goto GOHERE;
						}
						else
						{
							outputType = tempVal;

							arg.OutputType = outputType;
							PWM_IOCTRl(timer,OUTPUT_TYPE_UPDATE,&arg);
							DBG("outputType:%d\n",outputType);
							goto GOHERE;
						}
					}
					break;
				case '5':
					{
						uint32_t		tempVal;
						DBG("5.Frequency:");
						str = GetString();
						DBG("us\n");
						tempVal = StrToI(str);
						frequency = tempVal;

						arg.FreqDiv = frequency * FREQUENCY_1US;
						arg.Duty =  duty;//改变频率时，让占空比保持不变
						PWM_IOCTRl(timer,OUTPUT_FREQUENCY_UPDATE | OUTPUT_DUTY_UPDATE,&arg);
						DBG("Frequency = %d us\n",(int)frequency);
						goto GOHERE;
					}
					break;
				case 'p':
					{
						DBG("1. Timer%d\t2. Count Mode = %d\t3. Duty = %d\t4. Output Type = %d\t5. Frequency = %d us\n", timer+1, countMode, duty, outputType, (int)frequency);
					}
					break;
				case 'g':
					{
						goto GOHERE;
					}
					break;
				default:
					{
						DBG("Error input\n");
					}
					break;
				}
			}
		}
	}
}

__attribute__((section(".driver.isr"))) void Timer6Interrupt(void)
{
    uint32_t ret;

    ret = Timer_InterruptFlagGet(TIMER6,PWM1_COMP_INTERRUPT_SRC);

    if(ret & PWM1_COMP_INTERRUPT_SRC)
    {
    	DBG("\n> PWM compare interrupt\n");
    	Timer_InterruptFlagClear(TIMER6,PWM1_COMP_INTERRUPT_SRC);
    }

}
void PwmInterruptExample(void)
{
	TIMER_INDEX 	TimerIndex = TIMER6;
	PWM_StructInit  PWMParam;
	uint8_t         type = PWM_OUTPUT_SINGLE_1;
	uint8_t			recvBuf;

	DBG("PWM Interrupt example\n");
	PWMParam.CounterMode   			= PWM_COUNTER_MODE_DOWN;
	PWMParam.OutputType    			= type;//;
	PWMParam.DMAReqEnable  			= PWM_REQ_INTERRUPT_MODE;
	PWMParam.FreqDiv				= 1000000*FREQUENCY_1US;
	PWMParam.Duty			        = 50;


	//GPIO Config
	PWM_GpioConfigInit(TimerIndex);

	//PWM Config
	PWM_Config(TimerIndex,&PWMParam);

	NVIC_EnableIRQ(Timer6_IRQn);
	Timer_InterrputSrcEnable(TimerIndex, PWM1_COMP_INTERRUPT_SRC);

	//PWM Start
//	PWM_Enable(TimerIndex);

	DBG("'s' : Start\n");
	DBG("'p' : Pause\n");
	DBG("'x' : exit this example\n\n");
	while(1)
	{
		recvBuf = 0;
		recvBuf = GetChar();

		if(recvBuf == 'x')
		{
			PWM_Disable(TimerIndex);
			NVIC_DisableIRQ(Timer6_IRQn);
			break;
		}
		else if(recvBuf == 's')
		{
			DBG("Start\n");
			PWM_Enable(TimerIndex);
		}
		else if(recvBuf == 'p')
		{
			DBG("Pause\n");
			PWM_Disable(TimerIndex);
		}
		else if(recvBuf == 'c')
		{
		if(type == PWM_OUTPUT_SINGLE_1)
		{
			type = PWM_OUTPUT_SINGLE_2;
			DBG("change one pulse Type: PWM_OUTPUT_SINGLE_2\n");
		}
		else
		{
			type = PWM_OUTPUT_SINGLE_1;
			DBG("change one pulse Type: PWM_OUTPUT_SINGLE_1\n");
		}

		PWM_IOCTRL_ARG arg;
		arg.OutputType = type;

		PWM_IOCTRl(TimerIndex,OUTPUT_TYPE_UPDATE,&arg);

		PWM_Enable(TimerIndex);
		}
	}
}

void PwmMasterSlaveExample(void)
{
	PWM_StructInit PWMParam;
	uint8_t	recvBuf;

	DBG("PwmMasterSlaveExample in\n");

	PWM_GpioConfigInit(TIMER5);
	PWM_GpioConfigInit(TIMER6);
	PWM_GpioConfigInit(TIMER7);
	PWM_GpioConfigInit(TIMER8);

	//主机TIMER5配置PWM参数
	PWMParam.CounterMode   = PWM_COUNTER_MODE_DOWN;
	PWMParam.OutputType    = PWM_OUTPUT_FORCE_HIGH;
	PWMParam.FreqDiv       = 400;//PWMFreqTable[FreqIndex];
	PWMParam.Duty          = 50;
	PWMParam.DMAReqEnable  = 0;
	PWMParam.MasterModeSel = 2;
	PWMParam.MasterSlaveSel = 0;
	PWMParam.TriggerInSrc = 0;
	PWMParam.CcxmAndCccUpdataSel = 0;
	PWM_Config(TIMER5, &PWMParam);
	//从机TIMER6配置PWM参数
	PWMParam.FreqDiv       = 400;
	PWMParam.OutputType    = PWM_OUTPUT_SINGLE_1;
	PWMParam.MasterModeSel = 0;
	PWMParam.MasterSlaveSel = 1;
	PWMParam.TriggerInSrc = 0;
	PWMParam.CcxmAndCccUpdataSel = 1;
	PWM_Config(TIMER6, &PWMParam);
	//从机TIMER7配置PWM参数
	PWMParam.FreqDiv       = 400;
	PWMParam.OutputType    = PWM_OUTPUT_SINGLE_1;
	PWMParam.MasterModeSel = 0;
	PWMParam.MasterSlaveSel = 1;
	PWMParam.TriggerInSrc = 0;
	PWMParam.CcxmAndCccUpdataSel = 1;
	PWM_Config(TIMER7, &PWMParam);
	//从机TIMER8配置PWM参数
	PWMParam.FreqDiv       = 400;
	PWMParam.OutputType    = PWM_OUTPUT_SINGLE_1;
	PWMParam.MasterModeSel = 0;
	PWMParam.MasterSlaveSel = 1;
	PWMParam.TriggerInSrc = 0;
	PWMParam.CcxmAndCccUpdataSel = 1;
	PWM_Config(TIMER8, &PWMParam);

	DBG("'s' : Start\n");
	DBG("'x' : exit this example\n\n");
	while(1)
	{
		recvBuf = GetChar();

		if(recvBuf == 'x')
		{
			PWM_Disable(TIMER5);
			PWM_Disable(TIMER6);
			PWM_Disable(TIMER7);
			PWM_Disable(TIMER8);

			NVIC_DisableIRQ(Timer5_IRQn);
			NVIC_DisableIRQ(Timer6_IRQn);
			NVIC_DisableIRQ(Timer7_IRQn);
			NVIC_DisableIRQ(Timer8_IRQn);
			break;
		}
		else if(recvBuf == 's')
		{
			DBG("Start\n");
			PWM_Enable(TIMER5);
		}
		else
		{

		}
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

    // BP15系列开发板启用串口，默认使用
    GPIO_PortAModeSet(GPIOA9, 1);  // Rx, A9:uart1_rxd_1
    GPIO_PortAModeSet(GPIOA10, 5); // Tx, A10:uart1_txd_1
    DbgUartInit(1, 2000000, 8, 0, 1);

    SysTickInit();
	GIE_ENABLE();

	DBG("-------------------------------------------------------\n");
	DBG("\t\tPWM Example\n");
	DBG("Example Menu:\n");
	DBG("1: PWM Force output example\n");
	DBG("2: PWM One pulse output example\n");
	DBG("3: PWM Continuous output example\n");
	DBG("4: PWM Interrupt example\n");
	DBG("5: PWM MasterSlave example\n");
	DBG("-------------------------------------------------------\n");

	while(1)
	{
		Temp = GetChar();

		if(Temp == '1')
		{
			PwmForceOutputExample();
		}
		else if(Temp == '2')
		{
			PwmOnePulseOutputExample();
		}
		else if(Temp == '3')
		{
			PwmContinuousOutputExample();
		}
		else if(Temp == '4')
		{
			PwmInterruptExample();
		}
		else if(Temp == '5')
		{
			PwmMasterSlaveExample();
		}
	}

}
