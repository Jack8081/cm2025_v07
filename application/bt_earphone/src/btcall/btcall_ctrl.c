/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call ctrl.
 */
#include "btcall.h"

static void btcall_key_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct btcall_app_t *btcall = btcall_get_app();
	
	ui_key_filter(false);
	btcall->need_resume_key = 0;
    btcall->key_handled = 0;
}


void btcall_key_filter(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

	if (thread_timer_is_running(&btcall->key_timer)) 
    {
		thread_timer_stop(&btcall->key_timer);

		if(btcall->need_resume_key)
			ui_key_filter(false);
	}

	ui_key_filter(true);
	btcall->need_resume_key = 1;

	thread_timer_init(&btcall->key_timer, btcall_key_resume, NULL);
	thread_timer_start(&btcall->key_timer, 200, 0);
}

void btcall_event_notify(int event)
{
	SYS_LOG_INF("### event=%d\n", event);

	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE)
	{
		return;
	}

	sys_manager_event_notify(event, true);
}

void _btcall_sco_check(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct btcall_app_t *btcall = btcall_get_app();

    switch(btcall->switch_source_mode) {
    case SWITCH_SOURCE_MODE_IN:
        //先播放提示音（如何判断结束？等200毫秒?），再切换音源，播放sco
        if(bt_manager_tws_get_dev_role() != BTSRV_TWS_SLAVE){
    		bt_manager_call_switch_sound_source();
    	}

        thread_timer_start(&btcall->sco_check_timer, 1500, 0);
        btcall->switch_source_mode = SWITCH_SOURCE_MODE_NONE;
        break;

    case SWITCH_SOURCE_MODE_OUT:
        //协商200毫秒后停止播放，100毫秒后切音源，200毫秒后播放提示音
        btcall_stop_play();
        thread_timer_start(&btcall->sco_check_timer, 100, 0);
        btcall->switch_source_mode = SWITCH_SOURCE_MODE_OUT_STEP2;
        break;
    case SWITCH_SOURCE_MODE_OUT_STEP2:
        if(bt_manager_tws_get_dev_role() != BTSRV_TWS_SLAVE){
    		bt_manager_call_switch_sound_source();
    	}

        thread_timer_start(&btcall->sco_check_timer, 200, 0);
        btcall->switch_source_mode = SWITCH_SOURCE_MODE_OUT_STEP3;
        break;
    case SWITCH_SOURCE_MODE_OUT_STEP3:
        if(bt_manager_tws_get_dev_role() != BTSRV_TWS_SLAVE) {
    		btcall_event_notify(SYS_EVENT_SWITCH_CALL_OUT);
    	}

        thread_timer_start(&btcall->sco_check_timer, 1000, 0);
        btcall->switch_source_mode = SWITCH_SOURCE_MODE_NONE;
        break;

    default:
        if(!btcall->player && btcall->sco_established && !btcall->need_resume_play) {
    		SYS_LOG_INF("### resume call start.\n");
    		btcall_start_play();
    	} else {
    		SYS_LOG_INF("### not resume call start.\n");
    	}
        btcall->switch_source_mode = SWITCH_SOURCE_MODE_NONE;
        break;
    }
}


void _btcall_key_func_volume_adjust(int updown)
{
	struct btcall_app_t *btcall = btcall_get_app();
	int volume = 0;

	volume = system_volume_get(AUDIO_STREAM_VOICE);
	if(updown) 
	{
		system_volume_set(AUDIO_STREAM_VOICE, volume+1, false);
	}
	else
	{
		system_volume_set(AUDIO_STREAM_VOICE, volume-1, false);
	}


	/* 来电播报号码时不再播报最大/最小音量 */
	if(bt_manager_hfp_get_status() == BT_STATUS_INCOMING)
	{
		if(btcall->incoming_config.Play_Phone_Number && strlen(btcall->ring_phone_num)>0)
		{
			return;
		}
	}

	volume = system_volume_get(AUDIO_STREAM_VOICE);

	if(volume >= CFG_MAX_BT_CALL_VOLUME)
	{
		btcall_event_notify(SYS_EVENT_MAX_VOLUME);
	}
	else if(volume == 0)
	{
		btcall_event_notify(SYS_EVENT_MIN_VOLUME);
	}
}

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
void _btcall_key_func_uac_gain_adjust(int updown)
{
    bool update_flag = false;
    struct btcall_app_t *btcall = btcall_get_app();

    if (updown)
    {
        if (btcall->uac_gain < BTACLL_UAC_GAIN_UNKNOWN - 1)
        {
            btcall->uac_gain  += 1;
            update_flag = true;
        }
    }
    else 
    {
        if (btcall->uac_gain > BTACLL_UAC_GAIN_0)
        {
            btcall->uac_gain  -= 1;
            update_flag = true;
        }
    }
    
    if (update_flag)
    {
        bt_manager_volume_uac_gain(btcall->uac_gain);
        LOG_INF("uac gain : %d", btcall->uac_gain);
    }

    if(btcall->uac_gain >= BTACLL_UAC_GAIN_UNKNOWN - 1)
	{
		btcall_event_notify(SYS_EVENT_MIN_VOLUME);
	}
	else if(btcall->uac_gain == BTACLL_UAC_GAIN_0)
	{
		btcall_event_notify(SYS_EVENT_MAX_VOLUME);
	}
}
#endif

void _btcall_key_func_accept_call(void)
{
	bt_manager_call_accept(NULL);
}

void _btcall_key_func_reject_call(void)
{
	bt_manager_call_terminate(NULL, 0);

	btcall_event_notify(SYS_EVENT_BT_CALL_REJECT);
}

void _btcall_key_func_hangup_call(void)
{
	bt_manager_call_terminate(NULL, 0);
}

