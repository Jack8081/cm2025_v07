#include "bt_manager_inner.h"
#include "bt_manager.h"
#include "audio_system.h"
#include <property_manager.h>

#define CFG_BTMGR_SAVED_VOLUME	"BTMGR_SAVED_VOLUME"

void bt_manager_init_dev_volume(void)
{
    bt_manager_context_t*  bt_manager = bt_manager_get_context();
    bt_mgr_saved_dev_volume_t*  p = bt_manager->saved_dev_volume;

    property_get(CFG_BTMGR_SAVED_VOLUME, (char *)p, sizeof(*p) * BT_MAX_SAVE_DEV_NUM);
}


void bt_manager_save_dev_volume(void)
{
    bt_manager_context_t*  bt_manager = bt_manager_get_context();
    bt_mgr_saved_dev_volume_t*  p = bt_manager->saved_dev_volume;
	bt_mgr_dev_info_t* dev_info;
    bool  changed = false;
    int     i, j;

    for (j = 0; j < MAX_MGR_DEV; j++) 
    {
        dev_info = &bt_manager->dev[j];
        if ((dev_info->used == 0) || (dev_info->is_tws != 0) || (dev_info->hdl == 0)) {
            continue;
        }

        for (i = 0; i < BT_MAX_SAVE_DEV_NUM; i++)
        {
            if (memcmp(&p[i].addr, &dev_info->addr, sizeof(bd_address_t)) == 0)
            {
                if (p[i].bt_music_vol != dev_info->bt_music_vol ||
                    p[i].bt_call_vol  != dev_info->bt_call_vol)
                {
                    p[i].bt_music_vol = dev_info->bt_music_vol;
                    p[i].bt_call_vol  = dev_info->bt_call_vol;
                    
                    changed = true;
                }
                break;
            }
        }

        if (i >= BT_MAX_SAVE_DEV_NUM)
        {
            for (i = 0; i < BT_MAX_SAVE_DEV_NUM - 1; i++)
            {
                p[i] = p[i+1];
            }
            
            memcpy(&p[i].addr, &dev_info->addr, sizeof(bd_address_t));
            
            p[i].bt_music_vol = dev_info->bt_music_vol;
            p[i].bt_call_vol  = dev_info->bt_call_vol;
            
            changed = true;
        }
	}

    if (changed)
    {
		property_set(CFG_BTMGR_SAVED_VOLUME, (char *)p, sizeof(*p) * BT_MAX_SAVE_DEV_NUM);
		//property_flush(CFG_BTMGR_SAVED_VOLUME);	
    }
}


bool bt_manager_read_dev_volume(bd_address_t *addr, u8_t* bt_music_vol, u8_t* bt_call_vol)
{
    bt_manager_context_t*  bt_manager = bt_manager_get_context();
    bt_mgr_saved_dev_volume_t*  p = bt_manager->saved_dev_volume;
	btmgr_sync_ctrl_cfg_t *sync_ctrl_config = bt_manager_get_sync_ctrl_config();
    int  i;
    
    for (i = 0; i < BT_MAX_SAVE_DEV_NUM; i++)
    {
        if (memcmp(&p[i].addr, addr, sizeof(bd_address_t)) == 0)
        {
            *bt_music_vol = p[i].bt_music_vol;
            *bt_call_vol  = p[i].bt_call_vol;
            return true;
        }
    }

    *bt_music_vol = sync_ctrl_config->bt_music_default_volume_ex;
    *bt_call_vol  = sync_ctrl_config->bt_call_default_volume;
    return false;
}


void bt_mgr_add_dev_info(bd_address_t *addr, uint16_t hdl)
{
	int i;
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_manager->dev[i].used && !memcmp(&bt_manager->dev[i].addr, addr, sizeof(bd_address_t))) {
			SYS_LOG_WRN("Already exist!\n");
			goto Exit_Alreay_exit;
		}
	}

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_manager->dev[i].used == 0) {
			bt_manager->dev[i].used = 1;
			bt_manager->dev[i].hdl = hdl;
			memcpy(&bt_manager->dev[i].addr, addr, sizeof(bd_address_t));
			goto Exit;
		}
	}

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_manager->dev[i].timeout_disconnected &&
			bt_manager->dev[i].hdl == 0) {
			bt_manager->dev[i].used = 1;
			bt_manager->dev[i].hdl = hdl;
			memcpy(&bt_manager->dev[i].addr, addr, sizeof(bd_address_t));
			SYS_LOG_WRN("use timeout_disconnected dev info!\n");		
			goto Exit;
		}
	}

	SYS_LOG_WRN("Without new dev info!\n");
	return;

