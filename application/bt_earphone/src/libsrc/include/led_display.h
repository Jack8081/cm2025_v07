/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file led_display.h
 * @brief led controller display
 */

#ifndef _LED_DISPLAY_H
#define _LED_DISPLAY_H

#include <system_notify.h>


#define LED_REGION_NONE  0
#define LED_REGION_ALL_LEDS  ((1 << MAX_LED_BITS) - 1)  // 0xff

enum LED_LAYER_ID {
    LED_LAYER_CHARGER = 0,
    LED_LAYER_SYS_NOTIFY,
    LED_LAYER_BT,
    LED_LAYER_BT_MANAGER,
    LED_LAYER_MAX,
};


/*!
 * \brief 显示区域数据类型
 * \n  以 bit 位定义显示区域, 每个 bit 表示一块区域
 */
typedef u32_t  led_region_t;

typedef struct
{
    struct list_head  layer_list;
    led_region_t       update_region;
}led_display_context_t;

typedef int (*led_layer_proc_t)(uint8_t layer_id, void *msg_data);

typedef struct
{
    struct list_head  node;

    led_region_t     region;
    led_layer_proc_t  layer_proc;
    u8_t   order;
    u8_t   layer_id;
    u8_t   app_id;
    
    u8_t   led_model_id;
    u16_t  disp_leds;

} led_layer_context_t;


/* 剪切显示区域
 */
static inline led_region_t led_region_clip(led_region_t region1, led_region_t region2)
{
    return (region1 & ~region2);
}


/* 相交显示区域
 */
static inline led_region_t led_region_intersect(led_region_t region1, led_region_t region2)
{
    return (region1 & region2);
}


/* 合并显示区域
 */
static inline led_region_t led_region_merge(led_region_t region1, led_region_t region2)
{
    return (region1 | region2);
}


/* 显示区域是否有效
 */
static inline bool led_region_valid(led_region_t region)
{
    return (region != 0);
}

led_layer_context_t* led_layer_get_context(u8_t layer_id);
u8_t led_get_layer_id(int ui_event);
bool led_layer_need_update_all(led_region_t region);
bool led_layer_set_region(u8_t layer_id, led_region_t region);
void led_layer_add_list(led_layer_context_t* layer);
int led_layer_get_index(u8_t layer_id);
bool led_layer_set_order(u8_t layer_id, u32_t order);
u8_t* led_layer_get_list(int* num_layers);
bool led_layer_delete(u8_t layer_id);
bool led_layer_create(u8_t layer_id);
bool is_led_layer_notify(system_notify_ctrl_t* notify_ctrl);
void led_notify_start_display(system_notify_ctrl_t* notify_ctrl);
void led_notify_stop_display(u8_t          layer_id);

int led_display_init(void);
int led_display_deinit(void);
led_display_context_t *led_display_get_context();

void led_display_sleep(void);
void led_display_wakeup(void);


#endif //_LED_DISPLAY_H


