/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief
 */

#ifndef __OTA_TWS_TRANSMIT_H__
#define __OTA_TWS_TRANSMIT_H__
#include <bt_manager.h>
#include <ota_manifest.h>
#include <ota_breakpoint.h>
#include <user_comm/sys_comm.h>

#define OTA_TWS_DIRECTION_MASTER_TO_SLAVE 0x01 
#define OTA_TWS_DIRECTION_TO_SLAVE_MASTER 0x10 

#define OTA_TWS_TYPE_DATA_SLAVE 0x00
#define OTA_TWS_TYPE_DATA_MASTER 0x01 
#define OTA_TWS_TYPE_CMD 0x02

enum TWS_OTA_CMD
{
    OTA_NOUSE           = 0x00,
    OTA_READY		    = 0x01,
    OTA_PAUSE      		= 0x02,
    OTA_HALT       		= 0x03,
    OTA_CHECK_COMPLETE  = 0x04,
    OTA_BOOT_SWITCH 	= 0x05,
    OTA_REBOOT     		= 0x06,
    OTA_INQUIRE     	= 0x07,
    OTA_RESPONSE        = 0x08,
    OTA_DATA_END        = 0x09,
    OTA_RESUME		    = 0x0A,
    OTA_BREAKPOINT		= 0x0B,
	OTA_BREAKPOINT_RSP	= 0x0C,

    OTA_CMD_MAX         = 0x0F,
};

enum TWS_OTA_STATUS
{
    OTA_STATUS_NOUSE    = 0x00,
    OTA_STATUS_READY	= 0x01,
    OTA_STATUS_RUNNING	= 0x02,
    OTA_STATUS_PAUSE    = 0x03,
    OTA_STATUS_HALT     = 0x04,
    OTA_STATUS_CHECK_PASS  = 0x05,
    OTA_STATUS_BOOT_SWITCH 	= 0x06,
    OTA_STATUS_REBOOT   = 0x07,
    OTA_STATUS_DATA_END = 0x08,
    
    OTA_STATUS_BK_SYNC = 0x0E,
	OTA_STATUS_VALUE_MAX = 0x0F,
};

typedef struct
{
	// u8_t direction; // 0x01 or 0x10
	u8_t type; //0x00 or 0x01
	u8_t cmd;
	u8_t status;
    u8_t fn;
	u8_t tws_num;
	u32_t checksum;
	u16_t ctx_len;
} __attribute__((packed)) tws_ota_info_t;

typedef struct
{
    void (*connect_cb)(bool connected);
    void (*tws_receive_data)(u8_t, void *,u8_t, u8_t*, u16_t);
} bt_ota_master_cb;

void tws_ota_transmit_data(tws_ota_info_t *info, u8_t *buf);
u8_t  tws_ota_access_receive(u8_t* buf, u16_t len);
int tws_ota_slave_register(struct btmgr_spp_cb *pcb);
int tws_ota_master_register(u8_t channel, void *ble_attr, bt_ota_master_cb *pcb);
u32_t ota_transmit_version_get(void);
void tws_ota_resource_release(void);

#endif /* __OTA_TWS_TRANSMIT_H__ */
