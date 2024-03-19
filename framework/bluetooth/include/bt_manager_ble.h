/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt ble manager.
*/

#ifndef __BT_MANAGER_BLE_H__
#define __BT_MANAGER_BLE_H__
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/gatt.h>

/**
 * @defgroup bt_manager_ble_apis Bt Manager Ble APIs
 * @ingroup bluetooth_system_apis
 * @{
 */

struct ble_advertising_params_t
{
	uint16_t  advertising_interval_min;
	uint16_t  advertising_interval_max;
	uint8_t   advertising_type;
	uint8_t   own_address_type;
	uint8_t   peer_address_type;
	uint8_t   peer_address[6];
	uint8_t   advertising_channel_map;
	uint8_t   advertising_filter_policy;
} __packed;

/** ble register manager structure */
struct ble_reg_manager {
	void (*link_cb)(uint8_t *mac, uint8_t connected);
	struct bt_gatt_service gatt_svc;
	sys_snode_t node;
};

/**
 * @brief get ble mtu
 *
 * This routine provides to get ble mtu
 *
 * @return ble mtu
 */
uint16_t bt_manager_get_ble_mtu(bt_addr_t* addr);

/**
 * @brief bt manager send ble data
 *
 * This routine provides to bt manager send ble data
 *
 * @param chrc_attr
 * @param des_attr 
 * @param data pointer of send data
 * @param len length of data
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_ble_send_data(bt_addr_t* addr, struct bt_gatt_attr *chrc_attr,
					struct bt_gatt_attr *des_attr, uint8_t *data, uint16_t len);

/**
 * @brief ble disconnect
 *
 * This routine do ble disconnect
 *
 * @return  N/A
 */
void bt_manager_ble_disconnect(bt_addr_t* addr);

/**
 * @brief ble service register
 *
 * This routine provides ble service register
 *
 * @param le_mgr ble register info 
 *
 * @return  N/A
 */
void bt_manager_ble_service_reg(struct ble_reg_manager *le_mgr);

/**
 * @brief init btmanager ble
 *
 * This routine init btmanager ble
 *
 * @return  N/A
 */
void bt_manager_ble_init(void);

/**
 * @brief notify ble a2dp play state
 *
 * This routine notify ble a2dp play state
 *
 * @param play a2dp play or stop
 *
 * @return  N/A
 */
void bt_manager_ble_a2dp_play_notify(bool play);

/**
 * @brief notify ble hfp play state
 *
 * This routine notify ble hfp play state
 *
 * @param play hfp play or stop
 *
 * @return  N/A
 */
void bt_manager_ble_hfp_play_notify(bool play);

/**
 * @brief halt ble
 *
 * This routine disable ble adv
 *
 * @return  N/A
 */
void bt_manager_halt_ble(void);

/**
 * @brief resume ble
 *
 * This routine enable ble adv
 *
 * @return  N/A
 */
void bt_manager_resume_ble(void);

/**
 * @brief deinit btmanager ble
 *
 * This routine deinit btmanager ble
 *
 * @return  N/A
 */
void bt_manager_ble_deinit(void);

/**
 * @brief check ble is connected
 *
 * This routine check ble is connected
 *
 * @return  0: not connected, other: connected
 */
int bt_manager_ble_is_connected(void);

/**
 * @brief Ble set advertise parameter
 *
 * This routine ble set advertise parameter
 *
 * @param params Ble advertise parameter
 *
 * @return  0: set success, other: set failed
 */
int bt_manager_ble_set_advertising_params(struct ble_advertising_params_t *params);

/**
 * @brief Ble set advertise data
 *
 * This routine ble set advertise data
 *
 * ad_data sd_data format
 * refer bt core spe ADVERTISING AND SCAN RESPONSE DATA FORMAT
 * @param ad_data Advertise data buffer
 * @param ad_data_len Advertise data length
 * @param sd_data Advertise response data buffer
 * @param sd_data_len Advertise response data length
 *
 * @return  0: set success, other: set failed
 */
int bt_manager_ble_set_adv_data(uint8_t *ad_data, uint8_t ad_data_len, uint8_t *sd_data, uint8_t sd_data_len);

/**
 * @brief Ble set advertise enable/disable
 *
 * This routine ble set advertise enable/disable
 *
 * @return  0: set success, other: set failed
 */
int bt_manager_ble_set_advertise_enable(uint8_t enable);

typedef void (*btmgr_le_scan_result_cb)(uint8_t addr_type, uint8_t *addr, int8_t rssi,
										uint8_t adv_type, uint8_t *data, uint8_t data_len);

/** @brief Start (LE) scanning
 *
 *  Start LE scanning with given parameters and provide results through
 *  the specified callback.
 *
 *  @param scan_type Scan type.
 *  @param own_addr_type Own address type.
 *  @param options Scan options.
 *  @param interval_ms Scan interval in ms.
 *  @param window_ms Scan window in ms.
 *  @param cb Scan results callback function.
 *
 *  @return Zero on success or error code otherwise.
 */
int bt_manager_ble_start_scan(uint8_t scan_type, uint8_t own_addr_type, uint32_t options,
										uint16_t interval_ms, uint16_t window_ms, btmgr_le_scan_result_cb cb);

/** @brief Stop (LE) scanning.
 *
 *  Stops ongoing LE scanning.
 *
 *  @return Zero on success or error code otherwise.
 */
int bt_manager_ble_stop_scan(void);

/** @brief le add whitelist address.
 *
 *  @param addr_type Device type.
 *  @param addr Device address.
 *
 *  @return Zero on success or error code otherwise.
 */
int bt_manager_ble_add_whitelist(uint8_t addr_type, uint8_t *addr);

/** @brief le remove whitelist address.
 *
 *  @param addr_type Device type.
 *  @param addr Device address.
 *
 *  @return Zero on success or error code otherwise.
 */
int bt_manager_ble_remove_whitelist(uint8_t addr_type, uint8_t *addr);

/** @brief le clear whitelist.
 *
 *  @return Zero on success or error code otherwise.
 */
int bt_manager_ble_clear_whitelist(void);

/** @brief check le ready for send data.
 *
 *  @param conn  Connection object.
 *
 *  @return  0: no ready , 1:  ready.
 */
int bt_manager_ble_ready_send_data(bt_addr_t* addr);

/**
 * @brief get ble wake lock
 *
 * This routine get ble wake lock
 *
 * @return  0: idle can sleep; other: busy can't sleep.
 */
int bt_manager_get_ble_wake_lock(void);

/**
 * @brief Check ble switch finish
 *
 * This routine check ble switch finish
 *
 * @return  true: switch finish, false: switch not finish.
 */
bool bt_manager_ble_check_switch_finish(void);

/**
 * @} end defgroup bt_manager_ble_apis
 */
#ifdef CONFIG_BT_TRANSCEIVER
#include "tr_bt_manager_ble.h"
#endif
#endif  // __BT_MANAGER_BLE_H__
