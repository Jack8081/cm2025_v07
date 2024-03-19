/**
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio DAC physical channel implementation
 */

/*
 * Features:
 *    - Build in a 24 bits input sigma-delta DAC.
 *    - 16 * 2 * 24 bits FIFO.
 *    - Support digital volume with zero cross detection.
 *    - Sample rate support 8k/12k/11.025k/16k/22.05k/24k/32k/44.1k/48k/88.2k/96k.
 *	- Support antipop to restrain noise.
 */

/*
 * Signal List
 * 	- AVCC: Analog power
 *	- AGND: Analog ground
 *	- PAGND: Ground for PA
 *	- AOUTL/AOUTLP: Left output of PA / Left Positive output of PA
 *	- AOUTR/AOUTLN: Right output of PA / Left Negative output of PA
 *	- AOUTRP/VRO: Right Positive output of PA / Virtual Ground for PA
 *	- AOUTRN/VROS: Right Negative output of PA / VRO Sense for PA
 */

#include <kernel.h>
#include <device.h>
#include <ksched.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include "../phy_audio_common.h"
#include "audio_acts_utils.h"
#include "phy_audio.h"
#include <drivers/audio/audio_out.h>
#include <dvfs.h>
#include <soc_atp.h>


#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#include <drivers/gpio.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(dac0, CONFIG_LOG_DEFAULT_LEVEL);


#define VOL_LCH_SOFT_CFG_DEFAULT                               (VOL_LCH_VOLLZCEN | VOL_LCH_VOLLZCTOEN | VOL_LCH_SOFT_STEP_EN)
#define VOL_RCH_SOFT_CFG_DEFAULT                               (VOL_RCH_VOLRZCEN | VOL_RCH_VOLRZCTOEN | VOL_RCH_SOFT_STEP_EN)

/***************************************************************************************************
 * DAC FEATURES CONGIURATION
 */
#define DAC_FIFO_MAX_DRQ_LEVEL                                 (0xE)
#define DAC_FIFO_DRQ_LEVEL_DEFAULT                             (0x7) /* 16 level */

#define DAC_FIFO_MAX_VOL_LEVEL                                 (0xF)
#define DAC_FIFO_VOL_LEVEL_DEFAULT                             (0x3) /* 0db */

/* DAC volume soft step to_cnt default setting(0 : 8x; 1 : 128x).  */
#define DAC_VOL_TO_CNT_DEFAULT                                 (0)

/* The minimal volume value to mute automatically */
#define VOL_MUTE_MIN_DB                                        (-800000)

#define VOL_DB_TO_INDEX(x)                                     (((x) + 374) / 375)
#define VOL_INDEX_TO_DB(x)                                     ((x) * 375)

#define DAC_FIFO_INVALID_INDEX(x)                              ((x) != AOUT_FIFO_DAC0)

#define DAC_FIFO_MAX_LEVEL                                     (32)

#define DAC_WAIT_FIFO_EMPTY_TIMEOUT_MS                         (65) /* PCMBUF 2k samples spends 62.5ms in 16Kfs */

#define DAC_PCMBUF_MAX_CNT                                     (0x800)

#define DAC_PCMBUF_DEFAULT_IRQ                                 (PCM_BUF_CTL_PCMBHEIE | PCM_BUF_CTL_PCMBEPIE)

#define DAC_CHANNEL_NUM_MAX                                    (2)

#define DAC_FIFO_CNT_MAX_SAME_SAMPLES_TIME_US                  (100000)

#define DAC_FIFO_CNT_CLEAR_PENDING_TIME_US                     (200)

//#define DAC_ANALOG_DEBUG_IN_ENABLE

#ifdef CONFIG_SOC_SERIES_LARK_FPGA
#define DAC_DIGITAL_DEBUG_OUT_ENABLE
#endif

#define DAC_DIGITAL_DEBUG_OUT_CHANNEL_SEL                      (1) /* 1: debug left channel; others: debug right channel */

#define DAC_LDO_CAPACITOR_CHARGE_TIME_MS                       (10) /* Wait time for AOUT L/R capacitor charge full */

#define DAC_HIGH_PERFORMANCE_WAIT_SH_TIME_MS                   (3) /* Wait time for SH establish stable state */

/*
 * @struct acts_audio_dac
 * @brief DAC controller hardware register
 */
struct acts_audio_dac {
    volatile uint32_t digctl; /* DAC digital and control */
    volatile uint32_t fifoctl; /* DAC FIFO control */
    volatile uint32_t stat; /* DAC state */
    volatile uint32_t fifo0_dat; /* DAC FIFO0 data */
    volatile uint32_t pcm_buf_ctl; /* PCM buffer control */
    volatile uint32_t pcm_buf_stat; /* PCM buffer state */
    volatile uint32_t pcm_buf_thres_he; /* PCM buffer half-empty threshold */
    volatile uint32_t pcm_buf_thres_hf; /* PCM buffer half-full threshold */
    volatile uint32_t sdm_reset_ctl; /* SDM reset control */
    volatile uint32_t auto_mute_ctl; /* Auto mute control */
    volatile uint32_t vol_lch; /* volume left channel control */
    volatile uint32_t vol_rch; /* volume right channel control */
    volatile uint32_t pcm_buf_cnt; /* PCM buffer counter */
    volatile uint32_t anactl0; /* DAC analog control register 0 */
    volatile uint32_t anactl1; /* DAC analog control register 1 */
    volatile uint32_t anactl2; /* DAC analog control register 2 */
    volatile uint32_t bias; /* DAC bias control */
    volatile uint32_t sdm_samples_cnt; /* SDM samples counter */
    volatile uint32_t sdm_samples_num; /* SDM sample number */
    volatile uint32_t hw_trigger_dac_ctl; /* HW IRQ trigger DAC control */

    volatile uint32_t dc_ref_dat;    /* Left DC Reference Data */
    volatile uint32_t dac_osctl;     /* DAC offset Register */
    volatile uint32_t dc_ref_dat_r;  /* Right DC Reference Data */
};

struct phy_dac_channel {
    uint32_t fifo_cnt; /* DAC FIFO hardware counter max value is 0xFFFF */
    uint32_t fifo_cnt_timestamp; /* Record the timestamp of DAC FIFO counter overflow irq */
    int (*callback)(void *cb_data, u32_t reason); /* PCM Buffer IRQs callback */
    void *cb_data; /* callback user data */
};

#ifdef CONFIG_CFG_DRV
/**
 * struct phy_dac_external_config
 * @brief The DAC external configuration which generated by configuration tool
 */
struct phy_dac_external_config {
    cfg_uint8 Out_Mode; /* CFG_TYPE_AUDIO_OUT_MODE */
    cfg_uint32 DAC_Bias_Setting; /* DAC bias setting */
    cfg_uint8 Keep_DA_Enabled_When_Play_Pause; /* always enable DAC analog  */
    CFG_Type_Extern_PA_Control Extern_PA_Control[2]; /* GPIO pins to control external PA */
    cfg_uint8 AntiPOP_Process_Disable; /* forbidden antipop process */
    cfg_uint8 Enable_large_current_protect; /* enable large current protect */
    cfg_uint8 Pa_Vol; /* PA gain selection */
};
#endif


/**
 * struct phy_dac_drv_data
 * @brief The software related data that used by physical dac driver.
 */
struct phy_dac_drv_data {
    struct phy_dac_channel ch[DAC_CHANNEL_NUM_MAX]; /* dac channels infomation */
    uint32_t sdm_cnt; /* SDM samples counter */
    uint32_t sdm_cnt_timestamp; /* Record the timestamp of SDM counter by overflow irq */
    uint8_t sample_rate; /* The sample rate setting refer to enum audio_sr_sel_e */
    uint8_t lr_sel; /* left and right channel selection to enable, refer to enum a_lr_chl_e */
    uint8_t layout; /* DAC hardware layout */
    a_pll_mode_e audiopll_mode;     /*The audiopll mode: 0:integer-N; 1:fractional-N*/

#ifdef CONFIG_CFG_DRV
    struct phy_dac_external_config external_config; /* DAC external configuration */
#endif

#ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
    struct k_delayed_work antipop_timer;
    uint8_t is_antipop_complete;
#endif
    atomic_t refcount; /* DAC resources reference counter */

    uint8_t ch_fifo0_start : 1; /* The fifo0 channel start indicator */
    uint8_t vol_set_mute : 1; /* The flag of the volume setting less than #VOL_MUTE_MIN_DB event*/
    uint8_t cpu_fifouse_sel : 2; /* The fifouse sel when initialize at cpu */

    void (*dsp_audio_set_param)(uint8_t id, uint32_t param1, uint32_t param2);
};

/**
 * union phy_dac_features
 * @brief The infomation from DTS to control the DAC features to enable or nor.
 */
typedef union {
    uint32_t raw;
    struct {
        uint32_t layout : 2; /* DAC working layout(0: single-end non-direct; 1: single-end direct(VRO); 2 differencial) */
        uint32_t dac_lr_mix : 1; /* DAC left and right channels MIX */
        uint32_t noise_detect_mute : 1; /* noise detect mute */
        uint32_t automute : 1; /* auto-mute */
        uint32_t loopback : 1; /* ADC => DAC loopback */
        uint32_t left_mute : 1; /* DAC left mute */
        uint32_t right_mute : 1; /* DAC left mute */
        uint32_t pa_vol : 3; /* DAC PA gain config */
        uint32_t am_irq : 1; /* if 1 to enable auto mute irq */
    } v;
} phy_dac_features;

/**
 * struct phy_dac_config_data
 * @brief The hardware related data that used by physical dac driver.
 */
struct phy_dac_config_data {
    uint32_t reg_base; /* DAC controller register base address */
    struct audio_dma_dt dma_fifo0; /* DMA resource for FIFO0 */
    uint8_t clk_id; /* DAC devclk id */
    uint8_t rst_id; /* DAC reset id */
    void (*irq_config)(void); /* IRQ configuration function */
    phy_dac_features features; /* DAC features */
};

/*
 * enum a_dac_fifo_e
 * @brief DAC fifo index selection
 */
typedef enum {
    DAC_FIFO_0 = 0,
    DAC_FIFO_1
} a_dac_fifo_e;

/*
 * enum a_dac_sr_e
 * @brief DAC sample rate select
 */
typedef enum {
    DAC_FIR_MODE_A = 0,
    DAC_FIR_MODE_B,
    DAC_FIR_MODE_C
} a_dac_fir_mode_e;

/*
 * enum a_dac_sr_e
 * @brief DAC sample rate select
 */
typedef enum {
    DAC_SR_8K = 0,
    DAC_SR_11K = 1,
    DAC_SR_12K = 2,
    DAC_SR_16K = 3,
    DAC_SR_22K = 4,
    DAC_SR_24K = 5,
    DAC_SR_32K = 6,
    DAC_SR_44K = 7,
    DAC_SR_48K = 8,
    DAC_SR_88K = 9,
    DAC_SR_96K = 10
} a_dac_sr_e;

/*
 * enum a_layout_e
 * @brief The DAC working layout
 */
typedef enum {
    SINGLE_END_MODE = 0,
    SINGLE_END_VOR_MODE,
    DIFFERENTIAL_MODE
} a_layout_e;

/* @brief get the base address of DAC register */
static inline struct acts_audio_dac *get_dac_reg_base(struct device *dev)
{
    const struct phy_dac_config_data *cfg = dev->config;
    return (struct acts_audio_dac *)cfg->reg_base;
}

/* @brief dump dac controller register */
static void dac_dump_register(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    LOG_INF("** dac contoller regster **");
    LOG_INF("             BASE: %08x", (uint32_t)dac_reg);
    LOG_INF("       DAC_DIGCTL: %08x", dac_reg->digctl);
    LOG_INF("      DAC_FIFOCTL: %08x", dac_reg->fifoctl);
    LOG_INF("         DAC_STAT: %08x", dac_reg->stat);
    LOG_INF("        FIFO0_DAT: %08x", dac_reg->fifo0_dat);
    LOG_INF("      PCM_BUF_CTL: %08x", dac_reg->pcm_buf_ctl);
    LOG_INF("     PCM_BUF_STAT: %08x", dac_reg->pcm_buf_stat);
    LOG_INF(" PCM_BUF_THRES_HE: %08x", dac_reg->pcm_buf_thres_he);
    LOG_INF(" PCM_BUF_THRES_HF: %08x", dac_reg->pcm_buf_thres_hf);
    LOG_INF("    SDM_RESET_CTL: %08x", dac_reg->sdm_reset_ctl);
    LOG_INF("    AUTO_MUTE_CTL: %08x", dac_reg->auto_mute_ctl);
    LOG_INF("          VOL_LCH: %08x", dac_reg->vol_lch);
    LOG_INF("          VOL_RCH: %08x", dac_reg->vol_rch);
    LOG_INF("      PCM_BUF_CNT: %08x", dac_reg->pcm_buf_cnt);
    LOG_INF("      DAC_ANALOG0: %08x", dac_reg->anactl0);
    LOG_INF("      DAC_ANALOG1: %08x", dac_reg->anactl1);
    LOG_INF("      DAC_ANALOG2: %08x", dac_reg->anactl2);
    LOG_INF("         DAC_BIAS: %08x", dac_reg->bias);
    LOG_INF("  SDM_SAMPLES_CNT: %08x", dac_reg->sdm_samples_cnt);
    LOG_INF("  SDM_SAMPLES_NUM: %08x", dac_reg->sdm_samples_num);
    LOG_INF("   HW_TRIGGER_CTL: %08x", dac_reg->hw_trigger_dac_ctl);
    LOG_INF("   ADC_REF_LDO_CTL: %08x", sys_read32(ADC_REF_LDO_CTL));

    LOG_INF("    AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
    LOG_INF("       CMU_DACCLK: %08x", sys_read32(CMU_DACCLK));
    LOG_INF("       CMU_DEVCLKEN0: %08x", sys_read32(CMU_DEVCLKEN0));
    LOG_INF("       RMU_MRCR0: %08x", sys_read32(RMU_MRCR0));
}

/* @brief disable DAC FIFO by specified FIFO index */
static void __dac_fifo_disable(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    /**
      * CMU_DACCLK_DACFIFO0CLKEN/CMU_DACCLK_DACFIFO1CLKEN is a clock gate for DAC FIFO read/write for power consumption.
      * When to access DAC_VOL or PA_VOL, shall enable those bits.
      */
    if (DAC_FIFO_0 == idx) {
        dac_reg->fifoctl &= ~DAC_FIFOCTL_DAF0RT;
        /* disable DAC FIFO0 to access clock */
        sys_write32(sys_read32(CMU_DACCLK) & ~CMU_DACCLK_DACFIFO0CLKEN, CMU_DACCLK);
    } else if (DAC_FIFO_1 == idx) {
        LOG_ERR("invalid fifo idx %d", idx);
    }
}

/* @brief enable DAC FIFO0/FIFO1 */
static int __dac_fifo_enable(struct device *dev, audio_fifouse_sel_e sel,
                            audio_dma_width_e wd, uint8_t drq_level,
                            uint8_t fifo_vol, a_dac_fifo_e idx, bool fifo1_mix_en)
{
    struct phy_dac_drv_data *data = dev->data;
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->fifoctl;

    if (drq_level > DAC_FIFO_MAX_DRQ_LEVEL)
        drq_level = DAC_FIFO_MAX_DRQ_LEVEL;

    if (FIFO_SEL_ASRC== sel) {
        LOG_ERR("invalid fifo sel %d", sel);
        return -EINVAL;
    }

    data->cpu_fifouse_sel = sel;

    if (DAC_FIFO_0 == idx) {
        reg &= ~0xFFFF; /* clear all FIFO0 fields */

        if (FIFO_SEL_CPU == sel)
            reg |= DAC_FIFOCTL_DAF0EIE; /* enable irq */
        else if (FIFO_SEL_DMA == sel)
            reg |= DAC_FIFOCTL_DAF0EDE; /* enable drq */

        reg |= DAC_FIFOCTL_DAF0IS(sel);

        if (DMA_WIDTH_16BITS == wd)
            reg |= DAC_FIFOCTL_DACFIFO0_DMAWIDTH;

        reg |= DAC_FIFOCTL_DRQ0_LEVEL(drq_level);

        reg |= DAC_FIFOCTL_DAF0RT;

        dac_reg->fifoctl = reg;

        /* DAC FIFO0 MIX to DAC enable */
        dac_reg->digctl |= DAC_DIGCTL_DAF0M2DAEN;

        sys_write32(sys_read32(CMU_DACCLK) | CMU_ADCCLK_ADCFIFOCLKEN, CMU_DACCLK);

    } else{ 
        LOG_ERR("invalid fifo idx %d", idx);
	}

	return 0;
}
#if 0
/*
 * @brief DAC FIFO reset
 */
static void __dac_fifo_reset(struct device *dev, a_dac_fifo_e idx)
{
	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

	if (DAC_FIFO_0 == idx) {
		dac_reg->fifoctl &= ~DAC_FIFOCTL_DAF0RT;
		k_busy_wait(1);
		dac_reg->fifoctl |= DAC_FIFOCTL_DAF0RT;
	} else{
        LOG_ERR("invalid fifo idx %d", idx);
	}
}
#endif
static bool __dac_fifosrc_is_dsp(struct device *dev)
{
	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
	uint32_t reg = dac_reg->fifoctl;

	if ((reg & DAC_FIFOCTL_DAF0IS_MASK) == (FIFO_SEL_DSP << DAC_FIFOCTL_DAF0IS_SHIFT)) {
		return true;
	} else {
		return false;
	}
    return false;
}

/* @brief update DAC FIFO0/FIFO1 src */
static int __dac_fifo_update_src(struct device *dev, dac_fifosrc_setting_t *fifosrc)
{
	struct phy_dac_drv_data *data = dev->data;
	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
	uint32_t reg = dac_reg->fifoctl;
	audio_fifouse_sel_e sel = (fifosrc->fifo_from_dsp)? (FIFO_SEL_DSP): (data->cpu_fifouse_sel);
	a_dac_fifo_e idx = fifosrc->fifo_idx;

	data->dsp_audio_set_param = fifosrc->dsp_audio_set_param;

	if (DAC_FIFO_0 == idx) {
		reg &= ~DAC_FIFOCTL_DAF0IS_MASK;
		reg |= DAC_FIFOCTL_DAF0IS(sel);

		if (sel == FIFO_SEL_DSP) {
			reg &= ~DAC_FIFOCTL_DAF0EIE; /* disable irq */
			reg &= ~DAC_FIFOCTL_DAF0EDE; /* disable drq */
		} else {
			if (FIFO_SEL_CPU == sel) {
				reg |= DAC_FIFOCTL_DAF0EIE; /* enable irq */
				reg &= ~DAC_FIFOCTL_DAF0EDE; /* disable drq */
			} else if (FIFO_SEL_DMA == sel) {
				reg &= ~DAC_FIFOCTL_DAF0EIE; /* disable irq */
				reg |= DAC_FIFOCTL_DAF0EDE; /* enable drq */
			}
		}
	} else{
        LOG_ERR("invalid fifo idx %d", idx);
	}

	dac_reg->fifoctl = reg;

	if (sel == FIFO_SEL_DSP) {
		irq_disable(IRQ_ID_DACFIFO);
	} else {
		irq_enable(IRQ_ID_DACFIFO);
	}

	return 0;
}

/*
 * @brief Check and wait the DAC FIFO is empty or not
 */
static void __wait_dac_fifo_empty(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        while (((dac_reg->stat & DAC_STAT_DAF0S_MASK) >> DAC_STAT_DAF0S_SHIFT)
                != DAC_FIFO_MAX_LEVEL) {
            ;
        }
    } else{ 
        LOG_ERR("invalid fifo idx %d", idx);
    }
}

