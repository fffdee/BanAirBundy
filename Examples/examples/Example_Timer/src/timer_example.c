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
#include "audio_adc.h"
#include "dac.h"
#include "uarts_interface.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "i2s_interface.h"
#include "type.h"
#include "debug.h"
#include "timeout.h"
#include "clk.h"
#include "dma.h"
#include "i2s.h"
#include "watchdog.h"
#include "remap.h"
#include "spi_flash.h"
#include "chip_info.h"
#include "irqn.h"

static uint8_t DmaChannelMap[29] = {
		PERIPHERAL_ID_AUDIO_ADC0_RX,
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
__attribute__((section(".driver.isr"))) void Timer2Interrupt(void)
{
	DBG("Timer2Interrupt in\n");
    GPIO_RegOneBitSet(GPIO_A_TGL, GPIO_INDEX22); // toggle A22
    Timer_InterruptFlagClear(TIMER2, UPDATE_INTERRUPT_SRC);
}

__attribute__((section(".driver.isr"))) void Timer3Interrupt(void)
{
	DBG("Timer3Interrupt in\n");
    GPIO_RegOneBitSet(GPIO_A_TGL, GPIO_INDEX22); // toggle A22
    Timer_InterruptFlagClear(TIMER3, UPDATE_INTERRUPT_SRC);
}

__attribute__((section(".driver.isr"))) void Timer4Interrupt(void)
{
	DBG("Timer4Interrupt in\n");
    GPIO_RegOneBitSet(GPIO_A_TGL, GPIO_INDEX22); // toggle A22
    Timer_InterruptFlagClear(TIMER4, UPDATE_INTERRUPT_SRC);
}

__attribute__((section(".driver.isr"))) void Timer5Interrupt(void)
{
	DBG("Timer5Interrupt in\n");
    GPIO_RegOneBitSet(GPIO_A_TGL, GPIO_INDEX22); // toggle A22
    Timer_InterruptFlagClear(TIMER5, UPDATE_INTERRUPT_SRC);
}

__attribute__((section(".driver.isr"))) void Timer6Interrupt(void)
{
	DBG("Timer6Interrupt in\n");
    GPIO_RegOneBitSet(GPIO_A_TGL, GPIO_INDEX22); // toggle A22
    Timer_InterruptFlagClear(TIMER6, UPDATE_INTERRUPT_SRC);
}
__attribute__((section(".driver.isr"))) void Timer7Interrupt(void)
{
	DBG("Timer7Interrupt in\n");
    GPIO_RegOneBitSet(GPIO_A_TGL, GPIO_INDEX22); // toggle A22
    Timer_InterruptFlagClear(TIMER7, UPDATE_INTERRUPT_SRC);
}

__attribute__((section(".driver.isr"))) void Timer8Interrupt(void)
{
	DBG("Timer8Interrupt in\n");
    GPIO_RegOneBitSet(GPIO_A_TGL, GPIO_INDEX22); // toggle A22
    Timer_InterruptFlagClear(TIMER8, UPDATE_INTERRUPT_SRC);
}

//uint32_t usec: IO口翻转的时间，单位:1us
//TImer1默认作为系统定时Timer，用户应使用TIMER2~TIMER8
void TimerInterruptModeExample(TIMER_INDEX TimerId,uint32_t usec)
{
    uint8_t recvBuf;

    DBG("TimerInterruptModeExample:toggle A22\n");
    DBG("Input 'x' to exit this example\n");
    DBG("Input 'p' to Pause\n");
    DBG("Input 'c' to Continues\n");

    NVIC_EnableIRQ(Timer2_IRQn);
    NVIC_EnableIRQ(Timer3_IRQn);
    NVIC_EnableIRQ(Timer4_IRQn);
    NVIC_EnableIRQ(Timer5_IRQn);
    NVIC_EnableIRQ(Timer6_IRQn);
    NVIC_EnableIRQ(Timer7_IRQn);
    NVIC_EnableIRQ(Timer8_IRQn);

    Timer_InterruptFlagClear(TimerId, UPDATE_INTERRUPT_SRC);

    GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX22);
    GPIO_RegOneBitClear(GPIO_A_IE,GPIO_INDEX22);
    GPIO_RegOneBitClear(GPIO_A_OUT,GPIO_INDEX22);
    Timer_Config(TimerId, usec, 0);
    Timer_Start(TimerId);

    while(1)
    {
        recvBuf = 0;
        UARTS_Recv(0, &recvBuf, 1,100);
        if(recvBuf == 'x')
        {
            Timer_Pause(TimerId, 1);
            break;
        }
        if(recvBuf == 'p')
        {
            Timer_Pause(TimerId, 1);
            DBG("Timer Pause\n");
        }
        if(recvBuf == 'c')
        {
            Timer_Pause(TimerId, 0);
            DBG("Timer Continues\n");
        }
    }
}

