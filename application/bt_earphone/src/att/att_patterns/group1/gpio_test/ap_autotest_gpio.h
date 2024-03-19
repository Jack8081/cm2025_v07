#ifndef _AP_AUTOTEST_GPIO_H
#define _AP_AUTOTEST_GPIO_H

#define GPIO_REG_BSR 0x40068210
#define GPIO_REG_BRR 0x40068220
#define GPIO_REG_IDAT 0x40068230

typedef struct
{
    uint32 out_en;
    uint32 in_en;
    uint32 dat;
    //pull-up
    uint32 pu_en;
    //pull-down
    uint32 pd_en;
}gpio_ret_t;

typedef enum
{
    GPIO_TEST_OK = 0,

}gpio_test_result_e;

typedef struct
{
    uint32 gpio_test_pin;
    uint8 gpio_ctrl_pin;
}gpio_test_arg_t;
#endif

