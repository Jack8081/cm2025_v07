/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio_input event
 */

#include "audio_input.h"
#include "tts_manager.h"

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

int audio_input_get_audio_stream_type(char *app_name)
{
	if (memcmp(app_name, APP_ID_LINEIN, strlen(APP_ID_LINEIN)) == 0)
		return AUDIO_STREAM_MIC_IN;
	else if (memcmp(app_name, APP_ID_MIC_IN, strlen(APP_ID_MIC_IN)) == 0)
		return AUDIO_STREAM_MIC_IN;
	else if (memcmp(app_name, APP_ID_I2SRX_IN, strlen(APP_ID_I2SRX_IN)) == 0)
		return AUDIO_STREAM_I2SRX_IN;

	SYS_LOG_WRN("%s unsupport\n", app_name);
	return 0;
}

void audio_input_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();
	audio_input_start_play();
	audio_input->need_resume_play = 0;
}

void audio_input_tts_event_proc(struct app_msg *msg)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();

	if (!audio_input)
		return;

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		if (audio_input->player) {
			audio_input->need_resume_play = 1;
			audio_input_stop_play();
		}
		break;
	case TTS_EVENT_STOP_PLAY:
		if (audio_input->need_resume_play && !audio_input->playing) {
			if (thread_timer_is_running(&audio_input->monitor_timer)) {
				thread_timer_stop(&audio_input->monitor_timer);
			}
			thread_timer_init(&audio_input->monitor_timer, audio_input_delay_resume, NULL);
			thread_timer_start(&audio_input->monitor_timer, 200, 0);
		}
		break;
	default:
		break;
	}
}

