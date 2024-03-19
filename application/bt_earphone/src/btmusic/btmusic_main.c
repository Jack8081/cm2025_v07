/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music app main.
 */

#include "btmusic.h"
#include "tts_manager.h"
#include "property_manager.h"
#include <file_stream.h>
#include <sdfs.h>

static struct btmusic_app_t *p_btmusic_app;

#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
static struct gpio_callback btmusic_gpio_cb;

static void _btmusic_aux_plug_in_handle(struct k_work *work)
{
    struct app_msg new_msg = { 0 };

    if (system_check_aux_plug_in())
    {   
		new_msg.type = MSG_INPUT_EVENT;
		new_msg.cmd  = KEY_FUNC_POWER_OFF;
		send_async_msg(APP_ID_MAIN, &new_msg);
    }
}

static void _btmusic_aux_plug_in_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
    SYS_LOG_INF("");
    os_delayed_work_submit(&p_btmusic_app->aux_plug_in_delay_work, 10);
}
#endif
#endif

void bt_music_delay_start(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
	
	btmusic->tws_con_tts_playing = 0;
	if (p_btmusic_app->player)
		return;
	bt_manager_audio_stream_restore(BT_TYPE_BR);
}

static int _btmusic_init(void)
{
	int ret = 0;
	CFG_Struct_BTMusic_Multi_Dae_Settings multidae_setting;

	p_btmusic_app = app_mem_malloc(sizeof(struct btmusic_app_t));
	if (!p_btmusic_app) {
		SYS_LOG_ERR("malloc fail!\n");
		return -ENOMEM;
	}

	memset(p_btmusic_app, 0, sizeof(struct btmusic_app_t));

	app_config_read
	(
		CFG_ID_BT_MUSIC_STOP_HOLD, 
		&p_btmusic_app->cfg_bt_music_stop_hold, 0, sizeof(CFG_Struct_BT_Music_Stop_Hold)
	);

	app_config_read
    (
        CFG_ID_BTMUSIC_USER_SETTINGS,
        &p_btmusic_app->user_settings, 0, sizeof(CFG_Struct_BTMusic_User_Settings)
    );

	app_config_read
    (
        CFG_ID_BTMUSIC_MULTI_DAE_SETTINGS,
        &multidae_setting, 0, sizeof(CFG_Struct_BTMusic_Multi_Dae_Settings)
    );

	p_btmusic_app->multidae_enable = multidae_setting.Enable;
	p_btmusic_app->dae_cfg_nums = multidae_setting.Cur_Dae_Num + 1;
	p_btmusic_app->current_dae = 0;

    /* 从vram读回当前音效 */
    if (p_btmusic_app->multidae_enable)
    {
        if (property_get(BTMUSIC_MULTI_DAE, &p_btmusic_app->current_dae, sizeof(u8_t)) < 0) {
            p_btmusic_app->current_dae = 0;
			property_set(BTMUSIC_MULTI_DAE, &p_btmusic_app->current_dae, sizeof(u8_t));
		}

        if (p_btmusic_app->current_dae > p_btmusic_app->dae_cfg_nums) {
            p_btmusic_app->current_dae = 0;
			property_set(BTMUSIC_MULTI_DAE, &p_btmusic_app->current_dae, sizeof(u8_t));
        }

		btmusic_multi_dae_adjust(p_btmusic_app->current_dae);
    } else {
		app_config_read
		(
			CFG_ID_BT_MUSIC_DAE,
			&p_btmusic_app->dae_enalbe, offsetof(CFG_Struct_BT_Music_DAE, Enable_DAE), sizeof(cfg_uint8)
		);

		app_config_read
		(
			CFG_ID_BT_MUSIC_DAE_AL,
			p_btmusic_app->cfg_dae_data, 0, CFG_SIZE_BT_MUSIC_DAE_AL
		);
	}

#ifdef CONFIG_BT_MUSIC_SWITCH_LANTENCY_SUPPORT
    p_btmusic_app->lantency_mode = 1;
#endif
	btmusic_view_init(false);

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    bt_ui_monitor_submit_work(0);
#endif

	audio_system_set_stream_volume(AUDIO_STREAM_MUSIC, audio_system_get_stream_volume(AUDIO_STREAM_MUSIC));	
	
    bt_manager_stream_enable(STREAM_TYPE_A2DP, true);
	bt_manager_set_stream_type(AUDIO_STREAM_MUSIC);	

	thread_timer_init(&p_btmusic_app->play_timer, bt_music_delay_start, NULL);
	thread_timer_start(&p_btmusic_app->play_timer, 10, 0);
	SYS_LOG_INF("dae cfg: %d %d %d %d", p_btmusic_app->dae_enalbe, 
	p_btmusic_app->multidae_enable, p_btmusic_app->dae_cfg_nums, p_btmusic_app->current_dae);

	bt_manager_resume_phone();

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    btmusic_event_notify(SYS_EVENT_CUSTOMED_2);

    p_btmusic_app->gpio_dev = device_get_binding(AUX_GPIO_PORT_NAME);
	gpio_pin_interrupt_configure(p_btmusic_app->gpio_dev, AUX_GPIO_PIN_IDX, GPIO_INT_EDGE_RISING);
	gpio_init_callback(&btmusic_gpio_cb, _btmusic_aux_plug_in_cb, BIT(AUX_GPIO_PIN_IDX));
	gpio_add_callback(p_btmusic_app->gpio_dev, &btmusic_gpio_cb);
    
	os_delayed_work_init(&p_btmusic_app->aux_plug_in_delay_work, _btmusic_aux_plug_in_handle);
#endif

	return ret;
}

