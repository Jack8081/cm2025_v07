/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AUDIO physical common API
 */

#ifndef __PHY_AUDIO_H__
#define __PHY_AUDIO_H__

//----------------------DAC_Control_Register Bits Location-------------------------------------//
#define     DAC_DIGCTL_FIR_MODE_SHIFT                                         30
#define     DAC_DIGCTL_FIR_MODE_MASK                                          (0x3 << DAC_DIGCTL_FIR_MODE_SHIFT)
#define     DAC_DIGCTL_FIR_MODE(x)                                            ((x) << DAC_DIGCTL_FIR_MODE_SHIFT)
#define     DAC_DIGCTL_AUMUTE_PA_CTL                                          BIT(25)
#define     DAC_DIGCTL_SDM_DITH_EN                                            BIT(24)
#define     DAC_DIGCTL_MULT_DEVICE_SHIFT                                      21
#define     DAC_DIGCTL_MULT_DEVICE_MASK                                       (0x3 << DAC_DIGCTL_MULT_DEVICE_SHIFT)
#define     DAC_DIGCTL_MULT_DEVICE(x)                                         ((x) << DAC_DIGCTL_MULT_DEVICE_SHIFT)
#define     DAC_DIGCTL_AUDIO_128FS_256FS                                      BIT(19)
#define     DAC_DIGCTL_DAF0M2DAEN                                             BIT(17)
#define     DAC_DIGCTL_DADCS                                                  BIT(16)
#define     DAC_DIGCTL_DADEN                                                  BIT(15)
#define     DAC_DIGCTL_DDDEN                                                  BIT(14)
#define     DAC_DIGCTL_AD2DALPENL                                             BIT(12)
#define     DAC_DIGCTL_AD2DALPENR                                             BIT(11)
#define     DAC_DIGCTL_ADC01MIX                                               BIT(10)
#define     DAC_DIGCTL_DACHNUM                                                BIT(8 )
#define     DAC_DIGCTL_SR_SHIFT                                               4
#define     DAC_DIGCTL_SR_MASK                                                (0xF << DAC_DIGCTL_SR_SHIFT)
#define     DAC_DIGCTL_SR(x)                                                  ((x) << DAC_DIGCTL_SR_SHIFT)
#define     DAC_DIGCTL_CIC_RATE                                               BIT(3)
#define     DAC_DIGCTL_ENDITH                                                 BIT(2)
#define     DAC_DIGCTL_DDEN_R                                                 BIT(1)
#define     DAC_DIGCTL_DDEN_L                                                 BIT(0)

#define     DAC_FIFOCTL_DRQ0_LEVEL_SHIFT                                      12
#define     DAC_FIFOCTL_DRQ0_LEVEL_MASK                                       (0xF << DAC_FIFOCTL_DRQ0_LEVEL_SHIFT)
#define     DAC_FIFOCTL_DRQ0_LEVEL(x)                                         ((x) << DAC_FIFOCTL_DRQ0_LEVEL_SHIFT)
#define     DAC_FIFOCTL_DACFIFO0_DMAWIDTH                                     BIT(7)
#define     DAC_FIFOCTL_DAF0IS_SHIFT                                          4
#define     DAC_FIFOCTL_DAF0IS_MASK                                           (0x3<<DAC_FIFOCTL_DAF0IS_SHIFT)
#define     DAC_FIFOCTL_DAF0IS(x)                                             ((x) << DAC_FIFOCTL_DAF0IS_SHIFT)
#define     DAC_FIFOCTL_DAF0EIE                                               BIT(2)
#define     DAC_FIFOCTL_DAF0EDE                                               BIT(1)
#define     DAC_FIFOCTL_DAF0RT                                                BIT(0)

#define     DAC_STAT_FIFO0_ER                                                 BIT(8)
#define     DAC_STAT_DAF0EIP                                                  BIT(7)
#define     DAC_STAT_DAF0F                                                    BIT(6)
#define     DAC_STAT_DAF0S_SHIFT                                              0
#define     DAC_STAT_DAF0S_MASK                                               (0x3F<<DAC_STAT_DAF0S_SHIFT)
#define     DAC_STAT_DAF0S(x)                                                 ((x) << DAC_STAT_DAF0S_SHIFT)


#define     DAC_DAT_FIFO0_DAFDAT_SHIFT                                        8
#define     DAC_DAT_FIFO0_DAFDAT_MASK                                         (0xFFFFFF<<DAC_DAT_FIFO0_DAFDAT_SHIFT)
#define     DAC_DAT_FIFO0_DAFDAT(x)                                           ((x) << DAC_DAT_FIFO0_DAFDAT_SHIFT)

#define     PCM_BUF_CTL_PCMBEPIE                                              BIT(7)
#define     PCM_BUF_CTL_PCMBFUIE                                              BIT(6)
#define     PCM_BUF_CTL_PCMBHEIE                                              BIT(5)
#define     PCM_BUF_CTL_PCMBHFIE                                              BIT(4)
#define     PCM_BUF_CTL_IRQ_MASK                                              (0xF << 4)

