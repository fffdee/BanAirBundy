/**
 **************************************************************************************
 * @file    dmic_example.c
 * @brief   Example_AudioDMIC
 *
 * @author  Mike
 * @version V1.0.0
 *
 * $Created: 2023-11-3 11:30:00$
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

extern void __c_init_rom(void);

// 定义2个全局buf，用于缓存ADC和DAC的数据，注意单位
uint32_t AudioADC1Buf[1024] = {0}; // 1024 * 4 = 4K
uint32_t AudioDACBuf[1024] = {0};  // 1024 * 4 = 4K

static int32_t PcmBuf1[512] = {0};

static uint8_t DmaChannelMap[] =
{
    PERIPHERAL_ID_AUDIO_ADC0_RX,
    PERIPHERAL_ID_AUDIO_ADC1_RX,
    PERIPHERAL_ID_AUDIO_DAC0_TX,
    255,
    255,
    255,
};

static void AudioDMicExample(void)
{
    uint16_t RealLen;
    uint16_t n;

    uint32_t SampleRate = 48000;
    uint32_t DACBitWidth = 24;
    AUDIO_BitWidth ADCBitWidth = ADC_WIDTH_24BITS;

    DACParamCt ct;
    ct.DACModel = DAC_Single;
    ct.DACLoadStatus = DAC_NOLoad;
    ct.PVDDModel = PVDD33;
    ct.DACEnergyModel = DACCommonEnergy;
    ct.DACVcomModel = Disable;

	GPIO_PortAModeSet(GPIOA28, 4);  //GPIO DMIC Data
	GPIO_PortAModeSet(GPIOA29, 10); //GPIO DMIC Clock

    AudioADC_DigitalInit(ADC0_MODULE, SampleRate, ADCBitWidth, (void *)AudioADC1Buf, sizeof(AudioADC1Buf));
	AudioADC_DMICDownSampleSel(ADC0_MODULE, DOWN_SR_64);
	AudioADC_DMICEnable(ADC0_MODULE);
    // DAC init
    AudioDAC_Init(&ct, SampleRate, DACBitWidth, (void *)AudioDACBuf, sizeof(AudioDACBuf), NULL, 0);

    while(1)
    {
    	n = AudioADC0_DataLenGet();
        if(n >= 256)
        {
            n = AudioDAC0_DataSpaceLenGet();
            RealLen = AudioADC0_DataGet(PcmBuf1, 256);
            if(n > RealLen)
            {
                n = RealLen;
            }
            DBG(".");
            AudioDAC0_DataSet(PcmBuf1, n);
        }
    }
}

// ADC演示工程，主要演示ADC配置流程
// 1: MIC通路配置（ASDM1）
// 2: LineIn1/2通路配置（ASDM0）
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

    SpiFlashInit(80000000, MODE_4BIT, 0, 1);

    // BP15系列开发板启用串口，默认使用
    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
    DbgUartInit(1, 2000000, 8, 0, 1);

    DMA_ChannelAllocTableSet(DmaChannelMap);

    DBG("\n");
    DBG("/-----------------------------------------------------\\\n");
    DBG("|                     AudioDMIC Example                     |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");

    AudioDMicExample();

    while(1);
}
