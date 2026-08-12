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
#include "dac_interface.h"

#define I2S0_MCLK_GPIO					I2S0_MCLK_OUT_A24 //选择MCLK_OUT/MCLK_IN脚，I2S自动配置成master/slave
#define I2S0_LRCLK_GPIO					I2S0_LRCLK_A20
#define I2S0_BCLK_GPIO					I2S0_BCLK_A21
#define I2S0_DOUT_GPIO					I2S0_DOUT_A22
#define I2S0_DIN_GPIO					I2S0_DIN_A23
#define CFG_RES_I2S0_MODE				GET_I2S_MODE(I2S0_MCLK_GPIO)		//根据I2S_MCLK_GPIO自动配置master/slave

#define I2S1_MCLK_GPIO					I2S1_MCLK_IN_A6 //选择MCLK_OUT/MCLK_IN脚，I2S自动配置成master/slave
#define I2S1_LRCLK_GPIO					I2S1_LRCLK_A7
#define I2S1_BCLK_GPIO					I2S1_BCLK_A9
#define I2S1_DOUT_GPIO					I2S1_DOUT_A10
#define I2S1_DIN_GPIO					I2S1_DIN_A30
#define CFG_RES_I2S1_MODE				GET_I2S_MODE(I2S1_MCLK_GPIO)		//根据I2S_MCLK_GPIO自动配置master/slave

static uint8_t DmaChannelMap[29] = {
		PERIPHERAL_ID_AUDIO_ADC0_RX,
		PERIPHERAL_ID_AUDIO_DAC0_TX,
		PERIPHERAL_ID_I2S0_RX,
		PERIPHERAL_ID_I2S0_TX,
		PERIPHERAL_ID_I2S1_RX,
		PERIPHERAL_ID_I2S1_TX,
};

#define I2S0_RX_FIFO_LEN			(10 * 1024)
#define I2S0_TX_FIFO_LEN			(10 * 1024)
#define I2S1_RX_FIFO_LEN			(10 * 1024)
#define I2S1_TX_FIFO_LEN			(10 * 1024)
#define ADC_FIFO_LEN				(10 * 1024)
#define DAC_FIFO_LEN				(10 * 1024)

#define AUDIO_FRAME					(192)

int32_t PcmBuf[AUDIO_FRAME * 8];
int32_t AdcFifo[ADC_FIFO_LEN / 4];
int32_t DacFifo[DAC_FIFO_LEN / 4];
int32_t I2s0RxFifo[I2S0_RX_FIFO_LEN / 4];
int32_t I2s0TxFifo[I2S0_RX_FIFO_LEN / 4];
int32_t I2s1TxFifo[I2S1_TX_FIFO_LEN / 4];
int32_t I2s1RxFifo[I2S1_RX_FIFO_LEN / 4];

void I2S0_Master_RX_Example(void);
void I2S1_Slave_TX_Example(void);
void I2S0_I2S1_Binding_Example(void);
void I2S0_24Bit_Example(void);


int main(void)
{
    uint8_t key = 0;
    uint8_t UartPort = 1;

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

	DMA_ChannelAllocTableSet(DmaChannelMap);

	DBG("\n");
    DBG("/-----------------------------------------------------\\\n");
    DBG("|                     I2S Example                     |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("r: I2S0_Master_RX_Example(i2s0->DAC)    \n");
    DBG("t: I2S1_Slave_TX_Example(ADC->i2s1)     \n");
    DBG("s: I2S0_I2S1_Binding_Example \n");
    DBG("h: I2S0_24bit_Example        \n");
    DBG("\n");

    while(1)
	{
		key = 0;
		if(UARTS_Recv(UartPort, &key, 1,100) > 0)
		{
			switch(key)
			{
				case 'r':
					DBG("r: I2S0_Master_RX_Example\n");
					I2S0_Master_RX_Example();
					break;
				case 't':
					DBG("t: I2S1_Slave_TX_Example\n");
					I2S1_Slave_TX_Example();
					break;
				case 's':
					DBG("s: I2S0_I2S1_Binding_Example\n");
					I2S0_I2S1_Binding_Example();
					break;
				case 'h':
					DBG("h: I2S0_24bit_Example\n");
					I2S0_24Bit_Example();
					break;
				default:
					break;
			}
		}
	}



}


//I2S0 Config: master and rx
void I2S0_Master_RX_Example(void)
{
	I2SParamCt i2s_ct;
    DACParamCt dac_ct;

	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_MCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_MCLK_GPIO));//mclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_LRCLK_GPIO),GET_I2S_GPIO_MODE(I2S0_LRCLK_GPIO));//lrclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_BCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_BCLK_GPIO));//bclk
