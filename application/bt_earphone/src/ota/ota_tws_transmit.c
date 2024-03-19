/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include <sys_event.h>
#include <stream.h>
#include <app_ui.h>
#include "ota_tws_transmit.h"
#include "ota_app.h"
#include "user_comm/ap_ota_transmit.h"
#ifdef CONFIG_OTA_BACKEND_BLUETOOTH

#define TWS_OTA_SEND_MAX_TIME (5)
#define TWS_LONG_DATA_LEN_MAX (512)

struct btmgr_spp_cb ota_slave_cb;
bt_ota_master_cb ota_master_cb;
u8_t ota_channel = 0xFF;
void *attr = NULL;
u8_t ota_halt = 0;
u8_t frame_num_self = 0;
u8_t frame_num_tws = 0;

void tws_ota_transmit_data(tws_ota_info_t *info, u8_t *buf)
{
    u8_t  *data_buf;
    tws_ota_info_t *pst;
	u16_t total_len;
	u32_t ck = 0;
	int i, ret;
	u8_t retry_times = 0;
	u16_t res_len = 0, trans_len = 0, send_len = 0;

	if (BTSRV_TWS_NONE == bt_manager_tws_get_dev_role())
		return;

	if (!buf)
		info->ctx_len = 0;
	
	total_len = info->ctx_len + sizeof(tws_ota_info_t);

	data_buf = (u8_t *)mem_malloc(TWS_LONG_DATA_LEN_MAX);
	if (!data_buf)
		return;
	memset(data_buf, 0 ,TWS_LONG_DATA_LEN_MAX);
	pst = (tws_ota_info_t *)data_buf;
	memcpy(pst, info, sizeof(tws_ota_info_t));

	res_len = info->ctx_len;
	SYS_LOG_INF("total_len 0x%x.",total_len);

	while (1)
	{
		if (res_len > TWS_LONG_DATA_LEN_MAX-sizeof(tws_ota_info_t))
			send_len = TWS_LONG_DATA_LEN_MAX-sizeof(tws_ota_info_t);
		else
			send_len = res_len;

		if (send_len)
			memcpy(data_buf + sizeof(tws_ota_info_t), buf + trans_len, send_len);

		frame_num_self++;
		pst->tws_num = frame_num_self;
		pst->ctx_len = send_len;
		pst->checksum = 0;
		ck = 0;
		for (i = 0; i < send_len + sizeof(tws_ota_info_t); i++)
		{
			ck += data_buf[i];
		}
		pst->checksum = ck;
		
		if ((send_len + sizeof(tws_ota_info_t)) > 16)
			print_hex("tws_tx:", data_buf, 16);

		SYS_LOG_INF("tws_num 0x%x checksum 0x%x.", pst->tws_num, pst->checksum);

		while (retry_times < TWS_OTA_SEND_MAX_TIME) 
		{
			ret = bt_manager_tws_send_message(TWS_LONG_MSG_EVENT, TWS_EVENT_TWS_OTA, data_buf, send_len + sizeof(tws_ota_info_t));
			if (ret != send_len + sizeof(tws_ota_info_t)) {
				SYS_LOG_ERR("Failed to send data!");
				retry_times++;
				//os_sleep(10);
			}
			else 
			{
				break;
			}
		}

		res_len -= send_len;
		trans_len += send_len;
		retry_times = 0;
		if (0 == res_len)
		{
			break;
		}
		//pst->subpkg_num++;
	}

	mem_free(data_buf);
}

