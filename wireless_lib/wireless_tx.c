/**
 ******************************************************************************
 * @file    wireless_tx.c
 * @brief   TX: MIC → SBC mono → PackBuild → TransBufWrite
 ******************************************************************************
 */
#include "app_config.h"
#include "wireless_api.h"
#include "audio_codec_api.h"
#include "audio_driver_api.h"
#include "wireless_config.h"
#include "mvwire_port.h"
#include <string.h>

#if defined(BOOT_APP_WIRELESS_ROLE_TX) && (BOOT_APP_WIRELESS_ROLE_TX == 1)

static struct {
	bool             initialized;
	bool             pairing;
	ConnectStatus_t  connect_status;
	WirelessConfig_t config;
	int16_t         *mic_pcm_buf;
	uint8_t         *sbc_out_buf;
	void            *sbc_encoder;
	uint8_t          pkt_cnt;
	WirelessConnectedCb_t    connected_cb;
	WirelessDisconnectedCb_t disconnected_cb;
} s_tx;

static int16_t s_mic_pcm_storage[ONE_FRAME];
static uint8_t s_sbc_out_storage[RFAUDIO_TRANS_A + 8];

static void tx_on_conn(uint8_t idx)
{
	s_tx.connect_status = CONNECT_AUDIO;
	if (s_tx.connected_cb)
		s_tx.connected_cb(idx);
}

static void tx_on_disc(uint8_t idx)
{
	s_tx.connect_status = CONNECT_NONE;
	if (s_tx.disconnected_cb)
		s_tx.disconnected_cb(idx);
}

static void tx_audio_process(void)
{
	uint16_t samples_available;
	uint16_t frame_size;
	int      sbc_len;

	if (!s_tx.initialized || s_tx.connect_status != CONNECT_AUDIO)
		return;

	frame_size = s_tx.config.frame_size;
	samples_available = WlAudioAdc_DataLenGet(AUDIO_ADC1);
	if (samples_available < frame_size)
		return;

#if BOOT_APP_MVWIRE_EN
	if (MvWire_TransSpaceLen() < RFAUDIO_TRANS_A)
		return;
#endif

	WlAudioAdc_DataGet(AUDIO_ADC1, s_tx.mic_pcm_buf, frame_size);

	/* Turnkey 2_6: mono encode (no stereo upmix). */
	sbc_len = AudioCodec_Encode(s_tx.sbc_encoder, s_tx.mic_pcm_buf,
				    frame_size, s_tx.sbc_out_buf, RFAUDIO_TRANS_A);
	if (sbc_len <= 0)
		return;

#if BOOT_APP_MVWIRE_EN
	MvWire_PackBuild(s_tx.sbc_out_buf, s_tx.pkt_cnt++, 0xff);
	MvWire_TransBufWrite(&s_tx.sbc_out_buf[CRC_PACKSUB], RFAUDIO_TRANS_A);
#else
	if (Wireless_TxIsReady())
		Wireless_TxSend(s_tx.sbc_out_buf, (uint16_t)sbc_len);
#endif
}

int Wireless_Init(const WirelessConfig_t *config)
{
	if (!config)
		return -1;

	memset(&s_tx, 0, sizeof(s_tx));
	s_tx.config = *config;
	s_tx.mic_pcm_buf = s_mic_pcm_storage;
	s_tx.sbc_out_buf = s_sbc_out_storage;

	AudioDriver_Init(AUDIO_ROLE_TX);
	WlAudioAdc_AnaInit(AUDIO_ADC1, AUDIO_CHANNEL_LEFT, AUDIO_INPUT_MIC,
			   AUDIO_MODE_DIFF, MIC_PGA_GAIN_DEFAULT);
	WlAudioAdc_DigitalInit(AUDIO_ADC1, config->sample_rate, AUDIO_WIDTH_16BIT,
			       s_tx.mic_pcm_buf, MIC_FIFO_SAMPLES(config->frame_size));
	WlAudioAdc_VolSet(AUDIO_ADC1, 0x0FFFu, 0x0FFFu);

	s_tx.sbc_encoder = AudioCodec_EncoderInit(config->sample_rate,
						  config->channel_num, SBC_BITPOOL);
	if (!s_tx.sbc_encoder)
		return -2;

#if BOOT_APP_MVWIRE_EN
	if (MvWire_StackInit(1) != 0)
		return -3;
	MvWire_RegisterAppConnCb(tx_on_conn, tx_on_disc);
	MvWire_AudioReadySet(1);
#endif

	s_tx.initialized = TRUE;
	return 0;
}

void Wireless_Deinit(void)
{
	if (!s_tx.initialized)
		return;
	AudioCodec_EncoderDeinit(s_tx.sbc_encoder);
	AudioDriver_Deinit(AUDIO_ROLE_TX);
	memset(&s_tx, 0, sizeof(s_tx));
}

void Wireless_Schedule(void)
{
	if (!s_tx.initialized)
		return;
#if BOOT_APP_MVWIRE_EN
	MvWire_StackSchedule();
#endif
	tx_audio_process();
}

int Wireless_StartPairing(void)
{
	s_tx.pairing = TRUE;
	return 0;
}

void Wireless_StopPairing(void) { s_tx.pairing = FALSE; }
void Wireless_DisconnectAll(void) {}

ConnectStatus_t Wireless_GetConnectStatus(uint8_t device_index)
{
	(void)device_index;
#if BOOT_APP_MVWIRE_EN
	return (ConnectStatus_t)MvWire_GetDeviceStatus(0);
#else
	return s_tx.connect_status;
#endif
}

bool Wireless_IsConnected(void)
{
	return Wireless_GetConnectStatus(0) >= CONNECT_WIRELESS;
}

uint8_t Wireless_GetConnectedCount(void)
{
	return Wireless_IsConnected() ? 1 : 0;
}

bool Wireless_TxIsReady(void)
{
#if BOOT_APP_MVWIRE_EN
	return (Wireless_GetConnectStatus(0) == CONNECT_AUDIO) &&
	       (MvWire_TransSpaceLen() >= RFAUDIO_TRANS_A);
#else
	return s_tx.connect_status == CONNECT_AUDIO;
#endif
}

int Wireless_TxSend(const uint8_t *data, uint16_t len)
{
	if (!data || len == 0)
		return -1;
#if BOOT_APP_MVWIRE_EN
	MvWire_TransBufWrite(data, len);
	return 0;
#else
	(void)data;
	(void)len;
	return 0;
#endif
}

void Wireless_RegisterConnectedCb(WirelessConnectedCb_t cb)
{
	s_tx.connected_cb = cb;
}

void Wireless_RegisterDisconnectedCb(WirelessDisconnectedCb_t cb)
{
	s_tx.disconnected_cb = cb;
}

void Wireless_RegisterRxReadyCb(WirelessRxReadyCb_t cb) { (void)cb; }
void Wireless_SetFreqBand(uint8_t band) { (void)band; }
void Wireless_SetChannel(uint8_t channel) { (void)channel; }
void Wireless_SetDeviceAddr(const uint8_t addr[6]) { (void)addr; }
void Wireless_Sleep(void) {}
void Wireless_Active(void) {}

void AudioCodec_TxProcess(void) { tx_audio_process(); }

#endif /* BOOT_APP_WIRELESS_ROLE_TX == 1 */
