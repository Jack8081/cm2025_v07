/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions Cuckoo family PMUVDD and PMUSVCC Implementation
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include <soc_pmu.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(pmu0, CONFIG_LOG_DEFAULT_LEVEL);

/* PMU_INTMASK */
#define PMU_INTMASK_TIMER_INTEN                     BIT(10)
#define PMU_INTMASK_WIO1LV_INTEN                         BIT(6)
#define PMU_INTMASK_BATLV_INTEN                          BIT(5)
#define PMU_INTMASK_DC5VLV_INTEN                         BIT(4)
#define PMU_INTMASK_DC5VOUT_INTEN                        BIT(3)
#define PMU_INTMASK_DC5VIN_INTEN                         BIT(2)
#define PMU_INTMASK_ONOFF_S_INTEN                        BIT(1)
#define PMU_INTMASK_ONOFF_L_INTEN                        BIT(0)

/* WAKE_CTL_SVCC */
#define WAKE_CTL_SVCC_BATLV_VOL_SHIFT                    (29) /* Battery voltage level wakeup threshold */
#define WAKE_CTL_SVCC_BATLV_VOL_MASK                     (0x7 << WAKE_CTL_SVCC_BATLV_VOL_SHIFT)
#define WAKE_CTL_SVCC_DC5VLV_VOL_SHIFT                   (26) /* DC5V voltage level wakeup threshold */
#define WAKE_CTL_SVCC_DC5VLV_VOL_MASK                    (0x7 << WAKE_CTL_SVCC_DC5VLV_VOL_SHIFT)
#define WAKE_CTL_SVCC_WIO1LV_VOL_SHIFT                   (21)
#define WAKE_CTL_SVCC_WIO1LV_VOL_MASK                    (0x3 << WAKE_CTL_SVCC_WIO1LV_VOL_SHIFT)

#define WAKE_CTL_SVCC_TIMER_WKEN                         BIT(10)
#define WAKE_CTL_SVCC_WIO2_WKEN                          BIT(9)
#define WAKE_CTL_SVCC_WIO1_WKEN                          BIT(8)
#define WAKE_CTL_SVCC_WIO0_WKEN                          BIT(7)
#define WAKE_CTL_SVCC_WIO1LV_WKEN                        BIT(6)
#define WAKE_CTL_SVCC_BATLV_WKEN                         BIT(5)
#define WAKE_CTL_SVCC_DC5VLV_WKEN                        BIT(4)
#define WAKE_CTL_SVCC_DC5VOUT_WKEN                       BIT(3)
#define WAKE_CTL_SVCC_DC5VIN_WKEN                        BIT(2)
#define WAKE_CTL_SVCC_SHORT_WKEN                         BIT(1)
#define WAKE_CTL_SVCC_LONG_WKEN                          BIT(0)

/* WAKE_PD_SVCC */
#define WAKE_PD_SVCC_REG_SVCC_SHIFT                      (25)
#define WAKE_PD_SVCC_REG_SVCC_MASK                       (0x7F << WAKE_PD_SVCC_REG_SVCC_SHIFT)
#define WAKE_PD_SVCC_ONOFF_PATRST_PD                         BIT(24)
#define WAKE_PD_SVCC_DC5VPAT_RST_PD                         BIT(23)
#define WAKE_PD_SVCC_DC5VIN_RST_PD                         BIT(22)
#define WAKE_PD_SVCC_ONOFF_8S_RST                          BIT(21)
#define WAKE_PD_SVCC_WD_RST_PD                             BIT(20)
#define WAKE_PD_SVCC_PRST_PD                               BIT(19)
#define WAKE_PD_SVCC_POWEROK_PD                            BIT(18)
#define WAKE_PD_SVCC_LB_PD                                 BIT(17)
#define WAKE_PD_SVCC_OC_PD                                 BIT(16)
#define WAKE_PD_SVCC_LVPRO_PD                              BIT(15)
#define WAKE_PD_SVCC_VDDOKA_PD                             BIT(14)
#define WAKE_PD_SVCC_RESTART_PD                            BIT(13)
#define WAKE_PD_SVCC_CPU_RST_PD                            BIT(12)
#define WAKE_PD_SVCC_WIO2_RST_PD                           BIT(11)
#define WAKE_PD_SVCC_TIMER_PD                              BIT(10)
#define WAKE_PD_SVCC_WIO2_PD                               BIT(9)
#define WAKE_PD_SVCC_WIO1_PD                               BIT(8)
#define WAKE_PD_SVCC_WIO0_PD                               BIT(7)
#define WAKE_PD_SVCC_WIO1LV_PD                             BIT(6)
#define WAKE_PD_SVCC_BATLV_PD                              BIT(5)
#define WAKE_PD_SVCC_DC5VLV_PD                             BIT(4)
#define WAKE_PD_SVCC_DC5VOUT_PD                            BIT(3)
#define WAKE_PD_SVCC_DC5VIN_PD                             BIT(2)
#define WAKE_PD_SVCC_ONOFF_S_PD                            BIT(1)
#define WAKE_PD_SVCC_ONOFF_L_PD                            BIT(0)

