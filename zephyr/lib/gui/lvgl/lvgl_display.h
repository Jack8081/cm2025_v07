/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_GUI_LVGL_LVGL_DISPLAY_H_
#define ZEPHYR_LIB_GUI_LVGL_LVGL_DISPLAY_H_

#include <zephyr.h>
#include <board_cfg.h>
#include <drivers/display.h>
#include <drivers/display/display_graphics.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lvgl_disp_user_data {
	const struct device *disp_dev;    /* display device */
	struct display_callback callback; /* display callback to register */
	struct k_sem complete_sem;        /* display complete wait sem */

	bool new_frame; /* indicate first vdb of the frame */
} lvgl_disp_user_data_t;

static inline
const struct device *lvgl_get_display_dev(lv_disp_drv_t *disp_drv)
{
	lvgl_disp_user_data_t *user_data = disp_drv->user_data;

	return user_data->disp_dev;
}

int lvgl_display_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_GUI_LVGL_LVGL_DISPLAY_H */
