#include "soc.h"
#include "system_app.h"
//#include "host_interface.h"
#include "bt_manager.h"
#include "production_test.h"
#include "system_dc5v_io_cmd.h"
#include "dc5v_uart.h"

static uint8_t dc5v_io_cmd_completed = 0;

static void chg_box_open_reboot(uint32_t bat_level)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t   *sys_status = &manager->sys_status;
    uint8_t level = bat_level;
    int ret;

    /* 前台充电状态下开盖, 直接重启开机,
     * 开机后会再次检测到开盖命令 (命令要求发送 3 次),
     * 不进入前台充电, 而是正常启动回连
     */
    if (!sys_status->in_front_charge)
    {
        return;
    }

    ret = property_set(CFG_CHG_BOX_BAT_LEVEL, &level, sizeof(uint8_t));
    
    SYS_LOG_INF("%x, %d", ret, level);

    property_flush(NULL);
    
    sys_pm_reboot(REBOOT_TYPE_GOTO_SYSTEM|REBOOT_REASON_CHG_BOX_OPENED);

    return;
}

static void dc5v_io_cmd_complete_notify(void)
{
    /* 停止当前所有通知
     */
    //sys_manager_stop_notify(-1, -1);

    //audio_manager_set_volume(&max_vol);  // 设置最大音量

    sys_manager_event_notify(SYS_EVENT_DC5V_CMD_COMPLETE, false);
    
    //audio_manager_set_volume(bak_vol);  // 恢复音量
}

static void dc5v_io_cmd_complete_poweroff(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    /* 循环提示, DC5V 通讯断开后关机
     */
    dc5v_io_cmd_complete_notify();

    if (!system_check_dc5v_cmd_pending())
    {
        system_app_enter_poweroff(0);
    }
}

/* 产测命令完成后进行提示, 
 * 选择是否在通讯断开后关机 (从产测板盒中取出)
 */
void system_dc5v_io_cmd_complete(bool wait_powoff)
{
    system_app_context_t*  manager = system_app_get_context();

    if (!wait_powoff)
    {
        dc5v_io_cmd_complete_notify();
        return;
    }

    if (!thread_timer_is_running(&manager->dc5v_io_timer))
    {
        thread_timer_init(&manager->dc5v_io_timer, dc5v_io_cmd_complete_poweroff, NULL);
        thread_timer_start(&manager->dc5v_io_timer, 0, 500);
    }
}


/* 产测进入 BQB 测试模式, 重启后自动进入
 * bqb_mode : CFG_TYPE_BT_CTRL_TEST_MODE
 */
void dc5v_io_cmd_enter_bqb_mode(u32_t bqb_mode)
{
    btmgr_feature_cfg_t* btmgr_feature_cfg = bt_manager_get_feature_config();
    
    if (btmgr_feature_cfg->controller_test_mode == BT_CTRL_DISABLE_TEST)
    {
        SYS_LOG_INF("enter bqb %d", bqb_mode);
        
        system_enter_bqb_test_mode(bqb_mode);
    }

}

/* 产测清除配对列表
 * clear_tws : 是否清除tws
 */
void dc5v_io_cmd_clear_paired_list(u32_t clear_tws)
{
    SYS_LOG_INF("%d", clear_tws);

    bt_manager_clear_paired_list();

    if (clear_tws)
    {
        bt_manager_tws_clear_paired_list();
    }
}

/* 产测清除配对列表, 选择是否同时清除 TWS 组对设备信息,
 * 清除完成后从产测板盒取出自动关机
 */
void dc5v_io_cmd_clear_paired_list_ex(u32_t cmd_id, u32_t clear_tws)
{
    dc5v_io_cmd_clear_paired_list(clear_tws);
    
    system_dc5v_io_cmd_complete(true);
}

static void dc5v_io_cmd_tws_pair_check(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    int *state = (int *)expiry_fn_arg;
    
    if (bt_manager_tws_get_dev_role() != BTSRV_TWS_NONE)
        *state = TWS_PAIR_OK;
    else if (!system_check_dc5v_cmd_pending())
        *state = TWS_PAIR_ABORT;

}

