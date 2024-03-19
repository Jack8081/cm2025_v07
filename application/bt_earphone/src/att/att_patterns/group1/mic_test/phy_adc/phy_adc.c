#include "soc_common.h"
#include "phy_adc.h"
#define ADC_CH2REG(base, x)                ((uint32_t)&((base)->ch0_digctl) + ((x) << 2))
#define ADC_CTL2REG(base, x)               ((uint32_t)&((base)->adc0_anactl) + ((x) << 2))

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

/**
 * enum a_att_adc_ch_e
 * @beief ADC channels selection
 */
typedef enum {
    ADC_CHANNEL_0 = 0,
    ADC_CHANNEL_1,
} a_att_adc_ch_e;

typedef enum {
    ADC_AMIC = 0, /* analog mic */
    ADC_DMIC /* digital mic */
} a_att_adc_ch_type_e;

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


/**
 * @struct acts_audio_adc
 * @brief ADC controller hardware register
 */
struct att_audio_adc {
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


/* @brief  Get the ADC controller base address */
static inline struct att_audio_adc *_att_get_adc_reg_base(void)
{
    return (struct att_audio_adc *)AUDIO_ADC_REG_BASE;
}


/* @brief AUDIO LDO initialization */
static void _att_adc_ldo_enable(void)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();

    /* set the AUDIO LDO output voltage */
    adc_reg->ref_ldo_ctl &= ~ADC_REF_LDO_CTL_ADLDO_PD_CTL_MASK;
    adc_reg->ref_ldo_ctl &= ~ADC_REF_LDO_CTL_ADLDO_VOL_MASK;
    adc_reg->ref_ldo_ctl |= ADC_REF_LDO_CTL_ADLDO_VOL(2);

    uint32_t reg = adc_reg->ref_ldo_ctl;

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

    /* enable VREF fast charge */
    adc_reg->ref_ldo_ctl |= ADC_REF_LDO_CTL_VREF_FU;

    mdelay(5);

    /* disable LDO fast charge */
    adc_reg->ref_ldo_ctl &= ~ADC_REF_LDO_CTL_VREF_FU;

    /* Wait for AULDO stable */
    mdelay(2);

    /* reduce AULDO static power consume */
    uint32_t reg1 = adc_reg->ref_ldo_ctl;
    reg1 &= ~ADC_REF_LDO_CTL_ADLDO_PD_CTL_MASK;
    reg1 &= ~ADC_REF_LDO_CTL_VREF_VOL_MASK;
    reg1 |= ADC_REF_LDO_CTL_VREF_VOL(1);
    adc_reg->ref_ldo_ctl = reg1;
}

static void _att_adc_ldo_disable(void)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
    uint32_t reg = adc_reg->ref_ldo_ctl;

    reg &= ~ADC_REF_LDO_CTL_ADLDO_EN_MASK;
    reg &= ~ADC_REF_LDO_CTL_VREF_EN;
    adc_reg->ref_ldo_ctl = reg;
}

static void _att_adc_vmic_ctl_enable(void)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
    uint32_t reg;

    reg = adc_reg->vmic_ctl;
    //reg |= (ADC_VMIC_CTL_ISO_AVCC_AU | ADC_VMIC_CTL_ISO_VD18);

    reg &= ~ADC_VMIC_CTL_VMIC0_VOL_MASK;
    reg |= ADC_VMIC_CTL_VMIC0_VOL(2);

    reg &= ~ADC_VMIC_CTL_VMIC1_VOL_MASK;
    reg |= ADC_VMIC_CTL_VMIC1_VOL(2);

    reg &= ~ADC_VMIC_CTL_VMIC0_EN_MASK;
    reg |= ADC_VMIC_CTL_VMIC0_EN(3); /* enable VMIC0 OP */

    reg &= ~ADC_VMIC_CTL_VMIC1_EN_MASK;
    reg |= ADC_VMIC_CTL_VMIC1_EN(3); /* enable VMIC1 OP */

    adc_reg->vmic_ctl = reg;

}

