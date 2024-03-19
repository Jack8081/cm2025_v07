/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager hfp ag profile.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>
#include <media_type.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <bt_manager.h>
#include <power_manager.h>
#include <app_manager.h>
#include <sys_event.h>
#include <mem_manager.h>
#include "bt_manager_inner.h"
#include <assert.h>
#include "app_switch.h"
#include "btservice_api.h"
#include <audio_policy.h>
#include <audio_system.h>
#include <volume_manager.h>
#ifdef CONFIG_BT_BR_TRANSCEIVER
#include <acts_bluetooth/host_interface.h>
#endif

struct bt_manager_hfp_ag_info_t {
	uint8_t hfp_codec_id;
	uint8_t hfp_sample_rate;
	uint32_t hfp_status;
	uint8_t connected:1;
	uint8_t call:1;
#ifdef CONFIG_BT_BR_TRANSCEIVER
	uint8_t call_setup:3;
	uint8_t call_held:3;
#else
	uint8_t call_setup:1;
	uint8_t call_held:1;
#endif
#ifdef CONFIG_BT_BR_TRANSCEIVER
    u16_t dail_memory;
    u32_t sco_connect_time;
    u8_t cur_clcc;
    u8_t sco_auto_start;
#endif
};

static struct bt_manager_hfp_ag_info_t hfp_ag_manager;

int bt_manager_hfp_ag_send_event(uint8_t *event, uint16_t len)
{
	return btif_hfp_ag_send_event(event,len);
}

static void _bt_manager_hfp_ag_callback(btsrv_hfp_event_e event, uint8_t *param, int param_size)
{
	switch (event) {
	case BTSRV_HFP_CONNECTED:
	{
#ifdef CONFIG_BT_BR_TRANSCEIVER
        if(!hfp_ag_manager.connected)
        {
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_SERVICE_IND, 1);
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_BATTERY_IND, 5);
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_SINGNAL_IND, 5);
        }
#endif
		hfp_ag_manager.connected = 1;
#ifdef CONFIG_BT_BR_TRANSCEIVER
        if (hostif_bt_hfp_ag_get_indicator(BTSRV_HFP_AG_CALL_IND) == 1 && hfp_ag_manager.sco_auto_start == 1) {
            btif_pts_hfp_active_connect_sco();
        }
#endif
		SYS_LOG_INF("hfp ag connected\n");
		break;
	}
	case BTSRV_HFP_DISCONNECTED:
	{
		hfp_ag_manager.connected = 0;
		SYS_LOG_INF("hfp ag disconnected\n");
		break;
	}
