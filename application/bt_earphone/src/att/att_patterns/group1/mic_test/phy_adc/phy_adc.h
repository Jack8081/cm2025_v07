/*
* common define for all pattern
*/
#ifndef _PHY_ADC_H_
#define _PHY_ADC_H_

#include "att_pattern_test.h"

#define ADC_DMA_CHANNEL 0

/* CMU AUDIOPLL0 CTL bits */
#define CMU_AUDIOPLL0_CTL_PMD         (8)
#define CMU_AUDIOPLL0_CTL_EN          (7)
#define CMU_AUDIOPLL0_CTL_APS0_SHIFT  (0)
#define CMU_AUDIOPLL0_CTL_APS0(x)    ((x) << CMU_AUDIOPLL0_CTL_APS0_SHIFT)
#define CMU_AUDIOPLL0_CTL_APS0_MASK	  CMU_AUDIOPLL0_CTL_APS0(0x3F)

#define CMU_AUDIOPLL0_CTL_APS0_F(x)    ((x) << CMU_AUDIOPLL0_CTL_APS0_SHIFT)
#define CMU_AUDIOPLL0_CTL_APS0_F_MASK	  CMU_AUDIOPLL0_CTL_APS0(0x3F)
#define CMU_AUDIOPLL0_CTL_MODE_F     BIT(6)



#define AUDIO_PLL0_DEBUG             (CMUA_REG_BASE + 0x48)
#define AUDIO_PLL1_DEBUG             (CMUA_REG_BASE + 0x4C)

/* 48ksr serials */
#define PLL_65536               (65536000)
#define PLL_49152               (49152000)
#define PLL_24576               (24576000)
#define PLL_16384               (16384000)
#define PLL_12288               (12288000)
#define PLL_8192                 (8192000)
#define PLL_6144                 (6144000)
#define PLL_4096                 (4096000)
#define PLL_3072                 (3072000)
#define PLL_2048                 (2048000)

/* 44.1ksr serials */
#define PLL_602112             (60211200)
#define PLL_451584             (45158400)
#define PLL_225792             (22579200)
#define PLL_112896             (11289600)
#define PLL_56448               (5644800)
#define PLL_28224               (2822400)

#define ADC_CH_INPUT0N                         (1 << 0) /* ADC channel INPUT0 negative point */
#define ADC_CH_INPUT0P                         (1 << 1) /* ADC channel INPUT0 positive point */
#define ADC_CH_INPUT0NP_DIFF                   (ADC_CH_INPUT0N | ADC_CH_INPUT0P)
#define ADC_CH_INPUT1N                         (1 << 2) /* ADC channel INPUT1 negative point */
#define ADC_CH_INPUT1P                         (1 << 3) /* ADC channel INPUT1 positive point */
#define ADC_CH_INPUT1NP_DIFF                   (ADC_CH_INPUT1N | ADC_CH_INPUT1P)
#define ADC_CH_DISABLE                         (0)
#define ADC_CH_DMIC                            (0xFF)

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

//-----------------------------------ADC_Control_Register Bits Location------------------------------------------//
#define     ADC_DIGCTL_ADC_OVFS_SHIFT                                         16
#define     ADC_DIGCTL_ADC_OVFS_MASK                                          (0x7 << ADC_DIGCTL_ADC_OVFS_SHIFT)
#define     ADC_DIGCTL_ADC_OVFS(x)                                            ((x) << ADC_DIGCTL_ADC_OVFS_SHIFT)

#define     ADC_DIGCTL_ADC1_DIG_EN                                            BIT(13)
#define     ADC_DIGCTL_ADC0_DIG_EN                                            BIT(12)

#define      ADC_DIGCTL_ADC_DIG_SHIFT                                         (12)
#define      ADC_DIGCTL_ADC_DIG_MASK                                          ((0x3) << ADC_DIGCTL_ADC_DIG_SHIFT)

#define     ADC_DIGCTL_DMIC_PRE_GAIN_SHIFT                                    8
#define     ADC_DIGCTL_DMIC_PRE_GAIN_MASK                                     (0x7 << ADC_DIGCTL_DMIC_PRE_GAIN_SHIFT)
#define     ADC_DIGCTL_DMIC_PRE_GAIN(x)                                       ((x) << ADC_DIGCTL_DMIC_PRE_GAIN_SHIFT)

#define     ADC_DIGCTL_DMIC01_CHS                                             BIT(6)

#define     ADC_DIGCTL_ADC_FIR_MD_SEL_SHIFT                                   4
#define     ADC_DIGCTL_ADC_FIR_MD_SEL_MASK                                    (0x3 << ADC_DIGCTL_ADC_FIR_MD_SEL_SHIFT)
#define     ADC_DIGCTL_ADC_FIR_MD_SEL(x)                                      ((x) << ADC_DIGCTL_ADC_FIR_MD_SEL_SHIFT)

