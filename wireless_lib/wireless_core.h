/**
 ******************************************************************************
 * @file    wireless_core.h
 * @brief   无线核心数据结构与类型定义
 *
 * 来源: wireless/wireless_usr_type.h + wireless/audio_association.h
 ******************************************************************************
 */
#ifndef __WIRELESS_CORE_H__
#define __WIRELESS_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "wireless_config.h"

/*===========================================================================
 * 设备角色
 *===========================================================================*/
enum wireless_app_task_role {
    WIRELESS_ROLE_MASTER = 0,    /* RX端: 主机(扫描+连接) */
    WIRELESS_ROLE_SLAVE  = 1,    /* TX端: 从机(广播+等待连接) */
};

/*===========================================================================
 * 无线地址
 *===========================================================================*/
struct wireless_addr {
    uint8_t addr[6];    /* BD地址 */
};

/*===========================================================================
 * 连接状态
 *===========================================================================*/
typedef enum {
    CONNECT_NONE     = 0,   /* 未连接 */
    CONNECT_WIRELESS = 1,   /* 无线已连接, 音频未通 */
    CONNECT_AUDIO    = 2,   /* 音频已通 */
} ConnectStatus_t;

/*===========================================================================
 * 同步包状态 (RX端音频包重组)
 *===========================================================================*/
typedef enum {
    SYNC_PACKET_NONE   = 0,   /* 无包 */
    SYNC_PACKET_START  = 1,   /* 帧起始包 */
    SYNC_PACKET_ID     = 2,   /* ID包 */
    SYNC_PACKET_DECODE = 3,   /* 解码包 */
    SYNC_PACKET_LOST   = 4,   /* 丢包 */
} SyncPacketStatus_t;

/*===========================================================================
 * 设备上下文 (RX端管理多个TX设备)
 *===========================================================================*/
typedef struct {
    uint8_t          handle;          /* 设备句柄 */
    ConnectStatus_t  ConStatus;       /* 连接状态 */
    uint16_t         RecvNum;         /* 接收包计数 */
    uint16_t         PlayFrame;       /* 播放帧计数 */
    uint16_t         PlcFrame;        /* 丢包补偿帧计数 */
    uint16_t         LostFrame;       /* 丢包计数 */
} DeviceContext_t;

/*===========================================================================
 * RX端音频关联参数 (包重组+丢包补偿)
 *===========================================================================*/
typedef struct {
    /* 帧管理 */
    uint8_t          frame_group_num;       /* 每帧分组数 */
    uint8_t          mute_packet_num;        /* 静音包数 */
    uint16_t         encode_out_len;          /* 编码输出长度 */

    /* SBC缓冲 */
    uint8_t         *sbc_buf;                /* SBC数据缓冲区 */
    uint16_t         sbc_buf_len;

    /* PCM输出FIFO */
    int16_t         *pcm_left_fifo;          /* 左声道PCM */
    int16_t         *pcm_right_fifo;         /* 右声道PCM */
    uint16_t         pcm_fifo_size;

    /* 设备上下文 (支持2T1R) */
    DeviceContext_t  device[2];              /* 最多2个TX设备 */

    /* 丢包补偿统计 */
    uint32_t         total_lost_frames;      /* 总丢包帧数 */
    uint32_t         total_play_frames;      /* 总播放帧数 */
} RxAudioAssocParam_t;

/*===========================================================================
 * 无线配置结构
 *===========================================================================*/
typedef struct {
    uint8_t          role;             /* WIRELESS_ROLE_MASTER 或 WIRELESS_ROLE_SLAVE */
    uint32_t         sample_rate;      /* 采样率 (44100) */
    uint16_t         frame_size;       /* 每帧采样数 (128) */
    uint32_t         device_id;        /* 设备ID */
    uint8_t          channel_num;      /* 声道数 (1=单声道, 2=立体声) */
} WirelessConfig_t;

/*===========================================================================
 * 回调函数类型
 *===========================================================================*/
/* 无线连接回调 */
typedef void (*WirelessConnectedCb_t)(uint8_t device_index);
/* 无线断开回调 */
typedef void (*WirelessDisconnectedCb_t)(uint8_t device_index);
/* TX数据就绪回调 (TX端: 编码数据准备好, 可以发送) */
typedef void (*WirelessTxReadyCb_t)(uint8_t *data, uint16_t len);
/* RX数据就绪回调 (RX端: 收到无线音频数据) */
typedef void (*WirelessRxReadyCb_t)(uint8_t device_index, uint8_t *data, uint16_t len);

/*===========================================================================
 * 基带参数
 *===========================================================================*/
typedef struct {
    const char      *localDevName;     /* 设备名称 */
    uint8_t          localDevAddr[6];  /* 设备地址 */
    uint8_t          freqTrim;         /* 频率微调 */
    uint32_t         em_start_addr;    /* EM内存起始地址 */
    /* AGC配置 */
    uint8_t          pAgcDisable;      /* 0=自动AGC, 1=关闭 */
    uint8_t          pAgcLevel;        /* AGC等级 */
    /* Sniff配置 */
    uint8_t          sniffInterval;    /* Sniff间隔 */
} WirelessBbParams_t;

#ifdef __cplusplus
}
#endif

#endif /* __WIRELESS_CORE_H__ */
