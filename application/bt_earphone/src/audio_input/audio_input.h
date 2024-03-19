/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio input app
 */

#ifndef _AUDIO_INPUT_APP_H
#define _AUDIO_INPUT_APP_H


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio_input"
#ifdef SYS_LOG_LEVEL
#undef SYS_LOG_LEVEL
#endif

#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <property_manager.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <audiolcy_common.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <thread_timer.h>
#include "btservice_api.h"


#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#include "app_switch.h"
#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#ifdef CONFIG_BLUETOOTH
#include <bt_manager.h>
#endif

#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif

enum AUDIO_INPUT_PLAY_STATUS {
	AUDIO_INPUT_STATUS_NULL      = 0x0000,
	AUDIO_INPUT_STATUS_PLAYING   = 0x0001,
	AUDIO_INPUT_STATUS_PAUSED    = 0x0002,
};

#define AUDIO_INPUT_STATUS_ALL  (AUDIO_INPUT_STATUS_PLAYING | AUDIO_INPUT_STATUS_PAUSED)

struct audio_input_app_t {
	media_player_t *player;
	struct thread_timer monitor_timer;
	struct thread_timer key_timer;
	u32_t playing : 1;
	u32_t need_resume_play : 1;
	u32_t media_opened : 1;
	u32_t need_resample : 1;
	u32_t need_resume_key : 1;
	u32_t key_handled : 1;

	io_stream_t audio_input_stream;

	int last_disp_status;
};

void audio_input_view_init(bool key_remap);
void audio_input_view_deinit(void);
void audio_input_view_display(void);
struct audio_input_app_t *audio_input_get_app(void);
int audio_input_get_status(void);
void audio_input_start_play(void);
void audio_input_stop_play(void);
void audio_input_input_event_proc(struct app_msg *msg);
void audio_input_tts_event_proc(struct app_msg *msg);
void audio_input_bt_event_proc(struct app_msg *msg);
int audio_input_get_audio_stream_type(char *app_name);

#endif  /* _AUDIO_INPUT_APP_H */

