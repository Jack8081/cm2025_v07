#include "system_app.h"
#include "dc5v_uart.h"

u32_t system_dc5v_io_last_time(void);

bool system_check_dc5v_cmd_pending(void)
{
    u32_t  ts;
    
    /* DC5V_IO 通讯中?
     */
    ts = system_dc5v_io_last_time();

    if (ts != 0 && (os_uptime_get_32() - ts) < 500)
    {
        return true;
    }

    /* DC5V_UART 通讯中?
     */
    if (dc5v_uart_operate(DC5V_UART_CHECK_IO_TIME, NULL, 500, 0))
    {
        return true;
    }
    
    return false;
}

void system_check_in_charger_box(void)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t   *sys_status = &manager->sys_status;

    u8_t  in_charger_box;

    if (power_manager_get_dc5v_status_chargebox() != DC5V_STATUS_OUT)
    {
        in_charger_box = 1;
    }
    else
    {
        in_charger_box = 0;

        /* 耳机取出后, 如果再放入充电盒,
         * 关盖前处于后台充电, 关盖后将可以进入前台充电
         */
        // manager->disable_front_charge = 0;
    }

    if (sys_status->in_charger_box != in_charger_box)
    {
        sys_status->in_charger_box = in_charger_box;
        
        bt_manager_tws_send_message
        (
            TWS_USER_APP_EVENT, TWS_EVENT_IN_CHARGER_BOX_STATE, &in_charger_box, 1
        );
    }
}

