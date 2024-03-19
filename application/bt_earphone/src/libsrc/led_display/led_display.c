/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file led_display.c
 * @brief led display
 */
 
#include <os_common_api.h>
#include <app_common.h>

#include "led_display.h"
#include "led_ctrl.h"


typedef struct
{
    struct list_head  node;
    led_region_t  region;
    u8_t          layer_id;
    u8_t          led_model_id;
} led_paint_info_t;


led_display_context_t led_display_context;

led_display_context_t *led_display_get_context()
{
    return &led_display_context;
}

bool led_notify_check_options(u8_t option)
{
	int ret;
    uint32_t dev_type;

	SYS_LOG_INF("option: 0x%x\n",  option);

    dev_type = option >> 2;

	ret = ui_key_check_dev_type(dev_type);
    return ret;
}

/* 
 * get led model related config
 */
bool led_model_get_config(u32_t model_id, CFG_Type_LED_Display_Model* cfg_model)
{
    CFG_Struct_LED_Display_Models*  cfg;
    
    int  i;

    if (model_id == NONE)
    {
        return false;
    }

    cfg = mem_malloc(sizeof(*cfg));
	if(!cfg) return false;
    
    app_config_read(CFG_ID_LED_DISPLAY_MODELS, cfg, 0, sizeof(*cfg));

    for (i = 0; i < CFG_MAX_LED_DISPLAY_MODELS; i++)
    {
        if (cfg->Model[i].Display_Model == model_id)
        {
            *cfg_model = cfg->Model[i];
            break;
        }
    }

    mem_free(cfg);

    if (i < CFG_MAX_LED_DISPLAY_MODELS)
    {
        return true;
    }

    return false;
}

u32_t led_model_get_disp_leds(u32_t model_id)
{
    CFG_Type_LED_Display_Model  cfg_model;
    
    if (led_model_get_config(model_id, &cfg_model))
    {
        return (cfg_model.Display_LEDs | cfg_model.Disable_LEDs);
    }
    
    return LED_NULL;
}

void led_model_end_callback(u32_t model_id, u32_t layer_id)
{
    //u32_t  irq_flags = local_irq_save();

    led_layer_context_t*  layer;

	SYS_LOG_INF("%d\n",  __LINE__);

    layer = led_layer_get_context(layer_id);
    if (NULL == layer)
    {
        goto end;
    }

    if (layer->disp_leds != LED_NULL)
    {
        led_region_t  region = led_region_clip(layer->region, layer->disp_leds);
        
        led_layer_set_region(layer->layer_id, region);
    }

    layer->led_model_id = NONE;
    layer->disp_leds = LED_NULL;

end:
    //local_irq_restore(irq_flags);
    return;
}

static u8_t led_layer_get_model(u8_t layer_id)
{
    led_layer_context_t  *layer;

    layer = led_layer_get_context(layer_id);
    if (NULL == layer)
    {
        return NONE;
    }
    return layer->led_model_id;
}

static bool led_layer_set_model(u8_t layer_id, u32_t model_id)
{
    led_layer_context_t*  layer;
    led_region_t         region;
    bool  result = false;

    layer = led_layer_get_context(layer_id);
    if (layer == NULL)
    {
	    SYS_LOG_ERR("%d\n",  __LINE__);
        goto end;
    }

    if (layer->led_model_id != NONE)
    {
        led_ctrl_stop_model(layer->led_model_id, layer->layer_id);
    }

    if (layer->disp_leds != LED_NULL)
    {
        region = led_region_clip(layer->region, layer->disp_leds);
        led_layer_set_region(layer->layer_id, region);
    }

    layer->led_model_id = model_id;

    layer->disp_leds = led_model_get_disp_leds(model_id);

    if (layer->disp_leds != LED_NULL)
    {
        region = led_region_merge(layer->region, layer->disp_leds);
        
        led_layer_set_region(layer->layer_id, region);
    }

    result = true;

end:
    //local_irq_restore(irq_flags);

    return result;

}

/*!
 * \brief draw display update
 */
