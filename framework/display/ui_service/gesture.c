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
#include <view_manager.h>
#include <input_manager.h>
#include <ui_service.h>

#ifdef CONFIG_UI_CUSTOM_GESTURE_DETECTION
#  define POINTER_GESTURE_MIN_VEC  3
#  define POINTER_GESTURE_LIMIT    20
#endif /* CONFIG_UI_CUSTOM_GESTURE_DETECTION */

#define GESTURE_START_THRESHOLD  100
#define GESTURE_MOVE_THRESHOLD   10

static const uint8_t opposite_gestures[] = {
	0, GESTURE_DROP_UP, GESTURE_DROP_DOWN, GESTURE_DROP_RIGHT, GESTURE_DROP_LEFT,
};

static void _reposition_region_in_display(ui_region_t *region)
{
	ui_region_t cont = {
		.x1 = 0,
		.y1 = 0,
		.x2 = view_manager_get_disp_xres() - 1,
		.y2 = view_manager_get_disp_yres() - 1,
	};

	ui_region_fit_in(region, &cont);
}

#ifdef CONFIG_UI_CUSTOM_GESTURE_DETECTION
static uint8_t gesture_detect(input_dev_t * pointer_dev)
{
	static point_t gesture_sum;

	input_dev_runtime_t *runtime = &pointer_dev->proc;
	uint8_t gesture = 0;

	if (runtime->state == INPUT_DEV_STATE_PR) {
		if (UI_ABS(runtime->types.pointer.vect.x) < POINTER_GESTURE_MIN_VEC &&
			UI_ABS(runtime->types.pointer.vect.y) < POINTER_GESTURE_MIN_VEC) {
			gesture_sum.x = 0;
			gesture_sum.y = 0;
		}

		gesture_sum.x += runtime->types.pointer.vect.x;
		gesture_sum.y += runtime->types.pointer.vect.y;
		if (UI_ABS(gesture_sum.x) > POINTER_GESTURE_LIMIT ||
			UI_ABS(gesture_sum.y) > POINTER_GESTURE_LIMIT) {
			if (UI_ABS(gesture_sum.x) >= UI_ABS(gesture_sum.y)) {
				gesture = (gesture_sum.x > 0) ? GESTURE_DROP_RIGHT : GESTURE_DROP_LEFT;
			} else {
				gesture = (gesture_sum.y > 0) ? GESTURE_DROP_DOWN : GESTURE_DROP_UP;
			}
		}
	}

	return gesture;
}
#endif /* CONFIG_UI_CUSTOM_GESTURE_DETECTION */

static int gesture_scroll_begin(input_dev_t * pointer_dev)
{
	input_dev_runtime_t *runtime = &pointer_dev->proc;
	point_t *act_point = &runtime->types.pointer.act_point;	
	int16_t x_res = view_manager_get_disp_xres();
	int16_t y_res = view_manager_get_disp_yres();
	int16_t focus_id, focus_attr;
	bool towards_screen = false;

	runtime->view_id = view_manager_get_draggable_view(runtime->gesture, &towards_screen);
	if (runtime->view_id < 0) {
		return -1;
	}

	focus_id = view_manager_get_focused_view();
	focus_attr = view_get_drag_attribute(focus_id);
	if (focus_id != runtime->view_id && focus_attr > 0) {
		return -1;
	}

	runtime->types.pointer.scroll_to_screen = towards_screen;

	if (view_has_move_attribute(runtime->view_id)) { /* move */
		runtime->last_view_id = focus_id;
		runtime->pre_view_id = view_manager_get_draggable_view(opposite_gestures[runtime->gesture], NULL);
		runtime->types.pointer.scroll_start = *act_point;
		runtime->types.pointer.last_scroll_off = 0;
	} else  { /* drop */
		int16_t offset = 0;

		switch (runtime->gesture) {
		case GESTURE_DROP_DOWN:
			offset = act_point->y;
			break;
		case GESTURE_DROP_UP:
			offset = y_res - act_point->y;
			break;
		case GESTURE_DROP_RIGHT:
			offset = act_point->x;
			break;
		case GESTURE_DROP_LEFT:
			offset = x_res - act_point->x;
			break;
		default:
			break;
		}

		if (offset >= GESTURE_START_THRESHOLD)
			return -1;

		runtime->last_view_id = -1;
		runtime->pre_view_id = -1;

		/* consider as dragged from (0, 0) */
		runtime->types.pointer.scroll_start.x = 0;
		runtime->types.pointer.scroll_start.y = 0;
	}

	runtime->current_view_id = runtime->view_id;
	runtime->related_view_id = runtime->last_view_id;

	SYS_LOG_INF("gesture %d, start (%d %d), view %d, last_view %d, pre_view %d\n",
		runtime->gesture, act_point->x, act_point->y, runtime->view_id,
		runtime->last_view_id, runtime->pre_view_id);

	return 0;
}

