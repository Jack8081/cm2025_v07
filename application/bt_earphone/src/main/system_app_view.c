/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app view
 */
#include <mem_manager.h>
#include <msg_manager.h>
#include <ui_manager.h>
#include <system_notify.h>

#include "power_manager.h"
#include "app_ui.h"
#include "tts_manager.h"
#include "system_app.h"

#include <bt_manager.h>
#include "app_common.h"
#include "led_display.h"
#include <usb/usb_otg.h>

#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
#include <drivers/gpio.h>
#endif
#endif

#define NOTIFY_SYNC_WAIT_MS (100000)  //us

#define MAIN_STATUS_FG_BTCALL           (1<<0)
#define MAIN_STATUS_FG_NOT_BTCALL        (1<<1)
#define MAIN_STATUS_IN_VOICE_ASSIST     (1 << 2)
#define MAIN_STATUS_NOT_IN_VOICE_ASSIST (1 << 3)
#define MAIN_STATUS_IN_PAIR_MODE        (1 << 4)
#define MAIN_STATUS_NOT_IN_PAIR_MODE     (1 << 5)
#define MAIN_STATUS_IN_TWS_PAIR_SEARCH     (1 << 8)
#define MAIN_STATUS_NOT_IN_TWS_PAIR_SEARCH (1 << 9)
#define MAIN_STATUS_PHONE_CONNECTED       (1 << 10)
#define MAIN_STATUS_NOT_PHONE_CONNECTED   (1 << 11)
#define MAIN_STATUS_TWS_MODE       (1 << 12)
#define MAIN_STATUS_NOT_TWS_MODE   (1 << 13)
#define MAIN_STATUS_SUPPORT_BQB_MODE       (1 << 14)
#define MAIN_STATUS_NOT_SUPPORT_BQB_MODE   (1 << 15)
#define MAIN_STATUS_IN_FRONT_CHARGE       (1 << 16)
#define MAIN_STATUS_NOT_IN_FRONT_CHARGE   (1 << 17)
#define MAIN_STATUS_ENABLE_TWS_CONNECT    (1 << 18)
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
#define MAIN_STATUS_IN_BQB_MODE   (1 << 19)
#endif


const ui_key_func_t common_func_table[] = 
{
    { KEY_FUNC_POWER_OFF, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE
    },
    { KEY_FUNC_DIAL_LAST_NO, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL| 
        MAIN_STATUS_NOT_IN_VOICE_ASSIST|
        MAIN_STATUS_PHONE_CONNECTED
    },
    { KEY_FUNC_START_VOICE_ASSIST, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST|
        MAIN_STATUS_PHONE_CONNECTED
    },
    { KEY_FUNC_STOP_VOICE_ASSIST, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_IN_VOICE_ASSIST
    },
    { KEY_FUNC_HID_PHOTO_SHOT, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST|
        MAIN_STATUS_PHONE_CONNECTED
    },
    { KEY_FUNC_HID_CUSTOM_KEY, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST|
        MAIN_STATUS_PHONE_CONNECTED
    },
    { KEY_FUNC_ENTER_PAIR_MODE, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL | 
        MAIN_STATUS_NOT_IN_VOICE_ASSIST|
        MAIN_STATUS_NOT_IN_PAIR_MODE|
        MAIN_STATUS_NOT_IN_TWS_PAIR_SEARCH
    },
    { KEY_FUNC_TWS_PAIR_SEARCH, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_NOT_IN_TWS_PAIR_SEARCH|
        MAIN_STATUS_NOT_TWS_MODE|
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST |
        MAIN_STATUS_ENABLE_TWS_CONNECT
    },
    { KEY_FUNC_CLEAR_PAIRED_LIST, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_NOT_IN_TWS_PAIR_SEARCH|
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST
    },
    { KEY_FUNC_CLEAR_PAIRED_LIST_IN_PAIR_MODE, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_IN_PAIR_MODE
    },
    { KEY_FUNC_CLEAR_PAIRED_LIST_IN_FRONT_CHARGE, 
        MAIN_STATUS_IN_FRONT_CHARGE
    },
    { KEY_FUNC_CLEAR_PAIRED_LIST_IN_UNLINKED, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_NOT_PHONE_CONNECTED
    },
    { KEY_FUNC_START_RECONNECT, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_NOT_IN_TWS_PAIR_SEARCH|
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST|
        MAIN_STATUS_IN_PAIR_MODE
    },
    { KEY_FUNC_ENTER_BQB_TEST_MODE, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST|
        MAIN_STATUS_SUPPORT_BQB_MODE
    },
    { KEY_FUNC_ENTER_BQB_TEST_IN_PAIR_MODE, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST|
        MAIN_STATUS_SUPPORT_BQB_MODE|
        MAIN_STATUS_IN_PAIR_MODE
    },
    { KEY_FUNC_SWITCH_VOICE_LANG, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE
    },
    { KEY_FUNC_SWITCH_VOICE_LANG_IN_UNLINKED, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_NOT_PHONE_CONNECTED
    },
    { KEY_FUNC_SWITCH_VOICE_LANG_IN_PAIR_MODE, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_IN_PAIR_MODE
    },
    { KEY_FUNC_SWITCH_LOW_LATENCY_MODE, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE
    },
    { KEY_FUNC_START_PRIVMA_TALK, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST
    },
    { KEY_FUNC_NMA_KEY_FIRST, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST
    },
    { KEY_FUNC_NMA_KEY_SECOND, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST
    },
    { KEY_FUNC_TMA_ONE_KEY_TO_REQUEST, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST
    },
    { KEY_FUNC_TRANSPARENCY_MODE, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE
    },
    { KEY_FUNC_MANUAL_SWITCH_APP,
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST
    },
    { KEY_FUNC_SWITCH_AUDIO_INPUT,
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST
    },

    { KEY_FUNC_EXIT_BQB_TEST_MODE, 
        MAIN_STATUS_NOT_IN_FRONT_CHARGE | 
        MAIN_STATUS_FG_NOT_BTCALL|
        MAIN_STATUS_NOT_IN_VOICE_ASSIST
    },
	
}; 

char voice_name[CFG_MAX_VOICE_NAME_LEN + CFG_MAX_VOICE_FMT_LEN];
static ui_key_map_t* common_keymap;
static bool is_tts_finish = false;
static bool is_phone_num_playing = false;

