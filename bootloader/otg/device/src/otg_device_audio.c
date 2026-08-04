/**
 *****************************************************************************
 * @file     otg_device_audio.c
 * @author   Owen
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    device audio module driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */
#include <stdio.h>
#include <string.h>
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "otg_device_audio.h"
#include "usb_audio_api.h"


#ifdef CFG_APP_USB_AUDIO_MODE_EN
#define PCM_LEN_SIZE(a,b,c) (a*b*c/1000)
#define PCM_LEN_MAX MAX(PCM_LEN_SIZE(USBD_AUDIO_FREQ,PCM24BIT,PACKET_CHANNELS_NUM),PCM_LEN_SIZE(USBD_AUDIO_MIC_FREQ,PCM24BIT,MIC_CHANNELS_NUM))
#define OUT_PCM_LEN (USBD_AUDIO_FREQ/1000)*PACKET_CHANNELS_NUM*PCM24BIT
extern uint8_t Setup[];
extern uint8_t Request[];
extern void OTG_DeviceSendResp(uint16_t Resp, uint8_t n);

#ifdef CFG_AUDIO_WIDTH_24BIT
#define USB_AUDIO_WIDTH	24
typedef int32_t pcm_int;
typedef int64_t gain_int;
#else
#define USB_AUDIO_WIDTH	16
typedef int16_t pcm_int;
typedef int32_t gain_int;
#endif
UsbAudio UsbAudioSpeaker;
UsbAudio UsbAudioMic;

uint8_t iso_buf[PCM_LEN_MAX*4/3];
/////////////////////////////////////////
/**
 * @brief  USB声卡模式下，发送反向控制命令
 * @param  Cmd 反向控制命令
 * @return 1-成功，0-失败
 */
#define AUDIO_STOP        BIT(7) 
#define AUDIO_PP          BIT(6) 

#define AUDIO_MUTE        BIT(4)

#define AUDIO_NEXT        BIT(2) 
#define AUDIO_PREV        BIT(3) 

#define AUDIO_VOL_UP      BIT(0) 
#define AUDIO_VOL_DN      BIT(1)

/////////////////////////
void PCAudioStop(void)
{
	OTG_DeviceAudioSendPcCmd(AUDIO_STOP);
}
void PCAudioPP(void)
{
	OTG_DeviceAudioSendPcCmd(AUDIO_PP);
}
void PCAudioNext(void)
{
	OTG_DeviceAudioSendPcCmd(AUDIO_NEXT);
}
void PCAudioPrev(void)
{
	OTG_DeviceAudioSendPcCmd(AUDIO_PREV);
}

void PCAudioVolUp(void)
{
	OTG_DeviceAudioSendPcCmd(AUDIO_VOL_UP);
}

void PCAudioVolDn(void)
{
	OTG_DeviceAudioSendPcCmd(AUDIO_VOL_DN);
}

bool OTG_DeviceAudioSendPcCmd(uint8_t Cmd)
{
	OTG_DeviceInterruptSend(0x01,&Cmd, 1,1000);
	Cmd = 0;
	OTG_DeviceInterruptSend(0x01,&Cmd, 1,1000);
	return TRUE;
}
//////////////////////////////////////////////////audio core api/////////////////////////////////////////////////////
//pc->chip 从缓存区获取数据
uint16_t UsbAudioSpeakerDataGet(void *Buffer, uint16_t Len)
{
	uint16_t Length = 0;

	if(!UsbAudioSpeaker.PCMBuffer)
	{
		return 0;
	}
	Length = Len * sizeof(pcm_int) * PACKET_CHANNELS_NUM;
	Length = MCUCircular_GetData(&UsbAudioSpeaker.CircularBuf, Buffer, Length);
	return Length / (sizeof(pcm_int) * PACKET_CHANNELS_NUM);
}

