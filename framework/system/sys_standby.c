/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system standby
 */

#include <os_common_api.h>
#include <app_manager.h>
#include <msg_manager.h>
#include <power_manager.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#include <ui_mem.h>
#endif
#include <property_manager.h>
#include <esd_manager.h>
#include <tts_manager.h>
#include <string.h>

#include <sys_event.h>
#include <sys_manager.h>
#include <mem_manager.h>
#include <sys_monitor.h>
#include <srv_manager.h>
#include <sys_wakelock.h>
#ifdef CONFIG_RES_MANAGER
#include <res_manager_api.h>
#endif
#ifndef CONFIG_SIMULATOR
#include <kernel.h>
#include <pm/pm.h>
#include <device.h>
#include <soc.h>
#endif
//#include <input_dev.h>
#include <audio_hal.h>

#ifdef CONFIG_BLUETOOTH
#include "bt_manager.h"
#endif
#include <drivers/hrtimer.h>
#include <watchdog_hal.h>

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "sys_standby"
#endif

#define STANDBY_MIN_TIME_SEC (5)

enum STANDBY_STATE_E {
	STANDBY_NORMAL,
	STANDBY_S1,
	STANDBY_S2,
	STANDBY_S3,
};

enum STANDBY_MODE_E {
	STANDBY_SLEEP,
	STANDBY_LIGHT_SLEEP,
	STANDBY_DEEP_SLEEP,
};

struct standby_context_t {
	uint32_t	auto_standby_time;
	uint32_t	auto_powerdown_time;
	uint8_t		standby_state;
	uint8_t		standby_mode;
	uint8_t     auto_powerdown;
	struct hrtimer auto_powerdown_timer;
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	uint8_t	dvfs_level;
#endif
	uint32_t   wakeup_timestamp;
	uint32_t   wakeup_flag:1;
	uint32_t   force_standby:1;
	void * last_app;
	system_standby_notifier_t notifier;
};

struct standby_context_t *standby_context = NULL;

extern void thread_usleep(uint32_t usec);
extern int usb_hotplug_suspend(void);
extern int usb_hotplug_resume(void);

static int _sys_standby_check_auto_powerdown(void)
{
	if (standby_context->auto_powerdown)
		return 1;

	if ((sys_wakelocks_get_free_time(PARTIAL_WAKE_LOCK) > standby_context->auto_powerdown_time) && 
		(sys_wakelocks_get_free_time(FULL_WAKE_LOCK) > standby_context->auto_powerdown_time))
	{
		struct app_msg msg = {0};

		standby_context->auto_powerdown = 1;

		SYS_LOG_INF("auto powerdown");
		msg.type = MSG_POWER_OFF;
		msg.value = 1; // auto powerdown
		send_async_msg("main", &msg);

		return 1;
	}

	return 0;
}

static void _sys_standby_powerdown_timer_handler(struct hrtimer *ttimer, void *expiry_fn_arg)
{

	standby_context->auto_powerdown = 1;

}

static int _sys_standby_enter_s1(void)
{
	standby_context->standby_state = STANDBY_S1;

#ifndef CONFIG_SIMULATOR
	pm_early_suspend();
#endif

	/**set ble state stanyby*/
	SYS_LOG_INF("Enter S1");
	return 0;
}

static int _sys_standby_exit_s1(void)
{
	standby_context->standby_state = STANDBY_NORMAL;

	standby_context->wakeup_timestamp = os_uptime_get_32();

#ifndef CONFIG_SIMULATOR
	pm_late_resume();
#endif

	/**clear ble state stanyby*/
	SYS_LOG_INF("Exit S1");

	return 0;
}

static int _sys_standby_enter_s2(void)
{
	standby_context->standby_state = STANDBY_S2;

	if (standby_context->standby_mode == STANDBY_DEEP_SLEEP) {
		if (standby_context->notifier)
			standby_context->notifier(MSG_SUSPEND_APP);

		standby_context->last_app = app_manager_get_current_app();
		app_manager_notify_app(standby_context->last_app, MSG_SUSPEND_APP);

		if (sys_wakelocks_check(PARTIAL_WAKE_LOCK)
			 || sys_wakelocks_check(FULL_WAKE_LOCK)) {
			standby_context->standby_state = STANDBY_S1;
		}

		srv_manager_notify_service(NULL, MSG_SUSPEND_APP);
		if (sys_wakelocks_check(PARTIAL_WAKE_LOCK)
			|| sys_wakelocks_check(FULL_WAKE_LOCK)) {
			standby_context->standby_state = STANDBY_S1;
		}
		msg_manager_lock();
	}

	SYS_LOG_INF("Enter S2");
	return 0;
}

