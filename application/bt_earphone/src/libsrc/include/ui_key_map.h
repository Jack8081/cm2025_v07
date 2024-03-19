#ifndef _UI_KEY_MAP_H_
#define _UI_KEY_MAP_H_

#include <mem_manager.h>
#include <ui_manager.h>
#include <config.h>


typedef struct {
    /** key function define */
    uint32_t key_func;

    /** in whicth state, key_func valid */
    uint32_t func_state;
} ui_key_func_t;


ui_key_map_t* ui_key_map_create(const ui_key_func_t* ptable, int items);

int ui_key_map_delete(ui_key_map_t* key_maps);

int ui_key_check_dev_type(uint32_t dev_type);

void ui_key_filter(bool filter);

int ui_app_setting_key_map_init(void);
int ui_app_setting_key_map_update(int num, CFG_Type_Key_Func_Map* key_func_map);
void ui_app_setting_key_map_tws_sync(void* app_key_map_info);

int ui_key_notify_remap_keymap(void);


#endif /*_UI_KEY_MAP_H_*/