//	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_DOUT_GPIO), GET_I2S_GPIO_MODE(I2S0_DOUT_GPIO));//do
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_DIN_GPIO), GET_I2S_GPIO_MODE(I2S0_DIN_GPIO));//di

    dac_ct.DACModel = DAC_Single;
    dac_ct.DACLoadStatus = DAC_NOLoad;
    dac_ct.PVDDModel = PVDD33;
    dac_ct.DACEnergyModel = DACCommonEnergy;

	i2s_ct.IsMasterMode = CFG_RES_I2S0_MODE;
	i2s_ct.SampleRate   = 48000;
	i2s_ct.I2sFormat    = I2S_FORMAT_I2S;
	i2s_ct.I2sBits      = I2S_LENGTH_24BITS;
	i2s_ct.RxPeripheralID = PERIPHERAL_ID_I2S0_RX;
	i2s_ct.RxBuf          = (void* )I2s0RxFifo;
	i2s_ct.RxLen		  = I2S0_RX_FIFO_LEN;
	i2s_ct.I2sTxRxEnable  = 2;

    AudioDAC_Init(&dac_ct, 48000, 24, (void *)DacFifo, DAC_FIFO_LEN, NULL, 0);
	I2S_AlignModeSet(I2S0_MODULE, I2S_LOW_BITS_ACTIVE);
	AudioI2S_Init(I2S0_MODULE, &i2s_ct);

	while(1)
	{
		if(AudioI2S_DataLenGet(I2S0_MODULE) >= AUDIO_FRAME)
		{
			AudioI2S_DataGet(I2S0_MODULE, PcmBuf, AUDIO_FRAME);
			AudioDAC0_DataSet(PcmBuf, AUDIO_FRAME);
		}
	}
}

//I2S0 Config: slave and tx
void I2S1_Slave_TX_Example(void)
{
	int   cur_sampleRate = 48000;
	int   sampleRate     = 48000;
	I2SParamCt i2s_ct;

	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S1_MCLK_GPIO), GET_I2S_GPIO_MODE(I2S1_MCLK_GPIO));//mclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S1_LRCLK_GPIO),GET_I2S_GPIO_MODE(I2S1_LRCLK_GPIO));//lrclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S1_BCLK_GPIO), GET_I2S_GPIO_MODE(I2S1_BCLK_GPIO));//bclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S1_DOUT_GPIO), GET_I2S_GPIO_MODE(I2S1_DOUT_GPIO));//do
//	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S1_DIN_GPIO), GET_I2S_GPIO_MODE(I2S1_DIN_GPIO));//di

	i2s_ct.IsMasterMode = CFG_RES_I2S1_MODE;
	i2s_ct.SampleRate   = 48000;
	i2s_ct.I2sFormat    = I2S_FORMAT_I2S;
	i2s_ct.I2sBits      = I2S_LENGTH_24BITS;
	i2s_ct.TxPeripheralID = PERIPHERAL_ID_I2S1_TX;
	i2s_ct.TxBuf          = (void* )I2s1TxFifo;
	i2s_ct.TxLen		  = I2S1_TX_FIFO_LEN;
	i2s_ct.I2sTxRxEnable  = 1;
	I2S_AlignModeSet(I2S1_MODULE, I2S_LOW_BITS_ACTIVE);
	AudioI2S_Init(I2S1_MODULE, &i2s_ct);

    AudioADC_AnaInit(ADC0_MODULE, CHANNEL_LEFT, LINEIN1_LEFT, Single, ADCCommonEnergy, 17);
    AudioADC_AnaInit(ADC0_MODULE, CHANNEL_RIGHT, LINEIN1_RIGHT, Single, ADCCommonEnergy, 17);
    AudioADC_DigitalInit(ADC0_MODULE, 48000, ADC_WIDTH_24BITS,(void *)AdcFifo, ADC_FIFO_LEN);

//    DACParamCt ct;
//    ct.DACModel = DAC_Single;
//    ct.DACLoadStatus = DAC_NOLoad;
//    ct.PVDDModel = PVDD33;
//    ct.DACEnergyModel = DACCommonEnergy;
//    AudioDAC_Init(&ct, 48000, 24, (void *)DacFifo, DAC_FIFO_LEN, NULL, 0);

	I2S_SampleRateCheckInterruptClr(I2S1_MODULE);
	I2S_SampleRateCheckInterruptEnable(I2S1_MODULE);

	while(1)
	{
		if(AudioADC0_DataLenGet() >= AUDIO_FRAME)
		{
			AudioADC0_DataGet(PcmBuf, AUDIO_FRAME);
			AudioI2S_DataSet(I2S1_MODULE, PcmBuf, AUDIO_FRAME);

//			AudioDAC0_DataSet(PcmBuf, AUDIO_FRAME);
		}

		if(I2S_SampleRateCheckInterruptGet(I2S1_MODULE))
		{
			cur_sampleRate = I2S_SampleRateGet(I2S1_MODULE);
			if(cur_sampleRate != sampleRate)
			{
				DBG("I2S Master sample rate change: %d\n", cur_sampleRate);
				sampleRate = cur_sampleRate;
				if((sampleRate == 11025) || (sampleRate == 22050) || (sampleRate == 44100))
				{
					Clock_AudioMclkSel(AUDIO_ADC0, PLL_CLOCK1);
				}
				else
				{
					Clock_AudioMclkSel(AUDIO_ADC0, PLL_CLOCK2);
				}
				AudioADC_SampleRateSet(ADC0_MODULE, sampleRate);
			}
			I2S_SampleRateCheckInterruptClr(I2S1_MODULE);
		}
	}
}

