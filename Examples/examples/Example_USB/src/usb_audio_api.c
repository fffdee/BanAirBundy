/**
 **************************************************************************************
 * @file    usb_audio_mode.c
 * @brief
 *
 * @author  Shanks
 * @version V1.0.0
 *
 * $Created: 2025-1-22 10:16:10$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "irqn.h"
#include "gpio.h"
#include "dma.h"
#include "debug.h"
#include "delay.h"
#include "dac.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "otg_device_hcd.h"
#include "otg_device_audio.h"
#include "otg_device_standard_request.h"
#include "mcu_circular_buf.h"
#include "timer.h"
#include "resampler_polyphase.h"
#include "usb_audio_api.h"
#include "otg_device_audio.h"


#ifdef CFG_APP_USB_AUDIO_MODE_EN

uint32_t dac_play_buf[ONE_MS_SAMPLE*2];
uint32_t adc_play_buf[ONE_MS_SAMPLE*2];
//转采样
ResamplerPolyphaseContext UsbAudioSpeaker_Resampler;
ResamplerPolyphaseContext UsbAudioMic_Resampler;

uint8_t UsbAudioSpeaker_SRCOutBuf[USB_AUDIO_SRC_BUF_LEN];
uint8_t UsbAudioSpeaker_PCMBuffer[USB_FIFO_LEN];

uint8_t UsbAudioMic_SRCOutBuf[USB_AUDIO_SRC_BUF_LEN];
uint8_t UsbAudioMic_PCMBuffer[USB_FIFO_LEN];

uint32_t  usb_speaker_enable = 0;
uint32_t  usb_mic_enable = 0;

extern UsbAudio UsbAudioSpeaker;
extern UsbAudio UsbAudioMic;

//USB声卡模式参数配置，资源初始化
bool UsbDevicePlayInit(void)
{
	memset(&UsbAudioSpeaker,0,sizeof(UsbAudio));
	memset(&UsbAudioMic,0,sizeof(UsbAudio));
	UsbAudioSpeaker.Channels    = PACKET_CHANNELS_NUM;
	UsbAudioMic.Channels        = MIC_CHANNELS_NUM;
	UsbAudioSpeaker.LeftVol    = AUDIO_MAX_VOLUME;
	UsbAudioSpeaker.RightVol   = AUDIO_MAX_VOLUME;
	UsbAudioMic.LeftVol        = AUDIO_MAX_VOLUME;
	UsbAudioMic.RightVol       = AUDIO_MAX_VOLUME;
	return TRUE;
}

bool UsbDevicePlayResMalloc(void)
{
	APP_DBG("UsbDevicePlayResMalloc\n");
#ifdef CFG_RES_AUDIO_USB_IN_EN
	UsbAudioSpeaker.PCMBuffer = (int16_t *)UsbAudioSpeaker_PCMBuffer;
	memset(UsbAudioSpeaker.PCMBuffer, 0, USB_FIFO_LEN);
	MCUCircular_Config(&UsbAudioSpeaker.CircularBuf, UsbAudioSpeaker.PCMBuffer, USB_FIFO_LEN);
#endif

#ifdef CFG_RES_AUDIO_USB_OUT_EN
	UsbAudioMic.PCMBuffer = (int16_t *)UsbAudioMic_PCMBuffer;
	memset(UsbAudioMic.PCMBuffer, 0, USB_FIFO_LEN);
	MCUCircular_Config(&UsbAudioMic.CircularBuf, UsbAudioMic.PCMBuffer, USB_FIFO_LEN);
#endif
	return TRUE;
}

RESAMPLER_POLYPHASE_SRC_RATIO GetRatioEnum(uint32_t Scale1000);
void AudioCoreSinkChange(uint8_t Channels,uint32_t SampleRate)
{
#ifdef CFG_RES_AUDIO_USB_OUT_EN
	if(SampleRate != CFG_PARA_SAMPLE_RATE)
	{
		resampler_polyphase_init(&UsbAudioMic_Resampler, Channels,GetRatioEnum(1000 * CFG_PARA_SAMPLE_RATE / SampleRate));
	}
#endif
	MCUCircular_Config(&UsbAudioMic.CircularBuf, UsbAudioMic.PCMBuffer, USB_FIFO_LEN);
}

void AudioCoreSourceChange(uint8_t Channels,uint32_t SampleRate)
{
#ifdef CFG_RES_AUDIO_USB_IN_EN
	if(SampleRate != CFG_PARA_SAMPLE_RATE)
	{
		resampler_polyphase_init(&UsbAudioSpeaker_Resampler, Channels, GetRatioEnum(1000 * CFG_PARA_SAMPLE_RATE / SampleRate));
	}
#endif
	MCUCircular_Config(&UsbAudioSpeaker.CircularBuf, UsbAudioSpeaker.PCMBuffer, USB_FIFO_LEN);
}


static void DATA_1to2_apply24(int32_t *pcm_in, int32_t *pcm_out, int32_t n)
{
    int i;
    for (i = n - 1; i >= 0; i--)
    {
        pcm_out[i * 2] = pcm_in[i];
        pcm_out[i * 2 + 1] = pcm_in[i];
    }
}

void UsbAudioMicStreamProcess(void)
{
	int32_t SrcAudioLen;
	uint8_t *MIC_PCM;
	int32_t MIC_PCM_LEN;
	if(usb_mic_enable && AudioADC1_DataLenGet() > ONE_MS_SAMPLE)
	{
		AudioADC1_DataGet(adc_play_buf, ONE_MS_SAMPLE);
		if(UsbAudioMic.Channels == 2)
		{
			DATA_1to2_apply24((int32_t *)adc_play_buf, (int32_t *)adc_play_buf, ONE_MS_SAMPLE);  //单声道转双声道
		}

		if(UsbAudioMic.AudioSampleRate != CFG_PARA_SAMPLE_RATE)
		{
#ifdef CFG_AUDIO_WIDTH_24BIT
			SrcAudioLen = resampler_polyphase_apply24(&UsbAudioMic_Resampler, (int32_t *)adc_play_buf, (int32_t *)UsbAudioMic_SRCOutBuf, ONE_MS_SAMPLE);
#else
			SrcAudioLen = resampler_polyphase_apply(&UsbAudioMic_Resampler, (int16_t *)adc_play_buf, (int16_t *)UsbAudioMic_SRCOutBuf, ONE_MS_SAMPLE);
#endif
			MIC_PCM = (uint8_t*)UsbAudioMic_SRCOutBuf;
			MIC_PCM_LEN = SrcAudioLen;
		}
		else
		{
			MIC_PCM = (uint8_t*)adc_play_buf;
			MIC_PCM_LEN = ONE_MS_SAMPLE;
		}

		UsbAudioMicDataSet(MIC_PCM,MIC_PCM_LEN);
	}
}
void UsbAudioSpeakerStreamProcess(void)
{
	int32_t SrcAudioLen;
	uint8_t *DAC_PCM;
	int32_t DAC_PCM_LEN;

	if(usb_speaker_enable && UsbAudioSpeakerDataLenGet() >= ONE_MS_SAMPLE)
	{
		UsbAudioSpeakerDataGet(dac_play_buf,ONE_MS_SAMPLE);

		if(UsbAudioSpeaker.AudioSampleRate != CFG_PARA_SAMPLE_RATE)
		{
	#ifdef CFG_AUDIO_WIDTH_24BIT
			SrcAudioLen = resampler_polyphase_apply24(&UsbAudioSpeaker_Resampler, (int32_t *)dac_play_buf, (int32_t *)UsbAudioSpeaker_SRCOutBuf, ONE_MS_SAMPLE);
	#else
			SrcAudioLen = resampler_polyphase_apply(&UsbAudioSpeaker_Resampler, (int16_t *)dac_play_buf, (int16_t *)UsbAudioSpeaker_SRCOutBuf, ONE_MS_SAMPLE);
	#endif
			DAC_PCM = (uint8_t*)UsbAudioSpeaker_SRCOutBuf;
			DAC_PCM_LEN = SrcAudioLen;
		}
		else
		{
			DAC_PCM = (uint8_t*)dac_play_buf;
			DAC_PCM_LEN = ONE_MS_SAMPLE;
		}

		AudioDAC0_DataSet(DAC_PCM, DAC_PCM_LEN);
	}
	if(!usb_speaker_enable && AudioDAC0_DataSpaceLenGet() > ONE_MS_SAMPLE)
	{
		memset(dac_play_buf,0,sizeof(dac_play_buf));
		AudioDAC0_DataSet(dac_play_buf, ONE_MS_SAMPLE);
	}
}


RESAMPLER_POLYPHASE_SRC_RATIO GetRatioEnum(uint32_t Scale1000)
{
	switch(Scale1000)
	{
		case (1000 * 6):
			return RESAMPLER_POLYPHASE_SRC_RATIO_6_1;
		case (1000 * 441 / 80):
			return RESAMPLER_POLYPHASE_SRC_RATIO_441_80;
		case (1000 * 640 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_640_147;
		case (1000 * 4):
			return RESAMPLER_POLYPHASE_SRC_RATIO_4_1;
		case (1000 * 147 / 40):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_40;
		case (1000 * 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_3_1;
		case (1000 * 441 / 160):
			return RESAMPLER_POLYPHASE_SRC_RATIO_441_160;
		case (1000 * 320 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_320_147;
		case (1000 * 2):
			return RESAMPLER_POLYPHASE_SRC_RATIO_2_1;
		case (1000 * 147 / 80):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_80;
		case (1000 * 3 / 2):
			return RESAMPLER_POLYPHASE_SRC_RATIO_3_2;
		case (1000 * 441 / 320):
			return RESAMPLER_POLYPHASE_SRC_RATIO_441_320;
		case (1000 * 4 / 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_4_3;
		case (1000 * 160 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_160_147;
/*****************************************************************/
		case (1000 * 147 / 160):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_160;
		case (1000 * 3 / 4):
			return RESAMPLER_POLYPHASE_SRC_RATIO_3_4;
		case (1000 * 80 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_80_147;
		case (1000 / 2):
			return RESAMPLER_POLYPHASE_SRC_RATIO_1_2;
		case (1000 * 147 / 320):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_320;
		case (1000 * 160 / 441):
			return RESAMPLER_POLYPHASE_SRC_RATIO_160_441;
		case (1000 / 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_1_3;
		case (1000 * 40 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_40_147;
		case (1000 / 4):
			return RESAMPLER_POLYPHASE_SRC_RATIO_1_4;
		case (1000 * 147 / 640):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_640;
		case (1000 * 320 / 441):
			return RESAMPLER_POLYPHASE_SRC_RATIO_320_441;
		case (1000 * 2 / 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_2_3;
		default:
#ifdef AUDIO_CORE_DEBUG
			if(Scale1000 != 1000)
			{
				DBG("SRC Samplerate Error！\n");
			}
#endif
			return RESAMPLER_POLYPHASE_SRC_RATIO_UNSUPPORTED;
	}
}

#endif

