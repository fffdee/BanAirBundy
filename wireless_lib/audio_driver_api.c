/**
 ******************************************************************************
 * @file    audio_driver_api.c
 * @brief   ADC/DAC驱动接口实现
 *
 * 来源: driver/driver_api/src/adc_interface.c + dac_interface.c
 * 本文件为平台相关驱动的抽象层, 实际操作需要平台SDK的寄存器级驱动
 ******************************************************************************
 */
#include "audio_driver_api.h"
#include "wireless_config.h"

/*===========================================================================
 * 驱动初始化
 *===========================================================================*/
int AudioDriver_Init(AudioRole_t role)
{
    /* TODO: 平台相关初始化
     * - 配置音频时钟 (MCLK, BCLK)
     * - 配置DMA通道
     * - 配置中断
     */
    (void)role;
    return 0;
}

void AudioDriver_Deinit(AudioRole_t role)
{
    (void)role;
}

/*===========================================================================
 * ADC API (TX端: 麦克风采集)
 *===========================================================================*/

int AudioADC_AnaInit(AudioAdcModule_t module, AudioChannel_t channel,
                     AudioInput_t input, AudioMode_t mode, uint8_t pga_gain)
{
    /* TODO: 平台相关实现
     *
     * 来源: adc_interface.c -> AudioADC_AnaInit()
     *
     * 内部执行:
     *   1. 上电PGA: AudioADC_PGAPowerUp(module, 1, 1)
     *   2. 选择输入通道: AudioADC_PGASel(module, channel, input)
     *   3. 设置PGA增益: AudioADC_PGAGainSet(module, channel, input, 31-pga_gain)
     *      (注意: 硬件值 = 31 - 逻辑值, 0=最大+26.5dB)
     *   4. 上电ADC: AudioADC_PowerUp(module, 1, 1)
     *   5. 配置偏置电流 (低功耗/普通模式)
     */
    (void)module; (void)channel; (void)input; (void)mode; (void)pga_gain;
    return 0;
}

int AudioADC_DigitalInit(AudioAdcModule_t module, uint32_t sample_rate,
                          AudioWidth_t width, void *buf, uint16_t buf_len)
{
    /* TODO: 平台相关实现
     *
     * 来源: adc_interface.c -> AudioADC_DigitalInit()
     *
     * 内部执行:
     *   1. 配置采样率: AudioADC_SampleRateSet(module, sample_rate)
     *   2. 配置位宽: AudioADC_WidthSet(module, width)
     *   3. 配置DMA: Dma_Init(channel, src_addr, buf, buf_len, DMA_CIRCULAR)
     *   4. 使能DMA中断
     *   5. 使能ADC
     */
    (void)module; (void)sample_rate; (void)width; (void)buf; (void)buf_len;
    return 0;
}

void AudioADC_VolSet(AudioAdcModule_t module, uint16_t left_vol, uint16_t right_vol)
{
    /* TODO: 平台相关实现
     * 来源: adc_interface.c -> AudioADC_VolSet()
     * 范围: 0x001(-72dB) ~ 0xFFF(0dB)
     */
    (void)module; (void)left_vol; (void)right_vol;
}

uint16_t AudioADC_DataLenGet(AudioAdcModule_t module)
{
    /* TODO: 平台相关实现
     * 读取DMA FIFO中的可读数据量
     */
    (void)module;
    return 0;
}

uint16_t AudioADC_DataGet(AudioAdcModule_t module, int16_t *buf, uint16_t samples)
{
    /* TODO: 平台相关实现
     * 从DMA FIFO读取PCM数据到buf
     */
    (void)module; (void)buf; (void)samples;
    return 0;
}

/*===========================================================================
 * DAC API (RX端: 音频输出)
 *===========================================================================*/

int AudioDAC_Init(AudioDacModule_t module, uint32_t sample_rate,
                   AudioWidth_t width, void *buf, uint16_t buf_len)
{
    /* TODO: 平台相关实现
     *
     * 来源: dac_interface.c -> AudioDAC_Init()
     *
     * 内部执行:
     *   1. 配置采样率: AudioDAC_SampleRateSet(module, sample_rate)
     *   2. 配置位宽
     *   3. 设置默认音量: AudioDAC_VolSet(module, 0x1000, 0x1000)
     *   4. 关闭静音: AudioDAC_SoftMute(module, FALSE, FALSE)
     *   5. 配置DMA: Dma_Init(channel, buf, dst_addr, buf_len, DMA_CIRCULAR)
     *   6. 使能DAC输出
     */
    (void)module; (void)sample_rate; (void)width; (void)buf; (void)buf_len;
    return 0;
}

void AudioDAC_VolSet(AudioDacModule_t module, uint16_t left_vol, uint16_t right_vol)
{
    /* TODO: 平台相关实现
     * 来源: dac_interface.c -> AudioDAC_VolSet()
     * 范围: 0 ~ 0x3FFF (0x3FFF=最大)
     */
    (void)module; (void)left_vol; (void)right_vol;
}

uint16_t AudioDAC_DataLenGet(AudioDacModule_t module)
{
    /* TODO: 平台相关实现 */
    (void)module;
    return 0;
}

uint16_t AudioDAC_DataSet(AudioDacModule_t module, const int16_t *buf, uint16_t samples)
{
    /* TODO: 平台相关实现
     * 将PCM数据写入DMA FIFO
     */
    (void)module; (void)buf; (void)samples;
    return 0;
}

void AudioDAC_Mute(AudioDacModule_t module, bool mute)
{
    /* TODO: 平台相关实现
     * 来源: dac_interface.c -> AudioDAC_SoftMute()
     */
    (void)module; (void)mute;
}
