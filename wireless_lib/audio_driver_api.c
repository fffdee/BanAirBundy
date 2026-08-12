/**
 ******************************************************************************
 * @file    audio_driver_api.c
 * @brief   wireless_lib ADC/DAC porting stubs (Wl* API)
 *
 * Wrap platform SDK (adc_interface / dac_interface) inside these functions.
 * Do not export SDK symbol names from this file.
 ******************************************************************************
 */
#include "audio_driver_api.h"
#include "wireless_config.h"

int AudioDriver_Init(AudioRole_t role)
{
    (void)role;
    return 0;
}

void AudioDriver_Deinit(AudioRole_t role)
{
    (void)role;
}

int WlAudioAdc_AnaInit(AudioAdcModule_t module, AudioChannel_t channel,
                       AudioInput_t input, AudioMode_t mode, uint8_t pga_gain)
{
    /* TODO: call AudioADC_AnaInit / PGA / PowerUp from SDK */
    (void)module; (void)channel; (void)input; (void)mode; (void)pga_gain;
    return 0;
}

int WlAudioAdc_DigitalInit(AudioAdcModule_t module, uint32_t sample_rate,
                           AudioWidth_t width, void *buf, uint16_t buf_len)
{
    /* TODO: call AudioADC_DigitalInit + DMA setup from SDK */
    (void)module; (void)sample_rate; (void)width; (void)buf; (void)buf_len;
    return 0;
}

void WlAudioAdc_VolSet(AudioAdcModule_t module, uint16_t left_vol, uint16_t right_vol)
{
    /* TODO: call SDK AudioADC_VolSet */
    (void)module; (void)left_vol; (void)right_vol;
}

uint16_t WlAudioAdc_DataLenGet(AudioAdcModule_t module)
{
    /* TODO: call AudioADC1_DataLenGet / equivalent */
    (void)module;
    return 0;
}

uint16_t WlAudioAdc_DataGet(AudioAdcModule_t module, int16_t *buf, uint16_t samples)
{
    /* TODO: call AudioADC1_DataGet / equivalent */
    (void)module; (void)buf; (void)samples;
    return 0;
}

int WlAudioDac_Init(AudioDacModule_t module, uint32_t sample_rate,
                    AudioWidth_t width, void *buf, uint16_t buf_len)
{
    /* TODO: call SDK AudioDAC_Init with DACParamCt */
    (void)module; (void)sample_rate; (void)width; (void)buf; (void)buf_len;
    return 0;
}

void WlAudioDac_VolSet(AudioDacModule_t module, uint16_t left_vol, uint16_t right_vol)
{
    /* TODO: call SDK AudioDAC_VolSet */
    (void)module; (void)left_vol; (void)right_vol;
}

uint16_t WlAudioDac_DataLenGet(AudioDacModule_t module)
{
    (void)module;
    return 0;
}

uint16_t WlAudioDac_DataSet(AudioDacModule_t module, const int16_t *buf, uint16_t samples)
{
    /* TODO: call AudioDAC0_DataSet / equivalent */
    (void)module; (void)buf; (void)samples;
    return 0;
}

void WlAudioDac_Mute(AudioDacModule_t module, bool mute)
{
    /* TODO: call AudioDAC_SoftMute */
    (void)module; (void)mute;
}
