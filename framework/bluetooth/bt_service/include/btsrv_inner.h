/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt service inner head file
 */

#ifndef _BTSRV_INNER_H_
#define _BTSRV_INNER_H_

#include <bt_configs.h>
#include <btservice_api.h>
#include <acts_bluetooth/host_interface.h>
#include <hci_core.h>
#include <../tws_porting/btsrv_tws.h>

#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
#include <thread_timer.h>
#endif

/* Two phone device, one tws device */
#define BTSRV_SAVE_AUTOCONN_NUM					(3)
#define BT_MAC_LEN								(6)
#define BTSRV_RECONNECT_DISCOVERABLE_REMAIN     (3)

enum {
	LINK_ADJUST_IDLE,
	LINK_ADJUST_RUNNING,
	LINK_ADJUST_STOP,
	LINK_ADJUST_SINK_BLOCK,
	LINK_ADJUST_SINK_CLEAR_BLOCK,
};

enum {
	AUTOCONN_INFO_TWS      = 1,
	AUTOCONN_INFO_PHONE    = 2,
	AUTOCONN_INFO_ALL      = 3,
};

typedef enum {
	/** first time reconnect */
	RECONN_STARTUP,
	/** reconnect when timeout */
	RECONN_TIMEOUT,
} btstack_reconnect_mode_e;

struct rdm_sniff_info {
	uint8_t sniff_mode:4;
	uint8_t sniff_entering:1;
	uint8_t sniff_exiting:1;
	uint16_t sniff_interval;
	uint16_t conn_rxtx_cnt;
	uint32_t idle_start_time;
	uint32_t sniff_entering_time;
};

typedef struct  {
	int8_t event;
	const char *str;
}btsrv_event_strmap_t;

typedef void (*rdm_connected_dev_cb)(struct bt_conn *base_conn, uint8_t tws_dev, void *cb_param);
typedef void (*rdm_need_snoop_sync_dev_cb)(struct bt_conn *base_conn, void *cb_param);

#define MEDIA_RTP_HEAD_LEN		12
#define AVDTP_SBC_HEADER_LEN	13
#define AVDTP_AAC_HEADER_LEN	16

enum {
	MEDIA_RTP_HEAD	   = 0x80,
	MEDIA_RTP_TYPE_SBC = 0x60,
	MEDIA_RTP_TYPE_AAC = 0x80,		/* 0x80 or 0x62 ?? */
};

/** avrcp device state */
typedef enum {
	BTSRV_AVRCP_PLAYSTATUS_STOPPED,
	BTSRV_AVRCP_PLAYSTATUS_PAUSEED,
	BTSRV_AVRCP_PLAYSTATUS_PLAYING,
} btsrv_avrcp_state_e;

enum {
	BTSRV_LINK_BASE_CONNECTED,
	BTSRV_LINK_BASE_CONNECTED_FAILED,
	BTSRV_LINK_BASE_CONNECTED_TIMEOUT,
	BTSRV_LINK_BASE_DISCONNECTED,
	BTSRV_LINK_BASE_GET_NAME,
	BTSRV_LINK_HFP_CONNECTED,
	BTSRV_LINK_HFP_DISCONNECTED,
	BTSRV_LINK_A2DP_CONNECTED,
	BTSRV_LINK_A2DP_DISCONNECTED,
	BTSRV_LINK_AVRCP_CONNECTED,
	BTSRV_LINK_AVRCP_DISCONNECTED,
	BTSRV_LINK_SPP_CONNECTED,
	BTSRV_LINK_SPP_DISCONNECTED,
	BTSRV_LINK_PBAP_CONNECTED,
	BTSRV_LINK_PBAP_DISCONNECTED,
	BTSRV_LINK_HID_CONNECTED,
	BTSRV_LINK_HID_DISCONNECTED,
	BTSRV_LINK_MAP_CONNECTED,
	BTSRV_LINK_MAP_DISCONNECTED,
    BTSRV_LINK_TWS_CONNECTED,
    BTSRV_LINK_TWS_CONNECTED_TIMEOUT,
};

enum {
	BTSRV_WAKE_LOCK_WAIT_CONNECT = (0x1 << 0),
	BTSRV_WAKE_LOCK_PAIR_MODE = (0x1 << 1),
	BTSRV_WAKE_LOCK_SEARCH = (0x1 << 2),
	BTSRV_WAKE_LOCK_RECONNECT = (0x1 << 3),

	BTSRV_WAKE_LOCK_CONN_BUSY = (0x1 << 8),
	BTSRV_WAKE_LOCK_CALL_BUSY = (0x1 << 9),
	BTSRV_WAKE_LOCK_TWS_IRQ_BUSY = (0x1 << 10),
};

enum {
	BT_INIT_NONE = 0,
	BT_INIT_START = 1,
	BT_INIT_SUCCESS = 2,
	BT_INIT_FAILED = 3,
	BT_STOPPING = 4,	
};

struct btsrv_info_t {
	uint8_t init_state:3;
	uint8_t allow_sco_connect:1;
	uint8_t share_mac_dev:1;
    uint8_t power_off:1;
    uint8_t link_idle:1;
    uint8_t lea_connected:1;	
    uint8_t lea_is_foreground_dev:1;	
	uint8_t pair_status;
	uint16_t bt_wake_lock;
	btsrv_callback callback;
	uint8_t device_name[CONFIG_MAX_BT_NAME_LEN + 1];
	uint8_t device_addr[BT_MAC_LEN];
	struct btsrv_config_info cfg;
	struct thread_timer wait_disconnect_timer;
	struct thread_timer wake_lock_timer;
};

struct btsrv_addr_name {
	bd_address_t mac;
	uint8_t name[CONFIG_MAX_BT_NAME_LEN + 1];
};

struct btsrv_mode_change_param {
	struct bt_conn *conn;
	uint8_t mode;
	uint16_t interval;
};

extern struct btsrv_info_t *btsrv_info;
#define btsrv_max_conn_num()                      btsrv_info->cfg.max_conn_num
#define btsrv_cfg_device_num()                    btsrv_info->cfg.cfg_device_num
#define btsrv_max_phone_num()                     btsrv_info->cfg.max_phone_num
#define btsrv_is_pts_test()                       btsrv_info->cfg.pts_test_mode
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
#define btsrv_is_br_tr()                          btsrv_info->cfg.br_tr_mode
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_BQB
#define btsrv_is_leaudio_bqb()                    btsrv_info->cfg.leaudio_bqb_mode
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
#define btsrv_is_leaudio_sr_bqb()                 btsrv_info->cfg.leaudio_bqb_sr_mode
#endif
#define btsrv_is_preemption_mode()                btsrv_info->cfg.double_preemption_mode
#define btsrv_volume_sync_delay_ms()              btsrv_info->cfg.volume_sync_delay_ms
#define btsrv_get_tws_version()                   btsrv_info->cfg.tws_version
#define btsrv_get_tws_feature()                   btsrv_info->cfg.tws_feature
#define btsrv_is_tws_reconnect_search_mode()      btsrv_info->cfg.use_search_mode_when_tws_reconnect
#define btsrv_sniff_enable()                      btsrv_info->cfg.sniff_enable
#define btsrv_idle_enter_sniff_time()             btsrv_info->cfg.idle_enter_sniff_time
#define btsrv_sniff_interval()                    btsrv_info->cfg.sniff_interval
#define btsrv_stop_another_when_one_playing()     btsrv_info->cfg.stop_another_when_one_playing
#define btsrv_resume_another_when_one_stopped()   btsrv_info->cfg.resume_another_when_one_stopped
#define btsrv_a2dp_status_stopped_delay_ms()      btsrv_info->cfg.a2dp_status_stopped_delay_ms

