/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <device.h>
#include <soc.h>
#include <property_manager.h>
#include "bt_manager.h"

u8_t tr_work_mode;

u8_t sys_get_work_mode(void)
{
    return tr_work_mode;
}

void sys_set_work_mode(tr_app_work_mode_e mode)
{
    if(mode != tr_work_mode) {
        tr_work_mode = mode;
        property_set("TR_SYS_MODE", &tr_work_mode, 1);
    }
}

/*
void sys_set_switch_flag(void)
{
	u8_t switch_flag = 1;
    nvram_config_set("TR_SYS_SWITCH_FLAG", &switch_flag, 1);
}

u8_t sys_get_clear_switch_flag(void)
{
	u8_t switch_flag = 0;
	u8_t clear_switch_flag = 0;
    nvram_config_get("TR_SYS_SWITCH_FLAG", &switch_flag, 1);
    if (switch_flag != 0) {
        nvram_config_set("TR_SYS_SWITCH_FLAG", &clear_switch_flag, 1);
    }
	return switch_flag;
}

bool tr_app_switch_is_in_current_work_mode(const char *app_name)
{
	if(tr_get_app_type_by_name(app_name) == tr_work_mode) {
		return true;
	}

	return false;
}

uint8_t tr_get_current_work_mode_active_app_num(void)
{
	int i;
    uint8_t active_app_num = 0;

	for(i = 0; i < g_switch_ctx->app_num; i++)
	{
		struct app_switch_node_t *app_node = &g_switch_ctx->app_list[i];
		if( app_node->pluged )
        {
			if(tr_app_switch_is_in_current_work_mode(app_node->app_name) == false) {
				continue;
			}

            active_app_num++;
        }
	}

    return active_app_num;
}

void tr_app_switch_transceiver_mode(s8_t transceiver_mode)
{
	struct app_msg	msg = {0};

	msg.type = MSG_TR_RESTART_BT_SERVICE;
	msg.cmd = transceiver_mode == -1 ? !tr_work_mode : transceiver_mode;
	msg.value = 0xff;
	send_async_msg("main", &msg);
}
*/

void tr_app_switch_list_show(void)
{
    int i;

    for(i = 0; i < globle_switch_ctx.app_num; i++)
    {
        struct app_switch_node_t *app_node = &globle_switch_ctx.app_list[i];
        if( app_node->pluged )
        {
            printk( "%s ", app_node->app_name );
            if( !memcmp(app_manager_get_current_app(), app_node->app_name, strlen(app_node->app_name)) )
                printk("*\n");
            else
                printk("\n");
        }
    }
}

void tr_print_transceive_mode(void)
{
	if(sys_get_work_mode() == TRANSFER_MODE)
	{
		printk("\n");
		printk("**************************************\n");
		printk("**************************************\n");
		printk("********                        ******\n");
		printk("********  [ transmitter mode ]  ******\n");
		printk("********                        ******\n");
		printk("**************************************\n");
		printk("**************************************\n");
		printk("\n");
	}
	else
	{
		printk("\n");
		printk("**************************************\n");
		printk("**************************************\n");
		printk("*********                    *********\n");
		printk("*********  [ receive mode ]  *********\n");
		printk("*********                    *********\n");
		printk("**************************************\n");
		printk("**************************************\n");
		printk("\n");

	}
}

int tr_sys_work_mode_init(void)
{
    if(property_get("TR_SYS_MODE", &tr_work_mode, 1) < 0) {
        tr_btmgr_base_config_t *tr_base_config = tr_bt_manager_get_base_config();
	    SYS_LOG_INF("1SYS_MODE: %s\n", tr_work_mode ? "TRANS" : "RECEV");
        tr_work_mode = tr_base_config->trans_work_mode;
    }
    tr_print_transceive_mode();
    SYS_LOG_INF("SYS_MODE: %s\n", tr_work_mode ? "TRANS" : "RECEV");
	return 0;
}
