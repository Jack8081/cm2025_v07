/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tr_usound app main.
 */

#include "tr_usound.h"
#include "tts_manager.h"
#include "media_mem.h"
#include <sys_wakelock.h>
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif
#ifdef CONFIG_USB_UART_CONSOLE
#include <drivers/console/uart_usb.h>
#endif

#ifdef CONFIG_TOOL
#include "tool_app.h"
#endif

#ifdef CONFIG_USB_HOTPLUG
#include <hotplug_manager.h>
#endif

#ifdef CONFIG_MAGIC_TUNE
#include "drivers/mt.h"
#endif

//#include "./tr_usound_blehid.c"

#include <ui_manager.h>

#define VOL_TABLE_MEMBERS_CNT(x) (sizeof(x) / sizeof(u16_t))
#ifdef CONFIG_TR_USOUND_DOUBLE_SOUND_CARD
extern int soundcard1_soft_vol;
extern int soundcard2_soft_vol;
#endif
//extern int usound_mic_vol;
static struct tr_usound_app_t *p_tr_usound;
#define UAC_MAX_VOL                 0xfff0 //0db
#define UAC_MIN_VOL                 0xe3a0 //-60db


//windows os volume table
const u16_t transm_volume_table_host_windows[] = { 0xe3a3, 0xe871, 0xebc9, 0xee86, 0xf0d8, 0xf2dc, 0xf4a2, 0xf63a, 0xf7aa,
        0xf8fb, 0xfa31, 0xfb50, 0xfc5b, 0xfd55, 0xfe40, 0xff1e, 0xfff0 };

//apple mac os volume table
const u16_t transm_volume_table_host_apple_mac[] = { 0xe3a0, 0xe553, 0xe733, 0xe8e3, 0xeac3, 0xec73, 0xee53, 0xf003, 0xf1e3,
        0xf393, 0xf543, 0xf723, 0xf8d3, 0xfab3, 0xfc63, 0xfe43, 0xfff0 };

u8_t g_tr_usound_app = 0;

//u8_t os_type = 0;  //0: windows系统； 1:苹果mac系统；
static u8_t get_ud_sync_volume(u16_t volume, const u16_t* ptable_dat, u8_t table_size)
{
    u8_t i = 0;

    for (i = 0; i < table_size; i++) {
        if (volume <= ptable_dat[i]) {
            break;
        }
    }

    if (i == table_size) {
        i--;
    }

	if(i == table_size - 1 && volume != ptable_dat[i])
		i = table_size - 2;

	return i;
}

//获取当前host端音量在windows os 17级音量表下级数 (0 + 16级音量)
static u8_t get_usound_vol_level_for_windows(u16_t volume)
{
    u8_t volume_val_cnt = VOL_TABLE_MEMBERS_CNT(transm_volume_table_host_windows);
    return get_ud_sync_volume(volume, transm_volume_table_host_windows, volume_val_cnt);
}
//获取当前host端(apple mac os)音量表下级数 (0 + 16级音量)
static u8_t get_usound_vol_level_for_apple_mac(u16_t volume)
{
    u8_t volume_val_cnt = VOL_TABLE_MEMBERS_CNT(transm_volume_table_host_apple_mac);
    return get_ud_sync_volume(volume, transm_volume_table_host_apple_mac, volume_val_cnt);
}

static void _tr_usound_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	SYS_LOG_INF("playing %d\n", p_tr_usound->playing);
#ifdef CONFIG_TR_USOUND_SPEAKER
	if (!p_tr_usound->playing) {
		usb_hid_control_pause_play();
	}
#endif
	tr_usound_start_play();

}

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
static void _tr_usound_delay_search(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    struct app_msg new_msg = { 0 };

	new_msg.type = MSG_INPUT_EVENT;
	new_msg.cmd = KEY_FUNC_DONGLE_ENTER_SEARCH;
	send_async_msg(app_manager_get_current_app(), &new_msg);
}
#endif

static void _tr_usound_restore_play_state(void)
{
    return;//TODO

	u8_t init_play_state = USOUND_STATUS_PLAYING;
#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		esd_manager_restore_scene(TAG_PLAY_STATE, &init_play_state, 1);
	}
