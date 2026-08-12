/**
 * @file  usb_audio_api.h
 * @brief USB Audio + CDC composite APP glue for boot_app.
 *
 * Enables CFG_APP_USB_AUDIO_MODE_EN so otg_device_audio.c builds the ISO path.
 * Default mode: AUDIO_CDC (speaker + CDC). Switch to AUDIO_MIC_CDC for mic too.
 */
#ifndef __USB_AUDIO_API_H__
#define __USB_AUDIO_API_H__

#include "otg_device_standard_request.h"
#include "otg_device_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_APP_USB_AUDIO_MODE_EN

/* Composite: USB Speaker + CDC (change to AUDIO_MIC_CDC for mic+speaker+CDC) */
#ifndef CFG_PARA_USB_MODE
#define CFG_PARA_USB_MODE           AUDIO_CDC
#endif

#ifndef CFG_PARA_SAMPLE_RATE
#define CFG_PARA_SAMPLE_RATE        44100
#endif

#define CFG_RES_AUDIO_USB_IN_EN
#if (CFG_PARA_USB_MODE == AUDIO_MIC_CDC) || (CFG_PARA_USB_MODE == AUDIO_MIC) || \
	(CFG_PARA_USB_MODE == MIC_ONLY) || (CFG_PARA_USB_MODE == MIC_CDC)
#define CFG_RES_AUDIO_USB_OUT_EN
#endif
#define CFG_RES_AUDIO_USB_VOL_SET_EN

#define ONE_MS_SAMPLE               48
typedef int16_t PCM_DATA_TYPE;

#define USB_FIFO_LEN \
	(ONE_MS_SAMPLE * 2 * sizeof(PCM_DATA_TYPE) * PACKET_CHANNELS_NUM * 8)

bool UsbDevicePlayInit(void);
bool UsbDevicePlayResMalloc(void);
void UsbAudioSpeakerStreamProcess(void);
void UsbAudioMicStreamProcess(void);
void UsbAudioTimer1msProcess(void);
void AudioCoreSinkChange(uint8_t Channels, uint32_t SampleRate);
void AudioCoreSourceChange(uint8_t Channels, uint32_t SampleRate);

extern uint32_t usb_speaker_enable;
extern uint32_t usb_mic_enable;

/* Declared in otg_device_audio.c; re-export for APP code */
uint16_t UsbAudioSpeakerDataGet(void *Buffer, uint16_t Len);
uint16_t UsbAudioSpeakerDataLenGet(void);
uint16_t UsbAudioMicDataSet(void *Buffer, uint16_t Len);
uint16_t UsbAudioMicSpaceLenGet(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_AUDIO_API_H__ */
