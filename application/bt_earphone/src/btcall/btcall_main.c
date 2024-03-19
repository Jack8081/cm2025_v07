/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call main.
 */

#include "btcall.h"
#include "tts_manager.h"


static struct btcall_app_t *p_btcall_app;
int64_t btcall_exit_time = 0;
#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
static struct gpio_callback btcall_gpio_cb;

static void _btcall_aux_plug_in_handle(struct k_work *work)
{
    struct app_msg new_msg = { 0 };
    
    if (system_check_aux_plug_in())
    {   
		new_msg.type = MSG_INPUT_EVENT;
		new_msg.cmd  = KEY_FUNC_POWER_OFF;
		send_async_msg(APP_ID_MAIN, &new_msg);
    }   
}

static void _btcall_aux_plug_in_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
    SYS_LOG_INF("");
    os_delayed_work_submit(&p_btcall_app->aux_plug_in_delay_work, 10);  
}
#endif
#endif

static int _btcall_config_init(void)
{
	struct btcall_app_t *btcall = btcall_get_app();
    CFG_Struct_System_Settings    cfg_sys_settings;
	
    /* 系统配置
     */
    app_config_read
    (
        CFG_ID_SYSTEM_SETTINGS, 
        &cfg_sys_settings, 0, sizeof(CFG_Struct_System_Settings)
    );
	btcall->enable_tts_in_calling = cfg_sys_settings.Enable_Voice_Prompt_In_Calling;

	/* 读取"来电提示"配置 */
	app_config_read
	(
		CFG_ID_INCOMING_CALL_PROMPT, 
		&btcall->incoming_config, 0, sizeof(CFG_Struct_Incoming_Call_Prompt)
	);


    app_config_read
    (
        CFG_ID_BTSPEECH_USER_SETTINGS, 
        &btcall->user_settings, 0, sizeof(CFG_Struct_BTSpeech_User_Settings)
    );

	return 0;
}

static int _btcall_init(void)
{
	int ret = 0;

	p_btcall_app = app_mem_malloc(sizeof(struct btcall_app_t));
	if (!p_btcall_app) {
	    SYS_LOG_ERR("malloc fail!\n");
		ret = -ENOMEM;
		return ret;
	}

	_btcall_config_init();
#ifdef CONFIG_BT_PTS_TEST
    audio_system_set_stream_volume(AUDIO_STREAM_VOICE, 16);
#else
	audio_system_set_stream_volume(AUDIO_STREAM_VOICE, audio_system_get_stream_volume(AUDIO_STREAM_VOICE));	
#endif
	bt_manager_set_stream_type(AUDIO_STREAM_VOICE);

	btcall_view_init(false);

	btcall_ring_init();
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    p_btcall_app->gpio_dev = device_get_binding(AUX_GPIO_PORT_NAME);
	gpio_pin_interrupt_configure(p_btcall_app->gpio_dev, AUX_GPIO_PIN_IDX, GPIO_INT_EDGE_RISING);
	gpio_init_callback(&btcall_gpio_cb, _btcall_aux_plug_in_cb, BIT(AUX_GPIO_PIN_IDX));
	gpio_add_callback(p_btcall_app->gpio_dev, &btcall_gpio_cb);
    
	os_delayed_work_init(&p_btcall_app->aux_plug_in_delay_work, _btcall_aux_plug_in_handle);
#endif

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
    if(le_audio_is_in_background()) {
        p_btcall_app->uac_gain = BTACLL_UAC_GAIN_3;

        bt_manager_volume_uac_gain(p_btcall_app->uac_gain);
        tts_manager_unlock();
        tip_manager_unlock();
    }
#endif

	SYS_LOG_INF("finished\n");
	return ret;
}

