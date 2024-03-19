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
#include "btcall.h"
#include "ui_manager.h"
#include "tts_manager.h"
#include "audio_system.h"
#include "audio_policy.h"

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#ifdef CONFIG_NV_APP
void nv_va_stop_session(void);
#endif

extern uint16_t active_phone_handle;

static void _bt_call_hfp_disconnected(void)
{
	SYS_LOG_INF(" enter\n");
}

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
extern void btcall_event_notify(int event);
#endif
static void _bt_call_sco_connected(io_stream_t upload_stream)
{
	struct btcall_app_t *btcall = btcall_get_app();

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	SYS_LOG_INF("");
	le_audio_background_stop();
	le_audio_background_prepare();
#endif

	btcall->mic_mute = false;
	btcall->upload_stream = upload_stream;
	btcall->upload_stream_outer = upload_stream ? 1 : 0;
	btcall_start_play();
	
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	le_audio_background_start();
#endif
}

static void _bt_call_sco_disconnected(void)
{
	btcall_stop_play();
}

static void _bt_call_ring_first_start(struct thread_timer *ttimer, void *phone_num)
{
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	SYS_LOG_INF("");
	le_audio_background_prepare();
	le_audio_background_start();
#endif

	btcall_ring_start(ttimer, phone_num);
}

void btcall_bt_event_proc(struct app_msg *msg)
{
	struct btcall_app_t *btcall = btcall_get_app();
	int hfp_status;
	struct bt_call_report *call_rep;
	struct bt_audio_report* rep;
	
	switch (msg->cmd) 
	{
        case BT_TWS_CONNECTION_EVENT:
        {
            active_phone_handle = 0;
            break;
        }
        
		case BT_CALL_RING_STATR_EVENT:
		{
			call_rep = (struct bt_call_report *)(msg->ptr);		
			memset(btcall->ring_phone_num, 0, sizeof(btcall->ring_phone_num));
			SYS_LOG_INF("strlen((uint8_t *)(call_rep + 1)):%d",strlen((uint8_t *)(call_rep + 1)));
			strncpy(btcall->ring_phone_num, (uint8_t *)(call_rep + 1), strlen((uint8_t *)(call_rep + 1)));
			break;
		}
			
		case BT_CALL_RING_STOP_EVENT:
		{
			btcall_ring_stop();
			break;
		}
		case BT_CALL_DIALING:
		{
			ui_event_notify(UI_EVENT_BT_CALL_OUTGOING, NULL, false);
		
			break;
		}
		case BT_CALL_INCOMING:
		{
			/* 延迟一段时间再检查播放来电提示音?
			 *
			 * 手机来电自带铃声会通过 SCO 链路进行播放,
			 * 来电状态一般会早于 SCO 链路,
			 * 配置已有来电铃声时不播放提示音,
			 * 延迟可避免提前播放了提示音?
			 */
			 if(!btcall->ring_playing)
			 {
				 thread_timer_init(&btcall->ring_timer, _bt_call_ring_first_start, btcall->ring_phone_num);
				 thread_timer_start(&btcall->ring_timer, 1000, 0);
				 btcall->ring_playing = 1;
			 }
			 ui_event_notify(UI_EVENT_BT_CALL_INCOMING, NULL, false);			 
			break;
		}
		case BT_CALL_ACTIVE:
		{	
		    if((!btcall->enable_tts_in_calling) && (!btcall->is_tts_locked) && (btcall->player != NULL))
            {
                tts_manager_lock();
                btcall->is_tts_locked = true;
            }
			btcall_ring_stop();
			btcall->ring_playing = 0;
			btcall->phone_num_play = 0;

			if(btcall->player != NULL)
			{
				media_player_set_hfp_connected(btcall->player, true);
			}
			ui_event_notify(UI_EVENT_BT_CALL_ONGOING, NULL, false);
#if (defined CONFIG_SAMPLE_CASE_1) ||  (defined CONFIG_SAMPLE_CASE_XNT)
            ui_event_led_display(UI_EVENT_MIC_MUTE_ON);
#endif
			break;
		}

		case BT_CALL_3WAYIN:
		{
			if(!btcall->ring_playing)
			{
				thread_timer_init(&btcall->ring_timer, btcall_ring_start, btcall->ring_phone_num);
				thread_timer_start(&btcall->ring_timer, 0, 0);
				btcall->ring_playing = 1;
			}
			break;
		}

		case BT_CALL_MULTIPARTY:
		{
			break;
		}

		case BT_CALL_3WAYOUT:
		{
			break;
		}		
		
		case BT_CALL_SIRI_MODE:
		{
			SYS_LOG_INF("siri_mode %d\n", btcall->siri_mode);
			break;
		}
		
		case BT_CALL_ENDED:
		{
			btcall->ring_playing = 0;
			btcall->phone_num_play = 0;
			ui_event_notify(UI_EVENT_BT_CALL_END, NULL, false);
			
			break;
		}

		case BT_HFP_DISCONNECTION_EVENT:
		{
		    btcall->need_resume_play = 0;
			_bt_call_hfp_disconnected();
			break;
		}
		
		case BT_AUDIO_STREAM_ENABLE:
		{
			rep = (struct bt_audio_report*)msg->ptr;
			if(rep->audio_contexts != BT_AUDIO_CONTEXT_ASQT_TEST
				&& rep->audio_contexts != BT_AUDIO_CONTEXT_CONVERSATIONAL){
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
				le_audio_background_bt_event_proc(msg);
#endif
				break;
			}
			btcall_handle_enable(msg->ptr);
			break;
		}

		case BT_AUDIO_STREAM_START:
		{
			io_stream_t upload_stream = NULL;
			rep = (struct bt_audio_report*)msg->ptr;
			
			if(rep->audio_contexts != BT_AUDIO_CONTEXT_ASQT_TEST
				&& rep->audio_contexts != BT_AUDIO_CONTEXT_CONVERSATIONAL){
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
				le_audio_background_bt_event_proc(msg);
#endif
				break;
			}
			if(rep->audio_contexts == BT_AUDIO_CONTEXT_ASQT_TEST){
				struct hfp_msg_data {
					struct bt_audio_report audio_rep;
					io_stream_t zero_stream;
				};
				struct hfp_msg_data* param = (struct hfp_msg_data*)msg->ptr;
				upload_stream = param->zero_stream;
			}

			btcall->sco_established = 1;
			
			/* 配置: 只播放本地提示音 */
			if(btcall->ring_playing && btcall->incoming_config.BT_Call_Ring_Mode==BT_CALL_RING_MODE_LOCAL)
			{
				SYS_LOG_INF("### not play sco-ring.");
				break;
			}

			/* 通话中不播tts时，打开通话前先禁止tts */
			if((!btcall->enable_tts_in_calling) 
                && (bt_manager_hfp_get_status()==BT_STATUS_ONGOING)
                && (btcall->disable_voice != 1))
			{
				sys_manager_enable_audio_voice(FALSE);
				SYS_LOG_INF("### forbidden tts.\n");
				btcall->disable_voice = 1;
			}

			_bt_call_sco_connected(upload_stream);
			break;
		}
		
		case BT_AUDIO_STREAM_STOP:
		{
			rep = (struct bt_audio_report*)msg->ptr;
			if(rep->audio_contexts != BT_AUDIO_CONTEXT_ASQT_TEST
				&& rep->audio_contexts != BT_AUDIO_CONTEXT_CONVERSATIONAL){
				break;
			}		
		    _bt_call_sco_disconnected();
			btcall->sco_established = 0;
			/* 恢复播tts */
			if((btcall->disable_voice) && (btcall->disable_voice != 0))
			{
				sys_manager_enable_audio_voice(TRUE);
				SYS_LOG_INF("### restore tts.\n");
				btcall->disable_voice = 0;
			}
		#ifdef CONFIG_NV_APP
			nv_va_stop_session();
		#endif
			break;
		}
		case BT_REQ_RESTART_PLAY:
		{
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
			le_audio_background_stop();
#endif		
			btcall_stop_play();
			if(btcall->sco_established)
			{
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
				le_audio_background_prepare();
#endif
				btcall_start_play();
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
				le_audio_background_start();
#endif
			}
			break;
		}

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
		case BT_CALL_REPLAY:
			if (btcall->sco_established) {
				btcall_start_play();
			}
			break;
#endif
	
		default:
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
			le_audio_background_bt_event_proc(msg);
#endif
			break;
	}

	// display led
	hfp_status = bt_manager_hfp_get_status();
	if (btcall->last_disp_status != hfp_status)
	{
		btcall->last_disp_status = hfp_status;
		btcall_led_display(hfp_status);
	}
	
}

