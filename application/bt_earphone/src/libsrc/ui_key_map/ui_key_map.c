
#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "uikeymap"
#endif


#include <os_common_api.h>
#include "ui_key_map.h"
#include "app_config.h"
#include "input_manager.h"
#include <bt_manager.h>
#include "btservice_api.h"
#include "app_common.h"
#include "app_ui.h"
#include "property_manager.h"


#define CFG_MAX_APP_KEY_FUNC_MAPS 16
#define CFG_APP_KEYMAP   "APP_KEYMAP_INFO"

typedef struct
{
    u8_t  is_valid;
    u8_t  tws_synced;
	CFG_Type_Key_Func_Map map[CFG_MAX_APP_KEY_FUNC_MAPS];  // 映射
}app_key_map_info_t;

typedef struct
{
    CFG_Struct_Key_Func_Maps*        cfg_key_func_maps;
    CFG_Struct_Combo_Key_Func_Maps*  cfg_combo_key_maps;
    CFG_Struct_Customed_Key_Sequence* cfg_customed_key_seq;
    unsigned short   filter_key;

} ui_key_context_t;

static ui_key_context_t  ui_key_context;
static app_key_map_info_t* local_app_key_map_info;

static inline ui_key_context_t* ui_key_get_context(void)
{
    return &ui_key_context;
}

int ui_key_notify_remap_keymap(void)
{
    if (!local_app_key_map_info)
        return -1;
	
    /*notify app remap key func*/
    struct app_msg msg = {0};
    msg.type = MSG_APP_SYNC_KEY_MAP;		
    send_async_msg("main", &msg);
    return 0;
}

int ui_app_setting_key_map_init(void)
{
    int ret;
    local_app_key_map_info = app_mem_malloc(sizeof(app_key_map_info_t));
    if (!local_app_key_map_info)
    {
        return -1;
    }	

    ret = property_get(CFG_APP_KEYMAP, (char*)local_app_key_map_info, sizeof(app_key_map_info_t));
    if (ret != sizeof(app_key_map_info_t))
    {
        app_mem_free(local_app_key_map_info);
        local_app_key_map_info = NULL;
    }
    return ret;
}

int ui_app_setting_key_map_update(int num, CFG_Type_Key_Func_Map* key_func_map)
{
    int i,j;
    bool update = false;
	int dev_role = bt_manager_tws_get_dev_role();
    int match = 0;
    if (num > CFG_MAX_APP_KEY_FUNC_MAPS)
    {
        SYS_LOG_WRN("num:%d > %d",num, CFG_MAX_APP_KEY_FUNC_MAPS);
    }
	
    if (!local_app_key_map_info)
    {
        local_app_key_map_info = app_mem_malloc(sizeof(app_key_map_info_t));
        if (!local_app_key_map_info)
        {
            return -1;
        }
			
    }
	CFG_Type_Key_Func_Map* app_key_map = (CFG_Type_Key_Func_Map*)local_app_key_map_info->map;
	
    for (i = 0; i < num; i++)
    {
	    if (key_func_map[i].Key_Func  == KEY_FUNC_NONE ||
        	key_func_map[i].Key_Value == VKEY_NONE     ||
        	key_func_map[i].Key_Event == KEY_EVENT_NONE)
        {
            continue;
        }
			
        SYS_LOG_INF("src remap :0x%08x, 0x%08x,0x%08x,0x%08x\n",
					key_func_map[i].Key_Func,key_func_map[i].Key_Value,
					key_func_map[i].Key_Event,key_func_map[i].LR_Device); 

        update = false;
        for(j=0; j< CFG_MAX_APP_KEY_FUNC_MAPS; j++)
        {
            if(app_key_map[j].Key_Value == key_func_map[i].Key_Value &&
               app_key_map[j].Key_Event == key_func_map[i].Key_Event &&
               app_key_map[j].LR_Device == key_func_map[i].LR_Device)
            {
                if (app_key_map[j].Key_Func != key_func_map[i].Key_Func)
                {
                    app_key_map[j].Key_Func	= key_func_map[i].Key_Func;
                    update = true;
					match++;
					
                    SYS_LOG_INF("xx remap :0x%08x, 0x%08x,0x%08x,0x%08x\n",
                        app_key_map[j].Key_Func,app_key_map[j].Key_Value,
                        app_key_map[j].Key_Event,app_key_map[j].LR_Device); 						
                }
                break;
            }
        }

		if (!update && j >= CFG_MAX_APP_KEY_FUNC_MAPS)
		{
            for (j = 0; j < CFG_MAX_APP_KEY_FUNC_MAPS; j++)
            {
                if (app_key_map[j].Key_Func  == KEY_FUNC_NONE ||
                    app_key_map[j].Key_Value == VKEY_NONE	  ||
                    app_key_map[j].Key_Event == KEY_EVENT_NONE)
                {
                    *(&app_key_map[j]) = *(&key_func_map[i]);
                    update = true;
					match++;
					break;
                }
            }
        }
    }
	SYS_LOG_INF("%d,%d", match, num);
    if (match)
    {
        local_app_key_map_info->is_valid = 1;
        local_app_key_map_info->tws_synced = 0;
        if (dev_role != BTSRV_TWS_NONE)
        {
            local_app_key_map_info->tws_synced = 1;
        }
        property_set(CFG_APP_KEYMAP, (char*)local_app_key_map_info, sizeof(app_key_map_info_t));

        if (dev_role != BTSRV_TWS_NONE){
            ui_app_setting_key_map_tws_sync(NULL);
        }
        /*notify app remap key func*/
		ui_key_notify_remap_keymap();
	}

	return 0;
}

