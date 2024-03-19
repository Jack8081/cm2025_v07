/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bt service trs version and feature
 * Not need direct include this head file, just need include <btservice_api.h>.
 * this head file include by btservice_api.h
 */

#ifndef _TR_BTSERVICE_TRS_API_H_
#define _TR_BTSERVICE_TRS_API_H_
#include <btservice_tws_api.h>

/** Callbacks to report Bluetooth service's trs events*/
typedef enum {
	/** notifying that trs ready*/
	BTSRV_WAIT_PAIR,
    /** notifying that trs inquiry result*/
	BTSRV_TRS_INQUIRY_RESULT,
	/** notifying that trs inquiry result complete*/
	BTSRV_TRS_INQUIRY_COMPLETE,
	/** notifying that trs connected */
	BTSRV_TRS_CONNECTED,
	/** notifying that trs disconnected */
	BTSRV_TRS_DISCONNECTED,
	/** notifying that trs cancel connected */
	BTSRV_TRS_CANCEL_CONNECTED,
	/** notifying that trs connected failed */
	BTSRV_TRS_CONNECTED_FAILED,
	/** notifying that trs connected stoped*/
	BTSRV_TRS_CONNECTED_STOPED,
	/** notifying that trs ready play */
	BTSRV_TRS_READY_PLAY,
	/** notifying that trs restart play*/
	BTSRV_TRS_RESTART_PLAY,
	/** notifying that trs event*/
	BTSRV_TRS_EVENT_SYNC,
	/** notifying that trs a2dp connected */
	BTSRV_TRS_A2DP_CONNECTED,
	/** notifying that trs a2dp disconnected */
	BTSRV_TRS_A2DP_DISCONNECTED,
	/** notifying that trs a2dp open*/
	BTSRV_TRS_A2DP_OPEN,
	/** notifying that trs a2dp start*/
	BTSRV_TRS_A2DP_START,
	/** notifying that trs a2dp suspend*/
	BTSRV_TRS_A2DP_SUSPEND,
	/** notifying that trs a2dp close*/
	BTSRV_TRS_A2DP_CLOSE,
    /** notifying trs irq trigger, be carefull, call back in irq context  */
	BTSRV_TRS_IRQ_CB,
	/** notifying still have pending start play after disconnected */
	BTSRV_TRS_UNPROC_PENDING_START_CB,
	/** notifying slave btplay mode */
	BTSRV_TRS_UPDATE_BTPLAY_MODE,
	/** notifying peer version */
	BTSRV_TRS_UPDATE_PEER_VERSION,
	/** notifying sink start play*/
	BTSRV_TRS_SINK_START_PLAY,
	/** notifying trs failed to do trs pair */
	BTSRV_TRS_PAIR_FAILED,
	/** notifying that trs avrcp connected */
	BTSRV_TRS_AVRCP_CONNECTED,
	/** notifying that trs avrcp disconnected */
	BTSRV_TRS_AVRCP_DISCONNECTED,

	BTSRV_TRS_AVRCP_SYNC_VOLUME,
	BTSRV_TRS_AVRCP_SYNC_VOLUME_ACCEPT,
	BTSRV_TRS_AVRCP_GET_ID3_INFO,
	BTSRV_TRS_AVRCP_PASSTHROUGH_CMD,

    /** notifying that trs ag Battery level*/
	BTSRV_TRS_BATTERY_LEVEL,
} btsrv_trs_event_e;

typedef struct{
	/** Remote device address */
	u8_t addr[6];
	/** RSSI from inquiry */
	s8_t rssi;
	/** Class of Device */
	u8_t cod[3];
	/** Remote device name */
	u8_t name[CONFIG_MAX_BT_NAME_LEN + 1];

}inquiry_info_t;

typedef void (*btsrv_trs_callback)(int event, void *param, int param_size);
typedef int (*media_trigger_start_cb)(void *handle);
typedef int (*media_is_ready_cb)(void *handle);
typedef u32_t (*media_get_samples_cnt_cb)(void *handle, u8_t *audio_mode);
typedef int (*media_get_error_state_cb)(void *handle);
typedef int (*media_get_aps_level_cb)(void *handle);

int tr_btif_trs_init(btsrv_trs_callback cb);
int tr_btif_trs_deinit(void);
int tr_btif_trs_inquiry(u8_t times, u8_t nums, u32_t filter_cod);
int tr_btif_trs_cancel_inquiry(void);
int tr_btif_trs_register_processer(void);

int tr_btif_del_pairedlist(int index);
int tr_btif_br_disconnect_device(u8_t mode, void *parm);
void tr_btif_a2dp_mute(u8_t enable);
#endif
