/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UI_SERVICE_INNER_H__
#define __UI_SERVICE_INNER_H__

#include <assert.h>
#include <ui_service.h>
#include "view_manager_inner.h"

#ifdef __cplusplus
extern "C" {
#endif

#define X_ALIGN_SIZE 2
#define Y_ALIGN_SIZE 1

//#define CONFIG_TP_TRAIL_DRAW   1

/* lock gui draw and input */
enum GUI_LOCK_TYPE {
	UISRV_GUI_NONE  = 0,
	UISRV_GUI_DRAW  = (1 << 0),
	UISRV_GUI_INPUT = (1 << 1),
	UISRV_GUI_BOTH  = UISRV_GUI_DRAW | UISRV_GUI_INPUT,
};

/* gesture manager */
void gesture_manager_set_enabled(bool en);
void gesture_manager_set_dir(uint8_t filter);
void gesture_manager_wait_release(void);
void gesture_manager_forbid_draw(bool en);
void gesture_manager_process(input_dev_t * pointer_input_dev);
void gesture_manager_stop_process(input_dev_t * pointer_input_dev, bool anim_off);
int gesture_manager_register_callback(const ui_gesture_callback_t *callback);

int8_t gesture_manager_get_sw_gesture(input_dev_t * pointer_input_dev);

bool gesture_is_scrolling(void);

/* view animation */
int view_animation_start(ui_view_animation_t *animation,
		uint8_t view_id, int16_t last_view_id, ui_point_t *last_view_offset,
		ui_view_anim_cfg_t *cfg, bool is_slide_anim);
void view_animation_process(ui_view_animation_t *animation);
void view_animation_stop(ui_view_animation_t *animation, bool forced);

/* input dispatcher */
int input_dispatcher_init(void);
int input_dispatcher_deinit(void);
void input_dispatcher_read_task(void);
void input_dispatcher_set_gui_wait_release(void);
void input_dispatcher_set_gui_origin(int16_t x, int16_t y);
int input_dispatcher_register_gui_callaback(uint8_t type, const ui_input_gui_callback_t *callback, void *user_data);

static inline input_dev_t * input_dispatcher_get_pointer_dev(void)
{
	return input_manager_get_input_dev(INPUT_DEV_TYPE_POINTER);
}

#ifdef CONFIG_TP_TRAIL_DRAW
void input_dispatcher_draw_trail(int base, uint16_t width, uint16_t height, uint16_t stride, uint8_t bpp);
#endif

/* ui service */
void ui_service_lock_gui(uint8_t type);
void ui_service_unlock_gui(uint8_t type);
bool ui_service_is_draw_locked(void);
bool ui_service_is_input_locked(void);
bool ui_service_send_self_msg(uint8_t view_id, uint8_t msg_id);

static inline void ui_service_set_view_origin(int16_t x, int16_t y)
{
	input_dispatcher_set_gui_origin(x, y);
}

#ifdef __cplusplus
}
#endif

#endif /* __UI_SERVICE_INNER_H__ */