#define     PCM_BUF_STAT_PCMBEIP                                              BIT(19)
#define     PCM_BUF_STAT_PCMBFIP                                              BIT(18)
#define     PCM_BUF_STAT_PCMBHEIP                                             BIT(17)
#define     PCM_BUF_STAT_PCMBHFIP                                             BIT(16)
#define     PCM_BUF_STAT_IRQ_MASK                                             (0xF << 16)

#define     PCM_BUF_STAT_PCMBS_SHIFT                                          0
#define     PCM_BUF_STAT_PCMBS_MASK                                           (0xFFF<<PCM_BUF_STAT_PCMBS_SHIFT)
#define     PCM_BUF_STAT_PCMBS(x)                                             ((x) << PCM_BUF_STAT_PCMBS_SHIFT)
#define     PCM_BUF_THRES_HE_THRESHOLD_SHIFT                                  0
#define     PCM_BUF_THRES_HE_THRESHOLD_MASK                                   (0xFFF<<PCM_BUF_THRES_HE_THRESHOLD_SHIFT)
#define     PCM_BUF_THRES_HE_THRESHOLD(x)                                     ((x) << PCM_BUF_THRES_HE_THRESHOLD_SHIFT)
#define     PCM_BUF_THRES_HF_THRESHOLD_SHIFT                                  0
#define     PCM_BUF_THRES_HF_THRESHOLD_MASK                                   (0xFFF<<PCM_BUF_THRES_HF_THRESHOLD_SHIFT)
#define     PCM_BUF_THRES_HF_THRESHOLD(x)                                     ((x) << PCM_BUF_THRES_HF_THRESHOLD_SHIFT)


#define     SDM_RESET_CTL_SDMCNT_SHIFT                                        16
#define     SDM_RESET_CTL_SDMCNT_MASK                                         (0xFFFF<<SDM_RESET_CTL_SDMCNT_SHIFT)
#define     SDM_RESET_CTL_SDMCNT(x)                                           ((x) << SDM_RESET_CTL_SDMCNT_SHIFT)
#define     SDM_RESET_CTL_SDMNDTH_SHIFT                                       4
#define     SDM_RESET_CTL_SDMNDTH_MASK                                        (0xFFF<<SDM_RESET_CTL_SDMNDTH_SHIFT)
#define     SDM_RESET_CTL_SDMNDTH(x)                                          ((x) << SDM_RESET_CTL_SDMNDTH_SHIFT)
#define     SDM_RESET_CTL_SDMRDS_R                                            BIT(2)
#define     SDM_RESET_CTL_SDMRDS_L                                            BIT(1)
#define     SDM_RESET_CTL_SDMREEN                                             BIT(0)


#define     AUTO_MUTE_CTL_AMCNT_SHIFT                                         16
#define     AUTO_MUTE_CTL_AMCNT_MASK                                          (0xFFFF<<AUTO_MUTE_CTL_AMCNT_SHIFT)
#define     AUTO_MUTE_CTL_AMCNT(x)                                            ((x) << AUTO_MUTE_CTL_AMCNT_SHIFT)
#define     AUTO_MUTE_CTL_AMTH_SHIFT                                          4
#define     AUTO_MUTE_CTL_AMTH_MASK                                           (0xFFF<<AUTO_MUTE_CTL_AMTH_SHIFT)
#define     AUTO_MUTE_CTL_AMTH(x)                                            ((x) << AUTO_MUTE_CTL_AMTH_SHIFT)
#define     AUTO_MUTE_CTL_AMPD_OUT                                            BIT(3)
#define     AUTO_MUTE_CTL_AMPD_IN                                             BIT(2)
#define     AUTO_MUTE_CTL_AM_IRQ_EN                                           BIT(1)
#define     AUTO_MUTE_CTL_AMEN                                                BIT(0)

#define     VOL_LCH_DONE_PD                                                   BIT(22)
#define     VOL_LCH_VOLL_IRQ_EN                                               BIT(21)
#define     VOL_LCH_TO_CNT                                                    BIT(20)
#define     VOL_LCH_ADJ_CNT_SHIFT                                             12
#define     VOL_LCH_ADJ_CNT_MASK                                              (0xFF<<VOL_LCH_ADJ_CNT_SHIFT)
#define     VOL_LCH_ADJ_CNT(x)                                                ((x) << VOL_LCH_ADJ_CNT_SHIFT)
#define     VOL_LCH_DONE_STA                                                  BIT(11)
#define     VOL_LCH_SOFT_STEP_EN                                              BIT(10)
#define     VOL_LCH_VOLLZCTOEN                                                BIT(9 )
#define     VOL_LCH_VOLLZCEN                                                  BIT(8 )
#define     VOL_LCH_VOLL_SHIFT                                                0
#define     VOL_LCH_VOLL_MASK                                                 (0xFF << VOL_LCH_VOLL_SHIFT)
#define     VOL_LCH_VOLL(x)                                                   ((x) << VOL_LCH_VOLL_SHIFT)

#define     VOL_RCH_DONE_PD                                                   BIT(22)
#define     VOL_RCH_VOLR_IRQ_EN                                               BIT(21)
#define     VOL_RCH_TO_CNT                                                    BIT(20)

#define     VOL_RCH_ADJ_CNT_SHIFT                                             12
#define     VOL_RCH_ADJ_CNT_MASK                                              (0xFF<<VOL_RCH_ADJ_CNT_SHIFT)
#define     VOL_RCH_ADJ_CNT(x)                                                ((x) << VOL_RCH_ADJ_CNT_SHIFT)