/* @brief check if the specified FIFO is working or not */
static bool __is_dac_fifo_working(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        if (dac_reg->fifoctl & DAC_FIFOCTL_DAF0RT)
            return true;
    } else{
        LOG_ERR("invalid fifo idx %d", idx);
    }

    return false;
}

/* @brief get the available samples to fill into PCM buffer */
static uint32_t __get_pcmbuf_avail_length(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t key, avail;

    key = irq_lock();
    avail = (dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) >> PCM_BUF_STAT_PCMBS_SHIFT;
    irq_unlock(key);

    LOG_DBG("PCMBUF free space 0x%x samples", avail);

    return avail;
}

/* @brief check if dac fifo empty */
static bool __is_dac_fifo_empty(struct device *dev, a_dac_fifo_e idx)
{
    //struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        /* DAC FIFO0 connects with PCMBUF  */
        if (DAC_PCMBUF_MAX_CNT == __get_pcmbuf_avail_length(dev))
            return true;
    } else{
        LOG_ERR("invalid fifo idx %d", idx);
    }

    return false;
}

/* @brief check if all DAC FIFO resources are free */
static bool __is_dac_fifo_all_free(struct device *dev, bool check_mix)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (check_mix) {
        if (dac_reg->digctl & DAC_DIGCTL_DAF0M2DAEN)
            return false;
    } else {
        if (__is_dac_fifo_working(dev, DAC_FIFO_0))
            return false;
    }

    return true;
}

/* @brief check if there is error happened in given fifo index */
static bool __check_dac_fifo_error(struct device *dev, a_dac_fifo_e idx)
{
    return false;
}

/* @brief clear fifo error status */
static void __dac_clear_fifo_error(struct device *dev, a_dac_fifo_e idx)
{
    return;
}

/* @brief Claim that there is spdif(128fs) under linkage mode. */
static void __dac_digital_claim_128fs(struct device *dev, bool en)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (en)
        dac_reg->digctl |= DAC_DIGCTL_AUDIO_128FS_256FS;
    else
        dac_reg->digctl &= ~DAC_DIGCTL_AUDIO_128FS_256FS;
}

/* @brief enable the FIFO sample counter function and by default to enable overflow irq */
static void __dac_fifo_counter_enable(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_EN;
        /* By default to enbale pcm buf counter overflow IRQ */
        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IE;
        /* clear sample counter irq pending */
        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IP;
    } else{
        LOG_ERR("invalid fifo idx %d", idx);
    }
}

/* @brief disable the FIFO sample counter function */
static void __dac_fifo_counter_disable(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        dac_reg->pcm_buf_cnt &= ~(PCM_BUF_CNT_EN | PCM_BUF_CNT_IE);
    } else{
        LOG_ERR("invalid fifo idx %d", idx);
    }
}

/* @brief reset the FIFO sample counter function */
static void __dac_fifo_counter_reset(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        dac_reg->pcm_buf_cnt &= ~(PCM_BUF_CNT_EN | PCM_BUF_CNT_IE);
        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_EN;
        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IE;
    } else {
        LOG_ERR("invalid fifo idx %d", idx);
    }
}

/* @brief enable the DAC SDM sample counter function */
static void __dac_sdm_counter_enable(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_EN;
    dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_IE;
    dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_IP;
}

/* @brief disable DAC SDM sample counter function */
static void __dac_sdm_counter_disable(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    dac_reg->sdm_samples_cnt &= ~(SDM_SAMPLES_CNT_EN | SDM_SAMPLES_CNT_IE);
}

/* @brief reset the DAC SDM sample counter function */
static void __dac_sdm_counter_reset(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    dac_reg->sdm_samples_cnt &= ~(SDM_SAMPLES_CNT_EN | SDM_SAMPLES_CNT_IE);
    dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_EN;
    dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_IE;
}

/* @brief read the DAC SDM sample counter */
static uint32_t __dac_read_sdm_counter(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t val = 0;

    val = dac_reg->sdm_samples_cnt & SDM_SAMPLES_NUM_CNT_MASK;

    return val;
}

/* @brief read the DAC SDM sample stable counter */
static uint32_t __dac_read_sdm_stable_counter(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t val = 0;

    val = dac_reg->sdm_samples_num & SDM_SAMPLES_NUM_CNT_MASK;

    return val;
}


/* @brief set the DAC FIFO DRQ level */
static int __dac_fifo_drq_level_set(struct device *dev, a_dac_fifo_e idx, uint8_t level)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->fifoctl;

    if (level > DAC_FIFO_MAX_DRQ_LEVEL)
        return -EINVAL;

    if (DAC_FIFO_0 == idx) {
        reg &= ~DAC_FIFOCTL_DRQ0_LEVEL_MASK;
        reg |= DAC_FIFOCTL_DRQ0_LEVEL(level);
    } else {
        return -EINVAL;
    }

    dac_reg->fifoctl = reg;

    return 0;
}

/* @brief get the DAC FIFO DRQ level */
static int __dac_fifo_drq_level_get(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->fifoctl;
    int level;

    if (DAC_FIFO_0 == idx) {
        level = (reg & DAC_FIFOCTL_DRQ0_LEVEL_MASK) >> DAC_FIFOCTL_DRQ0_LEVEL_SHIFT;
    } else {
        level = -EINVAL;
    }

    return level;
}

/* @brief read the FIFO sample counter by specified FIFO index */
static uint32_t __dac_read_fifo_counter(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t val = 0;

    if (DAC_FIFO_0 == idx) {
        val = dac_reg->pcm_buf_cnt & PCM_BUF_CNT_CNT_MASK;
    } 

    return val;
}

/* @brief PCM BUF configuration */
static int __dac_pcmbuf_config(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->pcm_buf_ctl;

    reg &= ~PCM_BUF_CTL_IRQ_MASK;

    /* By default to enable PCMBUF half empty IRQs */
    reg |= DAC_PCMBUF_DEFAULT_IRQ;

    dac_reg->pcm_buf_ctl = reg;

    dac_reg->pcm_buf_thres_he = CONFIG_AUDIO_DAC_0_PCMBUF_HE_THRES;
    dac_reg->pcm_buf_thres_hf = CONFIG_AUDIO_DAC_0_PCMBUF_HF_THRES;

    /* Clean all pcm buf irqs pending */
    dac_reg->pcm_buf_stat |= PCM_BUF_STAT_IRQ_MASK;

    LOG_DBG("ctl:0x%x, thres_he:0x%x thres_hf:0x%x",
                dac_reg->pcm_buf_ctl, dac_reg->pcm_buf_thres_he,
                dac_reg->pcm_buf_thres_hf);

    return 0;
}

static int __dac_pcmbuf_threshold_update(struct device *dev, dac_threshold_setting_t *thres)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (!thres)
        return -EINVAL;

    if ((thres->hf_thres >= DAC_PCMBUF_MAX_CNT)
        || (thres->he_thres > thres->hf_thres)) {
        LOG_ERR("Invalid threshold hf:%d he:%d",
            thres->hf_thres, thres->he_thres);
        return -ENOEXEC;
    }

    dac_reg->pcm_buf_thres_he = thres->he_thres;
    dac_reg->pcm_buf_thres_hf = thres->hf_thres;

    LOG_INF("new dac threshold => he:0x%x hf:0x%x",
        dac_reg->pcm_buf_thres_he, dac_reg->pcm_buf_thres_hf);

    return 0;
}

/* @brief set the external trigger source for DAC digital start */
static int __dac_external_trigger_enable(struct device *dev, uint8_t trigger_src)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (trigger_src > 6) {
        LOG_ERR("Invalid DAC trigger source %d", trigger_src);
        return -EINVAL;
    }

    dac_reg->hw_trigger_dac_ctl &= ~ HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SEL_MASK;
    dac_reg->hw_trigger_dac_ctl |= HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SEL(trigger_src);

	//dac_reg->hw_trigger_dac_ctl |= HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN;

    LOG_INF("set DAC external trigger_src:%d", trigger_src);

    return 0;
}

/* @breif control the DAC functions that can be triggered by external signals  */
static int __dac_external_trigger_control(struct device *dev, dac_ext_trigger_ctl_t *trigger_ctl)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->hw_trigger_dac_ctl;
    bool valid = false;

    LOG_DBG("extern trigger ctl:0x%x",trigger_ctl->trigger_ctl);

    if (!trigger_ctl) {
        LOG_ERR("Invalid parameter");
        return -EINVAL;
    }

    if (trigger_ctl->t.sdm_cnt_trigger_en) {
        /* disable SDM CNT until external IRQ to trigger enable */
        if (dac_reg->sdm_samples_cnt & SDM_SAMPLES_CNT_EN)
            __dac_sdm_counter_disable(dev);

        reg |= HW_TRIGGER_DAC_CTL_INT_TO_SDMCNT_EN;

        valid = true;
        LOG_INF("enable external trigger DAC SDM_CNT enable");
    }

    if (trigger_ctl->t.sdm_cnt_lock_en) {
        reg |= HW_TRIGGER_DAC_CTL_INT_TO_SDM_CNT;

        valid = true;
        LOG_INF("enable external trigger DAC lock SDM_CNT");
    }

    if (trigger_ctl->t.dac_fifo_trigger_en) {
		if (dac_reg->fifoctl & DAC_FIFOCTL_DAF0RT) {
			dac_reg->fifoctl &= ~(DAC_FIFOCTL_DAF0RT);
		}
        if (dac_reg->fifoctl & DAC_FIFOCTL_DAF0EDE) {
			dac_reg->fifoctl &= ~(DAC_FIFOCTL_DAF0EDE);
		}

		reg |= HW_TRIGGER_DAC_CTL_INT_TO_DACFIFO_EN;

		valid = true;
		LOG_INF("enable external trigger DAC FIFO enable");
    }

    if (trigger_ctl->t.dac_digital_trigger_en) {
        /* disable DAC digital until external IRQ to trigger start */
        if (dac_reg->digctl & DAC_DIGCTL_DDEN_L)
            dac_reg->digctl &= ~DAC_DIGCTL_DDEN_L;

        if (dac_reg->digctl & DAC_DIGCTL_DDEN_R)
            dac_reg->digctl &= ~DAC_DIGCTL_DDEN_R;


        if (dac_reg->sdm_samples_cnt & SDM_SAMPLES_CNT_EN)
            __dac_sdm_counter_reset(dev);

        reg |= HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_L;
        reg |= HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_R;

        valid = true;
        LOG_INF("enable external trigger DAC digital start");
    }

    if (valid)
        dac_reg->hw_trigger_dac_ctl = reg;

    return 0;
}

/* @brief disable the external irq signal to start DAC digital function */
static void __dac_external_trigger_disable(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (dac_reg->hw_trigger_dac_ctl & HW_TRIGGER_DAC_CTL_INT_TO_SDM_CNT)
        dac_reg->hw_trigger_dac_ctl &= ~HW_TRIGGER_DAC_CTL_INT_TO_SDM_CNT;

    if ((dac_reg->hw_trigger_dac_ctl & HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_L) || 
        ((dac_reg->hw_trigger_dac_ctl & HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_R)))
    {
        dac_reg->hw_trigger_dac_ctl &= (~(HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_L | HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_R));
    }

}

/* @brief force DAC digital module to start */
static void __dac_digital_force_start(struct device *dev, dac_ext_trigger_ctl_t *trigger_ctl)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (trigger_ctl->t.dac_fifo_trigger_en) {
        dac_reg->fifoctl |= DAC_FIFOCTL_DAF0RT;
        dac_reg->fifoctl |= DAC_FIFOCTL_DAF0EDE;
        LOG_INF("force enable DAC FIFO");
    }

    if (trigger_ctl->t.dac_digital_trigger_en) {
        dac_reg->digctl |= DAC_DIGCTL_DDEN_L;
        dac_reg->digctl |= DAC_DIGCTL_DDEN_R;
        LOG_INF("force start DAC digital");
    }

    if (trigger_ctl->t.sdm_cnt_trigger_en) {
        __dac_sdm_counter_reset(dev);
        LOG_INF("force start DAC SDM_CNT");
    }
}

