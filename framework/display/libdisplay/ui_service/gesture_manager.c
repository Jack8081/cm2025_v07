/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ui service interface
 */

#include <os_common_api.h>
#ifdef CONFIG_DMA2D_HAL
#  include <dma2d_hal.h>
#endif
#include "view_manager_inner.h"
#include "ui_service_inner.h"

#define GESTURE_START_THRESHOLD  100
#define GESTURE_MOVE_THRESHOLD   10

#define SYS_LOG_DOMAIN "gesture_mananger"

typedef struct gesture_context {
	uint8_t dir; /* Gesture filter direction bitfield */
	uint8_t forbid_draw  : 1;

	const ui_gesture_callback_t *callback;
} gesture_context_t;

static gesture_context_t gesture_ctx = {
	.dir = GESTURE_ALL_BITFIELD,
	.forbid_draw = 1,
};

int gesture_manager_register_callback(const ui_gesture_callback_t *callback)
{
	gesture_ctx.callback = callback;
	return 0;
}

void gesture_manager_set_dir(uint8_t filter)
{
	gesture_ctx.dir = (filter & GESTURE_ALL_BITFIELD);
}

void gesture_manager_set_enabled(bool en)
{
	input_dev_t * pointer_input_dev = input_dispatcher_get_pointer_dev();
	input_dev_runtime_t *runtime = &pointer_input_dev->proc;

	runtime->types.pointer.scroll_disabled = !en;
}

void gesture_manager_wait_release(void)
{
	input_dev_t * pointer_input_dev = input_dispatcher_get_pointer_dev();
	input_dev_runtime_t *runtime = &pointer_input_dev->proc;

	runtime->types.pointer.scroll_wait_until_release = 1;
}

void gesture_manager_forbid_draw(bool en)
{
	gesture_ctx.forbid_draw = en;
}

static void _dispatch_gesture_event(uint8_t gesture)
{
	uint16_t key_val = gesture - GESTURE_DROP_DOWN + KEY_GESTURE_DOWN;

	view_manager_dispatch_key_event(KEY_EVENT(key_val, KEY_TYPE_ALL));
}

int8_t gesture_manager_get_sw_gesture(input_dev_t * pointer_input_dev)
{
	if (gesture_ctx.callback->detect) {
		return (int8_t)gesture_ctx.callback->detect(pointer_input_dev);
	}

	return -1;
}

void gesture_manager_process(input_dev_t * pointer_input_dev)
{
	input_dev_runtime_t *runtime = &pointer_input_dev->proc;
	point_t *act_point = &runtime->types.pointer.act_point;

	/* try to stop the single view drag animation if any */
	view_manager_animation_stop(false);

	if (runtime->types.pointer.scroll_in_prog == 0) {
		if (gesture_ctx.dir == 0 || !(gesture_ctx.dir & (0x1 << runtime->gesture))) {
			return;
		}

		/* stop the previous animation, and refocus the views */
		view_manager_animation_stop(true);

		if (gesture_ctx.callback->scroll_begin(pointer_input_dev)) {
			if (runtime->types.pointer.gesture_sent == 0) {
				_dispatch_gesture_event(runtime->gesture);
				runtime->types.pointer.gesture_sent = 1;
			}

			return;
		}

		runtime->types.pointer.scroll_in_prog = 1;
		runtime->types.pointer.vect.x = act_point->x - runtime->types.pointer.scroll_start.x;
		runtime->types.pointer.vect.y = act_point->y - runtime->types.pointer.scroll_start.y;
		runtime->types.pointer.scroll_sum.x = runtime->types.pointer.vect.x;
		runtime->types.pointer.scroll_sum.y = runtime->types.pointer.vect.y;

		input_dispatcher_set_gui_wait_release();

#ifdef CONFIG_SURFACE_DOUBLE_BUFFER
		/* FIXME: should we disable the drawing of current focused view
		 * which has only single buffer buffer
		 */
		ui_service_lock_gui(UISRV_GUI_INPUT);
	#ifdef CONFIG_DMA2D_HAL
		hal_dma2d_set_global_enabled(false);
	#endif
#else
		ui_service_lock_gui(UISRV_GUI_BOTH);
#endif
	}

	if (runtime->types.pointer.scroll_in_prog) {
		runtime->types.pointer.scroll_throw_vect.x = (runtime->types.pointer.scroll_throw_vect.x * 4) >> 3;
		runtime->types.pointer.scroll_throw_vect.x += (runtime->types.pointer.vect.x * 4) >> 3;

		runtime->types.pointer.scroll_throw_vect.y = (runtime->types.pointer.scroll_throw_vect.y * 4) >> 3;
		runtime->types.pointer.scroll_throw_vect.y += (runtime->types.pointer.vect.y * 4) >> 3;

		gesture_ctx.callback->scroll(pointer_input_dev);
	}
}

void gesture_manager_stop_process(input_dev_t * pointer_input_dev, bool anim_off)
{
	input_dev_runtime_t *runtime = &pointer_input_dev->proc;

	if (runtime->types.pointer.scroll_in_prog) {
		gesture_ctx.callback->scroll_end(pointer_input_dev);
		runtime->types.pointer.scroll_in_prog = 0;
	}

	if (view_manager_animation_get_state() == UI_ANIM_NONE) {
		/* unlock forbid draw ui */
		ui_service_unlock_gui(UISRV_GUI_BOTH);
#ifdef CONFIG_DMA2D_HAL
		hal_dma2d_set_global_enabled(true);
#endif
	} else if (anim_off) {
		view_manager_animation_stop(true);
	}
}

bool gesture_is_scrolling(void)
{
	input_dev_t * pointer_input_dev = input_dispatcher_get_pointer_dev();
	input_dev_runtime_t *runtime = &pointer_input_dev->proc;

	return runtime->types.pointer.scroll_in_prog ||
		view_manager_animation_get_state() != UI_ANIM_NONE;
}
