/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dynamic voltage and frequency scaling interface
 */

#ifndef	_DVFS_H_
#define	_DVFS_H_

#ifndef _ASMLANGUAGE

/*
 * dvfs levels
 */
typedef enum
{
    DVFS_LEVEL_LOW = 1,
    DVFS_LEVEL_S2 = 2,
    DVFS_LEVEL_NORMAL_NO_DRC = 3,
    DVFS_LEVEL_NORMAL = 4,
    DVFS_LEVEL_NORMAL_1 = 5,
    DVFS_LEVEL_NORMAL_2 = 6,
    DVFS_LEVEL_NORMAL_3 = 7,
    DVFS_LEVEL_NORMAL_4 = 8,
    DVFS_LEVEL_NORMAL_5 = 9,
    //DVFS_LEVEL_NORMAL_6 = 10,
    DVFS_LEVEL_NORMAL_7 = 11,
    DVFS_LEVEL_NORMAL_8 = 12,
    //DVFS_LEVEL_NORMAL_9 = 13,
    DVFS_LEVEL_NORMAL_MAX = 13,
    DVFS_LEVEL_MIDDLE = 14,
    DVFS_LEVEL_PERFORMANCE = 15,
    DVFS_LEVEL_HIGH_PERFORMANCE = 16,
    DVFS_LEVEL_HIGH_PERFORMANCE_2 = 17,
    DVFS_LEVEL_HIGH_PERFORMANCE_MAX = 18,
}dvfs_level_e;

#define DVFS_EVENT_PRE_CHANGE		(1)
#define DVFS_EVENT_POST_CHANGE		(2)

enum dvfs_dev_clk_type {
    DVFS_DEV_CLK_ASRC,
    DVFS_DEV_CLK_SPDIF,
};

struct dvfs_level {
	u8_t level_id;
	u8_t enable_cnt;
	u16_t cpu_freq;
	u16_t dsp_freq;
	u16_t vdd_volt;
};

//#define DSP_DYNAMIC_FREQ_ADJ_ENABLE

struct dvfs_freqadj_config_params {
	uint32_t enable : 1;
	uint32_t enable_src : 1;
	uint32_t src_type : 3; //2-corepll, others-ignore
	uint32_t dsp_freq_high_reg;
	uint32_t dsp_vdd_high_reg;
	uint32_t dsp_freq_low_reg;
	uint32_t dsp_vdd_low_reg;
	uint32_t dsp_freq_sleep_reg;
	uint32_t dsp_vdd_sleep_reg; /* NOTE : required equal to dsp_vdd_low_reg */
};

struct dsp_freqadj_config_params {
	uint32_t enable : 1;
	int (*dsp_freqadj_up)(void);
	int (*dsp_freqadj_down)(void);
	int (*dsp_freqadj_sleep)(void);
};

typedef int (*dsp_freqadj_config_cbk)(struct dsp_freqadj_config_params *config_params);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
void dvfs_dump_tbl(void);
int dvfs_set_freq_table(struct dvfs_level *dvfs_level_tbl, int level_cnt);
int dvfs_get_current_level(void);
int dvfs_set_level(int level_id, const char *user_info);
int dvfs_unset_level(int level_id, const char *user_info);
int dvfs_level_to_freq(int level_id, uint16_t *cpu_freq, uint16_t *dsp_freq);
int dvfs_set_dsp_freqadj_config_cbk(dsp_freqadj_config_cbk cfg_cbk);
int dvfs_set_dsp_freqadj_enable(bool enable);
#else

#define dvfs_set_freq_table(dvfs_level_tbl, level_cnt)		({ 0; })
#define dvfs_get_current_level()				({ 0; })
#define dvfs_set_level(level_id, user_info)			({ 0; })
#define dvfs_unset_level(level_id, user_info)			({ 0; })
#define dvfs_level_to_freq()				    do {} while (0)
#define soc_freq_set_asrc_freq(asrc_freq)	({ 0; })
#define soc_dvfs_set_asrc_rate(clk_mhz) ({ 0; })
#define soc_freq_get_spdif_corepll_freq(sample_rate)	({ 0; })
#define soc_dvfs_set_spdif_rate()				    do {} while (0)
#define dvfs_set_dsp_freqadj_config_cbk(cbs)	({ 0; })
#define dvfs_set_dsp_freqadj_enable(cbs)	({ 0; })
#endif	/* CONFIG_DVFS_DYNAMIC_LEVEL */


#endif /* _ASMLANGUAGE */

#endif /* _DVFS_H_	*/