#endif

	if (init_play_state == USOUND_STATUS_PLAYING) {
		if (thread_timer_is_running(&p_tr_usound->monitor_timer)) {
			thread_timer_stop(&p_tr_usound->monitor_timer);
		}
		thread_timer_init(&p_tr_usound->monitor_timer, _tr_usound_delay_resume, NULL);
		thread_timer_start(&p_tr_usound->monitor_timer, 2000, 0);
	}

	SYS_LOG_INF("%d\n", init_play_state);
}

#ifdef CONFIG_OTA_BACKEND_USBHID
extern uint8_t hid_is_updating_remote(void);
#endif
static void _usb_audio_event_callback_handle(u8_t info_type, int pstore_info)
{
	struct app_msg msg = { 0 };
    
#ifdef CONFIG_SUPPORT_MUTE_SHORTCUT_KEY_PAIR
    static u8_t mute_cnt = 0;
    static u32_t first_mute_time;
    u32_t now_mute_time;
#endif

#ifdef CONFIG_OTA_BACKEND_USBHID
    if (hid_is_updating_remote() == 1) 
    {
        return;
    }
#endif

	switch (info_type) {
	case USOUND_SYNC_HOST_MUTE:
#ifdef CONFIG_BT_MIC_GAIN_ADJUST //无线麦场景不需要音量调节
#ifdef CONFIG_USB_DOWNLOAD_LISTEN_SUPPORT
        soundcard_sink_mute_flag = 1; 
#endif
        break;
#endif
#ifdef CONFIG_SUPPORT_MUTE_SHORTCUT_KEY_PAIR
        if(first_mute_time == 0)
        {
            first_mute_time = k_cycle_get_32();
        }
        now_mute_time = k_cycle_get_32();

        if(((now_mute_time - first_mute_time) / 24000) <= 1200)
        {
            mute_cnt += 1;
            if(mute_cnt == 4)
            {
                msg.type = MSG_USOUND_EVENT;
                msg.cmd = MSG_USOUND_MUTE_SHORTCUT;
                send_async_msg(APP_ID_TR_USOUND, &msg);
            }
        }
        else
        {
            mute_cnt = 1;
            first_mute_time = k_cycle_get_32();
        }
#endif
        msg.type = MSG_USOUND_EVENT;
        msg.cmd = MSG_USOUND_STREAM_MUTE;
        send_async_msg(APP_ID_TR_USOUND, &msg);
		break;

	case USOUND_SYNC_HOST_UNMUTE:
#ifdef CONFIG_BT_MIC_GAIN_ADJUST //无线麦场景不需要音量调节
#ifdef CONFIG_USB_DOWNLOAD_LISTEN_SUPPORT
        soundcard_sink_mute_flag = 0; 
#endif
        break;
#endif
#ifdef CONFIG_SUPPORT_MUTE_SHORTCUT_KEY_PAIR
        if(first_mute_time == 0)
        {
            first_mute_time = k_cycle_get_32();
        }
        now_mute_time = k_cycle_get_32();

        if(((now_mute_time - first_mute_time) / 24000) <= 1200)
        {
            mute_cnt += 1;
            if(mute_cnt == 4)
            {
                msg.type = MSG_USOUND_EVENT;
                msg.cmd = MSG_USOUND_MUTE_SHORTCUT;
                send_async_msg(APP_ID_TR_USOUND, &msg);
            }
        }
        else
        {
            mute_cnt = 1;
            first_mute_time = k_cycle_get_32();
        }
#endif
        msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_STREAM_UNMUTE;
        send_async_msg(APP_ID_TR_USOUND, &msg);
		break;

	case USOUND_SYNC_HOST_VOL_TYPE:
	{
#ifdef CONFIG_TR_USOUND_DOUBLE_SOUND_CARD
        break;
#endif
#ifdef CONFIG_SUPPORT_VOL_SYNC_HOST_TO_DEV
        if(pstore_info > UAC_MAX_VOL || pstore_info < UAC_MIN_VOL)
        {
            SYS_LOG_WRN("error host vol:0x%x\n", pstore_info);
            break;
        }
#endif
#ifdef CONFIG_BT_MIC_GAIN_ADJUST //无线麦场景不需要音量调节
#ifdef CONFIG_USB_DOWNLOAD_LISTEN_SUPPORT
        extern u8_t soundcard_sink_vol;
        if(usb_device_get_host_os_type_flag() == 2)
		{
			soundcard_sink_vol = get_usound_vol_level_for_apple_mac(pstore_info);
			SYS_LOG_INF("%d: mac %x %d\n", __LINE__, pstore_info, soundcard_sink_vol);
		}
		else /* if(usb_get_host_os_type_flag() == 1) //windows */
		{
			soundcard_sink_vol = get_usound_vol_level_for_windows(pstore_info);
			SYS_LOG_INF("%d: windows %x %d\n", __LINE__, pstore_info, soundcard_sink_vol);
		}
#endif
        break;
#endif
		u8_t volume_level;
        if(usb_device_get_host_os_type_flag() == 2)
		{
			volume_level = get_usound_vol_level_for_apple_mac(pstore_info);
			SYS_LOG_INF("%d: mac %x %d\n", __LINE__, pstore_info, volume_level);
		}
		else /* if(usb_get_host_os_type_flag() == 1) //windows */
		{
			volume_level = get_usound_vol_level_for_windows(pstore_info);
			SYS_LOG_INF("%d: windows %x %d\n", __LINE__, pstore_info, volume_level);
		}
        
        SYS_LOG_INF("level %d volume_req_level %d \n", volume_level, p_tr_usound->volume_req_level);
        p_tr_usound->host_vol = volume_level;
		if (p_tr_usound->volume_req_type) {
			if (p_tr_usound->volume_req_level == p_tr_usound->host_vol) {
				system_volume_set(AUDIO_STREAM_TR_USOUND, p_tr_usound->host_vol, false);
			    p_tr_usound->current_volume_level = p_tr_usound->host_vol;
                p_tr_usound->host_vol_sync = 0;
				p_tr_usound->volume_req_type = USOUND_VOLUME_NONE;
			} else {
				if (p_tr_usound->volume_req_type == USOUND_VOLUME_INC) {
					if(p_tr_usound->volume_req_level > p_tr_usound->host_vol) {
						usb_hid_control_volume_inc();
                        p_tr_usound->host_vol_sync = 1;
					} else if(p_tr_usound->volume_req_level > p_tr_usound->host_vol - 2) {
						system_volume_set(AUDIO_STREAM_TR_USOUND, p_tr_usound->host_vol, false);
			            p_tr_usound->current_volume_level = p_tr_usound->host_vol;
                        p_tr_usound->host_vol_sync = 0;
						p_tr_usound->volume_req_type = USOUND_VOLUME_NONE;
					}
				} else if(p_tr_usound->volume_req_type == USOUND_VOLUME_DEC) {
					if(p_tr_usound->volume_req_level < p_tr_usound->host_vol) {
						usb_hid_control_volume_dec();
                        p_tr_usound->host_vol_sync = 1;
					} else if(p_tr_usound->volume_req_level < p_tr_usound->host_vol + 2) {
						system_volume_set(AUDIO_STREAM_TR_USOUND, p_tr_usound->host_vol, false);
			            p_tr_usound->current_volume_level = p_tr_usound->host_vol;
                        p_tr_usound->host_vol_sync = 0;
						p_tr_usound->volume_req_type = USOUND_VOLUME_NONE;
					}
				}
			}
		} else {
            if(p_tr_usound->current_volume_level == volume_level)
                break;
            
            //system_volume_set(AUDIO_STREAM_LE_AUDIO_MUSIC, volume_level, false);
			p_tr_usound->current_volume_level = volume_level;
			p_tr_usound->volume_req_type = USOUND_VOLUME_NONE;
            
            msg.type = MSG_USOUND_EVENT;
		    msg.cmd = MSG_USOUND_HOST_VOL_SYNC;
		    send_async_msg(APP_ID_TR_USOUND, &msg);
		}
		break;
	}
    
    case UMIC_SYNC_HOST_MUTE:
        SYS_LOG_INF("%d: USOUND_SYNC_HOST_MIC_MUTE\n", __LINE__);
        if(!p_tr_usound->host_mic_muted)
        {
            //media_player_set_mic_gain(NULL, 0);
            set_umic_soft_vol(0);
            p_tr_usound->host_mic_muted = 1;
        }
        break;

    case UMIC_SYNC_HOST_UNMUTE:
        SYS_LOG_INF("%d: USOUND_SYNC_HOST_MIC_UNMUTE\n", __LINE__);
        if(p_tr_usound->host_mic_muted)
        {
            //media_player_set_mic_gain(NULL, (p_tr_usound->host_mic_volume * 100) / 16);
            p_tr_usound->host_mic_muted = 0;
            set_umic_soft_vol(p_tr_usound->host_mic_volume);
        }
        break;

    case UMIC_SYNC_HOST_VOL_TYPE:
        SYS_LOG_INF("%d: USOUND_SYNC_HOST_MIC_VOL_TYPE\n", __LINE__);
        
        if(usb_device_get_host_os_type_flag() == 2)
		{
			p_tr_usound->host_mic_volume = get_usound_vol_level_for_apple_mac(pstore_info);
			SYS_LOG_INF("%d: mac %x %d\n", __LINE__, pstore_info, p_tr_usound->host_mic_volume);
		}
		else /* if(usb_get_host_os_type_flag() == 1) //windows */
		{
			p_tr_usound->host_mic_volume = get_usound_vol_level_for_windows(pstore_info);
			SYS_LOG_INF("%d: windows %x %d\n", __LINE__, pstore_info, p_tr_usound->host_mic_volume);
		}
        
        if(p_tr_usound->host_mic_muted)
            break;
        //media_player_set_mic_gain(NULL, (p_tr_usound->host_mic_volume * 100) / 16);
        set_umic_soft_vol(p_tr_usound->host_mic_volume);
        break;
	
    case USOUND_STREAM_STOP:
		p_tr_usound->usb_sink_started = 0;
		msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_STREAM_STOP;
		send_async_msg(APP_ID_TR_USOUND, &msg);
		break;

	case USOUND_STREAM_START:
		p_tr_usound->usb_sink_started = 1;
		msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_STREAM_START;
		send_async_msg(APP_ID_TR_USOUND, &msg);
		break;

	case USOUND_UPLOAD_STREAM_START:
		p_tr_usound->usb_source_started = 1;
		msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_UPLOAD_STREAM_START;
		send_async_msg(APP_ID_TR_USOUND, &msg);
		break;

	case USOUND_UPLOAD_STREAM_STOP:
		p_tr_usound->usb_source_started = 0;
		msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_UPLOAD_STREAM_STOP;
		send_async_msg(APP_ID_TR_USOUND, &msg);
		break;

	case USOUND_SWITCH_TO_IOS:
		msg.type = MSG_SWITH_IOS;
		msg.cmd = 0;
		send_async_msg(APP_ID_TR_USOUND, &msg);
		break;

    case USOUND_INCALL_RING:
        SYS_LOG_INF("incall ring\n");
        msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_INCALL_RING;
		send_async_msg(APP_ID_TR_USOUND, &msg);
        break;

    case USOUND_INCALL_HOOK_UP:
        SYS_LOG_INF("incall hook up\n");
        msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_INCALL_HOOK_UP;
		send_async_msg(APP_ID_TR_USOUND, &msg);
        break;

    case USOUND_INCALL_HOOK_HL:
        SYS_LOG_INF("incall hook hl\n");
        msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_INCALL_HOOK_HL;
		send_async_msg(APP_ID_TR_USOUND, &msg);
        break;

    case USOUND_INCALL_MUTE:
        SYS_LOG_INF("incall mute\n");
        msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_INCALL_MUTE;
		send_async_msg(APP_ID_TR_USOUND, &msg);
        break;

    case USOUND_INCALL_UNMUTE:
        SYS_LOG_INF("incall unmute\n");
        msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_INCALL_UNMUTE;
		send_async_msg(APP_ID_TR_USOUND, &msg);
        break;

    case USOUND_INCALL_OUTGOING:
        SYS_LOG_INF("incall outing\n");
        msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_INCALL_OUTGOING;
		send_async_msg(APP_ID_TR_USOUND, &msg);
        break;

    case USOUND_DEFINE_CMD_CONNECT_PAIRED_DEV:
        SYS_LOG_INF("define cmd connect paired dev\n");
        msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_DEFINE_CMD_CONNECT_PAIRED_DEV;
		send_async_msg(APP_ID_TR_USOUND, &msg);
        break;

    default:
		break;
	}
}