int dc5v_io_cmd_enter_tws_pair_mode(u32_t keyword)
{
    system_app_context_t*  manager = system_app_get_context();
    static int state= TWS_PAIR_NONE;

    SYS_LOG_INF("0x%x 0x%x", keyword, state);

    if (state != TWS_PAIR_NONE)
    {
        if (state != TWS_PAIR_PENDING)
            thread_timer_stop(&manager->dc5v_io_timer);
        return state;
    }

    /* Clear paired devices list and TWS device information?
     */
    dc5v_io_cmd_clear_paired_list(1);

    //bt_manager_tws_pair_search();
    bt_manager_tws_start_pair_search_ex(keyword);

    /* 等待组对成功
     */        
    thread_timer_init(&manager->dc5v_io_timer, dc5v_io_cmd_tws_pair_check, &state);
    thread_timer_start(&manager->dc5v_io_timer, 0, 100);

    state = TWS_PAIR_PENDING;

    return state;
}

/* 产测 TWS 组对, 将放入同一产测板盒的两只耳机进行组对,
 * 多个产测板盒可以设置不同组对命令, 对应不同的组对匹配关键字,
 * 可以避免不同板盒内的耳机错误配对问题,
 * 耳机组对成功后从产测板盒取出自动关机
 */
void dc5v_io_cmd_enter_tws_pair_mode_ex(u32_t cmd_id, u32_t keyword)
{
    int state;

    state = dc5v_io_cmd_enter_tws_pair_mode(keyword);

    if (state == TWS_PAIR_OK)
    {        
        system_dc5v_io_cmd_complete(YES);
    }
    else if (state == TWS_PAIR_ABORT)
    {
        system_app_enter_poweroff(0);
    }
}

void dc5v_io_cmd_ctrl_power_off(uint32_t cmd_id)
{
    SYS_LOG_INF("");
    
    system_dc5v_io_cmd_complete(true);
}

void dc5v_io_cmd_enter_pair_mode(void)
{
    
    SYS_LOG_INF("");
        
    bt_manager_enter_pair_mode();
}


void dc5v_io_cmd_enter_pair_mode_ex(uint32_t cmd_id)
{
    SYS_LOG_INF("");
    
    dc5v_io_cmd_enter_pair_mode();
    
    system_dc5v_io_cmd_complete(true);
}

void dc5v_io_cmd_enter_ota_mode(void)
{
    system_app_context_t*  manager = system_app_get_context();
    
    SYS_LOG_INF("");

    // Disable auto standby and auto power off
    {
        manager->sys_configs.cfg_sys_settings.Auto_Power_Off_Time_Sec = 0;
        manager->sys_configs.cfg_sys_settings.Auto_Standby_Time_Sec = 0;
    }

    //set_production_test_ota_flag(YES);

    bt_manager_enter_pair_mode();
}


void dc5v_io_cmd_enter_ota_mode_ex(uint32_t cmd_id, uint32_t keyword)
{
    dc5v_io_cmd_enter_ota_mode();
}

void dc5v_io_cmd_search_ble_adv_handler(uint8_t addr_type, uint8_t *addr, int8_t rssi,
										uint8_t adv_type, uint8_t *data, uint8_t data_len)
{
    int pos = 0, match = 0;

    if (dc5v_io_cmd_completed)
        return;
    
    SYS_LOG_INF("");

    production_test_ble_adv_header_t * header = (production_test_ble_adv_header_t *)data;
    do
    {
        if (header->type != ADV_TYPE_MANUFACTORY_DATA)
            break;
        if (header->company_id_0 != (ADV_COMPANY_ID_ACTIONS & 0xFF))
            break;
        if (header->company_id_1 != (ADV_COMPANY_ID_ACTIONS >> 8))
            break;
        if (header->type_0 != ADV_TYPE_0_ATS3019)
            break;
        if (header->type_1 != ADV_TYPE_1_PRODUCTION_TEST)
            break;

        SYS_LOG_INF("adv type match");
        match = 1;
    } while (0);

    if (match)
    {
        pos = sizeof(production_test_ble_adv_header_t);
        production_test_ble_adv_cmd_t * adv_cmd;
        while (pos < header->length)
        {
            adv_cmd = (production_test_ble_adv_cmd_t *)(data + pos);
            if (adv_cmd->cmd == PRODUCTION_TEST_ADV_CMD_CHANGE_BT_NAME)
            {
                uint8_t *new_name = app_mem_malloc(adv_cmd->length);
				if (!new_name) return;
                memcpy(new_name, adv_cmd->param, adv_cmd->length - 1);
                SYS_LOG_INF("new name: %s", new_name);
                dc5v_io_cmd_completed = !bt_manager_set_bt_name(new_name);
                app_mem_free(new_name);
            }
            pos += (1 + adv_cmd->length);
        }
    }
        
}


