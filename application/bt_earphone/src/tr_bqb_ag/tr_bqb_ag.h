/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief
 */

#ifndef _TR_BQB_AG_APP_H_
#define _TR_BQB_AG_APP_H_

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "tr_bqb_ag"
#include <logging/sys_log.h>
#endif

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <bt_manager.h>
#include <volume_manager.h>
#include <media_player.h>
//#include <global_mem.h>
#include <audio_system.h>
#include <dvfs.h>
#include <thread_timer.h>
#include "btservice_api.h"
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

enum 
{
    // bt call key message 
	MSG_TR_BQB_AG_VOLUP = MSG_APP_MESSAGE_START,

	MSG_TR_BQB_AG_VOLDOWN,
	
	MSG_TR_BQB_AG_SWITCH_CALLOUT,
	
	MSG_TR_BQB_AG_SWITCH_MICMUTE,

	MSG_BT_HOLD_CURR_ANSWER_ANOTHER,

	MSG_BT_HANGUP_ANOTHER,

	MSG_BT_HANGUP_CURR_ANSER_ANOTHER,

	MSG_BT_HANGUP_CALL,

	MSG_BT_ACCEPT_CALL,	

	MSG_BT_REJECT_CALL,
};

struct tr_bqb_ag_app_t {
	media_player_t *player;
	u8_t playing  : 1;
	u8_t mic_mute : 1;
	u8_t need_resume_play:1;
	u8_t phonenum_played : 1;
	u8_t sco_established : 1;
	u8_t siri_mode : 1;
	u8_t hfp_ongoing : 1;
	u8_t upload_stream_outer : 1;
	u8_t hfp_3way_status;
	io_stream_t bt_stream;
	io_stream_t upload_stream;

};

void tr_bqb_ag_bt_event_proc(struct app_msg *msg);
void tr_bqb_ag_input_event_proc(struct app_msg *msg);
void tr_bqb_ag_tts_event_proc(struct app_msg *msg);
bool tr_bqb_ag_key_event_handle(int key_event, int event_stage);
void tr_bqb_ag_start_play(void);
void tr_bqb_ag_stop_play(void);

struct tr_bqb_ag_app_t *tr_bqb_ag_get_app(void);

int tr_bqb_ag_ring_start(u8_t *phone_num, u16_t phone_num_cnt);
void tr_bqb_ag_ring_stop(void);
int tr_bqb_ag_ring_manager_init(void);
int tr_bqb_ag_ring_manager_deinit(void);
void tr_bqb_ag_view_init(void);
void tr_bqb_ag_view_deinit(void);
#endif  // _TR_BQB_AG_APP_H_