/* SYSTEM_SET_SVCC */
#define SYSTEM_SET_SVCC_S4ICTL BIT(29)
#define SYSTEM_SET_SVCC_DC5VPAT_RST_EN BIT(28)
#define SYSTEM_SET_SVCC_DC5VIN_RST_DELAY BIT(27)
#define SYSTEM_SET_SVCC_DC5VIN_RST_EN BIT(26)
#define SYSTEM_SET_SVCC_SIM_A_EN                         BIT(25)
#define SYSTEM_SET_SVCC_ONOFF_PRESS_TIME_SHIFT           (21)
#define SYSTEM_SET_SVCC_ONOFF_PRESS_TIME_MASK            (0x7 << SYSTEM_SET_SVCC_ONOFF_PRESS_TIME_SHIFT)
#define SYSTEM_SET_SVCC_ONOFF_PRESS_TIME(x)              ((x) << SYSTEM_SET_SVCC_ONOFF_PRESS_TIME_SHIFT)
#define SYSTEM_SET_SVCC_ONOFF_RST_EN_SHIFT               (19)
#define SYSTEM_SET_SVCC_ONOFF_RST_EN_MASK                (0x3 << SYSTEM_SET_SVCC_ONOFF_RST_EN_SHIFT)
#define SYSTEM_SET_SVCC_ONOFF_RST_EN(x)                  ((x) << SYSTEM_SET_SVCC_ONOFF_RST_EN_SHIFT)
#define SYSTEM_SET_SVCC_ONOFF_RST_TIME                   BIT(16)
#define SYSTEM_SET_SVCC_ONOFF_PRES                       BIT(15)

/* CHG_CTL_SVCC */
#define CHG_CTL_SVCC_CV_3V3                              BIT(19)
#define CHG_CTL_SVCC_CV_OFFSET_SHIFT                     (5)
#define CHG_CTL_SVCC_CV_OFFSET_MASK                      (0x1F << CHG_CTL_SVCC_CV_OFFSET_SHIFT)
#define CHG_CTL_SVCC_CV_OFFSET(x)                        ((x) << CHG_CTL_SVCC_CV_OFFSET_SHIFT)

/* PMU_DET */
#define PMU_DET_S1MAIN_REQ                   			 BIT(9)
#define PMU_DET_S1BT_REQ                     			 BIT(8)
#define PMU_DET_CHGPHASE_SHIFT		                     (4)
#define PMU_DET_CHGPHASE_MASK                      		 (0x3 << PMU_DET_CHGPHASE_SHIFT)
#define PMU_DET_CHCURAD_SHIFT		                     (1)
#define PMU_DET_CHCURAD_MASK                      		 (0x7 << PMU_DET_CHCURAD_SHIFT)
#define PMU_DET_DC5VIN_DET                               BIT(0)


#define PMU_MONITOR_DEV_INVALID(x) (((x) != PMU_DETECT_DEV_DC5V) \
			&& ((x) != PMU_DETECT_DEV_REMOTE) \
			&& ((x) != PMU_DETECT_DEV_ONOFF))

