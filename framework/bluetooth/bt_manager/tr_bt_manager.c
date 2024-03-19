/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <soc.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <sys_event.h>
#include <btservice_api.h>
#include <dvfs.h>
#include <acts_bluetooth/host_interface.h>
#include <property_manager.h>
#include <drivers/bluetooth/bt_drv.h>
#include <drivers/adc.h>
#include <drivers/cfg_drv/dev_config.h>
#include <app_manager.h>
#include <app_common.h>
#include <config_cuckoo.h>
#include <app_ui.h>

static tr_btmgr_base_config_t bt_manager_base_config;

tr_btmgr_base_config_t * tr_bt_manager_get_base_config(void)
{
    tr_btmgr_base_config_t*  base_config = &bt_manager_base_config;

    return base_config;
}

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
typedef struct {
    u8_t br_connect:1;
    u8_t br_pair_list:1;
    u8_t le_connect:1;
    u8_t le_pair_list:1;
    u8_t le_led_flush:1;
    u8_t br_led_flush:1;
    u8_t is_power_on:1;
    u8_t monitor_is_work:1;
	os_delayed_work bt_ui_monitor_work;
} bt_ui_monitor_t;

static bt_ui_monitor_t bt_ui_monitor;

extern bool btsrv_connect_is_reconnect_info_empty(void);
void bt_ui_monitor_work_handler(os_work *work)
{
    u8_t br_connect, le_connect;
    br_connect = !!bt_manager_get_connected_dev_num();
    le_connect = !!bt_manager_audio_lea_addr();
    
    if (bt_ui_monitor.is_power_on == 1) {
        ui_event_led_display(UI_EVENT_BT_DISCONNECTED_USB);//power on, lea is disconnect
        bt_ui_monitor.is_power_on = 0;
    }

	if(strcmp(app_manager_get_current_app(), APP_ID_BTMUSIC) == 0) {
        
        bt_ui_monitor.le_led_flush = 0;
        
        if (br_connect != bt_ui_monitor.br_connect) {
            bt_ui_monitor.br_connect = br_connect;
            
            SYS_LOG_INF("br %s\n", br_connect ? "connect" : "disconnect");
            
            bt_ui_monitor.br_led_flush = 1;
            
            if (br_connect) {
	            bt_manager_sys_event_notify(SYS_EVENT_BT_CONNECTED_BR);
            } else {
	            bt_manager_sys_event_notify(SYS_EVENT_BT_DISCONNECTED_BR);
            }

        } else if (!bt_ui_monitor.br_led_flush) { //btmusic le disconnect
            
            SYS_LOG_INF("flash br led %s\n", br_connect ? "connect" : "disconnect");
           
            bt_ui_monitor.br_led_flush = 1;

            if (br_connect) {
                ui_event_led_display(UI_EVENT_BT_CONNECTED_BR);
            } else {
                ui_event_led_display(UI_EVENT_BT_DISCONNECTED_BR);
            }
        }
    }
	
    if(strcmp(app_manager_get_current_app(), APP_ID_LE_AUDIO) == 0) {
        
        bt_ui_monitor.br_led_flush = 0;
        
        if (le_connect != bt_ui_monitor.le_connect) {
            bt_ui_monitor.le_connect = le_connect;
            
            SYS_LOG_INF("lea %s\n", le_connect ? "connect" : "disconnect");
            
            bt_ui_monitor.le_led_flush = 1;
            
            if (le_connect) {
	            bt_manager_sys_event_notify(SYS_EVENT_BT_CONNECTED_USB);
            } else {
	            bt_manager_sys_event_notify(SYS_EVENT_BT_DISCONNECTED_USB);
            }
        } else if (le_connect != bt_ui_monitor.br_connect && !bt_ui_monitor.le_led_flush) {
            SYS_LOG_INF("flash lea led %s\n", br_connect ? "connect" : "disconnect");
            
            bt_ui_monitor.le_led_flush = 1;
            
            if (le_connect) {
                ui_event_led_display(UI_EVENT_BT_CONNECTED_USB);
            } else {
                ui_event_led_display(UI_EVENT_BT_DISCONNECTED_USB);
            }
        }
    }
#if 0
    bt_addr_le_t tmp_addr;
    printk("leconnect %d   lepair %d\n\n", !!bt_manager_audio_lea_addr(), !bt_manager_audio_get_peer_addr(&tmp_addr));

    printk("br %d pair %d\n", bt_manager_get_connected_dev_num(), !btsrv_connect_is_reconnect_info_empty());
#endif
    os_delayed_work_submit(&bt_ui_monitor.bt_ui_monitor_work, 1000);
    bt_ui_monitor.monitor_is_work = 1;
}

void bt_ui_monitor_init(void)
{
	os_delayed_work_init(&bt_ui_monitor.bt_ui_monitor_work, bt_ui_monitor_work_handler);   
	os_delayed_work_submit(&bt_ui_monitor.bt_ui_monitor_work, 1000);
    bt_ui_monitor.is_power_on = 1;
}

