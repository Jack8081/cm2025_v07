/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio_input app main.
 */

#include "audio_input.h"
#include "app_config.h"

#include <audio_hal.h>

static struct audio_input_app_t *p_audio_input = NULL;

static int _audio_input_init(void)
{
	if (p_audio_input)
		return 0;

	p_audio_input = app_mem_malloc(sizeof(struct audio_input_app_t));
	if (!p_audio_input) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}

	memset(p_audio_input, 0, sizeof(struct audio_input_app_t));

	audio_input_view_init(false);

	/* FIXME:init set to max volume for easy to test */
	audio_system_set_stream_volume(audio_input_get_audio_stream_type(app_manager_get_current_app()), CFG_MAX_BT_MUSIC_VOLUME);
	/* FIXME:disable keytone & tts */
	extern void sys_manager_enable_audio_tone(bool enable);
	extern void sys_manager_enable_audio_voice(bool enable);
	sys_manager_enable_audio_tone(false);
	sys_manager_enable_audio_voice(false);

	SYS_LOG_INF("ok\n");
	return 0;
}
void _audio_input_exit(void)
{
	if (!p_audio_input)
		goto exit;

	audio_input_stop_play();

	if(p_audio_input->need_resume_key){
		ui_key_filter(false);
	}

	audio_input_view_deinit();

	/* FIXME:enable keytone & tts */
	extern void sys_manager_enable_audio_tone(bool enable);
	extern void sys_manager_enable_audio_voice(bool enable);
	sys_manager_enable_audio_tone(true);
	sys_manager_enable_audio_voice(true);

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

	app_mem_free(p_audio_input);

	p_audio_input = NULL;

exit:
	app_manager_thread_exit(app_manager_get_current_app());

	SYS_LOG_INF("exit finished\n");
}

extern void audio_input_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg);
static void _audio_input_restore_play_state(uint8_t init_play_state)
{
	if (init_play_state == AUDIO_INPUT_STATUS_PLAYING) {
		p_audio_input->playing = 1;
		if (thread_timer_is_running(&p_audio_input->monitor_timer)) {
			thread_timer_stop(&p_audio_input->monitor_timer);
		}
		thread_timer_init(&p_audio_input->monitor_timer, audio_input_delay_resume, NULL);
		thread_timer_start(&p_audio_input->monitor_timer, 200, 0);
	} else {
		p_audio_input->playing = 0;
	}
	audio_input_view_display();
}

static void _audio_input_main_prc(uint8_t init_play_state)
{
	struct app_msg msg = { 0 };
	int result = 0;
	bool terminated = false;

	if (_audio_input_init()) {
		SYS_LOG_ERR("init failed");
		_audio_input_exit();
		return;
	}

	_audio_input_restore_play_state(init_play_state);

	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n", msg.type, msg.value);
			switch (msg.type) {
			case MSG_INPUT_EVENT:
				audio_input_input_event_proc(&msg);
				break;
			case MSG_TTS_EVENT:
				audio_input_tts_event_proc(&msg);
				break;
			case MSG_BT_EVENT:
				//audio_input_bt_event_proc(&msg);
				break;
			case MSG_EXIT_APP:
				_audio_input_exit();
				terminated = true;
				break;
			default:
				SYS_LOG_WRN("error: 0x%x!\n", msg.type);
				break;
			}
			if (msg.callback)
				msg.callback(&msg, result, NULL);
		}
		if (!terminated)
			thread_timer_handle_expired();
	}
}

struct audio_input_app_t *audio_input_get_app(void)
{
	return p_audio_input;
}

static void audio_input_linein_main_loop(void *parama1, void *parama2, void *parama3)
{
   SYS_LOG_INF("Enter\n");
   _audio_input_main_prc(AUDIO_INPUT_STATUS_PLAYING);
   SYS_LOG_INF("Exit\n");
}

APP_DEFINE(linein, share_stack_area, sizeof(share_stack_area),
	   CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	   audio_input_linein_main_loop, NULL);

static void audio_input_i2srx_in_main_loop(void *parama1, void *parama2, void *parama3)
{
   SYS_LOG_INF("Enter\n");
   _audio_input_main_prc(AUDIO_INPUT_STATUS_PLAYING);
   SYS_LOG_INF("Exit\n");
}

APP_DEFINE(i2srx_in, share_stack_area, sizeof(share_stack_area),
	  CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	  audio_input_i2srx_in_main_loop, NULL);
