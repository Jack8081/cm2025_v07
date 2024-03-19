/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app input
 */
#include <soc.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <sys_monitor.h>
#include "sys_wakelock.h"
#include "system_app.h"
#include "bt_manager.h"
#ifdef CONFIG_ESD_MANAGER
#include <esd_manager.h>
#endif

#include "tts_manager.h"
#include "ui_key_map.h"
#include "app_config.h"

void _system_key_event_cb(u32_t key_value, uint16_t type)
{
    system_app_context_t*  manager = system_app_get_context();
	static int last_status = KEY_VALUE_UP;
	int cur_status = key_value>>31;
	int key_code = key_value & 0xffff;
	static int last_down_islock = 0;
    static char need_check_time = 1;

	/*fix boot hold key, dont play key tone*/
	if (manager->boot_hold_key_checking)
	{
		last_status = cur_status;
		SYS_LOG_INF("checking:%d",cur_status);		
		return;
	}

	if (need_check_time)
	{
		/*fix boot hold key, dont play key tone*/	
		if ((os_uptime_get_32() - manager->boot_hold_key_begin_time) < 800)
		{
			last_status = cur_status;
			SYS_LOG_INF("%d",cur_status);		
			return;
		}
	}
	need_check_time = 0;

	/* 按下时播放按键音
	 */
	if (cur_status == KEY_VALUE_DOWN)
	{
		if (last_status == KEY_VALUE_UP)
		{
			SYS_LOG_INF("key down");
			if (!input_manager_islock()) {
				system_key_tone_on_event_down(key_code);
			}else{
				last_down_islock = 1;
			}
		}

		if (last_down_islock)
		{
			if (!input_manager_islock()) {
				SYS_LOG_INF("down unlock");				
				system_key_tone_on_event_down(key_code);
				last_down_islock = 0;
			}
		}
	} 
	else if(last_status == KEY_VALUE_DOWN && cur_status == KEY_VALUE_UP)
	{
		if (last_down_islock)
		{
			if (!input_manager_islock()) {
				SYS_LOG_INF("up unlock");				
				system_key_tone_on_event_down(key_code);
			}else{
				SYS_LOG_INF("up still lock");				
			}
				
			last_down_islock = 0;			
		}
		SYS_LOG_INF("key up");
		
	}
	last_status = cur_status;
}

void key_event_log(u32_t key_event)
{
    char type_sbuf[32] = "";
    char name_sbuf[32] = "";
    int  i;

    static const struct { uint32_t key_type; const char* type_str; } 
        key_type_tab[] = 
    {
        { KEY_EVENT_SINGLE_CLICK,    "1_CLICK"      },
        { KEY_EVENT_DOUBLE_CLICK,    "2_CLICK"      },
        { KEY_EVENT_TRIPLE_CLICK,    "3_CLICK"      },
        { KEY_EVENT_QUAD_CLICK,      "4_CLICK"      },
        { KEY_EVENT_QUINT_CLICK,     "5_CLICK"      },
        { KEY_EVENT_REPEAT,          "REPEAT"       },
        { KEY_EVENT_LONG_PRESS,      "LONG_PRESS"   },
        { KEY_EVENT_LONG_UP,         "LONG_UP"      },
        { KEY_EVENT_LONG_LONG_PRESS, "L_LONG_PRESS" },
        { KEY_EVENT_LONG_LONG_UP,    "L_LONG_UP"    },
        { KEY_EVENT_VERY_LONG_PRESS, "V_LONG_PRESS" },
        { KEY_EVENT_VERY_LONG_UP,    "V_LONG_UP"    },
    };

    for (i = 0; i < ARRAY_SIZE(key_type_tab); i++)
    {
        u32_t tmp_key_type = key_event >>16;  
        if (tmp_key_type & key_type_tab[i].key_type)
        {
            if (type_sbuf[0] != '\0') {
                strcat(type_sbuf, " + ");
            }
            strcat(type_sbuf, key_type_tab[i].type_str);
        }
    }

    static const struct { uint8_t key_value; const char* key_name; }
        key_name_tab[] =
    {
        { VKEY_MEDIA,         "MEDIA"       },
        { VKEY_NR,            "AI_NR"       },
        { VKEY_SURROUND,      "SURROUND"    },
        { VKEY_MIC_MUTE,      "MIC_MUTE"    },
        { VKEY_ROTARY_BTN,    "ROTARY_BTN"	},
        { VKEY_ROTARY_FORD,   "ROTARY_FORD" },
        { VKEY_ROTARY_BACK,   "ROTARY_BACK" },
        { LRADC_COMBO_VKEY_1, "COMBO_KEY_1" },
        { LRADC_COMBO_VKEY_2, "COMBO_KEY_2" },
        { LRADC_COMBO_VKEY_3, "COMBO_KEY_3" },
        { VKEY_PLAY,          "KEY_PLAY"    },
        { VKEY_VSUB,          "KEY_VOL-"    },
        { VKEY_VADD,          "KEY_VOL+"    },
        { VKEY_MODE,          "KEY_MODE"    },
        { VKEY_MENU,          "KEY_MENU"    },
        { TAP_KEY,            "TAP_KEY"     },
    };

    for (i = 0; i < ARRAY_SIZE(key_name_tab); i++)
    {
        if (key_name_tab[i].key_value == (key_event & 0xff) ||
            key_name_tab[i].key_value == ((key_event >> 8) & 0xff))
        {
            if (name_sbuf[0] != '\0') {
                strcat(name_sbuf, " + ");
            }
            strcat(name_sbuf, key_name_tab[i].key_name);
        }
    }

    printk("<Key: 0x%x, %s, %s>\n", key_event, name_sbuf, type_sbuf);
}


