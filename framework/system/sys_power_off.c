/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system power off
 */

#include <os_common_api.h>
#include <app_manager.h>
#include <msg_manager.h>
#include <power_manager.h>
#include <property_manager.h>
#include <esd_manager.h>
#include <tts_manager.h>
#include <string.h>
#include <sys_event.h>
#include <sys_manager.h>
#include <sys_monitor.h>
#include <audio_hal.h>

#ifdef CONFIG_INPUT_MANAGER
#include <input_manager.h>
#endif

#ifdef CONFIG_TTS_MANAGER
#include <tts_manager.h>
#endif

#ifdef CONFIG_BLUETOOTH
#include <bt_manager.h>
#endif

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "sys_monitor"
#endif
#ifndef CONFIG_SIMULATOR
#include <soc.h>
#endif
void system_power_off(void)
{
#ifdef CONFIG_INPUT_MANAGER
	input_manager_lock();
#endif

	app_manager_active_app(APP_ID_ILLEGAL);

#ifdef CONFIG_BT_MANAGER
	bt_manager_deinit();
#endif

	sys_monitor_stop();

#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif

	system_deinit();

#ifdef CONFIG_AUDIO
	hal_aout_close_pa();
#endif

#ifdef CONFIG_SOC_EP_MODE
#ifdef CONFIG_POWER_MANAGER
	if (!power_manager_get_dc5v_status()) {
	#ifdef BOARD_POWER_LOCK_GPIO
		board_power_lock_enable(false);
	#endif
		SYS_LOG_DBG("poweroff");
		sys_pm_poweroff();
	} else {	
		//SYS_LOG_DBG("reboot");
		//sys_pm_reboot(REBOOT_TYPE_GOTO_SYSTEM | REBOOT_REASON_NORMAL);
	}
#else
    SYS_LOG_DBG("poweroff");
    sys_pm_poweroff();
#endif
#else
#ifndef CONFIG_SIMULATOR
	SYS_LOG_INF("poweroff");
	sys_pm_poweroff();
#endif
#endif
}

void system_power_reboot(int reason)
{
#ifdef CONFIG_SOC_EP_MODE
#ifdef CONFIG_POWER_MANAGER

	u32_t bat_vol_saved = power_manager_get_battery_vol()/1000;
	soc_pstore_set(SOC_PSTORE_TAG_CAPACITY, bat_vol_saved);
	soc_pstore_set(SOC_PSTORE_TAG_FLAG_CAP, 1);
	SYS_LOG_INF("bat_vol_saved:	%d\n", bat_vol_saved);
#endif
#endif

#ifdef CONFIG_INPUT_MANAGER
	input_manager_lock();
#endif

#ifdef CONFIG_TTS_MANAGER
	tts_manager_lock();
#endif

	app_manager_active_app(APP_ID_ILLEGAL);

#ifdef CONFIG_BT_MANAGER
	bt_manager_deinit();
#endif

	sys_monitor_stop();

#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif

	system_deinit();

#ifndef CONFIG_SIMULATOR
	sys_pm_reboot(reason);
#endif

}

void system_power_get_reboot_reason(uint16_t *reboot_type, uint8_t *reason)
{
#ifndef CONFIG_SIMULATOR
	union sys_pm_wakeup_src src = {0};

	if (!sys_pm_get_wakeup_source(&src)) {
		if (src.t.alarm) {
			*reboot_type = REBOOT_TYPE_ALARM;
			*reason = 0;
			goto exit;
		}

		if (src.t.watchdog) {
			if (!sys_pm_get_reboot_reason(reboot_type, reason)) {
				*reboot_type = REBOOT_TYPE_SF_RESET;
				goto exit;
			}
			*reboot_type = REBOOT_TYPE_WATCHDOG;
			*reason = 0;
			goto exit;
		}

		if (src.t.long_onoff || src.t.short_onoff) {
			*reboot_type = REBOOT_TYPE_ONOFF_RESET;
			*reason = 0;
			goto exit;
		}
	}

	//sys_pm_clear_wakeup_pending();

exit:
	SYS_LOG_ERR(" reboot_type: 0x%x, reason %d \n", *reboot_type, *reason);
#endif
	return;
}
