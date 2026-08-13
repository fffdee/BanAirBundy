/**
 ******************************************************************************
 * @file    audio_driver_api.c
 * @brief   BP1540 ADC1/DAC0 port and shared USB/wireless mixer.
 ******************************************************************************
 */
#include "audio_driver_api.h"
#include "wireless_config.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "audio_adc.h"
#include "dac.h"
#include <string.h>

#define WL_ADC_DMA_WORDS       1024u
#define WL_DAC_DMA_WORDS       1024u
#define WL_ADC_READ_WORDS      512u
#define WL_MIX_RING_SAMPLES    2048u  /* interleaved stereo int16 samples */
#define WL_MIX_CHUNK_FRAMES    64u

static uint32_t s_adc_dma_buf[WL_ADC_DMA_WORDS];
static uint32_t s_adc_read_buf[WL_ADC_READ_WORDS];
static uint32_t s_dac_dma_buf[WL_DAC_DMA_WORDS];
static int16_t s_usb_ring[WL_MIX_RING_SAMPLES];
static int16_t s_wireless_ring[WL_MIX_RING_SAMPLES];
static int16_t s_mix_buf[WL_MIX_CHUNK_FRAMES * 2u];
static volatile uint16_t s_usb_r;
static volatile uint16_t s_usb_w;
static volatile uint16_t s_wireless_r;
static volatile uint16_t s_wireless_w;
static AudioRole_t s_role = AUDIO_ROLE_RX;
static uint8_t s_dac_initialized;

static uint16_t ring_count(volatile uint16_t r, volatile uint16_t w)
{
    return (uint16_t)((w - r) & (WL_MIX_RING_SAMPLES - 1u));
}

static uint16_t ring_push(int16_t *ring, volatile uint16_t *r,
                          volatile uint16_t *w, const int16_t *data,
                          uint16_t samples)
{
    uint16_t pushed = 0;
    uint16_t write = *w;

    while (pushed < samples) {
        uint16_t next = (uint16_t)((write + 1u) &
                                   (WL_MIX_RING_SAMPLES - 1u));
        if (next == *r)
            break;
        ring[write] = data[pushed++];
        write = next;
    }
    *w = write;
    return pushed;
}

static int16_t ring_pop_one(int16_t *ring, volatile uint16_t *r,
                            volatile uint16_t *w, uint8_t *valid)
{
    uint16_t read = *r;
    int16_t value;

    if (read == *w) {
        *valid = 0;
        return 0;
    }
    value = ring[read];
    *r = (uint16_t)((read + 1u) & (WL_MIX_RING_SAMPLES - 1u));
    *valid = 1;
    return value;
}

static int16_t sat16(int32_t value)
{
    if (value > 32767)
        return 32767;
    if (value < -32768)
        return -32768;
    return (int16_t)value;
}

int AudioDriver_Init(AudioRole_t role)
{
    s_role = role;
    s_usb_r = s_usb_w = 0;
    s_wireless_r = s_wireless_w = 0;
    /* DAC0 is required in both roles: TX plays USB, RX plays the mix. */
    WlAudioDac_Init(AUDIO_DAC0, SAMPLE_RATE, AUDIO_WIDTH_16BIT, NULL, 0);
    WlAudioDac_VolSet(AUDIO_DAC0, DAC_VOLUME_DEFAULT, DAC_VOLUME_DEFAULT);
    return 0;
}

void AudioDriver_Deinit(AudioRole_t role)
{
    (void)role;
}

int WlAudioAdc_AnaInit(AudioAdcModule_t module, AudioChannel_t channel,
                       AudioInput_t input, AudioMode_t mode, uint8_t pga_gain)
{
    ADC_MODULE sdk_module = (module == AUDIO_ADC1) ? ADC1_MODULE : ADC0_MODULE;
    ADC_CHANNEL sdk_channel = (channel == AUDIO_CHANNEL_RIGHT)
                            ? CHANNEL_RIGHT : CHANNEL_LEFT;
    AUDIO_ADC_INPUT sdk_input = (input == AUDIO_INPUT_MIC)
                              ? MIC_LEFT : LINEIN1_LEFT;
    AUDIO_Mode sdk_mode = (mode == AUDIO_MODE_DIFF) ? Diff : Single;

    AudioADC_AnaInit(sdk_module, sdk_channel, sdk_input, sdk_mode,
                     ADCCommonEnergy, pga_gain);
    return 0;
}

int WlAudioAdc_DigitalInit(AudioAdcModule_t module, uint32_t sample_rate,
                           AudioWidth_t width, void *buf, uint16_t buf_len)
{
    ADC_MODULE sdk_module = (module == AUDIO_ADC1) ? ADC1_MODULE : ADC0_MODULE;
    AUDIO_BitWidth sdk_width = (width == AUDIO_WIDTH_24BIT)
                             ? ADC_WIDTH_24BITS : ADC_WIDTH_16BITS;
    (void)buf;
    (void)buf_len;
    AudioADC_DigitalInit(sdk_module, sample_rate, sdk_width,
                         s_adc_dma_buf, sizeof(s_adc_dma_buf));
    return 0;
}

