/*!
 * \file      system_dc5v_uart.c
 * \brief     DC5V_UART communication
 * \details   
 * \author    
 * \date      
 * \copyright Actions
 */

#include "system_app.h"
#include "bt_manager.h"

#include "dc5v_uart.h"
#include "app_config.h"
#include "system_dc5v_io_cmd.h"
#include "tool_app.h"


#define CHG_BOX_CMD_MAGIC_ID1  0x07
#define CHG_BOX_CMD_MAGIC_ID2  0x1E

#define CHG_BOX_CMD_RX_BYTE_TIMEOUT_MS   10
#define CHG_BOX_CMD_TX_BYTE_INTERVAL_MS  2
#define CHG_BOX_CMD_REPLY_WAIT_MS        2

#define CHG_BOX_CMD_MAX_PARAM_LEN  40

#define CHG_BOX_CMD_MAX_TOTAL_LEN  \
    (sizeof(chg_box_cmd_head_t) + CHG_BOX_CMD_MAX_PARAM_LEN + 1)


static inline u8_t CHG_BOX_CMD_CRC(u8_t* data, int len)
{
    u8_t  crc = 0;
    int   i;

    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
    }

    return crc;
}


static inline bool CHG_BOX_CMD_CHECK_DIRECTION(u8_t direction)
{
    audio_manager_configs_t*  cfg = audio_channel_get_config();

    u8_t  dev_channel = (cfg->hw_sel_dev_channel | cfg->sw_sel_dev_channel);

    if ((direction & 0xF0) != 0x00)
    {
        return NO;
    }

    if (direction == 0x01 &&
        dev_channel == AUDIO_DEVICE_CHANNEL_R)
    {
        return NO;
    }

    if (direction == 0x02 &&
        dev_channel == AUDIO_DEVICE_CHANNEL_L)
    {
        return NO;
    }

    return YES;
}


static inline u8_t CHG_BOX_CMD_REPLY_DIRECTION(void)
{
    audio_manager_configs_t*  cfg = audio_channel_get_config();

    u8_t  dev_channel = (cfg->hw_sel_dev_channel | cfg->sw_sel_dev_channel);

    switch (dev_channel)
    {
        case AUDIO_DEVICE_CHANNEL_L:  return 0x61;
        case AUDIO_DEVICE_CHANNEL_R:  return 0x62;
    }

    return 0x60;
}


typedef struct
{
    u8_t  magic_id1;
    u8_t  magic_id2;
    u8_t  direction;
    u8_t  cmd_id;
    u8_t  param_len;
    
} __packed chg_box_cmd_head_t;


typedef struct
{
    u8_t  cmd_id;
    u8_t  param[1];
    u8_t  complete;
    u8_t  ongoing;
    
} dc5v_uart_cmd_context_t;


typedef struct
{
    u32_t reserved;
} manager_dc5v_uart_ctrl_t;


manager_dc5v_uart_ctrl_t*  manager_dc5v_uart_ctrl;

static void dc5v_uart_delay_proc(MSG_CALLBAK fn, void *arg)
{
    struct app_msg msg = {0};

    msg.type = MSG_APP_DC5V_IO_CMD;
	msg.cmd = 0xff;
    msg.ptr = arg;
    msg.callback = fn;
	send_async_msg("main", &msg);
}

void dc5v_uart_chg_box_cmd_reply(uint32_t cmd_id, void* param, int param_len)
{
    u8_t  data[CHG_BOX_CMD_MAX_TOTAL_LEN];
    int   len;
    int   i;
    
    chg_box_cmd_head_t*  head = (void*)data;
    
    printk("id: 0x%x", cmd_id);

    memset(data, 0, sizeof(data));

    head->magic_id1 = CHG_BOX_CMD_MAGIC_ID1;
    head->magic_id2 = CHG_BOX_CMD_MAGIC_ID2;
    head->direction = CHG_BOX_CMD_REPLY_DIRECTION();
    head->cmd_id    = cmd_id;
    head->param_len = param_len;

    len = sizeof(chg_box_cmd_head_t);

    memcpy(&data[len], param, param_len);
    len += param_len;

    data[len] = CHG_BOX_CMD_CRC(data, len);
    len += 1;

    os_sleep(CHG_BOX_CMD_REPLY_WAIT_MS);
    
    //print_hex("DC5V_UART tx:", data, len);

    for (i = 0; i < len; i++)
    {
        dc5v_uart_operate(DC5V_UART_WRITE, &data[i], 1, 0);

        os_sleep(CHG_BOX_CMD_TX_BYTE_INTERVAL_MS);
    }
}


