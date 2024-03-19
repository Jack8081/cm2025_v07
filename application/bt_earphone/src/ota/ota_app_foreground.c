/*
 * Copyright (c) 2019 Actions Semiconductor Co, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <thread_timer.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <app_defines.h>
#include <bt_manager.h>
#include <app_manager.h>
#include <app_switch.h>
#include <app_config.h>
#include <srv_manager.h>
#include <sys_manager.h>
#include <sys_event.h>
#include <sys_wakelock.h>
#include <ota_upgrade.h>
#include <ota_backend.h>
#include <ota_backend_bt.h>
#include <config.h>
#include <drivers/nvram_config.h>
#include <user_comm/sys_comm.h>
#include "ota_app.h"
#include <ui_manager.h>
#include <drivers/flash.h>
#include <tts_manager.h>
#include <app_ui.h>
#include <system_app.h>
#include "ota_tws_transmit.h"
#include <user_comm/ap_status.h>
#include <anc_hal.h>
#ifdef CONFIG_BT_TRANSCEIVER
#include <ringbuff_stream.h>
#include <partition/partition.h>
#include <ota_backend_usbhid.h>
#include <usb/class/usb_hid.h>
#include <sys_wakelock.h>
#include <api_common.h>
#endif

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

#include <usb/usb_custom_handler.h>
#include <xear_hid_protocol.h>
#include <usb/usb_custom_hid_info.h>

#ifdef CONFIG_XEAR_LED_CTRL
#include <xear_led_ctrl.h>
#endif

#define CONFIG_XSPI_NOR_ACTS_DEV_NAME "spi_flash"
#define OTA_STORAGE_DEVICE_NAME		CONFIG_XSPI_NOR_ACTS_DEV_NAME
#define CONFIG_OTA_STACK_SIZE (2048)

typedef struct {
	char *stack;
	u8_t type;
} act_ota_data_t;

act_ota_data_t *ota_thread_data;
//char __aligned(ARCH_STACK_PTR_ALIGN) ota_thread_stack[CONFIG_OTA_STACK_SIZE];
static struct ota_upgrade_info *g_ota;
#ifdef CONFIG_OTA_BACKEND_BLUETOOTH
static struct ota_backend *backend_bt;
#endif

#ifdef CONFIG_BT_TRANSCEIVER

#ifndef CONFIG_HID_INTERRUPT_OUT_EP_MPS
#define CONFIG_HID_INTERRUPT_OUT_EP_MPS 64
#endif

u8_t hidble_loopback_mode;
os_delayed_work usbhid_loopback_delay_work;
u8_t *loopback_buf;
u8_t loopback_wr_error;
#define HID_BT_CMD_BUFFER 64
#ifdef CONFIG_OTA_BACKEND_USBHID
static struct ota_backend *backend_usbhid_remote;
#endif
#endif

io_stream_t sppble_stream_handle = NULL;
io_stream_t slave_stream_handle = NULL;

//int print_hex(const char* prefix, const void* data, size_t size);
extern io_stream_t sppble_stream_tws_create(void *param);
extern io_stream_t slave_ota_stream_tws_create(void *param);
extern int tr_bt_manager_ble_cli_send(uint16_t handle, u8_t * buf, u16_t len ,s32_t timeout);


#ifdef CONFIG_BT_TRANSCEIVER
#ifdef CONFIG_SPPBLE_DATA_CHAN
int tr_ble_srv_send_data_other(u8_t *buf, u8_t packet_idx, u8_t packet_len)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

    packet_len  = (packet_len > (CONFIG_HID_INTERRUPT_OUT_EP_MPS - 4)) ? \
                  (CONFIG_HID_INTERRUPT_OUT_EP_MPS - 4) : packet_len;
    memset(cmd_buf, 0, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
    if(buf[0] == 0x41)
        memcpy(cmd_buf, buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
    else
    {
        cmd_buf[0] = 0x80;
        cmd_buf[2] = packet_idx;
        cmd_buf[3] = packet_len;
        memcpy(cmd_buf + 4, buf + 4, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
    }

    return tr_ota_write_data(backend_bt, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
}
#endif
#endif

int tr_ble_srv_send_xear_data_other(u8_t *buf, u8_t packet_idx, u8_t packet_len)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

    packet_len  = (packet_len > (CONFIG_HID_INTERRUPT_OUT_EP_MPS - 2)) ? \
                  (CONFIG_HID_INTERRUPT_OUT_EP_MPS - 2) : packet_len;
    memset(cmd_buf, 0, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
    cmd_buf[0]  = 0x88;
    cmd_buf[1] = packet_len;
    memcpy(cmd_buf + 2, buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
    return tr_ota_write_data(backend_bt, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
}



//TX to RX
//Send Cmedia define package
int tr_ble_cli_send_data_other(u16_t handle, u8_t *buf, u16_t data_len)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};
    int ret;
    data_len  = (data_len > (CONFIG_HID_INTERRUPT_OUT_EP_MPS  - 2))? (CONFIG_HID_INTERRUPT_OUT_EP_MPS  - 2) : data_len; 
    memset(cmd_buf, 0 , CONFIG_HID_INTERRUPT_OUT_EP_MPS);
    memcpy(cmd_buf+2, buf, data_len);
    cmd_buf[0] = 0x88; //Cmedia Header
    cmd_buf[1] = data_len & 0xff;

    SYS_LOG_INF("\n [0]=%X [1]=%X [2]=%X [3]=%X \n",cmd_buf[0],cmd_buf[1],cmd_buf[2],cmd_buf[3]);
    SYS_LOG_INF("\n [4]=%X [5]=%X [6]=%X [7]=%X \n",cmd_buf[4],cmd_buf[5],cmd_buf[6],cmd_buf[7]);
   ret = tr_bt_manager_ble_cli_send(handle, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 0);
   return ret;
}


static u8_t ota_type_get(void)
{
	if (!ota_thread_data)
		return PROT_OTA_PHONE_APP;

	return ota_thread_data->type;
}

static void ota_app_start(void)
{
	struct app_msg msg = {0};

	SYS_LOG_INF("ota app start");

	const struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, false);

	msg.type = MSG_START_APP;
	msg.ptr = APP_ID_OTA;
	msg.reserve = APP_SWITCH_CURR;
	send_async_msg(APP_ID_MAIN, &msg);
	sys_wake_lock(FULL_WAKE_LOCK);
}

static void ota_app_stop(void)
{
	struct app_msg msg = {0};

	SYS_LOG_INF("ok");

	const struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, true);

	msg.type = MSG_START_APP;
	msg.ptr = NULL;
	msg.reserve = APP_SWITCH_LAST;
	send_async_msg(APP_ID_MAIN, &msg);
	sys_wake_unlock(FULL_WAKE_LOCK);
}

extern void ota_app_backend_callback(struct ota_backend *backend, int cmd, int state)
{
	int err;

	SYS_LOG_INF("backend %p cmd %d state %d", backend, cmd, state);
	char* current_app = app_manager_get_current_app();

	if (current_app && !strcmp(APP_ID_BTCALL, current_app)) {
		SYS_LOG_WRN("btcall unsupport ota, skip...");
		return;
	}
	if (cmd == OTA_BACKEND_UPGRADE_STATE) {
		if (state == 1) {
			SYS_LOG_INF("bt_manager_allow_sco_connect false\n");
			bt_manager_allow_sco_connect(false);
			err = ota_upgrade_attach_backend(g_ota, backend);
			if (err) {
				SYS_LOG_INF("unable attach backend");
				return;
			}

			ota_app_start();
		} else {
			ota_upgrade_detach_backend(g_ota, backend);
			SYS_LOG_INF("bt_manager_allow_sco_connect true\n");
			bt_manager_allow_sco_connect(true);
		}
	} else if (cmd == OTA_BACKEND_UPGRADE_PROGRESS) {
		//ota_view_show_upgrade_progress(state);
	}
}

#ifdef CONFIG_OTA_BACKEND_BLUETOOTH
/* UUID order using BLE format */
/*static const u8_t ota_spp_uuid[16] = {0x00,0x00,0x66,0x66, \
	0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB};*/