/* For snoop tws sync info start */

enum {
	BTSRV_SYNC_TYPE_BASE,
	BTSRV_SYNC_TYPE_A2DP,
	BTSRV_SYNC_TYPE_AVRCP,
	BTSRV_SYNC_TYPE_HFP,
	BTSRV_SYNC_TYPE_SPP,
};

/* Sync info not always change but need large size */
struct btsrv_sync_stack_info_ack {
	bd_address_t addr;
} __packed;

struct btsrv_sync_base_info_ext {
	uint8_t len;
  uint8_t data[]; /*node: type len data;type len data........*/
} __packed;

struct btsrv_sync_base_info {
	bd_address_t addr;
	uint8_t security_changed:1;
	uint8_t controler_role:1;
	uint8_t sniff_mode:4;
	uint8_t device_name[CONFIG_MAX_BT_NAME_LEN + 1];
} __packed;

struct btsrv_sync_paired_list_info {
	uint8_t pl_len;
    struct autoconn_info paired_list[BTSRV_SAVE_AUTOCONN_NUM];
} __packed;


struct btsrv_sync_reconnect_status_info {
	bd_address_t addr;
} __packed;


struct btsrv_sync_a2dp_info {
	bd_address_t addr;
	uint8_t callback_ev;		/* Event callback to bt manager */
	uint8_t a2dp_connected:1;
	uint8_t a2dp_active:1;
	uint8_t a2dp_priority;
	uint8_t format;
	uint8_t sample_rate;
	uint8_t cp_type;
	uint16_t a2dp_media_rx_cid;
} __packed;

struct btsrv_sync_avrcp_info {
	bd_address_t addr;
	uint8_t callback_ev;		/* Event callback to bt manager */
	uint8_t avrcp_connected:1;
} __packed;

struct btsrv_sync_hfp_info {
	bd_address_t addr;
	uint8_t callback_ev;		/* Event callback to bt manager */
	uint8_t hfp_connected:1;
	uint8_t hfp_notify_phone_num:1;
	uint8_t hfp_active_state:3;
	uint8_t siri_state:3;
	uint8_t hfp_state:6;
	uint8_t sco_state:2;
	uint8_t active_call:1;
	uint8_t held_call:1;
	uint8_t incoming_call:1;
	uint8_t outgoing_call:1;
	uint8_t hfp_format;
	uint8_t hfp_sample_rate;
	int     btmgr_state;
} __packed;

enum {
	BTSRV_HFP_SYNC_ACK_NONE = 0,
	BTSRV_HFP_SYNC_ACK_CONNECTED = 1,
};

struct btsrv_sync_ack_hfp_info {
	bd_address_t addr;
	uint8_t ack_evt;		/* Event callback to bt manager */
} __packed;

#define SYNC_HFP_CBEV_DATA_MAX_LEN	36
struct btsrv_sync_hfp_cbev_data_info {
	bd_address_t addr;
	uint8_t callback_ev;
	uint8_t data_len;
	uint8_t buf[SYNC_HFP_CBEV_DATA_MAX_LEN];
} __packed;

struct btsrv_sync_spp_one {
	uint8_t spp_id;			/* spp_id from spp stack */
	uint8_t app_id;			/* app_id from spp manager */
	uint8_t connected:1;
	uint8_t active_connect:1;
	uint8_t conn_for_spp:1;		/* This spp info is for the conn, or not for the conn and disconnected  */
};

struct btsrv_sync_spp_info {
	bd_address_t addr;
	struct btsrv_sync_spp_one info[CFG_BT_MAX_SPP_CHANNEL];
} __packed;

/* For snoop tws sync info end */

/* btsrv_adapter.c */
int btsrv_adapter_init(btsrv_callback cb);
int btsrv_adapter_set_power_off(void);
void btsrv_get_local_mac(bd_address_t *addr);
int btsrv_adapter_update_pair_state(uint8_t pair_state);
int btsrv_adapter_process(struct app_msg *msg);
int btsrv_adapter_callback(btsrv_event_e event, void *param);
int btsrv_adapter_stop(void);
int btsrv_adapter_set_config_info(void *param);
void btsrv_adapter_set_clear_wake_lock(uint16_t wake_lock, uint8_t set);
bool btsrv_adapter_check_wake_lock_bit(uint16_t wake_lock);
int btsrv_adapter_get_wake_lock(void);
int btsrv_adapter_get_link_idle(void);
int btsrv_adapter_start_discover(struct btsrv_discover_param *param);
int btsrv_adapter_stop_discover(void);
int btsrv_adapter_connect(bd_address_t *addr);
int btsrv_adapter_check_cancal_connect(bd_address_t *addr);
int btsrv_adapter_do_conn_disconnect(struct bt_conn *conn, uint8_t reason);
int btsrv_adapter_disconnect(struct bt_conn *conn);
int btsrv_adapter_set_discoverable(bool enable);
int btsrv_adapter_set_connnectable(bool enable);
void btsrv_adapter_dump_info(void);
uint8_t btsrv_adapter_get_pair_state(void);
void btsrv_adapter_set_tws_search_state(bool enable);
void btsrv_adapter_set_reconnect_state(bool enable);
void btsrv_adapter_set_wait_connect_state(bool enable, uint32_t duration_ms);
void btsrv_adapter_set_pair_mode_state(bool enable, uint32_t duration_ms);
int btsrv_adapter_get_type_by_handle(uint16_t handle);
int btsrv_adapter_get_role(uint16_t handle);
int btsrv_adapter_leaudio_is_connected(void);
int btsrv_adapter_set_leaudio_foreground_dev(bool enable);
int btsrv_adapter_leaudio_is_foreground_dev(void);
int32_t  btsrv_is_remote_connnect_req(bt_addr_t *addr);

struct btsrv_info_t *btsrv_adapter_get_context(void);

/* btsrv_a2dp.c */
enum {
	A2DP_PRIOROTY_FIRST_USED = (0x1 << 0),		/* For first open first used a2dp or preempt a2dp */
	A2DP_PRIOROTY_USER_START = (0x1 << 1), 		/* For user suspend play and switch to second phone */
	A2DP_PRIOROTY_AVRCP_PLAY = (0x1 << 2),		/* avrcp playing state */
	A2DP_PRIOROTY_STREAM_OPEN = (0x1 << 3),		/* A2dp stream open or close */
	A2DP_PRIOROTY_STOP_WAIT = (0x1 << 4),		/* For stop a2dp_status_stopped_delay_ms */
	A2DP_PRIOROTY_CALL_WAIT = (0x1 << 5),		/* For call break a2dp */
};

