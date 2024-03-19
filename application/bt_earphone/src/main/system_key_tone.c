/*!
 * \file      system_key_tone.c
 * \brief     按键音播放管理
 * \details   
 * \author    
 * \date      
 * \copyright Actions
 */

#include "system_app.h"
#include <tts_manager.h>
#include <bt_manager.h>

static char  cur_keytone_name[CFG_MAX_TONE_NAME_LEN + CFG_MAX_TONE_FMT_LEN];

typedef struct{
	uint64_t bt_clk;
	u8_t key_tone;	
}tws_tone_t;

static void send_key_tone_play(u8_t  tone_id)
{
	struct app_msg msg = {0};

	msg.type = MSG_APP_KEY_TONE_PLAY;
	msg.value = tone_id;
	send_async_msg(APP_ID_MAIN, &msg);
}


/* 按键音是否正在播放
 */
bool system_key_tone_is_playing(void)
{
    char*  playing_tone = (char*)tip_manager_get_playing_filename();

    if (playing_tone == NULL)
    {
        return false;
    }

    if (playing_tone[0] == 0 || cur_keytone_name[0] == 0)
    {
        return false;
    }
	
    if (strcmp(playing_tone, cur_keytone_name) == 0)
    {
        return true;
    }

    return false;
}


void system_key_tone_on_event_down(uint32_t vkey)
{
    system_app_context_t*  manager = system_app_get_context();
    system_configs_t*  sys_configs = system_get_configs();	

    do
    {
        CFG_Struct_Key_Tone*  cfg = &sys_configs->cfg_key_tone;
        
        u8_t  key_tone = cfg->Key_Tone_Select;

        /* 敲击按键音?
         */
        if (vkey == TAP_KEY)
        {
            key_tone = sys_configs->tap_key_tone;
        }

        if (manager->sys_status.disable_audio_tone || 
            manager->sys_status.disable_key_tone)
        {
            break;
        }

        if (key_tone == TONE_NONE)
        {
            break;
        }

        /* 还有按键音等待播放? 最多支持 5 击按键音
         */
        if (manager->key_tone_play_msg_count >= 5)
        {
            break;
        }

        manager->key_tone_play_msg_count += 1;
        send_key_tone_play(key_tone);
    }
    while (0);
}


void system_key_tone_on_event_long(uint32_t event)
{
    system_app_context_t*  manager = system_app_get_context();

    do
    {
        CFG_Struct_Key_Tone*  cfg = &manager->sys_configs.cfg_key_tone;
        
        u8_t  key_tone = NONE;

        if (manager->sys_status.disable_audio_tone || 
            manager->sys_status.disable_key_tone)
        {
            break;
        }
        
        if (event & KEY_EVENT_LONG_PRESS)
        {
            key_tone = cfg->Long_Key_Tone_Select;
        }
        else if (event & KEY_EVENT_LONG_LONG_PRESS)
        {
            key_tone = cfg->Long_Long_Key_Tone_Select;
        }
        else if (event & KEY_EVENT_VERY_LONG_PRESS)
        {
            key_tone = cfg->Very_Long_Key_Tone_Select;
        }
        
        if (key_tone == NONE)
        {
            break;
        }
        
        manager->key_tone_play_msg_count += 1;

        send_key_tone_play(key_tone);

    }
    while (0);
}


/* 播放按键音
 */