void _btcall_key_func_hangup_another_call(void)
{
	bt_manager_call_hangup_another_call();
}

void _btcall_key_func_holdcur_answer_call(void)
{
	// int hfp_status = bt_manager_hfp_get_status();
	bt_manager_call_holdcur_answer_call();
	
	// btcall_led_display(hfp_status);
}

void _btcall_key_func_hangupcur_answer_call(void)
{
	// int hfp_status = bt_manager_hfp_get_status();
	
	bt_manager_call_hangupcur_answer_call();
	// btcall_led_display(hfp_status);
}

void _btcall_key_func_switch_sound_source(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

    if(btcall->switch_source_mode != SWITCH_SOURCE_MODE_NONE) {
        SYS_LOG_WRN("cur mode: %d\n", btcall->switch_source_mode);
        return;
    }

    thread_timer_stop(&btcall->sco_check_timer);
	thread_timer_init(&btcall->sco_check_timer, _btcall_sco_check, NULL);

    if(btcall->player) {
        btcall->switch_source_mode = SWITCH_SOURCE_MODE_OUT;

        //协商200毫秒后停止播放，后切音源，300毫秒后播放提示音
        if(bt_manager_tws_get_dev_role() != BTSRV_TWS_SLAVE) {
            tws_runtime_observer_t * observer = btif_tws_get_tws_observer();
        	uint64_t bt_clk = observer->get_bt_clk_us();
            
            thread_timer_start(&btcall->sco_check_timer, 200, 0);
            if(bt_clk != UINT64_MAX) {
                bt_clk += 200000;
                bt_manager_tws_send_message(TWS_USER_APP_EVENT, TWS_EVENT_BT_CALL_STOP, (uint8_t*)&bt_clk, sizeof(bt_clk));
            }
        }
    } else {
        btcall->switch_source_mode = SWITCH_SOURCE_MODE_IN;
        
        //先播放提示音（如何判断结束？等200毫秒?），再切换音源，播放sco
        if(bt_manager_tws_get_dev_role() != BTSRV_TWS_SLAVE) {
    		btcall_event_notify(SYS_EVENT_SWITCH_CALL_OUT);
    	}

        thread_timer_start(&btcall->sco_check_timer, 200, 0);
    }
}

void _btcall_key_func_switch_mic_mute(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

	btcall->mic_mute ^= 1;

	audio_system_mute_microphone(btcall->mic_mute);

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
	if (!btcall->mic_mute)  //mic_mute = 0 : unmute - ledon
	{
		btcall_event_notify(SYS_EVENT_MIC_MUTE_ON);
	}
	else
	{
		btcall_event_notify(SYS_EVENT_MIC_MUTE_OFF);
	}
#else
	if (btcall->mic_mute)
	{
		btcall_event_notify(SYS_EVENT_MIC_MUTE_ON);
	}
	else
	{
		btcall_event_notify(SYS_EVENT_MIC_MUTE_OFF);
	}
#endif
}

void _btcall_key_func_stop_siri(void)
{
	bt_manager_call_stop_siri();
}

void _btcall_key_func_switch_mic_nr_flag(void)
{
	int nr_flag;

	nr_flag = audio_system_get_microphone_nr_flag();
	if (nr_flag <= 0) {
		nr_flag = 1;
	} else {
		nr_flag = 0;
	}

	audio_system_set_microphone_nr_flag(nr_flag);
}


void btcall_key_func_comm_ctrl(uint8_t key_func)
{
	struct btcall_app_t *btcall = btcall_get_app();
	SYS_LOG_INF("### key func id = 0x%x\n", key_func);

	switch (key_func) 
	{
		case KEY_FUNC_ADD_CALL_VOLUME:
		{
			_btcall_key_func_volume_adjust(1);
            break;
		}
		case KEY_FUNC_SUB_CALL_VOLUME:
		{
			_btcall_key_func_volume_adjust(0);
			break;
		}
        
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
        case KEY_FUNC_ADD_MUSIC_VOLUME_IN_LINKED:
        {
            _btcall_key_func_uac_gain_adjust(0);
            break;
        }
        case KEY_FUNC_SUB_MUSIC_VOLUME_IN_LINKED:
        {
            _btcall_key_func_uac_gain_adjust(1);
            break;
        }
#endif
		case KEY_FUNC_ACCEPT_CALL:
		{
			_btcall_key_func_accept_call();
			break;
		}
		case KEY_FUNC_REJECT_CALL:
		{
			_btcall_key_func_reject_call();
			break;
		}
		case KEY_FUNC_HANGUP_CALL:
		{
			_btcall_key_func_hangup_call();
			break;
		}
		case KEY_FUNC_KEEP_CALL_RELEASE_3WAY:
		{
			_btcall_key_func_hangup_another_call();
			break;
		}
		case KEY_FUNC_HOLD_CALL_ACTIVE_3WAY:
		{
			_btcall_key_func_holdcur_answer_call();
			break;
		}
		case KEY_FUNC_HANGUP_CALL_ACTIVE_3WAY:
		{
			_btcall_key_func_hangupcur_answer_call();
			break;
		}
		case KEY_FUNC_SWITCH_CALL_OUT:
		{
			_btcall_key_func_switch_sound_source();
			break;
		}
		case KEY_FUNC_SWITCH_MIC_MUTE:
		{
			_btcall_key_func_switch_mic_mute();
			break;
		}
		case KEY_FUNC_STOP_VOICE_ASSIST:
		{
			_btcall_key_func_stop_siri();
			break;
		}
		case KEY_FUNC_SWITCH_NR_FLAG:
		{
			_btcall_key_func_switch_mic_nr_flag();
			break;
		}

		default:
			SYS_LOG_ERR("unknow key func %d\n", key_func);
			break;
	}

	btcall->key_handled = 1;

}


