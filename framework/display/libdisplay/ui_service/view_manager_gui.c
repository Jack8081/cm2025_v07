/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <os_common_api.h>
#include <memory/mem_cache.h>
#include <ui_surface.h>
#include <display/ui_memsetcpy.h>
#include <display/display_composer.h>
#include "ui_service_inner.h"

static uint8_t gui_type;
static uint32_t gui_preferred_format;
static const ui_view_gui_callback_t *gui_callback;

int view_manager_register_gui_callback(uint8_t type,
		uint32_t preferred_pixel_format, const ui_view_gui_callback_t *callback)
{
	if (type >= NUM_VIEW_TYPES || callback == NULL) {
		return -EINVAL;
	}

	if (gui_callback) {
		return -EPERM;
	}

	gui_type = type;
	gui_callback = callback;
	gui_preferred_format = preferred_pixel_format;

	return 0;
}

void view_manager_gui_task_handler(void)
{
	os_strace_void(SYS_TRACE_ID_GUI_TASK);

	if (gui_callback) {
		gui_callback->task_handler();
	}

	os_strace_end_call(SYS_TRACE_ID_GUI_TASK);
}

static void _surface_post_handler(uint32_t event, void *data, void *user_data)
{
	const surface_post_data_t *post_data = data;
	ui_view_context_t *view = user_data;

	if (event == SURFACE_EVT_POST_START) {
		view_set_dirty_region(view, post_data->area, post_data->flags);
	}
}

static uint8_t _decide_surface_create_flags(void)
{
	ui_region_t region = { .x1 = 2, .y1 = 2, .x2 = 3, .y2 = 3, };

	display_composer_round(&region);

	/* Test whether display requires full-screen refreshing. If yes,
	 * the display should run in sync-mode.
	 */
	if (region.x1 == 0 && region.y1 == 0 &&
		region.x2 == view_manager_get_disp_xres() - 1 &&
		region.y2 == view_manager_get_disp_yres() - 1) {
		return 0;
	}

	return SURFACE_POST_IN_SYNC_MODE;
}

int view_gui_init(ui_view_context_t *view, uint8_t create_flags)
{
	view_data_t *data = &view->data;
	uint8_t mode;
	int res = 0;

	if (view->entry->type != gui_type || gui_callback == NULL) {
		SYS_LOG_ERR("view type %d not registered\n", view->entry->type);
		return -EINVAL;
	}

	if (view->entry->width <= 0 || view->entry->height <= 0) {
		return -EINVAL;
	}

#ifdef CONFIG_SURFACE_ZERO_BUFFER
	mode = SURFACE_SWAP_ZERO_BUF;
#else
	if (create_flags & UI_CREATE_FLAG_SINGLE_BUF) {
		mode = SURFACE_SWAP_SINGLE_BUF;
	} else if (create_flags & UI_CREATE_FLAG_ZERO_BUF) {
		mode = SURFACE_SWAP_ZERO_BUF;
	} else {
		mode = SURFACE_SWAP_DEFAULT;
	}
#endif

	data->surface = surface_create(
			view->entry->width, view->entry->height, gui_preferred_format,
			(mode == SURFACE_SWAP_ZERO_BUF) ? 0 : 1, mode,
			_decide_surface_create_flags());
	if (!data->surface) {
		return -ENOMEM;
	}

	surface_register_callback(data->surface, SURFACE_CB_POST, _surface_post_handler, view);

	os_strace_u32(SYS_TRACE_ID_GUI_INIT, view->entry->id);

	data->display = gui_callback->init(data->surface);
	if (data->display == NULL) {
		SYS_LOG_ERR("Failed to init gui\n");
		surface_destroy(data->surface);
		data->surface = NULL;
		res = -ENOMEM;
	}

	os_strace_end_call_u32(SYS_TRACE_ID_GUI_INIT, (uint32_t)data->display);

	return res;
}

void view_gui_deinit(ui_view_context_t *view)
{
	view_data_t *data = &view->data;

	os_strace_u32x2(SYS_TRACE_ID_GUI_DEINIT, view->entry->id, (uint32_t)data->display);

	gui_callback->deinit(data->display);
	data->display = NULL;

	os_strace_end_call(SYS_TRACE_ID_GUI_DEINIT);

	if (data->surface) {
		surface_destroy(data->surface);
		data->surface = NULL;
	}
}

void view_gui_clear_begin(ui_view_context_t *view)
{
	view_data_t *data = &view->data;

	os_strace_u32(SYS_TRACE_ID_GUI_CLEAR, view->entry->id);

	/* FIXME: Skip long view */
	if (surface_get_width(data->surface) <= view_manager_get_disp_xres() &&
		surface_get_height(data->surface) <= view_manager_get_disp_yres()) {
		graphic_buffer_t *buf = surface_get_frontbuffer(data->surface);
		void *vaddr = (void *)graphic_buffer_get_bufptr(buf, 0, 0);

		ui_memset(mem_addr_to_uncache(vaddr), 0,
			graphic_buffer_get_bytes_per_line(buf) * graphic_buffer_get_height(buf));
	}
}

void view_gui_clear_end(ui_view_context_t *view)
{
	view_data_t *data = &view->data;

	/* FIXME: Skip long view */
	if (surface_get_width(data->surface) <= view_manager_get_disp_xres() &&
		surface_get_height(data->surface) <= view_manager_get_disp_yres()) {
		ui_memsetcpy_wait_finish(-1);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_GUI_CLEAR, view->entry->id);
}

int view_gui_set_focus(ui_view_context_t *view)
{
	return gui_callback->focus(view->data.display, true);
}

void view_gui_refresh(ui_view_context_t *view)
{
	gui_callback->refresh(view->data.display, false);
}

void view_gui_pause_draw(ui_view_context_t *view)
{
	gui_callback->pause(view->data.display);
}

void view_gui_resume_draw(ui_view_context_t *view, bool force_redraw)
{
	gui_callback->resume(view->data.display, force_redraw);
}

void view_gui_post_cleanup(void *surface)
{
	surface_complete_one_post(surface);
}
