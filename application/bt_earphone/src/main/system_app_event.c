/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief system app event.
 */

#include <mem_manager.h>
#include <msg_manager.h>
#include <sys_event.h>

#include "app_ui.h"
#include "system_app.h"
#include <sys_wakelock.h>
#include <bt_manager.h>
#include "audio_system.h"
#include <audiolcy_common.h>
#include <user_comm/ap_status.h>
#include <user_comm/ap_record.h>
#include "anc_hal.h"

#ifdef CONFIG_GMA_APP
extern uint8_t gma_dev_sent_record_start_stop(uint8_t cmd);
#endif
#ifdef CONFIG_TME_APP
extern uint8_t tme_dev_sent_record_start_stop(uint8_t cmd);
#endif
#ifdef CONFIG_NV_APP
extern uint8_t nv_dev_sent_record_start_stop(uint8_t cmd);
u8_t ble_nv_status(void);
#endif

#ifdef CONFIG_TUYA_APP
extern uint8_t tuya_dev_sent_record_start_stop(uint8_t cmd);
#endif

#ifdef CONFIG_ANC_HAL
static void system_tws_sync_anc_mode(uint8_t anc_mode);
#endif

static void bt_manager_privma_talk_start(void)
{
	SYS_LOG_INF("");
#ifdef CONFIG_AP_USER_COMM
	if (ap_status_get() < AP_STATUS_AUTH)
		return;
#endif

#ifdef CONFIG_NV_APP
	if (ble_nv_status())
	{
		SYS_LOG_INF("Nativevoice stop.");
	}
	else
	{
		if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER)
			bt_manager_tws_send_event_sync(TWS_UI_EVENT, SYS_EVENT_PRIVMA_TALK_START);
		else
			sys_event_notify(SYS_EVENT_PRIVMA_TALK_START);
	}
	nv_dev_sent_record_start_stop(0);
	return;
#endif
	if (bt_manager_hfp_get_status() > BT_STATUS_HFP_NONE)
		return;

	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER)
		bt_manager_tws_send_event_sync(TWS_UI_EVENT, SYS_EVENT_PRIVMA_TALK_START);
	else
		sys_event_notify(SYS_EVENT_PRIVMA_TALK_START);
#ifdef CONFIG_GMA_APP
	gma_dev_sent_record_start_stop(0);
#endif
#ifdef CONFIG_TME_APP
	tme_dev_sent_record_start_stop(0);
#endif
#ifdef CONFIG_TUYA_APP
	tuya_dev_sent_record_start_stop(0);
#endif

	return;
}


int bt_manager_tws_sync_app_status_info(void* param)
{
    system_app_context_t*  manager = system_app_get_context();
    int dev_role = bt_manager_tws_get_dev_role();

    if (dev_role == BTSRV_TWS_MASTER)
    {
        app_tws_sync_status_info_t info;	
        info.voice_language = manager->select_voice_language;
        info.low_latency_mode = audiolcy_get_latency_mode(); 
	
        if (property_get(BTMUSIC_MULTI_DAE, &info.dae_index, sizeof(u8_t)) < 0){
            info.dae_index = 0;
        }

       info.bt_music_vol = audio_system_get_stream_volume(AUDIO_STREAM_MUSIC);
       info.bt_call_vol	= audio_system_get_stream_volume(AUDIO_STREAM_VOICE);
       
       #ifdef CONFIG_ANC_HAL
       info.anc_mode = manager->sys_status.anc_mode;
       #endif
       SYS_LOG_INF("[master]bt_music_vol:%d,bt_call_vol:%d",info.bt_music_vol,info.bt_call_vol);

	   bt_manager_tws_send_message
	   (
	   TWS_USER_APP_EVENT, TWS_EVENT_SYNC_STATUS_INFO, 
	   (uint8_t *)&info, sizeof(info)
	   );

	   ui_app_setting_key_map_tws_sync(NULL); 
    }
    else if(dev_role == BTSRV_TWS_SLAVE)
    {
        app_tws_sync_status_info_t*  status_info = (app_tws_sync_status_info_t*)param;
        if (param == NULL)
            return -1;
				
        char *current_app = app_manager_get_current_app();
        if (!current_app)
            return -1;

        SYS_LOG_INF("[slave]bt_music_vol:%d,bt_call_vol:%d",status_info->bt_music_vol,status_info->bt_call_vol);
        if (memcmp(current_app, "btcall", strlen("btcall")) == 0)
        {
            audio_system_set_stream_volume(AUDIO_STREAM_MUSIC, status_info->bt_music_vol);	
            system_player_volume_set(AUDIO_STREAM_VOICE, status_info->bt_call_vol);
        }
        else
        {
            audio_system_set_stream_volume(AUDIO_STREAM_VOICE, status_info->bt_call_vol);
            system_player_volume_set(AUDIO_STREAM_MUSIC, status_info->bt_music_vol);			
        }

        system_tws_set_voice_language(status_info->voice_language);
        audiolcy_set_latency_mode(status_info->low_latency_mode, 1);
		
        struct app_msg msg={0};
        msg.type = MSG_APP_SYNC_MUSIC_DAE;
        msg.value = status_info->dae_index;
        send_async_msg(current_app, &msg);

        #ifdef CONFIG_ANC_HAL
        system_tws_sync_anc_mode(status_info->anc_mode);
        #endif
    }

	return 0;
}