static int ui_app_setting_sync_key_map(app_key_map_info_t* local_map, app_key_map_info_t* remote_map)
{
    int dev_role = bt_manager_tws_get_dev_role();
    bool  use_remote = false;
    int ret = -1;

    if (local_map->is_valid == 0)
    {
        if (remote_map->is_valid == 1)
        {
            use_remote = true;
        }
    }
    else if (local_map->tws_synced == 1)
    {
        if (remote_map->is_valid   == 1 &&
            remote_map->tws_synced == 0)
        {
            use_remote = true;
        }
    }
    else
    {
        if (remote_map->is_valid   == 1 &&
            remote_map->tws_synced == 0)
        {
            if (dev_role == BTSRV_TWS_SLAVE)
            {
                use_remote = true;
            }
        }
    }

    if (use_remote)
    {
        *local_map = *remote_map;
        local_map->tws_synced = 1;
        ret = 1;
    }
    else
    {
        if (local_map->is_valid   == 1 &&
            local_map->tws_synced == 0)
        {
            local_map->tws_synced = 1;
            ret = 0;
        }
    }
	
    SYS_LOG_INF("use_remote:%d, %d", use_remote,ret);
    return ret;
}


void ui_app_setting_key_map_tws_sync(void* app_key_map_info)
{
    /* 本机同步至对方?
     */
    if (app_key_map_info == NULL)
    {
        if (!local_app_key_map_info)
        {
            return;
        }

        bt_manager_tws_send_message
        (
            TWS_LONG_MSG_EVENT, TWS_EVENT_APP_SETTING_KEY_MAP,(char*)local_app_key_map_info, sizeof(app_key_map_info_t)
        );
    }
    /* 对方同步至本机
     */
    else
    {
        if (!local_app_key_map_info)
        {
            local_app_key_map_info = app_mem_malloc(sizeof(app_key_map_info_t));
            if (!local_app_key_map_info)
            {
                return;
            }			
        }
        app_key_map_info_t*  remote_settings = (app_key_map_info_t*)app_key_map_info;
        app_key_map_info_t*	local_settings = (app_key_map_info_t*)local_app_key_map_info;

        int retval =ui_app_setting_sync_key_map(local_settings, remote_settings);
        if (retval >= 0)
        {
            property_set(CFG_APP_KEYMAP, (char*)local_settings, sizeof(app_key_map_info_t));
            if (retval == 1)
            {
                /*notify app remap key func*/
                ui_key_notify_remap_keymap();
            }
        }
    }
}