#define     VOL_RCH_DONE_STA                                                  BIT(11)
#define     VOL_RCH_SOFT_STEP_EN                                              BIT(10)
#define     VOL_RCH_VOLRZCTOEN                                                BIT(9 )
#define     VOL_RCH_VOLRZCEN                                                  BIT(8 )

#define     VOL_RCH_VOLR_SHIFT                                                0
#define     VOL_RCH_VOLR_MASK                                                 (0xFF<<VOL_RCH_VOLR_SHIFT)
#define     VOL_RCH_VOLR(x)                                                   ((x) << VOL_RCH_VOLR_SHIFT)

#define     PCM_BUF_CNT_IP                                                    BIT(18)
#define     PCM_BUF_CNT_IE                                                    BIT(17)
#define     PCM_BUF_CNT_EN                                                    BIT(16)

#define     PCM_BUF_CNT_CNT_SHIFT                                             0
#define     PCM_BUF_CNT_CNT_MASK                                              (0xFFFF<<0)
#define     VOL_RCH_VOLR(x)                                                   ((x) << VOL_RCH_VOLR_SHIFT)

#define     DAC_ANACTL0_OVDTOUT                                               BIT(23)
#define     DAC_ANACTL0_OVCSELH_e                                             BIT(18)
#define     DAC_ANACTL0_OVCSELH_SHIFT                                         17
#define     DAC_ANACTL0_OVCSELH_MASK                                          (0x3<<17)
#define     VOL_RCH_VOLR(x)                                                   ((x) << VOL_RCH_VOLR_SHIFT)

#define     DAC_ANACTL0_OVDTEN                                                BIT(16)
#define     DAC_ANACTL0_SEL_FBCAP                                             BIT(13)
#define     DAC_ANACTL0_DFCEN                                                 BIT(12)

#define     DAC_ANACTL0_PAVOL_SHIFT                                           9
#define     DAC_ANACTL0_PAVOL_MASK                                            (0x7<<DAC_ANACTL0_PAVOL_SHIFT)
#define     DAC_ANACTL0_PAVOL(x)                                              ((x) << DAC_ANACTL0_PAVOL_SHIFT)

#define     DAC_ANACTL0_DARSEL                                                BIT(8)
#define     DAC_ANACTL0_PAROSEN                                               BIT(7)
#define     DAC_ANACTL0_PAREN                                                 BIT(6)
#define     DAC_ANACTL0_PALOSEN                                               BIT(5)
#define     DAC_ANACTL0_PALEN                                                 BIT(4)
#define     DAC_ANACTL0_ZERODT                                                BIT(3)
#define     DAC_ANACTL0_DAENR                                                 BIT(2)
#define     DAC_ANACTL0_DAENL                                                 BIT(1)
#define     DAC_ANACTL0_BIASEN                                                BIT(0)

#define     DAC_ANACTL1_RAMDDGB                                               BIT(26)
#define     DAC_ANACTL1_RAMPDSTEP_SHIFT                                       24
#define     DAC_ANACTL1_RAMPDSTEP_MASK                                        (0x3<<DAC_ANACTL1_RAMPDSTEP_SHIFT)
#define     DAC_ANACTL1_RAMPDSTEP(x)                                          ((x) << DAC_ANACTL1_RAMPDSTEP_SHIFT)

#define     DAC_ANACTL1_RAMPDINI                                              BIT(23)

#define     DAC_ANACTL1_RAMPVOL_SHIFT                                         20
#define     DAC_ANACTL1_RAMPVOL_MASK                                          (0x7 << DAC_ANACTL1_RAMPVOL_SHIFT)
#define     DAC_ANACTL1_RAMPVOL(x)                                            ((x) << DAC_ANACTL1_RAMPVOL_SHIFT)


#define     DAC_ANACTL1_RAMPCLKSEL_SHIFT                                      18
#define     DAC_ANACTL1_RAMPCLKSEL_MASK                                       (0x3<<DAC_ANACTL1_RAMPCLKSEL_SHIFT)
#define     DAC_ANACTL1_RAMPCLKSEL(x)                                         ((x) << DAC_ANACTL1_RAMPCLKSEL_SHIFT)

#define     DAC_ANACTL1_RAMPOPEN                                              BIT(17)
#define     DAC_ANACTL1_RAMPDEN                                               BIT(16)
#define     DAC_ANACTL1_ATPPD_R                                               BIT(14)
#define     DAC_ANACTL1_ATPSW2_R                                              BIT(13)
#define     DAC_ANACTL1_ATPSW1_R                                              BIT(12)
#define     DAC_ANACTL1_BCDISCH_R                                             BIT(11)
#define     DAC_ANACTL1_ATPRC2EN_R                                            BIT(10)
#define     DAC_ANACTL1_ATPRCEN_R                                             BIT(9 )
#define     DAC_ANACTL1_LP2EN_R                                               BIT(8 )
#define     DAC_ANACTL1_ATPPD_L                                               BIT(6 )
#define     DAC_ANACTL1_ATPSW2_L                                              BIT(5 )
#define     DAC_ANACTL1_ATPSW1_L                                              BIT(4 )
#define     DAC_ANACTL1_BCDISCH_L                                             BIT(3 )
#define     DAC_ANACTL1_ATPRC2EN_L                                            BIT(2 )
#define     DAC_ANACTL1_ATPRCEN_L                                             BIT(1 )
#define     DAC_ANACTL1_LP2EN_L                                               BIT(0 )

