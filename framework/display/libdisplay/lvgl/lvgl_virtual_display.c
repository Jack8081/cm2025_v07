/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <os_common_api.h>
#include <mem_manager.h>
#include <memory/mem_cache.h>
#include <display/ui_memsetcpy.h>
#include <drivers/display.h>
#ifdef CONFIG_TRACING
#include <tracing/tracing.h>
#endif
#include <view_manager.h>
#include <lvgl.h>
#include <lvgl/lvgl_memory.h>
#include "lvgl_virtual_display.h"
#include "lvgl_gpu.h"

//#define CONFIG_DO_END_DRAW_IN_ISR

typedef struct lvgl_virtual_disp_data {
	surface_t *surface;
	os_sem flush_sem;

	uint8_t flush_idx;
	uint8_t flush_flag;

#ifdef CONFIG_LV_USE_GPU
	ui_region_t delayed_flush_area;
	const lv_color_t *delayed_flush_buf;
#ifndef CONFIG_DO_END_DRAW_IN_ISR
	os_work delayed_flush_work;
#endif
#endif

#ifdef CONFIG_TRACING
	uint32_t frame_cnt;
#endif
} lvgl_virtual_disp_data_t;

/* active display with indev attached to */
static lv_disp_t *act_disp = NULL;

static void lvgl_virtual_prepare_cb(lv_disp_drv_t *disp_drv,
		const lv_area_t *area)
{
	lvgl_virtual_disp_data_t *data = disp_drv->user_data;

	data->flush_flag = (data->flush_idx == 0) ? SURFACE_FIRST_DRAW : 0;

#ifdef CONFIG_TRACING
	ui_view_context_t *view = data->surface->user_data[SURFACE_CB_POST];

	os_strace_u32x7(SYS_TRACE_ID_VIEW_DRAW, view->entry->id, data->frame_cnt,
			data->flush_idx, area->x1, area->y1, area->x2, area->y2);
#endif /* CONFIG_TRACING */
}

#ifdef CONFIG_LV_USE_GPU
#ifdef CONFIG_DO_END_DRAW_IN_ISR
static void lvgl_virtual_gpu_delayed_flush_start_cb(int status, void *user_data)
{
	lvgl_virtual_disp_data_t *data = user_data;

	surface_end_draw(data->surface, &data->delayed_flush_area, data->delayed_flush_buf);
}
#else
static void lvgl_virtual_delayed_flush_work_handler(os_work *work)
{
	lvgl_virtual_disp_data_t *data = CONTAINER_OF(work, lvgl_virtual_disp_data_t, delayed_flush_work);

	surface_end_draw(data->surface, &data->delayed_flush_area, data->delayed_flush_buf);
}

static void lvgl_virtual_gpu_delayed_flush_start_cb(int status, void *user_data)
{
	lvgl_virtual_disp_data_t *data = user_data;

	os_work_q *queue = os_get_display_work_queue();

	if (queue) {
		os_work_submit_to_queue(queue, &data->delayed_flush_work);
	} else {
		os_work_submit(&data->delayed_flush_work);
	}
}
#endif /* CONFIG_DO_END_DRAW_IN_ISR */
#endif /* CONFIG_LV_USE_GPU */

static void lvgl_virtual_refresh_start_cb(lv_disp_drv_t *disp_drv)
{
	lvgl_virtual_disp_data_t *data = disp_drv->user_data;

#ifdef CONFIG_LV_USE_GPU
	/* must make sure previous surface_end_draw() finished */
	lvgl_gpu_wait_marker_finish();
#endif

	surface_begin_frame(data->surface);
}

