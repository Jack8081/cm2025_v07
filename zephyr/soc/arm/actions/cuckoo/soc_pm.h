/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file reboot configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_PM_H_
#define	_ACTIONS_SOC_PM_H_

#define COREPLL_FIX_FREQ (168)

#ifdef COREPLL_FIX_FREQ
#define SPI0_FIX_CLK (68)
#endif

//reboot type, mask is 0xf00, low byte for details
//reboot reason, mask is 0xff, low byte for details
#define REBOOT_TYPE_NORMAL			0x000
#define REBOOT_TYPE_GOTO_ADFU		0x100
#define REBOOT_TYPE_GOTO_SYSTEM		0x200
#define REBOOT_TYPE_GOTO_RECOVERY	0x300
#define REBOOT_TYPE_GOTO_BTSYS		0x400
#define REBOOT_TYPE_GOTO_WIFISYS	0x500
#define REBOOT_TYPE_GOTO_SWJTAG		0x600


#define     WAKE_CTL_BATLV_VOL_e                                         31
#define     WAKE_CTL_BATLV_VOL_SHIFT                                     29
#define     WAKE_CTL_BATLV_VOL_MASK                                      (0x7<<29)
#define     WAKE_CTL_DC5VLV_VOL_e                                        28
#define     WAKE_CTL_DC5VLV_VOL_SHIFT                                    26
#define     WAKE_CTL_DC5VLV_VOL_MASK                                     (0x7<<26)

#define     WAKE_CTL_WIO1LV_VOL_e                                        22
#define     WAKE_CTL_WIO1LV_VOL_SHIFT                                    21
#define     WAKE_CTL_WIO1LV_VOL_MASK                                     (0x3<<21)
#define     WAKE_CTL_WIO1LV_DETEN                                        (1<<20)

#define     WAKE_CTL_TIMER_CNT_e                                         19
#define     WAKE_CTL_TIMER_CNT_SHIFT                                     11
#define     WAKE_CTL_TIMER_CNT_MASK                                     (0x1ff<<11)

#define     WAKE_CTL_TIMER_WKEN                                          (1<<10)
#define     WAKE_CTL_WIO2_WKEN                                           (1<<9)
#define     WAKE_CTL_WIO1_WKEN                                           (1<<8)
#define     WAKE_CTL_WIO0_WKEN                                           (1<<7)
#define     WAKE_CTL_WIO1LV_WKEN                                         (1<<6)
#define     WAKE_CTL_BATLV_WKEN                                          (1<<5)
#define     WAKE_CTL_DC5VLV_WKEN                                         (1<<4)
#define     WAKE_CTL_DC5VOUT_WKEN                                        (1<<3)
#define     WAKE_CTL_DC5VIN_WKEN                                         (1<<2)
#define     WAKE_CTL_SHORT_WKEN                                          (1<<1)
#define     WAKE_CTL_LONG_WKEN                                           (1<<0)

union sys_pm_wakeup_src {
	uint32_t data;
	struct {
		uint32_t long_onoff : 1; /* ONOFF key long pressed wakeup */
		uint32_t short_onoff : 1; /* ONOFF key short pressed wakeup */
		uint32_t bat : 1; /* battery plug in wakeup */
		uint32_t alarm : 1; /* RTC alarm wakeup */
		uint32_t wio0 : 1; /* WIO0 wakeup */
		uint32_t wio1 : 1; /* WIO1 wakeup */
		uint32_t wio2 : 1; /* WIO2 wakeup */
		uint32_t remote : 1; /* remote wakeup */
		uint32_t batlv : 1; /*battery low power*/
		uint32_t dc5vlv : 1; /*dc5v low voltage*/
		uint32_t dc5vin : 1; /*dc5v in wakeup*/
		uint32_t watchdog : 1; /* watchdog reboot */
		uint32_t wio1lv  : 1; /* wio1 low voltage */
	} t;
};

struct sys_pm_backup_time {
	uint32_t rtc_time_sec;
	uint32_t rtc_time_msec;
	uint32_t counter8hz_msec;
	uint8_t is_backup_time_valid;
};

void sys_pm_reboot(int type);
int sys_pm_get_reboot_reason(u16_t *reboot_type, u8_t *reason);
int sys_pm_get_wakeup_source(union sys_pm_wakeup_src *src);
void sys_pm_set_dc5v_low_wakeup(int enable, int threshold);
int sys_pm_get_dc5v_low_threshold(uint8_t *volt);
void sys_pm_set_bat_low_wakeup(int enable, int threshold);
int sys_pm_get_bat_low_threshold(uint8_t *volt);
void sys_pm_set_onoff_wakeup(int enable);
void sys_pm_poweroff(void);


/*soc_sleep.c*/
void soc_enter_deep_sleep(void);
void soc_enter_light_sleep(void);
void soc_udelay(uint32_t us);

/*soc_pm.c*/

void sys_pm_enter_deep_sleep(void);
void sys_pm_enter_light_sleep(void);


#endif /* _ACTIONS_SOC_PM_H_	*/
