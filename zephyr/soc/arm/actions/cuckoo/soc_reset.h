/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral reset configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_RESET_H_
#define	_ACTIONS_SOC_RESET_H_

#define	RESET_ID_DMA			0
#define	RESET_ID_SPI0			2
#define	RESET_ID_SPI1			3
#define	RESET_ID_SPI0CACHE		4
#define	RESET_ID_TIMER			6
#define	RESET_ID_PWM			7
#define	RESET_ID_UART0			9
#define	RESET_ID_UART1			10
#define	RESET_ID_I2C0			11
#define	RESET_ID_BT				14
#define	RESET_ID_DSP			15
#define	RESET_ID_DSP_PART       16
#define	RESET_ID_ADC			17
#define	RESET_ID_DAC			19
#define	RESET_ID_I2STX			20
#define	RESET_ID_I2SRX			21
#define	RESET_ID_USB			27
#define	RESET_ID_USB2			28
#define	RESET_ID_MAX_ID			28

#ifndef _ASMLANGUAGE

void acts_reset_peripheral_assert(int reset_id);
void acts_reset_peripheral_deassert(int reset_id);
void acts_reset_peripheral(int reset_id);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_RESET_H_	*/