static inline void dc5v_uart_chg_box_cmd_reply_ok(uint32_t cmd_id)
{
    u8_t  ok = 0x00;
    
    dc5v_uart_chg_box_cmd_reply(cmd_id, &ok, 1);
}


void dc5v_uart_cmd_enter_bqb_mode(uint32_t cmd_id, uint32_t bqb_mode)
{
    static dc5v_uart_cmd_context_t  cmd;

    SYS_LOG_INF("%d", cmd_id);

    if (cmd.complete)
    {
        dc5v_uart_chg_box_cmd_reply_ok(cmd_id);
        return;
    }
    cmd.complete = YES;

    dc5v_io_cmd_enter_bqb_mode(bqb_mode);
}

void dc5v_uart_cmd_complete(struct app_msg *msg, int result, void *reply)
{
    system_dc5v_io_cmd_complete(YES);
}

void dc5v_uart_cmd_clear_paired_list(struct app_msg *msg, int result, void *reply)
{
    dc5v_uart_cmd_context_t* cmd = (dc5v_uart_cmd_context_t*)msg->ptr;
    
    dc5v_io_cmd_clear_paired_list(cmd->param[0]);

    cmd->complete = YES;

    system_dc5v_io_cmd_complete(YES);
}

void dc5v_uart_cmd_clear_paired_list_ex(uint32_t cmd_id, uint32_t clear_tws)
{
    static dc5v_uart_cmd_context_t  cmd;
    
    SYS_LOG_INF("%d", clear_tws);

    if (cmd.complete)
    {
        dc5v_uart_chg_box_cmd_reply_ok(cmd_id);
        return;
    }

    cmd.cmd_id   = cmd_id;
    cmd.param[0] = clear_tws;
    
    dc5v_uart_delay_proc(dc5v_uart_cmd_clear_paired_list, &cmd);
}


void dc5v_uart_cmd_enter_tws_pair_mode(struct app_msg *msg, int result, void *reply)
{
    dc5v_uart_cmd_context_t* cmd = (dc5v_uart_cmd_context_t*)msg->ptr;
    int state;

    state = dc5v_io_cmd_enter_tws_pair_mode(cmd->param[0]);

    if (state == TWS_PAIR_OK)
    {
        cmd->complete = YES;
        
        system_dc5v_io_cmd_complete(YES);
    }
    else if (state == TWS_PAIR_ABORT)
    {
        system_app_enter_poweroff(0);
    }
}


void dc5v_uart_cmd_enter_tws_pair_mode_ex(uint32_t cmd_id, uint32_t keyword)
{
    static dc5v_uart_cmd_context_t  cmd;
    
    SYS_LOG_INF("0x%x", keyword);

    if (!bt_manager_is_ready())
        return;

    if (cmd.complete)
    {
        dc5v_uart_chg_box_cmd_reply_ok(cmd_id);
        return;
    }

    cmd.cmd_id   = cmd_id;
    cmd.param[0] = keyword;
    
    dc5v_uart_delay_proc(dc5v_uart_cmd_enter_tws_pair_mode, &cmd);
}


void dc5v_uart_cmd_power_off(uint32_t cmd_id)
{
    static dc5v_uart_cmd_context_t  cmd;
    
    SYS_LOG_INF("%d", cmd_id);

    if (cmd.complete)
    {
        dc5v_uart_chg_box_cmd_reply_ok(cmd_id);
        return;
    }
    
    cmd.complete = YES;

    dc5v_uart_delay_proc(dc5v_uart_cmd_complete, NULL);

}


void dc5v_uart_cmd_enter_pair_mode(struct app_msg *msg, int result, void *reply)
{
    dc5v_uart_cmd_context_t* cmd = msg->ptr;

    dc5v_io_cmd_enter_pair_mode();

    cmd->complete = YES;

    system_dc5v_io_cmd_complete(YES);
}