static uint32_t ui_key_get_customed_key_seq(uint32_t key_event)
{
    ui_key_context_t* ui_key = ui_key_get_context();
    int i;

    if (!(key_event & (
        KEY_EVENT_CUSTOMED_SEQUENCE_1 | 
        KEY_EVENT_CUSTOMED_SEQUENCE_2)))
    {
        return key_event;
    }

    if (ui_key->cfg_customed_key_seq == NULL)
    {
        ui_key->cfg_customed_key_seq = 
            app_mem_malloc(sizeof(CFG_Struct_Customed_Key_Sequence));
        if(!ui_key->cfg_customed_key_seq)
        {
            return KEY_EVENT_NONE;
        }
        
        app_config_read(
            CFG_ID_CUSTOMED_KEY_SEQUENCE, 
            ui_key->cfg_customed_key_seq, 0, sizeof(CFG_Struct_Customed_Key_Sequence)
        );
    }

    for (i = 0; i < 2; i++)
    {
        CFG_Type_Customed_Key_Sequence* seq = 
            &ui_key->cfg_customed_key_seq->Customed_Key_Sequence[i];
        
        if (key_event == seq->Key_Sequence) {
            return (seq->Key_Event_1 | seq->Key_Event_2);
        }
    }

    return KEY_EVENT_NONE;
}