int check_dc5v_io_cmd_search_ble_adv_finish(void)
{
    return (dc5v_io_cmd_completed == YES);
}


void dc5v_io_cmd_search_ble_adv(void)
{
    u8_t ble_addr[] = { 0x55, 0x44, 0x33, 0x22, 0x11, 0xF0, };

    SYS_LOG_INF("");

    bt_manager_ble_clear_whitelist();

    bt_manager_ble_add_whitelist(BT_ADDR_LE_PUBLIC, ble_addr);

    bt_manager_ble_start_scan(BT_LE_SCAN_TYPE_PASSIVE, 0x00, BT_LE_SCAN_OPT_FILTER_WHITELIST,
                                                        70, 20, dc5v_io_cmd_search_ble_adv_handler);

}

void check_dc5v_io_cmd_search_ble_adv_finish_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    int search_finish = NO;
    u8_t* complete = expiry_fn_arg;

    do {
        search_finish = check_dc5v_io_cmd_search_ble_adv_finish();
        if (!search_finish)
        {
            break;
        }
        
        if (complete)
            *complete = (search_finish == YES);

        thread_timer_stop(ttimer);

        system_dc5v_io_cmd_complete(YES);

    } while(0);
}

void dc5v_io_cmd_search_ble_adv_ex(uint32_t cmd_id, uint32_t keyword)
{
    system_app_context_t*  manager = system_app_get_context();

    dc5v_io_cmd_search_ble_adv();
    
    if (thread_timer_is_running(&manager->dc5v_io_timer))
    {
        SYS_LOG_ERR("no timer");
        return;
    }

    thread_timer_init(&manager->dc5v_io_timer, check_dc5v_io_cmd_search_ble_adv_finish_timer, NULL);
    thread_timer_start(&manager->dc5v_io_timer, 0, 50);
}

/* TWS 同步充电盒相关状态
 */
bool system_tws_sync_chg_box_status(void* pdata)
{
    system_app_context_t*  manager = system_app_get_context();

    system_status_t*  sys_status = &manager->sys_status;

    /* 本机同步给对方
     */
    if (pdata == NULL)
    {
        u8_t  params[4];
        
        params[0] = sys_status->charger_box_opened;
        params[1] = sys_status->charger_box_bat_level;
        params[2] = sys_status->in_charger_box;
        params[3] = sys_status->chg_box_bat_low;

        SYS_LOG_INF("tx 0x%x", *(u32_t *)params);

        bt_manager_tws_send_message(TWS_USER_APP_EVENT, TWS_EVENT_SYNC_CHG_BOX_STATUS, params, sizeof(params));
    }
    /* 对方同步给本机
     */
    else
    {
        u8_t*  params = pdata;

        SYS_LOG_INF("rx 0x%x", *(u32_t *)params);
        /* 已处于充电盒中时不需要从对方同步状态
         */
        if (!sys_status->in_charger_box)
        {
            sys_status->charger_box_opened    = params[0];
            sys_status->charger_box_bat_level = params[1];
            sys_status->chg_box_bat_low          = params[3];
        }
        
        sys_status->tws_remote_in_charger_box = params[2];
    }

    return 1;
}

/* 充电盒开盖, 开盖时可以不发送开盖命令,
 * 而直接发送电量命令替代开盖命令以能快速获取充电盒电池电量
 */