static u8_t  tws_ota_cmd_process(u8_t cmd, u8_t *buf ,u16_t len)
{
	struct ota_breakpoint *bp = NULL;
	tws_ota_info_t st;
	u8_t *ptr;
	u32_t version;
	u32_t remote_version;
	CFG_Struct_OTA_Settings cfg;

    app_config_read
    (
        CFG_ID_OTA_SETTINGS,
        &cfg,0,sizeof(CFG_Struct_OTA_Settings)
    );

    switch (cmd)
    {
        case OTA_NOUSE:
        case OTA_RESUME:
        case OTA_READY:
        {
        	SYS_LOG_INF("OTA_READY");
			if (1 == ota_halt)
			{
				ota_halt = 0;
				SYS_LOG_ERR("OTA exit");
				return 0;
			}

			ota_app_init_slave_bluetooth();
			if (ota_slave_cb.connected)
        		ota_slave_cb.connected(0, NULL);
            break;
        }

        case OTA_HALT:
        {
        	ota_halt = 0;
        	if (ota_slave_cb.disconnected)
        		ota_slave_cb.disconnected(0);
            return 0;
        }

        case OTA_BREAKPOINT:
        {
			memset(&st, 0, sizeof(tws_ota_info_t));
			st.type = OTA_TWS_TYPE_CMD;
			st.cmd = OTA_BREAKPOINT_RSP;
			st.ctx_len = sizeof(struct ota_breakpoint) + sizeof(u32_t);

  			ptr = mem_malloc(sizeof(struct ota_breakpoint)+sizeof(u32_t));
            if (!ptr)
                return 0;			
			bp = (struct ota_breakpoint *)ptr;
			memset(ptr, 0 , sizeof(struct ota_breakpoint)+sizeof(u32_t));
			ota_breakpoint_load(bp);
			bp->bp_id = 0;
			print_hex("slave bp:",bp,sizeof(struct ota_breakpoint));
			if ((len != sizeof(struct ota_breakpoint)+sizeof(u32_t)) ||
				(memcmp(bp, buf, sizeof(struct ota_breakpoint)) != 0))
			{
				//ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADE_PENDING);	
				SYS_LOG_INF("SLAVE BP ERR!");
				ota_breakpoint_init_default_value(bp);
				ota_breakpoint_save(bp);
				ota_app_init();
			}

			memcpy(&remote_version, &buf[sizeof(struct ota_breakpoint)], sizeof(u32_t));		
			version = ota_transmit_version_get();
			
			if ((0 == cfg.Enable_Ver_Diff) &&
				(version != remote_version))
			{
				ota_halt = 1;
			}
			
			memcpy(&ptr[sizeof(struct ota_breakpoint)], &version, sizeof(u32_t));
			tws_ota_transmit_data(&st, ptr);
			mem_free(ptr);       	
            break;
        }
        case OTA_BREAKPOINT_RSP:
        {
  			bp = mem_malloc(sizeof(struct ota_breakpoint));
            if (!bp)
                return 0;			
			memset(bp, 0 , sizeof(struct ota_breakpoint));
			ota_breakpoint_load(bp);
			bp->bp_id = 0;
			print_hex("master bp:",bp,sizeof(struct ota_breakpoint));
			if ((len != sizeof(struct ota_breakpoint)+sizeof(u32_t)) ||
				(memcmp(bp, buf, sizeof(struct ota_breakpoint)) != 0))
			{
				//ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADE_PENDING);
				SYS_LOG_INF("MASTER BP ERR!");
				ota_breakpoint_init_default_value(bp);
				ota_breakpoint_save(bp);
				ota_app_init();
			}
			mem_free(bp);

			memcpy(&remote_version, &buf[sizeof(struct ota_breakpoint)], sizeof(u32_t));		
			version = ota_transmit_version_get();

			if ((0 == cfg.Enable_Ver_Diff) &&
				(version != remote_version))
			{
				SYS_LOG_ERR("remote_version 0x%x version 0x%x", remote_version, version);
				break;
			}

			if (ota_master_cb.connect_cb)
				ota_master_cb.connect_cb(true);

            break;
        }
    }	
	return 0;
}

