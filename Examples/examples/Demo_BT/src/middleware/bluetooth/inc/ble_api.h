/**
 * @file ble_api.h
 * @author Cole (cole@mvsilicon.com)
 * @brief Le API
 * @version 0.1
 * @date 2023-03-09
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef _LE_APP_API_H_
#define _LE_APP_API_H_

#include <stdint.h>
#include <stdio.h>

#define BLE_INFO(fmt, args...)       printf("[BLE_INFO]:" fmt , ##args)
#define BLE_GATT_UUID_128_LEN (16)
#define BLE_DFLT_DEVICE_MAX_NAME_LEN (18)
typedef struct
{
    void (*LeHciInit)(void);
    uint32_t (*LeHciHostReceive)(uint8_t *Buf, uint16_t Len);
    uint32_t (*LeHciControllerSend)(uint8_t *Buf, uint16_t Len);
} ble_transport_t;
/*****************************************************************************************/
typedef struct set_adv_data
{
    uint8_t *adv_data;
    uint16_t adv_len;
} set_adv_data_t;

typedef struct set_rsp_adv_data
{
    uint8_t *adv_rsp_data;
    uint16_t adv_rsp_len;
} set_rsp_adv_data_t;

// 标准16位uuid模型
typedef struct ble_gatt_att16_desc
{
    /// Attribute UUID (16-bit UUID - LSB First)
    uint16_t uuid16;
    /// Attribute information bit field (see enum #gatt_att_info_bf)
    uint16_t info;
    /// Attribute extended information bit field (see enum #gatt_att_ext_info_bf)
    /// Note:
    ///   - For Included Services and Characteristic Declarations, this field contains targeted handle.
    ///   - For Characteristic Extended Properties, this field contains 2 byte value
    ///   - For Client Characteristic Configuration and Server Characteristic Configuration, this field is not used.
    uint16_t ext_info;
} ble_gatt_att16_desc_t;

// 标准自定义128位uuid模型
typedef struct ble_gatt_att128_desc
{
    /// Attribute UUID (LSB First)
    uint8_t uuid[BLE_GATT_UUID_128_LEN];
    /// Attribute information bit field (see enum #gatt_att_info_bf)
    uint16_t info;
    /// Attribute extended information bit field (see enum #gatt_att_ext_info_bf)
    /// Note:
    ///   - For Included Services and Characteristic Declarations, this field contains targeted handle.
    ///   - For Characteristic Extended Properties, this field contains 2 byte value
    ///   - For Client Characteristic Configuration and Server Characteristic Configuration, this field is not used.
    uint16_t ext_info;
} ble_gatt_att128_desc_t;

typedef struct set_adv_interval
{
    uint32_t adv_intv_min;
    uint32_t adv_intv_max;
    uint8_t ch_map;
} set_adv_interval_t;

typedef struct le_init_parameter
{
    set_adv_interval_t adv_interval_param;
    set_rsp_adv_data_t rsp_data;
    set_adv_data_t adv_data;
    uint16_t att_default_mtu;
    uint16_t ble_uuid16_service;
    uint16_t *ble_uuid128_service;
    uint16_t ble_service_idxnb;
    ble_gatt_att128_desc_t *profile_uuid128;
    ble_gatt_att16_desc_t *profile_uuid16;
    uint8_t ble_device_name_len;
    uint8_t ble_device_name[BLE_DFLT_DEVICE_MAX_NAME_LEN];
} le_init_parameter_t;

le_init_parameter_t le_user_config;
/***********************************************************************************************/
typedef enum
{
	LE_INIT_STATUS = 0,
    LE_CONNECTED,
    LE_DISCONNECT,
    LE_CONNECT_PARAMS_UPDATE,
	LE_MTU_EXCHANGE_RESULT,
    LE_RCV_DATA_EVENT,
    LE_APP_READ_DATA_EVENT,

} LE_CB_EVENT;

typedef struct le_addr
{
    /// BD Address of device
    uint8_t addr[6];
} le_addr_t;

typedef struct ble_app_read_data
{
    uint8_t *data;
    /// value handle
    uint16_t handle;
    /// len
    uint16_t len;
} ble_app_read_data_t;
ble_app_read_data_t ble_read_data;

typedef struct ble_rcv_data
{
    /// data
    uint8_t *data;
    /// Connection handle
    uint16_t conhdl;
    /// value handle
    uint16_t handle;
    /// len
    uint32_t len;
} ble_rcv_data_t;

