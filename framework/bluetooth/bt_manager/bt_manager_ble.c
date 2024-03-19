/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt ble manager.
 */
#define SYS_LOG_DOMAIN "btmgr_ble"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <msg_manager.h>
#include <mem_manager.h>
#include <acts_bluetooth/host_interface.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include "bt_porting_inner.h"
#include <sys_event.h>

/* TODO: New only support one ble connection */
#define BLE_ADV_WITH_SET_NAME			0
#define BLE_ADV_DATA_MAX_LEN			31
#define BLE_ADV_MAX_ARRAY_SIZE			4

#define BLE_CONNECT_INTERVAL_CHECK		(5000)		/* 5s */
#define BLE_CONNECT_DELAY_UPDATE_PARAM	(4000)
#define BLE_UPDATE_PARAM_FINISH_TIME	(4000)
#define BLE_DELAY_UPDATE_PARAM_TIME		(50)
#define BLE_NOINIT_INTERVAL				(0xFFFF)
#define BLE_DEFAULT_IDLE_INTERVAL		(80)		/* 80*1.25 = 100ms */
#define BLE_TRANSFER_INTERVAL			(12)		/* 12*1.25 = 15ms */
#define BLE_SWITCH_DISCONNECT_REASON	(120)		/* 0x78 */
#define BLE_SWITCH_CHECK_TIMEOUT		(200)		/* 200ms */
#define BLE_DELAY_EXCHANGE_MTU_TIME		(200)		/* 200ms */
#define BLE_CHECK_IS_LEA_DEVICE_TIME	(4000)


enum {
	PARAM_UPDATE_EXCHANGE_MTU,
	PARAM_UPDATE_IDLE_STATE,
	PARAM_UPDATE_WAITO_UPDATE_STATE,
	PARAM_UPDATE_RUNING,
};

enum {
	BLE_SWITCH_STATE_IDLE,
	BLE_SWITCH_STATE_START,
	BLE_SWITCH_STATE_SEND_SYNC_INFO,
	BLE_SWITCH_STATE_WAIT_ACK_SYNC,
	BLE_SWITCH_STATE_SYNC_FINISH,
	BLE_SWITCH_STATE_DO_SWITCH,
};

enum {
	BLE_SWITCH_CMD_SYNC_STACK_INFO,
	BLE_SWITCH_CMD_ACK_STACK_INFO,
};

struct ble_switch_cmd_info {
	uint8_t id;
	uint8_t cmd;
	uint16_t ble_current_interval;
};

static OS_MUTEX_DEFINE(ble_mgr_lock);
static os_sem ble_ind_sem __IN_BT_SECTION;
static struct bt_gatt_indicate_params ble_ind_params __IN_BT_SECTION;
static sys_slist_t ble_list __IN_BT_SECTION;
static btmgr_le_scan_result_cb le_scan_result_cb;
static const struct tws_snoop_switch_cb_func ble_mgr_snoop_cb;
#ifdef CONFIG_BT_ADV_MANAGER
static const btmgr_adv_register_func_t ble_adv_funcs;
#endif


struct ble_conn{
	struct bt_conn *conn;
	uint16_t handle;
	uint8_t* addr;
	u32_t connected_time;
	u32_t flag_num;	

	os_delayed_work ble_delay_work;	
	uint8_t is_lea:1;
	uint8_t update_work_state:4;
	uint16_t conn_rxtx_cnt;	
	uint16_t ble_current_interval;
	uint16_t ble_idle_interval;
	
	/* For le switch */
	uint8_t switch_state;
	uint32_t switch_state_time;	

	sys_snode_t node;
	
};

struct ble_mgr_info {
	sys_slist_t conn_list;	/* list of struct ble_conn */

	uint8_t br_a2dp_runing:1;
	uint8_t br_hfp_runing:1;
	uint8_t initialized : 1;
	uint8_t adv_enable;

	os_delayed_work delay_work;	

	struct ble_advertising_params_t adv_params;
	uint8_t ad_data[BLE_ADV_DATA_MAX_LEN];
	uint8_t sd_data[BLE_ADV_DATA_MAX_LEN];

	/* For le switch */
	void *sync_info;

};

static struct ble_mgr_info ble_info;

static void ble_delay_work_handler(struct k_work *work);
static void ble_mgr_adv_update(int enable);

static int ble_is_connected(void)
{
	struct ble_conn *ble_conn;
	int num = 0;
	
	os_sched_lock();
	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		num++;
	}
	os_sched_unlock();
	return num;
}


static struct ble_conn *find_ble_conn_by_conn(struct bt_conn *conn)
{
	struct ble_conn *ble_conn;

	if (!conn)
		return NULL;
	
	os_sched_lock();
	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		if (ble_conn->conn == conn){
			os_sched_unlock();			
			return ble_conn;
		}
	}
	os_sched_unlock();
	
	return NULL;
}


static struct ble_conn *find_ble_conn_by_addr(bt_addr_t* addr)
{
	struct ble_conn *ble_conn;

