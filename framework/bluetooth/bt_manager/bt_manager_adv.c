/*
 * Copyright (c) 2022 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <bt_manager.h>
#include <media_type.h>
#include <mem_manager.h>
#include <app_manager.h>
#include <os_common_api.h>
#include <thread_timer.h>
#include "bt_manager_inner.h"
#include "bt_porting_inner.h"

typedef struct btmgr_adv
{
	u8_t cur_enable_type;
	struct thread_timer switch_adv_timer;
	btmgr_adv_register_func_t* adv[ADV_TYPE_MAX];

	u32_t begin_wait_time;
	u8_t wait_tws_paired;
}btmgr_adv_t;

static btmgr_adv_t gbtmgr_adv;

#define ADV_INTERVAL_MS (2000)
#define TWS_WAIT_TIME_MS (5000)

static char* adv_map_to_str(u8_t type)
{
	switch(type)
	{
		case ADV_TYPE_BLE:
			return "ble";
		case ADV_TYPE_LEAUDIO:
			return "lea";
		default:
			return "none";			
	}
}
static void adv_switch_check(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	btmgr_adv_t *p = &gbtmgr_adv;
	uint8_t ble_adv_enable = 0;
	uint8_t ble_adv_update_bit = 0;
	uint8_t leaudio_adv_enable = 0;
	uint8_t leaudio_adv_update_bit = 0;	
	uint8_t last_enable_type = p->cur_enable_type;

	if (p->wait_tws_paired == 1){
		if(btif_tws_get_dev_role() != BTSRV_TWS_NONE){
			p->wait_tws_paired = 0;
			goto ADV;
		}
		uint32_t cur_time = os_uptime_get_32();

		if(cur_time - p->begin_wait_time > TWS_WAIT_TIME_MS){
			p->wait_tws_paired = 0;
			goto ADV;
		}
	}

ADV:

	if (p->adv[ADV_TYPE_BLE])
	{
		ble_adv_enable = p->adv[ADV_TYPE_BLE]->adv_get_state()&ADV_STATE_BIT_ENABLE?1:0;
		ble_adv_update_bit = p->adv[ADV_TYPE_BLE]->adv_get_state()&ADV_STATE_BIT_UPDATE?1:0;
	}

	if (p->adv[ADV_TYPE_LEAUDIO])
	{
		leaudio_adv_enable = p->adv[ADV_TYPE_LEAUDIO]->adv_get_state()&ADV_STATE_BIT_ENABLE?1:0;
		leaudio_adv_update_bit = p->adv[ADV_TYPE_LEAUDIO]->adv_get_state()&ADV_STATE_BIT_UPDATE?1:0;
	}


	if (!ble_adv_enable && !leaudio_adv_enable && (p->cur_enable_type == ADV_TYPE_NONE) 
		&& (!ble_adv_update_bit) && (!leaudio_adv_update_bit)){
		return;
	}	

	if (!ble_adv_enable && !leaudio_adv_enable){
		p->cur_enable_type = ADV_TYPE_NONE;
		goto Done;
	}
	
	if (p->cur_enable_type == ADV_TYPE_NONE){
		if (leaudio_adv_enable){
			p->cur_enable_type = ADV_TYPE_LEAUDIO;
		}else if (ble_adv_enable){
			p->cur_enable_type = ADV_TYPE_BLE;
		}
	}else{
		if (p->cur_enable_type == ADV_TYPE_BLE && leaudio_adv_enable){
			p->cur_enable_type = ADV_TYPE_LEAUDIO;
		}else if (p->cur_enable_type == ADV_TYPE_LEAUDIO && ble_adv_enable){
			p->cur_enable_type = ADV_TYPE_BLE;
		}
	}
	
Done:
	if ((last_enable_type != p->cur_enable_type) || 
		(p->cur_enable_type == ADV_TYPE_LEAUDIO && leaudio_adv_update_bit) || 
		(p->cur_enable_type == ADV_TYPE_BLE && ble_adv_update_bit)){
		SYS_LOG_INF("adv: %s -> %s",adv_map_to_str(last_enable_type), adv_map_to_str(p->cur_enable_type));

		if (last_enable_type != ADV_TYPE_NONE && p->adv[last_enable_type] && p->adv[last_enable_type]->adv_disable){
			p->adv[last_enable_type]->adv_disable();
		}

		if (p->cur_enable_type != ADV_TYPE_NONE && p->adv[p->cur_enable_type]&& p->adv[p->cur_enable_type]->adv_enable){
			p->adv[p->cur_enable_type]->adv_enable();
		}
	}
}

int btmgr_adv_init(void)
{
	uint8_t tws_paired = 0;
	int cnt = 0;
	int i = 0;
	struct autoconn_info *info = NULL;
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	btmgr_adv_t *p = &gbtmgr_adv;

	memset(p, 0, sizeof(btmgr_adv_t));
	p->cur_enable_type = ADV_TYPE_NONE;

	if (cfg_feature->sp_tws)
	{
		info = bt_mem_malloc(sizeof(struct autoconn_info)*BT_MAX_AUTOCONN_DEV);
		if (info == NULL) {
			SYS_LOG_ERR("malloc failed");
			goto Done;
		}

		cnt = btif_br_get_auto_reconnect_info(info, BT_MAX_AUTOCONN_DEV);
	    if(cnt > 0){
	        for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++){
	            if(info[i].addr_valid &&
	                ((info[i].tws_role == BTSRV_TWS_MASTER) || (info[i].tws_role == BTSRV_TWS_SLAVE))){
	                tws_paired = 1;
	                break;
	            }
	        }
	    }
	}

Done:
	if(info){
		bt_mem_free(info);
	}

	if (tws_paired){
		p->begin_wait_time = os_uptime_get_32();
		p->wait_tws_paired = 1;
	}

	thread_timer_init(&p->switch_adv_timer, adv_switch_check, NULL);
	thread_timer_start(&p->switch_adv_timer, ADV_INTERVAL_MS, ADV_INTERVAL_MS);

	return 0;
}


int btmgr_adv_deinit(void)
{
	btmgr_adv_t *p = &gbtmgr_adv;

	thread_timer_stop(&p->switch_adv_timer);

	return 0;
}




int btmgr_adv_register(int adv_type, btmgr_adv_register_func_t* funcs)
{
	SYS_LOG_INF("%d",adv_type);

	if (adv_type < ADV_TYPE_MAX){
		gbtmgr_adv.adv[adv_type] = funcs;
	}
	return 0;
}

int btmgr_adv_update_immediately(void)
{
	adv_switch_check(NULL, NULL);
	return 0;
}

