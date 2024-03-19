/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral clock interface for Actions SoC
 */

#include <kernel.h>
#include <device.h>
#include <soc.h>
extern void cmu_dev_clk_enable(uint32_t id);
extern void cmu_dev_clk_disable(uint32_t id);

extern void soc_pmu_set_vd12_voltage(uint32_t volt_mv);
extern uint32_t soc_pmu_get_vd12_voltage(void);

static void acts_clock_peripheral_control(int clock_id, int enable)
{
	unsigned int key;

	if (clock_id > CLOCK_ID_MAX_ID)
		return;

	key = irq_lock();

	if (enable) {
		sys_set_bit(CMU_DEVCLKEN0, clock_id);
	} else {
		sys_clear_bit(CMU_DEVCLKEN0, clock_id);
	}
	irq_unlock(key);
}

void acts_clock_peripheral_enable(int clock_id)
{
	acts_clock_peripheral_control(clock_id, 1);
}

void acts_clock_peripheral_disable(int clock_id)
{
	acts_clock_peripheral_control(clock_id, 0);
}

static void _cfg_corepll_master(uint8_t master)
{
	if (sys_read32(CMU_DEVCLKEN0) & (1 << 16)) {
		return;
	}

	if(master >= 1) {
		return;
	}

	//master: 0-cpu, 1-dsp
	printk("switch corepll master: %d\n", master);
	if (master == 0) {
		while ((sys_read32(CMU_DSPCLK) & (1<<31)) != 0) {
			sys_write32((sys_read32(CMU_DSPCLK) & ~(1<<31)), CMU_DSPCLK);
		}
	} else {
		while ((sys_read32(CMU_DSPCLK) & (1<<31)) == 0) {
			sys_write32((sys_read32(CMU_DSPCLK) | (1<<31)), CMU_DSPCLK);
		}
	}
}


/* get current corepll freq, unit is Hz */
uint32_t clk_rate_get_corepll(void)
{
    _cfg_corepll_master(0);
	return MHZ(((sys_read32(COREPLL_CTL)&0x3F)*8));
}

/* set corepll by special rate */
void clk_rate_set_corepll(uint32_t rate)
{
	int multi = (rate+8-1)/8;

	if (multi < 5) {
		multi = 5;
	}
	
	if (multi > 31) {
		multi = 31;
	}

	_cfg_corepll_master(0);

	printk("set corepll: %dMHz\n", multi*8);
	sys_write32((sys_read32(COREPLL_CTL) & ~(0x3f)) | (1<<7) | multi, COREPLL_CTL);
	
	return;
}

void close_corepll(void)
{
	_cfg_corepll_master(0);

	printk("close pll!\n");
	sys_write32(sys_read32(COREPLL_CTL) & ~(1<<7), COREPLL_CTL);
	
	return;	
}

/* get freq by source and div */
static int _get_clk_from_source(uint32_t div, uint32_t source_hz)
{
	uint32_t freq;

	if (div == 14) {
		/* 1.5 divisor */
		freq = source_hz * 10 / 15;
	} else if (div == 15) {
		/* 2.5 divisor */
		freq = source_hz * 10 / 25;
	} else {
		freq = source_hz / (div + 1);
	}

	return freq;
}

#if 0
static uint8_t soc_get_cpu_clksrc(void)
{
	uint8_t cpuclk_src;
	
	cpuclk_src = sys_read32(CMU_SYSCLK) & 0x7;

	return cpuclk_src;
}

static uint8_t soc_get_dsp_clksrc(void)
{
	uint8_t dspclk_src;
	
	//_cfg_corepll_master(0);
	dspclk_src = sys_read32(CMU_DSPCLK) & 0x7;

	return dspclk_src;
}

static uint8_t soc_get_spi0_clksrc(void)
{
	uint8_t spi0clk_src;
	
	spi0clk_src = (sys_read32(CMU_SPI0CLK) & 0x300) >> 8;

	return spi0clk_src;
}
#endif

static uint8_t target_dsp_src;
static uint32_t target_dsp_freq_mhz;