void system_input_event_handle(u32_t key_event)
{
	/**input event means esd proecess finished*/
#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		tts_manager_unlock();
		esd_manager_reset_finished();
	}
#endif

	sys_wake_lock(FULL_WAKE_LOCK);
	key_event_log(key_event);
	system_key_tone_on_event_long(key_event >>16);

#if 1
    /* always respond key function after wake up */
    ui_manager_dispatch_key_event(key_event);
#else
	system_configs_t*  sys_configs = system_get_configs();
	bool undrop_key = false;

    /* 按键唤醒后允许继续响应按键功能?
     */
	if (sys_configs->cfg_onoff_key.Use_Inner_ONOFF_Key &&
		sys_configs->cfg_onoff_key.Continue_Key_Function_After_Wake_Up)
    {
    	int keycode = key_event & 0xffff;
		if (keycode == sys_configs->cfg_onoff_key.Key_Value)
			undrop_key = true;
	}

	/**drop fisrt key event when resume*/
	if (undrop_key || system_wakeup_time() > 600) 
	{
		ui_manager_dispatch_key_event(key_event);
	} 
	else 
	{
		SYS_LOG_INF("drop key workup %d \n",system_wakeup_time());
	}
#endif

	sys_wake_unlock(FULL_WAKE_LOCK);
}

void system_input_handle_init(void)
{
	input_dev_init_param data;
	CFG_Struct_Key_Threshold key_threshold;

	app_config_read
	(
		CFG_ID_KEY_THRESHOLD, 
		&key_threshold, 0, sizeof(CFG_Struct_Key_Threshold)
	);

	data.long_press_time = key_threshold.Long_Press_Time_Ms;
	data.super_long_press_time = key_threshold.Long_Long_Press_Time_Ms;
	data.super_long_press_6s_time = key_threshold.Very_Long_Press_Time_Ms;
    
    data.single_click_valid_time = key_threshold.Single_Click_Valid_Ms;
	data.quickly_click_duration = key_threshold.Multi_Click_Interval_Ms; 
	data.key_event_cancel_duration = 0;
	data.hold_delay_time = key_threshold.Repeat_Start_Delay_Ms;
	data.hold_interval_time = key_threshold.Repeat_Interval_Ms;

	input_manager_init(_system_key_event_cb);

	input_manager_set_init_param(&data);
	/** input init is locked ,so we must unlock*/
	input_manager_unlock();
}


int system_app_check_onnff_key_state(void)
{
    system_app_context_t*  manager = system_app_get_context();

	CFG_Struct_ONOFF_Key*  cfg = &manager->sys_configs.cfg_onoff_key;

	int state = KEY_VALUE_UP;
	bool inner_onoffkey = false;
	
    if (cfg->Use_Inner_ONOFF_Key)
		inner_onoffkey = true;
	
	if (!input_keypad_onoff_inquiry_enable(inner_onoffkey, true))
	{
		state = input_keypad_onoff_inquiry(inner_onoffkey, cfg->Key_Value);
		input_keypad_onoff_inquiry_enable(inner_onoffkey, false);
	}
	return state;
}



static void system_boot_hold_key_reboot(void)
{
    /* 等待 ONOFF 按键抬起
     */
    while (1)
    {
        int  state = system_app_check_onnff_key_state();
        if (state == KEY_VALUE_UP)
        {
            break;
        }

        os_sleep(10);
    }

    struct app_msg	msg = {0};
    msg.type = MSG_REBOOT;
    msg.cmd = REBOOT_REASON_NORMAL;
    send_async_msg("main", &msg);
}

