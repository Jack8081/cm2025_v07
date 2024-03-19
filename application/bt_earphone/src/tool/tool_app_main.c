/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
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


/* magic sysrq key: CTLR + 'A' 'C' 'T' 'I' 'O' 'N' 'S' */
const char sysrq_toolbuf[7] = {0x01, 0x03, 0x20, 0x09, 0x15, 0x14, 0x19};
const uint8_t tool_ack[6] = {0x05, 0x00, 0x00, 0x00, 0x00, 0xcc};
static uint8_t sysrq_tool_idx, tool_type_idx;

#ifdef CONFIG_ATT_SUPPORT_MULTIPLE_DEVICES
/* ascending sequence, and the bigest value should be 880(0x370) or lower */
static const u16_t tool_adc2serial[] = {0, 90, 180, 270, 360, 450, 540, 630, 720, 880};
#endif

act_tool_data_t g_tool_data;

extern void shell_dbg_disable(void);
extern void sysrq_register_user_handler(void (*callback)(const struct device * port, char c));


int tool_init(char *type, unsigned int dev_type)
{
    void (*tool_process_deal)(void *p1, void *p2, void *p3) = NULL;

    if (NULL == g_tool_data.stack) {
	    g_tool_data.stack = app_mem_malloc(CONFIG_TOOL_STACK_SIZE);
		SYS_LOG_INF("stack %p %s\n", g_tool_data.stack, type);
    } else {
		//if tools thread not exit, cannot init again
		if(!g_tool_data.quited){			
			return -EALREADY;			
		}
	}

	if (!g_tool_data.stack) {
		SYS_LOG_ERR("tool stack mem_malloc failed");
		return -ENOMEM;
	}

    memset(&g_tool_data.usp_handle, 0, sizeof(usp_handle_t));

    if (TOOL_DEV_TYPE_SPP == dev_type) {
        g_tool_data.usp_handle.usp_protocol_global_data.transparent_bak = 1;

        g_tool_data.usp_handle.api.read    = (void*)spp_test_backend_read;
        g_tool_data.usp_handle.api.write   = (void*)spp_test_backend_write;
        g_tool_data.usp_handle.api.ioctl   = (void*)spp_test_backend_ioctl;

        tool_process_deal = tool_spp_test_main;
    } else {
        g_tool_data.dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
        // g_tool_data.usp_handle.usp_protocol_global_data.transparent_bak = 0;

        g_tool_data.usp_handle.api.read    = (void*)tool_uart_read;
        g_tool_data.usp_handle.api.write   = (void*)tool_uart_write;
        g_tool_data.usp_handle.api.ioctl   = (void*)tool_uart_ioctl;

        if(0 == strcmp(type, "aset")) {
            g_tool_data.type = USP_PROTOCOL_TYPE_ASET;
            tool_process_deal = tool_aset_loop;
        }
#ifdef CONFIG_ACTIONS_ATT
        else if(0 == strcmp(type, "att")) {
            g_tool_data.type = USP_PROTOCOL_TYPE_STUB;
            tool_process_deal = tool_att_loop;
        }
#endif
        else {
            return -1;
        }
    }

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

	g_tool_data.dev_type = dev_type;
	g_tool_data.quit = 0;
	g_tool_data.quited = 0;

	// os_sem_init(&g_tool_data.init_sem, 0, 1);
	os_thread_create(g_tool_data.stack, CONFIG_TOOL_STACK_SIZE,
			tool_process_deal, NULL, NULL, NULL, 8, 0, 0);

	// os_sem_take(&g_tool_data.init_sem, OS_FOREVER);

	SYS_LOG_INF("begin trying to connect pc tool");
	return g_tool_data.type;
}

void tool_deinit(void)
{
	if (!g_tool_data.stack) {
		return;
	}
	SYS_LOG_WRN("will disconnect pc tool, then exit");

	g_tool_data.quit = 1;
	while (!g_tool_data.quited) {
		os_sleep(1);
	}
	
	app_mem_free(g_tool_data.stack);
	g_tool_data.stack = NULL;
#ifdef CONFIG_SYS_WAKELOCK
    sys_wake_unlock(FULL_WAKE_LOCK);
#endif
	SYS_LOG_INF("exit now");
}

static void tool_init_sysrq(const struct device *port)
{
	struct app_msg msg;
	
	memset(&msg, 0, sizeof(msg));

	msg.type = MSG_APP_TOOL_INIT;
	msg.cmd = 0;

	send_async_msg(APP_ID_MAIN, &msg);
}

void tool_sysrq_init(void)
{
	tool_init(g_tool_data.type_buf, TOOL_DEV_TYPE_UART);
}

static void tool_init_work(struct k_work *work)
{
	const struct device *dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);

	if(dev){
		tool_init_sysrq(dev);
	}
}

void handle_sysrq_tool_main(const struct device * port)
{	
	//must called by application init, avoid printk switch dma crash
	if (!g_tool_data.init_printk_hw){		
		return;
	}
	
	if (g_tool_data.init_work){
		return;
	}

#ifdef CONFIG_SHELL_DBG
	shell_dbg_disable();
#endif

	printk("parse tool type:%s\n", g_tool_data.type_buf);

	//response ack    
	tool_uart_write(tool_ack, sizeof(tool_ack), 0);

    if (0 == strcmp(g_tool_data.type_buf, "reboot")) {
        printk("reboot system\n");
        k_busy_wait(200 * 1000);
        sys_pm_reboot(0);
    }

	g_tool_data.init_work = true;

	k_work_init(&g_tool_data.work, tool_init_work);
	k_work_submit(&g_tool_data.work);
}

static void sysrq_tool_handler(const struct device * port, char c)
{
	if (c == sysrq_toolbuf[sysrq_tool_idx]) {
		sysrq_tool_idx++;
	} else if (sysrq_tool_idx == sizeof(sysrq_toolbuf)) {
		if (tool_type_idx >= (ARRAY_SIZE(g_tool_data.type_buf) - 1)) {
			tool_type_idx = 0;
			sysrq_tool_idx = 0;
		} else if (0x0a == c) {
			g_tool_data.type_buf[tool_type_idx] = 0;
			handle_sysrq_tool_main(port);
			tool_type_idx = 0;
			sysrq_tool_idx = 0;
		} else {
			g_tool_data.type_buf[tool_type_idx++] = c;
		}
	} else {
		tool_type_idx = 0;
		sysrq_tool_idx = 0;
	}
}


static int uart_data_init(const struct device *dev)
{
	memset(&g_tool_data, 0, sizeof(g_tool_data));

	sysrq_register_user_handler(sysrq_tool_handler);
	
	g_tool_data.init_printk_hw = true;
	
	return 0;
}

SYS_INIT(uart_data_init, APPLICATION, 10);
