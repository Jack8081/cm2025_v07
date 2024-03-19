/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2021 Actions Semiconductor. All rights reserved.
 *
 *  \file       tool_uart.c
 *  \brief      uart access during tool connect
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2021-9-11
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include <errno.h>
#include "tool_app_inner.h"
#include "tts_manager.h"
#include <drivers/uart.h>
#include <spp_test_backend.h>
#include <soc_pm.h>
#include <msg_manager.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#include <media_mem.h>
#include <audio_system.h>


#define USB_MAX_HS_BULK_MPS	512
#define USB_WRITE_CBUF_SIZE 2048

#define USB_CONNECTION_INQUIRY_TIMER_MS    (1000)

#define TOOL_USB_THREAD_PRIO (8)


extern int usb_phy_enter_b_idle(void);
extern int usb_phy_init(void);
#ifdef CONFIG_POWER_MANAGER
extern int power_manager_get_dc5v_status(void);
#endif

#ifdef CONFIG_TOOL_ASET_USB_SUPPORT

/**************************************************************
* Description: usp handle api for aset stub
* Input: mode
* Output: none
* Return: success/fail
* Note:
***************************************************************
**/
int usb_driver_ioctl(uint32_t mode, void *para1, void *para2)
{
    int ret = 0;

    switch (mode) {
        case USP_IOCTL_INQUIRY_STATE:
        #ifdef CONFIG_POWER_MANAGER
        if (!power_manager_get_dc5v_status()) {
			SYS_LOG_INF("usb cable is not in!");
            ret = -1;
        }
        #endif
        break;

        default:
        break;
    }

    return ret;
}

/**************************************************************
* Description: tool init work for aset stub
* Input: work
* Output: none
* Return: none
* Note:
***************************************************************
**/
static void tool_init_work(struct k_work *work)
{
	struct app_msg msg;
	
	memset(&msg, 0, sizeof(msg));

	msg.type = MSG_APP_TOOL_INIT;
	msg.cmd = 1;

	send_async_msg(APP_ID_MAIN, &msg);
}

/**************************************************************
* Description: tool exit work for aset stub
* Input: work
* Output: none
* Return: none
* Note:
***************************************************************
**/
static void tool_exit_work(struct k_work *work)
{
	struct app_msg msg;
	
	memset(&msg, 0, sizeof(msg));

	msg.type = MSG_APP_TOOL_EXIT;
	msg.cmd = 1;

	send_async_msg(APP_ID_MAIN, &msg);
}


/**************************************************************
* Description: init for stub device
* Input: none
* Output: none
* Return: 
* Note:
***************************************************************
**/
int tool_init_usb(void)
{
	void (*tool_process_deal)(void *p1, void *p2, void *p3) = NULL;

	g_tool_data.dev_type = TOOL_DEV_TYPE_USB;

	memset(&g_tool_data.usp_handle, 0, sizeof(usp_handle_t));

	g_tool_data.usp_handle.api.read  = (void*)usb_stub_dev_read_data;
	//g_tool_data.usp_handle.api.write = (void*)usb_stub_ep_write;
	g_tool_data.usp_handle.api.write = (void*)usb_stub_ep_write_async;
	g_tool_data.usp_handle.api.ioctl = (void*)usb_driver_ioctl;

	g_tool_data.usp_handle.usp_protocol_global_data.transparent_bak = 1;
	g_tool_data.usp_handle.usp_protocol_global_data.transparent = 1;	

    if (NULL == g_tool_data.stack) {
	    g_tool_data.stack = app_mem_malloc(CONFIG_TOOL_STACK_SIZE);
    } else {
		if(!g_tool_data.quited){			
			return -EALREADY;			
		}
	}

	if (!g_tool_data.stack) {
		SYS_LOG_ERR("tool stack mem_malloc failed");
		return -ENOMEM;
	}

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

	tool_process_deal = tool_stub_loop_usb;

	g_tool_data.quit = 0;
	g_tool_data.quited = 0;
	g_tool_data.aset_usb_status = ASET_USB_STATUS_INIT;

	os_thread_create(g_tool_data.stack, CONFIG_TOOL_STACK_SIZE,
			tool_process_deal, NULL, NULL, NULL, TOOL_USB_THREAD_PRIO, 0, 0);

	SYS_LOG_INF("tool thread create ok: %p", g_tool_data.stack);
	
	return 0;
}