void system_do_clear_notify(void);

int system_app_ui_event(int event)
{
	SYS_LOG_INF(" %d", event);
	ui_manager_send_async(MAIN_VIEW, MSG_VIEW_PAINT, event);
	return 0;
}

void system_app_volume_show(struct app_msg *msg)
{
    return;
}

bool system_is_local_event(u32_t ui_event, bool led_disp_only)
{
    bool ret = false;

    switch(ui_event){
        case UI_EVENT_POWER_OFF:
        case UI_EVENT_POWER_ON:
        case UI_EVENT_STANDBY:
        case UI_EVENT_WAKE_UP:
        case UI_EVENT_BATTERY_LOW:
        case UI_EVENT_BATTERY_LOW_EX:
        case UI_EVENT_BATTERY_TOO_LOW:
        case UI_EVENT_REPEAT_BAT_LOW:
        case UI_EVENT_CHARGE_START:
        case UI_EVENT_CHARGE_STOP:
        case UI_EVENT_CHARGE_FULL:
        case UI_EVENT_FRONT_CHARGE_POWON:
        case UI_EVENT_TWS_WAIT_PAIR:
        case UI_EVENT_BT_UNLINKED:
            ret = true;       
            break;

        /* BT status LED display could be different for TWS master and slave */
        case UI_EVENT_ENTER_PAIR_MODE:
        case UI_EVENT_BT_WAIT_CONNECT:
        case UI_EVENT_BT_CONNECTED:
        case UI_EVENT_TWS_CONNECTED:
            ret = (led_disp_only) ? true : false;
            break;
            
        default:
            break;
    }
    return ret;
}

bool system_need_sync_event(u32_t ui_event, bool led_disp_only)
{
    bool ret = true;
    if (system_is_local_event(ui_event, led_disp_only) || 
        (bt_manager_tws_get_dev_role() != BTSRV_TWS_MASTER))
    {
        ret = false;
    }
    return ret;
}

void notify_slave_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{    
    system_app_context_t*  manager = system_app_get_context();
    int hfp_state;
    bool is_incoming = true;

    hfp_state = bt_manager_hfp_get_status();

	if((hfp_state != BT_STATUS_INCOMING) && (hfp_state != BT_STATUS_3WAYIN))
    {
        SYS_LOG_INF("slave stop tip or tts, %d\n", __LINE__);
        tip_manager_stop();
        tts_manager_stop(NULL);
        tts_manager_wait_finished(true);
        is_incoming = false;
    }

    /* 
     * all led notify has set to led ctrl
     */
    if (!is_incoming)
    {
        SYS_LOG_INF("thread_timer_stop");
        if (thread_timer_is_running(&manager->notify_slave_timer))
            thread_timer_stop(&manager->notify_slave_timer);
    }

}

u8_t system_slave_led_display(CFG_Type_Event_Notify* cfg_notify)
{
    system_notify_ctrl_t notify_ctrl;

    memset(&notify_ctrl, 0, sizeof(notify_ctrl));

    notify_ctrl.cfg_notify   = *cfg_notify;
    notify_ctrl.layer_id     = led_get_layer_id(cfg_notify->Event_Type);
    notify_ctrl.flags.led_disp_only = true;
    led_notify_start_display(&notify_ctrl);
    return 0;
}

u8_t system_slave_tone_play(CFG_Type_Event_Notify* cfg_notify, u64_t sync_time)
{
    system_app_context_t*  manager = system_app_get_context();
    char name[CFG_MAX_VOICE_NAME_LEN + CFG_MAX_VOICE_FMT_LEN];
	uint64_t bt_clk = 0;

	tws_runtime_observer_t * observer = btif_tws_get_tws_observer();
	bt_clk = observer->get_bt_clk_us();
	SYS_LOG_INF("event_type:%d curr_time:%u_%u play_time:%u_%u",
		cfg_notify->Event_Type, (uint32_t)((bt_clk >> 32) & 0xffffffff), (uint32_t)(bt_clk & 0xffffffff),
		(uint32_t)((sync_time >> 32) & 0xffffffff), (uint32_t)(sync_time & 0xffffffff));

    if (audio_tone_get_name(cfg_notify->Tone_Play, name) != NULL)
    {             
        SYS_LOG_INF("tone_name:%s", name);
        tip_manager_play(name, sync_time);
    }

    if ((cfg_notify->Event_Type == UI_EVENT_BT_CALL_INCOMING) ||
        (cfg_notify->Event_Type == UI_EVENT_BT_CALL_3WAYIN))
    {
        /*
         * init timer
         */
        if (!thread_timer_is_running(&manager->notify_slave_timer))
        {
            SYS_LOG_INF("timer start");
            thread_timer_init(&manager->notify_slave_timer, notify_slave_timer_handler, NULL);
            thread_timer_start(&manager->notify_slave_timer, 20, 20);
        }
    }
    return 0;		
}

void system_slave_voice_play(CFG_Type_Event_Notify* cfg_notify, u8_t *voice_list, u64_t sync_time)
{
    system_app_context_t*  manager = system_app_get_context();
    char name[CFG_MAX_VOICE_NAME_LEN + CFG_MAX_VOICE_FMT_LEN];
	uint64_t bt_clk = 0;

	tws_runtime_observer_t * observer = btif_tws_get_tws_observer();
	bt_clk = observer->get_bt_clk_us();
	SYS_LOG_INF("event_type:%d curr_time:%u_%u play_time:%u_%u",
		cfg_notify->Event_Type, (uint32_t)((bt_clk >> 32) & 0xffffffff), (uint32_t)(bt_clk & 0xffffffff),
		(uint32_t)((sync_time >> 32) & 0xffffffff), (uint32_t)(sync_time & 0xffffffff));

    if ((cfg_notify->Event_Type == UI_EVENT_BT_CALL_INCOMING) ||
        (cfg_notify->Event_Type == UI_EVENT_BT_CALL_3WAYIN))
    {
    
        SYS_LOG_INF("phone_num:%s", voice_list);
        
        strcpy(name, "V_NO_%c.act");
        if (manager->alter_voice_language == VOICE_LANGUAGE_2)
        {
            name[0] = 'E';
        }

        tip_manager_stop();
        tts_manager_play_with_time(voice_list, PLAY_IMMEDIATELY | TTS_MODE_TELE_NUM, sync_time, name);

        /*
         * init timer
         */
        if (!thread_timer_is_running(&manager->notify_slave_timer))
        {
            SYS_LOG_INF("timer start");
            thread_timer_init(&manager->notify_slave_timer, notify_slave_timer_handler, NULL);
            thread_timer_start(&manager->notify_slave_timer, 20, 20);
        }

        return ;
    }

    if (audio_voice_get_name_ex(cfg_notify->Voice_Play, name))
    {
        SYS_LOG_INF("voice_name:%s", name);
        tts_manager_play_with_time(name, PLAY_IMMEDIATELY, sync_time, NULL);
    }
}

