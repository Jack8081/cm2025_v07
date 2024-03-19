/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call status
 */
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <dvfs.h>
#include "tr_bqb_ag.h"
#include "ui_manager.h"
#include "tts_manager.h"
#include "audio_system.h"
#include "audio_policy.h"

static void _tr_bqb_ag_hfp_disconnected(void)
{
	SYS_LOG_INF(" enter\n");
}

static void _tr_bqb_ag_sco_connected(io_stream_t upload_stream)
{
	struct tr_bqb_ag_app_t *tr_bqb_ag = tr_bqb_ag_get_app();

	tr_bqb_ag->mic_mute = false;
	tr_bqb_ag->upload_stream = upload_stream;
	tr_bqb_ag->upload_stream_outer = upload_stream ? 1 : 0;
#ifdef CONFIG_TR_BQB_AG_FORCE_PLAY_PHONE_NUM
	if (tr_bqb_ag->phonenum_played) {
		tr_bqb_ag_start_play();
		tr_bqb_ag->playing = 1;
	}
#else
	tr_bqb_ag_start_play();
	tr_bqb_ag->playing = 1;
#endif
}

static void _tr_bqb_ag_sco_disconnected(void)
{
	struct tr_bqb_ag_app_t *tr_bqb_ag = tr_bqb_ag_get_app();

	tr_bqb_ag_stop_play();
	tr_bqb_ag->playing = 0;
}

static void _tr_bqb_ag_hfp_active_dev_changed(void)
{
	struct tr_bqb_ag_app_t *tr_bqb_ag = tr_bqb_ag_get_app();

	if (tr_bqb_ag->playing) {
		tr_bqb_ag_stop_play();
		tr_bqb_ag->playing = 0;
	}
}

void tr_bqb_ag_bt_event_proc(struct app_msg *msg)
{
	struct tr_bqb_ag_app_t *tr_bqb_ag = tr_bqb_ag_get_app();

	switch (msg->cmd) {
	case BT_CALL_RING_STATR_EVENT:
		if (!tr_bqb_ag->phonenum_played) {
			tr_bqb_ag_ring_start(msg->ptr, strlen(msg->ptr));
			tr_bqb_ag->phonenum_played = 1;
		}
		break;
	case BT_CALL_RING_STOP_EVENT:
		tr_bqb_ag_ring_stop();
	#ifdef CONFIG_TR_BQB_AG_FORCE_PLAY_PHONE_NUM
		if (tr_bqb_ag->sco_established) {
			tr_bqb_ag_start_play();
			tr_bqb_ag->playing = 1;
		}
	#endif
		break;

	case BT_HFP_ACTIVEDEV_CHANGE_EVENT:
	{
		_tr_bqb_ag_hfp_active_dev_changed();
		break;
	}

	case BT_HFP_CALL_OUTGOING:
	{
		tr_bqb_ag->phonenum_played = 1;
		break;
	}
	case BT_HFP_CALL_INCOMING:
	{
		break;
	}
	case BT_HFP_CALL_ONGOING:
	{
		tr_bqb_ag_ring_stop();
		break;
	}
	case BT_HFP_CALL_SIRI_MODE:
		tr_bqb_ag->phonenum_played = 1;
		SYS_LOG_INF("siri_mode %d\n", tr_bqb_ag->siri_mode);
		break;
	case BT_HFP_CALL_HUNGUP:
	{
		break;
	}

	case BT_HFP_DISCONNECTION_EVENT:
	{
        SYS_LOG_INF("BT_HFP_DISCONNECTION_EVENT\n");
	    tr_bqb_ag->need_resume_play = 0;
		_tr_bqb_ag_hfp_disconnected();
		break;
	}
	case BT_HFP_ESCO_ESTABLISHED_EVENT:
	{
        SYS_LOG_INF("BT_HFP_ESCO_ESTABLISHED_EVENT\n");
		io_stream_t upload_stream = NULL;

		if (msg->ptr)
			memcpy(&upload_stream, msg->ptr, sizeof(upload_stream));
		tr_bqb_ag->sco_established = 1;
		_tr_bqb_ag_sco_connected(upload_stream);
		break;
	}
	case BT_HFP_ESCO_RELEASED_EVENT:
	{
	    SYS_LOG_INF("BT_HFP_ESCO_RELEASED_EVENT\n");
	    _tr_bqb_ag_sco_disconnected();
		tr_bqb_ag->sco_established = 0;
		break;
	}
    case BT_HFP_CALL_ATA:
    {
        SYS_LOG_INF("BT_HFP_CALL_ATA\n");
        k_sleep(K_MSEC(2000));
        btif_pts_hfp_active_connect_sco();
        break;
    }
	/*case BT_RMT_VOL_SYNC_EVENT:
	{
		audio_system_set_stream_volume(AUDIO_STREAM_VOICE, msg->value);

		if (tr_bqb_ag && tr_bqb_ag->player) {
			media_player_set_volume(tr_bqb_ag->player, msg->value, msg->value);
		}
		break;
	}*/

	default:
		break;
	}
}