#define A2DP_PRIOROTY_ALL	(A2DP_PRIOROTY_FIRST_USED | A2DP_PRIOROTY_USER_START | A2DP_PRIOROTY_AVRCP_PLAY |  \
								A2DP_PRIOROTY_STREAM_OPEN | A2DP_PRIOROTY_STOP_WAIT | A2DP_PRIOROTY_CALL_WAIT)

bool btsrv_a2dp_is_stream_start(void);
void btsrv_a2dp_user_callback(struct bt_conn *conn, btsrv_a2dp_event_e event,  void *param, int param_size);
void btsrv_a2dp_start_stop_check_prio_timer(uint8_t start);

int btsrv_a2dp_process(struct app_msg *msg);
int btsrv_a2dp_deinit(void);
int btsrv_a2dp_disconnect(struct bt_conn *conn);
int btsrv_a2dp_connect(struct bt_conn *conn, uint8_t role);
void btsrv_a2dp_halt_aac_endpoint(bool halt);
int btsrv_a2dp_media_state_change(struct bt_conn *conn, uint8_t state);
int btsrv_a2dp_media_parser_frame_info(uint8_t codec_id, uint8_t *data, uint32_t data_len, uint16_t *frame_cnt, uint16_t *frame_len);
uint32_t btsrv_a2dp_media_cal_frame_time_us(uint8_t codec_id, uint8_t *data);
uint16_t btsrv_a2dp_media_cal_frame_samples(uint8_t codec_id, uint8_t *data);
uint8_t btsrv_a2dp_media_get_samples_rate(uint8_t codec_id, uint8_t *data);
uint8_t *btsrv_a2dp_media_get_zero_frame(uint8_t codec_id, uint16_t *len, uint8_t sample_rate);

struct btsrv_rdm_avrcp_pass_info {
	uint8_t pass_state;
	uint8_t op_id;
	uint32_t op_time;
};

/* btsrv_avrcp.c */
void btsrv_avrcp_user_callback_ev(struct bt_conn *conn, btsrv_avrcp_event_e event, void *param);
int btsrv_avrcp_process(struct app_msg *msg);
int btsrv_avrcp_init(btsrv_avrcp_callback_t *cb);
int btsrv_avrcp_deinit(void);
int btsrv_avrcp_disconnect(struct bt_conn *conn);
int btsrv_avrcp_connect(struct bt_conn *conn);

typedef enum {
	BTSRV_SCO_STATE_INIT,
	BTSRV_SCO_STATE_PHONE,		/* In call state, sco not connected */
	BTSRV_SCO_STATE_HFP,		/* In call state, sco connected */
	BTSRV_SCO_STATE_DISCONNECT,
} btsrv_sco_state;

typedef enum {
	BTSRV_HFP_ROLE_HF =0,
	BTSRV_HFP_ROLE_AG,
} btsrv_hfp_role;

/* btsrv_hfp.c */
int btsrv_hfp_user_callback(struct bt_conn *conn, btsrv_hfp_event_e event,  uint8_t *param, int param_size);
int btsrv_hfp_event_from_tws_cb(struct bt_conn *conn, btsrv_hfp_event_e event);

int btsrv_hfp_process(struct app_msg *msg);
int btsrv_hfp_init(btsrv_hfp_callback cb);
int btsrv_hfp_deinit(void);
int btsrv_hfp_disconnect(struct bt_conn *conn);
int btsrv_hfp_connect(struct bt_conn *conn);
int btsrv_hfp_proc_event(struct bt_conn *conn, uint8_t event);
int btsrv_hfp_get_call_state(uint8_t active_call);
int btsrv_hfp_get_dev_num(void);

/* btsrv_sco.c */
int btsrv_sco_process(struct app_msg *msg);
void btsrv_sco_acl_disconnect_clear_sco_conn(struct bt_conn *sco_conn);
void btsrv_sco_disconnect(struct bt_conn *sco_conn);
struct bt_conn *btsrv_sco_get_conn(void);
void btsrv_sco_set_first_pkt_clk(struct bt_conn *sco_conn, uint32_t clock);
uint32_t btsrv_sco_get_first_pkt_clk(struct bt_conn *sco_conn);
void btsrv_soc_hfp_active_dev_change(struct bt_conn *conn,struct bt_conn *last_conn);

/* btsrv_hfp_ag.c */
int btsrv_hfp_ag_process(struct app_msg *msg);
int btsrv_hfp_ag_connect(struct bt_conn *conn);
int btsrv_hfp_ag_proc_event(struct bt_conn *conn, uint8_t event);

/* btsrv_spp.c */
int btsrv_spp_send_data(uint8_t app_id, uint8_t *data, uint32_t len);
int btsrv_spp_process(struct app_msg *msg);
int btsrv_spp_sync_get_info(struct bt_conn *conn, void *info);
int btsrv_spp_sync_set_info(struct bt_conn *conn, void *info);
void btsrv_spp_dump_info(void);
void btsrv_spp_snoop_link_disconnected_clear(struct bt_conn *conn);

/* btsrv_pbap.c */
int btsrv_pbap_process(struct app_msg *msg);
int btsrv_map_process(struct app_msg *msg);

/* btsrv_hid.c */
void btsrv_hid_connect(struct bt_conn *conn);
int btsrv_hid_process(struct app_msg *msg);

/* btsrv_rdm.c */
bool btsrv_rdm_need_high_performance(void);
struct bt_conn *btsrv_rdm_find_conn_by_addr(bd_address_t *addr);
struct bt_conn *btsrv_rdm_find_conn_by_hdl(uint16_t hdl);
struct bt_conn *btsrv_rdm_sync_find_conn_by_addr(bd_address_t *addr);
void *btsrv_rdm_get_addr_by_conn(struct bt_conn *base_conn);
int btsrv_rdm_get_connected_dev(rdm_connected_dev_cb cb, void *cb_param);
int btsrv_rdm_get_dev_state(struct bt_dev_rdm_state *state);
int btsrv_rdm_get_connected_dev_cnt_by_type(int type);
int btsrv_rdm_get_autoconn_dev(struct autoconn_info *info, int max_dev);
int btsrv_rdm_base_disconnected(struct bt_conn *base_conn);
int btsrv_rdm_add_dev(struct bt_conn *base_conn);
int btsrv_rdm_remove_dev(uint8_t *mac);
void btsrv_rdm_set_security_changed(struct bt_conn *base_conn);
bool btsrv_rdm_is_security_changed(struct bt_conn *base_conn);
bool btsrv_rdm_is_connected(struct bt_conn *base_conn);

bool btsrv_rdm_is_a2dp_connected(struct bt_conn *base_conn);
bool btsrv_rdm_is_avrcp_connected(struct bt_conn *base_conn);
bool btsrv_rdm_is_hfp_connected(struct bt_conn *base_conn);
bool btsrv_rdm_is_spp_connected(struct bt_conn *base_conn);
bool btsrv_rdm_is_hid_connected(struct bt_conn *base_conn);
int btsrv_rdm_set_a2dp_connected(struct bt_conn *base_conn, bool connected);
void btsrv_rdm_a2dp_set_clear_priority(struct bt_conn *base_conn, uint8_t prio, uint8_t set);
uint8_t btsrv_rdm_a2dp_get_priority(struct bt_conn *base_conn);
void btsrv_rdm_a2dp_set_start_stop_time(struct bt_conn *base_conn, uint32_t time, uint8_t start);
uint32_t btsrv_rdm_a2dp_get_start_stop_time(struct bt_conn *base_conn, uint8_t start);
void btsrv_rdm_a2dp_set_active(struct bt_conn *base_conn, uint8_t set);
uint8_t btsrv_rdm_a2dp_get_active(struct bt_conn *base_conn);
struct bt_conn *btsrv_rdm_a2dp_get_active_dev(void);
struct bt_conn *btsrv_rdm_a2dp_get_second_dev(void);
int btsrv_rdm_is_a2dp_stream_open(struct bt_conn *base_conn);
int btsrv_rdm_is_a2dp_stop_wait(struct bt_conn *base_conn);