static void gesture_scroll(input_dev_t * pointer_dev)
{
	input_dev_runtime_t *runtime = &pointer_dev->proc;
	point_t *act_point = &runtime->types.pointer.act_point;	
	int16_t x_res = view_manager_get_disp_xres();
	int16_t y_res = view_manager_get_disp_yres();
	bool is_vscroll = (runtime->gesture <= GESTURE_DROP_UP);
	ui_point_t drag_pos = { 0, 0 };
	ui_region_t rel_region = { 0, 0, x_res - 1, y_res - 1 };

	if (view_has_move_attribute(runtime->current_view_id)) {
		if (runtime->view_id == runtime->related_view_id) { /* long view  */
			view_get_region(runtime->view_id, &rel_region);
			if (is_vscroll) {
				ui_region_set_y(&rel_region, rel_region.y1 + runtime->types.pointer.vect.y);
			} else {
				ui_region_set_x(&rel_region, rel_region.x1 + runtime->types.pointer.vect.x);
			}

			_reposition_region_in_display(&rel_region);

			view_set_pos(runtime->view_id, rel_region.x1, rel_region.y1);
		} else {
			int16_t drop_sign;
			int16_t scroll_off;

			if (is_vscroll) {
				scroll_off = act_point->y - runtime->types.pointer.scroll_start.y;
				drag_pos.y = (runtime->gesture == GESTURE_DROP_DOWN) ? -y_res : y_res;
				drop_sign = (runtime->gesture == GESTURE_DROP_DOWN) ? 1 : -1;
			} else {
				scroll_off = act_point->x - runtime->types.pointer.scroll_start.x;
				drag_pos.x = (runtime->gesture == GESTURE_DROP_RIGHT) ? -x_res : x_res;
				drop_sign = (runtime->gesture == GESTURE_DROP_RIGHT) ? 1 : -1;
			}

			/* check move direction compared to the origin */
			if (scroll_off * drop_sign > 0) {
				if (runtime->view_id != runtime->current_view_id) {
					view_set_pos(runtime->view_id, -drag_pos.x, -drag_pos.y);
					runtime->view_id = runtime->current_view_id;
				}
			} else if (scroll_off * drop_sign < 0) {
				drag_pos.x = -drag_pos.x;
				drag_pos.y = -drag_pos.y;

				if (runtime->view_id == runtime->current_view_id) {
					view_set_pos(runtime->view_id, drag_pos.x, drag_pos.y);
					runtime->view_id = runtime->pre_view_id;
				}
			}

			runtime->types.pointer.last_scroll_off = scroll_off;

			if (runtime->view_id >= 0) {
				if (is_vscroll) {
					drag_pos.y += runtime->types.pointer.scroll_sum.y;
				} else {
					drag_pos.x += runtime->types.pointer.scroll_sum.x;
				}

				view_set_pos(runtime->view_id, drag_pos.x, drag_pos.y);
			}

			if (runtime->related_view_id >= 0) {
				if (is_vscroll) {
					ui_region_set_y(&rel_region, runtime->types.pointer.scroll_sum.y);
				} else {
					ui_region_set_x(&rel_region, runtime->types.pointer.scroll_sum.x);
				}

				if (runtime->view_id < 0) {
					_reposition_region_in_display(&rel_region);
				}

				view_set_pos(runtime->related_view_id, rel_region.x1, rel_region.y1);
			}

			SYS_LOG_DBG("scroll_off %d, view %d pos (%d,%d), related_view %d (%d,%d)\n",
				scroll_off, runtime->view_id, drag_pos.x, drag_pos.y,
				runtime->related_view_id, rel_region.x1, rel_region.y1);
		}
	} else {
		switch (runtime->gesture) {
		case GESTURE_DROP_DOWN:
			drag_pos.y = runtime->types.pointer.scroll_to_screen ?
					(act_point->y - y_res) : act_point->y;
			break;
		case GESTURE_DROP_UP:
			drag_pos.y = runtime->types.pointer.scroll_to_screen ?
					act_point->y : (act_point->y - y_res);
			break;
		case GESTURE_DROP_RIGHT:
			drag_pos.x = runtime->types.pointer.scroll_to_screen ?
					(act_point->x - x_res) : act_point->x;
			break;
		case GESTURE_DROP_LEFT:
		default:
			drag_pos.x = runtime->types.pointer.scroll_to_screen ?
					act_point->x : (act_point->x - x_res);
			break;
		}

		view_set_pos(runtime->view_id, drag_pos.x, drag_pos.y);
	}
}

static void gesture_fixed(input_dev_runtime_t *runtime)
{
	if (runtime->gesture != runtime->last_gesture && runtime->last_gesture > 0) {
		uint8_t temp_view_id = runtime->related_view_id;
		runtime->gesture = runtime->last_gesture;
		runtime->related_view_id = runtime->view_id;
		runtime->view_id = temp_view_id;
	}
}

