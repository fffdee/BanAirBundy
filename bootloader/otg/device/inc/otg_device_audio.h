/**
 * @file  otg_device_audio.h
 * @brief Minimal USB audio types for CDC_ONLY bootloader (no audio runtime).
 */
#ifndef __OTG_DEVICE_AUDIO_H__
#define __OTG_DEVICE_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "type.h"

#ifndef AUDIO_MAX_VOLUME
#define AUDIO_MAX_VOLUME  4096
#endif

typedef struct _UsbAudio {
    uint8_t  InitOk;
    uint8_t  AltSlow;
    uint8_t  AltSet;
    uint8_t  ByteSet;
    uint8_t  Channels;
    uint8_t  Mute;
    uint16_t Volume;
    uint32_t SampleRate;
} UsbAudio;

extern UsbAudio UsbAudioSpeaker;
extern UsbAudio UsbAudioMic;

void OTG_DeviceAudioInit(void);
void OTG_DeviceAudioRequest(void);
void OnDeviceAudioRcvIsoPacket(void);
void OnDeviceAudioSendIsoPacket(void);

#ifdef __cplusplus
}
#endif

#endif /* __OTG_DEVICE_AUDIO_H__ */