Exit_Alreay_exit:
	bt_manager->dev[i].hdl = hdl;

Exit:
#ifdef CONFIG_BT_HID
	os_delayed_work_init(&bt_manager->dev[i].hid_delay_work, bt_manager_hid_delay_work);
#endif
	os_delayed_work_init(&bt_manager->dev[i].a2dp_stop_delay_work, bt_manager_a2dp_stop_delay_proc);
	os_delayed_work_init(&bt_manager->dev[i].hfp_vol_to_remote_timer, bt_manager_hfp_sync_origin_vol_proc);
	os_delayed_work_init(&bt_manager->dev[i].resume_play_delay_work, bt_manager_resume_play_proc);
	os_delayed_work_init(&bt_manager->dev[i].sco_disconnect_delay_work, bt_manager_hfp_sco_disconnect_proc);
	os_delayed_work_init(&bt_manager->dev[i].profile_disconnected_delay_work, bt_manager_profile_disconnected_delay_proc);

	bt_manager->dev[i].new_dev = !bt_manager_read_dev_volume(addr, 
		&bt_manager->dev[i].bt_music_vol, &bt_manager->dev[i].bt_call_vol);

	SYS_LOG_INF("hdl:0x%x, new_dev:%d, bt_music_vol:%d, bt_call_vol:%d\n",
		hdl, bt_manager->dev[i].new_dev,
		bt_manager->dev[i].bt_music_vol, bt_manager->dev[i].bt_call_vol);
	return;
}

void bt_mgr_free_dev_info(struct bt_mgr_dev_info *info)
{
#ifdef CONFIG_BT_HID
	os_delayed_work_cancel(&info->hid_delay_work);
#endif
	os_delayed_work_cancel(&info->a2dp_stop_delay_work);
	os_delayed_work_cancel(&info->hfp_vol_to_remote_timer);	
	os_delayed_work_cancel(&info->resume_play_delay_work);	
	os_delayed_work_cancel(&info->sco_disconnect_delay_work);	
	os_delayed_work_cancel(&info->profile_disconnected_delay_work);	

	if (!info->timeout_disconnected)
	{
		memset(info, 0, sizeof(struct bt_mgr_dev_info));
		return;	
	}

	bd_address_t bd_addr;
	u8_t  need_resume_play			  = info->need_resume_play;
	u8_t  timeout_disconnected		  = info->timeout_disconnected;
	memcpy(&bd_addr, &info->addr, sizeof(bd_address_t));
	memset(info, 0, sizeof(struct bt_mgr_dev_info));

	memcpy(&info->addr, &bd_addr, sizeof(bd_address_t));
	info->need_resume_play			  = need_resume_play;
	info->timeout_disconnected		  = timeout_disconnected;
	info->used = 1;
	SYS_LOG_INF("need_resume_play:%d,timeout_disconnected:%d",need_resume_play,timeout_disconnected);
}

struct bt_mgr_dev_info *bt_mgr_find_dev_info(bd_address_t *addr)
{
	int i;
	struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_manager->dev[i].used && !memcmp(&bt_manager->dev[i].addr, addr, sizeof(bd_address_t))) {
			return &bt_manager->dev[i];
		}
	}

	return NULL;
}

struct bt_mgr_dev_info *bt_mgr_find_dev_info_by_hdl(uint16_t hdl)
{
	int i;
	struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_manager->dev[i].used && bt_manager->dev[i].hdl == hdl) {
			return &bt_manager->dev[i];
		}
	}

	return NULL;
}

bt_mgr_dev_info_t* bt_mgr_get_a2dp_active_dev(void)
{
	uint16_t hdl = btif_a2dp_get_active_hdl();

	return bt_mgr_find_dev_info_by_hdl(hdl);
}


bt_mgr_dev_info_t* bt_mgr_get_hfp_active_dev(void)
{
	uint16_t hdl = btif_hfp_get_active_hdl();

	return bt_mgr_find_dev_info_by_hdl(hdl);
}

bool btmgr_is_snoop_link_created(void)
{
	uint8_t i;
	struct bt_manager_context_t *bt_manager = bt_manager_get_context();

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_manager->dev[i].used && (bt_manager->dev[i].snoop_role != BTSRV_SNOOP_NONE)) {
			return true;
		}
	}

	return false;
}
