/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file led_ctrl.h
 * @brief led controller display
 */

#ifndef _LED_CTL_H
#define _LED_CTL_H

#include "list.h"
#include "app_config.h"

/*!
 * \brief 最多定义 LED 个数
 * \n  参考 CFG_TYPE_LED_NO
 */
#define MAX_LED_BITS  8

/*!
 * \brief 全部 LED 显示模式
 */
#define LED_DISPLAY_MODEL_ALL  (u32_t)-1


/*!
 * \brief LED 显示状态定义
 */
typedef enum
{
    LED_STATE_ON  = 1,  //!< 亮
    LED_STATE_OFF = 0,  //!< 灭
    
} led_state_t;

/*!
 * \brief LED 显示模式结束回调函数类型
 */
typedef void (*led_model_end_callback_t)(u32_t model_id, u32_t layer_id);

/*!
 * \brief LED 配置结构类型
 */
typedef struct
{
    CFG_Struct_LED_Drives  cfg_leds;  //!< LED 驱动配置

} led_configs_t;

bool led_ctrl_start_model
(    
    CFG_Type_LED_Display_Model*  cfg_model,
    u32_t                       visiable_leds,
    u32_t                       layer_id,
    led_model_end_callback_t     end_callback
);
void led_ctrl_stop_model(u32_t model_id, u32_t layer_id);
bool led_ctrl_check_model(u32_t model_id, u32_t layer_id);
void led_ctrl_init(void);
void led_ctrl_deinit(void);


#endif //_LED_CTL_H


