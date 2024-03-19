/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BOARD_CFG_H
#define __BOARD_CFG_H
#include <drivers/cfg_drv/dev_config.h>
#include <soc.h>

//#define FPGA_BOARD

/*
 *  The device module enables the definition, If 1, the corresponding module   is opened, the GPIO configuration is enabled,
 *		If 0, the corresponding module is closed, and the GPIO configuration is turned off
 */
#define CONFIG_GPIO_A							1
#define CONFIG_GPIO_A_NAME      			"GPIOA"
#define CONFIG_GPIO_B							0
#define CONFIG_GPIO_B_NAME      			"GPIOB"
#define CONFIG_GPIO_C							0
#define CONFIG_GPIO_C_NAME      			"GPIOC"
#define CONFIG_WIO      						1
#define CONFIG_WIO_NAME         			"WIO"
#define CONFIG_EXTEND_GPIO						1
#define CONFIG_EXTEND_GPIO_NAME             "GPIOD"

#define CONFIG_GPIO_PIN2NAME(x)          (((x) < 32) ? CONFIG_GPIO_A_NAME : (((x) < 64) ? CONFIG_GPIO_B_NAME : CONFIG_GPIO_C_NAME))

#define CONFIG_SPI_FLASH_0						1
#define CONFIG_SPI_FLASH_NAME      			"spi_flash"
#define CONFIG_SPI_FLASH_1                      0
#define CONFIG_SPI_FLASH_1_NAME      		"spi_flash_1"
#define CONFIG_SPI_FLASH_2                      0
#define CONFIG_SPI_FLASH_2_NAME      		"spi_flash_2"

#define CONFIG_SPINAND_3                    	0
#define CONFIG_SPINAND_FLASH_NAME      	    "spinand"

#define CONFIG_MMC_0                			0
#define CONFIG_MMC_0_NAME           		"MMC_0"
#define CONFIG_MMC_1                			0
#define CONFIG_MMC_1_NAME           		"MMC_1"

#define CONFIG_SD                   			0
#define CONFIG_SD_NAME              		"sd"

#define CONFIG_UART_0           				1
#define CONFIG_UART_0_NAME      			"UART_0"
#define CONFIG_UART_1           				1
#define CONFIG_UART_1_NAME      			"UART_1"
#define CONFIG_UART_2           				0
#define CONFIG_UART_2_NAME      			"UART_2"
#define CONFIG_UART_3           				0
#define CONFIG_UART_3_NAME      			"UART_3"
#define CONFIG_UART_4           				0
#define CONFIG_UART_4_NAME      			"UART_4"

#define CONFIG_PWM          					1
#define CONFIG_PWM_NAME      				"PWM"

#define CONFIG_I2C_0           					1
#define CONFIG_I2C_0_NAME      				"I2C_0"

#define CONFIG_SPI_1           					1
#define CONFIG_SPI_1_NAME      				"SPI_1"
#define CONFIG_SPI_2           					0
#define CONFIG_SPI_2_NAME      				"SPI_2"
#define CONFIG_SPI_3           					0
#define CONFIG_SPI_3_NAME      				"SPI_3"

#define CONFIG_I2CMT_0           				0
#define CONFIG_I2CMT_0_NAME      			"I2CMT_0"
#define CONFIG_I2CMT_1           				0
#define CONFIG_I2CMT_1_NAME      			"I2CMT_1"

#define CONFIG_SPIMT_0           				0
#define CONFIG_SPIMT_0_NAME      			"SPIMT_0"
#define CONFIG_SPIMT_1           				0
#define CONFIG_SPIMT_1_NAME      			"SPIMT_1"


#define CONFIG_AUDIO_DAC_0                      1
#define CONFIG_AUDIO_DAC_0_NAME             "DAC_0"
#define CONFIG_AUDIO_ADC_0						1
#define CONFIG_AUDIO_ADC_0_NAME             "ADC_0"

#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
#define CONFIG_AUDIO_I2STX_0					1
#define CONFIG_AUDIO_I2STX_0_NAME           "I2STX_0"
#else
#define CONFIG_AUDIO_I2STX_0					0
#define CONFIG_AUDIO_I2STX_0_NAME           "I2STX_0"
#endif

#define CONFIG_AUDIO_I2SRX_0					0
#define CONFIG_AUDIO_I2SRX_0_NAME           "I2SRX_0"
#define CONFIG_AUDIO_SPDIFRX_0					0
#define CONFIG_AUDIO_SPDIFRX_0_NAME         "SPDIFRX_0"
#define CONFIG_AUDIO_SPDIFTX_0                  0
#define CONFIG_AUDIO_SPDIFTX_0_NAME         "SPDIFTX_0"

#define CONFIG_PANEL							0
#define CONFIG_LCD_DISPLAY_DEV_NAME			"lcd_panel"

#define CONFIG_DISPLAY_ENGINE_DEV				0
#define CONFIG_DISPLAY_ENGINE_DEV_NAME		"de_acts"

#define CONFIG_LCDC_DEV                         0
#define CONFIG_LCDC_DEV_NAME				"lcdc_acts"

#define CONFIG_ADCKEY                           1
#define CONFIG_INPUT_DEV_ACTS_ADCKEY_NAME   "keyadc"