static void gesture_scroll_end(input_dev_t * pointer_dev)
{
	static const uint8_t slidein_anim[] = {
		 0, UI_ANIM_SLIDE_IN_DOWN, UI_ANIM_SLIDE_IN_UP,
		UI_ANIM_SLIDE_IN_LEFT, UI_ANIM_SLIDE_IN_RIGHT,
	};
	static const uint8_t slideout_anim[] = {
		0, UI_ANIM_SLIDE_OUT_UP, UI_ANIM_SLIDE_OUT_DOWN,
		UI_ANIM_SLIDE_OUT_RIGHT, UI_ANIM_SLIDE_OUT_LEFT,
	};
	static const uint8_t slideback_anim[] = {
		0, UI_ANIM_SLIDE_IN_UP, UI_ANIM_SLIDE_IN_DOWN,
		UI_ANIM_SLIDE_IN_RIGHT, UI_ANIM_SLIDE_IN_LEFT,
	};
	static const uint8_t slideout_continue_anim[] = {
		0, UI_ANIM_SLIDE_OUT_DOWN, UI_ANIM_SLIDE_OUT_UP,
		UI_ANIM_SLIDE_OUT_LEFT, UI_ANIM_SLIDE_OUT_RIGHT,
	};
	input_dev_runtime_t *runtime = &pointer_dev->proc;
	int16_t x_res = view_manager_get_disp_xres();
	int16_t y_res = view_manager_get_disp_yres();
	int16_t animation_type = -1;
	int16_t scroll_off = 0;

	if (view_has_move_attribute(runtime->current_view_id)) {
		if (runtime->view_id == runtime->related_view_id) { /* long view  */
			view_manager_drag_animation_start(runtime->view_id, runtime);
			return;
		}

		scroll_off = UI_ABS(runtime->types.pointer.last_scroll_off);

		if (runtime->view_id == runtime->current_view_id) {
			if (scroll_off >= GESTURE_MOVE_THRESHOLD) {	
				gesture_fixed(runtime);
				animation_type = slidein_anim[runtime->gesture];
			} else {
				animation_type = slideout_anim[runtime->gesture];
			}
		} else {
			if (runtime->view_id >= 0) {
				if (scroll_off >= GESTURE_MOVE_THRESHOLD) {
					gesture_fixed(runtime);
					animation_type = slidein_anim[opposite_gestures[runtime->gesture]];
				} else {
					animation_type = slideout_anim[opposite_gestures[runtime->gesture]];
				}
			} else {
				runtime->view_id = runtime->related_view_id;
				runtime->related_view_id = -1;
				animation_type = slideback_anim[runtime->gesture];
			}
		}
	} else {
		switch (runtime->gesture) {
		case GESTURE_DROP_DOWN:
			scroll_off = runtime->types.pointer.last_point.y;
			break;
		case GESTURE_DROP_UP:
			scroll_off = y_res - runtime->types.pointer.last_point.y;
			break;
		case GESTURE_DROP_RIGHT:
			scroll_off = runtime->types.pointer.last_point.x;
			break;
		case GESTURE_DROP_LEFT:
		default:
			scroll_off = x_res - runtime->types.pointer.last_point.x;
			break;
		}

		if (runtime->types.pointer.scroll_to_screen) {
			if (scroll_off >= GESTURE_START_THRESHOLD) {
				animation_type = slidein_anim[runtime->gesture];
			} else {
				animation_type = slideout_anim[runtime->gesture];
			}
		} else {
			animation_type = slideout_continue_anim[runtime->gesture];
		}
	}

	SYS_LOG_INF("gesture %d, view %d, related_view %d, scroll_off %d, point (%d %d), anim %d\n",
		runtime->gesture, runtime->view_id, runtime->related_view_id,
		runtime->types.pointer.last_scroll_off, runtime->types.pointer.last_point.x,
		runtime->types.pointer.last_point.y, animation_type);

	if (runtime->view_id >= 0) {
		ui_view_anim_cfg_t cfg;

		if (runtime->related_view_id >= 0) {
			if (animation_type <= UI_ANIM_SLIDE_IN_LEFT) {
				view_manager_pre_anim_refocus(runtime->view_id);
			} else {
				view_manager_pre_anim_refocus(runtime->related_view_id);
			}
		}

		view_manager_get_slide_animation_points(runtime->view_id, &cfg.start, &cfg.stop, animation_type);

		uint16_t step_x = UI_ABS(cfg.stop.x - cfg.start.x);
		uint16_t step_y = UI_ABS(cfg.stop.y - cfg.start.y);

		/* move 32 pixels per 16 ms*/
		cfg.duration = UI_MAX(step_x, step_y) * 16 / 32;
		cfg.path_cb = NULL;
		cfg.stop_cb = NULL;

		view_manager_slide_animation_start(runtime->view_id, runtime->related_view_id, animation_type, &cfg);
	}
}

static const ui_gesture_callback_t gesture_callback = {
#ifdef CONFIG_UI_CUSTOM_GESTURE_DETECTION
	.detect = gesture_detect,
#else
	.detect = NULL,
#endif
	.scroll_begin = gesture_scroll_begin,
	.scroll = gesture_scroll,
	.scroll_end = gesture_scroll_end,
};

int ui_service_register_gesture_default_callback(void)
{
	return ui_service_register_gesture_callback(&gesture_callback);
}