#define     DAC_ANACTL2_SHDBGEN                                               BIT(20)

#define     DAC_ANACTL2_SHCL_SET_SHIFT                                        12
#define     DAC_ANACTL2_SHCL_SET_MASK                                         (0xFF<<DAC_ANACTL2_SHCL_SET_SHIFT)
#define     DAC_ANACTL2_SHCL_SET(x)                                           ((x) << DAC_ANACTL2_SHCL_SET_SHIFT)

#define     DAC_ANACTL2_SHCL_PW_SHIFT                                         4
#define     DAC_ANACTL2_SHCL_PW_MASK                                          (0xFF<<DAC_ANACTL2_SHCL_PW_SHIFT)
#define     DAC_ANACTL2_SHCL_PW(x)                                            ((x) << DAC_ANACTL2_SHCL_PW_SHIFT)

#define     DAC_ANACTL2_SHCL_SEL_SHIFT                                        2
#define     DAC_ANACTL2_SHCL_SEL_MASK                                         (0x3<<DAC_ANACTL2_SHCL_SEL_SHIFT)
#define     DAC_ANACTL2_SHCL_SEL(x)                                           ((x) << DAC_ANACTL2_SHCL_SEL_SHIFT)

#define     DAC_ANACTL2_OPCLAMP_EN                                            BIT(1)
#define     DAC_ANACTL2_SH_CLKEN                                              BIT(0)

#define     DAC_BIAS_IDLY_SHIFT                                               22
#define     DAC_BIAS_IDLY_MASK                                                (0x3<<DAC_BIAS_IDLY_SHIFT)
#define     DAC_BIAS_IDLY(x)                                                  ((x) << DAC_BIAS_IDLY_SHIFT)

#define     DAC_BIAS_BIASCTRLH_SHIFT                                          20
#define     DAC_BIAS_BIASCTRLH_MASK                                           (0x3<<DAC_BIAS_BIASCTRLH_SHIFT)
#define     DAC_BIAS_BIASCTRLH(x)                                             ((x) << DAC_BIAS_BIASCTRLH_SHIFT)

#define     DAC_BIAS_PAIQS_SHIFT                                              16
#define     DAC_BIAS_PAIQS_MASK                                               (0x7<<DAC_BIAS_PAIQS_SHIFT)
#define     DAC_BIAS_PAIQS(x)                                                 ((x) << DAC_BIAS_PAIQS_SHIFT)

#define     DAC_BIAS_IPA_SHIFT                                                12
#define     DAC_BIAS_IPA_MASK                                                 (0x7<<DAC_BIAS_IPA_SHIFT)
#define     DAC_BIAS_IPA(x)                                                   ((x) << DAC_BIAS_IPA_SHIFT)

#define     DAC_BIAS_IOPRAMP_SHIFT                                            10
#define     DAC_BIAS_IOPRAMP_MASK                                             (0x3<<DAC_BIAS_IOPRAMP_SHIFT)
#define     DAC_BIAS_IOPRAMP(x)                                               ((x) << DAC_BIAS_IOPRAMP_SHIFT)

#define     DAC_BIAS_ITC_SHIFT                                                8
#define     DAC_BIAS_ITC_MASK                                                 (0x3 << DAC_BIAS_ITC_SHIFT)
#define     DAC_BIAS_ITC(x)                                                   ((x) << DAC_BIAS_ITC_SHIFT)

#define     DAC_BIAS_IOPCLAMPN_SHIFT                                          6
#define     DAC_BIAS_IOPCLAMPN_MASK                                           (0x3 << DAC_BIAS_IOPCLAMPN_SHIFT)
#define     DAC_BIAS_IOPCLAMPN(x)                                                   ((x) << DAC_BIAS_IOPCLAMPN_SHIFT)

#define     DAC_BIAS_IOPCLAMPP_SHIFT                                          4
#define     DAC_BIAS_IOPCLAMPP_MASK                                           (0x3<<DAC_BIAS_IOPCLAMPP_SHIFT)
#define     DAC_BIAS_IOPCLAMPP(x)                                             ((x) << DAC_BIAS_IOPCLAMPP_SHIFT)

#define     DAC_BIAS_IOPCM1_SHIFT                                             2
#define     DAC_BIAS_IOPCM1_MASK                                              (0x3 << DAC_BIAS_IOPCM1_SHIFT)
#define     DAC_BIAS_IOPCM1(x)                                                ((x) << DAC_BIAS_IOPCM1_SHIFT)

#define     DAC_BIAS_IOPVB_SHIFT                                              0
#define     DAC_BIAS_IOPVB_MASK                                               (0x3<<DAC_BIAS_IOPVB_SHIFT)
#define     DAC_BIAS_IOPVB(x)                                                 ((x) << DAC_BIAS_IOPVB_SHIFT)

#define     SDM_SAMPLES_CNT_IP                                                BIT(30)
#define     SDM_SAMPLES_CNT_IE                                                BIT(29)
#define     SDM_SAMPLES_CNT_EN                                                BIT(28)

