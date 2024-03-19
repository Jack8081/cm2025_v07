/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file led_ctrl.c
 * @brief led controller display
 */
 
#include <os_common_api.h>
#include <app_common.h>
#include <led_manager.h>

#include "led_ctrl.h"

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "ledctl"
#endif

typedef struct
{
    struct list_head   node;
	struct k_delayed_work timer;

    u8_t  disabled;
    u8_t  onoff_state;
    u8_t  flash_count;
    u8_t  loop_count;
    
    CFG_Type_LED_Display_Model  disp_model;
    
    led_model_end_callback_t  end_callback;

    u32_t  visiable_leds;
    u32_t  layer_id;
    
    void*  reserved;

} led_display_ctrl_t;


typedef struct
{
    led_configs_t  configs;
    
    struct list_head  disp_ctrl_list;
    
} led_driver_context_t;


static led_driver_context_t  led_driver_context;

static void led_ctrl_start_display(led_display_ctrl_t* disp_ctrl);
static void led_ctrl_stop_display(led_display_ctrl_t* disp_ctrl, led_model_end_callback_t end_callback);


static led_driver_context_t* led_ctl_get_context(void)
{
    return &led_driver_context;
}

static CFG_Type_LED_Drive* get_led_drive(u32_t led_no)
{
    led_driver_context_t*  led_driver = led_ctl_get_context();

    int  i;

    for (i = 0; i < CFG_MAX_LEDS; i++)
    {
        CFG_Type_LED_Drive*  led = &led_driver->configs.cfg_leds.LED[i];

        if (led->LED_No   == LED_NULL ||
            led->GPIO_Pin == LED_GPIO_NONE)
        {
            continue;
        }

        #if 0
        if (gpio_get_drv_module(led->GPIO_Pin) != GPIO_DRV_MODULE_LED)
        {
            continue;
        }
        #endif

        if (led->LED_No == led_no)
        {
            return led;
        }
    }

    return NULL;
}

static CFG_Type_LED_Drive* get_pwm_led_drive(CFG_Type_LED_Display_Model* disp_model)
{
    CFG_Type_LED_Drive*  led;
    
    if (disp_model->ON_Time_Ms == 0 &&
        disp_model->Breath_Time_Ms == 0)
    {
        return NULL;
    }
    
    led = get_led_drive(disp_model->Display_LEDs);

    return led;
}

static int get_num_leds(u32_t leds)
{
    int  num_leds = 0;
    int  i;

    for (i = 0; i < MAX_LED_BITS; i++)
    {
        u32_t  led_no = (1 << i);
        
        if (!(leds & led_no))
        {
            continue;
        }
        
        if (get_led_drive(led_no) != NULL)
        {
            num_leds += 1;
        }
    }

    return num_leds;
}


/*
 * control designated led on or off
 */
static void led_ctrl_set_state(u32_t leds, led_state_t state)
{
    int  i;

    //SYS_LOG_INF("0x%x, %d", leds, state);

    for (i = 0; i < MAX_LED_BITS; i++)
    {
        CFG_Type_LED_Drive*  led;
        
        u32_t  led_no = (1 << i);
        
        if (!(leds & led_no))
        {
            continue;
        }
        
        if ((led = get_led_drive(led_no)) == NULL)
        {
            continue;
        }

        if (state)
        {
            led_manager_set_display(i, led->Active_Level, OS_FOREVER, NULL);
        }
        else
        {
            led_manager_set_display(i, !(led->Active_Level), OS_FOREVER, NULL);
        }
    }
}

static bool led_ctrl_check_visiable(led_display_ctrl_t* disp_ctrl)
{
    CFG_Type_LED_Display_Model*  disp_model = &disp_ctrl->disp_model;
    
    if (disp_ctrl->disabled)
    {
        return false;
    }
    
    if (disp_model->Display_LEDs & disp_ctrl->visiable_leds)
    {
        return true;
    }
    
    return false;
}

static void led_ctrl_set_state_ex(led_display_ctrl_t* disp_ctrl, led_state_t state)
{
    CFG_Type_LED_Display_Model*  disp_model = &disp_ctrl->disp_model;
    
    if (led_ctrl_check_visiable(disp_ctrl) == true)
    {
        led_ctrl_set_state(disp_model->Display_LEDs, state);
    }
}


