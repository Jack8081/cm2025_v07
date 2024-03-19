/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief hardware module for CUCKOO processor
 */
//#include <soc_regs.h>
#include "soc_common.h"

void delay(unsigned int count)
{
	volatile unsigned int i;
	for(i=0;i<count;i++)
	{
		
	}
}

#define	RESET_ID_MAX_ID			28
static void att_reset_peripheral_control(int reset_id, int assert)
{

	if (reset_id > RESET_ID_MAX_ID)
		return;

	if (assert) {
		sys_clear_bit(RMU_MRCR0, reset_id);
	} else {
		sys_set_bit(RMU_MRCR0, reset_id);
	}
}

void att_reset_peripheral_assert(int reset_id)
{
	att_reset_peripheral_control(reset_id, 1);
}

void att_reset_peripheral_deassert(int reset_id)
{
	att_reset_peripheral_control(reset_id, 0);
}

void att_reset_peripheral(int reset_id)
{
	att_reset_peripheral_assert(reset_id);
	att_reset_peripheral_deassert(reset_id);
}

#define	CLOCK_ID_MAX_ID			31
static void att_clock_peripheral_control(int clock_id, int enable)
{

	if (clock_id > CLOCK_ID_MAX_ID)
		return;


	if (enable) {
		sys_set_bit(CMU_DEVCLKEN0, clock_id);
	} else {
		sys_clear_bit(CMU_DEVCLKEN0, clock_id);
	}
}

void att_clock_peripheral_enable(int clock_id)
{
	att_clock_peripheral_control(clock_id, 1);
}

void att_clock_peripheral_disable(int clock_id)
{
	att_clock_peripheral_control(clock_id, 0);
}



