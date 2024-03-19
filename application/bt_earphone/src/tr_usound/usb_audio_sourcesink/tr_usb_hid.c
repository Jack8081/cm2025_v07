/***************************************************************************
 * Copyright (c) 2018 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * Description:    usb hid hal
 *
 * Change log:
 ****************************************************************************
 */
#include <usb/class/usb_hid.h>
//#include "ota/ota_upgrade.h"
#include "bt_manager.h"
#include <stdio.h>
#include <mem_manager.h>
#include <os_common_api.h>
#include <ota_upgrade.h>
#include <soc_pm.h>
#include "fw_version.h"
#include <partition/partition.h>
#include <drivers/nvram_config.h>
#include <ringbuff_stream.h>
#include <ctype.h>
#include "../tr_usound.h"
#include <api_common.h>

#include <usb/usb_custom_handler.h>
#include <xear_led_ctrl.h>

u8_t *asp_temp_data = NULL;
int detect_time;
//u8_t bIdle = 0;

#define     CHANNEL_MODE_AT             0
#define     CHANNEL_MODE_SPP_SHARE      1
#define     CHANNEL_MODE_SPP_EXC        2
#define     CHANNEL_MODE_BLE            3
#define     CHANNEL_MODE_UART           4
#define     HID_TIME_OUT      50   //hid 写超时时间

#ifndef CONFIG_TR_USOUND_APP
#define usound_send_key_msg(a) do{}while(0)
#define usound_get_paired_list(a) do{}while(0)
#endif

#ifdef CONFIG_HID_INTERRUPT_OUT
char *hid_thread_stack;
static struct k_sem hid_read_sem;
static int hid_thread_exit = 0;
extern int tr_bt_manager_ble_cli_send(uint16_t handle, u8_t * buf, u16_t len ,s32_t timeout);
struct ota_backend  *backend_usbhid;
extern struct ota_backend *ota_app_init_usbhid(void *mode);
extern void ota_upgrade_exit(struct ota_upgrade_info *ota);
#ifdef CONFIG_OTA_BACKEND_USBHID
static struct ota_upgrade_info *g_ota;
#endif

#ifdef CONFIG_OTA_RECOVERY
#define OTA_STORAGE_DEVICE_NAME     CONFIG_OTA_TEMP_PART_DEV_NAME
#else
//#define OTA_STORAGE_DEVICE_NAME     CONFIG_XSPI_NOR_ACTS_DEV_NAME
#define OTA_STORAGE_DEVICE_NAME     "spi_flash"
#endif

#ifdef CONFIG_SPPBLE_DATA_CHAN
extern int cli_send_ble_data_to_pc(u8_t index, u8_t *buf, u16_t len);
typedef int (*ble_send_data_t)(u8_t index, u8_t *buf, u16_t len);
extern void usb_hid_register_recv_ble_data_cb(ble_send_data_t cb);
u8_t* hid_download_cache_buf = NULL;
os_delayed_work hid_loopback_delay_work;
#endif
int usb_hid_ep_write(const u8_t *data, u32_t data_len, u32_t timeout);
u8_t hid_download_index = 0;
u8_t hid_loopback_mode = 0;

u8_t hid_telephone_cmd_out = 0;
struct k_sem btname_sem;  //init mod location
u8_t *bt_name_le;
u8_t hid0_channel_mode = 3;
u8_t hid1_channel_mode = 3;
u8_t remote_upgrade_exit = 0;

void hid_sem_init(void)
{
	k_sem_init(&hid_read_sem, 0, 1);
    hid_thread_stack = mem_malloc(1536);
    if(!hid_thread_stack)
    {
        SYS_LOG_ERR("hid thread malloc fail");
    }
}

void hid_set_loopback_mode(u8_t mode)
{
    hid_loopback_mode = mode;
}

u8_t hid_get_loopback_mode(void)
{
    return hid_loopback_mode;
}

void hid_set_loopback_index(u8_t index)
{
    hid_download_index = index;
}

u8_t hid_get_loopback_index(void)
{
    return hid_download_index;
}

int dongle_send_data_to_pc(u16_t handle, u8_t *buf, u8_t len)
{
    struct conn_status status;
    u16_t handle0, handle1;
	u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    //loopback send
    if((hid_get_loopback_mode() > 0) && (buf[0] == 0x41)){
        u8_t tmp[64] = {0};
        memcpy(tmp, buf, 64);
        return usb_hid_ep_write(tmp, 64, 5000);
    }

    if(handle == 0)
    {
        cmd_buf[1] = 0x67;
        goto exit;
    }

    API_APP_CONNECTION_CONFIG(0, &status);
    handle0 = status.handle;
    API_APP_CONNECTION_CONFIG(1, &status);
    handle1 = status.handle;

    if(handle == handle0)
        cmd_buf[1] = 0x62;
    else if(handle == handle1)
        cmd_buf[1] = 0x66;
    else
        return -1;
    
    //SYS_LOG_INF ("cmd_buf[1]:%x", cmd_buf[1]);
exit:
    cmd_buf[0] = 0x55;
    memcpy(cmd_buf + 2, buf + 2, len);
    return usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
}

#if 1
void hid_read_complete(void)
{
	k_sem_give(&hid_read_sem);
}
#endif
#endif

int usb_hid_ep_write(const u8_t *data, u32_t data_len, u32_t timeout)
{
	u32_t bytes_ret, len;
 	int ret;

	while (data_len > 0) {

		len = data_len > CONFIG_HID_INTERRUPT_IN_EP_MPS ? CONFIG_HID_INTERRUPT_IN_EP_MPS : data_len;

        u32_t start_time;
        start_time = k_cycle_get_32();
        do {
            if (k_cycle_get_32() - start_time > HID_TIME_OUT * 24000)
            {
                ret = -ETIME;
                break;
            }

		    ret = hid_int_ep_write(data, len, &bytes_ret);
        } while (ret == -EAGAIN);
		
		if (ret) {
			usb_hid_in_ep_flush();
			SYS_LOG_ERR("err ret: %d", ret);
			return ret;
		}

		if (len != bytes_ret) {
			SYS_LOG_ERR("err len: %d, wrote: %d", len, bytes_ret);
			return -EIO;
		}

		data_len -= len;
		data += len;
        if(data_len > 0)
            k_sleep(K_MSEC(100));

	}

	return 0;
}

