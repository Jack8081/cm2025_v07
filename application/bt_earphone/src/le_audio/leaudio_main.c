/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief le audio app main.
 */

#include "leaudio.h"
#include "app_config.h"

#ifdef CONFIG_LE_AUDIO_SR_BQB
#include <sys_wakelock.h>
#endif

static struct leaudio_app_t *p_leaudio_app;

#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
static struct gpio_callback leaudio_gpio_cb;

static void _leaudio_aux_plug_in_handle(struct k_work *work)
{
    struct app_msg new_msg = { 0 };

    if (system_check_aux_plug_in())
    {   
		new_msg.type = MSG_INPUT_EVENT;
		new_msg.cmd  = KEY_FUNC_POWER_OFF;
		send_async_msg(APP_ID_MAIN, &new_msg);
    }
}

static void _leaudio_aux_plug_in_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
    SYS_LOG_INF("");
    os_delayed_work_submit(&p_leaudio_app->aux_plug_in_delay_work, 10);
}
#endif
#endif

static int _leaudio_init(void)
{
	CFG_Struct_BTMusic_Multi_Dae_Settings multidae_setting;

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	if(le_audio_is_in_background()) {
		le_audio_enter_foreground();
		leaudio_view_init(BTMUSIC_VIEW, false);
		audio_system_set_stream_volume(AUDIO_STREAM_LE_AUDIO_MUSIC, audio_system_get_stream_volume(AUDIO_STREAM_LE_AUDIO_MUSIC));	
		return 0;
	}

	bt_manager_allow_sco_connect(true);
#endif

	p_leaudio_app = app_mem_malloc(sizeof(struct leaudio_app_t));
	if (!p_leaudio_app) {
		SYS_LOG_ERR("malloc fail!\n");
		return -ENOMEM;
	}

	memset(p_leaudio_app, 0, sizeof(struct leaudio_app_t));

	app_config_read
    (
        CFG_ID_BTMUSIC_MULTI_DAE_SETTINGS,
        &multidae_setting, 0, sizeof(CFG_Struct_BTMusic_Multi_Dae_Settings)
    );

	p_leaudio_app->multidae_enable = multidae_setting.Enable;
	p_leaudio_app->dae_cfg_nums = multidae_setting.Cur_Dae_Num + 1;
	p_leaudio_app->current_dae = 0;

    /* 从vram读回当前音效 */
    if (p_leaudio_app->multidae_enable) {
        if (property_get(BTMUSIC_MULTI_DAE, &p_leaudio_app->current_dae, sizeof(uint8_t)) < 0) {
            p_leaudio_app->current_dae = 0;
			property_set(BTMUSIC_MULTI_DAE, &p_leaudio_app->current_dae, sizeof(uint8_t));
		}

        if (p_leaudio_app->current_dae > p_leaudio_app->dae_cfg_nums) {
            p_leaudio_app->current_dae = 0;
			property_set(BTMUSIC_MULTI_DAE, &p_leaudio_app->current_dae, sizeof(uint8_t));
		}

		leaudio_multi_dae_adjust(p_leaudio_app->current_dae);
    } else {
		app_config_read
		(
			CFG_ID_BT_MUSIC_DAE,
			&p_leaudio_app->dae_enalbe, offsetof(CFG_Struct_BT_Music_DAE, Enable_DAE), sizeof(cfg_uint8)
		);

		app_config_read
		(
			CFG_ID_BT_MUSIC_DAE_AL,
			p_leaudio_app->cfg_dae_data, 0, CFG_SIZE_BT_MUSIC_DAE_AL
		);
	}

	/* FIXME */
	leaudio_view_init(BTMUSIC_VIEW, false);

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    bt_ui_monitor_submit_work(2000);
#endif

	audio_system_set_stream_volume(AUDIO_STREAM_LE_AUDIO_MUSIC, audio_system_get_stream_volume(AUDIO_STREAM_LE_AUDIO_MUSIC));	

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
	leaudio_event_notify(SYS_EVENT_CUSTOMED_1);
	//leaudio_event_notify(SYS_EVENT_MIC_MUTE_ON);
    p_leaudio_app->gpio_dev = device_get_binding("GPIOA");
    gpio_pin_configure(p_leaudio_app->gpio_dev, 7, GPIO_OUTPUT_INACTIVE);
    
	gpio_pin_interrupt_configure(p_leaudio_app->gpio_dev, AUX_GPIO_PIN_IDX, GPIO_INT_EDGE_RISING);
	gpio_init_callback(&leaudio_gpio_cb, _leaudio_aux_plug_in_cb, BIT(AUX_GPIO_PIN_IDX));
	gpio_add_callback(p_leaudio_app->gpio_dev, &leaudio_gpio_cb);
	os_delayed_work_init(&p_leaudio_app->aux_plug_in_delay_work, _leaudio_aux_plug_in_handle);

    if (system_check_aux_plug_in())
    {
        gpio_pin_set(p_leaudio_app->gpio_dev, 7, 0);
    }
    else
    {
        gpio_pin_set(p_leaudio_app->gpio_dev, 7, 1);
    }
#endif

	SYS_LOG_INF("dae cfg: %d %d %d %d", p_leaudio_app->dae_enalbe, 
		p_leaudio_app->multidae_enable, p_leaudio_app->dae_cfg_nums, p_leaudio_app->current_dae);

#ifdef CONFIG_LE_AUDIO_SR_BQB
    sys_wake_lock(FULL_WAKE_LOCK);
#endif

	return 0;
}

