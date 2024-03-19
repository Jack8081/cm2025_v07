#define SYS_LOG_DOMAIN "tr_btmgr_ble"

#include <ringbuff_stream.h>
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
#include "api_common.h"

#include "xear_hid_protocol.h"
#include "xear_led_ctrl.h"
#include "usb/usb_custom_handler.h"
#include "usb/usb_custom_hid_info.h"

#ifdef CONFIG_BT_TRANSCEIVER
#ifndef CONFIG_HID_INTERRUPT_IN_EP_MPS
#define CONFIG_HID_INTERRUPT_IN_EP_MPS 64
#endif
#endif
struct ble_cli_mgr_info
{
	struct bt_conn *ble_conn;
    uint16_t handle;
	uint8_t device_mac[6];
	sys_snode_t node;

    uint8_t dis : 1;

    uint8_t set_parm_flag : 1;

	uint16_t start_handle;
	uint16_t end_handle;

    struct bt_le_conn_param conn_param;
    uint16_t interval;
    uint16_t latency;
    uint16_t timeout;

	void *uuid;

	struct bt_gatt_discover_params discover_params;
	struct bt_gatt_subscribe_params desc_sub_params;

    struct bt_gatt_attr *attr;
    uint16_t count;

	uint16_t write_handle;
	uint32_t last_write_tsp;
	uint32_t last_sync_conn_parma;
};

static OS_MUTEX_DEFINE(ble_cli_mgr_lock);
static sys_slist_t ble_cli_list;

void tr_bt_manager_ble_cli_rev(uint16_t handle, u8_t * buf, u16_t len);

static struct ble_cli_mgr_info *tr_le_cli_get_dev(struct bt_conn *conn)
{
    struct ble_cli_mgr_info *dev;

    os_mutex_lock(&ble_cli_mgr_lock, OS_FOREVER);

    SYS_SLIST_FOR_EACH_CONTAINER(&ble_cli_list, dev, node) {
        if(dev->ble_conn == conn)
        {
            os_mutex_unlock(&ble_cli_mgr_lock);
            return dev;
        }
    }
    os_mutex_unlock(&ble_cli_mgr_lock);
    return NULL;
}

static struct ble_cli_mgr_info *tr_le_cli_get_dev_by_handle(uint16_t handle)
{
    struct ble_cli_mgr_info *dev;
    os_mutex_lock(&ble_cli_mgr_lock, OS_FOREVER);
    SYS_SLIST_FOR_EACH_CONTAINER(&ble_cli_list, dev, node)
    {
        if(bt_conn_get_handle(dev->ble_conn) == handle)
        {
            os_mutex_unlock(&ble_cli_mgr_lock);
            return dev;
        }
    }
    os_mutex_unlock(&ble_cli_mgr_lock);
    return NULL;
}

static uint8_t le_cli_notify_handler(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	if (!data || (!length))
    {
		return BT_GATT_ITER_CONTINUE;
	}

    tr_bt_manager_ble_cli_rev(bt_conn_get_handle(conn), (u8_t *)data, length);
    return BT_GATT_ITER_CONTINUE;
}