void system_slave_notify_proc(uint8_t *slave_notify)
{
    system_app_context_t*  manager = system_app_get_context();
    notify_sync_t notify_event; //= (notify_sync_t *)slave_notify;
    CFG_Type_Event_Notify* cfg_notify = NULL;

    if(manager->sys_status.in_power_off_stage)
    {
        SYS_LOG_INF("in_power_off_stage..");
        return;
    }

    memcpy(&notify_event, slave_notify, sizeof(notify_event));

    SYS_LOG_DBG("event_type:%d, event_stage:%d, time:%u_%u", 
                notify_event.event_type, 
                notify_event.event_stage, 
                (uint32_t)((notify_event.sync_time >> 32) & 0xffffffff),
                (uint32_t)(notify_event.sync_time & 0xffffffff)
               );

    if (notify_event.event_type == UI_EVENT_TWS_CONNECTED) {
        system_do_clear_notify();
    }
    
	cfg_notify =  (CFG_Type_Event_Notify*)system_notify_get_config(notify_event.event_type);
    if(NULL == cfg_notify)
    {
        SYS_LOG_WRN("%d, cfg_notify is null.", __LINE__);
        return;
    }

    switch(notify_event.event_stage)
    {
        case START_PLAY_LED:
            system_slave_led_display(cfg_notify);
            break;
        case START_PLAY_TONE:
            system_slave_tone_play(cfg_notify, notify_event.sync_time);
            break;
        case START_PLAY_VOICE:
            if ((cfg_notify->Event_Type == UI_EVENT_BT_CALL_INCOMING) ||
                    (cfg_notify->Event_Type == UI_EVENT_BT_CALL_3WAYIN))
            {
                notify_sync_phone_num_t notify_sync_phone;
                memcpy(&notify_sync_phone, slave_notify, sizeof(notify_sync_phone));
                system_slave_voice_play(cfg_notify, notify_sync_phone.phone_num, notify_event.sync_time);
            }else{
                system_slave_voice_play(cfg_notify, NULL, notify_event.sync_time);
            }
                    
            break;

        default:
            break;
    }
}

static int system_notify_tws_send(int receiver, int event, void* param, int len, bool sync_wait)
{
	int sender = bt_manager_tws_get_dev_role();
	
	if ((sender != BTSRV_TWS_NONE) && (sender != receiver))
	{
		bt_manager_tws_send_message(TWS_USER_APP_EVENT, event, param, len);

		return 0;
	}
	
	return -1;
}

static void system_notify_led_tws(uint8_t event_type, uint8_t stage, bool led_disp_only)
{
    notify_sync_t notify_event;

    if (!system_need_sync_event(event_type, led_disp_only))
    {
        return;
    }

    /* BT status LED display could be different for TWS master and slave */
    switch (event_type)
    {
        case UI_EVENT_TWS_WAIT_PAIR:
        case UI_EVENT_BT_UNLINKED:
        case UI_EVENT_ENTER_PAIR_MODE:
        case UI_EVENT_BT_WAIT_CONNECT:
        case UI_EVENT_BT_CONNECTED:
        case UI_EVENT_TWS_CONNECTED:
            return;
    }

    SYS_LOG_INF("event_type:%d, stage:%d", event_type, stage);

    notify_event.event_type = event_type;
    notify_event.event_stage = stage;
    notify_event.sync_time = 0;

    system_notify_tws_send(BTSRV_TWS_SLAVE, TWS_EVENT_SYS_NOTIFY, &notify_event, sizeof(notify_event), false);
}


static void system_notify_tone_tws(uint8_t event_type, uint8_t stage, uint64_t bt_clk)
{
    notify_sync_t notify_event;

    SYS_LOG_INF("event_type:%d, event_stage:%d, time:%u_%u", 
                event_type, 
                stage, 
                (uint32_t)((bt_clk >> 32) & 0xffffffff),
                (uint32_t)(bt_clk & 0xffffffff)
               );

    notify_event.event_type = event_type;
    notify_event.event_stage = stage;
    notify_event.sync_time = bt_clk;
    system_notify_tws_send(BTSRV_TWS_SLAVE, TWS_EVENT_SYS_NOTIFY, &notify_event, sizeof(notify_event), false);   
}

static void system_notify_voice_tws(uint8_t event_type, uint8_t stage, uint8_t *phone_num, uint8_t len, uint64_t bt_clk)
{
    SYS_LOG_INF("event_type:%d, event_stage:%d, time:%u_%u", 
                event_type, 
                stage, 
                (uint32_t)((bt_clk >> 32) & 0xffffffff),
                (uint32_t)(bt_clk & 0xffffffff)
               );

    if ((event_type == UI_EVENT_BT_CALL_INCOMING) ||
        (event_type == UI_EVENT_BT_CALL_3WAYIN))
    {
        notify_sync_phone_num_t notify_event_phone = {0};
        
        notify_event_phone.event_type = event_type;
        notify_event_phone.event_stage = stage;
        notify_event_phone.sync_time = bt_clk;
        if(len < MAX_PHONENUM)
        {
            memcpy(notify_event_phone.phone_num, phone_num, len);
        }
        else
        {
            memcpy(notify_event_phone.phone_num, phone_num, MAX_PHONENUM);
        }
        system_notify_tws_send(BTSRV_TWS_SLAVE, TWS_EVENT_SYS_NOTIFY, &notify_event_phone, sizeof(notify_event_phone), false);

    }
    else
    {
        notify_sync_t notify_event;
        notify_event.event_type = event_type;
        notify_event.event_stage = stage;
        notify_event.sync_time = bt_clk;
        system_notify_tws_send(BTSRV_TWS_SLAVE, TWS_EVENT_SYS_NOTIFY, &notify_event, sizeof(notify_event), false);
    }
}