/* @brief audio pll set */
static void _att_audio_pll_set(a_pll_type_e index, a_pll_series_e series, a_pll_mode_e pll_mode)
{
	uint32_t reg;

    /* Enable AUDIO_PLL0 */
    reg = sys_read32(AUDIO_PLL0_CTL) & (~CMU_AUDIOPLL0_CTL_APS0_MASK);
    reg |= (1 << CMU_AUDIOPLL0_CTL_EN);

    if(AUDIOPLL_INTEGER_N == pll_mode){
        sys_write32(0x2a8a4, AUDIO_PLL0_DEBUG);
        reg &= ~CMU_AUDIOPLL0_CTL_MODE_F;
        if (AUDIOPLL_44KSR == series){
            reg |= (0x04 << CMU_AUDIOPLL0_CTL_APS0_SHIFT);
        }else{
            reg |= (0x0c << CMU_AUDIOPLL0_CTL_APS0_SHIFT);
        }
    }else{
        sys_write32(0x2be24, AUDIO_PLL0_DEBUG);
        reg |= CMU_AUDIOPLL0_CTL_MODE_F;
        if (AUDIOPLL_44KSR == series){
            reg |= (0x10 << CMU_AUDIOPLL0_CTL_APS0_SHIFT);
        }else{
            reg |= (0x30 << CMU_AUDIOPLL0_CTL_APS0_SHIFT);
        }
        
    }
    sys_write32(reg, AUDIO_PLL0_CTL);

    //att_buf_printf("AUDIO_PLL0_CTL - 0x%x\n", sys_read32(AUDIO_PLL0_CTL));
    //att_buf_printf("AUDIOPLL_DEBUG - 0x%x\n", sys_read32(AUDIOPLL_DEBUG));
}

/* @brief ADC sample rate config */
static int _att_adc_sample_rate_set(audio_sr_sel_e sr_khz)
{
     uint8_t clk_div, firclkdiv, ovfsclkdiv, fs_sel;
     uint32_t reg;
     uint8_t i;


     for (i = 0; i < ARRAY_SIZE(adc_clk_mapping); i++) {
         if (sr_khz == adc_clk_mapping[i].sample_rate) {
             clk_div = adc_clk_mapping[i].clk_div;
             ovfsclkdiv = adc_clk_mapping[i].ovfsclkdiv;
             firclkdiv = adc_clk_mapping[i].firclkdiv;
             fs_sel = adc_clk_mapping[i].fs_sel;
         }
     }

    /*
     * 16K: clk_div:1; ovfsclkdiv:0; firclkdiv:1; fs_sel:ADC_OVFS_384FS
     */
    //clk_div = 1;
    //ovfsclkdiv = 0;
    //firclkdiv = 1;
    //fs_sel = ADC_OVFS_384FS;


    /*
     * 8K: clk_div:1; ovfsclkdiv:0; firclkdiv:3; fs_sel:ADC_OVFS_768FS
     */
    //clk_div = 1;
    //ovfsclkdiv = 0;
    //firclkdiv = 3;
    //fs_sel = ADC_OVFS_768FS;


     /* set audiopll */
    _att_audio_pll_set(AUDIOPLL_TYPE_0, AUDIOPLL_48KSR, AUDIOPLL_INTEGER_N);

    reg = sys_read32(CMU_ADCCLK) & ~0x1FF;

    /* Select pll0 */
    reg |= (AUDIOPLL_TYPE_0 & 0x1) << CMU_ADCCLK_ADCCLKSRC_SHIFT;
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
    reg |= CMU_ADCCLK_ADCCICEN;  //ADC CIC Filter enable
    reg |= CMU_ADCCLK_ADCFIREN;  //ADC FIR Filter CLK enable
    reg |= CMU_ADCCLK_ADCFIFOCLKEN;  //ADC FIFO Access Clock enable

    sys_write32(reg, CMU_ADCCLK);

    return 0;
}