void WlAudioAdc_VolSet(AudioAdcModule_t module, uint16_t left_vol, uint16_t right_vol)
{
    AudioADC_VolSet((module == AUDIO_ADC1) ? ADC1_MODULE : ADC0_MODULE,
                    left_vol, right_vol);
}

uint16_t WlAudioAdc_DataLenGet(AudioAdcModule_t module)
{
    return (module == AUDIO_ADC1) ? AudioADC1_DataLenGet()
                                 : AudioADC0_DataLenGet();
}

uint16_t WlAudioAdc_DataGet(AudioAdcModule_t module, int16_t *buf, uint16_t samples)
{
    uint16_t actual;

    if (!buf || !samples)
        return 0;
    if (samples > WL_ADC_READ_WORDS)
        samples = WL_ADC_READ_WORDS;

    /*
     * SDK ADC1 DMA stores one 32-bit word per mono sample even in 16-bit
     * mode, then compacts to int16 in place.  Reading directly into an
     * int16 frame buffer would overwrite it before compaction.
     */
    actual = (module == AUDIO_ADC1)
           ? AudioADC1_DataGet(s_adc_read_buf, samples)
           : AudioADC0_DataGet(s_adc_read_buf, samples);
    memcpy(buf, s_adc_read_buf, (uint32_t)actual * sizeof(int16_t));
    return actual;
}

int WlAudioDac_Init(AudioDacModule_t module, uint32_t sample_rate,
                    AudioWidth_t width, void *buf, uint16_t buf_len)
{
    DACParamCt ct;
    (void)module;
    (void)buf;
    (void)buf_len;

    if (s_dac_initialized)
        return 0;

    memset(&ct, 0, sizeof(ct));
    ct.DACModel = DAC_Single;
    ct.DACLoadStatus = DAC_NOLoad;
    ct.PVDDModel = PVDD33;
    ct.DACEnergyModel = DACCommonEnergy;
    ct.DACVcomModel = Disable;
    AudioDAC_Init(&ct, sample_rate, (uint16_t)width,
                  s_dac_dma_buf, sizeof(s_dac_dma_buf), NULL, 0);
    s_dac_initialized = 1;
    return 0;
}

void WlAudioDac_VolSet(AudioDacModule_t module, uint16_t left_vol, uint16_t right_vol)
{
    (void)module;
    AudioDAC_VolSet(DAC0, left_vol, right_vol);
}

uint16_t WlAudioDac_DataLenGet(AudioDacModule_t module)
{
    (void)module;
    return (uint16_t)(AudioDAC0_DataLenGet() * 2u);
}

uint16_t WlAudioDac_DataSet(AudioDacModule_t module, const int16_t *buf, uint16_t samples)
{
    (void)module;
    /* Wl API counts interleaved int16 samples; SDK counts stereo frames. */
    return AudioDAC0_DataSet((void *)buf, (uint16_t)(samples / 2u));
}

void WlAudioDac_Mute(AudioDacModule_t module, bool mute)
{
    (void)module;
    AudioDAC_SoftMute(DAC0, mute, mute);
}

void WlAudioOutput_SetRole(AudioRole_t role)
{
    s_role = role;
}

uint16_t WlAudioOutput_PushUsb(const int16_t *stereo_pcm,
                               uint16_t stereo_samples)
{
    if (!stereo_pcm || !stereo_samples)
        return 0;
    return ring_push(s_usb_ring, &s_usb_r, &s_usb_w,
                     stereo_pcm, stereo_samples);
}

uint16_t WlAudioOutput_PushWireless(const int16_t *stereo_pcm,
                                    uint16_t stereo_samples)
{
    if (!stereo_pcm || !stereo_samples)
        return 0;
    return ring_push(s_wireless_ring, &s_wireless_r, &s_wireless_w,
                     stereo_pcm, stereo_samples);
}

void WlAudioOutput_Process(void)
{
    uint16_t usb_count;
    uint16_t wireless_count;
    uint16_t space;
    uint16_t frames;
    uint16_t i;

    if (!s_dac_initialized)
        return;

    usb_count = ring_count(s_usb_r, s_usb_w);
    wireless_count = ring_count(s_wireless_r, s_wireless_w);
    if (usb_count < 2u && (s_role == AUDIO_ROLE_TX || wireless_count < 2u))
        return;

    /* SDK reports writable stereo frames. */
    space = AudioDAC0_DataSpaceLenGet();
    frames = space;
    if (frames > WL_MIX_CHUNK_FRAMES)
        frames = WL_MIX_CHUNK_FRAMES;
    if (!frames)
        return;

    for (i = 0; i < frames * 2u; i++) {
        uint8_t usb_valid;
        uint8_t wireless_valid;
        int16_t usb = ring_pop_one(s_usb_ring, &s_usb_r, &s_usb_w,
                                   &usb_valid);
        int16_t wireless = 0;

        if (s_role == AUDIO_ROLE_RX) {
            wireless = ring_pop_one(s_wireless_ring, &s_wireless_r,
                                    &s_wireless_w, &wireless_valid);
            (void)wireless_valid;
        }
        s_mix_buf[i] = sat16((int32_t)usb + (int32_t)wireless);
        (void)usb_valid;
    }
    AudioDAC0_DataSet(s_mix_buf, frames);
}