//pc->chip 获取缓存区数据长度
uint16_t UsbAudioSpeakerDataLenGet(void)
{
	uint16_t Len;

	if(!UsbAudioSpeaker.PCMBuffer)
	{
		return 0;
	}
	Len = MCUCircular_GetDataLen(&UsbAudioSpeaker.CircularBuf);
	Len = Len / (sizeof(pcm_int) * PACKET_CHANNELS_NUM);
	return Len;
}
uint16_t UsbAudioSpeakerDepthGet(void)
{
	uint16_t Len;
	Len = UsbAudioSpeaker.CircularBuf.BufDepth;
	Len = Len / (sizeof(pcm_int) * PACKET_CHANNELS_NUM);
//	printf(" UsbAudioSpeaker BufDepth: %d\r\n",Len);
	return Len;
}
//chip->pc 保存数据到缓存区
uint16_t UsbAudioMicDataSet(void *Buffer, uint16_t Len)
{
#ifdef CFG_RES_AUDIO_USB_OUT_EN
	if(!UsbAudioMic.PCMBuffer)
	{
		return 0;
	}

	MCUCircular_PutData(&UsbAudioMic.CircularBuf, Buffer, Len * sizeof(pcm_int) * UsbAudioMic.Channels);
#endif
	return Len;
}

//chip->pc 数据缓存区剩余空间
uint16_t UsbAudioMicSpaceLenGet(void)
{
	uint16_t Len;

	if(!UsbAudioMic.PCMBuffer)
	{
		return 0;
	}
	Len = MCUCircular_GetSpaceLen(&UsbAudioMic.CircularBuf);
	Len = Len / (sizeof(pcm_int) * UsbAudioMic.Channels);
	return Len;
}

uint16_t UsbAudioMicDepthGet(void)
{
	uint16_t Len;
	Len = UsbAudioMic.CircularBuf.BufDepth;
	Len = Len / (sizeof(pcm_int) * MIC_CHANNELS_NUM);
//	printf(" UsbAudioMic BufDepth: %d\r\n",Len);
	return Len;
}

//转采样直接在中断中处理，转采样时间大约是180us。
//注意一下需要4字节对齐
void OnDeviceAudioRcvIsoPacket(void)
{
#ifdef CFG_RES_AUDIO_USB_IN_EN
	uint32_t Len;
	int32_t s;
	int32_t left_pregain = UsbAudioSpeaker.LeftVol;
	int32_t rigth_pregain = UsbAudioSpeaker.RightVol;
	uint32_t channel = UsbAudioSpeaker.Channels;
	uint32_t sample = 0;
	OTG_DeviceISOReceive(DEVICE_ISO_OUT_EP, (uint8_t*)iso_buf, OUT_PCM_LEN, &Len);

	UsbAudioSpeaker.FramCount++;

	if(UsbAudioSpeaker.ByteSet == PCM16BIT)
	{
		sample = Len/PCM16BIT;
#ifdef CFG_AUDIO_WIDTH_24BIT
		for(s = sample-1; s >= 0; s--)
		{
//			memcpy(&iso_buf[s*4+1],&iso_buf[s*2],4);
			iso_buf[s*4+2] = iso_buf[s*2+1];
			iso_buf[s*4+1] = iso_buf[s*2];

			if(iso_buf[s*4+2] & 0x80)
			{
				iso_buf[s*4+3] = 0xff;
			}else{
				iso_buf[s*4+3] = 0x00;
			}
			iso_buf[s*4] = 0x00;
		}
#endif
	}
	else if(UsbAudioSpeaker.ByteSet == PCM24BIT)
	{
		sample = Len/PCM24BIT;
#ifdef CFG_AUDIO_WIDTH_24BIT
		for(s = sample-1; s >= 0; s--)
		{
//			memcpy(&iso_buf[s*4],&iso_buf[s*3],4);
			iso_buf[s*4+2] = iso_buf[s*3+2];
			iso_buf[s*4+1] = iso_buf[s*3+1];
			iso_buf[s*4] = iso_buf[s*3];

			if(iso_buf[s*4+2] & 0x80)
			{
				iso_buf[s*4+3] = 0xff;
			}else{
				iso_buf[s*4+3] = 0x00;
			}
		}
#else
		for(s = 0; s < sample; s++)
		{
			iso_buf[s*2] = iso_buf[s*3+1];
			iso_buf[s*2+1] = iso_buf[s*3+2];
		}
#endif
	}
#ifdef CFG_RES_AUDIO_USB_VOL_SET_EN
	if(UsbAudioSpeaker.Mute)
	{
		left_pregain = 0;
		rigth_pregain = 0;
	}
	for(s = 0; s<sample/channel; s++)
	{
		pcm_int *pcm = (pcm_int *)iso_buf;
		if(channel == 2)//立体声
		{
			pcm[2 * s + 0] = __nds32__clips(((((gain_int)pcm[2 * s + 0]) * left_pregain + 2048) >> 12), (USB_AUDIO_WIDTH)-1);
			pcm[2 * s + 1] = __nds32__clips(((((gain_int)pcm[2 * s + 1]) * rigth_pregain + 2048) >> 12), (USB_AUDIO_WIDTH)-1);
		}
		else
		{
			pcm[s] = __nds32__clips(((((gain_int)pcm[s]) * left_pregain + 2048) >> 12), (USB_AUDIO_WIDTH)-1);
		}
	}
#endif
	if(UsbAudioSpeaker.PCMBuffer == NULL)
	{
		return;
	}

	MCUCircular_PutData(&UsbAudioSpeaker.CircularBuf, (uint8_t*)iso_buf, sample*sizeof(pcm_int));
#endif
}