#ifdef CONFIG_HID_INTERRUPT_OUT
int usb_hid_ep_read(u8_t *data, u32_t data_len, u32_t timeout)
{
	u32_t bytes_ret = 0;
    u32_t len = 0;
	u8_t *buf;
	int ret;
    
	buf = data;

    if(hid_thread_exit == 1)
		return -EIO;

    if (k_sem_take(&hid_read_sem, K_MSEC(timeout))) {
		usb_hid_out_ep_flush();
		SYS_LOG_ERR("timeout");
		return -ETIME;
	}

    len = data_len > CONFIG_HID_INTERRUPT_OUT_EP_MPS ? CONFIG_HID_INTERRUPT_OUT_EP_MPS : data_len;

	ret = hid_int_ep_read(buf, len, &bytes_ret);
	if (ret) {
		SYS_LOG_ERR("err ret: %d\n", ret);
		return -EIO;
	}

	if (len != bytes_ret) {
		if( buf[0] != HID_REPORT_ID_LED_CTRL )
		{
		SYS_LOG_ERR("err len: %d, read: %d\n", len, bytes_ret);
		return -EIO;
		}
	}

    return 0;
}
#endif

static void hid_cmd_enteradfu(u8_t param)
{
    printk("now reboot to adfu:%d\n", param);
#ifdef CONFIG_SPPBLE_DATA_CHAN
    struct conn_status status;
#endif
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};
    u8_t cmd_buf_hid[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

    cmd_buf_hid[0] = 0x55;
    cmd_buf_hid[1] = 0x0;
    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x0;

    if(param == 0)
    {
        sys_pm_reboot(0x100);
        return;
    }
    //The local end accesses the adfu
#ifdef CONFIG_SPPBLE_DATA_CHAN
    else if(param == 1)
    {
        //Send the message to the Bluetooth device. 1 Go to the adfu
        API_APP_CONNECTION_CONFIG(param - 1, &status);
        if(tr_bt_manager_ble_cli_send(status.handle, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 1000) < 0)
        {
	        usb_hid_ep_write(cmd_buf_hid, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
            return;
        }
    }
    else if(param == 2)
    {
        API_APP_CONNECTION_CONFIG(param - 1, &status);
        if(tr_bt_manager_ble_cli_send(status.handle, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 1000) < 0)
        {
	        usb_hid_ep_write(cmd_buf_hid, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
            return;
        }
    }
    else
    {
	    usb_hid_ep_write(cmd_buf_hid, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
        return;
    }
    cmd_buf_hid[2] = 1;
	usb_hid_ep_write(cmd_buf_hid, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
#endif
}

static void hid_cmd_hfp_test(u8_t param)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x58;

    if(param)
    {
        //input hfp test mode
    }
    else
    {
        //output hfp test mode
    }

    cmd_buf[2] = 1;
	usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
}

#define CFG_BT_TEST_MODE_HID            "BQB_TEST_MODE"
static int usb_hid_enter_bqb(void)
{
	uint8_t temp = 1;
	int ret = 0;

	ret = nvram_config_set(CFG_BT_TEST_MODE_HID, &temp, sizeof(uint8_t));

	if (ret < 0) {
		SYS_LOG_ERR("ENTER_BQB_MODE ret %d failed\n", ret);
	} else {
		sys_pm_reboot(REBOOT_TYPE_NORMAL);
	}

	return 0;
}

static void hid_cmd_enterqbq_mode(void)
{
    printk("now enter bqb mode\n");
    usb_hid_enter_bqb();
}

static void hid_cmd_reboot(u8_t param)
{
    printk("now restart mech:%d\n", param);
#ifdef CONFIG_SPPBLE_DATA_CHAN
    struct conn_status status;
#endif
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    u8_t cmd_buf_hid[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    cmd_buf_hid[0] = 0x55;
    cmd_buf_hid[1] = 0x52;
    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x52;
    if(param == 0)
    {
        sys_pm_reboot(0); //reboot
        return;
    }
#ifdef CONFIG_SPPBLE_DATA_CHAN
    else if(param == 1)
    {
        API_APP_CONNECTION_CONFIG(param - 1, &status);
        if(tr_bt_manager_ble_cli_send(status.handle, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 1000) < 0)
        {
	        usb_hid_ep_write(cmd_buf_hid, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
            return;
        }
    }
    else if(param == 2)
    {
        API_APP_CONNECTION_CONFIG(param - 1, &status);
        if(tr_bt_manager_ble_cli_send(status.handle, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 1000) < 0)
        {
	        usb_hid_ep_write(cmd_buf_hid, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
            return;
        }
    }
    else
    {
	    usb_hid_ep_write(cmd_buf_hid, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
        return;
    }
    cmd_buf_hid[2] = 1;
	usb_hid_ep_write(cmd_buf_hid, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
#endif
}

void hid_update_answer(int rsptype ,int rspcode)
{
    u8_t cmd_buf[64];
	memset(cmd_buf, 0, 64);
    cmd_buf[0] = 'U';
    cmd_buf[1] = rsptype;
    cmd_buf[2] = (rspcode == 0) ? 0:1;

    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 5000);
}

#ifdef CONFIG_OTA_BACKEND_USBHID
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
#endif

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

int ota_upgrade_hid_read(int offset, void *buf, int size)
{
	u8_t tmp[64] = {0};
	int res, num;
	num = (size % USB_HID_UPDATE_LEN) ? (size / USB_HID_UPDATE_LEN + 1) : (size / USB_HID_UPDATE_LEN);
	num *= USB_HID_UPDATE_LEN;
    u8_t read_exit = 0;

    while(1)
    {
        if(stream_get_length(up_info->update_data_stream) >= num)
        {
            res = stream_read(up_info->update_data_stream, buf, num);
            if(res <= 0)
            {
                printk("stream_read update data fail, res:%d\n", res);
            }
            return size;
        }
        else
        {
            res = usb_hid_ep_read(tmp, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 5000);
            if(res)
            {
                read_exit++;
                printk("usb_hid_ep_read update fail, res:%d\n", res);
                if(read_exit == 4)
                {
                    return -1;
                }
            }
            else
            {
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

static int ota_upgrade_hid_init(void)
{
	int res = 0;

	os_sleep(50);
#ifdef  CONFIG_BT_TRANS_BLEDATA_SERVER_FAREND
    if(!hid_trans_flag)
#endif
    {
	    //*((u32_t *)0xc0001008)&= ~0x27;//close bt
    }

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

int update_usbhid_read_wait_host(bool flag)
{
    u8_t buf[64] = {0};

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
#ifdef  CONFIG_BT_TRANS_BLEDATA_SERVER_FAREND
    if(hid_trans_flag)
    {
        return tr_bt_manager_ble_send_data(buf, 3);
    }
#endif

    return usb_hid_ep_write(buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 5000);
}

void hid_update_host(void)
{
    int ret = 0;
    char mode = 0;
    u32_t update_time = 0;

    printk("enter update\n");
    
    bt_manager_audio_stop(BT_TYPE_LE);
    struct ota_upgrade_param param;
    memset(&param, 0, sizeof(struct ota_upgrade_param));
	param.storage_name = OTA_STORAGE_DEVICE_NAME;
	param.notify = NULL;
	param.flag_use_recovery = 1;
	param.flag_erase_part_for_upg = 1;

    param.no_version_control = 1; 
    
    update_restore_ota_bp();
    
    g_ota = ota_upgrade_init(&param);
    if(!g_ota)
    {
        printk("upgrade init fail\n");
        hid_update_answer(2, 0);
        goto exit;
    }
    else
    {
        hid_update_answer(2, 1);
        printk("upgrade init suc\n");
        backend_usbhid = ota_app_init_usbhid(&mode);
        ota_upgrade_attach_backend(g_ota, backend_usbhid);
    }
    
    ret = ota_upgrade_hid_init();
    if(ret<0)
    {   
        printk("upgrade hid init fail\n");
        hid_update_answer(1, 0);
        goto exit;
    }
    
    update_time = os_uptime_get_32();
    ret = ota_upgrade_check(g_ota);
    if(ret == 0)
    {
        int res;
        u8_t tmp[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

        while(1){
            update_usbhid_read_wait_host(false);
            res = usb_hid_ep_read(tmp, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 5000);
            update_usbhid_read_wait_host(true);
            if(res == -ETIME )
            {
                break;
            }
        }
        printk("hid update successful, Time spent:%d\n", os_uptime_get_32() - update_time);
        update_usbhid_read_wait_host(false);
        hid_update_answer(1, 1);
        sys_pm_reboot(0);
    }
    else
    {
        printk("hid update fail\n");
        hid_update_answer(1, 0);
        sys_pm_reboot(0);
    }

    return;
exit:
#ifdef  CONFIG_BT_TRANS_BLEDATA_SERVER_FAREND
if(!hid_trans_flag)
#endif
    sys_pm_reboot(0); //reboot
    ota_upgrade_hid_exit();
    bt_manager_audio_start(BT_TYPE_LE);
}

extern u8_t tr_bt_manager_le_audio_get_cis_num(void);
extern int tr_bt_manager_ble_cli_set_interval(struct bt_conn *ble_conn, u16_t interval);

static u8_t is_updating_remote = 0;
uint8_t hid_is_updating_remote(void)
{
    return is_updating_remote;
}

static void hid_set_updating_remote(u8_t value)
{
    is_updating_remote = value;
}

#ifdef CONFIG_SPPBLE_DATA_CHAN

static void hid_update_remote_defluxing(void)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x60;
    cmd_buf[2] = 0;

    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 5000);
}

static int hid_update_remote(u8_t idx)
{
		//send remote dev update enter
    u8_t cnt = 0, idx_other;
    int print = 0;
    struct conn_status status;
    uint16_t handle = 0;
    int ret = 0, res = 0;
    extern struct tr_usound_app_t *tr_usound_get_app(void);
	
    struct tr_usound_app_t *tr_usound = tr_usound_get_app();
	struct bt_audio_chan *source_chan = &tr_usound->dev[0].chan[SPK_CHAN];
	struct bt_audio_chan *sink_chan = &tr_usound->dev[0].chan[MIC_CHAN];

    if(idx == 1)
    {
        idx_other = 2;
    }
    else if(idx == 2)
    {
        idx_other = 1;
    }
    else
    {
        idx_other = 0;
    }

    if(idx)
    {
        API_APP_CONNECTION_CONFIG(idx - 1, &status);
        if(status.connected)
        {
            handle = status.handle;
            if(handle == 0)
            {
                SYS_LOG_INF("no dev\n");
                //bt_manager_enable_high_speed(0);
                return 0;
            }

            if(status.audio_connected)
            {
                ret = 1;
                //bt_manager_audio_stream_disconnect_single(source_chan);
                //bt_manager_audio_stream_disconnect_single(sink_chan);
                tr_bt_manager_audio_stream_disable_single(source_chan);
                tr_bt_manager_audio_stream_disable_single(sink_chan);
                printk("bt audio stream stop suc\n");
            }
        }
    }

    if(idx_other)
    {
        API_APP_CONNECTION_CONFIG(idx_other - 1, &status);
        if(status.connected)
        {
            tr_bt_manager_audio_disconn(status.handle);
        }
    }
    
    //API_APP_SCAN_WORK(4);
    while(tr_bt_manager_le_audio_get_cis_num())
    {
        SYS_LOG_INF("wait cis disconnecting\n");
        k_sleep(K_MSEC(100));
    }

    if(ret)
    {
        SYS_LOG_INF("wait cis update interval\n");
        tr_bt_manager_ble_cli_set_interval(NULL, 8);
        k_sleep(K_MSEC(800));
    }

    u8_t buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};
    buf[0] = 0x55;
    buf[1] = 0x55;
    res = tr_bt_manager_ble_cli_send(handle, buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 1000);
    
    hid_set_updating_remote(1);
    //stop bt log RX: and TX:
    extern void hci_log_flag_set(bool type);
    hci_log_flag_set(true);

    while(!remote_upgrade_exit)
    {
		res = usb_hid_ep_read(buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 5000);
        if(res == 0)
        {
            if(buf[0] != 0x41)
                continue;

            /*
            while(1)
            {
                ret = tr_bt_manager_ble_cli_send(handle, buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 1000);
                if(ret >= 0)
                    break;
                k_sleep(K_MSEC(1));
            }
*/
            ret = tr_bt_manager_ble_cli_send(handle, buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 100);
            if (ret < 0){
                printk("remote update fail ret %d\n",ret);
                hid_update_remote_defluxing();
                hid_update_answer(1, 0);
                remote_upgrade_exit = 1;
            } else {
                SYS_LOG_INF("send remote ota update count:%d", print);
                if(print == 0) {
                    printk("is updating remote\n");
                }
                print++;
            }
            cnt = 0;
        }
        else
        {
            printk("hid read fail res %d\n",res);
            if(++cnt >= 2)
            {
                remote_upgrade_exit = 1;
                hid_update_remote_defluxing();
                hid_update_answer(1, 0);
            }
        }
    }
    
    sys_pm_reboot(0);
    hid_set_updating_remote(0);
    tr_bt_manager_audio_stream_enable_single(source_chan, BT_AUDIO_CONTEXT_UNSPECIFIED);
    tr_bt_manager_audio_stream_enable_single(sink_chan, BT_AUDIO_CONTEXT_UNSPECIFIED);

    hci_log_flag_set(false);
    //printk("res:%d, bytes:%d\n", res, bytes);
	return 0;
}
#endif
#endif

void API_APP_SETPAIRADDR(u8_t index, u8_t *param)
{
    CK_PRIO();
    u8_t pairaddr[6] = {0};
    
    SYS_LOG_INF("set pairaddr:%d\n", index);
	memcpy(pairaddr, param, 6);
    if(index == 0)
    {
	    nvram_config_set("BINDING_BT_ADDR_L", pairaddr, 6);
    }
    else if(index == 1)
    {
	    nvram_config_set("BINDING_BT_ADDR_R", pairaddr, 6);
    }
}

static void hid_cmd_setpairaddr(u8_t index, u8_t *param)
{
    u8_t addr_buf[20] = {0};

    memcpy(addr_buf, param, 6);

    API_APP_SETPAIRADDR(index, addr_buf);

    hid_update_answer('T', 1);
}

void API_APP_GETPAIRADDR(u8_t index, u8_t *param)
{
    CK_PRIO();
    SYS_LOG_INF("get pairaddr:%d\n", index);
    if(index == 0)
    {
	    nvram_config_get("BINDING_BT_ADDR_L", param, 6);
    }
    else if(index == 1)
    {
	    nvram_config_get("BINDING_BT_ADDR_R", param, 6);
    }
}

static void hid_cmd_getpairaddr(u8_t index){
    u8_t addr_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    u8_t ret = 0;

    API_APP_GETPAIRADDR(index, addr_buf + 2);
    
    addr_buf[0] = 0x55;
    addr_buf[1] = 0x53;

    if(ret < 0 || (addr_buf[2] + addr_buf[3] + addr_buf[4] + addr_buf[5] + addr_buf[6] + addr_buf[7] == 0))
    {
        SYS_LOG_ERR("get pairaddr fail\n");
        memset(addr_buf + 2, 0xff, 6);
        usb_hid_ep_write(addr_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
        return;
    }

    usb_hid_ep_write(addr_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}

static void hid_cmd_getpairlistaddr(u8_t param, u8_t bNeedName)
{
    paired_dev_info_t *paired_list = NULL;
    int idx = param;
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    memset(cmd_buf, 0xff, CONFIG_HID_INTERRUPT_IN_EP_MPS);
    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x50;
    
    if(idx == 0xff)
    {
        usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
        return;
    }

    memset(cmd_buf, 0, CONFIG_HID_INTERRUPT_IN_EP_MPS);
    paired_list = mem_malloc(sizeof(paired_dev_info_t));
    if(!paired_list)
    {
        SYS_LOG_ERR("malloc paired list fail\n");
        usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
        return;
    }
    
    memset(cmd_buf + 2, 0, CONFIG_HID_INTERRUPT_IN_EP_MPS - 2);
    memset(paired_list, 0, sizeof(paired_dev_info_t));
    //tr_btif_get_pairedlist(idx, paired_list);
    memcpy(cmd_buf + 2, paired_list, sizeof(paired_dev_info_t));
    if(!bNeedName)
    {
        //clear Remote device name
        memset(cmd_buf + 8, 0, CONFIG_HID_INTERRUPT_IN_EP_MPS - 8);
    }
    
    mem_free(paired_list);
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}

size_t hex_to_str(char *pszDest, char *pbSrc, int nLen)
{
    char ddl, ddh;
    for (int i = 0; i < nLen; i++) {
        ddh = 48 + ((unsigned char)pbSrc[i]) / 16;
        ddl = 48 + ((unsigned char)pbSrc[i]) % 16;

        if (ddh > 57) ddh = ddh + 39;
        if (ddl > 57) ddl = ddl + 39;

        pszDest[i * 2] = ddh;
        pszDest[i * 2 + 1] = ddl;
    }
    pszDest[nLen * 2] = '\0';
    return 2 * nLen;
}

void str_to_hex(char *pbDest, char *pszSrc, int nLen)
{
    char h1, h2;
    char s1, s2;
    for (int i = 0; i < nLen; i++) {
        h1 = pszSrc[2 * i];
        h2 = pszSrc[2 * i + 1];

        s1 = tolower(h1) - 0x30;
        if (s1 > 9) s1 -= 39;

        s2 = tolower(h2) - 0x30;
        if (s2 > 9) s2 -= 39;
        
        pbDest[i] = ((s1 << 4) | s2);
    }
}

static void hid_cmd_getbtaddr(void)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    u8_t tmp_buf[12] = {0};
    u8_t ret = 0;

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x41;

    ret = nvram_config_get_factory("BT_MAC", tmp_buf, 12);
    if(ret < 0)
    {
        SYS_LOG_ERR("read nvram BT_MAC fail, get bt local bt addr fail\n");
        return;
    }

    str_to_hex(cmd_buf + 2, tmp_buf, 6);
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}

static void hid_cmd_setlocalbtaddr(u8_t *param)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    u8_t ret = 0;
    u8_t tmp_buf[12] = {0};

    memset(cmd_buf, 0, CONFIG_HID_INTERRUPT_IN_EP_MPS);
    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x41;

    hex_to_str(tmp_buf, param, 12);
    ret = nvram_config_set_factory("BT_MAC", tmp_buf, 12);
    if(ret)
    {
        SYS_LOG_DBG("bt addr write fail\n");
        memset(cmd_buf + 2, 0xff, 6);
        usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
        return;
    }

    memcpy(cmd_buf + 2, param, 6);
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}


//extern int tr_btif_del_pairedlist(int index);
static void hid_cmd_delpairedlist(u8_t *param){
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    u8_t ret = 0;

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x43;

    if(param[0]==0x0 && param[1]== 0x0 && param[2]== 0x0 && param[3]== 0x0 && param[4] == 0x0)
    {
        if(param[5] == 0xff)
        {
            //ret = tr_btif_del_pairedlist(0xff);
            btif_br_clear_list(BTSRV_DEVICE_ALL);

            k_sleep(K_MSEC(100));
            //Need to add later
            if(ret)
                //delete paired list successful
                cmd_buf[2] = 1;
            else
                cmd_buf[2] = 0;
        }
        else
        {
            SYS_LOG_INF("Delete a device by index: index = 0x00-0xfe (Reserved function, not supported yet)\n");
            cmd_buf[2] = 0;
        }
    }
    else if(param[0] == 0xff)
    {
        //ret = tr_btif_del_pairedlist(0xff);
        btif_br_clear_list(BTSRV_DEVICE_ALL);

        k_sleep(K_MSEC(100));
    }
    else
        cmd_buf[2] = 0;

    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}

extern int tr_usound_mic_dev_num(void);
static void hid_cmd_bt_ck_connected(void)
{       
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x59;

    struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    cmd_buf[2] = tr_usound->dev_num;
    printk("connected dev num %d\n", cmd_buf[2]);

    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}   

static void hid_cmd_getremoteaddr(u8_t index)
{
    struct conn_status status;
	u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
		
    if(index == 0)
    {
        return;
    }

    /*
    u8_t con_num = tr_usound_mic_dev_num();

    if(con_num == 0)
        return;
    */
    cmd_buf[0] = 'U';
    cmd_buf[1] = 0x5a;

    API_APP_CONNECTION_CONFIG(index - 1, &status);
    if(status.connected && status.handle != 0)
    {
        cmd_buf[2] =  status.addr[0];
        cmd_buf[3] =  status.addr[1];
        cmd_buf[4] =  status.addr[2];
        cmd_buf[5] =  status.addr[3];
        cmd_buf[6] =  status.addr[4];
        cmd_buf[7] =  status.addr[5];
        usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
        return;
    }
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 500);
}

void hid_getremotename(u8_t index)
{
    struct conn_status status;
    uint16_t handle = 0;
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

    API_APP_CONNECTION_CONFIG(index - 1, &status);

    if(!status.connected)
    {
        return;
    }

    handle = status.handle;

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x5b;

#ifdef CONFIG_SPPBLE_DATA_CHAN
    tr_bt_manager_ble_cli_send(handle, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 1000);
#endif
}

static void hid_cmd_getremotename(u8_t index)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};
    if(index == 0)
    {
        return;
    }

    cmd_buf[0] = 'U';
    cmd_buf[1] = 0x5b;
    
    k_sem_init(&btname_sem, 0, 1);
    bt_name_le = (u8_t*)mem_malloc(50);

    hid_getremotename(index);

    if(k_sem_take(&btname_sem, K_MSEC(200))!=0)
    {
        printk("bt%d get name sem error\n",index);
        memset(cmd_buf + 2, 0, CONFIG_HID_INTERRUPT_OUT_EP_MPS - 2);
        usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
    }else{
        memcpy(cmd_buf + 2, bt_name_le, 32);
        printk("bt%d name :%s, len:%d\n", index, cmd_buf + 2, strlen(cmd_buf + 2));
        cmd_buf[strlen(cmd_buf + 2) + 2] = '\0';
        usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
    }
    mem_free(bt_name_le);
}

static void hid_cmd_get_version(u8_t index, u8_t type)
{
	const struct fw_version *cur_ver;
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
	cmd_buf[0] = 'U';
	cmd_buf[1] = 0x5e;
    u32_t version_code_temp = 0;

	if(index == 0x0 && type == 0x3){
		cur_ver = fw_version_get_current();
        version_code_temp = cur_ver->version_code;

		cmd_buf[2] = (version_code_temp & (0xff << 24)) >> 24;
		cmd_buf[3] = (version_code_temp & (0xff << 16)) >> 16;
		cmd_buf[4] = (version_code_temp & (0xff << 8)) >> 8;
		cmd_buf[5] = version_code_temp & 0xff;
	}
    
    SYS_LOG_INF("%x %x %x %x\n", cmd_buf[2], cmd_buf[3], cmd_buf[4], cmd_buf[5]);
	usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
}

static void hid_cmd_swbtchan(u8_t * buf)
{
    u8_t dev_idx, chan, cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    u8_t hid_channel_mode = 3;
    struct conn_status status;

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x5c;

    dev_idx = buf[0];
    chan = buf[1];

    if(dev_idx == 0)
    {
        return;
    }

    if(chan == 0)
    {
        hid_channel_mode = CHANNEL_MODE_AT;
        cmd_buf[2] = 0xff;
        goto errExit;
    }
    else if(chan == 1)
    {
        hid_channel_mode = CHANNEL_MODE_SPP_SHARE;
        cmd_buf[2] = 0xff;
        goto errExit;
    }
    else if(chan == 2)
    {
        hid_channel_mode = CHANNEL_MODE_SPP_EXC;
        cmd_buf[2] = 0xff;
        goto errExit;
    }
    else if(chan == 3)
    {
        API_APP_CONNECTION_CONFIG(dev_idx - 1, &status);
        if(!status.connected)
        {
            cmd_buf[2] = 0xff;
            goto errExit;
        }
        hid_channel_mode = CHANNEL_MODE_BLE;
        cmd_buf[2] = 3;
    }
    else if(chan == 5)
    {
        hid_channel_mode = CHANNEL_MODE_UART;
        cmd_buf[2] = 5;
    }
    else
    {
        return;
    }

    if(dev_idx == 1)
    {
        hid0_channel_mode = hid_channel_mode;
    }
    else if(dev_idx == 2)
    {
        hid1_channel_mode = hid_channel_mode;
    }

errExit:
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);

    return;
}

void hid_event_data_download(u8_t result, u8_t idx)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

	cmd_buf[0] = 'U';
	cmd_buf[1] = 0x61;
	cmd_buf[2] = result;
	cmd_buf[3] = idx;
	
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
}

void hid_cmd_data_download0(u8_t *buf)
{
    u8_t len = buf[3];
    if(len > 60)
    {
        hid_event_data_download(0x0, 0xff);
        return;
    }
    SYS_LOG_INF("dongle recv pc data:%d\n", buf[3]);
    hid_event_data_download(0x1, 0xff);
}

void hid_cmd_rsp_data_upload(u8_t result, u8_t idx)
{
	if(result == 1)
	{
		printk("pc suc recv :%d\n", idx);
	}
	else if(result == 0)
	{
		printk("pc recv fail:%d\n", idx);
	}
}

void hid_spp_ble_spd_control(u8_t value)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x60;
    cmd_buf[2] = value;

    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
}

static void hid_loopback_notify_func(uint16_t handle, u8_t type, u8_t bOpen)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    if(type == 1)	//at
    {
        return;
    }else if(type == 3) //ble
    {
        cmd_buf[0]= 0x56;
        cmd_buf[1]= 0x5d;
        if(bOpen == 1)
            cmd_buf[2]= 0x01;
        else if(!bOpen)
            cmd_buf[2] = 0x0;
        else
            cmd_buf[2] = bOpen;

#ifdef CONFIG_SPPBLE_DATA_CHAN
        tr_bt_manager_ble_cli_send(handle, cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 0);
#else
        ;
#endif

    }
    else
    {
        return;
    }
}

u8_t hid_lppoint = 0;
#define HID_DOWNLOAD_CACHE_BUF_MAX_LEN   256
#ifdef CONFIG_SPPBLE_DATA_CHAN
static void hid_loopback_delay_handle(struct k_work *work)
{
    struct conn_status status;
    uint16_t handle = 0;
    u8_t index = hid_get_loopback_index();
    int ret = 0;
    int ret1 = 0;

    API_APP_CONNECTION_CONFIG(index-1, &status);
    handle = status.handle;

    if(!status.connected)
        return;

    u8_t len = *(hid_download_cache_buf + hid_lppoint * CONFIG_HID_INTERRUPT_OUT_EP_MPS + 1);

    ret = tr_bt_manager_ble_cli_send(handle, hid_download_cache_buf + hid_lppoint \
            * CONFIG_HID_INTERRUPT_OUT_EP_MPS, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 0);
    if(ret < 0){
        os_delayed_work_submit(&hid_loopback_delay_work, 10);
    }else{
        memset(hid_download_cache_buf + CONFIG_HID_INTERRUPT_OUT_EP_MPS * hid_lppoint, \
                0, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
        for(ret = 0; ret < 3; ret++)
        {
            if(hid_download_cache_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS * ret] == 0x41){
                len = *(hid_download_cache_buf + ret * CONFIG_HID_INTERRUPT_OUT_EP_MPS + 1);
                ret1 = tr_bt_manager_ble_cli_send(handle, hid_download_cache_buf + \
                        CONFIG_HID_INTERRUPT_OUT_EP_MPS * ret, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 0);
                if(ret1 < 0)
                {
                    hid_lppoint = ret;
                    os_delayed_work_submit(&hid_loopback_delay_work, 10);
                    return;
                }
                memset(hid_download_cache_buf + CONFIG_HID_INTERRUPT_OUT_EP_MPS * ret,\
                    0, CONFIG_HID_INTERRUPT_OUT_EP_MPS);
            }
        }
        hid_lppoint = 0;
        hid_spp_ble_spd_control(0x0);
        mem_free(hid_download_cache_buf);
    }
}
#endif

static void hid_cmd_loopback(u8_t *param)
{
    u8_t dev_idx, mode;
    struct conn_status status;
    uint16_t handle = 0;
    u8_t hid_channel_mode = 0;
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
#if 0
    extern struct tr_usound_app_t *tr_usound_get_app(void);
	
    struct tr_usound_app_t *tr_usound = tr_usound_get_app();
	struct bt_audio_chan *source_chan = &tr_usound->dev[0].chan[SPK_CHAN];
	struct bt_audio_chan *sink_chan = &tr_usound->dev[0].chan[MIC_CHAN];
#endif
    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x5d;

    dev_idx = *param;

    if(dev_idx == 0)
    {
        return;
    }

    if(dev_idx == 1)
    {
        hid_channel_mode = hid0_channel_mode;
    }
    else if(dev_idx == 2)
    {
        hid_channel_mode = hid1_channel_mode;
    }

    API_APP_CONNECTION_CONFIG(dev_idx - 1, &status);
    handle = status.handle;

    mode = *(param+1);
    if(mode)
    {
#ifdef CONFIG_SOC_DVFS_DYNAMIC_LEVEL
        soc_dvfs_set_level(SOC_DVFS_LEVEL_BR_FULL_PERFORMANCE, __func__);
#endif
    }

    if(mode == 0)  //close loopback
    {
        SYS_LOG_INF("close loopback test");
        hid_set_loopback_mode(0);
        hid_set_loopback_index(0);
#ifdef CONFIG_SPPBLE_DATA_CHAN
        os_delayed_work_cancel(&hid_loopback_delay_work);
#endif
#ifdef CONFIG_SOC_DVFS_DYNAMIC_LEVEL
            soc_dvfs_unset_level(SOC_DVFS_LEVEL_BR_FULL_PERFORMANCE, __func__);
#endif
    }
    else if(mode == 1)	//dongle loopback
    {
        SYS_LOG_INF("open dongle loopback test");
        hid_set_loopback_mode(1);
        hid_set_loopback_index(0);
#ifdef CONFIG_SPPBLE_DATA_CHAN
        os_delayed_work_cancel(&hid_loopback_delay_work);
#endif
    }
#ifdef CONFIG_SPPBLE_DATA_CHAN
    else if(mode == 2)	//bt loopback
    {
        SYS_LOG_INF("open bt loopback test");
        hid_set_loopback_mode(2);
        hid_set_loopback_index(dev_idx);
        //notify bt open loopback
        if(hid_channel_mode == CHANNEL_MODE_BLE){

            if(!status.connected)
            {
                cmd_buf[2] = 0xff;
                goto errExit;
            }
            hid_loopback_notify_func(handle, 3, 1);
            os_delayed_work_init(&hid_loopback_delay_work, hid_loopback_delay_handle);
        }
        else if(hid_channel_mode == CHANNEL_MODE_UART){
            cmd_buf[2] = 0xff;
            goto errExit;
        }
    }
#endif
    cmd_buf[2] = mode;
    if(mode < 2){
        if(hid_channel_mode == CHANNEL_MODE_BLE)
            hid_loopback_notify_func(handle, 3, 0);
        else if(hid_channel_mode == CHANNEL_MODE_UART)
            hid_loopback_notify_func(0, 4, 0);
    }
#ifdef CONFIG_SPPBLE_DATA_CHAN
errExit:
#endif
    
#ifndef CONFIG_SPPBLE_DATA_CHAN
    if(mode == 2)
        cmd_buf[2] = 0xff;
#endif
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
}

#ifdef CONFIG_SPPBLE_DATA_CHAN
static void hid_channel_send_ble(u8_t* buffer)
{
    struct conn_status status;
    uint16_t handle = 0;
    u8_t index = hid_get_loopback_index();
    u8_t hid_channel_mode = 0;
    int res = 0;

    if(index == 0)
    {
        return;
    }

    API_APP_CONNECTION_CONFIG(index - 1, &status);
    handle = status.handle;
    if(!status.connected)
    {
        return;
    }

    if(index == 1)
    {
        hid_channel_mode = hid0_channel_mode;
    }
    else if(index == 2)
    {
        hid_channel_mode = hid1_channel_mode;
    }

    if(hid_channel_mode == CHANNEL_MODE_BLE)
    {
        u8_t len = 0;
        len = *(buffer + 1);
        if(len > CONFIG_HID_INTERRUPT_IN_EP_MPS - 2)
        {
            printk("hid ble cmd to long %d!!\n",len);
            return;
        }
        else{
            int ret = 0;
            u8_t temp_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
            memcpy(temp_buf, buffer, CONFIG_HID_INTERRUPT_IN_EP_MPS);
            //dongle send to ble
            ret = tr_bt_manager_ble_cli_send(handle, temp_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 0);
            //printk(" loopback send data to ble,ret:%d\n",ret);
            if(ret < 0)  //speed control
            {
                hid_download_cache_buf = mem_malloc(HID_DOWNLOAD_CACHE_BUF_MAX_LEN);
                if(!hid_download_cache_buf)
                {
                    SYS_LOG_ERR("malloc hid_download_cache_buf fail");
                    return;
                }
                hid_spp_ble_spd_control(0xff);
                memset(hid_download_cache_buf, 0, HID_DOWNLOAD_CACHE_BUF_MAX_LEN);
                memcpy(hid_download_cache_buf, temp_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS);

                for(ret = 0; ret < 3; ret++){
                    res = usb_hid_ep_read(hid_download_cache_buf + \
                            CONFIG_HID_INTERRUPT_IN_EP_MPS * (ret + 1), \
                            CONFIG_HID_INTERRUPT_IN_EP_MPS, 10);
                    if(res == -79)
                    {
                        printk("dongle read loopback data fail");
                        break;
                    }
                }
                os_delayed_work_submit(&hid_loopback_delay_work, 10);
            }
        }
    }
}
#endif

static void hid_enter_loopback(u8_t *buffer, u8_t len)
{
    u8_t mode = hid_get_loopback_mode();

    if(mode == 1) //dongle loopback
    {
        dongle_send_data_to_pc(0, buffer, buffer[1]);
    }
    else if(mode == 2) //bt loopback
    {
        //if(hid_channel_mode == CHANNEL_MODE_BLE)
        {
#ifdef CONFIG_SPPBLE_DATA_CHAN
            hid_channel_send_ble(buffer);
#endif
        }
    }

}

void hid_cmd_data_download1(u8_t *buf)
{
    u8_t rst_num, len, cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    struct conn_status status;
    int ret;

    rst_num = buf[2];
    len = buf[3];
   
    API_APP_CONNECTION_CONFIG(0, &status);
    if(!status.connected)
    {
        hid_event_data_download(0xfe, 0);
        return;
    }

    if(len > CONFIG_HID_INTERRUPT_IN_EP_MPS - 4)
    {
        hid_event_data_download(0x0, 0);
        return;
    }

    if(rst_num > 0)
    {
        if(len != 60)
        {
            hid_event_data_download(0x0, 0);
            return;
        }

        cmd_buf[0] = 0x56;
        cmd_buf[1] = 0x60;
        memcpy(cmd_buf + 2, buf + 2, len);
        ret = tr_bt_manager_ble_cli_send(status.handle, cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 100);
        return;
    }
    else if(rst_num == 0)
    {
        memcpy(cmd_buf + 2, buf + 2, len);
        cmd_buf[0] = 0x56;
        cmd_buf[1] = 0x60;
        ret = tr_bt_manager_ble_cli_send(status.handle, cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 100);
        if(ret <= 0)
            hid_event_data_download(0xfe, 0);
        else
            hid_event_data_download(0x1, 0);
    }
}

void hid_cmd_data_download2(u8_t *buf)
{
    u8_t rst_num, len, cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};
    struct conn_status status;
    int ret;

    rst_num = buf[2];
    len = buf[3];
   
    API_APP_CONNECTION_CONFIG(1, &status);
    if(!status.connected)
    {
        hid_event_data_download(0xfe, 1);
        return;
    }

    if(len > CONFIG_HID_INTERRUPT_IN_EP_MPS - 4)
    {
        hid_event_data_download(0x0, 1);
        return;
    }

    if(rst_num > 0)
    {
        if(len != 60)
        {
            hid_event_data_download(0x0, 1);
            return;
        }

        cmd_buf[0] = 0x56;
        cmd_buf[1] = 0x60;
        memcpy(cmd_buf + 2, buf + 2, len);
        ret = tr_bt_manager_ble_cli_send(status.handle, cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 100);
        return;
    }
    else if(rst_num == 0)
    {
        memcpy(cmd_buf + 2, buf + 2, len);
        cmd_buf[0] = 0x56;
        cmd_buf[1] = 0x60;
        ret = tr_bt_manager_ble_cli_send(status.handle, cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 100);
        if(ret <= 0)
            hid_event_data_download(0xfe, 1);
        else
            hid_event_data_download(0x1, 1);
    }
}
#ifdef CONFIG_POWER
extern int power_manager_get_battery_capacity(void);
void hid_cmd_get_battery_cap(u8_t index, u8_t mode)
{
    u8_t bat_cap = 0, cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    if(index == 0)
    {
        bat_cap = (u8_t)power_manager_get_battery_capacity();
        cmd_buf[0] = 'U';
        cmd_buf[1] =0x65;
        cmd_buf[2] = index;
        cmd_buf[3] = bat_cap;
        usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);    
        return;
    }

#ifdef CONFIG_SPPBLE_DATA_CHAN
    struct conn_status status;
    if(mode == 0)
    {
        if(index == 1)
        {
            cmd_buf[0] = 0x55;
            cmd_buf[1] = 0x65;
            API_APP_CONNECTION_CONFIG(index - 1, &status);
            if(tr_bt_manager_ble_cli_send(status.handle, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 1000) < 0)
            {
                return;
            }
        }
        else if(index == 2)
        {
            cmd_buf[0] = 0x55;
            cmd_buf[1] = 0x65;
            API_APP_CONNECTION_CONFIG(index - 1, &status);
            if(tr_bt_manager_ble_cli_send(status.handle, cmd_buf, CONFIG_HID_INTERRUPT_OUT_EP_MPS, 1000) < 0)
            {
                return;
            }
        }
    }
    else
    {
        return;
    }
#endif    
}
#endif    

void hid_cmd_set_peq(u8_t *buf)
{
    u8_t dev_idx, cmd_buf[CONFIG_HID_INTERRUPT_OUT_EP_MPS] = {0};

    struct conn_status status;
    uint16_t handle = 0;
    dev_idx = buf[2];

    if((dev_idx - 1)!=0)
    {
        cmd_buf[2] = 0x0;
        goto errExit;
    }

    API_APP_CONNECTION_CONFIG(0, &status);

    if(!status.connected)
    {
        cmd_buf[2] = 0x0;
        goto errExit;
    }

    handle = status.handle;

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x68;

    memcpy(cmd_buf + 2, buf + 3, 50);

#ifdef CONFIG_SPPBLE_DATA_CHAN
    int ret = tr_bt_manager_ble_cli_send(handle, cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 0);
    if(ret < 0)
    {
        cmd_buf[2] = 0x0;
        goto errExit;
    }
#endif
    return;

errExit:

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x68;
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
}

void hid_cmd_get_peq(u8_t *buf)
{
    u8_t dev_idx, point, cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

    dev_idx = buf[2];
    point = buf[3];

    struct conn_status status;
    uint16_t handle = 0;

    if((dev_idx - 1) != 0)
    {
        cmd_buf[2] = 0x0;
        goto errExit;
    }

    API_APP_CONNECTION_CONFIG(0, &status);

    if(!status.connected)
    {
        cmd_buf[2] = 0x0;
        goto errExit;
    }
    handle = status.handle;

    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x69;
    cmd_buf[2] = point; 

#ifdef CONFIG_SPPBLE_DATA_CHAN
    int ret = tr_bt_manager_ble_cli_send(handle, cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 0);
    if(ret < 0)
    {
        cmd_buf[2] = 0x0;
        goto errExit;
    }
#endif
    return;

errExit:
    cmd_buf[0] = 0x55;
    cmd_buf[1] = 0x69;
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
}

void hid_command_process_func(u8_t * buf, int flag, u8_t len)
{
#ifdef  CONFIG_BT_TRANS_BLEDATA_SERVER_FAREND
    hid_trans_flag=flag;
#endif
#if 1
    //SYS_LOG_INF("buf[0]:%x,buf[1]%x,buf[2]:%x, %x, %x, %x, %x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
    switch(buf[0])
    {
#ifdef CONFIG_XEAR_LED_CTRL
	case HID_REPORT_ID_LED_CTRL:
		xear_led_ctrl_cmd_handler2(buf, 16);
		break;
#endif
        case 'U':
        {
            switch(buf[1])
            {
                case 'U':
                    if(buf[2] == 0 || buf[2] == 0xff)
                    {
#ifdef CONFIG_OTA_BACKEND_USBHID
                        printk("enter update\n");
                        hid_update_host();
#else
                        printk("enter update fail\n");
                        hid_update_answer(2, 0);
#endif
                    }
                    else
                    {
#ifdef CONFIG_SOC_DVFS_DYNAMIC_LEVEL
                        soc_dvfs_set_level(SOC_DVFS_LEVEL_BR_FULL_PERFORMANCE, __func__);
#endif
#ifdef CONFIG_SPPBLE_DATA_CHAN
#ifdef CONFIG_OTA_BACKEND_USBHID
                        printk("enter remote update\n");
                        remote_upgrade_exit = 0;
                        hid_update_remote(buf[2]);
#endif
#endif
                    }
                    break;
                case 0x0:
					hid_cmd_enteradfu(buf[2]);
					break;
				case 0x58:
					hid_cmd_hfp_test(buf[2]);
					break;
				case 'Q':   //enter bqb mode
					hid_cmd_enterqbq_mode();
					break;
				case 'R':
					hid_cmd_reboot(buf[2]);
					break;
                case 'T':
					hid_cmd_setpairaddr(buf[8], buf + 2);
					break;
				case 'S':
					hid_cmd_getpairaddr(buf[2]);
					break;
                case 0x59:
                    hid_cmd_bt_ck_connected();
					break;
				case 0x5a:
					hid_cmd_getremoteaddr(buf[2]);
					break;
				case 0x5b:
					hid_cmd_getremotename(buf[2]);
					break;
				case 'A':	//get dongle bt addr
					hid_cmd_getbtaddr();
					break;
				case 'F':	//set paired dev add finish
					//hid_cmd_setpairaddrfinish();
					break;
				case 'P':
					hid_cmd_getpairlistaddr(buf[2], buf[3]);
					break;
				case 'B':
					hid_cmd_setlocalbtaddr(buf + 2);
					break;
				case 'C': 	//del local paired list
					hid_cmd_delpairedlist(buf+2);
					break;
				case 0x5e:
					hid_cmd_get_version(buf[2], buf[3]);
					break;
                case 0x5C:
					hid_cmd_swbtchan(buf+2);
					break;
#if 0
				case 0x5f:
					hid_cmd_get_bt_audio_chan(buf[2]);
					break;
#endif
				case 0x5d:
					hid_cmd_loopback(buf+2);
					break;
#ifdef CONFIG_SPPBLE_DATA_CHAN
				case 0x61:
					hid_cmd_data_download1(buf);
					break;
                case 0x66:
					hid_cmd_data_download2(buf);
					break;
#endif
                case 0x67:
					hid_cmd_data_download0(buf);
					break;
                case 0x62:
					hid_cmd_rsp_data_upload(buf[2], buf[3]);
					break;
				case 0x63:
                    //hid_cmd_btprofile_connect(buf+2);
					break;
                case 0x64:
                    //hid_cmd_btprofile_disconnect(buf+2);
					break;
                case 0x65:
            #ifdef CONFIG_POWER
					hid_cmd_get_battery_cap(buf[2], buf[3]);
            #endif
					break;
                case 0x6a:
					//hid_cmd_get_list_index(buf+2);
					break;
                case 0x68:
					hid_cmd_set_peq(buf);
					break;
                case 0x69:
					hid_cmd_get_peq(buf);
					break;
                default:
                    break;
            }
        }
        
        case 'D':
            //update uppack
            break;
        
        case 'A':
        {
            if(hid_get_loopback_mode() > 0)
            {
                //printk("enter loopback test\n");
				hid_enter_loopback(buf, len);
				break;
			}
#if 0
			if(hid_channel_mode == CHANNEL_MODE_AT)
			{
				hid_channel_send_at(buf);
			}
			else if(hid_channel_mode == CHANNEL_MODE_SPP_SHARE ||hid_channel_mode == CHANNEL_MODE_SPP_EXC)
			hid_channel_send_spp(buf);
#endif
            break;
        }
        default:
            break;
	}
#endif
}


extern void hid_telephone_process(u8_t *temp_data);
extern u8_t g_incall_zoom;
/***  hid rvdata cb ***/
void hid_out_rev_data(void * arg1, void *arg2, void *arg3)
{
    int ret = 0;
    unsigned char buf[64] = {0};

    printk("\n\n\n---hid_out_rev enter---\n\n\n");
	//bIdle =LOOPBACK_CLIENT_IDLE |(LOOPBACK_SERVER_IDLE);
	detect_time = k_uptime_get_32();

#ifdef CONFIG_SPPBLE_DATA_CHAN
//    usb_hid_register_recv_ble_data_cb(cli_send_ble_data_to_pc);
#endif
    
    while(!hid_thread_exit)
    {
        ret = usb_hid_ep_read(buf, 64, -1);
        //printk("===== buf[0]:%x, buf[1]:%x, buf[2]:%x, buf[3]:%x, buf[4]:%x\n",buf[0],buf[1],buf[2],buf[3],buf[4]);
        if(ret == 0)
        {
            hid_command_process_func(buf, 0, 64);
    	}
        else
        {
            if(buf[0] == TELEPHONE_REPORT_ID1)
            {
                g_incall_zoom = 1;
                hid_telephone_cmd_out = 1;
                hid_telephone_process(buf);
                hid_telephone_cmd_out = 0;
            }
        }
    }
    hid_thread_exit = -1;
}

int usb_hid_device_exit(void)
{
#ifdef CONFIG_HID_INTERRUPT_OUT
    int time;
    SYS_LOG_INF("Enter\n");

    if(!hid_thread_exit)
        hid_thread_exit = 1;

    k_sem_give(&hid_read_sem);
    time = k_uptime_get_32();
    while(hid_thread_exit != -1)
    {
        k_sleep(K_MSEC(10));
        if(k_uptime_get_32() - time >= 300)
            break;
    }
#ifdef OTA_BACKEND_USBHID
	//os_delayed_work_cancel(&usb_hid_switch_work);
#endif

    if(hid_thread_stack)
    {
        mem_free(hid_thread_stack);
        hid_thread_stack = NULL;
    }
#ifdef CONFIG_USB_HID_TEAMS_CERT
    if(asp_temp_data)
        mem_free(asp_temp_data);
#endif
    return 0;
#else
    return 0;
#endif
}

int usb_hid_exit_func(void)
{
#ifdef CONFIG_HID_INTERRUPT_OUT
    return hid_thread_exit;
#else
    return 0;
#endif
}