/* @brief Translate the AMIC/AUX gain from dB fromat to hardware register value */
static int _att_adc_aux_amic_gain_translate(int16_t gain, uint8_t *darsel, uint8_t *input_ics)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(amic_aux_gain_mapping); i++) {
        if (gain <= amic_aux_gain_mapping[i].gain) {
            *darsel = amic_aux_gain_mapping[i].darsel;
            *input_ics = amic_aux_gain_mapping[i].input_ics;
            att_buf_printf("again:%d map [%d %d]\n",
                gain, *darsel, *input_ics);
            break;
        }
    }

    if (i == ARRAY_SIZE(amic_aux_gain_mapping)) {
        att_buf_printf("can not find out gain map %d\n", gain);
        return -ENOENT;
    }

    return 0;
}

/* @brief ADC input analog gain setting */
static int _att_adc_input_gain_set(a_att_adc_ch_e ch, uint8_t darsel,
                                    uint8_t inpu_ics, bool update_fb)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
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

/* @brief Translate the gain from dB fromat to hardware register value */
static int _att_adc_mic_dgain_translate(int16_t gain, uint8_t *sgain, uint8_t *fgain)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(mic_dgain_mapping); i++) {
        if (gain <= mic_dgain_mapping[i].gain) {
            *sgain = mic_dgain_mapping[i].sgain;
            *fgain = mic_dgain_mapping[i].fgain;
            att_buf_printf("dgain:%d map [sg:%d, fg:%d]\n", gain, *sgain, *fgain);
            break;
        }
    }
                                    
    if (i == ARRAY_SIZE(mic_dgain_mapping)) {
        att_buf_printf("can not find out gain map %d\n", gain);
        return -1;
    }
                                    
    return 0;
}

/* @brief ADC channel digital gain setting */
static void _att_adc_digital_gain_set(a_att_adc_ch_e ch, uint8_t sgain, uint8_t fgain)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
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


static int _att_adc_gain_config(att_adc_gain_t *gain)
{
    uint8_t ch0_input, ch1_input;
    int16_t ch0_again = 0, ch1_again = 0;
    int16_t ch0_dgain = 0, ch1_dgain = 0;
    uint8_t darsel, input_res;
    uint8_t fgain, sgain;

    ch0_input = ADC_CH_DISABLE;
    ch1_input = ADC_CH_DISABLE;

    /* gain set when input channel enabled */
    ch0_again = gain->ch_again[0];
    ch0_dgain = gain->ch_dgain[0];
    ch1_again = gain->ch_again[1];
    ch1_dgain = gain->ch_dgain[1];

    if (_att_adc_mic_dgain_translate(ch0_dgain, &sgain, &fgain)) {
        att_buf_printf("failed to translate mic ch0 dgain %d\n", ch0_dgain);
        sgain = 2;
        fgain = 1;
    }
    
    _att_adc_digital_gain_set(ADC_CHANNEL_0, sgain, fgain);

    if (_att_adc_mic_dgain_translate(ch1_dgain, &sgain, &fgain)) {
        att_buf_printf("failed to translate mic ch1 dgain %d\n", ch1_dgain);
        sgain = 2;
        fgain = 1;
    }else{
    }
    _att_adc_digital_gain_set(ADC_CHANNEL_1, sgain, fgain);

    /* config channel0 again */
    if (_att_adc_aux_amic_gain_translate(ch0_again, &darsel, &input_res)) {
        att_buf_printf("failed to translate amic_aux ch0 gain %d\n", ch0_again);
        darsel = 7;
        input_res = 3; 
    } else {
    }
    _att_adc_input_gain_set(ADC_CHANNEL_0, darsel, input_res, true);

    /* config channel1 again */
    if (_att_adc_aux_amic_gain_translate(ch1_again, &darsel, &input_res)) {
        att_buf_printf("failed to translate amic_aux ch0 gain %d\n", ch0_again);
        darsel = 7;
        input_res = 3; 
    } else {
    }
    _att_adc_input_gain_set(ADC_CHANNEL_1, darsel, input_res, true);

    return 0;
}