void OnDeviceAudioSendIsoPacket(void)
{
#ifdef CFG_RES_AUDIO_USB_OUT_EN
	int32_t s;
	int32_t left_pregain = UsbAudioMic.LeftVol;
	int32_t rigth_pregain = UsbAudioMic.RightVol;
	uint32_t channel = UsbAudioMic.Channels;
	uint32_t RealLen = 0;

	UsbAudioMic.FramCount++;
	RealLen = 44;
	if(UsbAudioMic.AudioSampleRate == 44100)
	{
		RealLen = 44;
		if((UsbAudioMic.FramCount%10) == 0)
		{
			RealLen = 45;
		}
	}
	else
	{
		RealLen = UsbAudioMic.AudioSampleRate/1000;
	}
	if(UsbAudioMic.FramCount < (UsbAudioMic.AudioSampleRate/CFG_PARA_SAMPLE_RATE)*10)
	{
		memset(iso_buf,0,RealLen*sizeof(pcm_int)*channel);
	}
	else if(UsbAudioMic.PCMBuffer != NULL)
	{
		if(MCUCircular_GetDataLen(&UsbAudioMic.CircularBuf) < RealLen*sizeof(pcm_int)*channel)
		{
			memset(iso_buf,0,RealLen*sizeof(pcm_int)*channel);
		}
		else
		{
			MCUCircular_GetData(&UsbAudioMic.CircularBuf, iso_buf,RealLen*sizeof(pcm_int)*channel);
		}
	}

#ifdef CFG_RES_AUDIO_USB_VOL_SET_EN
	if(UsbAudioMic.Mute)
	{
		left_pregain = 0;
		rigth_pregain = 0;
	}
	for(s = 0; s<RealLen; s++)
	{
		pcm_int *pcm = (pcm_int *)iso_buf;
		if(channel == 2)//立体声
		{
			pcm[2 * s + 0] = __nds32__clips(((((gain_int)pcm[2 * s + 0]) * left_pregain + 2048) >> 12), (USB_AUDIO_WIDTH)-1);
			pcm[2 * s + 1] = __nds32__clips(((((gain_int)pcm[2 * s + 1]) * rigth_pregain + 2048) >> 12), (USB_AUDIO_WIDTH)-1);
		}
		else
		{
			pcm[s] = __nds32__clips(((((gain_int)pcm[s]) * left_pregain + 2048) >> 12), (USB_AUDIO_WIDTH)-1);
		}
	}
#endif

	if(UsbAudioMic.ByteSet == PCM16BIT)
	{
#ifdef CFG_AUDIO_WIDTH_24BIT
		int32_t *PcmBuf32 = (int32_t *)iso_buf;
		int16_t *PcmBuf16 = (int16_t *)iso_buf;
		for(s=0; s < RealLen*channel; s++)
		{
			PcmBuf16[s] = PcmBuf32[s] >> 8;
		}
#endif
	}
	else if(UsbAudioMic.ByteSet == PCM24BIT)
	{
#ifdef CFG_AUDIO_WIDTH_24BIT
		for(s = 0; s<RealLen*channel; s++)
		{
			memcpy(&iso_buf[s*3],&iso_buf[s*4],4);
		}
#else
		for(s = RealLen*channel-1; s >= 0; s--)
		{
			iso_buf[s*3+2] = iso_buf[s*2+1];
			iso_buf[s*3+1] = iso_buf[s*2];
		}
#endif
	}
	OTG_DeviceISOSend(DEVICE_ISO_IN_EP,(uint8_t*)iso_buf,RealLen*UsbAudioMic.ByteSet*channel);
#endif
}