/* @brief enable DAC mono mode */
static void __dac_digital_enable_mono(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    dac_reg->digctl |= DAC_DIGCTL_DACHNUM;
}

/* @brief enable DAC digital function */
static int __dac_digital_enable(struct device *dev, a_dac_sr_e sr, a_dac_fir_mode_e fir_mode,
            audio_ch_mode_e type, uint8_t channel_type)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    //const struct phy_dac_config_data *cfg = dev->config;

    uint32_t reg = dac_reg->digctl;

    /* clear interpolation/SR/digital_en etc. */
    reg &= ~(DAC_DIGCTL_DDEN_L | DAC_DIGCTL_DDEN_R | DAC_DIGCTL_ENDITH| DAC_DIGCTL_DACHNUM 
             | DAC_DIGCTL_MULT_DEVICE_MASK | DAC_DIGCTL_SR_MASK | DAC_DIGCTL_FIR_MODE_MASK);

    if ((STEREO_MODE != type) && (MONO_MODE != type))
        return -EINVAL;

    reg |= (fir_mode << DAC_DIGCTL_FIR_MODE_SHIFT);   /*set dac fir mode*/
    reg |= (sr << DAC_DIGCTL_SR_SHIFT);   /* set dac sr */

    /*cic rate*/
    reg &= ~DAC_DIGCTL_CIC_RATE; /* default: 3.072M/2.8224M high performance */

    if (MONO_MODE == type)
        reg |= DAC_DIGCTL_DACHNUM;

    
    #if 0
    /* mono does not support DAC LR MIX */
    if (PHY_DEV_FEATURE(dac_lr_mix) && (STEREO_MODE == type)) {
        LOG_INF("DAC LR MIX enable");
        reg |= DAC_DIGCTL_DALRMIX;
    }
    #endif

    if (channel_type & AUDIO_CHANNEL_I2STX) {
        LOG_INF("Enable DAC linkage with I2STX");
        reg |= DAC_DIGCTL_MULT_DEVICE(1);
    } else{
        LOG_INF("");
    }

    /* digital and dith enable */
    reg |= (DAC_DIGCTL_DDEN_L | DAC_DIGCTL_DDEN_R | DAC_DIGCTL_ENDITH);

    dac_reg->digctl = reg;

    return 0;
}

/* @brief disable digital fifo usage */
static void __dac_digital_disable_fifo(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    if (DAC_FIFO_0 == idx) {
        dac_reg->digctl &= ~DAC_DIGCTL_DAF0M2DAEN; /* disable DAC_FIFO0 MIX to PCMBUF */
    } else{
        LOG_ERR("invalid fifo idx %d", idx);
    }
}

/* @brief disable DAC digital function */
static void __dac_digital_disable(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    const struct phy_dac_config_data *cfg = dev->config;

    /* clear mult-linkage function */
    dac_reg->digctl &= ~DAC_DIGCTL_MULT_DEVICE_MASK;

    /* disable left/right channel volume soft step function */
    dac_reg->vol_lch &= ~VOL_LCH_SOFT_CFG_DEFAULT;
    dac_reg->vol_rch &= ~VOL_RCH_SOFT_CFG_DEFAULT;

    /* disable SDM reset function */
    if (PHY_DEV_FEATURE(noise_detect_mute)) {
        if (dac_reg->sdm_reset_ctl & SDM_RESET_CTL_SDMREEN)
            dac_reg->sdm_reset_ctl &= ~SDM_RESET_CTL_SDMREEN;
    }

    /* disable external irq signal to start DAC */
    if ((dac_reg->hw_trigger_dac_ctl & HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_L)
        && (dac_reg->hw_trigger_dac_ctl & HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_R)
       ) {
        dac_reg->hw_trigger_dac_ctl &= ~(HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_L
            | HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_R | HW_TRIGGER_DAC_CTL_INT_TO_SDM_CNT);
    }

    dac_reg->digctl &= ~(DAC_DIGCTL_ENDITH | DAC_DIGCTL_DDEN_L | DAC_DIGCTL_DDEN_R);
}

/* @brief check if the DAC digital function is working */
static bool __dac_is_digital_working(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if ((dac_reg->digctl & DAC_DIGCTL_DDEN_L) || (dac_reg->digctl & DAC_DIGCTL_DDEN_R))
        return true;

    return false;
}

/* @brief DAC L/R channel volume setting */
static void __dac_volume_set(struct device *dev, uint8_t lr_sel, uint8_t left_v, uint8_t right_v)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg_l, reg_l_old, reg_r, reg_r_old;

    /* DAC left channel volume setting */
    if (lr_sel & LEFT_CHANNEL_SEL) {
        reg_l_old = reg_l = dac_reg->vol_lch;
        reg_l &= ~VOL_LCH_VOLL_MASK;
        reg_l |= VOL_LCH_VOLL(left_v);
        dac_reg->vol_lch = reg_l;
        LOG_INF("left volume: 0x%x => 0x%x", reg_l_old & 0xFF, left_v);
    }

    if (lr_sel & RIGHT_CHANNEL_SEL) {
        reg_r_old = reg_r = dac_reg->vol_rch;
        reg_r &= ~VOL_RCH_VOLR_MASK;
        reg_r |= VOL_RCH_VOLR(right_v);
        dac_reg->vol_rch = reg_r;
        LOG_INF("right volume: 0x%x => 0x%x", reg_r_old & 0xFF, right_v);
    }
}

/* @brief get the current dac L/R volume setting */
static uint8_t __dac_volume_get(struct device *dev, uint8_t lr_sel)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint8_t vol;

    if (lr_sel & LEFT_CHANNEL_SEL)
        vol = dac_reg->vol_lch & VOL_LCH_VOLL_MASK;
    else
        vol = dac_reg->vol_rch & VOL_RCH_VOLR_MASK;

    return vol;
}

/* @brief DAC SDM(noise detect mute) configuration */
static void __dac_sdm_mute_cfg(struct device *dev, uint8_t sr)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->sdm_reset_ctl;
    uint16_t sdm_cnt = CONFIG_AUDIO_DAC_0_SDM_CNT;
    uint16_t sdm_thres = CONFIG_AUDIO_DAC_0_SDM_THRES;

    reg &= ~(SDM_RESET_CTL_SDMNDTH_MASK | SDM_RESET_CTL_SDMCNT_MASK);

    reg |= SDM_RESET_CTL_SDMCNT(sdm_cnt);
    reg |= SDM_RESET_CTL_SDMNDTH(sdm_thres);

    reg |= SDM_RESET_CTL_SDMREEN;

    dac_reg->sdm_reset_ctl = reg;

    LOG_INF("DAC SDM function enable");

    /* Reset SDM after has detected noise
     * NOTE: When sample rate are 8k/11k/12k/16k shall enable this bit
     */
#if 0
    if (sr <= SAMPLE_RATE_16KHZ) {
        reg |= SDM_RESET_CTL_SDMREEN;
        dac_reg->sdm_reset_ctl = reg;
        LOG_INF("DAC SDM function enable");
    }
#endif
}

/* @brief DAC automute function configuration */
static void __dac_automute_cfg(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    const struct phy_dac_config_data *cfg = dev->config;
    uint32_t reg = dac_reg->auto_mute_ctl;
    uint16_t am_cnt = CONFIG_AUDIO_DAC_0_AM_CNT;
    uint16_t am_thres = CONFIG_AUDIO_DAC_0_AM_THRES;

    reg &= ~(AUTO_MUTE_CTL_AMCNT_MASK | AUTO_MUTE_CTL_AMTH_MASK);

    reg |= AUTO_MUTE_CTL_AMCNT(am_cnt);
    reg |= AUTO_MUTE_CTL_AMTH(am_thres);

    if (PHY_DEV_FEATURE(am_irq))
        reg |= AUTO_MUTE_CTL_AM_IRQ_EN; /* enable auto mute IRQ */

    /* Auto mute enable */
    reg |= AUTO_MUTE_CTL_AMEN;

    dac_reg->auto_mute_ctl = reg;

    reg = dac_reg->digctl;
    /* Enable auto mute PA Mute */
    reg |= DAC_DIGCTL_AUMUTE_PA_CTL;
    dac_reg->digctl |= reg;

    LOG_INF("DAC automute function enable");
}

/* @brief ADC loopback to DAC function configuration */
static void __dac_loopback_cfg(struct device *dev, uint8_t lr_sel, bool dac_lr_mix)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->digctl;

    /* Clear  L/R mix, ADCR loopback to DAC, ADCL loopback to DAC */
    reg &= ~(DAC_DIGCTL_ADC01MIX | DAC_DIGCTL_AD2DALPENL | DAC_DIGCTL_AD2DALPENR);

    /* ADC0 L to DAC L loopback */
    if (lr_sel & LEFT_CHANNEL_SEL)
        reg |= DAC_DIGCTL_AD2DALPENL;

    /* ADC1 R to DAC R loopback */
    if (lr_sel & RIGHT_CHANNEL_SEL)
        reg |= DAC_DIGCTL_AD2DALPENR;

    if (dac_lr_mix)
        reg |= DAC_DIGCTL_ADC01MIX;
    else
        reg &= ~DAC_DIGCTL_ADC01MIX;

    dac_reg->digctl = reg;

    LOG_INF("ADDA loopback(lr:%d mix:%d) enable", lr_sel, dac_lr_mix);
}

/* @brief dac left volume soft step setting and 'adj_cnt' is the same as 'sample_rate' */
static void __dac_volume_left_softstep(struct device *dev, uint8_t adj_cnt, bool irq_flag)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg;

    /* Clear soft step done IRQ pending */
    if (dac_reg->vol_lch & VOL_LCH_DONE_PD)
        dac_reg->vol_lch |= VOL_LCH_DONE_PD;

    reg = dac_reg->vol_lch;

    /* Clear all VOL_LCH exclude VOLL */
    reg &= ~0x3FFF00;
    reg |= VOL_LCH_ADJ_CNT(adj_cnt);

    /* to_cnt setting */
    if (DAC_VOL_TO_CNT_DEFAULT)
        reg |= VOL_LCH_TO_CNT;

    if (irq_flag)
        reg |= VOL_LCH_VOLL_IRQ_EN;
    else
        reg &= ~VOL_LCH_VOLL_IRQ_EN;

    reg |= VOL_LCH_SOFT_CFG_DEFAULT;

    dac_reg->vol_lch = reg;
}

/* @brief dac right volume soft step setting and 'adj_cnt' is the same as 'sample_rate' */
static void __dac_volume_right_softstep(struct device *dev, uint8_t adj_cnt, bool irq_flag)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg;

    /* Clear soft step done IRQ pending */
    if (dac_reg->vol_rch & VOL_RCH_DONE_PD)
        dac_reg->vol_rch |= VOL_RCH_DONE_PD;

    reg = dac_reg->vol_rch;

    /* Clear all VOL_RCH exclude VOLR */
    reg &= ~0x3FFF00;

    reg |= VOL_RCH_ADJ_CNT(adj_cnt);

    /* to_cnt setting */
    if (DAC_VOL_TO_CNT_DEFAULT)
        reg |= VOL_RCH_TO_CNT;

    if (irq_flag)
        reg |= VOL_RCH_VOLR_IRQ_EN;
    else
        reg &= ~VOL_RCH_VOLR_IRQ_EN;

    reg |= VOL_RCH_SOFT_CFG_DEFAULT;

    dac_reg->vol_rch = reg;
}

/* @brief DAC enable mute or disable mute control. */
static void __dac_mute_control(struct device *dev, bool mute_en)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    //struct phy_dac_drv_data *data = dev->data;

    if (mute_en) 
        dac_reg->anactl0 |= DAC_ANACTL0_ZERODT; /* DAC L/R channel will output zero data */
    else
        dac_reg->anactl0 &= ~DAC_ANACTL0_ZERODT;
}

/* @brief Disable DAC analog by specified channels */
static int __dac_analog_disable(struct device *dev, uint8_t lr_sel)
{
    struct phy_dac_drv_data *data = dev->data;
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg0, reg1;

    reg0 = dac_reg->anactl0;
    reg1 = dac_reg->anactl1;

    if (lr_sel & LEFT_CHANNEL_SEL) {
        if (data->layout == DIFFERENTIAL_MODE) {
            //Cuckoo deleted
            /* enable LN/LP playback mute */
            //reg1 &= ~(DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN);
        } else {
            /* disable LP channel */
            //reg1 &= ~DAC_ANACTL1_DPBMLP;
        }
        /* disable L channel */
        reg0 &= ~DAC_ANACTL0_DAENL;
        reg0 &= ~(DAC_ANACTL0_PALEN | DAC_ANACTL0_PALOSEN);

        //Cuckoo deleted
        //reg0 &= ~(DAC_ANACTL0_SW1LP | DAC_ANACTL0_SW1LN);
        //reg0 &= ~(DAC_ANACTL0_PALPEN | DAC_ANACTL0_PALPOSEN | DAC_ANACTL0_PALNEN | DAC_ANACTL0_PALNOSEN);
    }

    if (lr_sel & RIGHT_CHANNEL_SEL) {
        if (data->layout == DIFFERENTIAL_MODE) {
            //Cuckoo deleted
            /* enable R playback mute */
            //reg1 &= ~DAC_ANACTL1_DPBMR;
        } else {
            /* enable LN playback mute */
           // reg1 &= ~DAC_ANACTL1_DPBMLN;
        }

        /* disable R channel */
        reg0 &= ~DAC_ANACTL0_DAENR;
        reg0 &= ~(DAC_ANACTL0_PAREN | DAC_ANACTL0_PAROSEN);

        //Cuckoo deleted
        //reg0 &= ~(DAC_ANACTL0_SW1RP | DAC_ANACTL0_SW1RN);
        //reg0 &= ~(DAC_ANACTL0_PARPEN | DAC_ANACTL0_PARPOSEN | DAC_ANACTL0_PARNEN | DAC_ANACTL0_PARNOSEN);
    }

    if ((lr_sel & LEFT_CHANNEL_SEL)
        || (lr_sel & RIGHT_CHANNEL_SEL)) {
        dac_reg->anactl0 = reg0;
        dac_reg->anactl1 = reg1;
    }

    return 0;
}

/* @brief DAC works in single-end non-direct mode or direct mode (VRO)  or diff layout */
static int __dac_analog_cfg(struct device *dev, uint8_t lr_sel)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg0 = dac_reg->anactl0;

    //Cuckoo deleted
    //reg1 = dac_reg->anactl1 & ~(DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN | DAC_ANACTL1_DPBMR);

    /* enable all bias */
    reg0 |= DAC_ANACTL0_BIASEN;

    /* disable zero data */
    reg0 &= ~DAC_ANACTL0_ZERODT;

    /* clear PA_VOL */
    reg0 &= ~DAC_ANACTL0_PAVOL_MASK;


    /* enable PA DFC option */
    reg0 |= DAC_ANACTL0_DFCEN;

    /*PA millerfeedback cap select*/
    //reg0 |= DAC_ANACTL0_SEL_FBCAP;

    /* pa volume config */
    #ifdef CONFIG_CFG_DRV
    struct phy_dac_drv_data *data = dev->data;
    reg0 |= DAC_ANACTL0_PAVOL(data->external_config.Pa_Vol);
    #else
    const struct phy_dac_config_data *cfg = dev->config;
    reg0 |= DAC_ANACTL0_PAVOL(PHY_DEV_FEATURE(pa_vol));
    #endif
    reg0 |= DAC_ANACTL0_DARSEL;

    /* left channel enable */
    if (lr_sel & LEFT_CHANNEL_SEL) {
        /* PA output stage and op enable */
        reg0 |= (DAC_ANACTL0_PALEN | DAC_ANACTL0_PALOSEN);
        
        /* left channel enable */
        reg0 |= DAC_ANACTL0_DAENL;
    }

    /* right channel enable */
    if (lr_sel & RIGHT_CHANNEL_SEL) {
        /* PA output stage and op enable */
        reg0 |= (DAC_ANACTL0_PAREN | DAC_ANACTL0_PAROSEN);
        /* right channel enable */
        reg0 |= DAC_ANACTL0_DAENR;
    }

    dac_reg->anactl0 = reg0;
    //dac_reg->anactl1 = reg1;
    dac_reg->anactl1 = (1 << 12) | (1 << 4);
    dac_reg->anactl2 = 0xff068; 
    if(data->external_config.Pa_Vol == 4){
        dac_reg->anactl2 |= 0x1;
    }

    return 0;
}