/* get target dsp src */
uint32_t soc_freq_get_target_dsp_src(void)
{
	return target_dsp_src;
}

/* get current dsp freq */
uint32_t soc_freq_get_target_dsp_freq_mhz(void)
{
	return target_dsp_freq_mhz;
}

/* get current dsp freq */
uint32_t soc_freq_get_dsp_freq(void)
{
	//must switch to cpu first    
	uint32_t corepll_freq = clk_rate_get_corepll();
	uint8_t dspclk_div = (sys_read32(CMU_DSPCLK) & (0xF << 4)) >> 4;
	uint8_t dspclk_src = sys_read32(CMU_DSPCLK) & 0x7;
	uint32_t dsp_freq = 0;

	if (dspclk_src == 0) { /* RC4M */
		dsp_freq = _get_clk_from_source(dspclk_div, 4000000UL);
	} else if (dspclk_src == 1) { /* HOSC */
		dsp_freq = _get_clk_from_source(dspclk_div, 32000000UL);
	} else if (dspclk_src == 3) { /* RC64M */
		dsp_freq = _get_clk_from_source(dspclk_div, 64000000UL);
	} else if (dspclk_src == 4) { /* RC128M */
		dsp_freq = _get_clk_from_source(dspclk_div, 128000000UL);
	} else if (dspclk_src == 6) { /* RC32K */
		dsp_freq = _get_clk_from_source(dspclk_div, 32000UL);
	} else if (dspclk_src == 2) { /* COREPLL */
		dsp_freq = _get_clk_from_source(dspclk_div, corepll_freq);
	}

	return dsp_freq;
}

/* get current cpu freq */
uint32_t soc_freq_get_cpu_freq(void)
{
	uint32_t corepll_freq = clk_rate_get_corepll();
	uint8_t cpuclk_div = (sys_read32(CMU_SYSCLK) & (0xF << 4)) >> 4;
	uint8_t cpuclk_src = sys_read32(CMU_SYSCLK) & 0x7;
	uint32_t cpu_freq = 0;

	if (cpuclk_src == 0) { /* RC4M */
		cpu_freq = _get_clk_from_source(cpuclk_div, 4000000UL);
	} else if (cpuclk_src == 1) { /* HOSC */
		cpu_freq = _get_clk_from_source(cpuclk_div, 32000000UL);
	} else if (cpuclk_src == 3) { /* RC64M */
		cpu_freq = _get_clk_from_source(cpuclk_div, 64000000UL);
	} else if (cpuclk_src == 4) { /* RC128M */
		cpu_freq = _get_clk_from_source(cpuclk_div, 128000000UL);
	} else if (cpuclk_src == 6) { /* RC32K */
		cpu_freq = _get_clk_from_source(cpuclk_div, 32000UL);
	} else if (cpuclk_src == 2) { /* COREPLL */
		cpu_freq = _get_clk_from_source(cpuclk_div, corepll_freq);
	}

	return cpu_freq;
}

/* get current spi0 freq */
uint32_t soc_freq_get_spi0_freq(void)
{
	uint32_t corepll_freq = clk_rate_get_corepll();
	uint8_t spi0clk_div = (sys_read32(CMU_SPI0CLK) & 0xF);
	uint8_t spi0clk_src = (sys_read32(CMU_SPI0CLK) & (0x3 << 8))>>8;
	uint32_t spi0_freq = 0;

	if (spi0clk_src == 0) { /* HOSC */
		spi0_freq = _get_clk_from_source(spi0clk_div, 32000000UL);
	} else if (spi0clk_src == 2) { /* RC128M */
		spi0_freq = _get_clk_from_source(spi0clk_div, 128000000UL);
	} else if (spi0clk_src == 3) { /* RC64M */
		spi0_freq = _get_clk_from_source(spi0clk_div, 64000000UL);
	} else if (spi0clk_src == 1) { /* COREPLL */
		spi0_freq = _get_clk_from_source(spi0clk_div, corepll_freq);
	}

	return spi0_freq;
}