/*
 * @struct acts_pmuvdd
 * @brief Actions PMUVDD controller hardware register
 */
struct acts_pmuvdd {
	volatile uint32_t vout_ctl0;
	volatile uint32_t vout_ctl1_s1;
	volatile uint32_t vout_ctl1_s3;
	volatile uint32_t pmu_det;
	volatile uint32_t dcdc_vc18_ctl;
	volatile uint32_t dcdc_vd12_ctl;
	volatile uint32_t pwrgate_dig;
	volatile uint32_t pwrgate_dig_ack;
	volatile uint32_t pwrgate_ram;
	volatile uint32_t pwrgate_ram_ack;
	volatile uint32_t pmu_intmask;
};

/*
 * @struct acts_pmusvcc
 * @brief Actions PMUSVCC controller hardware register
 */
struct acts_pmusvcc {
	volatile uint32_t chg_ctl_svcc;
	volatile uint32_t bdg_ctl_svcc;
	volatile uint32_t system_set_svcc;
	volatile uint32_t power_ctl_svcc;
	volatile uint32_t wake_ctl_svcc;
	volatile uint32_t wake_pd_svcc;
};

/**
 * struct pmu_drv_data
 * @brief The meta data which related to Actions PMU module.
 */
struct pmu_context_t {
	struct detect_param_t detect_devs[PMU_DETECT_MAX_DEV]; /* PMU monitor peripheral devices */
	uint32_t pmuvdd_base; /* PMUVDD register base address */
	uint32_t pmusvcc_base; /* PMUSVCC register base address */
	uint8_t onoff_short_detect : 1; /* if 1 to enable onoff key short press detection */
	uint8_t onoff_remote_same_wio : 1; /* if 1 to indicate that onoff key and remote key use the same WIO source */
};

/* @brief get the PMU management context handler */
static inline struct pmu_context_t *get_pmu_context(void)
{
	static struct pmu_context_t pmu_context = {0};
	static struct pmu_context_t *p_pmu_context = NULL;

	if (!p_pmu_context) {
		pmu_context.pmuvdd_base = PMUVDD_BASE;
		pmu_context.pmusvcc_base = CHG_CTL_SVCC;
		pmu_context.onoff_short_detect = CONFIG_PMU_ONOFF_SHORT_DETECT;
#if 0		
		pmu_context.onoff_remote_same_wio = CONFIG_PMU_ONOFF_REMOTE_SAME_WIO;
#else
		pmu_context.onoff_remote_same_wio = 0;		
#endif
		p_pmu_context = &pmu_context;
	}

	return p_pmu_context;
}

/* @brief get the base address of PMUVDD register */
static inline struct acts_pmuvdd *get_pmuvdd_reg_base(struct pmu_context_t *ctx)
{
	return (struct acts_pmuvdd *)ctx->pmuvdd_base;
}

/* @brief get the base address of PMUSVCC register */
static inline struct acts_pmusvcc *get_pmusvcc_reg_base(struct pmu_context_t *ctx)
{
	return (struct acts_pmusvcc *)ctx->pmusvcc_base;
}

/* @brief find the monitor device parameters by given detect_dev */
static struct detect_param_t *soc_pmu_find_detect_dev(struct pmu_context_t *ctx, uint8_t detect_dev)
{
	uint8_t i;

	for (i = 0; i < PMU_DETECT_MAX_DEV; i++) {
		if (ctx->detect_devs[i].detect_dev == detect_dev) {
			return &ctx->detect_devs[i];
		}
		LOG_DBG("dev slot[%d]:%d", i, ctx->detect_devs[i].detect_dev);
	}

	return NULL;
}