	if (!addr)
		return NULL;

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		if (addr && !memcmp((char*)addr->val, (char*)ble_conn->addr, 6)){
			os_sched_unlock();			
			return ble_conn;
		}	
	}
	os_sched_unlock();
	
	return NULL;
}


static struct ble_conn* new_ble_conn(struct bt_conn* conn)
{
	struct ble_conn *ble_conn;
	struct bt_conn_info info;
	static uint32_t index = 0;

	ble_conn = mem_malloc(sizeof(struct ble_conn));
	if (!ble_conn) {
		SYS_LOG_ERR("no mem");
		return NULL;
	}
	memset(ble_conn, 0, sizeof(struct ble_conn));
	ble_conn->conn = hostif_bt_conn_ref(conn);
	ble_conn->handle= hostif_bt_conn_get_handle(conn);

	hostif_bt_conn_get_info(conn, &info);
	ble_conn->addr = (uint8_t*)info.le.dst->a.val;

	SYS_LOG_INF("MAC %02x:%02x:%02x:%02x:%02x:%02x", ble_conn->addr[5], ble_conn->addr[4], 
		ble_conn->addr[3], ble_conn->addr[2], ble_conn->addr[1], ble_conn->addr[0]);

	ble_conn->connected_time = os_uptime_get_32();
	ble_conn->flag_num = index++;

	ble_conn->ble_current_interval = BLE_NOINIT_INTERVAL;
	ble_conn->ble_idle_interval = BLE_DEFAULT_IDLE_INTERVAL;

	os_delayed_work_init(&ble_conn->ble_delay_work, ble_delay_work_handler);

	os_sched_lock();
	sys_slist_append(&ble_info.conn_list, &ble_conn->node);
	os_sched_unlock();

	os_delayed_work_submit(&ble_info.delay_work, BLE_CHECK_IS_LEA_DEVICE_TIME);

	return ble_conn;
}


static int delete_ble_conn(struct bt_conn* conn)
{
	struct ble_conn *ble_conn;

	ble_conn = find_ble_conn_by_conn(conn);
	if (!ble_conn) {
		SYS_LOG_ERR("not found");
		return -ENODEV;
	}

	os_delayed_work_cancel(&ble_conn->ble_delay_work);

	os_sched_lock();

	hostif_bt_conn_unref(ble_conn->conn);

	sys_slist_find_and_remove(&ble_info.conn_list, &ble_conn->node);

	mem_free(ble_conn);

	os_sched_unlock();

	return 0;
}


static int ble_leaudio_device_callback(uint16_t handle, uint8_t is_lea)
{
	struct ble_conn *ble_conn;
	
	os_sched_lock();
	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		if (ble_conn->handle == handle){
			ble_conn->is_lea = is_lea?1:0;
			os_sched_unlock();			
			return 0;
		}
	}
	os_sched_unlock();
	
	return 1;
}



static uint8_t ble_format_adv_data(struct bt_data *ad, uint8_t *ad_data)
{
	uint8_t ad_len = 0;
	uint8_t i, pos = 0;

	for (i = 0; i < BLE_ADV_MAX_ARRAY_SIZE; i++) {
		if (ad_data[pos] == 0) {
			break;
		}

		if ((pos + ad_data[pos] + 1) > BLE_ADV_DATA_MAX_LEN) {
			break;
		}

		ad[ad_len].data_len = ad_data[pos] - 1;
		ad[ad_len].type = ad_data[pos + 1];
		ad[ad_len].data = &ad_data[pos + 2];
		ad_len++;

		pos += (ad_data[pos] + 1);
	}

	return ad_len;
}

static bool ble_ad_has_name(struct bt_data *ad, uint8_t ad_len)
{
	uint8_t i;

	for (i = 0; i < ad_len; i++) {
		if (ad[i].type == BT_DATA_NAME_COMPLETE ||
		    ad[i].type == BT_DATA_NAME_SHORTENED) {
			return true;
		}
	}

	return false;
}

static void ble_advertise(void)
{
	struct bt_le_adv_param param;
	struct bt_data ad[BLE_ADV_MAX_ARRAY_SIZE], sd[BLE_ADV_MAX_ARRAY_SIZE];
	size_t ad_len, sd_len;
	int err;

	ad_len = ble_format_adv_data((struct bt_data *)ad, ble_info.ad_data);
	sd_len = ble_format_adv_data((struct bt_data *)sd, ble_info.sd_data);

	memset(&param, 0, sizeof(param));
	param.id = BT_ID_DEFAULT;
	param.interval_min = ble_info.adv_params.advertising_interval_min;
	param.interval_max = ble_info.adv_params.advertising_interval_max;
	if (ble_ad_has_name((struct bt_data *)ad, ad_len) || ble_ad_has_name((struct bt_data *)sd, sd_len)) {
		param.options = (BT_LE_ADV_OPT_CONNECTABLE);
	} else {
		param.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME);
	}

	err = hostif_bt_le_adv_start(&param, (ad_len ? (const struct bt_data *)ad : NULL), ad_len,
								(sd_len ? (const struct bt_data *)sd : NULL), sd_len);
	if (err < 0 && err != (-EALREADY)) {
		SYS_LOG_ERR("Failed to start advertising (err %d)", err);
	} else {
		SYS_LOG_INF("Advertising started");
	}
}