extern bool bt_manager_startup_reconnect(void);

static void _leaudio_exit(void)
{
	if (!p_leaudio_app) {
		goto exit;
	}

#ifdef CONFIG_LE_AUDIO_SR_BQB
    sys_wake_unlock(FULL_WAKE_LOCK);
#endif

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	le_audio_check_set_background();
#endif

	if (thread_timer_is_running(&p_leaudio_app->monitor_timer))
		thread_timer_stop(&p_leaudio_app->monitor_timer);

	if (thread_timer_is_running(&p_leaudio_app->key_timer))
		thread_timer_stop(&p_leaudio_app->key_timer);

	if (thread_timer_is_running(&p_leaudio_app->delay_play_timer))
		thread_timer_stop(&p_leaudio_app->delay_play_timer);

	if(p_leaudio_app->need_resume_key){
		ui_key_filter(false);
	}

	leaudio_stop_playback();
	leaudio_stop_capture();

	if ((!p_leaudio_app->playback_player || !p_leaudio_app->playback_player_run)
		&& (!p_leaudio_app->capture_player || !p_leaudio_app->capture_player_run)) {
		leaudio_set_share_info(false, false);
		leaudio_exit_playback();
		leaudio_exit_capture();
	}

#ifdef CONFIG_PLAYTTS
    tts_manager_wait_finished(true);
#endif

	leaudio_view_deinit();

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	if(p_leaudio_app->leaudio_need_backgroud) {
		le_audio_enter_background();
		app_manager_thread_exit(APP_ID_LE_AUDIO);
		return;
	}
#endif

#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    gpio_remove_callback(p_leaudio_app->gpio_dev, &leaudio_gpio_cb);
    os_delayed_work_cancel(&p_leaudio_app->aux_plug_in_delay_work);
#endif
#endif

	app_mem_free(p_leaudio_app);
	p_leaudio_app = NULL;

exit:
	app_manager_thread_exit(APP_ID_LE_AUDIO);

	SYS_LOG_INF("exit finished\n");
}

static void leaudio_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;
	bool terminaltion = false;

	if (_leaudio_init()) {
		SYS_LOG_ERR("init failed");
		_leaudio_exit();
		return;
	}

	bt_manager_audio_stream_restore(BT_TYPE_LE);

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
            app_msg_print(&msg, __FUNCTION__);
			switch (msg.type) {
			case MSG_EXIT_APP:
				_leaudio_exit();
				terminaltion = true;
				break;
			case MSG_BT_EVENT:
				leaudio_bt_event_proc(&msg);
				break;
			case MSG_INPUT_EVENT:
				leaudio_input_event_proc(&msg);
				break;
			case MSG_TTS_EVENT:
				leaudio_tts_event_proc(&msg);
				break;
			case MSG_TWS_EVENT:
				leaudio_tws_event_proc(&msg);
				break;
			default:
				SYS_LOG_WRN("error: 0x%x!\n", msg.type);
				break;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
		thread_timer_handle_expired();
	}
}

struct leaudio_app_t *leaudio_get_app(void)
{
	return p_leaudio_app;
}

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL

void leaudio_restore(struct thread_timer *ttimer, void *expiry_fn_arg);
void leaudio_bt_event_proc(struct app_msg *msg);

void le_audio_enter_background(void)
{
    p_leaudio_app->leaudio_backgroud = 1;
	p_leaudio_app->leaudio_need_backgroud = 0;
	
	SYS_LOG_INF("");
#ifdef CONFIG_PLAYTTS
	tip_manager_lock();
    tts_manager_lock();
    tts_manager_wait_finished(false);
#endif

	if(p_leaudio_app->tts_playing) {
		p_leaudio_app->tts_playing = 0;
	}
}

void le_audio_enter_foreground(void)
{
	if(p_leaudio_app->leaudio_backgroud) {
		p_leaudio_app->leaudio_backgroud = 0;
		
		SYS_LOG_INF("");
#ifdef CONFIG_PLAYTTS
		tts_manager_unlock();
		tip_manager_unlock();
#endif
	}
}

void le_audio_check_set_background(void)
{
	if(p_leaudio_app && p_leaudio_app->leaudio_backgroud == 0) {
		const u8_t * app_name = app_manager_get_next_app();
		if(app_name && strcmp(app_name, APP_ID_BTCALL) == 0 
			&& (p_leaudio_app->playback_player 
				|| (p_leaudio_app->bt_sink_enabled && p_leaudio_app->tts_playing))) {
			p_leaudio_app->leaudio_need_backgroud = 1;
		}
	}
}

bool le_audio_is_in_background(void)
{
	if(p_leaudio_app && p_leaudio_app->leaudio_backgroud) {
		return true;
	}

	return false;
}

void le_audio_background_stop(void)
{
	if(le_audio_is_in_background() == false) {
		return;
	}

	SYS_LOG_INF("");
	leaudio_stop_playback();
	leaudio_set_share_info(false, false);
	leaudio_exit_playback();
}

void le_audio_background_prepare(void)
{
	if(le_audio_is_in_background() == false) {
		return;
	}
	
	SYS_LOG_INF("");
	leaudio_restore(&p_leaudio_app->monitor_timer, NULL);
}

void le_audio_background_start(void)
{
	if(le_audio_is_in_background() == false) {
		return;
	}
	
	SYS_LOG_INF("");
	leaudio_start_playback();
}


void le_audio_background_exit(void)
{
	if(le_audio_is_in_background()) {
		struct leaudio_app_t *leaudio_app = p_leaudio_app;

		SYS_LOG_INF("");
		le_audio_background_stop();
		bt_manager_event_notify(BT_CALL_REPLAY, NULL, 0);
	
		leaudio_app->leaudio_backgroud = 0;
		p_leaudio_app = NULL;
		
		app_mem_free(leaudio_app);
	}
}

void le_audio_background_bt_event_proc(struct app_msg *msg)
{
	if(le_audio_is_in_background() == false) {
		return;
	}
	
	leaudio_bt_event_proc(msg);
}
#endif

APP_DEFINE(le_audio, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   FOREGROUND_APP, NULL, NULL, NULL,
	   leaudio_main_loop, NULL);

#ifdef CONFIG_LE_AUDIO_SR_BQB
#include <shell/shell.h>

static int leaudio_release_sink(const struct shell *shell, size_t argc, char *argv[])
{
    struct leaudio_app_t *leaudio = leaudio_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	leaudio->sink_chan.handle = leaudio->handle;
	leaudio->sink_chan.id = id;

    return bt_manager_audio_stream_release_single(&p_leaudio_app->sink_chan);
}

static int leaudio_release_source(const struct shell *shell, size_t argc, char *argv[])
{
    struct leaudio_app_t *leaudio = leaudio_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	leaudio->source_chan.handle = leaudio->handle;
	leaudio->source_chan.id = id;

    return bt_manager_audio_stream_release_single(&p_leaudio_app->source_chan);
}

