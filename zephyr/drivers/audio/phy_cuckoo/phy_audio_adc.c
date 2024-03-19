/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio ADC physical implementation
 */

/*
 * Features
 *	- Support 2 independent channels with high performance ADC.
 *	- ADC0/ADC1 support ANC
 *    - 1 x  FIFOs (ADC0/ADC1 uses FIFO0(4 * 16 level * 24bits))
 *    - Support single ended and full differential input
 *    - Support 2 DMIC input
 *	- Programmable HPF
 *	- Support 2 different kinds of frequency response curves
 *    - Sample rate support 8k/12k/11.025k/16k/22.05k/24k/32k/44.1k/48k/88.2k/96k
 */

/*
 * Signal List
 * 	- AVCC: Analog power
 *	- AGND: Analog ground
 *	- INPUT0P: Analog input for ADC0 or differential input ADC0 INPUT0N
 *	- INPUT0N: Analog input to ADC1 or differential input ADC0 INPUT0P
 *	- INPUT1P: Analog input to ADC0/1 or differential input ADC1 INPUT1N
 *	- INPUT1N: Analog input to ADC1 or differential input ADC1 INPUT1P
 *	- DMIC_CLK: DMIC clk output
 *	- DMIC_DATA: DMIC data input
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
#include <drivers/audio/audio_in.h>

#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(adc0, CONFIG_LOG_DEFAULT_LEVEL);

/***************************************************************************************************
 * ADC_DEBUG
 */
#define DEBUGSEL                                               (0x40068400)
#define DEBUGIE0                                               (0x40068410)
#define DEBUGOE0                                               (0x40068420)
#define DEBUGSEL_DBGSE_SHIFT                                   (0)
#define DEBUGSEL_DBGSE_MASK                                    (0x3F << DEBUGSEL_DBGSE_SHIFT)
#define DEBUGSEL_DBGSE(x)                                      ((x) << DEBUGSEL_DBGSE_SHIFT)
#define DBGSE_ADC											   (0xd)

/***************************************************************************************************
 * ADC FEATURES CONGIURATION
 */
#define ADC_FIFO_MAX_NUMBER                                    (1)

#define ADC_FIFO_MAX_DRQ_LEVEL                                 (32)
#define ADC_FIFO_DRQ_LEVEL_DEFAULT                             (8) /* 8 level */

#define ADC_OSR_DEFAULT                                        (2) /* ADC_OVFS 128fs */

#define ADC_DIGITAL_CH_GAIN_MAX                                (0x7)
#define ADC_DIGITAL_DMIC_GAIN_MAX                              (0x7)

#define ADC_FEEDBACK_RES_INVALID                               (0xFF) /* invalid ADC feedback resistor */

#define ADC_MAX_CHANNELS_NUMBER                                (2) /* max ADC channels number */

#define ADC_HPF_HIGH_FREQ_HZ                                   (500) /* 500Hz for high frequency */

#define ADC_HPF_FAST_STABLE_MS                                 (10) /* 10 milliseconds for HFP fast stable */

#define ADC_FAST_CAP_CHARGE_TIME_MS                            (80) /* 80 milliseconds for CAP charging */

#define ADC_LDO_CAPACITOR_CHARGE_TIME_MS                       (5) /* Wait time for ADC input capacitor charge full */

//#define ADC_ANALOG_DEBUG_OUT_ENABLE

#ifdef CONFIG_SOC_SERIES_LARK_FPGA
#define ADC_DIGITAL_DEBUG_IN_ENABLE
#endif

#define ADC_CH2REG(base, x)                                    ((uint32_t)&((base)->ch0_digctl) + ((x) << 2))
#define ADC_CTL2REG(base, x)                                   ((uint32_t)&((base)->adc0_anactl) + ((x) << 2))