static uint16_t ble_param_interval_to_latency(uint16_t default_interval, uint16_t set_interval)
{
	uint16_t latency;

	latency = set_interval/default_interval;
	if (latency >= 1) {
		latency--;
	}

	return latency;
}

static void exchange_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
	SYS_LOG_INF("conn:%p, Exchange %s\n", conn, err == 0 ? "successful" : "failed");
}

static struct bt_gatt_exchange_params exchange_params = {
	.func = exchange_func,
};

static void ble_delay_work_handler(struct k_work *work)
{
	struct ble_conn * ble_conn = CONTAINER_OF(work, struct ble_conn, ble_delay_work);

	uint16_t req_interval;
	uint16_t conn_rxtx_cnt;
	uint8_t conn_busy = 0;
	if (ble_conn && ble_conn->conn) {
		struct bt_le_conn_param param;

		if (ble_conn->update_work_state == PARAM_UPDATE_EXCHANGE_MTU) {
			if (exchange_params.func && !ble_conn->is_lea) {
				hostif_bt_gatt_exchange_mtu(ble_conn->conn, &exchange_params);
			}

			ble_conn->conn_rxtx_cnt = 0;
			ble_conn->update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
			os_delayed_work_submit(&ble_conn->ble_delay_work, BLE_CONNECT_DELAY_UPDATE_PARAM);
			return;
		}

		if (ble_conn->switch_state != BLE_SWITCH_STATE_IDLE) {
			ble_conn->update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
			os_delayed_work_submit(&ble_conn->ble_delay_work, BLE_CONNECT_DELAY_UPDATE_PARAM);
			return;
		}

		if (ble_conn->update_work_state == PARAM_UPDATE_RUNING) {
			ble_conn->update_work_state = PARAM_UPDATE_IDLE_STATE;
			ble_conn->conn_rxtx_cnt = hostif_bt_conn_get_rxtx_cnt(ble_conn->conn);
			os_delayed_work_submit(&ble_conn->ble_delay_work, BLE_CONNECT_INTERVAL_CHECK);
			return;
		}

		if (ble_conn->update_work_state == PARAM_UPDATE_IDLE_STATE) {
			conn_rxtx_cnt = hostif_bt_conn_get_rxtx_cnt(ble_conn->conn);
			if (ble_conn->conn_rxtx_cnt != conn_rxtx_cnt) {
				ble_conn->conn_rxtx_cnt = conn_rxtx_cnt;
				conn_busy = 1;
			}
		}

		if (ble_info.br_a2dp_runing || ble_info.br_hfp_runing) {
			req_interval = ble_conn->ble_idle_interval;
		} else if (conn_busy) {
			req_interval = BLE_TRANSFER_INTERVAL;
		} else {
			req_interval = ble_conn->ble_idle_interval;
		}

		if (req_interval == ble_conn->ble_current_interval) {
			ble_conn->update_work_state = PARAM_UPDATE_IDLE_STATE;
			ble_conn->conn_rxtx_cnt = hostif_bt_conn_get_rxtx_cnt(ble_conn->conn);
			os_delayed_work_submit(&ble_conn->ble_delay_work, BLE_CONNECT_INTERVAL_CHECK);
			return;
		}

		ble_conn->ble_current_interval = req_interval;

		/* interval time: x*1.25 */
		param.interval_min = BLE_TRANSFER_INTERVAL;
		param.interval_max = BLE_TRANSFER_INTERVAL;
		param.latency = ble_param_interval_to_latency(BLE_TRANSFER_INTERVAL, req_interval);
		param.timeout = 500;
		if (!ble_conn->is_lea){		
			hostif_bt_conn_le_param_update(ble_conn->conn, &param);
			SYS_LOG_INF("%d\n", req_interval);
		}
		
		ble_conn->update_work_state = PARAM_UPDATE_RUNING;
		os_delayed_work_submit(&ble_conn->ble_delay_work, BLE_UPDATE_PARAM_FINISH_TIME);
	}

}

static void ble_check_update_param(struct ble_conn * ble_conn)
{
	if (ble_conn->update_work_state == PARAM_UPDATE_IDLE_STATE) {
		ble_conn->update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
		os_delayed_work_submit(&ble_conn->ble_delay_work, BLE_DELAY_UPDATE_PARAM_TIME);
	} else if (ble_conn->update_work_state == PARAM_UPDATE_RUNING) {
		ble_conn->update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
		os_delayed_work_submit(&ble_conn->ble_delay_work, BLE_UPDATE_PARAM_FINISH_TIME);
	} else {
		/* Already in PARAM_UPDATE_WAITO_UPDATE_STATE */
	}
}

static void ble_send_data_check_interval(struct ble_conn * ble_conn)
{
	if (!ble_conn)
		return;
	
	if ((ble_conn->ble_current_interval != BLE_TRANSFER_INTERVAL) &&
		!ble_info.br_a2dp_runing && !ble_info.br_hfp_runing) {
		ble_check_update_param(ble_conn);
	}
}