#ifdef CONFIG_BT_BR_TRANSCEIVER
    case BTSRV_SCO_CONNECTED:
    {
        SYS_LOG_INF("hfp sco connected\n");
        hfp_ag_manager.sco_connect_time = os_uptime_get_32();
        bt_manager_event_notify(BT_HFP_ESCO_ESTABLISHED_EVENT, NULL, 0);
        break;
    }
    case BTSRV_SCO_DISCONNECTED:
    {
        SYS_LOG_INF("hfp sco disconnected\n");
        hfp_ag_manager.sco_connect_time = 0;
        bt_manager_event_notify(BT_HFP_ESCO_RELEASED_EVENT, NULL, 0);
        break;
    }
    case BTSRV_HFP_CALL_ATA:
    {
        SYS_LOG_INF("hfp ag ata\n");
        bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_IND,1);
        bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);//on going
        bt_manager_event_notify(BT_HFP_CALL_ATA, NULL, 0);
        break;
    }
    case BTSRV_HFP_CALL_CHUP:
    {
        SYS_LOG_INF("hfp ag chup\n");
        bt_manager_hfp_ag_hungup();
        break;
    }
    case BTSRV_HFP_CALL_ATD:
    {
        SYS_LOG_INF("hfp ag atd:%s\n", param);
        if(param_size)
        {
            if(hfp_ag_manager.dail_memory)
            {

                bt_manager_hfp_ag_send_event("OK", strlen("OK"));
                bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,2);//out going
            }
            else
            {
                bt_manager_hfp_ag_send_event("ERROR", strlen("ERROR"));
            }
        }
        else
        {
            bt_manager_hfp_ag_send_event("OK", strlen("OK"));
            if(hfp_ag_manager.dail_memory < 10000)
            {
                hfp_ag_manager.dail_memory++;
            }

            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,2);//out going
        }
        break;
    }
    case BTSRV_HFP_CALL_BLDN:
    {
        if(hfp_ag_manager.dail_memory)
        {

            bt_manager_hfp_ag_send_event("OK", strlen("OK"));
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,2);//out going
        }
        else
        {
            bt_manager_hfp_ag_send_event("ERROR", strlen("ERROR"));
        }
        break;
    }
    case BTSRV_HFP_CALL_CLCC:
    {
        if(hfp_ag_manager.cur_clcc == 1)
        {
            bt_manager_hfp_ag_send_event("+CLCC: 1,1,0,0,0,\"1234567\"", strlen("+CLCC: 1,1,0,0,0,\"1234567\""));
        }
        else if(hfp_ag_manager.cur_clcc == 2)
        {
//     bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,1);
//    	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_HELD_IND,1);//out going
            bt_manager_hfp_ag_send_event("+CLCC: 2,1,0,0,0,\"7654321\"", strlen("+CLCC: 1,1,0,0,0,\"3456789\""));
            bt_manager_hfp_ag_send_event("+CLCC: 1,1,1,0,0,\"1234567\"", strlen("+CLCC: 1,1,0,0,0,\"1234567\""));
        }
        bt_manager_hfp_ag_send_event("OK", strlen("OK"));
        break;
    }
    case BTSRV_HFP_CALL_CHLD:
    {
        u32_t code = (u32_t)param;

        if(code == 0)
        {
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_HELD_IND, 0);//out going
        }

        break;
    }
#endif
	default:
		break;
	}
}

int bt_manager_hfp_ag_update_indicator(enum btsrv_hfp_hf_ag_indicators index, uint8_t value)
{
	if(index == BTSRV_HFP_AG_CALL_IND)
		hfp_ag_manager.call = value;
	else if(index == BTSRV_HFP_AG_CALL_SETUP_IND)
		hfp_ag_manager.call_setup = value;
	else if(index == BTSRV_HFP_AG_CALL_HELD_IND)
		hfp_ag_manager.call_held = value;
	return btif_hfp_ag_update_indicator(index,value);
}

int bt_manager_hfp_ag_start_call_out(void){
	if(!hfp_ag_manager.connected)
		return -EINVAL;
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,2);//out going
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,3);

	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_IND,1);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);//on going

	//creat sco
	return 0;
}

#ifdef CONFIG_BT_BR_TRANSCEIVER
void bt_manager_hfp_ag_answer_call(void)
{
    if(hfp_ag_manager.call_setup)
    {
        if(hfp_ag_manager.call)
        {
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_HELD_IND, 1);
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);//on going
        }
        else
        {
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_IND,1);
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);//on going
            btif_pts_hfp_active_connect_sco();
        }
    }
}

void bt_manager_hfp_ag_answer_call_2(void)
{
    if(hfp_ag_manager.call_setup)
    {
        if(hfp_ag_manager.call)
        {
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_HELD_IND, 1);
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);//on going
        }
        else
        {
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_IND,1);
            bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);//on going
        }
    }
}

void bt_manager_hfp_ag_reject_call(void)
{
    hfp_ag_manager.cur_clcc = 0;
    bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);//on going
	btif_pts_hfp_active_disconnect_sco();

}

int bt_manager_hfp_ag_start_call_in(void)
{
    if(hfp_ag_manager.dail_memory < 10000)
    {
        hfp_ag_manager.dail_memory++;
    }
    hfp_ag_manager.cur_clcc = 1;
    bt_manager_hfp_ag_send_event("+BSIR: 0", strlen("+BSIR: 1"));
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,1);//out going
    bt_manager_hfp_ag_send_event("RING", strlen("RING"));
    bt_manager_hfp_ag_send_event("+CLIP: \"1234567\",129",strlen("+CLIP: \"1234567\",129"));
	return 0;
}

