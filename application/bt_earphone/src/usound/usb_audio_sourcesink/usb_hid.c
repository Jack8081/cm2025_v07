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
#include "../usound.h"
#include <api_common.h>

u8_t *asp_temp_data = NULL;
int detect_time;
//u8_t bIdle = 0;

#define     CHANNEL_MODE_AT             0
#define     CHANNEL_MODE_SPP_SHARE      1
#define     CHANNEL_MODE_SPP_EXC        2
#define     CHANNEL_MODE_BLE            3
#define     CHANNEL_MODE_UART           4
#define     HID_TIME_OUT                50   //hid 写超时时间

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
		SYS_LOG_ERR("err len: %d, read: %d\n", len, bytes_ret);
		return -EIO;
	}

    return 0;
}
#endif

static void hid_cmd_enteradfu(u8_t param)
{
    printk("now reboot to adfu:%d\n", param);

    if(param == 0)
    {
        sys_pm_reboot(0x100);
        return;
    }
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

    if(param == 0)
    {
        sys_pm_reboot(0); //reboot
        return;
    }
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
    sys_pm_reboot(0); //reboot
    ota_upgrade_hid_exit();
    bt_manager_audio_start(BT_TYPE_LE);
}

//static u8_t is_updating_remote = 0;
uint8_t hid_is_updating_remote(void)
{
    return 0;
}
#if 0
static void hid_set_updating_remote(u8_t value)
{
    is_updating_remote = value;
}
#endif
#endif

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

void hid_event_data_download(u8_t result, u8_t idx)
{
    u8_t cmd_buf[CONFIG_HID_INTERRUPT_IN_EP_MPS] = {0};

	cmd_buf[0] = 'U';
	cmd_buf[1] = 0x61;
	cmd_buf[2] = result;
	cmd_buf[3] = idx;
	
    usb_hid_ep_write(cmd_buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, HID_TIME_OUT);
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

#define HID_DOWNLOAD_CACHE_BUF_MAX_LEN   256

void hid_command_process_func(u8_t * buf, int flag, u8_t len)
{
#ifdef  CONFIG_BT_TRANS_BLEDATA_SERVER_FAREND
    hid_trans_flag=flag;
#endif
#if 1
    switch(buf[0])
    {
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
                    break;
                case 0x0:
					hid_cmd_enteradfu(buf[2]);
					break;
				case 'Q':   //enter bqb mode
					hid_cmd_enterqbq_mode();
					break;
				case 'R':
					hid_cmd_reboot(buf[2]);
					break;
				case 0x5e:
					hid_cmd_get_version(buf[2], buf[3]);
					break;
                case 0x62:
					hid_cmd_rsp_data_upload(buf[2], buf[3]);
					break;

                default:
                    break;
            }
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
	detect_time = k_uptime_get_32();
    
    while(!hid_thread_exit)
    {
        ret = usb_hid_ep_read(buf, 64, -1);
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


