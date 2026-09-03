/**
 ******************************************************************************
 * @file    audio_driver_api.h
 * @brief   wireless_lib ADC/DAC porting layer (platform-independent names)
 *
 * NOTE: Do NOT reuse SDK names like AudioADC_VolSet / AudioDAC_Init — those
 * collide with libDriver.a. Port implementations should wrap SDK APIs inside
 * WlAudio* functions.
 ******************************************************************************
 */
#ifndef __AUDIO_DRIVER_API_H__
#define __AUDIO_DRIVER_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "type.h"

/*===========================================================================
 * 音频角色
 *===========================================================================*/
typedef enum {
    AUDIO_ROLE_TX = 0,   /* 发射端: 使用ADC采集 */
    AUDIO_ROLE_RX = 1,   /* 接收端: 使用DAC输出 */
} AudioRole_t;

/*===========================================================================
 * ADC模块
 *===========================================================================*/
typedef enum {
    AUDIO_ADC0 = 0,   /* ADC0: LineIn */
    AUDIO_ADC1 = 1,   /* ADC1: MIC */
} AudioAdcModule_t;

typedef enum {
    AUDIO_CHANNEL_LEFT  = 0,
    AUDIO_CHANNEL_RIGHT = 1,
} AudioChannel_t;

typedef enum {
    AUDIO_INPUT_MIC    = 0,   /* 麦克风输入 */
    AUDIO_INPUT_LINEIN = 1,   /* 线路输入 */
} AudioInput_t;

typedef enum {
    AUDIO_MODE_SINGLE = 0,    /* 单端模式 */
    AUDIO_MODE_DIFF   = 1,    /* 差分模式 */
} AudioMode_t;

typedef enum {
    AUDIO_WIDTH_16BIT = 16,
    AUDIO_WIDTH_24BIT = 24,
} AudioWidth_t;

/*===========================================================================
 * DAC模块
 *===========================================================================*/
typedef enum {
    AUDIO_DAC0 = 0,
} AudioDacModule_t;

/*===========================================================================
 * 驱动初始化/反初始化
 *===========================================================================*/
int  AudioDriver_Init(AudioRole_t role);
void AudioDriver_Deinit(AudioRole_t role);

/*===========================================================================
 * ADC API (TX端使用) — Wl* 前缀避免与 libDriver 冲突
 *===========================================================================*/
int WlAudioAdc_AnaInit(AudioAdcModule_t module, AudioChannel_t channel,
                       AudioInput_t input, AudioMode_t mode, uint8_t pga_gain);

int WlAudioAdc_DigitalInit(AudioAdcModule_t module, uint32_t sample_rate,
                           AudioWidth_t width, void *buf, uint16_t buf_len);

void WlAudioAdc_VolSet(AudioAdcModule_t module, uint16_t left_vol, uint16_t right_vol);

uint16_t WlAudioAdc_DataLenGet(AudioAdcModule_t module);

uint16_t WlAudioAdc_DataGet(AudioAdcModule_t module, int16_t *buf, uint16_t samples);

/*===========================================================================
 * DAC API (RX端使用)
 *===========================================================================*/
int WlAudioDac_Init(AudioDacModule_t module, uint32_t sample_rate,
                    AudioWidth_t width, void *buf, uint16_t buf_len);

void WlAudioDac_VolSet(AudioDacModule_t module, uint16_t left_vol, uint16_t right_vol);

/* DAC wrapper lengths are interleaved int16 sample counts (L+R). */
uint16_t WlAudioDac_DataLenGet(AudioDacModule_t module);

uint16_t WlAudioDac_DataSet(AudioDacModule_t module, const int16_t *buf, uint16_t samples);

void WlAudioDac_Mute(AudioDacModule_t module, bool mute);

/*===========================================================================
 * Shared DAC0 mixer
 *
 * USB Speaker and decoded wireless PCM are independent producers.  The USB
 * task calls WlAudioOutput_Process() and is the only DAC FIFO writer.
 *===========================================================================*/
void WlAudioOutput_SetRole(AudioRole_t role);
uint16_t WlAudioOutput_PushUsb(const int16_t *stereo_pcm,
                               uint16_t stereo_samples);
uint16_t WlAudioOutput_PushWireless(const int16_t *stereo_pcm,
                                    uint16_t stereo_samples);
void WlAudioOutput_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_DRIVER_API_H__ */
