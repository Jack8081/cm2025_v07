/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for ATJ215X processor
 */

#include <device.h>
#include <init.h>
#include <arch/cpu.h>
#include "soc.h"

//#include <arch/arm/aarch32/cortex_m/cmsis.h>


static void jtag_config(unsigned int group_id)
{
	printk("jtag switch to group=%d\n", group_id);
	if (group_id < 2)
		sys_write32((sys_read32(JTAG_CTL) & ~(3 << 0)) | (group_id << 0) | (1 << 4), JTAG_CTL);
}

void jtag_set(void)
{
	jtag_config(0);
}

int soc_dvfs_opt(void)
{
	return sys_read32(0x400000a0)&0x0f;
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int cuckoo_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 16 MHz from HSI */
	//SystemCoreClock = 16000000;
	//while(!arg);

	//RAM4 shareRAM select HOSC(32MHZ)
	sys_write32((sys_read32(CMU_MEMCLKSRC0)&(~0x3ff)) | (0x1<<5),
				CMU_MEMCLKSRC0);
	//ANCDSP RAM select HOSC(32MHZ)
	sys_write32((sys_read32(CMU_MEMCLKSRC1)&(~0xf)) | (0x1<<1),
				CMU_MEMCLKSRC1);

	/*for lowpower*/
	//sys_write32(0x30F, SPI1_CLKGATING);
	
	return 0;
}

SYS_INIT(cuckoo_init, PRE_KERNEL_1, 0);