void dc5v_io_cmd_chg_box_opened(u32_t bat_level)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t   *sys_status = &manager->sys_status;

    SYS_LOG_INF("0x%x", bat_level);

    chg_box_open_reboot(bat_level);

    if (bat_level != 0xff)
    {
        sys_status->charger_box_bat_level = bat_level;
    }
    
    /* 充满关机状态下开盖, DC5V 上电开机,
     * 开机后会检测到开盖命令 (命令要求发送 3 次),
     * 不进入前台充电, 而是正常启动回连
     */
    sys_status->charger_box_opened = 1;
    sys_status->in_charger_box = 1;
    sys_status->disable_front_charge = 1;

    /* TWS 同步充电盒相关状态
     */
    system_tws_sync_chg_box_status(NULL);
}

#if 0
/* 充电盒电池电量, 开盖时可以不发送开盖命令,
 * 而直接发送电量命令替代开盖命令以能快速获取充电盒电池电量
 */
void dc5v_io_cmd_chg_box_bat_level(u32_t bat_level)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t   *sys_status = &manager->sys_status;

    /* bit0 ~ bit6 指示充电盒电池电量百分比 (0 ~ 100)
     * bit7 指示充电盒自身充电状态
     */
    SYS_LOG_INF("%d, %d%%", ((bat_level >> 7) & 1), (bat_level & 0x7f));

    chg_box_open_reboot(bat_level);

    /* 充满关机状态下开盖 (或电量命令), DC5V 上电开机,
     * 开机后会检测到开盖命令 (或电量命令, 命令要求发送 3 次),
     * 不进入前台充电, 而是正常启动回连
     */
    sys_status->charger_box_bat_level = bat_level;
    sys_status->charger_box_opened = 1;
    sys_status->in_charger_box = 1;
    sys_status->disable_front_charge = 1;

    /* TWS 同步充电盒相关状态
     */
    system_tws_sync_chg_box_status(NULL);
}
#endif

/* bit0 ~ bit4 指示充电盒电池电量 (0 ~ 31)
 * bit5 指示充电盒自身充电状态
 */
void dc5v_io_cmd_chg_box_bat_level_ex(u32_t cmd_id)
{
    u8_t  percent   = (cmd_id & 0x1f) * 100 / 0x1f;
    u8_t  chg_state = (cmd_id >> 5) & 1;

    /* 转换为百分比, MSB 表示充电状态
     */
    u8_t  bat_level = (chg_state << 7) | percent;

    dc5v_io_cmd_chg_box_opened(bat_level);
}

/* 充电盒关盖
 */
void dc5v_io_cmd_chg_box_closed(void)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t   *sys_status = &manager->sys_status;

    SYS_LOG_INF("");

    /* 耳机处于充电盒中, 正常回连或已连接状态,
     * 关盖后将停止回连或断开连接, 并允许进入前台充电
     */
    sys_status->charger_box_opened = 0;
    sys_status->in_charger_box = 1;
    sys_status->disable_front_charge = 0;
    //ble_adv_delay_disable = 10*1000;

    /* TWS 同步充电盒相关状态
     */
    system_tws_sync_chg_box_status(NULL);
}

/* 充电盒自身电池低电
 */
void dc5v_io_cmd_chg_box_bat_low(void)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t   *sys_status = &manager->sys_status;

    SYS_LOG_INF("");

    /* 充电盒自身电池低电时, 耳机允许进入前台充电,
     * 并在之后关机, 关机时禁止 DC5V 拔出唤醒功能,
     * 充电盒电池低电时不能维持 DC5V 输出, 与耳机取出动作无法区分,
     * 因此不能唤醒开机, 避免在用户未知情况下意外开机回连
     */
    sys_status->in_charger_box = 1;
    sys_status->disable_front_charge = 0;
    //ble_adv_delay_disable = 10*1000;
    sys_status->chg_box_bat_low = 1;

    /* TWS 同步充电盒相关状态
     */
    system_tws_sync_chg_box_status(NULL);

}

/* pressed 可定义充电盒按键操作
 * 1 : 短按
 * 2 : 长按
 * 3 : 双击
 * 4 : 超长按
 */