static void led_ctrl_start_pwm_display(led_display_ctrl_t* disp_ctrl)
{
    CFG_Type_LED_Display_Model*  disp_model = &disp_ctrl->disp_model;
    
    if (led_ctrl_check_visiable(disp_ctrl) == YES)
    {
        CFG_Type_LED_Drive*  led;
        uint8_t i;
        u32_t  active_time_ms = disp_model->ON_Time_Ms;
        u32_t  inactive_time_ms = disp_model->OFF_Time_Ms + disp_model->Loop_Wait_Time_Ms;
        u32_t  period = active_time_ms + inactive_time_ms;

        if ((led = get_pwm_led_drive(disp_model)) == NULL)
        {
            return;
        }

        //SYS_LOG_INF("0x%x", disp_model->Display_LEDs);

        for (i = 0; i < MAX_LED_BITS; i++)
        {
            u32_t  led_no = (1 << i);
            if(led_no == led->LED_No)
            {
                if (disp_model->Breath_Time_Ms > 0)
                { 
                    pwm_breath_ctrl_t pwm_ctl;
                    pwm_ctl.high_time_ms = active_time_ms;
                    pwm_ctl.low_time_ms = inactive_time_ms;
                    pwm_ctl.rise_time_ms = disp_model->Breath_Time_Ms;
                    pwm_ctl.down_time_ms = disp_model->Breath_Time_Ms;
                    led_manager_set_breath(i, &pwm_ctl, OS_FOREVER, NULL);
                }
                else
                {
                    led_manager_set_blink(i, period, active_time_ms, OS_FOREVER, led->Active_Level, NULL);
                }
            }
        }
    }
}

static void led_ctrl_stop_pwm_display(led_display_ctrl_t* disp_ctrl)
{
    CFG_Type_LED_Display_Model*  disp_model = &disp_ctrl->disp_model;
    
    if (led_ctrl_check_visiable(disp_ctrl) == YES)
    {
        CFG_Type_LED_Drive*  led;
        uint8_t i;

        if ((led = get_pwm_led_drive(disp_model)) == NULL)
        {
            return;
        }

        //SYS_LOG_INF("0x%x", disp_model->Display_LEDs);
        #if 0
        gpio_pwm_disable(led->GPIO_Pin, led->Active_Level);
        #else
        
        for (i = 0; i < MAX_LED_BITS; i++)
        {
            u32_t  led_no = (1 << i);
            if(led_no == led->LED_No)
            {
                led_manager_set_blink(i, 0, 0, OS_FOREVER, !(led->Active_Level), NULL);
                led_manager_set_display(i, !(led->Active_Level), OS_FOREVER, NULL);
            }
        }
        #endif
    }
}


static bool led_model_check_limit_time(CFG_Type_LED_Display_Model* disp_model)
{
    /* 
     *  limitless time, return false
     */
    if (disp_model->Flash_Count == 0 ||
        disp_model->Loop_Count  == 0)
    {
        return false;
    }

    if (disp_model->Use_PWM_Control)
    {
        return false;
    }

    //limit time ,return true
    return true;
}

