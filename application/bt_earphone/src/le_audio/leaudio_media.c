/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief le audio app media
 */

#include "leaudio.h"
#include "media_mem.h"
#include <ringbuff_stream.h>
#include <acts_ringbuf.h>
#include "tts_manager.h"
#include "ui_manager.h"
#include "app_common.h"
#include <dvfs.h>

#ifndef CONFIG_LE_AUDIO_PHONE
static const uint8_t s_c_framesize_up[2][LC3_BITRATE_CALL_1CH_COUNT] =
{
	/*10ms*/
	{ 100, 98, 80, 60, 40 },
	/*7.5ms*/
	{ 76, 74, 60, 46, 30 },
};
#endif

static uint8_t _get_samplerate_index(uint16_t sample_khz)
{
	if (sample_khz == 48)
		return 0;
	else if (sample_khz == 32)
		return 1;
	else if (sample_khz == 24)
		return 2;
	else if (sample_khz == 16)
		return 3;
	else
		return 0;
}

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
static uint16_t _get_dsp_freq_by_dvfs_level(uint8_t dvfs_level)
{
	return LEAUDIO_MEDIA_HIGH_FREQ_MHZ;
}
#endif

extern void *ringbuff_stream_get_ringbuf(io_stream_t handle);

static io_stream_t leaudio_create_playback_stream(struct bt_audio_chan_info *info)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	int ret = 0;
	io_stream_t input_stream = NULL;

#ifndef ENABLE_BT_DSP_XFER_BYPASS
	int buff_size = media_mem_get_cache_pool_size_ext(INPUT_PLAYBACK, AUDIO_STREAM_LE_AUDIO_MUSIC,
		bt_manager_audio_codec_type(info->format), info->sdu, 4);
	if (buff_size <= 0) {
		goto exit;
	}
#else
    int buff_size = media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_LE_AUDIO_MUSIC);