#ifdef DAC_ANALOG_DEBUG_IN_ENABLE
static void __dac_analog_dbgi(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = sys_read32(DEBUGSEL) & ~DEBUGSEL_DBGSE_MASK;
    reg |= DEBUGSEL_DBGSE(DBGSE_DAC);
    sys_write32(reg, DEBUGSEL);

    /* debug GPIO input pin13 ~ pin22 */
    sys_write32(0x7fe000, DEBUGIE0);

    reg = dac_reg->digctl & ~(DAC_DIGCTL_DADCS | DAC_DIGCTL_DADEN);
    reg |= DAC_DIGCTL_DADEN;

    dac_reg->digctl = reg;
}
#endif

#ifdef DAC_DIGITAL_DEBUG_OUT_ENABLE
static void __dac_digital_dbgo(struct device *dev, uint8_t lr_sel)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = sys_read32(DEBUGSEL) & ~DEBUGSEL_DBGSE_MASK;
    reg |= DEBUGSEL_DBGSE(DBGSE_DAC);
    sys_write32(reg, DEBUGSEL);

    /* debug GPIO output pin13 ~ pin22 */
    sys_write32(0x7fe000, DEBUGOE0);

    reg = dac_reg->digctl & ~(DAC_DIGCTL_DADCS | DAC_DIGCTL_DDDEN);

    if (lr_sel & LEFT_CHANNEL_SEL)
        reg &= ~DAC_DIGCTL_DADCS; /* left channel debug enable */
    else
        reg |= DAC_DIGCTL_DADCS; /* right channel debug enable */

    reg |= DAC_DIGCTL_DDDEN;
    dac_reg->digctl = reg;
}
#endif

#ifdef CONFIG_CFG_DRV
/* @brief Enable PA/VRO output over load detection. */
static void dac_enable_pa_overload_detect(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    /* PA/VRO over load protection function needs enable DACANCCLK */
    acts_clock_peripheral_enable(CLOCK_ID_DACANACLK);

    dac_reg->anactl0 |= DAC_ANACTL0_OVDTEN;
}
#endif

/* @brief Power control(enable or disable) by DAC LDO */
static void dac_ldo_power_control(struct device *dev, bool enable)
{
    ARG_UNUSED(dev);

    uint32_t reg = sys_read32(ADC_REF_LDO_CTL);
    if (enable) {
        /** FIXME: HW issue
         * ADC LDO shall be enabled when use DAC individually, otherwise VREF_ADD will get low voltage.
         */
        acts_clock_peripheral_enable(CLOCK_ID_ADC);

        /* AULDO pull down current control */
        reg &= ~ADC_REF_LDO_CTL_ADLDO_PD_CTL_MASK;
        reg |= ADC_REF_LDO_CTL_ADLDO_PD_CTL(0);
               

        /* VREF voltage divide res control */
        //reg &= ~ADC_REF_LDO_CTL_VREF_RSEL_MASK;
        //reg |= ADC_REF_LDO_CTL_VREF_RSEL(0);

        reg |= ADC_REF_LDO_CTL_ADLDO_EN(3) | ADC_REF_LDO_CTL_DALDO_EN(3);
        sys_write32(reg, ADC_REF_LDO_CTL);

        /* ADC/DAC VREF voltage enable */
        sys_write32(sys_read32(ADC_REF_LDO_CTL) \
                    | ADC_REF_LDO_CTL_VREF_EN, ADC_REF_LDO_CTL);

        if (!(reg & ADC_REF_LDO_CTL_VREF_EN)) {
            LOG_INF("DAC wait for capacitor charge full");

            sys_write32(sys_read32(ADC_REF_LDO_CTL) | ADC_REF_LDO_CTL_VREF_FU,
                        ADC_REF_LDO_CTL);

            if (!z_is_idle_thread_object(_current))
                k_sleep(K_MSEC(DAC_LDO_CAPACITOR_CHARGE_TIME_MS));
            else
                k_busy_wait(DAC_LDO_CAPACITOR_CHARGE_TIME_MS * 1000UL);

            /* disable LDO fast charge */
            sys_write32(sys_read32(ADC_REF_LDO_CTL) & ~ADC_REF_LDO_CTL_VREF_FU,
                        ADC_REF_LDO_CTL);
        }

        /* Wait for AULDO stable */
        if (!z_is_idle_thread_object(_current))
            k_sleep(K_MSEC(1));
        else
            k_busy_wait(1000);

        /* reduce AULDO static power consume */
        uint32_t reg1 = sys_read32(ADC_REF_LDO_CTL);
        reg1 &= ~ADC_REF_LDO_CTL_ADLDO_PD_CTL_MASK;
        sys_write32(reg1, ADC_REF_LDO_CTL);
    } else {
        reg &= ~ADC_REF_LDO_CTL_DALDO_EN_MASK;

        uint8_t is_busy = 0;
        struct device *adc_dev = (struct device *)device_get_binding(CONFIG_AUDIO_ADC_0_NAME);
        if (!adc_dev)
            LOG_ERR("failed to bind adc device:%s", CONFIG_AUDIO_ADC_0_NAME);

        uint32_t key = irq_lock();

        /* check ADC is busy */
        phy_audio_control(adc_dev, PHY_CMD_IS_ADC_BUSY, &is_busy);

        if (is_busy)
            LOG_INF("ADC current is using");

        /* If ADC is idle to disable ADC LDO and VREF */
        if (adc_dev && !is_busy) {
            reg &= ~ADC_REF_LDO_CTL_ADLDO_EN_MASK;
            reg &= ~ADC_REF_LDO_CTL_VREF_EN;
        }

        irq_unlock(key);

        sys_write32(reg, ADC_REF_LDO_CTL);
    }

}

/* @brief Translate the volume in db format to DAC hardware volume level. */
static uint16_t dac_volume_db_to_level(int32_t vol)
{
    uint32_t level = 0;
    if (vol < 0) {
        vol = -vol;
        level = VOL_DB_TO_INDEX(vol);
        if (level > 0xBE)
            level = 0;
        else
            level = 0xBE - level;
    } else {
        level = VOL_DB_TO_INDEX(vol);
        if (level > 0x40)
            level = 0xFE;   //max 24db
        else
            level = 0xBE + level;
    }
    return level;
}

/* @brief Translate the DAC hardware volume level to volume in db format. */
static int32_t dac_volume_level_to_db(uint16_t level)
{
    int32_t vol = 0;
    if (level < 0xBE) {
        level = 0xBE - level;
        vol = VOL_INDEX_TO_DB(level);
        vol = -vol;
    } else {
        level = level - 0xBE;
        vol = VOL_INDEX_TO_DB(level);
    }
    return vol;
}

/* @brief DAC left/right channel volume setting */
static int dac_volume_set(struct device *dev, int32_t left_vol, int32_t right_vol, uint8_t sr)
{
    const struct phy_dac_config_data *cfg = dev->config;
    struct phy_dac_drv_data *data = dev->data;

    uint8_t lr_sel = 0;
    uint16_t cur_vol_level, l_vol_level, r_vol_level;

    /* check left channel not mute and volume is valid */
    if (!PHY_DEV_FEATURE(left_mute) && (left_vol != AOUT_VOLUME_INVALID))
        lr_sel |= LEFT_CHANNEL_SEL;

    /* check right channel not mute and volume is valid */
    if (!PHY_DEV_FEATURE(right_mute) && (right_vol != AOUT_VOLUME_INVALID))
        lr_sel |= RIGHT_CHANNEL_SEL;

    if ((left_vol <= VOL_MUTE_MIN_DB) || (right_vol <= VOL_MUTE_MIN_DB)) {
        LOG_INF("volume [%d, %d] less than mute level %d",
        left_vol, right_vol, VOL_MUTE_MIN_DB);
        __dac_mute_control(dev, true);
        data->vol_set_mute = 1;
    } else {
        /* disable mute when volume become normal */
        if (data->vol_set_mute) {
            __dac_mute_control(dev, false);
            data->vol_set_mute = 0;
        }
    }

    l_vol_level = dac_volume_db_to_level(left_vol);
    cur_vol_level = __dac_volume_get(dev, LEFT_CHANNEL_SEL);
    if (cur_vol_level == l_vol_level) {
        LOG_DBG("ignore same left volume:%d", cur_vol_level);
        lr_sel &= ~LEFT_CHANNEL_SEL;
    }

    r_vol_level = dac_volume_db_to_level(right_vol);
    cur_vol_level = __dac_volume_get(dev, RIGHT_CHANNEL_SEL);
    if (cur_vol_level == r_vol_level) {
        LOG_DBG("ignore same right volume:%d", cur_vol_level);
        lr_sel &= ~RIGHT_CHANNEL_SEL;
    }

    __dac_volume_set(dev, lr_sel, l_vol_level, r_vol_level);

	if (__dac_fifosrc_is_dsp(dev) && data->dsp_audio_set_param) {
		data->dsp_audio_set_param(DSP_AUDIO_SET_VOLUME, l_vol_level, r_vol_level);
	}

    LOG_DBG("set volume {db:[%d, %d] level:[%x, %x]}",
            left_vol, right_vol, l_vol_level, r_vol_level);

    if (__get_pcmbuf_avail_length(dev) >= DAC_PCMBUF_MAX_CNT) {
        LOG_DBG("no data in pcmbuf can not enable soft step volume");
        return 0;
    }

    if (lr_sel & LEFT_CHANNEL_SEL)
        __dac_volume_left_softstep(dev, sr, false);

    if (lr_sel & RIGHT_CHANNEL_SEL)
        __dac_volume_right_softstep(dev, sr, false);

    return 0;
}

/* @brief Configure the physical layout within DAC */
static int dac_physical_layout_cfg(struct device *dev)
{
    const struct phy_dac_config_data *cfg = dev->config;
    struct phy_dac_drv_data *data = dev->data;
    int ret = -1;
    uint8_t lr_sel = 0, layout = PHY_DEV_FEATURE(layout);

    if (!PHY_DEV_FEATURE(left_mute))
        lr_sel |= LEFT_CHANNEL_SEL;

    if (!PHY_DEV_FEATURE(right_mute))
        lr_sel |= RIGHT_CHANNEL_SEL;

        /* External configuration with higher priority than DTS setting */
#ifdef CONFIG_CFG_DRV
    if (AUDIO_OUT_MODE_DAC_NODIRECT == data->external_config.Out_Mode)
        layout = SINGLE_END_MODE;
    else
        layout = DIFFERENTIAL_MODE;
#endif

    switch (layout) {
        case SINGLE_END_MODE:
            ret = __dac_analog_cfg(dev, lr_sel);
            break;
        case SINGLE_END_VOR_MODE:
            ret = __dac_analog_cfg(dev, lr_sel);
            break;
        case DIFFERENTIAL_MODE:
            ret = __dac_analog_cfg(dev, lr_sel);
            break;
        default:
            ret = -1;
    }

    if (!ret) {
        data->lr_sel = lr_sel;
        data->layout = layout;
    }

#ifdef CONFIG_CFG_DRV
    if (data->external_config.Enable_large_current_protect)
        dac_enable_pa_overload_detect(dev);
#endif

    return ret;
}

/* @brief Enable the features that supported by DAC */
static int dac_enable_features(struct device *dev, uint8_t sr)
{
    const struct phy_dac_config_data *cfg = dev->config;

    if (PHY_DEV_FEATURE(automute))
        __dac_automute_cfg(dev);

    if (PHY_DEV_FEATURE(noise_detect_mute))
        __dac_sdm_mute_cfg(dev, sr);

    if (PHY_DEV_FEATURE(loopback))
        __dac_loopback_cfg(dev, LEFT_CHANNEL_SEL | RIGHT_CHANNEL_SEL, false);

    return dac_physical_layout_cfg(dev);
}
/* @brief DAC OSR selection according to the sample rate */
static int dac_get_fir_mode(struct device *dev, audio_sr_sel_e sample_rate)
{
    int fir_mode = -1;

    ARG_UNUSED(dev);

    switch (sample_rate) {
        case SAMPLE_RATE_8KHZ:
        case SAMPLE_RATE_11KHZ:
        case SAMPLE_RATE_12KHZ:
        case SAMPLE_RATE_16KHZ:
            fir_mode = DAC_FIR_MODE_C;
            break;

        case SAMPLE_RATE_22KHZ:
        case SAMPLE_RATE_24KHZ:
        case SAMPLE_RATE_32KHZ:
        case SAMPLE_RATE_44KHZ:
        case SAMPLE_RATE_48KHZ:
            fir_mode = DAC_FIR_MODE_B;
            break;

        case SAMPLE_RATE_88KHZ:
        case SAMPLE_RATE_96KHZ:
        default:
            fir_mode = DAC_FIR_MODE_A;
            break;
    }

    return fir_mode;
}


/* @brief DAC OSR selection according to the sample rate */
static int dac_sample_rate_to_sr(struct device *dev, audio_sr_sel_e sample_rate)
{
    int sr = -1;

    ARG_UNUSED(dev);

    switch (sample_rate) {
        case SAMPLE_RATE_8KHZ:
            sr = DAC_SR_8K;
            break;

        case SAMPLE_RATE_11KHZ:
            sr = DAC_SR_11K;
            break;
        case SAMPLE_RATE_12KHZ:
            sr = DAC_SR_12K;
            break;

        case SAMPLE_RATE_16KHZ:
            sr = DAC_SR_16K;
            break;

        case SAMPLE_RATE_22KHZ:
            sr = DAC_SR_22K;
            break;

        case SAMPLE_RATE_24KHZ:
            sr = DAC_SR_24K;
            break;

        case SAMPLE_RATE_32KHZ:
            sr = DAC_SR_32K;
            break;

        case SAMPLE_RATE_44KHZ:
            sr = DAC_SR_44K;
            break;

        case SAMPLE_RATE_48KHZ:
            sr = DAC_SR_48K;
            break;

        case SAMPLE_RATE_88KHZ:
            sr = DAC_SR_88K;
            break;

        case SAMPLE_RATE_96KHZ:
            sr = DAC_SR_96K;
            break;

        default:
            sr = DAC_SR_48K;
            break;
    }

    return sr;
}

#if 0
/* check the specified sample rate is need to enable interpolation x3 */
static int dac_sample_rate_is_interpolation_x3(struct device *dev, audio_sr_sel_e sample_rate)
{
    int sr;

    sr = dac_sample_rate_to_sr(dev, sample_rate);
    if (sr < 0) {
        LOG_ERR("mapping sample rate:%d to sr error", sample_rate);
        return -EFAULT;
    }

    if (sr > DAC_SR_96K)
        return 1;

    return 0;
}
#endif

