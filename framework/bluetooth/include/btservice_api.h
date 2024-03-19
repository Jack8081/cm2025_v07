/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bt service interface
 */

#ifndef _BTSERVICE_API_H_
#define _BTSERVICE_API_H_
#include <stream.h>
#include <btservice_base.h>
#include <btservice_tws_api.h>
#include <acts_bluetooth/sdp.h>
#include <acts_bluetooth/audio/common.h>

/**
 * @defgroup bt_service_apis Bt Service APIs
 * @ingroup bluetooth_system_apis
 * @{
 */

enum {
	BTSRV_TWS_MATCH_MODE_NAME = 0,  /* Tws pair match by name */
	BTSRV_TWS_MATCH_MODE_ID   = 1,  /* Tws pair match by device id */
};

enum {
	CONTROLER_ROLE_MASTER,
	CONTROLER_ROLE_SLAVE,
};

/** btsrv auto conn mode */
enum btsrv_auto_conn_mode_e {
	/** btsrv auto connet device alternate */
	BTSRV_AUTOCONN_ALTERNATE,
	/** btsrv auto connect by device order */
	BTSRV_AUTOCONN_ORDER,
};

enum btsrv_disconnect_mode_e {
	/**  Disconnect all device */
	BTSRV_DISCONNECT_ALL_MODE,
	/** Just disconnect all phone */
	BTSRV_DISCONNECT_PHONE_MODE,
	/** Just disconnect tws */
	BTSRV_DISCONNECT_TWS_MODE,
};

enum btsrv_device_type_e {
	/** all device */
	BTSRV_DEVICE_ALL,
	/** Just phone device*/
	BTSRV_DEVICE_PHONE,
	/** Just tws device*/
	BTSRV_DEVICE_TWS,
};

enum btsrv_stop_auto_reconnect_mode_e {
	BTSRV_STOP_AUTO_RECONNECT_NONE    = 0x00,
	/** Just phone device*/
	BTSRV_STOP_AUTO_RECONNECT_PHONE   = 0x01,
	/** Just tws device*/
	BTSRV_STOP_AUTO_RECONNECT_TWS     = 0x02,
	/** all device */
	BTSRV_STOP_AUTO_RECONNECT_ALL     = 0x03,
	/** Just for addr device*/
	BTSRV_STOP_AUTO_RECONNECT_BY_ADDR = 0x04,

	BTSRV_SYNC_AUTO_RECONNECT_STATUS  = 0x08,

};

enum btsrv_scan_mode_e
{
    BTSRV_SCAN_MODE_DEFAULT_INQUIRY_PAGE,
    BTSRV_SCAN_MODE_FAST_PAGE,
    BTSRV_SCAN_MODE_FAST_PAGE_EX,
    BTSRV_SCAN_MODE_NORMAL_PAGE,
    BTSRV_SCAN_MODE_NORMAL_PAGE_S3,
    BTSRV_SCAN_MODE_NORMAL_PAGE_EX,
    BTSRV_SCAN_MODE_FAST_INQUIRY_PAGE,
    BTSRV_SCAN_MODE_MAX,
};

enum btsrv_visual_mode_e
{
    BTSRV_VISUAL_MODE_NODISC_NOCON,
    BTSRV_VISUAL_MODE_DISC_NOCON,
    BTSRV_VISUAL_MODE_NODISC_CON,
    BTSRV_VISUAL_MODE_DISC_CON,
    BTSRV_VISUAL_MODE_LIMIT_DISC_CON,
};

enum btsrv_pair_status_e {
    BT_PAIR_STATUS_NONE             = 0x0000,
    BT_PAIR_STATUS_WAIT_CONNECT     = 0x0001,
	BT_PAIR_STATUS_PAIR_MODE        = 0x0002,
	BT_PAIR_STATUS_TWS_SEARCH       = 0x0004,
    BT_PAIR_STATUS_RECONNECT        = 0x0008,
    BT_PAIR_STATUS_NODISC_NOCON     = 0x0010,
};

enum {
	BTSRV_PAIR_KEY_MODE_NONE        = 0,
    BTSRV_PAIR_KEY_MODE_ONE         = 1,
    BTSRV_PAIR_KEY_MODE_TWO         = 2,
};

enum btsrv_tws_reconnect_mode_e {
    BTSRV_TWS_RECONNECT_NONE          = 0x0000,
    BTSRV_TWS_RECONNECT_AUTO_PAIR     = 0x0001,
	BTSRV_TWS_RECONNECT_FAST_PAIR     = 0x0002,
};

struct btsrv_config_stack_param {
	uint8_t sniff_enable:1;
	uint8_t linkkey_miss_reject:1;
	uint8_t max_pair_list_num:4;
	uint8_t le_adv_random_addr:1;
	uint8_t avrcp_vol_sync:1;		/* avrcp support vol sync  */	
	uint8_t hfp_sp_codec_neg:1;
	uint8_t hfp_sp_voice_recg:1;	/* Hfp support voice recognition  */
	uint8_t hfp_sp_3way_call:1;		/* Hfp support Three-way calling  */
	uint8_t hfp_sp_volume:1;		/* Hfp support Remote volume control  */
	uint8_t hfp_sp_ecnr:1;		/* Hfp support EC and/or NR  */
	uint8_t bt_version;
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
    uint8_t bt_br_tr_enable:1;
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_BQB
    uint8_t leaudio_bqb_enable:1;
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
    uint8_t leaudio_bqb_sr_enable:1;
#endif
	uint16_t bt_sub_version;
	uint32_t classOfDevice;
	uint16_t bt_device_id[4];
};

/** BR inquiry/page scan parameter */
struct bt_scan_parameter{
	unsigned char   scan_mode;
	unsigned short	inquiry_scan_window;
	unsigned short	inquiry_scan_interval;
	unsigned char   inquiry_scan_type;
	unsigned short	page_scan_window;
	unsigned short	page_scan_interval;
	unsigned char	page_scan_type;
};

/** device auto connect info */
struct autoconn_info {
	/** bluetooth device address */
	bd_address_t addr;
	/** bluetooth device address valid flag*/
	uint8_t addr_valid:1;
	/** bluetooth device tws role */
	uint8_t tws_role:3;
	/** bluetooth device need connect a2dp */
	uint8_t a2dp:1;
	/** bluetooth device need connect avrcp */
	uint8_t avrcp:1;
	/** bluetooth device need connect hfp */
	uint8_t hfp:1;
	/** bluetooth connect hfp first */
	uint8_t hfp_first:1;
	/** bluetooth device need connect hid */
	uint8_t hid:1;
};

struct btsrv_config_info {
	/** Max br connect device number */
	uint8_t max_conn_num:4;
	/** Max br phone device number */
	uint8_t max_phone_num:4;
    uint8_t cfg_device_num:2;
	/** Set pts test mode */
	uint8_t pts_test_mode:1;
	uint8_t double_preemption_mode:1;
	uint8_t pair_key_mode:2;
    uint8_t power_on_auto_pair_search:1;
	uint8_t default_state_discoverable:1;
    uint8_t bt_not_discoverable_when_connected:1;
	uint8_t use_search_mode_when_tws_reconnect:1;
	uint8_t device_l_r_match:1;
	uint8_t sniff_enable:1;
	uint8_t stop_another_when_one_playing:1;
	uint8_t resume_another_when_one_stopped:1;
	uint8_t clear_link_key_when_clear_paired_list:1;
	uint8_t cfg_support_tws:1;
    uint8_t default_state_wait_connect_sec;
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
    uint8_t br_tr_mode:1;
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_BQB
    uint8_t leaudio_bqb_mode:1;
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
        uint8_t leaudio_bqb_sr_mode:1;
#endif
	uint16_t a2dp_status_stopped_delay_ms;
	uint16_t idle_enter_sniff_time;
	uint16_t sniff_interval;
	/** delay time for volume sync */
	uint16_t volume_sync_delay_ms;
	/** Tws version */
	uint16_t tws_version;
	/** Tws feature */
	uint32_t tws_feature;
};

#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
typedef struct {
    u8_t a2dp:1;
    u8_t avrcp:1;
    u8_t hfp:1;
}tr_bt_set_autoconn_t;
#endif

/** auto connect info */
struct bt_set_autoconn {
	bd_address_t addr;
	uint8_t strategy;
	uint8_t base_try;
	uint8_t profile_try;
    uint8_t reconnect_mode;
	uint8_t base_interval;
	uint8_t profile_interval;
    uint8_t reconnect_tws_timeout;
    uint8_t reconnect_phone_timeout;
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
    tr_bt_set_autoconn_t trs;
#endif
};

struct bt_set_user_visual {
	uint8_t enable;
    uint8_t discoverable;
	uint8_t connectable;
    uint8_t scan_mode;
};

struct bt_linkkey_info {
	bd_address_t addr;
	uint8_t linkkey[16];
};

enum {
	BT_LINK_EV_ACL_CONNECT_REQ,
	BT_LINK_EV_ACL_CONNECTED,
	BT_LINK_EV_ACL_DISCONNECTED,
	BT_LINK_EV_GET_NAME,
	BT_LINK_EV_HF_CONNECTED,
	BT_LINK_EV_HF_DISCONNECTED,
	BT_LINK_EV_A2DP_CONNECTED,
	BT_LINK_EV_A2DP_DISCONNECTED,
	BT_LINK_EV_AVRCP_CONNECTED,
	BT_LINK_EV_AVRCP_DISCONNECTED,
	BT_LINK_EV_SPP_CONNECTED,
	BT_LINK_EV_SPP_DISCONNECTED,
	BT_LINK_EV_HID_CONNECTED,
	BT_LINK_EV_HID_DISCONNECTED,
	BT_LINK_EV_SNOOP_ROLE,
};