static uint8_t cli_discover_func(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
    struct ble_cli_mgr_info *dev;

    dev = tr_le_cli_get_dev(conn);
	if (!dev || !attr)
    {
		SYS_LOG_DBG("Discovery complete for GATT %p", dev);
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		int err;

        struct bt_gatt_service_val *prim_service = (struct bt_gatt_service_val *)attr->user_data;

		dev->start_handle = attr->handle + 1;
		dev->end_handle = prim_service->end_handle;

        memset(&dev->discover_params, 0, sizeof(dev->discover_params));
		dev->discover_params.uuid = NULL;
		dev->discover_params.start_handle = dev->start_handle;
		dev->discover_params.end_handle = dev->end_handle;
		dev->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		dev->discover_params.func = cli_discover_func;

		err = bt_gatt_discover(conn, &dev->discover_params);
		if (err != 0) {
			SYS_LOG_DBG("Discover failed (err %d)", err);
		}

		return BT_GATT_ITER_STOP;
	}
    else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC)
    {
		struct bt_gatt_chrc *chrc;

		chrc = (struct bt_gatt_chrc *)attr->user_data;

        for(int i = 0; i < dev->count; i++)
        {
            if (!bt_uuid_cmp((dev->attr + i)->uuid, chrc->uuid))
            {
                (dev->attr + i)->handle = chrc->value_handle;
                if((chrc->properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) == BT_GATT_CHRC_WRITE_WITHOUT_RESP)
                {
                    dev->write_handle = chrc->value_handle;
                }
            }
        }

        if (chrc->properties & BT_GATT_CHRC_NOTIFY)
        {
            SYS_LOG_INF("BT_GATT_CHRC_NOTIFY\n");
            struct bt_gatt_subscribe_params *sub_params = &dev->desc_sub_params;
            memset(sub_params, 0, sizeof(*sub_params));
            sub_params->notify = le_cli_notify_handler;
			sub_params->value = 0xf5;
			sub_params->value_handle = chrc->value_handle;
			sub_params->ccc_handle = attr->handle + 2;
			bt_gatt_subscribe(conn, sub_params);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static int le_cli_discover(struct bt_conn *conn, struct bt_gatt_attr *attr, uint16_t count)
{
    struct ble_cli_mgr_info *dev;
	int err = -1;

    dev = tr_le_cli_get_dev(conn);
    if(dev == NULL)
    {
		SYS_LOG_ERR("Conn %p discover failed\n", conn);
        return err;
    }

    for(int i = 0; i < count; i++)
    {
		if (!bt_uuid_cmp((attr + i)->uuid, BT_UUID_GATT_PRIMARY))
        {
            dev->start_handle = 0;
            dev->end_handle = 0;
            dev->attr = attr;
            dev->count = count;

            (void)memset(&dev->discover_params, 0, sizeof(dev->discover_params));
            dev->uuid = (struct bt_uuid_128 *)(attr + i)->user_data;
            dev->discover_params.func = cli_discover_func;
            dev->discover_params.uuid = dev->uuid;
            dev->discover_params.type = BT_GATT_DISCOVER_PRIMARY;
            dev->discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
            dev->discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

            err = bt_gatt_discover(conn, &dev->discover_params);
            if (err) {
                SYS_LOG_ERR("Conn %p discover failed (err %d)", conn, err);
            } else {
                SYS_LOG_INF("Conn %p discover pending", conn);
            }
            break;
        }
    }

	return err;
}

extern struct bt_gatt_attr ota_gatt_attr[];
int get_ota_gatt_attr_size(void);

int ble_data_chan_discover(struct bt_conn *conn)
{
    return le_cli_discover(conn, ota_gatt_attr, get_ota_gatt_attr_size());
}

static void cli_exchange_func(struct bt_conn *conn, uint8_t err,
        struct bt_gatt_exchange_params *params)
{
    SYS_LOG_INF("Exchange %s\n", err == 0 ? "successful" : "failed");
    if(err) return;
    ble_data_chan_discover(conn);
}

static struct bt_gatt_exchange_params cli_exchange_params = {
    .func = cli_exchange_func,
};

#ifdef CONFIG_LE_AUDIO_BQB
void set_ble_pts_con(struct bt_conn *conn);
void clear_ble_pts_con(struct bt_conn *conn);
#endif

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[13];
	struct bt_conn_info info;
    struct ble_cli_mgr_info *dev;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)) {
		return;
	}

    if(info.role)
    {
        SYS_LOG_INF("connected role %d\n", info.role);
        return;
    }

    if(err)
    {
        SYS_LOG_ERR("connected err %d\n", err);
        return;
    }

    dev = tr_le_cli_get_dev(conn);
    if(dev)
    {
        SYS_LOG_ERR("Already connected");
        return;
    }

    dev = mem_malloc(sizeof(struct ble_cli_mgr_info));
    if(dev == NULL)
    {
        return;
    }
    memset(dev, 0, sizeof(struct ble_cli_mgr_info));
    memcpy(dev->device_mac, info.le.dst->a.val, 6);
    memset(addr, 0, 13);
    hostif_bt_addr_to_str((void *)dev->device_mac, addr, BT_ADDR_STR_LEN);
    SYS_LOG_INF("MAC: %s\n", addr);
    dev->handle = bt_conn_get_handle(conn);
    SYS_LOG_INF("ble data handle: %04x, %p\n", dev->handle, conn);

    dev->ble_conn = hostif_bt_conn_ref(conn);
	sys_slist_append(&ble_cli_list, &dev->node);

#ifdef CONFIG_LE_AUDIO_BQB
    set_ble_pts_con(conn);
#endif
    if (cli_exchange_params.func)
    {
        hostif_bt_gatt_exchange_mtu(dev->ble_conn, &cli_exchange_params);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[13];
	struct bt_conn_info info;
    struct ble_cli_mgr_info *dev;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)) {
		return;
	}

    dev = tr_le_cli_get_dev(conn);
    if(!dev)
    {
		SYS_LOG_ERR("Error conn %p(%p)", dev->ble_conn, conn);
        return;
    }

    memset(addr, 0, sizeof(addr));
    hostif_bt_addr_to_str((void *)dev->device_mac, addr, BT_ADDR_STR_LEN);
    SYS_LOG_INF("MAC: %s, reason: %d", addr, reason);

    if(dev->attr)
    {
        for(int i = 0; i < dev->count; i++)
        {
            (dev->attr + i)->handle = 0;
        }
    }
