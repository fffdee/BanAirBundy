/**
 **************************************************************************************
 * @file    AudioADC_example.c
 * @brief   AudioADC example
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
uint32_t AudioADC2Buf[1024] = {0}; // 1024 * 4 = 4K
uint32_t AudioDACBuf[1024] = {0};  // 1024 * 4 = 4K

static int32_t PcmBuf1[512] = {0};
static int32_t PcmBuf2[512] = {0};
static int32_t PcmBuf3[512] = {0};

static uint8_t DmaChannelMap[] =
{
    PERIPHERAL_ID_AUDIO_ADC0_RX,
    PERIPHERAL_ID_AUDIO_ADC1_RX,
    PERIPHERAL_ID_AUDIO_DAC0_TX,
    255,
    255,
    255,
};

static void DATA_1to2_apply(int32_t *pcm_in, int32_t *pcm_out, int32_t n)
{
    int i;
    for (i = n - 1; i >= 0; i--)
    {
        pcm_out[i * 2] = pcm_in[i];
        pcm_out[i * 2 + 1] = pcm_in[i];
    }
}

static void AudioMicExample(void)
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
	
	GPIO_PortBModeSet(GPIOB7,0);
	GPIO_PortBModeSet(GPIOB8,0);
	GPIO_RegBitsClear(GPIO_B_IE,GPIOB7);
	GPIO_RegBitsClear(GPIO_B_OE,GPIOB7);
	GPIO_RegBitsClear(GPIO_B_IE,GPIOB8);
	GPIO_RegBitsClear(GPIO_B_OE,GPIOB8);

    // DAC init
    AudioDAC_Init(&ct, SampleRate, DACBitWidth, (void *)AudioDACBuf, sizeof(AudioDACBuf), NULL, 0);

    //PGAGain设置为22是按照BP1564全功能demo板为平台来设置的(BP1564全功能demo板MIC端有放大器，放大20dB)。如果使用其他没有放大器的平台可以调大MIC PGAGain
    AudioADC_AnaInit(ADC1_MODULE, CHANNEL_LEFT, MIC_LEFT, Diff, ADCCommonEnergy, 22);  
    AudioADC_DigitalInit(ADC1_MODULE, SampleRate, ADCBitWidth, (void *)AudioADC1Buf, sizeof(AudioADC1Buf));

    while(1)
    {
        if(AudioADC1_DataLenGet() >= 256)
        {
            n = AudioDAC0_DataSpaceLenGet();
            RealLen = AudioADC1_DataGet(PcmBuf1, 256);
            DATA_1to2_apply(PcmBuf1, PcmBuf1, 256);  //单声道转双声道
            if(n > RealLen)
            {
                n = RealLen;
            }
            AudioDAC0_DataSet(PcmBuf1, n);
        }
    }
}

static void AudioLineIn1Example()
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

	GPIO_PortBModeSet(GPIOB2,0);
	GPIO_PortBModeSet(GPIOB3,0);
	GPIO_RegBitsClear(GPIO_B_IE,GPIOB2);
	GPIO_RegBitsClear(GPIO_B_OE,GPIOB2);
	GPIO_RegBitsClear(GPIO_B_IE,GPIOB3);
	GPIO_RegBitsClear(GPIO_B_OE,GPIOB3);

    // DAC init
    AudioDAC_Init(&ct, SampleRate, DACBitWidth, (void *)AudioDACBuf, sizeof(AudioDACBuf), NULL, 0);

    AudioADC_AnaInit(ADC0_MODULE, CHANNEL_LEFT, LINEIN1_LEFT, Single, ADCCommonEnergy, 17);
    AudioADC_AnaInit(ADC0_MODULE, CHANNEL_RIGHT, LINEIN1_RIGHT, Single, ADCCommonEnergy, 17);
    AudioADC_DigitalInit(ADC0_MODULE, SampleRate, ADCBitWidth, (void *)AudioADC2Buf, sizeof(AudioADC2Buf));

    while(1)
    {
        if(AudioADC0_DataLenGet() >= 256)
        {
            n = AudioDAC0_DataSpaceLenGet();
            RealLen = AudioADC0_DataGet(PcmBuf2, 256);
            if(n > RealLen)
            {
                n = RealLen;
            }
            AudioDAC0_DataSet(PcmBuf2, n);
        }
    }
}

static void AudioLineIn2Example()
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

	GPIO_PortBModeSet(GPIOB0,0);
	GPIO_PortBModeSet(GPIOB1,0);
	GPIO_RegBitsClear(GPIO_B_IE,GPIOB0);
	GPIO_RegBitsClear(GPIO_B_OE,GPIOB0);
	GPIO_RegBitsClear(GPIO_B_IE,GPIOB1);
	GPIO_RegBitsClear(GPIO_B_OE,GPIOB1);

    // DAC init
    AudioDAC_Init(&ct, SampleRate, DACBitWidth, (void *)AudioDACBuf, sizeof(AudioDACBuf), NULL, 0);

    AudioADC_AnaInit(ADC0_MODULE, CHANNEL_LEFT, LINEIN2_LEFT, Single, ADCCommonEnergy, 17);
    AudioADC_AnaInit(ADC0_MODULE, CHANNEL_RIGHT, LINEIN2_RIGHT, Single, ADCCommonEnergy, 17);
    AudioADC_DigitalInit(ADC0_MODULE, SampleRate, ADCBitWidth, (void *)AudioADC2Buf, sizeof(AudioADC2Buf));

    while(1)
    {
        if(AudioADC0_DataLenGet() >= 256)
        {
            n = AudioDAC0_DataSpaceLenGet();
            RealLen = AudioADC0_DataGet(PcmBuf3, 256);
            if(n > RealLen)
            {
                n = RealLen;
            }
            AudioDAC0_DataSet(PcmBuf3, n);
        }
    }
}

// ADC演示工程，主要演示ADC配置流程
// 1: MIC通路配置（ASDM1）
// 2: LineIn1/2通路配置（ASDM0）
int main(void)
{
    uint8_t Key = 0;
    uint8_t UartPort = 1;

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
    DBG("|                     AudioADC Example                     |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");
    DBG("please choose the channel:\nM: MIC;\nL: LINEIN1;\nX: LINEIN2\n");
    while(1)
    {
        if(UARTS_Recv(UartPort, &Key, 1, 100) > 0)
        {
            switch (Key)
            {
            case 'M':
            case 'm':
                DBG("MIC ---> DAC\n");
                AudioMicExample();
                break;
            case 'L':
            case 'l':
                DBG("LineIn1 ---> DAC\n");
                AudioLineIn1Example();
                break;
            case 'X':
            case 'x':
                DBG("LineIn2  ---> DAC\n");
                AudioLineIn2Example();
                break;
            default:
                break;
            }
        }
    }
    while(1);
}
