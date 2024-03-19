/**
 *  ***************************************************************************
 *  Copyright (c) 2014-2020 Actions (Zhuhai) Technology. All rights reserved.
 *
 *  \file       soc_reset.c
 *  \brief      reset operations
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

#define RESET_MAX 32

//#define     RMU_BASE                                                          0x40000000
//#define     MRCR                                                              (RMU_BASE+0x00000000)

static void reset_and_enable_set(int rst_id, int assert)
{
	if(rst_id >= RESET_MAX) {
		return;
	}
	
	if (assert) {
		sys_clrsetbits(RMU_MRCR0, 1 << rst_id, 0);
	} else {
		sys_clrsetbits(RMU_MRCR0, 1 << rst_id, 1 << rst_id);
	}
}

void reset_assert(int rst_id)
{
	reset_and_enable_set(rst_id, 1);
}

void reset_deassert(int rst_id)
{
	reset_and_enable_set(rst_id, 0);
}

void reset_and_enable(int rst_id)
{
	reset_assert(rst_id);
	udelay(1);
	reset_deassert(rst_id);
}

void reset_and_enable_clk(int rst_id, int clk_id)
{
	reset_assert(rst_id);
	clk_enable(clk_id);
	udelay(1);
	reset_deassert(rst_id);
}
