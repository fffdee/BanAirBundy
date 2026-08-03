/* Bootloader stub for USB audio symbols referenced by otg_device_standard_request.c.
 * The bootloader uses CDC-only mode, so these audio callbacks are no-ops.
 */

#include "type.h"
#include "otg_device_audio.h"

UsbAudio UsbAudioSpeaker;
UsbAudio UsbAudioMic;

void OnDeviceAudioRcvIsoPacket(void) {}
void OnDeviceAudioSendIsoPacket(void) {}
void OTG_DeviceAudioInit(void) {}
void OTG_DeviceAudioRequest(void) {}
