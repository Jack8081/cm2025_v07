/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call ring
 */

#include "tr_bqb_ag.h"
#include "tts_manager.h"

enum
{
    STATE_RING_INIT,
    STATE_RING_READY,
    STATE_RING_PLAYING,
    STATE_RING_DONE,
};

typedef struct {
	u8_t current_index;
	u8_t phone_num_cnt;
	u8_t phone_num[32];
}tr_bqb_ag_ring_info_t;

typedef struct {
	u8_t state;
	tr_bqb_ag_ring_info_t *ring_info;
	struct thread_timer call_ring_timer;
}tr_bqb_ag_ring_context_t;

static tr_bqb_ag_ring_context_t *ring_manager_context = NULL;

static tr_bqb_ag_ring_context_t * _tr_bqb_ag_ring_get_manager(void)
{
	return ring_manager_context;
}

static int _tr_bqb_ag_ring_get_file_name(u8_t num, u8_t *file_name, u8_t file_name_len)
{
	if ((num >= '0') && (num <= '9')) {
		snprintf(file_name, file_name_len, "%c.mp3", num);
	} else if(num == 0xff){
		snprintf(file_name, file_name_len, "%s.mp3", "callring");
	}

	return 0;
}
#ifdef CONFIG_PLAYTTS
static void _tr_bqb_ag_ring_tts_event_nodify(u8_t *tts_id, u32_t event)
{
	u8_t file_name[13];
	tr_bqb_ag_ring_context_t *ring_manager = _tr_bqb_ag_ring_get_manager();
	tr_bqb_ag_ring_info_t *ring_info = ring_manager->ring_info;

	switch (event) {
		case TTS_EVENT_START_PLAY:
			break;
		case TTS_EVENT_STOP_PLAY:
			if(ring_info && ring_manager->state != STATE_RING_DONE) {
				ring_info->current_index %= ring_info->phone_num_cnt; 
				_tr_bqb_ag_ring_get_file_name(ring_info->phone_num[ring_info->current_index++], file_name, sizeof(file_name));
			#ifdef CONFIG_PLAYTTS
				tts_manager_play(file_name, 0);
			#endif
			}
			break;
	}	
}
#endif

static void _tr_bqb_ag_ring_start(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	tr_bqb_ag_ring_context_t *ring_manager = _tr_bqb_ag_ring_get_manager();
	u8_t file_name[13];

	_tr_bqb_ag_ring_get_file_name(
		ring_manager->ring_info->phone_num[ring_manager->ring_info->current_index++],
		file_name, sizeof(file_name));

#ifdef CONFIG_PLAYTTS
	tts_manager_add_event_lisener(_tr_bqb_ag_ring_tts_event_nodify);

	tts_manager_play(file_name, 0);
#endif

	ring_manager->state = STATE_RING_PLAYING;
}

int tr_bqb_ag_ring_start(u8_t *phone_num, u16_t phone_num_cnt)
{
	tr_bqb_ag_ring_info_t *ring_info = NULL;
	tr_bqb_ag_ring_context_t *ring_manager = _tr_bqb_ag_ring_get_manager();
	SYS_LOG_INF(" %s cnt %d\n", phone_num, phone_num_cnt);
	if (ring_manager->ring_info)
		return -EBUSY;

	ring_info = app_mem_malloc(sizeof(tr_bqb_ag_ring_info_t));
	if (!ring_info)
		return -ENOMEM;

	memset(ring_info, 0, sizeof(tr_bqb_ag_ring_info_t));

	if(phone_num_cnt > sizeof(ring_info->phone_num))
		phone_num_cnt = sizeof(ring_info->phone_num);

	ring_info->phone_num_cnt = phone_num_cnt;
	ring_info->current_index = 0;
	memcpy(ring_info->phone_num, phone_num, phone_num_cnt);
	ring_info->phone_num[ring_info->phone_num_cnt++] = 0xff;

	ring_manager->state = STATE_RING_READY;
	ring_manager->ring_info = ring_info;

	thread_timer_start(&ring_manager->call_ring_timer, 800, 0);
	return 0;
}

void tr_bqb_ag_ring_stop(void)
{
	tr_bqb_ag_ring_context_t *ring_manager = _tr_bqb_ag_ring_get_manager();

	ring_manager->state = STATE_RING_DONE;

	if (thread_timer_is_running(&ring_manager->call_ring_timer))
		thread_timer_stop(&ring_manager->call_ring_timer);

	if (ring_manager->ring_info) {
	#ifdef CONFIG_PLAYTTS
		tts_manager_wait_finished(true);
	#endif
		app_mem_free(ring_manager->ring_info);
		ring_manager->ring_info = NULL;
	}
#ifdef CONFIG_PLAYTTS
	tts_manager_remove_event_lisener(_tr_bqb_ag_ring_tts_event_nodify);
#endif
	SYS_LOG_INF(" ok \n");
}

int tr_bqb_ag_ring_manager_init(void)
{
	if (ring_manager_context)
		return -EEXIST;

	ring_manager_context = app_mem_malloc(sizeof(tr_bqb_ag_ring_context_t));
	if (!ring_manager_context)
		return -ENOMEM;

	ring_manager_context->state = STATE_RING_INIT;

	thread_timer_init(&ring_manager_context->call_ring_timer, _tr_bqb_ag_ring_start, NULL);

	return 0;
}

int tr_bqb_ag_ring_manager_deinit(void)
{
	tr_bqb_ag_ring_stop();
	if (ring_manager_context) {
		if (ring_manager_context->ring_info) {
			app_mem_free(ring_manager_context->ring_info);
			ring_manager_context->ring_info = NULL;
		}
		app_mem_free(ring_manager_context);
		ring_manager_context = NULL;
	}

	return 0;
}

