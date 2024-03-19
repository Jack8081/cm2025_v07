/********************************************************************************
 *        Copyright(c) 2017-2027 Actions (Zhuhai) Technology Co., Limited,
 *                            All Rights Reserved.
 *
 * Description: audio driver module external interface header file
 * Author: FGR
 * Date: February 10, 2017
 ********************************************************************************/

#ifndef __AUDIO_ADC_H__
#define __AUDIO_ADC_H__

#include "phy_adc/phy_adc.h"


#define   DMADST_DACFIFO0 							0x8b
#define   DMADST_DACFIFO1 							0x8c
#define   DMADST_IISTXFIFO 							0x8e
                              					
#define   DMASRC_ADCFIFO0 							0x8b
#define   DMASRC_IISRXFIFO 							0x8e

#define   DMA_IRQ_DIS								0
#define   DMA_TCIE_EN								0x1
#define   DMA_HFIE_EN								0x10000
                                  			
#define   DMA_BRUST8_interleaves  			        0x0
#define   DMA_BRUST8_separated  				    0x1
#define   DMA_SINGLE_interleaves 				    0x2
#define   DMA_SINGLE_separated					    0x3
#define   DMA_32B									0X0
#define	  DMA_16B									0X1

//--------------Bits Location------------------------------------------//
#define     DMAIP_DMA9HFIP                                                    25
#define     DMAIP_DMA8HFIP                                                    24
#define     DMAIP_DMA7HFIP                                                    23
#define     DMAIP_DMA6HFIP                                                    22
#define     DMAIP_DMA5HFIP                                                    21
#define     DMAIP_DMA4HFIP                                                    20
#define     DMAIP_DMA3HFIP                                                    19
#define     DMAIP_DMA2HFIP                                                    18
#define     DMAIP_DMA1HFIP                                                    17
#define     DMAIP_DMA0HFIP                                                    16
#define     DMAIP_DMA9TCIP                                                    9
#define     DMAIP_DMA8TCIP                                                    8
#define     DMAIP_DMA7TCIP                                                    7
#define     DMAIP_DMA6TCIP                                                    6
#define     DMAIP_DMA5TCIP                                                    5
#define     DMAIP_DMA4TCIP                                                    4
#define     DMAIP_DMA3TCIP                                                    3
#define     DMAIP_DMA2TCIP                                                    2
#define     DMAIP_DMA1TCIP                                                    1
#define     DMAIP_DMA0TCIP                                                    0

#define     DMAIE_DMA9HFIE                                                    25
#define     DMAIE_DMA8HFIE                                                    24
#define     DMAIE_DMA7HFIE                                                    23
#define     DMAIE_DMA6HFIE                                                    22
#define     DMAIE_DMA5HFIE                                                    21
#define     DMAIE_DMA4HFIE                                                    20
#define     DMAIE_DMA3HFIE                                                    19
#define     DMAIE_DMA2HFIE                                                    18
#define     DMAIE_DMA1HFIE                                                    17
#define     DMAIE_DMA0HFIE                                                    16
#define     DMAIE_DMA9TCIE                                                    9
#define     DMAIE_DMA8TCIE                                                    8
#define     DMAIE_DMA7TCIE                                                    7
#define     DMAIE_DMA6TCIE                                                    6
#define     DMAIE_DMA5TCIE                                                    5
#define     DMAIE_DMA4TCIE                                                    4
#define     DMAIE_DMA3TCIE                                                    3
#define     DMAIE_DMA2TCIE                                                    2
#define     DMAIE_DMA1TCIE                                                    1
#define     DMAIE_DMA0TCIE                                                    0

#define     DMADEBUG_DMAAPri_CTL                                              23
#define     DMADEBUG_DEBUGSEL_e                                               5
#define     DMADEBUG_DEBUGSEL_SHIFT                                           0
#define     DMADEBUG_DEBUGSEL_MASK                                            (0x3F<<0)