/* "00001101-0000-1000-8000-00805F9B34FB"  ota uuid spp */
static const u8_t ota_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, \
	0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x03, 0xe0, 0x00, 0x00};

#ifdef CONFIG_BT_TRANSCEIVER
extern struct bt_gatt_attr ota_gatt_attr[];
int get_ota_gatt_attr_size(void);
#else
/* BLE */
/*	"e49a25f8-f69a-11e8-8eb2-f2801f1b9fd1" reverse order  */
#define OTA_SERVICE_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xf8, 0x25, 0x9a, 0xe4)

/* "e49a25e0-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define OTA_CHA_RX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe0, 0x25, 0x9a, 0xe4)

/* "e49a28e1-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define OTA_CHA_TX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe1, 0x28, 0x9a, 0xe4)

// static struct bt_gatt_ccc_cfg g_ota_ccc_cfg[1];

static struct bt_gatt_attr ota_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(OTA_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(OTA_CHA_RX_UUID, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
						BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CHARACTERISTIC(OTA_CHA_TX_UUID, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
};

static const struct ota_backend_bt_init_param bt_init_param = {
	.spp_uuid = ota_spp_uuid,
	.gatt_attr = &ota_gatt_attr[0],
	.attr_size = ARRAY_SIZE(ota_gatt_attr),
	.tx_chrc_attr = &ota_gatt_attr[3],
	.tx_attr = &ota_gatt_attr[4],
	.tx_ccc_attr = &ota_gatt_attr[5],
	.rx_attr = &ota_gatt_attr[2],
	.read_timeout = OS_FOREVER,	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	.write_timeout = OS_FOREVER,
};
#endif
int ota_app_init_bluetooth(void)
{
	//print_buffer(bt_init_param.spp_uuid, 1, 16, 16, 0);

	if (sppble_stream_handle)
	{
		SYS_LOG_INF("sppble exit!\n");
		ota_backend_stream_set(sppble_stream_handle);
		return 0;
	}

	//backend_bt = ota_backend_bt_init(ota_app_backend_callback,
	//	(struct ota_backend_bt_init_param *)&bt_init_param);
#ifdef CONFIG_BT_TRANSCEIVER
    struct ota_backend_bt_init_param bt_init_param;
    bt_init_param.spp_uuid = ota_spp_uuid;
    bt_init_param.gatt_attr = &ota_gatt_attr[0];
    bt_init_param.attr_size = get_ota_gatt_attr_size();
    bt_init_param.tx_chrc_attr = &ota_gatt_attr[3];
    bt_init_param.tx_attr = &ota_gatt_attr[4];
    bt_init_param.tx_ccc_attr = &ota_gatt_attr[5];
    bt_init_param.rx_attr = &ota_gatt_attr[2];
    bt_init_param.read_timeout = OS_FOREVER;
    bt_init_param.write_timeout = OS_FOREVER;
#endif

	backend_bt = ota_backend_load_bt_init(ota_app_backend_callback,
			(struct ota_backend_bt_init_param *)&bt_init_param, sppble_stream_tws_create, &sppble_stream_handle);

	if (!backend_bt) {
		SYS_LOG_INF("failed");
		return -ENODEV;
	}
	
	// bt_manager_tws_register_long_message_cb(tws_long_data_cb);
	return 0;
}

int ota_app_init_slave_bluetooth(void)
{
	if (slave_stream_handle)
	{
		SYS_LOG_INF("slave exit!\n");
		ota_backend_stream_set(slave_stream_handle);
		return 0;
	}

#ifdef CONFIG_BT_TRANSCEIVER
    struct ota_backend_bt_init_param bt_init_param;
    bt_init_param.spp_uuid = ota_spp_uuid;
    bt_init_param.gatt_attr = &ota_gatt_attr[0];
    bt_init_param.attr_size = get_ota_gatt_attr_size();
    bt_init_param.tx_chrc_attr = &ota_gatt_attr[3];
    bt_init_param.tx_attr = &ota_gatt_attr[4];
    bt_init_param.tx_ccc_attr = &ota_gatt_attr[5];
    bt_init_param.rx_attr = &ota_gatt_attr[2];
    bt_init_param.read_timeout = OS_FOREVER;
    bt_init_param.write_timeout = OS_FOREVER;
#endif


	//backend_bt = ota_backend_bt_init(ota_app_backend_callback,
	//	(struct ota_backend_bt_init_param *)&bt_init_param);
	backend_bt = ota_backend_load_bt_init(ota_app_backend_callback,
			(struct ota_backend_bt_init_param *)&bt_init_param, slave_ota_stream_tws_create, &slave_stream_handle);

	if (!backend_bt) {
		SYS_LOG_INF("failed");
		return -ENODEV;
	}

	return 0;
}
#endif

#ifdef CONFIG_BT_TRANSCEIVER
#ifdef CONFIG_OTA_BACKEND_USBHID
struct ota_backend *ota_app_init_usbhid(void *mode)
{
    struct ota_backend_usbhid_init_param ota_back_param;
    ota_back_param.mode = mode;
    backend_usbhid_remote = ota_backend_usbhid_init(NULL, &ota_back_param);

    if(!backend_usbhid_remote)
        return NULL;

    return backend_usbhid_remote;
}
#endif
#endif

static void sys_reboot_by_ota(void)
{
	struct app_msg  msg = {0};
	msg.type = MSG_REBOOT;
	msg.cmd = REBOOT_REASON_OTA_FINISHED;
	send_async_msg("main", &msg);
}

void ota_app_notify(int state, int old_state)
{
	SYS_LOG_INF("ota state: %d->%d", old_state, state);
	CFG_Struct_OTA_Settings cfg;

    app_config_read
    (
        CFG_ID_OTA_SETTINGS,
        &cfg,0,sizeof(CFG_Struct_OTA_Settings)
    );

	if (state == OTA_DONE) {
		SYS_LOG_INF("Enable_APP_OTA_Erase_VRAM %d ", cfg.Enable_APP_OTA_Erase_VRAM);

		if ((cfg.Enable_APP_OTA_Erase_VRAM) && 
			(PROT_OTA_DONGLE_PC_CONDIF == ota_type_get()))
			nvram_config_clear_all();
		else if ((cfg.Enable_Dongle_OTA_Erase_VRAM) && 
			(PROT_OTA_FACTORY_OFFLINE != ota_type_get()))
			nvram_config_clear_all();

		SYS_LOG_INF("Enable_Poweroff %d ", cfg.Enable_Poweroff);
		if (cfg.Enable_Poweroff)
		{
			int upgrade_success = 1;		
			property_set("OTA_UPGRADE_FLAG", (char *)&upgrade_success, sizeof(upgrade_success));
			sys_event_send_message(MSG_POWER_OFF);
			os_sleep(1000);
			return;
		}
			
		os_sleep(1000);
		sys_reboot_by_ota();
	}
}

int ota_app_init(void)
{
	struct ota_upgrade_param param;
	CFG_Struct_OTA_Settings cfg;

	SYS_LOG_INF("device name %s ", OTA_STORAGE_DEVICE_NAME);

    app_config_read
    (
        CFG_ID_OTA_SETTINGS,
        &cfg,0,sizeof(CFG_Struct_OTA_Settings)
    );
	
	memset(&param, 0x0, sizeof(struct ota_upgrade_param));

	param.storage_name = OTA_STORAGE_DEVICE_NAME;
	param.notify = ota_app_notify;
	param.flag_use_recovery = 1;
	param.flag_erase_part_for_upg = 1;
	SYS_LOG_INF("Enable_Ver_Low %d ", cfg.Enable_Ver_Low);

	if (cfg.Enable_Ver_Low)
		param.no_version_control = 1;

	const struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set((const struct device *)flash_device, false);

	g_ota = ota_upgrade_init(&param);
	if (!g_ota) {
		SYS_LOG_INF("init failed");
		if (flash_device)
			flash_write_protection_set(flash_device, true);
		return -1;
	}

	if (flash_device)
		flash_write_protection_set(flash_device, true);
	return 0;
}

struct ota_upgrade_info *ota_app_info_get(void)
{
	return g_ota;
}

static void ota_app_start_ota_upgrade(void)
{
	struct app_msg msg = {0};

	msg.type = MSG_START_APP;
	send_async_msg(APP_ID_OTA, &msg);
}

#ifdef CONFIG_BT_TRANSCEIVER
#ifdef CONFIG_SPPBLE_DATA_CHAN
#define USB_HID_UPDATE_LEN 			512
#define USB_HID_UPDATE_MAX_LEN 		2048
#define USB_HID_UPDATE_CACHE_BUF	512
#define HID_SPEED_LIMIT_HIG			((22*1024)/1000)
#define HID_SPEED_LIMIT_DOWN		((10*1024)/1000)

#ifdef CONFIG_OTA_BACKEND_USBHID
typedef struct update_information{
	u8_t *buffer;   //read buff
    io_stream_t update_data_stream;
}update_info;
static update_info *up_info;

static void ota_upgrade_hid_exit(void)
{
    if(up_info->buffer)
    {
        mem_free(up_info->buffer);
        up_info->buffer = NULL;
    }

    stream_close(up_info->update_data_stream);

    if(up_info)
    {
        mem_free(up_info);
        up_info = NULL;
    }
}

static int ota_upgrade_hid_init(void)
{
	int res = 0;

	os_sleep(50);
#ifdef  CONFIG_BT_TRANS_BLEDATA_SERVER_FAREND
    if(!hid_trans_flag)
#endif

	up_info = mem_malloc(sizeof(update_info));
	if(!up_info)
	{
		printk("up_info alloc fail\n");
		res = -1;
		goto ErrExit_1;
	}

	up_info->buffer = mem_malloc(USB_HID_UPDATE_MAX_LEN);
    if(!up_info->buffer)
    {
        printk("buffer alloc fail\n");
        res = -2;
        goto ErrExit_2;
    }
    
    up_info->update_data_stream = ringbuff_stream_create_ext((void*)up_info->buffer, USB_HID_UPDATE_MAX_LEN);
    if(!up_info->update_data_stream)
    {
        printk("update_data_stream alloc fail\n");
        res = -2;
        goto ErrExit_3;
    }
    
    res = stream_open(up_info->update_data_stream, MODE_IN_OUT);
    if(res)
    {
        printk("res:%d, stream_open fail\n", res);
        goto ErrExit_3;
    }
    
    return 0;

ErrExit_3:
    mem_free(up_info->buffer);
    up_info->buffer = NULL;
ErrExit_2:
    mem_free(up_info);
    up_info = NULL;
ErrExit_1:

	return res;
}

int ota_backend_usbhid_read_wait_remote(bool flag)
{
    u8_t buf[HID_BT_CMD_BUFFER] = {0};
    buf[0] = 0x60;
    
    if(flag)
    {
        buf[1] = 0xff;
    }
    else
    {
        buf[1] = 0x00;
    }
    
    return tr_ota_write_data(backend_bt, buf, HID_BT_CMD_BUFFER);
}

static int count = 0;
int ota_upgrade_hid_remote_read(int offset, void *buf, int size)
{
	u8_t tmp[HID_BT_CMD_BUFFER] = {0};
	int res, num, read_exit = 0;
	num = (size % USB_HID_UPDATE_LEN) ? (size / USB_HID_UPDATE_LEN + 1) : (size / USB_HID_UPDATE_LEN);
	num *= USB_HID_UPDATE_LEN;

    ota_backend_usbhid_read_wait_remote(false);
    while(1)
    {
        if(stream_get_length(up_info->update_data_stream) >= num)
        {
            res = stream_read(up_info->update_data_stream, buf, num);
            if(res <= 0)
            {
                printk("stream_read update data fail, res:%d\n", res);
                return -1;
            }
            SYS_LOG_INF("ota upgrade read size:%d", res); 
            ota_backend_usbhid_read_wait_remote(true);
            return 0;
        }
        else
        {
            res = tr_ota_read_data(backend_bt, tmp, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
            if(res)
            {
                read_exit++;
                printk("usb_hid_ep_read update fail, res:%d\n", res);
                if(read_exit == 4)
                    return -1;
            }
            else
            {
                count++;
                SYS_LOG_INF("count_pack:%d", count);
                read_exit = 0;
            }
            res = stream_write(up_info->update_data_stream, tmp + 1, CONFIG_HID_INTERRUPT_OUT_EP_MPS - 1);
            if(res <= 0)
            {
                printk("stream_write update data fail, res:%d\n", res);
            }
        }
    }

	return 0;
}

static void hid_remote_update_ble_answer(u8_t type, u8_t event)
{
    int ret = 0;
    u8_t cmd_buf[HID_BT_CMD_BUFFER] = {0};

    cmd_buf[0] = 0x55;
    cmd_buf[1] = type;
    cmd_buf[2] = event;

    ret = tr_ota_write_data(backend_bt, cmd_buf, HID_BT_CMD_BUFFER);
}

#define  OTA_BP_STRUCT_SIZE 62
static int update_restore_ota_bp(void)
{
    int rlen = 0;
    const struct fw_version *cur_ver;

    u8_t bp[OTA_BP_STRUCT_SIZE] = {0};

    rlen = nvram_config_get("OTA_BP", bp, OTA_BP_STRUCT_SIZE);
    if (rlen != OTA_BP_STRUCT_SIZE) {
        SYS_LOG_INF("cannot found OTA_BP");
        return -1;
    }

    memset(bp, 0, OTA_BP_STRUCT_SIZE);
    bp[3] = 0x0;
    bp[1] = !partition_get_current_mirror_id();
    cur_ver = fw_version_get_current();
    *((u32_t *)bp+1) = cur_ver->version_code;
    rlen = nvram_config_set("OTA_BP", bp, OTA_BP_STRUCT_SIZE);
    if(rlen)
        return -1;

    return 0;
}

static void hid_ota_init(void)
{
    int ret = 0;
    u8_t mode = 1;
    u32_t update_time = 0;
    
    printk("remote enter update\n");
    //start to update
    //bt_manager_audio_stop(BT_TYPE_LE);
    struct ota_upgrade_param param;
    memset(&param, 0, sizeof(struct ota_upgrade_param));
    param.storage_name = OTA_STORAGE_DEVICE_NAME;
    param.notify = NULL;

    param.flag_use_recovery = 1;
    param.flag_erase_part_for_upg = 1;

    param.no_version_control = 1;

    update_restore_ota_bp();
    g_ota = ota_upgrade_init(&param);
    if(!g_ota){
        printk("remote upgrade init fail\n");
        hid_remote_update_ble_answer(2, 0);
        goto exit;
    }else{
        hid_remote_update_ble_answer(2, 1);
        printk("remote upgrade init suc\n");
        backend_usbhid_remote = ota_app_init_usbhid(&mode);
        ota_upgrade_attach_backend(g_ota, backend_usbhid_remote);
        extern void hci_log_flag_set(bool type);
        hci_log_flag_set(true);
    }
    ret = ota_upgrade_hid_init();
    if(ret<0)
    {
        printk("upgrade mem fail\n");
        hid_remote_update_ble_answer(1, 0);
        goto exit;
    }

	update_time = os_uptime_get_32();
    sys_wake_lock(FULL_WAKE_LOCK);
    ret = ota_upgrade_check(g_ota);
    if(ret == 0)
    {
        int res, j = 0;
        u8_t tmp[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

        while(1){
            ota_backend_usbhid_read_wait_remote(false);
            res = tr_ota_read_data(backend_bt, tmp, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
            ota_backend_usbhid_read_wait_remote(true);
            //printk("res:%d, j:%d\n", res, j++);
            j++;
            if(res == 0 && j == 81)
            {
                break;
            }
        }
        printk("hid update successful, Time spent:%d\n", os_uptime_get_32() - update_time);
        ota_backend_usbhid_read_wait_remote(false);
        hid_remote_update_ble_answer(1, 1);
        k_sleep(K_MSEC(1000));
        sys_pm_reboot(0); //reboot
    }
    else
    {
        printk("upgrade fail\n");
        ota_backend_usbhid_read_wait_remote(false);
        hid_remote_update_ble_answer(1, 0);
        k_sleep(K_MSEC(1000));
        sys_pm_reboot(0);
        goto exit;
    }

exit:
    sys_wake_unlock(FULL_WAKE_LOCK);
    ota_upgrade_hid_exit();
    //ota_upgrade_exit(g_ota);
    return;
}
#endif

/*
static void hid_ota_download(u8_t *buf)
{
    if(buf[1] != 0x41)
    {
        SYS_LOG_ERR("not support hid data id!\n");
        return;
    }
    k_sem_give(&hid_read_remote_sem);
}

static int ble_usbhid_send_ack(u8_t event, u8_t result)
{
    u8_t buf[4];
    buf[0] = 0x5d;
    buf[1] = 0x2;
    buf[2] = event;
    buf[3] = result;

    return tr_ble_srv_send_data_other(buf, 0, 4);
}
*/

