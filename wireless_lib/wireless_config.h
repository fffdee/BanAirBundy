/**
 ******************************************************************************
 * @file    wireless_config.h
 * @brief   无线麦克风系统配置
 *
 * 从 system_config/app_config.h 中提炼的核心配置宏
 ******************************************************************************
 */
#ifndef __WIRELESS_CONFIG_H__
#define __WIRELESS_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 系统方案选择
 *===========================================================================*/
/* 无线Turnkey方案: 2T1R (2个发射 + 1个接收, 单向单声道) */
#define WIRELESS_TURNKEY2_6

/*===========================================================================
 * 音频参数配置
 *===========================================================================*/
/* 采样率 */
#define SAMPLE_RATE              44100

/* 每帧采样数 (一帧音频的处理单位) */
#define ONE_FRAME                128

/* SBC编码配置 */
#define SBC_CHANNEL_MODE         SBC_MODE_MONO      /* 单声道 */
#define SBC_ALLOCATION_METHOD    SBC_ALLOCATION_SNR /* SNR分配 */
#define SBC_SUB_BANDS            8                   /* 子带数 */
#define SBC_BLOCK_SIZE           16                  /* 块大小 */
#define SBC_BITPOOL              31                  /* 位池 (音质/码率平衡) */

/* 编码声道数 (1=单声道, 2=立体声) */
#define ENCODE_CH                1

/*===========================================================================
 * 无线协议配置
 *===========================================================================*/
/* 设备角色 */
#define WIRELESS_ROLE_MASTER     0   /* RX端: 主机 */
#define WIRELESS_ROLE_SLAVE      1   /* TX端: 从机 */

/* 无线配对密钥 */
#define COMPANY_BYTE0            0x77
#define COMPANY_BYTE1            0x88
#define COMPANY_BYTE2            0x99
#define COMPANY_BYTE3            0xAA
#define WIRELESS_LINK_KEY0       0x12
#define WIRELESS_LINK_KEY1       0x34

/* 无线RF配置 */
#define RF_FREQ_BAND             0   /* 0=2.4GHz, 1=2.3GHz */
#define RF_CHANNEL_NUM           40  /* 频道数 */
#define RF_INTERVAL              6   /* RF间隔(ms) */

/* EM内存大小 (基带事件内存) */
#define BB_EM_SIZE               (16 * 1024)

/*===========================================================================
 * 音频缓冲区配置
 *===========================================================================*/
/* 无线音频传输包长度 (SBC编码后) */
#define RFAUDIO_TRANS_A          120

/* 麦克风FIFO采样数 */
#define MIC_FIFO_SAMPLES(frame)  ((frame) * 4)

/* DAC FIFO采样数 */
#define DAC_FIFO_SAMPLES(frame)  ((frame) * 4)

/* 无线发送包数 (每个音频帧的RF分包数) */
#define NPACK_DEFAULT            3

/*===========================================================================
 * 功能开关
 *===========================================================================*/
/* 双向音频通道 (TX端也能接收RX端的反向音频) */
#define PACKET_AUDIO_CH_BACKWARD 2

/* 音效处理 (本最小库不包含, 设为0) */
#define CFG_FUNC_AUDIO_EFFECT_EN 0

/* 音效参数覆盖Codec设置 (本最小库不包含) */
#define CFG_FUNC_AUDIO_EFFECT_SET_CODEC_EN 0

/*===========================================================================
 * 音量/增益默认值
 *===========================================================================*/
/* 系统音量等级数 */
#define CFG_PARA_MAX_VOLUME_NUM  32

/* 默认音量等级 (0~CFG_PARA_MAX_VOLUME_NUM) */
#define CFG_PARA_SYS_VOLUME_DEFAULT  (CFG_PARA_MAX_VOLUME_NUM)

/* 麦克风PGA增益 (0~31, 31=最大+26.5dB) */
#define MIC_PGA_GAIN_DEFAULT     31

/* DAC输出音量 (0~0x3FFF, 0x3FFF=最大) */
#define DAC_VOLUME_DEFAULT       0x3FFF

#ifdef __cplusplus
}
#endif

#endif /* __WIRELESS_CONFIG_H__ */