#define     SDM_SAMPLES_CNT_CNT_SHIFT                                         0
#define     SDM_SAMPLES_CNT_CNT_MASK                                          (0xFFFFFFF << SDM_SAMPLES_CNT_CNT_SHIFT)
#define     SDM_SAMPLES_CNT_CNT(x)                                            ((x) << SDM_SAMPLES_CNT_CNT_SHIFT)

#define     SDM_SAMPLES_NUM_CNT_SHIFT                                         0
#define     SDM_SAMPLES_NUM_CNT_MASK                                          (0xFFFFFFF<<SDM_SAMPLES_NUM_CNT_SHIFT)
#define     SDM_SAMPLES_NUM_CNT(x)                                            ((x) << SDM_SAMPLES_NUM_CNT_SHIFT)

#define     HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_R                                BIT(8)
#define     HW_TRIGGER_DAC_CTL_INT_TO_SDMCNT_EN                               BIT(7)
#define     HW_TRIGGER_DAC_CTL_INT_TO_DACFIFO_EN                              BIT(6)
#define     HW_TRIGGER_DAC_CTL_INT_TO_SDM_CNT                                 BIT(5)
#define     HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN_L                                BIT(4)

#define     HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SEL_SHIFT                          0
#define     HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SEL_MASK                           (0xF<<HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SEL_SHIFT)
#define     HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SEL(x)                             ((x) << HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SEL_SHIFT)

#define     DC_REF_DAT_DC_DAT_SHIFT                                           8
#define     DC_REF_DAT_DC_DAT_MASK                                            (0xFFFFFF << DC_REF_DAT_DC_DAT_SHIFT)
#define     DC_REF_DAT_DC_DAT(x)                                              ((x) << DC_REF_DAT_DC_DAT_SHIFT)

#define     DAC_OSCTL_DAOS_R_SHIFT                                            8
#define     DAC_OSCTL_DAOS_R_MASK                                             (0xFF << DAC_OSCTL_DAOS_R_SHIFT)
#define     DAC_OSCTL_DAOS_R(x)                                               (x) << DAC_OSCTL_DAOS_R_SHIFT

#define     DAC_OSCTL_DAOS_L_SHIFT                                            0
#define     DAC_OSCTL_DAOS_L_MASK                                             (0xFF<<DAC_OSCTL_DAOS_L_SHIFT)
#define     DAC_OSCTL_DAOS_L(x)                                               (x) << DAC_OSCTL_DAOS_L_SHIFT


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

//-------------------------------I2STX0 Bits Location------------------------------------------//
#define     I2STX0_CTL_TXD_DELAY_SHIFT                                        21
#define     I2STX0_CTL_TXD_DELAY_MASK                                         (0x3 << I2STX0_CTL_TXD_DELAY_SHIFT)
#define     I2STX0_CTL_TXD_DELAY(x)                                           ((x) << I2STX0_CTL_TXD_DELAY_SHIFT)
#define     I2STX0_CTL_TDMTX_CHAN                                             BIT(20)
#define     I2STX0_CTL_TDMTX_MODE                                             BIT(19)

#define     I2STX0_CTL_TDMTX_SYNC_SHIFT                                       17
#define     I2STX0_CTL_TDMTX_SYNC_MASK                                        (0x3 << I2STX0_CTL_TDMTX_SYNC_SHIFT)
#define     I2STX0_CTL_TDMTX_SYNC(x)                                          ((x) << I2STX0_CTL_TDMTX_SYNC_SHIFT)
#define     I2STX0_CTL_MULT_DEVICE                                            BIT(16)
#define     I2STX0_CTL_LPEN0                                                  BIT(8 )

#define     I2STX0_CTL_TXWIDTH_SHIFT                                          4
#define     I2STX0_CTL_TXWIDTH_MASK                                           (0x3 << I2STX0_CTL_TXWIDTH_SHIFT)
#define     I2STX0_CTL_TXWIDTH(x)                                             ((x) << I2STX0_CTL_TXWIDTH_SHIFT)
#define     I2STX0_CTL_TXBCLKSET                                              BIT(3)

#define     I2STX0_CTL_TXMODELSEL_SHIFT                                       1
#define     I2STX0_CTL_TXMODELSEL_MASK                                        (0x3 << I2STX0_CTL_TXMODELSEL_SHIFT)
#define     I2STX0_CTL_TXMODELSEL(x)                                          ((x) << I2STX0_CTL_TXMODELSEL_SHIFT)
#define     I2STX0_CTL_TXEN                                                   BIT(0)

#define     I2STX0_FIFOCTL_FIFO0_VOL_SHIFT                                    8
#define     I2STX0_FIFOCTL_FIFO0_VOL_MASK                                     (0xF << I2STX0_FIFOCTL_FIFO0_VOL_SHIFT)
#define     I2STX0_FIFOCTL_FIFO0_VOL(x)                                       ((x) << I2STX0_FIFOCTL_FIFO0_VOL_SHIFT)
#define     I2STX0_FIFOCTL_TXFIFO_DMAWIDTH                                    BIT(7)

