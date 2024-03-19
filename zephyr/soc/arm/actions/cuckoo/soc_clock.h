/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral clock configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_CLOCK_H_
#define	_ACTIONS_SOC_CLOCK_H_



#define	CLOCK_ID_DMA			0
#define CLOCK_ID_CPUTIMER       1
#define	CLOCK_ID_SPI0			2
#define	CLOCK_ID_SPI1			3
#define	CLOCK_ID_SPI0CACHE		4
#define	CLOCK_ID_TIMER			6
#define	CLOCK_ID_PWM			7
#define	CLOCK_ID_UART0			9
#define	CLOCK_ID_UART1			10
#define	CLOCK_ID_I2C0			11
#define CLOCK_ID_32KOUT         12
#define CLOCK_ID_32MOUT         13
#define	CLOCK_ID_AUDDSPTIMER	15
#define	CLOCK_ID_DSP			16
#define	CLOCK_ID_ADC			17
#define	CLOCK_ID_DAC			18
#define CLOCK_ID_DACANACLK      19
#define	CLOCK_ID_I2STX			20
#define	CLOCK_ID_I2SRX			21
#define CLOCK_ID_I2SSRDCLK      22
#define CLOCK_ID_I2SHCLKEN      23
#define CLOCK_ID_ADCSH          24
#define	CLOCK_ID_USB2			27
#define	CLOCK_ID_USB			28
#define CLOCK_ID_LRADC          29
#define CLOCK_ID_EXINT          30
#define CLOCK_ID_EFUSE          31

#define	CLOCK_ID_MAX_ID			31

#ifndef _ASMLANGUAGE

void acts_clock_peripheral_enable(int clock_id);
void acts_clock_peripheral_disable(int clock_id);
uint32_t clk_rate_get_corepll(void);
void clk_rate_set_corepll(uint32_t rate);
void close_corepll(void);
int clk_set_rate(int clock_id,  uint32_t rate_hz);
uint32_t clk_get_rate(int clock_id);
uint32_t clk_ahb_set(uint32_t div);
uint32_t soc_freq_get_dsp_freq(void);
uint32_t soc_freq_get_cpu_freq(void);
void soc_freq_set_cpu_clk(uint32_t cpu_mhz);

uint32_t soc_freq_get_target_dsp_src(void);
uint32_t soc_freq_get_target_dsp_freq_mhz(void);


#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_CLOCK_H_	*/