struct bt_link_cb_param {
	uint8_t link_event;
	uint8_t is_tws:1;
	uint8_t new_dev:1;
	uint8_t param;
	uint16_t hdl;
	bd_address_t *addr;
	uint8_t *name;
};

struct bt_connected_cb_param {
	uint8_t acl_connected:1;	/* Just BR acl connected */
	bd_address_t *addr;
	uint8_t *name;
};

struct bt_disconnected_cb_param {
	uint8_t acl_disconnected:1;	/* BR acl disconnected */
	bd_address_t addr;
	uint8_t reason;
};

/** bt device disconnect info */
struct bt_disconnect_reason {
	/** bt device addr */
	bd_address_t addr;
	/** bt device disconnected reason */
	uint8_t reason;
	/** bt device tws role */
	uint8_t tws_role;
    uint8_t conn_type;
};

/* Bt device rdm state  */
struct bt_dev_rdm_state {
	bd_address_t addr;
	uint8_t acl_connected:1;
	uint8_t hfp_connected:1;
	uint8_t a2dp_connected:1;
	uint8_t avrcp_connected:1;
	uint8_t hid_connected:1;
	uint8_t sco_connected:1;
	uint8_t a2dp_stream_started:1;
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
	u8_t peer_connected_in:1;
#endif
};

struct btsrv_discover_result {
	uint8_t discover_finish:1;
	bd_address_t addr;
	int8_t rssi;
	uint8_t len;
	uint8_t *name;
	uint16_t device_id[4];
};

typedef void (*btsrv_discover_result_cb)(void *result);

struct btsrv_discover_param {
	/** Maximum length of the discovery in units of 1.28 seconds. Valid range is 0x01 - 0x30. */
	uint8_t length;

	/** 0: Unlimited number of responses, Maximum number of responses. */
	uint8_t num_responses;

	/** Result callback function */
	btsrv_discover_result_cb cb;
};

struct btsrv_check_device_role_s {
	bd_address_t addr;
	uint8_t len;
	uint8_t *name;
};

/*=============================================================================
 *				global api
 *===========================================================================*/
/** Callbacks to report Bluetooth service's events*/
typedef enum {
	/** notifying that Bluetooth service has been started err, zero on success or (negative) error code otherwise*/
	BTSRV_READY,
	/** notifying link event */
	BTSRV_LINK_EVENT,
	/** notifying remote device disconnected mac with reason */
	BTSRV_DISCONNECTED_REASON,
	/** Request high cpu performace */
	BTSRV_REQ_HIGH_PERFORMANCE,
	/** Release high cpu performace */
	BTSRV_RELEASE_HIGH_PERFORMANCE,
	/** Request flush property to nvram */
	BTSRV_REQ_FLUSH_PROPERTY,
	/** CHECK device is phone or tws */
	BTSRV_CHECK_NEW_DEVICE_ROLE,
	/** notifying that Auto Reconnect complete */
	BTSRV_AUTO_RECONNECT_COMPLETE,

    BTSRV_SYNC_AOTO_RECONNECT_STATUS,
} btsrv_event_e;

/**
 * @brief btsrv callback
 *
 * This routine is btsrv callback, this callback context is in bt service
 *
 * @param event btsrv event
 * @param param param of btsrv event
 *
 * @return N/A
 */
typedef int (*btsrv_callback)(btsrv_event_e event, void *param);

/**
 * @brief Rgister base processer
 *
 * @return 0 excute successed , others failed
 */
int btif_base_register_processer(void);

/**
 * @brief start bt service
 *
 * This routine start bt service.
 *
 * @param param stack init parameter
 *
 * @return 0 excute successed , others failed
 */
int btif_start(btsrv_callback cb, struct btsrv_config_stack_param *param);

/**
 * @brief stop bt service
 *
 * This routine stop bt service.
 *
 * @return 0 excute successed , others failed
 */
int btif_stop(void);

/**
 * @brief set base config info to bt service
 *
 * This routine set base config info to bt service.
 *
 * @return 0 excute successed , others failed
 */
int btif_base_set_config_info(void *param);

/**
 * @brief Get bluetooth wake lock
 *
 * This routine get bluetooth wake lock.
 *
 * @return 0: idle can sleep,  other: busy can't sleep.
 */
int btif_base_get_wake_lock(void);

int btif_base_get_link_idle(void);


/**
 * @brief set br scan parameter
 *
 * This routine set br scan parameter.
 *
 * @param param parameter
 * @enhance_param: true enhance param, false: default param
 *
 * @return 0 excute successed , others failed
 */
int btif_base_set_power_off(void);

int btif_br_update_pair_status(uint8_t state);

int btif_br_get_pair_status(uint8_t *pair_status);

int btif_br_set_scan_param(struct bt_scan_parameter *param);

int btif_br_get_scan_visual(uint8_t *visual_mode);

int btif_br_get_scan_mode(uint8_t *scan_mode);

int btif_br_update_scan_mode(void);

int btif_br_set_pair_mode_status(uint8_t enable, uint32_t duration_ms);

int btif_br_set_wait_connect_status(uint8_t enable, uint32_t duration_ms);

int btif_br_set_tws_search_status(uint8_t enable);

/**
 * @brief set br visual
 *
 * This routine set br visual.
 *
 * @param user_visual
 *
 * @return 0 excute successed , others failed
 */

int btif_br_set_user_visual(struct bt_set_user_visual *user_visual);

/**
 * @brief allow br sco connnectable
 *
 * This routine disable br sco connect or not.
 *
 * @param enable true enable, false disable 
 *
 * @return 0 excute successed , others failed
 */

int btif_br_allow_sco_connect(bool allowed);

/**
 * @brief Start br disover
 *
 * This routine Start br disover.
 *
 * @param disover parameter
 *
 * @return 0 excute successed , others failed
 */
int btif_br_start_discover(struct btsrv_discover_param *param);

/**
 * @brief Stop br disover
 *
 * This routine Stop br disover.
 *
 * @return 0 excute successed , others failed
 */
int btif_br_stop_discover(void);

/**
 * @brief connect to target bluetooth device
 *
 * This routine connect to target bluetooth device with bluetooth mac addr.
 *
 * @param bd bluetooth addr of target bluetooth device
 *                 Mac low address store in low memory address
 *
 * @return 0 excute successed , others failed
 */
int btif_br_connect(bd_address_t *bd);

/**
 * @brief disconnect to target bluetooth device
 *
 * This routine disconnect to target bluetooth device with bluetooth mac addr.
 *
 * @param bd bluetooth addr of target bluetooth device
 *                 Mac low address store in low memory address
 *
 * @return 0 excute successed , others failed
 */
int btif_br_disconnect(bd_address_t *bd);

/**
 * @brief get auto reconnect info from bt service
 *
 * This routine get auto reconnect info from bt service.
 *
 * @param info pointer store the audo reconnect info
 * @param max_cnt max number of bluetooth device
 *
 * @return 0 excute successed , others failed
 */
int btif_br_get_auto_reconnect_info(struct autoconn_info *info, uint8_t max_cnt);

/**
 * @brief set auto reconnect info from bt service
 *
 * This routine set auto reconnect info from bt service.
 *
 * @param info pointer store the audo reconnect info
 * @param max_cnt max number of bluetooth device
 */
void btif_br_set_auto_reconnect_info(struct autoconn_info *info, uint8_t max_cnt);


/**
 * @brief check tws paired info from bt service
 *
 * This routine check tws paired info from bt service.
 *
 * @return true ,others null
 */
bool btif_br_check_tws_paired_info(void);

/**
 * @brief check phone paired info from bt service
 *
 * This routine check phone paired info from bt service.
 *
 * @return true ,others null
 */

bool btif_br_check_phone_paired_info(void);

/**
 * @brief auto reconnect to all need reconnect device
 *
 * This routine auto reconnect to all need reconnect device
 *
 * @param param auto reconnect info
 *
 * @return 0 excute successed , others failed
 */
int btif_br_auto_reconnect(struct bt_set_autoconn *param);

/**
 * @brief stop auto reconnect
 *
 * This routine stop auto reconnect
 *
 * @return 0 excute successed , others failed
 */

int btif_br_auto_reconnect_stop(int mode);

/**
 * @brief stop auto reconnect device
 *
 * This routine stop auto reconnect device
 *
 * @return 0 excute successed , others failed
 */
int btif_br_auto_reconnect_stop_device(bd_address_t *bd);

/**
 * @brief check tws reconnect is first time
 *
 * This routine check tws auto reconnect is first time
 *
 * @return ture: first; false not;
 */
bool btif_br_is_auto_reconnect_tws_first(uint8_t tws_role);

/**
 * @brief check auto reconnect is runing
 *
 * This routine check auto reconnect is runing
 *
 * @return ture: runing; false stop;
 */
bool btif_br_is_auto_reconnect_runing(void);

/**
 * @brief clear br connected info list
 *
 * This routine clear br connected info list, will disconnected all connnected device first.
 *
 * @return 0 excute successed , others failed
 */
int btif_br_clear_list(int mode);