#define CONFIG_GPIOKEY                          1
#define CONFIG_INPUT_DEV_ACTS_GPIOKEY_NAME  "keygpio"

#define CONFIG_ONOFFKEY                         1
#define CONFIG_INPUT_DEV_ACTS_ONOFF_KEY_NAME "onoffkey"

#define CONFIG_TPKEY							0
#define CONFIG_TPKEY_DEV_NAME				 "tpkey"

#define CONFIG_ACTS_BATTERY						1
#define CONFIG_ACTS_BATTERY_DEV_NAME         "batadc"

#define CONFIG_CEC				 				0
#define CONFIG_ACTS_BATTERY_NTC 				1

#define CONFIG_EAR_INOUT                          0
#define CONFIG_INPUT_DEV_EAR_INOUT_NAME     "ear_inout"

#define CONFIG_TAPKEY							0
#define CONFIG_TAPKEY_DEV_NAME				 "tapkey"

#define CONFIG_ROTARY_ENCODER                   1
#define CONFIG_ROTARY_ENCODER_NAME         "rotary_encoder"

/*
 *  Device module DMA channel definition
 *        DMA CHAN: Defines the DMA channel used, 0xff means automatic allocation
 *        DMA ID: used DMA slots
 */
#define CONFIG_LCDC_DMA_CHAN_ID		CONFIG_DMA_LCD_RESEVER_CHAN
#define CONFIG_LCDC_DMA_LINE_ID		(0)

#define CONFIG_UART_0_USE_TX_DMA   1
#define CONFIG_UART_0_TX_DMA_CHAN  0xff
#define CONFIG_UART_0_TX_DMA_ID    1
#define CONFIG_UART_0_USE_RX_DMA   1
#define CONFIG_UART_0_RX_DMA_CHAN  0xff
#define CONFIG_UART_0_RX_DMA_ID    1

#define CONFIG_UART_1_USE_TX_DMA   1
#define CONFIG_UART_1_TX_DMA_CHAN  0xff
#define CONFIG_UART_1_TX_DMA_ID    2
#define CONFIG_UART_1_USE_RX_DMA   1
#define CONFIG_UART_1_RX_DMA_CHAN  0xff
#define CONFIG_UART_1_RX_DMA_ID    2


#define CONFIG_UART_2_USE_TX_DMA   1
#define CONFIG_UART_2_TX_DMA_CHAN  0xff
#define CONFIG_UART_2_TX_DMA_ID    3
#define CONFIG_UART_2_USE_RX_DMA   1
#define CONFIG_UART_2_RX_DMA_CHAN  0xff
#define CONFIG_UART_2_RX_DMA_ID    3


#define CONFIG_MMC_0_USE_DMA        1
#define CONFIG_MMC_0_DMA_CHAN       0xff
#define CONFIG_MMC_0_DMA_ID         5

#define CONFIG_MMC_1_USE_DMA        0
#define CONFIG_MMC_1_DMA_CHAN       0xff
#define CONFIG_MMC_1_DMA_ID         6

#define CONFIG_PWM_USE_DMA   		1
#define CONFIG_PWM_DMA_CHAN  		0xff
#define CONFIG_PWM_DMA_ID    		21

#define CONFIG_I2C_0_USE_DMA   0
#define CONFIG_I2C_0_DMA_CHAN  0xff
#define CONFIG_I2C_0_DMA_ID    19

#define CONFIG_I2C_1_USE_DMA   0
#define CONFIG_I2C_1_DMA_CHAN  0xff
#define CONFIG_I2C_1_DMA_ID    20

#define CONFIG_I2CMT_0_USE_DMA   0
#define CONFIG_I2CMT_0_DMA_CHAN  0xff
#define CONFIG_I2CMT_1_USE_DMA   0
#define CONFIG_I2CMT_1_DMA_CHAN  0xff

#define CONFIG_SPI_1_USE_DMA   1
#define CONFIG_SPI_1_TXDMA_CHAN  0xff
#define CONFIG_SPI_1_RXDMA_CHAN  0xff
#define CONFIG_SPI_1_DMA_ID    8

#define CONFIG_SPI_2_USE_DMA   1
#define CONFIG_SPI_2_TXDMA_CHAN  0xff
#define CONFIG_SPI_2_RXDMA_CHAN  0xff
#define CONFIG_SPI_2_DMA_ID    9

#define CONFIG_SPI_3_USE_DMA   1
#define CONFIG_SPI_3_TXDMA_CHAN  0xff
#define CONFIG_SPI_3_RXDMA_CHAN  0xff
#define CONFIG_SPI_3_DMA_ID    10

#define CONFIG_SPIMT_0_DMA_CHAN  0xff
#define CONFIG_SPIMT_1_DMA_CHAN  0xff