/* @brief disable ADC FIFO by specified FIFO index */
static void _att_adc_fifo_disable(void)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();

    adc_reg->fifoctl &= (~ADC_FIFOCTL_ADF0RT);
    adc_reg->fifoctl &= (~ADC_FIFOCTL_ADF0FDE);
    adc_reg->fifoctl &= (~ADC_FIFOCTL_ADF0FIE);

    /* disable ADC FIFO0 access clock */
    sys_write32(sys_read32(CMU_ADCCLK) & ~CMU_ADCCLK_ADCFIFOCLKEN, CMU_ADCCLK);
}

/* @brief enable ADC FIFO by specified FIFO index */
static int _att_adc_fifo_enable(void)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
    uint32_t reg = adc_reg->fifoctl;

    reg &= ~0x3FFF; /* clear FIFO0 fields */

    /* enable DRQ */
    reg |= (ADC_FIFOCTL_ADF0FDE);

    /*ADC FIFO0 output Select: 0:CPU; 1:DMA*/
    reg |= ADC_FIFOCTL_ADF0OS(0x1) | ADC_FIFOCTL_ADF0RT;
    /* width 0:32bits; 1:16bits */
    reg |= ADC_FIFOCTL_ADCFIFO0_DMAWIDTH;
    reg |= ADC_FIFOCTL_IDRQ_LEVEL(8);
    adc_reg->fifoctl = reg;

    /* enable ADC FIFO to access ADC CLOCK */
    sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCFIFOCLKEN, CMU_ADCCLK);

    return 0;
}


/* @brief ADC FIFO enable on the basic of the input audio device usage */
int att_adc_fifo_enable(void)
{
    _att_adc_fifo_disable();
    return _att_adc_fifo_enable();
}

/* @brief ADC digital CIC over sample rate and FIR mode setting */
static void _att_adc_digital_ovfs_fir_cfg(audio_sr_sel_e sr)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
    uint32_t reg;
    a_adc_fir_e fir;
    a_adc_ovfs_e ovfs = ADC_OVFS_128FS;
    uint8_t i;

    /* Configure the Programmable frequency response curve according with the sample rate */
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
    
    //fir = ADC_FIR_MODE_B;

    reg = adc_reg->adc_digctl;
    reg &= ~(ADC_DIGCTL_ADC_OVFS_MASK | ADC_DIGCTL_ADC_FIR_MD_SEL_MASK);

    for (i = 0; i < ARRAY_SIZE(adc_clk_mapping); i++) {
        if (sr == adc_clk_mapping[i].sample_rate) {
            ovfs = adc_clk_mapping[i].ovfs;
        }
    }

    //ovfs = ADC_OVFS_384FS;  //16k
    //ovfs = ADC_OVFS_768FS;  //8k
    
    /* ovfs */
    reg |= ADC_DIGCTL_ADC_OVFS(ovfs);

    reg |= ADC_DIGCTL_ADC_FIR_MD_SEL(fir);

    adc_reg->adc_digctl = reg;
}

/* @brief ADC HPF configuration for fast stable */
static void _att_adc_hpf_fast_stable(uint16_t input_dev)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
    uint8_t i;
    uint32_t reg, ch_digctl, en_flag = 0;

    uint16_t fc = (ADC_HPF_HIGH_FREQ_HZ + 29) / 30; /* under 48kfs */

    en_flag |= BIT(0);
    en_flag |= BIT(1);
    fc *= 3;

    for (i = 0; i < 2; i++) {
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
        mdelay(30);
}

