/**
 ******************************************************************************
 * @file    wireless_api.h
 * @brief   无线协议栈统一API (TX/RX共用)
 *
 * 来源: wireless/wireless2.h + wireless/wireless_usr_api.h + wireless/bb_api.h
 ******************************************************************************
 */
#ifndef __WIRELESS_API_H__
#define __WIRELESS_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "wireless_core.h"

/*===========================================================================
 * 无线协议栈 初始化与控制
 *===========================================================================*/

/**
 * @brief  初始化无线协议栈
 * @param  config: 无线配置参数
 * @retval 0=成功, <0=失败
 *
 * 内部执行:
 *   1. 配置BB基带参数(AGC, EM内存等)
 *   2. 设置设备角色(Master/Slave)
 *   3. 设置设备地址和配对密钥
 *   4. 初始化协议栈
 */
int Wireless_Init(const WirelessConfig_t *config);

/**
 * @brief  反初始化无线协议栈
 */
void Wireless_Deinit(void);

/**
 * @brief  无线协议栈调度 (主循环中调用)
 * @note   必须在主循环中持续调用, 处理RF收发、配对、连接管理
 */
void Wireless_Schedule(void);

/**
 * @brief  开始广播/扫描 (启动配对)
 * @retval 0=成功
 *
 * TX端: 开始广播, 等待RX连接
 * RX端: 开始扫描, 搜索TX并连接
 */
int Wireless_StartPairing(void);

/**
 * @brief  停止广播/扫描
 */
void Wireless_StopPairing(void);

/**
 * @brief  断开所有连接
 */
void Wireless_DisconnectAll(void);

/*===========================================================================
 * 连接状态查询
 *===========================================================================*/

/**
 * @brief  获取设备连接状态
 * @param  device_index: 设备索引 (0或1, 2T1R模式支持2个TX)
 * @retval CONNECT_NONE / CONNECT_WIRELESS / CONNECT_AUDIO
 */
ConnectStatus_t Wireless_GetConnectStatus(uint8_t device_index);

/**
 * @brief  是否已连接 (任意设备)
 */
bool Wireless_IsConnected(void);

/**
 * @brief  获取已连接设备数量
 */
uint8_t Wireless_GetConnectedCount(void);

/*===========================================================================
 * 数据收发
 *===========================================================================*/

/**
 * @brief  检查无线发送是否就绪 (TX端)
 * @retval true=可以发送数据
 */
bool Wireless_TxIsReady(void);

/**
 * @brief  发送音频数据 (TX端)
 * @param  data: 数据指针
 * @param  len: 数据长度
 * @retval 0=成功, <0=失败
 *
 * 内部调用 lld_con_wireless_tx_send() 将数据通过RF发送
 */
int Wireless_TxSend(const uint8_t *data, uint16_t len);

/**
 * @brief  检查是否有接收数据 (RX端)
 * @param  device_index: 设备索引
 * @retval true=有数据可读
 */
bool Wireless_RxIsReady(uint8_t device_index);

/**
 * @brief  读取接收数据 (RX端)
 * @param  device_index: 设备索引
 * @param  buf: 输出缓冲区
 * @param  max_len: 缓冲区最大长度
 * @retval 实际读取长度, <0=错误
 */
int Wireless_RxRead(uint8_t device_index, uint8_t *buf, uint16_t max_len);

/*===========================================================================
 * 回调注册
 *===========================================================================*/

/**
 * @brief  注册连接回调
 */
void Wireless_RegisterConnectedCb(WirelessConnectedCb_t cb);

/**
 * @brief  注册断开回调
 */
void Wireless_RegisterDisconnectedCb(WirelessDisconnectedCb_t cb);

/**
 * @brief  注册RX数据就绪回调 (替代轮询)
 */
void Wireless_RegisterRxReadyCb(WirelessRxReadyCb_t cb);

/*===========================================================================
 * RF配置
 *===========================================================================*/

/**
 * @brief  设置频段
 * @param  band: 0=2.4GHz, 1=2.3GHz
 */
void Wireless_SetFreqBand(uint8_t band);

/**
 * @brief  设置RF频道
 * @param  channel: 频道号 (0~RF_CHANNEL_NUM-1)
 */
void Wireless_SetChannel(uint8_t channel);

/**
 * @brief  设置设备地址
 * @param  addr: 6字节地址
 */
void Wireless_SetDeviceAddr(const uint8_t addr[6]);

/*===========================================================================
 * Sleep/Wakeup
 *===========================================================================*/

/**
 * @brief  无线进入睡眠 (低功耗)
 */
void Wireless_Sleep(void);

/**
 * @brief  无线唤醒
 */
void Wireless_Active(void);

#ifdef __cplusplus
}
#endif

#endif /* __WIRELESS_API_H__ */
