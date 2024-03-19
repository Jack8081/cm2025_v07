/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dynamic voltage and frequency scaling interface
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <soc.h>
#include <dvfs.h>
#include <soc_atp.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(dvfs0, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL

struct dvfs_manager {
	struct k_sem lock;
	uint8_t cur_dvfs_idx;
	uint8_t dvfs_level_cnt;
	uint8_t asrc_limit_clk_mhz;
	uint8_t spdif_limit_clk_mhz;
	struct dvfs_level *dvfs_level_tbl;
};

struct dvfs_level_max {
	u16_t cpu_freq;
	u16_t vdd_volt;
};

#if 1
/* NOTE: DON'T modify max_soc_dvfs_table except actions ic designers */
#define CPU_FREQ_MAX  144
#define VDD_VOLT_MAX  1250

static const struct dvfs_level_max max_soc_dvfs_table_TT[] = {
	/* cpu_freq, vodd_volt  */
	{32,          1000},
	{48,          1000},
	{64,          1000},
	{72,          1050},
	{96,          1100},
	{112,         1150},
	{128,         1200},
	{CPU_FREQ_MAX, VDD_VOLT_MAX},
};

static const struct dvfs_level_max max_soc_dvfs_table_FF[] = {
	/* cpu_freq, vodd_volt  */
	{32,          1000},
	{48,          1000},
	{64,          1000},
	{72,          1050},
	{96,          1100},
	{112,         1150},
	{128,         1200},
	{CPU_FREQ_MAX, VDD_VOLT_MAX},
};
#else
/* NOTE: DON'T modify max_soc_dvfs_table except actions ic designers */
#define CPU_FREQ_MAX  136
#define VDD_VOLT_MAX  1250

#define CPU_FREQ_MAX_FF2 112
#define VDD_VOLT_MAX_FF2 1200

static const struct dvfs_level_max max_soc_dvfs_table_TT[] = {
	/* cpu_freq, vodd_volt  */
	{32,          1000},
	{56,          1000},
	{80,          1100},
	{112,         1200},
	{CPU_FREQ_MAX, VDD_VOLT_MAX},
};

static const struct dvfs_level_max max_soc_dvfs_table_FF[] = {
	/* cpu_freq, vodd_volt  */
	{32,          950},
	{64,          1000},
	{88,          1100},
	{CPU_FREQ_MAX_FF2, VDD_VOLT_MAX_FF2},
};
#endif

/* config vdd setting only by cpu freq */
static u16_t dvfs_get_optimal_volt(u16_t cpu_freq)
{
	u16_t volt;
	int i;
	unsigned int wafer_inf;

	i = soc_atp_get_wafer_info(&wafer_inf);
	if((i==0) && (wafer_inf == 1)) {
		volt = max_soc_dvfs_table_FF[ARRAY_SIZE(max_soc_dvfs_table_FF) - 1].vdd_volt;
		for (i = 0; i < ARRAY_SIZE(max_soc_dvfs_table_FF); i++) {
			if (cpu_freq <= max_soc_dvfs_table_FF[i].cpu_freq) {
				volt = max_soc_dvfs_table_FF[i].vdd_volt;
				break;
			}
		}
	} else {
		volt = max_soc_dvfs_table_TT[ARRAY_SIZE(max_soc_dvfs_table_TT) - 1].vdd_volt;
		for (i = 0; i < ARRAY_SIZE(max_soc_dvfs_table_TT); i++) {
			if (cpu_freq <= max_soc_dvfs_table_TT[i].cpu_freq) {
				volt = max_soc_dvfs_table_TT[i].vdd_volt;
				break;
			}
		}		
	}

	return volt;
}

/* only for cpu */
static struct dvfs_level default_soc_dvfs_table[] = {
    /* level                       enable_cnt cpu_freq,  dsp_freq, vodd_volt  */
    {DVFS_LEVEL_LOW,               0,        16,        32,      0},
    //{DVFS_LEVEL_S2,                0,        21,        32,      0},
    {DVFS_LEVEL_NORMAL,            0,        32,        32,      0},
    {DVFS_LEVEL_PERFORMANCE,       0,        68,        32,      0},
    {DVFS_LEVEL_HIGH_PERFORMANCE,  0,        84,        32,      0},
};

static struct dvfs_manager g_dvfs;

static int level_id_to_tbl_idx(int level_id)
{
	int i;

	for (i = 0; i < g_dvfs.dvfs_level_cnt; i++) {
		if (g_dvfs.dvfs_level_tbl[i].level_id == level_id) {
			return i;
		}
	}

	return -1;
}

static int dvfs_get_max_idx(void)
{
	int i;

	for (i = (g_dvfs.dvfs_level_cnt - 1); i >= 0; i--) {
		if (g_dvfs.dvfs_level_tbl[i].enable_cnt > 0) {
			return i;
		}
	}

	return 0;
}

void dvfs_dump_tbl(void)
{
	const struct dvfs_level *dvfs_level = &g_dvfs.dvfs_level_tbl[0];
	int i;

	LOG_INF("idx   level_id   dsp   cpu  vdd  enable_cnt");
	for (i = 0; i < g_dvfs.dvfs_level_cnt; i++, dvfs_level++) {
		LOG_INF("%d: %d   %d   %d   %d   %d", i,
			dvfs_level->level_id,
			dvfs_level->dsp_freq,
			dvfs_level->cpu_freq,
			dvfs_level->vdd_volt,
			dvfs_level->enable_cnt);
	}
}