static void _att_adc_hpf_control(a_att_adc_ch_e ch, bool enable)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
    uint32_t reg, ch_digctl = ADC_CH2REG(adc_reg, ch);
    bool is_high = 0;
    uint8_t frequency = 0;

    if (!enable) {
        sys_write32(sys_read32(ch_digctl) & ~CH0_DIGCTL_HPFEN, ch_digctl);
        return ;
    }

    if (ADC_CHANNEL_0 == ch) {
        is_high = 0;
        frequency = 0;
    } else if (ADC_CHANNEL_1 == ch) {
        is_high = 0;
        frequency = 0;
    } else{
    }
    /* clear HPF_S and HPF_N */
    reg = sys_read32(ch_digctl) & (~(0x7f << CH0_DIGCTL_HPF_N_SHIFT));
    reg |= CH0_DIGCTL_HPF_N(frequency);

    if (is_high)
        reg |= CH0_DIGCTL_HPF_S;

    /* enable HPF */
    reg |= CH0_DIGCTL_HPFEN;

    sys_write32(reg, ch_digctl);

}

/* @brief ADC channel digital configuration */
static void _att_adc_digital_channel_cfg(a_att_adc_ch_e ch, a_att_adc_ch_type_e type, bool out_en)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
    uint32_t reg0, reg1;
    uint32_t ch_digital = ADC_CH2REG(adc_reg, ch);

    reg0 = adc_reg->adc_digctl;
    reg1 = sys_read32(ch_digital);

    att_buf_printf("channel:%d type:%d enable:%d\n", ch, type, out_en);

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


/* @brief ADC channels enable according to the input audio device */
static int _att_adc_channels_enable(uint16_t input_dev)
{

    if(input_dev & (1 << 0)){
        _att_adc_hpf_control(ADC_CHANNEL_0, true);
        _att_adc_digital_channel_cfg(ADC_CHANNEL_0, ADC_AMIC, true); \
    }

    
    if(input_dev & (1 << 1)){
        _att_adc_hpf_control(ADC_CHANNEL_1, true);
        _att_adc_digital_channel_cfg(ADC_CHANNEL_1, ADC_AMIC, true); \
    }
    return 0;
}


/* @brief  Enable the ADC digital function */
static int _att_adc_digital_enable(uint16_t input_dev, audio_sr_sel_e sr)
{
    /* configure OVFS and FIR */
    _att_adc_digital_ovfs_fir_cfg(sr);

    /* set HPF high frequency range for fast stable */
    _att_adc_hpf_fast_stable(input_dev);

    return _att_adc_channels_enable(input_dev);
}


/* @brief ADC channel0 analog control */
static int _att_adc_ch0_analog_control(uint8_t inputx)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
    uint32_t reg = adc_reg->adc0_anactl;

    if ((inputx != ADC_CH_INPUT0P) && (inputx != ADC_CH_INPUT0NP_DIFF)) {
        att_buf_printf("invalid input:0x%x for ADC0\n", inputx);
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
static int _att_adc_ch1_analog_control(uint8_t inputx)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
    uint32_t reg = adc_reg->adc1_anactl;


    if ((inputx != ADC_CH_INPUT1P) && (inputx != ADC_CH_INPUT1NP_DIFF)) {
        att_buf_printf("invalid input:0x%x for ADC1\n", inputx);
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

/* @brief ADC channel0 analog control */
static int _att_adc_com_analog_control(uint8_t ch0_inputx, uint8_t ch1_inputx)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
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
    mdelay(40);
 
    adc_reg->adc0_anactl &= ~ADC0_ANACTL_OPFAU;
    adc_reg->adc1_anactl |= ADC0_ANACTL_OPFAU;

    return 0;
}

/* @brief ADC fast capacitor charge function */
static void _att_adc_fast_cap_charge(uint16_t input_dev)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
    //fast up
    adc_reg->adc_com_anactl |=  0x1; //bias en
    adc_reg->adc0_anactl |=  (1<<7);
    adc_reg->adc1_anactl &= ~(1<<7);
    adc_reg->bias |= (0xa<<28); //diff
    adc_reg->bias |= (0x3<<20);
    adc_reg->bias |= (0x1<<27);

    //Delay 12ms
    mdelay(12);

    //fast up ok
    adc_reg->adc0_anactl &= ~(1<<7);
    adc_reg->adc1_anactl |=  (1<<7);
    adc_reg->bias &= ~ (0xf<<28);
    adc_reg->bias &= ~(0x3<<20);
    adc_reg->bias &= ~(0x1<<27);
}