static int tws_s2m_message(int event, char keyfunc)
{
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) 
	{
		bt_manager_tws_send_message(TWS_USER_APP_EVENT, event, &keyfunc, 1);

		return 0;
	}
	
	return -1;
}
#ifdef CONFIG_ANC_HAL
static int tws_m2s_message(int event, char keyfunc)
{
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) 
	{
		bt_manager_tws_send_message(TWS_USER_APP_EVENT, event, &keyfunc, 1);

		return 0;
	}
	
	return -1;
}

static bool is_anc_wake_lock = false;
static void system_anc_open(uint8_t anc_mode)
{
    CFG_Struct_Audio_Settings audio_settings;
    anc_info_t anc_init_info = {0};

    if(!is_anc_wake_lock){
        sys_wake_lock(FULL_WAKE_LOCK);
        is_anc_wake_lock = true;
    }

    app_config_read(CFG_ID_AUDIO_SETTINGS, &audio_settings, 0, sizeof(audio_settings));
    anc_init_info.ffmic = audio_settings.anc_ff_mic;
    anc_init_info.fbmic = audio_settings.anc_fb_mic;
    anc_init_info.speak = audio_settings.ANC_Speaker_Out;
    anc_init_info.rate = audio_settings.ANC_Sample_rate;
    anc_init_info.aal = 1;
    switch(anc_mode){
        case SYS_ANC_DENOISE_MODE:
            anc_init_info.mode = ANC_MODE_ANCON;
            break;
        case SYS_ANC_TRANSPARENCY__MODE:
            anc_init_info.mode = ANC_MODE_TRANSPARENT;
            break;
        case SYS_ANC_WINDY__MODE:
            anc_init_info.mode = ANC_MODE_ANCON;
            break;
        case SYS_ANC_LEISURE__MODE:
            anc_init_info.mode = ANC_MODE_ANCON;
            anc_init_info.gain = -60;
            break;
        default:
            anc_init_info.mode = ANC_MODE_NORMAL;
            break;
    }

    if(anc_mode == SYS_ANC_DENOISE_MODE){
        anc_dsp_open(&anc_init_info);
    }else if(anc_init_info.mode != ANC_MODE_NORMAL){
        anc_dsp_reopen(&anc_init_info);
    }else{
    }
    

    switch(anc_mode){
        case SYS_ANC_DENOISE_MODE:
            SYS_LOG_INF("ANC DENOISE MODE");
            sys_manager_event_notify(SYS_EVENT_ANC_MODE, false);
            break;
        case SYS_ANC_TRANSPARENCY__MODE:
            SYS_LOG_INF("ANC TRANSPARENT MODE");
            sys_manager_event_notify(SYS_EVENT_TRANSPARENCY_MODE, false);
            break;
        case SYS_ANC_WINDY__MODE:
            SYS_LOG_INF("ANC WINDY MODE");
            anc_dsp_send_command(ANC_COMMAND_OFF_FF, NULL, 0);
            sys_manager_event_notify(SYS_EVENT_ANC_WIND_NOISE_MODE, false);
            break;
        case SYS_ANC_LEISURE__MODE:
            SYS_LOG_INF("ANC LEISURE MODE");
            //anc_dsp_set_gain(ANC_MODE_ANCON, ANC_FILTER_FF, -60, 0);
            //anc_dsp_set_gain(ANC_MODE_ANCON, ANC_FILTER_FB, -60, 0);
            sys_manager_event_notify(SYS_EVENT_ANC_LEISURE_MODE, false);
            break;
        default:
            break;
    }
}

static void system_anc_close(void)
{
    anc_dsp_close();
    
    if(is_anc_wake_lock){
        sys_wake_unlock(FULL_WAKE_LOCK);
        is_anc_wake_lock = false;
    }    
}