#define     I2STX0_FIFOCTL_FIFO_IS_SHIFT                                      4
#define     I2STX0_FIFOCTL_FIFO_IS_MASK                                       (0x3 << I2STX0_FIFOCTL_FIFO_IS_SHIFT)
#define     I2STX0_FIFOCTL_FIFO_IS(x)                                         ((x) << I2STX0_FIFOCTL_FIFO_IS_SHIFT)
#define     I2STX0_FIFOCTL_FIFO_SEL                                           BIT(3)
#define     I2STX0_FIFOCTL_FIFO_IEN                                           BIT(2)
#define     I2STX0_FIFOCTL_FIFO_DEN                                           BIT(1)
#define     I2STX0_FIFOCTL_FIFO_RST                                           BIT(0)

#define     I2STX0_FIFOSTA_TFEP                                               BIT(8)
#define     I2STX0_FIFOSTA_IP                                                 BIT(7)
#define     I2STX0_FIFOSTA_TFFU                                               BIT(6)

#define     I2STX0_FIFOSTA_STA_SHIFT                                          0
#define     I2STX0_FIFOSTA_STA_MASK                                           (0x3F << I2STX0_FIFOSTA_STA_SHIFT)
#define     I2STX0_FIFOSTA_STA(x)                                             ((x) << I2STX0_FIFOSTA_STA_SHIFT)


#define     I2STX0_DAT_DAT_SHIFT                                              8
#define     I2STX0_DAT_DAT_MASK                                               (0xFFFFFF << I2STX0_DAT_DAT_SHIFT)
#define     I2STX0_DAT_DAT(x)                                                 ((x) << I2STX0_DAT_DAT_SHIFT)

#define     I2STX0_SRDCTL_MUTE_EN                                             BIT(12)
#define     I2STX0_SRDCTL_SRD_IE                                              BIT(8 )

#define     I2STX0_SRDCTL_CNT_TIM_SHIFT                                       4
#define     I2STX0_SRDCTL_CNT_TIM_MASK                                        (0x3 << I2STX0_SRDCTL_CNT_TIM_SHIFT)
#define     I2STX0_SRDCTL_CNT_TIM(x)                                          ((x) << I2STX0_SRDCTL_CNT_TIM_SHIFT)

#define     I2STX0_SRDCTL_SRD_TH_SHIFT                                        1
#define     I2STX0_SRDCTL_SRD_TH_MASK                                         (0x7 << I2STX0_SRDCTL_SRD_TH_SHIFT)
#define     I2STX0_SRDCTL_SRD_TH(x)                                           ((x) << I2STX0_SRDCTL_SRD_TH_SHIFT)
#define     I2STX0_SRDCTL_SRD_EN                                              BIT(0)

#define     I2STX0_SRDSTA_CNT_SHIFT                                           12
#define     I2STX0_SRDSTA_CNT_MASK                                            (0x1FFF << I2STX0_SRDSTA_CNT_SHIFT)
#define     I2STX0_SRDSTA_CNT(x)                                              ((x) << I2STX0_SRDSTA_CNT_SHIFT)
#define     I2STX0_SRDSTA_TO_PD                                               BIT(11)
#define     I2STX0_SRDSTA_SRC_PD                                              BIT(10)
#define     I2STX0_SRDSTA_CHW_PD                                              BIT(8 )

#define     I2STX0_SRDSTA_WL_SHIFT                                            0
#define     I2STX0_SRDSTA_WL_MASK                                             (0x7 << I2STX0_SRDSTA_WL_SHIFT)
#define     I2STX0_SRDSTA_WL(x)                                               ((x) << I2STX0_SRDSTA_WL_SHIFT)

#define     I2STX0_FIFO_CNT_IP                                                BIT(18)
#define     I2STX0_FIFO_CNT_IE                                                BIT(17)
#define     I2STX0_FIFO_CNT_EN                                                BIT(16)

#define     I2STX0_FIFO_CNT_CNT_SHIFT                                         0
#define     I2STX0_FIFO_CNT_CNT_MASK                                          (0xFFFF << I2STX0_FIFO_CNT_CNT_SHIFT)
#define     I2STX0_FIFO_CNT_CNT(x)                                            ((x) << I2STX0_FIFO_CNT_CNT_SHIFT)

//-------------------------I2SRX0 Bits Location------------------------------------------//
#define     I2SRX0_CTL_TDMRX_CHAN                                             BIT(16)
#define     I2SRX0_CTL_TDMRX_MODE                                             BIT(15)

#define     I2SRX0_CTL_TDMRX_SYNC_SHIFT                                       13
#define     I2SRX0_CTL_TDMRX_SYNC_MASK                                        (0x3 << I2SRX0_CTL_TDMRX_SYNC_SHIFT)
#define     I2SRX0_CTL_TDMRX_SYNC(x)                                          ((x) << I2SRX0_CTL_TDMRX_SYNC_SHIFT)

#define     I2SRX0_CTL_RXWIDTH_SHIFT                                          4
#define     I2SRX0_CTL_RXWIDTH_MASK                                           (0x3 << I2SRX0_CTL_RXWIDTH_SHIFT)
#define     I2SRX0_CTL_RXWIDTH(x)                                             ((x) << I2SRX0_CTL_RXWIDTH_SHIFT)
#define     I2SRX0_CTL_RXBCLKSET                                              BIT(3)

