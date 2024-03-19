/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager a2dp profile.
 */
#define SYS_LOG_DOMAIN "bt manager"

//#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <shell/shell.h>
#include <stream.h>
#include <acts_ringbuf.h>
#include <bt_manager.h>
#include <sys_event.h>
#include <app_manager.h>
#include <power_manager.h>
#include <bt_manager_inner.h>
#include "audio_policy.h"
#include "app_switch.h"
#include "btservice_api.h"
#include "media_player.h"
#include <volume_manager.h>
#include <audio_system.h>
#include <msg_manager.h>
#include "api_bt_module.h"
#include <property_manager.h>
#include "drivers/nvram_config.h"
#include <acts_bluetooth/hci.h>
#include "acts_bluetooth/host_interface.h"

#include "tr_bt_manager_auto.c"
#include "tr_bt_manager_inquiry.c"
#include "tr_bt_manager_auto_shareme.c"

#define MAC_STR_LEN		(12+1)	/* 12(len) + 1(NULL) */
#define BT_NAME_LEN		(32+1)	/* 32(len) + 1(NULL) */

tr_bt_notify_callback tr_bt_event_cbk = NULL;
api_bt_module_t *api_bt_module_info = NULL;

void tr_bt_mgr_link_event(struct bt_link_cb_param *param)
{
	switch (param->link_event) {
        case BT_LINK_EV_ACL_CONNECTED:
            _bt_manager_trs_event_notify(BTSRV_TRS_CONNECTED, param->addr, 6);
		break;
	case BT_LINK_EV_ACL_DISCONNECTED:
            _bt_manager_trs_event_notify(BTSRV_TRS_DISCONNECTED, param->addr, 6);
		break;
	case BT_LINK_EV_GET_NAME:
		break;
	case BT_LINK_EV_HF_CONNECTED:
		break;
	case BT_LINK_EV_HF_DISCONNECTED:
		break;
	case BT_LINK_EV_A2DP_CONNECTED:
		_bt_manager_trs_event_notify(BTSRV_TRS_A2DP_CONNECTED, param->addr, 6);
		break;
	case BT_LINK_EV_A2DP_DISCONNECTED:
		_bt_manager_trs_event_notify(BTSRV_TRS_A2DP_DISCONNECTED, param->addr, 6);
		break;
	case BT_LINK_EV_AVRCP_CONNECTED:
		_bt_manager_trs_event_notify(BTSRV_TRS_AVRCP_CONNECTED, param->addr, 6);
		break;
	case BT_LINK_EV_AVRCP_DISCONNECTED:
		_bt_manager_trs_event_notify(BTSRV_TRS_AVRCP_DISCONNECTED, param->addr, 6);
		break;
	case BT_LINK_EV_SPP_CONNECTED:
		break;
	case BT_LINK_EV_SPP_DISCONNECTED:
		break;
	case BT_LINK_EV_HID_CONNECTED:
		break;
	case BT_LINK_EV_HID_DISCONNECTED:
		break;
	default:
		break;
	}
}

static void tr_bt_manager_inquiry_ctrl_display(void *param, int dev_cnt)
{
	u32_t   cod;
	inquiry_info_t * buf = (inquiry_info_t *)param;
	u8_t i;
	
	printk("=======================inquriy result:%d=====================\n", dev_cnt);
	for(i = 0; i < dev_cnt; i++)
	{
		cod = (buf + i)->cod[0] | ((buf + i)->cod[1]<< 8) | ((buf + i)->cod[2]<< 16);
		printk("discovery %02x:%02x:%02x:%02x:%02x:%02x RSSI %i COD %04x %s\n", \
				(buf + i)->addr[5],(buf + i)->addr[4],(buf + i)->addr[3],(buf + i)->addr[2],(buf + i)->addr[1],(buf + i)->addr[0],\
				(buf + i)->rssi, cod, (buf + i)->name);
	}
	printk("============================================================\n");

}