bool system_notify_is_end(system_notify_ctrl_t* notify_ctrl)
{
    if (notify_ctrl->flags.end_led_display &&
        notify_ctrl->flags.end_tone_play   &&
        notify_ctrl->flags.end_voice_play)
    {
        return true;
    }
    
    return false;
}

void system_notify_finish(system_notify_ctrl_t* notify_ctrl)
{
    system_notify_ctrl_t  t = *notify_ctrl;

    SYS_LOG_DBG("%d, 0x%x", t.cfg_notify.Event_Type, t.layer_id);

    mem_free(notify_ctrl);

    if(is_led_layer_notify(&t))
    {
        /* STANDBY display on LED_LAYER_SYS_NOTIFY, stop display after WAKE_UP
         */
        if (t.cfg_notify.Event_Type != UI_EVENT_STANDBY) {
            led_notify_stop_display(t.layer_id);
        }
    }

    if (t.end_callback != NULL)
    {
        SYS_LOG_DBG("%d", __LINE__);
        t.end_callback();
    }
    SYS_LOG_DBG("%d", __LINE__);
}


bool system_notify_tone_check_incoming(system_notify_ctrl_t* notify_ctrl)
{
    int hfp_state;

    hfp_state = bt_manager_hfp_get_status();

	if((hfp_state != BT_STATUS_INCOMING) && (hfp_state != BT_STATUS_3WAYIN))
    {
        notify_ctrl->flags.end_tone_play = true;
        
        if (notify_ctrl->flags.start_tone_play)
        {
            tip_manager_stop();
        }
        return false;
    }

    //need add call ring related processing
    if(!notify_ctrl->call_incoming.is_play_tone)
    {
        if (!notify_ctrl->flags.start_tone_play)
        {        
            notify_ctrl->flags.end_tone_play = true;
            return false;
        }
    }

    return true;
}

static void _system_tts_event_nodify(u8_t *tts_id, u32_t event)
{
	switch (event) {
		case TTS_EVENT_START_PLAY:
			break;
		case TTS_EVENT_STOP_PLAY:
            if(is_phone_num_playing){ 
                SYS_LOG_DBG("%s end", tts_id);
                is_tts_finish = true;
            }else if(!strcmp(tts_id, voice_name)){
                SYS_LOG_DBG("%s end", tts_id);
                is_tts_finish = true;
            }
			break;
        default:
            break;
	}	

}

void system_notify_tone_play(system_notify_ctrl_t* notify_ctrl)
{
    system_app_context_t*  manager = system_app_get_context();
    static char tone_name[CFG_MAX_VOICE_NAME_LEN + CFG_MAX_VOICE_FMT_LEN];
    uint64_t bt_clk = 0;
    
    if (notify_ctrl->flags.led_disp_only ||
        notify_ctrl->flags.disable_tone  ||
        notify_ctrl->cfg_notify.Tone_Play == NONE)
    {
        notify_ctrl->flags.end_tone_play = true;
        
        SYS_LOG_INF("++%d, %d, %d, %d", __LINE__, notify_ctrl->flags.led_disp_only, 
            notify_ctrl->flags.disable_tone, notify_ctrl->cfg_notify.Tone_Play);
        return;
    }


    if ((notify_ctrl->cfg_notify.Event_Type == UI_EVENT_BT_CALL_INCOMING) || 
        (notify_ctrl->cfg_notify.Event_Type == UI_EVENT_BT_CALL_3WAYIN))
    {
        if (system_notify_tone_check_incoming(notify_ctrl) == false)
        {
            SYS_LOG_INF("%d", __LINE__);
            return;
        }
    }

    /* 
     * check if play tone first
     */
    if (notify_ctrl->cfg_notify.Options & EVENT_NOTIFY_VOICE_FIRST)
    {
        if (!notify_ctrl->flags.end_voice_play)
        {
            SYS_LOG_INF("++%d", __LINE__);
            return;
        }
    }
    
    /*
     * start play tone
     */
     if (!notify_ctrl->flags.start_tone_play)
     {
         if(system_key_tone_is_playing())
         {
            SYS_LOG_INF("+++%d", __LINE__);
            return;
         }
 
         if (manager->sys_status.disable_audio_tone)
         {
             notify_ctrl->flags.end_tone_play = true;
    
             SYS_LOG_INF("+++%d", __LINE__);
             //tip_manager_stop();
             return;
         }
    
         notify_ctrl->flags.start_tone_play = true;
    
         /*
          * check tws sysc play tone time.
          */
         //bt_manager_tws_sync_play_notify();
         SYS_LOG_DBG("%d, start_tone_play.", notify_ctrl->cfg_notify.Event_Type);

         if (audio_tone_get_name(notify_ctrl->cfg_notify.Tone_Play, tone_name) != NULL)
         {             
             SYS_LOG_INF("tone_name:%s", tone_name);

             
             if (notify_ctrl->flags.is_tws_mode &&
                system_need_sync_event(notify_ctrl->cfg_notify.Event_Type, false))
             {
                tws_runtime_observer_t * observer = btif_tws_get_tws_observer();
                bt_manager_wait_tws_ready_send_sync_msg(APP_WAIT_TWS_READY_SEND_SYNC_TIMEOUT);
                bt_clk = observer->get_bt_clk_us();
                if(bt_clk != UINT64_MAX){
                    bt_clk += NOTIFY_SYNC_WAIT_MS;
                }
                system_notify_tone_tws(notify_ctrl->cfg_notify.Event_Type, START_PLAY_TONE, bt_clk);
             }else{
                bt_clk = 0;
             }
             if(tip_manager_play(tone_name, bt_clk) < 0)
             {
                 notify_ctrl->flags.end_tone_play = true;;
             }
         }
         else
         { 
            notify_ctrl->flags.end_tone_play = true;;
         }
     }
 
     /*
      * wait end of tone play
      */
     else if (strcmp(tip_manager_get_playing_filename(), tone_name))
     {
         notify_ctrl->flags.end_tone_play = true;
         //tip_manager_stop();
         SYS_LOG_INF("%d, end_tone_play.", notify_ctrl->cfg_notify.Event_Type);
     }

}