static uint32_t _get_clk_div_from_source(uint32_t cpu_mhz, uint32_t source_mhz, uint8_t *divisor_out)
{
	uint32_t divisor;
	uint32_t mhz_25 = (source_mhz * 10 / 25);
	uint32_t mhz_15 = (source_mhz * 10 / 15);
	uint32_t real_mhz;

	divisor = source_mhz / cpu_mhz;

	if (source_mhz % cpu_mhz) {
		divisor++;
	}
	
	if (divisor > 14) {
		divisor = 14;
	}
	
	real_mhz = source_mhz / divisor;

	if (mhz_15 <= cpu_mhz) {
		if ((cpu_mhz - real_mhz) > (cpu_mhz - mhz_15)) {
			divisor = 14+1;
			real_mhz = mhz_15;
		}
	} else if (mhz_25 <= cpu_mhz) {
		if ((cpu_mhz - real_mhz) > (cpu_mhz - mhz_25)) {
			divisor = 15+1;
			real_mhz = mhz_25;
		}
	}

	*divisor_out = divisor;
	return real_mhz;
}

static uint8_t _freq_get_src_div(uint32_t freq_mhz, uint8_t *divisor_out, uint32_t *freq_out)
{
	uint8_t COREPLL_MZH;
	uint8_t clk_source[] = {32, 64, 128, 0};
	uint8_t real_divisor = 0;
	uint8_t source_real = 0;
	int i;
	uint32_t real_freq = 0;

	COREPLL_MZH = (freq_mhz/8) * 8;
	clk_source[sizeof(clk_source)-1] = COREPLL_MZH;

	for (i=0; i<sizeof(clk_source); i++) {
		uint8_t divisor;
		uint32_t freq;
		freq = _get_clk_div_from_source(freq_mhz, clk_source[i], &divisor);

		if (freq > freq_mhz) {
			continue;
		}
		
		if ((real_freq == 0) || ((freq_mhz - freq) < (freq_mhz - real_freq))) {
			real_freq = freq;
			real_divisor = divisor;
			source_real = clk_source[i];
		}

		if (real_freq == freq_mhz)
			break;
	}

	if (divisor_out)
		*divisor_out = real_divisor;
	if (freq_out)
		*freq_out = real_freq;
	
	return source_real;
}


/* calibrate rc clk */
static void _rc_clk_calibrate(uint32_t reg, uint32_t val)
{
	uint32_t ori_val, tmp;

	printk("0x%x 0x%x\n",reg, sys_read32(reg));

	ori_val = sys_read32(reg);

    if (((ori_val&1) == 0) || ((ori_val&0xFFF000) != (val&0xFFF000))) {
        sys_write32(val, reg);
        /* wait calibration done */
        while(!(sys_read32(reg) & (1 << 24))) {
            ;
        }
		
		tmp = sys_read32(reg) >> 25;
		tmp -= 3;
		printk("0x%x 0x%x\n",reg, sys_read32(reg));

		sys_write32( (sys_read32(reg)&~(0xFF000FFE)) | ((tmp&0x7F)<<4), reg);

		printk("0x%x 0x%x\n",reg, sys_read32(reg));

    }
}

static inline void _enable_rc128m(void)
{
	_rc_clk_calibrate(RC128M_CTL, 0x20100d);
}

static inline void _enable_rc120m(void)
{
	_rc_clk_calibrate(RC128M_CTL, 0x1e100d);
}

static inline void _disable_rc128m(void)
{
	sys_write32(sys_read32(RC128M_CTL)&(~1), RC128M_CTL);
}

#ifndef COREPLL_FIX_FREQ	
static uint32_t _get_clksrc_idx(uint32_t src)
{
	uint8_t CLKSRC[5] = {4, 32, 0, 64, 128};
	uint32_t i;

	for (i=0; i<ARRAY_SIZE(CLKSRC); i++) {
		if (CLKSRC[i] == src)
			break;
	}

	return i;
}
#endif