typedef struct le_disconnect_params
{
    /// Connection index
    uint8_t conidx;
    /// Connection handle
    uint16_t conhdl;
    /// Reason of disconnection
    uint16_t reason;
} le_disconnect_params_t;

typedef struct _le_connected_complete
{
    /// Connection index
    uint8_t conidx;
    /// Connection handle
    uint16_t conhdl;
    /// Connection interval
    uint16_t con_interval;
    /// Connection latency
    uint16_t con_latency;
    /// Link supervision timeout
    uint16_t sup_to;
    /// Clock accuracy
    uint8_t clk_accuracy;
    /// Peer address type
    uint8_t peer_addr_type;
    /// Peer BT address
    le_addr_t peer_addr;
    /// Role of device in connection (0 = Central / 1 = Peripheral)
    uint8_t role;
} le_connected_complete;

typedef struct _le_con_update_params
{
    /// LE Subevent code
    uint8_t subcode;
    /// Status of received command
    uint8_t status;
    /// Connection handle
    uint16_t conhdl;
    /// Connection interval value
    uint16_t con_interval;
    /// Connection latency value
    uint16_t con_latency;
    /// Supervision timeout
    uint16_t sup_to;
} le_con_update_params;

typedef struct _LE_CB_PARAMS
{

    le_disconnect_params_t *dis_params;
    le_connected_complete *con_params;
    le_con_update_params *con_update_param;
    uint16_t le_init_status;
    uint16_t mtu_size;
    ble_rcv_data_t rcv_data;
    uint16_t att_handle;
} LE_CB_PARAMS;

typedef void (*LeParamCB)(LE_CB_EVENT event, LE_CB_PARAMS *param);

LeParamCB LeCallBackAPP;
/********************************************************************************************/
/*****************************固化API，非必要禁止改动******************************/
/**
 * @brief 注册函数
 *
 */
void LeAppRegCB(LeParamCB LeCBFunc);
/**
 * @brief 初始化BLE基础参数
 *
 * @return uint8_t
 */
uint8_t LeInitConfigParams(void);

/**
 * @brief Le事件回调函数
 *
 * @param event
 * @param param
 */
void AppEventCallBack(LE_CB_EVENT event, LE_CB_PARAMS *param);
/*****************************固化API，非必要禁止改动******************************/
/********************************************************************************/

/**
 * @brief @brief 设置广播数据(MAx LEN < 31)
 *
 * @param data
 * @param len
 */
void app_set_adv_data(uint8_t *data, uint16_t len);
/**
 * @brief @brief 设置广播回复数据(MAx LEN < 31)
 *
 * @param data
 * @param len
 */
void app_set_scan_rsp_data(uint8_t *data, uint16_t len);
/**
 * @brief           发送数据接口 
 *
 * @param conidx    0x00
 * @param user_lid  0x00
 * @param metainfo  0x00
 * @param hdl       属性handle-1为value handle
 * @param p         数据
 * @param len       长度
 * @param nf_or_nd  0x00 GATT_NOTIFY, 0x01 GATT_INDICATE 方式
 * @return uint32_t
 */
uint32_t ble_send_data(uint8_t conidx, uint8_t user_lid, uint16_t metainfo, uint16_t hdl, uint8_t *p, uint32_t len, uint8_t nf_or_nd);
/**
 * @brief 主动断开连接
 *
 */
void ble_app_disconnect(void);
/**
 * @brief 开始广播
 *
 */
void app_start_advertising(void);
/**
 * @brief 停止广播
 *
 */
void app_stop_advertising(void);
/**
 * @brief 设置广播间隔
 * 
 * @param adv_param 
 */
void app_set_adv_param(set_adv_interval_t adv_param);

/**
 * @brief Send to request to update the connection parameters
 * 
 */
typedef struct le_connection_param
{
    /// Connection interval minimum
    uint16_t intv_min;
    /// Connection interval maximum
    uint16_t intv_max;
    /// Latency
    uint16_t latency;
    /// Supervision timeout
    uint16_t time_out;
} le_connection_param_t;
/**
 * @brief 更新连接参数 
 * 
 */
void app_update_param(struct le_connection_param *p_conn_param);
/**
 * @brief 获得当前设备名
 * 
 * @param p_name 
 * @return uint8_t 
 */
uint8_t app_get_dev_name(uint8_t* p_name);
#endif