void dc5v_uart_cmd_enter_pair_mode_ex(uint32_t cmd_id)
{
    static dc5v_uart_cmd_context_t  cmd;
    
    SYS_LOG_INF("");

    if (cmd.complete)
    {
        dc5v_uart_chg_box_cmd_reply_ok(cmd_id);
        return;
    }

    cmd.cmd_id = cmd_id;
    
    dc5v_uart_delay_proc(dc5v_uart_cmd_enter_pair_mode, &cmd);
}


void dc5v_uart_cmd_enter_ota_mode(struct app_msg *msg, int result, void *reply)
{
    dc5v_io_cmd_enter_ota_mode();
}


void dc5v_uart_cmd_enter_ota_mode_ex(uint32_t cmd_id)
{
    static dc5v_uart_cmd_context_t  cmd;
    
    if (cmd.complete)
    {
        dc5v_uart_chg_box_cmd_reply_ok(cmd_id);
        return;
    }
    
    cmd.complete = YES;
    
}

void dc5v_uart_cmd_search_ble_adv(struct app_msg *msg, int result, void *reply)
{
    dc5v_uart_cmd_context_t* cmd = msg->ptr;
    system_app_context_t*  manager = system_app_get_context();
    
    dc5v_io_cmd_search_ble_adv();

    if (thread_timer_is_running(&manager->dc5v_io_timer))
    {
        SYS_LOG_ERR("no timer");
        return;
    }

    thread_timer_init(&manager->dc5v_io_timer, check_dc5v_io_cmd_search_ble_adv_finish_timer, &cmd->complete);
    thread_timer_start(&manager->dc5v_io_timer, 0, 50);
}

void dc5v_uart_cmd_search_ble_adv_ex(uint32_t cmd_id)
{
    static dc5v_uart_cmd_context_t  cmd;
    
    if (cmd.complete)
    {
        dc5v_uart_chg_box_cmd_reply_ok(cmd_id);
        return;
    }
    if (cmd.ongoing)
    {
        return;
    }
    cmd.ongoing = YES;
    
    dc5v_uart_delay_proc(dc5v_uart_cmd_search_ble_adv, &cmd);
}

enum {
    PT_CFO_STATUS_OK         = 0x00,
    PT_CFO_STATUS_RUNNING     = 0x01,
    PT_CFO_STATUS_FAIL       = 0x02,
    PT_CFO_STATUS_NONE       = 0x03,
};

#define TEST_PASS  0
#define CFO_PARAM_ADJUST_SIZE 				(26)

extern int act_test_cfo_adjust(void *arg_buffer, u32_t arg_len);

u8_t pt_cfo_status = PT_CFO_STATUS_NONE;

const u8_t cfo_adjust_param[108] = {
    0x0D, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0xF6, 0xFF, 0xFF, 0xFF, 
    0x04, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0x58, 0x1B, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0xBA, 0xFF, 0xFF, 0xFF, 
    0x04, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0xD0, 0x07, 0x00, 0x00,
};

void pt_cfo_adjust_param_deal(void * param, void * adjust)
{
    s32_t * t = (s32_t *)param;
    s16_t * s = (s16_t *)adjust;

    int i;
    
    for (i=0; i<CFO_PARAM_ADJUST_SIZE/2; i++)
    {
        t[1 + 2*i + 1] = (s32_t)s[i];
    }
}

int pt_cfo_check_status(void)
{
    SYS_LOG_INF("%d", pt_cfo_status);
    
    return (pt_cfo_status);
}

void pt_cfo_set_status(int status)
{
    pt_cfo_status = status;
    SYS_LOG_INF("%d", pt_cfo_status);
}

void pt_cfo_adjust(void)
{
    int ret_val;
    u8_t*  cfo_buf = app_mem_malloc(sizeof(cfo_adjust_param));
	if (!cfo_buf) return;
	
    memcpy(cfo_buf, cfo_adjust_param, sizeof(cfo_adjust_param));

    u8_t * cfo_param_adj = app_mem_malloc(CFO_PARAM_ADJUST_SIZE);
	if (!cfo_param_adj) return;
	
    if (property_get(CFG_PT_CFO_ADJUST_PARAM, cfo_param_adj, CFO_PARAM_ADJUST_SIZE) >= 0)
    {
        pt_cfo_adjust_param_deal(cfo_buf, cfo_param_adj);
    }
    app_mem_free(cfo_param_adj);

    SYS_LOG_INF("");
    
    tool_init("att", 3);
    os_sleep(10);
    pt_cfo_set_status(PT_CFO_STATUS_RUNNING);
    // ret_val = (int)act_test_cfo_adjust((void *)cfo_buf, sizeof(cfo_adjust_param));
    ret_val = 1;
    SYS_LOG_INF("result = %d", ret_val);
    
    if (TEST_PASS == ret_val)
        pt_cfo_set_status(PT_CFO_STATUS_OK);
    else
        pt_cfo_set_status(PT_CFO_STATUS_FAIL);
    app_mem_free(cfo_buf);
}

