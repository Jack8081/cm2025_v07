/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call ring
 */

#include "btcall.h"
#include "tts_manager.h"

/* 电话号码开始播放前互斥停止其它媒体音频
 */
void btcall_phonenum_pre_play(void)
{
	struct btcall_app_t *btcall = btcall_get_app();
	struct app_msg msg = {0};
	os_sem sync_sem;

	if(btcall == NULL)
		return;

	os_sem_init(&sync_sem, 0, 1);

	memset(&msg,0,sizeof(struct app_msg));
	msg.type = MSG_APP_BTCALL_EVENT;
	msg.cmd = PHONENUM_PRE_PLAY_EVENT;
	msg.sync_sem = &sync_sem;
	send_async_msg(APP_ID_BTCALL, &msg);

	os_sem_take(&sync_sem, 3*1000);
	
}

int btcall_ring_check(void)
{
	int hfp_state;
	
	hfp_state = bt_manager_hfp_get_status();

	if(hfp_state == BT_STATUS_INCOMING)
	{
		return UI_EVENT_BT_CALL_INCOMING;	  // 来电
	}
	else if(hfp_state == BT_STATUS_3WAYIN)
	{
		return UI_EVENT_BT_CALL_3WAYIN;	  // 三方来电
	}

	return NONE;
}

void btcall_ring_start(struct thread_timer *ttimer, void *phone_num)
{
	struct btcall_app_t *btcall = btcall_get_app();
	static event_call_incoming_t incoming_param;
	uint8_t event;

	if(btcall == NULL)
		return;

	if(bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE)
	{
		return;
	}
	
	SYS_LOG_INF("### start call ring.\n");

	event = btcall_ring_check();
	if(event == NONE)
		return;

	if(thread_timer_is_running(ttimer))
		thread_timer_stop(ttimer);

	memset(&incoming_param, 0, sizeof(event_call_incoming_t));
	
	if(btcall->incoming_config.Play_Phone_Number)
	{
		memcpy(incoming_param.voice_list, phone_num, strlen(phone_num));
		incoming_param.voice_num = strlen(phone_num);
		SYS_LOG_INF("### phone num[%d]: %s\n", incoming_param.voice_num, (char*)phone_num);
	}
	
	/* 配置: 已有来电铃声时不播提示音 ? */
	if(btcall->sco_established && btcall->incoming_config.BT_Call_Ring_Mode==BT_CALL_RING_MODE_REMOTE)
	{
		incoming_param.is_play_tone = 0;
	}
	else	// 其他情况均播放提示音
	{
		incoming_param.is_play_tone = 1;
	}
	incoming_param.start_callback = btcall_phonenum_pre_play;
	incoming_param.end_callback = btcall_ring_repeat;

	if(bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER)
	{
		/* 通知副耳播放来电铃声 */
		bt_manager_tws_send_message(TWS_USER_APP_EVENT, TWS_EVENT_BT_CALL_START_RING, &event, 1);
	}

	ui_event_notify(event, &incoming_param, NO);

}

void btcall_ring_repeat(void)
{
	struct btcall_app_t *btcall = btcall_get_app();
	struct app_msg msg = {0};

	if(btcall == NULL)
		return;

	SYS_LOG_INF("### repeat call ring.\n");
	memset(&msg,0,sizeof(struct app_msg));
	msg.type = MSG_APP_BTCALL_EVENT;
	msg.cmd = RING_REPEATE_EVENT;
	send_async_msg(APP_ID_BTCALL, &msg);
}

void btcall_ring_stop(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

	if (thread_timer_is_running(&btcall->ring_timer))
		thread_timer_stop(&btcall->ring_timer);

	memset(btcall->ring_phone_num, 0, sizeof(btcall->ring_phone_num));

	SYS_LOG_INF(" ok \n");
}

int btcall_ring_init(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

	thread_timer_init(&btcall->ring_timer, btcall_ring_start, btcall->ring_phone_num);

	return 0;
}

int btcall_ring_deinit(void)
{
	btcall_ring_stop();

	return 0;
}

