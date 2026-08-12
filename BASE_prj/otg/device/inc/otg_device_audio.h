/**
 * @file  otg_device_audio.h
 * @brief USB Audio (UAC) device types and APIs for composite CDC+Audio.
 */
#ifndef __OTG_DEVICE_AUDIO_H__
#define __OTG_DEVICE_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "type.h"
#include "mcu_circular_buf.h"

#ifndef AUDIO_UAC_10
#define AUDIO_UAC_10            1
#endif
#ifndef AUDIO_UAC_20
#define AUDIO_UAC_20            2
#endif
#ifndef USB_AUDIO_PROTOCOL
#define USB_AUDIO_PROTOCOL      AUDIO_UAC_10
#endif

#ifndef USBD_AUDIO_FREQ
#define USBD_AUDIO_FREQ         44100
#endif
#ifndef USBD_AUDIO_FREQ1
#define USBD_AUDIO_FREQ1        48000
#endif
#ifndef USBD_AUDIO_MIC_FREQ
#define USBD_AUDIO_MIC_FREQ     44100
#endif
#ifndef USBD_AUDIO_MIC_FREQ1
#define USBD_AUDIO_MIC_FREQ1    48000
#endif
#ifndef USBD_AUDIO_MIC_FREQ2
#define USBD_AUDIO_MIC_FREQ2    16000
#endif

#ifndef PACKET_CHANNELS_NUM
#define PACKET_CHANNELS_NUM     2
#endif
#ifndef MIC_CHANNELS_NUM
#define MIC_CHANNELS_NUM        2
#endif

#define PCM16BIT                2
#define PCM24BIT                3

#ifndef AUDIO_MIN_VOLUME
#define AUDIO_MIN_VOLUME        0
#endif
#ifndef AUDIO_MAX_VOLUME
#define AUDIO_MAX_VOLUME        4096
#endif
#ifndef AUDIO_RES_VOLUME
#define AUDIO_RES_VOLUME        1
#endif

#ifndef AUDIO_FU_ID
#define AUDIO_FU_ID             2
#endif

/* UAC1 Feature Unit control selectors (bmaControls) */
#ifndef AUDIO_CONTROL_MUTE
#define AUDIO_CONTROL_MUTE      0x01
#endif
#ifndef AUDIO_CONTROL_VOLUME
#define AUDIO_CONTROL_VOLUME    0x02
#endif

/* Entity IDs used by AUDIO_MIC / AUDIO_MIC_CDC descriptors */
#ifndef AUDIO_MIC_IT_ID
#define AUDIO_MIC_IT_ID         4
#endif
#ifndef AUDIO_MIC_FU_ID
#define AUDIO_MIC_FU_ID         5
#endif
#ifndef AUDIO_MIC_SL_ID
#define AUDIO_MIC_SL_ID         6
#endif
#ifndef AUDIO_MIC_OT_ID
#define AUDIO_MIC_OT_ID         7
#endif

#ifndef SWAP_BUF_TO_U32
#define SWAP_BUF_TO_U32(p) \
	((uint32_t)((p)[0]) | ((uint32_t)((p)[1]) << 8) | ((uint32_t)((p)[2]) << 16))
#endif

#ifndef SAMPLE_FREQ_4B
#define SAMPLE_FREQ_4B(f) \
	(uint8_t)((f) & 0xFF), (uint8_t)(((f) >> 8) & 0xFF), \
	(uint8_t)(((f) >> 16) & 0xFF), (uint8_t)(((f) >> 24) & 0xFF)
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef APP_DBG
#define APP_DBG DBG
#endif

typedef struct _UsbAudio {
	uint8_t  InitOk;
	uint8_t  AltSlow;
	uint8_t  AltSet;
	uint8_t  ByteSet;
	uint8_t  Channels;
	uint8_t  Mute;
	uint16_t Volume;
	uint16_t LeftVol;
	uint16_t RightVol;
	uint32_t SampleRate;
	uint32_t AudioSampleRate;
	uint32_t FramCount;
	uint32_t TempFramCount;
	int16_t *PCMBuffer;
	MCU_CIRCULAR_CONTEXT CircularBuf;
} UsbAudio;

extern UsbAudio UsbAudioSpeaker;
extern UsbAudio UsbAudioMic;

void OTG_DeviceAudioInit(void);
void OTG_DeviceAudioRequest(void);
void OnDeviceAudioRcvIsoPacket(void);
void OnDeviceAudioSendIsoPacket(void);
bool OTG_DeviceAudioSendPcCmd(uint8_t Cmd);

#ifdef __cplusplus
}
#endif

#endif /* __OTG_DEVICE_AUDIO_H__ */