static int leaudio_codec_sink(const struct shell *shell, size_t argc, char *argv[])
{
   struct bt_audio_preferred_qos qos;
   struct bt_audio_codec codec;
   uint32_t type;

   if (argc != 2) {
       SYS_LOG_ERR("argc: %d", argc);
       return -EINVAL;
   }

   type = strtoul(argv[1], (char **) NULL, 0);
   SYS_LOG_INF("type: %d", type);

   codec.handle = p_leaudio_app->handle;
   codec.id = 1;
   codec.dir = BT_AUDIO_DIR_SINK;
   codec.format = BT_AUDIO_CODEC_LC3;
   codec.target_latency = BT_AUDIO_TARGET_LATENCY_LOW;
   codec.target_phy = BT_AUDIO_TARGET_PHY_2M;
   codec.blocks = 1;
   codec.locations = BT_AUDIO_LOCATIONS_FL;

   switch (type) {
   case 81:
       codec.sample_rate = 8;
       codec.octets = 26;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 82:
       codec.sample_rate = 8;
       codec.octets = 30;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 161:
       codec.sample_rate = 16;
       codec.octets = 30;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 162:
       codec.sample_rate = 16;
       codec.octets = 40;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 241:
       codec.sample_rate = 24;
       codec.octets = 45;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 242:
       codec.sample_rate = 24;
       codec.octets = 60;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 321:
       codec.sample_rate = 32;
       codec.octets = 60;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 322:
       codec.sample_rate = 32;
       codec.octets = 80;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 481:
       codec.sample_rate = 48;
       codec.octets = 75;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 482:
       codec.sample_rate = 48;
       codec.octets = 100;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 483:
       codec.sample_rate = 48;
       codec.octets = 90;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 484:
       codec.sample_rate = 48;
       codec.octets = 120;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 485:
       codec.sample_rate = 48;
       codec.octets = 117;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 486:
       codec.sample_rate = 48;
       codec.octets = 155;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   default:
       return -EINVAL;
   }

   bt_manager_audio_stream_config_codec(&codec);

   qos.handle = p_leaudio_app->handle;
   qos.id = 1;
   qos.framing = BT_AUDIO_UNFRAMED_SUPPORTED;
   qos.phy = BT_AUDIO_PHY_2M;
   qos.rtn = 3;
   qos.delay_min = 35000;
   qos.delay_max = 40000;
   qos.preferred_delay_min = 35000;
   qos.preferred_delay_max = 40000;

   return bt_manager_audio_stream_prefer_qos(&qos);
}

static int leaudio_codec_source(const struct shell *shell, size_t argc, char *argv[])
{
   struct bt_audio_preferred_qos qos;
   struct bt_audio_codec codec;
   uint32_t type;

   if (argc != 2) {
       SYS_LOG_ERR("argc: %d", argc);
       return -EINVAL;
   }

   type = strtoul(argv[1], (char **) NULL, 0);
   SYS_LOG_INF("type: %d", type);

   codec.handle = p_leaudio_app->handle;
   codec.id = 3;
   codec.dir = BT_AUDIO_DIR_SOURCE;
   codec.format = BT_AUDIO_CODEC_LC3;
   codec.target_latency = BT_AUDIO_TARGET_LATENCY_LOW;
   codec.target_phy = BT_AUDIO_TARGET_PHY_2M;
   codec.frame_duration = BT_FRAME_DURATION_10MS;
   codec.blocks = 1;
   codec.locations = BT_AUDIO_LOCATIONS_FL;

   switch (type) {
   case 81:
       codec.sample_rate = 8;
       codec.octets = 26;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 82:
       codec.sample_rate = 8;
       codec.octets = 30;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 161:
       codec.sample_rate = 16;
       codec.octets = 30;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 162:
       codec.sample_rate = 16;
       codec.octets = 40;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 241:
       codec.sample_rate = 24;
       codec.octets = 45;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 242:
       codec.sample_rate = 24;
       codec.octets = 60;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 321:
       codec.sample_rate = 32;
       codec.octets = 60;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 322:
       codec.sample_rate = 32;
       codec.octets = 80;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 481:
       codec.sample_rate = 48;
       codec.octets = 75;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 482:
       codec.sample_rate = 48;
       codec.octets = 100;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 483:
       codec.sample_rate = 48;
       codec.octets = 90;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 484:
       codec.sample_rate = 48;
       codec.octets = 120;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   case 485:
       codec.sample_rate = 48;
       codec.octets = 117;
       codec.frame_duration = BT_FRAME_DURATION_7_5MS;
       qos.latency = 8;
       break;
   case 486:
       codec.sample_rate = 48;
       codec.octets = 155;
       codec.frame_duration = BT_FRAME_DURATION_10MS;
       qos.latency = 10;
       break;
   default:
       return -EINVAL;
   }

   bt_manager_audio_stream_config_codec(&codec);

   qos.handle = p_leaudio_app->handle;
   qos.id = 3;
   qos.framing = BT_AUDIO_UNFRAMED_SUPPORTED;
   qos.phy = BT_AUDIO_PHY_2M;
   qos.rtn = 3;
   qos.delay_min = 11000;
   qos.delay_max = 40000;
   qos.preferred_delay_min = 11000;
   qos.preferred_delay_max = 40000;
   
   return bt_manager_audio_stream_prefer_qos(&qos);
}