/* @brief register the function will be called when the state of monitor device change */
int soc_pmu_register_notify(struct detect_param_t *param)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);
	uint8_t i;

	if (!param)
		return -EINVAL;

	if (PMU_MONITOR_DEV_INVALID(param->detect_dev)) {
		LOG_ERR("Invalid monitor dev:%d", param->detect_dev);
		return -EINVAL;
	}

	for (i = 0; i < PMU_DETECT_MAX_DEV; i++) {
		/* new or update a monitor device into PMU context */
		if ((!(ctx->detect_devs[i].detect_dev))
			|| (ctx->detect_devs[i].detect_dev == param->detect_dev)) {
			ctx->detect_devs[i].detect_dev = param->detect_dev;
			ctx->detect_devs[i].cb_data = param->cb_data;
			ctx->detect_devs[i].notify = param->notify;
			break;
		} else {
			LOG_DBG("Busy slot[%d]:%d", i, ctx->detect_devs[i].detect_dev);
		}
	}

	if (i == PMU_DETECT_MAX_DEV) {
		LOG_ERR("no space for dev:0x%x", param->detect_dev);
		return -ENOMEM;
	}

	/* enable detect device interrupt and DC5V is already enabled at the stage of initialization */
	if (PMU_DETECT_DEV_ONOFF == param->detect_dev) {
		if (ctx->onoff_short_detect) {
			pmusvcc_reg->wake_ctl_svcc |= (WAKE_CTL_SVCC_SHORT_WKEN | WAKE_CTL_SVCC_LONG_WKEN);
			pmuvdd_reg->pmu_intmask |= (PMU_INTMASK_ONOFF_S_INTEN | PMU_INTMASK_ONOFF_L_INTEN);
		} else {
			pmusvcc_reg->wake_ctl_svcc |= WAKE_CTL_SVCC_LONG_WKEN;
			pmuvdd_reg->pmu_intmask |= PMU_INTMASK_ONOFF_L_INTEN;
		}
	} else if (PMU_DETECT_DEV_REMOTE == param->detect_dev) {
		pmusvcc_reg->wake_ctl_svcc |= WAKE_CTL_SVCC_WIO1LV_WKEN;
		pmuvdd_reg->pmu_intmask |= PMU_INTMASK_WIO1LV_INTEN;
	} else {
		;	//empty
	}

	LOG_DBG("pmu register dev:%d", param->detect_dev);

	return 0;
}

/* @brief unregister the notify function for specified monitor device */
void soc_pmu_unregister_notify(uint8_t detect_dev)
{
	struct pmu_context_t *ctx = get_pmu_context();
	uint8_t i;
	uint32_t key;

	for (i = 0; i < PMU_DETECT_MAX_DEV; i++) {
		if (ctx->detect_devs[i].detect_dev == detect_dev) {
			key = irq_lock();
			ctx->detect_devs[i].detect_dev = 0;
			ctx->detect_devs[i].notify = 0;
			ctx->detect_devs[i].cb_data = NULL;
			irq_unlock(key);
		}
	}
}

/* @brief get the current DC5V status of plug in or out */
bool soc_pmu_get_dc5v_status(void)
{
	bool status;
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);

	/* DC5VIN indicator, 0: DC5V < BAT +33mV; 1: DC5V > BAT + 80mV */
	if (pmuvdd_reg->pmu_det & PMU_DET_DC5VIN_DET)
		status = true;
	else
		status = false;

	return status;
}

/* @brief return the wakeup pending by system startup */
uint32_t soc_pmu_get_wakeup_source(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	static uint32_t pmu_wakeup_pending;

	if (!pmu_wakeup_pending) {
		pmu_wakeup_pending = pmusvcc_reg->wake_pd_svcc;
		LOG_INF("pmu wakeup pending:0x%x", pmu_wakeup_pending);
	}

	return pmu_wakeup_pending;
}

/* @brief return the wakeup setting by system startup */
uint32_t soc_pmu_get_wakeup_setting(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	static uint32_t pmu_wakeup_ctl;

	if (!pmu_wakeup_ctl) {
		pmu_wakeup_ctl = pmusvcc_reg->wake_ctl_svcc;
		LOG_INF("pmu wakeup setting:0x%x", pmu_wakeup_ctl);
	}

	return pmu_wakeup_ctl;
}

