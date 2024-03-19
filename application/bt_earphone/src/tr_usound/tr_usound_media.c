/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tr_usound media
 */

#include "tr_usound.h"
#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"
#include <acts_ringbuf.h>
#include "ui_manager.h"
#include "app_common.h"
#include <dvfs.h>

#include <soc.h>

#ifdef CONFIG_TR_USOUND_UPLOAD_EFX_EN
#include "tr_usound_effect.c"
#endif

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
	uint16_t dsp_freq;

	if(dvfs_level_to_freq(dvfs_level, NULL, &dsp_freq) == 0) {
		return dsp_freq;
	} else {
		return LEAUDIO_MEDIA_HIGH_FREQ_MHZ;
	}
}
#endif

void tr_usound_start_play(void)
{
	tr_usound_init_playback();
}

void tr_usound_stop_play(void)
{
	tr_usound_stop_playback();
}


int tr_usound_get_status(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	if (!tr_usound)
		return USOUND_STATUS_NULL;

	if (tr_usound->playing)
		return USOUND_STATUS_PLAYING;
	else
		return USOUND_STATUS_PAUSED;

}

static io_stream_t _tr_usound_create_uploadstream(void)//上行给PC的流
{
	int ret = 0;
	io_stream_t upload_stream = NULL;

	upload_stream = usb_audio_upload_stream_create();
	if (!upload_stream) {
		goto exit;
	}

	ret = stream_open(upload_stream, MODE_IN_OUT);
	if (ret) {
		stream_destroy(upload_stream);
		upload_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p\n", upload_stream);
	return	upload_stream;
}

extern void *ringbuff_stream_get_ringbuf(io_stream_t handle);

static io_stream_t tr_usound_create_playback_stream(struct bt_audio_chan_info *info)//dongle蓝牙收到数据流
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
	int ret = 0;
	io_stream_t input_stream = NULL;

#ifndef ENABLE_BT_DSP_XFER_BYPASS
	int buff_size = media_mem_get_cache_pool_size_ext(INPUT_PLAYBACK, AUDIO_STREAM_TR_USOUND,
		bt_manager_audio_codec_type(info->format), info->sdu, 4);
	if (buff_size <= 0) {
		goto exit;
	}
#else
    int buff_size = media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_TR_USOUND);