static void led_ctrl_timer_handler(struct k_work *work)
{
    led_display_ctrl_t*  disp_ctrl = 
			CONTAINER_OF(work, led_display_ctrl_t, timer);

    CFG_Type_LED_Display_Model*  disp_model = &disp_ctrl->disp_model;
    
    //SYS_LOG_INF("Model:%d, leds:0x%x", disp_model->Display_Model, disp_model->Display_LEDs);

    /*
     * set to on state when off state end.
     */
    if (disp_ctrl->onoff_state == LED_STATE_OFF)
    {
        led_ctrl_set_state_ex(disp_ctrl, LED_STATE_ON);

        disp_ctrl->onoff_state = LED_STATE_ON;

        k_delayed_work_submit(&disp_ctrl->timer, K_MSEC(disp_model->ON_Time_Ms));
        return;
    }

    
    /*
     * led on state end
     */
    disp_ctrl->flash_count += 1;
    if (disp_model->Flash_Count > 0 && 
         disp_ctrl->flash_count >= disp_model->Flash_Count)
    {
         u32_t  off_time_ms;

        disp_ctrl->loop_count += 1;
        disp_ctrl->flash_count = 0;

        /*
         * stop display control when end of cycle
         */
        if (disp_model->Loop_Count > 0 &&
        disp_ctrl->loop_count >= disp_model->Loop_Count)
        {
            led_ctrl_stop_display(disp_ctrl, disp_ctrl->end_callback);
            return;
        }

        off_time_ms = 
            disp_model->OFF_Time_Ms + 
            disp_model->Loop_Wait_Time_Ms;

        /*
         * cancel timer when off time and circle wait time all is 0. led keep on.
         */
        if (off_time_ms == 0)
        {
            return;
        }

        /* 
         * set led to off state when in waiting circle
         */
        led_ctrl_set_state_ex(disp_ctrl, LED_STATE_OFF);

        disp_ctrl->onoff_state = LED_STATE_OFF;

        k_delayed_work_submit(&disp_ctrl->timer, K_MSEC(off_time_ms));
        return;
    }

    if (disp_model->OFF_Time_Ms == 0)
    {
        /*
         * cancel timer when off time is 0 and limitless flashing, led keep on
         */
        if (disp_model->Flash_Count == 0)
        {
            return;
        }

        /*
         * when off time is 0, keep the led to on state.
        */
        k_delayed_work_submit(&disp_ctrl->timer, K_MSEC(disp_model->ON_Time_Ms));
        return;
    }

    /*
     * set to off state after end of on state.
     */
    led_ctrl_set_state_ex(disp_ctrl, LED_STATE_OFF);

    disp_ctrl->onoff_state = LED_STATE_OFF;

    k_delayed_work_submit(&disp_ctrl->timer, K_MSEC(disp_model->OFF_Time_Ms));

}

static void led_ctrl_start_display(led_display_ctrl_t* disp_ctrl)
{
    CFG_Type_LED_Display_Model*  disp_model = &disp_ctrl->disp_model;

    /*
     * PWM control mode
     */
    if (disp_model->Use_PWM_Control)
    {
        led_ctrl_start_pwm_display(disp_ctrl);
    }
    /*
     * always in off state
     */
    else if (disp_model->ON_Time_Ms == 0)
    {
        led_ctrl_set_state_ex(disp_ctrl, LED_STATE_OFF);
    }
    /*
     * intial set to off state, avoid flash when switch display mode.
     * not always on and not always off,  run this branch
     */
    else if (disp_model->OFF_Time_Ms + disp_model->Loop_Wait_Time_Ms > 0)
    {
        u32_t  time_ms;
        
        led_ctrl_set_state_ex(disp_ctrl, LED_STATE_OFF);

        disp_ctrl->onoff_state = LED_STATE_OFF;

        if (disp_model->OFF_Time_Ms > 0)
        {
            time_ms = disp_model->OFF_Time_Ms;
        }
        else
        {
            time_ms = disp_model->Loop_Wait_Time_Ms;
        }

        if (time_ms > 500)
        {
            time_ms = 500;
        }

        time_ms += disp_model->Delay_Time_Ms;

        //timer to display
         
         k_delayed_work_init(&disp_ctrl->timer, led_ctrl_timer_handler);
         k_delayed_work_submit(&disp_ctrl->timer, K_MSEC(time_ms));
    }
    /*
     * delay light on
     */
    else
    {
        led_ctrl_set_state_ex(disp_ctrl, LED_STATE_OFF);

        disp_ctrl->onoff_state = LED_STATE_OFF;

        k_delayed_work_init(&disp_ctrl->timer, led_ctrl_timer_handler);
        //k_delayed_work_submit(&disp_ctrl->timer, K_MSEC(200 + disp_model->Delay_Time_Ms));
        k_delayed_work_submit(&disp_ctrl->timer, K_MSEC(disp_model->Delay_Time_Ms));
    }
}

