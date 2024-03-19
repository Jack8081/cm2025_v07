/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music media.
 */
#include "btmusic.h"
#include "media_mem.h"
#include <ringbuff_stream.h>
#include "tts_manager.h"
#include "ui_manager.h"
#include "app_common.h"

extern io_stream_t dma_upload_stream_create(void);

static io_stream_t _bt_music_a2dp_create_inputstream(void)
{
	int ret = 0;
	io_stream_t input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC));

	if (!input_stream) {
		goto exit;
	}

	ret = stream_open(input_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
	if (ret) {
		stream_destroy(input_stream);
		input_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p", input_stream);
	return	input_stream;
}

int bt_music_start_play(void)
{
	media_init_param_t init_param;
	io_stream_t input_stream = NULL;
	struct btmusic_app_t *btmusic = btmusic_get_app();
    CFG_Struct_BTMusic_User_Settings *user_settings = &btmusic->user_settings;
	struct bt_audio_chan *chan = &btmusic->sink_chan;
	struct bt_audio_chan_info chan_info;
    const void *dae_param = NULL;
	int ret;

	if (!btmusic)
		return -EINVAL;

	ret = bt_manager_audio_stream_info(chan, &chan_info);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_BT_MUSIC_SWITCH_LANTENCY_SUPPORT
    if(btmusic->lantency_mode) {
        audio_policy_set_a2dp_lantency_time(CONFIG_BT_MUSIC_LOW_LANTENCY_TIME_MS);
    } else {
        audio_policy_set_a2dp_lantency_time(0);
    }
#endif

	if (btmusic->player) {
		if (btmusic->sink_stream) {
			//bt_manager_set_codec(chan_info.format);
			//bt_manager_audio_sink_stream_set(chan, btmusic->sink_stream);
			//media_player_play(btmusic->player);
			SYS_LOG_INF("already open");
			return 0;
		}
		media_player_stop(btmusic->player);
		media_player_close(btmusic->player);
		os_sleep(200);
	}

	memset(&init_param, 0, sizeof(media_init_param_t));
	init_param.type = MEDIA_SRV_TYPE_PLAYBACK;

	SYS_LOG_INF("codec_id: %d sample_rate %d, data witdh: %u, fade in: %u, fade out: %u",
        chan_info.format, chan_info.sample,
        user_settings->BM_DataWidth,
        user_settings->BM_Fadein_Continue_Time,
        user_settings->BM_Fadeout_Continue_Time);


	init_param.format = bt_manager_audio_codec_type(chan_info.format);	
	input_stream = _bt_music_a2dp_create_inputstream();
	init_param.stream_type = AUDIO_STREAM_MUSIC;
	init_param.efx_stream_type = AUDIO_STREAM_MUSIC;
	init_param.sample_rate = chan_info.sample;
#ifdef CONFIG_BT_MUSIC_RES_SR_UP_TO_48
    if(chan_info.sample != 48)
	    init_param.res_out_sample_rate = 48;
#endif
	init_param.input_stream = input_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.support_tws = 1;
    init_param.dsp_output = 1;
	init_param.dumpable = 1;
    init_param.sample_bits = user_settings->BM_DataWidth * 8;

	if (audio_policy_get_out_audio_mode(init_param.stream_type) == AUDIO_MODE_STEREO) {
		init_param.channels = 2;
	} else {
		init_param.channels = 1;
	}

	btmusic->sink_stream = input_stream;

	btmusic->player = media_player_open(&init_param);
	if (!btmusic->player) {
        SYS_LOG_ERR("open failed");
		goto err_exit;
	}

	btmusic->media_opened = 1;
	bt_manager_set_codec(chan_info.format);
	bt_manager_audio_sink_stream_set(chan, input_stream);

	int threshold = audiolcy_get_latency_threshold(init_param.format);
	media_player_set_audio_latency(btmusic->player, threshold); 
	
	bt_music_check_channel_output(true);

	media_player_fade_in(btmusic->player, user_settings->BM_Fadein_Continue_Time);

	media_player_set_effect_enable(btmusic->player, btmusic->dae_enalbe);

    media_effect_get_param(AUDIO_STREAM_MUSIC, AUDIO_STREAM_MUSIC, &dae_param, NULL);
    if(!dae_param) {
    	btmusic_update_dae();
    }

	media_player_play(btmusic->player);

	SYS_LOG_INF(" %p", btmusic->player);

	btmusic_view_display();

	return 0;

err_exit:
	if (input_stream) {
		stream_close(input_stream);
		stream_destroy(input_stream);
		bt_manager_audio_sink_stream_set(chan, NULL);
	}

	SYS_LOG_ERR("open failed\n");
	return -EINVAL;	
}

int bt_music_handle_enable(struct bt_audio_report *rep)
{
	struct bt_audio_chan *sink_chan = &btmusic_get_app()->sink_chan;

	sink_chan->handle = rep->handle;
	sink_chan->id = rep->id;

	return 0;
}

int bt_music_handle_start(struct bt_audio_report *rep)
{
	return bt_music_start_play();
}

int bt_music_handle_stop(struct bt_audio_report *rep)
{
	return bt_music_stop_play();
}


int bt_music_stop_play(void)
{
	struct btmusic_app_t *btmusic = btmusic_get_app();

	if (!btmusic ||	!btmusic->media_opened)
		return 0;

	if (!btmusic->player)
		return 0;
	
    cfg_uint16  BM_Fadeout_Continue_Time = btmusic->user_settings.BM_Fadeout_Continue_Time;	

	if (btmusic->app_exiting || btmusic->mutex_stop || bt_manager_audio_entering_btcall()){
		BM_Fadeout_Continue_Time = 10;
	}

	media_player_fade_out(btmusic->player, BM_Fadeout_Continue_Time);

	/** reserve time to fade out*/
	os_sleep(BM_Fadeout_Continue_Time);

	bt_manager_set_stream(STREAM_TYPE_A2DP, NULL);
	btmusic->media_opened = 0;

	if (btmusic->sink_stream) {
		stream_close(btmusic->sink_stream);
	}

	media_player_stop(btmusic->player);
	media_player_close(btmusic->player);

	SYS_LOG_INF(" %p ok", btmusic->player);

	btmusic->player = NULL;

	if (btmusic->sink_stream) {
		stream_destroy(btmusic->sink_stream);
		btmusic->sink_stream = NULL;
	}
#ifndef CONFIG_BT_MUSIC_SWITCH_LANTENCY_SUPPORT
	memset(&btmusic->sink_chan, 0, sizeof(struct bt_audio_chan));
#endif
	btmusic_view_display();
	return 0;
	
}

int bt_music_playback_is_working(void)
{
	struct btmusic_app_t *btmusic = btmusic_get_app();

	if (!btmusic ||	!btmusic->media_opened)
		return 0;

	if (!btmusic->player)
		return 0;

	return 1;
}

int bt_music_pre_stop_play(void)
{
	struct btmusic_app_t *btmusic = btmusic_get_app();

	if (!btmusic ||	!btmusic->media_opened)
		return 0;

	if (!btmusic->player)
		return 0;

	SYS_LOG_INF("fadeout");

	media_player_fade_out(btmusic->player, 100);

	/** reserve time to fade out*/
	os_sleep(100);

	return 0;
}

void bt_music_check_channel_output(bool speaker_update)
{
	struct btmusic_app_t *btmusic = btmusic_get_app();

	if (!btmusic ||	!btmusic->media_opened)
		return;

	if (!btmusic->player)
		return;

	int L =0, R =0;
	
	audio_speaker_out_select(&L, &R);

	if (speaker_update)
	{
		audio_system_set_lr_channel_enable(L, R);
	}

	media_player_set_effect_output_mode(btmusic->player, audio_channel_sel_track());
}
#ifdef CONFIG_BT_MUSIC_SWITCH_LANTENCY_SUPPORT
void bt_music_handle_switch_lantency(void)
{
	struct btmusic_app_t *btmusic = btmusic_get_app();

	if (!btmusic)
        return;
    
    btmusic->lantency_mode ^= 1;
    SYS_LOG_INF("Lantency mode:%d\n", btmusic->lantency_mode); 
    if(btmusic->player)
    {
        bt_music_stop_play();
        bt_music_start_play();
    }
}
#endif