static void anc_switch(void)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t*  sys_status = &manager->sys_status;

    sys_status->anc_mode++;

    if(sys_status->anc_mode == SYS_ANC_MODE_MAX){
        sys_status->anc_mode = SYS_ANC_OFF_MODE;

        system_anc_close();        
        sys_manager_event_notify(SYS_EVENT_NOMAL_MODE, true);
        return;
    }


    system_anc_open(sys_status->anc_mode);
}


static void system_tws_sync_anc_mode(uint8_t anc_mode)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t*  sys_status = &manager->sys_status;

    if(anc_mode != sys_status->anc_mode)
    {
        sys_status->anc_mode = anc_mode;

        if(anc_mode == SYS_ANC_OFF_MODE)
        {
            system_anc_close();
        }
        else
        {
            system_anc_open(anc_mode);
        }
    }

}

#endif // CONFIG_ANC_HAL



static void system_key_func_proc(int keyfunc, bool from_slave)
{
	switch (keyfunc) {
	case KEY_FUNC_DIAL_LAST_NO:
	{
		bt_manager_call_dial_last();
	}
		break;
	case KEY_FUNC_START_VOICE_ASSIST:
	{
		bt_manager_call_start_siri();
	}
		break;
	case KEY_FUNC_STOP_VOICE_ASSIST:
	{
		bt_manager_call_stop_siri();
	}
		break;
	case KEY_FUNC_HID_PHOTO_SHOT:
	{
		bt_manager_hid_take_photo();
	}
		break;
	case KEY_FUNC_HID_CUSTOM_KEY:
	{
		SYS_LOG_INF("TODO!!!!KEY_FUNC_HID_CUSTOM_KEY. ");
		break;
	}
	case KEY_FUNC_ENTER_PAIR_MODE:
	{
		bt_manager_enter_pair_mode();
	}	
		break;
	case KEY_FUNC_TWS_PAIR_SEARCH:
	{
		bt_manager_tws_pair_search();
	}	
		break;
	case KEY_FUNC_CLEAR_PAIRED_LIST:
	case KEY_FUNC_CLEAR_PAIRED_LIST_IN_PAIR_MODE:
	case KEY_FUNC_CLEAR_PAIRED_LIST_IN_FRONT_CHARGE:
	case KEY_FUNC_CLEAR_PAIRED_LIST_IN_UNLINKED:
	{
		SYS_LOG_INF("KEY_FUNC_CLEAR_PAIRED_LIST.");
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
        system_do_event_notify(UI_EVENT_CLEAR_PAIRED_LIST, false, NULL);
#endif
		bt_manager_audio_clear_paired_list(BT_TYPE_LE);
		bt_manager_audio_clear_paired_list(BT_TYPE_BR);    
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
        system_do_event_notify(SYS_EVENT_BT_UNLINKED, false, NULL);
#endif
	}
		break;
	case KEY_FUNC_START_RECONNECT:
	{
		bt_manager_manual_reconnect();
	}
		break;
	case KEY_FUNC_ENTER_BQB_TEST_MODE:
	{
		SYS_LOG_INF("KEY_FUNC_ENTER_BQB_TEST_MODE.");
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
        printk("enter bqb test\n");
        system_do_event_notify(UI_EVENT_ENTER_BQB_TEST_MODE, false, NULL);
        system_do_event_notify(UI_EVENT_CUSTOMED_6, false, NULL);
#endif
	    system_enter_bqb_test_mode(0);
	}
		break;
	case KEY_FUNC_ENTER_BQB_TEST_IN_PAIR_MODE:
	{
		SYS_LOG_INF("KEY_FUNC_ENTER_BQB_TEST_IN_PAIR_MODE.");	
	    system_enter_bqb_test_mode(0);
	}		
		break;
	case KEY_FUNC_SWITCH_VOICE_LANG:
	{
		SYS_LOG_INF("KEY_FUNC_SWITCH_VOICE_LANG.");
		system_switch_voice_language();    
	}
        break;
	case KEY_FUNC_SWITCH_VOICE_LANG_IN_UNLINKED:
	{
		SYS_LOG_INF("KEY_FUNC_SWITCH_VOICE_LANG_IN_UNLINKED.");
		system_switch_voice_language();    
	}
		break;
	case KEY_FUNC_SWITCH_VOICE_LANG_IN_PAIR_MODE:
	{
		SYS_LOG_INF("KEY_FUNC_SWITCH_VOICE_LANG_IN_PAIR_MODE.");
		system_switch_voice_language();    
	}
		break;

	case KEY_FUNC_START_PRIVMA_TALK:	
		bt_manager_privma_talk_start();
		break;

	case KEY_FUNC_NMA_KEY_FIRST:
#ifdef CONFIG_NMA_APP
		nma_key_press_upload(1);
#endif
#ifdef CONFIG_TME_APP
		tma_one_key_request(0);
#endif
		break;

	case KEY_FUNC_NMA_KEY_SECOND:
#ifdef CONFIG_NMA_APP
		nma_key_press_upload(2);
#endif
		break;

	case KEY_FUNC_TMA_ONE_KEY_TO_REQUEST:
		break;

	case KEY_FUNC_SWITCH_LOW_LATENCY_MODE:
		system_switch_low_latency_mode();		
		break;
		
	case KEY_FUNC_TRANSPARENCY_MODE:
        SYS_LOG_INF("KEY_FUNC_TRANSPARENCY_MODE");
#ifdef CONFIG_ANC_HAL
        anc_switch();
#endif
		break;

	case KEY_FUNC_MANUAL_SWITCH_APP:
	{
#ifdef CONFIG_AUDIO_INPUT_APP
        if (!strcmp(APP_ID_LINEIN, app_manager_get_current_app())) {
            SYS_LOG_INF("switch audio input source %s -> %s", APP_ID_LINEIN, APP_ID_I2SRX_IN);
            app_switch(APP_ID_I2SRX_IN, APP_SWITCH_CURR, true);
        } else if (!strcmp(APP_ID_I2SRX_IN, app_manager_get_current_app())) {
            SYS_LOG_INF("switch audio input source %s -> %s", APP_ID_I2SRX_IN, APP_ID_LINEIN);
            app_switch(APP_ID_LINEIN, APP_SWITCH_CURR, true);
        } else
#endif
        {

#ifdef CONFIG_USOUND_APP
            /* when usound is running, couldn't switch to leaudio/btmusic */
            if (!strcmp(APP_ID_USOUND, app_manager_get_current_app())) {
                break;
            }
#endif
            if ((!strcmp(APP_ID_LE_AUDIO, app_manager_get_current_app()))
                && (bt_manager_switch_active_app_check() > 0)) {
                extern int leaudio_playback_is_working(void);
                if (leaudio_playback_is_working()) {
                    struct app_msg  msg = {0};
                    msg.type = MSG_BT_EVENT;
                    msg.cmd = BT_AUDIO_STREAM_PRE_STOP;
                    send_async_msg(APP_ID_LE_AUDIO, &msg);
                    os_sleep(120);
                }
            } else if ((!strcmp(APP_ID_BTMUSIC, app_manager_get_current_app()))
                && (bt_manager_switch_active_app_check() > 0)) {
                extern int bt_music_playback_is_working(void);
                if (bt_music_playback_is_working()) {
                    struct app_msg  msg = {0};
                    msg.type = MSG_BT_EVENT;
                    msg.cmd = BT_AUDIO_STREAM_PRE_STOP;
                    send_async_msg(APP_ID_BTMUSIC, &msg);
                    os_sleep(120);
                }
            }
            bt_manager_switch_active_app();
        }
		break;
	}

	case KEY_FUNC_SWITCH_AUDIO_INPUT:
	{
#ifdef CONFIG_AUDIO_INPUT_APP
		if (strcmp(APP_ID_LINEIN, app_manager_get_current_app())) {
			app_switch(APP_ID_LINEIN, APP_SWITCH_CURR, true);
		}
#endif
		break;
	}
	case KEY_FUNC_EXIT_BQB_TEST_MODE:
	{
        SYS_LOG_INF("KEY_FUNC_EXIT_BQB_TEST_MODE");
		system_exit_bqb_test_mode();
	}
		break;	

	default:
		break;
	}

}

