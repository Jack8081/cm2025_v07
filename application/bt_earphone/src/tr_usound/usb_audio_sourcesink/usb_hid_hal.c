/*
 * Copyright (c) 2018 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <usb/class/usb_audio.h>
#include <usb/usb_device.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <sys/byteorder.h>

#include "usb_hid_inner.h"
#include "./tr_usb_hid.c" 

#define LOG_LEVEL CONFIG_SYS_LOG_USB_SOURCESINK_LEVEL
//#include <logging/log.h>
//LOG_MODULE_REGISTER(usb_hid_hal);

system_call_status_flag call_status_cb;
static u8_t usb_hid_buf[64];

static struct k_sem hid_write_sem;

extern void usound_send_key_msg(int msg_type);
static u32_t g_recv_report;
static u8_t  g_incall_status = INCALL_STATUS_NULL;
static u8_t  g_mute_status;
u8_t  g_incall_zoom = 0;
static u32_t  g_last_hook_down_time;
static struct k_delayed_work hid_telephone_ring_handle_work;

void usound_hid_hung_up_notify(void)
{
	g_last_hook_down_time = k_uptime_get_32();
}

void usound_hid_if_ringed(int status)
{
    g_incall_status |= status;
}

void usb_audio_register_call_status_cb(system_call_status_flag cb)
{
	call_status_cb = cb;
}

static void _hid_inter_out_ready_cb(void)
{
#ifdef CONFIG_HID_INTERRUPT_OUT
	hid_read_complete();
#else
	u32_t bytes_to_read;
	u8_t ret;
	u8_t usb_hid_out_buf[64];
	/* Get all bytes were received */
	ret = hid_int_ep_read(usb_hid_out_buf, sizeof(usb_hid_out_buf),
			&bytes_to_read);
	if (!ret && bytes_to_read != 0) {
		for (u8_t i = 0; i < bytes_to_read; i++) {
			SYS_LOG_DBG("usb_hid_out_buf[%d]: 0x%02x", i,
			usb_hid_out_buf[i]);
		}

	} else {
		SYS_LOG_INF("Read Self-defined data fail");
	}
#endif
}

static void _hid_inter_in_ready_cb(void)
{
	SYS_LOG_DBG("Self-defined data has been sent");
	k_sem_give(&hid_write_sem);
}

static int _usb_hid_tx(const u8_t *buf, u16_t len)
{
	u32_t wrote, timeout = 0;
	u8_t speed_mode = usb_device_speed();
    int ret = 0;

	/* wait one interval at most, unit: 125us */
	if (speed_mode == USB_SPEED_HIGH) {
		timeout = (1 << (CONFIG_HID_INTERRUPT_EP_INTERVAL_HS - 1))/8 + 1;
	} else if (speed_mode == USB_SPEED_FULL || speed_mode == USB_SPEED_LOW) {
		timeout = CONFIG_HID_INTERRUPT_EP_INTERVAL_FS + 1;
	}

	k_sem_take(&hid_write_sem, K_MSEC(timeout));
    
    if(buf[0] == TELEPHONE_REPORT_ID1)
    {
        printk("hid_tel_ctrl_send: %02x %02x %02x\n", buf[0], buf[1], buf[2]);
    }

    u32_t start_time;
    start_time = k_cycle_get_32();

    do {
        if (k_cycle_get_32() - start_time > HID_TIME_OUT * 24000)
        {
            ret = -ETIME;
            break;
        }

        ret = hid_int_ep_write(buf, len, &wrote);
    } while (ret == -EAGAIN);

	return ret;
}

static int _usb_hid_tx_command(u8_t hid_cmd)
{
	u8_t data[3], ret;

	memset(data, 0, sizeof(data));
	data[0] = REPORT_ID_6;
	data[1] = hid_cmd;
	ret = _usb_hid_tx(data, sizeof(data));
	if (ret == 0) {
		memset(data, 0, sizeof(data));
		data[0] = REPORT_ID_6;
		ret = _usb_hid_tx(data, sizeof(data));
		SYS_LOG_DBG("test_ret: %d\n", ret);
	}
	return ret;
}