static void _btmusic_exit(void)
{
	if (!p_btmusic_app)
		goto exit;

	p_btmusic_app->app_exiting 	= 1;

	if (thread_timer_is_running(&p_btmusic_app->monitor_timer))
		thread_timer_stop(&p_btmusic_app->monitor_timer);

	if (thread_timer_is_running(&p_btmusic_app->play_timer))
		thread_timer_stop(&p_btmusic_app->play_timer);

	if (thread_timer_is_running(&p_btmusic_app->key_timer))
		thread_timer_stop(&p_btmusic_app->key_timer);

	if(p_btmusic_app->need_resume_key)
		ui_key_filter(false);

	bt_manager_stream_enable(STREAM_TYPE_A2DP, false);

	bt_music_stop_play();
#ifdef CONFIG_BT_MUSIC_SWITCH_LANTENCY_SUPPORT
    audio_policy_set_a2dp_lantency_time(0);
#endif
	btmusic_view_deinit();

#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
	gpio_remove_callback(p_btmusic_app->gpio_dev, &btmusic_gpio_cb);
    os_delayed_work_cancel(&p_btmusic_app->aux_plug_in_delay_work);
#endif
#endif

	app_mem_free(p_btmusic_app);
	p_btmusic_app = NULL;

	property_flush_req(NULL);

exit:
	app_manager_thread_exit(APP_ID_BTMUSIC);
	SYS_LOG_INF("exit finished");
}

static void btmusic_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;
	bool terminaltion = false;
	uint32_t start_time, cost_time;

	if (_btmusic_init())
	{
		SYS_LOG_ERR("init failed");
		_btmusic_exit();
		return;
	}

	while (!terminaltion) 
	{
		if (receive_msg(&msg, thread_timer_next_timeout())) 
		{
            app_msg_print(&msg, __FUNCTION__);
			start_time = k_cycle_get_32();
			switch (msg.type) {

				case MSG_EXIT_APP:
				{
					_btmusic_exit();
					terminaltion = true;
					break;
				}

				case MSG_BT_EVENT:
				{
					btmusic_bt_event_proc(&msg);
					break;
				}

				case MSG_INPUT_EVENT:
				{
					btmusic_input_event_proc(&msg);
					break;
				}

				case MSG_TTS_EVENT:
				{
					btmusic_tts_event_proc(&msg);
					break;
				}

				case MSG_TWS_EVENT:
				{
					btmusic_tws_event_proc(&msg);
					break;
				}

				case MSG_SUSPEND_APP:
				{
					break;
				}

				case MSG_RESUME_APP:
				{
					break;
				}

				case MSG_APP_SYNC_KEY_MAP:
				{
					btmusic_view_init(true);
					break;
				}
				case MSG_APP_SYNC_MUSIC_DAE:
				{
					u8_t dae_index = msg.value;
					struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
					
					if (btmusic->multidae_enable == 0 || dae_index >= btmusic->dae_cfg_nums)
						break;
					
					btmusic_multi_dae_adjust(dae_index);
					break;
				}
                case MSG_APP_UPDATE_MUSIC_DAE:
                    printk("sssssss line=%d, func=%s\n", __LINE__, __func__);
                    btmusic_update_dae();
                    break;
				default:
				{
					SYS_LOG_ERR("error: 0x%x!", msg.type);
					break;
				}
			}

			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);

			cost_time = k_cyc_to_us_floor32(k_cycle_get_32() - start_time);
			if (cost_time > APP_PORC_MONITOR_TIME) {
				printk("xxxx:(%s) msg.type:%d run %d us\n",__func__, msg.type, cost_time);
			}
		}

		start_time = k_cycle_get_32();
		thread_timer_handle_expired();
		cost_time = k_cyc_to_us_floor32(k_cycle_get_32() - start_time);

		if (cost_time > APP_PORC_MONITOR_TIME) {
			printk("xxxx:(%s) timer run %d us\n",__func__, cost_time);
		}
		
	}
}

struct btmusic_app_t *btmusic_get_app(void)
{
	return p_btmusic_app;
}

APP_DEFINE(btmusic, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   DEFAULT_APP | FOREGROUND_APP, NULL, NULL, NULL,
	   btmusic_main_loop, NULL);