/**
 * @brief disconnect all connected device
 *
 * This routine disconnect all connected device.
 * @param:  disconnect_mode
 *
 * @return 0 excute successed , others failed
 */
int btif_br_disconnect_device(int disconnect_mode);

/**
 * @brief get br connected device number
 *
 * This routine get connected device number.
 *
 * @return connected device number(include phone and tws device)
 */
int btif_br_get_connected_device_num(void);

/**
 * @brief get br device state from rdm
 *
 * This routine get device state from rdm.
 *
 * @return 0 successed, others failed.
 */
int btif_br_get_dev_rdm_state(struct bt_dev_rdm_state *state);

/**
 * @brief set phone controler role
 *
 * This routine get device state from rdm.
 */
void btif_br_set_phone_controler_role(bd_address_t *bd, uint8_t role);

/**
 * @brief Get linkkey information
 *
 * This routine Get linkkey information.
 * @param info for store get linkkey information
 * @param cnt want to get linkkey number
 *
 * return int  number of linkkey getted
 */

#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER

/**
 * @brief set br discoverable
 *
 * This routine set br discoverable.
 *
 * @param enable true enable discoverable, false disable discoverable
 *
 * @return 0 excute successed , others failed
 */
int btif_br_set_discoverable(bool enable);

/**
 * @brief set br connnectable
 *
 * This routine set br connnectable.
 *
 * @param enable true enable connnectable, false disable connnectable
 *
 * @return 0 excute successed , others failed
 */
int btif_br_set_connnectable(bool enable);
#endif

int btif_br_get_linkkey(struct bt_linkkey_info *info, uint8_t cnt);

/**
 * @brief Update linkkey information
 *
 * This routine Update linkkey information.
 * @param info for update linkkey information
 * @param cnt update linkkey number
 *
 * return int 0: update successful, other update failed.
 */
int btif_br_update_linkkey(struct bt_linkkey_info *info, uint8_t cnt);

/**
 * @brief Writ original linkkey information
 *
 * This routine Writ original linkkey information.
 * @param addr update device mac address
 * @param link_key original linkkey
 *
 * return 0: successfull, other: failed.
 */
int btif_br_write_ori_linkkey(bd_address_t *addr, uint8_t *link_key);

/**
 * @brief clean bt service linkkey from nvram
 *
 * This routine clean bt service linkkey from nvram.
 */
void btif_br_clean_linkkey(void);

/**
 * @brief pts test send hfp command
 *
 * This routine pts test send hfp command
 *
 * @return 0 successed, others failed.
 */
 
int btif_br_clear_share_tws(void);
bool btif_br_check_share_tws(void);
void btif_br_get_local_mac(bd_address_t *addr);
uint16_t btif_br_get_active_phone_hdl(void);
int btif_br_disconnect_noactive_device(void);
int btif_bt_set_apll_temp_comp(uint8_t enable);
int btif_bt_do_apll_temp_comp(void);

int btif_pts_send_hfp_cmd(char *cmd);

/**
 * @brief pts test hfp active connect sco
 *
 * This routine pts test hfp active connect sco
 *
 * @return 0 successed, others failed.
 */
int btif_pts_hfp_active_connect_sco(void);

/**
 * @brief get connection type
 *
 * This routine return connection type
 *
 * @param handle connection acl handle
 * @return connection type.
 */
int btif_get_type_by_handle(uint16_t handle);
int btif_get_conn_role(uint16_t handle);
int btif_get_connecting_dev(void);
bd_address_t *btif_get_addr_by_handle(uint16_t handle);
bt_addr_le_t *btif_get_le_addr_by_handle(uint16_t handle);
int btif_set_leaudio_connected(int connected);
int btif_set_leaudio_foreground_dev(bool enable);

int btif_set_mailbox_ctx_to_controler(int *param, int array_size);


int btif_pts_avrcp_get_play_status(void);
int btif_pts_avrcp_pass_through_cmd(uint8_t opid);
int btif_pts_avrcp_notify_volume_change(uint8_t volume);
int btif_pts_avrcp_reg_notify_volume_change(void);
int btif_pts_register_auth_cb(bool reg_auth);
int btif_pts_a2dp_set_err_code(uint8_t err_code);

/**
 * @brief dump bt srv info
 *
 * This routine dump bt srv info.
 *
 * @return 0 excute successed , others failed
 */
int btif_dump_brsrv_info(void);

/*=============================================================================
 *				a2dp service api
 *===========================================================================*/


/** @brief Codec ID */
enum btsrv_a2dp_codec_id {
	/** Codec SBC */
	BTSRV_A2DP_SBC = 0x00,
	/** Codec MPEG-1 */
	BTSRV_A2DP_MPEG1 = 0x01,
	/** Codec MPEG-2 */
	BTSRV_A2DP_MPEG2 = 0x02,
	/** Codec ATRAC */
	BTSRV_A2DP_ATRAC = 0x04,
	/** Codec Non-A2DP */
	BTSRV_A2DP_VENDOR = 0xff
};


/** Callbacks to report a2dp profile events*/
typedef enum {
	/** Nothing for notify */
	BTSRV_A2DP_NONE_EV,
	/** notifying that a2dp connected*/
	BTSRV_A2DP_CONNECTED,
	/** notifying that a2dp disconnected */
	BTSRV_A2DP_DISCONNECTED,
	/** notifying that a2dp stream started */
	BTSRV_A2DP_STREAM_STARTED,
	/** notifying that a2dp stream check started */
	BTSRV_A2DP_STREAM_CHECK_STARTED,	
	/** notifying that a2dp stream suspend */
	BTSRV_A2DP_STREAM_SUSPEND,
	/** notifying that a2dp stream data indicated */
	BTSRV_A2DP_DATA_INDICATED,
	/** notifying that a2dp codec info */
	BTSRV_A2DP_CODEC_INFO,
	/** Get initialize delay report  */
	BTSRV_A2DP_GET_INIT_DELAY_REPORT,

	/** Not callback event, just for snoop sync info.  */
	BTSRV_A2DP_JUST_SNOOP_SYNC_INFO,
	/** Notifying device need delay check start play.  */
	BTSRV_A2DP_REQ_DELAY_CHECK_START,
	/** notifying that a2dp stream suspend because a2dp disconnected */
	BTSRV_A2DP_DISCONNECTED_STREAM_SUSPEND,	
	/** notifying that BR a2dp stream started but avrcp paused */
	BTSRV_A2DP_STREAM_STARTED_AVRCP_PAUSED,		
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
	/** notifying that a2dp codec started*/
	TR_BTSRV_A2DP_CODEC_START,
	/** notifying that a2dp codec started*/
	TR_BTSRV_A2DP_CODEC_STOP,
	/** notifying that a2dp actived device change*/
	TR_BTSRV_A2DP_CODEC_INFO_CHANGED,
	/** notifying that a2dp bit rate */
	TR_BTSRV_A2DP_BIT_RATE,
#endif
} btsrv_a2dp_event_e;

/**
 * @brief callback of bt service a2dp profile
 *
 * This routine callback of bt service a2dp profile.
 *
 * @param event id of bt service a2dp event
 * @param packet param of btservice a2dp event
 * @param size length of param
 *
 * @return N/A
 */
typedef void (*btsrv_a2dp_callback)(uint16_t hdl, btsrv_a2dp_event_e event, void *packet, int size);

struct btsrv_a2dp_start_param {
	btsrv_a2dp_callback cb;
	uint8_t *sbc_codec;
	uint8_t *aac_codec;
	uint8_t sbc_endpoint_num;
	uint8_t aac_endpoint_num;
	uint8_t a2dp_cp_scms_t:1;		/* Support content protection SCMS-T */
	uint8_t a2dp_delay_report:1;	/* Support delay report */
};

/**
 * @brief Rgister a2dp processer
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_register_processer(void);

/**
 * @brief start bt service a2dp profile
 *
 * This routine start bt service a2dp profile.
 *
 * @param cbs handle of btsrv event callback
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_start(struct btsrv_a2dp_start_param *param);

/**
 * @brief stop bt service a2dp profile
 *
 * This routine stop bt service a2dp profile.
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_stop(void);

/**
 * @brief connect target device a2dp profile
 *
 * This routine connect target device a2dp profile.
 *
 * @param is_src mark as a2dp source or a2dp sink
 * @param bd bt address of target device
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_connect(bool is_src, bd_address_t *bd);

/**
 * @brief disconnect target device a2dp profile
 *
 * This routine disconnect target device a2dp profile.
 *
 * @param bd bt address of target device
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_disconnect(bd_address_t *bd);

/**
 * @brief trigger start bt service a2dp profile
 *
 * This routine use by app to check a2dp playback
 * state,if state is playback ,trigger start event
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_check_state(void);

/**
 * @brief User set pause(for tirgger switch to second phone)
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_user_pause(uint16_t hdl);

/**
 * @brief update avrcp state for a2dp
 *
 * @return 0 excute successed , others failed
 */

int btif_a2dp_update_avrcp_state(struct bt_conn * conn, uint8_t avrcp_state);


/**
 * @brief Send delay report to a2dp active phone
 *
 * This routine Send delay report to a2dp active phone
 *
 * @param delay_time delay time (unit: 1/10 milliseconds)
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_send_delay_report(uint16_t delay_time);

/**
 * @brief Get a2dp active device handl
 *
 * @return Active device handl.
 */
uint16_t btif_a2dp_get_active_hdl(void);


/**
 * @brief check a2dp active device stream open ?
 *
 * @return 1:open, others: close.
 */