/* @brief DAC sample rate config */
static int dac_sample_rate_set(struct device *dev, audio_sr_sel_e sr_khz)
{
    struct phy_dac_drv_data *data = dev->data;

    uint8_t clk_div=0, firclkdiv=0, fir2xclkdiv=0, cicclkdiv=0, fs_sel=0, pll_index=0;
    uint32_t reg;
    int ret;

    ARG_UNUSED(dev);

	switch(sr_khz){
		case(SAMPLE_RATE_96KHZ):{clk_div=0; firclkdiv=0; fir2xclkdiv=0; cicclkdiv=1; fs_sel=AUDIOPLL_48KSR; break;}  //96K
		case(SAMPLE_RATE_88KHZ):{clk_div=0; firclkdiv=0; fir2xclkdiv=0; cicclkdiv=1; fs_sel=AUDIOPLL_44KSR; break;}  //88.2K
        case(SAMPLE_RATE_64KHZ):{clk_div=0; firclkdiv=0; fir2xclkdiv=0; cicclkdiv=1; fs_sel=AUDIOPLL_48KSR; break;}  //64K
		case(SAMPLE_RATE_48KHZ):{clk_div=0; firclkdiv=0; fir2xclkdiv=1; cicclkdiv=1; fs_sel=AUDIOPLL_48KSR; break;}  //48K
		case(SAMPLE_RATE_44KHZ):{clk_div=0; firclkdiv=0; fir2xclkdiv=1; cicclkdiv=1; fs_sel=AUDIOPLL_44KSR; break;}  //44.1K
		case(SAMPLE_RATE_32KHZ):{clk_div=0; firclkdiv=0; fir2xclkdiv=2; cicclkdiv=1; fs_sel=AUDIOPLL_48KSR; break;}  //32K
		case(SAMPLE_RATE_24KHZ):{clk_div=1; firclkdiv=0; fir2xclkdiv=1; cicclkdiv=0; fs_sel=AUDIOPLL_48KSR; break;}  //24K
		case(SAMPLE_RATE_22KHZ):{clk_div=1; firclkdiv=0; fir2xclkdiv=1; cicclkdiv=0; fs_sel=AUDIOPLL_44KSR; break;}  //22.05K
		case(SAMPLE_RATE_16KHZ):{clk_div=1; firclkdiv=0; fir2xclkdiv=2; cicclkdiv=0; fs_sel=AUDIOPLL_48KSR; break;}  //16K
		case(SAMPLE_RATE_12KHZ):{clk_div=1; firclkdiv=1; fir2xclkdiv=1; cicclkdiv=0; fs_sel=AUDIOPLL_48KSR; break;}  //12K
		case(SAMPLE_RATE_11KHZ):{clk_div=1; firclkdiv=1; fir2xclkdiv=1; cicclkdiv=0; fs_sel=AUDIOPLL_44KSR; break;}  //11.025K
		case(SAMPLE_RATE_8KHZ) :{clk_div=1; firclkdiv=1; fir2xclkdiv=2; cicclkdiv=0; fs_sel=AUDIOPLL_48KSR; break;}  //8K
		default:break;
	}

    /* Check the pll usage and then config */
    ret = audio_pll_check_config(fs_sel, &pll_index, data->audiopll_mode);
    if (ret) {
        LOG_DBG("check pll config error:%d", ret);
        return ret;
    }


    reg = sys_read32(CMU_DACCLK) & (~0x1FFFFFF);

    /* Select audio_pll0 or audio_pll1 */
    reg |= (pll_index & 0x1) << CMU_DACCLK_DACCLKSRC_SHIFT;
    reg |= CMU_DACCLK_DACSDMLCLKEN;
    reg |= CMU_DACCLK_DACCICFIRCLKEN;
    reg |= CMU_DACCLK_DACFIFO0CLKEN;
    reg |=  (clk_div << CMU_DACCLK_DACCLKDIV_SHIFT);
    reg |= (firclkdiv << CMU_DACCLK_DACFIRCLKDIV_SHIFT);
    reg |= CMU_DACCLK_DACFIFCLK2XDIV(fir2xclkdiv);
    reg |= (cicclkdiv << CMU_DACCLK_DACCICCLKDIV_SHIFT);

    sys_write32(reg, CMU_DACCLK);

    return 0;
}

/* @brief Get the sample rate from the DAC config */
static int dac_sample_rate_get(struct device *dev)
{
    uint8_t clk_div, pll_index;
    uint32_t reg = sys_read32(CMU_DACCLK);

    ARG_UNUSED(dev);

    pll_index = (reg & (1 << CMU_DACCLK_DACCLKSRC_SHIFT)) >> CMU_DACCLK_DACCLKSRC_SHIFT;
    clk_div = reg & CMU_ADCCLK_ADCCLKDIV_MASK;

    return audio_get_pll_sample_rate(clk_div, pll_index);
}

/* @brief Get the AUDIO_PLL APS used by DAC */
static int dac_get_pll_aps(struct device *dev)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_DACCLK);
    pll_index = (reg & (1 << CMU_DACCLK_DACCLKSRC_SHIFT)) >> CMU_DACCLK_DACCLKSRC_SHIFT;

    return audio_pll_get_aps((a_pll_type_e)pll_index);
}

/* @brief Get the AUDIO_PLL APS used by DAC */
static int dac_get_pll_aps_f(struct device *dev)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_DACCLK);
    pll_index = (reg & (1 << CMU_DACCLK_DACCLKSRC_SHIFT)) >> CMU_DACCLK_DACCLKSRC_SHIFT;

    return audio_pll_get_aps_f((a_pll_type_e)pll_index);
}

/* @brief Set the AUDIO_PLL APS used by DAC */
static int dac_set_pll_aps(struct device *dev, audio_aps_level_e level)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_DACCLK);
    pll_index = (reg & (1 << CMU_DACCLK_DACCLKSRC_SHIFT)) >> CMU_DACCLK_DACCLKSRC_SHIFT;

    return audio_pll_set_aps((a_pll_type_e)pll_index, level);
}

/* @brief Set the AUDIO_PLL APS used by DAC */
static int dac_set_pll_aps_f(struct device *dev, unsigned int level)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_DACCLK);
    pll_index = (reg & (1 << CMU_DACCLK_DACCLKSRC_SHIFT)) >> CMU_DACCLK_DACCLKSRC_SHIFT;

    return audio_pll_set_aps_f((a_pll_type_e)pll_index, level);
}

/* @brief Get the DAC DMA information */
static int dac_get_dma_info(struct device *dev, struct audio_out_dma_info *info)
{
    const struct phy_dac_config_data *cfg = dev->config;

    if (AOUT_FIFO_DAC0 == info->fifo_type) {
        info->dma_info.dma_chan = cfg->dma_fifo0.dma_chan;
        info->dma_info.dma_dev_name = cfg->dma_fifo0.dma_dev_name;
        info->dma_info.dma_id = cfg->dma_fifo0.dma_id;
    } else {
        //LOG_ERR("invalid fifo type %d", info->fifo_type);
        return -ENOENT;
    }

    return 0;
}

#ifdef CONFIG_CFG_DRV
/* @brief DAC external PA control */
static int dac_external_pa_ctl(struct device *dev, uint8_t ctrl_func)
{
    struct phy_dac_drv_data *data = dev->data;
    uint8_t i, pa_func, enable;
    gpio_flags_t flags = GPIO_OUTPUT;
    const struct device *gpio_dev = NULL;

    if ((ctrl_func != EXTERNAL_PA_ENABLE)
        && (ctrl_func != EXTERNAL_PA_DISABLE)
        && (ctrl_func != EXTERNAL_PA_MUTE)
        && (ctrl_func != EXTERNAL_PA_UNMUTE)) {
        LOG_ERR("invalid external pa ctrl:%d", ctrl_func);
        return -EINVAL;
    }

    if (ctrl_func == EXTERNAL_PA_ENABLE) {
        pa_func = EXTERN_PA_ENABLE;
        enable = true;
    } else if (ctrl_func == EXTERNAL_PA_DISABLE) {
        pa_func = EXTERN_PA_ENABLE;
        enable = false;
    }  else if (ctrl_func == EXTERNAL_PA_MUTE) {
        pa_func = EXTERN_PA_MUTE;
        enable = true;
    } else {
        pa_func = EXTERN_PA_MUTE;
		enable = false;
	}

	for (i = 0; i < ARRAY_SIZE(data->external_config.Extern_PA_Control); i++) {
		CFG_Type_Extern_PA_Control *cfg = &data->external_config.Extern_PA_Control[i];
        if (cfg->PA_Function == pa_func && cfg->GPIO_Pin != GPIO_NONE) {
			if (cfg->Pull_Up_Down != CFG_GPIO_PULL_NONE) {
				if (cfg->Pull_Up_Down == CFG_GPIO_PULL_DOWN)
					flags |= GPIO_PULL_DOWN;
			#ifdef GPIO_PULL_UP_STRONG
				else if (cfg->Pull_Up_Down == CFG_GPIO_PULL_UP_10K)
				    flags |= GPIO_PULL_UP_STRONG;
			#endif
				else
					flags |= GPIO_PULL_UP;
			}

            gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(cfg->GPIO_Pin));

            if (!gpio_dev) {
                LOG_ERR("failed to bind GPIO(%d) device", cfg->GPIO_Pin);
                return -ENODEV;
            }

            gpio_pin_configure(gpio_dev, cfg->GPIO_Pin % 32, flags);

            if (enable)
                gpio_pin_set(gpio_dev, cfg->GPIO_Pin % 32, cfg->Active_Level);
            else
                gpio_pin_set(gpio_dev, cfg->GPIO_Pin % 32, !cfg->Active_Level);
        }
    }

    return 0;
}
#endif

/* @brief prepare dac runtime resources such as clock etc. */
static int phy_dac_prepare_enable(struct device *dev, aout_param_t *out_param)
{
    dac_setting_t *dac_setting = out_param->dac_setting;
    const struct phy_dac_config_data *cfg = dev->config;

    if ((!out_param) || (!out_param->dac_setting)
        || (!out_param->sample_rate)) {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }

    if ((dac_setting->channel_mode != STEREO_MODE)
        && (dac_setting->channel_mode != MONO_MODE)) {
        LOG_ERR("Invalid channel mode %d", dac_setting->channel_mode);
        return -EINVAL;
    }

    if (!(out_param->channel_type & AUDIO_CHANNEL_DAC)) {
        LOG_ERR("Invalid channel type %d", out_param->channel_type);
        return -EINVAL;
    }

    if (out_param->outfifo_type != AOUT_FIFO_DAC0) {
        LOG_ERR("Invalid FIFO type %d", out_param->outfifo_type);
        return -EINVAL;
    }

    if (out_param->reload_setting) {
        LOG_ERR("DAC FIFO does not support reload mode");
        //return -EINVAL;
    }

    /* Enable DAC clock gate */
    acts_clock_peripheral_enable(cfg->clk_id);
    acts_clock_peripheral_enable(CLOCK_ID_DACANACLK);

    /* DAC main clock source is alway 256FS */
    if (dac_sample_rate_set(dev, out_param->sample_rate)) {
        LOG_ERR("Failed to config sample rate %d", out_param->sample_rate);
        return -ESRCH;
    }

    return 0;
}

static int phy_dac_disable_pa(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

#ifdef CONFIG_CFG_DRV
	dac_external_pa_ctl((struct device *)dev, EXTERNAL_PA_MUTE);
    dac_external_pa_ctl((struct device *)dev, EXTERNAL_PA_DISABLE);
#endif

    dac_reg->anactl0 = 0;
    dac_reg->anactl1 = 0;
    dac_reg->anactl2 = 0;

    dac_ldo_power_control(dev, false);

    acts_clock_peripheral_disable(CMU_DACCLK);

    return 0;
}

/* @brief ADC BIAS setting for power saving */
static void dac_bias_setting(struct device *dev)
{
#ifdef CONFIG_CFG_DRV
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    struct phy_dac_drv_data *data = dev->data;
    dac_reg->bias = data->external_config.DAC_Bias_Setting;
#endif
}

/* @brief Wait the DAC FIFO empty */
static int dac_wait_fifo_empty(struct device *dev, a_dac_fifo_e idx, uint32_t timeout_ms)
{
    uint32_t start_time;
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    start_time = k_cycle_get_32();

    LOG_DBG("wait DAC FIFO empty start time:%d", start_time);

    /* disable PCMBUF irqs to avoid user continuously writing data */
    if (DAC_FIFO_0 == idx) {
        dac_reg->pcm_buf_ctl &= ~DAC_PCMBUF_DEFAULT_IRQ;
    }

    while (!__is_dac_fifo_empty(dev, idx)) {
        if (k_cyc_to_us_floor32(k_cycle_get_32() - start_time)
            >= (timeout_ms * 1000)) {
            LOG_ERR("wait dac fifo(%d) empty(0x%x) timeout",
                    idx, __get_pcmbuf_avail_length(dev));
            return -ETIMEDOUT;
        }

        /* PM works in IDLE thread and not allow to sleep */
        if (!z_is_idle_thread_object(_current))
            k_sleep(K_MSEC(1));
    }

    LOG_DBG("wait DAC FIFO empty end time:%d and total use %dus",
            k_cycle_get_32(), k_cycle_get_32() - start_time);

    return 0;
}

static void dac_efuse_init(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
	unsigned int dac_efuse_val;

    if(!soc_atp_get_dac_calib(0,  &dac_efuse_val)){
        dac_reg->dac_osctl = (dac_reg->dac_osctl & 0xff00) | (dac_efuse_val & 0xff);
    }else{
        printk("read dac efuse err!\n");
    }

    
    if(!soc_atp_get_dac_calib(1,  &dac_efuse_val)){
        dac_reg->dac_osctl = (dac_reg->dac_osctl & 0x00ff) | ((dac_efuse_val & 0xff) << 8);
    }else{
        printk("read dac efuse err!\n");
    }
    
    printk("dac_osctl:0x%8x\n", dac_reg->dac_osctl);
}

static int phy_dac_enable(struct device *dev, void *param)
{
    struct phy_dac_drv_data *data = dev->data;
    const struct phy_dac_config_data *cfg = dev->config;
    aout_param_t *out_param = (aout_param_t *)param;
    dac_setting_t *dac_setting = out_param->dac_setting;
    int ret;
    uint8_t fifo_idx, sr = DAC_SR_48K, fir_mode;

    data->audiopll_mode = out_param->audiopll_mode;
    printk("%s, audiopll mode:%d\n", __func__, data->audiopll_mode);

    /* enable DAC LDO */
    dac_ldo_power_control(dev, true);

    ret = phy_dac_prepare_enable(dev, out_param);
    if (ret) {
        LOG_ERR("Failed to prepare enable dac err=%d", ret);
        return ret;
    }

    dac_efuse_init(dev);

    fifo_idx = out_param->outfifo_type;

    ret = __dac_fifo_enable(dev, FIFO_SEL_DMA,
                    (out_param->channel_width == CHANNEL_WIDTH_16BITS)
                    ? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS,
                    DAC_FIFO_DRQ_LEVEL_DEFAULT,
                    DAC_FIFO_VOL_LEVEL_DEFAULT,
                    fifo_idx, true);
    if (ret)
        return ret;

    __dac_pcmbuf_config(dev);

    sr = dac_sample_rate_to_sr(dev, out_param->sample_rate);
    if (sr < 0) {
        LOG_ERR("mapping sample rate:%d to sr error", out_param->sample_rate);
        ret = -EFAULT;
        goto err;
    }

    fir_mode = dac_get_fir_mode(dev, out_param->sample_rate);

    ret = __dac_digital_enable(dev, sr, fir_mode,
            dac_setting->channel_mode, out_param->channel_type);
    if (ret) {
        LOG_ERR("Failed to enable DAC digital err=%d", ret);
        goto err;
    }


    dac_bias_setting(dev);

    if (!PHY_DEV_FEATURE(automute) && out_param->automute_force) {
        __dac_automute_cfg(dev);
    }

    ret = dac_enable_features(dev, out_param->sample_rate);
    if (ret) {
        LOG_ERR("DAC enable features error %d", ret);
        goto err;
    }

    data->sample_rate = out_param->sample_rate;

    /* Record the PCM BUF data callback */
    data->ch[fifo_idx].callback = out_param->callback;
    data->ch[fifo_idx].cb_data = out_param->cb_data;

    LOG_DBG("DAC ch@%d register callback %p cb_data %p",
        fifo_idx, data->ch[fifo_idx].callback, data->ch[fifo_idx].cb_data);

    /* Clear FIFO ERROR */
    __dac_clear_fifo_error(dev, fifo_idx);

#ifdef DAC_ANALOG_DEBUG_IN_ENABLE
    __dac_analog_dbgi(dev);
#endif

#ifdef DAC_DIGITAL_DEBUG_OUT_ENABLE
    __dac_digital_dbgo(dev, DAC_DIGITAL_DEBUG_OUT_CHANNEL_SEL);
#endif

    ret = dac_volume_set(dev, dac_setting->volume.left_volume,
                            dac_setting->volume.right_volume,
                            out_param->sample_rate);
    if (ret)
        goto err;

    uint32_t key = irq_lock();
    /* set channel start flag */
    if (DAC_FIFO_0 == fifo_idx)
        data->ch_fifo0_start = 1;
 
#ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
    data->is_antipop_complete = 1;
#endif

    atomic_inc(&data->refcount);
    irq_unlock(key);

    return ret;

err:
    __dac_fifo_disable(dev, fifo_idx);
    return ret;
}


#ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
static u32_t antipop_process_time;
static bool is_dvfs_set = false;
struct device *anti_dev = NULL;

static void dac_single_on_antipop_start(struct device *dev)
{
	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg;

    //dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "antipop");
    //is_dvfs_set = true;
    printk("antipop start...");
	/* enable DAC clock gate */
	acts_clock_peripheral_enable(CLOCK_ID_DAC);

    /* PA/VRO over load protection function needs enable DACANCCLK */
    acts_clock_peripheral_enable(CLOCK_ID_DACANACLK);

	/* set sample rate 48K */
	dac_sample_rate_set(dev, SAMPLE_RATE_48KHZ);

	/* ldo enable */
	dac_ldo_power_control(dev, true);

    dac_reg->bias = 0x00621655;
    //[13]PA miller feedback cap:	0: large cap, small bw;  1:small cap, large bw.
    //[12]PA DFC option 1-en
    //[11:8]PAVOL 111/0-0xe-2.7V	110/0-0xc-2.2V	101/0-0xa-1.7V; [8]1-small res:100/1-0x9-2.7V	011/1-0x7-2.2V	010/1-0x5-1.7V; 
    //[7:6][5:4]PA R-L en
    //[3]zero data
    //[2:1] R-L channel en
    //[0]bias en	
    dac_reg->anactl0 = (0<<13)|(1<<12)|(7<<8)|(0x0<<4)|(1<<3)|(3<<1)|0x1; 

    //[20]SH debug out en
    //[19:12]SHCL_SET
    //[11:4]SHCL_PW
    //[3:2] 0x2-31khz
    //[1] 1-clamp op en
    //[0] 1-SHCLK en 
    dac_reg->anactl2 = (0<<20)| (0xff<<12)|(6<<4)|(0x2<<2)|(0<<1)|(0<<0); 

    /*antipop process time relate with this*/
    dac_reg->anactl1 = (0x1<<24) | (0x0<<18); //[24]-rampdat step small;  [18]-slow clk;

    /*single no-direct output*/
    /* REAMPDEN   and RAMPDINI enable, RAMPVOL:0v, init ramp data source*/
    dac_reg->anactl1 |= (0x1<<16);    //Ramp dat generat block en.

    reg = dac_reg->anactl1;
    reg &= ~(0x7<<20);
    reg |= (0x1<<20);//RAMPVOL=0v.
    dac_reg->anactl1 = reg;    
    dac_reg->anactl1 |= (0x1<<23);  //Ramp dat init set en. 

    /*wait 100us, open RAMPOPEN, close RAMPDINI*/
	k_busy_wait(100UL);
    dac_reg->anactl1 |= (0x1<<17); 	   //Ramp dat buff op en.
    dac_reg->anactl1 &= ~(0x1<<23); 	//Ramp dat init set dis.

    /*open PAEN, ATPSW1, LP2EN, ATPRCEN*/
    dac_reg->anactl0 |= (5<<4);  //PAEN.
    
    dac_reg->anactl1 |= (0x1<<12);    //PA-R Switch1 en.  
    dac_reg->anactl1 |= (0x1<<4);       //PA-L Switch1 en.      
      
    dac_reg->anactl1 |= (0x1<<8);     //PA-R Loop2 en.    
    dac_reg->anactl1 |= (0x1<<0);   //PA-L Loop2 en.
    
    dac_reg->anactl1 |= (0x1<<9);     //PA-R Ramp connect en.
    dac_reg->anactl1 |= (0x1<<1);     //PA-L Ramp connect en.

    /*RAMPVOL = PAVCC*/
    reg = dac_reg->anactl1;
    reg &= ~(0x7<<20);
    reg |= (0x6<<20);//pavcc
    dac_reg->anactl1 = reg;

}

static void dac_single_on_antipop_end(struct k_work *work)
{
	struct acts_audio_dac *dac_reg = get_dac_reg_base(anti_dev);
    struct phy_dac_drv_data *data = anti_dev->data;

    dac_reg->anactl0 |= (0xa<<4);  //PA output stage en.

    //close LP2EN,ATPRCEN,RAMPOPEN,RAMPDEN
    dac_reg->anactl1 &= ~(0x1<<8);        //PA-R Loop2 dis.   
    dac_reg->anactl1 &= ~(0x1<<0);   //PA-L Loop2  dis.
        
    dac_reg->anactl1 &= ~(0x1<<9);      //PA-R Ramp connect dis.
    dac_reg->anactl1 &= ~(0x1<<1);        //PA-L Ramp connect dis. 
    
    dac_reg->anactl1 &= ~(0x1<<17);   //Ramp dat buff op dis.
    dac_reg->anactl1 &= ~(0x1<<16);     //Ramp dat generat block dis.
    
    //complete;
    dac_reg->anactl0 &= ~DAC_ANACTL0_ZERODT; //dis zerodata.
    data->is_antipop_complete = 1;

	printk("poweron antipop process take %dus\n",
				k_cyc_to_us_floor32(k_cycle_get_32() - antipop_process_time));
}


static void dac_single_off_antipop(struct device *dev)
{
	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg;

	/* enable DAC clock gate */
	acts_clock_peripheral_enable(CLOCK_ID_DAC);

	/* set sample rate 16K */
	dac_sample_rate_set(dev, SAMPLE_RATE_16KHZ);

	/* ldo enable */
	dac_ldo_power_control(dev, true);

    dac_reg->anactl0 |= DAC_ANACTL0_ZERODT;; //dis zerodata.
    dac_reg->vol_lch = (dac_reg->vol_lch & ~VOL_LCH_VOLL_MASK) | 0xBE;
    dac_reg->vol_rch = (dac_reg->vol_rch & ~VOL_LCH_VOLL_MASK) | 0xBE;

    /*antipop process time relate with this*/
    dac_reg->anactl1 = (0x1<<24) | (0x0<<18); //[24]-rampdat step small; [18]-slow clk;

    //enable REAMPDEN and RAMPDINI,RAMPVOL:0v, init ramp data source
    dac_reg->anactl1 |= (0x1<<16);    //Ramp dat generat block en
    reg = dac_reg->anactl1;
    reg &= ~(0x7<<20);
    reg |= (0x1<<20);//RAMPVOL=0v
    dac_reg->anactl1 = reg;
    dac_reg->anactl1 |= (0x1<<23);  //Ramp dat init set en

    // wait 100us，open RAMPOPEN, close RAMPDINI
    k_busy_wait(100UL);
    dac_reg->anactl1 |= (0x1<<17);    //Ramp dat buff op en
    dac_reg->anactl1 &= ~(0x1<<23);     //Ramp dat init set dis
      
    //close PAOSEN and PAEN
    dac_reg->anactl0 &= ~(0xf<<4);  //PA output stage
        
    //open LP2EN,ATPRCEN,BCDISCH
    //dac_reg->anactl1 &= ~ (0x1<<12);    //PA-R Switch1 en, not care!
    //dac_reg->anactl1 &= ~ (0x1<<4);       //PA-L Switch1 en.      
        
    dac_reg->anactl1 |= (0x1<<8);     //PA-R Loop2 en.    
    dac_reg->anactl1 |= (0x1<<0);   //PA-L Loop2 en.
    
    dac_reg->anactl1 |= (0x1<<9);     //PA-R Ramp connect en.
    dac_reg->anactl1 |= (0x1<<1);     //PA-L Ramp connect en.
        
    dac_reg->anactl1 |= (0x1<<11);   //PA-R cap discharge en
    dac_reg->anactl1 |= (0x1<<3);     //PA-L cap discharge en
          
    //RAMPVOL:PAVCC, wait 120ms
    reg = dac_reg->anactl1;
    reg &= ~(0x7<<20);
    reg |= (0x6<<20);//pavcc
    dac_reg->anactl1 = reg; 
        
    k_busy_wait(180000UL);

    //close LP2EN,ATPRCEN,RAMPOPEN,RAMPDEN,BCDISCH
    dac_reg->anactl1 &= ~(0x1<<8);   //PA-R Loop2 dis
    dac_reg->anactl1 &= ~(0x1<<0);   //PA-L Loop2  dis

    dac_reg->anactl1 &= ~(0x1<<9);    //PA-R Ramp connect dis
    dac_reg->anactl1 &= ~(0x1<<1);        //PA-L Ramp connect dis 
    
    dac_reg->anactl1 &= ~(0x1<<17);   //Ramp dat buff op dis
    dac_reg->anactl1 &= ~(0x1<<16);     //Ramp dat generat block dis
        
    dac_reg->anactl1 &= ~(0x1<<11);     //PA-R cap discharge dis
    dac_reg->anactl1 &= ~(0x1<<3);         //PA-L cap discharge dis
}

/* @brief DAC antipop process when system power up */
static void dac_poweron_antipop_process(struct device *dev)
{
    struct phy_dac_drv_data *data = dev->data;

    if(AUDIO_OUT_MODE_DAC_NODIRECT != data->external_config.Out_Mode){
        data->is_antipop_complete = 1;
        return;
    }

	dac_single_on_antipop_start(dev);
    
	antipop_process_time = k_cycle_get_32();
    data->is_antipop_complete = 0;

    anti_dev = dev;
    k_delayed_work_submit(&data->antipop_timer, K_MSEC(120));
}

/* @brief DAC antipop process when system power off */
static void dac_poweroff_antipop_process(struct device *dev)
{
    struct phy_dac_drv_data *data = dev->data;
    u32_t times;

    if(AUDIO_OUT_MODE_DAC_NODIRECT != data->external_config.Out_Mode){
        data->is_antipop_complete = 1;
        return;
    }

	times = k_cycle_get_32();

	dac_single_off_antipop(dev);

	LOG_INF("poweroff antipop process take %dus",
				k_cyc_to_us_floor32(k_cycle_get_32() - times));
}
#endif

static int phy_dac_disable(struct device *dev, void *param);
static int __phy_dac_cmd_fifo_get(struct device *dev, void *param)
{
    const struct phy_dac_config_data *cfg = dev->config;
    struct phy_dac_drv_data *data = dev->data;
    aout_param_t *out_param = (aout_param_t *)param;
    uint8_t fifo_type = out_param->outfifo_type;
    bool fifo_mix_en = true;
    int ret = 0;
    
    if (!out_param)
        return -EINVAL;
    
    if (fifo_type != AOUT_FIFO_DAC0) {
        LOG_ERR("Invalid FIFO type %d", fifo_type);
        return -EINVAL;
    }

    if (__is_dac_fifo_working(dev, fifo_type)) {
        LOG_ERR("DAC FIFO(%d) now is using", out_param->outfifo_type);
        return -EBUSY;
    }
    
    /* reset dac module */
    if (fifo_type == AOUT_FIFO_DAC0)
        acts_reset_peripheral(cfg->rst_id);
    
    /* enable dac clock */
    acts_clock_peripheral_enable(cfg->clk_id);
    
    __dac_fifo_enable(dev, FIFO_SEL_DMA,
                    (out_param->channel_width == CHANNEL_WIDTH_16BITS)
                    ? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS,
                    DAC_FIFO_DRQ_LEVEL_DEFAULT,
                    DAC_FIFO_VOL_LEVEL_DEFAULT,
                    fifo_type, fifo_mix_en);
    
    if (AOUT_FIFO_DAC0 == out_param->outfifo_type) {
        __dac_pcmbuf_config(dev);
        data->ch[fifo_type].fifo_cnt = 0;
        data->ch[fifo_type].fifo_cnt_timestamp = 0;
        /* Record the PCM BUF data callback */
        data->ch[fifo_type].callback = out_param->callback;
        data->ch[fifo_type].cb_data = out_param->cb_data;
        LOG_DBG("Enable PCMBUF callback:%p", data->ch[fifo_type].callback);
    
        if (AOUT_FIFO_DAC0 == out_param->outfifo_type)
            data->ch_fifo0_start = 1;
    }
    
    dac_setting_t *dac_setting = out_param->dac_setting;
    if (dac_setting) {
        if (dac_setting->channel_mode == MONO_MODE)
            __dac_digital_enable_mono(dev);
    
        ret = dac_volume_set(dev, dac_setting->volume.left_volume,
            dac_setting->volume.right_volume, out_param->sample_rate);
    }
    return ret;
}

static int __phy_cmd_sel_dac_en_chan(struct device *dev, void *param)
{
    struct phy_dac_drv_data *data = dev->data;
    int ret = 0;

    uint8_t lr_sel = *(uint8_t *)param;
    /* Select both left and right channels to enable */
    if (lr_sel == (LEFT_CHANNEL_SEL | RIGHT_CHANNEL_SEL)) {
        if (data->lr_sel != lr_sel) {
            LOG_ERR("DAC DTS lr sel:%d conflict", data->lr_sel);
            ret = -EPERM;
        }
    } else if (lr_sel == LEFT_CHANNEL_SEL) { /* Only select left channel to enable */
        if (data->lr_sel & RIGHT_CHANNEL_SEL)
            ret = __dac_analog_disable(dev, RIGHT_CHANNEL_SEL);
    } else if (lr_sel == RIGHT_CHANNEL_SEL) { /* Only select right channel to enable */
        if (data->lr_sel & LEFT_CHANNEL_SEL)
            ret = __dac_analog_disable(dev, LEFT_CHANNEL_SEL);
    } else {
        LOG_ERR("invalid lr sel:%d", lr_sel);
        ret = -EINVAL;
    }
    return ret;
}

static int __phy_dac_cmd_fifo_reset_sample_cnt(struct device *dev, void *param)
{
    struct phy_dac_drv_data *data = dev->data;
    uint8_t idx = *(uint8_t *)param;
    int ret = 0;

    if (DAC_FIFO_INVALID_INDEX(idx)) {
        LOG_ERR("invalid fifo index %d", idx);
        return -EINVAL;
    }
    
    uint32_t key = irq_lock();
    
    __dac_fifo_counter_reset(dev, idx);
    
    if (AOUT_FIFO_DAC0 == idx) {
        data->ch[0].fifo_cnt = 0;
        data->ch[0].fifo_cnt_timestamp = 0;
    } else {
        LOG_ERR("invalid fifo idx %d", idx);
    }
    
    irq_unlock(key);
    return ret;
}