/* @brief the macro to configure the ADC channels */
#define ADC_CHANNEL_CFG(n, en) \
    { \
        if (ADC_CH_DISABLE != ch##n##_input) { \
            if (ADC_CH_DMIC == ch##n##_input) { \
                adc_hpf_config(dev, n, en); \
                __adc_digital_channel_cfg(dev, ADC_CHANNEL_##n, ADC_DMIC, en); \
            } else { \
                adc_hpf_config(dev, n, en); \
                __adc_digital_channel_cfg(dev, ADC_CHANNEL_##n, ADC_AMIC, en); \
            } \
        } \
    }

/* @brief the macro to control ADC channels to enable or disable */
#define ADC_CHANNELS_CTL(en) \
    { \
        if (en) { \
            ADC_CHANNEL_CFG(0, true); \
            ADC_CHANNEL_CFG(1, true); \
        } else { \
            ADC_CHANNEL_CFG(0, false); \
            ADC_CHANNEL_CFG(1, false); \
        } \
    }

/* @brief the macro to set the  dgain */
#define ADC_MIC_DGAIN_CFG(n) \
    { \
        if ((ADC_CH_DISABLE!= ch##n##_input) && (ADC_GAIN_INVALID != ch##n##_dgain)) { \
            if (adc_mic_dgain_translate(ch##n##_dgain, &sgain, &fgain)) { \
                LOG_DBG("failed to translate mic ch%d dgain %d", n, ch##n##_dgain); \
                return -EFAULT; \
            } \
            __adc_digital_gain_set(dev, ADC_CHANNEL_##n, sgain, fgain); \
        } \
    }

/**
 * @struct acts_audio_adc
 * @brief ADC controller hardware register
 */
struct acts_audio_adc {
    volatile uint32_t adc_digctl;		/* ADC digital control */
    volatile uint32_t ch0_digctl;		/* channel0 digital control */
    volatile uint32_t ch1_digctl;		/* channel1 digital control */
    volatile uint32_t fifoctl;			/* ADC fifo control */
    volatile uint32_t stat;				/* ADC stat */
    volatile uint32_t fifo0_dat;		/* ADC FIFO0 data */
    volatile uint32_t hw_trigger_ctl;	/* ADC HW trigger ADC control */
    
    volatile uint32_t adc0_anactl;			/* ADC0 analog Control */
    volatile uint32_t adc1_anactl;			/* ADC1 analog Control*/
    volatile uint32_t adc_com_anactl;		/* ADC common analog Control*/

    volatile uint32_t bias;				/* ADC bias */
    volatile uint32_t vmic_ctl;			/* VMIC control */
    volatile uint32_t ref_ldo_ctl;		/* ADC reference LDO control */
};

#ifdef CONFIG_CFG_DRV
/**
 * struct phy_adc_external_config
 * @brief The ADC external configuration which generated by configuration tool.
 */
struct phy_adc_external_config {
    cfg_uint8  main_mic;       /*main record adc channel select*/
    cfg_uint8  sub_mic;        /*sub record adc channel select*/
    cfg_uint8  aec_ch;         /*hw aec adc channel select*/

    uint8_t input_ch0;     /* ADC channel0 INPUT selection */
    uint8_t input_ch1;     /* ADC channel1 INPUT selection */

    uint8_t adc0_sel_vmic;  /* ADC channel0 power select */
    uint8_t adc1_sel_vmic;  /* ADC channel1 power select */

    cfg_uint32 ADC_Bias_Setting; /* ADC bias setting */
    cfg_uint8 DMIC01_Channel_Aligning; /* DMIC latch policy selection. 0: L/R 1:R/L */
    cfg_uint8 DMIC23_Channel_Aligning; /* DMIC latch policy selection. 2: L/R 3:R/L */
    CFG_Type_DMIC_Select_GPIO DMIC_Select_GPIO; /* DMIC GPIO pin  */
    cfg_uint8 Enable_ANC; /* ANC configuration. 0:disable; 1:AUDIO_ANC_FF; 2:AUDIO_ANC_FB; 3:AUDIO_ANC_HY */
    CFG_Type_DMIC_Select_GPIO ANCDMIC_Select_GPIO; /* DMIC GPIO pin for ANC */
	cfg_uint8 Hw_Aec_Select; /* Hardware AEC enable */
    cfg_int16 ANC_FF_GAIN; /* ANC FF MIC gain */
    cfg_int16 ANC_FB_GAIN; /* ANC FB MIC gain */
};
#endif

/**
 * struct phy_adc_drv_data
 * @brief The software related data that used by physical adc driver.
 */
struct phy_adc_drv_data {
#ifdef CONFIG_CFG_DRV
        struct phy_adc_external_config external_config; /* ADC external configuration */
#endif
    uint16_t input_dev;
    uint8_t hw_trigger_en : 1; /* If 1 to enable hw IRQ signal to trigger ADC digital start */
    uint8_t anc_en : 1; /* If 1 to indicate ANC has been enabled */
    a_pll_mode_e audiopll_mode;     /*The audiopll mode: 0:integer-N; 1:fractional-N*/
};

/**
 * union phy_adc_features
 * @brief The infomation from DTS to control the ADC features to enable or nor.
 */
typedef union {
    uint64_t raw;
    struct {
        uint64_t adc0_hpf_time : 2; /* ADC0 HPF auto-set time */
        uint64_t adc1_hpf_time : 2; /* ADC1 HPF auto-set time */
        uint64_t adc0_hpf_fc_high: 1; /* ADC0 HPF use high frequency range */
        uint64_t adc1_hpf_fc_high: 1; /* ADC1 HPF use high frequency range */
        uint64_t adc0_frequency : 6; /* ADC0 HFP frequency */
        uint64_t adc1_frequency : 6; /* ADC1 HFP frequency */
        uint64_t ldo_voltage : 2; /* AUDIO LDO voltage */
        uint64_t fast_cap_charge : 1; /* Fast CAP charge function */
    } v;
} phy_adc_features;

#ifndef CONFIG_CFG_DRV
static uint8_t vmic_ctl_array[] = CONFIG_AUDIO_ADC_0_VMIC_CTL_ARRAY;
#endif

static uint8_t vmic_voltage_array[] = CONFIG_AUDIO_ADC_0_VMIC_VOLTAGE_ARRAY;

/**
 * struct phy_adc_config_data
 * @brief The hardware related data that used by physical adc driver.
 */
struct phy_adc_config_data {
    uint32_t reg_base; /* ADC controller register base address */
    struct audio_dma_dt dma_fifo0; /* DMA resource for FIFO0 */
    uint8_t clk_id; /* ADC devclk id */
    uint8_t rst_id; /* ADC reset id */
    phy_adc_features features; /* ADC features */
};

struct adc_amic_aux_gain_setting {
    int16_t gain;
    uint8_t darsel;
    uint8_t input_ics;
};

struct adc_dmic_gain_setting {
    int16_t gain;
    uint8_t dmic_pre_gain;
    uint8_t digital_gain;
};

struct adc_mic_dgain_setting {
    int16_t gain;
    uint8_t sgain:4;
    uint8_t fgain:4;
};


struct adc_clk_setting {
    uint8_t sample_rate;
    uint8_t clk_div:2;     /* 00: div1;  01:div2;  10:div3; 11:/4*/
    uint8_t ovfsclkdiv:2;  /* 00: div1;  01:div2;  10:div4; 11:reserved */
    uint8_t firclkdiv:2;   /* 00: div1;  01:div3;  10:div4; 11: div6 */
    uint8_t fs_sel:2;      /*0:AUDIOPLL_44KSR ; 1:AUDIOPLL_48KSR*/
    uint8_t ovfs;
};
/*
 * ADCx_ANACTL[4]  ADC0_IRS:0
 * ADCx_ANACTL[1:0] INPUT0_IRS:0
 * ADCx_ANACTL[3:2] INUT0_ICS: select by this mapping
 * ADCx_COM_ANACTL[6:4] DARSEL: select by this mapping
 */
static const struct adc_amic_aux_gain_setting amic_aux_gain_mapping[] = {
    {0,    4, 0},
    {60,   6, 2},
    {120,  7, 3},
};

static const struct adc_mic_dgain_setting mic_dgain_mapping[] = {
    {0,    0,  0},
    {10,   0,  1},
    {20,   0,  2},
    {30,   0,  3},
    {40,   0,  4},
    {51,   0,  5},
    {61,   0,  6},
    {71,   0,  7},
    {60,   1,  0},
    {70,   1,  1},
    {80,   1,  2},
    {90,   1,  3},
    {100,  1,  4},
    {111,  1,  5},
    {121,  1,  6},
    {131,  1,  7},
    {120,  2,  0},
    {130,  2,  1},
    {140,  2,  2},
    {150,  2,  3},
    {160,  2,  4},
    {171,  2,  5},
    {181,  2,  6},
    {191,  2,  7},
    {180,  3,  0},
    {190,  3,  1},
    {200,  3,  2},
    {210,  3,  3},
    {220,  3,  4},
    {231,  3,  5},
    {241,  3,  6},
    {251,  3,  7},
    {240,  4,  0},
    {250,  4,  1},
    {260,  4,  2},
    {270,  4,  3},
    {280,  4,  4},
    {291,  4,  5},
    {301,  4,  6},
    {311,  4,  7},
    {300,  5,  0},
    {310,  5,  1},
    {320,  5,  2},
    {330,  5,  3},
    {340,  5,  4},
    {351,  5,  5},
    {361,  5,  6},
    {371,  5,  7},
    {360,  6,  0},
    {370,  6,  1},
    {380,  6,  2},
    {390,  6,  3},
    {400,  6,  4},
    {411,  6,  5},
    {421,  6,  6},
    {431,  6,  7},
    {420,  7,  0},
    {430,  7,  1},
    {440,  7,  2},
    {450,  7,  3},
    {460,  7,  4},
    {471,  7,  5},
    {481,  7,  6},
    {491,  7,  7},
};


/**
 * enum a_adc_fifo_e
 * @brief ADC fifo index selection
 */
typedef enum {
    ADC_FIFO_0 = 0,
} a_adc_fifo_e;

/**
 * enum a_adc_ovfs_e
 * @brief ADC CIC over sample rate selection
 */
typedef enum {
    ADC_OVFS_64FS = 0,
    ADC_OVFS_96FS,
    ADC_OVFS_128FS,
    ADC_OVFS_192FS,
    ADC_OVFS_256FS,
    ADC_OVFS_384FS,
    ADC_OVFS_512FS,
    ADC_OVFS_768FS
} a_adc_ovfs_e;

/**
 * enum a_adc_fir_e
 * @brief ADC frequency response (FIR) mode selection
 */
typedef enum {
    ADC_FIR_MODE_A = 0,
    ADC_FIR_MODE_B,
    ADC_FIR_MODE_C,
    ADC_FIR_MODE_D
} a_adc_fir_e;

/**
 * enum a_adc_ch_e
 * @beief ADC channels selection
 */
typedef enum {
    ADC_CHANNEL_0 = 0,
    ADC_CHANNEL_1,
} a_adc_ch_e;

typedef enum {
    ADC_AMIC = 0, /* analog mic */
    ADC_DMIC /* digital mic */
} a_adc_ch_type_e;

/*
 * enum a_hpf_time_e
 * @brief HPF(High Pass Filter) auto setting time selection
 */
typedef enum {
    HPF_TIME_0 = 0, /* 1.3ms at 48kfs*/
    HPF_TIME_1, /* 5ms at 48kfs */
    HPF_TIME_2, /* 10ms  at 48kfs*/
    HPF_TIME_3 /* 20ms at 48kfs */
} a_hpf_time_e;

/*
 * enum a_input_lr_e
 * @brief ADC input left and right selection
 */
typedef enum {
    INPUT_POSITIVE_SEL = (1 << 0), /* INPUTxP selection */
    INPUT_NEGATIVE_SEL = (1 << 1) /* INPUTxN selection */
} a_input_lr_e;

typedef struct {
    int16_t gain;
    uint8_t fb_res;
    uint8_t input_res;
} adc_gain_input_t;

#ifdef ADC_CLK_LOW_POWER_CONFIG
static const struct adc_clk_setting adc_clk_mapping[] = {
    {SAMPLE_RATE_96KHZ, 0, 1, 0, AUDIOPLL_48KSR, ADC_OVFS_64FS },
    {SAMPLE_RATE_48KHZ, 1, 1, 0, AUDIOPLL_48KSR, ADC_OVFS_128FS},
    {SAMPLE_RATE_44KHZ, 1, 1, 0, AUDIOPLL_44KSR, ADC_OVFS_128FS},
    {SAMPLE_RATE_32KHZ, 0, 2, 1, AUDIOPLL_48KSR, ADC_OVFS_192FS},
    {SAMPLE_RATE_24KHZ, 3, 0, 0, AUDIOPLL_48KSR, ADC_OVFS_256FS},
    {SAMPLE_RATE_16KHZ, 1, 1, 1, AUDIOPLL_48KSR, ADC_OVFS_384FS},
    {SAMPLE_RATE_12KHZ, 1, 1, 2, AUDIOPLL_48KSR, ADC_OVFS_512FS},
    {SAMPLE_RATE_8KHZ,  3, 0, 1, AUDIOPLL_48KSR, ADC_OVFS_768FS},
};

#else
static const struct adc_clk_setting adc_clk_mapping[] = {
    {SAMPLE_RATE_96KHZ, 0, 1, 0, AUDIOPLL_48KSR, ADC_OVFS_64FS},
    {SAMPLE_RATE_48KHZ, 1, 0, 0, AUDIOPLL_48KSR, ADC_OVFS_128FS},
    {SAMPLE_RATE_44KHZ, 1, 0, 0, AUDIOPLL_44KSR, ADC_OVFS_128FS},
    {SAMPLE_RATE_32KHZ, 0, 1, 1, AUDIOPLL_48KSR, ADC_OVFS_192FS},
    {SAMPLE_RATE_24KHZ, 0, 1, 2, AUDIOPLL_48KSR, ADC_OVFS_256FS},
    {SAMPLE_RATE_16KHZ, 1, 0, 1, AUDIOPLL_48KSR, ADC_OVFS_384FS},
    {SAMPLE_RATE_12KHZ, 1, 0, 2, AUDIOPLL_48KSR, ADC_OVFS_512FS},
    {SAMPLE_RATE_8KHZ,  1, 0, 3, AUDIOPLL_48KSR, ADC_OVFS_768FS},
};
#endif


/* @brief  Get the ADC controller base address */
static inline struct acts_audio_adc *get_adc_reg_base(struct device *dev)
{
    const struct phy_adc_config_data *cfg = dev->config;
    return (struct acts_audio_adc *)cfg->reg_base;
}

/* @brief Dump the ADC relative registers */
static void adc_dump_register(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    LOG_INF("** adc contoller regster **");
    LOG_INF("          BASE: %08x", (uint32_t)adc_reg);
    LOG_INF("    ADC_DIGCTL: %08x", adc_reg->adc_digctl);
    LOG_INF("    CH0_DIGCTL: %08x", adc_reg->ch0_digctl);
    LOG_INF("    CH1_DIGCTL: %08x", adc_reg->ch1_digctl);
    LOG_INF("   ADC_FIFOCTL: %08x", adc_reg->fifoctl);
    LOG_INF("      ADC_STAT: %08x", adc_reg->stat);
    LOG_INF(" ADC_FIFO0_DAT: %08x", adc_reg->fifo0_dat);
    
    LOG_INF("      ADC_CTL0: %08x", adc_reg->adc0_anactl);
    LOG_INF("      ADC_CTL1: %08x", adc_reg->adc1_anactl);
    
    LOG_INF("      ADC_COM_ANACTL: %08x", adc_reg->adc_com_anactl);
    LOG_INF("      ADC_BIAS: %08x", adc_reg->bias);
    LOG_INF("      VMIC_CTL: %08x", adc_reg->vmic_ctl);
    LOG_INF("   REF_LDO_CTL: %08x", adc_reg->ref_ldo_ctl);
    LOG_INF("HW_TRIGGER_CTL: %08x", adc_reg->hw_trigger_ctl);
    LOG_INF(" AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
    LOG_INF("    CMU_ADCCLK: %08x", sys_read32(CMU_ADCCLK));
    LOG_INF(" CMU_DEVCLKEN0: %08x", sys_read32(CMU_DEVCLKEN0));
    LOG_INF("     RMU_MRCR0: %08x", sys_read32(RMU_MRCR0));

}

/* @brief disable ADC FIFO by specified FIFO index */
static void __adc_fifo_disable(struct device *dev, a_adc_fifo_e idx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if (ADC_FIFO_0 == idx) {
        adc_reg->fifoctl &= (~ADC_FIFOCTL_ADF0RT);
        adc_reg->fifoctl &= (~ADC_FIFOCTL_ADF0FDE);
        adc_reg->fifoctl &= (~ADC_FIFOCTL_ADF0FIE);

        /* disable ADC FIFO0 access clock */
        sys_write32(sys_read32(CMU_ADCCLK) & ~CMU_ADCCLK_ADCFIFOCLKEN, CMU_ADCCLK);
    }
}

/* @brief check all ADC FIFOs are idle */
static inline bool __adc_check_fifo_all_disable(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if (adc_reg->fifoctl & ADC_FIFOCTL_ADF0RT)
        return false;

    return true;
}

/* @brief check whether the ADC FIFO is working now */
static bool __is_adc_fifo_working(struct device *dev, a_adc_fifo_e idx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if (ADC_FIFO_0 == idx) {
        if (adc_reg->fifoctl & ADC_FIFOCTL_ADF0RT)
            return true;
    }

    return false;
}

/* @brief enable ADC FIFO by specified FIFO index */
static int __adc_fifo_enable(struct device *dev, audio_fifouse_sel_e sel,
                            audio_dma_width_e wd, uint8_t drq_level, a_adc_fifo_e idx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->fifoctl;

    if ((drq_level > ADC_FIFO_MAX_DRQ_LEVEL) || (drq_level == 0))
        drq_level = ADC_FIFO_DRQ_LEVEL_DEFAULT;

    if (FIFO_SEL_ASRC == sel) {
        LOG_ERR("invalid fifo sel %d", sel);
        return -EINVAL;
    }

    if (ADC_FIFO_0 == idx) {
        reg &= ~0x3FFF; /* clear FIFO0 fields */

        if (FIFO_SEL_CPU == sel) /* enable IRQ */
            reg |= (ADC_FIFOCTL_ADF0FIE);
        else if (FIFO_SEL_DMA == sel) /* enable DRQ */
            reg |= (ADC_FIFOCTL_ADF0FDE);

        //reg |= ADC_FIFOCTL_ADF0OS(sel) | ADC_FIFOCTL_ADF0RT;
        reg |= ADC_FIFOCTL_ADF0OS(sel);
        if (DMA_WIDTH_16BITS == wd) /* width 0:32bits; 1:16bits */
            reg |= ADC_FIFOCTL_ADCFIFO0_DMAWIDTH;
        reg |= ADC_FIFOCTL_IDRQ_LEVEL(drq_level);

        adc_reg->fifoctl = reg;

        /* enable ADC FIFO to access ADC CLOCK */
        sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCFIFOCLKEN, CMU_ADCCLK);
    }

    return 0;
}

/* @brief set the ADC FIFO DRQ level */
static int __adc_fifo_drq_level_set(struct device *dev, a_adc_fifo_e idx, uint8_t level)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->fifoctl;

    if ((level > ADC_FIFO_MAX_DRQ_LEVEL) || (level == 0))
        return -EINVAL;

    if (ADC_FIFO_0 == idx) {
        reg &= ~ADC_FIFOCTL_IDRQ_LEVEL_MASK;
        reg |= ADC_FIFOCTL_IDRQ_LEVEL(level);
    } else {
        return -EINVAL;
    }

    adc_reg->fifoctl = reg;

    return 0;
}

/* @brief get the ADC FIFO DRQ level */
static int __adc_fifo_drq_level_get(struct device *dev, a_adc_fifo_e idx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->fifoctl;
    int level;

    if (ADC_FIFO_0 == idx) {
        level = (reg & ADC_FIFOCTL_IDRQ_LEVEL_MASK) >> ADC_FIFOCTL_IDRQ_LEVEL_SHIFT;
    } else {
        level = -EINVAL;
    }

    return level;
}

/* @brief ADC digital CIC over sample rate and FIR mode setting */
static void __adc_digital_ovfs_fir_cfg(struct device *dev, uint8_t sr)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    //struct phy_adc_drv_data *data = dev->data;
    uint32_t reg;
    a_adc_fir_e fir;
    a_adc_ovfs_e ovfs = ADC_OVFS_128FS;
    uint8_t i;

    /* Configure the Programmable frequency response curve according with the sample rate */
    if ((sr <= SAMPLE_RATE_16KHZ) || (sr == SAMPLE_RATE_48KHZ)) {
        fir = ADC_FIR_MODE_B;
    } else if (sr <= SAMPLE_RATE_32KHZ) {
        /* LE Audio Speech 24KHz or 32KHz, low latency first */
        fir = ADC_FIR_MODE_D;
    } else if (sr < SAMPLE_RATE_96KHZ) {
        fir = ADC_FIR_MODE_A;
    } else {
        fir = ADC_FIR_MODE_C;
    }

    reg = adc_reg->adc_digctl;
    reg &= ~(ADC_DIGCTL_ADC_OVFS_MASK | ADC_DIGCTL_ADC_FIR_MD_SEL_MASK);


    for (i = 0; i < ARRAY_SIZE(adc_clk_mapping); i++) {
        if (sr == adc_clk_mapping[i].sample_rate) {
            ovfs = adc_clk_mapping[i].ovfs;
        }
    }

    /* ovfs */
    reg |= ADC_DIGCTL_ADC_OVFS(ovfs);

    reg |= ADC_DIGCTL_ADC_FIR_MD_SEL(fir);

    adc_reg->adc_digctl = reg;
}

/* @brief ADC channel digital configuration */
static void __adc_digital_channel_cfg(struct device *dev, a_adc_ch_e ch, a_adc_ch_type_e type, bool out_en)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    //struct phy_adc_drv_data *data = dev->data;
    uint32_t reg0, reg1;
    uint32_t ch_digital = ADC_CH2REG(adc_reg, ch);

    reg0 = adc_reg->adc_digctl;
    reg1 = sys_read32(ch_digital);

    LOG_DBG("channel:%d type:%d enable:%d", ch, type, out_en);

    if (out_en) {
        /* enable FIR/CIC clock */
        sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCCICEN | CMU_ADCCLK_ADCFIREN, CMU_ADCCLK);
        adc_reg->stat |= (1 << 9);  //solve two adc data convert bug

        if (ADC_AMIC == type) {
            /* enable ANA clock */
            sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCANAEN, CMU_ADCCLK);
            reg1 &= ~CH0_DIGCTL_MIC_SEL; /* enable ADC analog part */
        } else if (ADC_DMIC == type) {
            /* enable DMIC clock */
            sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCDMICEN, CMU_ADCCLK);
            reg1 |= CH0_DIGCTL_MIC_SEL; /* enable ADC  digital MIC part */
        }
        reg1 |= CH0_DIGCTL_DAT_OUT_EN; /* channel FIFO timing slot enable */
    } else {
        reg1 &= ~CH0_DIGCTL_DAT_OUT_EN;
        reg0 &= ~(1 << (ADC_DIGCTL_ADC_DIG_SHIFT + ch)); /* channel disable */
        adc_reg->adc_digctl = reg0;
        /* check all channels disable */
        if (!(reg0 & ADC_DIGCTL_ADC_DIG_MASK)) {
            sys_write32(sys_read32(CMU_ADCCLK) & ~(CMU_ADCCLK_ADCDMICEN
                | CMU_ADCCLK_ADCANAEN | CMU_ADCCLK_ADCCICEN
                | CMU_ADCCLK_ADCFIREN), CMU_ADCCLK);
        }
    }

    sys_write32(reg1, ch_digital);
}

