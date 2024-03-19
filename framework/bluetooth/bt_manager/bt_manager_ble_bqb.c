/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager a2dp profile.
 */
#define SYS_LOG_DOMAIN "bt manager ble bqb"

//#include <logging/sys_log.h>

#include <os_common_api.h>

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
#include <property_manager.h>
#include <acts_bluetooth/host_interface.h>
#include <bt_manager.h>
#include <bt_manager_inner.h>
#include <sys_event.h>

#if (defined(CONFIG_BT_BLE_BQB) && defined(CONFIG_BT_TRANSCEIVER))

#define CTS_VALUE_LEN  10
#define NDST_VALUE_LEN 8
#define BAS_VALUE_LEN  1
#define DIS_VALUE_LEN  12
#define RTU_VALUE_LEN  2
#define IAS_VALUE_LEN  1
#define HRS_VALUE_LEN  8
#define HTS_VALUE_LEN  12

/* 
 * BAS server define
 */
static u8_t bas_val_1[BAS_VALUE_LEN] = {50};
static u8_t bas_notify = 0;

static struct bt_gatt_ccc_cfg g_bas_ccc_cfg[1];

static bt_addr_t ble_bqb_addr;

static void bas_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	SYS_LOG_INF("value: %d", value);
    bas_notify = (u8_t)value;
}
static ssize_t	 _bas_charct_read(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, u16_t len,
					u16_t offset)
{
    SYS_LOG_INF("");
    *((u8_t *)buf) = *((u8_t *)attr->user_data);
    return BAS_VALUE_LEN;
}

static struct bt_gatt_attr ble_bas_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),

	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _bas_charct_read, NULL, bas_val_1),
	//BT_GATT_DESCRIPTOR(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_PERM_READ, _bas_charct_read, NULL, bas_val_1),
	//BT_GATT_CCC(g_bas_ccc_cfg, bas_ccc_cfg_changed)
	BT_GATT_CCC(bas_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
};

static void ble_bas_connect_cb(u8_t *mac, u8_t connected)
{
    SYS_LOG_INF("BLE %s\n", connected ? "connected" : "disconnected");
    SYS_LOG_INF("MAC %2x:%2x:%2x:%2x:%2x:%2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    memcpy(&ble_bqb_addr, mac, sizeof(bt_addr_t));
    //for(int ii = 0; ii < 6; ii++)
    //   ble_bqb_addr.val[ii] =  mac[5-ii];
    bas_notify = 0;
}

static struct ble_reg_manager ble_bas_mgr = {
	.link_cb = ble_bas_connect_cb,
};

static int bt_manager_ble_bas_init(void)
{
    g_bas_ccc_cfg[0].value = 1;
    ble_bas_mgr.gatt_svc.attrs = ble_bas_attrs;
    ble_bas_mgr.gatt_svc.attr_count = ARRAY_SIZE(ble_bas_attrs);
    bt_manager_ble_service_reg(&ble_bas_mgr);
	return 0;
}

void ble_bas_notify(u8_t level)
{
    bas_val_1[0] = level;
    if(bas_notify)
        bt_manager_ble_send_data(&ble_bqb_addr, &ble_bas_attrs[1], &ble_bas_attrs[2], &level, 1);
}
/* 
 * CTS server define
 */
static u8_t cts_val_1[CTS_VALUE_LEN] = {0xE3, 0x7, 7, 11, 10, 17, 50, 4, 50, 2};
static u8_t cts_notify = 0;

static struct bt_gatt_ccc_cfg g_cts_ccc_cfg[1];
static void cts_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	SYS_LOG_INF("value: %d", value);
    cts_notify = (u8_t)value;
}
static ssize_t	 _cts_charct_read(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, u16_t len,
					u16_t offset)
{
    SYS_LOG_INF("");
    memcpy(buf, attr->user_data, CTS_VALUE_LEN);
    return CTS_VALUE_LEN;
}

static struct bt_gatt_attr ble_cts_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CTS),

	BT_GATT_CHARACTERISTIC(BT_UUID_CTS_CURRENT_TIME, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _cts_charct_read, NULL, cts_val_1),
	//BT_GATT_DESCRIPTOR(BT_UUID_CTS_CURRENT_TIME, BT_GATT_PERM_READ, _cts_charct_read, NULL, cts_val_1),
	//BT_GATT_CCC(g_cts_ccc_cfg, cts_ccc_cfg_changed)
	BT_GATT_CCC(cts_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
};