void OTG_DeviceAudioInit()
{

}

#if (USB_AUDIO_PROTOCOL == AUDIO_UAC_10)
void OTG_DeviceAudioRequest(void)
{
	//AUDIO控制接口组件ID号定义（必须与device_stor_audio_request.c中的定义保持一致！）
	#define AUDIO_SPEAKER_IT_ID		1
	#define AUDIO_SPEAKER_FU_ID		2	//控制MUTE、VOLUME
	#define AUDIO_SPEAKER_OT_ID		3
//	#define AUDIO_MIC_IT_ID			4
//	#define AUDIO_MIC_FU_ID			5
//	#define AUDIO_MIC_SL_ID			6
//	#define AUDIO_MIC_OT_ID			7
	
	#define AudioCmd	((Setup[0] << 8) | Setup[1])
	#define Channel		Setup[2]
	#define Control		Setup[3]
	#define Entity		Setup[5]
	
	#define SET_CUR		0x2101
	#define SET_IDLE	0x210A
	#define GET_CUR		0xA181
	#define GET_MIN		0xA182
	#define GET_MAX		0xA183
	#define GET_RES		0xA184
	
	#define SET_CUR_EP	0x2201
	#define GET_CUR_EP	0xA281

	//AUDIO类请求处理
	if(AudioCmd == SET_CUR_EP)
	{
		if(Setup[4] == DEVICE_ISO_IN_EP)
		{
			if(UsbAudioMic.AudioSampleRate != SWAP_BUF_TO_U32(Request))
			{
				UsbAudioMic.AudioSampleRate = SWAP_BUF_TO_U32(Request);
				printf("UsbAudioMic.AudioSampleRate:%u\n",(unsigned int)UsbAudioMic.AudioSampleRate);
				AudioCoreSinkChange(UsbAudioMic.Channels, UsbAudioMic.AudioSampleRate);
			}
		}
		else if(Setup[4] == DEVICE_ISO_OUT_EP)
		{
			if(UsbAudioSpeaker.AudioSampleRate != SWAP_BUF_TO_U32(Request))
			{
				UsbAudioSpeaker.AudioSampleRate = SWAP_BUF_TO_U32(Request);
				printf("UsbAudioSpeaker.AudioSampleRate:%u\n",(unsigned int)UsbAudioSpeaker.AudioSampleRate);
				AudioCoreSourceChange(UsbAudioSpeaker.Channels, UsbAudioSpeaker.AudioSampleRate);
			}
		}
		return;
	}
	if(AudioCmd == GET_CUR_EP)
	{
		uint32_t Temp = 0;
		if(Setup[4] == DEVICE_ISO_IN_EP)
		{
			Temp = UsbAudioMic.AudioSampleRate;
		}
		else if(Setup[4] == DEVICE_ISO_OUT_EP)
		{
			Temp = UsbAudioSpeaker.AudioSampleRate;
		}
		Setup[0] = (Temp>>0 ) & 0x000000FF;
		Setup[1] = (Temp>>8 ) & 0x000000FF;
		Setup[2] = (Temp>>16) & 0x000000FF;
		OTG_DeviceControlSend(Setup,3,3);
		return;
	}

	if((Entity == AUDIO_SPEAKER_FU_ID) && (Control == AUDIO_CONTROL_MUTE))
	{
		//Speaker mute的操作
		if(AudioCmd == GET_CUR)
		{
			Setup[0] = UsbAudioSpeaker.Mute;
			OTG_DeviceControlSend(Setup,1,3);
		}
		else if(AudioCmd == SET_CUR)
		{
			//APP_DBG("Set speaker mute: %d\n", Request[0]);
			UsbAudioSpeaker.Mute=Request[0];
		}
		else
		{
			//APP_DBG("%s %d\n",__FILE__,__LINE__);
		}
	}
	else if((Entity == AUDIO_SPEAKER_FU_ID) && (Control == AUDIO_CONTROL_VOLUME))
	{
		//Speaker volume的操作
		if(AudioCmd == GET_MIN)
		{
			//APP_DBG("Get speaker min volume\n");
			OTG_DeviceSendResp(AUDIO_MIN_VOLUME, 2);
		}
		else if(AudioCmd == GET_MAX)
		{
			//APP_DBG("Get speaker max volume\n");
			OTG_DeviceSendResp(AUDIO_MAX_VOLUME, 2);
		}
		else if(AudioCmd == GET_RES)
		{
			//APP_DBG("Get speaker res volume\n");
			OTG_DeviceSendResp(AUDIO_RES_VOLUME, 2);
		}
		else if(AudioCmd == GET_CUR)
		{
			uint32_t Vol = 0;
			if(Channel == 0x01)
			{
				Vol = UsbAudioSpeaker.LeftVol;//UsbAudioSpeaker.FuncLeftVolGet();
			}
			else
			{
				Vol = UsbAudioSpeaker.RightVol;//UsbAudioSpeaker.FuncRightVolGet();
			}
			OTG_DeviceSendResp(Vol, 2);
		}
		else if(AudioCmd == SET_CUR)
		{
			uint32_t Temp = 0;
			Temp = Request[1]* 256 + Request[0];
			if(Setup[2] == 0x01)
			{
				UsbAudioSpeaker.LeftVol = Temp;
			}
			else
			{
				UsbAudioSpeaker.RightVol = Temp;
			}
		}
		else
		{
			//APP_DBG("%s %d\n",__FILE__,__LINE__);
		}
	}
	else if((Entity == AUDIO_MIC_FU_ID) && (Control == AUDIO_CONTROL_MUTE))
	{
		//Mic mute的操作
		if(AudioCmd == GET_CUR)
		{
			OTG_DeviceSendResp(UsbAudioMic.Mute, 1);
		}
		else if(AudioCmd == SET_CUR)
		{
			UsbAudioMic.Mute = Request[0];
		}
		else
		{
			//APP_DBG("%s %d\n",__FILE__,__LINE__);
		}
	}
	else if((Entity == AUDIO_MIC_FU_ID) && (Control == AUDIO_CONTROL_VOLUME))
	{
		//Mic volume的操作
		if(AudioCmd == GET_MIN)
		{
			//APP_DBG("Get mic min volume\n");
			OTG_DeviceSendResp(AUDIO_MIN_VOLUME, 2);
		}
		else if(AudioCmd == GET_MAX)
		{
			OTG_DeviceSendResp(AUDIO_MAX_VOLUME, 2);	//此处乘以4的原因请看本文件开头的注释说明
		}
		else if(AudioCmd == GET_RES)
		{
			//APP_DBG("Get mic res volume\n");
			OTG_DeviceSendResp(AUDIO_RES_VOLUME, 2);
		}
		else if(AudioCmd == GET_CUR)
		{
			uint32_t Vol = 0;
			if(Channel == 0x01)
			{
				Vol = UsbAudioMic.LeftVol;
			}
			else
			{
				Vol = UsbAudioMic.RightVol;
			}
			OTG_DeviceSendResp(Vol, 2);
		}
		else if(AudioCmd == SET_CUR)
		{
			uint32_t Vol = Request[1] * 256 + Request[0];
			if(Setup[2] == 0x01)
			{
				UsbAudioMic.LeftVol = Vol;
			}
			else
			{
				UsbAudioMic.RightVol = Vol;
			}
		}
		else
		{
			//APP_DBG("%s %d\n",__FILE__,__LINE__);
		}
	}
	else if(Entity == AUDIO_MIC_SL_ID)
	{
		//Selector的操作
		if(AudioCmd == GET_CUR)
		{
			//APP_DBG("Get selector: 1\n");
			OTG_DeviceSendResp(0x01, 1);
		}
		else
		{
			//APP_DBG("%s %d\n",__FILE__,__LINE__);
		}
	}
	else if(AudioCmd == SET_IDLE)
	{
		//APP_DBG("Set idle\n");
	}	
	else
	{
		//其他AUDIO类的输入请求
		OTG_DeviceSendResp(0x0000, 1);
	}
}
#elif(USB_AUDIO_PROTOCOL == AUDIO_UAC_20)
void OTG_DeviceAudioRequest(void)
{
	//AUDIO控制接口组件ID号定义（必须与device_stor_audio_request.c中的定义保持一致！）
	#define SET_CUR		0x2101
	#define SET_IDLE	0x210A
	#define GET_CUR		0xA181
	#define GET_MIN		0xA182
	#define GET_MAX		0xA183
	#define GET_RES		0xA184

	#define SET_CUR_EP	0x2201
	#define GET_CUR_EP	0xA281
	//uac 2.0
	#define GET_RANGE	0xA102
	#define GET_CUR_2	0xA101

	#define AudioCmd	((Setup[0] << 8) | Setup[1])
	#define Channel		Setup[2]
	#define Control		Setup[3]
	#define Entity		Setup[5]
	#define wLength		((Setup[7] << 8) | Setup[6])
	/*
	 * uac2.0  指令解析开始
	 */
	if((Entity == AUDIO_FU_ID))
	{
		if(Control == 0x01)
		{
			//Speaker mute的操作
			if(AudioCmd == GET_CUR_2)
			{
				Setup[0] = UsbAudioSpeaker.Mute;
				OTG_DeviceControlSend(Setup,1,3);
			}
			else if(AudioCmd == SET_CUR)
			{
				APP_DBG("Set speaker mute: %d\n", Request[0]);
				UsbAudioSpeaker.Mute=Request[0];
			}
			else
			{
				//APP_DBG("%s %d\n",__FILE__,__LINE__);
			}
		}
		else if(Control == 0x02)
		{
			if(AudioCmd == GET_RANGE)
			{
				uint8_t AudioCtl[8];
				AudioCtl[0] = 1;    //wNumSubRanges
				AudioCtl[1] = 0;
				AudioCtl[2] = (uint8_t)(AUDIO_MIN_VOLUME);    //wMIN(1)
				AudioCtl[3] = (uint8_t)(AUDIO_MIN_VOLUME >> 8);
				AudioCtl[4] = (uint8_t)(AUDIO_MAX_VOLUME);  //wMAX(1)
				AudioCtl[5] = (uint8_t)(AUDIO_MAX_VOLUME >> 8);
				AudioCtl[6] = (uint8_t)(AUDIO_RES_VOLUME);    //wRES(1)
				AudioCtl[7] = (uint8_t)(AUDIO_RES_VOLUME >> 8);
				OTG_DeviceControlSend(AudioCtl, sizeof(AudioCtl),3);
			}
			else if(AudioCmd == GET_CUR_2)
			{
				int16_t Vol = 0;
				if(Channel == 0x01)
				{
					Vol = UsbAudioSpeaker.LeftVol;//UsbAudioSpeaker.FuncLeftVolGet();
				}
				else
				{
					Vol = UsbAudioSpeaker.RightVol;//UsbAudioSpeaker.FuncRightVolGet();
				}
				OTG_DeviceSendResp(Vol, 2);
			}
			else if(AudioCmd == SET_CUR)
			{
				uint32_t Temp = 0;
				Temp = Request[1]* 256 + Request[0];
				if(Setup[2] == 0x01)
				{
//					UsbAudioSpeaker.LeftVol = Temp&0x7fff;
					UsbAudioSpeaker.LeftVol = Temp;
				}
				else
				{
//					UsbAudioSpeaker.RightVol = Temp&0x7fff;
					UsbAudioSpeaker.RightVol = Temp;
				}
			}
		}
		return ;
	}
	else if((Entity == AUDIO_MIC_FU_ID))
	{
		if(Control == 0x01)
		{
			//Speaker mute的操作
			if(AudioCmd == GET_CUR_2)
			{
				Setup[0] = UsbAudioMic.Mute;
				OTG_DeviceControlSend(Setup,1,3);
			}
			else if(AudioCmd == SET_CUR)
			{
				APP_DBG("Set UsbAudioMic mute: %d\n", Request[0]);
				UsbAudioMic.Mute=Request[0];
			}
			else
			{
				//APP_DBG("%s %d\n",__FILE__,__LINE__);
			}
		}
		else if(Control == 0x02)
		{
			if(AudioCmd == GET_RANGE)
			{
				uint8_t AudioCtl[8];
				AudioCtl[0] = 1;    //wNumSubRanges
				AudioCtl[1] = 0;
				AudioCtl[2] = (uint8_t)(AUDIO_MIN_VOLUME);    //wMIN(1)
				AudioCtl[3] = (uint8_t)(AUDIO_MIN_VOLUME >> 8);
				AudioCtl[4] = (uint8_t)(AUDIO_MAX_VOLUME);  //wMAX(1)
				AudioCtl[5] = (uint8_t)(AUDIO_MAX_VOLUME >> 8);
				AudioCtl[6] = (uint8_t)(AUDIO_RES_VOLUME);    //wRES(1)
				AudioCtl[7] = (uint8_t)(AUDIO_RES_VOLUME >> 8);
				OTG_DeviceControlSend(AudioCtl, sizeof(AudioCtl),3);
			}
			else if(AudioCmd == GET_CUR_2)
			{
				int16_t Vol = 0;
				if(Channel == 0x01)
				{
					Vol = UsbAudioMic.LeftVol;//UsbAudioSpeaker.FuncLeftVolGet();
				}
				else
				{
					Vol = UsbAudioMic.RightVol;//UsbAudioSpeaker.FuncRightVolGet();
				}
				OTG_DeviceSendResp(Vol, 2);
			}
			else if(AudioCmd == SET_CUR)
			{
				uint32_t Temp = 0;
				Temp = Request[1]* 256 + Request[0];
				if(Setup[2] == 0x01)
				{
//					UsbAudioMic.LeftVol = Temp&0x7fff;
					UsbAudioMic.LeftVol = Temp;
				}
				else
				{
//					UsbAudioMic.RightVol = Temp&0x7fff;
					UsbAudioMic.RightVol = Temp;
				}
			}
		}
		return ;
	}
	else if(Entity == AUDIO_MIC_CLK_ID)
	{
		if(AudioCmd == 0x2101)
		{
			UsbAudioMic.AudioSampleRate = SWAP_BUF_TO_U32(Request);//Request[1]*256 + Request[0];
			APP_DBG("UsbAudioMic.AudioSampleRate:%u\n",(unsigned int)UsbAudioMic.AudioSampleRate);
			AudioCoreSinkChange(USB_AUDIO_SINK_NUM, UsbAudioMic.Channels, UsbAudioMic.AudioSampleRate);
		}
		else if(AudioCmd == GET_CUR_2)
		{
			if(UsbAudioMic.AudioSampleRate == 0)
			{
				UsbAudioMic.AudioSampleRate = USBD_AUDIO_MIC_FREQ2;
			}
			OTG_DeviceControlSend((uint8_t*)&UsbAudioMic.AudioSampleRate, wLength,3);
		}
		else if(AudioCmd == GET_RANGE)
		{
			//Get Layout 3 parameter block
			uint8_t para_block[] = {
				SAMPLE_FREQ_NUM(4),           /* wNumSubRanges */

                SAMPLE_FREQ_4B(USBD_AUDIO_MIC_FREQ2),        /* dMIN(1) */
                SAMPLE_FREQ_4B(USBD_AUDIO_MIC_FREQ2),        /* dMAX(1) */
                SAMPLE_FREQ_4B(0x00),         /* dRES(1) */

                SAMPLE_FREQ_4B(44100),        /* dMIN(1) */
                SAMPLE_FREQ_4B(44100),        /* dMAX(1) */
                SAMPLE_FREQ_4B(0x00),         /* dRES(1) */

                SAMPLE_FREQ_4B(USBD_AUDIO_MIC_FREQ1),        /* dMIN(1) */
                SAMPLE_FREQ_4B(USBD_AUDIO_MIC_FREQ1),        /* dMAX(1) */
                SAMPLE_FREQ_4B(0x00),         /* dRES(1) */

                SAMPLE_FREQ_4B(USBD_AUDIO_MIC_FREQ),        /* dMIN(2) */
                SAMPLE_FREQ_4B(USBD_AUDIO_MIC_FREQ),        /* dMAX(2) */
                SAMPLE_FREQ_4B(0x00),         /* dRES(2) */

			};
            if(wLength > sizeof(para_block))
            {
                OTG_DeviceControlSend(para_block, sizeof(para_block),3);
            }
            else
            {
            	OTG_DeviceControlSend(para_block, wLength,3);
            }
		}

		return ;
	}
	else if(Entity == AUDIO_CLK_ID)
	{
		if(AudioCmd == 0x2101)
		{
			UsbAudioSpeaker.AudioSampleRate = SWAP_BUF_TO_U32(Request);//Request[1]*256 + Request[0];
			APP_DBG("UsbAudioSpeaker.AudioSampleRate:%u\n",(unsigned int)UsbAudioSpeaker.AudioSampleRate);
			AudioCoreSourceChange(USB_AUDIO_SOURCE_NUM, UsbAudioSpeaker.Channels, UsbAudioSpeaker.AudioSampleRate);
		}
		else if(AudioCmd == GET_CUR_2)
		{
			if(UsbAudioSpeaker.AudioSampleRate == 0)
			{
				UsbAudioSpeaker.AudioSampleRate = CFG_PARA_SAMPLE_RATE;
			}
			OTG_DeviceControlSend((uint8_t*)&UsbAudioSpeaker.AudioSampleRate, wLength,3);
		}
		else if(AudioCmd == GET_RANGE)
		{
            //Get Layout 3 parameter block
            uint8_t para_block[] = {
				SAMPLE_FREQ_NUM(2),           /* wNumSubRanges */

				SAMPLE_FREQ_4B(USBD_AUDIO_FREQ),        /* dMIN(1) */
				SAMPLE_FREQ_4B(USBD_AUDIO_FREQ),        /* dMAX(1) */
				SAMPLE_FREQ_4B(0x00),         /* dRES(1) */

                SAMPLE_FREQ_4B(USBD_AUDIO_FREQ1),        /* dMIN(2) */
                SAMPLE_FREQ_4B(USBD_AUDIO_FREQ1),        /* dMAX(2) */
                SAMPLE_FREQ_4B(0x00),         /* dRES(2) */
            };
            if(wLength > sizeof(para_block))
            {
                OTG_DeviceControlSend(para_block, sizeof(para_block),3);
            }else{
            	OTG_DeviceControlSend(para_block, wLength,3);
            }

		}

		return ;
	}
}
#endif

