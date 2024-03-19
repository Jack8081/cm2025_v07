/*
 * Copyright (c) 2020 Linaro Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEV_CUCKOO_H_
#define DEV_CUCKOO_H_

/*
dma cfg
*/
#define  CONFIG_DMA_0_NAME              "DMA_0"
#define  CONFIG_DMA_0_PCHAN_NUM         6
#define  CONFIG_DMA_0_VCHAN_NUM         6
#define  CONFIG_DMA_0_VCHAN_PCHAN_NUM   3
#define  CONFIG_DMA_0_VCHAN_PCHAN_START 3

/*
gpio cfg
*/
#define CONFIG_GPIO_A                   1
#define CONFIG_GPIO_A_NAME              "GPIOA"
#define CONFIG_GPIO_B                   0
#define CONFIG_GPIO_B_NAME              "GPIOB"
#define CONFIG_GPIO_C                   0
#define CONFIG_GPIO_C_NAME              "GPIOC"

#define CONFIG_GPIO_PIN2NAME(x)         (((x) < 32) ? CONFIG_GPIO_A_NAME : (((x) < 64) ? CONFIG_GPIO_B_NAME : CONFIG_GPIO_C_NAME))

/*
DSP cfg
*/
#define  CONFIG_DSP_NAME                "DSP"
#define  CONFIG_DSP_IRQ_PRI             0

/*
ANC cfg
*/
#define CONFIG_ANC_NAME                 "anc"

/*
BTC cfg
*/
#define  CONFIG_BTC_NAME                "BTC"
#define  CONFIG_BTC_IRQ_PRI             0
#define  CONFIG_TWS_IRQ_PRI             0

/*
 * Audio cfg
 */
#ifndef CONFIG_AUDIO_OUT_ACTS_DEV_NAME
#define CONFIG_AUDIO_OUT_ACTS_DEV_NAME  "audio_out"
#endif
#ifndef CONFIG_AUDIO_IN_ACTS_DEV_NAME
#define CONFIG_AUDIO_IN_ACTS_DEV_NAME   "audio_in"
#endif

/*
 * DSP cfg
 */
#ifndef CONFIG_DSP_ACTS_DEV_NAME
#define CONFIG_DSP_ACTS_DEV_NAME        "dsp_acts"
#endif

/*
 * DE cfg
 */
#define CONFIG_DISPLAY_ENGINE_DEV_NAME  "de_acts"

/*
 * LCD cfg
 */
#define CONFIG_LCD_DISPLAY_DEV_NAME     "lcd_panel"
#define CONFIG_LCDC_DEV_NAME            "lcdc_acts"
#define CONFIG_LCDC_DMA_CHAN_ID         CONFIG_DMA_LCD_RESEVER_CHAN
#define CONFIG_LCDC_DMA_LINE_ID         (0)

/*
 * TP cfg
 */
#define CONFIG_TPKEY_DEV_NAME           "tpkey"

/*
PMUADC cfg
*/
#define PMUADC_ID_CHARGI                (0)
#define PMUADC_ID_DC5V                  (1)
#define PMUADC_ID_BATV                  (2)
#define PMUADC_ID_SENSOR                (3)
#define PMUADC_ID_SVCC                  (4)
#define PMUADC_ID_LRADC1                (5)
#define PMUADC_ID_LRADC2                (6)
#define PMUADC_ID_LRADC3                (7)

#endif /* DEV_CUCKOO_H_ */