static void ble_cts_connect_cb(u8_t *mac, u8_t connected)
{
    SYS_LOG_INF("BLE %s\n", connected ? "connected" : "disconnected");
    SYS_LOG_INF("MAC %2x:%2x:%2x:%2x:%2x:%2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    memcpy(&ble_bqb_addr, mac, sizeof(bt_addr_t));
    //for(int ii = 0; ii < 6; ii++)
    //    ble_bqb_addr.val[ii] =  mac[5-ii];

    cts_notify = 0;
}

static struct ble_reg_manager ble_cts_mgr = {
	.link_cb = ble_cts_connect_cb,
};

static int bt_manager_ble_cts_init(void)
{
    g_cts_ccc_cfg[0].value = 1;
    ble_cts_mgr.gatt_svc.attrs = ble_cts_attrs;
    ble_cts_mgr.gatt_svc.attr_count = ARRAY_SIZE(ble_cts_attrs);
    bt_manager_ble_service_reg(&ble_cts_mgr);
	return 0;
}
void ble_cts_notify(u8_t level)
{
    cts_val_1[0] = level;
    if (cts_notify)
    {
        SYS_LOG_INF("");
        int ret = bt_manager_ble_send_data(&ble_bqb_addr, &ble_cts_attrs[1], &ble_cts_attrs[2], cts_val_1, CTS_VALUE_LEN);
        SYS_LOG_INF("ret: %d", ret);
    }
}
/* 
 * HTS server define
 */
//#define BT_UUID_HTS         BT_UUID_DECLARE_16(0x1809)
#define HTS_CHA_UUID_1		BT_UUID_DECLARE_16(0x2A1C)
#define HTS_CHA_UUID_2		BT_UUID_DECLARE_16(0x2A1D)
#define HTS_CHA_UUID_3		BT_UUID_DECLARE_16(0x2A1E)
#define HTS_CHA_UUID_4		BT_UUID_DECLARE_16(0x2A21)

static u8_t hts_val_1[HTS_VALUE_LEN] = {0};
static u8_t hts_notify = 0;

static struct bt_gatt_ccc_cfg g_hts_ccc_cfg[1];
static void hts_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	SYS_LOG_INF("value: %d", value);
    hts_notify = (u8_t)value;
}

static ssize_t	 _hts_charct_read(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, u16_t len,
					u16_t offset)
{
    SYS_LOG_INF("");
    memcpy(buf, attr->user_data, HTS_VALUE_LEN);
    return CTS_VALUE_LEN;
}