#ifdef CONFIG_LE_AUDIO_BQB
    clear_ble_pts_con(conn);
#endif

    hostif_bt_conn_unref(dev->ble_conn);
    sys_slist_find_and_remove(&ble_cli_list, &dev->node);
    mem_free(dev);
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
    struct ble_cli_mgr_info *dev;
    dev = tr_le_cli_get_dev(conn);
    if(dev)
    {
        dev->conn_param.interval_max = interval;
        dev->conn_param.interval_min = interval;
        dev->conn_param.latency = latency;
        dev->conn_param.timeout = timeout;
        dev->set_parm_flag = 0;
    }

	SYS_LOG_INF("int 0x%x lat %d to %d\n", interval, latency, timeout);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

static int _tr_bt_manager_gatt_write_without_response(struct bt_conn *conn, uint16_t handle, u8_t * buf, u16_t len)
{
    uint16_t mtu = hostif_bt_gatt_get_mtu(conn) - 3;
    if(len > mtu)
    {
        SYS_LOG_ERR("%d greater than %d\n", len, mtu);
		return -EINVAL;
    }
    return bt_gatt_write_without_response(conn, handle, buf, len, false);
}

void tr_bt_manager_ble_cli_init(void)
{
    sys_slist_init(&ble_cli_list);
    hostif_bt_conn_cb_register(&conn_callbacks);
}

void tr_bt_manager_ble_cli_deinit(void)
{
}

static int _tr_bt_manager_ble_cli_set_interval(struct ble_cli_mgr_info *dev, uint16_t interval)
{
    if(interval < 6)
        interval = 6;
    else if(interval > 3200)
        interval = 3200;

    if(dev->conn_param.interval_max == interval)
    {
        return -1;
    }

    dev->conn_param.interval_max = interval;
    dev->conn_param.interval_min = interval;
    return hostif_bt_conn_le_param_update(dev->ble_conn, &dev->conn_param);
}

int tr_bt_manager_ble_cli_set_interval(struct bt_conn *ble_conn, u16_t interval)
{
    int ret = 0;
    struct ble_cli_mgr_info *dev;

    u32_t cycle_stime_tmp = k_cycle_get_32();
    os_mutex_lock(&ble_cli_mgr_lock, OS_FOREVER);
    SYS_SLIST_FOR_EACH_CONTAINER(&ble_cli_list, dev, node)
    {
        if(bt_conn_get_handle(dev->ble_conn) && (!ble_conn || ble_conn == dev->ble_conn))
        {
            if(dev->conn_param.interval_max != interval || cycle_stime_tmp - dev->last_sync_conn_parma > 24000 * 2000)
            {
                ret = _tr_bt_manager_ble_cli_set_interval(dev, interval);
                dev->last_sync_conn_parma = cycle_stime_tmp;

                k_msleep(500);
            }
        }
    }
    os_mutex_unlock(&ble_cli_mgr_lock);
    return ret;
}
/* ******************************************************************
 *
 *
 *
 *
 * */

static uint8_t _send_pause;
void tr_bt_manager_ble_cli_send_pause(uint8_t pause)//流控
{
    _send_pause = pause;
}

int tr_bt_manager_ble_cli_send_data(struct bt_conn *conn, uint16_t handle, uint8_t *buf, int len, s32_t waiting)
{
	int ret = -1;
    uint8_t timeout = 0;

send_try:
    if(tr_le_cli_get_dev(conn) == NULL)
    {
        SYS_LOG_ERR("dev null\n");
        return -EIO;
    }

    if(_send_pause)
    {
        SYS_LOG_INF("waiting...\n");
        k_msleep(1);
        if(timeout++ > 500)
        {
            return -EIO;
        }
        goto send_try;
    }

    ret = _tr_bt_manager_gatt_write_without_response(conn, handle, buf, len);

    if(ret < 0)
    {
        if(waiting == 0)
        {
            return ret;
        }
        else if(timeout++ < (waiting / 10))
        {
            k_msleep(10);
            goto send_try;
        }
        return ret;
    }
	return len;
}

int tr_bt_manager_ble_cli_send(uint16_t handle, u8_t * buf, u16_t len, s32_t timeout)
{
    struct ble_cli_mgr_info *dev;

    if(!handle)
    {
        printk("handle null\n");
        return -EIO;
    }

    dev = tr_le_cli_get_dev_by_handle(handle);
    if(dev == NULL)
    {
        printk("dev null\n");
        return -EIO;
    }

    if(dev->write_handle == 0)
    {
        return -EIO;
    }

    u32_t cycle_stime_tmp = k_cycle_get_32();
    if(tr_bt_manager_le_audio_get_cis_num() && (!bt_le_conn_ready_send_data(dev->ble_conn) || cycle_stime_tmp - dev->last_write_tsp < 24000 * 10))
    {
        return -EBUSY;
    }

    int ret = tr_bt_manager_ble_cli_send_data(dev->ble_conn, dev->write_handle, buf, len, timeout);
    if(ret > 0)
    {
        dev->last_write_tsp = cycle_stime_tmp;
    }
    return ret;
}

