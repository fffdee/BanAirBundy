/*
 * wireless_usr_type.h
 *
 *  Created on: Nov 2, 2023
 *      Author: richard
 */

#ifndef _WIRELESS_USR_TYPE_H_
#define _WIRELESS_USR_TYPE_H_
#include <stdint.h>


enum wireless_app_task_role
{
	WIRELESS_APP_TASK_MASTER,
	WIRELESS_APP_TASK_SLAVE
};

struct wireless_addr
{
    ///6-byte array address value
    uint8_t  addr[6];
};

struct app_wireless_scan_result_t {
	struct wireless_addr *adv_addr;
	uint8_t addr_type;
	uint8_t is_scan_rsp;
	uint8_t adv_len;
	uint8_t* adv_data;
};

struct app_wireless_err_info_t {
	uint32_t sync_err_cnt;
	uint32_t crc_err_cnt;
};

// APP level wireless connected call back
typedef void (*app_wireless_conn_cb_fn) (uint8_t *addr, uint16_t handle,uint8_t role);

// App level wireless disconnected call back
typedef void (*app_wireless_dis_conn_cb_fn) (uint16_t handle,uint8_t reason);

typedef void (*app_wireless_scan_result_cb_fn) (struct  app_wireless_scan_result_t *pScanRes);

typedef void (*lld_con_tx_ready_cb_fn) (uint16_t con_hdl, uint8_t empty_em_cnt, uint16_t empty_em_size);

typedef void (*lld_con_rx_ready_cb_fn)(uint16_t link_id, uint8_t *p,uint16_t len);

#endif /* _WIRELESS_USR_TYPE_H_ */