#define     ADC_DIGCTL_ADDEN                                                  BIT(3)
#define     ADC_DIGCTL_AADEN                                                  BIT(2)

#define     CH0_DIGCTL_FGAIN_SHIFT                                            24
#define     CH0_DIGCTL_FGAIN_MASK                                             (0x7 << CH0_DIGCTL_FGAIN_SHIFT)
#define     CH0_DIGCTL_FGAIN(x)                                               ((x) << CH0_DIGCTL_FGAIN_SHIFT)
#define     CH0_DIGCTL_SGAIN_SHIFT                                            20
#define     CH0_DIGCTL_SGAIN_MASK                                             (0x7 << CH0_DIGCTL_SGAIN_SHIFT)
#define     CH0_DIGCTL_SGAIN(x)                                               ((x) << CH0_DIGCTL_SGAIN_SHIFT)
#define     CH0_DIGCTL_MIC_SEL                                                BIT(17)
#define     CH0_DIGCTL_DAT_OUT_EN                                             BIT(15)
#define     CH0_DIGCTL_HPF_AS_TS_SHIFT                                        13
#define     CH0_DIGCTL_HPF_AS_TS_MASK                                         (0x3 << CH0_DIGCTL_HPF_AS_TS_SHIFT)
#define     CH0_DIGCTL_HPF_AS_TS(x)                                           ((x) << CH0_DIGCTL_HPF_AS_TS_SHIFT)
#define     CH0_DIGCTL_HPF_AS_EN                                              BIT(12)
#define     CH0_DIGCTL_HPFEN                                                  BIT(11)
#define     CH0_DIGCTL_HPF_S                                                  BIT(10)
#define     CH0_DIGCTL_HPF_N_SHIFT                                            4
#define     CH0_DIGCTL_HPF_N_MASK                                             (0x3F << CH0_DIGCTL_HPF_N_SHIFT)
#define     CH0_DIGCTL_HPF_N(x)                                               ((x) << CH0_DIGCTL_HPF_N_SHIFT)

#define     CH1_DIGCTL_FGAIN_SHIFT                                            24
#define     CH1_DIGCTL_FGAIN_MASK                                             (0x7 << CH1_DIGCTL_FGAIN_SHIFT)
#define     CH1_DIGCTL_FGAIN(x)                                               ((x) << CH1_DIGCTL_FGAIN_SHIFT)
#define     CH1_DIGCTL_SGAIN_SHIFT                                            20
#define     CH1_DIGCTL_SGAIN_MASK                                             (0x7 << CH1_DIGCTL_SGAIN_SHIFT)
#define     CH1_DIGCTL_SGAIN(x)                                               ((x) << CH1_DIGCTL_SGAIN_SHIFT)
#define     CH1_DIGCTL_MIC_SEL                                                BIT(17)
#define     CH1_DIGCTL_DAT_OUT_EN                                             BIT(15)
#define     CH1_DIGCTL_HPF_AS_TS_SHIFT                                        13
#define     CH1_DIGCTL_HPF_AS_TS_MASK                                         (0x3 << CH1_DIGCTL_HPF_AS_TS_SHIFT)
#define     CH1_DIGCTL_HPF_AS_TS(x)                                           ((x) << CH1_DIGCTL_HPF_AS_TS_SHIFT)
#define     CH1_DIGCTL_HPF_AS_EN                                              BIT(12)
#define     CH1_DIGCTL_HPFEN                                                  BIT(11)
#define     CH1_DIGCTL_HPF_S                                                  BIT(10)
#define     CH1_DIGCTL_HPF_N_SHIFT                                            4
#define     CH1_DIGCTL_HPF_N_MASK                                             (0x3F << CH1_DIGCTL_HPF_N_SHIFT)
#define     CH1_DIGCTL_HPF_N(x)                                               ((x) << CH1_DIGCTL_HPF_N_SHIFT)
#define     ADC_FIFOCTL_IDRQ_LEVEL_SHIFT                                      8
#define     ADC_FIFOCTL_IDRQ_LEVEL_MASK                                       (0x3F << ADC_FIFOCTL_IDRQ_LEVEL_SHIFT)
#define     ADC_FIFOCTL_IDRQ_LEVEL(x)                                         ((x) << ADC_FIFOCTL_IDRQ_LEVEL_SHIFT)
#define     ADC_FIFOCTL_ADCFIFO0_DMAWIDTH                                     BIT(7)
#define     ADC_FIFOCTL_ADF0OS_SHIFT                                          4
#define     ADC_FIFOCTL_ADF0OS_MASK                                           (0x3 << ADC_FIFOCTL_ADF0OS_SHIFT)
#define     ADC_FIFOCTL_ADF0OS(x)                                             ((x) << ADC_FIFOCTL_ADF0OS_SHIFT)
#define     ADC_FIFOCTL_ADF0FIE                                               BIT(2)
#define     ADC_FIFOCTL_ADF0FDE                                               BIT(1)
#define     ADC_FIFOCTL_ADF0RT                                                BIT(0)