static int _sys_standby_exit_s2(void)
{
	standby_context->standby_state = STANDBY_S1;

	/**TODO: add registry notify*/
	//if (standby_context->standby_mode == STANDBY_DEEP_SLEEP) {
	//#if CONFIG_DISPLAY
	//	ui_memory_init();
	//#endif
	//#ifdef CONFIG_RES_MANAGER
	//	res_manager_init();
	//#endif
	//}

	if (standby_context->last_app) {
		msg_manager_unlock();

		srv_manager_notify_service(NULL, MSG_RESUME_APP);
		app_manager_notify_app(standby_context->last_app, MSG_RESUME_APP);
	}

	if (standby_context->notifier)
		standby_context->notifier(MSG_RESUME_APP);

	SYS_LOG_INF("Exit S2");
	return 0;
}

static int _sys_standby_enter_s3(void)
{
	standby_context->standby_state = STANDBY_S3;

	SYS_LOG_INF("Enter S3BT");

	standby_context->wakeup_flag = 0;

	sys_pm_enter_deep_sleep();

	/**wait for wake up*/
	while (!standby_context->wakeup_flag)
		os_sleep(2);

	return 0;
}

static int _sys_standby_process_normal(void)
{
	uint32_t wakelocks = sys_wakelocks_check(FULL_WAKE_LOCK);

	/**have sys wake lock*/
	if (wakelocks) {
		SYS_LOG_DBG("wakelocks: 0x%08x\n", wakelocks);
		return 0;
	}

	if (standby_context->force_standby
		|| sys_wakelocks_get_free_time(FULL_WAKE_LOCK) > standby_context->auto_standby_time)
		_sys_standby_enter_s1();

	return 0;
}

static int _sys_standby_process_s1(void)
{
	uint32_t wakelocks = sys_wakelocks_check(FULL_WAKE_LOCK);
	/**have sys wake lock*/
	if (wakelocks) {
		SYS_LOG_DBG("hold status: 0x%08x\n", wakelocks);
		_sys_standby_exit_s1();
	} else if (sys_wakelocks_get_free_time(FULL_WAKE_LOCK) < standby_context->auto_standby_time) {
		if (!standby_context->force_standby) {
			_sys_standby_exit_s1();
		}
	} else if (sys_wakelocks_get_free_time(PARTIAL_WAKE_LOCK) > standby_context->auto_standby_time) {
		if (!sys_wakelocks_check(PARTIAL_WAKE_LOCK)) {
			_sys_standby_enter_s2();
		}
	} else if (standby_context->force_standby){
		if (!sys_wakelocks_check(PARTIAL_WAKE_LOCK)) {
			_sys_standby_enter_s2();
		}
	}

	return 0;
}

static bool _sys_standby_wakeup_from_s2(void)
{
#if 0
	uint32_t wakelocks = sys_wakelocks_check(FULL_WAKE_LOCK);
	uint32_t pending = sys_pm_get_wakeup_pending();

	if (_sys_standby_check_auto_powerdown()) {
		return true;
	}

	if (pending & STANDBY_VALID_WAKEUP_PD) {
		sys_pm_clear_wakeup_pending();
		SYS_LOG_DBG("wakeup from S2: 0x%x", pending);
		return true;
	}

	if (power_manager_get_dc5v_status()) {
		SYS_LOG_DBG("wakeup from S2 because dc5v \n");
		return true;
	}

	if (wakelocks || sys_wakelocks_get_free_time(FULL_WAKE_LOCK) < standby_context->auto_standby_time) {
		SYS_LOG_DBG("wakelock: 0x%x\n", wakelocks);
		return true;
	}

	if (power_manager_check_is_no_power()) {
		SYS_LOG_INF("NO POWER\n");
		sys_pm_poweroff();
		return true;
	}

	/* FIXME: Here catch a hardware related issue.
	  * There is no wakeup pending of on-off key during s3bt stage,
	  * whereas the pending is happened at s1/s2 stage.
	  */
	if (sys_read32(PMU_ONOFF_KEY) & 0x1) {
		/* wait until ON-OFF key bounce */
		while (sys_read32(PMU_ONOFF_KEY) & 0x1)
			;
		SYS_LOG_INF("wakeup from ON-OFF key");
		return true;
	}
#endif
	return true;
}

