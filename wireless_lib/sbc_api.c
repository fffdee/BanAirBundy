/**
 * @file sbc_api.c
 * @brief SBC encoder/decoder wrappers required by libaudio_association.a
 */
#include "app_config.h"
#include "sbc_encoder.h"
#include "sbc_frame_decoder.h"
#include "debug.h"
#include "type.h"
#include <string.h>

#if defined(ENCODE_CH) && (ENCODE_CH == 1)
void wireless_sbc_encoder_init(SBC_ENC_PARAMS *ct)
{
	memset(ct, 0, sizeof(SBC_ENC_PARAMS));
	ct->s16NumOfBlocks = SBC_BLOCK;
	ct->s16NumOfSubBands = SBC_SUBBAND;
	ct->s16AllocationMethod = SBC_SNR;
	ct->s16BitPool = ENC_BITPOOL;
	ct->mSBCEnabled = 0;
	ct->s16ChannelMode = SBC_MONO;
	ct->s16NumOfChannels = 1;
#if (SAMPLE_RATE == 44100)
	ct->s16SamplingFreq = SBC_sf44100;
#elif (SAMPLE_RATE == 32000)
	ct->s16SamplingFreq = SBC_sf32000;
#else
	ct->s16SamplingFreq = SBC_sf48000;
#endif
	SBC_Encoder_Init(ct);
}

void wireless_sbc_encoder_aplly(void *sbc_enc, int16_t *in_pcm,
				uint8_t *out_sbc, uint32_t *length)
{
	SBC_ENC_PARAMS *ct = (SBC_ENC_PARAMS *)sbc_enc;
	ct->ps16PcmBuffer = in_pcm;
	ct->pu8Packet = out_sbc;
	SBC_Encoder(ct);
	*length = ct->u16PacketLength;
}
#endif

void wireless_sbc_decoder_init(SBCFrameDecoderContext *p, uint8_t ch)
{
	(void)ch;
	sbc_frame_decoder_initialize(p);
}

int32_t wireless_sbc_decoder_apply(SBCFrameDecoderContext *sbc_dec,
				   uint8_t *sbc_buf, uint8_t sbc_size,
				   int16_t *pcm_buf)
{
	int32_t status = sbc_frame_decoder_decode(sbc_dec, sbc_buf, sbc_size);
	if (status == 0) {
#if defined(DECODE_CH) && (DECODE_CH != 0)
		memcpy(pcm_buf, sbc_dec->pcm,
		       (size_t)sbc_dec->pcm_length * DECODE_CH * 2u);
#else
		memcpy(pcm_buf, sbc_dec->pcm,
		       (size_t)sbc_dec->pcm_length * 2u);
#endif
	} else {
		DBG("SBC Decoder Err!%d\n", (int)status);
	}
	return status;
}