void system_key_event_handle(struct app_msg *msg)
{
	SYS_LOG_INF("msg->cmd 0x%x\n", msg->cmd);

	if (msg->cmd == KEY_FUNC_POWER_OFF)
	{
		system_app_enter_poweroff(0);
		return;
	}

#ifdef CONFIG_ANC_HAL
    if(msg->cmd == KEY_FUNC_TRANSPARENCY_MODE){
        SYS_LOG_INF("KEY_FUNC_TRANSPARENCY_MODE");
        if(tws_m2s_message(TWS_EVENT_SYSTEM_KEY_CTRL, msg->cmd) == 0 ||
           tws_s2m_message(TWS_EVENT_SYSTEM_KEY_CTRL, msg->cmd) == 0){
            anc_switch();
            return;
        } 
    }
#endif

	if (tws_s2m_message(TWS_EVENT_SYSTEM_KEY_CTRL, msg->cmd) == 0)
		return;

	system_key_func_proc(msg->cmd, false);
}

int system_tws_event_handle(struct app_msg *msg)
{
	system_app_context_t*  manager = system_app_get_context();
	tws_message_t* message = (tws_message_t*)msg->ptr;
    int ret_val = 0;

    switch (message->cmd_id)
    {
        case TWS_EVENT_SYSTEM_KEY_CTRL:
        {
            system_key_func_proc(message->cmd_data[0], true);		
            break;
        }

        case TWS_EVENT_DIAL_PHONE_NO:
        {
            bt_manager_hfp_dial_number((char*)message->cmd_data);
            break;
        }

        case TWS_EVENT_SYS_NOTIFY:
        {
            system_slave_notify_proc((char*)message->cmd_data);
            break;
        }
		
        case TWS_EVENT_SET_VOICE_LANG:
        {
            system_tws_set_voice_language(message->cmd_data[0]);
            break;
        }

        case TWS_EVENT_KEY_TONE_PLAY:
        {
            system_tws_key_tone_play(message->cmd_data);
            break;
        }

        case TWS_EVENT_SYNC_LOW_LATENCY_MODE:
        {
            audiolcy_set_latency_mode(message->cmd_data[0], 1);
            break;
        }

        case TWS_EVENT_POWER_OFF:
        {
            system_app_enter_poweroff(1);
            break;
        }
#ifdef CONFIG_POWER_MANAGER
		case TWS_EVENT_SYNC_CHG_BOX_STATUS:
		{
			system_tws_sync_chg_box_status(message->cmd_data);
			break;
		}
#endif
		case TWS_EVENT_IN_CHARGER_BOX_STATE:
		{
			manager->sys_status.tws_remote_in_charger_box = message->cmd_data[0];
			break;
		}
		case TWS_EVENT_SYNC_STATUS_INFO:
		{	
			bt_manager_tws_sync_app_status_info(message->cmd_data);			
			break;
		}
 		case TWS_EVENT_AP_RECORD_STATUS:
		{
			record_slave_status_set(message->cmd_data[0]);
			break;
		}
    }

    switch (message->cmd_id)
    {
        case TWS_EVENT_BT_MUSIC_KEY_CTRL:
        case TWS_EVENT_SYNC_BT_MUSIC_DAE:
        case TWS_EVENT_KEY_CTRL_COMPLETE:
        case TWS_EVENT_AVDTP_FILTER:
		case TWS_EVENT_REMOTE_KEY_PROC_BEGIN:
		case TWS_EVENT_REMOTE_KEY_PROC_END:

        case TWS_EVENT_BT_CALL_KEY_CTRL:
        case TWS_EVENT_BT_CALL_START_RING:
        case TWS_EVENT_BT_CALL_PRE_RING:
        case TWS_EVENT_BT_MUSIC_START:
        case TWS_EVENT_BT_CALL_STOP:
        {
			char *fg_app = app_manager_get_current_app();
			
			if (!fg_app)
				break;

			if (memcmp(fg_app, "main", strlen("main")) == 0)
				break;
       
			send_async_msg(fg_app, msg);
		    ret_val = 1;
            break;
        }
    }
	
    return ret_val;
}