bool system_notify_voice_check_incoming(system_notify_ctrl_t* notify_ctrl)
{
    int hfp_state;

    hfp_state = bt_manager_hfp_get_status();

	if((hfp_state != BT_STATUS_INCOMING) && (hfp_state != BT_STATUS_3WAYIN))
    {
        notify_ctrl->flags.end_voice_play = true;

        if (notify_ctrl->flags.start_voice_play)
        {
            tts_manager_stop(NULL);
            tts_manager_wait_finished(true);
        }
        return false;
    }

    return true;
}

void system_notify_voice_play(system_notify_ctrl_t* notify_ctrl)
{
    system_app_context_t*  manager = system_app_get_context();
    uint8_t *p_num_list;
    int   voice_num = 0;
    uint64_t bt_clk = 0;

    //char voice_name[CFG_MAX_VOICE_NAME_LEN + CFG_MAX_VOICE_FMT_LEN];

    if (notify_ctrl->flags.led_disp_only ||
        notify_ctrl->flags.disable_voice)
    {
        SYS_LOG_INF("%d, %d,%d", __LINE__, notify_ctrl->flags.led_disp_only, notify_ctrl->flags.disable_voice);
        notify_ctrl->flags.end_voice_play = true;
        return;
    }

    if ((notify_ctrl->cfg_notify.Event_Type == UI_EVENT_BT_CALL_INCOMING) ||
        (notify_ctrl->cfg_notify.Event_Type == UI_EVENT_BT_CALL_3WAYIN))
    {
        if (system_notify_voice_check_incoming(notify_ctrl) == false)
        {
            SYS_LOG_INF("%d", __LINE__);
            return;
        }
    
        //need add call ring related processing
        if(notify_ctrl->call_incoming.voice_num == 0)
        {
            SYS_LOG_INF("%d", __LINE__);
            notify_ctrl->flags.end_voice_play = true;
            return;
        }
        p_num_list = notify_ctrl->call_incoming.voice_list;
        voice_num = notify_ctrl->call_incoming.voice_num;
    }

    if (notify_ctrl->cfg_notify.Voice_Play == NONE && 
        voice_num == 0)
    {
        notify_ctrl->flags.end_voice_play = true;
        SYS_LOG_INF("%d", __LINE__);
        return;
    }

    /*
     * check if play voice first
     */
    if (!(notify_ctrl->cfg_notify.Options & EVENT_NOTIFY_VOICE_FIRST))
    {
        if (!notify_ctrl->flags.end_tone_play)
        { 
            return;
        }
    }

    /* 
     * play voice
     */
#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    const struct device* gpio_dev = device_get_binding("GPIOA");
    gpio_pin_configure(gpio_dev, 19, GPIO_OUTPUT_ACTIVE);
#endif
#endif
    if (!notify_ctrl->flags.start_voice_play)
    {
        if (manager->sys_status.disable_audio_voice)
        {
            SYS_LOG_INF("%d", __LINE__);
            notify_ctrl->flags.end_voice_play = true;
            return;
        }

        notify_ctrl->flags.start_voice_play = true;

        /* 
         * check tws sysc play voice time.
         */
        //bt_manager_tws_sync_play_notify();
        SYS_LOG_INF("%d, start_voice_play.", notify_ctrl->cfg_notify.Event_Type);
        if(voice_num != 0)
        {
            /*
             * should stop btcall sco play when play tts.
             */  
            #if 0
            if (notify_ctrl->start_callback != NULL)
            {
                notify_ctrl->start_callback();
            }
            #endif
            
            if (system_need_sync_event(notify_ctrl->cfg_notify.Event_Type, false))
            {
                tws_runtime_observer_t * observer = btif_tws_get_tws_observer();
                bt_manager_wait_tws_ready_send_sync_msg(APP_WAIT_TWS_READY_SEND_SYNC_TIMEOUT);
                bt_clk = observer->get_bt_clk_us();
                if(bt_clk != UINT64_MAX){
                    bt_clk += NOTIFY_SYNC_WAIT_MS;
                }
                system_notify_voice_tws(notify_ctrl->cfg_notify.Event_Type, START_PLAY_VOICE, p_num_list, voice_num, bt_clk);
            }else{
                bt_clk = 0;
            }
            strcpy(voice_name, "V_NO_%c.act");
            if (manager->alter_voice_language == VOICE_LANGUAGE_2)
            {
                voice_name[0] = 'E';
            }
            SYS_LOG_INF("phone_num:%s", p_num_list);

            is_phone_num_playing = true;
            tts_manager_add_event_lisener(_system_tts_event_nodify);
            if(tts_manager_play_with_time(p_num_list, PLAY_IMMEDIATELY | TTS_MODE_TELE_NUM, bt_clk, voice_name) < 0)
            {
                is_tts_finish = true;
            }
            else
            {
                is_tts_finish = false;
            }
        }
        else
        {
            //phone_num_play.is_playing = false;

            /*
             * get voice name and play voice
             */
            is_phone_num_playing = false;
            if (audio_voice_get_name_ex(notify_ctrl->cfg_notify.Voice_Play, voice_name))
            {
                SYS_LOG_INF("voice_name:%s", voice_name);

                if(notify_ctrl->cfg_notify.Event_Type == UI_EVENT_POWER_OFF)
                {
                    tts_manager_wait_finished(true);
                    SYS_LOG_INF("wait finish end..");
                }
 
                if (notify_ctrl->flags.is_tws_mode &&
                    system_need_sync_event(notify_ctrl->cfg_notify.Event_Type, false))
                {
                    tws_runtime_observer_t * observer = btif_tws_get_tws_observer();
                    bt_manager_wait_tws_ready_send_sync_msg(APP_WAIT_TWS_READY_SEND_SYNC_TIMEOUT);
                    bt_clk = observer->get_bt_clk_us();
                    if(bt_clk != UINT64_MAX){
                        bt_clk += NOTIFY_SYNC_WAIT_MS;
                    }
                    system_notify_voice_tws(notify_ctrl->cfg_notify.Event_Type, START_PLAY_VOICE, NULL, 0, bt_clk);
                }else{
                    bt_clk = 0;
                }

                tts_manager_add_event_lisener(_system_tts_event_nodify);
                if(tts_manager_play_with_time(voice_name, PLAY_IMMEDIATELY, bt_clk, NULL) < 0)
                {
                    is_tts_finish = true;
                }
                else
                {
                    is_tts_finish = false;
                }
            }
            else
            {
                is_tts_finish = true;
            }
        }
    }
    else if(is_tts_finish)
    {
        notify_ctrl->flags.end_voice_play = true;
        is_phone_num_playing = false;
#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
        if(strcmp("V_POWOFF.act", voice_name) == 0)
        {
            const struct device* gpio_dev = device_get_binding("GPIOA");
            gpio_pin_configure(gpio_dev, 19, GPIO_OUTPUT_INACTIVE);
        }
#endif
#endif
        SYS_LOG_INF("%d, end_voice_play.", notify_ctrl->cfg_notify.Event_Type);
    }
    else
    {
    }
}

