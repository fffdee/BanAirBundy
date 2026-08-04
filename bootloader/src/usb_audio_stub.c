/**
 * @file  usb_audio_stub.c
 * @brief No-op USB audio symbols for CDC_ONLY bootloader build.
 */
#include "type.h"
#include "otg_device_audio.h"

UsbAudio UsbAudioSpeaker;
UsbAudio UsbAudioMic;

void OnDeviceAudioRcvIsoPacket(void) {}
void OnDeviceAudioSendIsoPacket(void) {}
void OTG_DeviceAudioInit(void) {}
void OTG_DeviceAudioRequest(void) {}