void system_hotplug_event_handle(struct app_msg *msg)
{
    struct app_msg  msg_tmp = {0};

    switch (msg->cmd)
    {
        case HOTPLUG_USB_DEVICE:
            if ((msg->value == AUDIO_APP_ID_BT_MUSIC) ||
                (msg->value == AUDIO_APP_ID_LE_AUDIO) ||
                (msg->value == AUDIO_APP_ID_USOUND)) {
                if ((!strcmp(APP_ID_LE_AUDIO, app_manager_get_current_app()))
                    && (bt_manager_switch_active_app_check() > 0)) {
                    extern int leaudio_playback_is_working(void);
                    if (leaudio_playback_is_working()) {
                        msg_tmp.type = MSG_BT_EVENT;
                        msg_tmp.cmd = BT_AUDIO_STREAM_PRE_STOP;
                        send_async_msg(APP_ID_LE_AUDIO, &msg_tmp);
                        os_sleep(120);
                    }
                } else if ((!strcmp(APP_ID_BTMUSIC, app_manager_get_current_app()))
                    && (bt_manager_switch_active_app_check() > 0)) {
                    extern int bt_music_playback_is_working(void);
                    if (bt_music_playback_is_working()) {
                        msg_tmp.type = MSG_BT_EVENT;
                        msg_tmp.cmd = BT_AUDIO_STREAM_PRE_STOP;
                        send_async_msg(APP_ID_BTMUSIC, &msg_tmp);
                        os_sleep(120);
                    }
                }

                bt_manager_resume_active_app(msg->value);
            }
            break;

        default:
            break;
    }
}

