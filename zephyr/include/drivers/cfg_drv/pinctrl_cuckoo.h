/*
 * Copyright (c) 2020 Linaro Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PINCTRL_CUCKOO_H_
#define PINCTRL_CUCKOO_H_

#define PIN_MFP_SET(pin, val)               \
    {.pin_num = pin,        \
     .mode = val}


#define GPIO_CTL_MFP_SHIFT                  (0)
#define GPIO_CTL_MFP_MASK                   (0x0f << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_MFP_GPIO                   (0x0 << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_MFP(x)                     (x << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_SMIT                       (0x1 << 5)
#define GPIO_CTL_GPIO_OUTEN                 (0x1 << 6)
#define GPIO_CTL_GPIO_INEN                  (0x1 << 7)
#define GPIO_CTL_PULL_SHIFT                 (8)
#define GPIO_CTL_PULL_MASK                  (0xf << GPIO_CTL_PULL_SHIFT)
#define GPIO_CTL_PULLUP_STRONG              (0x1 << 8)
#define GPIO_CTL_PULLDOWN                   (0x1 << 9)
#define GPIO_CTL_PULLUP                     (0x1 << 11)
#define GPIO_CTL_PADDRV_SHIFT               (12)
#define GPIO_CTL_PADDRV_LEVEL(x)            (((x)/2) << GPIO_CTL_PADDRV_SHIFT)
#define GPIO_CTL_PADDRV_MASK                GPIO_CTL_PADDRV_LEVEL(0x7)
#define GPIO_CTL_FAST_SLEW_RATE             (0x1 << 15)
#define GPIO_CTL_INTC_EN                    (0x1 << 20)
#define GPIO_CTL_INC_TRIGGER_SHIFT          (21)
#define GPIO_CTL_INC_TRIGGER(x)             ((x) << GPIO_CTL_INC_TRIGGER_SHIFT)
#define GPIO_CTL_INC_TRIGGER_MASK           GPIO_CTL_INC_TRIGGER(0x7)
#define GPIO_CTL_INC_TRIGGER_RISING_EDGE    GPIO_CTL_INC_TRIGGER(0x0)
#define GPIO_CTL_INC_TRIGGER_FALLING_EDGE   GPIO_CTL_INC_TRIGGER(0x1)
#define GPIO_CTL_INC_TRIGGER_DUAL_EDGE      GPIO_CTL_INC_TRIGGER(0x2)
#define GPIO_CTL_INC_TRIGGER_HIGH_LEVEL     GPIO_CTL_INC_TRIGGER(0x3)
#define GPIO_CTL_INC_TRIGGER_LOW_LEVEL      GPIO_CTL_INC_TRIGGER(0x4)
#define GPIO_CTL_INTC_MASK                  (0x1 << 25)


/*********I2C mfp config*****************/
#define MFP0_I2C                4
#define I2C_MFP_CFG(x)          (GPIO_CTL_MFP(x)|GPIO_CTL_PULLUP|GPIO_CTL_PADDRV_LEVEL(3))


/*********I2CMT mfp config*****************/
#define MFP0_I2CMT  26
#define MFP1_I2CMT  27
#define I2CMT_MFP_CFG(x)    (GPIO_CTL_MFP(x)|GPIO_CTL_PULLUP_STRONG|GPIO_CTL_PADDRV_LEVEL(3))


/*********SPI mfp config*****************/
#define MFP_SPI1    6

#define SPI_MFP_CFG(x)  (GPIO_CTL_MFP(x)|GPIO_CTL_PULLUP_STRONG|GPIO_CTL_PADDRV_LEVEL(3))


/*********SPIMT mfp config*****************/
#define MFP0_SPIMT  24
#define MFP1_SPIMT  25

#define SPIMT_MFP_CFG(x)    (GPIO_CTL_MFP(x)|GPIO_CTL_PULLUP_STRONG|GPIO_CTL_PADDRV_LEVEL(3))

/* lcdc */
#define LCD_MFP_SEL                  (19 | GPIO_CTL_PADDRV_LEVEL(3))