#define     DMAxCTL_SEL_STR_REG                                               25
#define     DMAxCTL_STRME                                                     24
#define     DMAxCTL_TWS_e                                                     21
#define     DMAxCTL_TWS_SHIFT                                                 20
#define     DMAxCTL_TWS_MASK                                                  (0x3<<20)
#define     DMAxCTL_reload                                                    18
#define     DMAxCTL_TRM                                                       17
#define     DMAxCTL_AUDIOTYPE                                                 16
#define     DMAxCTL_DAM                                                       15
#define     DMAxCTL_DSTSL_e                                                   12
#define     DMAxCTL_DSTSL_SHIFT                                               8
#define     DMAxCTL_DSTSL_MASK                                                (0x1F<<8)
#define     DMAxCTL_SAM                                                       7
#define     DMAxCTL_SRCSL_e                                                   4
#define     DMAxCTL_SRCSL_SHIFT                                               0
#define     DMAxCTL_SRCSL_MASK                                                (0x1F<<0)

#define     DMAxSTART_DMASTART                                                0

#define     DMAxSADDR0_DMASADDR_e                                             31
#define     DMAxSADDR0_DMASADDR_SHIFT                                         0
#define     DMAxSADDR0_DMASADDR_MASK                                          (0xFFFFFFFF<<0)

#define     DMAxSADDR1_DMASADDR_e                                             31
#define     DMAxSADDR1_DMASADDR_SHIFT                                         0
#define     DMAxSADDR1_DMASADDR_MASK                                          (0xFFFFFFFF<<0)

#define     DMAxDADDR0_DMADADDR_e                                             31
#define     DMAxDADDR0_DMADADDR_SHIFT                                         0
#define     DMAxDADDR0_DMADADDR_MASK                                          (0xFFFFFFFF<<0)

#define     DMAxDADDR1_DMADADDR_e                                             31
#define     DMAxDADDR1_DMADADDR_SHIFT                                         0
#define     DMAxDADDR1_DMADADDR_MASK                                          (0xFFFFFFFF<<0)

#define     DMAxBC_DMABYTECOUNTER_e                                           17
#define     DMAxBC_DMABYTECOUNTER_SHIFT                                       0
#define     DMAxBC_DMABYTECOUNTER_MASK                                        (0x3FFFF<<0)

#define     DMAxRC_DMARemainCounterH_e                                        17
#define     DMAxRC_DMARemainCounterH_SHIFT                                    0
#define     DMAxRC_DMARemainCounterH_MASK                                     (0x3FFFF<<0)

#define     DMA0CTL_SEL_STR_REG                                               25
#define     DMA0CTL_STRME                                                     24
#define     DMA0CTL_TWS_e                                                     21
#define     DMA0CTL_TWS_SHIFT                                                 20
#define     DMA0CTL_TWS_MASK                                                  (0x3<<20)
#define     DMA0CTL_reload                                                    18
#define     DMA0CTL_TRM                                                       17
#define     DMA0CTL_AUDIOTYPE                                                 16
#define     DMA0CTL_DAM                                                       15
#define     DMA0CTL_DSTSL_e                                                   12
#define     DMA0CTL_DSTSL_SHIFT                                               8
#define     DMA0CTL_DSTSL_MASK                                                (0x1F<<8)
#define     DMA0CTL_SAM                                                       7
#define     DMA0CTL_SRCSL_e                                                   4
#define     DMA0CTL_SRCSL_SHIFT                                               0
#define     DMA0CTL_SRCSL_MASK                                                (0x1F<<0)

#define     DMA0START_DMASTART                                                0

#define     DMA0SADDR0_DMASADDR_e                                             31
#define     DMA0SADDR0_DMASADDR_SHIFT                                         0
#define     DMA0SADDR0_DMASADDR_MASK                                          (0xFFFFFFFF<<0)

#define     DMA0SADDR1_DMASADDR_e                                             31
#define     DMA0SADDR1_DMASADDR_SHIFT                                         0
#define     DMA0SADDR1_DMASADDR_MASK                                          (0xFFFFFFFF<<0)

#define     DMA0DADDR0_DMADADDR_e                                             31
#define     DMA0DADDR0_DMADADDR_SHIFT                                         0
#define     DMA0DADDR0_DMADADDR_MASK                                          (0xFFFFFFFF<<0)

#define     DMA0DADDR1_DMADADDR_e                                             31
#define     DMA0DADDR1_DMADADDR_SHIFT                                         0
#define     DMA0DADDR1_DMADADDR_MASK                                          (0xFFFFFFFF<<0)