static void dc5v_uart_cmd_wait_cfo_adjust_finish(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    u8_t cfo_status;

    do
    {
        cfo_status = pt_cfo_check_status();
        if (cfo_status != PT_CFO_STATUS_OK)
            break;

        thread_timer_stop(ttimer);

        system_dc5v_io_cmd_complete(YES);

    }while (0);
}

void dc5v_uart_cmd_cfo_adjust(struct app_msg *msg, int result, void *reply)
{
    system_app_context_t*  manager = system_app_get_context();   

    if (thread_timer_is_running(&manager->dc5v_io_timer))
    {
        SYS_LOG_ERR("no timer");
        return;
    }

    thread_timer_init(&manager->dc5v_io_timer, dc5v_uart_cmd_wait_cfo_adjust_finish, NULL);
    thread_timer_start(&manager->dc5v_io_timer, 0, 500);

}

void dc5v_uart_cmd_cfo_adjust_ex(uint32_t cmd_id, u8_t * param)
{
    static dc5v_uart_cmd_context_t  cmd;
    u8_t cfo_status = pt_cfo_check_status();

    SYS_LOG_INF("%d", cfo_status);

    if (cfo_status == PT_CFO_STATUS_NONE)
    {
        property_set(CFG_PT_CFO_ADJUST_PARAM, param, CFO_PARAM_ADJUST_SIZE);
        system_power_reboot(REBOOT_TYPE_GOTO_SYSTEM|REBOOT_REASON_PRODUCTION_TEST_CFO_ADJUST);
        return;
    }
    
    if (cfo_status == PT_CFO_STATUS_OK)
    {
        dc5v_uart_chg_box_cmd_reply_ok(cmd_id);
    }

    if (cmd.complete)
        return;
    
    cmd.complete = YES;

    dc5v_uart_delay_proc(dc5v_uart_cmd_cfo_adjust, NULL);
}


u8_t dc5v_uart_get_local_bat_level(void)
{
    system_status_t*  sys_status = &system_app_get_context()->sys_status;
#ifdef CONFIG_POWER_MANAGER
    u8_t  percent   = power_manager_get_battery_capacity_local();
#else
    u8_t  percent   = 100;
#endif
    u8_t  chg_state = sys_status->charge_state;
    u8_t  bat_level;

    if (chg_state != 0 &&
        percent >= 100)
    {
        percent = 99;
    }

    bat_level = (chg_state << 7) | percent;

    return bat_level;
}

void dc5v_uart_cmd_chg_box_opened(uint32_t cmd_id, uint32_t chg_box_bat_level)
{
    system_app_context_t*  manager = system_app_get_context();

    if (!manager->sys_status.in_front_charge)
    {
        dc5v_uart_chg_box_cmd_reply_ok(cmd_id);
    }

    dc5v_io_cmd_chg_box_opened(chg_box_bat_level);
}


void dc5v_uart_cmd_chg_box_bat_level(uint32_t cmd_id, uint32_t chg_box_bat_level)
{
    system_app_context_t*  manager = system_app_get_context();

    if (!manager->sys_status.in_front_charge)
    {
        u8_t  local_bat_level = dc5v_uart_get_local_bat_level();

        dc5v_uart_chg_box_cmd_reply(cmd_id, &local_bat_level, 1);
    }
    
    dc5v_io_cmd_chg_box_opened(chg_box_bat_level);
}


void dc5v_uart_cmd_chg_box_bat_query(uint32_t cmd_id, uint32_t chg_box_bat_level)
{
    system_status_t*  sys_status = &system_app_get_context()->sys_status;

    u8_t  local_bat_level = dc5v_uart_get_local_bat_level();

    if (chg_box_bat_level != 0xff)
    {
        sys_status->charger_box_bat_level = chg_box_bat_level;
    }

    SYS_LOG_INF("0x%x, 0x%x", chg_box_bat_level, local_bat_level);

    dc5v_uart_chg_box_cmd_reply(cmd_id, &local_bat_level, 1);
}