static void ble_check_device_delay_work_handler(struct k_work *work)
{
	struct ble_conn *ble_conn;	
	struct ble_conn *ble_conn1 = NULL;
	struct ble_conn *ble_conn2 = NULL;
	int num = 0;

	if (ble_is_connected() != 2){
		return;
	}

	os_sched_lock();
	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		if (num == 0)
			ble_conn1 = ble_conn;
		else
			ble_conn2 = ble_conn;
		num++;
	}
	os_sched_unlock();

	if (!ble_conn1 || !ble_conn2){
		return;
	}

	if (ble_conn1->is_lea || ble_conn2->is_lea){
		return;
	}

	if (ble_conn1->flag_num <  ble_conn2->flag_num){
		bt_manager_ble_disconnect((bt_addr_t*)ble_conn2->addr);
	}else{
		bt_manager_ble_disconnect((bt_addr_t*)ble_conn1->addr);
	}
}

static void notify_ble_connected(uint8_t *mac, uint8_t connected)
{
	struct ble_reg_manager *le_mgr;

	os_mutex_lock(&ble_mgr_lock, OS_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_list, le_mgr, node) {
		if (le_mgr->link_cb) {
			le_mgr->link_cb(mac, connected);
		}
	}

	os_mutex_unlock(&ble_mgr_lock);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;
	char *p_info;
	struct ble_switch_cmd_info *cmd;
	struct ble_conn *ble_conn;	
#ifdef CONFIG_BT_TRANSCEIVER
    if(err)
    {
        return;
    }
    uint8_t role = bt_conn_get_role(conn);

    if (role == BT_HCI_ROLE_MASTER)
    {
        return;
    }
#endif

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)) {
		return;
	}

	ble_conn = find_ble_conn_by_conn(conn);

	if (!ble_conn) {
		ble_conn =  new_ble_conn(conn);
		notify_ble_connected(ble_conn->addr, true);

		if (hostif_bt_is_le_switch_connected(conn) && ble_info.sync_info) {
			p_info = (char *)ble_info.sync_info;
			cmd = (struct ble_switch_cmd_info *)p_info;
			if (cmd->cmd == BLE_SWITCH_CMD_SYNC_STACK_INFO) {
				ble_conn->ble_current_interval = cmd->ble_current_interval;
				hostif_bt_le_stack_set_sync_info(&p_info[sizeof(struct ble_switch_cmd_info)],
												hostif_bt_le_stack_sync_info_len());
			} else {
				SYS_LOG_WRN("Without le sync stack info");
			}

			bt_mem_free(ble_info.sync_info);
			ble_info.sync_info = NULL;

			ble_conn->conn_rxtx_cnt = 0;
			ble_conn->update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
			os_delayed_work_submit(&ble_conn->ble_delay_work, BLE_CONNECT_DELAY_UPDATE_PARAM);
		} else {
			ble_conn->conn_rxtx_cnt = 0;
			ble_conn->ble_current_interval = BLE_NOINIT_INTERVAL;
			ble_conn->update_work_state = PARAM_UPDATE_EXCHANGE_MTU;
			os_delayed_work_submit(&ble_conn->ble_delay_work, BLE_DELAY_EXCHANGE_MTU_TIME);
		}
	} else {
		SYS_LOG_ERR("Already connected");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info info;
	struct ble_conn *ble_conn;	

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)) {
		return;
	}

	ble_conn = find_ble_conn_by_conn(conn);

	if (ble_conn) {
		SYS_LOG_INF("MAC %02x:%02x:%02x:%02x:%02x:%02x", ble_conn->addr[5], ble_conn->addr[4], 
			ble_conn->addr[3], ble_conn->addr[2], ble_conn->addr[1], ble_conn->addr[0]);

		notify_ble_connected(ble_conn->addr, false);

		delete_ble_conn(conn);
		
		os_sem_give(&ble_ind_sem);

		/* param.options set BT_LE_ADV_OPT_ONE_TIME,
		 * need advertise again after disconnect.
		 */
#ifndef CONFIG_BT_EARPHONE_SPEC
		if (reason != BLE_SWITCH_DISCONNECT_REASON) {
			ble_mgr_adv_update(1);
	#ifndef CONFIG_BT_ADV_MANAGER			
			ble_advertise();
	#endif
		}
#endif
	} else {
		SYS_LOG_ERR("Error conn %p(%p)", ble_conn, conn);
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	SYS_LOG_INF("int (0x%04x, 0x%04x) lat %d to %d", param->interval_min,
		param->interval_max, param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	SYS_LOG_INF("int 0x%x lat %d to %d", interval, latency, timeout);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

static int ble_notify_data(struct bt_conn *conn, struct bt_gatt_attr *attr, uint8_t *data, uint16_t len)
{
	int ret;

	ret = hostif_bt_gatt_notify(conn, attr, data, len);
	if (ret < 0) {
		return ret;
	} else {
		return (int)len;
	}
}

static void ble_indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *attr,
			  uint8_t err)
{
	os_sem_give(&ble_ind_sem);
}

static int ble_indicate_data(struct bt_conn *conn, struct bt_gatt_attr *attr, uint8_t *data, uint16_t len)
{
	int ret;

	if (os_sem_take(&ble_ind_sem, OS_NO_WAIT) < 0) {
		return -EBUSY;
	}

	if (!conn) {
		return -EIO;
	}

	ble_ind_params.attr = attr;
	ble_ind_params.func = ble_indicate_cb;
	ble_ind_params.len = len;
	ble_ind_params.data = data;

	ret = hostif_bt_gatt_indicate(conn, &ble_ind_params);
	if (ret < 0) {
		return ret;
	} else {
		return (int)len;
	}
}

uint16_t bt_manager_get_ble_mtu(bt_addr_t* addr)
{
	struct ble_conn *ble_conn;	
	ble_conn = find_ble_conn_by_addr(addr);

	return ble_conn ? hostif_bt_gatt_get_mtu(ble_conn->conn) : 0;
}

int bt_manager_ble_send_data(bt_addr_t* addr, struct bt_gatt_attr *chrc_attr,
					struct bt_gatt_attr *des_attr, uint8_t *data, uint16_t len)
{
	struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)(chrc_attr->user_data);
	struct ble_conn *ble_conn;	
	ble_conn = find_ble_conn_by_addr(addr);

	if (!ble_conn) {
		return -EIO;
	}

	if (1 != bt_manager_ble_ready_send_data(addr)) {
		return -EBUSY;
	}

	if (len > (bt_manager_get_ble_mtu(addr) - 3)) {
		return -EFBIG;
	}

	ble_send_data_check_interval(ble_conn);

	if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
		return ble_notify_data(ble_conn->conn, des_attr, data, len);
	} else if (chrc->properties & BT_GATT_CHRC_INDICATE) {
		return ble_indicate_data(ble_conn->conn, des_attr, data, len);
	}

	/* Wait TODO */
	/* return ble_write_data(attr, data, len) */
	SYS_LOG_WRN("Wait todo");
	return -EIO;
}