void btcall_input_event_proc(struct app_msg *msg)
{
	uint8_t key_func = msg->cmd;
	struct btcall_app_t *btcall = btcall_get_app();

    /* 检查key是否已经屏蔽，防止两边同时按下处理两次按键
     */
	if(btcall->need_resume_key)
		return;

	if(bt_manager_tws_get_dev_role() != BTSRV_TWS_NONE)
	{
		bt_manager_tws_send_message(TWS_USER_APP_EVENT, TWS_EVENT_REMOTE_KEY_PROC_BEGIN, &key_func, 1);
	}

	btcall_key_filter();

	/* 副耳转发给主耳处理 */
	if(bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE)
	{
		btcall_tws_slave_key_ctrl(key_func);
		return;
	}

	/* 主耳按键处理 */
	btcall_key_func_comm_ctrl(key_func);

}

void btcall_tts_event_proc(struct app_msg *msg)
{
	struct btcall_app_t *btcall = btcall_get_app();

	switch (msg->value) 
	{
		case TTS_EVENT_START_PLAY:
		{
			btcall->tts_start_stop = 1;
			//if(!btcall->phone_num_play)
			{
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
				SYS_LOG_INF("TTS_EVENT_START_PLAY");
				le_audio_background_stop();
#endif
				btcall_media_mutex_stop();
			}
			break;
		}

		case TTS_EVENT_STOP_PLAY:
		{
			btcall->tts_start_stop = 0;
			//if(!btcall->phone_num_play)
			{
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
				SYS_LOG_INF("TTS_EVENT_STOP_PLAY");
				le_audio_background_prepare();
#endif
				btcall_media_mutex_resume();
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
				le_audio_background_start();
#endif
				break;
			}
		}
	}
}

void btcall_app_event_proc(struct app_msg *msg)
{
	struct btcall_app_t *btcall = btcall_get_app();
	uint8_t event;

	switch (msg->cmd)
	{
		case PHONENUM_PRE_PLAY_EVENT:
		{
			btcall->phone_num_play = 1;
			btcall_media_mutex_stop();
			os_sem_give(msg->sync_sem);
			break;
		}
	
		case RING_REPEATE_EVENT:
		{
			btcall->phone_num_play = 0;
			btcall_media_mutex_resume();
			
			event = btcall_ring_check();
			if(event == NONE)
			{
				SYS_LOG_INF("### not repeat ring.\n");
				return;
			}
			
			thread_timer_init(&btcall->ring_timer, btcall_ring_start, btcall->ring_phone_num);
			thread_timer_start(&btcall->ring_timer, btcall->incoming_config.Prompt_Interval_Ms, 0);
			break;
		}

		default:
			break;
	}
}

