/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hotplug manager interface
 */
#include <string.h>
#include <kernel.h>
#include "hotplug_manager.h"
#include "../sys_app_defines.h"

#define LOG_LEVEL LOG_LEVEL_WRN
#include <logging/log.h>
LOG_MODULE_REGISTER(hotplug_manager);

struct app_msg msg;

char __aligned(4) msg_buffer[10 * sizeof(msg)];

struct k_msgq my_msg_que;

static struct k_timer my_timer;

static struct hotplug_manager_context_t hotplug_manager_context;

static struct hotplug_manager_context_t  *hotplug_manager;

int _hotplug_device_register(const struct hotplug_device_t *device)
{
	const struct hotplug_device_t *temp_device = NULL;
	int free_index = -1;

	for (int i = 0; i < MAX_HOTPLUG_DEVICE_NUM; i++) {
		temp_device = hotplug_manager->device[i];
		if (temp_device && temp_device->type == device->type) {
			return -EEXIST;
		} else if (!temp_device) {
			free_index = i;
			break;
		}
	}

	if (free_index != -1 && free_index < MAX_HOTPLUG_DEVICE_NUM) {
		hotplug_manager->device[free_index] = device;
		hotplug_manager->device_num++;
	}

	return 0;
}

int _hotplug_device_unregister(const struct hotplug_device_t *device)
{
	const struct hotplug_device_t *temp_device = NULL;

	for (int i = 0; i < MAX_HOTPLUG_DEVICE_NUM; i++) {
		temp_device = hotplug_manager->device[i];
		if (temp_device && temp_device->type == device->type) {
			hotplug_manager->device[i] = NULL;
			hotplug_manager->device_num--;
		}
	}
	return 0;
}

int _hotplug_event_report(int device_type, int device_state)
{
	struct app_msg msg = {0};

	msg.type = MSG_HOTPLUG_EVENT;
	msg.cmd = device_type;
	msg.value = device_state;
	LOG_WRN("type: %d state: %d\n", device_type, device_state);

	/* send data to consumers */
        while (k_msgq_put(&my_msg_que, &msg, K_NO_WAIT) != 0) {
            /* message queue is full: purge old data & try again */
            k_msgq_purge(&my_msg_que);
        }

	return 0;
}

static void hotplug_manager_work_handle(struct k_work *work)
{
	int state = HOTPLUG_NONE;
	const struct hotplug_device_t *device = NULL;

	for (int i = 0; i < MAX_HOTPLUG_DEVICE_NUM; i++) {
		device = hotplug_manager->device[i];
		if(!device)
			continue;

		if (device->hotplug_detect) {
			state = device->hotplug_detect();
		}

		if (state != HOTPLUG_NONE) {
			if (device->fs_process) {
				if (device->fs_process(state)) {
					continue;
				}
			}
			if (device->get_type) {
				_hotplug_event_report(device->get_type(), state);
			} else {
				_hotplug_event_report(device->type, state);
			}
		}
	}
}

int _hotplug_manager_get_state(int hotplug_device_type)
{
	int state = HOTPLUG_NONE;
	const struct hotplug_device_t *device = NULL;

	for (int i = 0; i < MAX_HOTPLUG_DEVICE_NUM; i++) {
		device = hotplug_manager->device[i];
		if (device->type == hotplug_device_type) {
			state = device->get_state();
			break;
		}
	}

	return state;
}

static struct k_work my_work;

static void _my_expiry_function(struct k_timer *timer)
{
	k_work_submit(&my_work);
}

int _hotplug_manager_init(void)
{
	if (hotplug_manager)
		return -EEXIST;

	hotplug_manager = &hotplug_manager_context;

	memset(hotplug_manager, 0, sizeof(struct hotplug_manager_context_t));

#if defined(CONFIG_USB_DEVICE) || defined(CONFIG_USB_HOST)
	_hotplug_usb_init();
#endif
	/* Monitor */
	k_timer_init(&my_timer, _my_expiry_function, NULL);
	k_timer_start(&my_timer, K_MSEC(1000), K_MSEC(1000));
	k_work_init(&my_work, hotplug_manager_work_handle);

	k_msgq_init(&my_msg_que, msg_buffer, sizeof(msg), 10);

	return 0;
}