#define INCALL_HOOK_UP      0x400
#define INCALL_HOOK_DOWN     0x800

int usb_hid_telephone_ctrl(u32_t hid_cmd)
{
    int ret;
    u32_t cmd = hid_cmd;

    ret = _usb_hid_tx((u8_t *)&cmd, sizeof(hid_cmd) - 1);
    return ret;
}

void usb_hid_hung_up(void)
{
    usound_hid_if_ringed(INCALL_HOOK_DOWN);
    usb_hid_telephone_ctrl(TELEPHONE_HOOK_SWITCH);
    //k_sleep(1);
    usb_hid_telephone_ctrl(TELEPHONE_HOOK_SWITCH_END);
    usound_hid_hung_up_notify();
}

void usb_hid_answer(void)
{
    //挂断和接听按键消息
    usound_hid_if_ringed(INCALL_HOOK_UP);
    usb_hid_telephone_ctrl(TELEPHONE_HOOK_SWITCH_END);
    k_sleep(K_MSEC(1));
	usb_hid_telephone_ctrl(TELEPHONE_HOOK_SWITCH);
}

void usb_hid_refuse_incoming(void)
{
    usb_hid_telephone_ctrl(TELEPHONE_BUTTON);
    k_sleep(K_MSEC(1));
    usb_hid_telephone_ctrl(TELEPHONE_HOOK_SWITCH_END);
}

void usb_hid_refuse_outgoing(void)
{
    usb_hid_telephone_ctrl(TELEPHONE_HOOK_SWITCH);
    k_sleep(K_MSEC(1));
    usb_hid_telephone_ctrl(TELEPHONE_HOOK_SWITCH_END);
}

void usb_hid_mute(void)
{
    if(g_incall_zoom)
    {
        usb_hid_telephone_ctrl(TELEPHONE_MUTE3);
        os_sleep(1);
        usb_hid_telephone_ctrl(TELEPHONE_MUTE4);
    }
    else
    {
        usb_hid_telephone_ctrl(TELEPHONE_MUTE1);
        os_sleep(1);
        usb_hid_telephone_ctrl(TELEPHONE_MUTE2);
    }
}

void usb_hid_refuse_phone(void)
{
    if(g_incall_status == INCALL_STATUS_RING)
    {
        usb_hid_refuse_incoming();
    }
    else if(g_incall_status == INCALL_STATUS_OUTCALL)
    {
        usb_hid_refuse_outgoing();
    }
    else
    {
        usb_hid_hung_up();
    }
}

static int _usb_hid_debug_cb(struct usb_setup_packet *setup, s32_t *len,
	     u8_t **data)
{
	SYS_LOG_DBG("Debug callback");

	return -ENOTSUP;
}

int usb_hid_control_pause_play(void)
{
	return _usb_hid_tx_command(PLAYING_AND_PAUSE);
}

int usb_hid_control_volume_dec(void)
{
	return _usb_hid_tx_command(PLAYING_VOLUME_DEC);
}

int usb_hid_control_volume_inc(void)
{
	return _usb_hid_tx_command(PLAYING_VOLUME_INC);
}

int usb_hid_control_volume_mute(void)
{
	return _usb_hid_tx_command(PLAYING_VOLUME_MUTE);
}

int usb_hid_control_play_next(void)
{
	return _usb_hid_tx_command(PLAYING_NEXTSONG);
}

int usb_hid_control_play_prev(void)
{
	return _usb_hid_tx_command(PLAYING_PREVSONG);
}

extern u8_t hid_telephone_cmd_out;

void hid_telephone_ring_handle(struct k_work * work)
{
    printk("%s:hung up\n",__func__);
    g_incall_status = INCALL_STATUS_NULL;
	g_last_hook_down_time = k_uptime_get_32();
	usound_send_key_msg(3);
}