/* @brief ADC channels enable at the same time */
static void __adc_digital_channels_en(struct device *dev, bool ch0_en, bool ch1_en)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;
    uint32_t reg = adc_reg->adc_digctl;
    uint32_t reg1 = adc_reg->hw_trigger_ctl;

    reg &= ~ADC_DIGCTL_ADC_DIG_MASK;
    reg1 &= ~HW_TRIGGER_ADC_CTL_INT_TO_ADC_MASK;

    if (ch0_en) {
        reg |= ADC_DIGCTL_ADC0_DIG_EN;
        reg1 |= HW_TRIGGER_ADC_CTL_INT_TO_ADC0_EN;
    }

    if (ch1_en) {
        reg |= ADC_DIGCTL_ADC1_DIG_EN;
        reg1 |= HW_TRIGGER_ADC_CTL_INT_TO_ADC1_EN;
    }

    if (data->hw_trigger_en)
        adc_reg->hw_trigger_ctl = reg1;
    else
        adc_reg->adc_digctl = reg;
}
#if 0
/* @brief ADC HPF(High Pass Filter) audo-set configuration */
static void __adc_hpf_auto_set(struct device *dev, a_adc_ch_e ch, bool enable)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    const struct phy_adc_config_data *cfg = dev->config;
    uint32_t reg, ch_digctl = ADC_CH2REG(adc_reg, ch);
    a_hpf_time_e time = HPF_TIME_2;

    if (!enable) {
        /* disable HPF auto-set function */
        sys_write32(sys_read32(ch_digctl) & ~CH0_DIGCTL_HPF_AS_EN, ch_digctl);
        return ;
    }

    if (ADC_CHANNEL_0 == ch)
        time = PHY_DEV_FEATURE(adc0_hpf_time);
    else if (ADC_CHANNEL_1 == ch)
        time = PHY_DEV_FEATURE(adc1_hpf_time);

    reg = sys_read32(ch_digctl) & ~CH0_DIGCTL_HPF_AS_TS_MASK;
    reg |= CH0_DIGCTL_HPF_AS_TS(time);
    reg |= CH0_DIGCTL_HPF_AS_EN; /* HPF auto-set enable */

    sys_write32(reg, ch_digctl);

    LOG_DBG("%d ch@%d HPF reg:0x%x", __LINE__, ch, sys_read32(ch_digctl));
}
#endif
#if 1
/* @brief ADC HPF configuration for fast stable */
static void __adc_hpf_fast_stable(struct device *dev, uint16_t input_dev, uint8_t sample_rate)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint8_t i;
    uint32_t reg, ch_digctl, en_flag = 0;
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};
    uint16_t fc = (ADC_HPF_HIGH_FREQ_HZ + 29) / 30; /* under 48kfs */

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    if(input_dev & BIT(0)){
	    adc_input_map.ch0_input = data->external_config.input_ch0;
    }else{
        adc_input_map.ch0_input = ADC_CH_DISABLE;
    }

    if(input_dev & BIT(1)){
	    adc_input_map.ch1_input = data->external_config.input_ch1;
    }else{
        adc_input_map.ch1_input = ADC_CH_DISABLE;
    }

#else
    if (board_audio_device_mapping(&adc_input_map)) {
        LOG_ERR("invalid input device:0x%x", input_dev);
        return ;
    }
#endif

    if (adc_input_map.ch0_input != ADC_CH_DISABLE)
        en_flag |= BIT(0);

    if (adc_input_map.ch1_input != ADC_CH_DISABLE)
        en_flag |= BIT(1);

    if (sample_rate <= SAMPLE_RATE_16KHZ)
        fc *= 3;
    else if (sample_rate >= SAMPLE_RATE_96KHZ)
        fc /= 2;

    for (i = 0; i < ADC_MAX_CHANNELS_NUMBER; i++) {
        if (en_flag & BIT(i)) {
            ch_digctl = ADC_CH2REG(adc_reg, i);
            reg = sys_read32(ch_digctl) & ~(0x7f << CH0_DIGCTL_HPF_N_SHIFT);

            reg |= CH0_DIGCTL_HPF_N(fc);

            /* enable high frequency range and HPF function */
            //reg |= (CH0_DIGCTL_HPF_S | CH0_DIGCTL_HPFEN);
            reg |= (CH0_DIGCTL_HPF_S | CH0_DIGCTL_HPFEN  | CH0_DIGCTL_HPF_N_MASK);

            sys_write32(reg, ch_digctl);
        }
    }

    if (en_flag)
        k_sleep(K_MSEC(ADC_HPF_FAST_STABLE_MS));
}
#endif
/* @brief ADC HPF(High Pass Filter) enable */
static void __adc_hpf_control(struct device *dev, a_adc_ch_e ch, bool enable)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    const struct phy_adc_config_data *cfg = dev->config;
    uint32_t reg, ch_digctl = ADC_CH2REG(adc_reg, ch);
    bool is_high;
    uint8_t frequency;

    if (!enable) {
        sys_write32(sys_read32(ch_digctl) & ~CH0_DIGCTL_HPFEN, ch_digctl);
        return ;
    }

    if (ADC_CHANNEL_0 == ch) {
        is_high = PHY_DEV_FEATURE(adc0_hpf_fc_high);
        frequency = PHY_DEV_FEATURE(adc0_frequency);
    } else if (ADC_CHANNEL_1 == ch) {
        is_high = PHY_DEV_FEATURE(adc1_hpf_fc_high);
        frequency = PHY_DEV_FEATURE(adc1_frequency);
    } 
    /* clear HPF_S and HPF_N */
    reg = sys_read32(ch_digctl) & (~(0x7f << CH0_DIGCTL_HPF_N_SHIFT));
    reg |= CH0_DIGCTL_HPF_N(frequency);

    if (is_high)
        reg |= CH0_DIGCTL_HPF_S;

    /* enable HPF */
    reg |= CH0_DIGCTL_HPFEN;

    sys_write32(reg, ch_digctl);

    LOG_DBG("%d ch@%d HPF reg:0x%x", __LINE__, ch, sys_read32(ch_digctl));
}

/* @brief ADC channel digital gain setting */
static void __adc_digital_gain_set(struct device *dev, a_adc_ch_e ch, uint8_t sgain, uint8_t fgain)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg, ch_digctl = ADC_CH2REG(adc_reg, ch);

    if (sgain > ADC_DIGITAL_CH_GAIN_MAX)
        sgain = ADC_DIGITAL_CH_GAIN_MAX;
    
    if (fgain > ADC_DIGITAL_CH_GAIN_MAX)
        fgain = ADC_DIGITAL_CH_GAIN_MAX;

    reg = sys_read32(ch_digctl) & (~CH0_DIGCTL_SGAIN_MASK) & (~CH0_DIGCTL_FGAIN_MASK);
    reg |= CH0_DIGCTL_SGAIN(sgain);
    reg |= CH0_DIGCTL_FGAIN(fgain);

    sys_write32(reg, ch_digctl);
}

#ifdef CONFIG_ADC_DMIC
/* @brief ADC digital dmic gain setting */
static void __adc_dmic_gain_set(struct device *dev, uint8_t gain)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg;

    if (gain > ADC_DIGITAL_DMIC_GAIN_MAX)
        gain = ADC_DIGITAL_DMIC_GAIN_MAX;

    reg = adc_reg->adc_digctl & ~ADC_DIGCTL_DMIC_PRE_GAIN_MASK;
    reg |= ADC_DIGCTL_DMIC_PRE_GAIN(gain);

    /* by default DMIC01 latch sequence is L firstly and then R */
#ifdef CONFIG_ADC_DMIC_RL_SEQUENCE
    reg |= ADC_DIGCTL_DMIC01_CHS;
#endif

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    if (data->external_config.DMIC01_Channel_Aligning)
        reg |= ADC_DIGCTL_DMIC01_CHS; /* DMIC01 latch sequency: R/L  */
#endif

    adc_reg->adc_digctl = reg;
}
#endif

