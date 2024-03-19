/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ui service interface
 */
#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <msg_manager.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif
#include <os_common_api.h>
#include <thread_timer.h>
#include <display/display_composer.h>
#include "ui_service_inner.h"
#include "view_manager_inner.h"

#define SYS_LOG_DOMAIN "ui_service"

#ifndef CONFIG_SIMULATOR
static void _input_work_handler(struct k_work *work);
static OS_DELAY_WORK_DEFINE(input_work, _input_work_handler);
#endif

static uint8_t gui_lock_type = UISRV_GUI_NONE;
static atomic_t draw_task_cnt;
/* when draw_invoke_cnt is equal to draw_invoke_multiple, the draw task will be invoked */
static uint8_t draw_invoke_multiple = 1;
static uint8_t draw_invoke_cnt = 0;

void ui_service_lock_gui(uint8_t type)
{
	gui_lock_type |= type;
}

void ui_service_unlock_gui(uint8_t type)
{
	gui_lock_type &= ~type;
}

bool ui_service_is_draw_locked(void)
{
	return (gui_lock_type & UISRV_GUI_DRAW) ? true : false;
}

bool ui_service_is_input_locked(void)
{
	return (gui_lock_type & UISRV_GUI_INPUT) ? true : false;
}

int ui_service_register_gui_view_callback(uint8_t type,
		uint32_t preferred_pixel_format, const ui_view_gui_callback_t *callback)
{
	return view_manager_register_gui_callback(type, preferred_pixel_format, callback);
}

int ui_service_register_gui_input_callback(uint8_t type,
		const ui_input_gui_callback_t *callback, void *user_data)
{
	return input_dispatcher_register_gui_callaback(type,callback, user_data);
}

int ui_service_register_gesture_callback(const ui_gesture_callback_t *callback)
{
	return gesture_manager_register_callback(callback);
}

bool ui_service_send_self_msg(uint8_t view_id, uint8_t msg_id)
{
	struct app_msg msg = { .type = MSG_UI_EVENT, .sender = view_id, .cmd = msg_id, };

	return send_async_msg(UI_SERVICE_NAME, &msg);
}

#ifndef CONFIG_SIMULATOR
static void _submit_to_display_queue(os_delayed_work * work, uint32_t delay_ms)
{
	os_work_q *queue = os_get_display_work_queue();

	if (os_delayed_work_is_pending(work)) {
		SYS_LOG_DBG("work %p pending", work);
		return;
	}

	if (queue) {
		os_delayed_work_submit_to_queue(queue, work, delay_ms);
	} else {
		os_delayed_work_submit(work, delay_ms);
	}
}
#endif

static void _input_work_handler(struct k_work *work)
{
	input_dispatcher_read_task();
	view_manager_animation_process();
	view_manager_post_display();

	/* Control the draw task run cnt: 1 running + 1 waiting */
	if (!ui_service_is_draw_locked()) {
		if (draw_invoke_cnt == 0 && atomic_get(&draw_task_cnt) < 2) {
			if (ui_service_send_self_msg(UI_VIEW_INV_ID, MSG_POST_DISPLAY) == true) {
				atomic_inc(&draw_task_cnt);
			}
		}

		if (++draw_invoke_cnt >= draw_invoke_multiple) {
			draw_invoke_cnt = 0;
		}
	}
}

static void _display_vsync_callback(const struct display_callback * callback, uint32_t timestamp)
{
#ifndef CONFIG_SIMULATOR
	static uint32_t last_timestamp;
	uint32_t period_minus3;

	/* current tick is 2 ms, so minus 3 */
	period_minus3 = k_cyc_to_ms_floor32(timestamp - last_timestamp) - 3;
	if (period_minus3 > 57) {
		/* exceed normal range, fallback to default value.
		 * it may be the first vsync after bootup or resume
		 */
		period_minus3 = 13;
	}

	last_timestamp = timestamp;
	_submit_to_display_queue(&input_work, period_minus3);
#else
	_input_work_handler(NULL);
#endif
}

static void _display_pm_notify_handler(const struct display_callback *callback, uint32_t pm_action)
{
#ifndef CONFIG_SIMULATOR
	switch (pm_action) {
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		//gesture_manager_lock_scroll();
		ui_service_lock_gui(UISRV_GUI_INPUT);
		break;
	case PM_DEVICE_ACTION_LATE_RESUME:
		ui_service_unlock_gui(UISRV_GUI_INPUT);
		//gesture_manager_unlock_scroll();
		view_manager_resume_display();
		break;
	default:
		break;
	}
#endif
}

static const struct display_callback callback = {
	.vsync = _display_vsync_callback,
	.complete = NULL,
	.pm_notify = _display_pm_notify_handler,
};

static void _ui_service_init(void)
{
	if (display_composer_init())
		SYS_LOG_ERR("display_composer_init failed\n");

	display_composer_register_callback(&callback);

	view_manager_init();
	input_dispatcher_init();

	SYS_LOG_INF("ok");
}