static void get_ble_name(void)
{
    u8_t cmd_buf[64] = {0};

    cmd_buf[0] = 0x5b;
    
    nvram_config_get("BT_LE_NAME", cmd_buf + 1, 32);

    tr_ota_write_data(backend_bt, cmd_buf, 64);
}

#ifdef CONFIG_POWER
static void get_bat_level(void)
{
    u8_t cmd_buf[64] = {0};
    u8_t bat_cap = 0;
    extern int power_manager_get_battery_capacity(void);
    bat_cap = (u8_t)power_manager_get_battery_capacity();

    SYS_LOG_INF("get bat_level:%d\n", bat_cap);
    cmd_buf[0] = 0x65;
    cmd_buf[1] = bat_cap;

    tr_ota_write_data(backend_bt, cmd_buf, 64);
}
#endif

static int ble_usbhid_send_ack(u8_t event, u8_t result)
{
    u8_t buf[4];
    buf[0] = 0x5d;
    buf[1] = 0x2;
    buf[2] = event;
    buf[3] = result;

    return tr_ble_srv_send_data_other(buf, 0, 4);
}

static void usbhid_loopback_delay_handle(struct k_work *work)
{
    int ret = 0;
    if(loopback_wr_error){
        ret = ble_usbhid_send_ack(0x60, 0xff);
        if(ret<0){
            os_delayed_work_submit(&usbhid_loopback_delay_work, 5);
            return;
        }
        else{
            loopback_wr_error = 0;
            os_delayed_work_submit(&usbhid_loopback_delay_work, 50);
        }
    }else{
        ret = tr_ble_srv_send_data_other(loopback_buf, 0, 64);
        if(ret < 0){
            os_delayed_work_submit(&usbhid_loopback_delay_work, 200);
        }
        else{
            ble_usbhid_send_ack(0x60, 0x0);
        }
    }
}

