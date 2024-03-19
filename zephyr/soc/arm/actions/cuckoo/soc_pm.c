/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system reboot interface for Actions SoC
 */

#include <device.h>
#include <init.h>
#include <soc.h>
#include <pm/pm.h>
#include <drivers/rtc.h>
#include <board_cfg.h>
#include <linker/linker-defs.h>

#if (CONFIG_PM_BACKUP_TIME_FUNCTION_EN == 1)
#include <drivers/nvram_config.h>
#endif

#define REBOOT_REASON_MAGIC 		0x4252	/* 'RB' */

#ifdef CONFIG_SOC_EP_MODE
static struct {
	uint8_t  enable_inner_onoff_key;
    uint8_t  enable_dc5v_low_wake;
    uint8_t  dc5v_low_wake_voltage;
    uint8_t  enable_bat_low_wake;
    uint8_t  bat_low_wake_voltage;
} sys_pm_wakeup_config;


void sys_pm_set_dc5v_low_wakeup(int enable, int threshold)
{
	printk("dc5vlv enable=%d,threshold=0x%x\n", enable, threshold);
	sys_pm_wakeup_config.enable_dc5v_low_wake = enable;
	sys_pm_wakeup_config.dc5v_low_wake_voltage = threshold;
}

int sys_pm_get_dc5v_low_threshold(uint8_t *volt)
{
	uint32_t wake_ctl;

	wake_ctl = soc_pmu_get_wakeup_setting();

	if (wake_ctl & WAKE_CTL_DC5VLV_WKEN) {
		*volt = (wake_ctl & WAKE_CTL_DC5VLV_VOL_MASK) >> WAKE_CTL_DC5VLV_VOL_SHIFT;
		return 0;
	}

	return -1;
}

void sys_pm_set_bat_low_wakeup(int enable, int threshold)
{
	printk("batlv enable=%d,threshold=0x%x\n", enable, threshold);

	sys_pm_wakeup_config.enable_bat_low_wake = enable;
	sys_pm_wakeup_config.bat_low_wake_voltage = threshold;
}

int sys_pm_get_bat_low_threshold(uint8_t *volt)
{
	uint32_t wake_ctl;

	wake_ctl = soc_pmu_get_wakeup_setting();

	if (wake_ctl & WAKE_CTL_BATLV_WKEN) {
		*volt = (wake_ctl & WAKE_CTL_BATLV_VOL_MASK) >> WAKE_CTL_BATLV_VOL_SHIFT;
		return 0;
	}

	return -1;
}

void sys_pm_set_onoff_wakeup(int enable)
{
	sys_pm_wakeup_config.enable_inner_onoff_key = enable;
}
#endif

int sys_pm_get_wakeup_source(union sys_pm_wakeup_src *src)
{
	uint32_t wk_pd;

	if (!src)
		return -EINVAL;

	src->data = 0;

	wk_pd = soc_pmu_get_wakeup_source();

	if (wk_pd & BIT(0))
		src->t.long_onoff = 1;

	if (wk_pd & BIT(1))
		src->t.short_onoff = 1;

	if (wk_pd & BIT(2))
		src->t.dc5vin = 1;

	if (wk_pd & BIT(4))
		src->t.dc5vlv = 1;

	if (wk_pd & BIT(5))
		src->t.batlv = 1;

	if (wk_pd & BIT(6))
		src->t.wio1lv = 1;

	if (wk_pd & BIT(7))
		src->t.wio0 = 1;

	if (wk_pd & BIT(8))
		src->t.wio1 = 1;	

	if (wk_pd & BIT(9))
		src->t.wio2 = 1;		

	if (soc_boot_get_watchdog_is_reboot() == 1)
		src->t.watchdog = 1;

	return 0;
}

void sys_pm_set_wakeup_src(void)
{
	uint32_t key, val;

	key = irq_lock();

#ifdef CONFIG_SOC_EP_MODE
	val = 0;

	sys_write32(sys_read32(WAKE_PD_SVCC), WAKE_PD_SVCC);
	k_busy_wait(200);

	if (sys_pm_wakeup_config.enable_inner_onoff_key) {
		val = WAKE_CTL_LONG_WKEN | WAKE_CTL_SHORT_WKEN;
	}

	if (sys_pm_wakeup_config.enable_dc5v_low_wake) {
		val |= WAKE_CTL_DC5VLV_WKEN | (sys_pm_wakeup_config.dc5v_low_wake_voltage << WAKE_CTL_DC5VLV_VOL_SHIFT);
	}

	if (sys_pm_wakeup_config.enable_bat_low_wake) {
		val |= WAKE_CTL_BATLV_WKEN | (sys_pm_wakeup_config.bat_low_wake_voltage << WAKE_CTL_BATLV_VOL_SHIFT);
	}
#else
	val = WAKE_CTL_LONG_WKEN;
#endif

	if(!soc_pmu_get_dc5v_status()) {
		val |= WAKE_CTL_DC5VIN_WKEN;
	}

	sys_write32(val, WKEN_CTL_SVCC);
	k_busy_wait(500);

	irq_unlock(key);
}

