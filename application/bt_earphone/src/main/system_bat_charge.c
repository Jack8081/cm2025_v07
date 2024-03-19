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
#include <system_notify.h>
#include <soc_pm.h>
#include "led_display.h"
#include <anc_hal.h>


void _ble_delay_poweroff(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	if(bt_manager_ble_is_connected())
	{
		bt_manager_ble_disconnect(NULL);
	}
    bt_manager_ble_set_advertise_enable(false);
	
	sys_ble_advertise_deinit();
}

void system_bat_low_prompt(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    system_app_context_t*  manager = system_app_get_context();
    uint32_t event = (uint32_t)expiry_fn_arg;

    uint32_t  notify_id = SYS_EVENT_BATTERY_LOW;

    uint32_t  bat_low_prompt_interval_sec =
        manager->sys_configs.bat_low_prompt_interval_sec;

    /* 更低电量提示?
     */
    if (event == POWER_EVENT_BATTERY_LOW_EX)
    {
        if (system_notify_get_config(SYS_EVENT_BATTERY_LOW_EX))
        {
            notify_id = SYS_EVENT_BATTERY_LOW_EX;
        }
    }

    /* 重复低电提示?
     */
    if (ttimer)
    {
        if (system_notify_get_config(SYS_EVENT_REPEAT_BAT_LOW))
        {
            notify_id = SYS_EVENT_REPEAT_BAT_LOW;
        }
    }

    thread_timer_stop(&manager->bat_low_prompt_timer);

    sys_manager_event_notify(notify_id, false);

    if (bat_low_prompt_interval_sec != 0)
    {
        /* 重复低电提示?
         */
        thread_timer_init
        (
            &manager->bat_low_prompt_timer,
            system_bat_low_prompt,
            expiry_fn_arg
        );

        thread_timer_start
        (
            &manager->bat_low_prompt_timer,
            bat_low_prompt_interval_sec * 1000,
            bat_low_prompt_interval_sec * 1000
        );
    }
}

void system_bat_too_low(void)
{
    system_app_context_t*  manager = system_app_get_context();

    sys_manager_event_notify(SYS_EVENT_BATTERY_TOO_LOW, false);

    /* 电量过低时自动关机
     */
    manager->sys_status.bat_too_low_powoff = YES;

    sys_event_send_message(MSG_POWER_OFF);
}

void system_bat_charge_stop(void)
{
    system_app_context_t*  manager = system_app_get_context();

    uint32_t  led_disp = UI_EVENT_CHARGE_STOP;

    /* 前台充电模式, DC5V 进入低压待机状态,
     * 如果电池电压已达到停充电压, 则显示满电状态?
     */
    if (manager->sys_status.in_front_charge)
    {
        if (power_manager_get_dc5v_status_chargebox() == DC5V_STATUS_STANDBY)
        {
            if (power_manager_get_battery_capacity_local() >= 100)
            {
                led_disp = UI_EVENT_CHARGE_FULL;
            }
        }
    }
    
    ui_event_led_display(led_disp);
}

int system_bat_charge_event_proc(int event)
{
    system_app_context_t*  manager = system_app_get_context();
    
    system_status_t*  sys_status = &manager->sys_status;

    /* DC5V 插拔时重置自动待机计时
     */
    if (event == POWER_EVENT_DC5V_IN ||
        event == POWER_EVENT_DC5V_OUT)
    {
        sys_wake_lock(FULL_WAKE_LOCK);
        sys_wake_unlock(FULL_WAKE_LOCK);
    }

    switch (event)
    {
        case POWER_EVENT_DC5V_IN:
        {
            system_check_in_charger_box();
            
            /* DC5V 接入后重置低电检测
             */
            thread_timer_stop(&manager->bat_low_prompt_timer);
            break;
        }

        case POWER_EVENT_DC5V_OUT:
        case POWER_EVENT_DC5V_STANDBY:
        {
            system_check_in_charger_box();
            break;
        }

        case POWER_EVENT_CHARGE_START:
        {
            sys_status->charge_state = 1;

            sys_manager_event_notify(SYS_EVENT_CHARGE_START, false);
            break;
        }

        case POWER_EVENT_CHARGE_STOP:
        {
            sys_status->charge_state = 0;
                        
            system_bat_charge_stop();
            break;
        }

        case POWER_EVENT_CHARGE_FULL:
        {
            sys_manager_event_notify(SYS_EVENT_CHARGE_FULL, false);
            break;
        }

        case POWER_EVENT_BATTERY_NORMAL:
            ui_event_led_display(UI_EVENT_CUSTOMED_8);
            break;
        case POWER_EVENT_BATTERY_MEDIUM:
            ui_event_led_display(UI_EVENT_CUSTOMED_9);
            break;

        case POWER_EVENT_BATTERY_LOW:
            ui_event_led_display(UI_EVENT_BATTERY_LOW);
			
            // go through...
        case POWER_EVENT_BATTERY_LOW_EX:
        {
            system_bat_low_prompt(NULL, (void *)event);
            break;
        }

        case POWER_EVENT_BATTERY_TOO_LOW:
        {
            system_bat_too_low();
            break;
        }

        case POWER_EVENT_BATTERY_FULL:
        {
            ui_event_led_display(UI_EVENT_CHARGE_FULL);
            break;
        }

        default:
            break;
    }

    return 0;
}