/**************************************************************
* Description: init for stub device
* Input: none
* Output: none
* Return: none
* Note:
***************************************************************
**/
void tool_sysrq_init_usb(void)
{
    //install usb stub
	usb_phy_enter_b_idle();
	usb_phy_init();

	//malloc stub read buffer, 512bytes
#ifdef CONFIG_BT_TRANSCEIVER
	g_tool_data.stub_read_buf = media_mem_get_cache_pool(RESAMPLE_SHARE_DATA, AUDIO_STREAM_TIP);
#else
	g_tool_data.stub_read_buf = mem_malloc(USB_MAX_HS_BULK_MPS);
#endif
	if(!g_tool_data.stub_read_buf) {
		SYS_LOG_INF("malloc stub read buf fail!");
		return;
	}

	if (!g_tool_data.stub_write_buf) {
#ifdef CONFIG_BT_TRANSCEIVER
	    g_tool_data.stub_write_buf = media_mem_get_cache_pool(RESAMPLE_FRAME_DATA, AUDIO_STREAM_TIP);
#else
		g_tool_data.stub_write_buf = mem_malloc(USB_WRITE_CBUF_SIZE);
#endif
		if (!g_tool_data.stub_write_buf) {
			SYS_LOG_ERR("tx_stub_buf malloc fail");
			return;
		}
	}

	usb_stub_dev_init(NULL, g_tool_data.stub_read_buf, USB_MAX_HS_BULK_MPS, g_tool_data.stub_write_buf, USB_WRITE_CBUF_SIZE);

	g_tool_data.stub_install_timer_ms = os_uptime_get_32();
	g_tool_data.wait_stub_drv_ok_flag = true;
	SYS_LOG_INF("stub install time: %d", g_tool_data.stub_install_timer_ms);

	tool_init_usb();
}

/**************************************************************
* Description: exit for stub device
* Input: none
* Output: none
* Return: none
* Note:
***************************************************************
**/
void tool_sysrq_exit_usb(void)
{
	SYS_LOG_INF("tool stub exit now!\n");
	g_tool_data.aset_usb_status = ASET_USB_STATUS_FINISH;
	tool_deinit();
}

/**************************************************************
* Description: timer to detect usb stub in or out
* Input: ttimer
* Output: none
* Return: none
* Note:
***************************************************************
**/
void det_ttimer_handler(os_timer *ttimer)
{
	bool new_usb_status;
	bool need_restart_timer = TRUE;
    
#ifdef CONFIG_POWER_MANAGER
	new_usb_status = power_manager_get_dc5v_status();
#else
    new_usb_status = true;
#endif

	if (g_tool_data.usb_in_flag != 0) {
		if(!new_usb_status) {
			SYS_LOG_INF("usb disconnect");
			g_tool_data.usb_in_flag = 0;
			g_tool_data.exit_work = true;
			k_work_init(&g_tool_data.work_exit, tool_exit_work);
			k_work_submit(&g_tool_data.work_exit);							
		}
	} else {
		if(new_usb_status) {
			SYS_LOG_INF("usb in, need init stub!");
			g_tool_data.usb_in_flag = 1;
			g_tool_data.init_work = true;
			k_work_init(&g_tool_data.work, tool_init_work);
			k_work_submit(&g_tool_data.work);				
		}
	}

	if(need_restart_timer) {
		os_timer_start(ttimer, K_MSEC(USB_CONNECTION_INQUIRY_TIMER_MS), K_MSEC(0));
	}
}


/**************************************************************
* Description: start usb stub check timer
* Input: none
* Output: none
* Return: none
* Note:
***************************************************************
**/
void start_tool_stub_timer(void)
{
	SYS_LOG_INF("start stub timer!");
    os_timer_init(&g_tool_data.stub_det_timer, det_ttimer_handler, NULL);   
    os_timer_start(&g_tool_data.stub_det_timer, K_MSEC(USB_CONNECTION_INQUIRY_TIMER_MS), K_MSEC(0)); 
}

#endif