/* @brief set the max constant current */
void soc_pmu_set_max_current(uint16_t cur_ma)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	uint8_t level;
	uint16_t current_array[16] = {10, 20, 30, 40, 60, 80, 100, 120, 140, 150, 160, 180, 200, 220, 250, 300};

	for(level=15; level>=0; level--) {
		if(current_array[level] <= cur_ma) {
			break;
		}
	}
	
	pmusvcc_reg->chg_ctl_svcc = (pmusvcc_reg->chg_ctl_svcc & ~0xF) | level;
}

/* @brief get the max constant current */
uint16_t soc_pmu_get_max_current(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	uint8_t level;
	uint16_t cur_ma;
	uint16_t current_array[16] = {10, 20, 30, 40, 60, 80, 100, 120, 140, 150, 160, 180, 200, 220, 250, 300};

	level = pmusvcc_reg->chg_ctl_svcc & 0xF;

	cur_ma = current_array[level];
	
	return cur_ma;
}

/* @brief lock(stop) DC5V charging for reading battery voltage */
void soc_pmu_read_bat_lock(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	pmusvcc_reg->chg_ctl_svcc |= CHG_CTL_SVCC_CV_3V3;
	k_busy_wait(300);
}

/* @brief unlock(resume) and restart DC5V charging */
void soc_pmu_read_bat_unlock(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	pmusvcc_reg->chg_ctl_svcc &= ~CHG_CTL_SVCC_CV_3V3;
	k_busy_wait(300);
}

/* @brief set const voltage value */
void soc_pmu_set_const_voltage(uint8_t cv)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	if (cv <= 0x1F) {
		pmusvcc_reg->chg_ctl_svcc = \
			(pmusvcc_reg->chg_ctl_svcc & ~CHG_CTL_SVCC_CV_OFFSET_MASK) | CHG_CTL_SVCC_CV_OFFSET(cv);
		k_busy_wait(300);
	}
}

/* @brief configure the long press on-off key time */
void soc_pmu_config_onoffkey_time(uint8_t val)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
    uint32_t reg = pmusvcc_reg->system_set_svcc;
    /* always set short time press wake up for earphone that need detect boot hold key after power on */
    #ifdef CONFIG_SOC_EP_MODE
        val = 0;
    #endif

	if (val > 7)
		return ;

	reg &= ~SYSTEM_SET_SVCC_ONOFF_PRESS_TIME_MASK;
	reg |= SYSTEM_SET_SVCC_ONOFF_PRESS_TIME(val);
    pmusvcc_reg->system_set_svcc = reg;

	k_busy_wait(300);
}

/* @brief configure the long press on-off key time */
void soc_pmu_config_onoffkey_function(uint8_t val)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
    uint32_t reg = pmusvcc_reg->system_set_svcc;

	/**
	 * ONOFF long pressed function
	 * 0: no function
	 * 1: reset
	 * 2: restart (S1 => S4 => S1)
	 */
	if (val > 2)
		return ;

	reg &= ~SYSTEM_SET_SVCC_ONOFF_RST_EN_MASK;
	reg |= SYSTEM_SET_SVCC_ONOFF_RST_EN(val);
    pmusvcc_reg->system_set_svcc = reg;

	k_busy_wait(300);
}

/* @brief check if the onoff key has been pressed or not */
bool soc_pmu_is_onoff_key_pressed(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	/* if 1 indicates that onoff key has been pressed */
	if (pmusvcc_reg->system_set_svcc & SYSTEM_SET_SVCC_ONOFF_PRES) {
		return true;
	}
	
	return false;
}

/* @brief configure the long press on-off key reset/restart time */
void soc_pmu_config_onoffkey_reset_time(uint8_t val)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	if (val) {
		pmusvcc_reg->system_set_svcc |= SYSTEM_SET_SVCC_ONOFF_RST_TIME; /* 16s */
	} else {
		pmusvcc_reg->system_set_svcc &= ~SYSTEM_SET_SVCC_ONOFF_RST_TIME; /* 8s */
	}

	k_busy_wait(300);
}

/* @brief configure the  on-off key pullr_sel  */
void soc_pmu_config_onoffkey_pullr_sel(uint8_t val)
{
}