void system_boot_hold_key_func(unsigned int key_func)
{
    system_app_context_t*  manager = system_app_get_context();
    
    SYS_LOG_INF("%d", key_func);
    switch (key_func)
    {
        case BOOT_HOLD_KEY_FUNC_ENTER_PAIR_MODE:
        {
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
            extern int bt_manager_audio_adv_param_init(bt_addr_le_t *peer);
 	        bt_manager_audio_adv_param_init(NULL);
            bt_manager_audio_le_start_adv();
#endif
            bt_manager_enter_pair_mode();
            break;
        }
        
        case BOOT_HOLD_KEY_FUNC_TWS_PAIR_SEARCH:
        {
            bt_manager_start_pair_mode();
            bt_manager_tws_start_pair_search(TWS_PAIR_SEARCH_BY_KEY);
            break;
        }
        
        case BOOT_HOLD_KEY_FUNC_AUTO_SELECT:
        {
            if (bt_manager_is_tws_paired_valid() == false)
            {
                bt_manager_start_pair_mode();
                bt_manager_tws_start_pair_search(TWS_PAIR_SEARCH_BY_KEY);
            }
            else
            {
                bt_manager_enter_pair_mode();
            }
            break;
        }
        
        case BOOT_HOLD_KEY_FUNC_CLEAR_PAIRED_LIST:
        {
            CFG_Struct_ONOFF_Key*  cfg = &manager->sys_configs.cfg_onoff_key;
            bt_manager_clear_paired_list();
        
            /* 开机长按键清除配对列表后自动重启?
             */
            if (cfg->Reboot_After_Boot_Hold_Key_Clear_Paired_List)
            {
                system_boot_hold_key_reboot();	
            }
            break;
        }
    }
}

static void boot_hold_key_release_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    system_app_context_t*  manager = system_app_get_context();
	int  state = system_app_check_onnff_key_state();
	if (state == KEY_VALUE_UP)
	{
		SYS_LOG_INF("");
		manager->boot_hold_key_checking = 0;		
		ui_key_filter(false);		
		thread_timer_stop(ttimer);
	}
}



/* 检查开机长按键功能
 */
int system_check_boot_hold_key(void)
{
    system_app_context_t*  manager = system_app_get_context();
    system_configs_t*  sys_configs = system_get_configs();	
    
    unsigned int  vkey = manager->sys_configs.cfg_onoff_key.Key_Value;
    unsigned int  time = manager->sys_configs.cfg_onoff_key.Boot_Hold_Key_Time_Ms;

    if (vkey == VKEY_NONE || 
        sys_configs->cfg_onoff_key.Boot_Hold_Key_Func == BOOT_HOLD_KEY_FUNC_NONE)
    {
        return 0;
    }

	manager->boot_hold_key_checking = 1;
	SYS_LOG_INF("%d ,%d",manager->boot_hold_key_begin_time, time);
	
	SYS_LOG_INF("start: %d ms",os_uptime_get_32());
	ui_key_filter(true);	
    while (1)
    {
        int  state = system_app_check_onnff_key_state();
        if (state == KEY_VALUE_UP)
        {
            manager->boot_hold_key_checking = 0;
			ui_key_filter(false);	
            return 0;
        }

        if ((os_uptime_get_32() - manager->boot_hold_key_begin_time) > time)
        {
            break;
        }
        os_sleep(50);
        
        thread_timer_handle_expired();
    }
	SYS_LOG_INF("end: %d ms",os_uptime_get_32());

	thread_timer_init(&manager->boot_hold_key_release_timer, boot_hold_key_release_timer_handler, NULL);
	thread_timer_start(&manager->boot_hold_key_release_timer, 50, 1);

    manager->boot_hold_key_func = sys_configs->cfg_onoff_key.Boot_Hold_Key_Func;

    return manager->boot_hold_key_func;
}

#if (defined CONFIG_SAMPLE_CASE_1)
bool system_check_aux_plug_in(void)
{
    int   aux_state = FALSE;
    const struct device* p_aux_dev = NULL;

    p_aux_dev = device_get_binding(AUX_GPIO_PORT_NAME);
    if (p_aux_dev)
    {
        gpio_pin_configure(p_aux_dev, AUX_GPIO_PIN_IDX, GPIO_INPUT);
        aux_state = gpio_pin_get_raw(p_aux_dev, AUX_GPIO_PIN_IDX);
        //SYS_LOG_INF("aux state %d", aux_state);
    }
    
    return (aux_state ? TRUE : FALSE);
}
#elif (defined CONFIG_SAMPLE_CASE_XNT)
bool system_check_aux_plug_in(void)
{
#if 0
    int   aux_state = FALSE;
    const struct device* p_aux_dev = NULL;

    p_aux_dev = device_get_binding(AUX_GPIO_PORT_NAME);
    if (p_aux_dev)
    {
        gpio_pin_configure(p_aux_dev, AUX_GPIO_PIN_IDX, GPIO_INPUT);
        aux_state = gpio_pin_get_raw(p_aux_dev, AUX_GPIO_PIN_IDX);
        //SYS_LOG_INF("aux state %d", aux_state);
    }

    return (aux_state ? TRUE : FALSE);
#else
    return FALSE;
#endif
}
#endif