/* @brief ADC VMIC control initialization */
static void __adc_vmic_ctl_init(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg;

    reg = adc_reg->vmic_ctl;
    //reg |= (ADC_VMIC_CTL_ISO_AVCC_AU | ADC_VMIC_CTL_ISO_VD18);

    reg &= ~ADC_VMIC_CTL_VMIC0_VOL_MASK;
    reg |= ADC_VMIC_CTL_VMIC0_VOL(vmic_voltage_array[0]);

    reg &= ~ADC_VMIC_CTL_VMIC1_VOL_MASK;
    reg |= ADC_VMIC_CTL_VMIC1_VOL(vmic_voltage_array[1]);

    adc_reg->vmic_ctl = reg;

}

#ifdef CONFIG_CFG_DRV
static void __adc_vmic_ctl(struct device *dev, uint8_t adc2vmic_index, bool is_en)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->vmic_ctl;

    LOG_DBG("adc2vmic index:%d is_en:%d",adc2vmic_index, is_en);

    if (is_en) {
        if (adc2vmic_index == 0){
            reg &= ~ADC_VMIC_CTL_VMIC0_EN_MASK;
            reg |= ADC_VMIC_CTL_VMIC0_EN(3); /* enable VMIC0 OP */
        }else if (adc2vmic_index == 1){
            reg &= ~ADC_VMIC_CTL_VMIC1_EN_MASK;
            reg |= ADC_VMIC_CTL_VMIC1_EN(3); /* enable VMIC1 OP */
        }else{
        }
        
    } else {
        if (adc2vmic_index == 0){
            reg &= ~ADC_VMIC_CTL_VMIC0_EN_MASK; /* disable VMIC0 OP */
        }else if (adc2vmic_index == 1){
            reg &= ~ADC_VMIC_CTL_VMIC1_EN_MASK; /* disable VMIC1 OP */
        }else{
        }
    }

    adc_reg->vmic_ctl = reg;
    
    k_sleep(K_MSEC(1));
}


/* @brief ADC record VMIC control */
static void __adc_vmic_ctl_enable(struct device *dev, uint16_t input_dev)
{
    struct phy_adc_drv_data *data = dev->data;
    uint8_t adc0_sel_vmic = data->external_config.adc0_sel_vmic;
    uint8_t adc1_sel_vmic = data->external_config.adc1_sel_vmic;
    if(input_dev & BIT(0)){
        if((adc0_sel_vmic != VMIC_NONE))
            __adc_vmic_ctl(dev, adc0_sel_vmic, true);
    }

    if(input_dev & BIT(1)){
        if((adc1_sel_vmic != VMIC_NONE))
            __adc_vmic_ctl(dev, adc1_sel_vmic, true);
    }
}

static void __adc_vmic_ctl_disable(struct device *dev, uint16_t input_dev)
{
    struct phy_adc_drv_data *data = dev->data;
    uint8_t main_mic = data->external_config.main_mic;
    uint8_t sub_mic = data->external_config. sub_mic;
    uint8_t adc0_sel_vmic = data->external_config.adc0_sel_vmic;
    uint8_t adc1_sel_vmic = data->external_config.adc1_sel_vmic;

    switch(main_mic){
        case ADC_0:
            if(adc0_sel_vmic != VMIC_NONE)
                __adc_vmic_ctl(dev, adc0_sel_vmic, false);
            break;
        
        case ADC_1:
            if(adc1_sel_vmic != VMIC_NONE)
                __adc_vmic_ctl(dev, adc1_sel_vmic, false);
            break;
         default:
            break;
    }

    switch(sub_mic){
        case ADC_0:
            if(adc0_sel_vmic != VMIC_NONE)
                __adc_vmic_ctl(dev, adc0_sel_vmic, false);
            break;
        
        case ADC_1:
            if(adc1_sel_vmic != VMIC_NONE)
                __adc_vmic_ctl(dev, adc1_sel_vmic, false);
            break;
        default:
            break;
        }
}

#else
/* @brief ADC VMIC control */
static void __adc_vmic_ctl_enable(struct device *dev, uint16_t input_dev)
{
    /* power-on MIC voltage */
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if ((input_dev & AUDIO_DEV_TYPE_AMIC) || (input_dev & AUDIO_DEV_TYPE_DMIC))
    {
        /* vmic_ctl_array
          *   - 0: disable VMICx OP
          *   - 2: bypass VMICx OP
          *   - 3: enable VMICx OP
          */
        if (vmic_ctl_array[0] <= 3)
        {
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC0_EN_MASK;
            adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC0_EN(vmic_ctl_array[0]);
        }

        if (vmic_ctl_array[1] <= 3)
        {
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC1_EN_MASK;
            adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC1_EN(vmic_ctl_array[1]);
        }
    }
}

/* @brief ADC VMIC control */
static void __adc_vmic_ctl_disable(struct device *dev, uint16_t input_dev)
{
    /* power-off MIC voltage */
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if ((input_dev & AUDIO_DEV_TYPE_AMIC) || (input_dev & AUDIO_DEV_TYPE_DMIC))
    {
        if (vmic_ctl_array[0] <= 3)
        {
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC0_EN_MASK; /* disable VMIC0 OP */
        }

        if (vmic_ctl_array[1] <= 3)
        {
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC1_EN_MASK; /* disable VMIC1 OP */
        }
    }
}
#endif

/* @brief ADC input analog gain setting */
static int __adc_input_gain_set(struct device *dev, a_adc_ch_e ch, uint8_t darsel,
                                    uint8_t inpu_ics, bool update_fb)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg, reg_com, adc_anactl = ADC_CTL2REG(adc_reg, ch);

    /* Set FDBUFx when in single end mode */
    reg = sys_read32(adc_anactl);
    reg_com = adc_reg->adc_com_anactl;

    reg &= ~ADC0_ANACTL_INPUT0_ICS_MASK;
    reg |= ADC0_ANACTL_INPUT0_ICS(inpu_ics);

    if (update_fb) {
        reg_com &= ~ADC_COM_ANACTL_DARSEL_MASK;
        reg_com |= ADC_COM_ANACTL_DARSEL(darsel);
    }

    sys_write32(reg, adc_anactl);
    adc_reg->adc_com_anactl = reg_com;

    return 0;
}

/* @brief ADC channel0 analog control */
static int __adc_com_analog_control(struct device *dev, uint8_t ch0_inputx, uint8_t ch1_inputx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg_com = adc_reg->adc_com_anactl;
    uint32_t bias = adc_reg->bias;

    reg_com = 0x000008c7;
    adc_reg->adc_com_anactl = reg_com;

    if ((ADC_CH_INPUT0NP_DIFF == ch0_inputx) || (ADC_CH_INPUT0NP_DIFF == ch1_inputx)) {
        bias |= ADC_BIAS_FSUP_CTL(0xf);
        bias |= 1 << ADC_BIAS_OPFSUPEN;
        bias |= ADC_BIAS_IOPFSUP(0x3);
    }
    adc_reg->bias = bias;
    adc_reg->adc_com_anactl |= ADC_COM_ANACTL_BIASEN;

    /* Wait for AULDO stable */
    if (!z_is_idle_thread_object(_current))
        k_sleep(K_MSEC(4));
    else
        k_busy_wait(4000);

    adc_reg->adc0_anactl &= ~ADC0_ANACTL_OPFAU;
    adc_reg->adc1_anactl |= ADC0_ANACTL_OPFAU;

    return 0;
}

/* @brief ADC channel0 analog control */
static int __adc_ch0_analog_control(struct device *dev, uint8_t inputx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->adc0_anactl;

    /* ADC channel disable or DMIC does not need to set ADCx_ctl */
    if ((inputx == ADC_CH_DISABLE) || (inputx == ADC_CH_DMIC))
        return 0;

    if ((inputx != ADC_CH_INPUT0P) && (inputx != ADC_CH_INPUT0NP_DIFF)) {
        LOG_ERR("invalid input:0x%x for ADC0", inputx);
        return -EINVAL;
    }

    reg &= ~ADC0_ANACTL_INPUT0_IRS_MASK; /* clear input resistor */
    reg &= ~ADC0_ANACTL_INPUT0_IN_MODE;
    reg &= ~(0xf << ADC0_ANACTL_INPUT0P_EN_SHIFT); /* clear INPUTN/P pad to ADC channel input enable */
    
    /* ADC0 channel sdm / PREOP 0 / FD BUF OP 0 diable*/
    reg &= ~(ADC0_ANACTL_ADC0_EN | ADC0_ANACTL_FDBUF0_EN);

    reg |= ADC0_ANACTL_AD0_DLY(0x1);  /*AD0_DLY*/
    //reg |= ADC0_ANACTL_AD0_CHOP; /* AD0 shop enable */
    reg |= ADC0_ANACTL_FDBUF0_EN; /* FD BUF OP 0 enable for SE mode */
    reg |= ADC0_ANACTL_ADC0_EN; /* ADC0 channel enable */
    
    reg |= ADC0_ANACTL_FDBUF0_IRS(2); /* FD BUF OP 0 input res select: se mode:1x; diff mode:0*/

    /* by default to enable single end input mode: INPUT0P=>ADC0, INPUT0N=>ADC2 */
    reg |= ADC0_ANACTL_INPUT0_IN_MODE;

    /* input differential mode */
    if (ADC_CH_INPUT0NP_DIFF == inputx) {
        reg &= ~ADC0_ANACTL_INPUT0_IN_MODE; /* enable differential input mode */
        reg &= ~ADC0_ANACTL_FDBUF0_EN;	    /* when sel diff mode, disable buf */
        reg &= ~ADC0_ANACTL_FDBUF0_IRS_MASK;   /* when select diff mode to disable connect buf */
    }

    reg |= ADC0_ANACTL_OPFAU;

    /* enable INPUT0N pad to ADC0/2 channel */
    if (inputx & ADC_CH_INPUT0N)
        reg |= ADC0_ANACTL_INPUT0N_EN(3);

    /* enable INPUT0P pad to ADC0 channel */
    if (inputx & ADC_CH_INPUT0P)
        reg |= ADC0_ANACTL_INPUT0P_EN(3);

    reg |= ADC0_ANACTL_INPUT0_ICS(0x0);
    reg |= ADC0_ANACTL_INPUT0_IRS(0x0);

    adc_reg->adc0_anactl = reg;
    

    return 0;
}

/* @brief ADC channel1 analog control */
static int __adc_ch1_analog_control(struct device *dev, uint8_t inputx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->adc1_anactl;

    /* ADC channel disable or DMIC does not need to set ADCx_ctl */
    if ((inputx == ADC_CH_DISABLE) || (inputx == ADC_CH_DMIC))
        return 0;

    if ((inputx != ADC_CH_INPUT1P) && (inputx != ADC_CH_INPUT1NP_DIFF)) {
        LOG_ERR("invalid input:0x%x for ADC1", inputx);
        return -EINVAL;
    }
    
    reg &= ~ADC1_ANACTL_INPUT1_IRS_MASK; /* clear input resistor */
    reg &= ~ADC1_ANACTL_INPUT1_IN_MODE;
    reg &= ~(0xf << ADC1_ANACTL_INPUT1P_EN_SHIFT); /* clear INPUTN/P pad to ADC channel input enable */
    
    /* ADC0 channel sdm / PREOP 0 / FD BUF OP 0 diable*/
    reg &= ~(ADC1_ANACTL_ADC1_EN | ADC1_ANACTL_FDBUF1_EN);

    reg |= ADC1_ANACTL_AD1_DLY(0x1);  /*AD0_DLY*/
    //reg |= ADC1_ANACTL_AD1_CHOP; /* AD0 shop enable */
    reg |= ADC1_ANACTL_FDBUF1_EN; /* FD BUF OP 0 enable for SE mode */
    reg |= ADC1_ANACTL_ADC1_EN; /* ADC0 channel enable */
    
    reg |= ADC1_ANACTL_FDBUF1_IRS(2); /* FD BUF OP 0 input res select: se mode:1x; diff mode:0*/

    /* by default to enable single end input mode: INPUT0P=>ADC0, INPUT0N=>ADC2 */
    reg |= ADC1_ANACTL_INPUT1_IN_MODE;

    /* input differential mode */
    if (ADC_CH_INPUT1NP_DIFF == inputx) {
        reg &= ~ADC1_ANACTL_INPUT1_IN_MODE; /* enable differential input mode */
        reg &= ~ADC1_ANACTL_FDBUF1_EN;	    /* when sel diff mode, disable buf */
        reg &= ~ADC1_ANACTL_FDBUF1_IRS_MASK;   /* when select diff mode to disable connect buf */
    }

    reg &= ~ADC1_ANACTL_OPFAU;

    /* enable INPUT0N pad to ADC0/2 channel */
    if (inputx & ADC_CH_INPUT1N)
        reg |= ADC1_ANACTL_INPUT1N_EN(3);

    /* enable INPUT0P pad to ADC0 channel */
    if (inputx & ADC_CH_INPUT1P)
        reg |= ADC1_ANACTL_INPUT1P_EN(3);

    reg |= ADC1_ANACTL_INPUT1_ICS(0x0);
    reg |= ADC1_ANACTL_INPUT1_IRS(0x0);

    adc_reg->adc1_anactl = reg;
    return 0;
}

