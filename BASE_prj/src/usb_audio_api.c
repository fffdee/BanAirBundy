/**
 * @file  usb_audio_api.c
 * @brief USB Audio resource init + stream drain for composite device.
 *
 * Speaker ISO path fills circular buffer in IRQ; this file drains it so
 * the FIFO does not overflow. DAC/ADC wiring can replace the drain later.
 */
#include <string.h>
#include "type.h"
#include "debug.h"
#include "usb_audio_api.h"

#ifdef CFG_APP_USB_AUDIO_MODE_EN

static uint8_t UsbAudioSpeaker_PCMBuffer[USB_FIFO_LEN];
#ifdef CFG_RES_AUDIO_USB_OUT_EN
static uint8_t UsbAudioMic_PCMBuffer[USB_FIFO_LEN];
#endif
static uint32_t s_drain_buf[ONE_MS_SAMPLE * PACKET_CHANNELS_NUM];

/* Referenced by otg_device_audio.c UsbAudioTimer1msProcess() */
uint32_t usb_speaker_enable = 0;
uint32_t usb_mic_enable = 0;

bool UsbDevicePlayInit(void)
{
	memset(&UsbAudioSpeaker, 0, sizeof(UsbAudio));
	memset(&UsbAudioMic, 0, sizeof(UsbAudio));

	UsbAudioSpeaker.Channels = PACKET_CHANNELS_NUM;
	UsbAudioSpeaker.ByteSet = PCM16BIT;
	UsbAudioSpeaker.LeftVol = AUDIO_MAX_VOLUME;
	UsbAudioSpeaker.RightVol = AUDIO_MAX_VOLUME;
	UsbAudioSpeaker.AudioSampleRate = CFG_PARA_SAMPLE_RATE;
	UsbAudioSpeaker.SampleRate = CFG_PARA_SAMPLE_RATE;

	UsbAudioMic.Channels = MIC_CHANNELS_NUM;
	UsbAudioMic.ByteSet = PCM16BIT;
	UsbAudioMic.LeftVol = AUDIO_MAX_VOLUME;
	UsbAudioMic.RightVol = AUDIO_MAX_VOLUME;
	UsbAudioMic.AudioSampleRate = CFG_PARA_SAMPLE_RATE;
	UsbAudioMic.SampleRate = CFG_PARA_SAMPLE_RATE;
	return TRUE;
}

bool UsbDevicePlayResMalloc(void)
{
	DBG("[USB] UsbDevicePlayResMalloc\n");
#ifdef CFG_RES_AUDIO_USB_IN_EN
	UsbAudioSpeaker.PCMBuffer = (int16_t *)UsbAudioSpeaker_PCMBuffer;
	memset(UsbAudioSpeaker.PCMBuffer, 0, USB_FIFO_LEN);
	MCUCircular_Config(&UsbAudioSpeaker.CircularBuf,
			   UsbAudioSpeaker.PCMBuffer, USB_FIFO_LEN);
#endif
#ifdef CFG_RES_AUDIO_USB_OUT_EN
	UsbAudioMic.PCMBuffer = (int16_t *)UsbAudioMic_PCMBuffer;
	memset(UsbAudioMic.PCMBuffer, 0, USB_FIFO_LEN);
	MCUCircular_Config(&UsbAudioMic.CircularBuf,
			   UsbAudioMic.PCMBuffer, USB_FIFO_LEN);
#endif
	return TRUE;
}

void AudioCoreSinkChange(uint8_t Channels, uint32_t SampleRate)
{
	(void)Channels;
	(void)SampleRate;
#ifdef CFG_RES_AUDIO_USB_OUT_EN
	if (UsbAudioMic.PCMBuffer) {
		MCUCircular_Config(&UsbAudioMic.CircularBuf,
				   UsbAudioMic.PCMBuffer, USB_FIFO_LEN);
	}
#endif
}

void AudioCoreSourceChange(uint8_t Channels, uint32_t SampleRate)
{
	(void)Channels;
	(void)SampleRate;
#ifdef CFG_RES_AUDIO_USB_IN_EN
	if (UsbAudioSpeaker.PCMBuffer) {
		MCUCircular_Config(&UsbAudioSpeaker.CircularBuf,
				   UsbAudioSpeaker.PCMBuffer, USB_FIFO_LEN);
	}
#endif
}

void UsbAudioSpeakerStreamProcess(void)
{
#ifdef CFG_RES_AUDIO_USB_IN_EN
	/* Drain ISO speaker FIFO (replace with DAC feed when ready). */
	while (UsbAudioSpeakerDataLenGet() >= ONE_MS_SAMPLE) {
		UsbAudioSpeakerDataGet(s_drain_buf, ONE_MS_SAMPLE);
	}
#endif
}

void UsbAudioMicStreamProcess(void)
{
#ifdef CFG_RES_AUDIO_USB_OUT_EN
	/* Feed silence to mic ISO path until ADC is wired. */
	if (UsbAudioMicSpaceLenGet() >= ONE_MS_SAMPLE) {
		memset(s_drain_buf, 0, sizeof(s_drain_buf));
		UsbAudioMicDataSet(s_drain_buf, ONE_MS_SAMPLE);
	}
#endif
}

#endif /* CFG_APP_USB_AUDIO_MODE_EN */