/* The DMA channel for DAC FIFO0  */
#define CONFIG_AUDIO_DAC_0_FIFO0_DMA_CHAN       (0xff)
/* The DMA slot ID for DAC FIFO0 */
#define CONFIG_AUDIO_DAC_0_FIFO0_DMA_ID         (0xb)
/* The DMA channel for DAC FIFO1  */
#define CONFIG_AUDIO_DAC_0_FIFO1_DMA_CHAN       (0xff)
/* The DMA slot ID for DAC FIFO1 */
#define CONFIG_AUDIO_DAC_0_FIFO1_DMA_ID         (0xc)
/* The DMA channel for ADC FIFO0  */
#define CONFIG_AUDIO_ADC_0_FIFO0_DMA_CHAN       (0xff)
/* The DMA slot ID for ADC FIFO0 */
#define CONFIG_AUDIO_ADC_0_FIFO0_DMA_ID         (0xb)
/* The DMA channel for ADC FIFO1  */
#define CONFIG_AUDIO_ADC_0_FIFO1_DMA_CHAN       (0xff)
/* The DMA slot ID for ADC FIFO1 */
#define CONFIG_AUDIO_ADC_0_FIFO1_DMA_ID         (0xc)
/* The DMA channel for I2STX FIFO0  */
#define CONFIG_AUDIO_I2STX_0_FIFO0_DMA_CHAN     (0xff)
/* The DMA slot ID for I2STX FIFO0 */
#define CONFIG_AUDIO_I2STX_0_FIFO0_DMA_ID       (0xe)
/* The DMA channel for I2SRX FIFO0  */
#define CONFIG_AUDIO_I2SRX_0_FIFO0_DMA_CHAN     (0xff)
/* The DMA slot ID for I2SRX FIFO0 */
#define CONFIG_AUDIO_I2SRX_0_FIFO0_DMA_ID       (0xe)
/* The DMA channel for SPDIFRX FIFO0  */
#define CONFIG_AUDIO_SPDIFRX_0_FIFO0_DMA_CHAN    (0xff)
/* The DMA slot ID for SPDIFRX FIFO0 */
#define CONFIG_AUDIO_SPDIFRX_0_FIFO0_DMA_ID      (0x10)

/*
 *  Device module interrupt priority definition
 */
#define CONFIG_BTC_IRQ_PRI                		0

#define CONFIG_TWS_IRQ_PRI                		0

#define CONFIG_UART_0_IRQ_PRI   				0

#define CONFIG_UART_1_IRQ_PRI   				0

#define CONFIG_UART_2_IRQ_PRI   				0

#define CONFIG_MMC_0_IRQ_PRI        			0

#define CONFIG_MMC_1_IRQ_PRI        			0

#define CONFIG_DMA_IRQ_PRI                      0

#define CONFIG_MPU_IRQ_PRI                      0

#define CONFIG_GPIO_IRQ_PRI                     0

#define CONFIG_I2C_0_IRQ_PRI   					0

#define CONFIG_I2C_1_IRQ_PRI   					0

#define CONFIG_SPI_1_IRQ_PRI   					0

#define CONFIG_SPI_2_IRQ_PRI  					0

#define CONFIG_SPI_3_IRQ_PRI   					0

#define CONFIG_I2CMT_0_IRQ_PRI   			    0

#define CONFIG_I2CMT_1_IRQ_PRI   				0

#define CONFIG_SPIMT_0_IRQ_PRI   				0

#define CONFIG_SPIMT_1_IRQ_PRI   				0

#define CONFIG_AUDIO_DAC_0_IRQ_PRI              0

#define CONFIG_AUDIO_ADC_0_IRQ_PRI              0

#define CONFIG_AUDIO_I2STX_0_IRQ_PRI            0

#define CONFIG_AUDIO_I2SRX_0_IRQ_PRI            0

#define CONFIG_AUDIO_SPDIFRX_0_IRQ_PRI          0

#define CONFIG_PMU_IRQ_PRI                      0

#define CONFIG_RTC_IRQ_PRI                      0

#define CONFIG_WDT_0_IRQ_PRI                    0

#define CONFIG_DSP_IRQ_PRI                		0

#define CONFIG_PMUADC_IRQ_PRI                   0

/*
spi nor flash cfg
*/
#define CONFIG_SPI_FLASH_CHIP_SIZE      0x200000
#define CONFIG_SPI_FLASH_BUS_WIDTH      2
#define CONFIG_SPI_FLASH_DELAY_CHAIN    8
#define CONFIG_SPI_FLASH_FREQ_MHZ    	64

/*
spi nand flash cfg
*/
/**
 * The LARK spinand best delaychain setting as below:
 * Manufactor    delaychain
 *   GD             10
 *   XT	            13
 */
#define CONFIG_SPINAND_FLASH_DELAY_CHAIN    10

/*
mmc board cfg
*/

#define CONFIG_MMC_0_BUS_WIDTH      4
#define CONFIG_MMC_0_CLKSEL         1     /*0 or 1, config by pinctrls*/
#define CONFIG_MMC_0_DATA_REG_WIDTH 4

#define CONFIG_MMC_0_USE_GPIO_IRQ   0
#define CONFIG_MMC_0_GPIO_IRQ_DEV   CONFIG_GPIO_A_NAME  /*CONFIG_GPIO_A_NAME&CONFIG_GPIO_B_NAME&CONFIG_GPIO_C_NAME*/
#define CONFIG_MMC_0_GPIO_IRQ_NUM   10    /*GPIOA10*/
#define CONFIG_MMC_0_GPIO_IRQ_FLAG  0    /*0=GPIO_ACTIVE_HIGH OR 1=GPIO_ACTIVE_LOW*/
#define CONFIG_MMC_0_ENABLE_SDIO_IRQ 0   /* If 1 to enable SD0 SDIO IRQ */