#ifdef ADC_DIGITAL_DEBUG_IN_ENABLE
static void __adc_digital_debug_in(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    /* set ADC clock divisor to '1' */
    sys_write32(sys_read32(CMU_ADCCLK) & (~0x7), CMU_ADCCLK);

    /* switch ADC debug clock to external PAD */
    sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCDEBUGEN, CMU_ADCCLK);

    /* ADC Digital debug enable */
    adc_reg->adc_digctl |= ADC_DIGCTL_ADDEN;
}
#endif

#ifdef ADC_ANALOG_DEBUG_OUT_ENABLE
static void __adc_analog_debug_out(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    uint32_t reg = sys_read32(DEBUGSEL) & ~DEBUGSEL_DBGSE_MASK;
    reg |= DBGSE_ADC << DEBUGSEL_DBGSE_SHIFT;
    sys_write32(reg, DEBUGSEL);

    sys_write32(sys_read32(DEBUGIE0) & (~0x107fe000), DEBUGIE0);
    sys_write32(sys_read32(DEBUGOE0) | 0x107fe000, DEBUGOE0);

    adc_reg->adc_digctl |= ADC_DIGCTL_AADEN;
}
#endif

/* @brief set the external trigger source for DAC digital start */
static int __adc_external_trigger_enable(struct device *dev, uint8_t trigger_src)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;

    if (trigger_src > 6) {
        LOG_ERR("Invalid ADC trigger source %d", trigger_src);
        return -EINVAL;
    }

    adc_reg->hw_trigger_ctl &= ~ HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL_MASK;
    adc_reg->hw_trigger_ctl |= HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL(trigger_src);

    data->hw_trigger_en = 1;

    return 0;
}

/* @brief disable the external irq signal to start ADC digital function */
static void __adc_external_trigger_disable(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;

    if (adc_reg->hw_trigger_ctl & HW_TRIGGER_ADC_CTL_INT_TO_ADC_MASK)
        adc_reg->hw_trigger_ctl &= ~HW_TRIGGER_ADC_CTL_INT_TO_ADC_MASK;

    data->hw_trigger_en = 0;
}

/* @brief ADC fast capacitor charge function */
static void adc_fast_cap_charge(struct device *dev, uint16_t input_dev)
{
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;

    if(input_dev & BIT(0)){
        adc_input_map.ch0_input = data->external_config.input_ch0;
    }else{
        adc_input_map.ch0_input = ADC_CH_DISABLE;
    }

    if(input_dev & BIT(1)){
        adc_input_map.ch1_input = data->external_config.input_ch1;
    }else{
        adc_input_map.ch1_input = ADC_CH_DISABLE;
    }

    if ((ADC_CH_INPUT0NP_DIFF == adc_input_map.ch0_input) || (ADC_CH_INPUT1NP_DIFF == adc_input_map.ch1_input)) {
        //fast up
        adc_reg->adc_com_anactl |=  0x7; //bias en
        adc_reg->adc0_anactl |=  (1<<7);
        adc_reg->adc1_anactl &= ~(1<<7);
        adc_reg->bias |= (0xf<<28); //se
        adc_reg->bias |= (0x3<<20);
        adc_reg->bias |= (0x1<<27);

        //Delay 12ms
        k_busy_wait(12 * 1000UL);

        //fast up ok
        adc_reg->adc0_anactl  &= ~(1<<7);
        adc_reg->adc1_anactl  |=  (1<<7);
        adc_reg->bias &= ~ (0xf<<28);
        adc_reg->bias &= ~(0x3<<20);
        adc_reg->bias &= ~(0x1<<27);
    }else{
        //fast up
        adc_reg->adc_com_anactl |=  0x1; //bias en
        adc_reg->adc0_anactl |=  (1<<7);
        adc_reg->adc1_anactl &= ~(1<<7);
        adc_reg->bias |= (0xa<<28); //diff
        adc_reg->bias |= (0x3<<20);
        adc_reg->bias |= (0x1<<27);

        //Delay 12ms
        k_busy_wait(12 * 1000UL);

        //fast up ok
        adc_reg->adc0_anactl &= ~(1<<7);
        adc_reg->adc1_anactl |=  (1<<7);
        adc_reg->bias &= ~ (0xf<<28);
        adc_reg->bias &= ~(0x3<<20);
        adc_reg->bias &= ~(0x1<<27);
    }

}

/* @brief ADC input channel analog configuration */
static int adc_input_config(struct device *dev, uint16_t input_dev)
{
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};
    int ret;

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;

    if(input_dev & BIT(0)){
	    adc_input_map.ch0_input = data->external_config.input_ch0;
    }else{
        adc_input_map.ch0_input = ADC_CH_DISABLE;
    }

    if(input_dev & BIT(1)){
	    adc_input_map.ch1_input = data->external_config.input_ch1;
    }else{
        adc_input_map.ch1_input = ADC_CH_DISABLE;
    }

#else
    if (board_audio_device_mapping(&adc_input_map))
        return -ENOENT;
#endif

    LOG_INF("ADC channel {dev:0x%x, [0x%x, 0x%x, 0x%x, 0x%x]}",
            input_dev, adc_input_map.ch0_input, adc_input_map.ch1_input,
            adc_input_map.ch2_input, adc_input_map.ch3_input);

    adc_fast_cap_charge(dev, input_dev);

    ret = __adc_ch0_analog_control(dev, adc_input_map.ch0_input);
    if (ret)
        return ret;

    ret = __adc_ch1_analog_control(dev, adc_input_map.ch1_input);
    if (ret)
        return ret;

    ret = __adc_com_analog_control(dev, adc_input_map.ch0_input, adc_input_map.ch1_input);
    if (ret)
        return ret;

    return 0;
}

/* @brief ADC HPF (High Pass Filter) configuration */
static void adc_hpf_config(struct device *dev, a_adc_ch_e ch, bool enable)
{
    //__adc_hpf_auto_set(dev, ch, enable);
    __adc_hpf_control(dev, ch, enable);
}

/* @brief ADC channels enable according to the input audio device */
static int adc_channels_enable(struct device *dev, uint16_t input_dev)
{
    uint8_t ch0_input = 0, ch1_input = 0;
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;

    if(input_dev & BIT(0)){
	    adc_input_map.ch0_input = data->external_config.input_ch0;
    }else{
        adc_input_map.ch0_input = ADC_CH_DISABLE;
    }

    if(input_dev & BIT(1)){
	    adc_input_map.ch1_input = data->external_config.input_ch1;
    }else{
        adc_input_map.ch1_input = ADC_CH_DISABLE;
    }

#else
    if (board_audio_device_mapping(&adc_input_map)) {
        LOG_ERR("invalid input device:0x%x", input_dev);
        return -ENOENT;
    }
#endif

    ch0_input = adc_input_map.ch0_input;
    ch1_input = adc_input_map.ch1_input;

    ADC_CHANNELS_CTL(true);

    return 0;
}

/* @brief ADC channels disable according to the input audio device */
static int adc_channels_disable(struct device *dev, uint16_t input_dev)
{
    uint8_t ch0_input = 0, ch1_input = 0;
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    if(input_dev & BIT(0)){
	    adc_input_map.ch0_input = data->external_config.input_ch0;
    }else{
        adc_input_map.ch0_input = ADC_CH_DISABLE;
    }

    if(input_dev & BIT(1)){
	    adc_input_map.ch1_input = data->external_config.input_ch1;
    }else{
        adc_input_map.ch1_input = ADC_CH_DISABLE;
    }

#else
    if (board_audio_device_mapping(&adc_input_map)) {
        LOG_ERR("invalid input device:0x%x", input_dev);
        return -ENOENT;
    }
#endif

    ch0_input = adc_input_map.ch0_input;
    ch1_input = adc_input_map.ch1_input;

    ADC_CHANNELS_CTL(false);

    return 0;
}

/* @brief  Enable the ADC digital function */
static int adc_digital_enable(struct device *dev, uint16_t input_dev, uint8_t sample_rate)
{
    /* configure OVFS and FIR */
    __adc_digital_ovfs_fir_cfg(dev, sample_rate);

    /* set HPF high frequency range for fast stable */
    __adc_hpf_fast_stable(dev, input_dev, sample_rate);

    return adc_channels_enable(dev, input_dev);
}

/* @brief  Disable the ADC digital function */
static void adc_digital_disable(struct device *dev, uint16_t input_dev)
{
    adc_channels_disable(dev, input_dev);
}

/* @brief Translate the AMIC/AUX gain from dB fromat to hardware register value */
static int adc_aux_amic_gain_translate(int16_t gain, uint8_t *darsel, uint8_t *input_ics)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(amic_aux_gain_mapping); i++) {
        if (gain <= amic_aux_gain_mapping[i].gain) {
            *darsel = amic_aux_gain_mapping[i].darsel;
            *input_ics = amic_aux_gain_mapping[i].input_ics;
            LOG_INF("gain:%d map [%d %d]",
                gain, *darsel, *input_ics);
            break;
        }
    }

    if (i == ARRAY_SIZE(amic_aux_gain_mapping)) {
        LOG_ERR("can not find out gain map %d", gain);
        return -ENOENT;
    }

    return 0;
}

/* @brief Translate the gain from dB fromat to hardware register value */
static int adc_mic_dgain_translate(int16_t gain, uint8_t *sgain, uint8_t *fgain)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(mic_dgain_mapping); i++) {
        if (gain <= mic_dgain_mapping[i].gain) {
            *sgain = mic_dgain_mapping[i].sgain;
            *fgain = mic_dgain_mapping[i].fgain;
            LOG_DBG("gain:%d map [sg:%d, fg:%d]",
                gain, *sgain, *fgain);
            break;
        }
    }

    if (i == ARRAY_SIZE(mic_dgain_mapping)) {
        LOG_ERR("can not find out gain map %d", gain);
        return -ENOENT;
    }

    return 0;
}
/* @brief ADC config channel0 again */
static int adc_ch0_gain_config(struct device *dev, uint8_t ch_input, int16_t ch_gain)
{
    uint8_t darsel, input_res;

    if ((ADC_CH_DISABLE != ch_input) && (ADC_CH_DMIC != ch_input)) {
        if (adc_aux_amic_gain_translate(ch_gain, &darsel, &input_res)) {
            LOG_ERR("failed to translate amic_aux ch0 gain %d", ch_gain);
        } else {
            if ((ADC_CH_INPUT0NP_DIFF & ch_input) == ADC_CH_INPUT0NP_DIFF)
                __adc_input_gain_set(dev, ADC_CHANNEL_0, darsel, input_res, true);
            else
                __adc_input_gain_set(dev, ADC_CHANNEL_0, darsel, input_res, true);
        }
    }

    return 0;
}