#define     I2SRX0_CTL_RXMODELSEL_SHIFT                                       1
#define     I2SRX0_CTL_RXMODELSEL_MASK                                        (0x3 << I2SRX0_CTL_RXMODELSEL_SHIFT)
#define     I2SRX0_CTL_RXMODELSEL(x)                                          ((x) << I2SRX0_CTL_RXMODELSEL_SHIFT)
#define     I2SRX0_CTL_RXEN                                                   BIT(0)

#define     I2SRX0_FIFOCTL_RXFIFO_DMAWIDTH                                    BIT(7)

#define     I2SRX0_FIFOCTL_RXFOS_SHIFT                                        4
#define     I2SRX0_FIFOCTL_RXFOS_MASK                                         (0x3 << I2SRX0_FIFOCTL_RXFOS_SHIFT)
#define     I2SRX0_FIFOCTL_RXFOS(x)                                           ((x) << I2SRX0_FIFOCTL_RXFOS_SHIFT)
#define     I2SRX0_FIFOCTL_RXFFIE                                             BIT(2)
#define     I2SRX0_FIFOCTL_RXFFDE                                             BIT(1)
#define     I2SRX0_FIFOCTL_RXFRT                                              BIT(0)

#define     I2SRX0_FIFOSTA_RFEP                                               BIT(8)
#define     I2SRX0_FIFOSTA_RXFEF                                              BIT(7)
#define     I2SRX0_FIFOSTA_RXFIP                                              BIT(6)

#define     I2SRX0_FIFOSTA_RXFS_SHIFT                                         0
#define     I2SRX0_FIFOSTA_RXFS_MASK                                          (0x3F << I2SRX0_FIFOSTA_RXFS_SHIFT)
#define     I2SRX0_FIFOSTA_RXFS(x)                                            ((x) << I2SRX0_FIFOSTA_RXFS_SHIFT)

#define     I2SRX0_DAT_RXDAT_SHIFT                                            8
#define     I2SRX0_DAT_RXDAT_MASK                                             (0xFFFFFF << I2SRX0_DAT_RXDAT_SHIFT)
#define     I2SRX0_DAT_RXDAT(x)                                               ((x) << I2SRX0_DAT_RXDAT_SHIFT)

#define     I2SRX0_SRDCTL_MUTE_EN                                             BIT(12)
#define     I2SRX0_SRDCTL_SRD_IE                                              BIT(8 )

#define     I2SRX0_SRDCTL_CNT_TIM_SHIFT                                       4
#define     I2SRX0_SRDCTL_CNT_TIM_MASK                                        (0x3 << I2SRX0_SRDCTL_CNT_TIM_SHIFT)
#define     I2SRX0_SRDCTL_CNT_TIM(x)                                          ((x) << I2SRX0_SRDCTL_CNT_TIM_SHIFT)

#define     I2SRX0_SRDCTL_SRD_TH_SHIFT                                        1
#define     I2SRX0_SRDCTL_SRD_TH_MASK                                         (0x7 << I2SRX0_SRDCTL_SRD_TH_SHIFT)
#define     I2SRX0_SRDCTL_SRD_TH(x)                                           ((x) << I2SRX0_SRDCTL_SRD_TH_SHIFT)
#define     I2SRX0_SRDCTL_SRD_EN                                              BIT(0)

#define     I2SRX0_SRDSTA_CNT_SHIFT                                           12
#define     I2SRX0_SRDSTA_CNT_MASK                                            (0x1FFF << I2SRX0_SRDSTA_CNT_SHIFT)
#define     I2SRX0_SRDSTA_CNT(x)                                              ((x) << I2SRX0_SRDSTA_CNT_SHIFT)
#define     I2SRX0_SRDSTA_TO_PD                                               BIT(11)
#define     I2SRX0_SRDSTA_SRC_PD                                              BIT(10)
#define     I2SRX0_SRDSTA_CHW_PD                                              BIT(8 )

#define     I2SRX0_SRDSTA_WL_SHIFT                                            0
#define     I2SRX0_SRDSTA_WL_MASK                                             (0x7 << I2SRX0_SRDSTA_WL_SHIFT)
#define     I2SRX0_SRDSTA_WL(x)                                               ((x) << I2SRX0_SRDSTA_WL_SHIFT)

//-------------------------------ADC/DAC/I2S CLK--------------------------------------------------//
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

#define     CMU_DACCLK_DACFIFO0CLKEN                                          BIT(24)
#define     CMU_DACCLK_DACCICFIRCLKEN                                         BIT(21)
#define     CMU_DACCLK_DACSDMLCLKEN                                           BIT(20)

#define     CMU_DACCLK_DACFIFCLK2XDIV_SHIFT                                   16
#define     CMU_DACCLK_DACFIFCLK2XDIV_MASK                                    (0x3 << CMU_DACCLK_DACFIFCLK2XDIV_SHIFT)
#define     CMU_DACCLK_DACFIFCLK2XDIV(x)                                      ((x) << CMU_DACCLK_DACFIFCLK2XDIV_SHIFT)

#define     CMU_DACCLK_DACFIRCLKDIV_SHIFT                                     14
#define     CMU_DACCLK_DACCICCLKDIV_SHIFT                                     12
#define     CMU_DACCLK_DACCLKSRC_SHIFT                                        4

