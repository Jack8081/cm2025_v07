/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tr_bqbput media
 */


#include "tr_bqb_app.h"
#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

int tr_bqb_get_status(void)
{
	struct tr_bqb_app_t *tr_bqbput = tr_bqb_get_app();

	if (!tr_bqbput)
		return BQB_STATUS_NULL;

	if (tr_bqbput->recording)
		return BQB_STATUS_PLAYING;
	else
		return BQB_STATUS_PAUSED;
}

static io_stream_t _tr_bqb_create_outputstream(void)
{
	int ret = 0;
	io_stream_t output_stream = NULL;

	output_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_MIC_IN ),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_MIC_IN ));
	if (!output_stream) {
		goto exit;
	}

	ret = stream_open(output_stream, MODE_IN_OUT);
	if (ret) {
		stream_destroy(output_stream);
		output_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p\n", output_stream);
	return	output_stream;
}


void tr_bqb_start_record(void)
{
	struct tr_bqb_app_t *tr_bqbput = tr_bqb_get_app();
	media_init_param_t init_param;
	//u8_t codec_id = bt_manager_a2dp_get_codecid();
	u8_t sample_rate = bt_manager_a2dp_get_sample_rate();
	u8_t bit_rate = tr_bt_manager_a2dp_get_bit_rate();
    //u8_t sample_rate = 44;
    //u8_t bit_rate = 53;

	if (!tr_bqbput)
		return;

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(true);
	tts_manager_lock();
#endif

    thread_timer_stop(&tr_bqbput->monitor_timer);

	if (tr_bqbput->recorder) {
		SYS_LOG_INF(" recorder already open ");
		return;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	init_param.type = MEDIA_SRV_TYPE_CAPTURE;
	init_param.stream_type = AUDIO_STREAM_MIC_IN;
	init_param.efx_stream_type = init_param.stream_type;
	init_param.event_notify_handle = NULL;
	init_param.support_tws = 0;
	init_param.dumpable = 1;

	init_param.capture_format = SBC_TYPE;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = _tr_bqb_create_outputstream();

	init_param.capture_bit_rate = bit_rate;
		/* TWS requires sbc encoder which only support 32/44.1/48 kHz */

    init_param.capture_channels_input = 2;
    init_param.capture_sample_rate_input = sample_rate;
    init_param.capture_sample_rate_output = sample_rate;
    //init_param.channels = 2;
    init_param.capture_channels_output = 2;
    ////////////////
    /*init_param.type = MEDIA_SRV_TYPE_CAPTURE;
    init_param.stream_type = AUDIO_STREAM_LINEIN;
    init_param.efx_stream_type = AUDIO_STREAM_DEFAULT;
    init_param.capture_format = SBC_TYPE;

        init_param.capture_sample_rate_input = 48;
        init_param.capture_sample_rate_output = 48;
        init_param.capture_sample_bits = 16;
        init_param.capture_channels_input = 2;
        init_param.capture_channels_output = 1;
        init_param.capture_bit_rate = 51;*/

   ////////////////

	tr_bqbput->recorder = media_player_open(&init_param);
	if (!tr_bqbput->recorder) {
		SYS_LOG_ERR("recorder open failed\n");
		goto err_exit;
	}

	tr_bqbput->audio_out_stream = init_param.capture_output_stream;
	
	media_player_play(tr_bqbput->recorder);

	tr_bqbput->recording = 1;

	bt_manager_set_stream(STREAM_TYPE_A2DP, tr_bqbput->audio_out_stream);
	bt_manager_stream_enable(STREAM_TYPE_A2DP, true);
    //api_bt_a2dp_mute(1);

	SYS_LOG_INF("recorder open sucessed %p ", tr_bqbput->recorder);
	
err_exit:
    return;
}

void tr_bqb_stop_record(void)
{
	struct tr_bqb_app_t *tr_bqbput = tr_bqb_get_app();

	if (!tr_bqbput)
		return;

	if (!tr_bqbput->recorder)
		return;

	bt_manager_stream_enable(STREAM_TYPE_A2DP, false);
	bt_manager_set_stream(STREAM_TYPE_A2DP, NULL);

	media_player_stop(tr_bqbput->recorder);
	media_player_close(tr_bqbput->recorder);

	tr_bqbput->recording = 0;

	tr_bqbput->recorder = NULL;
	if (tr_bqbput->audio_out_stream) {
		stream_close(tr_bqbput->audio_out_stream);
		stream_destroy(tr_bqbput->audio_out_stream);
		tr_bqbput->audio_out_stream = NULL;
	}

	SYS_LOG_INF("stopped\n");

#ifdef CONFIG_PLAYTTS
	tts_manager_unlock();
#endif
}

void tr_bqb_start_or_stop_record(void)
{
	int status = tr_bqb_get_status();

	if(status == BQB_STATUS_PLAYING)
	{
		tr_bqb_stop_record();
	}
	else
	{
		tr_bqb_start_record();
	}
}