void _bt_manager_trs_event_notify(int event_type, void *param, int param_size)
{
    u8_t *addr = (u8_t *)param;
    inquiry_info_t * buf = (inquiry_info_t *)param;
    u32_t cod;

    if(!api_bt_module_info)
    {
        if(event_type != BTSRV_WAIT_PAIR)
            return;
    }

    switch (event_type) 
    {
        case BTSRV_WAIT_PAIR:
            SYS_LOG_INF("trs ready!");
            if(sys_get_work_mode() == TRANSFER_MODE)
                tr_bt_manager_trs_init();
            break;
        case BTSRV_TRS_INQUIRY_COMPLETE:
            SYS_LOG_INF("trs inquiry complete");
			bt_manager_set_status(BT_STATUS_INQUIRY_COMPLETE);
            tr_bt_manager_inquiry_ctrl_display(param, param_size);
            bt_manager_event_notify(BT_INQUIRY_COMPLETE, NULL, 0);
            if(api_bt_module_info->inquiry_cb)
            {
                api_bt_module_info->inquiry_cb(buf, param_size);
                api_bt_module_info->inquiry_cb = NULL;
                API_BT_INQUIRY_FREE();
                return;
            }
            break;
	case BTSRV_TRS_INQUIRY_RESULT:
		SYS_LOG_INF("trs inquiry result");
        cod = buf->cod[0] | (buf->cod[1]<< 8) | (buf->cod[2]<< 16);
        printk("discovery %02x:%02x:%02x:%02x:%02x:%02x RSSI %i COD %04x %s\n", \
                buf->addr[5],buf->addr[4],buf->addr[3],buf->addr[2],buf->addr[1],buf->addr[0],\
                buf->rssi, cod, buf->name);
            if(api_bt_module_info->inquiry_cb)
            {
                api_bt_module_info->inquiry_cb(buf, 0);
                return;
            }

        break;
    case BTSRV_TRS_CONNECTED_FAILED:
        SYS_LOG_INF("trs connected failed");
        printk("addr:%02x:%02x:%02x:%02x:%02x:%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        SYS_LOG_INF("acl disconnect 0x%x\n", param_size);
        tr_bt_manager_set_disconnect_reason(param_size);
        break;
    case BTSRV_TRS_CONNECTED_STOPED:
        SYS_LOG_INF("trs connected stoped");
        printk("addr:%02x:%02x:%02x:%02x:%02x:%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        if(!bt_mgr_find_dev_info((bd_address_t *)addr))
        {
            bt_manager_set_status(BT_STATUS_FAILD);
            bt_manager_event_notify(BT_DISCONNECTED, NULL, 0);
        }
#ifdef CONFIG_PROPERTY
        property_flush(NULL);
        property_flush_req_deal();
#endif
        if(!param_size)
            return;
        break;
    case BTSRV_TRS_DISCONNECTED:
        SYS_LOG_INF("trs disconnected");
        printk("addr:%02x:%02x:%02x:%02x:%02x:%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        break;
    case BTSRV_TRS_CANCEL_CONNECTED:
        SYS_LOG_INF("cancel connected %d", (int)param);
        if(api_bt_module_info->cannect_cancel_cb)
            api_bt_module_info->cannect_cancel_cb((int)param);
        break;
    case BTSRV_TRS_CONNECTED:
        SYS_LOG_INF("trs connected");
        printk("addr:%02x:%02x:%02x:%02x:%02x:%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        api_bt_connect(addr);
        break;
    case BTSRV_TRS_A2DP_CONNECTED:
        SYS_LOG_INF("trs a2dp connected");
        printk("addr:%02x:%02x:%02x:%02x:%02x:%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        bt_manager_event_notify(BT_A2DP_CONNECTION_EVENT, NULL, 0);
        break;
    case BTSRV_TRS_A2DP_DISCONNECTED:
		SYS_LOG_INF("trs a2dp disconnected");
        printk("addr:%02x:%02x:%02x:%02x:%02x:%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        bt_manager_event_notify(BT_A2DP_DISCONNECTION_EVENT, NULL, 0);
        break;
    case BTSRV_TRS_A2DP_OPEN:
		SYS_LOG_INF("trs a2dp open");
        bt_manager_event_notify(BT_A2DP_STREAM_OPEN_EVENT, NULL, 0);
        break;
    case BTSRV_TRS_A2DP_START:
		SYS_LOG_INF("trs a2dp start");
        bt_manager_set_status(BT_STATUS_PLAYING);
        bt_manager_event_notify(BT_A2DP_STREAM_START_EVENT, NULL, 0);
        break;
    case BTSRV_TRS_A2DP_SUSPEND:
		SYS_LOG_INF("trs a2dp suspend");
        bt_manager_set_status(BT_STATUS_PAUSED);
//        bt_manager_event_notify(BT_A2DP_STREAM_SUSPEND_EVENT, NULL, 0);
        break;
    case BTSRV_TRS_A2DP_CLOSE:
		SYS_LOG_INF("trs a2dp close");
        bt_manager_set_status(BT_STATUS_PAUSED);
        bt_manager_event_notify(BT_A2DP_STREAM_CLOSE_EVENT, NULL, 0);
        break;
    case BTSRV_TRS_RESTART_PLAY:
		SYS_LOG_INF("trs restart");
		break;
	case BTSRV_TRS_READY_PLAY:
		SYS_LOG_INF("trs ready");
		break;
	case BTSRV_TRS_EVENT_SYNC:
		break;
	case BTSRV_TRS_IRQ_CB:
        break;
	case BTSRV_TRS_UNPROC_PENDING_START_CB:
		SYS_LOG_INF("Pending start");
		break;
	case BTSRV_TRS_UPDATE_BTPLAY_MODE:
		SYS_LOG_INF("Slave actived mode\n");
		break;
	case BTSRV_TRS_UPDATE_PEER_VERSION:
		SYS_LOG_INF("peer_version");
		break;
	case BTSRV_TRS_SINK_START_PLAY:
		break;
	case BTSRV_TRS_PAIR_FAILED:
		SYS_LOG_INF("Tws pair failed");
		break;
	case BTSRV_TRS_AVRCP_CONNECTED:
		SYS_LOG_INF("avrcp connected");
		break;
	case BTSRV_TRS_AVRCP_DISCONNECTED:
		SYS_LOG_INF("avrcp disconnected");
		break;
	case BTSRV_TRS_AVRCP_SYNC_VOLUME:
		tr_bt_manager_set_vol_sync_en(param);
		
		SYS_LOG_INF("avrcp sync volume %d", param_size);
		break;
	case BTSRV_TRS_AVRCP_SYNC_VOLUME_ACCEPT:
		tr_bt_manager_set_vol_sync_en(param);
		SYS_LOG_INF("avrcp sync volume rmp accept %d", param_size);
		break;
	case BTSRV_TRS_AVRCP_PASSTHROUGH_CMD:
		SYS_LOG_INF("avrcp passthrough cmd op_id 0x%x", param_size);
		break;
    case BTSRV_TRS_BATTERY_LEVEL:
        SYS_LOG_INF("Battery level %d\n", (int)param);
        break;
	default:
		break;
	}
    if(tr_bt_event_cbk)
        tr_bt_event_cbk(event_type, param, param_size);
}

void tr_bt_manager_app_notify_reg(tr_bt_notify_callback cbk)
{
    tr_bt_event_cbk  = cbk;
}

void tr_bt_manager_trs_init(void)
{
    if(!api_bt_module_info)
        api_bt_module_info = mem_malloc(sizeof(api_bt_module_t));
    memset(api_bt_module_info, 0, sizeof(api_bt_module_t));
	tr_btif_trs_init(_bt_manager_trs_event_notify);
    SYS_LOG_INF("\n");
}

void tr_bt_manager_trs_deinit(void)
{
	tr_btif_trs_deinit();
    if(api_bt_module_info) {
        mem_free(api_bt_module_info);
        api_bt_module_info = NULL;
    }
}
/***********************************************************************************
 *      api
 ************************************************************************************/
int api_bt_init(evt_bt_cb_t* callback)
{   
    if(api_bt_module_info == NULL)
        return 0;

    api_bt_module_info->cb_state = callback->cb_state;
    api_bt_module_info->cb_hwready = callback->cb_hwready;
    api_bt_module_info->cb_avrcp = callback->cb_avrcp;
    api_bt_module_info->cb_absvolume = callback->cb_absvolume;
    return 1;
}

int api_bt_inquiry(int number, int timeout, u32_t cod, evt_bt_inquiry evtcb)
{
    if(api_bt_module_info == NULL)
    {
        return 0;
    }

#ifdef CONFIG_BT_BR_TRANSCEIVER
#else
    if(api_bt_module_info->disable)
    {
        SYS_LOG_INF("disable\n");
        return 0;
    }
#endif

    if(number)
    {
        SYS_LOG_INF("NUM: %d, LENTH: %d\n", number, timeout);
        bt_manager_set_status(BT_STATUS_INQUIRY);
        bt_manager_event_notify(BT_INQUIRY_START, NULL, 0);

        api_bt_module_info->cannect_cancel_cb = NULL;

        api_bt_module_info->inquiry_cb = evtcb;
    }
    return tr_btif_trs_inquiry(timeout, number, cod);
}

int api_bt_inquiry_cancel(void)
{
    tr_btif_trs_cancel_inquiry();
    return 1;
}

int api_bt_connect(unsigned char * bt_addr)
{
    bt_dev_svr_info_t dev;

    if(api_bt_module_info == NULL)
    {
        return 0;
    }

    if(api_bt_module_info->disable)
    {
        SYS_LOG_INF("disable\n");
        return 0;
    }

    if((tr_get_svr_status(bt_addr, &dev)) && (dev.a2dp_connected))
    {
        SYS_LOG_INF("dev connected\n");
        return -1;
    }

    tr_btif_trs_cancel_inquiry();
    k_sleep(K_MSEC(10));
    tr_bt_manager_connected(bt_addr);
    return 1;
}

int api_bt_connect_cancel(evt_bt_connect_cancel evtcb)
{
    api_bt_module_info->cannect_cancel_cb = evtcb;
	tr_bt_manager_connect_stop();
    return 1;
}

int api_bt_disconnect(int data_base_index)
{
    if(sys_get_work_mode())
    {
        tr_bt_manager_disconnect(data_base_index);
    }
    else
    {
        switch(data_base_index)
        {
            case 0:
                btif_br_disconnect_device(BTSRV_DISCONNECT_PHONE_MODE);
                break;
            case 1:
                btif_br_disconnect_device(BTSRV_DISCONNECT_TWS_MODE);
                break;
            default:
                btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);
                break;
        }
    }


    return 1;
}

int api_bt_del_PDL (int index)
{
    int ret = tr_btif_del_pairedlist(index);
    if(index == 0xff)
    {
        btif_br_clear_list(BTSRV_DEVICE_ALL);
    }
    k_sleep(K_MSEC(100));
#ifdef CONFIG_PROPERTY
    property_flush(NULL);
    property_flush_req_deal();
#endif
    return ret;
}

int api_bt_get_PDL(paired_dev_info_t * pdl, int index)
{
    return tr_btif_get_pairedlist(index, pdl);
}

int api_bt_disallowed_connect(void *addr, u8_t disallowed)
{
    if(!addr)
        return -1;

    int ret = tr_btif_disallowed_reconnect(addr, disallowed ? 1 : 0);
    if(ret)
    {
        return ret;
    }
#ifdef CONFIG_PROPERTY
    property_flush(NULL);
    property_flush_req_deal();
#endif
    return 0;
}

int api_bt_get_absvolume(void *addr)
{
    //return tr_bt_manager_avrcp_sync_vol_get_by_local();
    return 0;
}

int api_bt_set_absvolume(void *addr, int volume)
{
	int connected_dev_num = bt_manager_get_connected_dev_num();
	bt_dev_svr_info_t* dev_info;
	int ret = 0xff;
	
    dev_info = mem_malloc(sizeof(bt_dev_svr_info_t));
    memset(dev_info, 0, sizeof(bt_dev_svr_info_t));

	//mutiple need addr; single dev not need addr
	if((connected_dev_num >1) && (addr == NULL))
	{
		ret = 0xff;
		goto exit;
	}

	// not connected bt
    if(!tr_get_svr_status(addr, dev_info))
    {
        ret = 0xff;
		goto exit;
    }

	if(dev_info->avrcp_connected)
	{
		//ret = system_volume_set(tr_audio_in_get_audio_stream_type(app_manager_get_current_app()), _tr_bt_manager_avrcp_to_music_volume(volume),  false);
		if(ret == _tr_bt_manager_avrcp_to_music_volume(volume))
		{
			ret = volume;
			goto exit;
		}
		else
		{
			SYS_LOG_INF("set volume return abnormal, set absvolume %d, system_volume_set music vol %d, return %d\n", volume, _tr_bt_manager_avrcp_to_music_volume(volume), ret);
			ret = 0xff;
			goto exit;
		}
	}
	else
	{
		ret = 0x80;
		goto exit;
	}

exit:
	mem_free(dev_info);
	return ret;

}

int api_bt_get_remote_name(unsigned char *addr ,unsigned char * name, int bufsize)
{
    return tr_btif_get_remote_name(addr, name, bufsize);
}

enum BT_PLAY_STATUS api_bt_get_status(void)
{
    int state = bt_manager_hfp_get_status();

	if (!strcmp("btcall", app_manager_get_current_app())) {
        switch(state)
        {
            case BT_STATUS_SIRI:
            case BT_STATUS_INCOMING:
            case BT_STATUS_OUTGOING:
            case BT_STATUS_ONGOING:
            case BT_STATUS_MULTIPARTY:
            case BT_STATUS_3WAYIN:
                return state;
        }
    }
    return bt_manager_get_status();
}

bool api_bt_get_svr_status(void *addr, bt_dev_svr_info_t *dev_svr)
{
    return tr_get_svr_status(addr, dev_svr);
}

int api_bt_get_connect_dev_num(void)
{
    return bt_manager_get_connected_dev_num();
}

void str_to_hex(char *pbDest, char *pszSrc, int nLen);
int api_bt_get_local_btaddr(unsigned char *addr)
{
#ifdef CONFIG_PROPERTY
	int ret;
    u8_t mac_str[MAC_STR_LEN];
    memset(mac_str, 0, MAC_STR_LEN);

    if(sys_get_work_mode() == TRANSFER_MODE)
    {
        //ret = property_get(CFG_BT_MAC_T, mac_str, MAC_STR_LEN - 1);
        ret = property_get(CFG_BT_MAC, mac_str, MAC_STR_LEN - 1);
    }
    else
    {
        ret = property_get(CFG_BT_MAC, mac_str, MAC_STR_LEN - 1);
    }
	
    if (ret < MAC_STR_LEN - 1)
    {
		return (u8_t)ret;
	}
    str_to_hex(addr, mac_str, 6);
#endif
    return 0;
}

extern void hex_to_str(char *pszDest, char *pbSrc, int nLen);
int api_bt_set_local_btaddr(unsigned char *addr)
{
#ifdef CONFIG_PROPERTY
	int ret;
    u8_t mac_str[MAC_STR_LEN];
    memset(mac_str, 0, MAC_STR_LEN);

    hex_to_str((char *)mac_str, (char *)addr, 6);
    
    if(sys_get_work_mode() == TRANSFER_MODE)
    {
        //ret = property_set(CFG_BT_MAC_T, mac_str, MAC_STR_LEN - 1);
        //property_flush(CFG_BT_MAC_T);
        ret = property_set(CFG_BT_MAC, mac_str, MAC_STR_LEN - 1);
        property_flush(CFG_BT_MAC);
    }
    else
    {
        ret = property_set(CFG_BT_MAC, mac_str, MAC_STR_LEN - 1);
        property_flush(CFG_BT_MAC);
    }
	
    if (ret < MAC_STR_LEN - 1)
    {
		return (u8_t)ret;
	}
#endif
    return 0;
}
int api_bt_get_local_name(unsigned char *buf)
{
	u8_t len;
#ifdef CONFIG_PROPERTY
	int ret;

    if(sys_get_work_mode() == TRANSFER_MODE)
    {
        //ret = property_get(CFG_BT_NAME_T, buf, BT_NAME_LEN);
        ret = property_get(CFG_BT_NAME, buf, BT_NAME_LEN);
    }
    else
    {
        ret = property_get(CFG_BT_NAME, buf, BT_NAME_LEN);
    }

	if (ret) {
		return (u8_t)ret;
	}
#endif

    len = strlen("ZS2851AH");
	memcpy(buf, "ZS2851AH", len);
    buf[len] = 0;
	return len;
}

void api_bt_a2dp_mute(u8_t enable)
{
    tr_btif_a2dp_mute(enable);
}

void api_bt_auto_move_keep_stop(bool need_to_disconn)
{
    if(!api_bt_module_info)
    {
        return;
    }

    if(api_bt_module_info->disable)
    {
        return;
    }

    api_bt_module_info->disable = 1;

	tr_bt_manager_auto_move_keep_stop(need_to_disconn);
}

void api_bt_auto_move_restart_from_stop(void)
{
    if(!api_bt_module_info)
    {
        return;
    }

    if(api_bt_module_info->disable)
    {
        api_bt_module_info->disable = 0;
        tr_bt_manager_auto_move_restart_from_stop();
    }
}

bool api_bt_get_disable_status(void)
{
    if(api_bt_module_info && !api_bt_module_info->disable)
    {
        return 0;
    }

    return 1;
}

u8_t api_bt_connect_index_by_addr(unsigned char *addr)
{
    return tr_get_connect_index_by_addr(addr);
}