int btsrv_rdm_a2dp_set_codec_info(struct bt_conn *base_conn, uint8_t format, uint8_t sample_rate, uint8_t cp_type);
int btsrv_rdm_a2dp_get_codec_info(struct bt_conn *base_conn, uint8_t *format, uint8_t *sample_rate, uint8_t *cp_type);
uint16_t btsrv_rdm_get_a2dp_acitve_hdl(void);
int btsrv_rdm_set_a2dp_pending_ahead_start(struct bt_conn *base_conn, uint8_t start);
uint8_t btsrv_rdm_get_a2dp_pending_ahead_start(struct bt_conn *base_conn);

int btsrv_rdm_set_avrcp_connected(struct bt_conn *base_conn, bool connected);
struct bt_conn *btsrv_rdm_avrcp_get_connected_dev(void);
void btsrv_rdm_set_avrcp_sync_vol_start_time(struct bt_conn *base_conn, uint32_t start_time);
uint32_t btsrv_rdm_get_avrcp_sync_vol_start_time(struct bt_conn *base_conn);
void btsrv_rdm_set_avrcp_sync_volume_wait(struct bt_conn *base_conn, uint8_t set);
uint8_t btsrv_rdm_get_avrcp_sync_volume_wait(struct bt_conn *base_conn);
void *btsrv_rdm_avrcp_get_pass_info(struct bt_conn *base_conn);

int btsrv_rdm_set_hfp_connected(struct bt_conn *base_conn, bool connected);
int btsrv_rdm_set_hfp_role(struct bt_conn *base_conn, uint8_t role);
int btsrv_rdm_get_hfp_role(struct bt_conn *base_conn);
int btsrv_rdm_hfp_actived(struct bt_conn *base_conn, uint8_t actived, uint8_t force);
struct bt_conn *btsrv_rdm_hfp_get_actived_sco(void);
int btsrv_rdm_hfp_set_codec_info(struct bt_conn *base_conn, uint8_t format, uint8_t sample_rate);
int btsrv_rdm_hfp_get_codec_info(struct bt_conn *base_conn, uint8_t *format, uint8_t *sample_rate);
int btsrv_rdm_hfp_set_state(struct bt_conn *base_conn, uint8_t state);
int btsrv_rdm_hfp_get_state(struct bt_conn *base_conn);
int btsrv_rdm_hfp_set_siri_state(struct bt_conn *base_conn, uint8_t state);
int btsrv_rdm_hfp_get_siri_state(struct bt_conn *base_conn);
int btsrv_rdm_hfp_set_sco_state(struct bt_conn *base_conn, uint8_t state);
int btsrv_rdm_hfp_get_sco_state(struct bt_conn *base_conn);
int btsrv_rdm_hfp_set_call_state(struct bt_conn *base_conn, uint8_t active, uint8_t held, uint8_t in, uint8_t out);
int btsrv_rdm_hfp_get_call_state(struct bt_conn *base_conn, uint8_t *active, uint8_t *held, uint8_t *in, uint8_t *out);
int btsrv_rdm_hfp_set_notify_phone_num_state(struct bt_conn *base_conn, uint8_t state);
int btsrv_rdm_hfp_get_notify_phone_num_state(struct bt_conn *base_conn);
struct bt_conn *btsrv_rdm_hfp_get_actived_dev(void);
struct bt_conn *btsrv_rdm_hfp_get_second_dev(void);
uint16_t btsrv_rdm_get_hfp_acitve_hdl(void);
bool btsrv_rdm_hfp_in_call_state(struct bt_conn *base_conn);
struct bt_conn *btsrv_rdm_get_base_conn_by_sco(struct bt_conn *sco_conn);
struct bt_conn *btsrv_rdm_get_sco_by_base_conn(struct bt_conn *base_conn);
int btsrv_rdm_set_sco_connected(struct bt_conn *base_conn, struct bt_conn *sco_conn, uint8_t connected);
int btsrv_rdm_set_spp_connected(struct bt_conn *base_conn, bool connected);
int btsrv_rdm_set_pbap_connected(struct bt_conn *base_conn, bool connected);
int btsrv_rdm_set_map_connected(struct bt_conn *base_conn, bool connected);
int btsrv_rdm_set_hid_connected(struct bt_conn *base_conn, bool connected);
struct bt_conn *btsrv_rdm_hid_get_actived(void);
int btsrv_rdm_set_tws_role(struct bt_conn *base_conn, uint8_t role);
struct bt_conn *btsrv_rdm_get_tws_by_role(uint8_t role);
int btsrv_rdm_get_tws_role(struct bt_conn *base_conn);
int btsrv_rdm_get_dev_role(void);
struct bt_conn *btsrv_rdm_get_actived_phone(void);
uint16_t btsrv_rdm_get_actived_phone_hdl(void);
struct bt_conn *btsrv_rdm_get_nonactived_phone(void);
int btsrv_rdm_set_snoop_role(struct bt_conn *base_conn, uint8_t role);
int btsrv_rdm_get_snoop_role(struct bt_conn *base_conn);
int btsrv_rdm_set_snoop_mode(struct bt_conn *base_conn, uint8_t mode);
int btsrv_rdm_get_snoop_mode(struct bt_conn *base_conn);
int btsrv_rdm_set_snoop_switch_lock(struct bt_conn *base_conn, uint8_t lock);
int btsrv_rdm_get_snoop_switch_lock(struct bt_conn *base_conn);
int btsrv_rdm_set_controler_role(struct bt_conn *base_conn, uint8_t role);
int btsrv_rdm_get_controler_role(struct bt_conn *base_conn);
int btsrv_rdm_set_link_time(struct bt_conn *base_conn, uint16_t link_time);
uint16_t btsrv_rdm_get_link_time(struct bt_conn *base_conn);
void btsrv_rdm_set_dev_name(struct bt_conn *base_conn, uint8_t *name);
uint8_t *btsrv_rdm_get_dev_name(struct bt_conn *base_conn);
void btsrv_rdm_set_wait_to_diconnect(struct bt_conn *base_conn, bool set);
bool btsrv_rdm_is_wait_to_diconnect(struct bt_conn *base_conn);
void btsrv_rdm_set_switch_sbc_state(struct bt_conn *base_conn, uint8_t state);
uint8_t btsrv_rdm_get_switch_sbc_state(struct bt_conn *base_conn);
void btsrv_rdm_set_a2dp_meida_rx_cid(struct bt_conn *base_conn, uint16_t cid);
int btsrv_rdm_sync_base_info(struct bt_conn *base_conn, void *local_info, void *remote_info);
int btsrv_rdm_sync_paired_list_info(void *local_info, void *remote_info, uint8_t max_cnt);
int btsrv_rdm_sync_a2dp_info(struct bt_conn *base_conn, void *local_info, void *remote_info);
int btsrv_rdm_sync_avrcp_info(struct bt_conn *base_conn, void *local_info, void *remote_info);
int btsrv_rdm_sync_hfp_info(struct bt_conn *base_conn, void *local_info, void *remote_info);
void *btsrv_rdm_get_sniff_info(struct bt_conn *base_conn);
int btsrv_rdm_get_connecting_dev(void);
int btsrv_rdm_init(void);
void btsrv_rdm_deinit(void);
void btsrv_rdm_dump_info(void);
uint32_t btsrv_rdm_get_sco_creat_time(struct bt_conn *base_conn);
void btsrv_rdm_set_sco_disconnect_wait(struct bt_conn *base_conn, uint8_t set);
uint8_t btsrv_rdm_get_sco_disconnect_wait(struct bt_conn *base_conn);
int btsrv_rdm_set_avrcp_state(struct bt_conn *base_conn, uint8_t state);
/*return 0:pause, 1:play*/
int btsrv_rdm_get_avrcp_state(struct bt_conn *base_conn);

