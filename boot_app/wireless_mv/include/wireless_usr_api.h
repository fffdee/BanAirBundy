/*
 * wireless_usr_api.h
 *
 *  Created on: Nov 2, 2023
 *      Author: richard
 */

#ifndef _WIRELESS_USR_API_H_
#define _WIRELESS_USR_API_H_
#include <stdint.h>
#include "wireless_usr_type.h"

uint8_t set_local_bdaddr(uint8_t *addr);

//参数0-所有频点;1-低频段;2-高频段
//注意是对1-x系列有效，默认0，--全频段
//在Wireless_common_init()之前调用
void sub_band_config(uint8_t value);

//参数:0-2.4G;1-2.3G
//注意是对1-x和2-x系列有效，默认1--2.3G
void set_OOB_band(uint8_t band);

// 0-2.4G;1-2.3G
uint8_t get_OOB_band();

// mode: 0 -- single way, slave  --> master; max payload 116
//       1 -- dual way,  master <--> slave;max payload  112+56
//       other -- for future usage
void wireless_set_trx_mode(uint8_t mode);

uint8_t* wireless_app_get_adv_data(uint8_t *adv_len);

/**
 * return： 0
 */
uint8_t wireless_app_start_adv(app_wireless_conn_cb_fn conn_notify_cb);

/**
 * return: 0
 */
uint8_t wireless_app_stop_adv(void);

uint8_t wireless_app_start_scan(app_wireless_scan_result_cb_fn res_cb);

uint8_t wireless_app_stop_scan(void);

/**
 * return 0
 */
uint8_t wireless_app_create_con(struct wireless_addr *adv_addr, app_wireless_conn_cb_fn conn_notify_cb);

uint8_t wireless_app_reg_disc_ind_cb(app_wireless_dis_conn_cb_fn dis_conn_notify_cb);

uint8_t wireless_app_dis_con(uint16_t handle);

void rwip_schedule(void);

/****
 * link callback function
 * addr: @DEFAULT_BDADDR
 * handle:WIRELESS connection handles
 * role:@enum wireless_app_task_role
 */
void wireless_app_test_conn_ind(uint8_t *addr, uint16_t handle,uint8_t role);

uint8_t lld_con_wireless_tx_send(uint8_t *p, uint16_t len, uint16_t con_hdl);

uint8_t lld_con_register_tx_ready_callback(uint16_t con_hdl, lld_con_tx_ready_cb_fn cb);

uint8_t lld_con_register_rx_ready_callback(uint16_t con_hdl, lld_con_rx_ready_cb_fn cb);

void lld_con_get_err_info(uint16_t con_hdl, struct app_wireless_err_info_t * err_info);

void lld_con_clear_err_info(uint16_t con_hdl);

const unsigned char *GetLibVersionWireless(void);

const unsigned char *GetLibVersionWireless2(void);

#endif /* _WIRELESS_USR_API_H_ */
