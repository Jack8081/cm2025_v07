/********************************************************************************
 *        Copyright(c) 2014-2016 Actions (Zhuhai) Technology Co., Limited,
 *                            All Rights Reserved.
 *
 * 描述：audio_record.c
 * 作者：zyz
 ********************************************************************************/

#include "att_pattern_test.h"
#include "audio_record.h"
#include "phy_adc/soc_common.h"

#define PLL_FREQ					8*11
#define Wholechip
#define Samples						512

//int audio_rx[Samples*4] = {0};

uint8_t g_use_vmic_flag = 0;

void adc_dump_register(void)
{
    att_buf_printf("** adc contoller regster **\n");
    att_buf_printf("    ADC_DIGCTL: %08x\n", act_read(0x4005c100));
    att_buf_printf("    CH0_DIGCTL: %08x\n", act_read(0x4005c104));
    att_buf_printf("    CH1_DIGCTL: %08x\n", act_read(0x4005c108));
    att_buf_printf("   ADC_FIFOCTL: %08x\n", act_read(0x4005c10c));
    att_buf_printf("      ADC_STAT: %08x\n", act_read(0x4005c110));
    att_buf_printf(" ADC_FIFO0_DAT: %08x\n", act_read(0x4005c114));
    att_buf_printf("HW_TRIGGER_CTL: %08x\n", act_read(0x4005c118));
    att_buf_printf("      ADC_CTL0: %08x\n", act_read(0x4005c11c));
    att_buf_printf("      ADC_CTL1: %08x\n", act_read(0x4005c120));
    att_buf_printf("      ADC_BIAS: %08x\n", act_read(0x4005c128));
    att_buf_printf("      VMIC_CTL: %08x\n", act_read(0x4005c12c));
    att_buf_printf("   REF_LDO_CTL: %08x\n", act_read(0x4005c130));

    att_buf_printf("    CMU_DEVCLKEN: %08x\n", act_read(0x40001004));
    att_buf_printf("    MRCR: %08x\n", act_read(0x40000000));

    att_buf_printf(" AUDIOPLL0_CTL: %08x\n", act_read(0x40000128));
    att_buf_printf("    CMU_ADCCLK: %08x\n", act_read(0x40001044));
    att_buf_printf("    DMAIP: %08x\n", act_read(0x4001c000));
    att_buf_printf("    DMAIE: %08x\n", act_read(0x4001c004));
    att_buf_printf("    DMA0CTL: %08x\n", act_read(0x4001c100));
    att_buf_printf("    DMA0START: %08x\n", act_read(0x4001c104));
    att_buf_printf("    DMA0SADDR0: %08x\n", act_read(0x4001c108));
    att_buf_printf("    DMA0SADDR1: %08x\n", act_read(0x4001c10c));
    att_buf_printf("    DMA0DADDR0: %08x\n", act_read(0x4001c110));
    att_buf_printf("    DMA0DADDR1: %08x\n", act_read(0x4001c114));
    att_buf_printf("    DMA0BC: %08x\n", act_read(0x4001c118));
    att_buf_printf("    DMA0RC: %08x\n", act_read(0x4001c11c));
}

/*******************dma************************/

