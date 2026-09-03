# 移植指南

## 概述

`wireless_lib` 是平台无关的无线麦克风音频收发最小代码库。要移植到具体平台，需要实现以下平台相关接口。

## 移植步骤

### 1. 实现音频驱动 (`audio_driver_api.c`)

这是最核心的移植工作。需要实现以下函数：

#### ADC (TX端麦克风采集)

| wireless_lib 函数 | 平台 SDK 对应 | 说明 |
|------|------------|------|
| `WlAudioAdc_AnaInit()` | `AudioADC_AnaInit()` | 配置PGA增益、输入通道、上电 |
| `WlAudioAdc_DigitalInit()` | `AudioADC_DigitalInit()` | 配置采样率、DMA FIFO |
| `WlAudioAdc_VolSet()` | `AudioADC_VolSet()` | 数字音量 (0x001~0xFFF) |
| `WlAudioAdc_DataLenGet()` | `AudioADC1_DataLenGet()` | DMA FIFO可读数据量 |
| `WlAudioAdc_DataGet()` | `AudioADC1_DataGet()` | 从DMA FIFO读取PCM |

**注意**: wireless_lib 使用 `WlAudio*` 前缀，避免与 `libDriver.a` 中的 SDK 符号重名。

#### DAC (RX端音频输出)

| wireless_lib 函数 | 平台 SDK 对应 | 说明 |
|------|------------|------|
| `WlAudioDac_Init()` | `AudioDAC_Init()` | 配置采样率、DMA FIFO、上电 |
| `WlAudioDac_VolSet()` | `AudioDAC_VolSet()` | 数字音量 (0~0x3FFF) |
| `WlAudioDac_DataSet()` | `AudioDAC0_DataSet()` | 写入PCM到DMA FIFO |
| `WlAudioDac_Mute()` | `AudioDAC_SoftMute()` | 软静音 |

### 2. 实现SBC编解码 (`audio_codec_api.c`)

需要对接平台SDK的SBC编解码库：

| 函数 | 平台SDK对应 |
|------|------------|
| `AudioCodec_EncoderInit()` | `sbc_encoder_init()` |
| `AudioCodec_Encode()` | `sbc_encoder()` |
| `AudioCodec_DecoderInit()` | `sbc_decoder_init()` |
| `AudioCodec_Decode()` | `sbc_decoder()` |

### 3. 实现无线协议栈 (`wireless_tx.c` / `wireless_rx.c`)

需要对接平台SDK的MVWIRE2协议栈：

| wireless_lib函数 | 平台SDK对应 |
|-----------------|------------|
| `Wireless_Init()` | `Wireless_common_init()` + `MVWIRE2_Init()` |
| `Wireless_Schedule()` | `rwip_schedule()` |
| `Wireless_StartPairing()` | `wireless_app_start_adv()` / `wireless_app_start_scan()` |
| `Wireless_TxSend()` | `lld_con_wireless_tx_send()` |
| `Wireless_RxRead()` | `Wireless_TransBufRead()` |
| `Wireless_TxIsReady()` | `Wireless_TransPacketIsReady()` |

### 4. 配置参数 (`wireless_config.h`)

根据实际需求修改：
- `SAMPLE_RATE`: 采样率 (44100)
- `ONE_FRAME`: 每帧采样数 (128)
- `SBC_BITPOOL`: SBC位池 (31=标准, 越大音质越好码率越高)
- `MIC_PGA_GAIN_DEFAULT`: 麦克风增益 (31=最大)
- `DAC_VOLUME_DEFAULT`: DAC音量 (0x3FFF=最大)

## 主循环示例

```c
#include "wireless_api.h"

int main(void)
{
    /* 1. 硬件初始化 (时钟、GPIO、中断等) */
    Platform_HwInit();

    /* 2. 初始化无线 */
    WirelessConfig_t cfg = {
        .role = WIRELESS_ROLE_SLAVE,  /* TX端 */
        .sample_rate = 44100,
        .frame_size = 128,
        .device_id = 0x00000001,
        .channel_num = 1,
    };
    Wireless_Init(&cfg);

    /* 3. 开始配对 */
    Wireless_StartPairing();

    /* 4. 主循环 */
    while (1) {
        Wireless_Schedule();     /* 无线调度+音频处理 */

        /* 或者分开调用 */
        // Wireless_Schedule();    /* 仅无线调度 */
        // AudioCodec_TxProcess(); /* 仅音频处理(TX) */
        // AudioCodec_RxProcess(); /* 仅音频处理(RX) */
    }
}
```

## 关键参数调优

### 音量太小

1. **麦克风增益**: 增大 `MIC_PGA_GAIN_DEFAULT` (最大31 = +26.5dB)
2. **ADC数字音量**: `AudioADC_VolSet(ADC1, 0xFFF, 0xFFF)` (0xFFF = 0dB最大)
3. **DAC输出音量**: 增大 `DAC_VOLUME_DEFAULT` (最大0x3FFF)
4. **SBC位池**: 增大 `SBC_BITPOOL` (提高编码质量)

### 音质问题

1. **杂音/失真**: 降低PGA增益 (避免削波)
2. **延迟太大**: 减小 `ONE_FRAME` (但会增加CPU占用)
3. **断续/丢包**: 检查RF信号强度，减小 `SBC_BITPOOL` 降低码率

## 原始工程文件对应

```
wireless_lib/              ← 原始工程
├── wireless_api.h         ← wireless/wireless2.h + wireless_usr_api.h + bb_api.h
├── wireless_config.h      ← system_config/app_config.h
├── wireless_core.h        ← wireless/wireless_usr_type.h + audio_association.h
├── wireless_tx.c          ← wireless/wireless_main.c(TX) + audio/audio_main.c(TX)
├── wireless_rx.c          ← wireless/wireless_main.c(RX) + audio/audio_main.c(RX) + audio_association.c
├── audio_codec_api.h/.c   ← sbc/sbc_api.h/.c
├── audio_driver_api.h/.c  ← driver/driver_api/src/adc_interface.c + dac_interface.c
└── porting_guide.md       ← (新增)
```
