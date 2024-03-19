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
#ifdef CONFIG_UI_INPUT_RECORDER
#include <input_recorder.h>
#endif
#include <input_manager.h>
#include <thread_timer.h>
#include <memory/mem_cache.h>
#include "ui_service_inner.h"
#include <sys_wakelock.h>

#define SYS_LOG_DOMAIN "input_dispatcher"

//#define CONFIG_PROFILING   1

struct input_dispatcher_t {
	point_t origin;
	const ui_input_gui_callback_t *callback;
	void *user_data;
};

#ifdef CONFIG_PROFILING
static bool press_flag = false;
static int tp_read_time_stampe = 0;
static int display_post_time_stampe = 0;
#endif

static struct input_dispatcher_t input_dispatcher;

#ifdef CONFIG_TP_TRAIL_DRAW
void input_dispatcher_draw_trail(int base, uint16_t width, uint16_t height, uint16_t stride, uint8_t bpp)
{
	short draw_color = 0x0000;
	static bool clean_flag = false;
	input_dev_t * pointer_input_dev = input_dispatcher_get_pointer_dev();
	int point_x = pointer_input_dev->proc.types.pointer.act_point.x - 120;
	int point_y = pointer_input_dev->proc.types.pointer.act_point.y;

#ifdef CONFIG_PROFILING
	if (press_flag) {
		os_printk("post (%d %d ) %d  duration %d us cost %d\n",
			 point_x, point_y, os_cycle_get_32(),
			(os_cycle_get_32() - display_post_time_stampe) / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			(os_cycle_get_32() - tp_read_time_stampe) / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
			display_post_time_stampe = os_cycle_get_32();
	}
#else
	if(pointer_input_dev->proc.state) {
		short * addr = (short *)(base + ((point_y) * stride + (point_x - 1)) * bpp / 8);
		*addr = draw_color;
		*(addr + 1) = draw_color;
		*(addr + 1) = draw_color;
		addr = (short *)(base + ((point_y + 1) * stride + (point_x - 1)) * bpp / 8);
		*addr = draw_color;
		*(addr + 1) = draw_color;
		*(addr + 1) = draw_color;

		addr = (short *)(base + ((point_y - 1) * stride + (point_x - 1)) * bpp / 8);
		*addr = draw_color;
		*(addr + 1) = draw_color;
		*(addr + 1) = draw_color;
		mem_dcache_clean(addr, width * 3 * bpp);
		clean_flag = false;
	} else if(!clean_flag){
		memset((void *)base, 0xff, width * height * bpp);
		mem_dcache_clean((void *)base, width * height * bpp);
		clean_flag = true;
	}
#endif /* CONFIG_PROFILING */
}
#endif /* CONFIG_TP_TRAIL_DRAW */

static bool _input_dispatcher_system_event_process(input_dev_t * pointer_input_dev, input_dev_data_t *data)
{
	input_dev_runtime_t *runtime = &pointer_input_dev->proc;
	uint8_t last_state = runtime->state;
	int8_t gesture = 0;
	bool is_system_event = false;

	runtime->types.pointer.act_point.x = data->point.x;
	runtime->types.pointer.act_point.y = data->point.y;
	runtime->state = data->state;

#ifdef CONFIG_SYS_WAKELOCK
	if (last_state != data->state) {
		if (data->state == INPUT_DEV_STATE_PR) {
	#ifdef CONFIG_SCREEN_WAKELOCK
			sys_wake_lock(SCREEN_WAKE_LOCK);
	#endif
			sys_wake_lock(FULL_WAKE_LOCK);
		} else {
	#ifdef CONFIG_SCREEN_WAKELOCK
			sys_wake_unlock(SCREEN_WAKE_LOCK);
	#endif
			sys_wake_unlock(FULL_WAKE_LOCK);
		}
	}
#endif /* CONFIG_SYS_WAKELOCK */

	if (runtime->types.pointer.scroll_disabled) {
		return false;
	}

	if (data->state == INPUT_DEV_STATE_PR) {
		if (runtime->types.pointer.scroll_wait_until_release) {
			return false;
		}

		if (last_state == INPUT_DEV_STATE_REL) {
			runtime->types.pointer.last_point.x = runtime->types.pointer.act_point.x;
			runtime->types.pointer.last_point.y = runtime->types.pointer.act_point.y;
			runtime->types.pointer.scroll_sum.x = 0;
			runtime->types.pointer.scroll_sum.y = 0;
		}

		runtime->types.pointer.vect.x = runtime->types.pointer.act_point.x - runtime->types.pointer.last_point.x;
		runtime->types.pointer.vect.y = runtime->types.pointer.act_point.y - runtime->types.pointer.last_point.y;
		runtime->types.pointer.scroll_sum.x += runtime->types.pointer.vect.x;
		runtime->types.pointer.scroll_sum.y += runtime->types.pointer.vect.y;

		gesture = gesture_manager_get_sw_gesture(pointer_input_dev);
		if (gesture < 0) {
			gesture = data->gesture;
		}

		if (runtime->types.pointer.scroll_in_prog == 0) {
			runtime->gesture = gesture;
			runtime->last_gesture = gesture;
		} else if (gesture != 0) {
			runtime->last_gesture = gesture;
		}

		gesture_manager_process(pointer_input_dev);
	} else {
		if (last_state == INPUT_DEV_STATE_PR) {
			gesture_manager_stop_process(pointer_input_dev, false);
		}

		runtime->types.pointer.scroll_wait_until_release = 0;
		runtime->types.pointer.gesture_sent = 0;
	}

	runtime->types.pointer.last_point.x = runtime->types.pointer.act_point.x;
	runtime->types.pointer.last_point.y = runtime->types.pointer.act_point.y;

	return is_system_event;
}

void input_dispatcher_read_task(void)
{
	input_dev_t * pointer_input_dev = input_dispatcher_get_pointer_dev();
	input_dev_data_t data;
	bool more_to_read = false;

	if (!pointer_input_dev)
		return;

	do {
	#ifdef CONFIG_UI_INPUT_RECORDER
		if (input_playback_is_running()) {
			input_playback_read(&data);
		} else {
			more_to_read = input_dev_read(pointer_input_dev, &data);
			if (input_capture_is_running()) {
				input_capture_write(&data);
			}
		}
	#else
		more_to_read = input_dev_read(pointer_input_dev, &data);

	#endif /* CONFIG_UI_INPUT_RECORDER */

	#ifdef CONFIG_PROFILING
		press_flag = data.state;
		if (press_flag) {
			os_printk("read (%d %d ) %d duration %d us \n", data.point.x, data.point.y,
				os_cycle_get_32(),(os_cycle_get_32() - tp_read_time_stampe) / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
			tp_read_time_stampe = os_cycle_get_32();
		}
	#endif
	} while(more_to_read);

	if (!_input_dispatcher_system_event_process(pointer_input_dev, &data)) {
		if (input_dispatcher.callback && !ui_service_is_input_locked()) {
			data.point.x -= input_dispatcher.origin.x;
			data.point.y -= input_dispatcher.origin.y;
			input_dispatcher.callback->put_data(&data, input_dispatcher.user_data);
		}
	}
}

void input_dispatcher_set_gui_wait_release(void)
{
	if (input_dispatcher.callback)
		input_dispatcher.callback->wait_release(input_dispatcher.user_data);
}

void input_dispatcher_set_gui_origin(int16_t x, int16_t y)
{
	input_dispatcher.origin.x = x;
	input_dispatcher.origin.y = y;

	if (input_dispatcher.callback)
		input_dispatcher.callback->wait_release(input_dispatcher.user_data);
}

int input_dispatcher_register_gui_callaback(uint8_t type,
		const ui_input_gui_callback_t *callback, void *user_data)
{
	assert(type == INPUT_DEV_TYPE_POINTER);

	input_dispatcher.callback = callback;
	input_dispatcher.user_data = user_data;
	return 0;
}

int input_dispatcher_init(void)
{
	input_dev_t * pointer_input_dev = input_dispatcher_get_pointer_dev();

	if (pointer_input_dev == NULL) {
		SYS_LOG_ERR("input pointer dev not found\n");
		return -ENODEV;
	}

	input_manager_dev_enable(pointer_input_dev);
	return 0;
}

int input_dispatcher_deinit(void)
{
	return 0;
}