#define     ADC_STAT_FIFO0_ER                                                 BIT(9)
#define     ADC_STAT_ADF0EF                                                   BIT(8)
#define     ADC_STAT_ADF0IP                                                   BIT(7)
#define     ADC_STAT_ADF0S_SHIFT                                              0
#define     ADC_STAT_ADF0S_MASK                                               (0x3F << ADC_STAT_ADF0S_SHIFT)
#define     ADC_STAT_ADF0S(x)                                                 ((x) << ADC_STAT_ADF0S_SHIFT)
#define     ADC_FIFO0_DAT_ADDAT_SHIFT                                         8
#define     ADC_FIFO0_DAT_ADDAT_MASK                                          (0xFFFFFF << ADC_FIFO0_DAT_ADDAT_SHIFT)
#define     ADC_FIFO0_DAT_ADDAT(x)                                            ((x) << ADC_FIFO0_DAT_ADDAT_SHIFT)

#define     HW_TRIGGER_ADC_CTL_INT_TO_ADC1_EN                                 BIT(5)
#define     HW_TRIGGER_ADC_CTL_INT_TO_ADC0_EN                                 BIT(4)
#define     HW_TRIGGER_ADC_CTL_INT_TO_ADC_SHIFT                               (4)
#define     HW_TRIGGER_ADC_CTL_INT_TO_ADC_MASK                                (0x3 << HW_TRIGGER_ADC_CTL_INT_TO_ADC_SHIFT)

#define     HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL_SHIFT                          0
#define     HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL_MASK                           (0xF << HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL_SHIFT)
#define     HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL(x)                             ((x) << HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL_SHIFT)

#define     ADC0_ANACTL_PA2AD0_EN                                             BIT(22)
#define     ADC0_ANACTL_PA2AD0_INV                                            BIT(21)
#define     ADC0_ANACTL_PA2AD0_PD_EN                                          BIT(20)
#define     ADC0_ANACTL_PA2AD0FDSE                                            BIT(19)
#define     ADC0_ANACTL_AD0_DLY_SHIFT                                         17
#define     ADC0_ANACTL_AD0_DLY_MASK                                          (0x3 << ADC0_ANACTL_AD0_DLY_SHIFT)
#define     ADC0_ANACTL_AD0_DLY(x)                                            ((x) << ADC0_ANACTL_AD0_DLY_SHIFT)
#define     ADC0_ANACTL_AD0_CHOP                                              BIT(16)
#define     ADC0_ANACTL_FDBUF0_EN                                             BIT(15)
#define     ADC0_ANACTL_FDBUF0_IRS_SHIFT                                      13
#define     ADC0_ANACTL_FDBUF0_IRS_MASK                                       (0x3 << ADC0_ANACTL_FDBUF0_IRS_SHIFT)
#define     ADC0_ANACTL_FDBUF0_IRS(x)                                         ((x) << ADC0_ANACTL_FDBUF0_IRS_SHIFT)
#define     ADC0_ANACTL_ADC0_EN                                               BIT(12)
#define     ADC0_ANACTL_INPUT0N_EN_SHIFT                                      10
#define     ADC0_ANACTL_INPUT0N_EN_MASK                                       (0x3 << ADC0_ANACTL_INPUT0N_EN_SHIFT)
#define     ADC0_ANACTL_INPUT0N_EN(x)                                         ((x) << ADC0_ANACTL_INPUT0N_EN_SHIFT)
#define     ADC0_ANACTL_INPUT0P_EN_SHIFT                                      8
#define     ADC0_ANACTL_INPUT0P_EN_MASK                                       (0x3 << ADC0_ANACTL_INPUT0P_EN_SHIFT)
#define     ADC0_ANACTL_INPUT0P_EN(x)                                         ((x) << ADC0_ANACTL_INPUT0P_EN_SHIFT)
#define     ADC0_ANACTL_OPFAU                                                 BIT(7)
#define     ADC0_ANACTL_INPUT0_IN_MODE                                        BIT(5)
#define     ADC0_ANACTL_ADC0_IRS                                              BIT(4)
#define     ADC0_ANACTL_INPUT0_ICS_SHIFT                                      2
#define     ADC0_ANACTL_INPUT0_ICS_MASK                                       (0x3 << ADC0_ANACTL_INPUT0_ICS_SHIFT)
#define     ADC0_ANACTL_INPUT0_ICS(x)                                         ((x) << ADC0_ANACTL_INPUT0_ICS_SHIFT)
#define     ADC0_ANACTL_INPUT0_IRS_SHIFT                                      0
#define     ADC0_ANACTL_INPUT0_IRS_MASK                                       (0x3 << ADC0_ANACTL_INPUT0_IRS_SHIFT)
#define     ADC0_ANACTL_INPUT0_IRS(x)                                         ((x) << ADC0_ANACTL_INPUT0_IRS_SHIFT)

