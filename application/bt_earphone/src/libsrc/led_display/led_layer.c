/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file led_layer.c
 * @brief led layer
 */
#include <os_common_api.h>
#include <app_common.h>

#include "led_display.h"
#include "led_ctrl.h"


u8_t led_get_layer_id(int ui_event)
{
    u8_t layer_id = LED_LAYER_SYS_NOTIFY;
    switch(ui_event){
        case UI_EVENT_CUSTOMED_8:   // BATTERY_NORMAL
        case UI_EVENT_CUSTOMED_9:   // BATTERY_MEDIUM
        case UI_EVENT_STANDBY:
        case UI_EVENT_BATTERY_LOW:
        case UI_EVENT_BATTERY_LOW_EX:
        case UI_EVENT_BATTERY_TOO_LOW:
        case UI_EVENT_REPEAT_BAT_LOW:
        case UI_EVENT_CHARGE_START:
        case UI_EVENT_CHARGE_STOP:
        case UI_EVENT_CHARGE_FULL:
        case UI_EVENT_FRONT_CHARGE_POWON:
            layer_id = LED_LAYER_CHARGER;
            break;

#if (!defined CONFIG_SAMPLE_CASE_1) && (!defined CONFIG_SAMPLE_CASE_XNT)
        case UI_EVENT_BT_CALL_OUTGOING:
        case UI_EVENT_BT_CALL_INCOMING:
        case UI_EVENT_BT_CALL_3WAYIN:
        case UI_EVENT_BT_CALL_ONGOING:
        case UI_EVENT_BT_MUSIC_PAUSE:
        case UI_EVENT_BT_MUSIC_PLAY:
        case UI_EVENT_VOICE_ASSIST_START:
        case UI_EVENT_BT_MUSIC_DISP_NONE:
        case UI_EVENT_BT_CALL_DISP_NONE:
#endif
        case UI_EVENT_MIC_MUTE_ON:
        case UI_EVENT_MIC_MUTE_OFF:
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
        case UI_EVENT_CUSTOMED_1:
        case UI_EVENT_CUSTOMED_2:
        case UI_EVENT_CUSTOMED_3:
        case UI_EVENT_CUSTOMED_4:
        case UI_EVENT_CUSTOMED_5:
        case UI_EVENT_CUSTOMED_6:
#endif
            layer_id = LED_LAYER_BT;
            break;

        case UI_EVENT_BT_UNLINKED:
        case UI_EVENT_ENTER_PAIR_MODE:
        case UI_EVENT_TWS_WAIT_PAIR:
        case UI_EVENT_TWS_CONNECTED:  
        case UI_EVENT_BT_CONNECTED:
        case UI_EVENT_BT_WAIT_CONNECT: 
        case UI_EVENT_ENTER_BQB_TEST_MODE:
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
        case UI_EVENT_BT_CONNECTED_USB:
        case UI_EVENT_BT_DISCONNECTED_USB:
        case UI_EVENT_BT_CONNECTED_BR: 
        case UI_EVENT_BT_DISCONNECTED_BR: 
        case UI_EVENT_ENTER_PAIRING_MODE:
#endif
            layer_id = LED_LAYER_BT_MANAGER;
            break;

        default:
            layer_id = LED_LAYER_SYS_NOTIFY;
            break;

    }
    return layer_id;
}


/* 
  * get led layer
 */
led_layer_context_t* led_layer_get_context(u8_t layer_id)
{
    //u32_t  irq_flags = local_irq_save();
    led_display_context_t *led_display = (led_display_context_t*)led_display_get_context();
    led_layer_context_t*  layer = NULL;
    led_layer_context_t*  t;

    list_for_each_entry(t, &led_display->layer_list, node)
    {
        if (t->layer_id == layer_id)
        {
            layer = t;
            break;
        }
    }

    //local_irq_restore(irq_flags);

    return layer;
}

/*!
 * \brief need update layer display region 
 * \param layer_id : 
 * \param region  : the region that need to update
 * \return: TRUE or FALSE
 */
bool led_layer_need_update(u8_t layer_id, led_region_t region)
{
    //u32_t  irq_flags = local_irq_save();
    led_display_context_t *led_display = (led_display_context_t*)led_display_get_context();
    led_layer_context_t*  layer;

    layer = led_layer_get_context(layer_id);
    if (layer == NULL)
    {
        goto end;
    }

    region = led_region_intersect(region, layer->region);

    if (!led_region_valid(region))
    {
        goto end;
    }

    list_for_each_entry(layer, &led_display->layer_list, node)
    {
        if (layer->layer_id == layer_id)
        {
            break;
        }

        region = led_region_clip(region, layer->region);

        if (!led_region_valid(region))
        {
            goto end;
        }
    }

    led_display->update_region = led_region_merge(led_display->update_region, region);

end:
    //local_irq_restore(irq_flags);

    return true;
}


/*
 * update all led layers
 */