static int __phy_fifo_disable_sample_cnt(struct device *dev, void *param)
{
    struct phy_dac_drv_data *data = dev->data;
    uint8_t idx = *(uint8_t *)param;
    int ret = 0;
    
    if (DAC_FIFO_INVALID_INDEX(idx)) {
        LOG_ERR("invalid fifo index %d", idx);
        return -EINVAL;
    }
    
    uint32_t key = irq_lock();
    
    __dac_fifo_counter_disable(dev, idx);
    
    if (AOUT_FIFO_DAC0 == idx) {
        data->ch[0].fifo_cnt = 0;
        data->ch[0].fifo_cnt_timestamp = 0;
    } else {
        data->ch[1].fifo_cnt = 0;
        data->ch[1].fifo_cnt_timestamp = 0;
    }
    irq_unlock(key);

    return ret;
}

static int __phy_cmd_fifo_put(struct device *dev, void *param)
{
    struct phy_dac_drv_data *data = dev->data;
    uint8_t idx = *(uint8_t *)param;

    if (__is_dac_fifo_working(dev, idx)) {
        dac_wait_fifo_empty(dev, (a_dac_fifo_e)idx,
            DAC_WAIT_FIFO_EMPTY_TIMEOUT_MS);
        __dac_fifo_disable(dev, idx);
        __dac_digital_disable_fifo(dev, idx);
        if (AOUT_FIFO_DAC0 == idx)
            data->ch_fifo0_start = 0;
    }
    return 0;
}
#ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
static uint8_t __phy_cmd_get_antipop_stage(struct device *dev)
{
    struct phy_dac_drv_data *data = dev->data;
    uint8_t stage;

    if(data->external_config.AntiPOP_Process_Disable == 0){
        stage = data->is_antipop_complete;
        if(data->is_antipop_complete == 1){
            if(is_dvfs_set){
                //dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "antipop");
                //is_dvfs_set = false;
            }
        }
    }else{
        stage = 1;
    }
    return stage;
}
#endif
static int __phy_fifo_cmd_process(struct device *dev, uint32_t cmd, void *param)
{
    struct phy_dac_drv_data *data = dev->data;
    int ret = 0;

    switch (cmd) {
        case AOUT_CMD_GET_VOLUME:
        {
            uint16_t level;
            volume_setting_t *volume = (volume_setting_t *)param;
            level = __dac_volume_get(dev, LEFT_CHANNEL_SEL);
            volume->left_volume = dac_volume_level_to_db(level);
            level = __dac_volume_get(dev, RIGHT_CHANNEL_SEL);
            volume->right_volume = dac_volume_level_to_db(level);
            LOG_INF("Get volume [%d, %d]", volume->left_volume, volume->right_volume);
            break;
        }
        case AOUT_CMD_SET_VOLUME:
        {
            volume_setting_t *volume = (volume_setting_t *)param;
            ret = dac_volume_set(dev, volume->left_volume,
                    volume->right_volume, data->sample_rate);
            if (ret) {
                LOG_ERR("Volume set[%d, %d] error:%d",
                    volume->left_volume, volume->right_volume, ret);
                return ret;
            }
            break;
        }
        case AOUT_CMD_GET_CHANNEL_STATUS:
        {
            uint8_t idx = *(uint8_t *)param;
            if (DAC_FIFO_INVALID_INDEX(idx)) {
                LOG_ERR("invalid fifo index %d", idx);
                return -EINVAL;
            }
            if (__check_dac_fifo_error(dev, idx))
                *(uint8_t *)param |= AUDIO_CHANNEL_STATUS_ERROR;
        
            if (!__is_dac_fifo_empty(dev, idx))
                *(uint8_t *)param |= AUDIO_CHANNEL_STATUS_BUSY;
        
            *(uint8_t *)param = 0;
            break;
        }
        case AOUT_CMD_GET_FIFO_LEN:
        {
            *(uint32_t *)param = DAC_PCMBUF_MAX_CNT;
            break;
        }
        case AOUT_CMD_GET_FIFO_AVAILABLE_LEN:
        {
            *(uint32_t *)param = __get_pcmbuf_avail_length(dev);
            break;
        }
        case AOUT_CMD_GET_APS:
        {
            ret = dac_get_pll_aps(dev);
            if (ret < 0) {
                LOG_ERR("Failed to get audio pll APS err=%d", ret);
                return ret;
            }
            *(audio_aps_level_e *)param = (audio_aps_level_e)ret;
            ret = 0;
            break;
        }
        case AOUT_CMD_GET_APS_F:
        {
            ret = dac_get_pll_aps_f(dev);
            if (ret < 0) {
                LOG_ERR("Failed to get audio pll APS err=%d", ret);
                return ret;
            }
            *(unsigned int *)param = (unsigned int)ret;
            ret = 0;
            break;
        }
        case AOUT_CMD_SET_DAC_THRESHOLD:
        {
            dac_threshold_setting_t *thres = (dac_threshold_setting_t *)param;
            ret = __dac_pcmbuf_threshold_update(dev, thres);
            break;
        }

        case AOUT_CMD_GET_DAC_SDM_SAMPLE_CNT:
        {
            uint32_t val = __dac_read_sdm_counter(dev);
            *(uint32_t *)param = data->sdm_cnt + val;
            break;
        }
        case AOUT_CMD_RESET_DAC_SDM_SAMPLE_CNT:
        {
            uint32_t key = irq_lock();
            __dac_sdm_counter_reset(dev);
            data->sdm_cnt = 0;
            data->sdm_cnt_timestamp = 0;
            irq_unlock(key);

            break;
        }
        case AOUT_CMD_ENABLE_DAC_SDM_SAMPLE_CNT:
        {
            uint32_t key = irq_lock();
            __dac_sdm_counter_enable(dev);
            irq_unlock(key);
            break;
        }
        case AOUT_CMD_DISABLE_DAC_SDM_SAMPLE_CNT:
        {
            uint32_t key = irq_lock();
            __dac_sdm_counter_disable(dev);
            data->sdm_cnt = 0;
            data->sdm_cnt_timestamp = 0;
            irq_unlock(key);
            break;
        }
        case AOUT_CMD_GET_DAC_SDM_STABLE_SAMPLE_CNT:
        {
            *(uint32_t *)param =  __dac_read_sdm_stable_counter(dev);
            break;
        }
        default:
            LOG_ERR("Unsupport command %d", cmd);
            ret = -ENOTSUP;

    }
    return ret;
}

static int __phy_cmd_set_aps(struct device *dev, void *param)
{
    int ret = 0;

    audio_aps_level_e level = *(audio_aps_level_e *)param;
    ret = dac_set_pll_aps(dev, level);
    if (ret) {
        LOG_ERR("Failed to set audio pll APS err=%d", ret);
        return ret;
    }
    LOG_DBG("set new aps level %d", level);
    return ret;
}

static int __phy_cmd_set_aps_f(struct device *dev, void *param)
{
    int ret = 0;

    unsigned int level = *(unsigned int *)param;
    ret = dac_set_pll_aps_f(dev, level);
    if (ret) {
        LOG_ERR("Failed to set audio pll APS err=%d", ret);
        return ret;
    }
    LOG_DBG("set new aps level %d", level);
    return ret;
}


static int __phy_cmd_get_fifo_sample_cnt(struct device *dev, void *param)
{
    struct phy_dac_drv_data *data = dev->data;
    uint32_t val;
    uint32_t idx = *(uint32_t *)param;

    if (DAC_FIFO_INVALID_INDEX(idx)) {
        LOG_ERR("invalid fifo index %d", idx);
        return -EINVAL;
    }

    val = __dac_read_fifo_counter(dev, idx);

    if (AOUT_FIFO_DAC0 == idx)
        *(uint32_t *)param = val + data->ch[0].fifo_cnt;
    else
        *(uint32_t *)param = val + data->ch[1].fifo_cnt;

    LOG_DBG("DAC FIFO counter: %d", *(uint32_t *)param);
    return 0;

}

static int __phy_cmd_ioctl(struct device *dev, uint32_t cmd, void *param)
{
#ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
    struct phy_dac_drv_data *data = dev->data;
#endif
    int ret = 0;

    switch (cmd) {
        case AOUT_CMD_GET_SAMPLERATE:
        {
            *(audio_sr_sel_e *)param = dac_sample_rate_get(dev);
            break;
        }
        case AOUT_CMD_SET_SAMPLERATE:
        {
            audio_sr_sel_e val = *(audio_sr_sel_e *)param;
            ret = dac_sample_rate_set(dev, val);
            if (ret) {
                LOG_ERR("Failed to set DAC sample rate err=%d", ret);
                return ret;
            }
            break;
        }
        case AOUT_CMD_OPEN_PA:
        {
            
#ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
            if(data->external_config.AntiPOP_Process_Disable == 0){
                dac_poweron_antipop_process(dev);
            }else{
                data->is_antipop_complete = 1;
            }
#endif
    
#ifdef CONFIG_CFG_DRV
            dac_external_pa_ctl((struct device *)dev, EXTERNAL_PA_ENABLE);
            dac_external_pa_ctl((struct device *)dev, EXTERNAL_PA_UNMUTE);
#endif
            break;
        }
        case AOUT_CMD_CLOSE_PA:
        {
        #ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
            if(data->external_config.AntiPOP_Process_Disable == 0)
                dac_poweroff_antipop_process(dev);
        #endif
            ret = phy_dac_disable_pa(dev);
            break;
        }
    
        case AOUT_CMD_EXTERNAL_PA_CONTROL:
        {
#ifdef CONFIG_CFG_DRV
            uint8_t ctrl_func = *(uint8_t *)param;
            ret = dac_external_pa_ctl(dev, ctrl_func);
#else
            ret = -ENOTSUP;
#endif
            break;
        }
        case AOUT_CMD_SET_FIFO_SRC:
        {
            dac_fifosrc_setting_t *fifosrc = (dac_fifosrc_setting_t *)param;
            ret = __dac_fifo_update_src(dev, fifosrc);
            break;
        }
        case AOUT_CMD_SET_DAC_TRIGGER_SRC:
        {
            uint8_t src = *(uint8_t *)param;
            ret = __dac_external_trigger_enable(dev, src);
            break;
        }
        case AOUT_CMD_SELECT_DAC_ENABLE_CHANNEL:
        {
            ret = __phy_cmd_sel_dac_en_chan(dev, param);
            break;
        }
        case AOUT_CMD_DAC_FORCE_START:
        {
            dac_ext_trigger_ctl_t *trigger_ctl = (dac_ext_trigger_ctl_t *)param;
            __dac_digital_force_start(dev, trigger_ctl);
            break;
        }
        
        case AOUT_CMD_DAC_TRIGGER_CONTROL:
        {
            dac_ext_trigger_ctl_t *trigger_ctl = (dac_ext_trigger_ctl_t *)param;
            ret = __dac_external_trigger_control(dev, trigger_ctl);
            break;
        }
        default:
            LOG_ERR("Unsupport command %d", cmd);
            ret = -ENOTSUP;
            
    }
    return ret;
}

static int __phy_base_cmd_ioctl(struct device *dev, uint32_t cmd, void *param)
{
    int ret = 0;

    switch (cmd) {
        case PHY_CMD_FIFO_GET:
        {
            ret = __phy_dac_cmd_fifo_get(dev, param);
            break;
        }
        case PHY_CMD_FIFO_PUT:
        {
            ret = __phy_cmd_fifo_put(dev, param);
            break;
        }
        case PHY_CMD_FIFO_DRQ_LEVEL_GET:
        {
            uint32_t fifo_cmd = *(uint32_t *)param;
            uint8_t fifo_idx = PHY_GET_FIFO_CMD_INDEX(fifo_cmd);

            ret = __dac_fifo_drq_level_get(dev, fifo_idx);
            if (ret < 0) {
                LOG_ERR("Get FIFO(%d) drq level error", fifo_idx);
            return ret;
            }

            *(uint32_t *)param = PHY_FIFO_CMD(fifo_idx, ret);
            ret = 0;

            break;
        }
        case PHY_CMD_FIFO_DRQ_LEVEL_SET:
        {
            uint32_t fifo_cmd = *(uint32_t *)param;
            uint8_t fifo_idx = PHY_GET_FIFO_CMD_INDEX(fifo_cmd);
            uint8_t level = PHY_GET_FIFO_CMD_VAL(fifo_cmd);

            ret = __dac_fifo_drq_level_set(dev, fifo_idx, level);

            break;
        }
        case PHY_CMD_GET_AOUT_DMA_INFO:
        {
            ret = dac_get_dma_info(dev, (struct audio_out_dma_info *)param);
            break;
        }
        
        case PHY_CMD_GET_FIFO_WORKING:
        {
            uint8_t idx = *(uint8_t *)param;
                
            if (__is_dac_fifo_working(dev, idx))
                ret = 1;
            else
                ret = 0;
            break;
        }
            
#ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
        case PHY_CMD_GET_ANTIPOP_STAGE:
        {
            *(uint8_t *)param = __phy_cmd_get_antipop_stage(dev);
            ret = 0;
            break;
        }
#endif
        case PHY_CMD_DUMP_REGS:
        {
            dac_dump_register(dev);
            break;
        }
        default:
            LOG_ERR("Unsupport command %d", cmd);
            ret = -ENOTSUP;
    }
    return ret;
}

static int __phy_dac_base_cmd_ioctl(struct device *dev, uint32_t cmd, void *param)
{
    const struct phy_dac_config_data *cfg = dev->config;
    int ret = 0;

    switch (cmd) {
        case PHY_CMD_DAC_FIFO_RESET_SAMPLE_CNT:
        {
            ret = __phy_dac_cmd_fifo_reset_sample_cnt(dev, param);
            break;
        }
        case PHY_CMD_DAC_FIFO_ENABLE_SAMPLE_CNT:
        {
            uint8_t idx = *(uint8_t *)param;

            if (DAC_FIFO_INVALID_INDEX(idx)) {
                LOG_ERR("invalid fifo index %d", idx);
                return -EINVAL;
            }

            uint32_t key = irq_lock();
            __dac_fifo_counter_enable(dev, idx);
            irq_unlock(key);

            break;
        }
        case PHY_CMD_DAC_FIFO_DISABLE_SAMPLE_CNT:
        {
            ret = __phy_fifo_disable_sample_cnt(dev, param);
            break;
        }
        case PHY_CMD_DAC_FIFO_VOLUME_GET:
        case PHY_CMD_DAC_FIFO_VOLUME_SET:
        {

            break;
        }

        case PHY_CMD_DAC_WAIT_EMPTY:
        {
            uint8_t fifo_idx = *(uint8_t *)param;
            __wait_dac_fifo_empty(dev, fifo_idx);
            break;
        }
        case PHY_CMD_CLAIM_WITH_128FS:
        {
            acts_clock_peripheral_enable(cfg->clk_id);
            __dac_digital_claim_128fs(dev, true);
            break;
        }
        case PHY_CMD_CLAIM_WITHOUT_128FS:
        {
            __dac_digital_claim_128fs(dev, false);
            break;
        }

        default:
            LOG_ERR("Unsupport command %d", cmd);
            ret = -ENOTSUP;
    }
    return ret;
}

static int phy_dac_ioctl(struct device *dev, uint32_t cmd, void *param)
{
    if(cmd == AOUT_CMD_SET_APS){
        return __phy_cmd_set_aps(dev, param);
    }

    if(cmd == AOUT_CMD_SET_APS_F){
        return __phy_cmd_set_aps_f(dev, param);
    }

    if(cmd == PHY_CMD_DAC_FIFO_GET_SAMPLE_CNT){
        return __phy_cmd_get_fifo_sample_cnt(dev, param);
    }

    if(cmd & AOUT_FIFO_CMD_FLAG){
        return __phy_fifo_cmd_process(dev, cmd, param);
    }

    if(cmd < PHY_CMD_BASE){
        return __phy_cmd_ioctl(dev, cmd, param);
    }else if(cmd < PHY_CMD_DAC_BASE){
        return __phy_base_cmd_ioctl(dev, cmd, param);
    }else{
        return __phy_dac_base_cmd_ioctl(dev, cmd, param);
    }

    return 0;

}