#define     ADC1_ANACTL_PA2AD1_EN                                             BIT(22)
#define     ADC1_ANACTL_PA2AD1_INV                                            BIT(21)
#define     ADC1_ANACTL_PA2AD1_PD_EN                                          BIT(20)
#define     ADC1_ANACTL_MIX2AD1FDSE                                           BIT(19)
#define     ADC1_ANACTL_AD1_DLY_SHIFT                                         17
#define     ADC1_ANACTL_AD1_DLY_MASK                                          (0x3 << ADC1_ANACTL_AD1_DLY_SHIFT)
#define     ADC1_ANACTL_AD1_DLY(x)                                            ((x) << ADC1_ANACTL_AD1_DLY_SHIFT)
#define     ADC1_ANACTL_AD1_CHOP                                              BIT(16)
#define     ADC1_ANACTL_FDBUF1_EN                                             BIT(15)
#define     ADC1_ANACTL_FDBUF1_IRS_SHIFT                                      13
#define     ADC1_ANACTL_FDBUF1_IRS_MASK                                       (0x3 << ADC1_ANACTL_FDBUF1_IRS_SHIFT)
#define     ADC1_ANACTL_FDBUF1_IRS(x)                                         ((x) << ADC1_ANACTL_FDBUF1_IRS_SHIFT)
#define     ADC1_ANACTL_ADC1_EN                                               BIT(12)
#define     ADC1_ANACTL_INPUT1N_EN_SHIFT                                      10
#define     ADC1_ANACTL_INPUT1N_EN_MASK                                       (0x3 << ADC1_ANACTL_INPUT1N_EN_SHIFT)
#define     ADC1_ANACTL_INPUT1N_EN(x)                                         ((x) << ADC1_ANACTL_INPUT1N_EN_SHIFT)
#define     ADC1_ANACTL_INPUT1P_EN_SHIFT                                      8
#define     ADC1_ANACTL_INPUT1P_EN_MASK                                       (0x3 << ADC1_ANACTL_INPUT1P_EN_SHIFT)
#define     ADC1_ANACTL_INPUT1P_EN(x)                                         ((x) << ADC1_ANACTL_INPUT1P_EN_SHIFT)
#define     ADC1_ANACTL_OPFAU                                                 BIT(7)
#define     ADC1_ANACTL_INPUT1_IN_MODE                                        BIT(5)
#define     ADC1_ANACTL_ADC1_IRS                                              BIT(4)
#define     ADC1_ANACTL_INPUT1_ICS_SHIFT                                      2
#define     ADC1_ANACTL_INPUT1_ICS_MASK                                       (0x3 << ADC1_ANACTL_INPUT1_ICS_SHIFT)
#define     ADC1_ANACTL_INPUT1_ICS(x)                                         ((x) << ADC1_ANACTL_INPUT1_ICS_SHIFT)
#define     ADC1_ANACTL_INPUT1_IRS_SHIFT                                      0
#define     ADC1_ANACTL_INPUT1_IRS_MASK                                       (0x3 << ADC1_ANACTL_INPUT1_IRS_SHIFT)
#define     ADC1_ANACTL_INPUT1_IRS(x)                                         ((x) << ADC1_ANACTL_INPUT1_IRS_SHIFT)

