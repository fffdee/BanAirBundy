/**
 ******************************************************************************
 * @file    wireless_rx.c
 * @brief   RX: AudioAssociation → stereo expand → WlAudioOutput_PushWireless
 ******************************************************************************
 */
#include "wireless_api.h"
#include "audio_codec_api.h"
#include "audio_driver_api.h"
#include "wireless_config.h"
#include "mvwire_port.h"
#include "debug.h"
#include <string.h>

#if !(defined(BOOT_APP_WIRELESS_ROLE_TX) && (BOOT_APP_WIRELESS_ROLE_TX == 1))

static struct {
	bool             initialized;
	bool             pairing;
	WirelessConfig_t config;
	int16_t          dac_pcm_buf[ONE_FRAME * 2];
	int16_t          pcm_l[ONE_FRAME * 4];
	int16_t          pcm_r[ONE_FRAME * 2];
	WirelessConnectedCb_t    connected_cb;
	WirelessDisconnectedCb_t disconnected_cb;
	WirelessRxReadyCb_t      rx_ready_cb;
} s_rx;

static void rx_on_conn(uint8_t idx)
{
	if (s_rx.connected_cb)
		s_rx.connected_cb(idx);
}

static void rx_on_disc(uint8_t idx)
{
	if (s_rx.disconnected_cb)
		s_rx.disconnected_cb(idx);
}

static void rx_audio_process(void)
{
	uint16_t frame_size = s_rx.config.frame_size;
	uint16_t got;
	uint16_t i;

	if (!s_rx.initialized)
		return;

#if BOOT_APP_MVWIRE_EN
	/*
	 * Bisect stack overflow: set to 1 after WL survives with hwm printed.
	 * AudioAssociationProcess (lib) appears to need a very deep task stack.
	 */
#ifndef WL_RX_ASSOC_PROCESS_EN
#define WL_RX_ASSOC_PROCESS_EN 0
#endif
#if WL_RX_ASSOC_PROCESS_EN
	got = MvWire_AssocProcess(s_rx.pcm_l, s_rx.pcm_r);
#else
	{
		static uint8_t once;
		if (!once) {
			once = 1;
			DBG("[WL] AssocProcess DISABLED (bisect)\n");
		}
		got = 0;
	}
#endif
	if (got == 0)
		return;

	/*
	 * 1st-frame sync DAC priming (mirrors official wireless_audio_process,
	 * audio_main.c:543-583). On the first schedule after a device reports
	 * 1st-frame sync (state==2), push a short silence block into the DAC ring
	 * to establish a stable playback-delay baseline and remove the startup
	 * underrun/pop. The latch resets when both devices lose sync so the next
	 * re-lock primes again. (The official AudioOutDelete fine-trim relies on
	 * Device0_2rdPackSample set by a lib callback not wired here; the elastic
	 * wireless ring absorbs that jitter, so the priming depth alone suffices.)
	 */
	{
		static uint8_t audio_1st_data;
		uint8_t sync0 = MvWire_1stFrameSyncState(0);
		uint8_t sync1 = MvWire_1stFrameSyncState(1);

		if ((sync0 != 2) && (sync1 != 2)) {
			audio_1st_data = 0;
		} else if (!audio_1st_data) {
			uint16_t prime_frames = (sync0 == 2) ? (uint16_t)(ONE_FRAME / 8)
							     : (uint16_t)(ONE_FRAME / 3);
			audio_1st_data = 1;
			memset(s_rx.dac_pcm_buf, 0,
			       (size_t)prime_frames * 2u * sizeof(int16_t));
			WlAudioOutput_PushWireless(s_rx.dac_pcm_buf,
						   (uint16_t)(prime_frames * 2u));
			DBG("[WL] 1st-frame sync: prime DAC ring %u frames\n",
			    (unsigned)prime_frames);
		}
	}

	/* Mono device1 → L/R duplicate (Turnkey 2_6 DECODE_CH=1). */
	for (i = 0; i < frame_size; i++) {
		int16_t s = s_rx.pcm_l[i];
		s_rx.dac_pcm_buf[2 * i + 0] = s;
		s_rx.dac_pcm_buf[2 * i + 1] = s;
	}
	/* If device2 also synced, mix (simple add+clip). */
	if (MvWire_GetDeviceStatus(1) == CONNECT_AUDIO) {
		for (i = 0; i < frame_size; i++) {
			int32_t l = (int32_t)s_rx.dac_pcm_buf[2 * i] + s_rx.pcm_r[i];
			int32_t r = (int32_t)s_rx.dac_pcm_buf[2 * i + 1] + s_rx.pcm_r[i];
			if (l > 32767)
				l = 32767;
			if (l < -32768)
				l = -32768;
			if (r > 32767)
				r = 32767;
			if (r < -32768)
				r = -32768;
			s_rx.dac_pcm_buf[2 * i] = (int16_t)l;
			s_rx.dac_pcm_buf[2 * i + 1] = (int16_t)r;
		}
	}

	WlAudioOutput_PushWireless(s_rx.dac_pcm_buf, (uint16_t)(frame_size * 2u));
#else
	(void)got;
	(void)i;
	(void)frame_size;
#endif
}