#define CONFIG_MMC_1_BUS_WIDTH      4
#define CONFIG_MMC_1_CLKSEL         0
#define CONFIG_MMC_1_DATA_REG_WIDTH 1
#define CONFIG_MMC_1_MFP
#define CONFIG_MMC_1_USE_GPIO_IRQ   0
#define CONFIG_MMC_1_ENABLE_SDIO_IRQ 0   /* If 1 to enable SD1 SDIO IRQ */

#define CONFIG_MMC_ACTS_ERROR_DETAIL    1 /* If 1 to print detail information when error occured */
#define CONFIG_MMC_WAIT_DAT1_BUSY       1 /* If 1 to wait SD/MMC card data 1 pin busy */
#define CONFIG_MMC_YIELD_WAIT_DMA_DONE  1 /* If  1 to yield task to wait DMA done */
#define CONFIG_MMC_SD0_FIFO_WIDTH_8BITS 0 /* If 1 to enable SD0 FIFO width 8 bits transfer */
#define CONFIG_MMC_STATE_FIFO           0 /* If 1 to enable using FIFO state for CPU read/write operations */

/*
sd board cfg
*/
#define CONFIG_SD_MMC_DEV           CONFIG_MMC_0_NAME /*CONFIG_MMC_0_NAME or CONFIG_MMC_1_NAME*/


/*
uart board cfg
*/
#define CONFIG_UART_0_SPEED     2000000
#define CONFIG_UART_1_SPEED     115200


#define CONFIG_UART_2_SPEED     9600

/*
pwm board cfg
*/
#define CONFIG_PWM_CHANS     4
#define CONFIG_PWM_CYCLE     8000

/*
I2C board cfg
*/
#define CONFIG_I2C_0_CLK_FREQ  100000
#define CONFIG_I2C_0_MAX_ASYNC_ITEMS 10

#define CONFIG_I2C_1_CLK_FREQ  100000
#define CONFIG_I2C_1_MAX_ASYNC_ITEMS 3

/*
SPI board cfg
*/

/*
I2CMT cfg
*/
#define CONFIG_I2CMT_0_CLK_FREQ  100000

#define CONFIG_I2CMT_1_CLK_FREQ  100000


/*
SPIMT cfg
*/

/*
tp board cfg
*/
#define CONFIG_TP_RESET_GPIO  1
#define CONFIG_TP_RESET_GPIO_NAME   CONFIG_GPIO_A_NAME
#define CONFIG_TP_RESET_GPIO_NUM    12
#define CONFIG_TP_RESET_GPIO_FLAG   GPIO_ACTIVE_LOW

/*
audio board cfg
*/

/**
 * The DAC working mode in dedicated PCB layout.
 * - 0: single-end(non-direct drive) mode
 * - 1: single-end(direct drive VRO) mode
 * - 2: differential mode
 */
#define CONFIG_AUDIO_DAC_0_LAYOUT               (2)

/* If 1 to enable DAC high performance which only works in differential layout */
#define CONFIG_AUDIO_DAC_HIGH_PERFORMACE_DIFF_EN (1)

#if (CONFIG_AUDIO_DAC_HIGH_PERFORMACE_DIFF_EN == 1)
#define CONFIG_AUDIO_DAC_HIGH_PERFORMANCE_SHCL_PW      (0x14)
#define CONFIG_AUDIO_DAC_HIGH_PERFORMANCE_SHCL_SET     (0x64)
#define CONFIG_AUDIO_DAC_HIGH_PERFORMANCE_SHCL_CURBIAS (0)
#endif

/**
 * The LARK DAC PA gain setting as below:
 * - 0: 0dB
 * - 1: -1.5dB
 * - 2: -3dB
 * - 3: -4.5dB
 * The LARK DAC PA gain setting as below:
 * PA gain    old plan    diff6db = 1    new plan
 *   7            0db	        6db	             --
 *   6	   -1.5db	     -7.5db	      --
 *   5	     -3db	   	-9db	     0db
 *   4	   -4.3db	    -10.3db	    -1.36db
 *   3	     -6db	      -12db	    -3db
 *   2	   -7.3db	    -13.3db	    -4.5db
 *   1	     -9db	      -15db	    -6db
 *   0	     mute	   	--		    mute
 */
#if (CONFIG_AUDIO_DAC_HIGH_PERFORMACE_DIFF_EN == 1)
#define CONFIG_AUDIO_DAC_0_PA_VOL               (5)
#else
#define CONFIG_AUDIO_DAC_0_PA_VOL               (7)
#endif

/* Enable DAC left and right channels mix function. */
#define CONFIG_AUDIO_DAC_0_LR_MIX               (0)

/* Enable DAC SDM(noise detect mute) function. */
#define CONFIG_AUDIO_DAC_0_NOISE_DETECT_MUTE    (1)

/* SDM mute counter configuration. */
#define CONFIG_AUDIO_DAC_0_SDM_CNT              (0x1000)

/* SDM noise dectection threshold */
#define CONFIG_AUDIO_DAC_0_SDM_THRES            (0x4)