int btif_a2dp_stream_is_open(uint16_t hdl);
/**
 * @brief check a2dp active device delay stoping ?
 *
 * @return 1:delay stoping, others: close.
 */
int btif_a2dp_stream_is_delay_stop(uint16_t hdl);

int btif_a2dp_stream_start(uint16_t hdl);

int btif_a2dp_stream_suspend(uint16_t hdl);




/*=============================================================================
 *				avrcp service api
 *===========================================================================*/

struct bt_attribute_info{
	uint32_t id;
	uint16_t character_id;
	uint16_t len;
	uint8_t* data;
}__packed;

#define BT_TOTAL_ATTRIBUTE_ITEM_NUM   5

struct bt_id3_info{
	struct bt_attribute_info item[BT_TOTAL_ATTRIBUTE_ITEM_NUM];
}__packed;

/** avrcp controller cmd*/
typedef enum {
	BTSRV_AVRCP_CMD_PLAY,
	BTSRV_AVRCP_CMD_PAUSE,
	BTSRV_AVRCP_CMD_STOP,
	BTSRV_AVRCP_CMD_FORWARD,
	BTSRV_AVRCP_CMD_BACKWARD,
	BTSRV_AVRCP_CMD_VOLUMEUP,
	BTSRV_AVRCP_CMD_VOLUMEDOWN,
	BTSRV_AVRCP_CMD_MUTE,
	BTSRV_AVRCP_CMD_FAST_FORWARD_START,
	BTSRV_AVRCP_CMD_FAST_FORWARD_STOP,
	BTSRV_AVRCP_CMD_FAST_BACKWARD_START,
	BTSRV_AVRCP_CMD_FAST_BACKWARD_STOP,
	BTSRV_AVRCP_CMD_REPEAT_SINGLE,
	BTSRV_AVRCP_CMD_REPEAT_ALL_TRACK,
	BTSRV_AVRCP_CMD_REPEAT_OFF,
	BTSRV_AVRCP_CMD_SHUFFLE_ON,
	BTSRV_AVRCP_CMD_SHUFFLE_OFF
} btsrv_avrcp_cmd_e;

/** Callbacks to report avrcp profile events*/
typedef enum {
	/** Nothing for notify */
	BTSRV_AVRCP_NONE_EV,
	/** notifying that avrcp connected */
	BTSRV_AVRCP_CONNECTED,
	/** notifying that avrcp disconnected*/
	BTSRV_AVRCP_DISCONNECTED,
	/** notifying that avrcp stoped */
	BTSRV_AVRCP_STOPED,
	/** notifying that avrcp paused */
	BTSRV_AVRCP_PAUSED,
	/** notifying that avrcp playing */
	BTSRV_AVRCP_PLAYING,
	/** notifying cur id3 info */
	BTSRV_AVRCP_UPDATE_ID3_INFO,
	/** notifying cur track change */
	BTSRV_AVRCP_TRACK_CHANGE,
} btsrv_avrcp_event_e;

typedef void (*btsrv_avrcp_callback)(uint16_t hdl, btsrv_avrcp_event_e event, void *param);
typedef void (*btsrv_avrcp_get_volume_callback)(uint16_t hdl, uint8_t *volume, bool remote_reg);
typedef void (*btsrv_avrcp_set_volume_callback)(uint16_t hdl, uint8_t volume, bool sync_to_remote_pending);
typedef void (*btsrv_avrcp_pass_ctrl_callback)(uint16_t hdl, uint8_t cmd, uint8_t state);

typedef struct {
	btsrv_avrcp_callback              event_cb;
	btsrv_avrcp_get_volume_callback   get_volume_cb;
	btsrv_avrcp_set_volume_callback   set_volume_cb;
	btsrv_avrcp_pass_ctrl_callback    pass_ctrl_cb;
} btsrv_avrcp_callback_t;

/**
 * @brief Rgister avrcp processer
 *
 * @return 0 excute successed , others failed
 */
int btif_avrcp_register_processer(void);
int btif_avrcp_start(btsrv_avrcp_callback_t *cbs);
int btif_avrcp_stop(void);
int btif_avrcp_connect(bd_address_t *bd);
int btif_avrcp_disconnect(bd_address_t *bd);
int btif_avrcp_send_command(int command);
int btif_avrcp_send_command_by_hdl(uint16_t hdl, int command);

int btif_avrcp_sync_vol(uint16_t hdl, uint16_t delay_ms);		/* Hdl as parameter, will use get_volume_cb get volume to sync */
int btif_avrcp_get_id3_info();
int btif_avrcp_set_absolute_volume(uint8_t *data, uint8_t len);
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
int btif_avrcp_sync_track(u32_t track);
#endif
/*return 0:pause, 1:play*/
int btif_avrcp_get_avrcp_status(uint16_t hdl);


/*=============================================================================
 *				hfp hf service api
 *===========================================================================*/

enum btsrv_sco_codec_id {
	/** Codec CVSD */
	BTSRV_SCO_CVSD = 0x01,
	/** Codec MSBC */
	BTSRV_SCO_MSBC = 0x02,
};


/** Callbacks to report avrcp profile events*/
typedef enum {
	/** Nothing for notify */
	BTSRV_HFP_NONE_EV            = 0,
	/** notifying that hfp connected */
	BTSRV_HFP_CONNECTED          = 1,
	/** notifying that hfp disconnected*/
	BTSRV_HFP_DISCONNECTED       = 2,
	/** notifying that hfp codec info*/
	BTSRV_HFP_CODEC_INFO         = 3,
	/** notifying that hfp phone number*/
	BTSRV_HFP_PHONE_NUM          = 4,
	/** notifying that hfp phone number stop report*/
	BTSRV_HFP_PHONE_NUM_STOP     = 5,
	/** notifying that hfp in coming call*/
	BTSRV_HFP_CALL_INCOMING      = 6,
	/** notifying that hfp call out going*/
	BTSRV_HFP_CALL_OUTGOING      = 7,
	BTSRV_HFP_CALL_3WAYIN        = 8,
	/** notifying that hfp call ongoing*/
	BTSRV_HFP_CALL_ONGOING       = 9,
	BTSRV_HFP_CALL_MULTIPARTY    = 10,
	BTSRV_HFP_CALL_ALERTED       = 11,
	BTSRV_HFP_CALL_EXIT          = 12,
	BTSRV_HFP_VOLUME_CHANGE      = 13,
	BTSRV_HFP_ACTIVE_DEV_CHANGE  = 14,
	BTSRV_HFP_SIRI_STATE_CHANGE  = 15,
	BTSRV_HFP_SCO                = 16,
	/** notifying that sco connected*/
	BTSRV_SCO_CONNECTED          = 17,
	/** notifying that sco disconnected*/
	BTSRV_SCO_DISCONNECTED       = 18,
	/** notifying that data indicatied*/
	BTSRV_SCO_DATA_INDICATED     = 19,
	/** update time from phone*/
	BTSRV_HFP_TIME_UPDATE        = 20,
	/** at cmd from hf*/
	BTSRV_HFP_AT_CMD             = 21,
	/** Get bt manger hfp state, for snoop sync used */
	BTSRV_HFP_GET_BTMGR_STATE    = 22,
	/** Get bt manger hfp state, for snoop sync used */
	BTSRV_HFP_SET_BTMGR_STATE    = 23,
	/** Not callback event, just for snoop sync info.  */
	BTSRV_HFP_JUST_SNOOP_SYNC_INFO = 24,

	/** notifying that hfp call 3way out going*/
	BTSRV_HFP_CALL_3WAYOUT        = 25,
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
    BTSRV_HFP_CALL_ATA,
    BTSRV_HFP_CALL_CHUP,
    BTSRV_HFP_CALL_ATD,
    BTSRV_HFP_CALL_BLDN,
    BTSRV_HFP_CALL_CLCC,
    BTSRV_HFP_CALL_CHLD,
    BTSRV_HFP_CALL_BIA,
#endif
} btsrv_hfp_event_e;

enum {
	HFP_SIRI_STATE_DEACTIVE     = 0,
	HFP_SIRI_STATE_ACTIVE       = 1,
	HFP_SIRI_STATE_SEND_ACTIVE  = 2,	/* Send SIRI start command */
};

enum btsrv_hfp_stack_event {
	/* CIEV call_setup */
	BTSRV_HFP_EVENT_CALL_SETUP_EXITED      = 0,
	BTSRV_HFP_EVENT_CALL_INCOMING          = 1,
	BTSRV_HFP_EVENT_CALL_OUTCOMING         = 2,
	BTSRV_HFP_EVENT_CALL_ALERTED           = 3,
	/* CIEV call */
	BTSRV_HFP_EVENT_CALL_EXITED            = 10,
	BTSRV_HFP_EVENT_CALL_ONGOING           = 11,
	/* CIEV call_held */
	BTSRV_HFP_EVENT_CALL_HELD_EXITED       = 20,
	BTSRV_HFP_EVENT_CALL_MULTIPARTY_HELD   = 21,
	BTSRV_HFP_EVENT_CALL_HELD              = 22,
	/* Sco event */
	BTSRV_HFP_EVENT_SCO_ESTABLISHED        = 30,
	BTSRV_HFP_EVENT_SCO_RELEASED           = 31,
};

