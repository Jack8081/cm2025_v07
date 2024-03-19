/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief le audio
 */

#ifndef _LE_AUDIO_APP_H
#define _LE_AUDIO_APP_H

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "leaudio"
#include <logging/sys_log.h>
#endif

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <volume_manager.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <audiolcy_common.h>
#include <thread_timer.h>
#include <property_manager.h>
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"
#include "config_al.h"
#include "btservice_api.h"
#include "bt_manager.h"
#include "ui_manager.h"
#include "tts_manager.h"
#include "system_bt.h"
#include <drivers/gpio.h>

#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
#include <system_app.h>
#endif
#endif

#define ENABLE_BT_DSP_XFER_BYPASS

#define LEAUDIO_MEDIA_HIGH_FREQ_MHZ  168

#define MAX_RECV_PKT_SIZE  256
#define MAX_SEND_PKT_SIZE  128

#ifdef CONFIG_SOC_SERIES_CUCKOO
#define CONFIG_DUMP_CRC_ERROR_PKT  0 //FIXME
#else
#define CONFIG_DUMP_CRC_ERROR_PKT  0
#endif

#ifdef CONFIG_SOC_SERIES_CUCKOO
#define LEAUDIO_AUDIOPLL_MODE      2 /*0-default,1-integer-N,2-fractional-N*/
#else
#define LEAUDIO_AUDIOPLL_MODE      0 /*0-default,1-integer-N,2-fractional-N*/
#endif

struct leaudio_app_t
{
	struct thread_timer monitor_timer;
	struct thread_timer key_timer;
	struct thread_timer delay_play_timer;
#ifdef CONFIG_LE_AUDIO_SR_BQB
	uint16_t handle;
#endif
	uint32_t tts_playing : 1;
	uint32_t mic_mute : 1;
	uint32_t need_resume_key : 1;
	uint32_t key_handled : 1;

	uint32_t bt_source_started : 1; /* bt audio source started */
	uint32_t adc_to_bt_started : 1;

	uint32_t bt_sink_enabled : 1; /* bt audio sink enabled */
	uint32_t bt_to_dac_started : 1;

	uint32_t capture_player_load : 1;
	uint32_t capture_player_run : 1;

	uint32_t playback_player_load : 1;
	uint32_t playback_player_run : 1;

	/* Audio Sink: BT -> sink_stream -> playback_stream */
	media_player_t *playback_player;
	io_stream_t sink_stream;
	struct bt_audio_chan sink_chan;
	uint16_t sink_audio_handle;

	/* Audio Source: capture_stream -> source_stream -> BT */
	media_player_t *capture_player;
	io_stream_t source_stream;
	struct bt_audio_chan source_chan;
	uint16_t source_audio_handle;

	uint8_t dae_enalbe;
	uint8_t cfg_dae_data[CFG_SIZE_BT_MUSIC_DAE_AL];
	uint8_t multidae_enable;
	uint8_t current_dae;
	uint8_t dae_cfg_nums;
	uint8_t set_share_info_flag;
	struct bt_dsp_cis_share_info *cis_share_info;
	io_stream_t recv_report_stream;
	io_stream_t send_report_stream;
	uint32_t playback_delay;
	uint32_t cig_sync_delay;
	uint32_t cis_sync_delay;
	uint32_t cis_rx_delay;
	uint8_t cis_m_bn;
	uint8_t cis_m_ft;
	uint8_t curr_view_id;
	uint8_t last_disp_status;

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	uint32_t leaudio_backgroud:1;
	uint32_t leaudio_need_backgroud:1;
#endif

#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    const struct device *gpio_dev;
    os_delayed_work aux_plug_in_delay_work;
#endif
#endif
//#ifdef CONFIG_LE_AUDIO_DISPLAY_MULTI_FREQ_BAND_DETECT
	struct thread_timer mfbd_display_timer;
	uint16_t *mfbd_energys;
//#endif
#ifdef CONFIG_LE_AUDIO_DISPLAY_TIME_ENERGE_DETECT
	uint16_t *tde_energys;
#endif
};

struct leaudio_app_t *leaudio_get_app(void);

void leaudio_bt_event_proc(struct app_msg *msg);
void leaudio_tts_event_proc(struct app_msg *msg);
void leaudio_input_event_proc(struct app_msg *msg);
void leaudio_tws_event_proc(struct app_msg *msg);
void leaudio_view_init(uint8_t view_id, bool key_remap);
void leaudio_view_deinit(void);
void leaudio_view_switch(uint8_t view_id);
void leaudio_view_display(void);
void leaudio_call_view_display(void);
void leaudio_event_notify(int event);
void leaudio_multi_dae_adjust(uint8_t dae_index);

int leaudio_init_capture(void);
int leaudio_start_capture(void);
int leaudio_stop_capture(void);
int leaudio_exit_capture(void);
int leaudio_init_playback(void);
int leaudio_start_playback(void);
int leaudio_stop_playback(void);
int leaudio_pre_stop_playback(void);
int leaudio_exit_playback(void);

void leaudio_set_share_info(bool enable, bool update);

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
void le_audio_enter_background(void);
void le_audio_enter_foreground(void);
void le_audio_check_set_background(void);
bool le_audio_is_in_background(void);
void le_audio_background_stop(void);
void le_audio_background_prepare(void);
void le_audio_background_start(void);
void le_audio_background_bt_event_proc(struct app_msg *msg);
void le_audio_background_exit(void);
#endif

/* profile datas */

/*unit is 0.1MHz*/
/*dimensional-1: lc3 interval, [0] 10ms, [1] 7.5ms*/
/*dimensional-2: lc3 bit width, [0] 16bits, [1] 24bits*/
/*dimensional-3: lc3 samplerate, [0] 48khz, [1] 32khz, [2] 24khz, [3] 16khz*/

/*dimensional-4: lc3 bitrate, [0] 192kbps, [1] 184kbps, [2] 176kbps, [3] 160kbps, [4] 158kbps*/
extern const uint16_t lc3_decode_run_mhz_dsp_music_2ch[2][2][1][LC3_BITRATE_MUSIC_2CH_COUNT];
/*dimensional-4: lc3 bitrate, [0] 80kbps, [1] 78kbps*/
extern const uint16_t lc3_decode_run_mhz_dsp_music_1ch[2][2][1][LC3_BITRATE_MUSIC_1CH_COUNT];
/*dimensional-4: lc3 bitrate, [0] 80kbps, [1] 78kbps, [2] 64kbps, [3] 48kbps, [4] 32kbps*/
extern const uint16_t lc3_decode_run_mhz_dsp_call_1ch[2][2][4][LC3_BITRATE_CALL_1CH_COUNT];
extern const uint16_t lc3_encode_run_mhz_dsp_call_1ch[2][2][4][LC3_BITRATE_CALL_1CH_COUNT];

/*unit is 0.1MHz*/
/*dimensional-1: lc3 samplerate, [0] 48khz, [1] 32khz, [2] 24khz, [3] 16khz*/
extern const uint16_t postdae_run_mhz_dsp_music_2ch[4];
extern const uint16_t postdae_run_mhz_dsp_music_1ch[4];
extern const uint16_t postdae_run_mhz_dsp_call_1ch[4];
extern const uint16_t predae_run_mhz_dsp_call_1ch[4];
extern const uint16_t preaec_dual_mic_run_mhz_dsp_call_1ch[4];
extern const uint16_t preaec_single_mic_run_mhz_dsp_call_1ch[4];

extern const uint16_t playback_system_run_mhz;
extern const uint16_t capture_system_run_mhz;


#endif /* _LE_AUDIO_APP_H */
