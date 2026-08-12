/*
 * backup_example.c
 *
 *  Created on: Jan 12, 2024
 *      Author: jiaozhenqiang
 */

#include <stdlib.h>
#include <nds32_intrinsic.h>
#include "gpio.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "type.h"
#include "debug.h"
#include "clk.h"
#include "timer.h"
#include "watchdog.h"
#include "spi_flash.h"
#include "chip_info.h"
#include "pmu_interface.h"
#include "sys.h"
#include "reset.h"
#include "timeout.h"

#define NVM_TEST_SIZE	(16)

//#define DEMO_8S_RESET


void NvmTestExample(void)
{
	uint8_t Nvm_Set_Data[NVM_TEST_SIZE] = {0};
	uint8_t Nvm_Get_Data[NVM_TEST_SIZE] = {0};

	int i = 0;

	PMU_NVMInit();

	PMU_NvmRead(0,Nvm_Get_Data,NVM_TEST_SIZE);

	DBG("NVM data before modified:");
	for(i = 0; i < 16; i++)
	{
		DBG("0x%02x,", Nvm_Get_Data[i]);
	}
	DBG("\r\n");

	for(i=0;i<NVM_TEST_SIZE;i++)
	{
		Nvm_Set_Data[i] = Nvm_Get_Data[i] + 1;
	}

	PMU_NvmWrite(0, Nvm_Set_Data, NVM_TEST_SIZE);
	PMU_NvmRead(0, Nvm_Get_Data, NVM_TEST_SIZE);

	DBG("NVM data after modified :");
	for(i=0;i<16;i++)
	{
		DBG("0x%02x,", Nvm_Get_Data[i]);
	}
	DBG("\r\n");
}


void PowerKeyExample(void)
{
	TE_PWRKEYINIT_RET ePwrKeyInitRet = E_PWRKEYINIT_OK;

	bool bLongPressRestFlag = (Reset_FlagGet() & 0x04)?TRUE:FALSE;
	Reset_FlagClear();

	DBG("===============%s==============\r\n", __func__);

	//长按复位检测
	if (bLongPressRestFlag)
	{
		DBG("Poweron from long press reset!!!\r\n");
	}

	//initialize setting 初始化设置
	ePwrKeyInitRet = SystemPowerKeyInit(E_PWRKEY_MODE_PUSHBUTTON, E_KEYDETTIME_512MS);

	//Powerkey 使能状态显示
	if (E_PWRKEYINIT_OK == ePwrKeyInitRet)
	{
		DBG("Enable powerkey !!!\r\n");
	}
	else if (E_PWRKEYINIT_ALREADY_ENABLED == ePwrKeyInitRet)
	{
		DBG("Powerkey has been enabled in previous poweron !!!\r\n");
	}
	else if (E_PWRKEYINIT_NONE == ePwrKeyInitRet)
	{
		DBG("Powerkey disabled !!!\r\n");
	}
	else if (E_PWRKEYINIT_NOTSUPPORT == ePwrKeyInitRet)
	{
		DBG("Powerkey function not support on this chip !!!\r\n");
	}
	else
	{
		DBG("Powerkey init failed, err code %d, please check setting parameters !!!\r\n", ePwrKeyInitRet);
	}

	//display current mode setting 显示当前设置模式
	if (SystemPowerKeyGetMode() == E_PWRKEY_MODE_SLIDESWITCH_HON)
	{
		DBG("Current setting is SLIDESWITCH HIGH ON MODE\r\n");
	}
	else if (SystemPowerKeyGetMode() == E_PWRKEY_MODE_SLIDESWITCH_LON)
	{
		DBG("Current setting is SLIDESWITCH LOW ON MODE\r\n");
	}
	else if (SystemPowerKeyGetMode() == E_PWRKEY_MODE_PUSHBUTTON)
	{
		DBG("Current setting is PUSHBUTTON MODE\r\n");
	}
	else
	{
		DBG("Current setting is BYPASS MODE\r\n");
	}

	//clear state flags
	SystemPowerKeyStateClear();

#ifdef DEMO_8S_RESET
	if (SystemPowerKeyGetMode() == E_PWRKEY_MODE_PUSHBUTTON)
	{
		DBG("Demo for 8s long press reset function !!!\r\n");
	}
	else
	{
		DBG("There is no 8s long press reset function in this mode !!!\r\n");
	}
#endif

	while(1)
	{
		DelayMs(100);
#ifndef DEMO_8S_RESET
		if (SystemPowerKeyDetect())
		{
			DBG("\r\nDetected powerkey signal, goto powerdown !!!\r\n");

			//goto powerdown
			PMU_SystemPowerDown();
		}
#endif
	}
}

int main(void)
{
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
    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
    DbgUartInit(1, 2000000, 8, 0, 1);

	SysTickInit();

	DBG("\n");
    DBG( "/-----------------------------------------------------\\\n");
    DBG( "|                    PMU Example                   |\n");
    DBG( "|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");

    //NVM example
    NvmTestExample();
	
    //powerkey demo
    PowerKeyExample();

    return 0;
}

