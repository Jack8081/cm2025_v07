/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <display/sw_draw.h>
#include "lvgl_gpu.h"

static void lvgl_sw_blend_cb(lv_disp_drv_t * disp_drv, lv_color_t * dest,
		lv_coord_t dest_w, const void * src, lv_coord_t src_w,
		lv_img_cf_t src_cf, lv_coord_t copy_w, lv_coord_t copy_h)
{
#ifdef CONFIG_LV_COLOR_DEPTH_16
	if (src_cf == LV_IMG_CF_TRUE_COLOR_ALPHA) {
		sw_blend_argb8565_over_rgb565(dest, src, dest_w, src_w, copy_w, copy_h);
	} else if (src_cf == LV_IMG_CF_ARGB_6666) {
		sw_blend_argb6666_over_rgb565(dest, src, dest_w, src_w, copy_w, copy_h);
	} else {
		sw_blend_argb8888_over_rgb565(dest, src, dest_w, src_w, copy_w, copy_h);
	}
#else
	{
		sw_blend_argb8888_over_argb8888(dest, src, dest_w, src_w, copy_w, copy_h);
	}
#endif
}

void set_lvgl_sw_gpu_cb(lv_disp_drv_t *disp_drv)
{
	disp_drv->sw_blend_cb = lvgl_sw_blend_cb;
}