bool led_layer_need_update_all(led_region_t region)
{    
    led_layer_context_t*  layer;
    led_display_context_t *led_display = (led_display_context_t*)led_display_get_context();

    list_for_each_entry(layer, &led_display->layer_list, node)
    {
        led_layer_need_update(layer->layer_id, region);

        region = led_region_clip(region, layer->region);

        if (!led_region_valid(region))
        {
            break;
        }
    }

    return true;
}


/*
 * \brief : set modified layer display region
 * \param layer_id :
 * \param region  : new display region
 * \return: TURE or FALSE
 */
bool led_layer_set_region(u8_t layer_id, led_region_t region)
{
    //u32_t  irq_flags = local_irq_save();
    led_layer_context_t*  layer;
    led_region_t         old_region;
    bool  result = false;

    layer = (led_layer_context_t*)led_layer_get_context(layer_id);
    if (NULL == layer)
    {
        goto end;
    }

    old_region   = layer->region;
    layer->region = region;

    region = led_region_merge(old_region, region);

    led_layer_need_update_all(region);

    result = true;
    
end:
    //local_irq_restore(irq_flags);

    return result;
}

void led_layer_add_list(led_layer_context_t* layer)
{    
    led_display_context_t *led_display = (led_display_context_t*)led_display_get_context();
    led_layer_context_t*  pos;

    if (list_empty(&led_display->layer_list))
    {
        list_add(&layer->node, &led_display->layer_list);
        return;
    }

    list_for_each_entry(pos, &led_display->layer_list, node)
    {
        if (layer->order >= pos->order)
        {
            __list_add(&layer->node, pos->node.prev, &pos->node);
            return;
        }
    }

    list_add_tail(&layer->node, &led_display->layer_list);
}

int led_layer_get_index(u8_t layer_id)
{
    led_display_context_t *led_display = (led_display_context_t*)led_display_get_context();
    led_layer_context_t*  layer;

    int  index = 0;

    list_for_each_entry(layer, &led_display->layer_list, node)
    {
        if (layer->layer_id == layer_id)
        {
            break;
        }

        index += 1;
    }

    return index;
}

/*!
 * \brief set changed layer order 
 * \n  order :  number more large more front 
 * \return: TRUE or FALSE
 */
bool led_layer_set_order(u8_t layer_id, u32_t order)
{
    //u32_t  irq_flags = local_irq_save();

    led_layer_context_t *layer;
    int                 old_index;

    bool  result = false;
    
    //LOG_INF("+++++layer_id:%d, order:%d", layer_id, order);

    layer = (led_layer_context_t*)led_layer_get_context(layer_id);
    if (NULL == layer)
    {
        goto end;
    }

    old_index = led_layer_get_index(layer_id);

    /*
     * delete from layer list and re-add
     */
    __list_del_entry(&layer->node);

    layer->order = order;

    led_layer_add_list(layer);

    /*
     * when layer order is change, need update layer display.
     */
    if (led_layer_get_index(layer_id) != old_index)
    {
        led_layer_need_update_all(layer->region);
    }

    result = true;
    
end:
    //local_irq_restore(irq_flags);

    return result;
}

/*
  * get layer list
 */
u8_t* led_layer_get_list(int* num_layers)
{
    //u32_t  irq_flags = local_irq_save();
    
    u8_t*   layer_list = NULL;
    int     i = 0;

    led_display_context_t *led_display = (led_display_context_t*)led_display_get_context();

    led_layer_context_t*  layer;

    list_for_each_entry(layer, &led_display->layer_list, node)
    {
        i += 1;
    }

    if (i > 0)
    {
        layer_list = mem_malloc(i * sizeof(u8_t));
        if (!layer_list)
            return NULL;	
        i = 0;
        
        list_for_each_entry(layer, &led_display->layer_list, node)
        {
            layer_list[i] = layer->layer_id;
            i += 1;
        }
    }

    //local_irq_restore(irq_flags);

    *num_layers = i;

    return layer_list;
}


bool led_layer_create(u8_t layer_id)
{
    led_layer_context_t*  layer;

    layer = mem_malloc(sizeof(led_layer_context_t));
    if(NULL == layer){
        //LOG_INF("create led layer err!");
        return false;
    }

    memset(layer, 0, sizeof(led_layer_context_t));

    layer->layer_id   = layer_id;
    //layer->region    = info->region;
    //layer->view_proc = info->view_proc;
    //layer->order     = info->order;

    led_layer_add_list(layer);
    
    return true;
}

bool led_layer_delete(u8_t layer_id)
{
    led_layer_context_t*  layer;
    //u32_t  irq_flags;

    //irq_flags = local_irq_save();
    layer = (led_layer_context_t*)led_layer_get_context(layer_id);
    if (NULL == layer)
    {
        goto end;
    }

    list_del(&layer->node);
    mem_free(layer);

end:
    //local_irq_restore(irq_flags);

    return true;
}