static void _ui_service_exit(void)
{
	display_composer_destroy();

	input_dispatcher_deinit();
	SYS_LOG_INF("ok");
}

static void _ui_service_process_ui_event(struct app_msg *msg)
{
	SYS_LOG_DBG("cmd %d \n",msg->cmd);
	switch (msg->cmd) {
	case MSG_VIEW_CREATE:
		view_create(msg->sender, msg->ptr, msg->reserve);
		break;
	case MSG_VIEW_LAYOUT:
		view_layout(msg->sender);
		break;
	case MSG_VIEW_DELETE:
		view_delete(msg->sender);
		break;
	case MSG_VIEW_PAINT:
		if (!ui_service_is_draw_locked()) {
			view_paint(msg->sender);
		}
		break;
	case MSG_VIEW_REFRESH:
		view_refresh(msg->sender);
		break;
	case MSG_VIEW_SET_HIDDEN:
		view_set_hidden(msg->sender, msg->value);
		break;
	case MSG_VIEW_SET_DRAG_ATTRIBUTE:
		view_set_drag_attribute(msg->sender, msg->value);
		break;
	case MSG_VIEW_FOCUS:
		view_notify_message(msg->sender, MSG_VIEW_FOCUS, NULL);
		break;
	case MSG_VIEW_DEFOCUS:
		view_notify_message(msg->sender, MSG_VIEW_DEFOCUS, NULL);
		break;
	case MSG_VIEW_UPDATE:
		view_notify_message(msg->sender, MSG_VIEW_UPDATE, NULL);
		break;
	case MSG_VIEW_PAUSE:
		view_pause(msg->sender);
		break;
	case MSG_VIEW_RESUME:
		view_resume(msg->sender);
		break;
	case MSG_VIEW_SET_ORDER:
		view_set_order(msg->sender, msg->value);
		break;
	case MSG_VIEW_SET_POS:
		view_set_pos(msg->sender, msg->short_content[0], msg->short_content[1]);
		break;

	case MSG_FORCE_POST_DISPLAY:
		atomic_inc(&draw_task_cnt);
		/* fall through */
	case MSG_POST_DISPLAY:
		if (!ui_service_is_draw_locked()) {
			view_manager_gui_task_handler();
		}
		atomic_dec(&draw_task_cnt);
		break;
	case MSG_RESUME_DISPLAY:
		view_manager_resume_display();
		break;

	case MSG_VIEW_SET_DRAW_PERIOD:
		if (msg->value > 0) {
			draw_invoke_multiple = (uint8_t)msg->value;
			SYS_LOG_INF("draw period %d", draw_invoke_multiple);
		}
		break;
	case MSG_VIEW_SET_CALLBACK:
		view_manager_set_callback(msg->reserve, msg->ptr);
		break;

	case MSG_MSGBOX_POPUP:
		if (!ui_service_is_draw_locked()) {
			view_manager_popup_msgbox(msg->sender);
		}
		break;
	case MSG_MSGBOX_CLOSE:
		view_manager_close_msgbox(msg->sender, -1);
		break;

	case MSG_GESTURE_ENABLE:
		gesture_manager_set_enabled(msg->value);
		break;
	case MSG_GESTURE_SET_DIR:
		gesture_manager_set_dir(msg->value);
		break;
	case MSG_GESTURE_WAIT_RELEASE:
		gesture_manager_wait_release();
		break;
	case MSG_GESTURE_FORBID_DRAW:
		gesture_manager_forbid_draw(msg->value);
		break;
	default:
		if (msg->cmd >= MSG_VIEW_USER_OFFSET) {
			view_notify_message(msg->sender, msg->cmd, (void *)msg->value);
		}
		break;
	}
}

static void _ui_service_process_input_event(struct app_msg *msg)
{
	view_manager_dispatch_key_event(msg->value);
}

void _ui_service_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;
	int result = 0;
	bool suspended = false;
	int timeout;

	SYS_LOG_INF("enter");

	while (!terminaltion) {
		timeout = suspended ? OS_FOREVER : thread_timer_next_timeout();
		if (receive_msg(&msg, timeout)) {
			SYS_LOG_DBG("%d %d \n",msg.type, msg.cmd);
			switch (msg.type) {
			case MSG_UI_EVENT:
				_ui_service_process_ui_event(&msg);
				break;
			case MSG_KEY_INPUT:
				_ui_service_process_input_event(&msg);
				break;
			case MSG_INIT_APP:
				_ui_service_init();
				break;
			case MSG_EXIT_APP:
				_ui_service_exit();
				terminaltion = true;
				break;
			case MSG_SUSPEND_APP:
				suspended = true;
				break;
			case MSG_RESUME_APP:
				suspended = false;
				break;
			default:
				break;
			}

			if (msg.callback) {
				msg.callback(&msg, result, NULL);
			}
		}

		thread_timer_handle_expired();
	}
}