void led_display_update(void)
{
    //u32_t  irq_flags = local_irq_save();
    led_display_context_t *led_display = led_display_get_context();
    led_region_t  update_region = led_display->update_region;
    struct list_head  paint_list;
    led_paint_info_t*  paint_info;
    led_paint_info_t*  next;
    u8_t*   layer_list;
    int     num_layers;
    int     i;
    bool  update_led_layers = false;

    if (!led_region_valid(update_region))
    {
        //local_irq_restore(irq_flags);
        return;
    }

    led_display->update_region = LED_REGION_NONE;

    layer_list = led_layer_get_list(&num_layers);

    //local_irq_restore(irq_flags);

    /* LED display area needs to be completely updated.
     * The display mode of LED configuration can control multiple leds, and there may be multi-mode coverage,
     * Updating an LED display area in a view separately may result in incorrect display
     */
    if (led_region_valid(led_region_intersect(update_region, LED_REGION_ALL_LEDS)))
    {
        update_led_layers = true;
        update_region = led_region_merge(update_region, LED_REGION_ALL_LEDS);
    }

    INIT_LIST_HEAD(&paint_list);
    
    for (i = 0; i < num_layers; i++)
    {
        led_layer_context_t  *layer;
        led_region_t  region;

        layer = led_layer_get_context(layer_list[i]);
        if (NULL == layer)
        {
            continue;
        }

        if (!led_region_valid(layer->region))
        {
            continue;
        }

        region = led_region_intersect(update_region, layer->region);

        if (!led_region_valid(region))
        {
            /*
             * if led layers need full update 
             */
            //if (!(update_led_layers && (layer.flags & UI_FLAG_LED_VIEW)))
            if (!(update_led_layers))
            {
                continue;
            }
        }

        /*
         * record layer message that need display from front to back
         */
        paint_info = mem_malloc(sizeof(led_paint_info_t));
		if(!paint_info) return;

        paint_info->layer_id = layer_list[i];
        paint_info->region  = region;
        paint_info->led_model_id = layer->led_model_id;

        list_add(&paint_info->node, &paint_list);

        update_region = led_region_clip(update_region, region);

        if (!led_region_valid(update_region))
        {
            if (!update_led_layers)
            {
                break;
            }
        }
    }

    if (layer_list != NULL)
    {
        mem_free(layer_list);
    }

    /*
     * start display from back to front
     */
    list_for_each_entry_safe(paint_info, next, &paint_list, node)
    {
        led_region_t  region = paint_info->region;   
        CFG_Type_LED_Display_Model  cfg_model;

        //start led_model_id display
        if (led_model_get_config(paint_info->led_model_id, &cfg_model))
        {
            led_ctrl_start_model
            (
                &cfg_model, region, paint_info->layer_id, led_model_end_callback
            );
        }

        mem_free(paint_info);
    }
}


void led_notify_start_display(system_notify_ctrl_t* notify_ctrl)
{
    u32_t  model_id;

    model_id = led_layer_get_model(notify_ctrl->layer_id);

    /* set led display */
    if (!notify_ctrl->flags.set_led_display)
    {
        notify_ctrl->flags.set_led_display = true;

        if (model_id != notify_ctrl->cfg_notify.LED_Display)
        {
            SYS_LOG_INF("model_id:%d start\n",  notify_ctrl->cfg_notify.LED_Display);
            led_layer_set_model
            (
                notify_ctrl->layer_id, notify_ctrl->cfg_notify.LED_Display
            );
        }

        /* set led layer order */
        led_layer_set_order(notify_ctrl->layer_id, notify_ctrl->cfg_notify.LED_Override);

        led_display_update();

        if (notify_ctrl->flags.led_disp_only)
        {
            SYS_LOG_INF("model_id:%d end\n",  model_id);
            notify_ctrl->flags.end_led_display = true;
            return;
        }        
    }

    
    /* wait display end */
    else if (led_ctrl_check_model(model_id, notify_ctrl->layer_id) == false)
    {
        notify_ctrl->flags.end_led_display = true;
        SYS_LOG_INF("model_id:%d end\n",  model_id);

        led_display_update();
    }
}

/*
 * when a notify_ctl finish, LED_LAYER_SYS_NOTIFY layer should stop display
 */
bool is_led_layer_notify(system_notify_ctrl_t* notify_ctrl)
{
    if(notify_ctrl->layer_id == LED_LAYER_SYS_NOTIFY)
    {
        return true;
    }
    else
    {
        return false;
    }

}

void led_notify_stop_display(u8_t          layer_id)
{
    if (led_layer_get_model(layer_id) != NONE)
    {
        SYS_LOG_INF("layer_id:%d", layer_id);
        led_layer_set_model(layer_id, NONE);

        led_display_update();
    }
}

/*
  * name: led_display_init
  * bref:   init led ctrl, and create led layers.
  */
int led_display_init(void)
{
    led_display_context_t *led_display = led_display_get_context();
    u8_t i;

    led_ctrl_init();

    if (led_display->layer_list.next == NULL)
    {
        INIT_LIST_HEAD(&led_display->layer_list);
    }

    for(i=0; i < LED_LAYER_MAX; i++){
        led_layer_create(i);
    }

    return 0;
}

/*
  * name: led_display_deinit
  * bref:   deinit led ctrl and release led layers
  */

int led_display_deinit(void)
{
    u8_t i;
    
    for(i=0; i < LED_LAYER_MAX; i++){
        led_layer_delete(i);
    }

    led_ctrl_deinit();
    return 0;
}

static u8_t s_led_model;
void led_display_sleep(void)
{
	s_led_model = led_layer_get_model(LED_LAYER_CHARGER);
}

void led_display_wakeup(void)
{
	led_layer_set_model(LED_LAYER_CHARGER, s_led_model);
	led_display_update();
}

