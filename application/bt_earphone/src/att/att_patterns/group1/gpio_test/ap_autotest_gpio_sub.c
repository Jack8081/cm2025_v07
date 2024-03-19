/*******************************************************************************
 *                              US212A
 *                            Module: Picture
 *                 Copyright(c) 2003-2012 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       zhangxs     2011-09-19 10:00     1.0             build this file
 *******************************************************************************/
/*!
 * \file     user1_main.c
 * \brief
 * \author   zhangxs
 * \version  1.0
 * \date  2011/9/05
 *******************************************************************************/
#include "att_pattern_test.h"
#include <gpio.h>
#include "ap_autotest_gpio.h"

test_result_e test_gpio(gpio_test_arg_t *gpio_test_arg)
{
    test_result_e ret = 0;
    uint32 *gpio_bak_array;
    uint32 gpio_reg;
    u32_t GPIO_GROUP_1_result = 0;
    uint8 i, test_num = 0;
    u32_t  value;
    struct device *gpio_dev;

    gpio_dev = self->dev_tbl->gpio_dev;

    if(!gpio_dev){
        att_buf_printf("gpio_dev is null\n");
        return TEST_FAIL;
    }

    if(gpio_test_arg->gpio_ctrl_pin > 30)
    {
        att_buf_printf("gpio test parameter error:%d\n",gpio_test_arg->gpio_ctrl_pin);
        gpio_test_arg->gpio_ctrl_pin = 0;
        return TEST_FAIL;
    }

    for (i = 0; i < 30; i++)
    {
        if(gpio_test_arg->gpio_test_pin  & (1 << i))
        {
            test_num = i;
        }
    }

    gpio_bak_array = app_mem_malloc(4 * (test_num + 1));
    if(!gpio_bak_array)
    {
        att_buf_printf("app_mem_malloc fail!");
        return TEST_FAIL;
    }

    gpio_reg = GPIO_REG_BASE;

    for (i = 0; i < test_num + 1; i++, gpio_reg += 4)
    {
        gpio_bak_array[i] = sys_read32(gpio_reg);
    }

    for (i = 0; i < test_num + 1 ; i++)
    {
        if(gpio_test_arg->gpio_test_pin  & (1 << i))
        {
            sys_write32((1 << 6), GPIO_REG_BASE + (i * 4) + 4);
            sys_write32((1 << i), GPIO_REG_BSR);
        }
    }

    sys_write32((1 << 6), GPIO_REG_BASE + gpio_test_arg->gpio_ctrl_pin * 4 + 4);

    for (i = 0; i < test_num + 1; i++)
    {
        ret = 0;
        if(gpio_test_arg->gpio_test_pin & (1 << i))
        {
            ret = 1;
        }
        if(ret == 1)
        {
            sys_write32((1 << 7), GPIO_REG_BASE + (i * 4) + 4);
            sys_write32((1 << gpio_test_arg->gpio_ctrl_pin), GPIO_REG_BRR);

            k_sleep(1);

            value = sys_read32(GPIO_REG_BASE + 0x230);
            value = (value >> i) & 1;
            if(value != 0)
            {
                GPIO_GROUP_1_result |= 1 << i;
                continue;
            }

            sys_write32((1 << gpio_test_arg->gpio_ctrl_pin), GPIO_REG_BSR);

            k_sleep(1);

            value = sys_read32(GPIO_REG_IDAT);
            value = (value >> i) & 1;
            if(value != 1)
            {
                GPIO_GROUP_1_result |= 1 << i;
                continue;
            }
        }

    }
    ret = TEST_PASS;

    gpio_reg = GPIO_REG_BASE;
    for (i = 0; i < test_num + 1; i++, gpio_reg += 4)
    {
        sys_write32(gpio_bak_array[i], gpio_reg);
    }

    app_mem_free(gpio_bak_array);

    gpio_test_arg->gpio_test_pin = GPIO_GROUP_1_result;

    if(GPIO_GROUP_1_result)
    {
        for (i = 0; i < test_num; i++)
        {
            if(GPIO_GROUP_1_result  & (1 << i))
            {
                att_buf_printf("gpio %d test fail\n",i);
            }
        }
        ret = TEST_FAIL;
    }

    return ret;
}

test_result_e act_test_gpio_test(void *arg_buffer, u32_t arg_len)
{
    test_result_e ret_val;
    output_arg_t gpio_arg[2];
    gpio_test_arg_t gpio_test_arg = {0};

    act_test_read_arg(arg_buffer, gpio_arg, ARRAY_SIZE(gpio_arg));

    gpio_test_arg.gpio_test_pin = *((u32_t*)gpio_arg[0].arg);
    gpio_test_arg.gpio_ctrl_pin = *((u32_t*)gpio_arg[1].arg);

    att_buf_printf("gpio test:0x%x, gpio ctrl:%d\n", gpio_test_arg.gpio_test_pin, gpio_test_arg.gpio_ctrl_pin);
    ret_val = test_gpio(&gpio_test_arg);

    ret_val = act_test_normal_report(TESTID_GPIO_TEST, ret_val);

    return ret_val;
}

bool __ENTRY_CODE pattern_main(struct att_env_var *para)
{
    bool ret_val = false;

	memset(&__bss_start, 0,
		 ((u32_t) &__bss_end - (u32_t) &__bss_start));

	self = para;

    att_buf_printf("input gpio test\n");
    
    if (self->test_id == TESTID_GPIO_TEST) {
        ret_val = act_test_gpio_test(self->arg_buffer, self->arg_len);
    }else /*TESTID_FM_CH_TEST*/ {
        //ret_val = act_test_audio_fm_in(&g_fm_rx_test_arg);
    }

    return ret_val;
}