static int ui_key_func_get_config(int cfg_index, CFG_Type_Key_Func_Map* cfg_map)
{
    ui_key_context_t*  ui_manager = ui_key_get_context();
    audio_manager_configs_t* cfg =	audio_channel_get_config();
    int i,j;
    bool remap = false;

    if (ui_manager->cfg_key_func_maps == NULL)
    {
        ui_manager->cfg_key_func_maps = 
            app_mem_malloc(sizeof(CFG_Struct_Key_Func_Maps));
        if(!ui_manager->cfg_key_func_maps)
            return NO;
        
        app_config_read
        (
            CFG_ID_KEY_FUNC_MAPS, 
            ui_manager->cfg_key_func_maps, 0, sizeof(CFG_Struct_Key_Func_Maps)
        );
		
        if (local_app_key_map_info && local_app_key_map_info->is_valid)
        {
            CFG_Type_Key_Func_Map* app_key_map = (CFG_Type_Key_Func_Map*)local_app_key_map_info->map;
            CFG_Type_Key_Func_Map* config_key_map = (CFG_Type_Key_Func_Map*)ui_manager->cfg_key_func_maps->Map;
			
            for (i = 0; i < CFG_MAX_APP_KEY_FUNC_MAPS; i++)
            {
                if (app_key_map[i].Key_Func  == KEY_FUNC_NONE ||
                    app_key_map[i].Key_Value == VKEY_NONE	  ||
                    app_key_map[i].Key_Event == KEY_EVENT_NONE)
                {
                    continue;
				}
		
                SYS_LOG_INF("remap :0x%08x, 0x%08x,0x%08x,0x%08x\n",
						app_key_map[i].Key_Func,app_key_map[i].Key_Value,
						app_key_map[i].Key_Event,app_key_map[i].LR_Device); 

                remap = false;
                for(j=0; j< CFG_MAX_KEY_FUNC_MAPS; j++)
                {		
                    if (config_key_map[j].Key_Value == app_key_map[i].Key_Value &&
                        config_key_map[j].Key_Event == app_key_map[i].Key_Event &&
                        config_key_map[j].Key_Func != app_key_map[i].Key_Func)
                    {						
                        SYS_LOG_INF("xx src :0x%08x, 0x%08x,0x%08x,0x%08x\n",
								config_key_map[j].Key_Func,config_key_map[j].Key_Value,
								config_key_map[j].Key_Event,config_key_map[j].LR_Device);						

						/* L 左设备标识?
						 */
						if (app_key_map[i].LR_Device & KEY_DEVICE_CHANNEL_L)
						{

							if (cfg->hw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_L ||
								cfg->sw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_L)
							{
                                config_key_map[j].LR_Device	&= ~(KEY_DEVICE_CHANNEL_L|KEY_DEVICE_CHANNEL_R);					
								config_key_map[j].LR_Device |= KEY_DEVICE_CHANNEL_L;
								config_key_map[j].Key_Func = app_key_map[i].Key_Func;								
							}
						}
						
						/* R 右设备标识?
						 */
						else if (app_key_map[i].LR_Device & KEY_DEVICE_CHANNEL_R) 
						{
							if (cfg->hw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_R ||
								cfg->sw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_R)
							{
                                config_key_map[j].LR_Device	&= ~(KEY_DEVICE_CHANNEL_L|KEY_DEVICE_CHANNEL_R);												
								config_key_map[j].LR_Device |= AUDIO_DEVICE_CHANNEL_R;
								config_key_map[j].Key_Func = app_key_map[i].Key_Func;
							}
						}
                        remap = true;
                        break;
                    }
                }
			
                if (!remap)
                {
                    for (j = 0; j < CFG_MAX_KEY_FUNC_MAPS; j++)
					{
                        if (config_key_map[j].Key_Func  == KEY_FUNC_NONE ||
                            config_key_map[j].Key_Value == VKEY_NONE	  ||
                            config_key_map[j].Key_Event == KEY_EVENT_NONE)
                        {
                            SYS_LOG_INF("xxxsrc :0x%08x, 0x%08x,0x%08x,0x%08x\n",
									config_key_map[j].Key_Func,config_key_map[j].Key_Value,
									config_key_map[j].Key_Event,config_key_map[j].LR_Device);	

                            *(&config_key_map[j]) = *(&app_key_map[i]);
                            remap = true;
							break;
                        }
                    }
                }

                if(!remap){
                    SYS_LOG_WRN("remap Fail:0x%08x, 0x%08x,0x%08x,0x%08x\n",
						app_key_map[i].Key_Func,app_key_map[i].Key_Value,
						app_key_map[i].Key_Event,app_key_map[i].LR_Device);				
                }else{
                    SYS_LOG_INF("remap :0x%08x, 0x%08x,0x%08x,0x%08x\n",
							config_key_map[j].Key_Func,config_key_map[j].Key_Value,
							config_key_map[j].Key_Event,config_key_map[j].LR_Device); 					
				}					
            }
        }
    }

    *cfg_map = ui_manager->cfg_key_func_maps->Map[cfg_index];
    
    if (cfg_map->Key_Func  == KEY_FUNC_NONE ||
        cfg_map->Key_Value == VKEY_NONE     ||
        cfg_map->Key_Event == KEY_EVENT_NONE)
    {
        return NO;
    }

    if (cfg_map->Key_Event & (
        KEY_EVENT_CUSTOMED_SEQUENCE_1 | 
        KEY_EVENT_CUSTOMED_SEQUENCE_2))
    {
        cfg_map->Key_Event = ui_key_get_customed_key_seq(cfg_map->Key_Event);
        
        if (cfg_map->Key_Event == KEY_EVENT_NONE) {
            return NO;
        }
    }

    return YES;
}

static bool ui_combo_key_get_config(int cfg_index, CFG_Type_Combo_Key_Map* cfg_map)
{
    ui_key_context_t* ui_key = ui_key_get_context();

    if (ui_key->cfg_combo_key_maps == NULL)
    {
        ui_key->cfg_combo_key_maps = 
            app_mem_malloc(sizeof(CFG_Struct_Combo_Key_Func_Maps));
        if (!ui_key->cfg_combo_key_maps)
        {
            return false;
        }
        
        app_config_read
        (
            CFG_ID_COMBO_KEY_FUNC_MAPS, 
            ui_key->cfg_combo_key_maps, 0, sizeof(CFG_Struct_Combo_Key_Func_Maps)
        );
    }

    *cfg_map = ui_key->cfg_combo_key_maps->Map[cfg_index];
    
    if (cfg_map->Key_Func    == KEY_FUNC_NONE ||
        cfg_map->Key_Value_1 == VKEY_NONE     ||
        cfg_map->Key_Value_2 == cfg_map->Key_Value_1 ||
        cfg_map->Key_Event   == KEY_EVENT_NONE)
    {
        return false;
    }
    
    if (cfg_map->Key_Event & (
        KEY_EVENT_CUSTOMED_SEQUENCE_1 | 
        KEY_EVENT_CUSTOMED_SEQUENCE_2))
    {
        cfg_map->Key_Event = ui_key_get_customed_key_seq(cfg_map->Key_Event);
        
        if (cfg_map->Key_Event == KEY_EVENT_NONE) {
            return false;
        }
    }
    
    return true;
}