/* adjust cpu clock */
void soc_freq_set_cpu_clk(uint32_t cpu_mhz)
{
	uint32_t val, flags;
	uint32_t cpu_real_mhz;
	uint8_t cpu_divisor;
	uint8_t clk_source;
	uint8_t CLKSRC[5] = {4, 32, 0, 64, 128};
	uint8_t old_clk_source;
	uint8_t clksrc_idx;

	clksrc_idx = sys_read32(CMU_SYSCLK)&0x7;

	if (clksrc_idx <=4) {
		old_clk_source = CLKSRC[clksrc_idx];
		if (!old_clk_source) {
			old_clk_source = clk_rate_get_corepll()/1000000;
		}
	} else {
		old_clk_source = 0;
	}

#ifndef COREPLL_FIX_FREQ

#ifdef CONFIG_SOC_CPU_USE_CK64M
	clk_source= 64;
#endif

#ifdef CONFIG_SOC_CPU_USE_CK128M
	if (cpu_mhz <= 32) {
		clk_source = 32;
	} else {
		clk_source = 128;
		_enable_rc128m();
	}
#endif

#else  // #ifdef COREPLL_FIX_FREQ

    //only use HOSC or corepll
	if(cpu_mhz <= 32) {
		clk_source = 32;  //HOSC
	} else {
		clk_source= COREPLL_FIX_FREQ; //corepll
	}
#endif

	cpu_real_mhz = _get_clk_div_from_source(cpu_mhz, clk_source, &cpu_divisor);
	printk("cpu clk, need: %dMHz, div: %d, real: %dMHz\n", cpu_mhz, cpu_divisor, cpu_real_mhz);

	flags = irq_lock();

	cpu_divisor--;

#ifndef COREPLL_FIX_FREQ	
	clksrc_idx = _get_clksrc_idx(clk_source);
#else
	if(clk_source == 32) {
		clksrc_idx = 1;  //HOSC
	} else {
		clksrc_idx = 2;//Corepll
	}
#endif

	if (old_clk_source < clk_source) {
		/* set cpu clock */
		val = sys_read32(CMU_SYSCLK);
		val &= (~(0xf0));
		val |= ((cpu_divisor& 0xf) << 4);
		sys_write32(val, CMU_SYSCLK); // set div
		val &= (~(0xf));
		val |= (clksrc_idx << 0);
		sys_write32(val, CMU_SYSCLK); // set clk src
	} else {
		/* set cpu clock */
		val = sys_read32(CMU_SYSCLK);
		val &= (~(0xf));
		val |= (clksrc_idx << 0);
		sys_write32(val, CMU_SYSCLK); // set clk src
		val &= (~(0xf0));
		val |= ((cpu_divisor& 0xf) << 4);
		sys_write32(val, CMU_SYSCLK); // set div
	}

	sys_read32(CMU_SYSCLK); // for delay

	irq_unlock(flags);	

#ifndef COREPLL_FIX_FREQ	

#ifdef CONFIG_SOC_CPU_USE_CK128M
	if (clk_source != 128) {
		_disable_rc128m();
	}
#endif

#endif

	printk("SYSCLK: 0x%x\n", sys_read32(CMU_SYSCLK));
}

#ifndef COREPLL_FIX_FREQ

#ifdef CONFIG_SOC_SPI0_USE_CK64M

static unsigned int calc_spi_clk_div(unsigned int max_freq, unsigned int spi_freq)
{
	unsigned int div;

	if (max_freq > spi_freq && max_freq <= (spi_freq * 3 / 2)) {
		/* /1.5 */
		div = 14;
	} else if ((max_freq > 2 * spi_freq) && max_freq <= (spi_freq * 5 / 2)) {
		/* /2.5 */
		div = 15;
	} else {
		/* /n */
		div = (max_freq + spi_freq - 1) / spi_freq - 1;
		if (div > 13) {
			div = 13;
		}
	}

	return div;
}