void system_notify_wait_end(system_notify_ctrl_t* notify_ctrl)
{
    uint32_t start_time = os_uptime_get_32();
    
    SYS_LOG_INF("%d", __LINE__);
    while (1)
    {
        if (system_notify_is_end(notify_ctrl) || (os_uptime_get_32() - start_time > 8000))
        {
            SYS_LOG_INF("");
            break;
        }
        thread_timer_handle_expired();
        os_sleep(10);
    }
    SYS_LOG_INF("%d", __LINE__);
}

void system_notify_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{    
    system_notify_ctrl_t*  notify_ctrl;
    system_app_context_t*  manager = system_app_get_context();

    /* 
     * all led notify has set to led ctrl
     */
    if (list_empty(&manager->notify_ctrl_list))
    {
        SYS_LOG_INF("notify_ctrl_list empty!");
        if (thread_timer_is_running(&manager->notify_ctrl_timer))
            thread_timer_stop(&manager->notify_ctrl_timer);
        return;
    }

    notify_ctrl = list_first_entry
    (
        &manager->notify_ctrl_list, system_notify_ctrl_t, node
    );

    if (!notify_ctrl->flags.set_led_display &&
        !notify_ctrl->flags.set_tone_play   &&
        !notify_ctrl->flags.set_voice_play)
    {
        SYS_LOG_DBG("%s: %d start", (notify_ctrl->flags.led_disp_only) ? "led_disp" : "ui_event", 
            notify_ctrl->cfg_notify.Event_Type);
    }

    notify_ctrl->nesting += 1;

    if (!notify_ctrl->flags.end_led_display)
    {
        /* 
         * not control led display by event notify in test mode, controlled by test its self.
         */
        if (manager->sys_status.in_test_mode)
        {
            notify_ctrl->flags.end_led_display = true;
        }
        if (!notify_ctrl->flags.set_led_display &&
            notify_ctrl->flags.is_tws_mode)
        {
            system_notify_led_tws(notify_ctrl->cfg_notify.Event_Type, START_PLAY_LED,
                notify_ctrl->flags.led_disp_only);
        }
        led_notify_start_display(notify_ctrl);
    }

    /* 
     * tone play
     */
    if (!notify_ctrl->flags.end_tone_play)
    {
        system_notify_tone_play(notify_ctrl);
    }

    /*
     *voice play
     */
    if (!notify_ctrl->flags.end_voice_play)
    {
        system_notify_voice_play(notify_ctrl);
    }


    notify_ctrl->nesting -= 1;

    if (system_notify_is_end(notify_ctrl))
    {
        SYS_LOG_DBG("%s: %d end", (notify_ctrl->flags.led_disp_only) ? "led_disp" : "ui_event", 
            notify_ctrl->cfg_notify.Event_Type);
        /*
         * delete from notify list.
         */
        if (!notify_ctrl->flags.list_deleted)
        {
            notify_ctrl->flags.list_deleted = true;

            list_del(&notify_ctrl->node);
        }

        /*
         * should not release when in nesting
         */
        if (notify_ctrl->nesting > 0)
        {
            return;
        }

        if (!notify_ctrl->flags.need_wait_end)
        {
            system_notify_finish(notify_ctrl);
        }
        
    }
}

/* 
  * clear notify
 */
void system_do_clear_notify(void)
{
    system_app_context_t*  manager = system_app_get_context();

    system_notify_ctrl_t*  notify_ctrl;
    
    SYS_LOG_INF("start");

    if (manager->notify_ctrl_list.next == NULL)
    {
        return;
    }

    if (list_empty(&manager->notify_ctrl_list))
    {
        return;
    }

    list_for_each_entry(notify_ctrl, &manager->notify_ctrl_list, node)
    {
        if (notify_ctrl->cfg_notify.Event_Type == UI_EVENT_TWS_WAIT_PAIR &&
            bt_manager_tws_get_dev_role() != BTSRV_TWS_NONE)
        {
            /* clear UI_EVENT_TWS_WAIT_PAIR when TWS already connected */
        }
        else if (system_is_local_event(notify_ctrl->cfg_notify.Event_Type,
                    notify_ctrl->flags.led_disp_only))
        {
            continue;
        }
 
        if (notify_ctrl->flags.set_led_display)
        {
            led_notify_stop_display(notify_ctrl->layer_id);
        }

        if ((!notify_ctrl->flags.set_led_display) && 
            (!notify_ctrl->flags.set_tone_play) && 
            (!notify_ctrl->flags.set_voice_play)
           )
        {
            SYS_LOG_INF("%s: %d", (notify_ctrl->flags.led_disp_only) ? "led_disp" : "ui_event", 
                notify_ctrl->cfg_notify.Event_Type);
            
            notify_ctrl->flags.end_led_display = true;
            notify_ctrl->flags.end_tone_play   = true;
            notify_ctrl->flags.end_voice_play  = true;
            notify_ctrl->end_callback = NULL;
        }
    }
    SYS_LOG_INF("end");

}

/* 
  * stop notify
 */
void system_do_stop_notify(u32_t ui_event)
{
    system_app_context_t*  manager = system_app_get_context();

    system_notify_ctrl_t*  notify_ctrl;

    //SYS_LOG_INF("ui_event:%d", ui_event);

    if (manager->notify_ctrl_list.next == NULL)
    {
        return;
    }

    if (list_empty(&manager->notify_ctrl_list))
    {
        return;
    }

    list_for_each_entry(notify_ctrl, &manager->notify_ctrl_list, node)
    {
        if (notify_ctrl->flags.led_disp_only)
        {
            continue;
        }

        if (notify_ctrl->cfg_notify.Event_Type != ui_event &&
            ui_event != (u8_t)-1)
        {
            continue;
        }

        if (notify_ctrl->flags.set_led_display)
        {
            led_notify_stop_display(notify_ctrl->layer_id);
        }

        if (!notify_ctrl->flags.end_tone_play)
        {
            if (notify_ctrl->flags.set_tone_play)
            {
                tip_manager_stop();
            }
        }
    
        if (!notify_ctrl->flags.end_voice_play)
        {
            if (notify_ctrl->flags.set_voice_play)
            {
                tts_manager_stop(NULL);
            }
        }
        
        notify_ctrl->flags.end_led_display = true;
        notify_ctrl->flags.end_tone_play   = true;
        notify_ctrl->flags.end_voice_play  = true;

        notify_ctrl->end_callback = NULL;
    }

}