static struct bt_gatt_attr ble_hts_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HTS),

	BT_GATT_CHARACTERISTIC(HTS_CHA_UUID_1, BT_GATT_CHRC_INDICATE, BT_GATT_PERM_NONE, NULL, NULL, NULL),	
	//BT_GATT_DESCRIPTOR(HTS_CHA_UUID_1, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	//BT_GATT_CCC(g_hts_ccc_cfg, hts_ccc_cfg_changed),
	BT_GATT_CCC(hts_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_CHARACTERISTIC(HTS_CHA_UUID_2, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _hts_charct_read, NULL, hts_val_1),	
	//BT_GATT_DESCRIPTOR(HTS_CHA_UUID_2, BT_GATT_PERM_READ, _hts_charct_read, NULL, hts_val_1),

	BT_GATT_CHARACTERISTIC(HTS_CHA_UUID_3, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),	
	//BT_GATT_DESCRIPTOR(HTS_CHA_UUID_3, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	//BT_GATT_CCC(g_hts_ccc_cfg, hts_ccc_cfg_changed),
	BT_GATT_CCC(hts_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(HTS_CHA_UUID_4, BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _hts_charct_read, NULL, hts_val_1),	
	//BT_GATT_DESCRIPTOR(HTS_CHA_UUID_4, BT_GATT_PERM_READ, _hts_charct_read, NULL, hts_val_1),
	//BT_GATT_CCC(g_hts_ccc_cfg, hts_ccc_cfg_changed),
	BT_GATT_CCC(hts_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static void ble_hts_connect_cb(u8_t *mac, u8_t connected)
{
    SYS_LOG_INF("BLE %s\n", connected ? "connected" : "disconnected");
    SYS_LOG_INF("MAC %2x:%2x:%2x:%2x:%2x:%2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    memcpy(&ble_bqb_addr, mac, sizeof(bt_addr_t));
    //for(int ii = 0; ii < 6; ii++)
    //    ble_bqb_addr.val[ii] =  mac[5-ii];
    hts_notify = 0;
}

static struct ble_reg_manager ble_hts_mgr = {
	.link_cb = ble_hts_connect_cb,
};

static int bt_manager_ble_hts_init(void)
{
    g_hts_ccc_cfg[0].value = 1;
    ble_hts_mgr.gatt_svc.attrs = ble_hts_attrs;
    ble_hts_mgr.gatt_svc.attr_count = ARRAY_SIZE(ble_hts_attrs);
    bt_manager_ble_service_reg(&ble_hts_mgr);
	return 0;
}

void ble_hts_notify(u8_t level)
{
    hts_val_1[0] = 0;
    hts_val_1[1] = level;
    if(hts_notify)
        bt_manager_ble_send_data(&ble_bqb_addr, &ble_hts_attrs[1], &ble_hts_attrs[2], hts_val_1, 5);
}

/* 
 * HRS server define
 */
static u8_t hrs_val_1[HRS_VALUE_LEN] = {0x19};
static u8_t hrs_val_2[HRS_VALUE_LEN] = {0};
static u8_t hrs_notify = 0;

static struct bt_gatt_ccc_cfg g_hrs_ccc_cfg[1];
static void hrs_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	SYS_LOG_INF("value: %d", value);
    hrs_notify = value;
}
static ssize_t _hrs_charct_read(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, u16_t len,
					u16_t offset)
{
    SYS_LOG_INF("");
    memcpy(buf, attr->user_data, HRS_VALUE_LEN);
    return HRS_VALUE_LEN;
}

static ssize_t _hrs_charct_wirte(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					const void *buf, u16_t len,
					u16_t offset, u8_t flags)
{
    SYS_LOG_INF("");
    memcpy(attr->user_data, buf, HRS_VALUE_LEN);
    return HRS_VALUE_LEN;
}


static struct bt_gatt_attr ble_hrs_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HRS),

	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_MEASUREMENT, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),	
	//BT_GATT_DESCRIPTOR(BT_UUID_HRS_MEASUREMENT, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	//BT_GATT_CCC(g_hrs_ccc_cfg, hrs_ccc_cfg_changed),
	BT_GATT_CCC(hrs_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_BODY_SENSOR, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, NULL, NULL, NULL),	
	BT_GATT_DESCRIPTOR(BT_UUID_HRS_BODY_SENSOR, BT_GATT_PERM_READ, _hrs_charct_read, NULL, hrs_val_2),
    BT_GATT_CHARACTERISTIC(BT_UUID_HRS_CONTROL_POINT, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL, NULL, NULL),	
	BT_GATT_DESCRIPTOR(BT_UUID_HRS_CONTROL_POINT, BT_GATT_PERM_WRITE, NULL, _hrs_charct_wirte, hrs_val_2),
};