#define     CMU_DACCLK_DACCLKDIV_SHIFT                                        0
#define     CMU_DACCLK_DACCLKDIV_MASK                                         (0x3 << CMU_DACCLK_DACCLKDIV_SHIFT)
#define     CMU_DACCLK_DACCLKDIV(x)                                           ((x) << CMU_DACCLK_DACCLKDIV_SHIFT)

#define     CMU_I2STXCLK_I2STXBLRCLKOEN                                       BIT(30)
#define     CMU_I2STXCLK_I2STXMCLKOEN                                         BIT(29)
#define     CMU_I2STXCLK_I2SG0LRCLKPROC                                       BIT(23)

#define     CMU_I2STXCLK_I2SG0LRCLKDIV_SHIFT                                  20
#define     CMU_I2STXCLK_I2SG0LRCLKDIV_MASK                                   (0x3 << CMU_I2STXCLK_I2SG0LRCLKDIV_SHIFT)
#define     CMU_I2STXCLK_I2SG0LRCLKDIV(x)                                     ((x) << CMU_I2STXCLK_I2SG0LRCLKDIV_SHIFT)

#define     CMU_I2STXCLK_I2SG0BCLKDIV_SHIFT                                   18
#define     CMU_I2STXCLK_I2SG0BCLKDIV_MASK                                    (0x3 << CMU_I2STXCLK_I2SG0BCLKDIV_SHIFT)
#define     CMU_I2STXCLK_I2SG0BCLKDIV(x)                                      ((x) << CMU_I2STXCLK_I2SG0BCLKDIV_SHIFT)

#define     CMU_I2STXCLK_I2SG0BLRCLKSRC                                       BIT(16)
#define     CMU_I2STXCLK_I2SG0MCLKEXTREV                                      BIT(14)

#define     CMU_I2STXCLK_I2SG0MCLKSRC_SHIFT                                   12
#define     CMU_I2STXCLK_I2SG0MCLKSRC_MASK                                    (0x3 << CMU_I2STXCLK_I2SG0MCLKSRC_SHIFT)
#define     CMU_I2STXCLK_I2SG0MCLKSRC(x)                                      ((x) << CMU_I2STXCLK_I2SG0MCLKSRC_SHIFT)

#define     CMU_I2STXCLK_I2SG0CLKDIV_SHIFT                                    0
#define     CMU_I2STXCLK_I2SG0CLKDIV_MASK                                     (0xF << CMU_I2STXCLK_I2SG0CLKDIV_SHIFT)
#define     CMU_I2STXCLK_I2SG0CLKDIV(x)                                       ((x) << CMU_I2STXCLK_I2SG0CLKDIV_SHIFT)

#define     CMU_I2SRXCLK_I2SRXBLRCLKOEN                                       BIT(30)
#define     CMU_I2SRXCLK_I2SRXMCLKOEN                                         BIT(29)
#define     CMU_I2SRXCLK_I2SRXCLKSRC                                          BIT(28)
#define     CMU_I2SRXCLK_I2SG1LRCLKPROC                                       BIT(23)

#define     CMU_I2SRXCLK_I2SG1LRCLKDIV_SHIFT                                  20
#define     CMU_I2SRXCLK_I2SG1LRCLKDIV_MASK                                   (0x3 << CMU_I2SRXCLK_I2SG1LRCLKDIV_SHIFT)
#define     CMU_I2SRXCLK_I2SG1LRCLKDIV(x)                                     ((x) << CMU_I2SRXCLK_I2SG1LRCLKDIV_SHIFT)

#define     CMU_I2SRXCLK_I2SG1BCLKDIV_SHIFT                                   18
#define     CMU_I2SRXCLK_I2SG1BCLKDIV_MASK                                    (0x3 << CMU_I2SRXCLK_I2SG1BCLKDIV_SHIFT)
#define     CMU_I2SRXCLK_I2SG1BCLKDIV(x)                                      ((x) << CMU_I2SRXCLK_I2SG1BCLKDIV_SHIFT)

#define     CMU_I2SRXCLK_I2SG1BLRCLKSRC                                       BIT(16)
#define     CMU_I2SRXCLK_I2SG1MCLKEXTREV                                      BIT(14)

#define     CMU_I2SRXCLK_I2SG1MCLKSRC_SHIFT                                   12
#define     CMU_I2SRXCLK_I2SG1MCLKSRC_MASK                                    (0x3 << CMU_I2SRXCLK_I2SG1MCLKSRC_SHIFT)
#define     CMU_I2SRXCLK_I2SG1MCLKSRC(x)                                      ((x) << CMU_I2SRXCLK_I2SG1MCLKSRC_SHIFT)

#define     CMU_I2SRXCLK_I2SG1CLKDIV_SHIFT                                    0
#define     CMU_I2SRXCLK_I2SG1CLKDIV_MASK                                     (0xF << CMU_I2SRXCLK_I2SG1CLKDIV_SHIFT)
#define     CMU_I2SRXCLK_I2SG1CLKDIV(x)                                       ((x) << CMU_I2SRXCLK_I2SG1CLKDIV_SHIFT)

#endif /* __PHY_AUDIO_H__ */
