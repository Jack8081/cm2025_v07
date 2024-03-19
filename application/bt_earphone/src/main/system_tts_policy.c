/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system tts policy
 */

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <system_app.h>
#include <string.h>
#include <tts_manager.h>
#include "app_config.h"
#include <sdfs.h>
#include <audiolcy_common.h>

char* audio_tone_get_name(u32_t tone_id, char* name_buf)
{
    CFG_Struct_Tone_List*  cfg;
    int  i;
	struct sd_file *file;

    *name_buf = '\0';

    if (tone_id == TONE_NONE)
    {
        return NULL;
    }

    cfg = app_mem_malloc(sizeof(CFG_Struct_Tone_List));
	if (!cfg) return NULL;

    app_config_read(CFG_ID_TONE_LIST, cfg, 0, sizeof(CFG_Struct_Tone_List));

    for (i = 0; i < CFG_MAX_TONES; i++)
    {
        if (cfg->Tone[i].Tone_ID == tone_id)
        {
            strcpy(name_buf, (char*)cfg->Tone[i].Tone_Name);
            strcat(name_buf, (char*)cfg->Tone_Format_Name);
            break;
        }
    }

    app_mem_free(cfg);

    if (i < CFG_MAX_TONES)
    {
        if ((file = sd_fopen(name_buf)) == NULL)
        {
            SYS_LOG_WRN("%s, NOT_EXIST", name_buf);
            return NULL;
        }
        sd_fclose(file);
        return name_buf;
    }

    return NULL;
}

char* audio_voice_get_name(u32_t voice_id, char* name_buf)
{
    int  i;

    *name_buf = '\0';

    if (voice_id == VOICE_NONE)
    {
        return NULL;
    }

    if (voice_id >= VOICE_NO_0)
    {
        CFG_Struct_Numeric_Voice_List*  cfg = app_mem_malloc(sizeof(CFG_Struct_Numeric_Voice_List));
		if (!cfg ) return NULL;

		
        char  fmt_name[CFG_MAX_VOICE_FMT_LEN];

        app_config_read(CFG_ID_NUMERIC_VOICE_LIST, cfg , 0, sizeof(CFG_Struct_Numeric_Voice_List));

        app_config_read
        (
            CFG_ID_VOICE_LIST,
            fmt_name,
            offsetof(CFG_Struct_Voice_List, Voice_Format_Name),
            CFG_MAX_VOICE_FMT_LEN
        );

        for (i = 0; i < CFG_MAX_NUMERIC_VOICES; i++)
        {
            if (cfg->Voice[i].Voice_ID == voice_id)
            {
                strcpy(name_buf, (char*)cfg->Voice[i].Voice_Name);
                strcat(name_buf, (char*)fmt_name);
                break;
            }
        }

        app_mem_free(cfg);

        if (i < CFG_MAX_NUMERIC_VOICES)
        {
            return name_buf;
        }
    }
    else
    {
        CFG_Struct_Voice_List*  cfg_vlist = app_mem_malloc(sizeof(CFG_Struct_Voice_List));
		if (!cfg_vlist) return NULL;

		app_config_read
		(
			CFG_ID_VOICE_LIST,
			cfg_vlist, 0, sizeof(CFG_Struct_Voice_List)
		);
    
        for (i = 0; i < CFG_MAX_VOICES; i++)
        {
            if (cfg_vlist->Voice[i].Voice_ID == voice_id)
            {
                strcpy(name_buf, (char*)cfg_vlist->Voice[i].Voice_Name);
                strcat(name_buf, (char*)cfg_vlist->Voice_Format_Name);
                break;
            }
        }
	
        app_mem_free(cfg_vlist);

        if (i < CFG_MAX_VOICES)
        {
            return name_buf;
        }
    }

    return NULL;
}