void dc5v_uart_cmd_chg_box_ctrl_powoff(uint32_t cmd_id)
{
    system_app_context_t*  manager = system_app_get_context();

    SYS_LOG_INF("");

    dc5v_uart_chg_box_cmd_reply_ok(cmd_id);

    manager->chg_box_ctrl_powoff = YES;
}

void dc5v_uart_cmd_chg_box_closed(uint32_t cmd_id)
{
    dc5v_uart_chg_box_cmd_reply_ok(cmd_id);

    dc5v_io_cmd_chg_box_closed();
}


void dc5v_uart_cmd_chg_box_bat_low(uint32_t cmd_id)
{
    dc5v_uart_chg_box_cmd_reply_ok(cmd_id);

    dc5v_io_cmd_chg_box_bat_low();
}


void dc5v_uart_cmd_chg_box_key_pressed(uint32_t cmd_id, uint32_t pressed)
{
    dc5v_uart_chg_box_cmd_reply(cmd_id, &pressed, 1);


    dc5v_io_cmd_chg_box_key_pressed(pressed);
}


void dc5v_uart_cmd_ota_board_connect(uint32_t cmd_id)
{
    SYS_LOG_INF("");

    if (bt_manager_is_ready())
    {
        dc5v_uart_chg_box_cmd_reply_ok(cmd_id);
    }
}

void dc5v_uart_cmd_ota_board_get_info(uint32_t cmd_id)
{
    u8_t bt_info[6+CFG_MAX_BT_DEV_NAME_LEN];
    u8_t temp1[12];
    u8_t temp2[6];
    int i;

    btmgr_base_config_t *p = bt_manager_get_base_config();
    
    property_get(CFG_BT_MAC, temp1, 12);
    hex2bin(temp1, 12, temp2, 6);
    for (i=0; i<6; i++)
        bt_info[i] = temp2[5-i];
    memcpy(&bt_info[6], p->bt_device_name, sizeof(p->bt_device_name));

    printk
    (
        "%s, %02X:%02X:%02X:%02X:%02X:%02X\n\n\n", 
        &bt_info[6],
        bt_info[5], bt_info[4], bt_info[3],
        bt_info[2], bt_info[1], bt_info[0]
    ); 

    dc5v_uart_chg_box_cmd_reply
    (
        cmd_id, bt_info, 6+CFG_MAX_BT_DEV_NAME_LEN
    );
}



/**
**	DC5V Uart 充电盒命令处理
**/
void dc5v_uart_chg_box_cmd_proc(u8_t cmd_id, u8_t* param)
{
    printk("0x%x", cmd_id);

    switch (cmd_id)
    {
        case 0x16: dc5v_uart_cmd_enter_bqb_mode(cmd_id, 3);                break;
        case 0x1a: dc5v_uart_cmd_enter_tws_pair_mode_ex(cmd_id, 1);        break;
        case 0x50: dc5v_uart_cmd_enter_tws_pair_mode_ex(cmd_id, param ? param[0] : 0xff); break;
        case 0x60: dc5v_uart_cmd_clear_paired_list_ex(cmd_id, param ? param[0] : 0xff);   break;
        case 0x20: dc5v_uart_cmd_power_off(cmd_id);                        break;
        case 0x21: dc5v_uart_cmd_enter_ota_mode_ex(cmd_id);                break;
        case 0x22: dc5v_uart_cmd_search_ble_adv_ex(cmd_id);                break;
        case 0x23: dc5v_uart_cmd_enter_pair_mode_ex(cmd_id);               break;
        case 0x28: dc5v_uart_cmd_cfo_adjust_ex(cmd_id, param);             break;

        case 0x11: dc5v_uart_cmd_chg_box_opened(cmd_id, param ? param[0] : 0xff);    break;
        case 0x12: dc5v_uart_cmd_chg_box_closed(cmd_id);                             break;
        case 0x13: dc5v_uart_cmd_chg_box_ctrl_powoff(cmd_id);                        break;
        case 0x14: dc5v_uart_cmd_chg_box_bat_low(cmd_id);                            break;
        case 0x1b: dc5v_uart_cmd_chg_box_bat_level(cmd_id, param ? param[0] : 0xff);                break;
        case 0x1c: dc5v_uart_cmd_chg_box_bat_query(cmd_id, param ? param[0] : 0xff); break;
        case 0x73: dc5v_uart_cmd_chg_box_key_pressed(cmd_id, param ? param[0] : 0xff);              break;
        
        case 0x31: dc5v_uart_cmd_ota_board_connect(cmd_id);                break;
        case 0x32: dc5v_uart_cmd_ota_board_get_info(cmd_id);               break;
    }
}



