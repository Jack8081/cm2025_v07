/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app poweron
 */
#include "soc.h"
#include "system_app.h"
#include "power_manager.h"
#include "tts_manager.h"
#include "bt_manager.h"
#include "sys_wakelock.h"

void system_powon_event_notify(uint32_t reboot_mode, bool front_charge)
{
    btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
    
    if (cfg_feature->con_real_test_mode != DISABLE_TEST)
    {
        system_do_event_notify(UI_EVENT_ENTER_BQB_TEST_MODE, false, NULL);
    }
    else if (front_charge)
    {
        system_do_event_notify(UI_EVENT_FRONT_CHARGE_POWON, false, NULL);
    }
    else
    {
        if (reboot_mode == REBOOT_REASON_CLEAR_TWS_INFO)
        {
            return;
        }

        /* 开机提示
         */
        system_do_event_notify(UI_EVENT_POWER_ON, false, NULL);
    }
}

bool system_powon_check_front_charge(void)
{
    system_app_context_t*  manager = system_app_get_context();

    int  sel_charge_mode = 
        manager->sys_configs.cfg_charge.Select_Charge_Mode;

    /* 前台充电模式?
     */
    if (sel_charge_mode == BAT_FRONT_CHARGE_MODE)
    {
        uint32_t  t = os_uptime_get_32();
    
        while (power_manager_get_dc5v_status())
        {
            if (system_check_dc5v_cmd_pending())
            {
                break;
            }
            
            if ((os_uptime_get_32() - t)  >= 500)
            {
                return true;
            }
            
            os_sleep(50);
        }
    }

    return false;
}

uint32_t get_diff_time(uint32_t end_time, uint32_t begin_time)
{
    uint32_t  diff_time;
    
    if (end_time >= begin_time)
    {
        diff_time = (end_time - begin_time);
    }
    else  // 溢出回绕?
    {
        diff_time = ((uint32_t)-1 - begin_time + end_time + 1);
    }

    return diff_time;
}

/* 检查 BATLV 唤醒开机
 */
bool system_check_bat_low_powon(uint8_t *wake_volt)
{
    system_app_context_t*  manager = system_app_get_context();
    CFG_Struct_Charger_Box*  cfg = &(manager->sys_configs.cfg_charger_box);
	CFG_Struct_Battery_Charge* cfg_bat = &(manager->sys_configs.cfg_charge);
    uint32_t  bat_mv = power_manager_get_battery_vol()/1000;

    SYS_LOG_INF("%d", bat_mv);

    if (cfg->Enable_Charger_Box &&
        cfg_bat->Enable_Battery_Recharge)
    {
        if (sys_pm_get_bat_low_threshold(wake_volt))
        {
            *wake_volt = 0xff;
            return false;
        }

        /* BATLV 异常唤醒?
         */
        if (bat_mv >= 4100)
        {
            if (*wake_volt > 0)
            {
                /* 将唤醒电压降低一级?
                 */
                *wake_volt -= 1;
            }
            else
            {
                /* 禁止 BATLV 唤醒?
                 */
                *wake_volt = 0xff;
            }

            /* 调整参数后再关机
             */
            return false;
        }
        
        /* 尝试唤醒充电盒
         */
        if (power_manger_wake_charger_box() == DC5V_STATUS_IN)
        {
            return true;
        }
    }

    /* 禁止 BATLV 唤醒
     */
    *wake_volt = 0xff;

    return false;
}

/* 检查 DC5VLV 唤醒开机
 */
bool system_check_dc5v_low_powon(void)
{
    system_app_context_t*  manager = system_app_get_context();
    CFG_Struct_Charger_Box*  cfg = &(manager->sys_configs.cfg_charger_box);

    SYS_LOG_INF("");
    
    if (cfg->Enable_Charger_Box)
    {
        int  state;
        
        uint32_t  t = os_uptime_get_32();
        
        /* 等待充电盒待机延迟
         */
        while (1)
        {
            state = power_manager_get_dc5v_status_chargebox();
            
            if (state == DC5V_STATUS_IN)
            {
                return true;
            }
        
            if (get_diff_time(os_uptime_get_32(), t) > cfg->Charger_Standby_Delay_Ms + 50)
            {
                break;
            }

            os_yield();
        }
        SYS_LOG_INF("state:%d", state);

        /* DC5V 状态处理中?
         */
        if (state == DC5V_STATUS_PENDING)
        {
            t = os_uptime_get_32();
            
            while (state == DC5V_STATUS_PENDING)
            {
                os_yield();
            
                state = power_manager_get_dc5v_status_chargebox();
        
                /* 可能处于 DC5V_UART 通讯状态?
                 */
                if (get_diff_time(os_uptime_get_32(), t) > 1000)
                {
                    break;
                }
            }
        }
        
        /* 充电盒还处于待机状态时不继续开机?
         */
        if (state == DC5V_STATUS_STANDBY)
        {
            return false;
        }
    }

    return true;
}