enum btsrv_hfp_state {
	BTSRV_HFP_STATE_INIT                  = 0,
	BTSRV_HFP_STATE_LINKED                = 1,
	BTSRV_HFP_STATE_CALL_INCOMING         = 2,
	BTSRV_HFP_STATE_CALL_OUTCOMING        = 3,
	BTSRV_HFP_STATE_CALL_ALERTED          = 4,
	BTSRV_HFP_STATE_CALL_ONGOING          = 5,
	BTSRV_HFP_STATE_CALL_3WAYIN           = 6,
	BTSRV_HFP_STATE_CALL_MULTIPARTY       = 7,
	BTSRV_HFP_STATE_SCO_ESTABLISHED       = 8,
	BTSRV_HFP_STATE_SCO_RELEASED          = 9,
	BTSRV_HFP_STATE_CALL_EXITED           = 10,
	BTSRV_HFP_STATE_CALL_SETUP_EXITED     = 11,
	BTSRV_HFP_STATE_CALL_HELD             = 12,
	BTSRV_HFP_STATE_CALL_MULTIPARTY_HELD  = 13,
	BTSRV_HFP_STATE_CALL_HELD_EXITED      = 14,
};

typedef int (*btsrv_hfp_callback)(uint16_t, btsrv_hfp_event_e event, uint8_t *param, int param_size);

int btif_hfp_register_processer(void);
int btif_hfp_start(btsrv_hfp_callback cb);
int btif_hfp_stop(void);
int btif_hfp_hf_dial_number(uint8_t *number);
int btif_hfp_hf_dial_last_number(void);
int btif_hfp_hf_dial_memory(int location);
int btif_hfp_hf_volume_control(uint8_t type, uint8_t volume);
int btif_hfp_hf_battery_report(uint8_t mode, uint8_t bat_val);
int btif_hfp_hf_accept_call(void);
int btif_hfp_hf_reject_call(void);
int btif_hfp_hf_hangup_call(void);
int btif_hfp_hf_hangup_another_call(void);
int btif_hfp_hf_holdcur_answer_call(void);
int btif_hfp_hf_hangupcur_answer_call(void);
int btif_hfp_hf_voice_recognition_start(void);
int btif_hfp_hf_voice_recognition_stop(void);
int btif_hfp_hf_send_at_command(uint8_t *command,uint8_t active_call);
int btif_hfp_hf_switch_sound_source(void);
int btif_hfp_hf_get_call_state(uint8_t active_call);
int btif_hfp_get_dev_num(void);

int btif_hfp_hf_get_time(void);
uint16_t btif_hfp_get_active_hdl(void);
void btif_hfp_get_active_conn_handle(uint16_t *handle);
#ifdef CONFIG_BT_TRANSCEIVER
int btif_pts_hfp_active_disconnect_sco(void);
#endif

io_stream_t sco_upload_stream_create(uint8_t hfp_codec_id);

typedef void (*btsrv_sco_callback)(uint16_t hdl, btsrv_hfp_event_e event,  uint8_t *param, int param_size);
int btif_sco_start(btsrv_sco_callback cb);
int btif_sco_stop(void);
int btif_disconnect_sco(uint16_t acl_hdl);

/*=============================================================================
 *				hfp ag service api
 *===========================================================================*/
enum btsrv_hfp_hf_ag_indicators {
	BTSRV_HFP_AG_SERVICE_IND,
	BTSRV_HFP_AG_CALL_IND,
	BTSRV_HFP_AG_CALL_SETUP_IND,
	BTSRV_HFP_AG_CALL_HELD_IND,
	BTSRV_HFP_AG_SINGNAL_IND,
	BTSRV_HFP_AG_ROAM_IND,
	BTSRV_HFP_AG_BATTERY_IND,
	BTSRV_HFP_AG_MAX_IND,
};

typedef void (*btsrv_hfp_ag_callback)(btsrv_hfp_event_e event,  uint8_t *param, int param_size);

int btif_hfp_ag_register_processer(void);
int btif_hfp_ag_start(btsrv_hfp_ag_callback cb);
int btif_hfp_ag_stop(void);
int btif_hfp_ag_update_indicator(enum btsrv_hfp_hf_ag_indicators index, uint8_t value);
int btif_hfp_ag_send_event(uint8_t *event, uint16_t len);

/*=============================================================================
 *				spp service api
 *===========================================================================*/
#define SPP_UUID_LEN		16

/** Callbacks to report spp profile events*/
typedef enum {
	/** notifying that spp register failed */
	BTSRV_SPP_REGISTER_FAILED,
	/** notifying that spp connect failed */
	BTSRV_SPP_REGISTER_SUCCESS,
	/** notifying that spp connect failed */
	BTSRV_SPP_CONNECT_FAILED,
	/** notifying that spp connected*/
	BTSRV_SPP_CONNECTED,
	/** notifying that spp disconnected */
	BTSRV_SPP_DISCONNECTED,
	/** notifying that spp stream data indicated */
	BTSRV_SPP_DATA_INDICATED,
	/** notifying that snoop spp connected*/
	BTSRV_SPP_SNOOP_CONNECTED,
	/** notifying that snoop spp disconnected */
	BTSRV_SPP_SNOOP_DISCONNECTED,
} btsrv_spp_event_e;

typedef void (*btsrv_spp_callback)(btsrv_spp_event_e event, uint8_t app_id, void *packet, int size);

struct bt_spp_reg_param {
	uint8_t app_id;
	uint8_t *uuid;
};

struct bt_spp_connect_param {
	uint8_t app_id;
	bd_address_t bd;
	uint8_t *uuid;
};

int btif_spp_register_processer(void);
int btif_spp_reg(struct bt_spp_reg_param *param);
int btif_spp_send_data(uint8_t app_id, uint8_t *data, uint32_t len);
int btif_spp_connect(struct bt_spp_connect_param *param);
int btif_spp_disconnect(uint8_t app_id);
int btif_spp_start(btsrv_spp_callback cb);
int btif_spp_stop(void);

/*=============================================================================
 *				PBAP service api
 *===========================================================================*/
typedef struct parse_vcard_result pbap_vcard_result_t;

typedef enum {
	/** notifying that pbap connect failed*/
	BTSRV_PBAP_CONNECT_FAILED,
	/** notifying that pbap connected*/
	BTSRV_PBAP_CONNECTED,
	/** notifying that pbap disconnected */
	BTSRV_PBAP_DISCONNECTED,
	/** notifying that pbap vcard result */
	BTSRV_PBAP_VCARD_RESULT,
} btsrv_pbap_event_e;

typedef void (*btsrv_pbap_callback)(btsrv_pbap_event_e event, uint8_t app_id, void *data, uint8_t size);

struct bt_get_pb_param {
	bd_address_t bd;
	uint8_t app_id;
	char *pb_path;
	btsrv_pbap_callback cb;
};

int btif_pbap_register_processer(void);
int btif_pbap_get_phonebook(struct bt_get_pb_param *param);
int btif_pbap_abort_get(uint8_t app_id);

/*=============================================================================
 *				MAP service api
 *===========================================================================*/
typedef enum {
	/** notifying that pbap connect failed*/
	BTSRV_MAP_CONNECT_FAILED,
	/** notifying that pbap connected*/
	BTSRV_MAP_CONNECTED,
	/** notifying that pbap disconnected */
	BTSRV_MAP_DISCONNECTED,
	/** notifying that pbap disconnected */
	BTSRV_MAP_SET_PATH_FINISHED,

	BTSRV_MAP_MESSAGES_RESULT,
} btsrv_map_event_e;

typedef void (*btsrv_map_callback)(btsrv_map_event_e event, uint8_t app_id, void *data, uint8_t size);

struct bt_map_connect_param {
	bd_address_t bd;
	uint8_t app_id;
	char *map_path;
	btsrv_map_callback cb;
};

struct bt_map_get_param {
	bd_address_t bd;
	uint8_t app_id;
	char *map_path;
	btsrv_map_callback cb;
};

struct bt_map_set_folder_param {
	uint8_t app_id;
	uint8_t flags;
	char *map_path; 
};

struct bt_map_get_messages_listing_param {
    uint8_t app_id;
    uint8_t reserved;
    uint16_t max_list_count;
    uint32_t parameter_mask;
};

int btif_map_client_connect(struct bt_map_connect_param *param);
int btif_map_get_message(struct bt_map_get_param *param);
int btif_map_get_messages_listing(struct bt_map_get_messages_listing_param *param);
int btif_map_get_folder_listing(uint8_t app_id);
int btif_map_abort_get(uint8_t app_id);
int btif_map_client_disconnect(uint8_t app_id);
int btif_map_client_set_folder(struct bt_map_set_folder_param *param);
int btif_map_register_processer(void);


/*=============================================================================
 *				hid service api
 *===========================================================================*/

struct bt_device_id_info{
	uint16_t vendor_id;
	uint16_t product_id;
	uint16_t id_source;
};

/* Different report types in get, set, data
*/
enum {
	BTSRV_HID_REP_TYPE_OTHER=0,
	BTSRV_HID_REP_TYPE_INPUT,
	BTSRV_HID_REP_TYPE_OUTPUT,
	BTSRV_HID_REP_TYPE_FEATURE,
};

/* Parameters for Handshake
*/
enum {
	BTSRV_HID_HANDSHAKE_RSP_SUCCESS=0,
	BTSRV_HID_HANDSHAKE_RSP_NOT_READY,
	BTSRV_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID,
	BTSRV_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ,
	BTSRV_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM,
	BTSRV_HID_HANDSHAKE_RSP_ERR_UNKNOWN=14,
	BTSRV_HID_HANDSHAKE_RSP_ERR_FATAL,
};