/* btsrv_connect.c */
void btsrv_autoconn_info_update(void);
void btsrv_notify_link_event(struct bt_conn *base_conn, uint8_t event, uint8_t param);
void btsrv_proc_link_change(uint8_t *mac, uint8_t type,uint8_t err);
int btsrv_connect_get_auto_reconnect_info(struct autoconn_info *info, uint8_t max_cnt);
void btsrv_sync_remote_paired_list_info(struct autoconn_info *info, uint8_t max_cnt);
void btsrv_connect_set_auto_reconnect_info(struct autoconn_info *info, uint8_t max_cnt,uint8_t mode);
struct autoconn_info * btsrv_connect_get_tws_paired_info(void);
struct autoconn_info *  btsrv_connect_get_phone_paired_info(void);
void btsrv_connect_set_phone_controler_role(bd_address_t *bd, uint8_t role);
int btsrv_connect_process(struct app_msg *msg);
bool btsrv_connect_is_reconnect_info_empty(void);
bool btsrv_autoconn_is_reconnecting(void);
bool btsrv_autoconn_is_nodiscoverable(void);
bool btsrv_autoconn_is_runing(void);
bool btsrv_autoconn_is_phone_connecting(void);
bool btsrv_autoconn_is_phone_first_reconnect(void);
bool btsrv_autoconn_is_tws_pair_first(uint8_t role);
void btsrv_connect_tws_role_confirm(void);
bool btsrv_connect_check_share_tws(void);
int btsrv_connect_init(void);
void btsrv_connect_deinit(void);
void btsrv_connect_dump_info(void);
void btsrv_connect_auto_connection_stop(int mode);
void btsrv_connect_auto_connection_stop_device(bd_address_t *addr);
bool btsrv_connect_is_pending(void);

/* btsrv_scan.c */
int btsrv_scan_set_param(struct bt_scan_parameter *param);
uint8_t btsrv_scan_get_visual(void);
uint8_t btsrv_scan_get_mode(void);
void btsrv_scan_set_user_discoverable(bool enable, bool immediate);
void btsrv_scan_set_user_connectable(bool enable, bool immediate);
void btsrv_scan_set_user_visual(struct bt_set_user_visual *param);
void btsrv_inner_set_service_visual(struct bt_set_user_visual *param);
void btsrv_scan_update_mode(bool immediate);
uint8_t btsrv_scan_get_inquiry_mode(void);
int btsrv_scan_init(void);
void btsrv_scan_deinit(void);
void btsrv_scan_dump_info(void);

/* btsrv_link_adjust.c */
int btsrv_link_adjust_set_tws_state(uint8_t adjust_state, uint16_t buff_size, uint16_t source_cache, uint16_t cache_sink);
int btsrv_link_adjust_tws_set_bt_play(bool bt_play);
void btsrv_link_adjust_quickly(void);
int btsrv_link_adjust_init(void);
void btsrv_link_adjust_deinit(void);

/* btsrv_sniff.c */
void btsrv_sniff_update_idle_time(struct bt_conn *conn);
void btsrv_sniff_mode_change(void *param);
bool btsrv_sniff_in_sniff_mode(struct bt_conn *conn);
bool btsrv_sniff_in_entering_sniff(struct bt_conn *conn);
void btsrv_sniff_init(void);
void btsrv_sniff_deinit(void);

/* btsrv_link_utilis.c */
bd_address_t *GET_CONN_BT_ADDR(struct bt_conn *conn);
bool btsrv_is_snoop_conn(struct bt_conn *conn);
int btsrv_set_negative_prio(void);
void btsrv_revert_prio(int prio);
int btsrv_property_set(const char *key, char *value, int value_len);
int btsrv_property_set_factory(const char *key, char *value, int value_len);
int btsrv_property_get(const char *key, char *value, int value_len);
void ctrl_adjust_link_time(struct bt_conn *base_conn, int16_t adjust_val);


#define BTSTAK_READY 0

enum {
	MSG_BTSRV_BASE = MSG_APP_MESSAGE_START,
	MSG_BTSRV_CONNECT,
	MSG_BTSRV_A2DP,
	MSG_BTSRV_AVRCP,
	MSG_BTSRV_HFP,
	MSG_BTSRV_HFP_AG,
	MSG_BTSRV_SCO,
	MSG_BTSRV_SPP,
	MSG_BTSRV_PBAP,
	MSG_BTSRV_HID,
	MSG_BTSRV_TWS,
	MSG_BTSRV_MAP,
	MSG_BTSRV_AUDIO,	/* LE Audio */
#ifdef CONFIG_BT_TRANSCEIVER
    MSG_BTSRV_TRS,
#endif
	MSG_BTSRV_MAX,
};

enum {
	MSG_BTSRV_UPDATE_SCAN_MODE,
    MSG_BTSRV_SET_USER_VISUAL,
	MSG_BTSRV_AUTO_RECONNECT,
	MSG_BTSRV_AUTO_RECONNECT_STOP,
	MSG_BTSRV_AUTO_RECONNECT_STOP_DEVICE,
	MSG_BTSRV_AUTO_RECONNECT_COMPLETE,
	MSG_BTSRV_CONNECT_TO,
	MSG_BTSRV_DISCONNECT,
	MSG_BTSRV_READY,
	MSG_BTSRV_REQ_FLUSH_NVRAM,
	MSG_BTSRV_CONNECTED,
	MSG_BTSRV_CONNECTED_FAILED,
	MSG_BTSRV_SECURITY_CHANGED,
	MSG_BTSRV_ROLE_CHANGE,
	MSG_BTSRV_MODE_CHANGE,
	MSG_BTSRV_DISCONNECTED,
	MSG_BTSRV_DISCONNECTED_REASON,
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
	MSG_BTSRV_SET_DISCOVERABLE,
	MSG_BTSRV_SET_CONNECTABLE,
#endif
	MSG_BTSRV_GET_NAME_FINISH,
	MSG_BTSRV_CLEAR_LIST_CMD,
	MGS_BTSRV_CLEAR_AUTO_INFO,
	MSG_BTSRV_DISCONNECT_DEVICE,
	MSG_BTSRV_ALLOW_SCO_CONNECT,
    MSG_BTSRV_CLEAR_SHARE_TWS,
	MSG_BTSRV_DISCONNECT_DEV_BY_PRIORITY,
	MSG_BTSRV_CONNREQ_ADD_MONITOR,
	MSG_BTSRV_SET_LEAUDIO_CONNECTED,
	MSG_BTSRV_REMOTE_LINKKEY_MISS,
	