u8_t  tws_ota_access_receive(u8_t* buf, u16_t len)
{
	tws_ota_info_t st;
	int i;
	u32_t calc_ck = 0;
	u32_t st_ck;
	u8_t *ptr;
	SYS_LOG_INF("len 0x%x.",len);
	if (len > 16)
		print_hex("tws_rx:", buf, 16);

	memcpy(&st, buf, sizeof(tws_ota_info_t));
	st_ck = st.checksum;
	st.checksum = 0;
	ptr = (u8_t *)&st;

	for (i = 0; i < sizeof(tws_ota_info_t); i++)
	{
		calc_ck += ptr[i];
	}

	for (; i < len; i++)
	{
		calc_ck += buf[i];
	}


	if (calc_ck != st_ck)
	{
		SYS_LOG_ERR("calc_ck 0x%x st_ck 0x%x.", calc_ck, st_ck);
	}

	frame_num_tws++;
	if (st.tws_num != frame_num_tws)
	{
		SYS_LOG_ERR("st.tws_num 0x%x frame_num_tws 0x%x.", st.tws_num, frame_num_tws);
	}
	SYS_LOG_INF("st.tws_num 0x%x frame_num_tws 0x%x.", st.tws_num, frame_num_tws);

	if (OTA_TWS_TYPE_DATA_SLAVE == st.type)
	{
		// write stream
		if (ota_slave_cb.receive_data)
			ota_slave_cb.receive_data(0, buf + sizeof(tws_ota_info_t), st.ctx_len);
	}
	else if (OTA_TWS_TYPE_DATA_MASTER == st.type)
	{
		if (ota_master_cb.tws_receive_data)
			ota_master_cb.tws_receive_data(ota_channel, attr, st.fn, buf + sizeof(tws_ota_info_t), st.ctx_len);
	}
	else
	{
		//SYS_LOG_ERR("140");
		tws_ota_cmd_process(st.cmd, buf + sizeof(tws_ota_info_t), st.ctx_len);
		// cmd
	}
	
	return 0;
}

u32_t ota_transmit_version_get(void)
{
    u8_t tver[4];
    u8_t i=0, j=0;
    u32_t version;
	CFG_Struct_OTA_Settings cfg;

    app_config_read
    (
        CFG_ID_OTA_SETTINGS,
        &cfg,0,sizeof(CFG_Struct_OTA_Settings)
    );

    SYS_LOG_INF("Cfg ota ver: %s\n", cfg.Version_Number);
    memset(tver, 0, sizeof(tver));
    for (i = 0;i < sizeof(cfg.Version_Number);i++)
    {
        if (0 == i)
        {
            tver[3-j] = strtol(&cfg.Version_Number[i], NULL, 10);
            j++;
        }
        
        if ('.' == cfg.Version_Number[i])
        {
            if(sizeof(cfg.Version_Number) == (++i))
                break;
            tver[3-j] = strtol(&cfg.Version_Number[i], NULL, 10);
            j++;
        }
    }
    memcpy(&version, tver, sizeof(version));
    version = (version >> (8*(4-j)));
    if (0 == version)
        version = 0x00010000; // gma default v1.0.0
        // version = 0x0a000000; // tma default v10.00.00.00
    //log_debug("version 0x%x\n", version);
    
    return version;
}


int tws_ota_slave_register(struct btmgr_spp_cb *pcb)
{
	memcpy(&ota_slave_cb, pcb, sizeof(struct btmgr_spp_cb));
	return 0;
}

int tws_ota_master_register(u8_t channel, void *ble_attr, bt_ota_master_cb *pcb)
{
	memcpy(&ota_master_cb, pcb, sizeof(bt_ota_master_cb));
	ota_channel = channel;
	attr = ble_attr;
	return 0;
}

void tws_ota_resource_release(void)
{
	if (ota_slave_cb.disconnected)
		ota_slave_cb.disconnected(0);

	ota_halt = 0;
	attr = NULL;
	ota_channel = 0xFF;
	frame_num_self = 0;
	frame_num_tws = 0;
	memset(&ota_master_cb, 0, sizeof(bt_ota_master_cb));
	// memset(&ota_slave_cb, 0, sizeof(struct btmgr_spp_cb));

	ota_transmit_resource_release(FALSE);

	return;
}

#endif