//uint32_t usec: IO口翻转的时间，单位:1us
void TimerCheckModeExample(TIMER_INDEX TimerId,uint32_t usec)
{
    uint8_t recvBuf;

    DBG("TimerCheckModeExample: toggle A22\n");
    DBG("Input 'x' to exit this example\n");

    NVIC_DisableIRQ(Timer2_IRQn);
    NVIC_DisableIRQ(Timer3_IRQn);
    NVIC_DisableIRQ(Timer4_IRQn);
    NVIC_DisableIRQ(Timer5_IRQn);
    NVIC_DisableIRQ(Timer6_IRQn);
    NVIC_DisableIRQ(Timer7_IRQn);
    NVIC_DisableIRQ(Timer8_IRQn);

    Timer_InterruptFlagClear(TimerId, UPDATE_INTERRUPT_SRC);
    GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX22);
	GPIO_RegOneBitClear(GPIO_A_IE,GPIO_INDEX22);
	GPIO_RegOneBitClear(GPIO_A_OUT,GPIO_INDEX22);

    Timer_Config(TimerId, usec, 0);
    Timer_Start(TimerId);

    while(1)
    {
        recvBuf = 0;
        UARTS_Recv(0, &recvBuf, 1,100);
        if(recvBuf == 'x')
        {
            Timer_Pause(TimerId, 1);
            break;
        }

        if(Timer_InterruptFlagGet(TimerId, UPDATE_INTERRUPT_SRC))
        {
            GPIO_RegOneBitSet(GPIO_A_TGL, GPIO_INDEX22);
            Timer_InterruptFlagClear(TimerId, UPDATE_INTERRUPT_SRC);
            if(usec >= 500000)
            {
                DBG("Timer%d Update\n",TimerId+1);
            }
        }
    }
}
void MastSlaveModeTest(void)
{
	TIMER_CTRL2 TimerArgs;
	uint8_t recvBuf;

	DBG("MastSlaveModeTest in\n");

    NVIC_EnableIRQ(Timer5_IRQn);
    NVIC_EnableIRQ(Timer6_IRQn);
    NVIC_EnableIRQ(Timer7_IRQn);
    NVIC_EnableIRQ(Timer8_IRQn);

    Timer_InterruptFlagClear(TIMER5, UPDATE_INTERRUPT_SRC);
    Timer_InterruptFlagClear(TIMER6, UPDATE_INTERRUPT_SRC);
    Timer_InterruptFlagClear(TIMER7, UPDATE_INTERRUPT_SRC);
    Timer_InterruptFlagClear(TIMER8, UPDATE_INTERRUPT_SRC);

    TimerArgs.MasterModeSel = 1;
    TimerArgs.MasterSlaveSel = 0;
    TimerArgs.TriggerInSrc = 0;
    TimerArgs.CcxmAndCccUpdataSel = 0;
    Timer_ConfigMasterSlave(TIMER5,3000*1000 , 1, &TimerArgs);
    TimerArgs.MasterModeSel = 0;
    TimerArgs.MasterSlaveSel = 1;
    TimerArgs.TriggerInSrc = 0;
    TimerArgs.CcxmAndCccUpdataSel = 1;
    Timer_ConfigMasterSlave(TIMER6,2200*1000 , 0, &TimerArgs);
    TimerArgs.MasterModeSel = 0;
    TimerArgs.MasterSlaveSel = 1;
    TimerArgs.TriggerInSrc = 0;
    TimerArgs.CcxmAndCccUpdataSel = 1;
    Timer_ConfigMasterSlave(TIMER7,2500*1000 , 0, &TimerArgs);
    TimerArgs.MasterModeSel = 0;
    TimerArgs.MasterSlaveSel = 1;
    TimerArgs.TriggerInSrc = 0;
    TimerArgs.CcxmAndCccUpdataSel = 1;
    Timer_ConfigMasterSlave(TIMER8,2800*1000 , 0, &TimerArgs);
    Timer_Start(TIMER5);

    while(1)
	{
		recvBuf = 0;
		UARTS_Recv(0, &recvBuf, 1,100);
		if(recvBuf == 'x')
		{
			Timer_Pause(TIMER5, 1);
			Timer_Pause(TIMER6, 1);
			Timer_Pause(TIMER7, 1);
			Timer_Pause(TIMER8, 1);
			break;
		}
		if(recvBuf == 'p')
		{
			Timer_Pause(TIMER5, 1);
			Timer_Pause(TIMER6, 1);
			Timer_Pause(TIMER7, 1);
			Timer_Pause(TIMER8, 1);
			DBG("Timer Pause\n");
		}
		if(recvBuf == 'c')
		{
			Timer_Pause(TIMER5, 0);
			Timer_Pause(TIMER6, 0);
			Timer_Pause(TIMER7, 0);
			Timer_Pause(TIMER8, 0);
			DBG("Timer Continues\n");
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

    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
    DbgUartInit(1, 2000000, 8, 0, 1);

    SysTickInit();
	GIE_ENABLE();
    RESTART:

        while(1)
        {
            DBG("-------------------------------------------------------\n");
            DBG("\t\tTimer Example\n");
            DBG("Examp Menu:\n");
            DBG("a: Timer Interrupt mode example start,default is Timer2\n");
            DBG("b: Timer Check mode example start,default is Timer2\n");
            DBG("c: Timer Master Slave mode test\n");
            DBG("n: Next Timer\n");
            DBG("-------------------------------------------------------\n");

            recvBuf = 0;
            recvBuf = GetChar();

            if(recvBuf == 'a')
            {
                uint32_t usec = 1000000;
                //Timer2 中断模式Toggle GPIO A22

                DBG("Timer%d Interrupt mode, toggle gpio every %lu ms\n",TimerId+1, usec/1000);

                TimerInterruptModeExample(TimerId,usec); // 1000ms
                goto RESTART;
            }
            else if(recvBuf == 'b')
            {

                uint32_t usec = 1000000;
                //Timer2 查询模式 Toggle GPIO A22

                DBG("Timer%d check mode, toggle gpio every %d ms\n",TimerId+1, (int)usec/1000);

                TimerCheckModeExample(TimerId,usec); // 1000ms
                goto RESTART;
            }
            else if(recvBuf == 'c')
            {
            	MastSlaveModeTest();
                goto RESTART;
            }
            else if(recvBuf == 'n')
            {
            	TimerId ++;
            	if(TimerId > TIMER8)
            	{
            		TimerId = TIMER2;
            	}
            	DBG("Select Timer%d now,Press a or A please\n",TimerId);
            }
        }



}