void bt_manager_ble_disconnect(bt_addr_t* addr)
{
	int err;
	struct ble_conn *ble_conn;	
	
	if (!ble_info.initialized) {
		return;
	}	

	if (!addr) {
		SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
			err = hostif_bt_conn_disconnect(ble_conn->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			if (err) {
				SYS_LOG_INF("Disconnection failed (err %d)", err);
			}
		}	
		return;
	}
	
	ble_conn = find_ble_conn_by_addr(addr);

	if (!ble_conn) {
		return;
	}

	err = hostif_bt_conn_disconnect(ble_conn->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		SYS_LOG_INF("Disconnection failed (err %d)", err);
	}
}

void bt_manager_ble_service_reg(struct ble_reg_manager *le_mgr)
{
	os_mutex_lock(&ble_mgr_lock, OS_FOREVER);

	sys_slist_append(&ble_list, &le_mgr->node);
	hostif_bt_gatt_service_register(&le_mgr->gatt_svc);

	os_mutex_unlock(&ble_mgr_lock);
}

void bt_manager_ble_init(void)
{
	btmgr_ble_cfg_t *cfg_ble = bt_manager_ble_config();

	memset(&ble_info, 0, sizeof(ble_info));
	ble_info.initialized = 1;
	ble_info.adv_params.advertising_interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	ble_info.adv_params.advertising_interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

	ble_info.ad_data[0] = 2;	/* Len */
	ble_info.ad_data[1] = BT_DATA_FLAGS;	/* Type */
	if (cfg_ble->ble_address_type) {
		ble_info.ad_data[2] = (BT_LE_AD_GENERAL);
	} else {
		/* 0: Public address, set BT_LE_AD_NO_BREDR, or trigger connect le will connect br */
		ble_info.ad_data[2] = (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR);
	}

#if BLE_ADV_WITH_SET_NAME
	ble_info.ad_data[3] = strlen("lark_le") + 1;
	ble_info.ad_data[4] = BT_DATA_NAME_COMPLETE;
	memcpy(&ble_info.ad_data[5], "lark_le", strlen("lark_le"));
#endif

	sys_slist_init(&ble_list);
	os_sem_init(&ble_ind_sem, 1, 1);

	hostif_bt_conn_cb_register(&conn_callbacks);
	bt_manager_tws_reg_snoop_switch_cb_func((struct tws_snoop_switch_cb_func *)&ble_mgr_snoop_cb, true);
	bt_manager_audio_leaudio_dev_cb_register(ble_leaudio_device_callback);

	os_delayed_work_init(&ble_info.delay_work, ble_check_device_delay_work_handler);

#ifdef CONFIG_BT_ADV_MANAGER
	btmgr_adv_register(ADV_TYPE_BLE, (btmgr_adv_register_func_t*)&ble_adv_funcs);
#endif

#ifndef CONFIG_BT_EARPHONE_SPEC
	ble_mgr_adv_update(1);

#ifndef CONFIG_BT_ADV_MANAGER 		
	ble_advertise();
#endif

#endif
}