void system_do_event_notify(u32_t ui_event, bool led_disp_only, event_call_incoming_t *incoming)
{
    system_app_context_t*  manager = system_app_get_context();
    CFG_Type_Event_Notify* cfg_notify = NULL;
    system_notify_ctrl_t*  notify_ctrl = NULL;
    bool  ignore = false;

    SYS_LOG_INF("%s: %d", (led_disp_only) ? "led_disp" : "ui_event", ui_event);

    if (ui_event == UI_EVENT_CHARGE_STOP)
    {
        led_notify_stop_display(led_get_layer_id(ui_event));
        return;
    }

#ifdef CONFIG_USOUND_APP
    if (usb_phy_get_vbus())  {
        /* usound is running, forbid all bt events */
        if ((ui_event == UI_EVENT_ENTER_PAIR_MODE)  ||
            (ui_event == UI_EVENT_BT_WAIT_CONNECT)  ||
            (ui_event == UI_EVENT_BT_CONNECTED)     ||
            (ui_event == UI_EVENT_2ND_CONNECTED)    ||
            (ui_event == UI_EVENT_BT_DISCONNECTED)  ||
            (ui_event == UI_EVENT_BT_UNLINKED)      ||
            (ui_event == UI_EVENT_TWS_WAIT_PAIR)    ||
            (ui_event == UI_EVENT_TWS_CONNECTED)    ||
            (ui_event == UI_EVENT_TWS_DISCONNECTED) ||
            (ui_event == UI_EVENT_ENTER_PAIRING_MODE))
        {
            return ;
        }
    }
#endif

    if ((bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) && 
        (!system_is_local_event(ui_event, led_disp_only)))
    {
        SYS_LOG_INF("tws slave");
        if(ui_event == UI_EVENT_TWS_CONNECTED)
        {
            system_do_clear_notify();
        }
        return;
    }

    if (!led_disp_only)
    {
        /* check repeated event notify */
        if (manager->notify_ctrl_list.next != NULL)
        {
            list_for_each_entry(notify_ctrl, &manager->notify_ctrl_list, node)
            {
                if ((!notify_ctrl->flags.led_disp_only) &&
                    (notify_ctrl->cfg_notify.Event_Type == ui_event) &&
                    (notify_ctrl->cfg_notify.Event_Type != UI_EVENT_BT_DISCONNECTED)
                   )
                {
                    SYS_LOG_WRN("REPEATED");
                    return;
                }
            }
        }
        
        system_do_stop_notify(ui_event);
    
        /* 
         * ignore all other notify event when system will poweroff
         */
        if (manager->sys_status.in_power_off_stage)
        {
            if (ui_event != UI_EVENT_POWER_OFF)
            {
                ignore = true;
            }
        }
    }

    if (ignore)
    {
        SYS_LOG_WRN("IGNORED");
        return;
    }

    notify_ctrl = mem_malloc(sizeof(*notify_ctrl));
    if(NULL == notify_ctrl)
    {
        SYS_LOG_ERR("%d, malloc fail!", __LINE__);
        return;
    }
    
    memset(notify_ctrl, 0, sizeof(*notify_ctrl));

	cfg_notify = (CFG_Type_Event_Notify*)system_notify_get_config(ui_event);
    if (NULL == cfg_notify)
    {
        if (led_disp_only) {
            notify_ctrl->cfg_notify.Event_Type = ui_event;
        } else {
            SYS_LOG_WRN("%d, cfg_notify is null.", __LINE__);
            mem_free(notify_ctrl);
            return;
        }
    } else {
        notify_ctrl->cfg_notify = *cfg_notify;
    }

    notify_ctrl->layer_id     = led_get_layer_id(ui_event);
    notify_ctrl->flags.led_disp_only = led_disp_only;

    if (bt_manager_tws_get_dev_role() >= BTSRV_TWS_MASTER) {
        notify_ctrl->flags.is_tws_mode = true;
    }
    
    if(NULL != incoming)
    {
        notify_ctrl->call_incoming.is_play_tone= incoming->is_play_tone;
        notify_ctrl->call_incoming.voice_num= incoming->voice_num;
        memcpy(notify_ctrl->call_incoming.voice_list, incoming->voice_list, MAX_PHONENUM);
        notify_ctrl->start_callback = incoming->start_callback;
        notify_ctrl->end_callback = incoming->end_callback;       
    }

    if (led_disp_only)
    {
        notify_ctrl->cfg_notify.Tone_Play  = TONE_NONE;
        notify_ctrl->cfg_notify.Voice_Play = VOICE_NONE;
    }

    if (manager->sys_status.disable_audio_tone)
    {
        notify_ctrl->flags.disable_tone = true;
    }

    if (manager->sys_status.disable_audio_voice)
    {
        notify_ctrl->flags.disable_voice = true;
    }

    /*
        * init notify_ctrl_list
        */
    if (manager->notify_ctrl_list.next == NULL)
    {
        INIT_LIST_HEAD(&manager->notify_ctrl_list);
    }

    list_add_tail(&notify_ctrl->node, &manager->notify_ctrl_list);

    /*
     * init timer
     */
    if (!thread_timer_is_running(&manager->notify_ctrl_timer))
    {
        //SYS_LOG_INF("****event_type:%d", notify_ctrl->cfg_notify.Event_Type);
	    thread_timer_init(&manager->notify_ctrl_timer, system_notify_timer_handler, NULL);
	    thread_timer_start(&manager->notify_ctrl_timer, 20, 20);
    }

    /*
     * "power on" tone didn't play immediately, Avoid causing caton during Bluetooth initialization
     */
    if(ui_event != UI_EVENT_POWER_ON)
    {
        //SYS_LOG_INF("***%d", __LINE__);
        system_notify_timer_handler(NULL,NULL);    
    }

    /* Wait poweroff notify end
     */
    if (ui_event == UI_EVENT_POWER_OFF)
    {
        system_notify_wait_end(notify_ctrl);        
    }
}