/**
**	DC5V Uart 收到的RX数据处理， 充电盒命令处理
**/
bool dc5v_uart_chg_box_rx_data_handler(uint8_t byte)
{
    u8_t  data[CHG_BOX_CMD_MAX_TOTAL_LEN];
    int   len;
    int   n;

    chg_box_cmd_head_t*  head = (void*)data;
    const char*  err_str = NULL;

    memset(data, 0, sizeof(data));

    head->magic_id1 = byte;
    
    len = 1;
    n = sizeof(chg_box_cmd_head_t) - 1;

    //先读取命令头
    n = dc5v_uart_operate
    (
        DC5V_UART_READ, &data[len], n, n * CHG_BOX_CMD_RX_BYTE_TIMEOUT_MS
    );
    
    len += n;

    if (n != sizeof(chg_box_cmd_head_t) - 1 ||
        head->magic_id2 != CHG_BOX_CMD_MAGIC_ID2 ||
        head->param_len >  CHG_BOX_CMD_MAX_PARAM_LEN)
    {
        err_str = "ERR_HEAD";
        goto err;
    }

    n = head->param_len + 1;

    //读取参数以及1个字节的CRC
    n = dc5v_uart_operate
    (
        DC5V_UART_READ, &data[len], n, n * CHG_BOX_CMD_RX_BYTE_TIMEOUT_MS
    );
    
    len += n;

    if (n != head->param_len + 1)
    {
        err_str = "ERR_PARAM";
        goto err;
    }

    if (CHG_BOX_CMD_CRC(data, len - 1) != data[len - 1])
    {
        err_str = "ERR_CRC";
        goto err;
    }

    if (!CHG_BOX_CMD_CHECK_DIRECTION(head->direction))
    {
        err_str = "ERR_DIR";
        goto err;
    }

	//打印收到的uart数据
    //print_hex("DC5V_UART rx:", data, len);

    //数据校验通过，处理命令
    dc5v_uart_chg_box_cmd_proc
    (
        head->cmd_id, 
        (head->param_len != 0) ? (&data[sizeof(chg_box_cmd_head_t)]) : NULL
    );
    
    return YES;

err:
    //print_hex("DC5V_UART rx:", data, len);

	SYS_LOG_ERR("Err: %s", err_str);
    return NO;
}


bool manager_dc5v_uart_rx_data_handler(u8_t byte)
{

    SYS_LOG_DBG("DC5V uart RX: 0x%x", byte);

    if (byte == CHG_BOX_CMD_MAGIC_ID1)
    {
        SYS_LOG_INF("ChargeBox CMD!\n\n");
		
        dc5v_uart_chg_box_rx_data_handler(byte);
        
        return YES;
    }

    return NO;
}


bool system_dc5v_uart_init(void)
{
    manager_dc5v_uart_ctrl_t*  p = manager_dc5v_uart_ctrl;
    
    CFG_Type_DC5V_UART_Comm_Settings  cfg;
    
    app_config_read
    (
        CFG_ID_CHARGER_BOX,
        &cfg,
        offsetof(CFG_Struct_Charger_Box, DC5V_UART_Comm_Settings),
        sizeof(CFG_Type_DC5V_UART_Comm_Settings)
    );	
    
    if (dc5v_uart_operate(DC5V_UART_INIT, &cfg, 0, 0) != YES)
    {
        return NO;
    }
    
    manager_dc5v_uart_ctrl =
        (p = app_mem_malloc(sizeof(manager_dc5v_uart_ctrl_t)));

	if (!p) return NO;

    //设置DC5V Uart收到的RX数据处理函数
    dc5v_uart_operate
    (
        DC5V_UART_SET_RX_DATA_HANDLER, 
        manager_dc5v_uart_rx_data_handler, 0, 0
    );

    //run rx data deal
    dc5v_uart_operate
    (
        DC5V_UART_RUN_RXDEAL, 
        0, 0, 0
    );

    return YES;
}
