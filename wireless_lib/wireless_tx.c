/**
 ******************************************************************************
 * @file    wireless_tx.c
 * @brief   TX端: 麦克风采集 → SBC编码 → 无线发送
 *
 * 来源: wireless/wireless_main.c + audio/audio_main.c (TX部分)
 ******************************************************************************
 */
#include "wireless_api.h"
#include "audio_codec_api.h"
#include "audio_driver_api.h"
#include "wireless_config.h"
#include <string.h>

/*===========================================================================
 * 内部状态
 *===========================================================================*/
static struct {
    bool             initialized;
    bool             pairing;
    ConnectStatus_t  connect_status;
    WirelessConfig_t config;

    /* 音频缓冲区 */
    int16_t         *mic_pcm_buf;       /* 麦克风PCM数据 (单声道) */
    int16_t         *stereo_pcm_buf;    /* 立体声PCM (编码输入) */
    uint8_t         *sbc_out_buf;       /* SBC编码输出 */

    /* SBC编码器句柄 */
    void            *sbc_encoder;

    /* 回调 */
    WirelessConnectedCb_t    connected_cb;
    WirelessDisconnectedCb_t disconnected_cb;
} s_tx;

/*===========================================================================
 * 内部函数
 *===========================================================================*/

/**
 * @brief  音频处理: 采集→编码→发送 (主循环调用)
 *
 * 完整数据流:
 *   1. 从ADC DMA FIFO读取PCM数据 (ONE_FRAME采样)
 *   2. 单声道转立体声 (upmix)
 *   3. SBC编码
 *   4. 无线发送
 */
static void tx_audio_process(void)
{
    uint16_t samples_available;
    uint16_t frame_size;
    int      sbc_len;

    if (!s_tx.initialized || s_tx.connect_status != CONNECT_AUDIO)
        return;

    frame_size = s_tx.config.frame_size;

    /* 1. 检查DMA FIFO是否有足够数据 */
    samples_available = AudioADC_DataLenGet(AUDIO_ADC1);
    if (samples_available < frame_size)
        return;

    /* 2. 从ADC读取PCM数据 */
    AudioADC_DataGet(AUDIO_ADC1, s_tx.mic_pcm_buf, frame_size);

    /* 3. 单声道转立体声 (SBC编码器需要立体声输入) */
    for (int16_t i = frame_size - 1; i >= 0; i--) {
        s_tx.stereo_pcm_buf[2 * i + 0] = s_tx.mic_pcm_buf[i];
        s_tx.stereo_pcm_buf[2 * i + 1] = s_tx.mic_pcm_buf[i];
    }

    /* 4. SBC编码 */
    sbc_len = AudioCodec_Encode(
        s_tx.sbc_encoder,
        s_tx.stereo_pcm_buf,
        frame_size,
        s_tx.sbc_out_buf,
        RFAUDIO_TRANS_A
    );

    if (sbc_len <= 0)
        return;

    /* 5. 无线发送 */
    if (Wireless_TxIsReady()) {
        Wireless_TxSend(s_tx.sbc_out_buf, (uint16_t)sbc_len);
    }
}

/*===========================================================================
 * 公共API实现
 *===========================================================================*/

int Wireless_Init(const WirelessConfig_t *config)
{
    if (!config)
        return -1;

    memset(&s_tx, 0, sizeof(s_tx));
    s_tx.config = *config;

    /* 1. 初始化音频驱动 (ADC采集) */
    AudioDriver_Init(AUDIO_ROLE_TX);

    /* 2. 初始化麦克风ADC */
    AudioADC_AnaInit(AUDIO_ADC1, AUDIO_CHANNEL_LEFT, AUDIO_INPUT_MIC,
                     AUDIO_MODE_SINGLE, MIC_PGA_GAIN_DEFAULT);
    AudioADC_DigitalInit(AUDIO_ADC1, config->sample_rate,
                         AUDIO_WIDTH_16BIT,
                         s_tx.mic_pcm_buf,
                         MIC_FIFO_SAMPLES(config->frame_size));

    /* 3. 设置ADC音量 */
    AudioADC_VolSet(AUDIO_ADC1, DAC_VOLUME_DEFAULT, DAC_VOLUME_DEFAULT);

    /* 4. 初始化SBC编码器 */
    s_tx.sbc_encoder = AudioCodec_EncoderInit(
        config->sample_rate,
        config->channel_num,
        SBC_BITPOOL
    );

    /* 5. 配置基带参数 (平台相关, 需要移植) */
    // WirelessBbParams_t bb_params = { ... };
    // Wireless_common_init(&bb_params);

    /* 6. 设置设备角色 */
    // MVWIRE2_DeviceRoleSet(WIRELESS_ROLE_SLAVE);

    /* 7. 设置配对密钥和设备地址 */
    // set_local_bdaddr(addr);
    // sub_band_config(RF_FREQ_BAND);

    s_tx.initialized = true;
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

    /* 1. 无线协议栈调度 (平台相关) */
    // rwip_schedule();

    /* 2. 音频处理 */
    tx_audio_process();
}

int Wireless_StartPairing(void)
{
    s_tx.pairing = true;
    /* TX端: 开始广播 */
    // wireless_app_start_adv();
    return 0;
}

void Wireless_StopPairing(void)
{
    s_tx.pairing = false;
    /* 停止广播 */
}

void Wireless_DisconnectAll(void)
{
    /* 断开所有连接 */
}

ConnectStatus_t Wireless_GetConnectStatus(uint8_t device_index)
{
    (void)device_index; /* TX端只有一个连接 */
    return s_tx.connect_status;
}

bool Wireless_IsConnected(void)
{
    return s_tx.connect_status >= CONNECT_WIRELESS;
}

uint8_t Wireless_GetConnectedCount(void)
{
    return Wireless_IsConnected() ? 1 : 0;
}

bool Wireless_TxIsReady(void)
{
    /* 检查RF发送缓冲区是否就绪 */
    // return Wireless_TransPacketIsReady();
    return s_tx.connect_status == CONNECT_AUDIO;
}

int Wireless_TxSend(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0)
        return -1;
    /* 调用平台RF发送函数 */
    // lld_con_wireless_tx_send(data, len);
    (void)data; (void)len;
    return 0;
}

void Wireless_RegisterConnectedCb(WirelessConnectedCb_t cb)
{
    s_tx.connected_cb = cb;
}

void Wireless_RegisterDisconnectedCb(WirelessDisconnectedCb_t cb)
{
    s_tx.disconnected_cb = cb;
}

void Wireless_RegisterRxReadyCb(WirelessRxReadyCb_t cb)
{
    (void)cb; /* TX端不需要RX回调 */
}

void Wireless_SetFreqBand(uint8_t band)
{
    // sub_band_config(band);
    (void)band;
}

void Wireless_SetChannel(uint8_t channel)
{
    (void)channel;
}

void Wireless_SetDeviceAddr(const uint8_t addr[6])
{
    // set_local_bdaddr(addr);
    (void)addr;
}

void Wireless_Sleep(void)
{
    // wireless2_Sleep();
}

void Wireless_Active(void)
{
    // wireless2_Active();
}

/* 音频处理入口 (供主循环调用) */
void AudioCodec_TxProcess(void)
{
    tx_audio_process();
}
