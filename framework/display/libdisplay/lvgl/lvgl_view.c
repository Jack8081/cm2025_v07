/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ui_surface.h>
#include <lvgl/lvgl_view.h>
#include <ui_service.h>
#include "lvgl_virtual_display.h"
#include "lvgl_input_dev.h"
#ifdef CONFIG_SIMULATOR
#include <display.h>
#endif

static void * _lvgl_view_init(surface_t * surface)
{
	lv_disp_t *disp = NULL;

	disp = lvgl_virtual_display_create(surface);
	if (disp == NULL) {
		SYS_LOG_ERR("Failed to create lvgl display");
	}

	return disp;
}

static void _lvgl_view_deinit(void * display)
{
	lvgl_virtual_display_destroy(display);
}

static int _lvgl_view_resume(void * display, bool full_invalidate)
{
	lv_disp_t *disp = display;

	disp->driver->refr_paused = 0;
	if (full_invalidate)
		lv_obj_invalidate(lv_disp_get_scr_act(disp));

	return 0;
}

static int _lvgl_view_pause(void * display)
{
	lv_disp_t *disp = display;

	disp->driver->refr_paused = 1;
	return 0;
}

static int _lvgl_view_focus(void * display, bool focus)
{
	if (focus) {
		return lvgl_virtual_display_set_focus(display, false);
	}

	return 0;
}

static int _lvgl_view_refresh(void * display, bool full_refresh)
{
	lv_disp_t *disp = display;

	if (disp->driver->refr_paused == 0) {
		if (full_refresh)
			lv_obj_invalidate(lv_disp_get_scr_act(disp));

		lv_refr_now(disp);
		return 0;
	}

	return -EPERM;
}

static void _lvgl_view_task_handler(void)
{
	lv_timer_handler();
}

static const ui_view_gui_callback_t view_callback = {
	.init = _lvgl_view_init,
	.deinit = _lvgl_view_deinit,
	.resume = _lvgl_view_resume,
	.pause = _lvgl_view_pause,
	.focus = _lvgl_view_focus,
	.refresh = _lvgl_view_refresh,
	.task_handler = _lvgl_view_task_handler,
};

int lvgl_view_system_init(void)
{
	uint32_t preferred_format;

#if defined(CONFIG_LV_COLOR_DEPTH_32)
	preferred_format = PIXEL_FORMAT_ARGB_8888;
#elif defined(CONFIG_LV_COLOR_DEPTH_16)
	preferred_format = PIXEL_FORMAT_RGB_565;
#else
	return -EINVAL;
#endif

	/* register view callback */
	ui_service_register_gui_view_callback(UI_VIEW_LVGL, preferred_format, &view_callback);

	/* register input callback */
	lvgl_input_pointer_init();

	return 0;
}
