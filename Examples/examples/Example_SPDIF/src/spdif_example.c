/**
 **************************************************************************************
 * @file    spdif_example.c
 * @brief   spdif example
 *
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * $Created: 2023-12-07 11:30:00$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <stdlib.h>
#include <nds32_intrinsic.h>
#include <string.h>
#include "math.h"
#include "type.h"
#include "debug.h"
#include "delay.h"
#include "gpio.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "type.h"
#include "debug.h"
#include "timeout.h"
#include "clk.h"
#include "dma.h"
#include "i2s.h"
#include "spdif.h"
#include "watchdog.h"
#include "remap.h"
#include "spi_flash.h"
#include "chip_info.h"
#include "audio_adc.h"
#include "irqn.h"
#include "i2s_interface.h"
#include "adc_interface.h"
#include "dac_interface.h"

#define SPDIF_RX_NUM	 	SPDIF0
#define SPDIF_RX_DMA_ID 	PERIPHERAL_ID_SPDIF0_RX
#define SPDIF_RX_IO_PIN		GPIOA28
#define SPDIF_RX_IO_SET		0x03
#define SPDIF_ANA_INPUT		SPDIF_ANA_INPUT_A28

#define SPDIF_TX_NUM	 	SPDIF1
#define SPDIF_TX_DMA_ID 	PERIPHERAL_ID_SPDIF1_TX
#define SPDIF_TX_IO_PIN		GPIOA29
#define SPDIF_TX_IO_SET		0x0B

 //#define CFG_AUDIO_WIDTH_24BIT


static const uint8_t DmaChannelMap[6] = {
	SPDIF_TX_DMA_ID,
	SPDIF_RX_DMA_ID,
	PERIPHERAL_ID_I2S0_TX,
	PERIPHERAL_ID_AUDIO_ADC0_RX,
	255,
	255,
};

#define I2S_FIFO_LEN			(20 * 1024)
uint32_t I2sAddr[I2S_FIFO_LEN/4];

#define SPDIF_FIFO_LEN				(10 * 1024)
static uint32_t SpdifAddr[SPDIF_FIFO_LEN/4] = {0};

uint32_t AudioADC2Buf[2048] = {0}; // 1024 * 4 = 4K

static int32_t PcmBuf[4*1024];
static int32_t SpdifBuf[4*1024];

static int16_t PcmBuf1[1024] = {0};

//#define SAMPLERATE_ADJUST_EN

#ifdef SAMPLERATE_ADJUST_EN
static int32_t AdjustVal = 0;
static int32_t AdjustValBak = 0;
static int32_t FreeLenWarn = 0;
static int32_t PllUpDownFlag = 0xff;
#endif

uint8_t SineBuf[768] =
{
		0x01, 0x00, 0x00, 0x00, 0x5F, 0x08, 0x5F, 0x08, 0x9A, 0x10, 0x9A, 0x10, 0x8D, 0x18, 0x8D, 0x18,
		0x13, 0x20, 0x14, 0x20, 0x0D, 0x27, 0x0D, 0x27, 0x5C, 0x2D, 0x5D, 0x2D, 0xE5, 0x32, 0xE4, 0x32,
		0x8E, 0x37, 0x8E, 0x37, 0x44, 0x3B, 0x44, 0x3B, 0xF7, 0x3D, 0xF7, 0x3D, 0x99, 0x3F, 0x9A, 0x3F,
		0x26, 0x40, 0x26, 0x40, 0x9A, 0x3F, 0x9A, 0x3F, 0xF8, 0x3D, 0xF6, 0x3D, 0x44, 0x3B, 0x45, 0x3B,
		0x8F, 0x37, 0x8D, 0x37, 0xE5, 0x32, 0xE5, 0x32, 0x5C, 0x2D, 0x5C, 0x2D, 0x0D, 0x27, 0x0D, 0x27,
		0x13, 0x20, 0x14, 0x20, 0x8D, 0x18, 0x8D, 0x18, 0x9B, 0x10, 0x9A, 0x10, 0x5F, 0x08, 0x5F, 0x08,
		0x00, 0x00, 0xFF, 0xFF, 0xA0, 0xF7, 0xA0, 0xF7, 0x65, 0xEF, 0x65, 0xEF, 0x73, 0xE7, 0x74, 0xE7,
		0xEC, 0xDF, 0xED, 0xDF, 0xF3, 0xD8, 0xF2, 0xD8, 0xA3, 0xD2, 0xA4, 0xD2, 0x1C, 0xCD, 0x1C, 0xCD,
		0x72, 0xC8, 0x73, 0xC8, 0xBC, 0xC4, 0xBC, 0xC4, 0x09, 0xC2, 0x09, 0xC2, 0x66, 0xC0, 0x66, 0xC0,
		0xDA, 0xBF, 0xDA, 0xBF, 0x66, 0xC0, 0x65, 0xC0, 0x09, 0xC2, 0x0A, 0xC2, 0xBC, 0xC4, 0xBC, 0xC4,
		0x72, 0xC8, 0x72, 0xC8, 0x1B, 0xCD, 0x1B, 0xCD, 0xA3, 0xD2, 0xA4, 0xD2, 0xF2, 0xD8, 0xF2, 0xD8,
		0xED, 0xDF, 0xEC, 0xDF, 0x74, 0xE7, 0x74, 0xE7, 0x65, 0xEF, 0x66, 0xEF, 0xA0, 0xF7, 0xA0, 0xF7,
		0x00, 0x00, 0x00, 0x00, 0x60, 0x08, 0x60, 0x08, 0x9B, 0x10, 0x9B, 0x10, 0x8D, 0x18, 0x8C, 0x18,
		0x13, 0x20, 0x13, 0x20, 0x0D, 0x27, 0x0D, 0x27, 0x5C, 0x2D, 0x5C, 0x2D, 0xE4, 0x32, 0xE4, 0x32,
		0x8E, 0x37, 0x8E, 0x37, 0x44, 0x3B, 0x45, 0x3B, 0xF7, 0x3D, 0xF7, 0x3D, 0x9A, 0x3F, 0x99, 0x3F,
		0x27, 0x40, 0x27, 0x40, 0x9A, 0x3F, 0x9A, 0x3F, 0xF7, 0x3D, 0xF6, 0x3D, 0x44, 0x3B, 0x44, 0x3B,
		0x8E, 0x37, 0x8F, 0x37, 0xE5, 0x32, 0xE5, 0x32, 0x5D, 0x2D, 0x5C, 0x2D, 0x0E, 0x27, 0x0E, 0x27,
		0x14, 0x20, 0x13, 0x20, 0x8D, 0x18, 0x8D, 0x18, 0x9B, 0x10, 0x9A, 0x10, 0x60, 0x08, 0x60, 0x08,
		0xFF, 0xFF, 0xFF, 0xFF, 0xA1, 0xF7, 0xA1, 0xF7, 0x65, 0xEF, 0x65, 0xEF, 0x73, 0xE7, 0x73, 0xE7,
		0xEC, 0xDF, 0xED, 0xDF, 0xF3, 0xD8, 0xF2, 0xD8, 0xA4, 0xD2, 0xA3, 0xD2, 0x1B, 0xCD, 0x1C, 0xCD,
		0x72, 0xC8, 0x71, 0xC8, 0xBB, 0xC4, 0xBB, 0xC4, 0x09, 0xC2, 0x09, 0xC2, 0x66, 0xC0, 0x66, 0xC0,
		0xDA, 0xBF, 0xDA, 0xBF, 0x66, 0xC0, 0x65, 0xC0, 0x09, 0xC2, 0x09, 0xC2, 0xBC, 0xC4, 0xBC, 0xC4,
		0x72, 0xC8, 0x71, 0xC8, 0x1B, 0xCD, 0x1C, 0xCD, 0xA3, 0xD2, 0xA5, 0xD2, 0xF3, 0xD8, 0xF2, 0xD8,
		0xEC, 0xDF, 0xED, 0xDF, 0x74, 0xE7, 0x74, 0xE7, 0x66, 0xEF, 0x65, 0xEF, 0xA0, 0xF7, 0xA1, 0xF7,
		0x00, 0x00, 0x00, 0x00, 0x60, 0x08, 0x5F, 0x08, 0x9A, 0x10, 0x9A, 0x10, 0x8C, 0x18, 0x8D, 0x18,
		0x14, 0x20, 0x13, 0x20, 0x0E, 0x27, 0x0D, 0x27, 0x5C, 0x2D, 0x5C, 0x2D, 0xE5, 0x32, 0xE5, 0x32,
		0x8E, 0x37, 0x8F, 0x37, 0x44, 0x3B, 0x44, 0x3B, 0xF7, 0x3D, 0xF7, 0x3D, 0x9A, 0x3F, 0x9B, 0x3F,
		0x26, 0x40, 0x26, 0x40, 0x9A, 0x3F, 0x99, 0x3F, 0xF7, 0x3D, 0xF7, 0x3D, 0x45, 0x3B, 0x44, 0x3B,
		0x8E, 0x37, 0x8E, 0x37, 0xE4, 0x32, 0xE5, 0x32, 0x5D, 0x2D, 0x5D, 0x2D, 0x0D, 0x27, 0x0E, 0x27,
		0x13, 0x20, 0x14, 0x20, 0x8C, 0x18, 0x8D, 0x18, 0x9A, 0x10, 0x9B, 0x10, 0x60, 0x08, 0x5F, 0x08,
		0x00, 0x00, 0x00, 0x00, 0xA1, 0xF7, 0xA0, 0xF7, 0x66, 0xEF, 0x65, 0xEF, 0x73, 0xE7, 0x73, 0xE7,
		0xED, 0xDF, 0xEC, 0xDF, 0xF3, 0xD8, 0xF3, 0xD8, 0xA4, 0xD2, 0xA3, 0xD2, 0x1C, 0xCD, 0x1B, 0xCD,
		0x72, 0xC8, 0x72, 0xC8, 0xBC, 0xC4, 0xBC, 0xC4, 0x0A, 0xC2, 0x09, 0xC2, 0x66, 0xC0, 0x66, 0xC0,
		0xDA, 0xBF, 0xDA, 0xBF, 0x65, 0xC0, 0x66, 0xC0, 0x09, 0xC2, 0x09, 0xC2, 0xBC, 0xC4, 0xBC, 0xC4,
		0x72, 0xC8, 0x71, 0xC8, 0x1B, 0xCD, 0x1C, 0xCD, 0xA4, 0xD2, 0xA4, 0xD2, 0xF3, 0xD8, 0xF2, 0xD8,
		0xED, 0xDF, 0xED, 0xDF, 0x74, 0xE7, 0x73, 0xE7, 0x65, 0xEF, 0x66, 0xEF, 0xA1, 0xF7, 0xA1, 0xF7,
		0x00, 0x00, 0x00, 0x00, 0x60, 0x08, 0x5F, 0x08, 0x9B, 0x10, 0x9A, 0x10, 0x8D, 0x18, 0x8D, 0x18,
		0x14, 0x20, 0x13, 0x20, 0x0D, 0x27, 0x0E, 0x27, 0x5C, 0x2D, 0x5D, 0x2D, 0xE5, 0x32, 0xE5, 0x32,
		0x8E, 0x37, 0x8F, 0x37, 0x44, 0x3B, 0x45, 0x3B, 0xF7, 0x3D, 0xF7, 0x3D, 0x9A, 0x3F, 0x9A, 0x3F,
		0x26, 0x40, 0x26, 0x40, 0x99, 0x3F, 0x99, 0x3F, 0xF6, 0x3D, 0xF6, 0x3D, 0x44, 0x3B, 0x44, 0x3B,
		0x8E, 0x37, 0x8F, 0x37, 0xE5, 0x32, 0xE4, 0x32, 0x5D, 0x2D, 0x5D, 0x2D, 0x0D, 0x27, 0x0D, 0x27,
		0x14, 0x20, 0x13, 0x20, 0x8C, 0x18, 0x8C, 0x18, 0x9A, 0x10, 0x9A, 0x10, 0x5F, 0x08, 0x60, 0x08,
		0xFF, 0xFF, 0x00, 0x00, 0xA0, 0xF7, 0xA1, 0xF7, 0x65, 0xEF, 0x66, 0xEF, 0x74, 0xE7, 0x73, 0xE7,
		0xED, 0xDF, 0xEC, 0xDF, 0xF3, 0xD8, 0xF3, 0xD8, 0xA4, 0xD2, 0xA3, 0xD2, 0x1A, 0xCD, 0x1B, 0xCD,
		0x72, 0xC8, 0x71, 0xC8, 0xBC, 0xC4, 0xBB, 0xC4, 0x09, 0xC2, 0x09, 0xC2, 0x66, 0xC0, 0x67, 0xC0,
		0xD9, 0xBF, 0xD9, 0xBF, 0x66, 0xC0, 0x65, 0xC0, 0x0A, 0xC2, 0x09, 0xC2, 0xBC, 0xC4, 0xBB, 0xC4,
		0x72, 0xC8, 0x72, 0xC8, 0x1B, 0xCD, 0x1B, 0xCD, 0xA4, 0xD2, 0xA3, 0xD2, 0xF2, 0xD8, 0xF3, 0xD8,
		0xED, 0xDF, 0xEC, 0xDF, 0x73, 0xE7, 0x74, 0xE7, 0x66, 0xEF, 0x65, 0xEF, 0xA0, 0xF7, 0xA1, 0xF7
};

//iis采样率微调，iss fifo总量是10k，当余量<2k时，降低audio时钟，当余量>8k时，提高audio时钟，
//保证iis fifo余量在2k~8k的缓冲区间内
#ifdef SAMPLERATE_ADJUST_EN
void I2SSampleRateAdjust(void)
{
	int32_t FreeLen = DMA_CircularSpaceLenGet(SPDIF_TX_DMA_ID);
	//连续20次检测到fifo余>8k,则提高audio pll
	if(FreeLen>1024*8)
	{
		FreeLenWarn++;
		if(FreeLenWarn>20)
		{
			AdjustVal = (FreeLen*10/1024 - 80) ;
			PllUpDownFlag = 1;
			FreeLenWarn = 0;
		}

	}
	//连续20次检测到fifo余量<2k,则降低audio pll
	else if(FreeLen<1024*2)
	{
		FreeLenWarn++;
		if(FreeLenWarn>20)
		{
			AdjustVal = (20-FreeLen*10/1024);
			PllUpDownFlag = 0;
			FreeLenWarn = 0;
		}
	}
	else
	{
		FreeLenWarn = 0;
	}
	if((AdjustValBak != AdjustVal)&&(PllUpDownFlag == 1))
	{
		Clock_AudioPllClockAdjust(PLL_CLK_1, 0, AdjustVal);
		Clock_AudioPllClockAdjust(PLL_CLK_2, 0, AdjustVal);
		//printf("FreeLen %d，AdjustVal %d\n",FreeLen,AdjustVal);
		AdjustValBak = AdjustVal;
	}
	else if((AdjustValBak != AdjustVal)&&(PllUpDownFlag == 0))
	{
		Clock_AudioPllClockAdjust(PLL_CLK_1, 1, AdjustVal);
		Clock_AudioPllClockAdjust(PLL_CLK_2, 1, AdjustVal);
		//printf("FreeLen %d,AdjustVal -%d\n",FreeLen,-AdjustVal);
		AdjustValBak = AdjustVal;
	}
}
#endif

void SpdifRxExample(void)
{
	bool SpdifLockFlag = FALSE;
	uint32_t samplerate = 44100;
	uint32_t Len_spdif;
	uint32_t Len_i2s;

	DBG("SpdifRxExample!\n");

	//i2s config
    I2SParamCt i2s_ct;

    GPIO_PortAModeSet(GPIOA24, 8);       //i2s0 mclk out
    GPIO_PortAModeSet(GPIOA20, 7);       //i2s0 lrclk out
    GPIO_PortAModeSet(GPIOA21, 5);       //i2s0  bclk out
    GPIO_PortAModeSet(GPIOA22, 9);       //i2s0  do

	i2s_ct.IsMasterMode = 0;
	i2s_ct.SampleRate   = samplerate;
	i2s_ct.I2sFormat    = I2S_FORMAT_I2S;

#ifdef CFG_AUDIO_WIDTH_24BIT
	i2s_ct.I2sBits      = I2S_LENGTH_24BITS;
#else
	i2s_ct.I2sBits      = I2S_LENGTH_16BITS;
#endif
	//tx config
	i2s_ct.TxPeripheralID = PERIPHERAL_ID_I2S0_TX;
	i2s_ct.TxBuf          = (void* )I2sAddr;
	i2s_ct.TxLen		  = I2S_FIFO_LEN;

	i2s_ct.I2sTxRxEnable = 1;
	AudioI2S_Init(I2S0_MODULE, &i2s_ct);

	//spdif config
    GPIO_RegBitsSet(GPIO_A_ANA_EN,SPDIF_RX_IO_PIN);
    GPIO_PortAModeSet(SPDIF_RX_IO_PIN, SPDIF_RX_IO_SET);

#if SPDIF_RX_NUM == SPDIF0
    SPDIF0_AnalogModuleEnable(SPDIF_ANA_INPUT, SPDIF_ANA_LEVEL_300mVpp);
#else
    SPDIF1_AnalogModuleEnable(SPDIF_ANA_INPUT, SPDIF_ANA_LEVEL_300mVpp);
#endif
    SPDIF_RXInit(SPDIF_RX_NUM, 1, 0, 0);
	DMA_CircularConfig(SPDIF_RX_DMA_ID, SPDIF_FIFO_LEN/2, SpdifAddr, SPDIF_FIFO_LEN);
	SPDIF_ModuleEnable(SPDIF_RX_NUM);

	DMA_ChannelEnable(SPDIF_RX_DMA_ID);
	DMA_ChannelEnable(PERIPHERAL_ID_I2S0_TX);
	while(1)
	{
		if(SpdifLockFlag && !SPDIF_FlagStatusGet(SPDIF_RX_NUM,LOCK_FLAG_STATUS))
		{
			DBG("SPDIF RX UNLOCK!\n");
			SpdifLockFlag = FALSE;
		}
		if(!SpdifLockFlag && SPDIF_FlagStatusGet(SPDIF_RX_NUM,LOCK_FLAG_STATUS))
		{
			DBG("SPDIF RX LOCK!\n");
			SpdifLockFlag = TRUE;
		}

		//监控SPDIF RX采样率是否改变
		if(SpdifLockFlag == TRUE)
		{
			if(samplerate != SPDIF_SampleRateGet(SPDIF_RX_NUM))
			{
				samplerate = SPDIF_SampleRateGet(SPDIF_RX_NUM);

		    	if(IsSelectMclkClk1(samplerate))
					Clock_AudioMclkSel(AUDIO_I2S0, PLL_CLOCK1);
				else
					Clock_AudioMclkSel(AUDIO_I2S0, PLL_CLOCK2);

				I2S_SampleRateSet(I2S0_MODULE, samplerate);
				DBG("Current sample rate: %d\n", (int)samplerate);
			}
#ifdef SAMPLERATE_ADJUST_EN
			I2SSampleRateAdjust();
#endif
		}

		if(DMA_CircularDataLenGet(SPDIF_RX_DMA_ID) >= 256 && DMA_CircularSpaceLenGet(PERIPHERAL_ID_I2S0_TX) >= 256*4)
		{

			Len_spdif = DMA_CircularDataGet(SPDIF_RX_DMA_ID, SpdifBuf, 256);

#ifdef CFG_AUDIO_WIDTH_24BIT
			Len_i2s   = SPDIF_SPDIFDataToPCMData(SPDIF_RX_NUM,(int32_t *)SpdifBuf, Len_spdif, (int32_t *)PcmBuf, SPDIF_WORDLTH_24BIT);
			if(Len_i2s>0)
			{
				int32_t *pcmBuf32  =  (int32_t *)PcmBuf;
				int32_t i = 0;
				for(i=0;i<Len_i2s/4;i++)
				{
					pcmBuf32[i] <<= 8;
					pcmBuf32[i] >>= 8;
				}
			}
#else
			Len_i2s   = SPDIF_SPDIFDataToPCMData(SPDIF_RX_NUM,(int32_t *)SpdifBuf, Len_spdif, (int32_t *)PcmBuf, SPDIF_WORDLTH_16BIT);
#endif
			printf("%d,%d\n",DMA_CircularDataLenGet(SPDIF_RX_DMA_ID),DMA_CircularDataLenGet(PERIPHERAL_ID_I2S0_TX));
			DMA_CircularDataPut(PERIPHERAL_ID_I2S0_TX, PcmBuf, Len_i2s);
		}
	}
}

void SpdifTxExample(void)
{
	uint32_t samplerate = 44100;

	DBG("SpdifTxExample!\n");

	//spdif config
	if(samplerate == 22050 || samplerate == 44100 || samplerate == 88200 || samplerate == 88200)
	{
		Clock_PllLock(225792);
	}
	else
	{
		Clock_PllLock(245760);
	}
 	Clock_SpdifClkSelect(PLL_CLK_MODE);//B5 spdif out 只能使用DPLL

 	GPIO_PortAModeSet(SPDIF_TX_IO_PIN, SPDIF_TX_IO_SET);

	SPDIF_ModuleRst(SPDIF_TX_NUM);
	SPDIF_ModuleDisable(SPDIF_TX_NUM);

	SPDIF_TXInit(SPDIF_TX_NUM, 1, 1, 0, 10);
	DMA_CircularConfig(SPDIF_TX_DMA_ID, SPDIF_FIFO_LEN/2, SpdifAddr, SPDIF_FIFO_LEN);
	DMA_CircularFIFOClear(SPDIF_TX_DMA_ID);

	SPDIF_SampleRateSet(SPDIF_TX_NUM,samplerate);
	SPDIF_ModuleEnable(SPDIF_TX_NUM);
	DMA_ChannelEnable(SPDIF_TX_DMA_ID);

	while(1)
	{
		if(((DMA_CircularSpaceLenGet(SPDIF_TX_DMA_ID) / 8) * 8) >= 768 * 4)
		{
			int m;

#ifdef CFG_AUDIO_WIDTH_24BIT
			m = SPDIF_PCMDataToSPDIFData(SPDIF_TX_NUM, (int32_t *)SineBuf, 768, (int32_t *)SpdifBuf, SPDIF_WORDLTH_24BIT);
#else
			m = SPDIF_PCMDataToSPDIFData(SPDIF_TX_NUM, (int32_t *)SineBuf, 768, (int32_t *)SpdifBuf, SPDIF_WORDLTH_16BIT);
#endif
			DMA_CircularDataPut(SPDIF_TX_DMA_ID, SpdifBuf, m & 0xFFFFFFFC);
		}
	}
}


void AudioLineInInit(uint32_t SampleRate)
{
#ifdef CFG_AUDIO_WIDTH_24BIT
    AUDIO_BitWidth ADCBitWidth = ADC_WIDTH_24BITS;
#else
    AUDIO_BitWidth ADCBitWidth = ADC_WIDTH_16BITS;
#endif

    AudioADC_AnaInit(ADC0_MODULE, CHANNEL_LEFT, LINEIN1_LEFT, Single, ADCCommonEnergy, 17);
    AudioADC_AnaInit(ADC0_MODULE, CHANNEL_RIGHT, LINEIN1_RIGHT, Single, ADCCommonEnergy, 17);
    AudioADC_DigitalInit(ADC0_MODULE, SampleRate, ADCBitWidth, (void *)AudioADC2Buf, sizeof(AudioADC2Buf));

    AudioDAC_AllPowerOn(DAC_Single,DAC_NOLoad,PVDD33,DACCommonEnergy,Disable);
}

//SPDIF发送测试，发送ADC采集到的数据，同时DAC也播放ADC采集到的数据
//ADC模块信号输入，SPDIF信号输出，ADC和SPDIF共用AUPLL，测试是否有杂音或断音
void ADCInSpdifTxExample()
{
	//int32_t RealLen = 0;
	uint32_t samplerate = 44100;

    DBG("ADCInSpdifTxExample in!\n");

	if(samplerate == 22050 || samplerate == 44100 || samplerate == 88200 || samplerate == 88200)
	{
		Clock_PllLock(225792);
	}
	else
	{
		Clock_PllLock(245760);
	}
 	Clock_SpdifClkSelect(PLL_CLK_MODE);//B5 spdif out 只能使用DPLL

    GPIO_PortAModeSet(SPDIF_TX_IO_PIN, SPDIF_TX_IO_SET);
    DMA_CircularConfig(SPDIF_TX_DMA_ID, SPDIF_FIFO_LEN/2, SpdifAddr, SPDIF_FIFO_LEN);

    SPDIF_TXInit(SPDIF_TX_NUM, 1, 1, 0, 10);
    SPDIF_SampleRateSet(SPDIF_TX_NUM,samplerate);
    DMA_ChannelEnable(SPDIF_TX_DMA_ID);
    SPDIF_ModuleEnable(SPDIF_TX_NUM);

    AudioLineInInit(samplerate);

    while(1)
    {
        if(AudioADC0_DataLenGet() >= 256)
        {
            AudioADC0_DataGet(PcmBuf1,256);

            if(((DMA_CircularSpaceLenGet(SPDIF_TX_DMA_ID) / 8) * 8) >= 256 * 8)
            {
                int m;
#ifdef CFG_AUDIO_WIDTH_24BIT
                m = SPDIF_PCMDataToSPDIFData(SPDIF_TX_NUM,(int32_t *)PcmBuf1, 256 * 4, (int32_t *)SpdifBuf, SPDIF_WORDLTH_24BIT);
#else
                m = SPDIF_PCMDataToSPDIFData(SPDIF_TX_NUM,(int32_t *)PcmBuf1, 256 * 4, (int32_t *)SpdifBuf, SPDIF_WORDLTH_16BIT);
#endif
                DMA_CircularDataPut(SPDIF_TX_DMA_ID, (void *)SpdifBuf, m & 0xFFFFFFFC);
            }
        }
    }
}


int main(void)
{
	uint8_t key;
	uint8_t	UartPort = 1;

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

    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
    DbgUartInit(1, 2000000, 8, 0, 1);

	DBG("\nBuilt time: %s %s\n",__TIME__,__DATE__);

	GIE_ENABLE();

	DBG("\n");
	DBG("/-----------------------------------------------------\\\n");
	DBG("|                     SPDIF Example                     |\n");
	DBG("|      Mountain View Silicon Technology Co.,Ltd.      |\n");
	DBG("\\-----------------------------------------------------/\n");
	DBG("\n");
	DBG("please choose the example:\n");
	DBG("R: SpdifRxExample\n");
	DBG("T: SpdifTxExample\n");
	DBG("A: ADCInSpdifTxExample\n");
	DMA_ChannelAllocTableSet(DmaChannelMap);

	while(1)
	{
		key = 0;
		if(UARTS_Recv(UartPort, &key, 1,100) > 0)
		{
			switch(key)
			{
				case 'R':
					SpdifRxExample();
					break;
				case 'T':
					SpdifTxExample();
					break;
				case 'A':
					ADCInSpdifTxExample();
				default:
					break;
			}
		}
	}
}