void bt_ui_monitor_submit_work(int timeout)
{
    if(bt_ui_monitor.monitor_is_work) {
        os_delayed_work_cancel(&bt_ui_monitor.bt_ui_monitor_work);
        bt_ui_monitor.monitor_is_work = 0;
    }
    os_delayed_work_submit(&bt_ui_monitor.bt_ui_monitor_work, timeout);
}
#endif

#ifdef CONFIG_BT_BR_TRANSCEIVER
void tr_bt_manager_change_mac_addr_and_str(u8_t *mac_addr, u8_t *mac_str)
{

	if(sys_get_work_mode() == RECEIVER_MODE)
		return;

	if(mac_addr)
	{
		printk("mac_addr %x->", mac_addr[5]);
		mac_addr[5] += 1;
		printk("%x\n", mac_addr[5]);
	}

	if(mac_str)
	{
		u8_t mac = 0, tmp;

		printk("mac_str %c%c->", mac_str[10], mac_str[11]);
		mac = mac_str[10] >= 'A' ? mac_str[10] - 'A' + 10 : mac_str[10] - '0';
		mac <<= 4;

		mac += mac_str[11] >= 'A' ? mac_str[11] - 'A' + 10 : mac_str[11] - '0';

		mac += 1;

		tmp = mac >> 4;
		mac_str[10] = tmp >= 10 ? tmp - 10 + 'A' : tmp + '0';

		tmp = mac & 0x0f;
		mac_str[11] = tmp >= 10 ? tmp - 10 + 'A' : tmp + '0';
		printk("%c%c\n", mac_str[10], mac_str[11]);
	}
}

struct bt_mgr_dev_info *tr_bt_mgr_find_dev_info(bd_address_t *addr, u8_t *ids)
{

	int i;
    u8_t index = 0;
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();
	if(addr == NULL)
	{
		for (i = 0; i < MAX_MGR_DEV; i++) {
			if (bt_manager->dev[i].used) {
	            *ids = index;
				return &bt_manager->dev[i];
			}
	        index++;
		}
	}
	else
	{
		for (i = 0; i < MAX_MGR_DEV; i++) {
			if (bt_manager->dev[i].used && !memcmp(&bt_manager->dev[i].addr, addr, sizeof(bd_address_t))) {
	            *ids = index;
				return &bt_manager->dev[i];
			}
	        index++;
		}
	}
	return NULL;
}

u8_t tr_get_connect_index_by_addr(void *addr)
{

    u8_t index = 0;
    if(tr_bt_mgr_find_dev_info(addr, &index))
    {
        return index;
    }
    return 0xff;
}

bool tr_get_svr_status(void *addr, bt_dev_svr_info_t *dev_svr)
{

	u8_t index = 0;
    struct bt_mgr_dev_info *dev = tr_bt_mgr_find_dev_info(addr,  &index);
    if(dev)
    {
        dev_svr->hf_connected = dev->hf_connected;
        dev_svr->avrcp_connected = dev->avrcp_connected;
        dev_svr->a2dp_connected = dev->a2dp_connected;
        dev_svr->spp_connected = dev->spp_connected;
        dev_svr->hid_connected = dev->hid_connected;

		//dev_svr->avrcp_vol_sync = dev->avrcp_vol_sync;
        return true;
    }
    return false;
}

void tr_bt_manager_set_vol_sync_en(void *param)
{
/*
	u8_t index = 0;
    struct bt_mgr_dev_info *dev = NULL;
	dev = tr_bt_mgr_find_dev_info(param,  &index);
    if(dev)
    {
		dev->avrcp_vol_sync = 1;
	}*/
}

void tr_bt_manager_standby_prepare(void)
{/*
	int prio;

	prio = hostif_set_negative_prio();
	tr_sys_wake_lock_suspend(WAKELOCK_MESSAGE);
	tr_sys_wake_lock_suspend(WAKELOCK_BT_EVENT);
	tr_btif_br_scan_set_lowpowerable(true);

	tr_sys_wake_lock_resume(WAKELOCK_BT_EVENT);
	tr_sys_wake_lock_resume(WAKELOCK_MESSAGE);
	hostif_revert_prio(prio);*/
}

void tr_bt_manager_standby_post(void)
{
	tr_btif_br_scan_set_lowpowerable(false);
}

bool tr_check_dev_first_passthrough_play(bd_address_t *addr)
{/*
    struct bt_mgr_dev_info *dev =bt_mgr_find_dev_info(addr);
    if((dev->avrcp_first_pth_play || (k_uptime_get_32() - dev->avrcp_connect_time) > 4000))
    {
        dev->avrcp_first_pth_play = 1;
        return 0;
    }

    dev->avrcp_first_pth_play = 1;*/
    return 1;
}

void tr_bt_manager_set_disconnect_reason(int reason)
{
    //bt_mgr_info.dis_reason = reason;		/* Transfer to BT_STATUS_DISCONNECTED */
}

int tr_bt_manager_get_disconnect_reason(void)
{
   // return bt_mgr_info.dis_reason;		/* Transfer to BT_STATUS_DISCONNECTED */
   return 0;
}
#endif