static int _sys_standby_process_s2(void)
{
#ifndef CONFIG_SIMULATOR
	SYS_LOG_INF("enter");
	/**clear watchdog */
#ifdef CONFIG_WATCHDOG
	watchdog_clear();
#endif
	standby_context->force_standby = 0;

	if (standby_context->auto_powerdown_time != (-1))
	{
		int auto_powerdown_time = standby_context->auto_powerdown_time - sys_wakelocks_get_free_time(FULL_WAKE_LOCK);

		if (auto_powerdown_time > standby_context->auto_powerdown_time - sys_wakelocks_get_free_time(PARTIAL_WAKE_LOCK))
			auto_powerdown_time = standby_context->auto_powerdown_time - sys_wakelocks_get_free_time(PARTIAL_WAKE_LOCK);
		
		if (auto_powerdown_time > 0)
		{
			SYS_LOG_INF("set timer %d\n", auto_powerdown_time);
			hrtimer_start(&standby_context->auto_powerdown_timer, auto_powerdown_time*1000, 0);
		}
	}

	while(true) {
		if (sys_wakelocks_check(PARTIAL_WAKE_LOCK)
			|| sys_wakelocks_check(FULL_WAKE_LOCK)) {
			_sys_standby_exit_s2();
			break;
		}
		_sys_standby_enter_s3();

		/**have sys wake lock*/
		if (_sys_standby_wakeup_from_s2()) {
			_sys_standby_exit_s2();
			break;
		}
	}

	if (standby_context->auto_powerdown)
	{
		struct app_msg msg = {0};

		SYS_LOG_INF("auto powerdown");

		msg.type = MSG_POWER_OFF;
		msg.value = 1; // auto powerdown
		send_async_msg("main", &msg);
		return true;
	}

	hrtimer_stop(&standby_context->auto_powerdown_timer);

	if (sys_s3_wksrc_get() != SLEEP_WK_SRC_BT 
#ifndef CONFIG_SOC_EP_MODE	
		&&!(soc_get_aod_mode() && sys_s3_wksrc_get() == SLEEP_WK_SRC_RTC)
#endif
	) {
		sys_wake_lock(FULL_WAKE_LOCK);
		_sys_standby_exit_s1();
		sys_wake_unlock(FULL_WAKE_LOCK);
	} else {
		sys_wake_lock(PARTIAL_WAKE_LOCK);
		sys_wake_unlock(PARTIAL_WAKE_LOCK);
	}
#endif
	return 0;
}

static int _sys_standby_work_handle(void)
{
	int ret = _sys_standby_check_auto_powerdown();

	if (ret)
		return ret;

	switch (standby_context->standby_state) {
	case STANDBY_NORMAL:
		ret = _sys_standby_process_normal();
		break;
	case STANDBY_S1:
		ret = _sys_standby_process_s1();
		break;
	case STANDBY_S2:
		ret = _sys_standby_process_s2();
		break;
	}
	return ret;
}

#ifdef CONFIG_AUTO_POWEDOWN_TIME_SEC
static bool _sys_standby_is_auto_powerdown(void)
{
	bool auto_powerdown = true;
	char temp[16];
	int ret;

	memset(temp, 0, sizeof(temp));
	ret = property_get(CFG_AUTO_POWERDOWN, temp, 16);
	if (ret > 0) {
		if (strcmp(temp, "false") == 0) {
			auto_powerdown = false;
		}
	}

	return auto_powerdown;
}
#endif