int bt_manager_hfp_ag_start_call_in_band(void)
{
    if(hfp_ag_manager.dail_memory < 10000)
    {
        hfp_ag_manager.dail_memory++;
    }
    hfp_ag_manager.cur_clcc = 1;
    bt_manager_hfp_ag_send_event("+BSIR: 1", strlen("+BSIR: 1"));
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,1);//out going
    bt_manager_hfp_ag_send_event("RING", strlen("RING"));
    bt_manager_hfp_ag_send_event("+CLIP: \"1234567\",129",strlen("+CLIP: \"1234567\",129"));
    if (btif_pts_hfp_active_connect_sco()) {
        SYS_LOG_WRN("Not ready\n");
    }
	return 0;
}

int bt_manager_hfp_ag_start_call_in_2(void)
{
    if(hfp_ag_manager.cur_clcc != 1)
        return 0;

    if(hfp_ag_manager.call_setup)
    {
        bt_manager_hfp_ag_answer_call();
        k_sleep(K_MSEC(100));
    }

    if(hfp_ag_manager.dail_memory < 10000)
    {
        hfp_ag_manager.dail_memory++;
    }
    hfp_ag_manager.cur_clcc = 2;
    if(!hfp_ag_manager.call || !hfp_ag_manager.call_held)
    {
        bt_manager_hfp_ag_send_event("+CCWA: \"7654321\",129",strlen("+CCWA: \"7654321\",129"));
        bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,1);//out going
    }
	return 0;
}

int bt_manager_hfp_ag_start_call_in_3(void)
{
    if(hfp_ag_manager.cur_clcc != 1)
        return 0;
#ifdef CONFIG_BT_BR_TRANSCEIVER
    hfp_ag_manager.sco_auto_start = 0;
#endif
    if(hfp_ag_manager.dail_memory < 10000)
    {
        hfp_ag_manager.dail_memory++;
    }
    hfp_ag_manager.cur_clcc = 2;
    if(!hfp_ag_manager.call || !hfp_ag_manager.call_held)
    {
        bt_manager_hfp_ag_send_event("+CCWA: \"7654321\",129",strlen("+CCWA: \"7654321\",129"));
        bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,1);
        //bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_HELD_IND,1);//out going

    }
	return 0;
}

int bt_manager_hfp_ag_dial_memory_clear(void)
{
    SYS_LOG_WRN("dail_memory = %d\n",  hfp_ag_manager.dail_memory);

    hfp_ag_manager.dail_memory = 0;
    return 0;
}

int bt_manager_hfp_ag_hungup(void)
{
    hfp_ag_manager.cur_clcc = 0;

    if(!hfp_ag_manager.connected)
    {
        bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);//on going
        bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_HELD_IND,0);//on going
        bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_IND,0);//out going
        return 0;
    }

    if(hfp_ag_manager.call_setup)
    {
        bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);//on going
    }

    if(hfp_ag_manager.call)
    {
        bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_IND,0);//out going
    }

    if(hfp_ag_manager.call_held)
    {
        bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_HELD_IND,0);//out going
    }

#ifdef CONFIG_BT_BR_TRANSCEIVER
    hfp_ag_manager.sco_auto_start = 1;
#endif

	btif_pts_hfp_active_disconnect_sco();
    return 0;
}

#endif
int bt_manager_hfp_ag_profile_start(void)
{
	btif_hfp_ag_start(&_bt_manager_hfp_ag_callback);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_SERVICE_IND,1);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_IND,0);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_HELD_IND,0);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_SINGNAL_IND,4);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_ROAM_IND,0);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_BATTERY_IND,5);//to do
	return 0;
}

int bt_manager_hfp_ag_profile_stop(void)
{
	return btif_hfp_ag_stop();
}

int bt_manager_hfp_ag_init(void)
{
	memset(&hfp_ag_manager, 0, sizeof(struct bt_manager_hfp_ag_info_t));
#ifdef CONFIG_BT_BR_TRANSCEIVER
    hfp_ag_manager.sco_auto_start = 1;
#endif
	return 0;
}