static void ble_hrs_connect_cb(u8_t *mac, u8_t connected)
{
    SYS_LOG_INF("BLE %s\n", connected ? "connected" : "disconnected");
    SYS_LOG_INF("MAC %2x:%2x:%2x:%2x:%2x:%2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    memcpy(&ble_bqb_addr, mac, sizeof(bt_addr_t));
    //for(int ii = 0; ii < 6; ii++)
    //    ble_bqb_addr.val[ii] =  mac[5-ii];
    hrs_notify = 0;
}

static struct ble_reg_manager ble_hrs_mgr = {
	.link_cb = ble_hrs_connect_cb,
};

static int bt_manager_ble_hrs_init(void)
{
    g_hrs_ccc_cfg[0].value = 1;
    ble_hrs_mgr.gatt_svc.attrs = ble_hrs_attrs;
    ble_hrs_mgr.gatt_svc.attr_count = ARRAY_SIZE(ble_hrs_attrs);
    bt_manager_ble_service_reg(&ble_hrs_mgr);
	return 0;
}

void ble_hrs_notify(u8_t level)
{
    hrs_val_1[0] = 0x19;
    hrs_val_1[1] = level;
    if(hrs_notify)
        bt_manager_ble_send_data(&ble_bqb_addr, &ble_hrs_attrs[1], &ble_hrs_attrs[2], hrs_val_1, 7);
}
/* 
 * DIS server define
 */
/*
#define DIS_BLE_SERVICE_UUID	0x180a
#define DIS_BLE_CHA_UUID_1		0x2A29
#define DIS_BLE_CHA_UUID_2		0x2A24
#define DIS_BLE_CHA_UUID_3		0x2A23
#define DIS_SERVICE_UUID BT_UUID_DECLARE_16(DIS_BLE_SERVICE_UUID)
#define DIS_CHA_UUID_1 	 BT_UUID_DECLARE_16(DIS_BLE_CHA_UUID_1)
#define DIS_CHA_UUID_2 	 BT_UUID_DECLARE_16(DIS_BLE_CHA_UUID_2)
#define DIS_CHA_UUID_3 	 BT_UUID_DECLARE_16(DIS_BLE_CHA_UUID_3)

UINT8 dis_val_1[DIS_VALUE_LEN] = {0};
static struct le_gatt_attr	dis_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(DIS_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(DIS_CHA_UUID_1,  ATT_READ, &dis_val_1, DIS_VALUE_LEN, NULL),
	BT_GATT_CHARACTERISTIC(DIS_CHA_UUID_2,  ATT_READ, &dis_val_1, DIS_VALUE_LEN, NULL),
	BT_GATT_CHARACTERISTIC(DIS_CHA_UUID_3,  ATT_READ, &dis_val_1, 8, NULL),
};

static int bt_manager_ble_dis_init(void)
{
	bt_manager_ble_reg_service(dis_gatt_attr, ARRAY_SIZE(dis_gatt_attr), NULL);
	return 0;
}*/
static u8_t dis_val_1[DIS_VALUE_LEN] = {0};
static u8_t dis_val_2[DIS_VALUE_LEN] = {0};
static u8_t dis_val_3[DIS_VALUE_LEN] = {1,1,1,1,1,1,1,1,1,1,1};
static ssize_t	 _dis_charct_read(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, u16_t len,
					u16_t offset)
{
    SYS_LOG_INF("");
    if(attr->user_data == dis_val_2)
    {
        memcpy(buf, attr->user_data, DIS_VALUE_LEN);
        return 8;
    }
    if(attr->user_data == dis_val_3)
    {
        memcpy(buf, attr->user_data, DIS_VALUE_LEN);
        return 7;
    }
    memcpy(buf, attr->user_data, DIS_VALUE_LEN);
    return DIS_VALUE_LEN;
}

static struct bt_gatt_attr ble_dis_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SERIAL_NUMBER, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),	
	//BT_GATT_DESCRIPTOR(BT_UUID_DIS_SERIAL_NUMBER, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),
	
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_HARDWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),	
	//BT_GATT_DESCRIPTOR(BT_UUID_DIS_HARDWARE_REVISION, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),
	
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FIRMWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),	
	//BT_GATT_DESCRIPTOR(BT_UUID_DIS_FIRMWARE_REVISION, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),
	
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SOFTWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),	
	//BT_GATT_DESCRIPTOR(BT_UUID_DIS_SOFTWARE_REVISION, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),
	
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_PNP_ID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_3),	
	//BT_GATT_DESCRIPTOR(BT_UUID_DIS_PNP_ID, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_3),

    //characteristic GGIT - IEEE 11073-20601 Regulatory Certification Data List
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2a2a), BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),	
	//BT_GATT_DESCRIPTOR(BT_UUID_DECLARE_16(0x2a2a), BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),
	
    BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),	
	//BT_GATT_DESCRIPTOR(BT_UUID_DIS_MANUFACTURER_NAME, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),

    BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),	
	//BT_GATT_DESCRIPTOR(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_1),

    BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SYSTEM_ID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_2),	
	//BT_GATT_DESCRIPTOR(BT_UUID_DIS_SYSTEM_ID, BT_GATT_PERM_READ, _dis_charct_read, NULL, dis_val_2),
};

