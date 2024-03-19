/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file register address define for Actions SoC
 */

#ifndef	_ACTIONS_SOC_REGS_H_
#define	_ACTIONS_SOC_REGS_H_
#include "brom_interface.h"

#define RMU_REG_BASE				0x40000000
#define CMUD_REG_BASE				0x40001000
#define CMUA_REG_BASE				0x40000100
#define PMU_REG_BASE				0x40004000
#define GPIO_REG_BASE				0x40068000
#define DMA_REG_BASE				0x4001c000
#define DMA_LINE0_REG_BASE			0x4001cc00
#define DMA_LINE1_REG_BASE			0x4001cc20
#define UART0_REG_BASE				0x40038000
#define UART1_REG_BASE				0x4003C000
#define MEM_REG_BASE                0x40010000
#define I2C0_REG_BASE				0x40048000
#define SPI0_REG_BASE				0x40028000
#define SPI1_REG_BASE				0x4002C000
#define WIC_BASE                    0x40090000
#define BTC_REG_BASE                0x01100000
#define PWM_REG_BASE                0x40060000
#define PWM_CLK0_BASE               0x40001028





#define HOSC_CTL					(CMUA_REG_BASE + 0x00)
#define HOSCLDO_CTL					(CMUA_REG_BASE + 0x04)
#define CK64M_CTL					(CMUA_REG_BASE + 0x08)
#define RC64M_CTL					(CMUA_REG_BASE + 0x08)
#define CK128M_CTL					(CMUA_REG_BASE + 0x0c)
#define RC128M_CTL					(CMUA_REG_BASE + 0x0c)

#define RC4M_CTL					(CMUA_REG_BASE + 0x14)
#define AVDDLDO_CTL					(CMUA_REG_BASE + 0x1c)
#define COREPLL_CTL					(CMUA_REG_BASE + 0x20)
#define SPLL_CTL					(CMUA_REG_BASE + 0x24)



/*fix build err, dsp/display pll not exist*/
#define DSPPLL_CTL					(CMUA_REG_BASE + 0x24)
#define DISPLAYPLL_CTL				(CMUA_REG_BASE + 0x30)
#define AUDIOLDO_CTL				(CMUA_REG_BASE + 0x1C)

#define AUDIO_PLL0_CTL				(CMUA_REG_BASE + 0x28)
#define AUDIOPLL_DEBUG				(CMUA_REG_BASE + 0x48)


#define AUDIOPLL_DEBUG              (CMUA_REG_BASE + 0x48)

#define RC32K_CTL					(CMUA_REG_BASE + 0x64)
#define RC32K_CAL					(CMUA_REG_BASE + 0x68)
#define RC32K_COUNT					(CMUA_REG_BASE + 0x6C)

#define HOSCOK_CTL					(CMUA_REG_BASE + 0x70)

#define CMU_SYSCLK					(CMUD_REG_BASE + 0x00)
#define CMU_DEVCLKEN0				(CMUD_REG_BASE + 0x04)
#define CMU_LRADCCLK				(CMUD_REG_BASE + 0x64)
#define CMU_DSPCLK                  (CMUD_REG_BASE + 0x40)
#define CMU_ADCCLK                  (CMUD_REG_BASE + 0x44)
#define CMU_DACCLK                  (CMUD_REG_BASE + 0x48)
#define CMU_I2STXCLK                (CMUD_REG_BASE + 0x4C)
#define CMU_I2SRXCLK                (CMUD_REG_BASE + 0x50)

#define CMU_SPI0CLK                 (CMUD_REG_BASE + 0x10)
#define CMU_SPI1CLK                 (CMUD_REG_BASE + 0x14)

#define CMU_MEMCLKEN0				(CMUD_REG_BASE+0x0080)
#define CMU_MEMCLKEN1				(CMUD_REG_BASE+0x0084)

#define CMU_MEMCLKSRC0				(CMUD_REG_BASE+0x0088)
#define CMU_MEMCLKSRC1				(CMUD_REG_BASE+0x008C)

#define CMU_S1CLKCTL                (CMUD_REG_BASE+0x0070)
#define CMU_S1BTCLKCTL              (CMUD_REG_BASE+0x0074)
#define CMU_S3CLKCTL                (CMUD_REG_BASE+0x0078)
#define CMU_GPIOCLKCTL              (CMUD_REG_BASE+0x0060)
#define CMU_PMUWKUPCLKCTL			(CMUD_REG_BASE+0x0068)