//extern u8_t g_tr_usound_app;

#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
static void _tr_usound_cis_start_stop(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    if (!p_tr_usound)
        return;

#ifdef CONFIG_OTA_BACKEND_USBHID
    if(hid_is_updating_remote() == 1)
    {
        return;
    }
#endif

    struct bt_audio_chan *download_chan = &p_tr_usound->dev[0].chan[SPK_CHAN];
    struct _le_dev_t *upload_dev = &p_tr_usound->dev[0];

    if(!p_tr_usound->usb_download_started)
    {
        if(p_tr_usound->downchan_mute_data_detect_disable) {
            return;
        }

        if(download_chan->handle && upload_dev->mic_chan_started == 0)
            tr_bt_manager_audio_stream_disable_single(download_chan);
    }
    else
    {
        if(download_chan->handle)
             tr_bt_manager_audio_stream_enable_single(download_chan, BT_AUDIO_CONTEXT_UNSPECIFIED);
    }
}
#endif
static int _tr_usound_init(void)
{
	if (p_tr_usound)
		return 0;

	CFG_Struct_BTMusic_Multi_Dae_Settings multidae_setting;
    g_tr_usound_app = 1;
	p_tr_usound = app_mem_malloc(sizeof(struct tr_usound_app_t));
	if (!p_tr_usound) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}

	tip_manager_lock();
	
    extern void sys_manager_enable_audio_tone(bool enable);
    extern void sys_manager_enable_audio_voice(bool enable);
    sys_manager_enable_audio_tone(false);
    sys_manager_enable_audio_voice(false);

	memset(p_tr_usound, 0, sizeof(struct tr_usound_app_t));
    
	app_config_read
    (
        CFG_ID_BT_MUSIC_DAE,
        &p_tr_usound->dae_enalbe, offsetof(CFG_Struct_BT_Music_DAE, Enable_DAE), sizeof(cfg_uint8)
    );

	app_config_read
    (
        CFG_ID_BT_MUSIC_DAE_AL,
        p_tr_usound->cfg_dae_data, 0, CFG_SIZE_BT_MUSIC_DAE_AL
    );

	app_config_read
    (
        CFG_ID_BTMUSIC_MULTI_DAE_SETTINGS,
        &multidae_setting, 0, sizeof(CFG_Struct_BTMusic_Multi_Dae_Settings)
    );

	p_tr_usound->multidae_enable = multidae_setting.Enable;
	p_tr_usound->dae_cfg_nums = multidae_setting.Cur_Dae_Num + 1;
	p_tr_usound->current_dae = 0;

    /* 从vram读回当前音效 */
    if (p_tr_usound->multidae_enable) {
        if (property_get(BTMUSIC_MULTI_DAE, &p_tr_usound->current_dae, sizeof(uint8_t)) < 0)
            p_tr_usound->current_dae = 0;

        if (p_tr_usound->current_dae > p_tr_usound->dae_cfg_nums)
            p_tr_usound->current_dae = 0;
    }

	property_set(BTMUSIC_MULTI_DAE, &p_tr_usound->current_dae, sizeof(uint8_t));

    p_tr_usound->host_mic_volume = 16; //usound mic init vol
    //usound_mic_vol = p_tr_usound->host_mic_volume;
	
    tr_usound_view_init(BTMUSIC_VIEW, false);