static void ble_dis_connect_cb(u8_t *mac, u8_t connected)
{
    SYS_LOG_INF("BLE %s\n", connected ? "connected" : "disconnected");
    SYS_LOG_INF("MAC %2x:%2x:%2x:%2x:%2x:%2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    memcpy(&ble_bqb_addr, mac, sizeof(bt_addr_t));
    //for(int ii = 0; ii < 6; ii++)
    //    ble_bqb_addr.val[ii] =  mac[5-ii];
}

static struct ble_reg_manager ble_dis_mgr = {
	.link_cb = ble_dis_connect_cb,
};

static int bt_manager_ble_dis_init(void)
{
    ble_dis_mgr.gatt_svc.attrs = ble_dis_attrs;
    ble_dis_mgr.gatt_svc.attr_count = ARRAY_SIZE(ble_dis_attrs);
    bt_manager_ble_service_reg(&ble_dis_mgr);
	return 0;
}

/*
static int bt_manager_br_key_deal(u16_t key_value)
{
	bt_manager_dev_info_t* dev_info;

	switch(key_value)
	{
		case KEY_PREVIOUSSONG:
			dev_info = bt_manager_get_dev_info(BT_MANAGER_A2DP_ACTIVE_DEV);

			if ((dev_info == NULL) || (dev_info->hfp_connected)){
				bteg_HF_BQB_OOR_BV_02_I();
			}
			break;

		default:
			break;
	}
	return 0;
}

static int bt_manager_ble_key_deal(u16_t key_value)
{
	u8_t send_buf[EXPECT_ATT_MTU - 3];
	
	switch(key_value)
	{
		case KEY_PREVIOUSSONG:
			if (bas_gatt_attr[2].notify_ind_enable && bt_manager_ble_ready_send_mass_data()) {
				send_buf[0] = 0x40;
				bt_manager_notifyind_send(&bas_gatt_attr[1], &bas_gatt_attr[2], send_buf, 1);
			}
			break;
		
		case KEY_VOLUMEUP:
			if (hts_gatt_attr[2].notify_ind_enable && bt_manager_ble_ready_send_mass_data()) {
				send_buf[0] = 0;
				send_buf[1] = 37;
				bt_manager_notifyind_send(&hts_gatt_attr[1], &hts_gatt_attr[2], send_buf, 5);
			}
			break;
		case KEY_VOLUMEDOWN:
			if (hts_gatt_attr[5].notify_ind_enable && bt_manager_ble_ready_send_mass_data()) {
				send_buf[0] = 0;
				send_buf[1] = 41;
				bt_manager_notifyind_send(&hts_gatt_attr[4], &hts_gatt_attr[5], send_buf, 5);
			}
			break;
			
		case KEY_MENU:
			if (hrs_gatt_attr[2].notify_ind_enable && bt_manager_ble_ready_send_mass_data()) {
				send_buf[0] = 0x19;
				send_buf[1] = 60;
				bt_manager_notifyind_send(&hrs_gatt_attr[1], &hrs_gatt_attr[2], send_buf, 7);
			}
			break;

		default:
			break;
	}
	return 0;
}

int	bt_manager_bqb_key_deal(u16_t key_value)
{
	SYS_LOG_INF("key_value:0x%x", key_value);

	//检测到按键退出host_bqb
#if (defined CONFIG_ACTIONS_HOST_BQB) && \
	((defined CONFIG_BT_BQB_MODE) || (defined CONFIG_ACTIONS_FCC))
	if(key_value > 0)
	{
#ifdef CONFIG_ACTIONS_FCC
		extern void fcc_update_rtc_regs(void);
		sys_write32(0, (0xC0120000+0x30));
        fcc_update_rtc_regs();
#endif
		bteg_set_bqb_flag(0);
		watchdog_stop();
		watchdog_start(100);
		while(1);
	}
#endif

#ifdef CONFIG_BT_TRANSCEIVER
	if (sys_get_current_transceive_mode() == 0)
	{
		bt_manager_br_key_deal(key_value);
		bt_manager_ble_key_deal(key_value);
	}
#else
	bt_manager_br_key_deal(key_value);
	bt_manager_ble_key_deal(key_value);
#endif
	return 0;
}
*/
int	bt_manager_bqb_init(void)
{
	bt_manager_ble_bas_init();
	bt_manager_ble_cts_init();
	bt_manager_ble_hts_init();
	bt_manager_ble_hrs_init();
	bt_manager_ble_dis_init();
    memset(&ble_bqb_addr, 0, sizeof(bt_addr_t));
	SYS_LOG_INF("");
	return 0;
}

#endif