#define CMU_TIMER0CLK               (CMUD_REG_BASE+0x001C)
#define CMU_TIMER1CLK               (CMUD_REG_BASE+0x0020)
#define CMU_TIMER2CLK               (CMUD_REG_BASE+0x0024)

#define RMU_MRCR0					(RMU_REG_BASE + 0x00)
#define DSP_VCT_ADDR                (RMU_REG_BASE+0x00000080)
#define DSP_STATUS_EXT_CTL          (RMU_REG_BASE+0x00000084)
#define	CHIPVERSION                 (RMU_REG_BASE+0x000000A0)

#define INTC_BASE					0x40003000
#define INT_TO_DSP					(INTC_BASE+0x00000000)
#define INFO_TO_DSP					(INTC_BASE+0x00000004)
#define INT_TO_BT_CPU				(INTC_BASE+0x00000010)
#define PENDING_FROM_DSP			(INTC_BASE+0x00000014)
#define PENDING_FROM_BT_CPU         (INTC_BASE+0x0000001C)

#define PENDING_FROM_SENSOR_CPU     INTC_BASE
#define INT_TO_SENSOR_CPU			INTC_BASE
#define INT_TO_SENSOR_CPU			INTC_BASE

#define MEMORY_CTL					(MEM_REG_BASE)
#define MEMORY_CTL2					(MEM_REG_BASE+0x00000020)
#define DSP_PAGE_ADDR0				(MEM_REG_BASE+0x00000080)
#define DSP_PAGE_ADDR1				(MEM_REG_BASE+0x00000084)
#define DSP_PAGE_ADDR2              (MEM_REG_BASE+0x00000088)
#define DSP_PAGE_ADDR3              (MEM_REG_BASE+0x0000008c)

#define WIO0_CTL					(GPIO_REG_BASE + 0x300)
#define WIO1_CTL					(GPIO_REG_BASE + 0x304)
#define WIO2_CTL					(GPIO_REG_BASE + 0x308)

// brom api address
//fpga
//#define SPINOR_API_ADDR				(0x00005860) //(0x00005814)


#define SPINOR_API_ADDR				(p_brom_api->p_spinor_api)


// Interaction RAM
#define INTER_RAM_ADDR				(0x01080000)
#define INTER_RAM_SIZE				(0x00008000)

//--------------SPICACHE_Control_Register-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     SPICACHE_Control_Register_BASE                                    0x40014000
#define     SPICACHE_CTL                                                      (SPICACHE_Control_Register_BASE+0x0000)
#define     SPICACHE_INVALIDATE                                               (SPICACHE_Control_Register_BASE+0x0004)
#define     SPICACHE_TOTAL_MISS_COUNT                                         (SPICACHE_Control_Register_BASE+0x0010)
#define     SPICACHE_TOTAL_HIT_COUNT                                          (SPICACHE_Control_Register_BASE+0x0014)
#define     SPICACHE_PROFILE_INDEX_START                                      (SPICACHE_Control_Register_BASE+0x0018)
#define     SPICACHE_PROFILE_INDEX_END                                        (SPICACHE_Control_Register_BASE+0x001C)
#define     SPICACHE_RANGE_INDEX_MISS_COUNT                                   (SPICACHE_Control_Register_BASE+0x0020)
#define     SPICACHE_RANGE_INDEX_HIT_COUNT                                    (SPICACHE_Control_Register_BASE+0x0024)
#define     SPICACHE_PROFILE_ADDR_START                                       (SPICACHE_Control_Register_BASE+0x0028)
#define     SPICACHE_PROFILE_ADDR_END                                         (SPICACHE_Control_Register_BASE+0x002C)
#define     SPICACHE_RANGE_ADDR_MISS_COUNT                                    (SPICACHE_Control_Register_BASE+0x0030)
#define     SPICACHE_RANGE_ADDR_HIT_COUNT                                     (SPICACHE_Control_Register_BASE+0x0034)
#define     SPICACHE_MAPPING_MISS_ADDR                                        (SPICACHE_Control_Register_BASE+0x0060)