//I2S Config: lrclk must be used A20, bclk must be used A21
void I2S0_I2S1_Binding_Example(void)
{
	I2SParamCt i2s0_ct, i2s1_ct;

	//I2S0和I2S1的lrclk和bclk采样内部link，减少2个GPIO的复用
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_MCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_MCLK_GPIO));//mclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_LRCLK_GPIO),GET_I2S_GPIO_MODE(I2S0_LRCLK_GPIO));//lrclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_BCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_BCLK_GPIO));//bclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_DOUT_GPIO), GET_I2S_GPIO_MODE(I2S0_DOUT_GPIO));//do
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S1_DIN_GPIO), GET_I2S_GPIO_MODE(I2S1_DIN_GPIO));//di

	//I2S0 Config: master and tx
	i2s0_ct.IsMasterMode   = CFG_RES_I2S0_MODE;
	i2s0_ct.SampleRate     = 48000;
	i2s0_ct.I2sFormat      = I2S_FORMAT_I2S;
	i2s0_ct.I2sBits        = I2S_LENGTH_24BITS;
	i2s0_ct.TxPeripheralID = PERIPHERAL_ID_I2S0_TX;
	i2s0_ct.TxBuf          = (void* )I2s0TxFifo;
	i2s0_ct.TxLen		   = I2S0_TX_FIFO_LEN;
	i2s0_ct.I2sTxRxEnable  = 1;
	I2S_AlignModeSet(I2S0_MODULE, I2S_LOW_BITS_ACTIVE);
	AudioI2S_Init(I2S0_MODULE, &i2s0_ct);

	//I2S1 Config: slave and rx
	i2s1_ct.IsMasterMode   = CFG_RES_I2S1_MODE;
	i2s1_ct.SampleRate     = 48000;
	i2s1_ct.I2sFormat      = I2S_FORMAT_I2S;
	i2s1_ct.I2sBits        = I2S_LENGTH_24BITS;
	i2s1_ct.RxPeripheralID = PERIPHERAL_ID_I2S1_RX;
	i2s1_ct.RxBuf          = (void* )I2s1RxFifo;
	i2s1_ct.RxLen		   = I2S1_RX_FIFO_LEN;
	i2s1_ct.I2sTxRxEnable  = 2;
	I2S_AlignModeSet(I2S1_MODULE, I2S_LOW_BITS_ACTIVE);
	AudioI2S_Init(I2S1_MODULE, &i2s1_ct);

	while(1)
	{
		if(AudioI2S_DataLenGet(I2S1_MODULE) > AUDIO_FRAME)
		{
			AudioI2S_DataGet(I2S1_MODULE, PcmBuf, AUDIO_FRAME);
			AudioI2S_DataSet(I2S0_MODULE, PcmBuf, AUDIO_FRAME);
		}
	}
}

void I2S0_24Bit_Example(void)
{
	I2SParamCt i2s_ct;

	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_MCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_MCLK_GPIO));//mclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_LRCLK_GPIO),GET_I2S_GPIO_MODE(I2S0_LRCLK_GPIO));//lrclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_BCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_BCLK_GPIO));//bclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_DOUT_GPIO), GET_I2S_GPIO_MODE(I2S0_DOUT_GPIO));//do
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S0_DIN_GPIO), GET_I2S_GPIO_MODE(I2S0_DIN_GPIO));//di

	i2s_ct.IsMasterMode = 0;
	i2s_ct.SampleRate   = 48000;
	i2s_ct.I2sFormat    = I2S_FORMAT_I2S;
	i2s_ct.I2sBits      = I2S_LENGTH_24BITS;

	//rx config
	i2s_ct.RxPeripheralID = PERIPHERAL_ID_I2S0_RX;
	i2s_ct.RxBuf          = (void* )I2s0RxFifo;
	i2s_ct.RxLen			= I2S0_RX_FIFO_LEN;

	//tx config
	i2s_ct.TxPeripheralID = PERIPHERAL_ID_I2S0_TX;
	i2s_ct.TxBuf          = (void* )I2s0TxFifo;
	i2s_ct.TxLen		  = I2S0_TX_FIFO_LEN;

	i2s_ct.I2sTxRxEnable = 3;
	I2S_AlignModeSet(I2S0_MODULE, I2S_LOW_BITS_ACTIVE);
	AudioI2S_Init(I2S0_MODULE, &i2s_ct);

	while(1)
	{
		if(AudioI2S_DataLenGet(I2S0_MODULE) >= AUDIO_FRAME)
		{
			AudioI2S_DataGet(I2S0_MODULE, PcmBuf, AUDIO_FRAME);
			AudioI2S_DataSet(I2S0_MODULE, PcmBuf, AUDIO_FRAME);
		}
	}
}

