/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <mem_manager.h>
#include <memory/mem_cache.h>
#include <ui_region.h>
#include <lvgl/lvgl_memory.h>

/* NOTE:
 * 1) depending on chosen color depth buffer may be accessed using uint8_t *,
 * uint16_t * or uint32_t *, therefore buffer needs to be aligned accordingly to
 * prevent unaligned memory accesses.
 * 2) must align each buffer address and size to psram cache line size (32 bytes)
 * if allocated in psram.
 */
#define BUFFER_SIZE UI_ROUND_UP(CONFIG_LVGL_VDB_SIZE * CONFIG_LV_COLOR_DEPTH / 8, 32)
#define NBR_PIXELS_IN_BUFFER (BUFFER_SIZE * 8 / CONFIG_LV_COLOR_DEPTH)

static uint8_t vdb_buf0[BUFFER_SIZE] __aligned(32) __in_section_unique(lvgl.noinit.vdb.0);
#ifdef CONFIG_LVGL_DOUBLE_VDB
static uint8_t vdb_buf1[BUFFER_SIZE] __aligned(32) __in_section_unique(lvgl.noinit.vdb.1);
#endif

int lvgl_allocate_draw_buffers(lv_disp_drv_t *disp_drv)
{
	disp_drv->draw_buf = mem_malloc(sizeof(lv_disp_draw_buf_t));
	if (disp_drv->draw_buf == NULL) {
		return -ENOMEM;
	}

#ifdef CONFIG_LVGL_DOUBLE_VDB
	lv_disp_draw_buf_init(disp_drv->draw_buf, &vdb_buf0, &vdb_buf1, NBR_PIXELS_IN_BUFFER);
#else
	lv_disp_draw_buf_init(disp_drv->draw_buf, &vdb_buf0, NULL, NBR_PIXELS_IN_BUFFER);
#endif

	return 0;
}

int lvgl_free_draw_buffers(lv_disp_drv_t *disp_drv)
{
	mem_free(disp_drv->draw_buf);
	return 0;
}

void lvgl_img_buf_clean_cache(const lv_img_dsc_t *dsc)
{
	uint32_t size = dsc->data_size;

	if (size == 0) {
		size = lv_img_buf_get_img_size(dsc->header.w, dsc->header.h, dsc->header.cf);
	}

	mem_dcache_clean(dsc->data, size);
}

void lvgl_img_buf_invalidate_cache(const lv_img_dsc_t *dsc)
{
	uint32_t size = dsc->data_size;

	if (size == 0) {
		size = lv_img_buf_get_img_size(dsc->header.w, dsc->header.h, dsc->header.cf);
	}

	mem_dcache_invalidate(dsc->data, size);
}