static void lvgl_virtual_flush_cb(lv_disp_drv_t *disp_drv,
		const lv_area_t *area, lv_color_t *color_p)
{
	lvgl_virtual_disp_data_t *data = disp_drv->user_data;
	lv_coord_t color_w = lv_area_get_width(area);
	lv_coord_t color_h = lv_area_get_height(area);
	graphic_buffer_t *draw_buf = NULL;
	lv_color_t *buf_p = NULL;
	int res = -EINVAL;

#ifdef CONFIG_LV_USE_GPU
	ui_region_t *rect = &data->delayed_flush_area;
#else
	ui_region_t flush_rect;
	ui_region_t *rect = &flush_rect;
#endif

	if (lv_disp_flush_is_last(disp_drv)) {
#ifdef CONFIG_TRACING
		data->frame_cnt++;
#endif
		data->flush_idx = 0;
		data->flush_flag |= SURFACE_LAST_DRAW;
	} else {
		data->flush_idx++;
	}

#ifdef CONFIG_LV_USE_GPU
	/* always clean dcache to make cache coherent */
	if (disp_drv->clean_dcache_cb) {
		disp_drv->clean_dcache_cb(disp_drv);
	}

	/* must make sure previous surface_end_draw() finished */
	lvgl_gpu_wait_marker_finish();
#endif

	ui_region_set(rect, area->x1, area->y1, area->x2, area->y2);

	surface_begin_draw(data->surface, data->flush_flag, &draw_buf);
	if (draw_buf)
		buf_p = (lv_color_t *)graphic_buffer_get_bufptr(draw_buf, area->x1, area->y1);

#ifdef CONFIG_LV_USE_GPU
	if (buf_p && disp_drv->gpu_copy_cb) {
		/* FIXME: no need to invalidate the surface buffer cache ? */
		res = lvgl_gpu_copy(buf_p, graphic_buffer_get_stride(draw_buf),
				color_p, color_w, LV_IMG_CF_TRUE_COLOR, color_w, color_h);
	} else {
		res = buf_p ? -1 : 0;
	}

	if (res >= 0) {
		data->delayed_flush_buf = color_p;
		lvgl_gpu_insert_marker(lvgl_virtual_gpu_delayed_flush_start_cb, data);
		return;
	} else {
		disp_drv->gpu_wait_cb(disp_drv);
		disp_drv->using_gpu = 1; /* avoid unnecessary cache clean */
	}
#endif /* CONFIG_LV_USE_GPU */

	if (buf_p) {
		buf_p = mem_addr_to_uncache(buf_p);

		/* fallback to cpu copy */
		if (color_w == graphic_buffer_get_stride(draw_buf)) {
			ui_memcpy(buf_p, mem_addr_to_uncache(color_p),
					color_w * sizeof(lv_color_t) * color_h);
			ui_memsetcpy_wait_finish(5000);
		} else {
			for (int j = color_h; j > 0; j--) {
				memcpy(buf_p, color_p, color_w * sizeof(lv_color_t));
				buf_p += graphic_buffer_get_stride(draw_buf);
				color_p += color_w;
			}
		}

		mem_writebuf_clean_all();
	}

	surface_end_draw(data->surface, rect, color_p);
}

static void lvgl_virtual_rounder_cb(lv_disp_drv_t * disp_drv, lv_area_t * area)
{
#if defined(CONFIG_LV_USE_GPU) && defined(CONFIG_LV_COLOR_DEPTH_16)
	/* display engine (even start and columns) and x constrain by some lcd driver IC constrain */
	area->x1 &= ~0x1;
	area->x2 |= 0x1;
#endif

	/* y constrain by some lcd driver IC */
	area->y1 &= ~0x1;
	area->y2 |= 0x1;
}

static void surface_draw_handler(uint32_t event, void *data, void *user_data)
{
	lv_disp_t *disp = user_data;
	lvgl_virtual_disp_data_t *disp_data = disp->driver->user_data;

	if (event == SURFACE_EVT_DRAW_READY) {
		lv_disp_flush_ready(disp->driver);
		os_sem_give(&disp_data->flush_sem);
	} else if (event == SURFACE_EVT_DRAW_COVER_CHECK) {
		surface_cover_check_data_t *cover_check = data;

		SYS_LOG_DBG("inv area (%d %d %d %d), old area (%d %d %d %d)\n", disp->inv_areas[0].x1, disp->inv_areas[0].y1, disp->inv_areas[0].x2, disp->inv_areas[0].y2,
					cover_check->area->x1, cover_check->area->y1, cover_check->area->x2, cover_check->area->y2);

		if (disp->inv_p == 1 &&
			  disp->inv_areas[0].x1 <= cover_check->area->x1 &&
			  disp->inv_areas[0].y1 <= cover_check->area->y1 &&
			  disp->inv_areas[0].x2 >= cover_check->area->x2 &&
			  disp->inv_areas[0].y2 >= cover_check->area->y2) {
			  cover_check->covered = true;
		} else {
			  cover_check->covered = false;
		}

	}
}

static void lvgl_virtual_wait_cb(lv_disp_drv_t *disp_drv)
{
	lvgl_virtual_disp_data_t *data = disp_drv->user_data;

	os_strace_void(SYS_TRACE_ID_GUI_WAIT);
	os_sem_take(&data->flush_sem, OS_FOREVER);
	os_strace_end_call(SYS_TRACE_ID_GUI_WAIT);
}

