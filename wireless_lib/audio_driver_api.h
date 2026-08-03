/**
 ******************************************************************************
 * @file    audio_driver_api.h
 * @brief   ADC/DAC驱动抽象层
 *
 * 来源: driver/driver_api/inc/adc_interface.h + dac_interface.h
 ******************************************************************************
 */
#ifndef __AUDIO_DRIVER_API_H__
#define __AUDIO_DRIVER_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

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

/**
 * @brief  初始化音频驱动
 * @param  role: TX使用ADC, RX使用DAC
 */
int  AudioDriver_Init(AudioRole_t role);
void AudioDriver_Deinit(AudioRole_t role);

/*===========================================================================
 * ADC API (TX端使用)
 *===========================================================================*/

/**
 * @brief  ADC模拟初始化 (配置PGA增益、输入通道)
 * @param  module: ADC模块 (AUDIO_ADC1=麦克风)
 * @param  channel: 声道
 * @param  input: 输入源 (MIC/LINEIN)
 * @param  mode: 模式 (单端/差分)
 * @param  pga_gain: PGA增益 (0~31, 31=最大+26.5dB)
 */
int AudioADC_AnaInit(AudioAdcModule_t module, AudioChannel_t channel,
                     AudioInput_t input, AudioMode_t mode, uint8_t pga_gain);

/**
 * @brief  ADC数字初始化 (配置采样率、位宽、DMA FIFO)
 * @param  module: ADC模块
 * @param  sample_rate: 采样率
 * @param  width: 位宽
 * @param  buf: DMA缓冲区
 * @param  buf_len: 缓冲区长度
 */
int AudioADC_DigitalInit(AudioAdcModule_t module, uint32_t sample_rate,
                          AudioWidth_t width, void *buf, uint16_t buf_len);

/**
 * @brief  设置ADC数字音量
 * @param  module: ADC模块
 * @param  left_vol: 左声道音量 (0~0xFFF, 0xFFF=0dB最大)
 * @param  right_vol: 右声道音量
 */
void AudioADC_VolSet(AudioAdcModule_t module, uint16_t left_vol, uint16_t right_vol);

/**
 * @brief  获取DMA FIFO中可读数据长度 (采样数)
 */
uint16_t AudioADC_DataLenGet(AudioAdcModule_t module);

/**
 * @brief  从DMA FIFO读取PCM数据
 * @param  module: ADC模块
 * @param  buf: 输出缓冲区
 * @param  samples: 要读取的采样数
 * @retval 实际读取的采样数
 */
uint16_t AudioADC_DataGet(AudioAdcModule_t module, int16_t *buf, uint16_t samples);

/*===========================================================================
 * DAC API (RX端使用)
 *===========================================================================*/

/**
 * @brief  DAC初始化 (配置采样率、位宽、DMA FIFO)
 * @param  module: DAC模块
 * @param  sample_rate: 采样率
 * @param  width: 位宽
 * @param  buf: DMA缓冲区
 * @param  buf_len: 缓冲区长度
 */
int AudioDAC_Init(AudioDacModule_t module, uint32_t sample_rate,
                   AudioWidth_t width, void *buf, uint16_t buf_len);

/**
 * @brief  设置DAC音量
 * @param  module: DAC模块
 * @param  left_vol: 左声道音量 (0~0x3FFF, 0x3FFF=最大)
 * @param  right_vol: 右声道音量
 */
void AudioDAC_VolSet(AudioDacModule_t module, uint16_t left_vol, uint16_t right_vol);

/**
 * @brief  获取DAC DMA FIFO可写空间 (采样数)
 */
uint16_t AudioDAC_DataLenGet(AudioDacModule_t module);

/**
 * @brief  写入PCM数据到DAC DMA FIFO
 * @param  module: DAC模块
 * @param  buf: PCM数据
 * @param  samples: 采样数
 * @retval 实际写入的采样数
 */
uint16_t AudioDAC_DataSet(AudioDacModule_t module, const int16_t *buf, uint16_t samples);

/**
 * @brief  DAC软静音控制
 * @param  module: DAC模块
 * @param  mute: true=静音, false=正常
 */
void AudioDAC_Mute(AudioDacModule_t module, bool mute);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_DRIVER_API_H__ */
