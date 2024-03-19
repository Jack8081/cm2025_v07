/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio_input media
 */


#include "audio_input.h"
#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#define SAMPLE_BITS_DEF  24 /*16 or 24*/

static io_stream_t _audio_input_create_inputstream(void)
{
	int ret = 0;
	io_stream_t input_stream = NULL;
	int stream_type = audio_input_get_audio_stream_type(app_manager_get_current_app());

	input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, stream_type),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, stream_type));
	if (!input_stream) {
		goto exit;
	}

	ret = stream_open(input_stream, MODE_IN_OUT);
	if (ret) {
		stream_destroy(input_stream);
		input_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p\n", input_stream);
	return	input_stream;
}

void audio_input_start_play(void)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();
	media_init_param_t init_param;
	io_stream_t input_stream = NULL;
	int stream_type = audio_input_get_audio_stream_type(app_manager_get_current_app());

	if (!audio_input)
		return;

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	if (audio_input->player) {
		SYS_LOG_INF(" player already open ");
		return;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	input_stream = _audio_input_create_inputstream();
	init_param.type = MEDIA_SRV_TYPE_PLAYBACK_AND_CAPTURE;
	init_param.stream_type = stream_type;
	init_param.efx_stream_type = stream_type;
	init_param.format = PCM_DSP_TYPE;
	init_param.input_stream = input_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.support_tws = 1;
	init_param.dumpable = 0;
	init_param.dsp_output = 1;
	init_param.dsp_latency_calc = 1;
	init_param.audiopll_mode = 2;

	int threshold_us = audiolcy_get_latency_threshold(init_param.format);
	init_param.latency_us = threshold_us;

	init_param.capture_format = PCM_DSP_TYPE;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = input_stream;

	init_param.channels = 2;
	init_param.sample_rate = 48;
	init_param.sample_bits = (SAMPLE_BITS_DEF == 24)? 32: 16;
	init_param.capture_channels_input = 2;
	init_param.capture_channels_output = 2;
	init_param.capture_sample_bits = SAMPLE_BITS_DEF;
	init_param.capture_sample_rate_input = 48;
	init_param.capture_sample_rate_output = 48;
	if (stream_type == AUDIO_STREAM_I2SRX_IN) {
		init_param.capture_source = 2;
	} else {
		init_param.capture_source = 1;
	}

	audio_input->player = media_player_open(&init_param);
	if (!audio_input->player) {
		SYS_LOG_ERR("player open failed\n");
		goto err_exit;
	}

	audio_input->audio_input_stream = init_param.input_stream;

	media_player_fade_in(audio_input->player, 60);

	media_player_set_audio_latency(audio_input->player, threshold_us); 

	media_player_pre_play(audio_input->player);
	media_player_play(audio_input->player);

	audio_input->playing = 1;

	SYS_LOG_INF("player open sucessed %p ", audio_input->player);
	return;
err_exit:
	if (input_stream) {
		stream_close(input_stream);
		stream_destroy(input_stream);
	}
}

void audio_input_stop_play(void)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();

	if (!audio_input)
		return;

	if (!audio_input->player)
		return;

	media_player_fade_out(audio_input->player, 60);

	/** reserve time to fade out*/
	os_sleep(60);

	if (audio_input->audio_input_stream)
		stream_close(audio_input->audio_input_stream);

	media_player_stop(audio_input->player);

	media_player_close(audio_input->player);

	audio_input->playing = BT_STATUS_PAUSED;

	audio_input->player = NULL;
	if (audio_input->audio_input_stream) {
		stream_destroy(audio_input->audio_input_stream);
		audio_input->audio_input_stream = NULL;
	}

	SYS_LOG_INF("stopped\n");
}