#ifdef CONFIG_TR_USOUND_MIC
	//thread_timer_init(&p_tr_usound->playback_delay_timer,  tr_usound_delayed_start_or_stop_playback_handle, NULL);
#endif
    
    usb_audio_init(_usb_audio_event_callback_handle);

	p_tr_usound->current_volume_level = 16;//默认第一次插入最大档

    set_umic_soft_vol(p_tr_usound->host_mic_volume);
#ifdef CONFIG_TR_USOUND_DOUBLE_SOUND_CARD
    if(!soundcard1_soft_vol && !soundcard2_soft_vol)
    {
        soundcard1_soft_vol = 8; 
        soundcard2_soft_vol = 8;
    }
#endif
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
    thread_timer_init(&p_tr_usound->usb_cis_start_stop_work, _tr_usound_cis_start_stop, NULL);
#endif
    sys_wake_lock(FULL_WAKE_LOCK);
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    ui_event_led_display(UI_EVENT_CUSTOMED_4);
	thread_timer_init(&p_tr_usound->auto_search_timer, _tr_usound_delay_search, NULL);
	thread_timer_start(&p_tr_usound->auto_search_timer, 10000, 0);
#endif
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE_MAX, "tr_usound");
#endif
    SYS_LOG_INF("init ok\n");
	return 0;
}