extern int usb_hid_ep_write(const u8_t *data, u32_t data_len, u32_t timeout);
void update_finish_result(u8_t * buf, u16_t len)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    cmd_buf[0] = 0x55;
    cmd_buf[1] = buf[1];
    cmd_buf[2] = buf[2];
    
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}

int ota_backend_usbhid_read_wait_remote_func(bool flag)
{
    u8_t buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    buf[0] = 'U';
    buf[1] = 0x60;

    if(flag)
    {
        buf[2] = 0xff;
    }
    else
    {
        buf[2] = 0x00;
    }
    return usb_hid_ep_write(buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 5000);
}

void return_remote_dev_bat(u8_t param)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x65;
    cmd_buf[2] = param;
    
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}

void hid_set_peq_result(u8_t* buf)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    cmd_buf[0] = 0x55;
    cmd_buf[1] = buf[0];
    cmd_buf[2] = buf[1];
    cmd_buf[3] = buf[2];

    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}

void hid_get_peq_result(u8_t* buf)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    cmd_buf[0] = 0x55;
    memcpy(cmd_buf + 1, buf, 9);

    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}

extern u8_t *bt_name_le;
extern int dongle_send_data_to_pc(u16_t handle, u8_t *buf, u8_t len);
void earphone_app_ctrl(u8_t parm)
{
    struct app_msg msg = {0};
    msg.type = MSG_BT_EVENT;
    msg.cmd = BT_APP_CTRL;
    msg.value = parm;
    
    send_async_msg("tr_usound", &msg);
}

void tr_ble_cli_data_process(uint16_t handle, u8_t * buf, u16_t len)
{
    SYS_LOG_INF("buf[0]:%x, buf[1]:%x, buf[2]:%x, buf[3]:%x, buf[4]:%x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
    switch(buf[0])
    {
        case 0x55:
            update_finish_result(buf, len); //update finish result
            if(buf[1] == 1)
            {
                if(buf[2])
                    SYS_LOG_INF("remote update success\n");
                else
                    SYS_LOG_INF("remote update fail\n");

                extern int remote_upgrade_exit;
                k_sleep(K_MSEC(1000));
                extern void sys_pm_reboot(int type);
                sys_pm_reboot(0);
                remote_upgrade_exit = 1;
            } else {
                if(buf[2])
                    SYS_LOG_INF("remote update init success\n");
                else
                {
                    SYS_LOG_INF("remote update init fail\n");
                    extern void sys_pm_reboot(int type);
                    sys_pm_reboot(0);
                    extern int remote_upgrade_exit;
                    remote_upgrade_exit = 1;
                }
            }
            break;

        case 0x5b:
            if(bt_name_le)
            {
                memcpy(bt_name_le, buf + 1, 32);
            }
            extern struct k_sem btname_sem;  //init mod location
            k_sem_give(&btname_sem);
            break;
        case 0x60:
            if(buf[1])
                ota_backend_usbhid_read_wait_remote_func(true);
            else
                ota_backend_usbhid_read_wait_remote_func(false);
            break;
        case 0x65:
            return_remote_dev_bat(buf[1]);
            break;
        case 0x68:
            hid_set_peq_result(buf);
            break;
        case 0x69:
            hid_get_peq_result(buf);
            break;
        case 0x41:
            dongle_send_data_to_pc(handle, buf, len); 
            break;
        case 0x80:
            dongle_send_data_to_pc(handle, buf, len); 
            break;
        case 0x90:
            earphone_app_ctrl(buf[1]);
            break;
	case 0x88:

#ifdef CONFIG_XEAR_HID_PROTOCOL
	   if(buf[2] == HID_REPORT_ID_XEAR)
           {
           	xear_hid_protocol_cli_rcv_data_handler(&buf[2]);
           }
#endif
	   if(buf[2] == HID_REPORT_ID_INFO)
	   {
		hid_info_cli_rcv_data_handler(&buf[2], buf[1]);

           }	   
	    break;
        default:
            break;
    }
}

void tr_bt_manager_ble_cli_rev(uint16_t handle, u8_t * buf, u16_t len)
{
    //printk("%s: 0x%02x 0x%02x 0x%02x(%d)\n", __func__, buf[0],  buf[1], buf[2], len);
    //SYS_LOG_INF("buf[0]:%x, buf[1]:%x, buf[2]:%x, buf[3]:%x, buf[4]:%x", buf[0], buf[1], buf[2], buf[3], buf[4]);
    tr_ble_cli_data_process(handle, buf, len);
}
