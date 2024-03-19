/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
*/

#ifndef __TR_BT_MANAGER_H__
#define __TR_BT_MANAGER_H__
#include <stream.h>
#include "btservice_api.h"
#include "bt_manager_ble.h"
#include <msg_manager.h>
#include "bt_manager_config.h"
#include "api_bt_module.h"
#include <tr_app_switch.h>

#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
typedef struct
{
	u16_t index;
	inquiry_info_t *dev_info;
}tr_bt_manager_inquiry_info_t;
#endif

typedef enum {
	BT_AVRCP_OPERATION_ID_SKIP = 0x3C,
	BT_AVRCP_OPERATION_ID_VOLUME_UP = 0x41,
	BT_AVRCP_OPERATION_ID_VOLUME_DOWN = 0x42,
	BT_AVRCP_OPERATION_ID_MUTE = 0x43,

	BT_AVRCP_OPERATION_ID_PLAY = 0x44,
	BT_AVRCP_OPERATION_ID_STOP = 0x45,
	BT_AVRCP_OPERATION_ID_PAUSE = 0x46,
	BT_AVRCP_OPERATION_ID_REWIND = 0x48,
	BT_AVRCP_OPERATION_ID_FAST_FORWARD = 0x49,
	BT_AVRCP_OPERATION_ID_FORWARD = 0x4B,
	BT_AVRCP_OPERATION_ID_BACKWARD = 0x4C,
	
	BT_AVRCP_OPERATION_ID_REWIND_RELEASE = 0xc8,  	
	BT_AVRCP_OPERATION_ID_FAST_FORWARD_RELEASE = 0xc9,
	BT_AVRCP_OPERATION_ID_UNDEFINED = 0xFF
} tr_bt_manager_avrcp_passthrough_op_id;

enum {
    BT_A2DP_STREAM_CLOSE_EVENT = BT_TRS_EVENT,
	BT_AVRCP_PASS_THROUGH_CTRL_EVENT,
};


typedef struct
{
 	u32_t disable : 1;
 	evt_bt_state cb_state;
	evt_bt_hwready cb_hwready;
	evt_bt_avrcp cb_avrcp;
	evt_bt_absvolume_changed cb_absvolume;
    evt_bt_inquiry inquiry_cb;
    evt_bt_connect_cancel cannect_cancel_cb;
}api_bt_module_t;

typedef void (*tr_bt_notify_callback)(int event, void *param, int param_size);

struct tr_bt_audio_avrcp_status
{
    struct bt_audio_chan *chan;
	uint8_t playing;
    uint8_t volume;
    uint8_t cmd;
};

struct tr_le_audio_link_list_t{
    bt_addr_le_t addr;
    uint8_t valid : 1;
    uint8_t status : 1;
    uint8_t name[32];
};

void tr_bt_manager_change_mac_addr_and_str(u8_t *mac_addr, u8_t *mac_str);
bool tr_get_svr_status(void *addr, bt_dev_svr_info_t *dev_svr);
u8_t tr_get_connect_index_by_addr(void *addr);

void tr_bt_manager_trs_init(void);
void tr_bt_manager_trs_deinit(void);
void tr_bt_manager_connected(void *addr);
void tr_bt_manager_connect_stop(void);
void tr_bt_manager_disconnect(int index);
void tr_bt_manager_disconnected_reason(void *param);

void tr_bt_manager_set_a2dp_low_lantency(bool enable);
bool tr_bt_manager_get_a2dp_low_lantency(void);

int tr_bt_manager_a2dp_get_bit_rate(void);

int tr_bt_manager_config_connect_dev_num(void);
int tr_bt_manager_config_dev_pairlist_num(void);