static int phy_dac_disable(struct device *dev, void *param)
{
    struct phy_dac_drv_data *data = dev->data;
    uint8_t fifo_idx = *(uint8_t *)param;

    if (fifo_idx != AOUT_FIFO_DAC0) {
        LOG_ERR("Invalid FIFO index %d", fifo_idx);
        return -EINVAL;
    }

    uint32_t key = irq_lock();
    /* set channel stop flag and DAC FIFO0 is the main control channel */
    if (fifo_idx == AOUT_FIFO_DAC0)
        data->ch_fifo0_start = 0;

    atomic_dec(&data->refcount);
    if (atomic_get(&data->refcount) != 1) {
        LOG_INF("DAC disable refcount:%d", data->refcount);
		//__dac_fifo_reset(dev, fifo_idx);
        irq_unlock(key);
        return 0;
    }

    irq_unlock(key);

    /* Timeout to wait DAC FIFO empty */
    if ((fifo_idx == AOUT_FIFO_DAC0) && __dac_is_digital_working(dev))
        dac_wait_fifo_empty(dev, fifo_idx, DAC_WAIT_FIFO_EMPTY_TIMEOUT_MS);

    if (AOUT_FIFO_DAC0 == fifo_idx) {
        __dac_digital_disable_fifo(dev, DAC_FIFO_0);
        __dac_fifo_disable(dev, DAC_FIFO_0);
        __dac_fifo_counter_disable(dev, DAC_FIFO_0);
        memset(&data->ch[0], 0, sizeof(struct phy_dac_channel));
    } else {
        LOG_ERR("invalid fifo idx %d", fifo_idx);
    }

    /* check if all dac fifos are free */
    if (__is_dac_fifo_all_free(dev, true)) {
        data->sample_rate = 0;
        __dac_external_trigger_disable(dev);
        __dac_sdm_counter_disable(dev);

#ifdef CONFIG_CFG_DRV
        if (data->external_config.Keep_DA_Enabled_When_Play_Pause)
            __dac_mute_control(dev, true);
        else
            __dac_analog_disable(dev, LEFT_CHANNEL_SEL | RIGHT_CHANNEL_SEL);
#else
#if (CONFIG_AUDIO_DAC_POWER_PREFERRED == 1)
        struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
        dac_reg->anactl0 = 0;
        dac_reg->anactl1 = 0;
        dac_reg->anactl2 = 0;
        dac_ldo_power_control(dev, false);
#else
        __dac_mute_control(dev, true);
#endif
#endif
        __dac_digital_disable(dev);
    }

    return 0;
}

const struct phy_audio_driver_api phy_dac_drv_api = {
    .audio_enable = phy_dac_enable,
    .audio_disable = phy_dac_disable,
    .audio_ioctl = phy_dac_ioctl
};

/* dump dac device tree infomation */
static void __dac_dt_dump_info(const struct phy_dac_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
    printk("**     DAC BASIC INFO     **\n");
    printk("     BASE: %08x\n", cfg->reg_base);
    printk("   CLK-ID: %08x\n", cfg->clk_id);
    printk("   RST-ID: %08x\n", cfg->rst_id);
    printk("DMA0-NAME: %s\n", cfg->dma_fifo0.dma_dev_name);
    printk("  DMA0-ID: %08x\n", cfg->dma_fifo0.dma_id);
    printk("  DMA0-CH: %08x\n", cfg->dma_fifo0.dma_chan);

    printk("** 	DAC FEATURES	 **\n");
    printk("    LAYOUT: %d\n", PHY_DEV_FEATURE(layout));
    printk("    LR-MIX: %d\n", PHY_DEV_FEATURE(dac_lr_mix));
    printk("       SDM: %d\n", PHY_DEV_FEATURE(noise_detect_mute));
    printk("  AUTOMUTE: %d\n", PHY_DEV_FEATURE(automute));
    printk("  LOOPBACK: %d\n", PHY_DEV_FEATURE(loopback));
    printk(" LEFT-MUTE: %d\n", PHY_DEV_FEATURE(left_mute));
    printk("RIGHT-MUTE: %d\n", PHY_DEV_FEATURE(right_mute));
    printk("    AM-IRQ: %d\n", PHY_DEV_FEATURE(am_irq));
#endif
}

/** @brief DAC digital IRQ routine
 * DAC digital IRQ source as below:
 *	- PCMBUF full IRQ/PD
 *	- PCMBUF half full IRQ/PD
 *	- PCMBUF half empty IRQ/PD
 *	- PCMBUF empty IRQ/PD
 *	- DACFIFO0 half empty IRQ/PD
 *	- DACFIFO1 half empty IRQ/PD
 */
static void phy_dac_fifo_isr(const void *arg)
{
    struct device *dev = (struct device *)arg;
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    struct phy_dac_drv_data *data = dev->data;
    uint32_t stat, pending = 0;

    //audio_debug_trace_start();

    stat = dac_reg->pcm_buf_stat;

    LOG_DBG("pcmbuf ctl:0x%x stat:0x%x", dac_reg->pcm_buf_ctl, stat);

    /* PCMBUF empty IRQ pending */
    if ((stat & PCM_BUF_STAT_PCMBEIP)
        && (dac_reg->pcm_buf_ctl & PCM_BUF_CTL_PCMBEPIE)) {
        pending |= AOUT_DMA_IRQ_TC;
        dac_reg->pcm_buf_ctl &= ~PCM_BUF_CTL_PCMBEPIE;
        
    }

    /* PCMBUF half empty IRQ pending */
    if (stat & PCM_BUF_STAT_PCMBHEIP) {
        pending |= AOUT_DMA_IRQ_HF;
        /* Wait until there is half empty irq happen and then start to detect empty irq */
        if (!(dac_reg->pcm_buf_ctl & PCM_BUF_CTL_PCMBEPIE))
            dac_reg->pcm_buf_ctl |= PCM_BUF_CTL_PCMBEPIE;
        
    }

    if (stat & PCM_BUF_STAT_IRQ_MASK)
        dac_reg->pcm_buf_stat = stat;

    if (pending) {
        if ((dac_reg->digctl & DAC_DIGCTL_DAF0M2DAEN)
            && data->ch[0].callback && data->ch_fifo0_start) {
            data->ch[0].callback(data->ch[0].cb_data, pending);
        }
        #if 0
        if ((dac_reg->digctl & DAC_DIGCTL_DAF1M2DAEN)
            && data->ch[1].callback
            //&& data->ch_fifo1_start
            && data->ch_fifo0_start) { /* DAC FIFO1 can not work without DAC FIFO0 */
            data->ch[1].callback(data->ch[1].cb_data, pending);
        }
        #endif
    }

    //audio_debug_trace_end();
}

/** @brief DAC FIFO IRQ routine
 * DAC FIFO IRQ source as below:
 *	- VOLL set IRQ/PD
 *	- VOLR IRQ/PD
 *	- FIFO1 CNT OF IRQ/PD
 *	- PCMBUF CNT OF IRQ/PD
 *	- SDM_SAMPLES CNT OF IRQ/PD
 *	- AUTO_MUTE_CTL[0].AMEN IRQ/PD
 *	- DAC_DIGCTL[30].SMC IRQ/PD
 */
static void phy_dac_digital_isr(const void *arg)
{
    struct device *dev = (struct device *)arg;
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    struct phy_dac_drv_data *data = dev->data;
    uint32_t timestamp;

    LOG_DBG("pcmbuf_cnt:0x%x sdm_cnt 0x%x",
            dac_reg->pcm_buf_cnt, dac_reg->sdm_samples_cnt);

    /* DAC auto mute detect pending */
    if (dac_reg->auto_mute_ctl & AUTO_MUTE_CTL_AMPD_IN)
        dac_reg->auto_mute_ctl |= AUTO_MUTE_CTL_AMPD_IN;

    /* PCMBUF sample counter overflow irq pending */
    if (dac_reg->pcm_buf_cnt & PCM_BUF_CNT_IP) {
        data->ch[0].fifo_cnt += (AOUT_FIFO_CNT_MAX + 1);

        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IP;

        if (data->ch[0].fifo_cnt_timestamp) {
            if (k_cyc_to_us_floor32(k_cycle_get_32() - data->ch[0].fifo_cnt_timestamp)
                < DAC_FIFO_CNT_MAX_SAME_SAMPLES_TIME_US) {
                __dac_fifo_counter_reset(dev, DAC_FIFO_0);
                data->ch[0].fifo_cnt_timestamp = 0;
            }
        }
        data->ch[0].fifo_cnt_timestamp = k_cycle_get_32();

        timestamp = k_cycle_get_32();
        while (dac_reg->pcm_buf_cnt & PCM_BUF_CNT_IP) {
            dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IP;
            if (k_cyc_to_us_floor32(k_cycle_get_32() - timestamp)
                > DAC_FIFO_CNT_CLEAR_PENDING_TIME_US) {
                LOG_ERR("failed to clear DAC FIFO0 PD:0x%x", dac_reg->pcm_buf_cnt);
                __dac_fifo_counter_reset(dev, DAC_FIFO_0);
            }
        }
    }

    /* SDM sample counter overflow irq pending */
    if (dac_reg->sdm_samples_cnt & SDM_SAMPLES_CNT_IP) {
        data->sdm_cnt += (AOUT_SDM_CNT_MAX + 1);

        dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_IP;

        timestamp = k_cycle_get_32();
        while (dac_reg->sdm_samples_cnt & SDM_SAMPLES_CNT_IP) {
            dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_IP;
            if (k_cyc_to_us_floor32(k_cycle_get_32() - timestamp
                > DAC_FIFO_CNT_CLEAR_PENDING_TIME_US)) {
                LOG_ERR("failed to clear SDM CNT:0x%x", dac_reg->sdm_samples_cnt);
                __dac_sdm_counter_reset(dev);
            }
        }

        if (data->sdm_cnt_timestamp) {
            if (k_cyc_to_us_floor32(k_cycle_get_32() - data->sdm_cnt_timestamp)
                < DAC_FIFO_CNT_MAX_SAME_SAMPLES_TIME_US) {
                __dac_sdm_counter_reset(dev);
                data->sdm_cnt_timestamp = 0;
            }
        }
        data->sdm_cnt_timestamp = k_cycle_get_32();
    }
}

#ifdef CONFIG_CFG_DRV
/* @brief initialize DAC external configuration */
static int phy_dac_config_init(const struct device *dev)
{
    struct phy_dac_drv_data *data = dev->data;
    int ret;
    uint8_t i;

    /* CFG_Struct_Audio_Settings */
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_OUT_MODE, Out_Mode);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_DAC_BIAS_SETTING, DAC_Bias_Setting);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_KEEP_DA_ENABLED_WHEN_PLAY_PAUSE, Keep_DA_Enabled_When_Play_Pause);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ANTIPOP_PROCESS_DISABLE, AntiPOP_Process_Disable);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_PA_GAIN, Pa_Vol);

    /* external PA pins */
    ret = cfg_get_by_key(ITEM_AUDIO_EXTERN_PA_CONTROL,
            &data->external_config.Extern_PA_Control, sizeof(data->external_config.Extern_PA_Control));
    if (ret) {
        for (i = 0; i < ARRAY_SIZE(data->external_config.Extern_PA_Control); i++) {
            LOG_INF("** External PA Pin@%d Info **", i);
            LOG_INF("PA_Function:%d", data->external_config.Extern_PA_Control[i].PA_Function);
            LOG_INF("GPIO_Pin:%d", data->external_config.Extern_PA_Control[i].GPIO_Pin);
            LOG_INF("Pull_Up_Down:%d", data->external_config.Extern_PA_Control[i].Pull_Up_Down);
            LOG_INF("Active_Level:%d", data->external_config.Extern_PA_Control[i].Active_Level);
        }

        dac_external_pa_ctl((struct device *)dev, EXTERNAL_PA_ENABLE);
    }

    return 0;
}
#endif

/* physical dac initialization */
static int phy_dac_init(const struct device *dev)
{
    const struct phy_dac_config_data *cfg = dev->config;
    struct phy_dac_drv_data *data = dev->data;

    __dac_dt_dump_info(cfg);

    /* reset DAC controller */
    acts_reset_peripheral(cfg->rst_id);

    memset(data, 0, sizeof(struct phy_dac_drv_data));

    atomic_set(&data->refcount, 1);

#ifdef CONFIG_CFG_DRV
    int ret;
    ret = phy_dac_config_init(dev);
    if (ret)
        LOG_ERR("DAC external config init error:%d", ret);
#endif
    
#ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
        k_delayed_work_init(&data->antipop_timer, dac_single_on_antipop_end);
#endif

    if (cfg->irq_config)
        cfg->irq_config();

    printk("DAC init successfully\n");
    return 0;
}

static void phy_dac_irq_config(void);

/* physical dac driver data */
static struct phy_dac_drv_data phy_dac_drv_data0;

/* physical dac config data */
static const struct phy_dac_config_data phy_dac_config_data0 = {
    .reg_base = AUDIO_DAC_REG_BASE,
    AUDIO_DMA_FIFO_DEF(DAC, 0),
    .clk_id = CLOCK_ID_DAC,
    .rst_id = RESET_ID_DAC,
    .irq_config = phy_dac_irq_config,

    PHY_DEV_FEATURE_DEF(layout) = CONFIG_AUDIO_DAC_0_LAYOUT,
    PHY_DEV_FEATURE_DEF(dac_lr_mix) = CONFIG_AUDIO_DAC_0_LR_MIX,
    PHY_DEV_FEATURE_DEF(noise_detect_mute) = CONFIG_AUDIO_DAC_0_NOISE_DETECT_MUTE,
    PHY_DEV_FEATURE_DEF(automute) = CONFIG_AUDIO_DAC_0_AUTOMUTE,
    PHY_DEV_FEATURE_DEF(loopback) = CONFIG_AUDIO_DAC_0_LOOPBACK,
    PHY_DEV_FEATURE_DEF(left_mute) = CONFIG_AUDIO_DAC_0_LEFT_MUTE,
    PHY_DEV_FEATURE_DEF(right_mute) = CONFIG_AUDIO_DAC_0_RIGHT_MUTE,
    PHY_DEV_FEATURE_DEF(pa_vol) = CONFIG_AUDIO_DAC_0_PA_VOL,
    PHY_DEV_FEATURE_DEF(am_irq) = CONFIG_AUDIO_DAC_0_AM_IRQ,
};

#if IS_ENABLED(CONFIG_AUDIO_DAC_0)
DEVICE_DEFINE(dac0, CONFIG_AUDIO_DAC_0_NAME, phy_dac_init, NULL,
        &phy_dac_drv_data0, &phy_dac_config_data0,
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_dac_drv_api);
#endif

static void phy_dac_irq_config(void)
{
    /* Connect and enable DAC digital IRQ */
    IRQ_CONNECT(IRQ_ID_DAC, CONFIG_AUDIO_DAC_0_IRQ_PRI,
            phy_dac_digital_isr,
            DEVICE_GET(dac0), 0);
    irq_enable(IRQ_ID_DAC);

    /* Connect and enable DAC FIFO IRQ */
    IRQ_CONNECT(IRQ_ID_DACFIFO, CONFIG_AUDIO_DAC_0_IRQ_PRI,
            phy_dac_fifo_isr,
            DEVICE_GET(dac0), 0);
    irq_enable(IRQ_ID_DACFIFO);
}