void hidble_set_loopback_mode(u8_t mode)
{
    hidble_loopback_mode = mode;
}

u8_t hidble_get_loopback_mode(void)
{
    return hidble_loopback_mode;
}

static void hid_set_peq(u8_t *buf)
{
    u8_t point, Mask;
    point_info_t point_info_arr[10] = {0};
    point_info_t point_info;
    int ret = 1, i = 0, Q_flag = 0, point_typeflag = 0, point_enflag = 0;
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

    point = buf[2];
    if(point == 0xff)
    {
        if(buf[11] & 0x4)
            Q_flag = 1;

        if(buf[11] & 0x8)
            point_typeflag = 1;

        if(buf[11] & 0x10)
            point_enflag = 1;

        for(i = 0; i < 10; i++)
        {
            point_info_arr[i].eq_fc = (buf[12 + i * 4] << 8) | buf[13 + i * 4];
            point_info_arr[i].eq_gain = (s16_t)((buf[14 + i * 4] << 8) | buf[15 + i * 4]);
            if(Q_flag)
                point_info_arr[i].eq_qvalue = (buf[7] << 8) | buf[8];
            if(point_typeflag)
                point_info_arr[i].eq_point_type = buf[9];
            if(point_enflag)
                point_info_arr[i].eq_point_en = buf[10];
            SYS_LOG_INF("point:%d, fc:%d, gain:%d, qvalue:%d, ptype:%d, pen:%d", i, \
                    point_info_arr[i].eq_fc, point_info_arr[i].eq_gain, point_info_arr[i].eq_qvalue, \
                    point_info_arr[i].eq_point_type, point_info_arr[i].eq_point_en);
        }

        ret = API_APP_MOD_MULTI_PEQ(point_info_arr, Q_flag, point_typeflag, point_enflag);
        SYS_LOG_INF("API_APP_MOD_MULTI_PEQ ret: %d\n",ret);
    }
    else
    {
        point_info.eq_fc = (buf[3] << 8) | buf[4];
        point_info.eq_gain = (s16_t)((buf[5] << 8) | buf[6]);
        point_info.eq_qvalue = (buf[7] << 8) | buf[8];
        point_info.eq_point_type = buf[9];
        point_info.eq_point_en = buf[10];
        Mask = buf[11];

        SYS_LOG_INF("point:%d, fc:%d, gain:%d, qvalue:%d, point_type:%d, point_en:%d, Mask:%d", \
                point, point_info.eq_fc, point_info.eq_gain, point_info.eq_qvalue, \
                point_info.eq_point_type, point_info.eq_point_en, Mask);

        ret = API_APP_MOD_PEQ(point, &point_info, Mask);
        SYS_LOG_INF("API_APP_MOD_PEQ ret: %d\n",ret);
    }
    
    cmd_buf[0] = 0x68;
    switch(ret)
    {
        case 0:
            cmd_buf[1] = 1;
            break;
        case -1:
            cmd_buf[1] = -1;
            break;
        case -2:
            cmd_buf[1] = -3;
            break;
        default:
            cmd_buf[1] = -2;
            cmd_buf[2] = ret;
            break;
    }

    tr_ota_write_data(backend_bt, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
}

static void hid_get_peq(u8_t *buff)
{
    u8_t point;
    int ret = -1;
    point_info_t point_info = {0};
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

    point = buff[2];
    ret = API_APP_GET_PEQ(point, &point_info);
    printk("get peq point:%d, ret:%d\n",point,ret);

    cmd_buf[0] = 0x69;
    if(!ret)
        cmd_buf[1] = 1;
    else
        cmd_buf[1] = ret;
    cmd_buf[2] = (point_info.eq_fc >> 8) & 0xff;
    cmd_buf[3] = point_info.eq_fc & 0xff;
    cmd_buf[4] = (point_info.eq_gain >> 8) & 0xff;
    cmd_buf[5] = point_info.eq_gain & 0xff;
    cmd_buf[6] = (point_info.eq_qvalue >> 8) & 0xff;
    cmd_buf[7] = point_info.eq_qvalue & 0xff;
    cmd_buf[8] = point_info.eq_point_type;

    tr_ota_write_data(backend_bt, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
}

/*
 *  cmd can trans to usound app by BT_APP_CTRL event
 *      0:enable cis mute data detect
 *      1:disable cis mute data detect
 *
 *
 *
 *
 * */
void app_ctrl(u8_t parm)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

    cmd_buf[0] = 0x90;
    cmd_buf[1] = parm;
    tr_ota_write_data(backend_bt, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
}

static void tr_sys_cmd_process(u8_t* buf, int len)
{
    switch(buf[1])
    {
        case 0x55:
#ifdef CONFIG_OTA_BACKEND_USBHID
            hid_ota_init();
#endif
			break;
        case 0x56:
            //hid_ota_download(buf);
            break;
        case 0x52:
            sys_pm_reboot(0); //reboot
            break;
        case 0x00:
            sys_pm_reboot(0x100);
            break;
        case 0x5b:
            get_ble_name();
            break;
        case 0x65:
    #ifdef CONFIG_POWER
            SYS_LOG_INF(" hid get bat");
            get_bat_level();
    #endif
            break;
        case 0x68:
            SYS_LOG_INF(" hid set PEQ");
            hid_set_peq(buf);
            break;
        case 0x69:
            SYS_LOG_INF(" hid get PEQ");
            hid_get_peq(buf);
            break;
        default:
            break;
    }
}

static void tr_ble_data_channel_rev_data_handle(uint8_t* buf, int len)
{
    int ret = 0;

    ret = tr_ble_srv_send_data_other(buf, 0, len);
    if(ret < 0){
        SYS_LOG_INF("ble send error");
        loopback_wr_error = 1;
        if(loopback_buf)
            memcpy(loopback_buf, buf, 64);
        os_delayed_work_submit(&usbhid_loopback_delay_work, 5);	
    }
}

static void tr_other_xear_cmd_process(u8_t* buf)
{
    SYS_LOG_INF("buf[0]:%x, buf[1]:%x, buf[2]:%x, buf[3]:%x, buf[4]:%x", buf[0], buf[1], buf[2], buf[3], buf[4]);

    switch(buf[2])
    {
#ifdef CONFIG_XEAR_HID_PROTOCOL
        case HID_REPORT_ID_XEAR:
			xear_hid_protocol_rcv_data_handler(&buf[2]);
			break;
#endif

#ifdef CONFIG_XEAR_LED_CTRL
        case HID_REPORT_ID_LED_CTRL:
			xear_led_ctrl_rcv_data_handler(&buf[2]);
			break;
#endif

        case HID_REPORT_ID_INFO:
                        hid_info_srv_rcv_data_handler(&buf[2], buf[1]);
                        break;
	default	:
	    break;


 
    }
}

static void tr_other_cmd_process(u8_t* buf)
{
    int len;
    SYS_LOG_INF("buf[0]:%x, buf[1]:%x, buf[2]:%x, buf[3]:%x, buf[4]:%x", buf[0], buf[1], buf[2], buf[3], buf[4]);
    switch(buf[1])
    {
        case 0x60:  //download
            len = buf[3];
            SYS_LOG_INF("near recv pc data len:%d\n", len);
            tr_ble_srv_send_data_other(buf, buf[2], buf[3]);
            // The user takes the data and processes it
            break;
        case 0x5d:  //set loopback mode
        	 if(buf[2] == 0){
                SYS_LOG_INF("close ble loopback\n");
                hidble_set_loopback_mode(0);
                os_delayed_work_cancel(&usbhid_loopback_delay_work);
                if(loopback_buf){
                    mem_free(loopback_buf);
                    loopback_buf = NULL;
#ifdef CONFIG_SOC_DVFS_DYNAMIC_LEVEL
                    soc_dvfs_unset_level(SOC_DVFS_LEVEL_BR_FULL_PERFORMANCE, __func__);
#endif
                }
                loopback_wr_error = 0;
            }
            else if(buf[2] == 1){
                SYS_LOG_INF("open ble loopback\n");
                if(hidble_get_loopback_mode() == 2)
                    return;

                hidble_set_loopback_mode(2);
                if(!loopback_buf){
                    loopback_buf = mem_malloc(64);
                    if(!loopback_buf){
                        printk("loopback buf malloc fail\n");
                    }
#ifdef CONFIG_SOC_DVFS_DYNAMIC_LEVEL
                    soc_dvfs_set_level(SOC_DVFS_LEVEL_BR_FULL_PERFORMANCE, __func__);
#endif
                }
                loopback_wr_error = 0;
                os_delayed_work_init(&usbhid_loopback_delay_work, usbhid_loopback_delay_handle);
			}
            break;

        default :
            break;
    }
}

static void tr_ota_read_process(u8_t* buf, u8_t flag, int len)
{
    SYS_LOG_INF("buf[0]:%x, buf[1]:%x, buf[2]:%x, buf[3]:%x, buf[4]:%x", buf[0], buf[1], buf[2], buf[3], buf[4]);
    switch(buf[0])
    {
        case 0x55:
            tr_sys_cmd_process(buf, len);
            break;
        case 0x41:
            if(hidble_get_loopback_mode() == 2)
            {
                tr_ble_data_channel_rev_data_handle(buf, len);
            }
            break;
        case 0x56:
            tr_other_cmd_process(buf);
            break;
            //Reserve the mobile phone upgrade interface
	case 0x88:
	    tr_other_xear_cmd_process(buf);
	    break;
        /*
        case 0x57:
            break;
        */

        default:
            break;
    }
}
#endif
#endif

#ifdef CONFIG_BT_TRANSCEIVER
u8_t data_chan_phone_value = 0;
void set_data_chan_phone_value(u8_t pram)
{
    data_chan_phone_value = pram;
}
        
u8_t get_data_chan_phone_value(void)
{
    return data_chan_phone_value;
}
#endif

static void ota_process_deal(void *p1, void *p2, void *p3)
{
	SYS_LOG_INF("Enter");

#ifdef CONFIG_BT_TRANSCEIVER
#ifdef CONFIG_SPPBLE_DATA_CHAN
    //* == 0xf5 is dongle connect; != 0xf5 is phone ota upgrade
    if(get_data_chan_phone_value() == 0xf5)
    {
        uint8_t cmd_buf[HID_BT_CMD_BUFFER] = {0};
        while(1)
        {
            if(g_ota == NULL)
            {
                k_sleep(K_MSEC(100));
                continue;
            }
            if(tr_ota_read_data(backend_bt, cmd_buf, HID_BT_CMD_BUFFER))
            {
                k_sleep(K_MSEC(100));
                continue;
            }
            else
            {
                tr_ota_read_process(cmd_buf, 0, HID_BT_CMD_BUFFER);
            }
        }
    }
#endif
#endif

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ota");
#endif

#ifdef CONFIG_ANC_HAL
    anc_dsp_close();
#endif

 	if (ota_upgrade_check(g_ota)) {
		SYS_LOG_INF("ota fail.");
	} else {
		SYS_LOG_INF("ota suss.");
	}
	tws_ota_resource_release();
	//os_thread_prepare_terminal(g_ota_data.tid);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ota");
#endif

	os_thread_priority_set(k_current_get(), -1);
	app_switch_force_unlock(1);
	ota_app_stop();

	SYS_LOG_INF("Exit");
}

int ota_init(void)
{
	if (ota_thread_data)
	{
		SYS_LOG_ERR("OTA ERR!");
		return -EALREADY;
	}

	ota_thread_data = app_mem_malloc(sizeof(act_ota_data_t));
	if (!ota_thread_data){
		return -ENOMEM;		
	}
	ota_thread_data->stack = app_mem_malloc(CONFIG_OTA_STACK_SIZE);
	if (!ota_thread_data->stack) {
		SYS_LOG_ERR("ota thread stack mem_malloc failed");
		app_mem_free(ota_thread_data);
		ota_thread_data = NULL;
		return -ENOMEM;
	}
	ota_thread_data->type = PROT_OTA_PHONE_APP;

	os_thread_create(ota_thread_data->stack, CONFIG_OTA_STACK_SIZE,
		ota_process_deal, NULL, NULL, NULL, CONFIG_APP_PRIORITY+1, 0, 0);

	SYS_LOG_INF("ota init");
	return 0;
}

void ota_deinit(void)
{
	if (!ota_thread_data)
		return;

	SYS_LOG_WRN("will disconnect ota, then exit");

	if (ota_thread_data->stack)
		app_mem_free(ota_thread_data->stack);
	app_mem_free(ota_thread_data);
	ota_thread_data = NULL;

	SYS_LOG_INF("ota exit now");
}

void btota_bt_event_proc(struct app_msg *msg)
{
	SYS_LOG_INF("cmd 0x%x",msg->cmd);

	switch (msg->cmd) {

		case BT_TWS_DISCONNECTION_EVENT:
		{
			if (AP_STATUS_OTA_RUNING == ap_status_get())
				ap_status_set(AP_STATUS_ERROR);

			tws_ota_resource_release();
			break;
		}

		default:
		{
			SYS_LOG_INF("invalid msg->cmd 0x%x.",msg->cmd);
			break;
		}
	}
}

static int ota_type_process_allow(int type)
{
	CFG_Struct_OTA_Settings cfg;

    app_config_read
    (
        CFG_ID_OTA_SETTINGS,
        &cfg,0,sizeof(CFG_Struct_OTA_Settings)
    );

	if ((0 == cfg.Enable_Single_OTA_Without_TWS) &&
		(PROT_OTA_PHONE_APP == type) && 
		(BTSRV_TWS_MASTER > bt_manager_tws_get_dev_role()))
	{
		SYS_LOG_ERR("please tws pair.");
		return -1;
	}

/*
	if (PROT_OTA_PHONE_APP != type)
		ota_update_sdfs_set(g_ota, TRUE);
	else
		ota_update_sdfs_set(g_ota, FALSE);
*/
	if (ota_thread_data)
		ota_thread_data->type = type;

	return 0;
}

static void ota_app_main(void *p1, void *p2, void *p3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;

	SYS_LOG_INF("enter");

	app_switch_force_lock(1);
	input_manager_lock();
	ble_adv_limit_set(SYS_BLE_LIMITED_OTA);
	ota_backend_ota_type_cb_set(ota_type_process_allow);
	ota_app_start_ota_upgrade();

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			int result = 0;

			switch (msg.type) {
			case MSG_START_APP:
				ota_init();
				break;

			case MSG_EXIT_APP:
				SYS_LOG_INF("ota application exit!");
				terminaltion = true;
				app_manager_thread_exit(APP_ID_OTA);
				break;

 			case MSG_BT_EVENT:
				btota_bt_event_proc(&msg);
				break;

			default:
				SYS_LOG_ERR("unknown: 0x%x!", msg.type);
				break;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}

		if (!terminaltion) {
		    thread_timer_handle_expired();
		}
	}
	
	ota_deinit();
	ota_backend_ota_type_cb_set(NULL);
	ble_adv_limit_clear(SYS_BLE_LIMITED_OTA);
	input_manager_unlock();
}

APP_DEFINE(ota, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   FOREGROUND_APP, NULL, NULL, NULL,
	   ota_app_main, NULL);