static void ui_key_func_map_end(void)
{
    ui_key_context_t*  ui_manager = ui_key_get_context();

    os_sched_lock();

    if (ui_manager->cfg_key_func_maps != NULL)
    {
        app_mem_free(ui_manager->cfg_key_func_maps);
        ui_manager->cfg_key_func_maps = NULL;
    }
    
    if (ui_manager->cfg_combo_key_maps != NULL)
    {
        app_mem_free(ui_manager->cfg_combo_key_maps);
        ui_manager->cfg_combo_key_maps = NULL;
    }
    
    if (ui_manager->cfg_customed_key_seq != NULL)
    {
        app_mem_free(ui_manager->cfg_customed_key_seq);
        ui_manager->cfg_customed_key_seq = NULL;
    }
    os_sched_unlock();
}

ui_key_map_t* ui_key_map_create(const ui_key_func_t* ptable, int items)
{
    int i, j;
    ui_key_map_t* keymaps = NULL;
    int match = 0;
    int size;
	
    SYS_LOG_DBG("items:%d", items);
#if 0	
    for(j=0; j< items; j++)
    {
        printf("key_func:0x%08x, func_state:0x%08x\n",
					ptable[j].key_func,ptable[j].func_state);
    }
#endif	
    for (i = 0; i < CFG_MAX_KEY_FUNC_MAPS; i++)
    {
        CFG_Type_Key_Func_Map  cfg_map;
        
        if (!ui_key_func_get_config(i, &cfg_map))
        {
            continue;
        }
        
		for(j=0; j< items; j++)
		{
			if(cfg_map.Key_Func == ptable[j].key_func)
			{
				match ++;
			}
		}
    }
    
    for (i = 0; i < CFG_MAX_COMBO_KEY_MAPS; i++)
    {
        CFG_Type_Combo_Key_Map cfg_map;
        
        if (!ui_combo_key_get_config(i, &cfg_map))
            continue;
        
		for (j = 0; j < items; j++)
		{
			if (cfg_map.Key_Func == ptable[j].key_func)
				match += 1;
		}
    }

    if (!match)
        goto end;	

	//最后一项，保存KEY_RESERVED，解决ui_manager_dispatch_key_event访问数组越界.
    size = (match+1)*sizeof(ui_key_map_t);
    keymaps = app_mem_malloc(size);
	if(!keymaps)
		goto end;
    memset(keymaps, 0, size);
	keymaps[match].key_val = KEY_RESERVED;

    int index = 0;	
    for (i = 0; i < CFG_MAX_KEY_FUNC_MAPS; i++)
    {
        CFG_Type_Key_Func_Map  cfg_map;
        
        if (!ui_key_func_get_config(i, &cfg_map))
        {
            continue;
        }
        
		for(j=0; j< items; j++)
		{
			if(cfg_map.Key_Func == ptable[j].key_func)
			{
				keymaps[index].key_val = cfg_map.Key_Value;
				keymaps[index].key_type = (cfg_map.Key_Event << 16);
				keymaps[index].app_state = ptable[j].func_state | (cfg_map.LR_Device<<28);
				keymaps[index].app_msg =  ptable[j].key_func;
				index ++;
			}
		}
    }

    for (i = 0; i < CFG_MAX_COMBO_KEY_MAPS; i++)
    {
        CFG_Type_Combo_Key_Map cfg_map;
        
        if (!ui_combo_key_get_config(i, &cfg_map))
            continue;
        
		for (j = 0; j < items; j++)
		{
			if (cfg_map.Key_Func == ptable[j].key_func)
            {
                /* combo key order: high byte small, low byte big */
                keymaps[index].key_val = (cfg_map.Key_Value_1 < cfg_map.Key_Value_2) ?
                    ((cfg_map.Key_Value_1 << 8) | cfg_map.Key_Value_2) :
                    ((cfg_map.Key_Value_2 << 8) | cfg_map.Key_Value_1);
                
                keymaps[index].key_type  = cfg_map.Key_Event << 16;
                keymaps[index].app_state = ptable[j].func_state | (cfg_map.LR_Device << 28);
                keymaps[index].app_msg   = ptable[j].key_func;
                index += 1;
            }
		}
    }

    if (index != match)
    {
        SYS_LOG_ERR("no match.");
    }
    else
    {
        SYS_LOG_DBG("match:%d", match);
#if 0
        for(j=0; j< match+1; j++)
        {
            printf("key_val:0x%08x, key_type:0x%08x, app_state:0x%08x, app_msg:0x%08x, key_policy:0x%08x\n",
							keymaps[j].key_val, keymaps[j].key_type, keymaps[j].app_state,
							keymaps[j].app_msg, keymaps[j].key_policy);
        }
#endif		
    }
	
end:
    ui_key_func_map_end();
    return keymaps;
}