	MSG_BTSRV_A2DP_START,
	MSG_BTSRV_A2DP_STOP,
	MSG_BTSRV_A2DP_CHECK_STATE,
	MSG_BTSRV_A2DP_USER_PAUSE,
	MSG_BTSRV_A2DP_CONNECT_TO,
	MSG_BTSRV_A2DP_DISCONNECT,
	MSG_BTSRV_A2DP_CONNECTED,
	MSG_BTSRV_A2DP_DISCONNECTED,
	MSG_BTSRV_A2DP_MEDIA_STATE_CB,
	MSG_BTSRV_A2DP_SET_CODEC_CB,
	MSG_BTSRV_A2DP_SEND_DELAY_REPORT,
	MSG_BTSRV_A2DP_CHECK_SWITCH_SBC,
	MSG_BTSRV_A2DP_UPDATE_AVRCP_PLAYSTATE,
	MSG_BTSRV_A2DP_STREAM_START,
	MSG_BTSRV_A2DP_STREAM_SUSPEND,	

	MSG_BTSRV_AVRCP_START,
	MSG_BTSRV_AVRCP_STOP,
	MSG_BTSRV_AVRCP_CONNECT_TO,
	MSG_BTSRV_AVRCP_DISCONNECT,
	MSG_BTSRV_AVRCP_CONNECTED,
	MSG_BTSRV_AVRCP_DISCONNECTED,
	MSG_BTSRV_AVRCP_NOTIFY_CB,
	MSG_BTSRV_AVRCP_PASS_CTRL_CB,
	MSG_BTSRV_AVRCP_SEND_CMD,
	MSG_BTSRV_AVRCP_SYNC_VOLUME,
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
	MSG_BTSRV_AVRCP_PLAYBACK_TRACK_CHANGE,
#endif
	MSG_BTSRV_AVRCP_GET_ID3_INFO,
	MSG_BTSRV_AVRCP_SET_ABSOLUTE_VOLUME,

	MSG_BTSRV_HFP_START,
	MSG_BTSRV_HFP_STOP,
	MSG_BTSRV_HFP_CONNECTED,
	MSG_BTSRV_HFP_DISCONNECTED,
	MSG_BTSRV_HFP_STACK_EVENT,
	MSG_BTSRV_HFP_VOLUME_CHANGE,
	MSG_BTSRV_HFP_PHONE_NUM,
	MSG_BTSRV_HFP_CODEC_INFO,
	MSG_BTSRV_HFP_SIRI_STATE,
	MSG_BTSRV_SCO_START,
	MSG_BTSRV_SCO_STOP,
	MSG_BTSRV_SCO_DISCONNECT,
	MSG_BTSRV_SCO_CONNECTED,
	MSG_BTSRV_SCO_DISCONNECTED,
	MSG_BTSRV_CREATE_SCO_SNOOP,

	MSG_BTSRV_HFP_SWITCH_SOUND_SOURCE,
	MSG_BTSRV_HFP_HF_DIAL_NUM,
	MSG_BTSRV_HFP_HF_DIAL_LAST_NUM,
	MSG_BTSRV_HFP_HF_DIAL_MEMORY,
	MSG_BTSRV_HFP_HF_VOLUME_CONTROL,
	MSG_BTSRV_HFP_HF_ACCEPT_CALL,
	MSG_BTSRV_HFP_HF_BATTERY_REPORT,
	MSG_BTSRV_HFP_HF_REJECT_CALL,
	MSG_BTSRV_HFP_HF_HANGUP_CALL,
	MSG_BTSRV_HFP_HF_HANGUP_ANOTHER_CALL,
	MSG_BTSRV_HFP_HF_HOLDCUR_ANSWER_CALL,
	MSG_BTSRV_HFP_HF_HANGUPCUR_ANSWER_CALL,
	MSG_BTSRV_HFP_HF_VOICE_RECOGNITION_START,
	MSG_BTSRV_HFP_HF_VOICE_RECOGNITION_STOP,
	MSG_BTSRV_HFP_HF_VOICE_SEND_AT_COMMAND,
	MSG_BTSRV_HFP_ACTIVED_DEV_CHANGED,
	MSG_BTSRV_HFP_GET_TIME,
	MSG_BTSRV_HFP_TIME_UPDATE,
	MSG_BTSRV_HFP_EVENT_FROM_TWS,
	
	MSG_BTSRV_HFP_AG_START,
	MSG_BTSRV_HFP_AG_STOP,
	MSG_BTSRV_HFP_AG_CONNECTED,
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
	MSG_BTSRV_HFP_AG_CONNECTE_FAILED,
#endif
	MSG_BTSRV_HFP_AG_DISCONNECTED,
	MSG_BTSRV_HFP_AG_UPDATE_INDICATOR,
	MSG_BTSRV_HFP_AG_SEND_EVENT,
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
    MSG_BTSRV_HFP_AG_ATA,
	MSG_BTSRV_HFP_AG_CHUP,
	MSG_BTSRV_HFP_AG_ATD,
	MSG_BTSRV_HFP_AG_BLDN,
	MSG_BTSRV_HFP_AG_CLCC,
	MSG_BTSRV_HFP_AG_CHLD,
#endif
	MSG_BTSRV_SPP_START,
	MSG_BTSRV_SPP_STOP,
	MSG_BTSRV_SPP_REGISTER,
	MSG_BTSRV_SPP_CONNECT,
	MSG_BTSRV_SPP_DISCONNECT,
	MSG_BTSRV_SPP_CONNECT_FAILED,
	MSG_BTSRV_SPP_CONNECTED,
	MSG_BTSRV_SPP_DISCONNECTED,

	MSG_BTSRV_PBAP_CONNECT_FAILED,
	MSG_BTSRV_PBAP_CONNECTED,
	MSG_BTSRV_PBAP_DISCONNECTED,
	MSG_BTSRV_PBAP_GET_PB,
	MSG_BTSRV_PBAP_ABORT_GET,

	MSG_BTSRV_HID_START,
	MSG_BTSRV_HID_STOP,
	MSG_BTSRV_HID_CONNECTED,
	MSG_BTSRV_HID_DISCONNECTED,
	MSG_BTSRV_HID_EVENT_CB,
	MSG_BTSRV_HID_REGISTER,
	MSG_BTSRV_HID_CONNECT,
	MSG_BTSRV_HID_DISCONNECT,
	MSG_BTSRV_HID_SEND_CTRL_DATA,
	MSG_BTSRV_HID_SEND_INTR_DATA,
	MSG_BTSRV_HID_SEND_RSP,
	MSG_BTSRV_HID_UNPLUG,
	MSG_BTSRV_DID_REGISTER,

