# wireless_lib - 无线麦克风音频收发最小代码库

# 移植到 boot_app

`boot_app` 已通过链接资源 `wireless_lib/` 接入本库：

1. `app_config.h`：`BOOT_APP_WIRELESS_EN` / `BOOT_APP_WIRELESS_ROLE_TX`
2. `src/wireless_app.c`：FreeRTOS 任务周期调用 `Wireless_Schedule()`
3. 角色二选一：`ROLE_TX=0` 编译 RX（`wireless_rx.c`），`=1` 编译 TX（`wireless_tx.c`）

当前为 **Phase 0 骨架**：ADC/DAC/SBC/MVWIRE2 仍为 stub，可先验证任务与链接。
后续按 `porting_guide.md` 对接 `libDriver` / SBC / BtStack。

---

## 概述

本文件夹从 `wireless_mic_tx_sdk` 和 `wireless_mic_rx_sdk` 中提炼了**无线音频收发**的核心最小代码集，
去除了 HMI、OTA、USB、音效处理(roboeffect)、Shell 等非核心功能，保留无线麦克风系统最本质的部分。

## 芯片平台

- **MCU**: MVsB1 / BP1532 (Andes NDS32 架构)
- **无线协议**: MVWIRE2 (2.4GHz 自定义协议栈)
- **音频编码**: SBC (蓝牙标准编码)
- **系统配置**: WIRELESS_TURNKEY2_6 (2T1R, 44.1kHz, ~16ms 延迟)

## 目录结构

```
wireless_lib/
├── README.md                  ← 本说明文件
├── wireless_api.h             ← 无线协议栈统一API (TX/RX共用)
├── wireless_config.h          ← 无线系统配置宏定义
├── wireless_core.h            ← 无线核心数据结构与类型
├── wireless_tx.c              ← TX端: 麦克风采集→SBC编码→无线发送
├── wireless_rx.c              ← RX端: 无线接收→SBC解码→DAC输出
├── audio_codec_api.h          ← SBC编解码API
├── audio_codec_api.c          ← SBC编解码封装实现
├── audio_driver_api.h         ← ADC/DAC驱动抽象层
├── audio_driver_api.c         ← ADC/DAC驱动接口实现
└── porting_guide.md           ← 移植指南
```

## 数据流架构

### TX端 (发射端/麦克风端)

```
MIC ──> ADC1(PGA) ──> DMA FIFO ──> PCM缓冲区
                                        │
                                        ▼
                              upmix_1to2 (单声道→立体声)
                                        │
                                        ▼
                              SBC编码器 (wireless_sbc_encoder_apply)
                                        │
                                        ▼
                              无线发送 FIFO (lld_con_wireless_tx_send)
                                        │
                                        ▼
                              2.4GHz RF 发送
```

### RX端 (接收端/喇叭端)

```
2.4GHz RF 接收
        │
        ▼
无线接收 FIFO (Wireless_TransBufRead)
        │
        ▼
音频包重组 (AudioAssociationProcess) ← 丢包补偿(PLC)
        │
        ▼
SBC解码器 (wireless_sbc_decoder_apply)
        │
        ▼
PCM缓冲区 ──> DMA FIFO ──> DAC0 ──> 喇叭/耳机
```

## 核心API使用示例

### TX端初始化与运行

```c
#include "wireless_api.h"

// 1. 初始化无线(从机模式)
WirelessConfig_t cfg = {
    .role = WIRELESS_ROLE_SLAVE,      // TX=从机
    .sample_rate = 44100,
    .frame_size = 128,                 // 每帧采样数
    .device_id = 0x00000001,
};
Wireless_Init(&cfg);

// 2. 初始化音频(ADC采集)
AudioCodec_Init(AUDIO_ROLE_TX);

// 3. 主循环
while (1) {
    Wireless_Schedule();    // 无线协议栈调度
    AudioCodec_TxProcess(); // 采集→编码→发送
}
```

### RX端初始化与运行

```c
#include "wireless_api.h"

// 1. 初始化无线(主机模式)
WirelessConfig_t cfg = {
    .role = WIRELESS_ROLE_MASTER,     // RX=主机
    .sample_rate = 44100,
    .frame_size = 128,
    .device_id = 0x00000002,
};
Wireless_Init(&cfg);

// 2. 初始化音频(DAC输出)
AudioCodec_Init(AUDIO_ROLE_RX);

// 3. 主循环
while (1) {
    Wireless_Schedule();    // 无线协议栈调度
    AudioCodec_RxProcess(); // 接收→解码→输出
}
```

## 移植说明

详见 `porting_guide.md`。核心需要实现以下平台相关函数:

1. **ADC驱动**: `WlAudioAdc_AnaInit()`, `WlAudioAdc_DigitalInit()`, `WlAudioAdc_DataGet()`
2. **DAC驱动**: `WlAudioDac_Init()`, `WlAudioDac_DataSet()`
   （`Wl*` 前缀避免与 `libDriver` 的 `AudioADC_*` / `AudioDAC_*` 冲突）
3. **无线驱动**: `MVWIRE2_Init()`, `lld_con_wireless_tx_send()`, `Wireless_TransBufRead()`
4. **SBC编解码**: `sbc_encoder_init()`, `sbc_encoder()`, `sbc_decoder_init()`, `sbc_decoder()`
5. **DMA配置**: `Dma_Init()`, `Dma_ChannelEnable()`

## 原始工程对应关系

| wireless_lib文件 | 原始工程来源 |
|-----------------|-------------|
| wireless_api.h | wireless/wireless2.h + wireless_usr_api.h + bb_api.h |
| wireless_config.h | system_config/app_config.h (核心宏) |
| wireless_core.h | wireless/wireless_usr_type.h + audio_association.h |
| wireless_tx.c | wireless/wireless_main.c + audio/audio_main.c (TX部分) |
| wireless_rx.c | wireless/wireless_main.c + audio/audio_main.c (RX部分) + audio_association.c |
| audio_codec_api.h/.c | sbc/sbc_api.h/.c |
| audio_driver_api.h/.c | driver/driver_api/src/adc_interface.c + dac_interface.c |
