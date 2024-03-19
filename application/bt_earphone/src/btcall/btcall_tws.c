/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call tws.
 */
#include "btcall.h"


/* TWS 副机按键控制
 */
void btcall_tws_slave_key_ctrl(uint8_t key_func)
{

	/* 副机将按键消息发送给主机处理 */
	bt_manager_tws_send_message(TWS_USER_APP_EVENT, TWS_EVENT_BT_CALL_KEY_CTRL, &key_func, 1);

	/* 切换通话输出：先关闭播放器，避免提示音播放不完整 */
	if(key_func == KEY_FUNC_SWITCH_CALL_OUT)
	{
		btcall_stop_play();
	}
}

void btcall_tws_ring_start(uint8_t event)
{
	struct btcall_app_t *btcall = btcall_get_app();
	static event_call_incoming_t incoming_param;

	if(thread_timer_is_running(&btcall->ring_timer))
		thread_timer_stop(&btcall->ring_timer);
	
	btcall->ring_playing = 1;
	memset(&incoming_param, 0, sizeof(event_call_incoming_t));
	
	if(btcall->incoming_config.Play_Phone_Number)
	{
		memcpy(incoming_param.voice_list, btcall->ring_phone_num, strlen(btcall->ring_phone_num));
		incoming_param.voice_num = strlen(btcall->ring_phone_num);
	}
	
	/* 配置: 已有来电铃声时不播提示音 ? */
	if(btcall->sco_established && btcall->incoming_config.BT_Call_Ring_Mode==BT_CALL_RING_MODE_REMOTE)
	{
		incoming_param.is_play_tone = 0;
	}
	else
	{
		incoming_param.is_play_tone = 1;
	}
	incoming_param.start_callback = btcall_phonenum_pre_play;
	incoming_param.end_callback = btcall_ring_repeat;

	ui_event_notify(event, &incoming_param, NO);
}

 /* TWS 事件消息处理
  */
int btcall_tws_event_proc(struct app_msg *msg)
{
	struct btcall_app_t *btcall = btcall_get_app();
	tws_message_t* message = (tws_message_t*)msg->ptr;

	SYS_LOG_INF("cmd_id %d\n", message->cmd_id);
    switch(message->cmd_id)
    {
    	case TWS_EVENT_REMOTE_KEY_PROC_BEGIN:
    	{
			btcall_key_filter();
			break;
    	}
			
        case TWS_EVENT_BT_CALL_KEY_CTRL:
        {
			uint8_t key_func = message->cmd_data[0];
			
            /* 检查key是否已经处理，防止两边同时按下处理两次按键
             */
			if(!btcall->key_handled)
        		btcall_key_func_comm_ctrl(key_func);
				
			break;
        }
			
        case TWS_EVENT_BT_CALL_START_RING:
        {
			uint8_t event = message->cmd_data[0];

			btcall_tws_ring_start(event);
			break;
        }

        case TWS_EVENT_BT_CALL_STOP:
        {
            tws_runtime_observer_t * observer = btif_tws_get_tws_observer();
        	uint64_t bt_clk_cur = observer->get_bt_clk_us();
            uint64_t bt_clk;
            int32_t clk_diff;

            memcpy(&bt_clk, message->cmd_data, sizeof(bt_clk));
            clk_diff = bt_manager_bt_clk_diff_us(bt_clk, bt_clk_cur);
            printk("switch clk diff: %d\n", clk_diff);
            if((clk_diff > 200000) || (clk_diff < 2000)) {
                clk_diff = 2000;
            }

            thread_timer_stop(&btcall->sco_check_timer);
        	thread_timer_init(&btcall->sco_check_timer, _btcall_sco_check, NULL);
            thread_timer_start(&btcall->sco_check_timer, clk_diff/1000, 0);
            break;
        }
		default:
			break;
    }
    return 0;
}