/* @brief ADC config channel1 again */
static int adc_ch1_gain_config(struct device *dev, uint8_t ch_input, int16_t ch_gain)
{
    uint8_t darsel, input_res;

    if ((ADC_CH_DISABLE != ch_input) && (ADC_CH_DMIC != ch_input)) {
        if (adc_aux_amic_gain_translate(ch_gain, &darsel, &input_res)) {
            LOG_ERR("failed to translate amic_aux ch1 gain %d", ch_gain);
        } else {
            if ((ADC_CH_INPUT1NP_DIFF & ch_input) == ADC_CH_INPUT1NP_DIFF)
                __adc_input_gain_set(dev, ADC_CHANNEL_1, darsel, input_res, true);
            else
                __adc_input_gain_set(dev, ADC_CHANNEL_1, darsel, input_res, true);
        }
    }

    return 0;
}

static int adc_gain_config(struct device *dev, uint16_t input_dev, adc_gain *gain)
{
    uint8_t ch0_input, ch1_input;
    int16_t ch0_again = 0, ch1_again = 0;
    int16_t ch0_dgain = 0, ch1_dgain = 0;
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};
    uint8_t fgain, sgain;

#ifdef CONFIG_CFG_DRV
	struct phy_adc_drv_data *data = dev->data;
    if(input_dev & BIT(0)){
	    adc_input_map.ch0_input = data->external_config.input_ch0;
    }else{
        adc_input_map.ch0_input = ADC_CH_DISABLE;
    }

    if(input_dev & BIT(1)){
	    adc_input_map.ch1_input = data->external_config.input_ch1;
    }else{
        adc_input_map.ch1_input = ADC_CH_DISABLE;
    }

#else
    if (board_audio_device_mapping(&adc_input_map))
        return -ENOENT;
#endif

    ch0_input = adc_input_map.ch0_input;
    ch1_input = adc_input_map.ch1_input;

    /* gain set when input channel enabled */
    if (ADC_CH_DISABLE != ch0_input)
    {
        ch0_again = gain->ch_again[0];
        ch0_dgain = gain->ch_dgain[0];
    }
    if (ADC_CH_DISABLE != ch1_input)
    {
        ch1_again = gain->ch_again[1];
        ch1_dgain = gain->ch_dgain[1];
    }
    LOG_INF("ADC channel again {%d, %d}, dgain {%d, %d}",
            ch0_again, ch1_again,ch0_dgain, ch1_dgain);

//#ifdef CONFIG_ADC_DMIC
//#endif
    ADC_MIC_DGAIN_CFG(0);
    ADC_MIC_DGAIN_CFG(1);

    /* config channel0 again */
    adc_ch0_gain_config(dev, ch0_input, ch0_again);

    /* config channel1 again */
    adc_ch1_gain_config(dev, ch1_input, ch1_again);

    return 0;
}


/* @brief ADC sample rate config */
static int adc_sample_rate_set(struct device *dev, audio_sr_sel_e sr_khz)
{   
    struct phy_adc_drv_data *data = dev->data;

     uint8_t clk_div, firclkdiv, ovfsclkdiv, fs_sel, pll_index;
     uint32_t reg;
     uint8_t i;
     uint16_t ret;

     ARG_UNUSED(dev);
    printk("adc_sample_rate_set, sample:%d++++++++\n", sr_khz);
     for (i = 0; i < ARRAY_SIZE(adc_clk_mapping); i++) {
         if (sr_khz == adc_clk_mapping[i].sample_rate) {
             clk_div = adc_clk_mapping[i].clk_div;
             ovfsclkdiv = adc_clk_mapping[i].ovfsclkdiv;
             firclkdiv = adc_clk_mapping[i].firclkdiv;
             fs_sel = adc_clk_mapping[i].fs_sel;
         }
     }

     /* Check the pll usage and then config */
     ret = audio_pll_check_config(fs_sel, &pll_index, data->audiopll_mode);
     if (ret) {
         LOG_DBG("check pll config error:%d", ret);
         return ret;
     }

     reg = sys_read32(CMU_ADCCLK) & ~0x1FF;

     /* Select pll0 */
     reg |= (pll_index & 0x1) << CMU_ADCCLK_ADCCLKSRC_SHIFT;
     /*clk div*/
     reg &= ~CMU_ADCCLK_ADCCLKDIV_MASK;
     reg |= CMU_ADCCLK_ADCCLKDIV(clk_div);
     /*fir clk*/
     reg &= ~CMU_ADCCLK_ADCFIRCLKDIV_MASK;
     reg |= CMU_ADCCLK_ADCFIRCLKDIV(firclkdiv);
     /*overfs clk*/
     reg &= ~CMU_ADCCLK_ADCOVFSCLKDIV_MASK;
     reg |= CMU_ADCCLK_ADCOVFSCLKDIV(ovfsclkdiv);

     reg |= CMU_ADCCLK_ADCANAEN;  //ADC analog CLK enable
     
     //enable in other functions
     //reg |= CMU_ADCCLK_ADCCICEN;  //ADC CIC Filter enable
     //reg |= CMU_ADCCLK_ADCFIREN;  //ADC FIR Filter CLK enable
     //reg |= CMU_ADCCLK_ADCFIFOCLKEN;  //ADC FIFO Access Clock enable

     sys_write32(reg, CMU_ADCCLK);

    return 0;
}

/* @brief Get the sample rate from the ADC config */
static int adc_sample_rate_get(struct device *dev)
{
    uint8_t clk_div, fir_div, pll_index;
    uint32_t reg;
    int sample_rate;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_ADCCLK);
    pll_index = (reg & (1 << CMU_ADCCLK_ADCCLKSRC_SHIFT)) >> CMU_ADCCLK_ADCCLKSRC_SHIFT;
    clk_div = reg & CMU_ADCCLK_ADCCLKDIV_MASK;
    
    fir_div = (reg & CMU_ADCCLK_ADCFIRCLKDIV_MASK) >> CMU_ADCCLK_ADCFIRCLKDIV_SHIFT;
    sample_rate = audio_get_pll_sample_rate(clk_div, pll_index);
    if(fir_div)
        sample_rate = sample_rate/3;
    return sample_rate;
}

/* @brief Get the AUDIO_PLL APS used by ADC */
static int adc_get_pll_aps(struct device *dev)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_ADCCLK);
    pll_index = (reg & (1 << CMU_ADCCLK_ADCCLKSRC_SHIFT)) >> CMU_ADCCLK_ADCCLKSRC_SHIFT;

    return audio_pll_get_aps((a_pll_type_e)pll_index);
}

/* @brief Get the AUDIO_PLL APS used by ADC */
static int adc_get_pll_aps_f(struct device *dev)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_ADCCLK);
    pll_index = (reg & (1 << CMU_ADCCLK_ADCCLKSRC_SHIFT)) >> CMU_ADCCLK_ADCCLKSRC_SHIFT;

    return audio_pll_get_aps_f((a_pll_type_e)pll_index);
}

/* @brief Set the AUDIO_PLL APS used by ADC */
static int adc_set_pll_aps(struct device *dev, audio_aps_level_e level)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_ADCCLK);
    pll_index = (reg & (1 << CMU_ADCCLK_ADCCLKSRC_SHIFT)) >> CMU_ADCCLK_ADCCLKSRC_SHIFT;

    return audio_pll_set_aps((a_pll_type_e)pll_index, level);
}

/* @brief Set the AUDIO_PLL APS_F used by ADC */
static int adc_set_pll_aps_f(struct device *dev, unsigned int level)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_ADCCLK);
    pll_index = (reg & (1 << CMU_ADCCLK_ADCCLKSRC_SHIFT)) >> CMU_ADCCLK_ADCCLKSRC_SHIFT;

    return audio_pll_set_aps_f((a_pll_type_e)pll_index, level);
}


/* @brief Set the AUDIO_PLL index that used by ADC */
static int adc_get_pll_index(struct device *dev, uint8_t *idx)
{
    uint32_t reg;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_ADCCLK);
    *idx = (reg & (1 << CMU_ADCCLK_ADCCLKSRC_SHIFT)) >> CMU_ADCCLK_ADCCLKSRC_SHIFT;

    return 0;
}

/* @brief ADC FIFO DRQ level set by input device */
static int adc_fifo_drq_level_set(struct device *dev, uint8_t level)
{
    return __adc_fifo_drq_level_set(dev, ADC_FIFO_0, level);
}

/* @brief ADC FIFO DRQ level get by input device */
static int adc_fifo_drq_level_get(struct device *dev, uint8_t *level)
{
    int ret;

    ret = __adc_fifo_drq_level_get(dev, ADC_FIFO_0);
    if (ret < 0) {
        LOG_ERR("Failed to get ADC FIFO DRQ level err=%d", ret);
        return ret;
    }

    *level = ret;

    LOG_DBG("ADC DRQ level %d", *level);

    return 0;
}

/* @brief check the FIFO is busy or not */
static int adc_check_fifo_busy(struct device *dev)
{
    if (__is_adc_fifo_working(dev, ADC_FIFO_0)) {
        LOG_INF("ADC FIFO0 now is working");
        return -EBUSY;
    }

    return 0;
}

/* @brief ADC FIFO enable on the basic of the input audio device usage */
static int adc_fifo_enable(struct device *dev, ain_param_t *in_param)
{
    audio_dma_width_e wd = (in_param->channel_width != CHANNEL_WIDTH_16BITS)
                            ? DMA_WIDTH_32BITS : DMA_WIDTH_16BITS;

    __adc_fifo_disable(dev, ADC_FIFO_0);
    return __adc_fifo_enable(dev, FIFO_SEL_DMA, wd,
                            ADC_FIFO_DRQ_LEVEL_DEFAULT, ADC_FIFO_0);;
}

/* @brief ADC FIFO enable on the basic of the input audio device usage */
static int adc_fifo_final_enable(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    adc_reg->fifoctl |=  ADC_FIFOCTL_ADF0RT;


    return 0;
}


/* @brief ADC FIFO enable on the basic of the input audio device usage */
static int adc_fifo_disable(struct device *dev)
{
    __adc_fifo_disable(dev, ADC_FIFO_0);;

    return 0;
}

/* @brief ADC BIAS setting for power saving */
static void adc_bias_setting(struct device *dev, uint16_t input_dev)
{
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    
#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    
    if(input_dev & BIT(0)){
        adc_input_map.ch0_input = data->external_config.input_ch0;
    }else{
        adc_input_map.ch0_input = ADC_CH_DISABLE;
    }
    
    if(input_dev & BIT(1)){
        adc_input_map.ch1_input = data->external_config.input_ch1;
    }else{
        adc_input_map.ch1_input = ADC_CH_DISABLE;
    }
#else
    if (board_audio_device_mapping(&adc_input_map))
        return -ENOENT;
#endif
    

    if ((ADC_CH_INPUT0NP_DIFF == adc_input_map.ch0_input) || (ADC_CH_INPUT0NP_DIFF == adc_input_map.ch1_input)) {
        #ifdef CONFIG_CFG_DRV
        adc_reg->bias = data->external_config.ADC_Bias_Setting;
        adc_reg->bias = 0xfc493a5a;
        #endif
    }else{
        adc_reg->bias = 0x05493a5a;
    }
    


    /* BIAS enable */
    adc_reg->adc_com_anactl |= ADC_COM_ANACTL_BIASEN;
}

/* @brief AUDIO LDO initialization */
static void adc_ldo_init(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    const struct phy_adc_config_data *cfg = dev->config;

    /* set the AUDIO LDO output voltage */
    adc_reg->ref_ldo_ctl &= ~ADC_REF_LDO_CTL_ADLDO_PD_CTL_MASK;
    adc_reg->ref_ldo_ctl &= ~ADC_REF_LDO_CTL_ADLDO_VOL_MASK;
    adc_reg->ref_ldo_ctl |= ADC_REF_LDO_CTL_ADLDO_VOL(PHY_DEV_FEATURE(ldo_voltage));
}