//--------------PMUVDD-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     PMUVDD_BASE                                                       0x40004000
#define     VOUT_CTL0                                                         (PMUVDD_BASE+0x00)
#define     VOUT_CTL1_S1                                                      (PMUVDD_BASE+0x04)
#define     VOUT_CTL1_S3                                                      (PMUVDD_BASE+0X08)
#define     PMU_DET                                                           (PMUVDD_BASE+0x0C)
#define     DCDC_VC18_CTL                                                     (PMUVDD_BASE+0X10)
#define     DCDC_VD12_CTL                                                     (PMUVDD_BASE+0X14)
#define     PWRGATE_DIG                                                       (PMUVDD_BASE+0X18)
#define     PWRGATE_DIG_ACK                                                   (PMUVDD_BASE+0X1C)
#define     PWRGATE_RAM                                                       (PMUVDD_BASE+0X20)
#define     PWRGATE_RAM_ACK                                                   (PMUVDD_BASE+0X24)
#define     PMU_INTMASK                                                       (PMUVDD_BASE+0X28)


//--------------PMUSVCC-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     PMUSVCC_BASE                                                      0x40004000
#define     CHG_CTL_SVCC                                                      (PMUSVCC_BASE+0X100)
#define     BDG_CTL_SVCC                                                      (PMUSVCC_BASE+0x104)
#define     SYSTEM_SET_SVCC                                                   (PMUSVCC_BASE+0x108)
#define     POWER_CTL_SVCC                                                    (PMUSVCC_BASE+0x10C)
#define     WKEN_CTL_SVCC                                                     (PMUSVCC_BASE+0x110)
#define     WAKE_PD_SVCC                                                      (PMUSVCC_BASE+0x114)


#define     PMUADC_BASE                                                       0x40004000
#define 	PMUADC_CTL 	  												 	  (PMUADC_BASE+0x2C)
#define 	BATADC_DATA 	  												  (PMUADC_BASE+0x34)
#define 	CHGIADC_DATA 	  												  (PMUADC_BASE+0x40)
#define 	LRADC1_DATA 	  												  (PMUADC_BASE+0x44)



//--------------EFUSE-------------------------------------------//
#define     EFUSE_BASE                                                        0x40008000
#define     EFUSE_CTL0                                                        (EFUSE_BASE+0x00)
#define     EFUSE_CTL1                                                        (EFUSE_BASE+0x04)
#define     EFUSE_CTL2                                                        (EFUSE_BASE+0x08)
#define     EFUSE_DATA0                                                       (EFUSE_BASE+0x0c)
#define     EFUSE_DATA1                                                       (EFUSE_BASE+0x10)
#define     EFUSE_DATA2                                                       (EFUSE_BASE+0x14)
#define     EFUSE_DATA3                                                       (EFUSE_BASE+0x18)
#define     EFUSE_DATA4                                                       (EFUSE_BASE+0x1c)
#define     EFUSE_DATA5                                                       (EFUSE_BASE+0x20)
#define     EFUSE_DATA6                                                       (EFUSE_BASE+0x24)
#define     EFUSE_DATA7                                                       (EFUSE_BASE+0x28)
#define     EFUSE_PGDATA0                                                     (EFUSE_BASE+0x3c)
#define     EFUSE_PGDATA1                                                     (EFUSE_BASE+0x40)
#define     EFUSE_PGDATA2                                                     (EFUSE_BASE+0x44)
#define     EFUSE_PGDATA3                                                     (EFUSE_BASE+0x48)


#define     InterruptController_BASE                                          0xe000e000
#define     NVIC_ISER0                                                        (InterruptController_BASE+0x00000100)
#define     NVIC_ISER1                                                        (InterruptController_BASE+0x00000104)
#define     NVIC_ICER0                                                        (InterruptController_BASE+0x00000180)
#define     NVIC_ICER1                                                        (InterruptController_BASE+0x00000184)
#define     NVIC_ISPR0                                                        (InterruptController_BASE+0x00000200)
#define     NVIC_ISPR1                                                        (InterruptController_BASE+0x00000204)
#define     NVIC_ICPR0                                                        (InterruptController_BASE+0x00000280)
#define     NVIC_ICPR1                                                        (InterruptController_BASE+0x00000284)
#define     NVIC_IABR0                                                        (InterruptController_BASE+0x00000300)
#define     NVIC_IABR1                                                        (InterruptController_BASE+0x00000304)
#define     NVIC_IPR0                                                         (InterruptController_BASE+0x00000400)
#define     NVIC_IPR1                                                         (InterruptController_BASE+0x00000404)
#define     NVIC_IPR2                                                         (InterruptController_BASE+0x00000408)
#define     NVIC_IPR3                                                         (InterruptController_BASE+0x0000040c)
#define     NVIC_IPR4                                                         (InterruptController_BASE+0x00000410)
#define     NVIC_IPR5                                                         (InterruptController_BASE+0x00000414)
#define     NVIC_IPR6                                                         (InterruptController_BASE+0x00000418)
#define     NVIC_IPR7                                                         (InterruptController_BASE+0x0000041c)
#define     NVIC_IPR8                                                         (InterruptController_BASE+0x00000420)
#define     NVIC_IPR9                                                         (InterruptController_BASE+0x00000424)
#define     NVIC_STIR                                                         (InterruptController_BASE+0x00000F00)


