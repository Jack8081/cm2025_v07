/**
 *  ***************************************************************************
 *  Copyright (c) 2014-2020 Actions (Zhuhai) Technology. All rights reserved.
 *
 *  \file       soc_clock.c
 *  \brief      clock operations
 *  \author     songzhining
 *  \version    1.00
 *  \date       2020/5/9
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "sys_io.h"
#include "types.h"
#include "att_pattern_test.h"

#define CLOCK_MAX 32

#define     CMU_Control_Register_BASE                                         0x40001000
#define     CMU_DEVCLKEN                                                      (CMU_Control_Register_BASE+0x0004)


static void owl_clk_set(int clk_id, int enable)
{
    if(clk_id >= CLOCK_MAX) {
		return;
    }
	
	if (enable) {
		sys_clrsetbits(CMU_DEVCLKEN, 1 << clk_id, 1 << clk_id);
	} else {
		sys_clrsetbits(CMU_DEVCLKEN, 1 << clk_id, 0);
	}
}

void clk_enable(int clk_id)
{
	owl_clk_set(clk_id, 1);
}

void clk_disable(int clk_id)
{
	owl_clk_set(clk_id, 0);
}