#define     DMA0BC_DMABYTECOUNTER_e                                           17
#define     DMA0BC_DMABYTECOUNTER_SHIFT                                       0
#define     DMA0BC_DMABYTECOUNTER_MASK                                        (0x3FFFF<<0)

#define     DMA0RC_DMARemainCounterH_e                                        17
#define     DMA0RC_DMARemainCounterH_SHIFT                                    0
#define     DMA0RC_DMARemainCounterH_MASK                                     (0x3FFFF<<0)


enum CFG_TYPE_ADC
{
    ADC_NONE    = 0,          // 无
    ADC_0       = (1 << 0),   // ADC0
    ADC_1       = (1 << 1),   // ADC1
};

enum CFG_TYPE_ADC_INPUT
{
    ADC_INPUT_NONE         = 0,           // 无
    ADC_INPUT_0N           = (1 << 0),    // INPUT_0N
    ADC_INPUT_0P           = (1 << 1),    // INPUT_0P
    ADC_INPUT_0NP_DIFF     = (ADC_INPUT_0N | ADC_INPUT_0P),    // INPUT_0NP_DIFF
    ADC_INPUT_1N           = (1 << 2),    // INPUT_1N
    ADC_INPUT_1P           = (1 << 3),    // INPUT_1P
    ADC_INPUT_1NP_DIFF     = (ADC_INPUT_1N | ADC_INPUT_1P),    // INPUT_1NP_DIFF
    ADC_INPUT_DMIC         = (0xFF),        // DMIC
};

enum CFG_TYPE_ADC0_INPUT
{
    ADC0_INPUT_NONE         = ADC_INPUT_NONE,       // 无
    ADC0_INPUT_0N           = ADC_INPUT_0N,         // INPUT_0N
    ADC0_INPUT_0P           = ADC_INPUT_0P,         // INPUT_0P
    ADC0_INPUT_0NP_DIFF     = ADC_INPUT_0NP_DIFF,   // INPUT_0NP_DIFF
    ADC0_INPUT_DMIC         = ADC_INPUT_DMIC,       // DMIC
};

enum CFG_TYPE_ADC1_INPUT
{
    ADC1_INPUT_NONE         = ADC_INPUT_NONE,      // 无
    ADC1_INPUT_1N           = ADC_INPUT_1N,        // INPUT_1N
    ADC1_INPUT_1P           = ADC_INPUT_1P,        // INPUT_1P
    ADC1_INPUT_1NP_DIFF     = ADC_INPUT_1NP_DIFF,  // INPUT_1NP_DIFF
    ADC1_INPUT_DMIC         = ADC_INPUT_DMIC,      // DMIC
};

typedef struct{
	uint32 dma_ch;
	uint32 src_addr0;//if src addr is mem,write it's addr directly;if it is fifo,write fifo type
	uint32 dst_addr0;
	uint32 src_addr1;
	uint32 dst_addr1;
	uint32 transfer_mode;
	uint32 dma_wl;
	uint32 reload;
	uint32 bc;
}cfg_dma,*p_cfg_dma;

//typedef void (*dma_irq_handler_t)(int dma_channel, enum dma_irq_type type, void *pdata);

struct dma
{
    //struct dma_config_t dma_config;
    void                *pdata;
    //dma_irq_handler_t   dma_irq_handler;
    unsigned int        remain_counter;
    int                 special_channel;
};

typedef struct
{
    uint8_t *dma_buf;
    uint32_t dma_buf_len;

    //struct dma* ain_dma;
    //DMACALLBACK ain_dma_cb;
    
    void *reserve;
    
} ain_channel_manager_t;

enum dma_irq_type
{
    DMA_IRQ_TC,
    DMA_IRQ_HF,
};




int32_t ain_open_transfer_channel(ain_track_e channel_id, ain_channel_param_t *channel_param);
int32_t ain_close_transfer_channel(ain_track_e channel_id);
int32_t ain_start_transmission(ain_track_e channel_id, ain_channel_param_t *channel_param);
bool is_dma_transfer_complete(void);
int32_t dma_cfg_start(ain_track_e channel_id, ain_channel_param_t *channel_param);
void dma_stop(int dma_ch);
void adc_dump_register(void);



#endif