char* audio_voice_get_name_ex(u32_t voice_id, char* name_buf)
{
    system_app_context_t*  manager = system_app_get_context();
	struct sd_file *file;

    if (audio_voice_get_name(voice_id, name_buf) == NULL)
    {
        return NULL;
    }

    if (manager->select_voice_language == VOICE_LANGUAGE_2)
    {
        name_buf[0] = 'E';
    }

    if (manager->alter_voice_language != manager->select_voice_language)
    {
        if ((file = sd_fopen(name_buf)) == NULL)
        {
            if (manager->alter_voice_language == VOICE_LANGUAGE_2)
            {
                name_buf[0] = 'E';
            }
            else
            {
                name_buf[0] = 'V';
            }
        }
        else
        {
            sd_fclose(file);
        }
    }
	
    if ((file = sd_fopen(name_buf)) == NULL)
    {
        SYS_LOG_WRN("%s, NOT_EXIST", name_buf);
        return NULL;
    }
    sd_fclose(file);

    return name_buf;
}



/*!
 * \brief 切换语音语言
 */
void system_switch_voice_language(void)
{
    system_app_context_t*  manager = system_app_get_context();

    int dev_role = bt_manager_tws_get_dev_role();

    manager->select_voice_language += 1;

    if (manager->select_voice_language > VOICE_LANGUAGE_2)
    {
        manager->select_voice_language = VOICE_LANGUAGE_1;
    }
    
    SYS_LOG_INF("%d", manager->select_voice_language);

    int ret = property_set(CFG_VOICE_LANGUAGE, (char*)&manager->select_voice_language, sizeof(u8_t));
    if (ret!=0)
    {
       SYS_LOG_ERR("property_set fail.");
    }

    /* TWS 同步设置语音语言?
     */
    if (dev_role >= BTSRV_TWS_MASTER)
    {
        bt_manager_tws_send_message
        (
            TWS_USER_APP_EVENT, TWS_EVENT_SET_VOICE_LANG, 
            &manager->select_voice_language, sizeof(u8_t)
		);
    }

    if (manager->select_voice_language == VOICE_LANGUAGE_1)
    {
        sys_manager_event_notify(SYS_EVENT_SEL_VOICE_LANG_1, true);
    }
    else
    {
        sys_manager_event_notify(SYS_EVENT_SEL_VOICE_LANG_2, true);
    }
}


/* 设置语音语言
 */
void system_tws_set_voice_language(u8_t voice_lang)
{
    system_app_context_t*  manager = system_app_get_context();

    SYS_LOG_INF("%d", voice_lang);

    manager->select_voice_language = voice_lang;

    int ret = property_set(CFG_VOICE_LANGUAGE, (char*)&manager->select_voice_language, sizeof(u8_t));
    if (ret!=0)
    {
       SYS_LOG_ERR("property_set fail.");
    }
}


void system_switch_low_latency_mode(void)
{
    u8_t  cur_lcymode, next_lcymode;
    int dev_role = bt_manager_tws_get_dev_role();

    cur_lcymode  = audiolcy_get_latency_mode();
    next_lcymode = audiolcy_check_latency_mode_switch_next();

    // switch is forbidden
    if (next_lcymode == cur_lcymode) {
        return ;
    }

    audiolcy_set_latency_mode(next_lcymode, 1);

    // TWS switch latency mode together
	if (dev_role >= BTSRV_TWS_MASTER)
    {
        bt_manager_tws_send_message
        (
            TWS_USER_APP_EVENT, TWS_EVENT_SYNC_LOW_LATENCY_MODE, 
            &next_lcymode, sizeof(u8_t)
		);
    }

    if (next_lcymode == LCYMODE_ADAPTIVE) {
        sys_manager_event_notify(SYS_EVENT_LOW_LATENCY_MODE, true);  // same notify as LLM
    } else if (next_lcymode == LCYMODE_LOW) {
        sys_manager_event_notify(SYS_EVENT_LOW_LATENCY_MODE, true);
    } else {
        sys_manager_event_notify(SYS_EVENT_NORMAL_LATENCY_MODE, true);
    }
}