	MSG_BTSRV_TWS_INIT,
	MSG_BTSRV_TWS_DEINIT,
	MSG_BTSRV_TWS_VERSION_NEGOTIATE,
	MSG_BTSRV_TWS_ROLE_NEGOTIATE,
	MSG_BTSRV_TWS_NEGOTIATE_SET_ROLE,
	MSG_BTSRV_TWS_CONNECTED,
	MSG_BTSRV_TWS_DISCONNECTED,
	MSG_BTSRV_TWS_DISCONNECTED_ADDR,
	MSG_BTSRV_TWS_START_PAIR,
	MSG_BTSRV_TWS_END_PAIR,
	MSG_BTSRV_TWS_DISCOVERY_RESULT,
	MSG_BTSRV_TWS_DISCONNECT,
	MSG_BTSRV_TWS_RESTART,
	MSG_BTSRV_TWS_PROTOCOL_DATA,
	MSG_BTSRV_TWS_EVENT_SYNC,
	MSG_BTSRV_TWS_SCO_DATA,
	MSG_BTSRV_TWS_DO_SNOOP_CONNECT,
	MSG_BTSRV_TWS_DO_SNOOP_DISCONNECT,
	MSG_BTSRV_TWS_SNOOP_CONNECTED,
	MSG_BTSRV_TWS_SNOOP_DISCONNECTED,
	MSG_BTSRV_TWS_SNOOP_MODE_CHANGE,
	MSG_BTSRV_TWS_SNOOP_ACTIVE_LINK,
	MSG_BTSRV_TWS_SYNC_1ST_PKT_CHG,
	MSG_BTSRV_TWS_SWITCH_SNOOP_LINK,
	MSG_BTSRV_TWS_SNOOP_SWITCH_COMPLETE,
	MSG_BTSRV_TWS_UPDATE_VISUAL_PAIR,
	MSG_BTSRV_TWS_RSSI_RESULT,
	MSG_BTSRV_TWS_LINK_QUALITY_RESULT,
	MSG_BTSRV_TWS_PHONE_ACTIVE_CHANGE,
	MSG_BTSRV_TWS_SET_STATE,
	MSG_BTSRV_TWS_DISABLE_VISABLE_PAIR_MODE,

	MSG_BTSRV_MAP_CONNECT,
	MSG_BTSRV_MAP_DISCONNECT,
	MSG_BTSRV_MAP_SET_FOLDER,
	MSG_BTSRV_MAP_GET_FOLDERLISTING,
	MSG_BTSRV_MAP_GET_MESSAGESLISTING,
	MSG_BTSRV_MAP_GET_MESSAGE,
	MSG_BTSRV_MAP_ABORT_GET,
	MSG_BTSRV_MAP_CONNECT_FAILED,
	MSG_BTSRV_MAP_CONNECTED,
	MSG_BTSRV_MAP_DISCONNECTED,

	MSG_BTSRV_DUMP_INFO,

#ifdef CONFIG_BT_TRANSCEIVER
    MSG_BTSRV_TRS_START,
#endif
};

static inline btsrv_callback _btsrv_get_msg_param_callback(struct app_msg *msg)
{
	return (btsrv_callback)msg->ptr;
}

static inline int _btsrv_get_msg_param_type(struct app_msg *msg)
{
	return msg->type;
}

static inline int _btsrv_get_msg_param_cmd(struct app_msg *msg)
{
	return msg->cmd;
}

static inline int _btsrv_get_msg_param_reserve(struct app_msg *msg)
{
	return msg->reserve;
}

static inline void *_btsrv_get_msg_param_ptr(struct app_msg *msg)
{
	return msg->ptr;
}

static inline int _btsrv_get_msg_param_value(struct app_msg *msg)
{
	return msg->value;
}

/* btsrv_message.c */
int btsrv_function_call(int func_type, int cmd, void *param);
int btsrv_event_notify(int event_type, int cmd, void *param);
int btsrv_event_notify_value(int event_type, int cmd, int value);
int btsrv_event_notify_ext(int event_type, int cmd, void *param, uint8_t code);
int btsrv_event_notify_malloc(int event_type, int cmd, uint8_t *data, uint16_t len, uint8_t code);
void *btsrv_event_buf_malloc(uint16_t len);
int btsrv_event_notify_buf(int event_type, int cmd, uint8_t *data, uint8_t code);

#define btsrv_function_call_malloc 		btsrv_event_notify_malloc

typedef int (*btsrv_msg_process)(struct app_msg *msg);

/* btsrv_main.c */
int btsrv_register_msg_processer(uint8_t msg_type, btsrv_msg_process processer);
const char *bt_service_evt2str(int num, int max_num, const btsrv_event_strmap_t *strmap);

/* btsrv_storage.c */
int btsrv_storage_init(void);
int btsrv_storage_get_linkkey(struct bt_linkkey_info *info, uint8_t cnt);
int btsrv_storage_update_linkkey(struct bt_linkkey_info *info, uint8_t cnt);
int btsrv_storage_write_ori_linkkey(bd_address_t *addr, uint8_t *link_key);
void btsrv_storage_clean_linkkey(void);

/* btsrv_pts_test.c */
int btsrv_pts_send_hfp_cmd(char *cmd);
int btsrv_pts_hfp_active_connect_sco(void);
int btsrv_pts_avrcp_get_play_status(void);
int btsrv_pts_avrcp_pass_through_cmd(uint8_t opid);
int btsrv_pts_avrcp_notify_volume_change(uint8_t volume);
int btsrv_pts_avrcp_reg_notify_volume_change(void);
int btsrv_pts_register_auth_cb(bool reg_auth);
#ifdef CONFIG_BT_TRANSCEIVER
int btsrv_pts_hfp_active_disconnect_sco(void);
#endif


/*
 * LE Audio
 */

enum {
	MSG_BTSRV_AUDIO_START,
	MSG_BTSRV_AUDIO_STOP,
	MSG_BTSRV_AUDIO_EXIT,

	MSG_BTSRV_AUDIO_CONNECT,
	MSG_BTSRV_AUDIO_PREPARE,
	MSG_BTSRV_AUDIO_DISCONNECT,
	MSG_BTSRV_AUDIO_CIS_DISCONNECT,
	MSG_BTSRV_AUDIO_CIS_CONNECT,
	MSG_BTSRV_AUDIO_CIS_RECONNECT,
	MSG_BTSRV_AUDIO_PAUSE,
	MSG_BTSRV_AUDIO_RESUME,
	MSG_BTSRV_AUDIO_CORE_CALLBACK,

#ifdef CONFIG_BT_TRANSCEIVER
    MSG_BTSRV_AUDIO_START_EXT,
    MSG_BTSRV_AUDIO_STOP_EXT,

    MSG_BTSRV_AUDIO_AUTO_START,
    MSG_BTSRV_AUDIO_AUTO_STOP,