lv_disp_t *lvgl_virtual_display_create(surface_t *surface)
{
	lv_disp_t *disp = NULL;
	lv_disp_drv_t *disp_drv;

#ifdef CONFIG_LVGL_COLOR_DEPTH_32
	if (surface_get_pixel_format(surface) != PIXEL_FORMAT_ARGB_8888) {
		SYS_LOG_ERR("only support ARGB_8888");
		return NULL;
	}
#else
	if (surface_get_pixel_format(surface) != PIXEL_FORMAT_RGB_565) {
		SYS_LOG_ERR("only support RGB_565");
		return NULL;
	}
#endif

	disp_drv = mem_malloc(sizeof(*disp_drv));
	if (disp_drv == NULL)
		return NULL;

	lv_disp_drv_init(disp_drv);

	if (lvgl_allocate_draw_buffers(disp_drv)) {
		SYS_LOG_ERR("Failed to allocate memory to store rendering buffers");
		mem_free(disp_drv);
		return NULL;
	}

	lvgl_virtual_disp_data_t *disp_data = mem_malloc(sizeof(lvgl_virtual_disp_data_t));
	if (disp_data == NULL) {
		SYS_LOG_ERR("Failed to allocate memory to store user data");
		mem_free(disp_drv->draw_buf);
		mem_free(disp_drv);
		return NULL;
	}

	memset(disp_data, 0, sizeof(*disp_data));
	os_sem_init(&disp_data->flush_sem, 0, 2);
	disp_data->surface = surface;

	disp_drv->user_data = disp_data;
	disp_drv->hor_res = surface_get_width(surface);
	disp_drv->ver_res = surface_get_height(surface);

#ifdef CONFIG_LV_USE_GPU
	if (set_lvgl_gpu_cb(disp_drv)) {
		SYS_LOG_WRN("GPU not supported.");
	} else {
#ifndef CONFIG_DO_END_DRAW_IN_ISR
		os_work_init(&disp_data->delayed_flush_work, lvgl_virtual_delayed_flush_work_handler);
#endif
	}
#endif

#ifndef CONFIG_SIMULATOR
	set_lvgl_sw_gpu_cb(disp_drv);
#endif

	disp_drv->rounder_cb = lvgl_virtual_rounder_cb;
	disp_drv->flush_cb = lvgl_virtual_flush_cb;
	disp_drv->wait_cb = lvgl_virtual_wait_cb;
	disp_drv->prepare_cb = lvgl_virtual_prepare_cb;
	disp_drv->refresh_start_cb = lvgl_virtual_refresh_start_cb,
#ifdef CONFIG_LV_USE_GPU
	disp_drv->using_gpu = 1;
#endif

	disp = lv_disp_drv_register(disp_drv);
	if (disp == NULL) {
		SYS_LOG_ERR("Failed to register display.");
		mem_free(disp_drv->draw_buf);
		mem_free(disp_drv->user_data);
		mem_free(disp_drv);
		return NULL;
	}

	lv_disp_set_bg_color(disp, lv_color_black());
	lv_disp_set_bg_opa(disp, LV_OPA_COVER);

	/* Skip drawing the empty frame */
	lv_timer_pause(disp->refr_timer);

	surface_register_callback(surface, SURFACE_CB_DRAW, surface_draw_handler, disp);

	SYS_LOG_DBG("disp %p created\n", disp);
	return disp;
}

void lvgl_virtual_display_destroy(lv_disp_t *disp)
{
	lv_disp_drv_t *disp_drv = disp->driver;
	lvgl_virtual_disp_data_t *disp_data = disp_drv->user_data;

	while (disp_drv->draw_buf->flushing) {
		lvgl_virtual_wait_cb(disp_drv);
	}

	if (act_disp == disp) {
		lvgl_virtual_display_set_focus(NULL, true);
	}

	lv_disp_remove(disp);

	lvgl_free_draw_buffers(disp_drv);
	mem_free(disp_drv);
	mem_free(disp_data);

	SYS_LOG_DBG("disp %p destroyed\n", disp);
}

int lvgl_virtual_display_set_focus(lv_disp_t *disp, bool reset_indev)
{
	if (disp == act_disp) {
		return 0;
	}

	if (disp == NULL) {
		SYS_LOG_WRN("no active display\n");
	}

	/* Retach the input devices */
	lv_indev_t *indev = lv_indev_get_next(NULL);
	while (indev) {
		 /* Reset the indev when focus changed */
		if (reset_indev) {
			lv_indev_reset(indev, NULL);
		} else {
			lv_indev_wait_release(indev);
		}

		indev->driver->disp = disp;
		indev = lv_indev_get_next(indev);

		SYS_LOG_DBG("indev %p attached to disp %p\n", indev, disp);
	}

	/* Set default display */
	lv_disp_set_default(disp);

	act_disp = disp;
	return 0;
}