/* Enable DAC automute function when continuously output 512x(configurable) samples 0 data. */
#define CONFIG_AUDIO_DAC_0_AUTOMUTE             (0)

/* Enable ADC loopback to DAC function. */
#define CONFIG_AUDIO_DAC_0_LOOPBACK             (0)

/* If 1 to mute the DAC left channel. */
#define CONFIG_AUDIO_DAC_0_LEFT_MUTE            (0)

/* If 1 to mute the DAC right channel. */
#define CONFIG_AUDIO_DAC_0_RIGHT_MUTE           (0)

/* Auto mute counter configuration. */
#define CONFIG_AUDIO_DAC_0_AM_CNT               (0x1000)

/* Auto noise dectection threshold */
#define CONFIG_AUDIO_DAC_0_AM_THRES             (0x4)

/* If 1 to enable DAC automute IRQ function. */
#define CONFIG_AUDIO_DAC_0_AM_IRQ               (0)

/* The threshold to generate half empty IRQ signal. */
#define CONFIG_AUDIO_DAC_0_PCMBUF_HE_THRES      (0x100)

/* The threshold to generate half full IRQ signal. */
#define CONFIG_AUDIO_DAC_0_PCMBUF_HF_THRES      (0x200)

/* If 1 to open external PA when power on */
#define CONFIG_POWERON_OPEN_EXTERNAL_PA         (0)

/* If 1 to enable DAC power perfered  */
#define CONFIG_AUDIO_DAC_POWER_PREFERRED        (0)

/* If 1 to wait for writting PCMBUF completly */
#define CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_FINISH (1)

/* The timeout out of writing PCMBUF in microsecond */
#define CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_TIMEOUT_US (1000000)

/* The sleep time in millisecond to of writing PCMBUF */
#define CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_SLEEP_MS   (1)

#if (CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_FINISH != 0)
/* Wait until next time writing pcmbuf to write previous one completely */
#define CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_NEXT_TIME  (0)
#endif

/********************************** I2STX CONFIGURATION **********************************/

/**
 * I2STX channel number selection.
 *   - 2: 2 channels
 *   - 4: 4 channels(TDM)
 *   - 8: 8 channels(TDM)
 */
#define CONFIG_AUDIO_I2STX_0_CHANNEL_NUM        (2)

/**
 * I2STX transfer format selection.
 *   - 0: I2S format
 *   - 1: left-justified format
 *   - 2: right-justified format
 *   - 3: TDM format
 */
#define CONFIG_AUDIO_I2STX_0_FORMAT             (0)

/**
 * I2STX BCLK data width.
 *   - 0: 32bits
 *   - 1: 16bits
 */
#define CONFIG_AUDIO_I2STX_0_BCLK_WIDTH         (0)

/* Enable the SRD(sample rate detect) function. */
#define CONFIG_AUDIO_I2STX_0_SRD_EN             (0)

/**
 * I2STX master or slaver mode selection.
 *   - 0: master
 *   - 1: slaver
 */
#define CONFIG_AUDIO_I2STX_0_MODE               (0)

/* Enable in slave mode MCLK to use internal clock. */
#define CONFIG_AUDIO_I2STX_0_SLAVE_INTERNAL_CLK (0)

/**
 * I2STX LRCLK process selection.
 *   - 0: 50% duty
 *   - 1: 1 BCLK
 */
#define CONFIG_AUDIO_I2STX_0_LRCLK_PROC         (0)

/**
 * I2STX MCLK reverse selection.
 *   - 0: normal
 *   - 1: reverse
 */
#define CONFIG_AUDIO_I2STX_0_MCLK_REVERSE       (0)

/* Enable I2STX channel BCLK/LRCLK alway existed which used in master mode. */
#define CONFIG_AUDIO_I2STX_0_ALWAYS_OPEN        (0)

/**
 * I2STX transfer TDM format selection.
 *   - 0: I2S format
 *   - 1: left-justified format
 */
#define CONFIG_AUDIO_I2STX_0_TDM_FORMAT         (0)

/**
 *  I2STX TDM frame start position selection.
 *   - 0: the rising edge of LRCLK with a pulse.
 *   - 1: the rising edge of LRCLK with a 50% duty cycle.
 *   - 2: the falling edge of LRCLK with a 50% duty cycle.
 */
#define CONFIG_AUDIO_I2STX_0_TDM_FRAME          (0)

/**
 * I2STX data output delay selection.
 *   - 0: 2 mclk cycles after the bclk rising edge.
 *   - 1: 3 mclk cycles after the bclk rising edge.
 *   - 2: 4 mclk cycles after the bclk rising edge.
 *   - 3: 5 mclk cycles after the bclk rising edge.
 */
#define CONFIG_AUDIO_I2STX_0_TX_DELAY           (0)


/********************************** SPDIFTX CONFIGURATION **********************************/

/* Enable the clock of SPDIFTX source from I2STX div2 clock. */
#define CONFIG_AUDIO_SPDIFTX_0_CLK_I2STX_DIV2       (0)


/********************************** ADC CONFIGURATION **********************************/
/* The end address of SRAM which used by DMA interleaved mode */
#define AUDIO_IN_DMA_RESERVED_ADDRESS           (0x02000000)