/* @brief Power control(enable or disable) by ADC LDO */
static void adc_ldo_power_control(struct device *dev, bool enable)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    uint32_t reg = adc_reg->ref_ldo_ctl;

    if (enable) {
        /** FIXME: HW issue
         * ADC LDO shall be enabled when use DAC individually, otherwise VREF_ADD will get low voltage.
         */

        /* AULDO pull down current control */
        reg &= ~ADC_REF_LDO_CTL_ADLDO_PD_CTL_SHIFT;
        reg |= ADC_REF_LDO_CTL_ADLDO_PD_CTL(0);

        /* VREF voltage control */
        reg &= ~ADC_REF_LDO_CTL_VREF_VOL_MASK;
        reg |= ADC_REF_LDO_CTL_VREF_VOL(1);

        /*VREF OP bias control*/
        reg &= ~ADC_REF_LDO_CTL_IB_VREF_MASK;
        reg |= ADC_REF_LDO_CTL_IB_VREF(1);

        /*ADLDO enable for ADC*/
        reg |= ADC_REF_LDO_CTL_ADLDO_EN(3);

        adc_reg->ref_ldo_ctl = reg;
        adc_reg->ref_ldo_ctl |= ADC_REF_LDO_CTL_VREF_EN;

        if (!(reg & ADC_REF_LDO_CTL_VREF_EN)) {
            LOG_INF("ADC wait for capacitor charge full");
            /* enable VREF fast charge */
            adc_reg->ref_ldo_ctl |= ADC_REF_LDO_CTL_VREF_FU;

            if (!z_is_idle_thread_object(_current))
                k_sleep(K_MSEC(ADC_LDO_CAPACITOR_CHARGE_TIME_MS));
            else
                k_busy_wait(ADC_LDO_CAPACITOR_CHARGE_TIME_MS * 1000UL);

            /* disable LDO fast charge */
            adc_reg->ref_ldo_ctl &= ~ADC_REF_LDO_CTL_VREF_FU;
        }

        /* Wait for AULDO stable */
        if (!z_is_idle_thread_object(_current))
            k_sleep(K_MSEC(1));
        else
            k_busy_wait(1000);

        /* reduce AULDO static power consume */
        uint32_t reg1 = adc_reg->ref_ldo_ctl;
        reg1 &= ~ADC_REF_LDO_CTL_ADLDO_PD_CTL_MASK;
        reg1 &= ~ADC_REF_LDO_CTL_VREF_VOL_MASK;
        reg1 |= ADC_REF_LDO_CTL_VREF_VOL(1);
        adc_reg->ref_ldo_ctl = reg1;
    } else {
        /* check DAC LDO status */
        uint32_t key = irq_lock();
        if (!(reg & ADC_REF_LDO_CTL_DALDO_EN_MASK)) {
            reg &= ~ADC_REF_LDO_CTL_ADLDO_EN_MASK;
            reg &= ~ADC_REF_LDO_CTL_VREF_EN;
            adc_reg->ref_ldo_ctl = reg;
        }
        irq_unlock(key);
    }
}

/* @brief ADC physical level enable procedure */
static int phy_adc_enable(struct device *dev, void *param)
{
    const struct phy_adc_config_data *cfg = dev->config;
    ain_param_t *in_param = (ain_param_t *)param;
    adc_setting_t *adc_setting = in_param->adc_setting;
	struct phy_adc_drv_data *data = dev->data;
    int ret;

    if ((!in_param) || (!adc_setting)
        || (!in_param->sample_rate)) {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }

    if (in_param->channel_type != AUDIO_CHANNEL_ADC) {
        LOG_ERR("Invalid channel type %d", in_param->channel_type);
        return -EINVAL;
    }

    ret = adc_check_fifo_busy(dev);
    if (ret)
        return ret;
    
    data->audiopll_mode = in_param->audiopll_mode;
    printk("%s, audiopll mode:%d\n", __func__, data->audiopll_mode);

    /* enable adc clock */
    acts_clock_peripheral_enable(cfg->clk_id);

    /* enable adc ldo */
    adc_ldo_power_control(dev, true);

    __adc_vmic_ctl_init(dev);

    __adc_vmic_ctl_enable(dev, adc_setting->device);

    /* audio_pll and adc clock setting */
    if (adc_sample_rate_set(dev, in_param->sample_rate)) {
        LOG_ERR("Failed to config sample rate %d", in_param->sample_rate);
        return -ESRCH;
    }

    /* ADC FIFO enable */
    ret = adc_fifo_enable(dev, in_param);
    if (ret)
        return ret;

    /* ADC digital enable */
    ret = adc_digital_enable(dev, adc_setting->device,
                        in_param->sample_rate);
    if (ret) {
        LOG_ERR("ADC digital enable error %d", ret);
        goto err;
    }

#ifdef ADC_DIGITAL_DEBUG_IN_ENABLE
    __adc_digital_debug_in(dev);
#endif

#ifdef ADC_ANALOG_DEBUG_OUT_ENABLE
    __adc_analog_debug_out(dev);
#endif

    /* ADC analog input enable */
    ret = adc_input_config(dev, adc_setting->device);
    if (ret) {
        LOG_ERR("ADC input config error %d", ret);
        goto err;
    }
    /* ADC gain setting */
    ret = adc_gain_config(dev, adc_setting->device, &adc_setting->gain);
    if (ret) {
        LOG_ERR("ADC gain config error %d", ret);
        goto err;
    }

    /* set ADC BIAS */
    adc_bias_setting(dev, adc_setting->device);

#ifdef CONFIG_CFG_DRV
    data->input_dev = adc_setting->device;
#endif

    return ret;

err:
    adc_fifo_disable(dev);
    return ret;
}

/* @brief ADC physical level disable procedure */
static int phy_adc_disable(struct device *dev, void *param)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;
    uint16_t input_dev = *(uint16_t *)param;

    LOG_INF("disable input device:0x%x", input_dev);

    __adc_external_trigger_disable(dev);

    /* DAC FIFO reset */
    adc_fifo_disable(dev);

    if (__adc_check_fifo_all_disable(dev)) {
        adc_digital_disable(dev, input_dev);


        adc_reg->adc0_anactl = 0;
        adc_reg->adc1_anactl = 0;

        /* TODO: check DAC depends on ADC issue */
        /*if (!data->anc_en)
            acts_clock_peripheral_disable(cfg->clk_id);*/

        data->hw_trigger_en = 0;

        /*should close hfp mic*/
        __adc_vmic_ctl_disable(dev, input_dev);

        /* disable VMIC power */
        /* disable ADC LDO */
        adc_ldo_power_control(dev, false);
    }

    return 0;
}

/* @brief Get the ADC DMA information */
static int adc_get_dma_info(struct device *dev, struct audio_in_dma_info *info)
{
    const struct phy_adc_config_data *cfg = dev->config;

    /* use ADC FIFO0 */
    info->dma_info.dma_chan = cfg->dma_fifo0.dma_chan;
    info->dma_info.dma_dev_name = cfg->dma_fifo0.dma_dev_name;
    info->dma_info.dma_id = cfg->dma_fifo0.dma_id;

    return 0;
}

static int adc_channels_start(struct device *dev, struct aduio_in_adc_en *ctl)
{
    uint8_t i;
    bool ch0_en = false, ch1_en = false;
    audio_input_map_t adc_input_map = {0};

    if (ctl->input_dev_num > ADC_FIFO_MAX_NUMBER) {
        LOG_ERR("invalid input device number:%d", ctl->input_dev_num);
        return -EINVAL;
    }

    for (i = 0; i < ctl->input_dev_num; i++) {
        LOG_INF("start audio device 0x%x", ctl->input_dev_array[i]);
        adc_input_map.audio_dev = ctl->input_dev_array[i];
#ifdef CONFIG_CFG_DRV
		struct phy_adc_drv_data *data = dev->data;
		adc_input_map.ch0_input = data->external_config.input_ch0;
		adc_input_map.ch1_input = data->external_config.input_ch1;

		if ((ADC_CH_DISABLE != adc_input_map.ch0_input) && (data->input_dev & BIT(0)))
			ch0_en = true;
		if ((ADC_CH_DISABLE != adc_input_map.ch1_input) && (data->input_dev & BIT(1)))
			ch1_en = true;
#else
        if (board_audio_device_mapping(&adc_input_map))
            return -ENOENT;

		if (ADC_CH_DISABLE != adc_input_map.ch0_input)
			ch0_en = true;
		if (ADC_CH_DISABLE != adc_input_map.ch1_input)
			ch1_en = true;
#endif
    }

    __adc_digital_channels_en(dev, ch0_en, ch1_en);

    return 0;
}

/* @brief check ADC is busy or not */
static bool adc_is_busy(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;
    bool is_busy = false;

    if (data->anc_en)
        is_busy = true;

    if (adc_reg->adc_digctl & ADC_DIGCTL_ADC_DIG_MASK)
        is_busy = true;

    return is_busy;
}


/* @brief ADC AEC configuration */
static int adc_aec_config(struct device *dev, bool is_en)
{
	//struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	//uint8_t aec_sel = 0, pull_down = 0;
    uint8_t aec_sel = 0;

#if defined(CONFIG_CFG_DRV)
	struct phy_adc_drv_data *data = dev->data;
	if (data->external_config.Hw_Aec_Select == ADC_1)
		aec_sel = 0;
	else
		aec_sel = 1;
#elif defined(CONFIG_AUDIO_ADC_0_AEC_SEL)
	aec_sel = CONFIG_AUDIO_ADC_0_AEC_SEL;
#endif

    printk("adc_aec_config %d, adc: %d\n",is_en, aec_sel + 2);

#if defined(CONFIG_AUDIO_ADC_0_MIX_PULL_DOWN)
	//pull_down = CONFIG_AUDIO_ADC_0_MIX_PULL_DOWN;
#endif

	if (is_en) {
		/* HW AEC only support ADC2/3 */
		if (aec_sel == 0) {

		} else if (aec_sel == 1) {

		} else {
			LOG_ERR("Invalid HW AEC channel select:%d", aec_sel);
			return -EINVAL;
		}
	} else {
		if (aec_sel == 0) {
		} else if (aec_sel == 1) {
		}
	}

	return 0;
}