void bt_manager_ble_deinit(void)
{
	uint32_t time_out = 0;
	if (!ble_info.initialized)
		return;
		
	ble_mgr_adv_update(0);

#ifdef CONFIG_BT_ADV_MANAGER
	btmgr_adv_register(ADV_TYPE_BLE, NULL);
#else
	hostif_bt_le_adv_stop();
#endif

	if (ble_is_connected()) {
		bt_manager_ble_disconnect(NULL);
		while (ble_is_connected() && time_out++ < 500) {
			os_sleep(10);
		}
	}

	os_delayed_work_cancel(&ble_info.delay_work);

	hostif_bt_conn_cb_unregister(&conn_callbacks);
	bt_manager_tws_reg_snoop_switch_cb_func((struct tws_snoop_switch_cb_func *)&ble_mgr_snoop_cb, false);
	ble_info.initialized = 0;

	SYS_LOG_INF("\n");
}

void bt_manager_ble_a2dp_play_notify(bool play)
{
	struct ble_conn *ble_conn;	

	ble_info.br_a2dp_runing = (play) ? 1 : 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		ble_check_update_param(ble_conn);
	}	
	
}

void bt_manager_ble_hfp_play_notify(bool play)
{
	struct ble_conn *ble_conn;	

	ble_info.br_hfp_runing = (play) ? 1 : 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		ble_check_update_param(ble_conn);
	}
}

void bt_manager_halt_ble(void)
{
	/* If ble connected, what todo */
#ifndef CONFIG_BT_EARPHONE_SPEC
	if (!ble_info.initialized) {
		return;
	}

	if (!ble_is_connected()) {
		ble_mgr_adv_update(0);
		
#ifndef CONFIG_BT_ADV_MANAGER 		
		hostif_bt_le_adv_stop();
#endif	
	}
#endif
}

void bt_manager_resume_ble(void)
{
	/* If ble connected, what todo */
#ifndef CONFIG_BT_EARPHONE_SPEC
	if (!ble_info.initialized) {
		return;
	}

	if (!ble_is_connected()) {
		ble_mgr_adv_update(1);
#ifndef CONFIG_BT_ADV_MANAGER 		
		ble_advertise();
#endif
	}
#endif
}

int bt_manager_ble_is_connected(void)
{
	if (!ble_info.initialized) {
		return 0;
	}
	//return ble_is_connected() ? 1 : 0;
	return ble_is_connected();
}

int bt_manager_ble_set_advertising_params(struct ble_advertising_params_t *params)
{
	SYS_LOG_INF("Le adv params 0x%x 0x%x %d %d 0x%x 0x%x", params->advertising_interval_min,
				params->advertising_interval_max, params->advertising_type, params->own_address_type,
				params->advertising_channel_map, params->advertising_filter_policy);
	memcpy(&ble_info.adv_params, params, sizeof(struct ble_advertising_params_t));
	return 0;
}

int bt_manager_ble_set_adv_data(uint8_t *ad_data, uint8_t ad_data_len, uint8_t *sd_data, uint8_t sd_data_len)
{
	if (ad_data && ad_data_len <= BLE_ADV_DATA_MAX_LEN) {
		memset(ble_info.ad_data, 0, BLE_ADV_DATA_MAX_LEN);
		memcpy(ble_info.ad_data, ad_data, ad_data_len);
	}

	if (sd_data && sd_data_len <= BLE_ADV_DATA_MAX_LEN) {
		memset(ble_info.sd_data, 0, BLE_ADV_DATA_MAX_LEN);
		memcpy(ble_info.sd_data, sd_data, sd_data_len);
	}

	return 0;
}

int bt_manager_ble_set_advertise_enable(uint8_t enable)
{
	if (!ble_info.initialized) {
		return 0;
	}
	SYS_LOG_INF("%d",enable);

	if (enable) {
		ble_mgr_adv_update(1);
#ifndef CONFIG_BT_ADV_MANAGER 		
		ble_advertise();
#endif		
	} else {
		ble_mgr_adv_update(0);
#ifndef CONFIG_BT_ADV_MANAGER 		
		hostif_bt_le_adv_stop();
#endif	
	}

	return 0;
}

void bt_le_scan_result_cb(const bt_addr_le_t *addr, int8_t rssi,
								uint8_t adv_type, struct net_buf_simple *buf)
{
	if (le_scan_result_cb) {
		le_scan_result_cb(addr->type, (uint8_t *)addr->a.val, rssi, adv_type, buf->data, buf->len);
	}
}

int bt_manager_ble_start_scan(uint8_t scan_type, uint8_t own_addr_type, uint32_t options,
										uint16_t interval_ms, uint16_t window_ms, btmgr_le_scan_result_cb cb)
{
	struct bt_le_scan_param param;
	uint32_t val;

	ARG_UNUSED(own_addr_type);		/* Auto set in stack */

	le_scan_result_cb = cb;
	memset(&param, 0, sizeof(param));
	param.type = scan_type;
	param.options = options;
	val = interval_ms;
	param.interval = (uint16_t)(val * 1000 / 625);
	val = window_ms;
	param.window = (uint16_t)(val * 1000 / 625);

	return hostif_bt_le_scan_start(&param, bt_le_scan_result_cb);
}

int bt_manager_ble_stop_scan(void)
{
	int ret = hostif_bt_le_scan_stop();

	le_scan_result_cb = NULL;
	return ret;
}