/**
 * ADC0 channel HPF auto-set time selection.
 *   - 0: 1.3ms in 48kfs
 *   - 1: 5ms in 48kfs
 *   - 2: 10ms in 48kfs
 *   - 3: 20ms in 48kfs
 */
#define CONFIG_AUDIO_ADC_0_CH0_HPF_TIME         (0)

/* ADC channel0 frequency which range from 0 ~ 111111b. */
#define CONFIG_AUDIO_ADC_0_CH0_FREQUENCY        (0)

/* If 1 to enable ADC channel0 HPF high frequency range. */
#define CONFIG_AUDIO_ADC_0_CH0_HPF_FC_HIGH      (0)

/**
 * ADC channel1 HPF auto-set time selection.
 *   - 0: 1.3ms in 48kfs
 *   - 1: 5ms in 48kfs
 *   - 2: 10ms in 48kfs
 *   - 3: 20ms in 48kfs
 */
#define CONFIG_AUDIO_ADC_0_CH1_HPF_TIME         (0)

/* ADC channel1 frequency which range from 0 ~ 111111b. */
#define CONFIG_AUDIO_ADC_0_CH1_FREQUENCY        (0)

/* If 1 to enable ADC channel1 HPF high frequency range. */
#define CONFIG_AUDIO_ADC_0_CH1_HPF_FC_HIGH      (0)


/**
 * Audio LDO output voltage selection.
 *   - 0: 1.9*VREF
 *   - 1: 1.95*VREF
 *   - 2: 2.0*VREF
 *   - 3: 2.05*VREF
 */
#define CONFIG_AUDIO_ADC_0_LDO_VOLTAGE          (2)

/*
 * Audio VMIC control MIC power as <vmic-ctl0, vmic-ctl1, vmic-ctl2>.
 *   - 0x: disable VMIC OP
 *   - 2: bypass VMIC OP
 *   - 3: enable VMIC OP
 */
#define CONFIG_AUDIO_ADC_0_VMIC_CTL_ARRAY       {3, 3}

/**
 * Audio VMIC control the MIC voltage as <vmic-vol0, vmic-vol1>.
 *   - 0: 0.8 AVCC
 *   - 1: 0.85 AVCC
 *   - 2: 0.9 AVCC
 *   - 3: 0.95 AVCC
 */
#define CONFIG_AUDIO_ADC_0_VMIC_VOLTAGE_ARRAY   {2, 2}

/**
 * ADC <=> VMIC index mapping
 * {ADC0_VMIC_index, ADC1_VMIC_index, ADC2_VMIC_index, ADC3_VMIC_index}
 */
#define CONFIG_AUDIO_ADC_0_VMIC_MAPPING         {1, 2, 0, 0}

/* Enable ADC fast capacitor charge function. */
#define CONFIG_AUDIO_ADC_0_FAST_CAP_CHARGE      (0)

/********************************** I2SRX CONFIGURATION **********************************/
/**
 * I2SRX channel number selection.
 *   - 2: 2 channels
 *   - 4: 4 channels(TDM)
 *   - 8: 8 channels(TDM)
 */
#define CONFIG_AUDIO_I2SRX_0_CHANNEL_NUM        (2)

/**
 * I2SRX transfer format selection.
 *   - 0: I2S format
 *   - 1: left-justified format
 *   - 2: right-justified format
 *   - 3: TDM format
 */
#define CONFIG_AUDIO_I2SRX_0_FORMAT             (0)

/**
 * I2SRX BCLK data width.
 *   - 0: 32bits
 *   - 1: 16bits
 */
#define CONFIG_AUDIO_I2SRX_0_BCLK_WIDTH         (0)

/* Enable the SRD(sample rate detect) function. */
#define CONFIG_AUDIO_I2SRX_0_SRD_EN             (0)

/**
 * I2SRX master or slaver mode selection.
 *   - 0: master
 *   - 1: slaver
 */
#define CONFIG_AUDIO_I2SRX_0_MODE               (0)

/* Enable in slave mode MCLK to use internal clock. */
#define CONFIG_AUDIO_I2SRX_0_SLAVE_INTERNAL_CLK    (0)

/**
 * I2SRX LRCLK process selection.
 *   - 0: 50% duty
 *   - 1: 1 BCLK
 */
#define CONFIG_AUDIO_I2SRX_0_LRCLK_PROC         (0)

/**
 * I2SRX MCLK reverse selection.
 *   - 0: normal
 *   - 1: reverse
 */
#define CONFIG_AUDIO_I2SRX_0_MCLK_REVERSE       (0)

/**
 * I2SRX transfer TDM format selection.
 *   - 0: I2S format
 *   - 1: left-justified format
 */
#define CONFIG_AUDIO_I2SRX_0_TDM_FORMAT         (0)

/**
 *  I2SRX TDM frame start position selection.
 *   - 0: the rising edge of LRCLK with a pulse.
 *   - 1: the rising edge of LRCLK with a 50% duty cycle.
 *   - 2: the falling edge of LRCLK with a 50% duty cycle.
 */
#define CONFIG_AUDIO_I2SRX_0_TDM_FRAME          (0)