void system_app_poweroff_charge(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    system_app_context_t*  manager = system_app_get_context();
    
    system_configs_t*  sys_configs = &manager->sys_configs;

    if(!manager->sys_status.in_power_off_charge)
        return;

    // check dc5v
    if(!power_manager_get_dc5v_status())
        goto poweroff;

    // check onoff key
    if(system_app_check_onnff_key_state())
    {
        sys_pm_reboot(0);
    }

    // check config
    if(!sys_configs->cfg_onoff_key.Use_Inner_ONOFF_Key)
    {
        if (!power_manager_check_bat_exist())
        {
            goto poweroff;
        }
    }

    // check battary full
    if (power_manager_check_bat_is_full())
        goto poweroff;

    return;
poweroff:
    sys_pm_poweroff();
}

void system_app_start_poweroff_charge(void)
{
	system_app_context_t*  manager = system_app_get_context();
	
	if (thread_timer_is_running(&manager->poweroff_charge_timer))
	{
		SYS_LOG_ERR("poweroff charge already started");
        return;
	}

    printk("\n     *****enter poweroff charge*****\n");

    manager->sys_status.in_power_off_charge = 1;

	thread_timer_init(&manager->poweroff_charge_timer, system_app_poweroff_charge, NULL);
	thread_timer_start(&manager->poweroff_charge_timer, 20, 20);
}

/*!
 * \brief 获取 DC5VLV 对应级别
 * \n  返回 CFG_TYPE_DC5VLV_LEVEL
 */
static uint32_t dc5v_get_low_level(uint32_t mv)
{
    if (mv >= 3100)
    {
        return DC5VLV_LEVEL_3_0_V;
    }
    else if (mv >= 2100)
    {
        return DC5VLV_LEVEL_2_0_V;
    }
    else if (mv >= 1100)
    {
        return DC5VLV_LEVEL_1_0_V;
    }
    
    return DC5VLV_LEVEL_0_2_V;
}

void system_app_front_charge(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    system_app_context_t*  manager = system_app_get_context();

    system_configs_t*  sys_configs = system_get_configs();

    CFG_Struct_Battery_Charge  *cfg_charge;

    CFG_Struct_Charger_Box     *cfg_charger_box;

    uint8_t enable_dc5v_out_reboot, enable_bat_recharge;

    static uint32_t  charge_full_time = 0;

    enable_dc5v_out_reboot = 0;
    enable_bat_recharge = 0;

    cfg_charge = &sys_configs->cfg_charge;
    cfg_charger_box = &sys_configs->cfg_charger_box;
    
    /* 前台充电 DC5V 拔出时重启? */
    {
        uint32_t  flags = sys_configs->cfg_sys_settings.Support_Features;
        
        if (flags & SYS_FRONT_CHARGE_DC5V_OUT_REBOOT)
        {
            enable_dc5v_out_reboot = 1;
        }
    }

    /* 电池低电复充?
     */
    if (cfg_charger_box->Enable_Charger_Box &&
        cfg_charge->Enable_Battery_Recharge)
    {
        /* 充电盒不能下拉唤醒时无法复充
         */
        if (cfg_charger_box->DC5V_Pull_Down_Current != DC5VPD_CURRENT_DISABLE)
        {
            enable_bat_recharge = 1;
        }
    }
    
    /* 前台充电状态下保留充电状态通知显示
     */
    do
    {
        /* 充电盒电池低电时不能维持 DC5V 输出, 与耳机取出动作无法区分,
         * 因此不能唤醒开机, 避免在用户未知情况下意外开机回连
         */
        if (manager->sys_status.chg_box_bat_low)
        {
            if (!system_check_dc5v_cmd_pending())
            {
                enable_dc5v_out_reboot = 0;
                enable_bat_recharge    = 0;
                break;
            }
        }

        /* 耳机充满电后, 充电盒控制耳机关机?
         */
        if (manager->chg_box_ctrl_powoff)
        {
            if (!system_check_dc5v_cmd_pending())
            {
                goto POWEROFF;
            }
        }

        int  dc5v_state = power_manager_get_dc5v_status_chargebox();
        if (dc5v_state != POWER_SUPPLY_STATUS_DC5V_IN)
        {
        SYS_LOG_INF("dc5v: %d, bat per: %d %d", dc5v_state, power_manager_get_battery_capacity_local(), power_manager_check_bat_is_full());
        }
        

        if (dc5v_state == DC5V_STATUS_OUT)
        {
            /* 前台充电 DC5V 拔出时重启?
             */
            if (enable_dc5v_out_reboot)
            {
            #ifdef CONFIG_PROPERTY
                property_flush(NULL);
            #endif
                sys_pm_reboot(0);
            }
            goto POWEROFF;
        }
        

        /* DC5V 接入状态下电池处于断开状态时允许关机 (硬开关方案)?
         */
        if (!sys_configs->cfg_onoff_key.Use_Inner_ONOFF_Key)
        {
            if (!power_manager_check_bat_exist())
            {
                enable_dc5v_out_reboot = 0;
                enable_bat_recharge    = 0;
                break;
            }
        }

        if (!power_manager_check_bat_is_full())
        {
            charge_full_time = 0;
        }
        else if (charge_full_time == 0)
        {
            charge_full_time = os_uptime_get_32();
        }
        else
        {
            uint32_t  t = os_uptime_get_32() - charge_full_time;

            /* 电池已充满等待一段时间后关机?
             */
            if (t > cfg_charge->Front_Charge_Full_Power_Off_Wait_Sec * 1000)
            {
                if (manager->sys_status.chg_box_bat_low)
                {
                    enable_dc5v_out_reboot = 0;
                    enable_bat_recharge    = 0;
                }
                goto POWEROFF;
            }
        }
    }
    while (0);

    return;

POWEROFF:
#ifdef CONFIG_PROPERTY
    property_flush(NULL);
#endif
    /* 关机
     */
    SYS_LOG_INF("dc5v out reboot: %d, bat recharge: %d", enable_dc5v_out_reboot, enable_bat_recharge);
    sys_pm_set_dc5v_low_wakeup(enable_dc5v_out_reboot, dc5v_get_low_level(cfg_charger_box->Charger_Standby_Voltage));
#ifdef CONFIG_SOC_SERIES_LARK
    sys_pm_set_bat_low_wakeup(enable_bat_recharge, cfg_charge->Battery_Recharge_Threshold);
#else
    sys_pm_set_bat_low_wakeup(enable_bat_recharge, cfg_charge->Recharge_Threshold_Normal_Temperature);
#endif
    sys_pm_poweroff();
}