static void _btcall_exit(void)
{
	if (!p_btcall_app)
		goto exit;

	p_btcall_app->app_exiting 	= 1;
    
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
    bt_manager_volume_uac_gain(BTACLL_UAC_GAIN_0);
	le_audio_background_stop();
#endif

	thread_timer_stop(&p_btcall_app->key_timer);
    thread_timer_stop(&p_btcall_app->ring_timer);
    thread_timer_stop(&p_btcall_app->sco_check_timer);

	if(p_btcall_app->need_resume_key)
		ui_key_filter(false);

	/* 恢复播tts */
	if(p_btcall_app->disable_voice)
	{
		sys_manager_enable_audio_voice(TRUE);
		SYS_LOG_INF("### restore tts.\n");
		p_btcall_app->disable_voice = 0;
	}

	btcall_stop_play();

	btcall_ring_deinit();

	btcall_view_deinit();

    if((!p_btcall_app->enable_tts_in_calling) && (p_btcall_app->is_tts_locked)) {
        tts_manager_wait_finished(true);
        tts_manager_unlock();
        p_btcall_app->is_tts_locked = false;
    }

#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    gpio_remove_callback(p_btcall_app->gpio_dev, &btcall_gpio_cb);
    os_delayed_work_cancel(&p_btcall_app->aux_plug_in_delay_work);
#endif
#endif

    app_mem_free(p_btcall_app);
	p_btcall_app = NULL;
    btcall_exit_time = os_uptime_get();

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
    if(le_audio_is_in_background()) {
        tip_manager_lock();
        tts_manager_lock();
        tts_manager_wait_finished(false);
    }
#endif

exit:
	app_manager_thread_exit(APP_ID_BTCALL);
	SYS_LOG_INF(" finished \n");
}

#ifdef CONFIG_SPPBLE_DATA_CHAN
void app_ctrl(u8_t parm);
#endif

static void btcall_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;
	bool terminaltion = false;
	uint32_t start_time, cost_time;

#if defined(CONFIG_SPPBLE_DATA_CHAN) && defined(CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL)
    app_ctrl(1);
#endif

	if (_btcall_init()) {
		SYS_LOG_ERR(" init failed");
		_btcall_exit();
#if defined(CONFIG_SPPBLE_DATA_CHAN) && defined(CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL)
        app_ctrl(0);
#endif
		return;
	}

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
            app_msg_print(&msg, __FUNCTION__);
			start_time = k_cycle_get_32();
			
			switch (msg.type) {
				case MSG_EXIT_APP:
					_btcall_exit();
					terminaltion = true;
					break;
					
				case MSG_BT_EVENT:
				    btcall_bt_event_proc(&msg);
					break;
				
				case MSG_INPUT_EVENT:
				    btcall_input_event_proc(&msg);
					break;
				
				case MSG_TTS_EVENT:
					btcall_tts_event_proc(&msg);
					break;

				case MSG_TWS_EVENT:
					btcall_tws_event_proc(&msg);
					break;

				case MSG_APP_BTCALL_EVENT:
					btcall_app_event_proc(&msg);
					break;

				case MSG_APP_SYNC_KEY_MAP:
				{
					btcall_view_init(true);
					break;
				}					
				default:
					SYS_LOG_ERR("error: 0x%x \n", msg.type);
					break;
			}
			
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
			
			cost_time = k_cyc_to_us_floor32(k_cycle_get_32() - start_time) ;
			if (cost_time > APP_PORC_MONITOR_TIME) {
				printk("xxxx:(%s) msg.type:%d run %d us\n",__func__, msg.type, cost_time);
			}
		}

		start_time = k_cycle_get_32();
		thread_timer_handle_expired();
		cost_time = k_cyc_to_us_floor32(k_cycle_get_32() - start_time) ;

		if (cost_time > APP_PORC_MONITOR_TIME) {
			printk("xxxx:(%s) timer run%d us\n",__func__, cost_time);
		}
	}

#if defined(CONFIG_SPPBLE_DATA_CHAN) && defined(CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL)
    app_ctrl(0);
#endif
}

struct btcall_app_t *btcall_get_app(void)
{
	return p_btcall_app;
}

APP_DEFINE(btcall, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   FOREGROUND_APP, NULL, NULL, NULL,
	   btcall_main_loop, NULL);