/* pmu isr */
static void soc_pmu_isr(void *arg)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	//struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);
	struct detect_param_t *detect_param = NULL;

	ARG_UNUSED(arg);

	uint32_t pending = pmusvcc_reg->wake_pd_svcc;

	LOG_DBG("pmu pending:0x%x", pending);

	if (pending & WAKE_PD_SVCC_WIO1LV_PD) {
		//remote key pressed
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_REMOTE);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_PRESSED);
	} else if (pending & WAKE_PD_SVCC_ONOFF_L_PD) {
		//onoff long pressed
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_ONOFF);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_LONG_PRESSED);
	} else if (pending & WAKE_PD_SVCC_ONOFF_S_PD) {
		//onoff short pressed
		if (ctx->onoff_short_detect) {
			detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_ONOFF);
			if (detect_param)
				detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_PRESSED);
		}
	} else {
		;	//empty
	}

	if (pending & WAKE_PD_SVCC_DC5VOUT_PD) {
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_DC5V);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_OUT);
	}

	if (pending & WAKE_PD_SVCC_DC5VIN_PD) {
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_DC5V);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_IN);
	}

	pmusvcc_reg->wake_pd_svcc = pending;
}

static void soc_pmu_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_PMU, CONFIG_PMU_IRQ_PRI,
			soc_pmu_isr,
			NULL, 0);
	irq_enable(IRQ_ID_PMU);
}

#if 1
/*return rc32k freq*/
static uint32_t acts_clock_rc32k_set_cal_cyc(uint32_t cal_cyc)
{
	uint32_t cnt;
	uint64_t tmp = 32000000;
	
	sys_write32(0, RC32K_CAL);
	//k_busy_wait(10);
	for(cnt=0; cnt<0x4000; cnt++);
	
    sys_write32((cal_cyc << 8)|1, RC32K_CAL);

    /* wait calibration done */
    while (!(sys_read32(RC32K_CAL) & (1 << 4))){}

	cnt =  sys_read32(RC32K_COUNT);
	if(cnt){
		cnt = tmp*cal_cyc/cnt;
	}
	LOG_INF("set freq=%d, ctl=0x%x\n", cnt, sys_read32(RC32K_CTL)>>1);
	return cnt;
}

static uint32_t g_rc32_mul;

/* calibrate RC32K */
static void acts_clock_rc32k_calibrate(void)
{
	uint32_t freq;
	uint32_t freqs, freqe, setf; 
	uint32_t wait_cnt;

	freq = acts_clock_rc32k_set_cal_cyc(100);
	LOG_DBG("cur freq=%d\n", freq);
	if(freq < 32768){
		freqs = 44; 
		freqe = 63;
	}else{
		freqs = 10; 
		freqe = 44;
	}
	while(freqs < (freqe-1)){
 		setf = (freqs + freqe)/2;
		sys_write32((setf << 1), RC32K_CTL);
		//k_busy_wait(10);
		for(wait_cnt=0; wait_cnt<0x4000; wait_cnt++);
		freq = acts_clock_rc32k_set_cal_cyc(100);
		if(freq < 32768){
			freqs = setf;
		}else{
			freqe = setf;
		}
	}

	if(freq) {
		g_rc32_mul = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC/freq);
	} else {
		g_rc32_mul = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC/32768);
	}
	
	LOG_INF("calibrate freq=%d, mul=%d, ctl=0x%x\n", freq, g_rc32_mul, sys_read32(RC32K_CTL) >> 1);
}

uint32_t soc_rc32K_mutiple_hosc(void)
{
	return g_rc32_mul;
}

#endif

uint32_t soc_pmu_get_vd12_voltage(void)
{
	uint32_t sel, volt_mv;

	sel = (sys_read32(VOUT_CTL1_S1) & (0xf << 8)) >> 8;

	volt_mv = 700 + sel * 50;

	return volt_mv;
}