void dma_transfer_cfg(cfg_dma* p_cfg_dma, uint16 dma_irq)
{
    int reg=0;
    int dmax_ctl, dmax_start, dmax_saddr0, dmax_saddr1, dmax_daddr0, dmax_daddr1, dmax_bc;
		
    dmax_ctl 	= DMAxCTL 	+ (p_cfg_dma->dma_ch) * 0x100;
    dmax_start 	= DMAxSTART     + (p_cfg_dma->dma_ch) * 0x100;
    dmax_saddr0 = DMAxSADDR0	+ (p_cfg_dma->dma_ch) * 0x100;
    dmax_daddr0 = DMAxDADDR0	+ (p_cfg_dma->dma_ch) * 0x100;
    dmax_saddr1 = DMAxSADDR1	+ (p_cfg_dma->dma_ch) * 0x100;
    dmax_daddr1 = DMAxDADDR1	+ (p_cfg_dma->dma_ch) * 0x100;
    dmax_bc		= DMAxBC		+ (p_cfg_dma->dma_ch) * 0x100;
  	
  	//enable dma clk and reset
    att_reset_peripheral(RESET_ID_DMA);
    att_clock_peripheral_enable(CLOCK_ID_DMA);
    
  	act_write(DMAIP, (1 << (p_cfg_dma->dma_ch)));
  	act_write(DMAIP, (1 << ((p_cfg_dma->dma_ch)+16)));
		//irq enable
  	act_or(DMAIE, (dma_irq << (p_cfg_dma->dma_ch)));//transfer complete(or half) irq
 
  	//dst set
  	if( (p_cfg_dma->dst_addr0) < 0xff)//is not mem addr
  		reg |= ((p_cfg_dma->dst_addr0) << DMAxCTL_DSTSL_SHIFT);
  	else	//if addr is mem addr(bigger than 7bit)
  		reg |= (0 << DMAxCTL_DSTSL_SHIFT);
  	
  	//src set
  	if( (p_cfg_dma->src_addr0) < 0xff)
  		reg |= ((p_cfg_dma->src_addr0) << DMAxCTL_SRCSL_SHIFT);
  	else//if addr is mem addr(bigger than 7bit)
  		reg |= (0 << DMAxCTL_SRCSL_SHIFT);
  		
  	//trsansfer_mode set(brust8 or single)
  	reg |= ((p_cfg_dma->transfer_mode) << DMAxCTL_AUDIOTYPE);//also cfg burst8/single
  	//reload
  	reg |= ((p_cfg_dma->reload) << DMAxCTL_reload);
  	//data width
  	reg |= ((p_cfg_dma->dma_wl) << DMAxCTL_TWS_SHIFT);

  	act_write(dmax_ctl, reg);
 		//write dst addr 
  	if( (p_cfg_dma->dst_addr0) ==  DMADST_DACFIFO0)
  		act_write(dmax_daddr0, DAC_DAT_FIFO0);
  	else if( (p_cfg_dma->dst_addr0) > 0xff)
  	{
        act_write(dmax_daddr0, (p_cfg_dma->dst_addr0));
        act_write(dmax_daddr1, (p_cfg_dma->dst_addr1));
    }
  	//write src addr
  	if( (p_cfg_dma->src_addr0) == DMASRC_ADCFIFO0)
  		act_write(dmax_saddr0, ADC_FIFO0_DAT);
  	else if( (p_cfg_dma->src_addr0) > 0xff)
  	{
        act_write(dmax_saddr0, (p_cfg_dma->src_addr0));
        act_write(dmax_saddr1, (p_cfg_dma->src_addr1));
    }
    //write bc
    act_write(dmax_bc, (p_cfg_dma->bc));
  	//start dma
    act_or(dmax_start, 1);
}

void dma_reload_times(int dma_ch,int cnt)
{
    int tmp;

    while(cnt)
    {
        do
        {				
            tmp = (act_read(DMAIP) & (1 << dma_ch)) >> dma_ch;
        }
        while(tmp != 0x1);			
        act_write(DMAIP, (1 << dma_ch));
        cnt--;
    }
}

void dma_stop(int dma_ch)
{
		uint32 dmax_start;
		dmax_start 	= DMAxSTART  + (dma_ch * 0x100);

		act_write(dmax_start, 0);
}

void dma_half_comple_pd(int dma_ch)
{
		uint32 tmp;
		act_write(DMAIP, (1 << (dma_ch + 16)));
		do
		{
			tmp = ( (act_read(DMAIP)) >> (16 + dma_ch) ) & 0x1;
		}while(tmp == 0);
		act_write(DMAIP, (1 << (dma_ch + 16)));
}

int32_t dma_cfg_start(ain_track_e channel_id, ain_channel_param_t *channel_param)
{
    cfg_dma dma0_init = {ADC_DMA_CHANNEL, DMASRC_ADCFIFO0, 0, 0, 0, DMA_BRUST8_interleaves, DMA_16B, false, 0};

    /***DMA START cfg***/
    dma0_init.dst_addr0 = (uint32)(channel_param->inputsrc_buf);
    dma0_init.bc        = channel_param->inputsrc_buf_len;

    dma_transfer_cfg(&dma0_init, DMA_IRQ_DIS);
    return 0;
}

bool is_dma_transfer_complete(void)
{
    if( !((act_read(DMAIP) >> ADC_DMA_CHANNEL) & 1))
    {
        return false;
    }
    else
    {
        //clear dma irq pending bit
        act_write(act_read(DMAIP), DMAIP);
        return true;
    }
    return true;
}

static int32_t PHY_audio_init_hw(ain_track_e channel_id, ain_channel_param_t *channel_param)
{ 

    return 0;
}

static int32_t PHY_audio_exit_hw(uint8_t channel_id)
{
    return 0;
}

int32_t ain_open_transfer_channel(ain_track_e channel_id, ain_channel_param_t *channel_param)
{
    /* 初始音频模块 
     */
    PHY_audio_init_hw(channel_id, channel_param);
    att_phy_audio_adc_enable(channel_id, channel_param);
    att_phy_audio_adc_start(channel_id);
    return 0;
}

int32_t ain_close_transfer_channel(ain_track_e channel_id)
{
    
    dma_stop(ADC_DMA_CHANNEL);
    att_phy_audio_adc_disable(channel_id);

    /* 卸载音频模块
     */
    PHY_audio_exit_hw(channel_id);

    return 0; 
}

int32_t ain_start_transmission(ain_track_e channel_id, ain_channel_param_t *channel_param)
{
    att_adc_fifo_enable();
    dma_cfg_start(channel_id, channel_param);
    //adc_dump_register();

    return 0;
}


