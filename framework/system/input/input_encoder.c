/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file input manager interface
 */


#include <os_common_api.h>
#include <kernel.h>
#include <device.h>
#include <sys_event.h>
#include <board_cfg.h>
#include <drivers/input/input_dev.h>
#include <input_manager_type.h>

static struct k_delayed_work encoder_report_work;
static u8_t encoder_keycode;

static void encoder_report_work_handle(struct k_work *work)
{
	sys_event_report_input(encoder_keycode|KEY_TYPE_SHORT_UP);
}

static void encoder_callback(struct device *dev, struct input_value *val)
{
	if (val->keypad.type != EV_KEY) {
		SYS_LOG_ERR("input type %d not support\n", val->keypad.type);
		return;
	}

	encoder_keycode = val->keypad.code;

	os_delayed_work_submit(&encoder_report_work, 0);
}

int input_encoder_device_init(void)
{
	const struct device *input_dev = device_get_binding(CONFIG_ROTARY_ENCODER_NAME);
	if (!input_dev) {
		printk("encoder device open failed\n");
		return -ENODEV;
	}

	os_delayed_work_init(&encoder_report_work, encoder_report_work_handle);

	input_dev_register_notify(input_dev, encoder_callback);
	input_dev_enable(input_dev);

	return 0;
}