void soc_pmu_set_vd12_voltage(uint32_t volt_mv)
{
	unsigned int sel;

	if (volt_mv < 700 || volt_mv > 1450) {
		return ;
	}
	
	sel = (volt_mv - 700) / 50;
	sys_write32((sys_read32(VOUT_CTL1_S1) & ~(0xf << 8)) | (sel << 8), VOUT_CTL1_S1);

	k_busy_wait(50);	

	LOG_INF("set vd12: %dmv\n", soc_pmu_get_vd12_voltage());
}

uint32_t soc_pmu_get_vdd_voltage(void)
{
	uint32_t sel, volt_mv;

	sel = sys_read32(VOUT_CTL1_S1) & 0xf;

	volt_mv = 550 + sel * 50;

	return volt_mv;
}

void soc_pmu_set_vdd_voltage(uint32_t volt_mv)
{
	unsigned int sel;
	uint32_t cur_vd12_mv, new_vd12_mv;
	uint32_t dsp_max_freq;
	uint32_t cur_vdd_mv;
	uint32_t delay_us_val;
		
	if (volt_mv < 550 || volt_mv > 1300) {
		return ;
	}

	cur_vd12_mv = soc_pmu_get_vd12_voltage();
	dsp_max_freq = soc_freq_get_target_dsp_freq_mhz();
	
	if((volt_mv >= 1200) || (dsp_max_freq > 120)) {
		new_vd12_mv = 1450;
	} else {
		new_vd12_mv = 1400;
	}

	if(new_vd12_mv != cur_vd12_mv) {
		soc_pmu_set_vd12_voltage(new_vd12_mv);
	}

	cur_vdd_mv = soc_pmu_get_vdd_voltage();
	if(volt_mv >= cur_vdd_mv) {
		delay_us_val = (volt_mv - cur_vdd_mv) / 10;
	} else {
		delay_us_val = (cur_vdd_mv - volt_mv) / 10;
	}

    if(volt_mv != cur_vdd_mv) {
		sel = (volt_mv - 550) / 50;
		sys_write32((sys_read32(VOUT_CTL1_S1) & ~(0xf)) | (sel), VOUT_CTL1_S1);
    }

	k_busy_wait(delay_us_val);
}

/* @brief PMUVDD and PMUSVCC initialization */
static int soc_pmu_init(const struct device *dev)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);

	ARG_UNUSED(dev);

	soc_pmu_get_wakeup_source();

	/* by default to enable DC5V plug in/out detection */
	pmusvcc_reg->wake_ctl_svcc |= (WAKE_CTL_SVCC_DC5VOUT_WKEN | WAKE_CTL_SVCC_DC5VIN_WKEN);
	pmuvdd_reg->pmu_intmask |= (PMU_INTMASK_DC5VIN_INTEN | PMU_INTMASK_DC5VOUT_INTEN);

	if (ctx->onoff_short_detect) {
		pmusvcc_reg->wake_ctl_svcc |= WAKE_CTL_SVCC_SHORT_WKEN;
		pmuvdd_reg->pmu_intmask |= PMU_INTMASK_ONOFF_S_INTEN;
	}

	pmusvcc_reg->wake_pd_svcc = pmusvcc_reg->wake_pd_svcc;

#if 0
	if (!ctx->onoff_remote_same_wio)
		sys_write32(0, WIO0_CTL);
#endif

	soc_pmu_irq_config();

	acts_clock_rc32k_calibrate();

#if (!CONFIG_ACTS_BATTERY_SUPPLY_INTERNAL) && (!CONFIG_ACTS_BATTERY_SUPPLY_EXTERNAL) && (!CONFIG_ACTS_BATTERY_ONLY_BATTERY_SAMPLE)
    sys_write32((sys_read32(CHG_CTL_SVCC) & 0xfffffff0) | 0xf | (1 << 29), CHG_CTL_SVCC);
    k_busy_wait(300);
#else 
    //disable DC5V bypass to BAT
    sys_write32((sys_read32(CHG_CTL_SVCC) & (~(1 << 29))), CHG_CTL_SVCC);
    k_busy_wait(300);
	printk("disable 5V to BAT: 0x%x\n", sys_read32(CHG_CTL_SVCC));
#endif
	return 0;
}

SYS_INIT(soc_pmu_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