static void _tr_bqb_ag_key_func_switch_mic_mute(void)
{
	struct tr_bqb_ag_app_t *tr_bqb_ag = tr_bqb_ag_get_app();

	tr_bqb_ag->mic_mute ^= 1;

	audio_system_mute_microphone(tr_bqb_ag->mic_mute);

	if (tr_bqb_ag->mic_mute)
		sys_event_notify(SYS_EVENT_MIC_MUTE_ON);
	else
		sys_event_notify(SYS_EVENT_MIC_MUTE_OFF);
}

static void _tr_bqb_ag_key_func_volume_adjust(int updown)
{
	int volume = 0;
	struct tr_bqb_ag_app_t *tr_bqb_ag = tr_bqb_ag_get_app();

	if (updown) {
		volume = system_volume_up(AUDIO_STREAM_VOICE, 1);
	} else {
		volume = system_volume_down(AUDIO_STREAM_VOICE, 1);
	}

	if (tr_bqb_ag->player) {
		media_player_set_volume(tr_bqb_ag->player, volume, volume);
	}
}

void tr_bqb_ag_input_event_proc(struct app_msg *msg)
{
	switch (msg->cmd) {
	case MSG_TR_BQB_AG_VOLUP:
	{
		_tr_bqb_ag_key_func_volume_adjust(1);
		break;
	}
	case MSG_TR_BQB_AG_VOLDOWN:
	{
		_tr_bqb_ag_key_func_volume_adjust(0);
		break;
	}
	case MSG_BT_ACCEPT_CALL:
	{
	    bt_manager_hfp_accept_call();
		break;
	}
	case MSG_BT_REJECT_CALL:
	{
	    bt_manager_hfp_reject_call();
		break;
	}
	case MSG_BT_HANGUP_CALL:
	{
	    bt_manager_hfp_hangup_call();
		break;
	}
	case MSG_BT_HANGUP_ANOTHER:
	{
		bt_manager_hfp_hangup_another_call();
	break;
	}
	case MSG_BT_HOLD_CURR_ANSWER_ANOTHER:
	{
		bt_manager_hfp_holdcur_answer_call();
		break;
	}
	case MSG_BT_HANGUP_CURR_ANSER_ANOTHER:
	{
		bt_manager_hfp_hangupcur_answer_call();
		break;
	}
	case MSG_TR_BQB_AG_SWITCH_CALLOUT:
	{
	#ifdef CONFIG_SEG_LED_MANAGER
		struct tr_bqb_ag_app_t *tr_bqb_ag = tr_bqb_ag_get_app();
		seg_led_display_string(SLED_NUMBER1, "C", true);
		if	(tr_bqb_ag->playing) {
			seg_led_display_string(SLED_NUMBER3, "AG", true);
		} else {
			seg_led_display_string(SLED_NUMBER3, "HF", true);
		}
	#endif
		bt_manager_hfp_switch_sound_source();
		break;
	}
	case MSG_TR_BQB_AG_SWITCH_MICMUTE:
	{
	    _tr_bqb_ag_key_func_switch_mic_mute();
		break;
	}
/*	case MSG_BT_CALL_LAST_NO:
	{
		bt_manager_hfp_dial_number(NULL);
		break;
	}
	case MSG_BT_SIRI_STOP:
	{
		bt_manager_hfp_stop_siri();
		break;
	}
	case MSG_BT_HID_START:
	{
		break;
	}*/
	}
}

void tr_bqb_ag_tts_event_proc(struct app_msg *msg)
{
	struct tr_bqb_ag_app_t *tr_bqb_ag = tr_bqb_ag_get_app();

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
	if (tr_bqb_ag->player) {
		tr_bqb_ag->need_resume_play = 1;
		tr_bqb_ag_stop_play();
	}
	break;
	case TTS_EVENT_STOP_PLAY:
	if (tr_bqb_ag->need_resume_play) {
		tr_bqb_ag->need_resume_play = 0;
		tr_bqb_ag_start_play();
	}
	break;
	}
}