/* @brief ADC ioctl commands */
static int phy_adc_ioctl(struct device *dev, uint32_t cmd, void *param)
{
    int ret = 0;

    switch (cmd) {
    case AIN_CMD_SET_APS:
    {
        audio_aps_level_e level = *(audio_aps_level_e *)param;
        ret = adc_set_pll_aps(dev, level);
        if (ret) {
            LOG_ERR("Failed to set audio pll APS err=%d", ret);
            return ret;
        }
        LOG_DBG("set new aps level %d", level);
        break;
    }
        
    case AIN_CMD_SET_APS_F:
    {
        unsigned int level = *(unsigned int *)param;
        ret = adc_set_pll_aps_f(dev, level);
        if (ret) {
            LOG_ERR("Failed to set audio pll APS_f err=%d", ret);
            return ret;
        }
        LOG_DBG("set new aps_f level %d", level);
        break;
    }

    
    case PHY_CMD_DUMP_REGS:
    {
        adc_dump_register(dev);
        break;
    }
    case PHY_CMD_GET_AIN_DMA_INFO:
    {
        ret = adc_get_dma_info(dev, (struct audio_in_dma_info *)param);
        break;
    }
    case PHY_CMD_ADC_DIGITAL_ENABLE:
    {
        ret = adc_channels_start(dev, (struct aduio_in_adc_en *)param);
        break;
    }
    case PHY_CMD_ADC_GAIN_CONFIG:
    {
        adc_setting_t *setting = (adc_setting_t *)param;
        ret = adc_gain_config(dev, setting->device, &setting->gain);
        break;
    }
    case AIN_CMD_GET_SAMPLERATE:
    {
        ret = adc_sample_rate_get(dev);
        if (ret < 0) {
            LOG_ERR("Failed to get ADC sample rate err=%d", ret);
            return ret;
        }
        *(audio_sr_sel_e *)param = (audio_sr_sel_e)ret;
        ret = 0;
        break;
    }
    case AIN_CMD_SET_SAMPLERATE:
    {
        audio_sr_sel_e val = *(audio_sr_sel_e *)param;
        ret = adc_sample_rate_set(dev, val);
        if (ret) {
            LOG_ERR("Failed to set ADC sample rate err=%d", ret);
            return ret;
        }
        break;
    }
    case AIN_CMD_GET_APS:
    {
        ret = adc_get_pll_aps(dev);
        if (ret < 0) {
            LOG_ERR("Failed to get audio pll APS err=%d", ret);
            return ret;
        }
        *(audio_aps_level_e *)param = (audio_aps_level_e)ret;
        ret = 0;
        break;
    }
    case AIN_CMD_GET_APS_F:
    {
        ret = adc_get_pll_aps_f(dev);
        if (ret < 0) {
            LOG_ERR("Failed to get audio pll APS err=%d", ret);
            return ret;
        }
        *(unsigned int *)param = (unsigned int)ret;
        ret = 0;
        break;
    }

    case PHY_CMD_GET_AUDIOPLL_IDX:
    {
        ret = adc_get_pll_index(dev, (uint8_t *)param);
        break;
    }
    case PHY_CMD_FIFO_DRQ_LEVEL_GET:
    {
        uint32_t fifo_cmd = *(uint32_t *)param;
        uint16_t input_dev = PHY_GET_FIFO_CMD_INDEX(fifo_cmd);
        uint8_t level;

        ret = adc_fifo_drq_level_get(dev, &level);
        if (ret < 0)
            return ret;

        *(uint32_t *)param = PHY_FIFO_CMD(input_dev, level);
        ret = 0;

        break;
    }
    case PHY_CMD_FIFO_DRQ_LEVEL_SET:
    {
        uint32_t fifo_cmd = *(uint32_t *)param;
        uint8_t level = PHY_GET_FIFO_CMD_VAL(fifo_cmd);

        ret = adc_fifo_drq_level_set(dev, level);

        break;
    }
    case AIN_CMD_SET_ADC_TRIGGER_SRC:
    {
        uint8_t src = *(uint8_t *)param;
        ret = __adc_external_trigger_enable(dev, src);
        break;
    }
    
    case PHY_CMD_IS_ADC_BUSY:
    {
        *(uint8_t *)param = (uint8_t)adc_is_busy(dev);
        break;
    }
	case AIN_CMD_AEC_CONTROL:
	{
		bool en = *(bool *)param;
		ret = adc_aec_config(dev, en);
		break;
	}

    case PHY_CMD_ADC_FIFO_ENABLE:
    {
        ret = adc_fifo_final_enable(dev);            
        break;
    }
    
    default:
        LOG_ERR("Unsupport command %d", cmd);
        return -ENOTSUP;
    }

    return ret;
}

const struct phy_audio_driver_api phy_adc_drv_api = {
    .audio_enable = phy_adc_enable,
    .audio_disable = phy_adc_disable,
    .audio_ioctl = phy_adc_ioctl
};

/* dump dac device tree infomation */
static void __adc_dt_dump_info(const struct phy_adc_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
    LOG_INF("**     ADC BASIC INFO     **");
    LOG_INF("     BASE: %08x", cfg->reg_base);
    LOG_INF("   CLK-ID: %08x", cfg->clk_id);
    LOG_INF("   RST-ID: %08x", cfg->rst_id);
    LOG_INF("DMA0-NAME: %s", cfg->dma_fifo0.dma_dev_name);
    LOG_INF("  DMA0-ID: %08x", cfg->dma_fifo0.dma_id);
    LOG_INF("  DMA0-CH: %08x", cfg->dma_fifo0.dma_chan);

    LOG_INF("** 	ADC FEATURES	 **");
    LOG_INF("   ADC0-HPF-TIME: %d", PHY_DEV_FEATURE(adc0_hpf_time));
    LOG_INF("ADC0-HPF-FC-HIGH: %d", PHY_DEV_FEATURE(adc0_hpf_fc_high));
    LOG_INF("   ADC1-HPF-TIME: %d", PHY_DEV_FEATURE(adc1_hpf_time));
    LOG_INF("ADC1-HPF-FC-HIGH: %d", PHY_DEV_FEATURE(adc1_hpf_fc_high));
    LOG_INF("  ADC0-FREQUENCY: %d", PHY_DEV_FEATURE(adc0_frequency));
    LOG_INF("  ADC1-FREQUENCY: %d", PHY_DEV_FEATURE(adc1_frequency));
    LOG_INF(" FAST-CAP-CHARGE: %d", PHY_DEV_FEATURE(fast_cap_charge));
    LOG_INF("     LDO-VOLTAGE: %d", PHY_DEV_FEATURE(ldo_voltage));
    LOG_INF("     VMIC-CTL-EN: <%d, %d, %d>",
            vmic_ctl_array[0], vmic_ctl_array[1], vmic_ctl_array[2]);
    LOG_INF("    VMIC-CTL-VOL: <%d, %d>",
            vmic_voltage_array[0], vmic_voltage_array[1]);
#endif
}

#ifdef CONFIG_CFG_DRV
static int __adc_config_dmic_mfp(struct device *dev, uint16_t clk_pin, uint16_t dat_pin)
{
    int ret;
    uint16_t pin, mfp;

    pin = PHY_AUDIO_PIN_NUM_CFG(clk_pin);
    mfp = PHY_AUDIO_PIN_MFP_CFG(clk_pin);
    ret = acts_pinmux_set(pin, mfp);
    if (ret) {
        LOG_ERR("pin@%d config mfp error:%d", pin, mfp);
        return ret;
    }

    pin = PHY_AUDIO_PIN_NUM_CFG(dat_pin);
    mfp = PHY_AUDIO_PIN_MFP_CFG(dat_pin);
    ret = acts_pinmux_set(pin, mfp);
    if (ret) {
        LOG_ERR("pin@%d config mfp error:%d", pin, mfp);
        return ret;
    }

    return 0;
}

/* @brief Configure PINMUX for DMIC(Digital MIC) */
static int adc_config_dmic_mfp(struct device *dev, uint8_t ch0_input,
            uint8_t ch1_input)
{
    int ret = 0;
    struct phy_adc_drv_data *data = dev->data;

    if ((ADC_CH_DMIC == ch0_input)
        || (ADC_CH_DMIC == ch1_input)) {
        if ((data->external_config.DMIC_Select_GPIO.DMIC01_CLK != GPIO_NONE)
            && (data->external_config.DMIC_Select_GPIO.DMIC01_DAT != GPIO_NONE)) {
            ret = __adc_config_dmic_mfp(dev,
                    data->external_config.DMIC_Select_GPIO.DMIC01_CLK,
                    data->external_config.DMIC_Select_GPIO.DMIC01_DAT);
        }
    }


    return ret;
}

/* @brief parser ADC INPUT from  */
static int adc_dmic_mfp_init(const struct device *dev)
{
	struct phy_adc_drv_data *data = dev->data;

	if ((data->external_config.input_ch0 == ADC_CH_DMIC)
		|| (data->external_config.input_ch1 == ADC_CH_DMIC)) 
	{
		/* configure DMIC MFP */
		if (adc_config_dmic_mfp((struct device *)dev, data->external_config.input_ch0,
			data->external_config.input_ch1)) {
			LOG_ERR("DMIC MFP config error");
			return -ENXIO;
		}
	}

	LOG_INF("ADC input parser ch0:%d ch1:%d",
			data->external_config.input_ch0, data->external_config.input_ch1);

	return 0;
}

/* @brief initialize ADC external configuration */
static int phy_adc_config_init(const struct device *dev)
{
    struct phy_adc_drv_data *data = dev->data;
    int ret;

    /* CFG_Struct_Audio_Settings */
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ADC_BIAS_SETTING, ADC_Bias_Setting);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_DMIC01_CHANNEL_ALIGNING, DMIC01_Channel_Aligning);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_HW_AEC_SELECT, Hw_Aec_Select);
	PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_MAIN_MIC, main_mic);
	PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_SUB_MIC,  sub_mic);

	PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ADC0_INPUT, input_ch0);
	PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ADC1_INPUT, input_ch1);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ADC0_SEL_VMIC, adc0_sel_vmic);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ADC1_SEL_VMIC, adc1_sel_vmic);

    data->external_config.aec_ch = ADC_CHANNEL_0;  //only ADC2 and ADC3 support aec, ADC0 and ADC1 not support aec.


    /* CFG_Type_DMIC_Select_GPIO */
    ret = cfg_get_by_key(ITEM_AUDIO_DMIC_SELECT_GPIO,
            &data->external_config.DMIC_Select_GPIO, sizeof(data->external_config.DMIC_Select_GPIO));
    if (ret) {
        LOG_INF("** DMIC PINMUX **");
        LOG_INF("DMIC01_CLK:%d", data->external_config.DMIC_Select_GPIO.DMIC01_CLK);
        LOG_INF("DMIC01_DAT:%d", data->external_config.DMIC_Select_GPIO.DMIC01_DAT);
    }

    adc_dmic_mfp_init(dev);

	return 0;
}
#endif

static int phy_adc_init(const struct device *dev)
{
    const struct phy_adc_config_data *cfg = dev->config;
    struct phy_adc_drv_data *data = dev->data;

    __adc_dt_dump_info(cfg);

    /* reset ADC controller */
    acts_reset_peripheral(cfg->rst_id);
    acts_clock_peripheral_enable(cfg->clk_id);

    memset(data, 0, sizeof(struct phy_adc_drv_data));

    adc_ldo_init((struct device *)dev);

#ifdef CONFIG_CFG_DRV
    int ret;
    ret = phy_adc_config_init(dev);
    if (ret)
        LOG_ERR("ADC external config init error:%d", ret);
#endif

    printk("ADC init successfully\n");

    return 0;
}

/* physical adc driver data */
static struct phy_adc_drv_data phy_adc_drv_data0;

/* physical adc config data */
static const struct phy_adc_config_data phy_adc_config_data0 = {
    .reg_base = AUDIO_ADC_REG_BASE,
    AUDIO_DMA_FIFO_DEF(ADC, 0),
    .clk_id = CLOCK_ID_ADC,
    .rst_id = RESET_ID_ADC,

    PHY_DEV_FEATURE_DEF(adc0_hpf_time) = CONFIG_AUDIO_ADC_0_CH0_HPF_TIME,
    PHY_DEV_FEATURE_DEF(adc0_hpf_fc_high) = CONFIG_AUDIO_ADC_0_CH0_HPF_FC_HIGH,
    PHY_DEV_FEATURE_DEF(adc1_hpf_time) = CONFIG_AUDIO_ADC_0_CH1_HPF_TIME,
    PHY_DEV_FEATURE_DEF(adc1_hpf_fc_high) = CONFIG_AUDIO_ADC_0_CH1_HPF_FC_HIGH,
    PHY_DEV_FEATURE_DEF(adc0_frequency) = CONFIG_AUDIO_ADC_0_CH0_FREQUENCY,
    PHY_DEV_FEATURE_DEF(adc1_frequency) = CONFIG_AUDIO_ADC_0_CH1_FREQUENCY,
    PHY_DEV_FEATURE_DEF(ldo_voltage) = CONFIG_AUDIO_ADC_0_LDO_VOLTAGE,
    PHY_DEV_FEATURE_DEF(fast_cap_charge) = CONFIG_AUDIO_ADC_0_FAST_CAP_CHARGE,
};

#if IS_ENABLED(CONFIG_AUDIO_ADC_0)
DEVICE_DEFINE(adc0, CONFIG_AUDIO_ADC_0_NAME, phy_adc_init, NULL,
        &phy_adc_drv_data0, &phy_adc_config_data0,
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_adc_drv_api);
#endif