static void _sys_standby_entry_sleep(enum pm_state state)
{
	SYS_LOG_INF("enter \n");
}
static void _sys_standby_exit_sleep(enum pm_state state)
{
	standby_context->wakeup_flag = 1;
	SYS_LOG_INF("enter \n");
}
#ifndef CONFIG_SIMULATOR
static struct pm_notifier notifier = {
	.state_entry = _sys_standby_entry_sleep,
	.state_exit = _sys_standby_exit_sleep,
};
#endif

static struct standby_context_t global_standby_context;

int sys_standby_init(void)
{
	standby_context = &global_standby_context;

	memset(standby_context, 0, sizeof(struct standby_context_t));

#ifdef CONFIG_AUTO_STANDBY_TIME_SEC
	if (0 == CONFIG_AUTO_STANDBY_TIME_SEC) {
		standby_context->auto_standby_time = (-1);
	} else if (CONFIG_AUTO_STANDBY_TIME_SEC < STANDBY_MIN_TIME_SEC) {
		SYS_LOG_WRN("too small, used default");
		standby_context->auto_standby_time = STANDBY_MIN_TIME_SEC * 1000;
	} else {
		standby_context->auto_standby_time = CONFIG_AUTO_STANDBY_TIME_SEC * 1000;
	}
#else
	standby_context->auto_standby_time = (-1);
#endif

#ifdef CONFIG_AUTO_POWEDOWN_TIME_SEC
	if (_sys_standby_is_auto_powerdown()) {
		standby_context->auto_powerdown_time = CONFIG_AUTO_POWEDOWN_TIME_SEC * 1000;
	} else {
		SYS_LOG_WRN("Disable auto powerdown\n");
		standby_context->auto_powerdown_time = (-1);
	}
#else
	standby_context->auto_powerdown_time = (-1);
#endif

	standby_context->standby_state = STANDBY_NORMAL;
	standby_context->standby_mode = STANDBY_DEEP_SLEEP;

	standby_context->wakeup_timestamp = os_uptime_get_32();

#ifdef CONFIG_SYS_WAKELOCK
	sys_wakelocks_init();
#endif

	if (sys_monitor_add_work(_sys_standby_work_handle)) {
		SYS_LOG_ERR("add work failed\n");
		return -EFAULT;
	}

#ifndef CONFIG_SIMULATOR
	pm_notifier_register(&notifier);
#endif
	hrtimer_init(&standby_context->auto_powerdown_timer, _sys_standby_powerdown_timer_handler, NULL);

	SYS_LOG_INF("standby time : %d", standby_context->auto_standby_time);

	return 0;
}

uint32_t system_wakeup_time(void)
{
	uint32_t wakeup_time = (-1);

	/** no need deal uint32_t overflow */
	if (standby_context->wakeup_timestamp) {
		wakeup_time = os_uptime_get_32() - standby_context->wakeup_timestamp;
	}

	SYS_LOG_INF("wakeup_time %d ms\n", wakeup_time);
	return wakeup_time;
}

uint32_t system_boot_time(void)
{
	return os_uptime_get();
}

void system_set_standby_mode(uint8_t sleep_mode)
{
	if (standby_context) {
		standby_context->standby_mode = sleep_mode;
	}
}

void system_set_autosleep_time(uint32_t timeout)
{
	if (standby_context) {
		standby_context->auto_standby_time = timeout ? timeout * 1000 : (-1);
	}
}

void system_request_fast_standby(void)
{
	if (standby_context) {
		standby_context->force_standby = 1;
	}
}

void system_clear_fast_standby(void)
{
	if (standby_context) {
		standby_context->force_standby = 0;
	}
}

bool system_is_screen_on(void)
{
	bool screen_on = false;
	if (standby_context) {
		screen_on = (standby_context->standby_state == STANDBY_NORMAL);
	}
	return screen_on;
}

void system_set_auto_poweroff_time(uint32_t timeout)
{
	if (standby_context) {
		standby_context->auto_powerdown_time = timeout ? timeout * 1000 : (-1);
	}
}

int system_register_standby_notifier(system_standby_notifier_t notifier)
{
	int ret = -1;
	
	if (standby_context && !standby_context->notifier)
	{
		standby_context->notifier = notifier;
		ret = 0;
	}

	return ret;
}