    MSG_BTSRV_AUDIO_DISCONNECT_REQ,
#endif
	/* function calls */
	MSG_BTSRV_AUDIO_SRV_CAP_INIT = 30,
	MSG_BTSRV_AUDIO_STREAM_CONFIG_CODEC,
	MSG_BTSRV_AUDIO_STREAM_PREFER_QOS,
	MSG_BTSRV_AUDIO_STREAM_CONFIG_QOS,
	MSG_BTSRV_AUDIO_STREAM_ENABLE,
	MSG_BTSRV_AUDIO_STREAM_DISBLE,
	MSG_BTSRV_AUDIO_STREAM_SET,	/* for sink stream */
	MSG_BTSRV_AUDIO_STREAM_START,
	MSG_BTSRV_AUDIO_STREAM_STOP,
	MSG_BTSRV_AUDIO_STREAM_UPDATE,
	MSG_BTSRV_AUDIO_STREAM_RELEASE,
	MSG_BTSRV_AUDIO_STREAM_DISCONNECT,
	MSG_BTSRV_AUDIO_STREAM_CB_REGISTER,
	MSG_BTSRV_AUDIO_STREAM_SOURCE_SET,	/* for source stream */
	MSG_BTSRV_AUDIO_STREAM_SYNC_FORWARD,
	MSG_BTSRV_AUDIO_STREAM_BANDWIDTH_FREE,
	MSG_BTSRV_AUDIO_STREAM_BANDWIDTH_ALLOC,
	MSG_BTSRV_AUDIO_STREAM_BIND_QOS,
	MSG_BTSRV_AUDIO_STREAM_RELEASED,

	MSG_BTSRV_AUDIO_VOLUME_UP = 60,
	MSG_BTSRV_AUDIO_VOLUME_DOWN,
	MSG_BTSRV_AUDIO_VOLUME_SET,
	MSG_BTSRV_AUDIO_VOLUME_MUTE,
	MSG_BTSRV_AUDIO_VOLUME_UNMUTE,
	MSG_BTSRV_AUDIO_VOLUME_UNMUTE_UP,
	MSG_BTSRV_AUDIO_VOLUME_UNMUTE_DOWN,
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
    MSG_BTSRV_AUDIO_VOLUME_INIT_SET,
#endif

	MSG_BTSRV_AUDIO_MIC_MUTE = 80,
	MSG_BTSRV_AUDIO_MIC_UNMUTE,
	MSG_BTSRV_AUDIO_MIC_MUTE_DISABLE,
	MSG_BTSRV_AUDIO_MIC_GAIN_SET,

	MSG_BTSRV_AUDIO_MEDIA_PLAY = 90,
	MSG_BTSRV_AUDIO_MEDIA_PAUSE,
	MSG_BTSRV_AUDIO_MEDIA_STOP,
	MSG_BTSRV_AUDIO_MEDIA_FAST_REWIND,
	MSG_BTSRV_AUDIO_MEDIA_FAST_FORWARD,
	MSG_BTSRV_AUDIO_MEDIA_PLAY_PREV,
	MSG_BTSRV_AUDIO_MEDIA_PLAY_NEXT,

	MSG_BTSRV_AUDIO_SET_PARAM = 130,

	MSG_BTSRV_AUDIO_VND_CB_REGISTER,

	MSG_BTSRV_AUDIO_KEYS_CLEAR,

	MSG_BTSRV_AUDIO_ADV_CB_REGISTER,

	MSG_BTSRV_AUDIO_CALL_ORIGINATE = 150,
	MSG_BTSRV_AUDIO_CALL_ACCEPT,
	MSG_BTSRV_AUDIO_CALL_HOLD,
	MSG_BTSRV_AUDIO_CALL_RETRIEVE,
	MSG_BTSRV_AUDIO_CALL_TERMINATE,

#ifdef CONFIG_BT_TRANSCEIVER
	MSG_BTSRV_AUDIO_CALL_REMOTE_ORIGINATE = 160,
	MSG_BTSRV_AUDIO_CALL_REMOTE_ALART,
	MSG_BTSRV_AUDIO_CALL_REMOTE_ACCEPT,
	MSG_BTSRV_AUDIO_CALL_REMOTE_HOLD,
	MSG_BTSRV_AUDIO_CALL_REMOTE_RETRIEVE,
	MSG_BTSRV_AUDIO_CALL_RETMOTE_TERMINATE,
#endif

#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
    MSG_BTSRV_AUDIO_CALL_PROVIDER_SET = 170,
    MSG_BTSRV_AUDIO_CALL_TECH_SET,
    MSG_BTSRV_AUDIO_CALL_STATUS_SET,

    MSG_BTSRV_AUDIO_AVAILABLE_CONTEXT = 180,
    MSG_BTSRV_AUDIO_SUPPORTED_CONTEXT,
#endif

};

/* types of MSG_BTSRV_AUDIO_SET_PARAM */
enum {
	BTSRV_AUDIO_ADV_PARAM = 1,
	BTSRV_AUDIO_SCAN_PARAM,
	BTSRV_AUDIO_CONN_CREATE_PARAM,
	BTSRV_AUDIO_CONN_PARAM,
	BTSRV_AUDIO_CONN_IDLE_PARAM,
};

int btsrv_audio_param_init(void *param, uint8_t type);

int btsrv_audio_process(struct app_msg *msg);

bool btsrv_audio_stopped(void);

int btsrv_audio_memory_configure(struct bt_audio_config config);

int btsrv_audio_memory_init(struct bt_audio_config config,
				btsrv_audio_callback cb, void *data, int size);
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_BQB
void btrv_ascs_ccid_set(uint8_t ccidcount);

#endif
int btsrv_audio_start_adv(void);

int btsrv_audio_stop_adv(void);

int btsrv_audio_exit(void);

io_stream_t btsrv_audio_source_stream_create(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t locations);

void *btsrv_ascs_bandwidth_alloc(struct bt_audio_qoss *qoss);

int btsrv_audio_conn_max_len(uint16_t handle);

ssize_t btsrv_audio_vnd_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, const void *buf,
				uint16_t len, uint16_t offset, uint8_t flags);

int btsrv_audio_vnd_send(uint16_t handle, uint8_t *buf, uint16_t len);

struct btsrv_audio_stream_op {
	uint8_t num_chans;
	uint8_t reserved[3];
	struct bt_audio_chan *chans[0];
};

struct btsrv_audio_stream_op_contexts {
	uint32_t contexts;
	uint8_t num_chans;
	uint8_t reserved[3];
	struct bt_audio_chan *chans[0];
};

struct btsrv_audio_stream_op_set {
	struct bt_audio_chan *chan;
	io_stream_t stream;
};

struct btsrv_audio_stream_op_set_loc {
	io_stream_t stream;
	uint32_t locations;
	uint8_t num_chans;
	uint8_t reserved[3];
	struct bt_audio_chan *chans[0];
};

struct btsrv_audio_vnd_cb {
	bt_audio_vnd_rx_cb rx_cb;
	uint16_t handle;
};

struct btsrv_audio_adv_cb {
	bt_audio_adv_cb start;
	bt_audio_adv_cb stop;
};

struct btsrv_audio_call_originate_op {
	uint16_t handle;
	uint8_t uri[BT_AUDIO_CALL_URI_LEN];
};

#ifdef CONFIG_BT_TRANSCEIVER
#include "tr_btsrv_inner.h"
#endif
#endif