/* If 1 to enable the I2SRX clock source from I2STX. */
#define CONFIG_AUDIO_I2SRX_0_CLK_FROM_I2STX     (0)


/********************************** SPDIFRX CONFIGURATION **********************************/

/* Specify minimal CORE_PLL clock for spdifrx. */
#define CONFIG_AUDIO_SPDIFRX_0_MIN_COREPLL_CLOCK (50000000)


/*
 * panel cfg
 */
/*
 * b'[15:8]: major type, 1-CPU, 4-SPI
 * b'[7-0]: minor type,
 *          For CPU, 0-Intel_8080, 1-Moto_6800
 *          For SPI, 0-3wire_type1, 1-3wire_type2, 2-4wire_type1, 3-4wire_type2, 4-quad, 5-quad_sync
 */
#define CONFIG_PANEL_PORT_TYPE		((4 << 8) | (4))
#define CONFIG_PANEL_PORT_CS		(0)
#define CONFIG_PANEL_PORT_SPI_CPOL  (1)
#define CONFIG_PANEL_PORT_SPI_RX_DELAY_CHAIN_NS	(8)
#define CONFIG_PANEL_PORT_SPI_DUAL_LANE	(1)
/* X-Resolution */
#define CONFIG_PANEL_TIMING_HACTIVE	(454)
/* Y-Resolution */
#define CONFIG_PANEL_TIMING_VACTIVE	(454)
/* Pixel transfer clock rate in KHz */
#define CONFIG_PANEL_TIMING_PIXEL_CLK_KHZ (50000)
/* Refresh rate in Hz */
#define CONFIG_PANEL_TIMING_REFRESH_RATE_HZ (60)
/* TE signal exists */
#define CONFIG_PANEL_TIMING_TE_ACTIVE	(1)

//#define CONFIG_PANEL_BACKLIGHT_PWM_CHAN	(0)
//#define CONFIG_PANEL_BACKLIGHT_GPIO	GPIO_CFG_MAKE(CONFIG_GPIO_C_NAME, 0, GPIO_ACTIVE_HIGH, 1)

/* brightness range [0, 255] */
#define CONFIG_PANEL_BRIGHTNESS		(255)
#define CONFIG_PANEL_TE_SCANLINE	(400)

/*
 * tp cfg
 */
#define CONFIG_TPKEY_I2C_NAME		CONFIG_I2C_0_NAME
#define CONFIG_TPKEY_LOWPOWER		(1)

/*
 * earinout cfg
 */
#define CONFIG_EARINOUT_I2C_NAME		CONFIG_I2C_0_NAME

/*
 * tabkey cfg
 */
#define CONFIG_TABKEY_I2C_NAME		CONFIG_I2C_0_NAME


/*
PMU cfg
*/

/* If 1 to enable the ON-OFF key short press detection function. */
#define CONFIG_PMU_ONOFF_SHORT_DETECT            (1)

/* If 1 to indicates that ON-OFF key and REMOTE key use the same WIO */
#define CONFIG_PMU_ONOFF_REMOTE_SAME_WIO         (0)

/*
PMUADC cfg
*/

#define CONFIG_PMUADC_DEBOUNCE                   (1)

/** PMUADC battery channel over sampling counter
 *   - 0: disable over sampling
 *   - 1: 8 times
 *   - 2: 32 times
 *   - 3: 128 times
 */
#define CONFIG_PMUADC_BAT_AVG_CNT                (1)

/* If 1 to wait PMUADC AVG sample completely  */
#define CONFIG_PMUADC_BAT_WAIT_AVG_COMPLETE      (0)

/** PMUADC LRADC1 channel over sampling counter
 *   - 0: disable over sampling
 *   - 1: 8 times
 *   - 2: 32 times
 *   - 3: 128 times
 */
#define CONFIG_PMUADC_LRADC1_AVG                 (0)

/**
 * PMU ADC LRADC clock source selection.
 *   - 0: HOSC/126 254KHz
 *   - 1: HOSC/62  516KHz
 *   - 2: HOSC/30  1067KHz
 *   - 3: RC4M/16  250KHz
 */
#define CONFIG_PMUADC_CLOCK_SOURCE               (0)

/**
 * PMU ADC LRADC clock source divisor selection.
 *   - 0: /1
 *   - 1: /2
 *   - 2: /4
 *   - 3: /8
 */
#define CONFIG_PMUADC_CLOCK_DIV                  (0)

/**
 * PMU ADC previous buffer current BIAS selection.
 *   - 0: 0.25uA
 *   - 1: 0.5uA
 *   - 2: 0.75uA
 *   - 3: 1uA
 */
#define CONFIG_PMUADC_IBIAS_BUF_SEL              (1)

/**
 * PMU ADC core current BIAS selection.
 *   - 0: 0.25uA
 *   - 1: 0.5uA
 *   - 2: 0.75uA
 *   - 3: 1uA
 */
#define CONFIG_PMUADC_IBIAS_ADC_SEL              (1)

/* The timeout of sync counter8hz */
#define CONFIG_PMU_COUNTER8HZ_SYNC_TIMEOUT_US    (200000)

/* If 1 to enable backup time when power off */
#define CONFIG_PM_BACKUP_TIME_FUNCTION_EN        (0)

