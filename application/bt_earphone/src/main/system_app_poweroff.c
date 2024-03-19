/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app poweroff
 */

#include "system_app.h"
#include "power_manager.h"
#include "tts_manager.h"
#include "bt_manager.h"
#include "sys_wakelock.h"
#include "led_display.h"
#include <dvfs.h>

//void system_app_proc_poweroff(os_work *work)
void system_app_do_poweroff(int result)
{
    int onoff_key_stable_up = 0;
    
    SYS_LOG_INF("bt poweroff result %d", result);
    if (result != BTMGR_LREQ_POFF_RESULT_OK && result != BTMGR_RREQ_SYNC_POFF_RESULT_OK)
    {
        SYS_LOG_ERR("bt poweroff failed %d", result);
        return;
    }

    /* 等待 ONOFF 按键抬起
    */
    while (1)
    {
        int  state = system_app_check_onnff_key_state();
        if (state == KEY_VALUE_UP)
        {
            onoff_key_stable_up += 1;
            if (onoff_key_stable_up >= 5)
                break;
        } else {
            onoff_key_stable_up = 0;
        }
        os_sleep(10);
    }

    system_power_off();

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "manager");
#endif

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    const struct device* gpio_dev = device_get_binding("GPIOA");
    gpio_pin_configure(gpio_dev, 19, GPIO_OUTPUT_INACTIVE);
    k_usleep(300000);
#endif

#ifdef CONFIG_POWER_MANAGER
    system_app_start_poweroff_charge();
#endif

}

void system_app_enter_poweroff(int tws_poweroff)
{
    system_app_context_t*  manager = system_app_get_context();
    system_configs_t*  sys_configs = system_get_configs();
    uint8_t single_poweroff;

    if (manager->sys_status.in_power_off_stage)
    {
        return;
    }

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "manager");
#endif

	
    //os_delayed_work_init(&manager->poweroff_work, system_app_proc_poweroff);

    SYS_LOG_INF("");

    manager->sys_status.in_power_off_stage = 1;

    ui_key_filter(true);


    /* 延迟处理关机流程?
     * 等待其它事件消息处理完成后再进行关机,
     * 避免关机过程中处理一些以前的事件消息导致状态异常问题?
     */

    /* TWS 同步关机?
     */
    if (sys_configs->cfg_tws_sync.Sync_Mode & TWS_SYNC_POWER_OFF)
    {
        single_poweroff = 0;
    }
    else
    {
        single_poweroff = 1;
    }

    /* 电量不足自动关机时不同步关机?
     */
    if (manager->sys_status.bat_too_low_powoff)
    {
        single_poweroff = 1;
    }

    /* 定时自动关机时同步关机?
     */
    if (manager->is_auto_timer_powoff)
    {
        single_poweroff = 0;
    }
    
    if (!tws_poweroff)
    {
        if(!single_poweroff)
        {
            bt_manager_tws_send_message(TWS_USER_APP_EVENT, TWS_EVENT_POWER_OFF, 0, 0);
        }
        
        bt_manager_proc_poweroff(single_poweroff);
    }

    char *fg_app = app_manager_get_current_app();
    if (fg_app)
    {
        if (strcmp(APP_ID_MAIN, fg_app) != 0)
        {
            app_manager_active_app(APP_ID_ILLEGAL);
        }
    }

    led_notify_stop_display(LED_LAYER_BT);
    led_layer_delete(LED_LAYER_BT);
    
    led_notify_stop_display(LED_LAYER_BT_MANAGER);
    led_layer_delete(LED_LAYER_BT_MANAGER);

    system_do_event_notify(UI_EVENT_POWER_OFF, false, NULL); //提示音，显示
}

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
/* Lite the process of event notify */
void system_app_enter_poweroff_lite(int tws_poweroff)
{
    char    *fg_app = NULL;
    uint8_t single_poweroff;
    system_app_context_t*  manager = system_app_get_context();
    system_configs_t*  sys_configs = system_get_configs();

    if (manager->sys_status.in_power_off_stage)
    {
        return;
    }

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "manager");
#endif

    SYS_LOG_INF("");

    manager->sys_status.in_power_off_stage = 1;

    ui_key_filter(true);


    /* 延迟处理关机流程?
     * 等待其它事件消息处理完成后再进行关机,
     * 避免关机过程中处理一些以前的事件消息导致状态异常问题?
     */

    /* TWS 同步关机?
     */
    if (sys_configs->cfg_tws_sync.Sync_Mode & TWS_SYNC_POWER_OFF)
    {
        single_poweroff = 0;
    }
    else
    {
        single_poweroff = 1;
    }

    /* 电量不足自动关机时不同步关机?
     */
    if (manager->sys_status.bat_too_low_powoff)
    {
        single_poweroff = 1;
    }

    /* 定时自动关机时同步关机?
     */
    if (manager->is_auto_timer_powoff)
    {
        single_poweroff = 0;
    }
    
    if (!tws_poweroff)
    {
        if(!single_poweroff)
        {
            bt_manager_tws_send_message(TWS_USER_APP_EVENT, TWS_EVENT_POWER_OFF, 0, 0);
        }
        
        bt_manager_proc_poweroff(single_poweroff);
    }

    fg_app = app_manager_get_current_app();
    if (fg_app)
    {
        if (strcmp(APP_ID_MAIN, fg_app) != 0)
        {
            app_manager_active_app(APP_ID_ILLEGAL);
        }
    }

    led_notify_stop_display(LED_LAYER_BT);
    led_layer_delete(LED_LAYER_BT);
    
    led_notify_stop_display(LED_LAYER_BT_MANAGER);
    led_layer_delete(LED_LAYER_BT_MANAGER);

    //system_do_event_notify(UI_EVENT_POWER_OFF, false, NULL); //提示音，显示
}
#endif