void system_app_sys_event_deal(struct app_msg *msg)
{
	u8_t ui_event = system_event_sys_2_ui(msg->cmd);
	bool led_disp_only = msg->reserve? true: false;

	system_do_event_notify(ui_event, led_disp_only, NULL);
}

void system_app_ui_event_deal(struct app_msg *msg)
{
	u32_t ui_event = msg->cmd;
	bool led_disp_only = msg->reserve? true: false;
    event_call_incoming_t *p_incoming_param;

    p_incoming_param = (event_call_incoming_t *)msg->ptr;

    if((ui_event == UI_EVENT_BT_CALL_INCOMING) ||
        (ui_event == UI_EVENT_BT_CALL_3WAYIN))
    {
        system_do_event_notify(ui_event, led_disp_only, p_incoming_param);
    }
    else
    {
        system_do_event_notify(ui_event, led_disp_only, NULL);

    }

}

static void system_app_view_deal(u32_t ui_event)
{
	system_do_event_notify(ui_event, false, NULL);
}

static int system_app_view_proc(u8_t view_id, u8_t msg_id, void* msg_data)
{
	u32_t ui_event = (u32_t)msg_data;

	SYS_LOG_INF(" msg_id %d ui_event %d\n", msg_id, ui_event);
	
	switch (msg_id) {
	case MSG_VIEW_CREATE:
		//system_app_view_deal(UI_EVENT_POWER_ON);
		break;
	case MSG_VIEW_PAINT:
	    system_app_view_deal(ui_event);
		break;
	case MSG_VIEW_DELETE:
		tts_manager_wait_finished(true);
		break;
	default:
		break;
	}
	return 0;
}

static int system_app_get_status(void)
{
    system_app_context_t*  manager = system_app_get_context();
	btmgr_feature_cfg_t* btmgr_feature_cfg = bt_manager_get_feature_config();
	int status = 0;
	char* current_app = app_manager_get_current_app();
	
	if (current_app && strcmp(APP_ID_BTCALL, current_app) == 0){
		status |= MAIN_STATUS_FG_BTCALL;
	}else{
		status |= MAIN_STATUS_FG_NOT_BTCALL;
	}
	
	if (bt_manager_hfp_get_status() == BT_STATUS_SIRI){
		status |= MAIN_STATUS_IN_VOICE_ASSIST;
	}else{
		status |=MAIN_STATUS_NOT_IN_VOICE_ASSIST;
	}
	
	if (bt_manager_is_pair_mode() || bt_manager_is_remote_in_pair_mode()){
		status |= MAIN_STATUS_IN_PAIR_MODE;
	}else{
		status |= MAIN_STATUS_NOT_IN_PAIR_MODE;
	}

	if (bt_manager_is_tws_pair_search()){
		status |= MAIN_STATUS_IN_TWS_PAIR_SEARCH;
	}else{
		status |= MAIN_STATUS_NOT_IN_TWS_PAIR_SEARCH;
	}
	
	if (bt_manager_get_connected_dev_num()){
		status |= MAIN_STATUS_PHONE_CONNECTED;
	}else{
		status |= MAIN_STATUS_NOT_PHONE_CONNECTED;
	}	

	if (bt_manager_tws_get_dev_role() >= BTSRV_TWS_MASTER){
		status |= MAIN_STATUS_TWS_MODE;
	}else{
		status |= MAIN_STATUS_NOT_TWS_MODE;
	}	

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
	if (btmgr_feature_cfg->controller_test_mode != BT_CTRL_DISABLE_TEST &&
	    btmgr_feature_cfg->enter_bqb_test_mode_by_key && !manager->bqb_enter){
		status |= MAIN_STATUS_SUPPORT_BQB_MODE;
#else
	if (btmgr_feature_cfg->controller_test_mode != BT_CTRL_DISABLE_TEST &&
	    btmgr_feature_cfg->enter_bqb_test_mode_by_key){
		status |= MAIN_STATUS_SUPPORT_BQB_MODE;
#endif
	}else{
		status |= MAIN_STATUS_NOT_SUPPORT_BQB_MODE;
	}

	if (manager->sys_status.in_front_charge){
		status |= MAIN_STATUS_IN_FRONT_CHARGE;
	}else{
		status |= MAIN_STATUS_NOT_IN_FRONT_CHARGE;
	}

	if (btmgr_feature_cfg->sp_tws &&
	    btmgr_feature_cfg->sp_device_num > 1 &&
	    bt_manager_get_connected_dev_num() < btmgr_feature_cfg->sp_device_num)
	{
	    status |= MAIN_STATUS_ENABLE_TWS_CONNECT;
	}

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    if(manager->bqb_enter)
    {
        status |= MAIN_STATUS_IN_BQB_MODE;
    }
#endif

	SYS_LOG_INF("status:0x%x ", status);

	return status;
}

static bool system_app_status_match(u32_t current_state, u32_t match_state)
{
	u32_t dev_type = match_state>>28;
	u32_t state = match_state&0xfffffff;

	if(ui_key_check_dev_type(dev_type))
	{
		SYS_LOG_INF("current:0x%x match:0x%x", current_state, state);
		return (current_state & state) == state;
	}
	return false;
}


void system_app_view_init(void)
{
	ui_view_info_t  view_info;

	ui_key_filter(true);
	
	system_app_view_exit();
	memset(&view_info, 0, sizeof(ui_view_info_t));
	common_keymap = ui_key_map_create(common_func_table, (sizeof(common_func_table)/sizeof(ui_key_func_t)));
	view_info.view_proc = system_app_view_proc;
	view_info.view_key_map = common_keymap;
	view_info.view_get_state = system_app_get_status;
	view_info.view_state_match = system_app_status_match;
	view_info.order = 0;
	view_info.app_id = APP_ID_MAIN;

	ui_view_create(MAIN_VIEW, &view_info);
	
	ui_key_filter(false);

	SYS_LOG_INF(" ok\n");
}

void system_app_view_exit(void)
{
	if (common_keymap)
	{
		ui_key_map_delete(common_keymap);
		ui_view_delete(MAIN_VIEW);
	}
}