static void __acts_clk_set_rate_spi_ck64m(int clock_id, unsigned int rate_hz)
{
	unsigned int div, reg_val, real_rate;

	div = calc_spi_clk_div(MHZ(64), rate_hz) & 0xf;

	/* check CK64M has been enabled or not */
	if (!(sys_read32(CMU_S1CLKCTL) & (1 << 2))) {
		/* enable S1 CK64M */
		sys_write32(sys_read32(CMU_S1CLKCTL) | (1 << 2), CMU_S1CLKCTL);
		/* enable S1BT CK64M */
		sys_write32(sys_read32(CMU_S1BTCLKCTL) | (1 << 2), CMU_S1BTCLKCTL);
		k_busy_wait(10);
		/* calibrate CK64M clock */
		sys_write32(0x8020140C, CK64M_CTL);
		/* wait calibration done */
		while(!(sys_read32(CK64M_CTL) & (1 << 24))) {
			;
		}
	}

	/* set SPIx clock source and divison */
	reg_val = sys_read32(CMU_SPI0CLK + ((clock_id - CLOCK_ID_SPI0) * 4));
	reg_val &= ~0x30f;
	reg_val |= (0x3 << 8) | (div << 0);

	if (div == 14)
		real_rate = MHZ(64) * 2 / 3;
	else if (div == 15)
		real_rate = MHZ(64) * 2 / 5;
	else
		real_rate = MHZ(64) / (div + 1);

	sys_write32(reg_val, CMU_SPI0CLK + ((clock_id - CLOCK_ID_SPI0) * 4));

	printk("SPI%d: set rate %d Hz real rate %d Hz\n",
			clock_id - CLOCK_ID_SPI0, rate_hz, real_rate);
}

#endif

#endif

static void  acts_clk_set_rate_spi(int clock_id, unsigned int rate_hz)
{
	unsigned int core_pll, val;

#ifdef COREPLL_FIX_FREQ
	uint32_t spi_mhz, spi_real_mhz;
	uint8_t clk_divisor;
#else
	unsigned int real_rate, div;
#endif	

#ifndef COREPLL_FIX_FREQ

#ifdef CONFIG_SOC_SPI0_USE_CK64M
	if (CLOCK_ID_SPI0 == clock_id) {
		return __acts_clk_set_rate_spi_ck64m(CLOCK_ID_SPI0, rate_hz);
	}
#endif

	core_pll = clk_rate_get_corepll();
	div = (core_pll+rate_hz-1)/rate_hz;
	real_rate = core_pll/div;
	val = (div-1)|(1<<8);

	sys_write32(val, CMU_SPI0CLK + (clock_id - CLOCK_ID_SPI0)*4);

	printk("SPI%d: set rate %d Hz, real rate %d Hz\n",
		clock_id - CLOCK_ID_SPI0, rate_hz, real_rate);

#else      //use corepll

    core_pll = clk_rate_get_corepll()/(1000*1000);
    spi_mhz = rate_hz/(1000*1000);
	spi_real_mhz = _get_clk_div_from_source(spi_mhz, core_pll, &clk_divisor);

	val = (clk_divisor-1)|(1<<8);

	sys_write32(val, CMU_SPI0CLK + (clock_id - CLOCK_ID_SPI0)*4);

	printk("SPI%d: set rate %d Hz, real rate %d MHz!\n",
		clock_id - CLOCK_ID_SPI0, rate_hz, spi_real_mhz);	

	printk("CMU_SPICLK(%d): 0x%x\n", clock_id, sys_read32(CMU_SPI0CLK + (clock_id - CLOCK_ID_SPI0)*4));

#endif
}