int Wireless_Init(const WirelessConfig_t *config)
{
	if (!config)
		return -1;

	memset(&s_rx, 0, sizeof(s_rx));
	s_rx.config = *config;

	AudioDriver_Init(AUDIO_ROLE_RX);
	WlAudioDac_Init(AUDIO_DAC0, config->sample_rate, AUDIO_WIDTH_16BIT,
			s_rx.dac_pcm_buf, DAC_FIFO_SAMPLES(config->frame_size));
	WlAudioDac_VolSet(AUDIO_DAC0, DAC_VOLUME_DEFAULT, DAC_VOLUME_DEFAULT);

#if BOOT_APP_MVWIRE_EN
	if (MvWire_StackInit(0) != 0)
		return -3;
	if (MvWire_AssocInit() != 0)
		return -4;
	MvWireless2AdvModePairingScanEn(1);
	MvWire_RegisterAppConnCb(rx_on_conn, rx_on_disc);
	MvWire_AudioReadySet(1);
#endif

	s_rx.initialized = TRUE;
	return 0;
}

void Wireless_Deinit(void)
{
	if (!s_rx.initialized)
		return;
	AudioDriver_Deinit(AUDIO_ROLE_RX);
	memset(&s_rx, 0, sizeof(s_rx));
}

void Wireless_Schedule(void)
{
	if (!s_rx.initialized)
		return;
#if BOOT_APP_MVWIRE_EN
	MvWire_StackSchedule();
#endif
	rx_audio_process();
}

int Wireless_StartPairing(void)
{
	s_rx.pairing = TRUE;
	return 0;
}

void Wireless_StopPairing(void) { s_rx.pairing = FALSE; }

void Wireless_DisconnectAll(void) {}

ConnectStatus_t Wireless_GetConnectStatus(uint8_t device_index)
{
#if BOOT_APP_MVWIRE_EN
	return (ConnectStatus_t)MvWire_GetDeviceStatus(device_index);
#else
	(void)device_index;
	return CONNECT_NONE;
#endif
}

bool Wireless_IsConnected(void)
{
	return Wireless_GetConnectStatus(0) >= CONNECT_WIRELESS ||
	       Wireless_GetConnectStatus(1) >= CONNECT_WIRELESS;
}

uint8_t Wireless_GetConnectedCount(void)
{
	uint8_t n = 0;
	if (Wireless_GetConnectStatus(0) >= CONNECT_WIRELESS)
		n++;
	if (Wireless_GetConnectStatus(1) >= CONNECT_WIRELESS)
		n++;
	return n;
}

bool Wireless_RxIsReady(uint8_t device_index)
{
	return Wireless_GetConnectStatus(device_index) == CONNECT_AUDIO;
}

int Wireless_RxRead(uint8_t device_index, uint8_t *buf, uint16_t max_len)
{
	(void)device_index;
	(void)buf;
	(void)max_len;
	return 0;
}

bool Wireless_TxIsReady(void) { return FALSE; }
int Wireless_TxSend(const uint8_t *data, uint16_t len)
{
	(void)data;
	(void)len;
	return -1;
}

void Wireless_RegisterConnectedCb(WirelessConnectedCb_t cb)
{
	s_rx.connected_cb = cb;
}

void Wireless_RegisterDisconnectedCb(WirelessDisconnectedCb_t cb)
{
	s_rx.disconnected_cb = cb;
}

void Wireless_RegisterRxReadyCb(WirelessRxReadyCb_t cb)
{
	s_rx.rx_ready_cb = cb;
}

void Wireless_SetFreqBand(uint8_t band) { (void)band; }
void Wireless_SetChannel(uint8_t channel) { (void)channel; }
void Wireless_SetDeviceAddr(const uint8_t addr[6]) { (void)addr; }
void Wireless_Sleep(void) {}
void Wireless_Active(void) {}

void AudioCodec_RxProcess(void) { rx_audio_process(); }

#endif /* !BOOT_APP_WIRELESS_ROLE_TX */