static void led_ctrl_stop_display(led_display_ctrl_t* disp_ctrl, 
                                     led_model_end_callback_t end_callback)
{
    led_driver_context_t*  led_driver = led_ctl_get_context();

    CFG_Type_LED_Display_Model*  disp_model = &disp_ctrl->disp_model;

    //SYS_LOG_INF("0x%x", disp_model->Display_LEDs);

    //if (disp_ctrl->timer.work.work != NULL)
    {
        k_delayed_work_cancel(&disp_ctrl->timer);
    }

    if (disp_model->Use_PWM_Control)
    {
        led_ctrl_stop_pwm_display(disp_ctrl);
    }
    else
    {
        led_ctrl_set_state_ex(disp_ctrl, LED_STATE_OFF);
    }
    list_del(&disp_ctrl->node);

    /*
     * when in limited display mode, invok callback in display end.
     */
    if (end_callback != NULL)
    {
        if (led_model_check_limit_time(&disp_ctrl->disp_model))
        {
            led_display_ctrl_t*  t;
            led_display_ctrl_t*  next;

            /*
             *  meanwhile, stop and delete other same model led 
             */
            list_for_each_entry_safe(t, next, &led_driver->disp_ctrl_list, node)
            {
                if ((t != disp_ctrl) &&
                    (t->disp_model.Display_Model == disp_ctrl->disp_model.Display_Model) &&
                    (t->layer_id == disp_ctrl->layer_id)
                )
                {
                    led_ctrl_stop_display(t, NULL);
                }
            }
            end_callback(disp_model->Display_Model, disp_ctrl->layer_id);
        }
    }

    mem_free(disp_ctrl);
}


/*!
 * \brief  control led display in specified mode
 * \n  visiable_leds: use to set or modify current visible and can display leds
 * \n  can controller multi model leds overlap led state display
 */

bool led_ctrl_start_model
(    
    CFG_Type_LED_Display_Model*  cfg_model,
    u32_t                       visiable_leds,
    u32_t                       layer_id,
    led_model_end_callback_t     end_callback
)
{
    //u32_t  irq_flags;

    led_driver_context_t*  led_driver = led_ctl_get_context();
    
    led_display_ctrl_t*  disp_ctrl;
    led_display_ctrl_t*  next;
    int     num_leds;
    int     i, n;

    u32_t  display_leds = cfg_model->Display_LEDs;
    u32_t  disable_leds = cfg_model->Disable_LEDs;

    //SYS_LOG_INF( "%d, 0x%x, 0x%x, 0x%x", cfg_model->Display_Model, 
        //(display_leds | disable_leds), visiable_leds, layer_id);

    //irq_flags = local_irq_save();

    if (cfg_model->Display_Model == LED_DISPLAY_MODEL_NONE)
    {
        goto end;
    }

    list_for_each_entry(disp_ctrl, &led_driver->disp_ctrl_list, node)
    {
        /* 
         * already in display state
         */
        if ((disp_ctrl->disp_model.Display_Model == cfg_model->Display_Model) &&
            (disp_ctrl->layer_id == layer_id))
        {
            if (disp_ctrl->visiable_leds == visiable_leds)
            {
                goto end;
            }

            list_for_each_entry_safe
                (disp_ctrl, next, &led_driver->disp_ctrl_list, node)
            {
                if ((disp_ctrl->disp_model.Display_Model == cfg_model->Display_Model) &&
                    (disp_ctrl->layer_id == layer_id))
                {
                    led_ctrl_stop_display(disp_ctrl, NULL);
                }
            }
            break;
        }
    }
    
    /* 
        * the leds need to display
       */
    num_leds = get_num_leds(display_leds);
    n = 0;

    /* 
     * control single led independently
     */
    for (i = 0; i < MAX_LED_BITS; i++)
    {
        u32_t  led_no = (1 << i);

        if (!(display_leds & led_no))
        {
            continue;
        }

        disable_leds &= ~led_no;

        if (get_led_drive(led_no) == NULL)
        {
            continue;
        }

        disp_ctrl = mem_malloc(sizeof(led_display_ctrl_t));
		if(!disp_ctrl) return false;

        memset(disp_ctrl, 0, sizeof(led_display_ctrl_t));

        disp_ctrl->disp_model = *cfg_model;
        disp_ctrl->disp_model.Display_LEDs = led_no;

        disp_ctrl->visiable_leds = visiable_leds;
        disp_ctrl->layer_id = layer_id;


         /*
          * in last led, set display end callback
          */
        if (n == (num_leds - 1))
        {
            disp_ctrl->end_callback = end_callback;
        }
        
        /*
         * Delay time is extended when controlling multiple leds
         * Water lamp implementation
         */
        if (num_leds > 1)
        {
            disp_ctrl->disp_model.Delay_Time_Ms = n * cfg_model->Delay_Time_Ms;
        }
        
        /*
         * add to display control list
         */
        list_add(&disp_ctrl->node, &led_driver->disp_ctrl_list);
        n += 1;

        led_ctrl_start_display(disp_ctrl);
    }

    /* 
     * the led need to closed
     */
    disp_ctrl = NULL;

    for (i = 0; i < MAX_LED_BITS; i++)
    {
        u32_t  led_no = (1 << i);
    
        if (!(disable_leds & led_no))
        {
            continue;
        }

        if (get_led_drive(led_no) == NULL)
        {
            continue;
        }

        /*
         * the led visiable and need to close 
         */
        if (led_no & visiable_leds)
        {
            led_ctrl_set_state(led_no, LED_STATE_OFF);
        }

         /*
          * leds that shall close didn't need control seperatly, only allocate one control struct to leds that shall close.
          */
        if (disp_ctrl != NULL)
        {
            continue;
        }

        disp_ctrl = mem_malloc(sizeof(led_display_ctrl_t));
		if(!disp_ctrl) return false;

        memset(disp_ctrl, 0, sizeof(led_display_ctrl_t));

        disp_ctrl->disp_model.Display_Model = cfg_model->Display_Model;
        disp_ctrl->disp_model.Display_LEDs  = disable_leds;

        disp_ctrl->disabled      = true;
        disp_ctrl->visiable_leds = visiable_leds;
        disp_ctrl->layer_id = layer_id;

        /* 
         * add to display control list
         */
        list_add(&disp_ctrl->node, &led_driver->disp_ctrl_list);
    }

end:
    //local_irq_restore(irq_flags);
    return true;
}