/* set dsp max freq */
static void acts_clk_set_rate_dsp(unsigned int rate_mhz)
{
	uint8_t dsp_divisor, new_dsp_src_idx;
	uint32_t dsp_source;

#ifndef COREPLL_FIX_FREQ	
	uint32_t new_pll_freq;
#endif

	printk("set dsp max freq: %d MHz\n", rate_mhz);

#ifndef COREPLL_FIX_FREQ
	if(rate_mhz != 0) {
		dsp_source = _freq_get_src_div(rate_mhz, &dsp_divisor, NULL);
		if (dsp_source == 0){
			return;
		}

#ifdef CONFIG_SOC_DSP_FIX_USE_COREPLL
		new_dsp_src_idx = 2;
#else
		if((dsp_source != 32) && (dsp_source != 64) && (dsp_source != 128)) {
			//use corepll
			new_dsp_src_idx = 2;
		} else {
			new_dsp_src_idx = _get_clksrc_idx(dsp_source);
		}
#endif
	} else {
		//dsp is closed
		new_dsp_src_idx = 0;
	}
	
	target_dsp_src = new_dsp_src_idx;
	target_dsp_freq_mhz = rate_mhz;

	if (new_dsp_src_idx != 2) {
		//dsp no need corepll, close
		close_corepll();
	} else {
		//dsp use corepll
		new_pll_freq = (rate_mhz/8)*8;
		target_dsp_freq_mhz = new_pll_freq;
		clk_rate_set_corepll(new_pll_freq);
		//sys_write32(sys_read32(COREPLL_CTL) | (1<<7), COREPLL_CTL);
	}

#ifdef CONFIG_SOC_CPU_USE_CK64M	
	//check CK128M need close?
	if (new_dsp_src_idx != 4) {
		_disable_rc128m();
	} else	{
		//dsp use rc128m
		_enable_rc128m();
	}
#endif	

#else    // dsp fix use corepll

	if(rate_mhz != 0) {
		dsp_source = _freq_get_src_div(rate_mhz, &dsp_divisor, NULL);
		if (dsp_source == 0){
			return;
		}
	
		new_dsp_src_idx = 2;
	} else {
		//dsp is closed
		new_dsp_src_idx = 0;
	}
		
	target_dsp_src = new_dsp_src_idx;
	target_dsp_freq_mhz = (rate_mhz/8)*8;

#endif
}

static void _check_vd12_voltage(uint32_t rate_hz)
{
	uint32_t cur_vd12_mv, new_vd12_mv;
	
	cur_vd12_mv = soc_pmu_get_vd12_voltage();

	//dsp max freq bigger then 120M, vd12 set to 1.45v 
	if(rate_hz > 120 * 1000 * 1000) {
		new_vd12_mv = 1450;	
	} else {
		//dsp no need vd12 1.45V, check vddm
		if(soc_pmu_get_vdd_voltage() >= 1200) {
			new_vd12_mv = 1450;
		} else {
			new_vd12_mv = 1400;
		}
	}

	if(new_vd12_mv != cur_vd12_mv) {
		soc_pmu_set_vd12_voltage(new_vd12_mv);
	}		
}

int clk_set_rate(int clock_id,  uint32_t rate_hz)
{
	int ret = 0;
	switch(clock_id) {
	case CLOCK_ID_DMA:
		break;
	case CLOCK_ID_SPI0:
	case CLOCK_ID_SPI1:
		acts_clk_set_rate_spi(clock_id, rate_hz);
		break;
	case CLOCK_ID_DSP:
		_check_vd12_voltage(rate_hz);
		acts_clk_set_rate_dsp(rate_hz/(1000*1000));
		break;
		
	case CLOCK_ID_SPI0CACHE:
	case CLOCK_ID_PWM:
	case CLOCK_ID_TIMER:
	case CLOCK_ID_LRADC:
	case CLOCK_ID_UART0:
	case CLOCK_ID_UART1:
	case CLOCK_ID_I2C0:
	case CLOCK_ID_DAC:
	case CLOCK_ID_ADC:
	case CLOCK_ID_I2STX:
	case CLOCK_ID_I2SRX:
		printk("clkid=%d not support clk set\n",clock_id);
		ret = -1;
		break;

	}

	return 0;
}

uint32_t clk_get_rate(int clock_id)
{
	uint32_t rate = 0;
	switch(clock_id) {
	case CLOCK_ID_DMA:
	case CLOCK_ID_SPI0:
	case CLOCK_ID_SPI1:
	case CLOCK_ID_SPI0CACHE:
	case CLOCK_ID_PWM:
	case CLOCK_ID_TIMER:
	case CLOCK_ID_LRADC:
	case CLOCK_ID_UART0:
	case CLOCK_ID_UART1:
	case CLOCK_ID_I2C0:
	case CLOCK_ID_DSP:
	case CLOCK_ID_DAC:
	case CLOCK_ID_ADC:
	case CLOCK_ID_I2STX:
	case CLOCK_ID_I2SRX:
		printk("clkid=%d not support clk get\n",clock_id);
		break;

	}

	return rate;
}

