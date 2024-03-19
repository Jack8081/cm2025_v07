/*******************************************************************************
 *                              US212A
 *                            Module: Picture
 *                 Copyright(c) 2003-2012 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       zhangxs     2011-09-19 10:00     1.0             build this file
 *******************************************************************************/
#include "att_pattern_test.h"
#include "ap_autotest_loader.h"

autotest_test_info_t g_test_info;
struct att_env_var g_att_env_var;

extern void uart_init(int id, int mfp, int baud);

static void test_init(void)
{
	self = &g_att_env_var;

    self->test_mode = TEST_MODE_UART;
}


__ENTRY_CODE void pattern_main(void *p1, void *p2, void *p3)
{
	void (*exit_routinue)(void);

	memset(&__bss_start, 0,
		 ((u32_t) &__bss_end - (u32_t) &__bss_start)); 

	g_att_env_var.api = (att_interface_api_t *)p1;

	g_att_env_var.dev_tbl = (att_interface_dev_t*)p3;

	test_init();

	uart_init(0, 3, 2000000);    //GPIO5/6

	att_printf("\nVer1.0-att (build %s %s)\n", __DATE__, __TIME__);

    test_dispatch();

    g_att_env_var.api->att_end_deal = ATT_END_POWEROFF;
    //g_att_env_var.api->att_end_deal = ATT_END_REBOOT;
	att_printf("att_loader exit exit_routinue 0x%x\n",(u32_t)p2);
	
	exit_routinue = (void (*)(void))p2;

	exit_routinue();
	
    return;
}

