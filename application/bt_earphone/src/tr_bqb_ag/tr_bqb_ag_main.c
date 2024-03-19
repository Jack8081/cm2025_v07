/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call main.
 */

#include "tr_bqb_ag.h"
#include "tts_manager.h"
#include <sys_wakelock.h>

#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif
static struct tr_bqb_ag_app_t *p_tr_bqb_ag_app;

static int _tr_bqb_ag_init(void)
{
	int ret = 0;

	p_tr_bqb_ag_app = app_mem_malloc(sizeof(struct tr_bqb_ag_app_t));
	if (!p_tr_bqb_ag_app) {
	    SYS_LOG_ERR("malloc fail!\n");
		ret = -ENOMEM;
		return ret;
	}

//	bt_manager_set_stream_type(AUDIO_STREAM_VOICE);

	tr_bqb_ag_view_init();

    api_bt_auto_move_keep_stop(1);
    bt_manager_set_connectable(true);

	tr_bqb_ag_ring_manager_init();

    sys_wake_lock(FULL_WAKE_LOCK);
	SYS_LOG_INF("finished\n");
	return ret;
}

static void _tr_bqb_ag_exit(void)
{
	if (!p_tr_bqb_ag_app)
   		goto exit;

	tr_bqb_ag_stop_play();

	tr_bqb_ag_ring_manager_deinit();

	tr_bqb_ag_view_deinit();

    app_mem_free(p_tr_bqb_ag_app);
	p_tr_bqb_ag_app = NULL;

    sys_wake_unlock(FULL_WAKE_LOCK);
#ifdef CONFIG_PROPERTY
    property_flush_req(NULL);
#endif

exit:
	app_manager_thread_exit(APP_ID_TR_BQB_AG);
	SYS_LOG_INF(" finished \n");
}


static void tr_bqb_ag_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;
	bool terminaltion = false;
    printk("***************************************************\n");
    printk("*****************   tr_bqb_ag   *******************\n");
    printk("***************************************************\n");
	if (_tr_bqb_ag_init()) {
		SYS_LOG_ERR(" init failed");
		_tr_bqb_ag_exit();
		return;
	}

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n",msg.type, msg.value);
			switch (msg.type) {
				case MSG_EXIT_APP:
					_tr_bqb_ag_exit();
					terminaltion = true;
					break;
				case MSG_BT_EVENT: //27
				    tr_bqb_ag_bt_event_proc(&msg);
					break;
				case MSG_INPUT_EVENT:
				    tr_bqb_ag_input_event_proc(&msg);
					break;
				case MSG_TTS_EVENT:
					tr_bqb_ag_tts_event_proc(&msg);
					break;
				default:
					SYS_LOG_ERR("error: 0x%x \n", msg.type);
					break;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
		thread_timer_handle_expired();
	}
}

struct tr_bqb_ag_app_t *tr_bqb_ag_get_app(void)
{
	return p_tr_bqb_ag_app;
}

APP_DEFINE(tr_bqb_ag, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   FOREGROUND_APP, NULL, NULL, NULL,
	   tr_bqb_ag_main_loop, NULL);
