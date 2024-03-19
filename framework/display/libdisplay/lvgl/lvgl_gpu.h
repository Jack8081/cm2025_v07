/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_LVGL_GPU_H_
#define FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_LVGL_GPU_H_

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Event callback */
typedef void (*lvgl_gpu_event_cb_t)(int status, void *user_data);

/* Set custom software draw callback */
void set_lvgl_sw_gpu_cb(lv_disp_drv_t *disp_drv);

/* Set gpu callback */
int set_lvgl_gpu_cb(lv_disp_drv_t *disp_drv);

/* GPU copy for internal use */
int lvgl_gpu_copy(lv_color_t * dest, lv_coord_t dest_w, const void * src,
		lv_coord_t src_w, lv_img_cf_t src_cf, lv_coord_t copy_w, lv_coord_t copy_h);

/* Insert a marker */
void lvgl_gpu_insert_marker(lvgl_gpu_event_cb_t event_cb, void *user_data);

/* Wait marker finish */
void lvgl_gpu_wait_marker_finish(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_LVGL_GPU_H_ */