extern bool bt_manager_startup_reconnect(void);

void _tr_usound_exit(void)
{
	if (!p_tr_usound)
		goto exit;

	audio_system_save_volume(CFG_USOUND_VOLUME, p_tr_usound->current_volume_level);
	audio_system_save_volume(CFG_TONE_VOLUME, p_tr_usound->current_volume_level);

	if (thread_timer_is_running(&p_tr_usound->monitor_timer))
		thread_timer_stop(&p_tr_usound->monitor_timer);

	if (thread_timer_is_running(&p_tr_usound->key_timer))
		thread_timer_stop(&p_tr_usound->key_timer);

	if (thread_timer_is_running(&p_tr_usound->delay_play_timer))
		thread_timer_stop(&p_tr_usound->delay_play_timer);

	if (thread_timer_is_running(&p_tr_usound->tts_monitor_timer))
		thread_timer_stop(&p_tr_usound->tts_monitor_timer);
	
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    if (thread_timer_is_running(&p_tr_usound->auto_search_timer))
		thread_timer_stop(&p_tr_usound->auto_search_timer);
#endif


	if(p_tr_usound->need_resume_key){
		ui_key_filter(false);
	}

	if (p_tr_usound->playing) {
		usb_hid_control_pause_play();
	}
   
    tr_usound_stop_playback();
	tr_usound_exit_playback();

	tr_usound_stop_capture();
	tr_usound_exit_capture();

    //hid_ble_command_process_exit();
	usb_audio_deinit();

    //iap2_protocol_deinit();
    
    tr_usound_view_deinit();

    thread_timer_stop(&p_tr_usound->host_vol_sync_timer);
    //thread_timer_stop(&p_tr_usound->repair_delay_timer);
	
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
    thread_timer_stop(&p_tr_usound->usb_cis_start_stop_work);
#endif
#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif
    sys_wake_unlock(FULL_WAKE_LOCK);
    
    app_mem_free(p_tr_usound);

	p_tr_usound = NULL;
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE_MAX, "tr_usound");
#endif

	tip_manager_unlock();