/* @brief ADC input channel analog configuration */
static int _att_adc_analog_input_config(uint16_t input_dev)
{
    int ret;

    _att_adc_fast_cap_charge(input_dev);

    if(input_dev & (1 << 0)){
        ret = _att_adc_ch0_analog_control(ADC_CH_INPUT0P);
        if (ret){
            att_buf_printf("%s,%d\n", __func__, __LINE__);
            return ret;
        }
    }
    
    if(input_dev & (1 << 1)){
        ret = _att_adc_ch1_analog_control(ADC_CH_INPUT1P);
        if (ret){
            att_buf_printf("%s,%d\n", __func__, __LINE__);
            return ret;
        }
    }
    ret = _att_adc_com_analog_control(ADC_CH_INPUT0P, ADC_CH_INPUT1P);
    if (ret){
        att_buf_printf("%s,%d\n", __func__, __LINE__);
        return ret;
    }
    return 0;
}

/* @brief ADC BIAS setting for power saving */
static void _att_adc_bias_setting(uint32_t bias)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();

    adc_reg->bias = bias;

    /* BIAS enable */
    adc_reg->adc_com_anactl |= ADC_COM_ANACTL_BIASEN;
}

/* @brief ADC channels enable at the same time */
static void _att_adc_digital_channels_en(bool ch0_en, bool ch1_en)
{
    struct att_audio_adc *adc_reg = _att_get_adc_reg_base();
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

    adc_reg->adc_digctl = reg;
}


int32_t att_phy_audio_adc_enable(ain_track_e channel_id, ain_channel_param_t *channel_param)
{
    uint16_t input_dev;
    
    if(channel_id == INPUTSRC_ONLY_L){
        input_dev = 0x1;
    }else if(channel_id == INPUTSRC_ONLY_R){
        input_dev = 0x2;
    }else{
        input_dev = 0x3;
    }

    /* reset ADC controller */
    att_reset_peripheral(RESET_ID_ADC);
    att_clock_peripheral_enable(CLOCK_ID_ADC);

    _att_adc_sample_rate_set(channel_param->ain_sample_rate);

    _att_adc_ldo_enable();
    _att_adc_vmic_ctl_enable();
    att_adc_fifo_enable();

    _att_adc_digital_enable(input_dev, channel_param->ain_sample_rate);
    _att_adc_analog_input_config(input_dev);

    _att_adc_gain_config(&channel_param->ain_gain);

    _att_adc_bias_setting(channel_param->adc_bias);

    return 0;
}

int32_t  att_phy_audio_adc_disable(ain_track_e channel_id)
{
    _att_adc_fifo_disable();
    _att_adc_ldo_disable();
    
    att_reset_peripheral(RESET_ID_ADC);
    att_clock_peripheral_disable(CLOCK_ID_ADC);

    return 0;
}


int32_t att_phy_audio_adc_start(ain_track_e channel_id)
{
    bool ch0_en = false, ch1_en = false;
    if(channel_id == INPUTSRC_ONLY_L){
        ch0_en = true;
    }else if(channel_id == INPUTSRC_ONLY_R){   
        ch1_en = true;
    }else{
        ch0_en = true;
        ch1_en = true;
    }
    
    _att_adc_digital_channels_en(ch0_en, ch1_en);
    return 0;
}

