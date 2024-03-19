/********************************************************************************
 *        Copyright(c) 2014-2016 Actions (Zhuhai) Technology Co., Limited,
 *                            All Rights Reserved.
 *
 * 描述：自动化测试：mic测试
 * 作者：
 ********************************************************************************/

#include "att_pattern_test.h"
#include "audio_record.h"

typedef struct
{
    u8_t*   data_buf;
    u32_t   buf_size;
    u32_t   sample_time;        ///< sampling time, ms unit
    u32_t   frequency;          ///< MIC input sample_rate, hz
    u32_t   threshold;
    u32_t   energy;
    int     count;              ///< analyse sample count
}_mic_analyse_result_t;


//_mic_analyse_result_t *mic_input;
_mic_analyse_result_t mic_input;
uint8_t mic_mode;

#define Samples						512
int audio_rx[Samples*4] = {0};


/* 音频数据分析处理
 */
u32_t audio_analyse_sample(void* buf, u32_t len, u32_t energy)
{
    s16_t*  data = buf;
    int     i, sign;
    u32_t   flip_count = 0;
    
    //att_buf_printf("buf:0x%x, len\n", (u32_t)buf, len);
    for (i = 0, sign = 1; i < len / 2; i++, data++)
    {
        if (abs(*data) > energy)
        {
            if ((sign > 0) && (*data < 0))
            {
                flip_count++;
                sign = -1;
            }
            else if ((sign < 0) && (*data >= 0))
            {
                flip_count++;
                sign = 1;
            }
        }
        #if 0
        if(i < 10){
            att_printf("%d ", *data);
        }
        #endif
    }
    //att_buf_printf("\n");

    return flip_count;
}

//void mic_dma_data_analyse(int irq, enum dma_irq_type type, void* pdata)
void mic_dma_data_analyse(void)
{
    u32_t flips, frequency;

    flips       = audio_analyse_sample(mic_input.data_buf, mic_input.buf_size, mic_input.energy);
    frequency   = flips * 8;
    att_buf_printf("frequency:%d\n", frequency);

    if (frequency > mic_input.frequency)
    {
        mic_input.frequency = frequency;
    }

    if ((mic_input.count <= 1) || (frequency >= mic_input.threshold))
    {
        att_buf_printf("mic_dma_data_analyse complete!\n");
        mic_input.count = 0;
        ain_close_transfer_channel(mic_mode);
    }

    mic_input.count--;
}


test_result_e mic_input_verify(void *arg_buffer, u32_t arg_len,  ain_track_e mic_mode)
{
    ain_channel_param_t  param;
    test_result_e ret;
    output_arg_t mic_arg[4];
    u32_t energy, threshold, again, dgain;
    u32_t delay_count = 0;

    act_test_read_arg(arg_buffer, mic_arg, ARRAY_SIZE(mic_arg));
    threshold   = *((u32_t*)mic_arg[0].arg);
    energy      = *((u32_t*)mic_arg[1].arg);
    again       = *((u32_t*)mic_arg[2].arg);
    dgain       = *((u32_t*)mic_arg[3].arg);

    att_buf_printf("threshold:%d, energy:%d, again:%d, dgain:%d\n", threshold, energy, again, dgain);

    /* 16bit 8000Hz per channel, 8 samples, 16bytes/ms, 4096 bytes buffer could store 256ms
     */
    //mic_input                       = malloc(sizeof(_mic_analyse_result_t));
    //mic_input->data_buf             = malloc(mic_input->buf_size);
    memset(&mic_input, 0, sizeof(_mic_analyse_result_t));
    mic_input.frequency            = 0;
    mic_input.count                = 16;
    mic_input.threshold            = threshold;
    mic_input.energy               = energy;
    mic_input.buf_size             = sizeof(audio_rx); //8192;
    mic_input.sample_time          = 256;
    mic_input.data_buf             = (u8_t *)audio_rx;

    param.ain_sample_rate           = SAMPLE_RATE_8KHZ;
    param.inputsrc_buf              = mic_input.data_buf;
    param.inputsrc_buf_len          = mic_input.buf_size;
    param.ain_track                 = mic_mode;
    param.adc_bias                  =  0x05493a5a;
    param.use_vmic                  = 0x3;
    param.ain_gain.ch_again[0]      = again;
    param.ain_gain.ch_again[1]      = again;
    param.ain_gain.ch_dgain[0]      = dgain;
    param.ain_gain.ch_dgain[1]      = dgain;
    

    ain_open_transfer_channel(mic_mode, &param);
    
    while(delay_count < 100){
        act_test_inform_state(DUT_STATE_BUSY);
        mdelay(10);
        delay_count++;
    }
    ain_start_transmission(mic_mode, &param);
    att_buf_printf("%s,%d, count:%d\n", __func__, __LINE__, mic_input.count);

    while(mic_input.count > 0)
    {
        if(is_dma_transfer_complete())
        {
             dma_stop(0);
            mic_dma_data_analyse();
            dma_cfg_start(mic_mode, &param);
        }
        // MUST communicate with PC in 1s, otherwise it would see as disconnect, cost 10ms
        act_test_inform_state(DUT_STATE_BUSY);
    }
    att_buf_printf("%s,%d, count:%d\n", __func__, __LINE__, mic_input.count);

    if (mic_input.frequency >= threshold)
    {
        ret = TEST_PASS;
    }
    else
    {
        ret = TEST_MIC_FAIL;
    }

    //free(mic_input->data_buf);
    //free(mic_input);
    return ret;
}

/**
 *  \brief Verify MIC input
 *
 *  \param [in] arg_buffer Arguments buffer
 *  \param [in] arg_len Arguments length.
 *  \return Test result \ref in test_result_e
 *
 *  \details
 */
test_result_e act_test_left_mic_verify(void *arg_buffer, u32_t arg_len)
{
    test_result_e ret_val;

    ret_val = mic_input_verify(arg_buffer, arg_len, INPUTSRC_ONLY_L);
    
    act_test_normal_report(TESTID_LEFT_MIC_VERIFY, ret_val);
    return ret_val;
}

test_result_e act_test_right_mic_verify(void *arg_buffer, u32_t arg_len)
{
    test_result_e ret_val;

    ret_val = mic_input_verify(arg_buffer, arg_len, INPUTSRC_ONLY_R);
    act_test_normal_report(TESTID_RIGHT_MIC_VERIFY, ret_val);
    return ret_val;
}



bool __ENTRY_CODE pattern_main(struct att_env_var *para)
{
    bool ret_val = false;

	memset(&__bss_start, 0,
		 ((u32_t) &__bss_end - (u32_t) &__bss_start));

	self = para;

    att_buf_printf("prepare mic testid:%d\n", self->test_id);
  
	if (self->test_id == TESTID_LEFT_MIC_VERIFY) {
		ret_val = act_test_left_mic_verify(self->arg_buffer, self->arg_len);
	} else if (self->test_id == TESTID_RIGHT_MIC_VERIFY) {
		ret_val = act_test_right_mic_verify(self->arg_buffer, self->arg_len);
	} else  {
	}

    return ret_val;
}
