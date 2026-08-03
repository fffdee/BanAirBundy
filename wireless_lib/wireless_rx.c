/**
 ******************************************************************************
 * @file    wireless_rx.c
 * @brief   RX端: 无线接收 → SBC解码 → DAC输出
 *
 * 来源: wireless/wireless_main.c + audio/audio_main.c (RX部分) + audio_association.c
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
    WirelessConfig_t config;

    /* 设备连接状态 (支持2T1R, 最多2个TX) */
    ConnectStatus_t  dev_status[2];

    /* 音频缓冲区 */
    uint8_t          rx_pkt_buf[2][RFAUDIO_TRANS_A];  /* 接收包缓冲 */
    uint8_t          sbc_buf[2][RFAUDIO_TRANS_A * 3]; /* SBC重组缓冲 */
    int16_t          pcm_out_buf[2][ONE_FRAME * 2];   /* PCM解码输出(立体声) */
    int16_t          dac_pcm_buf[ONE_FRAME * 2];       /* DAC输出缓冲(混合后) */

    /* SBC解码器 */
    void            *sbc_decoder[2];

    /* 音频关联参数 (包重组+丢包补偿) */
    RxAudioAssocParam_t assoc;

    /* 回调 */
    WirelessConnectedCb_t    connected_cb;
    WirelessDisconnectedCb_t disconnected_cb;
    WirelessRxReadyCb_t      rx_ready_cb;
} s_rx;

/*===========================================================================
 * 内部函数: 音频包重组与丢包补偿
 *===========================================================================*/

/**
 * @brief  处理接收到的无线音频包
 *
 * 数据流:
 *   1. 从无线接收FIFO读取数据包
 *   2. 包重组 (多个RF包组成一个完整SBC帧)
 *   3. SBC解码
 *   4. 丢包补偿 (PLC) - 丢包时用上一帧数据补偿
 *   5. 写入DAC输出FIFO
 */
static void rx_audio_process(void)
{
    uint16_t frame_size;
    int      decode_len;
    uint8_t  dev;

    if (!s_rx.initialized)
        return;

    frame_size = s_rx.config.frame_size;

    for (dev = 0; dev < 2; dev++) {
        if (s_rx.dev_status[dev] != CONNECT_AUDIO)
            continue;

        /* 1. 检查是否有接收数据 */
        if (!Wireless_RxIsReady(dev))
            continue;

        /* 2. 读取无线数据包 */
        int pkt_len = Wireless_RxRead(dev, s_rx.rx_pkt_buf[dev], RFAUDIO_TRANS_A);
        if (pkt_len <= 0)
            continue;

        /* 3. 包重组 (AudioAssociationProcess) */
        // 将多个RF包重组为完整SBC帧
        // 存入 s_rx.sbc_buf[dev]

        /* 4. SBC解码 */
        decode_len = AudioCodec_Decode(
            s_rx.sbc_decoder[dev],
            s_rx.sbc_buf[dev],
            RFAUDIO_TRANS_A,
            s_rx.pcm_out_buf[dev],
            frame_size * 2  /* 立体声 */
        );

        if (decode_len <= 0) {
            /* 解码失败, 丢包补偿(PLC): 重复上一帧 */
            s_rx.assoc.total_lost_frames++;
            continue;
        }

        s_rx.assoc.total_play_frames++;

        /* 5. 写入DAC输出缓冲 (多设备混合) */
        if (s_rx.dev_status[0] == CONNECT_AUDIO &&
            s_rx.dev_status[1] == CONNECT_AUDIO) {
            /* 双设备: 混音 */
            for (int i = 0; i < frame_size * 2; i++) {
                int32_t mix = (int32_t)s_rx.pcm_out_buf[0][i] +
                              (int32_t)s_rx.pcm_out_buf[1][i];
                s_rx.dac_pcm_buf[i] = (int16_t)(mix > 32767 ? 32767 :
                                               (mix < -32768 ? -32768 : mix));
            }
        } else {
            /* 单设备: 直接输出 */
            memcpy(s_rx.dac_pcm_buf, s_rx.pcm_out_buf[dev],
                   frame_size * 2 * sizeof(int16_t));
        }
    }

    /* 6. 写入DAC DMA FIFO */
    AudioDAC_DataSet(AUDIO_DAC0, s_rx.dac_pcm_buf, frame_size * 2);
}

/*===========================================================================
 * 公共API实现
 *===========================================================================*/