extern uint32_t  usb_speaker_enable;
extern uint32_t  usb_mic_enable;
static uint32_t FramCount = 0;
void UsbAudioTimer1msProcess(void)
{
	FramCount++;
	if(FramCount % 2)//2ms
	{
		return;
	}
#ifdef CFG_RES_AUDIO_USB_IN_EN
	if(UsbAudioSpeaker.AltSet)//open stream
	{
		if(UsbAudioSpeaker.FramCount)//正在传数据 1-2帧数据
		{
			if(UsbAudioSpeaker.FramCount != UsbAudioSpeaker.TempFramCount)
			{
				UsbAudioSpeaker.TempFramCount = UsbAudioSpeaker.FramCount;
				usb_speaker_enable = 1;
			}
			else
			{
				usb_speaker_enable = 0;
			}
		}
	}
	else
	{
		UsbAudioSpeaker.FramCount = 0;
		UsbAudioSpeaker.TempFramCount = 0;
		usb_speaker_enable = 0;
	}
#endif

#ifdef CFG_RES_AUDIO_USB_IN_EN
	if(UsbAudioMic.AltSet)//open stream
	{
		if(UsbAudioMic.FramCount)//正在传数据 切传输了1-2帧数据
		{
			if(UsbAudioMic.FramCount != UsbAudioMic.TempFramCount)
			{
				UsbAudioMic.TempFramCount = UsbAudioMic.FramCount;
				usb_mic_enable = 1;
			}
		}
	}
	else
	{
		UsbAudioMic.FramCount = 0;
		UsbAudioMic.TempFramCount = 0;
		usb_mic_enable = 0;
	}
#endif
}
#endif