/*!
 * \brief stop specially model display
 */
void led_ctrl_stop_model(u32_t model_id, u32_t layer_id)
{
    led_driver_context_t*  led_driver = led_ctl_get_context();

    led_display_ctrl_t*  disp_ctrl;
    led_display_ctrl_t*  next;

    if (model_id == LED_DISPLAY_MODEL_NONE)
    {
        return;
    }

    //SYS_LOG_INF("%d, 0x%x", model_id, layer_id);

    list_for_each_entry_safe(disp_ctrl, next, &led_driver->disp_ctrl_list, node)
    {
        if ((model_id == LED_DISPLAY_MODEL_ALL) || 
            ((model_id == disp_ctrl->disp_model.Display_Model) &&
            (disp_ctrl->layer_id == layer_id)))
        {
            led_ctrl_stop_display(disp_ctrl, NULL);
        }
    }

}

/*!
 * \brief check specified led model display state
 * \n when in limit time display and is displaying now ,return true, else return false.
 */
bool led_ctrl_check_model(u32_t model_id, u32_t layer_id)
{
    bool  result = false;
   
    led_driver_context_t*  led_driver = led_ctl_get_context();
    led_display_ctrl_t*    disp_ctrl;

    //u32_t  irq_flags = local_irq_save();
    
    list_for_each_entry(disp_ctrl, &led_driver->disp_ctrl_list, node)
    {
        if (!disp_ctrl->disabled &&
            disp_ctrl->disp_model.Display_Model == model_id && 
            disp_ctrl->layer_id == layer_id)
        {
            /*
             * limited time display mode , and  is displaying now
             */
            if (led_model_check_limit_time(&disp_ctrl->disp_model))
            {
                result = true;
                break;
            }
        }
    }

    //local_irq_restore(irq_flags);

    return result;
}

/*
  * name:led_ctrl_init
  * breaf: read led drivers config.
  */
void led_ctrl_init(void)
{
    led_driver_context_t*  led_driver = led_ctl_get_context();
    led_configs_t  configs;

    app_config_read
    (
        CFG_ID_LED_DRIVES, 
        &configs.cfg_leds, 0, sizeof(CFG_Struct_LED_Drives)
    );

    led_driver->configs = configs;

    INIT_LIST_HEAD(&led_driver->disp_ctrl_list);
}

/*
  * name:led_ctrl_init
  *
  */
void led_ctrl_deinit(void)
{
}