int bt_manager_ble_add_whitelist(uint8_t addr_type, uint8_t *addr)
{
	bt_addr_le_t addr_le;

	addr_le.type = addr_type;
	hostif_bt_addr_copy(&addr_le.a, (const bt_addr_t *)addr);

	return hostif_bt_le_whitelist_add(&addr_le);
}

int bt_manager_ble_remove_whitelist(uint8_t addr_type, uint8_t *addr)
{
	bt_addr_le_t addr_le;

	addr_le.type = addr_type;
	hostif_bt_addr_copy(&addr_le.a, (const bt_addr_t *)addr);

	return hostif_bt_le_whitelist_rem(&addr_le);
}

int bt_manager_ble_clear_whitelist(void)
{
	return hostif_bt_le_whitelist_clear();
}

int bt_manager_ble_ready_send_data(bt_addr_t* addr)
{
	struct ble_conn *ble_conn;	
	ble_conn = find_ble_conn_by_addr(addr);
	
	if (!ble_conn) {
		return 0;
	}

	return hostif_bt_le_conn_ready_send_data(ble_conn->conn);
}

int bt_manager_get_ble_wake_lock(void)
{
	struct ble_conn *ble_conn;	
	int wake_lock = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {

		if (ble_conn->ble_current_interval != BLE_DEFAULT_IDLE_INTERVAL)
			wake_lock++;
	}

	return wake_lock;
}

static int ble_mgr_send_sync_info(struct ble_conn* ble_conn)
{
	char *p_info;
	struct ble_switch_cmd_info *cmd;
	int sync_len = hostif_bt_le_stack_sync_info_len();
	int ret = -EINVAL;

	sync_len += sizeof(struct ble_switch_cmd_info);
	p_info = bt_mem_malloc(sync_len);
	if (!p_info) {
		return -EIO;
	}

	memset(p_info, 0, sync_len);
	cmd = (struct ble_switch_cmd_info *)p_info;
	cmd->id = BT_SWITCH_CB_ID_BLE_MGR;
	cmd->cmd = BLE_SWITCH_CMD_SYNC_STACK_INFO;
	cmd->ble_current_interval = ble_conn->ble_current_interval;

	if (hostif_bt_le_stack_get_sync_info(ble_conn->conn, &p_info[sizeof(struct ble_switch_cmd_info)])) {
		bt_manager_tws_send_snoop_switch_sync_info(p_info, sync_len);
		ret = 0;
	}
	bt_mem_free(p_info);
	return ret;
}

static void bt_mgr_ble_do_switch(struct ble_conn* ble_conn)
{
	uint32_t time;

	SYS_LOG_INF("state %d", ble_conn->switch_state);
	switch (ble_conn->switch_state) {
	case BLE_SWITCH_STATE_START:
	case BLE_SWITCH_STATE_SEND_SYNC_INFO:
		if (hostif_bt_le_stack_ready_sync_info(ble_conn->conn)) {
			if (ble_mgr_send_sync_info(ble_conn) == 0) {
				ble_conn->switch_state = BLE_SWITCH_STATE_WAIT_ACK_SYNC;
				ble_conn->switch_state_time = os_uptime_get_32();
			}
		}
		break;
	case BLE_SWITCH_STATE_WAIT_ACK_SYNC:
		if ((os_uptime_get_32() - ble_conn->switch_state_time) > BLE_SWITCH_CHECK_TIMEOUT) {
			ble_conn->switch_state = BLE_SWITCH_STATE_SEND_SYNC_INFO;
		}
		break;
	case BLE_SWITCH_STATE_SYNC_FINISH:
		ble_conn->switch_state = BLE_SWITCH_STATE_DO_SWITCH;
		ble_conn->switch_state_time = os_uptime_get_32();
		hostif_bt_vs_switch_le_link(ble_conn->conn);
		break;
	case BLE_SWITCH_STATE_DO_SWITCH:
		time = os_uptime_get_32();
		if ((time - ble_conn->switch_state_time) > BLE_SWITCH_CHECK_TIMEOUT) {
			ble_conn->switch_state = BLE_SWITCH_STATE_SYNC_FINISH;
		}
	default:
		break;
	}
}

bool bt_manager_ble_check_switch_finish(void)
{
	struct ble_conn *ble_conn;	
	int num = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		if(!ble_conn->is_lea){
			bt_mgr_ble_do_switch(ble_conn);
			num++;
		}
	}

	return num ? false : true;
}

static void ble_mgr_start_switch_cb(uint8_t id)
{
	struct ble_conn *ble_conn;	

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		if(!ble_conn->is_lea){
			SYS_LOG_INF("start %p %d", ble_conn->conn, ble_conn->switch_state);
			hostif_bt_conn_set_snoop_switch_lock(ble_conn->conn, 1);
			ble_conn->switch_state = BLE_SWITCH_STATE_START;
			break;
		}
	}
}