#define     ADC_COM_ANACTL_SHCL_SET_SHIFT                                     24
#define     ADC_COM_ANACTL_SHCL_SET_MASK                                      (0xFF << ADC_COM_ANACTL_SHCL_SET_SHIFT)
#define     ADC_COM_ANACTL_SHCL_SET(x)                                        ((x) << ADC_COM_ANACTL_SHCL_SET_SHIFT)
#define     ADC_COM_ANACTL_SHCL_PW_SHIFT                                      16
#define     ADC_COM_ANACTL_SHCL_PW_MASK                                       (0xFF << ADC_COM_ANACTL_SHCL_PW_SHIFT)
#define     ADC_COM_ANACTL_SHCL_PW(x)                                         ((x) << ADC_COM_ANACTL_SHCL_PW_SHIFT)
#define     ADC_COM_ANACTL_SHCL_SEL                                           BIT(15)
#define     ADC_COM_ANACTL_NDBFMRS                                            BIT(14)
#define     ADC_COM_ANACTL_NDBFM_SHIFT                                        12
#define     ADC_COM_ANACTL_NDBFM_MASK                                         (0x3 << ADC_COM_ANACTL_NDBFM_SHIFT)
#define     ADC_COM_ANACTL_NDBFM(x)                                           ((x) << ADC_COM_ANACTL_NDBFM_SHIFT)
#define     ADC_COM_ANACTL_EQU_CONS_SHIFT                                     9
#define     ADC_COM_ANACTL_EQU_CONS_MASK                                      (0x7 << ADC_COM_ANACTL_EQU_CONS_SHIFT)
#define     ADC_COM_ANACTL_EQU_CONS(x)                                        ((x) << ADC_COM_ANACTL_EQU_CONS_SHIFT)
#define     ADC_COM_ANACTL_DEMSEL                                             BIT(8)
#define     ADC_COM_ANACTL_DEMEN                                              BIT(7)
#define     ADC_COM_ANACTL_DARSEL_SHIFT                                       4
#define     ADC_COM_ANACTL_DARSEL_MASK                                        (0x7 << ADC_COM_ANACTL_DARSEL_SHIFT)
#define     ADC_COM_ANACTL_DARSEL(x)                                          ((x) << ADC_COM_ANACTL_DARSEL_SHIFT)
#define     ADC_COM_ANACTL_OPCLAMPEN                                          BIT(3)
#define     ADC_COM_ANACTL_DAC1EN                                             BIT(2)
#define     ADC_COM_ANACTL_DAC0EN                                             BIT(1)
#define     ADC_COM_ANACTL_BIASEN                                             BIT(0)

#define     ADC_BIAS_FSUP_CTL_SHIFT                                           28
#define     ADC_BIAS_FSUP_CTL_MASK                                            (0xF << ADC_BIAS_FSUP_CTL_SHIFT)
#define     ADC_BIAS_FSUP_CTL(x)                                              ((x) << ADC_BIAS_FSUP_CTL_SHIFT)
#define     ADC_BIAS_OPFSUPEN                                                 27
#define     ADC_BIAS_BIASSEL_SHIFT                                            25
#define     ADC_BIAS_BIASSEL_MASK                                             (0x3 << ADC_BIAS_BIASSEL_SHIFT)
#define     ADC_BIAS_BIASSEL(x)                                               ((x) << ADC_BIAS_BIASSEL_SHIFT)
#define     ADC_BIAS_OPBUF_ODSC                                               BIT(24)
#define     ADC_BIAS_OPBUF_IQS_SHIFT                                          22
#define     ADC_BIAS_OPBUF_IQS_MASK                                           (0x3 << ADC_BIAS_OPBUF_IQS_SHIFT)
#define     ADC_BIAS_OPBUF_IQS(x)                                             ((x) << ADC_BIAS_OPBUF_IQS_SHIFT)
#define     ADC_BIAS_IOPFSUP_SHIFT                                            20
#define     ADC_BIAS_IOPFSUP_MASK                                             (0x3 << ADC_BIAS_IOPFSUP_SHIFT)
#define     ADC_BIAS_IOPFSUP(x)                                               ((x) << ADC_BIAS_IOPFSUP_SHIFT)
#define     ADC_BIAS_IDLY_SHIFT                                               18
#define     ADC_BIAS_IDLY_MASK                                                (0x3 << ADC_BIAS_IDLY_SHIFT)
#define     ADC_BIAS_IDLY(x)                                                  ((x) << ADC_BIAS_IDLY_SHIFT)
#define     ADC_BIAS_IAD2_SHIFT                                               16
#define     ADC_BIAS_IAD2_MASK                                                (0x3 << ADC_BIAS_IAD2_SHIFT)
#define     ADC_BIAS_IAD2(x)                                                  ((x) << ADC_BIAS_IAD2_SHIFT)
#define     ADC_BIAS_IAD1_SHIFT                                               12
#define     ADC_BIAS_IAD1_MASK                                                (0x7 << ADC_BIAS_IAD1_SHIFT)
#define     ADC_BIAS_IAD1(x)                                                  ((x) << ADC_BIAS_IAD1_SHIFT)
#define     ADC_BIAS_IOPSE_SHIFT                                              10
#define     ADC_BIAS_IOPSE_MASK                                               (0x3 << ADC_BIAS_IOPSE_SHIFT)
#define     ADC_BIAS_IOPSE(x)                                                 ((x) << ADC_BIAS_IOPSE_SHIFT)
#define     ADC_BIAS_ITC_SHIFT                                                8
#define     ADC_BIAS_ITC_MASK                                                 (0x3 << ADC_BIAS_ITC_SHIFT)
#define     ADC_BIAS_ITC(x)                                                   ((x) << ADC_BIAS_ITC_SHIFT)
#define     ADC_BIAS_IOPCLAMPN_SHIFT                                          6
#define     ADC_BIAS_IOPCLAMPN_MASK                                           (0x3 << ADC_BIAS_IOPCLAMPN_SHIFT)
#define     ADC_BIAS_IOPCLAMPN(x)                                             ((x) << ADC_BIAS_IOPCLAMPN_SHIFT)
#define     ADC_BIAS_IOPCLAMPP_SHIFT                                          4
#define     ADC_BIAS_IOPCLAMPP_MASK                                           (0x3 << ADC_BIAS_IOPCLAMPP_SHIFT)
#define     ADC_BIAS_IOPCLAMPP(x)                                             ((x) << ADC_BIAS_IOPCLAMPP_SHIFT)
#define     ADC_BIAS_IOPCM1_SHIFT                                             2
#define     ADC_BIAS_IOPCM1_MASK                                              (0x3 << ADC_BIAS_IOPCM1_SHIFT)
#define     ADC_BIAS_IOPCM1(x)                                                ((x) << ADC_BIAS_IOPCM1_SHIFT)
#define     ADC_BIAS_IOPVB_SHIFT                                              0
#define     ADC_BIAS_IOPVB_MASK                                               (0x3 << ADC_BIAS_IOPVB_SHIFT)
#define     ADC_BIAS_IOPVB(x)                                                 ((x) << ADC_BIAS_IOPVB_SHIFT)