static int leaudio_prefered_qos_sink(const struct shell *shell, size_t argc, char *argv[])
{
   struct bt_audio_preferred_qos qos;

   qos.handle = p_leaudio_app->handle;
   qos.id = 1;
   qos.framing = BT_AUDIO_UNFRAMED_SUPPORTED;
   qos.phy = BT_AUDIO_PHY_2M;
   qos.rtn = 3;
   qos.latency = 10;
   qos.delay_min = 35000;
   qos.delay_max = 40000;
   qos.preferred_delay_min = 35000;
   qos.preferred_delay_max = 40000;

   return bt_manager_audio_stream_prefer_qos(&qos);
}

static int leaudio_prefered_qos_source(const struct shell *shell, size_t argc, char *argv[])
{
   struct bt_audio_preferred_qos qos;

   qos.handle = p_leaudio_app->handle;
   qos.id = 3;
   qos.framing = BT_AUDIO_UNFRAMED_SUPPORTED;
   qos.phy = BT_AUDIO_PHY_2M;
   qos.rtn = 3;
   qos.latency = 10;
   qos.delay_min = 11000;
   qos.delay_max = 40000;
   qos.preferred_delay_min = 11000;
   qos.preferred_delay_max = 40000;

   return bt_manager_audio_stream_prefer_qos(&qos);
}

static int leaudio_disable_sink(const struct shell *shell, size_t argc, char *argv[])
{
   return bt_manager_audio_stream_disable_single(&p_leaudio_app->sink_chan);
}

static int leaudio_disable_source(const struct shell *shell, size_t argc, char *argv[])
{
   return bt_manager_audio_stream_disable_single(&p_leaudio_app->source_chan);
}

static int leaudio_update_sink(const struct shell *shell, size_t argc, char *argv[])
{
   uint32_t contexts = BT_AUDIO_CONTEXT_MEDIA;

   return bt_manager_audio_stream_update_single(&p_leaudio_app->sink_chan, contexts);
}

static int leaudio_update_source(const struct shell *shell, size_t argc, char *argv[])
{
   uint32_t contexts = BT_AUDIO_CONTEXT_MEDIA;

   return bt_manager_audio_stream_update_single(&p_leaudio_app->source_chan, contexts);
}

int context_bqb[8] = {0x00030003, 0x00070007, 0x00010001, 0x000f000f};
static int leaudio_available_contexts_set(const struct shell *shell, size_t argc, char *argv[])
{
    uint8_t count;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	count = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("count: %d", count);
    return tr_bt_manager_contexts_set(1, 0, context_bqb[count]);
}

static int leaudio_supported_contexts_set(const struct shell *shell, size_t argc, char *argv[])
{
    uint8_t count;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	count = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("count: %d", count);
    return tr_bt_manager_contexts_set(0, 1, context_bqb[count]);
}

static int gtbs_accept(const struct shell *shell, size_t argc, char *argv[])
{
    uint8_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

    val = strtoul(argv[1], (char **) NULL, 0);

	SYS_LOG_INF("");

    struct bt_audio_call call_t = {0};
    call_t.handle = p_leaudio_app->handle;
    call_t
.index = val;

	return bt_manager_call_accept(&call_t);
}

uint8_t provider[20] = {0};
static int gtbs_set_provider(const struct shell *shell, size_t argc, char *argv[])
{
    char *cmd;
    uint8_t len = 0;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	SYS_LOG_INF("");

    cmd = argv[1];
    len = strlen(cmd);
    len = len > 20 ? 20: len;
    memcpy(provider, cmd, len);
    provider[len] = 0;

	return tr_bt_manager_server_provider_set(provider, len);
}

