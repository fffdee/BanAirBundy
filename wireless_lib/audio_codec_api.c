/**
 ******************************************************************************
 * @file    audio_codec_api.c
 * @brief   SBC音频编解码API实现
 *
 * 来源: sbc/sbc_api.c
 * 本文件为平台相关接口的封装层, 实际SBC编解码需要平台SDK的sbc_encoder/sbc_decoder库
 ******************************************************************************
 */
#include "audio_codec_api.h"
#include "wireless_config.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/*===========================================================================
 * SBC编码器内部结构
 *===========================================================================*/
typedef struct {
    uint32_t sample_rate;
    uint8_t  channel_num;
    uint8_t  bitpool;
    uint8_t  channel_mode;
    uint8_t  allocation;
    uint8_t  sub_bands;
    uint8_t  block_size;
    /* 平台SDK的SBC编码器句柄 */
    void    *platform_encoder;
} SbcEncoder_t;

typedef struct {
    uint32_t sample_rate;
    uint8_t  channel_num;
    /* 平台SDK的SBC解码器句柄 */
    void    *platform_decoder;
    /* 丢包补偿: 上一帧PCM数据 */
    int16_t  last_pcm[ONE_FRAME * 2];
} SbcDecoder_t;

/*===========================================================================
 * 编码器API
 *===========================================================================*/

void* AudioCodec_EncoderInit(uint32_t sample_rate, uint8_t channel_num, uint8_t bitpool)
{
    SbcEncoder_t *enc;

    enc = (SbcEncoder_t *)malloc(sizeof(SbcEncoder_t));
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

    /* TODO: 调用平台SDK的SBC编码器初始化 */
    // enc->platform_encoder = sbc_encoder_init(
    //     enc->channel_mode, enc->sub_bands, enc->block_size,
    //     enc->allocation, enc->bitpool, sample_rate);

    return enc;
}

int AudioCodec_Encode(void *encoder, const int16_t *pcm_in,
                      uint16_t samples, uint8_t *sbc_out, uint16_t max_out)
{
    SbcEncoder_t *enc = (SbcEncoder_t *)encoder;
    if (!enc || !pcm_in || !sbc_out)
        return -1;

    /* TODO: 调用平台SDK的SBC编码 */
    // int len = sbc_encoder(enc->platform_encoder, pcm_in, samples, sbc_out, max_out);
    // return len;

    (void)samples; (void)max_out;
    return -1; /* 需要平台SDK实现 */
}

void AudioCodec_EncoderDeinit(void *encoder)
{
    SbcEncoder_t *enc = (SbcEncoder_t *)encoder;
    if (!enc)
        return;

    /* TODO: 调用平台SDK的反初始化 */
    // sbc_encoder_deinit(enc->platform_encoder);
    free(enc);
}

/*===========================================================================
 * 解码器API
 *===========================================================================*/

void* AudioCodec_DecoderInit(uint32_t sample_rate, uint8_t channel_num)
{
    SbcDecoder_t *dec;

    dec = (SbcDecoder_t *)malloc(sizeof(SbcDecoder_t));
    if (!dec)
        return NULL;

    memset(dec, 0, sizeof(*dec));
    dec->sample_rate = sample_rate;
    dec->channel_num = channel_num;

    /* TODO: 调用平台SDK的SBC解码器初始化 */
    // dec->platform_decoder = sbc_decoder_init(sample_rate, channel_num);

    return dec;
}

int AudioCodec_Decode(void *decoder, const uint8_t *sbc_in, uint16_t sbc_len,
                      int16_t *pcm_out, uint16_t max_samples)
{
    SbcDecoder_t *dec = (SbcDecoder_t *)decoder;
    if (!dec || !sbc_in || !pcm_out)
        return -1;

    /* TODO: 调用平台SDK的SBC解码 */
    // int samples = sbc_decoder(dec->platform_decoder, sbc_in, sbc_len,
    //                            pcm_out, max_samples);
    // if (samples > 0) {
    //     /* 保存本帧用于丢包补偿 */
    //     memcpy(dec->last_pcm, pcm_out, samples * 2 * sizeof(int16_t));
    // }
    // return samples;

    (void)sbc_len; (void)max_samples;
    return -1; /* 需要平台SDK实现 */
}

void AudioCodec_DecoderDeinit(void *decoder)
{
    SbcDecoder_t *dec = (SbcDecoder_t *)decoder;
    if (!dec)
        return;

    /* TODO: 调用平台SDK的反初始化 */
    // sbc_decoder_deinit(dec->platform_decoder);
    free(dec);
}

/*===========================================================================
 * 便捷函数
 *===========================================================================*/

uint16_t AudioCodec_SbcFrameLen(uint8_t channel_mode, uint8_t sub_bands,
                                 uint8_t block_size, uint8_t bitpool)
{
    /* SBC帧长度计算公式 (蓝牙标准) */
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
