/*
 * Copyright (c) 2018-2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <zephyr.h>
#include <lvgl.h>
#ifdef CONFIG_LVGL_USE_FILESYSTEM
#include "lvgl_fs.h"
#endif
#ifndef CONFIG_LVGL_USE_VIRTUAL_DISPLAY
#include "lvgl_display.h"
#endif

#define LOG_LEVEL CONFIG_LVGL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lvgl);

#if defined(CONFIG_LV_USE_LOG) && !defined(CONFIG_LV_LOG_PRINTF)
static void lvgl_log(const char *buf)
{
	printk("%s", buf);
}
#endif

static int lvgl_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_LV_USE_LOG) && !defined(CONFIG_LV_LOG_PRINTF)
	lv_log_register_print_cb(lvgl_log);
#endif

	lv_init();

#ifdef CONFIG_LVGL_USE_FILESYSTEM
	lvgl_fs_init();
#endif

#ifndef CONFIG_LVGL_USE_VIRTUAL_DISPLAY
	if (lvgl_display_init()) {
		return -ENODEV;
	}
#endif /* CONFIG_LVGL_USE_VIRTUAL_DISPLAY */

	return 0;
}

SYS_INIT(lvgl_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