//#if (CONFIG_PM_BACKUP_TIME_FUNCTION_EN == 1)
#if 0
void sys_pm_poweroff_backup_time(void)
{
	int ret;
	const struct device *rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
	struct sys_pm_backup_time pm_bak_time = {0};
	struct rtc_time rtc_time = {0};

	ret = soc_pmu_get_counter8hz_msec(true);

	if ((ret > 0) && rtc_dev) {
		pm_bak_time.counter8hz_msec = ret;
		ret = rtc_get_time(rtc_dev, &rtc_time);
		if (!ret) {
			rtc_tm_to_time(&rtc_time, &pm_bak_time.rtc_time_sec);
			pm_bak_time.rtc_time_msec = rtc_time.tm_ms;
			pm_bak_time.is_backup_time_valid = 1;
			ret = nvram_config_set(CONFIG_PM_BACKUP_TIME_NVRAM_ITEM_NAME,
					&pm_bak_time, sizeof(struct sys_pm_backup_time));
			if (ret) {
				printk("failed to save pm backup time to nvram\n");
			} else {
				printk("power off current 8hz:%dms ", pm_bak_time.counter8hz_msec);
				print_rtc_time(&rtc_time);
			}
		}
	}
}
#endif
//#endif

/*
**  system power off
*/
void sys_pm_poweroff(void)
{
	unsigned int key;
	/* wait 10ms, avoid trigger onoff wakeup pending */
	k_busy_wait(10000);
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif
	sys_pm_set_wakeup_src();

	printk("system power down!WKEN_CTL=0x%x\n", sys_read32(WKEN_CTL_SVCC));

	key = irq_lock();
#ifdef CONFIG_PM_DEVICE
	printk("dev power off\n");
	pm_power_off_devices();
	printk("dev power end\n");
#endif

//#if (CONFIG_PM_BACKUP_TIME_FUNCTION_EN == 1)
#if 0
	sys_pm_poweroff_backup_time();
#endif

	while(1) {
		sys_write32(2, POWER_CTL_SVCC);
		/* wait 10ms */
		k_busy_wait(10000);
		printk("poweroff fail, need reboot!\n");
		sys_pm_reboot(0);
	}

	/* never return... */
}


void sys_pm_reboot(int type)
{
	unsigned int key;

#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif
	if(type == REBOOT_TYPE_GOTO_SWJTAG){
		printk("set jtag flag\n");
		type = REBOOT_TYPE_NORMAL;
		sys_set_bit(RTC_REMAIN2, 0); //bit 0 adfu flag
	}
	printk("system reboot, type 0x%x!\n", type);

	key = irq_lock();
	/* store reboot reason in RTC_REMAIN0 for bootloader */
	sys_write32((REBOOT_REASON_MAGIC << 16) | (type & 0xffff), RTC_REMAIN3);
	k_busy_wait(500);
	sys_write32(0x5f, WD_CTL);
	while (1) {
		;
	}

	/* never return... */
}

int sys_pm_get_reboot_reason(u16_t *reboot_type, u8_t *reason)
{

	uint32_t reg_val;
	reg_val = soc_boot_get_reboot_reason();
	*reboot_type = (reg_val >> 16) &  0xff;
	*reason      =  reg_val & 0xff;
	return 0;

}

static void  ctk_close(void)
{
	int i;
	for(i = 0; i < 20; i++) // tpkey poweroff
	{
		sys_write32(0x40000009, CTK_CTL);
		sys_write32(0x4000000d, CTK_CTL);
	}
}

__sleepfunc void soc_udelay(uint32_t us)
{
	uint32_t cycles_per_1us, wait_cycles;
	volatile uint32_t i;
	uint8_t cpuclk_src = sys_read32(CMU_SYSCLK) & 0x7;

	if (cpuclk_src == 0)
		cycles_per_1us = 4;
	else if (cpuclk_src == 1)
		cycles_per_1us = 25;
	else if (cpuclk_src == 3)
		cycles_per_1us = 56; /* %15 deviation */
	else
		cycles_per_1us = 74;

	wait_cycles = cycles_per_1us * us / 10;

	for (i = 0; i < wait_cycles; i++) { /* totally 13 instruction cycles */
		;
	}
}