static int ble_mgr_check_ready_switch_cb(uint8_t id)
{
	int ret = 1;

	struct ble_conn *ble_conn;	

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		if(!ble_conn->is_lea){
			if (ble_conn->switch_state == BLE_SWITCH_STATE_START) {
				if (!hostif_bt_le_stack_ready_sync_info(ble_conn->conn) &&
					(ble_conn->update_work_state > PARAM_UPDATE_EXCHANGE_MTU)) {
					ret = 0;
				}
			}
			SYS_LOG_INF("state %d ret %d", ble_conn->switch_state, ret);		
		}
	}

	return ret;
}

static void ble_mgr_switch_finish_cb(uint8_t id, uint8_t is_master)
{
	SYS_LOG_INF("Switch finish new role %d", is_master);
	struct ble_conn *ble_conn;	

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
		if(!ble_conn->is_lea){
			ble_conn->switch_state = BLE_SWITCH_STATE_IDLE;
			hostif_bt_conn_set_snoop_switch_lock(ble_conn->conn, 0);			
		}
	}
}

static void ble_mgr_rx_sync_info_cb(uint8_t id, uint8_t *data, uint16_t len)
{
	struct ble_switch_cmd_info *cmd = (struct ble_switch_cmd_info *)data;
	struct ble_switch_cmd_info ack_cmd;
	struct ble_conn *ble_conn;	

	if (cmd->cmd == BLE_SWITCH_CMD_SYNC_STACK_INFO) {
		if (ble_is_connected()) {
			return;
		}

		if (ble_info.sync_info) {
			bt_mem_free(ble_info.sync_info);
			ble_info.sync_info = NULL;
		}

		ble_info.sync_info = bt_mem_malloc(len);
		if (!ble_info.sync_info) {
			return;
		} else {
			memcpy(ble_info.sync_info, data, len);
		}

		ack_cmd.id = BT_SWITCH_CB_ID_BLE_MGR;
		ack_cmd.cmd = BLE_SWITCH_CMD_ACK_STACK_INFO;
		bt_manager_tws_send_snoop_switch_sync_info((uint8_t *)&ack_cmd, sizeof(ack_cmd));
	} else if (cmd->cmd == BLE_SWITCH_CMD_ACK_STACK_INFO) {

		SYS_SLIST_FOR_EACH_CONTAINER(&ble_info.conn_list, ble_conn, node) {
			if(!ble_conn->is_lea){
				if (ble_conn->conn && (ble_conn->switch_state == BLE_SWITCH_STATE_WAIT_ACK_SYNC)) {
					ble_conn->switch_state = BLE_SWITCH_STATE_SYNC_FINISH;
				}			
			}
		}
	}
}

static const struct tws_snoop_switch_cb_func ble_mgr_snoop_cb = {
	.id = BT_SWITCH_CB_ID_BLE_MGR,
	.start_cb = ble_mgr_start_switch_cb,
	.check_ready_cb = ble_mgr_check_ready_switch_cb,
	.finish_cb = ble_mgr_switch_finish_cb,
	.sync_info_cb = ble_mgr_rx_sync_info_cb,
};


static void ble_mgr_adv_update(int enable)
{
	if (enable){
		ble_info.adv_enable = ADV_STATE_BIT_ENABLE;
	}else{
		ble_info.adv_enable = ADV_STATE_BIT_DISABLE;
	}		
	ble_info.adv_enable |= ADV_STATE_BIT_UPDATE;
}

#ifdef CONFIG_BT_ADV_MANAGER

static int ble_mgr_adv_state(void)
{
	if(ble_info.adv_enable & ADV_STATE_BIT_ENABLE){
		/*if device has been connected 2 BR, don't advertising LEA*/
		if (bt_manager_get_connected_dev_num() >=2){
			return (ble_info.adv_enable&(~ADV_STATE_BIT_ENABLE));
		}

		/*If br calling, don't advertising  */
		if (bt_manager_audio_in_btcall()){
			return (ble_info.adv_enable&(~ADV_STATE_BIT_ENABLE));
		}
	}

	return ble_info.adv_enable;
}

static int ble_mgr_adv_enable(void)
{
	if (ble_info.adv_enable & ADV_STATE_BIT_ENABLE){
		ble_info.adv_enable &= ~ADV_STATE_BIT_UPDATE;
	}
	
	ble_advertise();

	return 0;
}

static int ble_mgr_adv_disable(void)
{
	if ((ble_info.adv_enable & ADV_STATE_BIT_ENABLE) == 0){
		ble_info.adv_enable &= ~ADV_STATE_BIT_UPDATE;
	}

	hostif_bt_le_adv_stop();
	return 0;
}


static const  btmgr_adv_register_func_t ble_adv_funcs = {
	.adv_get_state = ble_mgr_adv_state,
	.adv_enable = ble_mgr_adv_enable,
	.adv_disable = ble_mgr_adv_disable,
};
#endif


int bt_manager_conn_get_addr(struct bt_conn *conn, bt_addr_t* addr)
{
	struct bt_conn_info info;

	if (!conn || !addr)
		return -1;	

	if (hostif_bt_conn_get_info(conn, &info) == 0){
		memcpy(addr, &(info.le.dst->a),sizeof(bt_addr_t));
		return 0;		
	}
	return -1;	
}