/* sd0 */
#define SDC0_MFP_SEL             10
#define SDC0_MFP_CFG_VAL    (GPIO_CTL_MFP(SDC0_MFP_SEL)|GPIO_CTL_PULLUP|GPIO_CTL_PADDRV_LEVEL(5))


/* sd1 */
#define SDC1_MFP_SEL             10
#define SDC1_MFP_CFG_VAL    (GPIO_CTL_MFP(SDC1_MFP_SEL)|GPIO_CTL_PULLUP|GPIO_CTL_PADDRV_LEVEL(5))


/* uart0  */
#define UART0_MFP_SEL           2
#define UART0_MFP_CFG (GPIO_CTL_MFP(UART0_MFP_SEL)|GPIO_CTL_SMIT|GPIO_CTL_PULLUP_STRONG|GPIO_CTL_PADDRV_LEVEL(4))

/* uart1 */
#define UART1_MFP_SEL           3
#define UART1_MFP_CFG (GPIO_CTL_MFP(UART1_MFP_SEL) | GPIO_CTL_SMIT | GPIO_CTL_PULLUP_STRONG | GPIO_CTL_PADDRV_LEVEL(4))


/* SPDIFTX */
#define SPDIFTX_MFP_SEL           14
#define SPDIFTX_MFP_CFG (GPIO_CTL_MFP(SPDIFTX_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))

/* SPDIFRX */
#define SPDIFRX_MFP_SEL           15
#define SPDIFRX_MFP_CFG (GPIO_CTL_MFP(SPDIFRX_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))

/* I2STX */
#define I2STX_MFP_SEL             10
#define I2STX_MFP_CFG (GPIO_CTL_MFP(I2STX_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))

/* I2SRX */
#define I2SRX_MFP_SEL             11
#define I2SRX_MFP_CFG (GPIO_CTL_MFP(I2SRX_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))


/* GPIO KEY */

#define GPIOKEY_MFP_PU_CFG (GPIO_CTL_SMIT | GPIO_CTL_PULLUP | GPIO_CTL_GPIO_INEN | GPIO_CTL_INTC_EN | GPIO_CTL_INC_TRIGGER_DUAL_EDGE)
#define GPIOKEY_MFP_CFG (GPIO_CTL_SMIT | GPIO_CTL_GPIO_INEN | GPIO_CTL_INTC_EN | GPIO_CTL_INC_TRIGGER_DUAL_EDGE)


/* PWM */
#define PWM_MFP_CFG (8 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define PWM_PIN_MFP_SET(pin, chan, val)				\
	{.pin_num = pin,		\
	 .pin_chan = chan,		\
	 .mode = val}


/* SPI NOR */
#define SPINOR_MFP_SEL           5
#define SPINOR_MFP_SEL1          6
#define SPINOR_MFP_CFG      (GPIO_CTL_MFP(SPINOR_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4))
#define SPI1NOR_MFP_CFG     (GPIO_CTL_MFP(SPINOR_MFP_SEL1)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4))
#define SPINOR_MFP_PU_CFG   (GPIO_CTL_MFP(SPINOR_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4) | GPIO_CTL_PULLUP)

/* HDMI CEC */
#define CEC_MFP_SEL             21
#define CEC_MFP_CFG (GPIO_CTL_MFP(CEC_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP)
#define gpio12_cec0_d0_node PIN_MFP_SET(12,  CEC_MFP_CFG)

/* ADC KEY */
#define ADCKEY_MFP_SEL          28
#define ADCKEY_MFP_CFG (GPIO_CTL_MFP(ADCKEY_MFP_SEL) | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define gpio21_adckey_lradc3_node   PIN_MFP_SET(21,  ADCKEY_MFP_CFG)

#define ADCKEY_MFP_SEL_WIO1     1
#define ADCKEY_MFP_CFG_WIO1 (GPIO_CTL_MFP(ADCKEY_MFP_SEL_WIO1))
#define wio1_adckey_lradc1_node PIN_MFP_SET(79, ADCKEY_MFP_CFG_WIO1)

#endif /* PINCTRL_CUCKOO_H_ */