#endif
	input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_LE_AUDIO_MUSIC),
			buff_size);

	if (!input_stream) {
		goto exit;
	}

	if (!leaudio->cis_share_info) {
		leaudio->cis_share_info = media_mem_get_cache_pool(CIS_SHARE_INFO, AUDIO_STREAM_LE_AUDIO_MUSIC);
		SYS_LOG_INF("leaudio_cis_share_info %p", leaudio->cis_share_info);
		if (!leaudio->cis_share_info) {
			goto error_exit;
		}
		memset(leaudio->cis_share_info, 0x0, sizeof(struct bt_dsp_cis_share_info));
	}

	if (!leaudio->recv_report_stream) {
		leaudio->recv_report_stream = ringbuff_stream_create_ext(
				media_mem_get_cache_pool(CIS_XFER_REPORT_DATA, AUDIO_STREAM_LE_AUDIO_MUSIC),
				media_mem_get_cache_pool_size(CIS_XFER_REPORT_DATA, AUDIO_STREAM_LE_AUDIO_MUSIC));

		if (!leaudio->recv_report_stream) {
			goto error_exit;
		}

		ret = stream_open(leaudio->recv_report_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
		if (ret) {
			stream_destroy(leaudio->recv_report_stream);
			leaudio->recv_report_stream = NULL;
			goto error_exit;
		}

		leaudio->cis_share_info->recv_report_buf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(leaudio->recv_report_stream);
	}

	if (!leaudio->send_report_stream) {
		leaudio->send_report_stream = ringbuff_stream_create_ext(
				media_mem_get_cache_pool(CIS_SEND_REPORT_DATA, AUDIO_STREAM_LE_AUDIO_MUSIC),
				media_mem_get_cache_pool_size(CIS_SEND_REPORT_DATA, AUDIO_STREAM_LE_AUDIO_MUSIC));

		if (!leaudio->send_report_stream) {
			goto error_exit;
		}

		ret = stream_open(leaudio->send_report_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
		if (ret) {
			stream_destroy(leaudio->send_report_stream);
			leaudio->send_report_stream = NULL;
			goto error_exit;
		}

		leaudio->cis_share_info->send_report_buf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(leaudio->send_report_stream);
	}

	leaudio->cis_share_info->recvbuf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(input_stream);
	SYS_LOG_INF("leaudio_cis_share_info recvbuf %p, recv_report_buf %p, send_report_buf %p", 
			leaudio->cis_share_info->recvbuf, leaudio->cis_share_info->recv_report_buf, 
			leaudio->cis_share_info->send_report_buf);

	ret = stream_open(input_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
	if (ret) {
		stream_destroy(input_stream);
		input_stream = NULL;
	}

exit:
	SYS_LOG_INF("%p, sdu %d", input_stream, info->sdu);
	return input_stream;

error_exit:
	SYS_LOG_ERR("");
	if (input_stream) {
		stream_destroy(input_stream);
	}
	return NULL;
}

static io_stream_t leaudio_create_source_stream(struct bt_audio_chan_info *info)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	io_stream_t stream = NULL;
	int ret;

#ifndef ENABLE_BT_DSP_XFER_BYPASS
	int buff_size = media_mem_get_cache_pool_size_ext(OUTPUT_CAPTURE, AUDIO_STREAM_LE_AUDIO_MUSIC,
		bt_manager_audio_codec_type(info->format), info->sdu, 2);
	if (buff_size <= 0) {
		goto exit;
	}
#else
    int buff_size = media_mem_get_cache_pool_size(OUTPUT_CAPTURE, AUDIO_STREAM_LE_AUDIO_MUSIC);
#endif
	stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(OUTPUT_CAPTURE, AUDIO_STREAM_LE_AUDIO_MUSIC),
			buff_size);
	if (!stream) {
		goto exit;
	}

	if (!leaudio->cis_share_info) {
		leaudio->cis_share_info = media_mem_get_cache_pool(CIS_SHARE_INFO, AUDIO_STREAM_LE_AUDIO_MUSIC);
		SYS_LOG_INF("leaudio_cis_share_info %p", leaudio->cis_share_info);
		if (!leaudio->cis_share_info) {
			goto error_exit;
		}
		memset(leaudio->cis_share_info, 0x0, sizeof(struct bt_dsp_cis_share_info));
	}

	if (!leaudio->recv_report_stream) {
		leaudio->recv_report_stream = ringbuff_stream_create_ext(
				media_mem_get_cache_pool(CIS_XFER_REPORT_DATA, AUDIO_STREAM_LE_AUDIO_MUSIC),
				media_mem_get_cache_pool_size(CIS_XFER_REPORT_DATA, AUDIO_STREAM_LE_AUDIO_MUSIC));

		if (!leaudio->recv_report_stream) {
			goto error_exit;
		}

		ret = stream_open(leaudio->recv_report_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
		if (ret) {
			stream_destroy(leaudio->recv_report_stream);
			leaudio->recv_report_stream = NULL;
			goto error_exit;
		}

		leaudio->cis_share_info->recv_report_buf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(leaudio->recv_report_stream);
	}

	if (!leaudio->send_report_stream) {
		leaudio->send_report_stream = ringbuff_stream_create_ext(
				media_mem_get_cache_pool(CIS_SEND_REPORT_DATA, AUDIO_STREAM_LE_AUDIO_MUSIC),
				media_mem_get_cache_pool_size(CIS_SEND_REPORT_DATA, AUDIO_STREAM_LE_AUDIO_MUSIC));

		if (!leaudio->send_report_stream) {
			goto error_exit;
		}

		ret = stream_open(leaudio->send_report_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
		if (ret) {
			stream_destroy(leaudio->send_report_stream);
			leaudio->send_report_stream = NULL;
			goto error_exit;
		}

		leaudio->cis_share_info->send_report_buf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(leaudio->send_report_stream);
	}

	leaudio->cis_share_info->sendbuf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(stream);
	SYS_LOG_INF("leaudio_cis_share_info sendbuf %p, recv_report_buf %p, send_report_buf %p", 
			leaudio->cis_share_info->sendbuf, leaudio->cis_share_info->recv_report_buf, 
			leaudio->cis_share_info->send_report_buf);

	ret = stream_open(stream, MODE_OUT);
	if (ret) {
		stream_destroy(stream);
		stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF("%p, sdu %d", stream, info->sdu);
	return stream;

error_exit:
	SYS_LOG_ERR("");
	if (stream) {
		stream_destroy(stream);
	}
	return NULL;
}

int leaudio_init_capture(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	struct bt_audio_chan *chan = &leaudio->source_chan;
	struct bt_audio_chan_info chan_info;
	io_stream_t source_stream = NULL;
	media_init_param_t init_param;
	int ret;
	CFG_Struct_LE_AUDIO_Advertising_Mode cfg_le_audio_adv_mode;
	uint8_t LC3_Interval;
	uint8_t LC3_TX_Bit_Width;
	uint8_t LC3_Samplerate_idx;
	uint8_t LC3_Bitrate_idx;
	uint16_t encode_run_mhz, predae_run_mhz, preaec_run_mhz;
	uint16_t capture_run_mhz_total = 0;
	uint16_t dsp_freq = 0;
	
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	if(le_audio_is_in_background()) {
		return -ENODEV;
	}
#endif

	app_config_read(CFG_ID_LE_AUDIO_ADVERTISING_MODE, &cfg_le_audio_adv_mode, 0, sizeof(CFG_Struct_LE_AUDIO_Advertising_Mode));
	LC3_Interval = cfg_le_audio_adv_mode.LE_AUDIO_LC3_Interval;
	LC3_TX_Bit_Width = cfg_le_audio_adv_mode.LE_AUDIO_LC3_TX_Bit_Width;
	LC3_Bitrate_idx = cfg_le_audio_adv_mode.LE_AUDIO_LC3_TX_Call_1CH_Bitrate;

	if (leaudio->capture_player) {
		SYS_LOG_INF("already\n");
		return -EALREADY;
	}

	if (!leaudio->playback_player) {
		SYS_LOG_INF("Clear leaudio ref");
		media_player_set_session_ref(NULL);
	}

	ret = bt_manager_audio_stream_info(chan, &chan_info);
	if (ret) {
		return ret;
	}
	leaudio->source_audio_handle = chan_info.audio_handle;

	SYS_LOG_INF("leaudio->source_audio_handle :%d", leaudio->source_audio_handle );


	/* wait until start_capture */
#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	source_stream = leaudio_create_source_stream(&chan_info);
	if (!source_stream) {
		goto err_exit;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	init_param.type = MEDIA_SRV_TYPE_CAPTURE;
	init_param.stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
	init_param.efx_stream_type = AUDIO_STREAM_VOICE;

	LC3_Samplerate_idx = _get_samplerate_index(chan_info.sample);

	init_param.capture_format = bt_manager_audio_codec_type(chan_info.format);
	init_param.capture_sample_rate_input = chan_info.sample;
	init_param.capture_sample_rate_output = chan_info.sample;
	init_param.capture_sample_bits = (LC3_TX_Bit_Width == LC3_BIT_WIDTH_16)? (16): (24); /* 16 or 24 */

	if (audio_policy_get_record_audio_mode(init_param.stream_type, init_param.efx_stream_type) == AUDIO_MODE_STEREO) {
		init_param.capture_channels_input = 2;
	} else {
		init_param.capture_channels_input = 1;
	}
#ifdef CONFIG_AUDIO_VOICE_HARDWARE_REFERENCE
	init_param.capture_channels_input += 1;
#endif
	init_param.capture_channels_output = chan_info.channels;
	init_param.capture_bit_rate = chan_info.kbps;
	if (LC3_Interval == LC3_INTERVAL_7500US) {
		if (chan_info.sample == 16) {
			init_param.capture_frame_size = 120;
		} else if (chan_info.sample == 32) {
			init_param.capture_frame_size = 240;
		} else if (chan_info.sample == 48) {
			init_param.capture_frame_size = 360;
		}
	} else {
		if (chan_info.sample == 16) {
			init_param.capture_frame_size = 160;
		} else if (chan_info.sample == 32) {
			init_param.capture_frame_size = 320;
		} else if (chan_info.sample == 48) {
			init_param.capture_frame_size = 480;
		}
	}
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = source_stream;
	//init_param.waitto_start = 1;
	init_param.dsp_output = 1;
	init_param.dumpable = 1;
	init_param.audiopll_mode = LEAUDIO_AUDIOPLL_MODE;

	init_param.ext_param.nav_param.cis_sdu = chan_info.sdu;
	init_param.ext_param.nav_param.cis_sdu_interval = chan_info.interval;
	init_param.ext_param.nav_param.cis_sync_delay_xfer = chan_info.rx_delay;
#ifndef CONFIG_LE_AUDIO_PHONE
	/* FIXME: need controller give me xfer_delay < rx_delay */
	init_param.ext_param.nav_param.cis_sync_delay_xfer -= s_c_framesize_up[LC3_Interval][LC3_Bitrate_idx] * 4 + 44;
#endif
	init_param.ext_param.nav_param.recv_report_stream = leaudio->recv_report_stream;
	init_param.ext_param.nav_param.send_report_stream = leaudio->send_report_stream;
#ifdef CONFIG_LE_AUDIO_PHONE
	init_param.ext_param.nav_param.codec_stream_type = 0;
#else
	init_param.ext_param.nav_param.codec_stream_type = 1;
#endif
	init_param.ext_param.nav_param.codec_bit_width = LC3_TX_Bit_Width;
	init_param.ext_param.nav_param.working_mode = NAV_DEC_48K_2CH_ENC_32K_1CH;
	init_param.ext_param.nav_param.sync_delay_mode = 1;
	init_param.ext_param.nav_param.real_time_mode = 1;
	init_param.ext_param.nav_param.send_pkt_size = MAX_SEND_PKT_SIZE;

	encode_run_mhz = lc3_encode_run_mhz_dsp_call_1ch[LC3_Interval][LC3_TX_Bit_Width][LC3_Samplerate_idx][LC3_Bitrate_idx];;
	predae_run_mhz = predae_run_mhz_dsp_call_1ch[LC3_Samplerate_idx];
	if (init_param.capture_channels_input == 2) {
		preaec_run_mhz = preaec_dual_mic_run_mhz_dsp_call_1ch[LC3_Samplerate_idx];
#ifdef CONFIG_BT_TRANSCEIVER
        preaec_run_mhz += 224;//双麦降噪多预留1ms,防止影响下行解码
#endif
	} else {
		preaec_run_mhz = preaec_single_mic_run_mhz_dsp_call_1ch[LC3_Samplerate_idx];
	}

	capture_run_mhz_total = encode_run_mhz + predae_run_mhz + preaec_run_mhz + capture_system_run_mhz;
	dsp_freq = _get_dsp_freq_by_dvfs_level(DVFS_LEVEL_HIGH_PERFORMANCE_2);
	if (LC3_Interval == LC3_INTERVAL_7500US) {
		init_param.ext_param.nav_param.codec_time_us = 750 * capture_run_mhz_total / dsp_freq;
	} else {
		init_param.ext_param.nav_param.codec_time_us = 1000 * capture_run_mhz_total / dsp_freq;
	}

	SYS_LOG_INF("profile: encode %d, predae %d, preaec %d, total %d, dsp_freq %d, total comsumed %d us",
		encode_run_mhz, predae_run_mhz, preaec_run_mhz, capture_run_mhz_total, dsp_freq,
		init_param.ext_param.nav_param.codec_time_us);

	SYS_LOG_INF("format: %d, sample_rate: %d, channels: %d, kbps: %d, interval: %d, cis_sync_delay_xfer %d",
		chan_info.format, chan_info.sample, chan_info.channels,
		chan_info.kbps, chan_info.interval,
		init_param.ext_param.nav_param.cis_sync_delay_xfer);

	leaudio->capture_player = media_player_open(&init_param);
	if (!leaudio->capture_player) {
		goto err_exit;
	}

	/* 设置上行音效参数 */
	{
		/* 音效开关 */
		CFG_Struct_LEAUDIO_Call_MIC_DAE leaudio_call_dae;
		void *pbuf = NULL;
		int size;
		const void *dae_param = NULL;
		const void *aec_param = NULL;

        app_config_read(CFG_ID_LEAUDIO_CALL_MIC_DAE, &leaudio_call_dae, 0, sizeof(CFG_Struct_LEAUDIO_Call_MIC_DAE));
		media_player_set_upstream_dae_enable(leaudio->capture_player, leaudio_call_dae.Enable_DAE);

        uint32_t BS_Develop_Value1;
        app_config_read(
    		CFG_ID_BTSPEECH_PLAYER_PARAM, &BS_Develop_Value1,
    		offsetof(CFG_Struct_BTSpeech_Player_Param, BS_Develop_Value1),
    		sizeof(BS_Develop_Value1));
        if((BS_Develop_Value1 & BTCALL_ALSP_POST_AGC) == 0) {
            media_player_set_downstream_agc_enable(leaudio->capture_player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_ENC) == 0) {
            media_player_set_upstream_enc_enable(leaudio->capture_player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_AI_ENC) == 0) {
            media_player_set_upstream_ai_enc_enable(leaudio->capture_player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_CNG) == 0) {
            media_player_set_upstream_cng_enable(leaudio->capture_player, false);
        }

		size = (CFG_SIZE_LEAUDIO_CALL_DAE_AL > CFG_SIZE_BT_CALL_QUALITY_AL ? \
				CFG_SIZE_LEAUDIO_CALL_DAE_AL : CFG_SIZE_BT_CALL_QUALITY_AL);
		pbuf = mem_malloc(size);
		if(pbuf != NULL)
		{
            media_effect_get_param(AUDIO_STREAM_LE_AUDIO_MUSIC, AUDIO_STREAM_VOICE, &dae_param, &aec_param);

            if(!dae_param) {
    			/* 音效参数 */
    			app_config_read(CFG_ID_LEAUDIO_CALL_DAE_AL, pbuf, 0, CFG_SIZE_LEAUDIO_CALL_DAE_AL);
    			media_player_update_effect_param(leaudio->capture_player, pbuf, CFG_SIZE_LEAUDIO_CALL_DAE_AL);
            }

            if(!aec_param) {
    			/* 通话aec参数 */
    			app_config_read(CFG_ID_BT_CALL_QUALITY_AL, pbuf, 0, CFG_SIZE_BT_CALL_QUALITY_AL);
    			media_player_update_aec_param(leaudio->capture_player, pbuf, CFG_SIZE_BT_CALL_QUALITY_AL);
            }
			
			mem_free(pbuf);
		}
		else
		{
			SYS_LOG_ERR("malloc DAE buf failed!\n");
		}
	}

	leaudio->source_stream = source_stream;

	media_player_pre_enable(leaudio->capture_player);

	SYS_LOG_INF("%p\n", leaudio->capture_player);

	return 0;

err_exit:
	if (source_stream) {
		stream_close(source_stream);
		stream_destroy(source_stream);
	}

	SYS_LOG_ERR("open failed\n");
	return -EINVAL;
}

int leaudio_start_capture(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	if(le_audio_is_in_background()) {
		return -ENODEV;
	}
#endif

	if (!leaudio->capture_player) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

	if (leaudio->capture_player_run) {
		SYS_LOG_INF("already\n");
		return -EINVAL;
	}

#ifdef CONFIG_PLAYTTS
	if (leaudio->curr_view_id == BTCALL_VIEW) {
		tts_manager_lock();
	}
	tts_manager_wait_finished(true);
#endif

	if (!leaudio->capture_player_load) {
		media_player_play(leaudio->capture_player);
		leaudio->capture_player_load = 1;
	} else {
		media_player_resume(leaudio->capture_player);
	}

	bt_manager_audio_source_stream_set_single(&leaudio->source_chan,
					leaudio->source_stream, 0);

	leaudio->capture_player_run = 1;

	leaudio_set_share_info(true, true);

	if (!leaudio->playback_player_run) {
		media_player_set_record_aps_monitor_enable(leaudio->capture_player, true);
	}

	SYS_LOG_INF("%p\n", leaudio->capture_player);

	return 0;
}

int leaudio_stop_capture(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	if(le_audio_is_in_background()) {
		return -ENODEV;
	}
#endif

	if (!leaudio->capture_player || !leaudio->capture_player_run) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

	bt_manager_audio_source_stream_set_single(&leaudio->source_chan,
					NULL, 0);

	media_player_pause(leaudio->capture_player);

	media_player_set_record_aps_monitor_enable(leaudio->capture_player, false);

#ifdef CONFIG_PLAYTTS
	if (leaudio->curr_view_id == BTCALL_VIEW) {
		tts_manager_unlock();
	}
#endif

	leaudio->adc_to_bt_started = 0;
	leaudio->capture_player_run = 0;

	leaudio_set_share_info(true, true);

	SYS_LOG_INF("%p\n", leaudio->capture_player);

	if (!leaudio->playback_player || !leaudio->playback_player_run) {
		leaudio_set_share_info(false, false);
		leaudio_exit_playback();
		leaudio_exit_capture();
	}

	return 0;
}

int leaudio_exit_capture(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	if(le_audio_is_in_background()) {
		return -ENODEV;
	}
#endif

	if (!leaudio->capture_player) {
		SYS_LOG_INF("already\n");
		return -EALREADY;
	}

	if (leaudio->capture_player_load) {
		media_player_stop(leaudio->capture_player);
	}

	if (leaudio->source_stream) {
		leaudio->cis_share_info->sendbuf = NULL;
		stream_close(leaudio->source_stream);
	}

	media_player_close(leaudio->capture_player);

	if (leaudio->source_stream) {
		stream_destroy(leaudio->source_stream);
		leaudio->source_stream = NULL;
	}

	SYS_LOG_INF("%p\n", leaudio->capture_player);

	leaudio->capture_player = NULL;
	leaudio->capture_player_load = 0;
	leaudio->capture_player_run = 0;

	if (!leaudio->capture_player && !leaudio->playback_player) {
		if (leaudio->recv_report_stream) {
			leaudio->cis_share_info->recvbuf = NULL;
			leaudio->cis_share_info->recv_report_buf = NULL;
			stream_close(leaudio->recv_report_stream);
			stream_destroy(leaudio->recv_report_stream);
			leaudio->recv_report_stream = NULL;
		}
		if (leaudio->send_report_stream) {
			leaudio->cis_share_info->sendbuf = NULL;
			leaudio->cis_share_info->send_report_buf = NULL;
			stream_close(leaudio->send_report_stream);
			stream_destroy(leaudio->send_report_stream);
			leaudio->send_report_stream = NULL;
		}
	}

	return 0;
}

#if defined(CONFIG_LE_AUDIO_DISPLAY_MULTI_FREQ_BAND_DETECT) || defined (CONFIG_LE_AUDIO_DISPLAY_TIME_ENERGE_DETECT)
static void leaudio_mfbd_tde_display_proc(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	static int count = 0;
	if(count++ % 20)	//避免打印太多，每隔1秒打印一次
		return;

#ifdef CONFIG_LE_AUDIO_DISPLAY_MULTI_FREQ_BAND_DETECT
	if(leaudio->mfbd_energys) {
		printk("mfbd{%d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d}\n"
			, leaudio->mfbd_energys[0], leaudio->mfbd_energys[1], leaudio->mfbd_energys[2], leaudio->mfbd_energys[3]
			, leaudio->mfbd_energys[4], leaudio->mfbd_energys[5], leaudio->mfbd_energys[6], leaudio->mfbd_energys[7]
			, leaudio->mfbd_energys[8], leaudio->mfbd_energys[9], leaudio->mfbd_energys[10], leaudio->mfbd_energys[11]);
	}
#endif
#ifdef CONFIG_LE_AUDIO_DISPLAY_TIME_ENERGE_DETECT
	if(leaudio->tde_energys) {
		printk("tde{%d,%d}\n", leaudio->tde_energys[0], leaudio->tde_energys[1]);
	}
#endif
}
#endif

int leaudio_init_playback(void)
{
	media_init_param_t init_param;
	io_stream_t input_stream = NULL;
	struct leaudio_app_t *leaudio = leaudio_get_app();
	struct bt_audio_chan *chan = &leaudio->sink_chan;
	struct bt_audio_chan_info chan_info;
	int threshold_us;
	int ret;
	CFG_Struct_LE_AUDIO_Advertising_Mode cfg_le_audio_adv_mode;
	uint8_t LC3_Interval;
	uint8_t LC3_RX_Bit_Width;
	uint8_t LC3_Samplerate_idx;
	uint8_t LC3_Bitrate_idx;
	uint8_t LC3_TX_Bitrate_idx;
	uint16_t decode_run_mhz, postdae_run_mhz;
	uint16_t playback_run_mhz_total = 0;
	uint16_t dsp_freq = 0;

	app_config_read(CFG_ID_LE_AUDIO_ADVERTISING_MODE, &cfg_le_audio_adv_mode, 0, sizeof(CFG_Struct_LE_AUDIO_Advertising_Mode));
	LC3_Interval = cfg_le_audio_adv_mode.LE_AUDIO_LC3_Interval;
	LC3_RX_Bit_Width = cfg_le_audio_adv_mode.LE_AUDIO_LC3_RX_Bit_Width;
	LC3_TX_Bitrate_idx = cfg_le_audio_adv_mode.LE_AUDIO_LC3_TX_Call_1CH_Bitrate;

	if (leaudio->playback_player) {
		SYS_LOG_INF("already\n");
		return -EALREADY;
	}

	if (!leaudio->capture_player) {
		SYS_LOG_INF("Clear leaudio ref");
		media_player_set_session_ref(NULL);
	}

	ret = bt_manager_audio_stream_info(chan, &chan_info);
	if (ret) {
		return ret;
	}
	leaudio->sink_audio_handle = chan_info.audio_handle;
	SYS_LOG_INF("leaudio->sink_audio_handle :%d", leaudio->sink_audio_handle );

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	//leaudio_view_show_play_paused(true);

	input_stream = leaudio_create_playback_stream(&chan_info);

	leaudio->playback_delay = chan_info.delay;
	leaudio->cig_sync_delay = chan_info.cig_sync_delay;
	leaudio->cis_sync_delay = chan_info.cis_sync_delay;
	leaudio->cis_rx_delay = chan_info.rx_delay;
	leaudio->cis_m_bn = chan_info.m_bn;
	leaudio->cis_m_ft = chan_info.m_ft;

	SYS_LOG_INF("playback delay: %d\n", leaudio->playback_delay);

	LC3_Samplerate_idx = _get_samplerate_index(chan_info.sample);
	if (BTMUSIC_VIEW == leaudio->curr_view_id) {
		if (chan_info.channels == 2) {
			LC3_Bitrate_idx = cfg_le_audio_adv_mode.LE_AUDIO_LC3_RX_Music_2CH_Bitrate;
			decode_run_mhz = lc3_decode_run_mhz_dsp_music_2ch[LC3_Interval][LC3_RX_Bit_Width][LC3_Samplerate_idx][LC3_Bitrate_idx];
			postdae_run_mhz = postdae_run_mhz_dsp_music_2ch[LC3_Samplerate_idx];
		} else {
			LC3_Bitrate_idx = cfg_le_audio_adv_mode.LE_AUDIO_LC3_RX_Music_1CH_Bitrate;
			decode_run_mhz = lc3_decode_run_mhz_dsp_music_1ch[LC3_Interval][LC3_RX_Bit_Width][LC3_Samplerate_idx][LC3_Bitrate_idx];
			postdae_run_mhz = postdae_run_mhz_dsp_music_1ch[LC3_Samplerate_idx];
		}
	} else {
		LC3_Bitrate_idx = cfg_le_audio_adv_mode.LE_AUDIO_LC3_TX_Call_1CH_Bitrate;
		decode_run_mhz = lc3_decode_run_mhz_dsp_call_1ch[LC3_Interval][LC3_RX_Bit_Width][LC3_Samplerate_idx][LC3_Bitrate_idx];
		postdae_run_mhz = postdae_run_mhz_dsp_call_1ch[LC3_Samplerate_idx];
	}

	playback_run_mhz_total = decode_run_mhz + postdae_run_mhz/10 + playback_system_run_mhz;
	dsp_freq = _get_dsp_freq_by_dvfs_level(DVFS_LEVEL_HIGH_PERFORMANCE_2);
	if (LC3_Interval == LC3_INTERVAL_7500US) {
		threshold_us = 750 * playback_run_mhz_total / dsp_freq;
	} else {
		threshold_us = 1000 * playback_run_mhz_total / dsp_freq;
	}

	SYS_LOG_INF("profile: decode %d, postdae %d, total %d, dsp_freq %d, total comsumed %d us",
		decode_run_mhz, postdae_run_mhz, playback_run_mhz_total, dsp_freq, threshold_us);

#if 0
	int threshold_us = audiolcy_get_latency_threshold(init_param.format);
#endif

	if (chan_info.sample) {
		/* FIR ModeA for 48K, FIR ModeB for 44.1K, Group Delay 35 samples + 2 samples digital delay */
		threshold_us += 37*1000/chan_info.sample;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	init_param.ext_param.nav_param.cis_sdu = chan_info.sdu;
	init_param.ext_param.nav_param.cis_bitrate_kbps = chan_info.kbps;
	init_param.ext_param.nav_param.cis_sdu_interval = chan_info.interval;

	init_param.ext_param.nav_param.cig_sync_delay = leaudio->cig_sync_delay;
	init_param.ext_param.nav_param.cis_sync_delay_play = leaudio->cis_sync_delay;
	init_param.ext_param.nav_param.cis_sync_delay_xfer = leaudio->cis_rx_delay;
#ifndef CONFIG_LE_AUDIO_PHONE
	/* FIXME: need controller give me xfer_delay < rx_delay */
	init_param.ext_param.nav_param.cig_sync_delay -= s_c_framesize_up[LC3_Interval][LC3_TX_Bitrate_idx] * 4 + 44;
	init_param.ext_param.nav_param.cis_sync_delay_play -= s_c_framesize_up[LC3_Interval][LC3_TX_Bitrate_idx] * 4 + 44;
	init_param.ext_param.nav_param.cis_sync_delay_xfer -= s_c_framesize_up[LC3_Interval][LC3_TX_Bitrate_idx] * 4 + 44;
#endif
	init_param.ext_param.nav_param.cis_m_bn = leaudio->cis_m_bn;
	init_param.ext_param.nav_param.cis_m_ft = leaudio->cis_m_ft;
#ifdef CONFIG_LE_AUDIO_PHONE
	init_param.ext_param.nav_param.codec_stream_type = 0;
#else
	init_param.ext_param.nav_param.codec_stream_type = 1;
#endif
	init_param.ext_param.nav_param.codec_bit_width = LC3_RX_Bit_Width;
	init_param.ext_param.nav_param.working_mode = NAV_DEC_48K_2CH_ENC_32K_1CH;
	init_param.ext_param.nav_param.sync_delay_mode = 1;
	init_param.ext_param.nav_param.dump_crc_error_pkt = CONFIG_DUMP_CRC_ERROR_PKT;
	init_param.ext_param.nav_param.recv_pkt_size = MAX_RECV_PKT_SIZE;

	SYS_LOG_INF("cig_sync_delay %d, cis_sync_delay_play %d, cis_sync_delay_xfer %d, cis_m_bn %d, cis_m_ft %d, kbps %d, interval %d", 
		init_param.ext_param.nav_param.cig_sync_delay, 
		init_param.ext_param.nav_param.cis_sync_delay_play, 
		init_param.ext_param.nav_param.cis_sync_delay_xfer, 
		init_param.ext_param.nav_param.cis_m_bn, 
		init_param.ext_param.nav_param.cis_m_ft,
		init_param.ext_param.nav_param.cis_bitrate_kbps,
		chan_info.interval);

	threshold_us += chan_info.interval * chan_info.m_bn;
	if (chan_info.m_ft > 1) {
		threshold_us += chan_info.interval * chan_info.m_bn * (chan_info.m_ft-1);
	}
	threshold_us += init_param.ext_param.nav_param.cig_sync_delay;
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
    if(le_audio_is_in_background()) {
        threshold_us += 1000;
    }
#endif      

	init_param.type = MEDIA_SRV_TYPE_PLAYBACK;
	init_param.format = bt_manager_audio_codec_type(chan_info.format);
	init_param.stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
	init_param.efx_stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
	init_param.sample_rate = chan_info.sample;
	init_param.sample_bits = 32;
	init_param.input_stream = input_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.support_tws = 1;
	init_param.dumpable = 1;
	init_param.channels = chan_info.channels;
	init_param.dsp_output = 1;
	init_param.dsp_latency_calc = 1;
	init_param.latency_us = threshold_us;
	init_param.auto_mute = 1;
	init_param.auto_mute_threshold = 3;
	init_param.audiopll_mode = LEAUDIO_AUDIOPLL_MODE;

	init_param.freqadj_enable = 1;
	init_param.freqadj_low_freq_run = 1;
	init_param.freqadj_high_freq_mhz = LEAUDIO_MEDIA_HIGH_FREQ_MHZ;

	init_param.ext_param.nav_param.recv_report_stream = leaudio->recv_report_stream;
	init_param.ext_param.nav_param.send_report_stream = leaudio->send_report_stream;

	//init_param.waitto_start = 1; //FIXME

	SYS_LOG_INF("format: %d, sample_rate: %d, channels: %d, latency: %d\n",
		chan_info.format, chan_info.sample, chan_info.channels, threshold_us);

	leaudio->sink_stream = input_stream;
	
#ifdef CONFIG_LE_AUDIO_DISPLAY_MULTI_FREQ_BAND_DETECT
	//设置检测频点数量和频率
	init_param.mfbd_bands = 12;
	init_param.mfbd_ms = (chan_info.interval * 5) / 1000;	//自定义的检测周期
	init_param.mfbd_freqs[0] = 40;
	init_param.mfbd_freqs[1] = 80;
	init_param.mfbd_freqs[2] = 150;
	init_param.mfbd_freqs[3] = 280;
	init_param.mfbd_freqs[4] = 520;
	init_param.mfbd_freqs[5] = 1040;
	init_param.mfbd_freqs[6] = 2000;
	init_param.mfbd_freqs[7] = 3800;
	init_param.mfbd_freqs[8] = 7000;
	init_param.mfbd_freqs[9] = 13500;
	init_param.mfbd_freqs[10] = 100;
	init_param.mfbd_freqs[11] = 1000;
#endif
#ifdef CONFIG_LE_AUDIO_DISPLAY_TIME_ENERGE_DETECT
	init_param.tde_ms = (chan_info.interval * 5) / 1000;	//自定义的检测周期
#endif
	SYS_LOG_INF("tde_ms: %d\n", init_param.tde_ms);

	leaudio->playback_player = media_player_open(&init_param);
	if (!leaudio->playback_player) {
		goto err_exit;
	}

#ifdef CONFIG_LE_AUDIO_DISPLAY_MULTI_FREQ_BAND_DETECT
	leaudio->mfbd_energys = init_param.mfbd_energys;	//保存返回的mfbd_energys地址
#endif
#ifdef CONFIG_LE_AUDIO_DISPLAY_TIME_ENERGE_DETECT
	leaudio->tde_energys = init_param.tde_energys;		//保存返回的tde_energy地址
#endif
#if defined(CONFIG_LE_AUDIO_DISPLAY_MULTI_FREQ_BAND_DETECT) || defined (CONFIG_LE_AUDIO_DISPLAY_TIME_ENERGE_DETECT)
	thread_timer_init(&leaudio->mfbd_display_timer, leaudio_mfbd_tde_display_proc, NULL);
	//thread_timer_start(&leaudio->mfbd_display_timer, init_param.mfbd_ms, init_param.mfbd_ms);
	thread_timer_start(&leaudio->mfbd_display_timer, init_param.tde_ms, init_param.tde_ms);
#endif

	media_player_set_audio_latency(leaudio->playback_player, threshold_us);

	media_player_set_effect_enable(leaudio->playback_player, leaudio->dae_enalbe);

	media_player_update_effect_param(leaudio->playback_player, leaudio->cfg_dae_data, CFG_SIZE_BT_MUSIC_DAE_AL);

	media_player_pre_enable(leaudio->playback_player);

	SYS_LOG_INF(" %p\n", leaudio->playback_player);

	return 0;

err_exit:
	if (input_stream) {
		stream_close(input_stream);
		stream_destroy(input_stream);
	}

	SYS_LOG_ERR("open failed\n");
	return -EINVAL;
}

int leaudio_start_playback(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	if (!leaudio->playback_player) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

	if (leaudio->playback_player_run) {
		SYS_LOG_INF("already\n");
		return -EINVAL;
	}

	if (leaudio->capture_player_run) {
		media_player_set_record_aps_monitor_enable(leaudio->capture_player, false);
	}

	media_player_fade_in(leaudio->playback_player, 350);

	if (!leaudio->playback_player_load) {
		media_player_pre_play(leaudio->playback_player);
		media_player_play(leaudio->playback_player);
		leaudio->playback_player_load = 1;
	} else {
		media_player_resume(leaudio->playback_player);
	}

	bt_manager_audio_sink_stream_set(&leaudio->sink_chan, leaudio->sink_stream);

	leaudio->playback_player_run = 1;

	leaudio_set_share_info(true, true);

	SYS_LOG_INF("%p\n", leaudio->playback_player);

	return 0;
}

int leaudio_stop_playback(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	if (!leaudio->playback_player || !leaudio->playback_player_run) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	if(le_audio_is_in_background()) {
		tr_bt_manager_audio_auto_stop();	//断开后不允许回连
	}
#endif
	{
		media_player_fade_out(leaudio->playback_player, 25);

		/* reserve time to fade out */
		os_sleep(30);
	}

	//leaudio_view_show_play_paused(false);

	bt_manager_audio_sink_stream_set(&leaudio->sink_chan, NULL);
	media_player_pause(leaudio->playback_player);

	leaudio->bt_to_dac_started = 0;
	leaudio->playback_player_run = 0;

	leaudio_set_share_info(true, true);

	if (leaudio->capture_player_run) {
		media_player_set_record_aps_monitor_enable(leaudio->capture_player, true);
	}

	SYS_LOG_INF("%p\n", leaudio->playback_player);

	if (!leaudio->capture_player || !leaudio->capture_player_run) {
		leaudio_set_share_info(false, false);
		leaudio_exit_playback();
		leaudio_exit_capture();
	}

	return 0;
}

int leaudio_playback_is_working(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	if (!leaudio->playback_player || !leaudio->playback_player_run) {
		return 0;
	}

	return 1;
}

int leaudio_pre_stop_playback(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	if (!leaudio->playback_player || !leaudio->playback_player_run) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

	SYS_LOG_INF("fadeout");

	media_player_fade_out(leaudio->playback_player, 100);

	/* reserve time to fade out */
	os_sleep(100);

	return 0;
}

int leaudio_exit_playback(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	if (!leaudio->playback_player) {
		SYS_LOG_INF("already\n");
		return -EALREADY;
	}
	
#if defined(CONFIG_LE_AUDIO_DISPLAY_MULTI_FREQ_BAND_DETECT) || defined (CONFIG_LE_AUDIO_DISPLAY_TIME_ENERGE_DETECT)
	thread_timer_stop(&leaudio->mfbd_display_timer);
#endif
#ifdef CONFIG_LE_AUDIO_DISPLAY_MULTI_FREQ_BAND_DETECT
	leaudio->mfbd_energys = NULL;
#endif
#ifdef CONFIG_LE_AUDIO_DISPLAY_TIME_ENERGE_DETECT
	leaudio->tde_energys = NULL;
#endif

	if (leaudio->playback_player_load) {
		media_player_stop(leaudio->playback_player);
	}

	if (leaudio->sink_stream) {
		stream_close(leaudio->sink_stream);
	}

	media_player_close(leaudio->playback_player);

	if (leaudio->sink_stream) {
		stream_destroy(leaudio->sink_stream);
		leaudio->sink_stream = NULL;
	}

	SYS_LOG_INF("%p\n", leaudio->playback_player);

	leaudio->playback_player = NULL;
	leaudio->playback_player_load = 0;
	leaudio->playback_player_run = 0;

	if (!leaudio->capture_player && !leaudio->playback_player) {
		if (leaudio->recv_report_stream) {
			leaudio->cis_share_info->recvbuf = NULL;
			leaudio->cis_share_info->recv_report_buf = NULL;
			stream_close(leaudio->recv_report_stream);
			stream_destroy(leaudio->recv_report_stream);
			leaudio->recv_report_stream = NULL;
		}
		if (leaudio->send_report_stream) {
			leaudio->cis_share_info->sendbuf = NULL;
			leaudio->cis_share_info->send_report_buf = NULL;
			stream_close(leaudio->send_report_stream);
			stream_destroy(leaudio->send_report_stream);
			leaudio->send_report_stream = NULL;
		}
	}

	return 0;
}

#ifdef CONFIG_LE_AUDIO_DISPLAY_TIME_ENERGE_DETECT
uint16_t leaudio_playback_energy_level(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	if(leaudio && leaudio->tde_energys) 
	{
		// tde_energys[0]: peak value
		// tde_energys[1]: average value
		return leaudio->tde_energys[1];
	}

	return 0;
}
#endif