void dc5v_io_cmd_chg_box_key_pressed(u32_t pressed)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t   *sys_status = &manager->sys_status;

    SYS_LOG_INF("%d", pressed);

    /* 两只耳机都处于充电盒中, 并且已组对连接,
     * 只需要由 TWS 主机响应按键以避免冲突
     */
    if (bt_manager_tws_get_dev_role() != BTSRV_TWS_SLAVE &&
        sys_status->tws_remote_in_charger_box == 1)
    {
        return;
    }
    
    /* 充电盒按键长按, 耳机将进入配对模式
     */
    if (pressed == 2)
    {
        if (sys_status->in_front_charge == 0)
        {
            bt_manager_enter_pair_mode();
        }
    }
    /* 充电盒按键超长按, 耳机将清除配对列表
     */
    else if (pressed == 4)
    {
        bt_manager_clear_paired_list();
    }
}

void system_dc5v_io_cmd_proc(int cmd_id)
{
    SYS_LOG_INF("0x%x", cmd_id);

    if (cmd_id == 0)
    {
		int ret;
		uint8_t bat_level;
		ret = property_get(CFG_CHG_BOX_BAT_LEVEL, &bat_level, sizeof(uint8_t));
		if (ret > 0)
        {
			dc5v_io_cmd_chg_box_opened(bat_level);
            bat_level = 0xff;
            property_set(CFG_CHG_BOX_BAT_LEVEL, &bat_level, sizeof(uint8_t));
		}
        else
        {
            SYS_LOG_ERR("get box level fail %d", ret);
        }
        return;
    }

    switch (cmd_id)
    {
        /* 产测板盒命令
         */
        case 0x50: dc5v_io_cmd_enter_tws_pair_mode_ex(cmd_id, 2); break;
        case 0x51: dc5v_io_cmd_enter_tws_pair_mode_ex(cmd_id, 3); break;
        case 0x52: dc5v_io_cmd_enter_tws_pair_mode_ex(cmd_id, 4); break;
        case 0x53: dc5v_io_cmd_enter_tws_pair_mode_ex(cmd_id, 5); break;
        case 0x54: dc5v_io_cmd_enter_tws_pair_mode_ex(cmd_id, 6); break;
        case 0x55: dc5v_io_cmd_enter_tws_pair_mode_ex(cmd_id, 7); break;
        case 0x56: dc5v_io_cmd_enter_tws_pair_mode_ex(cmd_id, 8); break;
        case 0x57: dc5v_io_cmd_enter_tws_pair_mode_ex(cmd_id, 9); break;
        
        case 0x60: dc5v_io_cmd_clear_paired_list_ex(cmd_id, 0);   break;
        case 0x61: dc5v_io_cmd_clear_paired_list_ex(cmd_id, 1);   break;
        case 0x62: dc5v_io_cmd_enter_bqb_mode(3);                 break;
        case 0x63: dc5v_io_cmd_enter_tws_pair_mode_ex(cmd_id, 1); break;

        case 0x6a: dc5v_io_cmd_enter_ota_mode_ex(cmd_id, 0);      break;
        case 0x6b: dc5v_io_cmd_search_ble_adv_ex(cmd_id, 0);      break;

        case 0x64: dc5v_io_cmd_enter_pair_mode_ex(cmd_id);        break;
        case 0x65: dc5v_io_cmd_ctrl_power_off(cmd_id);            break;
        case 0x66: /* dc5v_io_cmd_ctrl_disconnect(); */  break;
        case 0x67: /* dc5v_io_cmd_enter_test_mode(0); */ break;
        case 0x68: /* dc5v_io_cmd_enter_test_mode(1); */ break;

        /* 充电盒命令
         */
        case 0x70: dc5v_io_cmd_chg_box_opened(0xff);   break;
        case 0x71: dc5v_io_cmd_chg_box_closed();       break;
        case 0x72: dc5v_io_cmd_chg_box_bat_low();      break;
        case 0x73: dc5v_io_cmd_chg_box_key_pressed(1); break;
        case 0x74: dc5v_io_cmd_chg_box_key_pressed(2); break;
        case 0x75: dc5v_io_cmd_chg_box_key_pressed(3); break;
        case 0x76: dc5v_io_cmd_chg_box_key_pressed(4); break;
    }

    if (cmd_id <= 0x3f)
    {
        dc5v_io_cmd_chg_box_bat_level_ex(cmd_id);
    }
}