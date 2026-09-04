/**
 ******************************************************************************
 * @file    audio_codec_api.c
 * @brief   SBC encode wrap (Turnkey 2_6 mono via 1532 SBC_Encoder)
 ******************************************************************************
 */
/* Board config first: it selects the WL_MALLOC backend (see wireless_config.h). */
#include "app_config.h"

#include "audio_codec_api.h"
#include "wireless_config.h"
#include "mvwire_port.h"
#include <string.h>
#include <stdlib.h>

#if BOOT_APP_MVWIRE_EN && defined(ENCODE_CH) && (ENCODE_CH == 1)
#include "sbc_encoder.h"
#endif

typedef struct {
	uint32_t sample_rate;
	uint8_t  channel_num;
	uint8_t  bitpool;
	uint8_t  channel_mode;
	uint8_t  allocation;
	uint8_t  sub_bands;
	uint8_t  block_size;
#if BOOT_APP_MVWIRE_EN && defined(ENCODE_CH) && (ENCODE_CH == 1)
	SBC_ENC_PARAMS enc;
	uint8_t ready;
#endif
} SbcEncoder_t;

typedef struct {
	uint32_t sample_rate;
	uint8_t  channel_num;
	int16_t  last_pcm[ONE_FRAME * 2];
} SbcDecoder_t;

void *AudioCodec_EncoderInit(uint32_t sample_rate, uint8_t channel_num, uint8_t bitpool)
{
	SbcEncoder_t *enc = (SbcEncoder_t *)WL_MALLOC(sizeof(SbcEncoder_t));
	if (!enc)
		return NULL;

	memset(enc, 0, sizeof(*enc));
	enc->sample_rate = sample_rate;
	enc->channel_num = channel_num;
	enc->bitpool = bitpool;
	enc->channel_mode = (channel_num == 1) ? SBC_MODE_MONO : SBC_MODE_JOINT;
	enc->allocation = SBC_ALLOCATION_SNR;
	enc->sub_bands = SBC_SUB_BANDS;
	enc->block_size = SBC_BLOCK_SIZE;

#if BOOT_APP_MVWIRE_EN && defined(ENCODE_CH) && (ENCODE_CH == 1)
	(void)bitpool;
	memset(&enc->enc, 0, sizeof(enc->enc));
	enc->enc.s16NumOfBlocks = SBC_BLOCK;
	enc->enc.s16NumOfSubBands = SBC_SUBBAND;
	enc->enc.s16AllocationMethod = SBC_SNR;
	enc->enc.s16BitPool = ENC_BITPOOL;
	enc->enc.mSBCEnabled = 0;
	enc->enc.s16ChannelMode = SBC_MONO;
	enc->enc.s16NumOfChannels = 1;
#if (SAMPLE_RATE == 44100)
	enc->enc.s16SamplingFreq = SBC_sf44100;
#elif (SAMPLE_RATE == 32000)
	enc->enc.s16SamplingFreq = SBC_sf32000;
#else
	enc->enc.s16SamplingFreq = SBC_sf48000;
#endif
	SBC_Encoder_Init(&enc->enc);
	enc->ready = 1;
#else
	(void)sample_rate;
#endif
	return enc;
}

int AudioCodec_Encode(void *encoder, const int16_t *pcm_in,
		      uint16_t samples, uint8_t *sbc_out, uint16_t max_out)
{
	SbcEncoder_t *enc = (SbcEncoder_t *)encoder;
	if (!enc || !pcm_in || !sbc_out)
		return -1;

#if BOOT_APP_MVWIRE_EN && defined(ENCODE_CH) && (ENCODE_CH == 1)
	if (!enc->ready)
		return -1;
	(void)samples;
	(void)max_out;
	enc->enc.ps16PcmBuffer = (SINT16 *)pcm_in;
	enc->enc.pu8Packet = sbc_out;
	SBC_Encoder(&enc->enc);
	return (int)enc->enc.u16PacketLength;
#else
	(void)samples;
	(void)max_out;
	return -1;
#endif
}

void AudioCodec_EncoderDeinit(void *encoder)
{
	WL_FREE(encoder);
}

void *AudioCodec_DecoderInit(uint32_t sample_rate, uint8_t channel_num)
{
	SbcDecoder_t *dec = (SbcDecoder_t *)WL_MALLOC(sizeof(SbcDecoder_t));
	if (!dec)
		return NULL;
	memset(dec, 0, sizeof(*dec));
	dec->sample_rate = sample_rate;
	dec->channel_num = channel_num;
	return dec;
}

int AudioCodec_Decode(void *decoder, const uint8_t *sbc_in, uint16_t sbc_len,
		      int16_t *pcm_out, uint16_t max_samples)
{
	/* RX uses AudioAssociationProcess; keep stub for API completeness. */
	(void)decoder;
	(void)sbc_in;
	(void)sbc_len;
	(void)pcm_out;
	(void)max_samples;
	return -1;
}

void AudioCodec_DecoderDeinit(void *decoder)
{
	WL_FREE(decoder);
}

uint16_t AudioCodec_SbcFrameLen(uint8_t channel_mode, uint8_t sub_bands,
				uint8_t block_size, uint8_t bitpool)
{
	uint16_t len;
	switch (channel_mode) {
	case SBC_MODE_MONO:
		len = 4 + (sub_bands * block_size * bitpool) / 8;
		break;
	case SBC_MODE_DUAL:
		len = 4 + 2 * (sub_bands * block_size * bitpool) / 8;
		break;
	case SBC_MODE_STEREO:
	case SBC_MODE_JOINT:
		len = 4 + (sub_bands * block_size * bitpool) / 8 + sub_bands / 8;
		break;
	default:
		len = 0;
		break;
	}
	return len;
}
