/**
 ******************************************************************************
 * @file    wireless_config.h
 * @brief   无线麦克风系统配置（默认对齐 Turnkey 2_6）
 *
 * 板级 app_config.h 可覆盖同名宏；本文件只提供默认值。
 ******************************************************************************
 */
#ifndef __WIRELESS_CONFIG_H__
#define __WIRELESS_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIRELESS_TURNKEY2_6
#define WIRELESS_TURNKEY2_6
#endif

#ifndef SAMPLE_RATE
#define SAMPLE_RATE              44100
#endif

#ifndef ONE_FRAME
#define ONE_FRAME                128
#endif

#ifndef SBC_CHANNEL_MODE
#define SBC_CHANNEL_MODE         0   /* SBC_MODE_MONO */
#endif

#ifndef SBC_ALLOCATION_METHOD
#define SBC_ALLOCATION_METHOD    1   /* SBC_ALLOCATION_SNR */
#endif

#ifndef SBC_SUB_BANDS
#define SBC_SUB_BANDS            8
#endif

#ifndef SBC_BLOCK_SIZE
#define SBC_BLOCK_SIZE           16
#endif

/* Turnkey 2_6: AUDIO_QUALITY=20 */
#ifndef SBC_BITPOOL
#define SBC_BITPOOL              20
#endif

#ifndef ENCODE_CH
#define ENCODE_CH                1
#endif

#ifndef COMPANY_BYTE0
#define COMPANY_BYTE0            0x77
#endif
#ifndef COMPANY_BYTE1
#define COMPANY_BYTE1            0x88
#endif
#ifndef COMPANY_BYTE2
#define COMPANY_BYTE2            0x65
#endif
#ifndef COMPANY_BYTE3
#define COMPANY_BYTE3            0x38
#endif

#ifndef WIRELESS_LINK_KEY0
#define WIRELESS_LINK_KEY0       0x21
#endif
#ifndef WIRELESS_LINK_KEY1
#define WIRELESS_LINK_KEY1       0x56
#endif

#ifndef RF_FREQ_BAND
#define RF_FREQ_BAND             0
#endif
#ifndef RF_CHANNEL_NUM
#define RF_CHANNEL_NUM           40
#endif
#ifndef RF_INTERVAL
#define RF_INTERVAL              6
#endif

#ifndef BB_EM_SIZE
#define BB_EM_SIZE               (16 * 1024)
#endif

/*
 * Turnkey 2_6 mono bitpool20: SBC=48B, RF packet=52B
 * (若已含 sbc_encoder.h 则以其中 RFAUDIO_TRANS_A 为准)
 */
#ifndef RFAUDIO_TRANS_A
#define RFAUDIO_TRANS_A          52
#endif

#ifndef CRC_PACKSUB
#define CRC_PACKSUB              0
#endif

#ifndef MIC_FIFO_SAMPLES
#define MIC_FIFO_SAMPLES(frame)  ((frame) * 4)
#endif

#ifndef DAC_FIFO_SAMPLES
#define DAC_FIFO_SAMPLES(frame)  ((frame) * 4)
#endif

#ifndef NPACK_DEFAULT
#define NPACK_DEFAULT            3
#endif

#ifndef PACKET_AUDIO_CH_BACKWARD
/* 单向 2_6：关闭回传宏（勿定义为正值通道数） */
#endif

#ifndef CFG_FUNC_AUDIO_EFFECT_EN
#define CFG_FUNC_AUDIO_EFFECT_EN 0
#endif

#ifndef CFG_PARA_MAX_VOLUME_NUM
#define CFG_PARA_MAX_VOLUME_NUM  16
#endif

#ifndef CFG_PARA_SYS_VOLUME_DEFAULT
#define CFG_PARA_SYS_VOLUME_DEFAULT  (CFG_PARA_MAX_VOLUME_NUM)
#endif

#ifndef MIC_PGA_GAIN_DEFAULT
#define MIC_PGA_GAIN_DEFAULT     31
#endif

#ifndef DAC_VOLUME_DEFAULT
#define DAC_VOLUME_DEFAULT       0x3FFF
#endif

#ifdef __cplusplus
}
#endif

#endif /* __WIRELESS_CONFIG_H__ */