void hid_telephone_process(u8_t *temp_data)
{
    printk("***********report:%02x %02x\n",temp_data[1],temp_data[2]);
    if(temp_data[1] == 0 && (g_recv_report & 0xff) == 0)
    {
        g_recv_report = (temp_data[2] << 16) | (temp_data[1] << 8) | temp_data[0];
        g_incall_status = 0;
        return;
    }

    if(temp_data[1] == TELEPHONE_RING && g_incall_status == INCALL_STATUS_NULL)  //来电响铃
    {
        printk("%s:ring\n",__func__);
        g_incall_status = INCALL_STATUS_RING;
		usound_send_key_msg(1);
    }
    else if(temp_data[1] == 0x0 && g_incall_status == INCALL_STATUS_RING && hid_telephone_cmd_out == 1) //zoom在接听操作前会先清下状态
    {
        printk("%s:ring off\n",__func__);
        g_incall_status = INCALL_STATUS_NULL;
        os_delayed_work_submit(&hid_telephone_ring_handle_work, 5);
        return;
    }
    else if((temp_data[1] == TELEPHONE_HOOK_SWITCH_STATE)
		&& ((((g_recv_report>>8) & 0xff) == TELEPHONE_HOOK_SWITCH_STATE) ||((g_recv_report>>8) & 0xff) == TELEPHONE_RING)
		&& g_incall_status != INCALL_STATUS_ACTIVE //接听电话
		&& g_incall_status != INCALL_STATUS_OUTCALL) 
    {
        printk("%s:active\n",__func__);
        k_delayed_work_cancel(&hid_telephone_ring_handle_work);
        g_incall_status = INCALL_STATUS_ACTIVE;
		usound_send_key_msg(2);
    }
    else if(temp_data[1] == 0x0
		&& ((((g_recv_report>>8) & 0xff) == TELEPHONE_HOOK_SWITCH_STATE) ||((g_recv_report>>8) & 0xff) == TELEPHONE_RING) ) //挂断
    {
        printk("%s:hung up\n",__func__);
        g_incall_status = INCALL_STATUS_NULL;
		g_last_hook_down_time = k_uptime_get_32();
        g_incall_zoom = 0;
		usound_send_key_msg(3);
    }
    else if(temp_data[1] == 0x0
		&& temp_data[2] == 0x0) //挂断
    {
        printk("%s:hung up\n",__func__);
        g_incall_status = INCALL_STATUS_NULL;
        g_mute_status = 0;
        g_recv_report = 0;
        g_incall_zoom = 0;
		usound_send_key_msg(3);
    }
    else if (temp_data[1] == TELEPHONE_HOOK_SWITCH_STATE && temp_data[2] == TELEPHONE_MUTE_STAT && !g_mute_status)
    {
        printk("%s:mute\n",__func__);
        g_mute_status = 1;
		usound_send_key_msg(4);
    }
    else if (temp_data[1] == TELEPHONE_HOOK_SWITCH_STATE && temp_data[2] == 0x00 && g_mute_status)
    {
        printk("%s:unmute\n",__func__);
        g_mute_status = 0;
		usound_send_key_msg(5);
    }
    else if ((temp_data[1] == TELEPHONE_HOOK_SWITCH_STATE)
		//&& (((g_recv_report>>8) & 0xff) == TELEPHONE_HOOK_SWITCH_STATE)
        && (g_incall_status == INCALL_STATUS_NULL)
		//&& (bOpenMic == 0)
        && k_uptime_get_32() - g_last_hook_down_time > 700)
    {
        printk("%s:outgoing\n",__func__);
        g_incall_status = INCALL_STATUS_OUTCALL;    //detect outgoing
        usound_send_key_msg(6);
    }
    else
    {
        return;
    }

    g_recv_report = (temp_data[2] << 16) | (temp_data[1] << 8) | temp_data[0];
}
/*
 * Self-defined protocol via set_report
 */