#define     ADC_VMIC_CTL_VMIC1_FILTER                                         BIT(13)
#define     ADC_VMIC_CTL_VMIC0_FILTER                                         BIT(12)
#define     ADC_VMIC_CTL_VMIC1_R_SEL                                          BIT(11)
#define     ADC_VMIC_CTL_VMIC0_R_SEL                                          BIT(10)
#define     ADC_VMIC_CTL_VMIC_BIAS_CTL_SHIFT                                  8
#define     ADC_VMIC_CTL_VMIC_BIAS_CTL_MASK                                   (0x3 << ADC_VMIC_CTL_VMIC_BIAS_CTL_SHIFT)
#define     ADC_VMIC_CTL_VMIC_BIAS_CTL(x)                                     ((x) << ADC_VMIC_CTL_VMIC_BIAS_CTL_SHIFT)
#define     ADC_VMIC_CTL_VMIC1_VOL_SHIFT                                      6
#define     ADC_VMIC_CTL_VMIC1_VOL_MASK                                       (0x3 << ADC_VMIC_CTL_VMIC1_VOL_SHIFT)
#define     ADC_VMIC_CTL_VMIC1_VOL(x)                                         ((x) << ADC_VMIC_CTL_VMIC1_VOL_SHIFT)
#define     ADC_VMIC_CTL_VMIC1_EN_SHIFT                                       4
#define     ADC_VMIC_CTL_VMIC1_EN_MASK                                        (0x3 << ADC_VMIC_CTL_VMIC1_EN_SHIFT)
#define     ADC_VMIC_CTL_VMIC1_EN(x)                                          ((x) << ADC_VMIC_CTL_VMIC1_EN_SHIFT)
#define     ADC_VMIC_CTL_VMIC0_VOL_SHIFT                                      2
#define     ADC_VMIC_CTL_VMIC0_VOL_MASK                                       (0x3 << ADC_VMIC_CTL_VMIC0_VOL_SHIFT)
#define     ADC_VMIC_CTL_VMIC0_VOL(x)                                         ((x) << ADC_VMIC_CTL_VMIC0_VOL_SHIFT)
#define     ADC_VMIC_CTL_VMIC0_EN_SHIFT                                       0
#define     ADC_VMIC_CTL_VMIC0_EN_MASK                                        (0x3 << ADC_VMIC_CTL_VMIC0_EN_SHIFT)
#define     ADC_VMIC_CTL_VMIC0_EN(x)                                          ((x) << ADC_VMIC_CTL_VMIC0_EN_SHIFT)