#define BT_HID_REPORT_DATA_LEN	64

struct bt_hid_report {
	uint16_t hdl;
	uint8_t report_type;
	uint8_t has_size; 
	uint8_t data[BT_HID_REPORT_DATA_LEN];
	int len;
};

/** Callbacks to report hid profile events*/
typedef enum {
	/** notifying that hid connected*/
	BTSRV_HID_CONNECTED,
	/** notifying that hid disconnected */
	BTSRV_HID_DISCONNECTED,
	/** notifying hid get reprot data */
	BTSRV_HID_GET_REPORT,
	/** notifying hid set reprot data */
	BTSRV_HID_SET_REPORT,
	/** notifying hid get protocol data */
	BTSRV_HID_GET_PROTOCOL,
	/** notifying hid set protocol data */
	BTSRV_HID_SET_PROTOCOL,
	/** notifying hid intr data */
	BTSRV_HID_INTR_DATA,
	/** notifying hid unplug */
	BTSRV_HID_UNPLUG,
	/** notifying hid suspend */
	BTSRV_HID_SUSPEND,
	/** notifying hid exit suspend */
	BTSRV_HID_EXIT_SUSPEND,
} btsrv_hid_event_e;

typedef void (*btsrv_hid_callback)(uint16_t hdl, btsrv_hid_event_e event, void *packet, int size);

int btif_hid_register_processer(void);
int btif_did_register_sdp(uint8_t *data, uint32_t len);
int btif_hid_register_sdp(struct bt_sdp_attribute * hid_attrs,uint8_t attrs_size);
int btif_hid_send_ctrl_data(uint16_t hdl, uint8_t report_type,uint8_t *data, uint32_t len);
int btif_hid_send_intr_data(uint16_t hdl, uint8_t report_type,uint8_t *data, uint32_t len);
int btif_hid_send_rsp(uint16_t hdl, uint8_t status);
int btif_hid_connect(uint16_t hdl);
int btif_hid_disconnect(uint16_t hdl);
int btif_hid_start(btsrv_hid_callback cb);
int btif_hid_stop(void);

/** Connection Type */
enum {
	/** LE Connection Type */
	BT_TYPE_LE = BIT(0),
	/** BR/EDR Connection Type */
	BT_TYPE_BR = BIT(1),
	/** SCO Connection Type */
	BT_TYPE_SCO = BIT(2),
	/** ISO Connection Type */
	BT_TYPE_ISO = BIT(3),
	/** BR/EDR SNOOP Connection Type */
	BT_TYPE_BR_SNOOP = BIT(4),
	/** SCO SNOOP Connection Type */
	BT_TYPE_SCO_SNOOP = BIT(5),	
	/** All Connection Type */
	BT_TYPE_ALL = BT_CONN_TYPE_LE | BT_CONN_TYPE_BR |
			   BT_CONN_TYPE_SCO | BT_CONN_TYPE_ISO | BT_TYPE_BR_SNOOP | BT_TYPE_SCO_SNOOP,
};

struct bt_audio_config {
	uint32_t num_master_conn : 3;	/* as a master, range: [0: 7] */
	uint32_t num_slave_conn : 3;	/* as a slave, range: [0: 7] */

	/* if BR coexists */
	uint32_t br : 1;

	/* if support encryption */
	uint32_t encryption : 1;
	/*
	 * Authenticated Secure Connections and 128-bit key which depends
	 * on encryption.
	 */
	uint32_t secure : 1;

	/* whether support pacs_client */
	uint32_t pacs_cli_num : 1;
	/* whether support unicast_client */
	uint32_t uni_cli_num : 1;
	/* whether support volume_controller */
	uint32_t vol_cli_num : 1;
	/* whether support microphone_controller */
	uint32_t mic_cli_num : 1;
	/* whether support call_control_server */
	uint32_t call_srv_num : 1;
	/* whether support media_control_server */
	uint32_t media_srv_num : 1;
	/* whether support cas_set_coordinator */
	uint32_t cas_cli_num : 1;

	/* whether support pacs_server */
	uint32_t pacs_srv_num : 1;
	/* whether support unicast_server */
	uint32_t uni_srv_num : 1;
	/* whether support volume_controller */
	uint32_t vol_srv_num : 1;
	/* whether support microphone_device */
	uint32_t mic_srv_num : 1;
	/* whether support call_control_client */
	uint32_t call_cli_num : 1;
	/* whether support media_control_client */
	uint32_t media_cli_num : 1;
	/* whether support cas_set_member */
	uint32_t cas_srv_num : 1;

	/* whether support tmas_client */
	uint32_t tmas_cli_num : 1;

	/* support how many connections for unicast_client */
	uint32_t uni_cli_dev_num : 3;	/* [0 : 7] */
	/* support how many connections for unicast_server */
	uint32_t uni_srv_dev_num : 3;	/* [0 : 7] */

	/* support how many CIS channels, >= num_conn */
	uint32_t cis_chan_num : 5;	/* [0 : 31] */
	/* support how many ASEs for ASCS Server */
	uint32_t ascs_srv_ase_num : 5;	/* [0 : 31] */
	/* support how many ASEs for ASCS Client */
	uint32_t ascs_cli_ase_num : 5;	/* [0 : 31] */

	/* support how many connections for call_control_client */
	uint32_t call_cli_dev_num : 3;	/* [0 : 7] */
	/* support how many connections for pacs_client */
	uint32_t pacs_cli_dev_num : 3;	/* [0 : 7] */
	/* support how many connections for tmas_client */
	uint32_t tmas_cli_dev_num : 3;	/* [0 : 7] */
	/* support how many connections for cas_client */
	uint32_t cas_cli_dev_num : 3;	/* [0 : 7] */

	/* support how many PAC Characteristic for server */
	uint32_t pacs_srv_pac_num : 3;	/* [0 : 7] */
	/* support how many PAC Characteristics for client */
	uint32_t pacs_cli_pac_num : 5;	/* [0 : 31] */
	/* support how many PAC records for client */
	uint32_t pacs_pac_num : 5;	/* [0 : 31] */

	/* support how many connections for volume_control_client */
	uint32_t vcs_cli_dev_num : 3;  /* [0 : 7] */
	/* support how many connections for microphone_control_client */
	uint32_t mic_cli_dev_num : 3;  /* [0 : 7] */
	/* support how many connections for media_control_client */
	uint32_t mcs_cli_dev_num : 3;  /* [0 : 7] */

	/* support how many connections for aics_server */
	uint32_t aics_srv_num : 3;  /* [0 : 7] */
	/* support how many connections for aics_client */
	uint32_t aics_cli_num : 5;  /* [0 : 31] */

	/* if support remote keys */
	uint32_t remote_keys : 1;

	/* matching remote(s) with LE Audio test address(es) */
	uint32_t test_addr : 1;

	/* if enable master latency function */
	uint32_t master_latency : 1;

	/* current id, in order to support different functions */
	uint8_t keys_id;

	/* if set_sirk is enabled  */
	uint32_t sirk_enabled : 1;
	/* if set_sirk is encrypted (otherwise plain) */
	uint32_t sirk_encrypted : 1;

	/* only works for cas_srv_num (set_member) */
	uint8_t set_size;
	uint8_t set_rank;
	uint8_t set_sirk[16];	/* depends on sirk_enabled */

	/* vcs master volume initial value and step */
	uint8_t initial_volume;
	uint8_t volume_step;
};

/*
 * Bt Audio Codecs
 */
#define BT_AUDIO_CODEC_UNKNOWN   0x0
/* LE Audio */
#define BT_AUDIO_CODEC_LC3       0x1
/* BR Call */
#define BT_AUDIO_CODEC_CVSD      0x2
#define BT_AUDIO_CODEC_MSBC      0x3
/* BR Music */
#define BT_AUDIO_CODEC_AAC       0x4
#define BT_AUDIO_CODEC_SBC       0x5