void system_app_start_front_charge(void)
{
	system_app_context_t*  manager = system_app_get_context();
	
	if (thread_timer_is_running(&manager->front_charge_timer))
	{
		SYS_LOG_ERR("front charge already started");
        return;
	}

    /* enable key for KEY_FUNC_CLEAR_PAIRED_LIST_IN_FRONT_CHARGE */
    ui_key_filter(false);

	thread_timer_init(&manager->front_charge_timer, system_app_front_charge, NULL);
	thread_timer_start(&manager->front_charge_timer, 100, 2000);
}

static void system_app_enter_front_charge(void)
{
    system_app_context_t*  manager = system_app_get_context();
	const char* fg_app;

    if (manager->sys_status.in_front_charge)
    {
        return;
    }
    
    //os_delayed_work_init(&manager->poweroff_work, system_app_proc_poweroff);

    SYS_LOG_INF("");

    manager->sys_status.in_front_charge = 1;

    ui_key_filter(true);
	
	tts_manager_lock();

    /* 关闭蓝牙（不会关闭ble）
     */
	bt_manager_proc_poweroff(1);

#ifndef CONFIG_LE_AUDIO_APP	
	_ble_delay_poweroff(NULL,NULL);
#else
    /* 延迟关闭BLE
     * (充电盒关盖后, 耳机进入前台充电, 可能还需要 BLE 广播一段时间以取消手机弹窗)
     */
	thread_timer_init(&manager->ble_delay_pwroff_timer, _ble_delay_poweroff, NULL);
	thread_timer_start(&manager->ble_delay_pwroff_timer, 10*1000, 0);
#endif

    fg_app = app_manager_get_current_app();
    
    if (fg_app != NULL &&
        strcmp(fg_app, APP_ID_MAIN) != 0)
    {
        app_manager_active_app(APP_ID_ILLEGAL);
    }


    led_notify_stop_display(LED_LAYER_BT);
    led_layer_delete(LED_LAYER_BT);
    
    led_notify_stop_display(LED_LAYER_BT_MANAGER);
    led_layer_delete(LED_LAYER_BT_MANAGER);

    #ifdef CONFIG_ANC_HAL
    anc_dsp_close();
    #endif
}

int system_app_check_front_charge(void)
{
    system_app_context_t*  manager = system_app_get_context();

    int  sel_charge_mode = 
        manager->sys_configs.cfg_charge.Select_Charge_Mode;

    /* 前台充电模式?
     */
    if (sel_charge_mode == BAT_FRONT_CHARGE_MODE)
    do
    {
        if (manager->sys_status.disable_front_charge ||
            manager->sys_status.in_front_charge ||
            manager->sys_status.in_test_mode)
        {
            break;
        }

		if(!power_manager_get_dc5v_status()){
			break;
		}

        if (power_manager_get_dc5v_status_chargebox() != DC5V_STATUS_IN)
        {        
            break;
        }

        if (system_check_dc5v_cmd_pending())
        {
            break;
        }
        
        system_app_enter_front_charge();
    }
    while (0);

    return 0;

}
