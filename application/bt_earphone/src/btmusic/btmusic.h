/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music
 */

#ifndef _BT_MUSIC_APP_H_
#define _BT_MUSIC_APP_H_

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "btmusic"
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
#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
#include <system_app.h>
#endif
#endif

#define DELAY_MUSIC_TIME_AFTER_BTCALL_MASTER (600)
#define DELAY_MUSIC_TIME_AFTER_BTCALL_SLAVE (1000)
#define DELAY_MUSIC_TIME_AFTER_SWITCH_PHONE (400)


struct btmusic_app_t {
	media_player_t *player;
	struct thread_timer monitor_timer;
	struct thread_timer play_timer;
	struct thread_timer key_timer;
	u8_t last_disp_status;
	u32_t need_resume_key : 1;
	u32_t key_handled : 1;
	u32_t mutex_stop : 1;
	u32_t tws_con_tts_playing : 1;
	u32_t media_opened : 1;
	u32_t app_exiting : 1;
    u32_t mic_mute : 1;
#ifdef CONFIG_BT_MUSIC_SWITCH_LANTENCY_SUPPORT 
    u32_t lantency_mode : 1;
#endif
	u8_t dae_enalbe;
	u8_t multidae_enable;
	u8_t current_dae;
	u8_t dae_cfg_nums;
	io_stream_t sink_stream;
	CFG_Struct_BT_Music_Stop_Hold    cfg_bt_music_stop_hold;
	u8_t          cfg_dae_data[CFG_SIZE_BT_MUSIC_DAE_AL];
	CFG_Struct_BTMusic_User_Settings user_settings;

	/* Audio Sink: BT -> sink_stream -> playback_stream */
	struct bt_audio_chan sink_chan;	
    u8_t  avdtp_filter;
    u8_t  tws_avdtp_filter;
    u8_t  avdtp_resume;
#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    const struct device *gpio_dev;
    os_delayed_work aux_plug_in_delay_work;
#endif
#endif
};

struct btmusic_app_t *btmusic_get_app(void);

int bt_music_handle_enable(struct bt_audio_report *rep);
int bt_music_handle_start(struct bt_audio_report *rep);
int bt_music_stop_play(void);
int bt_music_pre_stop_play(void);
int bt_music_handle_stop(struct bt_audio_report *rep);
int bt_music_handle_release(struct bt_audio_report *rep);
int bt_music_handle_disable(struct bt_audio_report *rep);
#ifdef CONFIG_BT_MUSIC_SWITCH_LANTENCY_SUPPORT
void bt_music_handle_switch_lantency(void);
#endif

bool btmusic_key_event_handle(int key_event, int event_stage);

void btmusic_bt_event_proc(struct app_msg *msg);

void btmusic_tts_event_proc(struct app_msg *msg);

void btmusic_input_event_proc(struct app_msg *msg);

void btmusic_tws_event_proc(struct app_msg *msg);

void bt_music_esd_restore(void);

void bt_music_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg);

void btmusic_view_init(bool key_remap);

void btmusic_view_deinit(void);

void btmusic_view_display(void);

void bt_music_check_channel_output(bool speaker_update);

void btmusic_event_notify(int event);

int btmusic_tws_send(int receiver, int event, void* param, int len, bool sync_wait);

void bt_music_tws_cancel_filter(void);
int btmusic_ctrl_avdtp_is_filter(void);
void btmusic_multi_dae_adjust(u8_t dae_index);
void btmusic_update_dae(void);

#endif  /* _BT_MUSIC_APP_H */


