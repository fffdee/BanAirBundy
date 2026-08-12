/**
 **************************************************************************************
 * @file    dac_example.c
 * @brief   dac example
 *
 * @author  Mike
 * @version V1.0.0
 *
 * $Created: 2024-02-19 11:30:00$
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
#include "i2s.h"
#include "watchdog.h"
#include "remap.h"
#include "gpio.h"
#include "chip_info.h"
#include "dac_interface.h"
#include "powercontroller.h"
#include "sine_gen.h"
#include "delay.h"
#include "random.h"
#include "sys.h"

extern void __c_init_rom(void);

uint32_t AudioDACBuf[1024] = {0};  // 1024 * 4 = 4K

static int32_t PcmBuf[1024] = {0};

static uint8_t DmaChannelMap[] =
{
    PERIPHERAL_ID_AUDIO_DAC0_TX,
    255,
    255,
    255,
    255,
    255,
};

static Sine32GenContext sineGen32;

//1mS 1K正弦波 0db单声道数据 @48K 24Bit
static int32_t Sin48k_Buf24[48] =
{
	0x00000000,
	0x00085F8D,
	0x00109A6E,
	0x00188C94,
	0x00201333,
	0x00270D53,
	0x002D5C64,
	0x0032E4C4,
	0x00378E37,
	0x003B4452,
	0x003DF6D2,
	0x003F99E8,
	0x00402667,
	0x003F99E8,
	0x003DF6D2,
	0x003B4452,
	0x00378E37,
	0x0032E4C4,
	0x002D5C64,
	0x00270D53,
	0x00201333,
	0x00188C94,
	0x00109A6E,
	0x00085F8D,
	0x00000000,
	0xFFF7A073,
	0xFFEF6592,
	0xFFE7736C,
	0xFFDFECCD,
	0xFFD8F2AD,
	0xFFD2A39C,
	0xFFCD1B3C,
	0xFFC871C9,
	0xFFC4BBAE,
	0xFFC2092E,
	0xFFC06618,
	0xFFBFD999,
	0xFFC06618,
	0xFFC2092E,
	0xFFC4BBAE,
	0xFFC871C9,
	0xFFCD1B3C,
	0xFFD2A39C,
	0xFFD8F2AD,
	0xFFDFECCD,
	0xFFE7736C,
	0xFFEF6592,
	0xFFF7A073,
};

void Dac_Example(void)
{
    while(1)
    {
        memset(PcmBuf, 0x00, sizeof(PcmBuf));
        if(AudioDAC0_DataSpaceLenGet() >= 256)
        {
            sine24_generator_apply(&sineGen32, PcmBuf, PcmBuf, 256);   //生成256samples
            AudioDAC0_DataSet(PcmBuf, 256);
        }
    }
}

void  Dac_BUG_Example(void)
{
    /**标准操作方案流程二选一**/
	/**全部注释可复现异常现象,并且有概率出现硬件无输出现象**/
	#define BUG_B_PLAN1//Pause->Run Dac优选
	//#define BUG_B_PLAN2//FunctionReset 需mute杂音 外设通用

    int32_t i;

	for(i = 0; i < 48; i++)
	{
		PcmBuf[2 * i + 0] = Sin48k_Buf24[i]/2;//half 防饱和
		PcmBuf[2 * i + 1] = 0;
	}

    TIMER TestTimer;

	#define TESTPERIOD		100//mS 50~5000mS 周期性操作测试,建议：监听时加长，录音时减短;注释此宏后只播正弦波，无测试操作
	#define TESTJITTER		1000//uS 时隙随机区间，勿改

	TimeOutSet(&TestTimer,TESTPERIOD);

	while(1)
	{
		if(AudioDAC0_DataSpaceLenGet() > 48)// len @ 1 word / samples
		{
			AudioDAC0_DataSet(PcmBuf, 48);
		}

	#ifdef TESTPERIOD
		if(IsTimeOut(&TestTimer))
		{
			DelayUs(GetRandomNum(GetSysTick1MsCnt(), TESTJITTER));//测试间隔毫秒级随机化

		#ifdef BUG_B_PLAN1
			AudioDAC_Pause(DAC0);
		#else
			AudioDAC_Disable(DAC0);
		#endif

		#ifdef BUG_B_PLAN2
			AudioDAC_FuncReset(DAC0);
		#else
			AudioDAC_Reset(DAC0);
		#endif
			DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_DONE_INT);
			DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_THRESHOLD_INT);
			DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_ERROR_INT);

			DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC0_TX);

			DMA_CircularConfig(PERIPHERAL_ID_AUDIO_DAC0_TX, sizeof(AudioDACBuf)/2, AudioDACBuf, sizeof(AudioDACBuf));
			//AudioDAC0_DataSet(PcmBuf, 48); //防播空,BP15不需要防播空
			DMA_ChannelEnable(PERIPHERAL_ID_AUDIO_DAC0_TX);

		#ifdef BUG_B_PLAN1
			AudioDAC_Run(DAC0);
		#else
			AudioDAC_Enable(DAC0);
		#endif
			TimeOutSet(&TestTimer,TESTPERIOD);
		}
	#endif //#ifdef TESTPERIOD
	}
}

// DAC演示工程，主要演示DAC配置流程
int main(void)
{
    Chip_Init(1);
    WDG_Disable();
    __c_init_rom();
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
    SysTickInit();

    SpiFlashInit(80000000, MODE_4BIT, 0, 1);

    // BP15系列开发板启用串口，默认使用
    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
    DbgUartInit(1, 2000000, 8, 0, 1);

    DMA_ChannelAllocTableSet(DmaChannelMap);

    DBG("\n");
    DBG("/-----------------------------------------------------\\\n");
    DBG("|                     DAC Example                      |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.       |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");

    uint32_t SampleRate = 48000;
    uint32_t DACBitWidth = 24;

    DACParamCt ct;
    ct.DACModel = DAC_Single;
    ct.DACLoadStatus = DAC_Load;
    ct.PVDDModel = PVDD33;
    ct.DACEnergyModel = DACLowEnergy;
    ct.DACVcomModel = Disable;

    sine24_generator_init(&sineGen32, SampleRate, 3, 1000, 1000, -200, -200);
    // DAC init
    AudioDAC_Init(&ct, SampleRate, DACBitWidth, (void *)AudioDACBuf, sizeof(AudioDACBuf), NULL, 0);

    Dac_Example();
    //Dac_BUG_Example();

    while(1);
}