#define CONFIG_PM_BACKUP_TIME_NVRAM_ITEM_NAME    "PM_BAK_TIME"

/*
ADCKEY cfg
*/
/* The time interval in millisecond to polling read the PMU ADC key. */
#define CONFIG_ADCKEY_POLL_INTERVAL_MS           (20)

/* The total time in millisecond to polling read the PMU ADC key. */
#define CONFIG_ADCKEY_POLL_TOTAL_MS              (1000)

/* The stable counter of sample filter. */
#define CONFIG_ADCKEY_SAMPLE_FILTER_CNT          (3)

/* The LRADC channel for ADC KEY */
#define CONFIG_ADCKEY_LRADC_CHAN                 (PMUADC_ID_LRADC1)

/*
ONOFFKEY cfg
*/
/*
 * The time threshold in millisecond to estimate the on-off key press is a long time pressed.
 *   - 0: 50ms < t < 0.125s is a short pressed key press; t >= 0.125s is a long pressed key.
 *   - 1: 50ms < t < 0.25s is a short pressed key; t >= 0.25s is a long pressed key.
 *   - 2: 50ms < t < 0.5s is a short pressed key press; t >= 0.5s is a long pressed key.
 *   - 3: 50ms < t < 1s is a short pressed key press; t >= 1s is a long pressed key.
 *   - 4: 50ms < t < 1.5s is a short pressed key press; t >= 1.5s is a long pressed key.
 *   - 5: 50ms < t < 2s is a short pressed key press; t >= 2s is a long pressed key.
 *   - 6: 50ms < t < 3s is a short pressed key press; t >= 3s is a long pressed key.
 *   - 7: 50ms < t < 4s is a short pressed key press; t >= 4s is a long pressed key.
 */
#define CONFIG_ONOFFKEY_LONG_PRESS_TIME                (3)

/*
 * ON-OFF key function selection.
 *   - 0: no function
 *   - 1: reset
 *   - 2: restart
 */
#define CONFIG_ONOFFKEY_FUNCTION                       (0)

/* The time interval in millisecond to polling read the PMU ADC key. */
#define CONFIG_ONOFFKEY_POLL_INTERVAL_MS               (20)

/* The total time in millisecond to polling onoff ADC key */
#define CONFIG_ONOFFKEY_POLL_TOTAL_MS                  (6000)

/* The stable counter for ONOFF KEY sample filter */
#define CONFIG_ONOFFKEY_SAMPLE_FILTER_CNT              (3)

/* The key code of ONOFF KEY which defined by user */
#define CONFIG_ONOFFKEY_USER_KEYCODE                   (1) /* KEY_POWER which reference to input_dev.h */

/*
GPIOKEY cfg
*/

/* The time interval in millisecond to polling read the GPIO key. */
#define CONFIG_GPIOKEY_POLL_INTERVAL_MS                (20)

/* The total time in millisecond to polling onoff GPIO key */
#define CONFIG_GPIOKEY_POLL_TOTAL_MS                   (6000)

/* The stable counter for GPIO KEY sample filter */
#define CONFIG_GPIOKEY_SAMPLE_FILTER_CNT               (3)

/* The voltage level when gpio key is pressed */
#define CONFIG_GPIOKEY_PRESSED_VOLTAGE_LEVEL           (0)

/* The key code of GPIO KEY which defined by user */
#define CONFIG_GPIOKEY_USER_KEYCODE                    (9) /* KEY_TBD which reference to input_dev.h */

/*
RTC cfg
*/
/**
 *  The RTC clock source selection.
 * - 0: RTC_CLKSRC_HOSC_4HZ
 * - 1: RTC_CLKSRC_HCL_LOSC_100HZ
 */
#define CONFIG_RTC_CLK_SOURCE                    (1)

/*
GPIO extend  ET6416 cfg
*/
#define CONFIG_EXTEND_GPIO_I2C_NAME       CONFIG_I2C_1_NAME
#define CONFIG_EXTEND_GPIO_ISR_GPIO       GPIO_CFG_MAKE(CONFIG_GPIO_B_NAME, 30, GPIO_ACTIVE_LOW, 1)
#define CONFIG_GPIO_VDD18V_POWER_ON       25   /*GPIO25*/

/*
Watchdog cfg
*/

/*
Battery cfg
*/

/* The time interval for battery voltage showing debug. */
#define CONFIG_BATTERY_DEBUG_INTERVAL_SEC        (60)

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER
/* extern coulometer device name */
#define CONFIG_ACTS_EXT_COULOMETER_DEV_NAME     "coulometer"

/* extern coulometer use i2c device name */
#define CONFIG_COULOMETER_I2C_NAME		CONFIG_I2C_0_NAME

/* extern coulometer poll interval period ms */
#define CONFIG_COULOMETER_INTERVAL_MSEC        (1000)

#endif

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_EXTERNAL
/* extern charger use i2c device name */
#define CONFIG_EXT_CHARGER_I2C_NAME		CONFIG_I2C_0_NAME

#define CONFIG_EXT_CHARGER_ISR_GPIO		GPIO_CFG_MAKE(CONFIG_WIO_NAME, 1, GPIO_ACTIVE_LOW, 1)    // WIO1

#endif


#endif /* __BOARD_CFG_H */