int system_app_standby_check_dc5v(void)
{
#ifdef CONFIG_POWER_MANAGER

    system_app_context_t*  manager = system_app_get_context();
    
    system_configs_t*  sys_configs = &manager->sys_configs;
#endif
    int wake = 0;
#ifdef CONFIG_POWER_MANAGER
    /* DC5V 处于接入状态?
     */
    if (power_manager_get_dc5v_status())
    {
        wake = 1;

        /* DC5V 接入状态下电池处于断开状态时允许自动待机或关机 (硬开关方案)?
         */
        if (!sys_configs->cfg_onoff_key.Use_Inner_ONOFF_Key)
        {
            if (!power_manager_check_bat_exist())
            {
                wake = 0;
            }
        }
        
        /* DC5V 接入状态下电池处于满电状态时允许自动待机或关机?
         */
        if (power_manager_check_bat_is_full())
        {
            wake = 0;
        }

    }
#endif
    return wake;
}

#define DEBUG 1
#if DEBUG
#define p_debug(...)  	\
		do {	\
            if (time_count%100) \
                break;            \
			printk(__VA_ARGS__);	\
		} while (0)
#else
#define p_debug(...)  	
#endif

int system_app_check_auto_standby(void)
{
    system_app_context_t*  manager = system_app_get_context();
    btmgr_pair_cfg_t * bt_pair_cfg = bt_manager_get_pair_config();

    CFG_Struct_System_Settings*  sys_settings = 
        &manager->sys_configs.cfg_sys_settings;

    int auto_standby_time_sec = sys_settings->Auto_Standby_Time_Sec;
    int auto_powoff_time_sec = sys_settings->Auto_Power_Off_Time_Sec;
    int auto_powoff_mode = sys_settings->Auto_Power_Off_Mode;

    static int last_connected_dev_num;

#if DEBUG
    static int time_count = 0;
    time_count++;
#endif

    if (manager->sys_status.in_front_charge ||
        manager->sys_status.in_power_off_stage ||
        manager->sys_status.in_power_off_charge)
    {
        system_set_auto_poweroff_time(0);
        system_set_autosleep_time(0);
        return 0;
    }

    if (auto_powoff_mode == AUTO_POWOFF_MODE_UNCONNECTED)
    {
        int connected_dev_num = bt_manager_get_connected_audio_dev_num();
        if (connected_dev_num > 0)
        {
            auto_powoff_time_sec = 0;
            p_debug("%d dev connected\n", connected_dev_num);
        }
        else if (last_connected_dev_num > 0)
        {
            /* free time 清零
             */
            //sys_wake_lock(FULL_WAKE_LOCK);
            //sys_wake_unlock(FULL_WAKE_LOCK);
            //p_debug("UNCONNECTED, free time 0\n");
            auto_standby_time_sec = 0;
            auto_powoff_time_sec = 0;
        }

        last_connected_dev_num = connected_dev_num;
    }

    if (system_app_standby_check_dc5v() ||
        bt_manager_is_tws_pair_search() ||
        manager->sys_status.in_front_charge ||
        manager->sys_status.in_test_mode ||
        manager->sys_status.in_power_off_stage ||
        manager->sys_status.in_ota_update ||
        manager->sys_status.disable_auto_standby)
    {
        /* free time 清零
         */
        //sys_wake_lock(FULL_WAKE_LOCK);
        //sys_wake_unlock(FULL_WAKE_LOCK);
        auto_standby_time_sec = 0;
        auto_powoff_time_sec = 0;

        p_debug("free time 0: %d %d %d %d %d %d %d\n", 
        system_app_standby_check_dc5v(),
        bt_manager_is_tws_pair_search(),
        manager->sys_status.in_front_charge,
        manager->sys_status.in_test_mode,
        manager->sys_status.in_power_off_stage,
        manager->sys_status.in_ota_update,
        manager->sys_status.disable_auto_standby);
    }

    if (bt_manager_get_wake_lock())
    {
        sys_wake_lock_reset(FULL_WAKE_LOCK);
        p_debug("bt wake lock: %d %d %d %d %d\n", 
        bt_manager_get_wake_lock(),
        bt_manager_is_pair_mode(),
        bt_pair_cfg->pair_mode_duration_sec,
        bt_manager_is_wait_connect(),
        bt_pair_cfg->default_state_wait_connect_sec);
    }

/*
    p_debug("time: %d %d %d %d\n", 
    auto_standby_time_sec,
    sys_settings->Auto_Standby_Time_Sec,
    auto_powoff_time_sec,
    sys_settings->Auto_Power_Off_Time_Sec);
*/
    system_set_auto_poweroff_time(auto_powoff_time_sec);
    system_set_autosleep_time(auto_standby_time_sec);

    return 0;
}