static int _usb_hid_set_report(struct usb_setup_packet *setup, s32_t *len,
				u8_t **data)
{
    
    if (sys_le16_to_cpu(setup->wValue) == 0x0300)
    {
        SYS_LOG_INF("Set Feature\n");
        return 0 ;
    }

	u8_t *temp_data = *data;

	SYS_LOG_DBG("temp_data[0]: 0x%04x\n", temp_data[0]);
	SYS_LOG_DBG("temp_data[1]: 0x%04x\n", temp_data[1]);
	SYS_LOG_DBG("temp_data[2]: 0x%04x\n", temp_data[2]);

	SYS_LOG_DBG("g_recv_report:0x%04x\n", g_recv_report);
    
    if(temp_data[0] == TELEPHONE_REPORT_ID1) //telephony led status
    {
        hid_telephone_process(temp_data);
        return 0;
    }
        
    g_recv_report = (temp_data[2] << 16) | (temp_data[1] << 8) | temp_data[0];
	
    if (call_status_cb) {
			call_status_cb(g_recv_report);
	}

	return 0;
}

u32_t usb_hid_get_call_status(void)
{
	return g_recv_report;
}

static int _usb_hid_get_report(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
    switch (sys_le16_to_cpu(setup->wValue)) 
    {
        case 0x0300:
            SYS_LOG_INF("Get Feature\n");
            usb_hid_buf[0] = 0x41;
            usb_hid_buf[1] = 0x63;
            usb_hid_buf[2] = 0x74;
            usb_hid_buf[3] = 0x69;
            usb_hid_buf[4] = 0x6F;
            usb_hid_buf[5] = 0x6E;
            usb_hid_buf[6] = 0x00;
            usb_hid_buf[7] = 0x46;
            usb_hid_buf[8] = 0x72;
            *data = usb_hid_buf;
			*len = min(*len, 64);
            break;
        
        default:
            break;
    }
    return 0;
}

static const struct hid_ops ops = {
	.get_report = _usb_hid_get_report,
	.get_idle = _usb_hid_debug_cb,
	.get_protocol = _usb_hid_debug_cb,
	.set_report = _usb_hid_set_report,
	.set_idle = _usb_hid_debug_cb,
	.set_protocol = _usb_hid_debug_cb,
	.int_in_ready = _hid_inter_in_ready_cb,
	.int_out_ready = _hid_inter_out_ready_cb,
};

int usb_hid_dev_init(void)
{
#ifdef CONFIG_HID_INTERRUPT_OUT
    hid_sem_init();

    hid_thread_exit = 0;

    if(hid_thread_stack)
        os_thread_create(hid_thread_stack, 1536, hid_out_rev_data, NULL, NULL, NULL, 5, 0, OS_NO_WAIT);
#endif
	/* Register hid report desc to HID core */
	usb_hid_register_device(hid_report_desc, sizeof(hid_report_desc) - 15, &ops);

	k_sem_init(&hid_write_sem, 1, 1);

	/* USB HID initialize */
    k_delayed_work_init(&hid_telephone_ring_handle_work, hid_telephone_ring_handle);
	
    return usb_hid_composite_init();
}


/*
 * media key value control
 * Examples:
 *     hid key dec
 *     hid key inc
 *     hid key next
 *     hid key pre
 *     hid key pause
 */
static int shell_hid_media_key_trl(const struct shell *shell, size_t argc, char **argv)
{
	if (!strcmp(argv[1], "dec")) {
		usb_hid_control_volume_dec();
	} else if (!strcmp(argv[1], "inc")) {
		usb_hid_control_volume_inc();
	} else if (!strcmp(argv[1], "next")) {
		usb_hid_control_play_next();
	} else if (!strcmp(argv[1], "pre")) {
		usb_hid_control_play_prev();
	} else if (!strcmp(argv[1], "pause")) {
		usb_hid_control_pause_play();
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(usb_hid_media,
	SHELL_CMD(media, NULL, "hid media key control", shell_hid_media_key_trl),
);

SHELL_CMD_REGISTER(key, &usb_hid_media, "hid media key ctrl", NULL);