#define     ADC_REF_LDO_CTL_ADLDO_PD_CTL_SHIFT                                22
#define     ADC_REF_LDO_CTL_ADLDO_PD_CTL_MASK                                 (0x3 << ADC_REF_LDO_CTL_ADLDO_PD_CTL_SHIFT)
#define     ADC_REF_LDO_CTL_ADLDO_PD_CTL(x)                                   ((x) << ADC_REF_LDO_CTL_ADLDO_PD_CTL_SHIFT)
#define     ADC_REF_LDO_CTL_ADLDO_VOL_SHIFT                                   20
#define     ADC_REF_LDO_CTL_ADLDO_VOL_MASK                                    (0x3 << ADC_REF_LDO_CTL_ADLDO_VOL_SHIFT)
#define     ADC_REF_LDO_CTL_ADLDO_VOL(x)                                      ((x) << ADC_REF_LDO_CTL_ADLDO_VOL_SHIFT)
#define     ADC_REF_LDO_CTL_ADLDO_EN_SHIFT                                    18
#define     ADC_REF_LDO_CTL_ADLDO_EN_MASK                                     (0x3 << ADC_REF_LDO_CTL_ADLDO_EN_SHIFT)
#define     ADC_REF_LDO_CTL_ADLDO_EN(x)                                       ((x) << ADC_REF_LDO_CTL_ADLDO_EN_SHIFT)
#define     ADC_REF_LDO_CTL_IB_ADLDO_SHIFT                                    16
#define     ADC_REF_LDO_CTL_IB_ADLDO_MASK                                     (0x3 << ADC_REF_LDO_CTL_IB_ADLDO_SHIFT)
#define     ADC_REF_LDO_CTL_IB_ADLDO(x)                                       ((x) << ADC_REF_LDO_CTL_IB_ADLDO_SHIFT)
#define     ADC_REF_LDO_CTL_DALDO_PD_CTL_SHIFT                                14
#define     ADC_REF_LDO_CTL_DALDO_PD_CTL_MASK                                 (0x3 << ADC_REF_LDO_CTL_DALDO_PD_CTL_SHIFT)
#define     ADC_REF_LDO_CTL_DALDO_PD_CTL(x)                                   ((x) << ADC_REF_LDO_CTL_DALDO_PD_CTL_SHIFT)
#define     ADC_REF_LDO_CTL_DALDO_VOL_SHIFT                                   12
#define     ADC_REF_LDO_CTL_DALDO_VOL_MASK                                    (0x3 << ADC_REF_LDO_CTL_DALDO_VOL_SHIFT)
#define     ADC_REF_LDO_CTL_DALDO_VOL(x)                                      ((x) << ADC_REF_LDO_CTL_DALDO_VOL_SHIFT)
#define     ADC_REF_LDO_CTL_DALDO_EN_SHIFT                                    10
#define     ADC_REF_LDO_CTL_DALDO_EN_MASK                                     (0x3 << ADC_REF_LDO_CTL_DALDO_EN_SHIFT)
#define     ADC_REF_LDO_CTL_DALDO_EN(x)                                       ((x) << ADC_REF_LDO_CTL_DALDO_EN_SHIFT)
#define     ADC_REF_LDO_CTL_IB_DALDO_SHIFT                                    8
#define     ADC_REF_LDO_CTL_IB_DALDO_MASK                                     (0x3 << ADC_REF_LDO_CTL_IB_DALDO_SHIFT)
#define     ADC_REF_LDO_CTL_IB_DALDO(x)                                       ((x) << ADC_REF_LDO_CTL_IB_DALDO_SHIFT)
#define     ADC_REF_LDO_CTL_IB_DGB                                            BIT(6)
#define     ADC_REF_LDO_CTL_IB_VREF_SHIFT                                     4
#define     ADC_REF_LDO_CTL_IB_VREF_MASK                                      (0x3 << ADC_REF_LDO_CTL_IB_VREF_SHIFT)
#define     ADC_REF_LDO_CTL_IB_VREF(x)                                        ((x) << ADC_REF_LDO_CTL_IB_VREF_SHIFT)
#define     ADC_REF_LDO_CTL_VREF_VOL_SHIFT                                    2
#define     ADC_REF_LDO_CTL_VREF_VOL_MASK                                     (0x3 << ADC_REF_LDO_CTL_VREF_VOL_SHIFT)
#define     ADC_REF_LDO_CTL_VREF_VOL(x)                                       ((x) << ADC_REF_LDO_CTL_VREF_VOL_SHIFT)
#define     ADC_REF_LDO_CTL_VREF_FU                                           BIT(1)
#define     ADC_REF_LDO_CTL_VREF_EN                                           BIT(0)
//-------------------------------ADC CLK--------------------------------------------------//
#define     CMU_ADCCLK_ADCDEBUGEN                                             BIT(31)
#define     CMU_ADCCLK_ADCFIFOCLKEN                                           BIT(24)
#define     CMU_ADCCLK_ADCFIREN                                               BIT(23)
#define     CMU_ADCCLK_ADCCICEN                                               BIT(22)
#define     CMU_ADCCLK_ADCDMICEN                                              BIT(21)
#define     CMU_ADCCLK_ADCANAEN                                               BIT(20)
#define     CMU_ADCCLK_ADCFIRCLKRVS                                           BIT(19)
#define     CMU_ADCCLK_ADCCICCLKRVS                                           BIT(18)
#define     CMU_ADCCLK_ADCDMICCLKRVS                                          BIT(17)
#define     CMU_ADCCLK_ADCANACLKRVS                                           BIT(16)