int tr_bt_manager_config_get_adjust_bitpool(void);
int tr_bt_manager_config_get_default_bitpool(void);
int tr_bt_manager_config_get_low_lantency_en(void);
int tr_bt_manager_config_get_low_lantency_fix_bitpool(void);
int tr_bt_manager_config_get_low_lantency_framenum(void);
int tr_bt_manager_config_get_low_lantency_timeout_slots(void);
int tr_bt_manager_config_get_lantency_detect_en(void);
int tr_bt_manager_config_get_headset_adjust_bitpool(void);
int tr_bt_manager_config_get_multi_dev_re_align(void);
int tr_bt_manager_config_get_a2dp_48K_en(void);
int tr_bt_manager_config_get_max_pending_pkts(void);
bool tr_bt_manager_config_get_switch_en_when_connect_out(void);

void tr_bt_manager_app_notify_reg(tr_bt_notify_callback cbk);


void tr_bt_manager_avrcp_play_for_leaudio(void);
/**
 * @brief Control Bluetooth to sync vol to remote through AVRCP profile
 *
 * This routine provides to Control Bluetooth to sync vol to remote through AVRCP profile.
 *
 * @param music_vol want to sync, range by local audio volume level 0-16
 *
 * @return 0 excute successed , others failed
 */
int tr_bt_manager_avrcp_sync_vol_to_remote_localinput(u32_t music_vol);

/**
 * @brief Control Bluetooth to sync vol to remote through AVRCP profile
 *
 * This routine provides to Control Bluetooth to sync vol to remote through AVRCP profile.
 *
 * @param music_vol want to sync, range by AVRCP profile 0-127
 *
 * @return 0 excute successed , others failed
 */
int tr_bt_manager_avrcp_sync_vol_to_remote_btinput(u32_t avrcp_vol);


/**
 * @brief Control Bluetooth to get sync vol from local
 *
 * This routine provides to Control Bluetooth to get sync vol from local.
 *
 * @return sync volume
 */
int tr_bt_manager_avrcp_sync_vol_get_by_local(void);
u32_t _tr_bt_manager_avrcp_to_music_volume(u32_t avrcp_vol);
void tr_bt_manager_set_vol_sync_en(void *param);

void tr_bt_manager_inquiry_timer_init(void);
void tr_bt_manager_inquiry_timer_deinit(void);

#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
int tr_bt_manager_inquiry_ctrl_select_dev(s8_t lowest_rssi, u8_t *name, u8_t **bd, int bd_num);
#endif

int _api_bt_inquiry_and_deal_result(int number, int timeout, u32_t cod);

int tr_bt_manager_check_app_bt_relevant(void);
void tr_bt_manager_auto_move_init(void);
void tr_bt_manager_auto_move_deinit(void);
void tr_bt_manager_auto_move_keep_stop(bool need_to_disconn);
void tr_bt_manager_auto_move_restart_from_stop(void);

struct bt_mgr_dev_info *tr_bt_mgr_find_dev_info(bd_address_t *addr, u8_t *ids);

void tr_bt_manager_disconnect_all_devices(void);

void tr_bt_manager_standby_prepare(void);
void tr_bt_manager_standby_post(void);

int tr_bt_manager_startup_reconnect(void);

void tr_bt_manager_auto_2nd_device_start_stop(void);

void bt_manager_user_api_switch_audio_source_transmitter(char *app);
uint32_t bt_manager_user_api_get_status_transmitter(void);
int bt_manager_user_api_get_last_dev_disconnect_reason_transmitter(void);
void bt_manager_user_api_force_to_inquiry_transmitter(void);
void bt_manager_user_api_force_to_reconnect_pairlist_transmitter(void);
void bt_manager_user_api_force_to_delete_pairlist_transmitter(void);
int tr_bt_manager_event_notify_simple(int event_id, void *param);

bool tr_le_audio_link_set_info(void *link_dev);
void *tr_le_audio_link_info(void);
int tr_le_audio_link_restore(void);
#ifdef CONFIG_SPPBLE_DATA_CHAN
int tr_ble_cli_send_data(u16_t handle, u8_t *buf, u16_t data_len);
#endif

u8_t tr_bt_manager_le_audio_get_cis_num(void);
u8_t tr_bt_manager_le_audio_get_con_num(void);

tr_btmgr_base_config_t * tr_bt_manager_get_base_config(void);

void bt_ui_monitor_init(void);
void bt_ui_monitor_submit_work(int timeout);
#endif