exit:
	app_manager_thread_exit(app_manager_get_current_app());

    g_tr_usound_app = 0;
	
    SYS_LOG_INF("exit ok\n");
}

struct tr_usound_app_t *tr_usound_get_app(void)
{
	return p_tr_usound;
}

static void _tr_usound_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = { 0 };

	bool terminated = false;

    bt_manager_halt_phone();

	if (_tr_usound_init()) {
		SYS_LOG_ERR("init failed");
		_tr_usound_exit();
		return;
	}

#ifdef CONFIG_MAGIC_TUNE
#ifdef CONFIG_MAGIC_TUNE_CM001X1
    const struct device *mt_dev;
    mt_dev = device_get_binding(CONFIG_MT_DEV_NAME);
    if (mt_dev != NULL) {
        mt_init(mt_dev);
    }
#endif
#endif

#ifdef CONFIG_PLAYTTS
	tts_manager_lock();
	tts_manager_wait_finished(true);
#endif

	bt_manager_audio_stream_restore(BT_TYPE_LE);
	
    _tr_usound_restore_play_state();

	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("\ntype %d, value %d\n", msg.type, msg.value);
			switch (msg.type) {
			case MSG_INPUT_EVENT:
				SYS_LOG_INF("USOUND INPUT EVENT\n");
				tr_usound_input_event_proc(&msg);
				break;

			case MSG_TTS_EVENT:
				tr_usound_tts_event_proc(&msg);
				break;

			case MSG_USOUND_EVENT:
				SYS_LOG_INF("USOUND EVENT\n");
				tr_usound_event_proc(&msg);
				break;

			case MSG_BT_EVENT:
				tr_usound_bt_event_proc(&msg);
				break;

			case MSG_EXIT_APP:
				_tr_usound_exit();
				terminated = true;
				break;
			
			case MSG_SWITH_IOS:
				SYS_LOG_INF("MSG_USOUND_SWITCH_TO_IOS\n");
				//if(iap2_detect_enable() == 1)
				{
					//iap2_protocol_init();
				}
				break;

            default:
				break;
			}
			if (msg.callback)
				msg.callback(&msg, 0, NULL);
		}
		if (!terminated)
			thread_timer_handle_expired();
	}
#ifdef CONFIG_PLAYTTS
    tts_manager_unlock();
#endif
}

APP_DEFINE(tr_usound, share_stack_area, sizeof(share_stack_area),
	CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	_tr_usound_main_loop, NULL);