/* Messages to report LE Audio events */
typedef enum {
	BTSRV_ACL_CONNECTED = 1,
	BTSRV_ACL_DISCONNECTED,
	BTSRV_CIS_CONNECTED,
	BTSRV_CIS_DISCONNECTED,

	/* ASCS */
	BTSRV_ASCS_CONFIG_CODEC = 10,
	BTSRV_ASCS_PREFER_QOS,
	BTSRV_ASCS_CONFIG_QOS,
	BTSRV_ASCS_ENABLE,
	BTSRV_ASCS_DISABLE,
	BTSRV_ASCS_UPDATE,
	BTSRV_ASCS_RECV_START,
	BTSRV_ASCS_RECV_STOP,
	BTSRV_ASCS_RELEASE,
	BTSRV_ASCS_ASES,	/* ASE topology */

	/* PACS */
	BTSRV_PACS_CAPS,	/* PACS capabilities */

	/* CIS */
	BTSRV_CIS_PARAMS,

	/* TBS */
	BTSRV_TBS_CONNECTED = 30,
	BTSRV_TBS_DISCONNECTED,
	BTSRV_TBS_INCOMING,
	BTSRV_TBS_DIALING,	/* outgoing */
	BTSRV_TBS_ALERTING,	/* outgoing */
	BTSRV_TBS_ACTIVE,
	BTSRV_TBS_LOCALLY_HELD,
	BTSRV_TBS_REMOTELY_HELD,
	BTSRV_TBS_HELD,
	BTSRV_TBS_ENDED,

	/* VCS */
	/* as a slave, report message initiated by a master */
	BTSRV_VCS_UP = 50,
	BTSRV_VCS_DOWN,
	BTSRV_VCS_VALUE,
	BTSRV_VCS_MUTE,
	BTSRV_VCS_UNMUTE,
	BTSRV_VCS_UNMUTE_UP,
	BTSRV_VCS_UNMUTE_DOWN,
	/* as a master, report message initiated by a slave */
	BTSRV_VCS_CLIENT_CONNECTED,
	BTSRV_VCS_CLIENT_UP,
	BTSRV_VCS_CLIENT_DOWN,
	BTSRV_VCS_CLIENT_VALUE,
	BTSRV_VCS_CLIENT_MUTE,
	BTSRV_VCS_CLIENT_UNMUTE,
	BTSRV_VCS_CLIENT_UNMUTE_UP,
	BTSRV_VCS_CLIENT_UNMUTE_DOWN,

	/* as a slave, report message initiated by a master */
	BTSRV_MICS_MUTE = 80,
	BTSRV_MICS_UNMUTE,
	BTSRV_MICS_GAIN_VALUE,
	/* as a master, report message initiated by a slave */
	BTSRV_MICS_CLIENT_CONNECTED,
	BTSRV_MICS_CLIENT_MUTE,
	BTSRV_MICS_CLIENT_UNMUTE,
	BTSRV_MICS_CLIENT_GAIN_VALUE,

	/* as a server, report message initiated by a client */
	BTSRV_MCS_SERVER_PLAY = 100,
	BTSRV_MCS_SERVER_PAUSE,
	BTSRV_MCS_SERVER_STOP,
	BTSRV_MCS_SERVER_FAST_REWIND,
	BTSRV_MCS_SERVER_FAST_FORWARD,
	BTSRV_MCS_SERVER_NEXT_TRACK,
	BTSRV_MCS_SERVER_PREV_TRACK,
	/* as a client, report message initiated by a server */
	BTSRV_MCS_CONNECTED,
	BTSRV_MCS_PLAY,
	BTSRV_MCS_PAUSE,
	BTSRV_MCS_STOP,
} btsrv_audio_event_e;

typedef void (*btsrv_audio_callback)(btsrv_audio_event_e event, void *data, int size);

/*
 * Bt Audio Direction
 *
 * 1. for client, BT_AUDIO_DIR_SINK means Audio Source role.
 * 2. for client, BT_AUDIO_DIR_SOURCE means Audio Sink role.
 * 3. for server, BT_AUDIO_DIR_SINK means Audio Sink role.
 * 4. for server, BT_AUDIO_DIR_SOURCE means Audio Source role.
 */
#define BT_AUDIO_DIR_SINK	0x0
#define BT_AUDIO_DIR_SOURCE	0x1

/* Framing Supported */
#define BT_AUDIO_UNFRAMED_SUPPORTED	0x00
#define BT_AUDIO_UNFRAMED_NOT_SUPPORTED	0x01

static inline bool bt_audio_unframed_supported(uint8_t framing)
{
	if (framing == BT_AUDIO_UNFRAMED_SUPPORTED) {
		return true;
	}

	return false;
}

/* Framing */
#define BT_AUDIO_UNFRAMED	0x00
#define BT_AUDIO_FRAMED	0x01


/* PHY & Preferred_PHY */
#define BT_AUDIO_PHY_1M	BIT(0)
#define BT_AUDIO_PHY_2M	BIT(1)
#define BT_AUDIO_PHY_CODED	BIT(2)

/* Target_Latency */
#define BT_AUDIO_TARGET_LATENCY_LOW	0x01
#define BT_AUDIO_TARGET_LATENCY_BALANCED	0x02
#define BT_AUDIO_TARGET_LATENCY_HIGH	0x03

/* Target_PHY */
#define BT_AUDIO_TARGET_PHY_1M	0x01
#define BT_AUDIO_TARGET_PHY_2M	0x02
#define BT_AUDIO_TARGET_PHY_CODED	0x03

/* Packing */
#define BT_AUDIO_PACKING_SEQUENTIAL	0x00
#define BT_AUDIO_PACKING_INTERLEAVED	0x01

/* Supported Audio Capability of Server */
struct bt_audio_capability {
	uint8_t format;	/* codec format */
	/* units below: refers to codec.h */
	uint8_t durations;	/* frame_durations */
	uint8_t channels;	/* channel_counts */
	uint8_t frames;	/* max_codec_frames_per_sdu */
	uint16_t samples;
	uint16_t octets_min;	/* octets_per_codec_frame_min */
	uint16_t octets_max;	/* octets_per_codec_frame_max */
	uint16_t preferred_contexts;
};

/* Supported Audio Capabilities of Server */
struct bt_audio_capabilities {
	uint16_t handle;	/* for client to distinguish different server */
	uint8_t source_cap_num;
	uint8_t sink_cap_num;

	uint16_t source_available_contexts;
	uint16_t source_supported_contexts;
	uint32_t source_locations;

	uint16_t sink_available_contexts;
	uint16_t sink_supported_contexts;
	uint32_t sink_locations;

	/* source cap first if any, followed by sink cap */
	struct bt_audio_capability cap[0];
};

/* endpoint for br */
#define BT_AUDIO_ENDPOINT_MUSIC    	0
#define BT_AUDIO_ENDPOINT_CALL  	1

/* Stream/Endpoint mapping of Server */
struct bt_audio_endpoint {
	uint8_t id;	/* endpoint id; 0: invalid, range: [1, 255] */
	uint8_t dir : 1;	/* BT_AUDIO_DIR_SINK/SOURCE */
	/*
	 * NOTICE: BT_AUDIO_STREAM_IDLE and BT_AUDIO_STREAM_CODEC_CONFIGURED
	 * are useful only, others are useless!
	 */
	uint8_t state : 3;	/* Audio Stream state */
};

struct bt_audio_endpoints {
	uint16_t handle;
	uint8_t num_of_endp;
	struct bt_audio_endpoint endp[0];
};

/*
 * 1. used to report codec info configured by the client to the server.
 * 2. used to report codec info configured by the server to the client.
 * 3. used to set codec info by the client.
 */
struct bt_audio_codec {
	uint16_t handle;
	uint8_t id;
	uint8_t dir;
	uint8_t format;	/* codec format */
	uint8_t target_latency;
	uint8_t target_phy;
	uint8_t frame_duration;
	uint8_t blocks;	/* codec_frame_blocks_per_sdu */
	uint16_t sample_rate;	/* unit: kHz */
	uint16_t octets;	/* octets_per_codec_frame */
	uint32_t locations;
};

/* Used to report preferred qos info of server */
struct bt_audio_preferred_qos {
	uint16_t handle;
	uint8_t id; /* endpoint id */
	uint8_t framing;
	uint8_t phy;
	uint8_t rtn;
	uint16_t latency;	/* max_transport_latency */
	uint32_t delay_min;	/* presentation_delay_min */
	uint32_t delay_max;	/* presentation_delay_max */
	uint32_t preferred_delay_min;	/* presentation_delay_min */
	uint32_t preferred_delay_max;	/* presentation_delay_max */
};

/*
 * Qos of Server
 *
 * variables with suffix "_m" means the direction is M_To_S,
 * variables with suffix "_s" means the direction is S_To_M.
 */
struct bt_audio_qos {
	uint16_t handle;

	/*
	 * case 1: id_m_to_s != 0 && id_s_to_m != 0, bidirectional
	 * case 2: id_m_to_s != 0 && id_s_to_m == 0, unidirectional
	 * case 3: id_m_to_s == 0 && id_s_to_m != 0, unidirectional
	 */
	uint8_t id_m_to_s;	/* sink endpoint id */
	uint8_t id_s_to_m;	/* source endpoint id */

	uint16_t max_sdu_m;
	uint16_t max_sdu_s;

	uint8_t phy_m;
	uint8_t phy_s;

	uint8_t rtn_m;
	uint8_t rtn_s;

	/* processing_time for this connection only, unit: us */
	uint32_t processing_m;
	uint32_t processing_s;

	/* presentation_delay for this connection only, unit: us */
	uint32_t delay_m;
	uint32_t delay_s;
};

/* Used to config qos actively by the client */
struct bt_audio_qoss {
	uint8_t num_of_qos;
	uint8_t sca;
	uint8_t packing;
	uint8_t framing;

	/* sdu_interval, unit: us */
	uint32_t interval_m;
	uint32_t interval_s;

	/* max_transport_latency, unit: ms */
	uint16_t latency_m;
	uint16_t latency_s;

	/*
	 * presentation_delay for all connections, unit: us
	 */
	/* mutual exclusive with qos->delay_m */
	uint32_t delay_m;
	/* mutual exclusive with qos->delay_s */
	uint32_t delay_s;

	/*
	 * processing_time for all connections, unit: us
	 */
	/* mutual exclusive with qos->processing_m */
	uint32_t processing_m;
	/* mutual exclusive with qos->processing_s */
	uint32_t processing_s;

	struct bt_audio_qos qos[0];	/* 1 for each bt_audio connection */
};