/* dvfs sync adjust */
static void dvfs_sync(void)
{
	struct dvfs_level *dvfs_level, *old_dvfs_level;
	unsigned int old_idx, new_idx, old_volt;
	//unsigned old_dsp_freq;

	old_idx = g_dvfs.cur_dvfs_idx;

	/* get current max dvfs level */
	new_idx = dvfs_get_max_idx();
	if (new_idx == old_idx) {
		/* same level, no need sync */
		LOG_INF("max idx %d\n", new_idx);
		return;
	}

	dvfs_level = &g_dvfs.dvfs_level_tbl[new_idx];

	old_dvfs_level = &g_dvfs.dvfs_level_tbl[old_idx];
	old_volt = soc_pmu_get_vdd_voltage();
	//old_dsp_freq = soc_freq_get_dsp_freq();

	LOG_INF("level_id [%d] -> [%d]", old_dvfs_level->level_id,
		dvfs_level->level_id);

	/* set vdd voltage before clock setting if new vdd is up */
	if (dvfs_level->vdd_volt > old_volt) {
		soc_pmu_set_vdd_voltage(dvfs_level->vdd_volt);
	}

	LOG_INF("new cpu freq %d, vdd volt %d", dvfs_level->cpu_freq, dvfs_level->vdd_volt);

	/* adjust core/dsp/cpu clock */
	soc_freq_set_cpu_clk(dvfs_level->cpu_freq);

	/* set vdd voltage after clock setting if new vdd is down */
	if (dvfs_level->vdd_volt < old_volt) {
		soc_pmu_set_vdd_voltage(dvfs_level->vdd_volt);
	}

	g_dvfs.cur_dvfs_idx = new_idx;
}

/* dvfs adjust to level */
static int dvfs_update_freq(int level_id, bool is_set, const char *user_info)
{
	struct dvfs_level *dvfs_level;
	int tbl_idx;

	printk("level %d, is_set %d %s\n", level_id, is_set, user_info);

	tbl_idx = level_id_to_tbl_idx(level_id);
	if (tbl_idx < 0) {
		LOG_ERR("%s: invalid level id %d\n", __func__, level_id);
		return -EINVAL;
	}

	dvfs_level = &g_dvfs.dvfs_level_tbl[tbl_idx];

	k_sem_take(&g_dvfs.lock, K_FOREVER);

	if (is_set) {
	    if(dvfs_level->enable_cnt < 255){
            dvfs_level->enable_cnt++;
	    }else{
            LOG_WRN("max dvfs level count");
	    }
	} else {
		if (dvfs_level->enable_cnt > 0) {
			dvfs_level->enable_cnt--;
		}else{
            LOG_WRN("min dvfs level count");
		}
	}

	dvfs_sync();

	k_sem_give(&g_dvfs.lock);

	return 0;
}

int dvfs_set_level(int level_id, const char *user_info)
{
	return dvfs_update_freq(level_id, 1, user_info);
}

int dvfs_unset_level(int level_id, const char *user_info)
{
	return dvfs_update_freq(level_id, 0, user_info);
}

int dvfs_get_current_level(void)
{
	int idx;

	if (!g_dvfs.dvfs_level_tbl)
		return -1;

	idx = g_dvfs.cur_dvfs_idx;
	if (idx < 0) {
		idx = 0;
	}

	return g_dvfs.dvfs_level_tbl[idx].level_id;
}

/* set dvfs freq table */
int dvfs_set_freq_table(struct dvfs_level *dvfs_level_tbl, int level_cnt)
{
	int i;

	if ((!dvfs_level_tbl) || (level_cnt <= 0)) {
		return -EINVAL;
	}
	
	for (i = 0; i < level_cnt; i++) {
		dvfs_level_tbl[i].vdd_volt = dvfs_get_optimal_volt(dvfs_level_tbl[i].cpu_freq);
	}

	g_dvfs.dvfs_level_cnt = level_cnt;
	g_dvfs.dvfs_level_tbl = dvfs_level_tbl;

	g_dvfs.cur_dvfs_idx = 0;

	dvfs_dump_tbl();

	return 0;
}

int dvfs_level_to_freq(int level_id, uint16_t *cpu_freq, uint16_t *dsp_freq)
{
	int i;

	for (i = 0; i < g_dvfs.dvfs_level_cnt; i++) {
		if (g_dvfs.dvfs_level_tbl[i].level_id == level_id) {
            if(cpu_freq) {
                *cpu_freq = g_dvfs.dvfs_level_tbl[i].cpu_freq;
            }
            if(dsp_freq) {
                *dsp_freq = g_dvfs.dvfs_level_tbl[i].dsp_freq;
            }
			return 0;
		}
	}

	return -1;
}

#endif /* CONFIG_DVFS_DYNAMIC_LEVEL */

static int dvfs_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	printk("default dsp freq %dHz, cpu freq %dHz, vdd %d mV\n\n",
		soc_freq_get_dsp_freq(), soc_freq_get_cpu_freq(),
		soc_pmu_get_vdd_voltage());

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_freq_table(default_soc_dvfs_table,
		ARRAY_SIZE(default_soc_dvfs_table));

	k_sem_init(&g_dvfs.lock, 1, 1);

#if 0
	dvfs_set_level(DVFS_LEVEL_LOW, "init");
#else
    dvfs_set_level(DVFS_LEVEL_NORMAL, "init");
#endif

#endif

	return 0;
}

SYS_INIT(dvfs_init, PRE_KERNEL_2, 2);