#define     TIMER_REGISTER_BASE                                               0x4000C100

/* For sys tick used */
#define     T0_CTL                                                            (TIMER_REGISTER_BASE+0x00)
#define     T0_VAL                                                            (TIMER_REGISTER_BASE+0x04)
#define     T0_CNT                                                            (TIMER_REGISTER_BASE+0x08)
#define     T0_CNT_TMP                                                        (TIMER_REGISTER_BASE+0x0c)

/* For hrtimer used */
#define     T1_CTL                                                            (TIMER_REGISTER_BASE+0x20)
#define     T1_VAL                                                            (TIMER_REGISTER_BASE+0x24)
#define     T1_CNT                                                            (TIMER_REGISTER_BASE+0x28)
#define     T1_CNT_TMP                                                        (TIMER_REGISTER_BASE+0x2c)

/* For system cycle/tws used, system not use irq, tws use timer irq.
 * must set continue mode, must define USE_T2_FOR_CYCLE in acts_timer.c,
 * must select hosc for CMU_TIMER2CLK when not in sleep.
 */
#define     T2_CTL                                                            (TIMER_REGISTER_BASE+0x40)
#define     T2_VAL                                                            (TIMER_REGISTER_BASE+0x44)
#define     T2_CNT                                                            (TIMER_REGISTER_BASE+0x48)
#define     T2_CNT_TMP                                                        (TIMER_REGISTER_BASE+0x4c)

#define     RTC_REG_BASE      												  0x4000C000
#define     WD_CTL        													  (RTC_REG_BASE+0x20)
#define     RTC_REMAIN0                                                       (RTC_REG_BASE+0x30)
#define     RTC_REMAIN1                                                       (RTC_REG_BASE+0x34)
#define     RTC_REMAIN2                                                       (RTC_REG_BASE+0x38)
#define     RTC_REMAIN3                                                       (RTC_REG_BASE+0x3C)
#define     RTC_REMAIN4                                                       (RTC_REG_BASE+0x40)
#define     RTC_REMAIN5                                                       (RTC_REG_BASE+0x44)

/* DAC control register */
#define     AUDIO_DAC_REG_BASE                                                0x4005C000

/* ADC control register  */
#define     AUDIO_ADC_REG_BASE                                                0x4005C100
#define     ADC_REF_LDO_CTL                                                   (AUDIO_ADC_REG_BASE + 0x30)

/* I2STX control register */
#define     AUDIO_I2STX0_REG_BASE                                             0x4005C400

/* I2SRX control register */
#define     AUDIO_I2SRX0_REG_BASE                                             0x4005C500

/*spi1*/
#define     SPI1_REGISTER_BASE                                                0x401B0000
#define     SPI1_CTL                                                          (SPI1_REGISTER_BASE+0x0000)

#define     CTK0_BASE                                                         0x4007C000
#define     CTK_CTL                                                           (CTK0_BASE+0x00)
#define     CTK_VSET                                                           (CTK0_BASE+0x00)


/* uart */
#define UART0_CTL			(UART0_REG_BASE+0x00)
#define UART0_RXDAT			(UART0_REG_BASE+0x04)
#define UART0_TXDAT			(UART0_REG_BASE+0x08)
#define UART0_STA			(UART0_REG_BASE+0x0c)
#define UART0_BR			(UART0_REG_BASE+0x10)

#define UART1_CTL			(UART1_REG_BASE+0x00)
#define UART1_RXDAT			(UART1_REG_BASE+0x04)
#define UART1_TXDAT			(UART1_REG_BASE+0x08)
#define UART1_STA			(UART1_REG_BASE+0x0c)
#define UART1_BR			(UART1_REG_BASE+0x10)

#endif /* _ACTIONS_SOC_REGS_H_	*/