/* Used to report qos info configured by the client to the server */
struct bt_audio_qos_configured {
	uint16_t handle;
	uint8_t id;	/* endpoint id */
	uint8_t framing;
	/* sdu_interval, unit: us */
	uint32_t interval;
	/* presentation_delay, unit: us */
	uint32_t delay;
	/* max_transport_latency, unit: ms */
	uint16_t latency;
	uint16_t max_sdu;
	uint8_t phy;
	uint8_t rtn;
};

/* used to report the current audio stream state and info */
struct bt_audio_report {
	uint16_t handle;
	uint8_t id;	/* endpoint id */
	/* NOTICE: only for enable and update */
	uint16_t audio_contexts;
};

#define MAGIC_BT_AUDIO_STREAM_ENABLE	0xEA
#define MAGIC_BT_AUDIO_STREAM_DISABLE	0xDA
struct bt_audio_chan {
	uint16_t handle;	/* indicate the connection */
	uint8_t id;	/* endpoint id */
	uint8_t magic;
};

typedef void (*bt_audio_trigger_cb)(uint16_t handle, uint8_t id,
				uint16_t audio_handle);

struct bt_audio_stream_cb {
	bt_audio_trigger_cb tx_trigger;
	bt_audio_trigger_cb tx_period_trigger;
	bt_audio_trigger_cb rx_trigger;
	bt_audio_trigger_cb rx_period_trigger;
};

struct bt_audio_params {
	uint16_t handle;	/* indicate the connection */
	uint8_t id;	/* endpoint id */
	uint8_t nse;
	uint8_t m_bn;
	uint8_t s_bn;
	uint8_t m_ft;
	uint8_t s_ft;
	uint32_t cig_sync_delay;
	uint32_t cis_sync_delay;
	uint32_t rx_delay;
	uint16_t audio_handle;	/* indicate the audio connection */
};

/*
 * Termination Reason
 */
#define BT_CALL_REASON_URI               0x00
#define BT_CALL_REASON_CALL_FAILED       0x01
#define BT_CALL_REASON_REMOTE_END        0x02
#define BT_CALL_REASON_SERVER_END        0x03
#define BT_CALL_REASON_LINE_BUSY         0x04
#define BT_CALL_REASON_NETWORK           0x05
#define BT_CALL_REASON_CLIENT_END        0x06
#define BT_CALL_REASON_NO_SERVICE        0x07
#define BT_CALL_REASON_NO_ANSWER         0x08
#define BT_CALL_REASON_UNSPECIFIED       0x09

#define BT_AUDIO_CALL_URI_LEN	24

struct bt_call_report_simple {
	uint16_t handle;
	uint8_t index;	/* call index */
};

/* used to report the current call state and info */
struct bt_call_report {
	uint16_t handle;
	uint8_t index;	/* call index */

	uint8_t reason;	/* termination reason */

	/*
	 * remote's uri, NULL if unnecessary, end with '\0' if valid
	 */
	uint8_t *uri;
};

struct bt_audio_call {
	/*
	 * For call gateway, "handle" will not be used to distinguish diffrent
	 * call (should use "index" instead), because for multiple terminals
	 * cases, all terminals share the same call list.
	 *
	 * For call terminal, "handle" will be used to distinguish diffrent
	 * call gateway, "index" will be used to distinguish diffrent call of
	 * the same call gateway.
	 */
	uint16_t handle;
	uint8_t index;	/* call index */
};

#if 0	/* Already define before */
struct bt_attribute_info {
	uint32_t id;
	uint16_t character_id;
	uint16_t len;
	uint8_t *data;
} __packed;
#endif

#define BT_TOTAL_ATTRIBUTE_ITEM_NUM   5

struct bt_media_id3_info {
	uint16_t handle;
	uint8_t num_of_item;

	struct bt_attribute_info item[0];
};

struct bt_media_play_status {
	uint16_t handle;
	uint8_t status;
};

struct bt_audio_media {
	uint16_t handle;

	uint8_t *name;
};

struct bt_media_report {
	uint16_t handle;
};

struct bt_volume_report {
	uint16_t handle;
	/* only for volume set */
	uint8_t value;
	uint8_t from_phone;
	uint8_t mute;
};

struct bt_audio_volume {
	uint16_t handle;
	uint8_t type;

	uint8_t value;
	uint8_t mute;
	uint32_t location;	/* 0: whole device */
	// TODO:
};

struct bt_mic_report {
	uint16_t handle;
	/* only for gain value */
	uint8_t gain;
};

struct bt_audio_microphone {
	uint16_t handle;
	uint8_t type;

	uint8_t mute;
	// TODO:
};

/* if the related handle (BT Connection) belongs to the same group */
struct bt_audio_group {
	uint8_t type;
	uint8_t num_of_handle;
	uint16_t handle[0];
};


struct bt_disconn_report {
	uint16_t handle;
	uint8_t reason;
};



typedef int (*bt_audio_vnd_rx_cb)(uint16_t handle, const uint8_t *buf,
				uint16_t len);

typedef void (*bt_audio_adv_cb)(void);



/*
 * LE Audio
 */

int btif_audio_register_processer(void);
int btif_audio_init(struct bt_audio_config config, btsrv_audio_callback cb,
				void *data, int size);
int btif_audio_exit(void);
int btif_audio_keys_clear(void);
int btif_audio_adv_param_init(struct bt_le_adv_param *param);
int btif_audio_scan_param_init(struct bt_le_scan_param *param);
int btif_audio_conn_create_param_init(struct bt_conn_le_create_param *param);
int btif_audio_conn_param_init(struct bt_le_conn_param *param);
int btif_audio_conn_idle_param_init(struct bt_le_conn_param *param);
int btif_audio_start(void);
int btif_audio_stop(void);
int btif_audio_pause(void);
int btif_audio_resume(void);
bool btif_audio_stopped(void);
int btif_audio_server_cap_init(struct bt_audio_capabilities *caps);
int btif_audio_stream_config_codec(struct bt_audio_codec *codec);
int btif_audio_stream_prefer_qos(struct bt_audio_preferred_qos *qos);
int btif_audio_stream_config_qos(struct bt_audio_qoss *qoss);
int btif_audio_stream_bind_qos(struct bt_audio_qoss *qoss, uint8_t index);
int btif_audio_stream_sync_forward(struct bt_audio_chan **chans,
				uint8_t num_chans);
int btif_audio_stream_enable(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t contexts);
int btif_audio_stream_disble(struct bt_audio_chan **chans, uint8_t num_chans);
io_stream_t btif_audio_source_stream_create(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t locations);
int btif_audio_source_stream_set(struct bt_audio_chan **chans,
				uint8_t num_chans, io_stream_t stream,
				uint32_t locations);
int btif_audio_sink_stream_set(struct bt_audio_chan *chan, io_stream_t stream);
int btif_audio_sink_stream_start(struct bt_audio_chan **chans,
				uint8_t num_chans);
int btif_audio_sink_stream_stop(struct bt_audio_chan **chans,
				uint8_t num_chans);
int btif_audio_stream_update(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t contexts);
int btif_audio_stream_release(struct bt_audio_chan **chans,
				uint8_t num_chans);
int btif_audio_stream_released(struct bt_audio_chan *chan);
int btif_audio_stream_disconnect(struct bt_audio_chan **chans,
				uint8_t num_chans);
void *btif_audio_stream_bandwidth_alloc(struct bt_audio_qoss *qoss);
int btif_audio_stream_bandwidth_free(void *cig_handle);
int btif_audio_stream_cb_register(struct bt_audio_stream_cb *cb);
int btif_audio_volume_up(uint16_t handle);
int btif_audio_volume_down(uint16_t handle);
int btif_audio_volume_set(uint16_t handle, uint8_t value);
int btif_audio_volume_mute(uint16_t handle);
int btif_audio_volume_unmute(uint16_t handle);
int btif_audio_volume_unmute_up(uint16_t handle);
int btif_audio_volume_unmute_down(uint16_t handle);
int btif_audio_mic_mute(uint16_t handle);
int btif_audio_mic_unmute(uint16_t handle);
int btif_audio_mic_mute_disable(uint16_t handle);
int btif_audio_mic_gain_set(uint16_t handle, uint8_t gain);
int btif_audio_media_play(uint16_t handle);
int btif_audio_media_pause(uint16_t handle);
int btif_audio_media_fast_rewind(uint16_t handle);
int btif_audio_media_fast_forward(uint16_t handle);
int btif_audio_media_stop(uint16_t handle);
int btif_audio_media_play_prev(uint16_t handle);
int btif_audio_media_play_next(uint16_t handle);
int btif_audio_conn_max_len(uint16_t handle);
int btif_audio_vnd_register_rx_cb(uint16_t handle, bt_audio_vnd_rx_cb rx_cb);
int btif_audio_vnd_send(uint16_t handle, uint8_t *buf, uint16_t len);
int btif_audio_adv_cb_register(bt_audio_adv_cb start, bt_audio_adv_cb stop);
int btif_audio_start_adv(void);
int btif_audio_stop_adv(void);
int btif_audio_call_originate(uint16_t handle, uint8_t *remote_uri);
int btif_audio_call_accept(struct bt_audio_call *call);
int btif_audio_call_hold(struct bt_audio_call *call);
int btif_audio_call_retrieve(struct bt_audio_call *call);
int btif_audio_call_terminate(struct bt_audio_call *call);

/*
 * LE Audio end
 */



/**
 * @} end defgroup bt_service_apis
 */
#ifdef CONFIG_BT_TRANSCEIVER
#include <tr_btservice_trs_api.h>
#include "tr_btservice_api.h"
#endif
#endif