#ifndef CONFIG_SOC_EP_MODE
static int soc_pm_init(const struct device *arg)
{

	printk("WAKE_PD_SVCC = 0x%x\n", sys_read32(WAKE_PD_SVCC));
	
	sys_write32(0x3, CMU_S1CLKCTL); // hosc+rc4m
	sys_write32(0x3, CMU_S1BTCLKCTL); // hosc+rc4m
	sys_write32(0x9, CMU_S3CLKCTL); // S3 colse hosc and RC4M, enable RAM4 CLK SPIMT ICMT CLK ENABLE
	sys_write32(0x0, CMU_PMUWKUPCLKCTL); //select wk clk RC32K, if sel RC4M/4 ,must enable RC4M IN sleep
	sys_write32(0x1, CMU_GPIOCLKCTL); //select gpio clk RC4M

	sys_write32(0x06202053, VOUT_CTL1_S3);//VDD=0.7, vdd1.2=0.7
	sys_write32(0x5958, RC4M_CTL);

	ctk_close();
	return 0;
}
#else
static int soc_pm_init(const struct device *arg)
{
	uint32_t tmp;
	printk("WAKE_PD_SVCC = 0x%x\n", sys_read32(WAKE_PD_SVCC));

	sys_write32(0x01648014, HOSCLDO_CTL);
	sys_write32((sys_read32(HOSC_CTL)&(~(0xffff)))|0x7730, HOSC_CTL);

	//enable RC64M_S1/HOSC_S1/RC4M_S1    
	sys_write32(0x7, CMU_S1CLKCTL);
	//enable RC64M_S1BT/HOSC_S1BT/RC4M_S1BT
	sys_write32(0x7, CMU_S1BTCLKCTL);

	sys_write32(0x1, CMU_S3CLKCTL); // S3 colse RC4M
	sys_write32(0x0, CMU_PMUWKUPCLKCTL); //select wk clk rc4m
	sys_write32(0x06a08663, VOUT_CTL1_S3);//VDD=0.7, vd1.2=1.0

	sys_write32(0x8020140C, RC64M_CTL);
	sys_write32(0x8020148d, RC128M_CTL);
	sys_write32(0x58, RC4M_CTL);
	sys_write32(0x58, RC32K_CTL);

	//avdd output 1.1v
	sys_write32((sys_read32(AVDDLDO_CTL)&(~(0xf<<2)))|(5<<2)|(1<<0), AVDDLDO_CTL);
	//enable spll
	sys_write32(sys_read32(SPLL_CTL) | (1<<0), SPLL_CTL);

	/* calibrate CK64M clock */
	sys_write32(0x8020140C, RC64M_CTL);
	/* wait calibration done */
	while(!(sys_read32(RC64M_CTL) & (1 << 24)))
	{
		;
	}

	tmp = sys_read32(RC64M_CTL) >> 25;
	tmp -= 5;

	sys_write32(0x80201000|(tmp<<4), RC64M_CTL);
	printk("RC64M_CTL %x %x\n",sys_read32(RC64M_CTL), tmp);

#ifdef COREPLL_FIX_FREQ
    //fix corepll freq
	clk_rate_set_corepll(COREPLL_FIX_FREQ);

	clk_set_rate(CLOCK_ID_SPI0, SPI0_FIX_CLK*1000*1000);
#endif

	//init set cpu freq to 32MHz
	soc_freq_set_cpu_clk(32);

#ifdef COREPLL_FIX_FREQ
	//close RC128M
	sys_write32(sys_read32(RC128M_CTL)&(~1), RC128M_CTL);

	/* disable S1 CK64M */
	sys_write32(sys_read32(CMU_S1CLKCTL) & (~(1 << 2)), CMU_S1CLKCTL);
	/* disable S1BT CK64M */
	sys_write32(sys_read32(CMU_S1BTCLKCTL) & (~(1 << 2)), CMU_S1BTCLKCTL);

	printk("RC128M_CTL: 0x%x\n", sys_read32(RC128M_CTL));	
	printk("CMU_S1CLKCTL: 0x%x\n", sys_read32(CMU_S1CLKCTL));	
	printk("CMU_S1BTCLKCTL: 0x%x\n", sys_read32(CMU_S1BTCLKCTL));	
#endif

	ctk_close();
	return 0;
}
#endif

SYS_INIT(soc_pm_init, PRE_KERNEL_1, 20);