static int gtbs_set_tech(const struct shell *shell, size_t argc, char *argv[])
{
    uint8_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	val = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("");

	return tr_bt_manager_server_tech_set(val);
}

static int gtbs_set_status(const struct shell *shell, size_t argc, char *argv[])
{
    uint16_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	val = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("");

	return tr_bt_manager_server_status_set(val);
}


static int vol_set_abs(const struct shell *shell, size_t argc, char *argv[])
{
    uint16_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	val = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("");

	return tr_bt_manager_volume_init_set(val);
}

static int csis_set_not_bonded(const struct shell *shell, size_t argc, char *argv[])
{
    uint16_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	val = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("");

	tr_bt_manager_csis_not_bonded_set(val);
    return 0;
}

static int csis_set_lock_val(const struct shell *shell, size_t argc, char *argv[])
{
    uint16_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	val = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("");

	tr_bt_manager_csis_update_lockstate(val);
    return 0;
}

static int mics_mute(const struct shell *shell, size_t argc, char *argv[])
{
    uint16_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	val = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("");

	tr_bt_manager_mics_mute(p_leaudio_app->handle, val);
    return 0;
}

static int lea_adv_stop(const struct shell *shell, size_t argc, char *argv[])
{

	SYS_LOG_INF("");

	bt_manager_audio_stop(BT_TYPE_LE);
    return 0;
}

static int lea_adv_start(const struct shell *shell, size_t argc, char *argv[])
{

	SYS_LOG_INF("");

	bt_manager_audio_start(BT_TYPE_LE);
    return 0;
}

static int mcs_adv_set(const struct shell *shell, size_t argc, char *argv[])
{

	SYS_LOG_INF("");

    tr_bt_manager_audio_mcs_adv_set();

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(lea_app_cmds,
   SHELL_CMD(rsink, NULL, "LE Audio release sink", leaudio_release_sink),
   SHELL_CMD(rsrc, NULL, "LE Audio release source", leaudio_release_source),

   SHELL_CMD(csink, NULL, "LE Audio codec sink", leaudio_codec_sink),
   SHELL_CMD(csrc, NULL, "LE Audio codec source", leaudio_codec_source),

   SHELL_CMD(psink, NULL, "LE Audio prefered_qos sink", leaudio_prefered_qos_sink),
   SHELL_CMD(psrc, NULL, "LE Audio prefered_qos source", leaudio_prefered_qos_source),

   SHELL_CMD(dsink, NULL, "LE Audio disable sink", leaudio_disable_sink),
   SHELL_CMD(dsrc, NULL, "LE Audio disable source", leaudio_disable_source),

   SHELL_CMD(usink, NULL, "LE Audio update sink", leaudio_update_sink),
   SHELL_CMD(usrc, NULL, "LE Audio update source", leaudio_update_source),

   SHELL_CMD(setac, NULL, "LE Audio available_contexts_set", leaudio_available_contexts_set),
   SHELL_CMD(setsc, NULL, "LE Audio supported_contexts_set", leaudio_supported_contexts_set),

   SHELL_CMD(tbsac, NULL, "LE Audio tbs accept", gtbs_accept),
   SHELL_CMD(tbssp, NULL, "LE Audio tbs set_provider", gtbs_set_provider),
   SHELL_CMD(tbsst, NULL, "LE Audio tbs set_tech", gtbs_set_tech),
   SHELL_CMD(tbsss, NULL, "LE Audio tbs set_status", gtbs_set_status),

   SHELL_CMD(volsa, NULL, "LE Audio vol set_abs", vol_set_abs),

   SHELL_CMD(csissnb, NULL, "LE Audio csis set_not_bonded", csis_set_not_bonded),
   SHELL_CMD(csisslv, NULL, "LE Audio csis set_lock_val", csis_set_lock_val),

   SHELL_CMD(micsmute, NULL, "LE Audio mics mute", mics_mute),

   SHELL_CMD(stopadv, NULL, "LE Audio stop adv", lea_adv_stop),
   SHELL_CMD(startadv, NULL, "LE Audio start adv", lea_adv_start),

   SHELL_CMD(mcsas, NULL, "LE Audio adv set", mcs_adv_set),


   SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(lea, &lea_app_cmds, "leaudio commands", NULL);
#endif

