/**
**************************************************************************************
* @file    powerconsumption_example.c
* @brief   powerconsumption_example.c
*
* @author  Long
* @version V1.0.0
*
* @Created: 2024/03/05 
*
* @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
**************************************************************************************
*/

#include <stdlib.h>
#include <nds32_intrinsic.h>
#include <string.h>
#include "uarts.h"
#include "uarts_interface.h"
#include "type.h"
#include "debug.h"
#include "timeout.h"
#include "clk.h"
#include "dma.h"
#include "dac.h"
#include "timer.h"
#include "adc.h"
#include "i2s.h"
#include "watchdog.h"
#include "spi_flash.h"
#include "remap.h"
#include "audio_adc.h"
#include "gpio.h"
#include "chip_info.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "powercontroller.h"

#define USE_DCDC  (0) // 1:使用DCDC，0:不使用DCDC，注意只有支持DCDC的开发板才可以使用DCDC，否则会导致芯片无法正常工作

extern void __c_init_rom(void);
uint8_t UartPort = 1;

extern void DeepSleepProc();

void UARTLogIOConfig(void)
{
    // BP15系列开发板启用串口，默认使用
    GPIO_PortAModeSet(GPIOA10, 5); // UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);  // UART1 RX
    DbgUartInit(UartPort, 2000000, 8, 0, 1);
}

void DeepsleepPowerConsumptionProc()
{
    DeepSleepProc();
}

void SleepPowerConsumptionTest()
{
    
}

void SystemClockInit(void)
{
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
}

int main(void)
{
    uint8_t Key = 0;

    Chip_Init(1);
    WDG_Disable();
    __c_init_rom();
    SystemClockInit();
#if (USE_DCDC == 1)
	ldo_switch_to_dcdc(6);
#else
    Power_LDO16Config(1);
#endif

    SpiFlashInit(80000000, MODE_4BIT, 0, 1);

    UARTLogIOConfig();

    DBG("\n");
    DBG("/-----------------------------------------------------\\\n");
    DBG("|              PowerConsumer Example                  |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("|       Build Time: %s %s              |\n", __DATE__, __TIME__);
    DBG("\\-----------------------------------------------------/\n"); 
    DBG("\n");
    DeepsleepPowerConsumptionProc();
    
    while(1);
}