int ui_key_map_delete(ui_key_map_t* key_maps)
{
    if (key_maps){
        app_mem_free(key_maps);
    }
    return 0;
}



/* 检查按键设备类型
 */
int ui_key_check_dev_type(uint32_t dev_type)
{
    int dev_role = bt_manager_tws_get_dev_role();
    int paired = 0;

    audio_manager_configs_t* cfg =  audio_channel_get_config();

    if ((dev_role == BTSRV_TWS_MASTER) || (dev_role == BTSRV_TWS_SLAVE))
    {
       paired = 1;
    }

    /* 已组对?
     */
    if ((dev_type & KEY_DEVICE_TWS_PAIRED) &&
        !(dev_type & KEY_DEVICE_TWS_UNPAIRED))
    {
        if (paired == 0)
        {
            return NO;
        }
    }

    /* 未组对?
     */
    if ((dev_type & KEY_DEVICE_TWS_UNPAIRED) &&
        !(dev_type & KEY_DEVICE_TWS_PAIRED))
    {
        if (paired != 0)
        {
            return NO;
        }
    }

    /* L 左设备标识?
     */
    if ((dev_type & KEY_DEVICE_CHANNEL_L) &&
        !(dev_type & KEY_DEVICE_CHANNEL_R))
    {
        if (cfg->hw_sel_dev_channel != AUDIO_DEVICE_CHANNEL_L &&
            cfg->sw_sel_dev_channel != AUDIO_DEVICE_CHANNEL_L)
        {
            return NO;
        }
    }

    /* R 右设备标识?
     */
    if ((dev_type & KEY_DEVICE_CHANNEL_R) &&
        !(dev_type & KEY_DEVICE_CHANNEL_L))
    {
        if (cfg->hw_sel_dev_channel != AUDIO_DEVICE_CHANNEL_R &&
            cfg->sw_sel_dev_channel != AUDIO_DEVICE_CHANNEL_R)
        {
            return NO;
        }
    }

    return YES;
}

/*!
 * \brief 设置或取消按键过滤
 */
void ui_key_filter(bool filter)
{
    ui_key_context_t*  manager = ui_key_get_context();
	
    /* 设置过滤?
     */
    if (filter)
    {
        if (manager->filter_key == 0)
        {
            SYS_LOG_INF("%d", filter);
			input_manager_lock();	
        }

        manager->filter_key += 1;
    }
    /* 取消过滤?
     */
    else if (manager->filter_key > 0)
    {
        manager->filter_key -= 1;

        if (manager->filter_key == 0)
        {
            SYS_LOG_INF("%d", filter);
			input_manager_unlock();	
        }
    }
}