int Wireless_Init(const WirelessConfig_t *config)
{
    if (!config)
        return -1;

    memset(&s_rx, 0, sizeof(s_rx));
    s_rx.config = *config;

    /* 1. 初始化音频驱动 (DAC输出) */
    AudioDriver_Init(AUDIO_ROLE_RX);

    /* 2. 初始化DAC */
    AudioDAC_Init(AUDIO_DAC0, config->sample_rate, AUDIO_WIDTH_16BIT,
                  s_rx.dac_pcm_buf, DAC_FIFO_SAMPLES(config->frame_size));

    /* 3. 设置DAC音量 */
    AudioDAC_VolSet(AUDIO_DAC0, DAC_VOLUME_DEFAULT, DAC_VOLUME_DEFAULT);

    /* 4. 初始化SBC解码器 (2个, 对应2个TX设备) */
    for (int i = 0; i < 2; i++) {
        s_rx.sbc_decoder[i] = AudioCodec_DecoderInit(
            config->sample_rate,
            config->channel_num
        );
    }

    /* 5. 初始化音频关联参数 */
    s_rx.assoc.frame_group_num = NPACK_DEFAULT;
    s_rx.assoc.sbc_buf = s_rx.sbc_buf[0];
    s_rx.assoc.sbc_buf_len = sizeof(s_rx.sbc_buf[0]);

    /* 6. 配置基带参数 (平台相关) */
    // WirelessBbParams_t bb_params = { ... };
    // Wireless_common_init(&bb_params);

    /* 7. 设置设备角色 (Master) */
    // MVWIRE2_DeviceRoleSet(WIRELESS_ROLE_MASTER);

    s_rx.initialized = true;
    return 0;
}

void Wireless_Deinit(void)
{
    if (!s_rx.initialized)
        return;

    for (int i = 0; i < 2; i++) {
        if (s_rx.sbc_decoder[i])
            AudioCodec_DecoderDeinit(s_rx.sbc_decoder[i]);
    }
    AudioDriver_Deinit(AUDIO_ROLE_RX);
    memset(&s_rx, 0, sizeof(s_rx));
}

void Wireless_Schedule(void)
{
    if (!s_rx.initialized)
        return;

    /* 1. 无线协议栈调度 */
    // rwip_schedule();

    /* 2. 音频接收处理 */
    rx_audio_process();
}

int Wireless_StartPairing(void)
{
    s_rx.pairing = true;
    /* RX端: 开始扫描 */
    // wireless_app_start_scan();
    return 0;
}

void Wireless_StopPairing(void)
{
    s_rx.pairing = false;
}

void Wireless_DisconnectAll(void)
{
    for (int i = 0; i < 2; i++) {
        s_rx.dev_status[i] = CONNECT_NONE;
    }
}

ConnectStatus_t Wireless_GetConnectStatus(uint8_t device_index)
{
    if (device_index >= 2)
        return CONNECT_NONE;
    return s_rx.dev_status[device_index];
}

bool Wireless_IsConnected(void)
{
    return s_rx.dev_status[0] >= CONNECT_WIRELESS ||
           s_rx.dev_status[1] >= CONNECT_WIRELESS;
}

uint8_t Wireless_GetConnectedCount(void)
{
    uint8_t count = 0;
    for (int i = 0; i < 2; i++) {
        if (s_rx.dev_status[i] >= CONNECT_WIRELESS)
            count++;
    }
    return count;
}

bool Wireless_RxIsReady(uint8_t device_index)
{
    if (device_index >= 2)
        return false;
    /* 检查RF接收FIFO */
    // return Wireless_TransPacketIsReady(device_index);
    return s_rx.dev_status[device_index] == CONNECT_AUDIO;
}

int Wireless_RxRead(uint8_t device_index, uint8_t *buf, uint16_t max_len)
{
    if (device_index >= 2 || !buf || max_len == 0)
        return -1;
    /* 从RF接收FIFO读取 */
    // return Wireless_TransBufRead(device_index, buf, max_len);
    return 0;
}

/* TX端不需要这些, 但API需要统一 */
bool Wireless_TxIsReady(void) { return false; }
int  Wireless_TxSend(const uint8_t *data, uint16_t len) { (void)data; (void)len; return -1; }

void Wireless_RegisterConnectedCb(WirelessConnectedCb_t cb)
{
    s_rx.connected_cb = cb;
}

void Wireless_RegisterDisconnectedCb(WirelessDisconnectedCb_t cb)
{
    s_rx.disconnected_cb = cb;
}

void Wireless_RegisterRxReadyCb(WirelessRxReadyCb_t cb)
{
    s_rx.rx_ready_cb = cb;
}

void Wireless_SetFreqBand(uint8_t band) { (void)band; }
void Wireless_SetChannel(uint8_t channel) { (void)channel; }
void Wireless_SetDeviceAddr(const uint8_t addr[6]) { (void)addr; }
void Wireless_Sleep(void) {}
void Wireless_Active(void) {}

/* 音频处理入口 (供主循环调用) */
void AudioCodec_RxProcess(void)
{
    rx_audio_process();
}
