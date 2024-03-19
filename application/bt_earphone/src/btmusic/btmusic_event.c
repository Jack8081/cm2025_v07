/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music event
 */
#include "btmusic.h"
#include "ui_manager.h"

#ifdef CONFIG_CONSUMER_EQ
#include "consumer_eq.h"
#endif



extern int64_t btcall_exit_time;
static int64_t btmusic_switch_phone_time = 0;
uint16_t active_phone_handle = 0;

void btmusic_bt_event_proc(struct app_msg *msg)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
	struct bt_audio_report* rep;

	SYS_LOG_INF("bt event %d", msg->cmd);

	switch (msg->cmd){
		case BT_CONNECTED:
		{
            if(active_phone_handle ==  *(uint16_t *)msg->ptr) {
                btmusic_switch_phone_time = os_uptime_get();
                printk("connect time: %u\n", (uint32_t)btmusic_switch_phone_time);
            }
			break;
		}

		case BT_DISCONNECTED:
		{
			break;
		}
		
		case BT_TWS_CONNECTION_EVENT:
		{
			// avoid play music before tws connected tts
			btmusic->tws_con_tts_playing = 1;
			thread_timer_start(&btmusic->play_timer, 200, 0);
			bt_music_check_channel_output(false);
			bt_music_tws_cancel_filter();

            //fix one earphone delay start, but another not.
            active_phone_handle = 0;
            btmusic_switch_phone_time = 0;
            btcall_exit_time = 0;

#ifdef CONFIG_CONSUMER_EQ
            consumer_eq_on_tws_connect();
#endif
			break;
		}

		case BT_TWS_DISCONNECTION_EVENT:
		{
			if (btmusic->player)
			{
				media_player_set_output_mode(btmusic->player, MEDIA_OUTPUT_MODE_DEFAULT);
			}

			bt_music_check_channel_output(false);
			bt_music_tws_cancel_filter();			
			break;
		}

		case BT_TWS_ROLE_CHANGE:
		{
			bt_music_check_channel_output(false);
			bt_music_tws_cancel_filter();
			break;
		}

		case BT_MEDIA_CONNECTED:
		{

			break;
		}

		case BT_MEDIA_DISCONNECTED:
		{
			break;
		}
		
		case BT_MEDIA_PLAYBACK_STATUS_CHANGED_EVENT:
		{
			struct bt_media_play_status *status = (struct bt_media_play_status *)msg->ptr;

			if (status->status == BT_STATUS_PLAYING)
			{
				bt_manager_audio_stream_restore(BT_TYPE_BR);
			}
			break;		
		}

		case BT_MEDIA_TRACK_CHANGED_EVENT:
		{
		#ifdef CONFIG_BT_AVRCP_GET_ID3
			bt_manager_media_get_id3_info();
		#endif
			break;
		}
		
		case BT_VOLUME_VALUE:
		{
			break;
		}

		
		case BT_AUDIO_STREAM_ENABLE:
		{
			rep = (struct bt_audio_report*)msg->ptr;
			if(rep->audio_contexts != BT_AUDIO_CONTEXT_MEDIA){
				break;
			}		
            SYS_LOG_INF("STREAM_ENABLE\n");
			bt_music_handle_enable(msg->ptr);			
			break;
		}

		case BT_AUDIO_STREAM_START:
		{
            int64_t cur_time = os_uptime_get();
            int64_t delta_time;			
			rep = (struct bt_audio_report*)msg->ptr;
			if(rep->audio_contexts != BT_AUDIO_CONTEXT_MEDIA){
				break;
			}
            
			SYS_LOG_INF("STREAM_START, %x, %x\n", rep->handle, active_phone_handle);
            if((active_phone_handle != rep->handle) && (active_phone_handle != 0)) {
                btmusic_switch_phone_time = cur_time;
                //printk("update time: %u\n", (uint32_t)btmusic_switch_phone_time);
            }
            active_phone_handle = rep->handle;
            
			if (btmusic->mutex_stop || btmusic->tws_con_tts_playing ||
				btmusic_ctrl_avdtp_is_filter())
			{
				SYS_LOG_INF("start later %d %d", btmusic->mutex_stop, btmusic->tws_con_tts_playing);
				break;
			}

			if (btcall_exit_time)
			{
                int64_t delay_time;
                
				delta_time = cur_time - btcall_exit_time;
                if(bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) {
                    delay_time = DELAY_MUSIC_TIME_AFTER_BTCALL_SLAVE;
                } else {
                    delay_time = DELAY_MUSIC_TIME_AFTER_BTCALL_MASTER;
                }
                
				if((delta_time < delay_time) && (delta_time >= 0)) {
					delta_time = delay_time - delta_time;
					SYS_LOG_WRN("call exit delay %d ms to start\n", (uint32_t)delta_time);
					thread_timer_start(&btmusic->play_timer, (uint32_t)delta_time, 0);
					break;
				}
                btcall_exit_time = 0;

                if(bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) {
                    bt_manager_tws_send_message(TWS_USER_APP_EVENT, TWS_EVENT_BT_MUSIC_START, NULL, 0);
                }
			}

			if (btmusic_switch_phone_time)
			{
				delta_time = cur_time - btmusic_switch_phone_time;
				if((delta_time < DELAY_MUSIC_TIME_AFTER_SWITCH_PHONE) && (delta_time >= 0)) {
					delta_time = DELAY_MUSIC_TIME_AFTER_SWITCH_PHONE - delta_time;
					SYS_LOG_WRN("phone switch delay %d ms to start\n", (uint32_t)delta_time);
					thread_timer_start(&btmusic->play_timer, (uint32_t)delta_time, 0);
					break;
				}
                btmusic_switch_phone_time = 0;
			}
			
			bt_music_handle_start(msg->ptr);
			break;
		}

		case BT_AUDIO_STREAM_STOP:
		{
			rep = (struct bt_audio_report*)msg->ptr;
			if(rep->audio_contexts != BT_AUDIO_CONTEXT_MEDIA){
				break;
			}		
			SYS_LOG_INF("STREAM_STOP\n");
			bt_music_handle_stop(msg->ptr);
			break;
		}	
		
        case BT_REQ_RESTART_PLAY:
    	{
    		bt_music_stop_play();
    		//os_sleep(200);
			bt_manager_audio_stream_restore(BT_TYPE_BR);
    		break;
    	}

		case BT_AUDIO_STREAM_PRE_STOP:
		{
			bt_music_pre_stop_play();
			break;
		}
		
		default:
			break;
	}

	btmusic_view_display();
}

