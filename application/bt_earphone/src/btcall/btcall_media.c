/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call media
 */
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ringbuff_stream.h>
#include "ui_manager.h"
#include "media_mem.h"
#include "btcall.h"
#include "tts_manager.h"
#include "config_al.h"

uint8_t btcall_efx_stream_type = AUDIO_STREAM_VOICE;

static void _bt_call_check_speaker_update(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

	if (!btcall )
		return;

	int L =0, R =0;
	
	audio_speaker_out_select(&L, &R);

	audio_system_set_lr_channel_enable(L, R);
}


static io_stream_t _bt_call_sco_create_inputstream(void)
{
	int ret = 0;
	io_stream_t input_stream = NULL;

	input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_VOICE),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_VOICE));

	if(!input_stream) {
		goto exit; 
	}

	ret = stream_open(input_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
	if (ret) {
		stream_destroy(input_stream);
		input_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p ",input_stream);
	return 	input_stream;
}

void btcall_start_play(void)
{
	media_init_param_t init_param;
	u8_t codec_id = 0;
	u8_t sample_rate = 0;
	void *pbuf = NULL;
	int size;
	struct btcall_app_t *btcall = btcall_get_app();
	struct bt_audio_chan *bt_chan = &btcall->bt_chan;
	struct bt_audio_chan_info chan_info;
    const void *dae_param = NULL;
    const void *aec_param = NULL;

	if(!btcall)
		return;

	/* 收到TTS_EVENT_START_PLAY后，除了关闭播放器外，禁止任何事件触发重新启动播放器
	   直到收到“TTS_EVENT_STOP_PLAY”事件，才能恢复播放 */
	if(btcall->tts_start_stop)
	{
		SYS_LOG_INF("### not start play, tts_start!.\n");
		btcall->need_resume_play = 1;
		return;
	}
	
	SYS_LOG_INF("### btcall play start.\n");

	if(bt_manager_audio_stream_info(bt_chan, &chan_info))
	{
		return;
	}

	SYS_LOG_INF("Clear leaudio ref");
	media_player_set_session_ref(NULL);

	codec_id = bt_manager_audio_codec_type(chan_info.format);
	sample_rate = chan_info.sample;

    if((!btcall->enable_tts_in_calling) && 
        (bt_manager_hfp_get_status() == BT_STATUS_ONGOING) && 
        (!btcall->is_tts_locked)
      ) 
    {
		tts_manager_lock();
        btcall->is_tts_locked = true;
	}

	if (btcall->player) {
		btcall_stop_play();
		SYS_LOG_INF("already open\n");
	}

	memset(&init_param, 0, sizeof(media_init_param_t));
	init_param.type = MEDIA_SRV_TYPE_PLAYBACK_AND_CAPTURE;

	if (!btcall->upload_stream_outer) {
		btcall->upload_stream = bt_manager_audio_source_stream_create_single(bt_chan, 0);
		if (!btcall->upload_stream) {
			SYS_LOG_INF("stream create failed");
			goto err_exit;
		}

		if (stream_open(btcall->upload_stream, MODE_OUT)) {
			stream_destroy(btcall->upload_stream);
			btcall->upload_stream = NULL;
			SYS_LOG_INF("stream open failed ");
			goto err_exit;
		}
	}

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	if (codec_id == MSBC_TYPE) {
		init_param.capture_bit_rate = 26;
	}


    SYS_LOG_INF("codec_id: %d sample_rate %d, data witdh: %u, fade in: %u, fade out: %u\n",
        codec_id, sample_rate,
        btcall->user_settings.BS_DataWidth,
        btcall->user_settings.BS_Fadein_Continue_Time,
        btcall->user_settings.BS_Fadeout_Continue_Time);

	btcall->bt_stream = _bt_call_sco_create_inputstream();
	init_param.stream_type = AUDIO_STREAM_VOICE;
	init_param.efx_stream_type = btcall_efx_stream_type;
	init_param.format = codec_id;
	init_param.sample_rate = sample_rate;
	init_param.input_stream = btcall->bt_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.capture_format = codec_id;
#ifdef CONFIG_BT_CALL_CVSD_RESAMPLE_TO_16KHZ
	init_param.capture_sample_rate_input = 16;
	init_param.capture_sample_rate_output = 16;
#else
    init_param.capture_sample_rate_input = sample_rate;
	init_param.capture_sample_rate_output = sample_rate;
#endif
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = btcall->upload_stream;
    init_param.support_tws = 1;
    init_param.dsp_output = 1;
	init_param.dumpable = 1;
    init_param.sample_bits = btcall->user_settings.BS_DataWidth * 8;

	if (audio_policy_get_out_audio_mode(init_param.stream_type) == AUDIO_MODE_STEREO) {
		init_param.channels = 2;
	} else {
		init_param.channels = 1;
	}

	if (audio_policy_get_record_audio_mode(init_param.stream_type, init_param.efx_stream_type) == AUDIO_MODE_STEREO) {
		init_param.capture_channels_input = 2;
	} else {
		init_param.capture_channels_input = 1;
	}
	
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	if(le_audio_is_in_background()) {	//btcall在双模共存时使用leaudio的track
		init_param.audio_track_dis = 1;
	}
#endif

#ifdef CONFIG_SOC_SERIES_CUCKOO
	init_param.capture_sample_bits = 24;
#else
	init_param.capture_sample_bits = 16;
#endif

	btcall->player = media_player_open(&init_param);
	if (!btcall->player) {
		goto err_exit;
	}

	/* 设置通话音效参数 */
	{
		/* 音效开关 */
		CFG_Struct_BT_Call_Out_DAE btcall_dae;

		app_config_read(CFG_ID_BT_CALL_OUT_DAE, &btcall_dae, 0, sizeof(CFG_Struct_BT_Call_Out_DAE));
		media_player_set_effect_enable(btcall->player, btcall_dae.Enable_DAE);

        app_config_read(CFG_ID_BT_CALL_MIC_DAE, &btcall_dae, 0, sizeof(CFG_Struct_BT_Call_Out_DAE));
		media_player_set_upstream_dae_enable(btcall->player, btcall_dae.Enable_DAE);

        uint32_t BS_Develop_Value1;
        app_config_read(
    		CFG_ID_BTSPEECH_PLAYER_PARAM, &BS_Develop_Value1,
    		offsetof(CFG_Struct_BTSpeech_Player_Param, BS_Develop_Value1),
    		sizeof(BS_Develop_Value1));
        if((BS_Develop_Value1 & BTCALL_ALSP_POST_AGC) == 0) {
            media_player_set_downstream_agc_enable(btcall->player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_ENC) == 0) {
            media_player_set_upstream_enc_enable(btcall->player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_AI_ENC) == 0) {
            media_player_set_upstream_ai_enc_enable(btcall->player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_CNG) == 0) {
            media_player_set_upstream_cng_enable(btcall->player, false);
        }

        uint8_t BS_noise_reduce;
        app_config_read(
    		CFG_ID_BT_CALL_MORE_SETTINGS, &BS_noise_reduce,
    		offsetof(CFG_Struct_BT_Call_More_Settings, BS_noise_reduce),
    		sizeof(BS_noise_reduce));
        if((BS_noise_reduce & BTCALL_NOISE_SPEAKER_NR) == 0) {
            media_player_set_downstream_speaker_nr_enable(btcall->player, false);
        }
        if((BS_noise_reduce & BTCALL_NOISE_TAIL_NR) == 0) {
            media_player_set_downstream_tail_nr_enable(btcall->player, false);
        }

		size = (CFG_SIZE_BT_CALL_DAE_AL > CFG_SIZE_BT_CALL_QUALITY_AL ? \
				CFG_SIZE_BT_CALL_DAE_AL : CFG_SIZE_BT_CALL_QUALITY_AL);
		pbuf = mem_malloc(size);
		if(pbuf != NULL)
		{
            media_effect_get_param(AUDIO_STREAM_VOICE, AUDIO_STREAM_VOICE, &dae_param, &aec_param);

            if(!dae_param) {
    			/* 音效参数 */
    			app_config_read(CFG_ID_BT_CALL_DAE_AL, pbuf, 0, CFG_SIZE_BT_CALL_DAE_AL);
    			media_player_update_effect_param(btcall->player, pbuf, CFG_SIZE_BT_CALL_DAE_AL);
            }

            if(!aec_param) {
    			/* 通话aec参数 */
    			app_config_read(CFG_ID_BT_CALL_QUALITY_AL, pbuf, 0, CFG_SIZE_BT_CALL_QUALITY_AL);
    			media_player_update_aec_param(btcall->player, pbuf, CFG_SIZE_BT_CALL_QUALITY_AL);
            }
			
			mem_free(pbuf);
		}
		else
		{
			SYS_LOG_ERR("malloc DAE buf failed!\n");
		}
	}

	media_player_pre_enable(btcall->player);

	int threshold = audiolcy_get_latency_threshold(codec_id);
	media_player_set_audio_latency(btcall->player, threshold); 

	bt_manager_audio_sink_stream_set(bt_chan, btcall->bt_stream);
	audio_system_mute_microphone(btcall->mic_mute);
	_bt_call_check_speaker_update();

	media_player_fade_in(btcall->player, btcall->user_settings.BS_Fadein_Continue_Time);

	media_player_play(btcall->player);

	if(bt_manager_hfp_get_status() == BT_STATUS_ONGOING)
	{
		media_player_set_hfp_connected(btcall->player, true);
	}

	SYS_LOG_INF("open sucessed %p ",btcall->player);

	return;

err_exit:
	if (btcall->upload_stream && !btcall->upload_stream_outer) {
		stream_close(btcall->upload_stream);
		stream_destroy(btcall->upload_stream);
		btcall->upload_stream = NULL;
	}

    if((!btcall->enable_tts_in_calling) && (btcall->is_tts_locked)) {
		tts_manager_unlock();
        btcall->is_tts_locked = false;
	}

	SYS_LOG_ERR("open failed \n");
	return;
}

void btcall_stop_play(void)
{
	struct btcall_app_t *btcall = btcall_get_app();
	
	SYS_LOG_INF("### btcall play stop.\n");
	if (!btcall)
		return;
	
	if (!btcall->player) {
		return;
	}


	if (btcall->app_exiting){
		btcall->user_settings.BS_Fadeout_Continue_Time = 10;
	}	

	media_player_fade_out(btcall->player, btcall->user_settings.BS_Fadeout_Continue_Time);

	/** reserve time to fade out*/
	os_sleep(btcall->user_settings.BS_Fadeout_Continue_Time);

	//bt_manager_audio_sink_stream_set(&btcall->bt_chan, NULL);
	bt_manager_set_stream(STREAM_TYPE_SCO, NULL);

	if (btcall->bt_stream) {
		stream_close(btcall->bt_stream);
	}

	media_player_stop(btcall->player);

	media_player_close(btcall->player);

	if (btcall->upload_stream && !btcall->upload_stream_outer) {
		stream_close(btcall->upload_stream);
		stream_destroy(btcall->upload_stream);
		btcall->upload_stream = NULL;
	}

	if (btcall->bt_stream) {
		stream_destroy(btcall->bt_stream);
		btcall->bt_stream = NULL;
	}

	SYS_LOG_INF(" %p ok \n",btcall->player);

	btcall->player = NULL;

	if((!btcall->enable_tts_in_calling) && (btcall->is_tts_locked)) {
        tts_manager_wait_finished(true);
		tts_manager_unlock();
        btcall->is_tts_locked = false;
	}
}

int btcall_handle_enable(struct bt_audio_report *rep)
{
	struct bt_audio_chan *bt_chan = &btcall_get_app()->bt_chan;

	bt_chan->handle = rep->handle;
	bt_chan->id = rep->id;

	return 0;
}

void btcall_media_mutex_stop(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

	SYS_LOG_INF("### mutex stop.\n");
	if (btcall->player) 
	{
		if(btcall->sco_established)
		{
			btcall->need_resume_play = 1;
			btcall_stop_play();
		}
	}
}

void btcall_media_mutex_resume(void)
{
	struct btcall_app_t *btcall = btcall_get_app();

	SYS_LOG_INF("### mutex resume, need_resume_play=%d, sco_established=%d.\n", btcall->need_resume_play, btcall->sco_established);
	if (btcall->need_resume_play) 
	{
		btcall->need_resume_play = 0;
		if(btcall->sco_established)
		{
			btcall_start_play();
		}
	}
}