/* 检查 ONOFF 开机
 */
bool system_check_onoff_powon(uint32_t need_press_time_ms)
{
    #ifdef CONFIG_POWER_MANAGER
    system_app_context_t*  manager = system_app_get_context();
    CFG_Struct_Charger_Box*  cfg = &(manager->sys_configs.cfg_charger_box);
    #endif
    int onoff_key_stable_up = 0;

    SYS_LOG_INF("");
    
    do
    {
        #ifdef CONFIG_POWER_MANAGER
        /* 检测到 DC5V 接入高电平?
         */
        if (power_manager_get_dc5v_status())
        {
            return true;
        }
        #endif
        /* 按下足够时间?
         */
        if (os_uptime_get_32() >= need_press_time_ms)
        {
            break;
        }

        /* 稳定抬起?
         */
        if (system_app_check_onnff_key_state() == KEY_VALUE_UP)
        {
            onoff_key_stable_up += 1;
            if (onoff_key_stable_up >= 5)
                return false;
        } else {
            onoff_key_stable_up = 0;
        }

        os_sleep(10);
    }
    while (1);

    
    #ifdef CONFIG_POWER_MANAGER
    /* 尝试唤醒充电盒
     */
    if (cfg->Enable_Charger_Box)
    do
    {
        if (power_manager_get_dc5v_status_chargebox() == DC5V_STATUS_OUT)
        {
            break;
        }
        
        /* 如果 DC5V 漏电低电压, 会误判为处于充电盒中,
         * 需要打开下拉 DC5V 一段时间后再检测实际状态?
         */
        power_manger_set_dc5v_pulldown();
        
        os_sleep(30);

        /* 充电盒不能唤醒时不继续开机
         */
        if (power_manger_wake_charger_box() == DC5V_STATUS_STANDBY)
        {
            return false;
        }
    }
    while (0);
    #endif
    return true;
}

int system_check_power_on(void)
{
    system_app_context_t*  manager = system_app_get_context();
    uint8_t enable_inner_onoff_key = manager->sys_configs.cfg_onoff_key.Use_Inner_ONOFF_Key;
    uint16_t onoff_need_press_time_ms = manager->sys_configs.cfg_onoff_key.Time_Press_Power_On;
    union sys_pm_wakeup_src wakeup_src;
    uint8_t dc5vlv_volt, batlv_volt;

    sys_pm_get_wakeup_source(&wakeup_src);
    sys_pm_set_onoff_wakeup(enable_inner_onoff_key);

    dc5vlv_volt = 0xff;
    batlv_volt = 0xff;

    if (wakeup_src.t.watchdog) {
        SYS_LOG_INF("WATCHDOG");
        return 0;
    }
    
    do
    {
        
        #ifdef CONFIG_POWER_MANAGER
        /* BATLV 唤醒开机?
         */
        if (wakeup_src.t.batlv)
        {
            if (system_check_bat_low_powon(&batlv_volt))
            {
                break;
            }
            else
            {
                goto powoff;
            }
        }
        
        /* DC5VLV 唤醒开机?
         */
        if (wakeup_src.t.dc5vlv)
        {
            if (system_check_dc5v_low_powon())
            {
                break;
            }
            else
            {
                goto powoff;
            }
        }

        /* 检测到 DC5V 接入高电平?
         */
        if (power_manager_get_dc5v_status())
        {
            break;
        }
        #endif
        /* ONOFF 按键唤醒开机?
         */
        if ((wakeup_src.t.short_onoff || wakeup_src.t.long_onoff) && 
            enable_inner_onoff_key)
        {
            if (wakeup_src.t.long_onoff &&
                onoff_need_press_time_ms >= 125) {
                onoff_need_press_time_ms -= 125;
            }
            if (system_check_onoff_powon(onoff_need_press_time_ms))
            {
                break;
            }
            else
            {
                goto powoff;
            }
        }

        if (wakeup_src.data == 0) {
            SYS_LOG_ERR("UNKNOWN");
            // goto powoff;
        }
    }
    while (0);

    return 0;

powoff:

    if (sys_pm_get_dc5v_low_threshold(&dc5vlv_volt))
    {
        sys_pm_set_dc5v_low_wakeup(0,0);
    }
    else
    {
        sys_pm_set_dc5v_low_wakeup(1,dc5vlv_volt);
    }

    if ((wakeup_src.t.batlv && batlv_volt == 0xff) || sys_pm_get_bat_low_threshold(&batlv_volt))
    {
        sys_pm_set_bat_low_wakeup(0,0);
    }
    else
    {
        sys_pm_set_bat_low_wakeup(1,batlv_volt);
    }

#ifdef CONFIG_PROPERTY
    property_flush(NULL);
#endif
    sys_pm_poweroff();

    return 0;
}
