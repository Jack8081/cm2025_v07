/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <utils/acts_ringbuf.h>
#include <ui_service.h>
#include <lvgl.h>
#include "lvgl_input_dev.h"

#define CONFIG_LVGL_INPUT_POINTER_MSGQ_COUNT 8

static lv_indev_data_t pointer_scan_rbuf_data[CONFIG_LVGL_INPUT_POINTER_MSGQ_COUNT];
static ACTS_RINGBUF_DEFINE(pointer_scan_rbuf, pointer_scan_rbuf_data, sizeof(pointer_scan_rbuf_data));

static int _lvgl_pointer_put(const input_dev_data_t *data, void *_indev)
{
	lv_indev_t *indev = _indev;
	lv_indev_data_t indev_data = {
		.point.x = data->point.x,
		.point.y = data->point.y,
		.state = data->state,
	};

	if (indev->driver->disp == NULL) {
		return 0;
	}

	if (acts_ringbuf_put(&pointer_scan_rbuf, &indev_data, sizeof(indev_data)) != sizeof(indev_data)) {
		SYS_LOG_WRN("Could put input data into queue");
		return -ENOBUFS;
	}

	return 0;
}

static void _lvgl_pointer_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
	static lv_indev_data_t prev = {
		.point.x = 0,
		.point.y = 0,
		.state = LV_INDEV_STATE_REL,
	};

	lv_indev_data_t curr;

	if (acts_ringbuf_get(&pointer_scan_rbuf, &curr, sizeof(curr)) == sizeof(curr)) {
		prev = curr;
	}

	*data = prev;
	data->continue_reading = (acts_ringbuf_is_empty(&pointer_scan_rbuf) == 0);
}

static void _lvgl_indev_enable(bool en, void *indev)
{
	lv_indev_enable((lv_indev_t *)indev, en);
}

static void _lvgl_indev_wait_release(void *indev)
{
	lv_indev_wait_release_clearing_state((lv_indev_t *)indev);
}

static const ui_input_gui_callback_t pointer_callback = {
	.enable = _lvgl_indev_enable,
	.wait_release = _lvgl_indev_wait_release,
	.put_data = _lvgl_pointer_put,
};

int lvgl_input_pointer_init(void)
{
	static lv_indev_drv_t pointer_indev_drv;

	lv_indev_t * indev = NULL;

	lv_indev_drv_init(&pointer_indev_drv);
	pointer_indev_drv.type = LV_INDEV_TYPE_POINTER;
	pointer_indev_drv.read_cb = _lvgl_pointer_read;

	indev = lv_indev_drv_register(&pointer_indev_drv);
	if (indev == NULL) {
		SYS_LOG_ERR("Failed to register input device.");
		return -EPERM;
	}

	if (ui_service_register_gui_input_callback(INPUT_DEV_TYPE_POINTER, &pointer_callback, indev)) {
		SYS_LOG_ERR("Failed to register input device to ui service.");
		return -EPERM;
	}

	return 0;
}
