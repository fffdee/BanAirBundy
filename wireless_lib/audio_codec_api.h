/**
 ******************************************************************************
 * @file    audio_codec_api.h
 * @brief   SBC音频编解码API
 *
 * 来源: sbc/sbc_api.h + sbc/encoder/include/sbc_encoder.h
 ******************************************************************************
 */
#ifndef __AUDIO_CODEC_API_H__
#define __AUDIO_CODEC_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*===========================================================================
 * SBC编码参数
 *===========================================================================*/
enum {
    SBC_MODE_MONO   = 0,    /* 单声道 */
    SBC_MODE_DUAL   = 1,    /* 双声道 */
    SBC_MODE_STEREO = 2,    /* 立体声 */
    SBC_MODE_JOINT  = 3,    /* 联合立体声 */
};

enum {
    SBC_ALLOCATION_LOUDNESS = 0,    /* 响度分配 */
    SBC_ALLOCATION_SNR      = 1,    /* SNR分配 */
};

/*===========================================================================
 * 编码器API (TX端)
 *===========================================================================*/

/**
 * @brief  初始化SBC编码器
 * @param  sample_rate: 采样率 (44100)
 * @param  channel_num: 声道数 (1=单声道, 2=立体声)
 * @param  bitpool: 位池 (31=标准音质)
 * @retval 编码器句柄, NULL=失败
 */
void* AudioCodec_EncoderInit(uint32_t sample_rate, uint8_t channel_num, uint8_t bitpool);

/**
 * @brief  SBC编码
 * @param  encoder: 编码器句柄
 * @param  pcm_in: PCM输入 (16bit, 立体声交错)
 * @param  samples: 采样数 (每声道)
 * @param  sbc_out: SBC数据输出缓冲
 * @param  max_out: 输出缓冲最大长度
 * @retval 编码后字节数, <0=失败
 */
int AudioCodec_Encode(void *encoder, const int16_t *pcm_in,
                      uint16_t samples, uint8_t *sbc_out, uint16_t max_out);

/**
 * @brief  反初始化编码器
 */
void AudioCodec_EncoderDeinit(void *encoder);

/*===========================================================================
 * 解码器API (RX端)
 *===========================================================================*/

/**
 * @brief  初始化SBC解码器
 * @param  sample_rate: 采样率
 * @param  channel_num: 声道数
 * @retval 解码器句柄, NULL=失败
 */
void* AudioCodec_DecoderInit(uint32_t sample_rate, uint8_t channel_num);

/**
 * @brief  SBC解码
 * @param  decoder: 解码器句柄
 * @param  sbc_in: SBC数据输入
 * @param  sbc_len: SBC数据长度
 * @param  pcm_out: PCM输出 (16bit, 立体声交错)
 * @param  max_samples: 最大输出采样数
 * @retval 解码后采样数, <0=失败
 */
int AudioCodec_Decode(void *decoder, const uint8_t *sbc_in, uint16_t sbc_len,
                      int16_t *pcm_out, uint16_t max_samples);

/**
 * @brief  反初始化解码器
 */
void AudioCodec_DecoderDeinit(void *decoder);

/*===========================================================================
 * 便捷函数
 *===========================================================================*/

/**
 * @brief  计算SBC编码后每帧的长度
 * @param  channel_mode: 声道模式
 * @param  sub_bands: 子带数 (4或8)
 * @param  block_size: 块大小 (4/8/12/16)
 * @param  bitpool: 位池
 * @retval 每帧SBC数据字节数
 */
uint16_t AudioCodec_SbcFrameLen(uint8_t channel_mode, uint8_t sub_bands,
                                 uint8_t block_size, uint8_t bitpool);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_CODEC_API_H__ */