#define     CMU_ADCCLK_ADCFIRCLKDIV_SHIFT                                     12
#define     CMU_ADCCLK_ADCFIRCLKDIV_MASK                                      (0x3<<CMU_ADCCLK_ADCFIRCLKDIV_SHIFT)
#define     CMU_ADCCLK_ADCFIRCLKDIV(x)                                        ((x) << CMU_ADCCLK_ADCFIRCLKDIV_SHIFT)

#define     CMU_ADCCLK_ADCOVFSCLKDIV_SHIFT                                    8
#define     CMU_ADCCLK_ADCOVFSCLKDIV_MASK                                     (0x3 << CMU_ADCCLK_ADCOVFSCLKDIV_SHIFT)
#define     CMU_ADCCLK_ADCOVFSCLKDIV(x)                                       ((x) << CMU_ADCCLK_ADCOVFSCLKDIV_SHIFT)

#define     CMU_ADCCLK_ADCCLKSRC_SHIFT                                        4

#define     CMU_ADCCLK_ADCCLKDIV_SHIFT                                        0
#define     CMU_ADCCLK_ADCCLKDIV_MASK                                         (0x3<<0)
#define     CMU_ADCCLK_ADCCLKDIV(x)                                           ((x) << CMU_ADCCLK_ADCCLKDIV_SHIFT)
/*---------------------------------------------------------------*/

#define ADC_GAIN_MAX 2

typedef struct {
	int16_t ch_again[2];
    int16_t ch_dgain[2];		 
} att_adc_gain_t;

/*
 * enum a_pll_series_e
 * @brief The series of audio pll
 */
typedef enum {
	AUDIOPLL_44KSR = 0, /* 44.1K sample rate seires */
	AUDIOPLL_48KSR /* 48K sample rate series */
} a_pll_series_e;

/*
 * enum a_pll_type_e
 * @brief The audio pll type selection
 */
typedef enum {
	AUDIOPLL_TYPE_0 = 0, /* AUDIO_PLL0 */
	AUDIOPLL_TYPE_1, /* AUDIO_PLL1 */
} a_pll_type_e;

/*
 * enum a_pll_mode_e
 * @brief The mode of audio pll
 */
typedef enum {
	AUDIOPLL_INTEGER_N = 0,  /* integer-N*/
	AUDIOPLL_FRACTIONAL_N    /* fractional-N */
} a_pll_mode_e;

/**
 * enum audio_sr_sel_e
 * @brief The sample rate choices
 */
typedef enum {
	SAMPLE_RATE_8KHZ   = 8,
	SAMPLE_RATE_11KHZ  = 11,   /* 11.025KHz */
	SAMPLE_RATE_12KHZ  = 12,
	SAMPLE_RATE_16KHZ  = 16,
	SAMPLE_RATE_22KHZ  = 22,   /* 22.05KHz */
	SAMPLE_RATE_24KHZ  = 24,
	SAMPLE_RATE_32KHZ  = 32,
	SAMPLE_RATE_44KHZ  = 44,   /* 44.1KHz */
	SAMPLE_RATE_48KHZ  = 48,
	SAMPLE_RATE_64KHZ  = 64,
	SAMPLE_RATE_88KHZ  = 88,   /* 88.2KHz */
	SAMPLE_RATE_96KHZ  = 96,
	SAMPLE_RATE_176KHZ = 176,  /* 176.4KHz */
	SAMPLE_RATE_192KHZ = 192,
} audio_sr_sel_e;

typedef enum
{
    AINCHANNEL_AMIC_L = 0,
    AINCHANNEL_AMIC_R,
    AINCHANNEL_AMIC_LR,
    AINCHANNEL_DMIC_L,
    AINCHANNEL_DMIC_R,
    AINCHANNEL_DMIC_LR,
    AINCHANNEL_I2SRX_MASTER,
    AINCHANNEL_I2SRX_SLAVE,
    AINCHANNEL_MAX,
} ain_channel_e;


typedef enum
{
    INPUTSRC_L_R = 0,
    INPUTSRC_ONLY_L,
    INPUTSRC_ONLY_R
    
} ain_track_e;

typedef struct
{
    att_adc_gain_t ain_gain;
    ain_track_e ain_track;
    //DMACALLBACK ain_dma_cb;
    
    uint8_t dmic_channel_aligning;
    
    audio_sr_sel_e ain_sample_rate;
    uint8_t *inputsrc_buf;
    uint32_t inputsrc_buf_len;
    
    uint32_t adc_bias;
    uint8_t use_vmic;

    void *reserve;
} ain_channel_param_t;


int32_t  att_phy_audio_adc_disable(ain_track_e channel_id);
int32_t att_phy_audio_adc_enable(ain_track_e channel_id, ain_channel_param_t *channel_param);
int32_t att_phy_audio_adc_start(ain_track_e channel_id);
int att_adc_fifo_enable(void);


#endif