void system_key_tone_play(u8_t key_tone)
{
    system_app_context_t*  manager = system_app_get_context();
	CFG_Struct_TWS_Sync*  cfg = &manager->sys_configs.cfg_tws_sync;
    int  i;
	uint64_t bt_clk = 0;
    
    if (manager->sys_status.disable_audio_tone || 
        manager->sys_status.disable_key_tone)
    {
        goto end;
    }
    
    SYS_LOG_DBG("%d", key_tone);

    /* TWS 同步按键音?
     */
    if (cfg->Sync_Mode & TWS_SYNC_KEY_TONE)
    {
        if (bt_manager_tws_get_dev_role() >= BTSRV_TWS_MASTER) 
        {
            tws_runtime_observer_t * observer = btif_tws_get_tws_observer();
            bt_manager_wait_tws_ready_send_sync_msg(APP_WAIT_TWS_READY_SEND_SYNC_TIMEOUT);
            bt_clk = observer->get_bt_clk_us();
            if(bt_clk != UINT64_MAX)
                bt_clk += 100000; 	//100ms

            tws_tone_t params;
            params.key_tone = key_tone;
            params.bt_clk = bt_clk;
            SYS_LOG_INF("%d, %u_%u",
                key_tone, (uint32_t)((bt_clk >> 32) & 0xffffffff), (uint32_t)(bt_clk & 0xffffffff));
            bt_manager_tws_send_message
            (
                TWS_USER_APP_EVENT,TWS_EVENT_KEY_TONE_PLAY, 
                (uint8_t*)&params,sizeof(params)
            );
        }
    }

    /* 连续播放按键音时间隔一点时间?
     */
    for (i = 0; i < 10; i++)
    {
        if (!system_key_tone_is_playing())
        {
            break;
        }

        os_sleep(10);
    }

	memset(cur_keytone_name,0,sizeof(cur_keytone_name));
    if (audio_tone_get_name(key_tone, cur_keytone_name) == NULL)
    {
		memset(cur_keytone_name,0,sizeof(cur_keytone_name));    
        goto end;
    }

    tip_manager_play(cur_keytone_name, bt_clk);
end:
    if (manager->key_tone_play_msg_count > 0)
    {
        manager->key_tone_play_msg_count -= 1;
    }
}

void system_tws_key_tone_play(void* tws_tone)
{
    system_app_context_t*  manager = system_app_get_context();
    int  i;
	tws_tone_t* tws_key_tone = (tws_tone_t*)tws_tone;
    uint64_t bt_clk = 0;

    if (manager->sys_status.disable_audio_tone || 
        manager->sys_status.disable_key_tone   ||
        manager->sys_status.in_power_off_stage)
    {
        SYS_LOG_INF("%d, DISABLED", tws_key_tone->key_tone);
    }
    else
    {
		tws_runtime_observer_t * observer = btif_tws_get_tws_observer();
		bt_clk = observer->get_bt_clk_us();
		SYS_LOG_INF("key_tone:%d curr_time:%u_%u play_time:%u_%u",
			tws_key_tone->key_tone, (uint32_t)((bt_clk >> 32) & 0xffffffff), (uint32_t)(bt_clk & 0xffffffff),
			(uint32_t)((tws_key_tone->bt_clk >> 32) & 0xffffffff), (uint32_t)(tws_key_tone->bt_clk & 0xffffffff));

		/* 连续播放按键音时间隔一点时间?
		 */
		for (i = 0; i < 10; i++)
		{
			if (!system_key_tone_is_playing())
			{
				break;
			}
		
			os_sleep(10);
		}

		memset(cur_keytone_name,0,sizeof(cur_keytone_name));
		if (audio_tone_get_name(tws_key_tone->key_tone, cur_keytone_name) == NULL)
		{
			memset(cur_keytone_name,0,sizeof(cur_keytone_name));		
			return;
		}

		tip_manager_play(cur_keytone_name, tws_key_tone->bt_clk);
    }
}



/*!
 * \brief 是否允许播放提示音
 */
void sys_manager_enable_audio_tone(bool enable)
{
    system_app_context_t*  manager = system_app_get_context();
    os_sched_lock();

    if (enable)
    {
        if (manager->sys_status.disable_audio_tone > 0)
        {
            manager->sys_status.disable_audio_tone -= 1;
        }

    }
    else
    {
        manager->sys_status.disable_audio_tone  += 1;
    }

    SYS_LOG_INF("disable tone: %d\n", 
        manager->sys_status.disable_audio_tone);
    os_sched_unlock();

}


/*!
 * \brief 是否允许播放语音
 */
void sys_manager_enable_audio_voice(bool enable)
{
    system_app_context_t*  manager = system_app_get_context();
    os_sched_lock();

    if (enable)
    {
        if (manager->sys_status.disable_audio_voice > 0)
        {
            manager->sys_status.disable_audio_voice -= 1;
        }
    }
    else
    {
        manager->sys_status.disable_audio_voice += 1;
    }

    SYS_LOG_INF("disable voice: %d\n", 
        manager->sys_status.disable_audio_voice);
    os_sched_unlock();

}