#endif
	input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_TR_USOUND),
			buff_size);

	if (!input_stream) {
		goto exit;
	}

	if (!tr_usound->cis_share_info) {
		tr_usound->cis_share_info = media_mem_get_cache_pool(CIS_SHARE_INFO, AUDIO_STREAM_TR_USOUND);
		SYS_LOG_INF("tr_usound_cis_share_info %p", tr_usound->cis_share_info);
		if (!tr_usound->cis_share_info) {
			goto error_exit;
		}
		memset(tr_usound->cis_share_info, 0x0, sizeof(struct bt_dsp_cis_share_info));
	}

	if (!tr_usound->recv_report_stream) {
		tr_usound->recv_report_stream = ringbuff_stream_create_ext(
				media_mem_get_cache_pool(CIS_XFER_REPORT_DATA, AUDIO_STREAM_TR_USOUND),
				media_mem_get_cache_pool_size(CIS_XFER_REPORT_DATA, AUDIO_STREAM_TR_USOUND));

		if (!tr_usound->recv_report_stream) {
			goto error_exit;
		}

		ret = stream_open(tr_usound->recv_report_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
		if (ret) {
			stream_destroy(tr_usound->recv_report_stream);
			tr_usound->recv_report_stream = NULL;
			goto error_exit;
		}

		tr_usound->cis_share_info->recv_report_buf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(tr_usound->recv_report_stream);
	}

	if (!tr_usound->send_report_stream) {
		tr_usound->send_report_stream = ringbuff_stream_create_ext(
				media_mem_get_cache_pool(CIS_SEND_REPORT_DATA, AUDIO_STREAM_TR_USOUND),
				media_mem_get_cache_pool_size(CIS_SEND_REPORT_DATA, AUDIO_STREAM_TR_USOUND));

		if (!tr_usound->send_report_stream) {
			goto error_exit;
		}

		ret = stream_open(tr_usound->send_report_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
		if (ret) {
			stream_destroy(tr_usound->send_report_stream);
			tr_usound->send_report_stream = NULL;
			goto error_exit;
		}

		tr_usound->cis_share_info->send_report_buf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(tr_usound->send_report_stream);
	}

	tr_usound->cis_share_info->recvbuf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(input_stream);
	SYS_LOG_INF("tr_usound_cis_share_info recvbuf %p, recv_report_buf %p, send_report_buf %p", 
			tr_usound->cis_share_info->recvbuf, tr_usound->cis_share_info->recv_report_buf,
            tr_usound->cis_share_info->send_report_buf);

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

int tr_usound_init_playback(void)
{
	media_init_param_t init_param;
	io_stream_t input_stream = NULL;
	io_stream_t upload_stream = NULL;
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	struct bt_audio_chan *chan = tr_usound_find_chan_by_dir(0, MIC_CHAN);
	struct bt_audio_chan_info chan_info;
	int threshold_us;
	int ret = -1;
	uint8_t LC3_Interval = LC3_INTERVAL_7500US;
	uint8_t LC3_RX_Bit_Width = LC3_BIT_WIDTH_24;
	uint8_t LC3_Samplerate_idx = 1;
	uint8_t LC3_Bitrate_idx;
	uint8_t LC3_TX_Bitrate_idx = LC3_BITRATE_CALL_1CH_64KBPS;
	uint16_t decode_run_mhz, postdae_run_mhz;
	uint16_t playback_run_mhz_total = 0;
	uint16_t dsp_freq = 0;

	if (tr_usound->playback_player) {
		SYS_LOG_INF("already\n");
		return -EALREADY;
	}

    if(chan) {
        ret = bt_manager_audio_stream_info(chan, &chan_info);
    }
    if (ret) {
		return ret;
	}
    tr_usound->dev[0].audio_handle = chan_info.audio_handle;
    SYS_LOG_INF("tr_usound->sink_audio_handle :%d", chan_info.audio_handle);

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	//tr_usound_view_show_play_paused(true);

	input_stream = tr_usound_create_playback_stream(&chan_info);
   
    if(tr_usound->usb_upload_stream)
    {
        upload_stream = tr_usound->usb_upload_stream;
    }
    else
    {
        upload_stream = _tr_usound_create_uploadstream(); 
        if(!upload_stream) {
            goto err_exit;
        }
    }
	
    tr_usound->playback_delay = chan_info.delay;
	tr_usound->cig_sync_delay = chan_info.cig_sync_delay;
	tr_usound->cis_sync_delay = chan_info.cis_sync_delay;
	tr_usound->cis_rx_delay = chan_info.rx_delay;
	tr_usound->cis_m_bn = chan_info.m_bn;
	tr_usound->cis_m_ft = chan_info.m_ft;

	SYS_LOG_INF("playback delay: %d\n", tr_usound->playback_delay);

	LC3_Samplerate_idx = _get_samplerate_index(chan_info.sample);
	if (BTMUSIC_VIEW == tr_usound->curr_view_id) {
		if (chan_info.channels == 2) {
			LC3_Bitrate_idx = LC3_BITRATE_MUSIC_2CH_158KBPS;
			decode_run_mhz = lc3_decode_run_mhz_dsp_music_2ch[LC3_Interval][LC3_RX_Bit_Width][LC3_Samplerate_idx][LC3_Bitrate_idx];
			postdae_run_mhz = postdae_run_mhz_dsp_music_2ch[LC3_Samplerate_idx];
		} else {
			LC3_Bitrate_idx = LC3_BITRATE_MUSIC_1CH_78KBPS;
			decode_run_mhz = lc3_decode_run_mhz_dsp_music_1ch[LC3_Interval][LC3_RX_Bit_Width][LC3_Samplerate_idx][LC3_Bitrate_idx];
			postdae_run_mhz = postdae_run_mhz_dsp_music_1ch[LC3_Samplerate_idx];
		}
	} else {
		LC3_Bitrate_idx = LC3_BITRATE_CALL_1CH_64KBPS;
		decode_run_mhz = lc3_decode_run_mhz_dsp_call_1ch[LC3_Interval][LC3_RX_Bit_Width][LC3_Samplerate_idx][LC3_Bitrate_idx];
		postdae_run_mhz = postdae_run_mhz_dsp_call_1ch[LC3_Samplerate_idx];
	}
    postdae_run_mhz = 600;

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
		threshold_us += 33*1000/chan_info.sample; /*dac fir delay*/
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	init_param.ext_param.nav_param.cis_sdu = chan_info.sdu;
	init_param.ext_param.nav_param.cis_bitrate_kbps = chan_info.kbps;
	init_param.ext_param.nav_param.cis_sdu_interval = chan_info.interval;

	init_param.ext_param.nav_param.cig_sync_delay = tr_usound->cig_sync_delay;
	init_param.ext_param.nav_param.cis_sync_delay_play = tr_usound->cis_sync_delay;
	init_param.ext_param.nav_param.cis_sync_delay_xfer = tr_usound->cis_rx_delay;
#ifndef CONFIG_LE_AUDIO_PHONE
	/* FIXME: need controller give me xfer_delay < rx_delay */
	init_param.ext_param.nav_param.cig_sync_delay -= s_c_framesize_up[LC3_Interval][LC3_TX_Bitrate_idx] * 4 + 44;
	init_param.ext_param.nav_param.cis_sync_delay_play -= s_c_framesize_up[LC3_Interval][LC3_TX_Bitrate_idx] * 4 + 44;
	init_param.ext_param.nav_param.cis_sync_delay_xfer -= s_c_framesize_up[LC3_Interval][LC3_TX_Bitrate_idx] * 4 + 44;
#endif
	init_param.ext_param.nav_param.cis_m_bn = tr_usound->cis_m_bn;
	init_param.ext_param.nav_param.cis_m_ft = tr_usound->cis_m_ft;
#ifdef CONFIG_LE_AUDIO_PHONE
	init_param.ext_param.nav_param.codec_stream_type = 0;
#else
	init_param.ext_param.nav_param.codec_stream_type = 1;
#endif
	init_param.ext_param.nav_param.codec_bit_width = LC3_RX_Bit_Width;
    init_param.ext_param.nav_param.working_mode = NAV_DEC_32K_1CH_ENC_48K_2CH ;
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

	init_param.type = MEDIA_SRV_TYPE_PLAYBACK;
	init_param.format = bt_manager_audio_codec_type(chan_info.format);
	init_param.stream_type = AUDIO_STREAM_TR_USOUND;
	init_param.efx_stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
	init_param.sample_rate = chan_info.sample;
	init_param.sample_bits = 32;
	init_param.input_stream = input_stream;
	init_param.output_stream = NULL;//upload_stream;
	init_param.usound_output_stream = upload_stream;
	init_param.event_notify_handle = NULL;
	//init_param.support_tws = 1;
	init_param.dumpable = 1;
	init_param.channels = chan_info.channels;
	//init_param.dsp_output = 1;
	init_param.dsp_output = 0;
	init_param.uac_upload_flag = 1;
	init_param.dsp_latency_calc = 1;
	init_param.latency_us = threshold_us;
	init_param.auto_mute = 1;
	init_param.auto_mute_threshold = 3;

	init_param.freqadj_enable = 1;
	init_param.freqadj_low_freq_run = 1;
	init_param.freqadj_high_freq_mhz = LEAUDIO_MEDIA_HIGH_FREQ_MHZ;

	init_param.ext_param.nav_param.recv_report_stream = tr_usound->recv_report_stream;
	init_param.ext_param.nav_param.send_report_stream = tr_usound->send_report_stream;

	//init_param.waitto_start = 1; //FIXME

	SYS_LOG_INF("format: %d, sample_rate: %d, channels: %d, latency: %d\n",
		chan_info.format, chan_info.sample, chan_info.channels, threshold_us);

	tr_usound->sink_stream = input_stream;

	tr_usound->playback_player = media_player_open(&init_param);
	if (!tr_usound->playback_player) {
		goto err_exit;
	}
    
    media_player_set_audio_latency(tr_usound->playback_player, threshold_us);

	media_player_set_effect_enable(tr_usound->playback_player, tr_usound->dae_enalbe);

	media_player_update_effect_param(tr_usound->playback_player, tr_usound->cfg_dae_data, CFG_SIZE_BT_MUSIC_DAE_AL);

	tr_usound->usb_upload_stream = upload_stream;
	SYS_LOG_INF(" %p\n", tr_usound->playback_player);

    if(tr_usound->upload_start)
    {
        tr_usound_start_playback();
    }
	
    return 0;

err_exit:
	if (input_stream) {
		stream_close(input_stream);
		stream_destroy(input_stream);
	}
    
    if (upload_stream) {
		stream_close(upload_stream);
		stream_destroy(upload_stream);
	}

	SYS_LOG_ERR("open failed\n");
	return -EINVAL;
}

int tr_usound_start_playback(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
	struct bt_audio_chan *upload_chan = tr_usound_find_chan_by_dir(0, MIC_CHAN);
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
	struct bt_audio_chan *download_chan = tr_usound_find_chan_by_dir(0, SPK_CHAN);
#endif

	if (!tr_usound->playback_player) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

	if (tr_usound->playback_player_run) {
		SYS_LOG_INF("already\n");
		return -EINVAL;
	}

	if (tr_usound->capture_player_run) {
		media_player_set_record_aps_monitor_enable(tr_usound->capture_player, false);
	}

	media_player_fade_in(tr_usound->playback_player, 350);

    extern int usb_reset_upload_skip_ms(void);
    usb_reset_upload_skip_ms();

	if (!tr_usound->playback_player_load) {
		media_player_pre_play(tr_usound->playback_player);
		media_player_play(tr_usound->playback_player);
		tr_usound->playback_player_load = 1;
	} else {
		media_player_resume(tr_usound->playback_player);

        acts_ringbuf_drop_all(stream_get_ringbuffer(tr_usound->sink_stream));
        acts_ringbuf_drop_all(tr_usound->cis_share_info->recv_report_buf);
	}

    if(upload_chan && upload_chan->handle)
    {
        if(!tr_usound->usb_download_started)
        {
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
            thread_timer_stop(&tr_usound->usb_cis_start_stop_work);
            tr_bt_manager_audio_stream_enable_single(download_chan, BT_AUDIO_CONTEXT_UNSPECIFIED);
#endif
        }
    }

	tr_usound->playback_player_run = 1;
	tr_usound_set_share_info(true, true);
	
    SYS_LOG_INF("%p\n", tr_usound->playback_player);

	return 0;
}

int tr_usound_stop_playback(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
	struct bt_audio_chan *upload_chan = tr_usound_find_chan_by_dir(0, MIC_CHAN);

	if (!tr_usound->playback_player || !tr_usound->playback_player_run) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

    if(upload_chan && upload_chan->handle)
    {
        if(!tr_usound->usb_download_started)
        {
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
            thread_timer_start(&tr_usound->usb_cis_start_stop_work, CONFIG_USOUND_DISCONNECT_CIS_WAIT_TIME, 0);
#endif
        }
    }
	media_player_pause(tr_usound->playback_player);

	tr_usound->playback_player_run = 0;

	tr_usound_set_share_info(true, true);

	if (tr_usound->capture_player_run) {
		media_player_set_record_aps_monitor_enable(tr_usound->capture_player, true);
	}

	SYS_LOG_INF("%p\n", tr_usound->playback_player);

	if (!tr_usound->capture_player || !tr_usound->capture_player_run) {
		tr_usound_set_share_info(false, false);
		tr_usound_exit_playback();
		tr_usound_exit_capture();
	}

	return 0;
}

int tr_usound_exit_playback(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	if (!tr_usound->playback_player) {
		SYS_LOG_INF("already\n");
		return -EALREADY;
	}

	if (tr_usound->playback_player_load) {
		media_player_stop(tr_usound->playback_player);
	}

	if (tr_usound->sink_stream) {
		stream_close(tr_usound->sink_stream);
	}
	
    if (tr_usound->usb_upload_stream) {
		stream_close(tr_usound->usb_upload_stream);
	}
	
    media_player_close(tr_usound->playback_player);

	if (tr_usound->sink_stream) {
		stream_destroy(tr_usound->sink_stream);
		tr_usound->sink_stream = NULL;
	}
	
    if (tr_usound->usb_upload_stream) {
		stream_destroy(tr_usound->usb_upload_stream);
		tr_usound->usb_upload_stream = NULL;
	}
	
    SYS_LOG_INF("%p\n", tr_usound->playback_player);

	tr_usound->playback_player = NULL;
	tr_usound->playback_player_load = 0;
	tr_usound->playback_player_run = 0;

	if (!tr_usound->capture_player && !tr_usound->playback_player) {
		if (tr_usound->recv_report_stream) {
			stream_close(tr_usound->recv_report_stream);
			stream_destroy(tr_usound->recv_report_stream);
			tr_usound->recv_report_stream = NULL;
		}
		if (tr_usound->send_report_stream) {
			stream_close(tr_usound->send_report_stream);
			stream_destroy(tr_usound->send_report_stream);
			tr_usound->send_report_stream = NULL;
		}
	}


	return 0;
}

static io_stream_t _tr_usound_create_inputstream(void)//PC下发的数据流
{
	int ret = 0;
	io_stream_t input_stream = NULL;

	input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_CAPTURE, AUDIO_STREAM_TR_USOUND),
			media_mem_get_cache_pool_size(INPUT_CAPTURE, AUDIO_STREAM_TR_USOUND));
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

static io_stream_t tr_usound_create_source_stream(struct bt_audio_chan_info *info)//dongle通过蓝牙发送的音频流
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
	io_stream_t stream = NULL;
	int ret;

#ifndef ENABLE_BT_DSP_XFER_BYPASS
	int buff_size = media_mem_get_cache_pool_size_ext(OUTPUT_CAPTURE, AUDIO_STREAM_TR_USOUND,
		bt_manager_audio_codec_type(info->format), info->sdu, 2);
	if (buff_size <= 0) {
		goto exit;
	}
#else
    int buff_size = media_mem_get_cache_pool_size(OUTPUT_CAPTURE, AUDIO_STREAM_TR_USOUND);
#endif
	stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(OUTPUT_CAPTURE, AUDIO_STREAM_TR_USOUND),
			buff_size);
	if (!stream) {
		goto exit;
	}

	if (!tr_usound->cis_share_info) {
		tr_usound->cis_share_info = media_mem_get_cache_pool(CIS_SHARE_INFO, AUDIO_STREAM_TR_USOUND);
		SYS_LOG_INF("tr_usound_cis_share_info %p", tr_usound->cis_share_info);
		if (!tr_usound->cis_share_info) {
			goto error_exit;
		}
		memset(tr_usound->cis_share_info, 0x0, sizeof(struct bt_dsp_cis_share_info));
	}

	if (!tr_usound->recv_report_stream) {
		tr_usound->recv_report_stream = ringbuff_stream_create_ext(
				media_mem_get_cache_pool(CIS_XFER_REPORT_DATA, AUDIO_STREAM_TR_USOUND),
				media_mem_get_cache_pool_size(CIS_XFER_REPORT_DATA, AUDIO_STREAM_TR_USOUND));

		if (!tr_usound->recv_report_stream) {
			goto error_exit;
		}

		ret = stream_open(tr_usound->recv_report_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
		if (ret) {
			stream_destroy(tr_usound->recv_report_stream);
			tr_usound->recv_report_stream = NULL;
			goto error_exit;
		}

		tr_usound->cis_share_info->recv_report_buf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(tr_usound->recv_report_stream);
	}

	if (!tr_usound->send_report_stream) {
		tr_usound->send_report_stream = ringbuff_stream_create_ext(
				media_mem_get_cache_pool(CIS_SEND_REPORT_DATA, AUDIO_STREAM_TR_USOUND),
				media_mem_get_cache_pool_size(CIS_SEND_REPORT_DATA, AUDIO_STREAM_TR_USOUND));

		if (!tr_usound->send_report_stream) {
			goto error_exit;
		}

		ret = stream_open(tr_usound->send_report_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
		if (ret) {
			stream_destroy(tr_usound->send_report_stream);
			tr_usound->send_report_stream = NULL;
			goto error_exit;
		}

		tr_usound->cis_share_info->send_report_buf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(tr_usound->send_report_stream);
	}

	tr_usound->cis_share_info->sendbuf = (struct acts_ringbuf *) ringbuff_stream_get_ringbuf(stream);
	SYS_LOG_INF("tr_usound_cis_share_info sendbuf %p, recv_report_buf %p, send_report_buf %p", 
			tr_usound->cis_share_info->sendbuf, tr_usound->cis_share_info->recv_report_buf,
            tr_usound->cis_share_info->send_report_buf);

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


int tr_usound_init_capture(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
	struct bt_audio_chan *chan = tr_usound_find_chan_by_dir(0, SPK_CHAN);
	struct bt_audio_chan_info chan_info;
	io_stream_t source_stream = NULL;
	io_stream_t download_stream = NULL;
	media_init_param_t init_param;
	int ret = -1;
	
	uint8_t LC3_Interval = LC3_INTERVAL_7500US;
	uint8_t LC3_TX_Bit_Width = LC3_BIT_WIDTH_24;
	uint8_t LC3_Samplerate_idx = 0;
	uint8_t LC3_Bitrate_idx = LC3_BITRATE_MUSIC_2CH_158KBPS;
	uint16_t encode_run_mhz, predae_run_mhz, preaec_run_mhz;
	uint16_t capture_run_mhz_total = 0;
	uint16_t dsp_freq = 0;
	
    if (tr_usound->capture_player) {
		SYS_LOG_INF("already\n");
		return -EALREADY;
	}

    if(chan) {
        ret = bt_manager_audio_stream_info(chan, &chan_info);
    }
    if (ret) {
		return ret;
	}

	/* wait until start_capture */
#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	source_stream = tr_usound_create_source_stream(&chan_info);
	if (!source_stream) {
		goto err_exit;
	}

    download_stream = _tr_usound_create_inputstream();
    if(!download_stream) {
        goto err_exit;
    }

    acts_ringbuf_fill(stream_get_ringbuffer(download_stream), 0, stream_get_space(download_stream));
    acts_ringbuf_drop_all(stream_get_ringbuffer(download_stream));
    //acts_ringbuf_fill_none(stream_get_ringbuffer(download_stream), stream_get_space(download_stream));
    SYS_LOG_INF("download stream:%d\n", stream_get_length(download_stream));

	memset(&init_param, 0, sizeof(media_init_param_t));

	init_param.type = MEDIA_SRV_TYPE_CAPTURE;
	init_param.stream_type = AUDIO_STREAM_TR_USOUND; 
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
	init_param.capture_input_stream = download_stream;
	init_param.capture_output_stream = source_stream;
	//init_param.waitto_start = 1;
	init_param.dsp_output = 1;
    init_param.tx_offset = TX_ANCHOR_OFFSET_US;
#ifdef CONFIG_DONGLE_SUPPORT_TWS
    init_param.is_double_encoder = 1;
#endif

	init_param.ext_param.nav_param.cis_sdu = chan_info.sdu;
	init_param.ext_param.nav_param.cis_sdu_interval = chan_info.interval;
	init_param.ext_param.nav_param.cis_sync_delay_xfer = chan_info.rx_delay;
#ifndef CONFIG_LE_AUDIO_PHONE
	/* FIXME: need controller give me xfer_delay < rx_delay */
	init_param.ext_param.nav_param.cis_sync_delay_xfer -= s_c_framesize_up[LC3_Interval][LC3_Bitrate_idx] * 4 + 44;
#endif
	init_param.ext_param.nav_param.recv_report_stream = tr_usound->recv_report_stream;
	init_param.ext_param.nav_param.send_report_stream = tr_usound->send_report_stream;
#ifdef CONFIG_LE_AUDIO_PHONE
	init_param.ext_param.nav_param.codec_stream_type = 0;
#else
	init_param.ext_param.nav_param.codec_stream_type = 1;
#endif
	init_param.ext_param.nav_param.codec_bit_width = LC3_TX_Bit_Width;
    init_param.ext_param.nav_param.working_mode = NAV_DEC_32K_1CH_ENC_48K_2CH ;
    init_param.ext_param.nav_param.sync_delay_mode = 1;
    init_param.ext_param.nav_param.real_time_mode = 1;
    init_param.ext_param.nav_param.send_pkt_size = MAX_SEND_PKT_SIZE;

	encode_run_mhz = lc3_encode_run_mhz_dsp_call_1ch[LC3_Interval][LC3_TX_Bit_Width][LC3_Samplerate_idx][LC3_Bitrate_idx];;
	predae_run_mhz = predae_run_mhz_dsp_call_1ch[LC3_Samplerate_idx];
	if (init_param.capture_channels_input == 2) {
		preaec_run_mhz = preaec_dual_mic_run_mhz_dsp_call_1ch[LC3_Samplerate_idx];
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

	tr_usound->capture_player = media_player_open(&init_param);
	if (!tr_usound->capture_player) {
		goto err_exit;
	}

	/* 设置上行音效参数 */
	{
		/* 音效开关 */
		CFG_Struct_BT_Call_Out_DAE btcall_dae;
		void *pbuf = NULL;
		int size;
		const void *dae_param = NULL;
		const void *aec_param = NULL;

        app_config_read(CFG_ID_BT_CALL_MIC_DAE, &btcall_dae, 0, sizeof(CFG_Struct_BT_Call_Out_DAE));
		media_player_set_upstream_dae_enable(tr_usound->capture_player, btcall_dae.Enable_DAE);

        uint32_t BS_Develop_Value1;
        app_config_read(
    		CFG_ID_BTSPEECH_PLAYER_PARAM, &BS_Develop_Value1,
    		offsetof(CFG_Struct_BTSpeech_Player_Param, BS_Develop_Value1),
    		sizeof(BS_Develop_Value1));
        if((BS_Develop_Value1 & BTCALL_ALSP_POST_AGC) == 0) {
            media_player_set_downstream_agc_enable(tr_usound->capture_player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_ENC) == 0) {
            media_player_set_upstream_enc_enable(tr_usound->capture_player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_AI_ENC) == 0) {
            media_player_set_upstream_ai_enc_enable(tr_usound->capture_player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_CNG) == 0) {
            media_player_set_upstream_cng_enable(tr_usound->capture_player, false);
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
    			media_player_update_effect_param(tr_usound->capture_player, pbuf, CFG_SIZE_BT_CALL_DAE_AL);
            }

            if(!aec_param) {
    			/* 通话aec参数 */
    			app_config_read(CFG_ID_BT_CALL_QUALITY_AL, pbuf, 0, CFG_SIZE_BT_CALL_QUALITY_AL);
    			media_player_update_aec_param(tr_usound->capture_player, pbuf, CFG_SIZE_BT_CALL_QUALITY_AL);
            }
			
			mem_free(pbuf);
		}
		else
		{
			SYS_LOG_ERR("malloc DAE buf failed!\n");
		}
	}

	tr_usound->source_stream = source_stream;
	
    tr_usound->usb_download_stream = download_stream;

    if(tr_usound->usb_download_stream)
        usb_audio_set_stream(tr_usound->usb_download_stream);


#ifdef CONFIG_DONGLE_SUPPORT_TWS
    uint16_t ch = chan_info.audio_handle;
    if(chan_info.locations == BT_AUDIO_LOCATIONS_FR)
        media_player_set_right_chan_handle(tr_usound->capture_player, &ch);
    else
        media_player_set_left_chan_handle(tr_usound->capture_player, &ch);
#endif

    SYS_LOG_INF("%p\n", tr_usound->capture_player);
	
    return 0;

err_exit:
	if (source_stream) {
		stream_close(source_stream);
		stream_destroy(source_stream);
	}

    if (download_stream) {
		stream_close(download_stream);
		stream_destroy(download_stream);
	}

    if(tr_usound->usb_download_stream)
    {
        usb_audio_set_stream(NULL);
        tr_usound->usb_download_stream = NULL;
    }

	SYS_LOG_ERR("open failed\n");
	return -EINVAL;
}

int tr_usound_start_capture(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	if (!tr_usound->capture_player) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

	if (tr_usound->capture_player_run) {
		SYS_LOG_INF("already\n");
		return -EINVAL;
	}

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(true);
#endif

	if (!tr_usound->capture_player_load) {
		media_player_play(tr_usound->capture_player);
		tr_usound->capture_player_load = 1;
	} else {
        acts_ringbuf_drop_all(stream_get_ringbuffer(tr_usound->usb_download_stream));
        acts_ringbuf_fill_none(stream_get_ringbuffer(tr_usound->usb_download_stream), stream_get_space(tr_usound->usb_download_stream));
        SYS_LOG_INF("download stream:%d\n", stream_get_length(tr_usound->usb_download_stream));
		media_player_resume(tr_usound->capture_player);
	}

	tr_usound->capture_player_run = 1;
    tr_usound_set_share_info(true, true); 

	SYS_LOG_INF("%p\n", tr_usound->capture_player);

	return 0;
}

int tr_usound_stop_capture(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	if (!tr_usound->capture_player || !tr_usound->capture_player_run) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

	media_player_pause(tr_usound->capture_player);

	tr_usound->capture_player_run = 0;

	tr_usound_set_share_info(true, true);
	SYS_LOG_INF("%p\n", tr_usound->capture_player);

	if (!tr_usound->playback_player || !tr_usound->playback_player_run) {
		tr_usound_set_share_info(false, false);
		tr_usound_exit_playback();
		tr_usound_exit_capture();
	}

	return 0;
}

int tr_usound_exit_capture(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	if (!tr_usound->capture_player) {
		SYS_LOG_INF("already\n");
		return -EALREADY;
	}

	if (tr_usound->capture_player_load) {
		media_player_stop(tr_usound->capture_player);
	}

	if (tr_usound->source_stream) {
		stream_close(tr_usound->source_stream);
	}

	if (tr_usound->usb_download_stream) {
		stream_close(tr_usound->usb_download_stream);
	}

	media_player_close(tr_usound->capture_player);

	if (tr_usound->source_stream) {
		stream_destroy(tr_usound->source_stream);
		tr_usound->source_stream = NULL;
	}

	if (tr_usound->usb_download_stream) {
		stream_destroy(tr_usound->usb_download_stream);
		tr_usound->usb_download_stream = NULL;
	}

	SYS_LOG_INF("%p\n", tr_usound->capture_player);

	tr_usound->capture_player = NULL;
	tr_usound->capture_player_load = 0;
	tr_usound->capture_player_run = 0;

	if (!tr_usound->capture_player && !tr_usound->playback_player) {
		if (tr_usound->recv_report_stream) {
			stream_close(tr_usound->recv_report_stream);
			stream_destroy(tr_usound->recv_report_stream);
			tr_usound->recv_report_stream = NULL;
		}
		if (tr_usound->send_report_stream) {
			stream_close(tr_usound->send_report_stream);
			stream_destroy(tr_usound->send_report_stream);
			tr_usound->send_report_stream = NULL;
		}
	}

	return 0;
}

