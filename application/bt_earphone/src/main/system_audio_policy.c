/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system audio policy
 */

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <system_app.h>
#include <string.h>
#include <audio_system.h>
#include <audio_policy.h>
#include "app_config.h"
#include <buffer_stream.h>
#include <audiolcy_common.h>
#include "anc_hal.h"
#include "sdfs.h"
#include "media_player.h"

#define MAX_AUDIO_VOL_LEVEL 16
#define MAX_RECORD_CONFIGS (4)

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

typedef struct
{
    int8_t version[8];
    int32_t main_mic_gain;
    int32_t reserve;
} dual_mic_param_t;

/* unit: 0.1 dB */
static short voice_da_table[CFG_MAX_BT_CALL_VOLUME + 1];

/* unit: 0.001 dB */
static int voice_pa_table[CFG_MAX_BT_CALL_VOLUME + 1];

/* unit: 0.1 dB */
static short music_da_table[CFG_MAX_BT_MUSIC_VOLUME + 1];

/* unit: 0.001 dB */
static int music_pa_table[CFG_MAX_BT_MUSIC_VOLUME + 1];

/* unit: 0.1 dB */
static short tts_da_table[CFG_MAX_VOICE_VOLUME + 1];

/* unit: 0.001 dB */
static int tts_pa_table[CFG_MAX_VOICE_VOLUME + 1];

#if 1
/* unit: 0.1 dB */
static short usound_da_table[CFG_MAX_LINEIN_VOLUME] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
};

/* unit: 0.001 dB */
static int usound_pa_table[CFG_MAX_LINEIN_VOLUME] = {
       -59000, -44000, -38000, -33900,
       -29000, -24000, -22000, -19400,
       -17000, -14700, -12300, -10000,
       -7400,  -5000,  -3000,  0,
};
#else
/* unit: 0.1 dB */
static short usound_da_table[CFG_MAX_VOICE_VOLUME + 1];

/* unit: 0.001 dB */
static int usound_pa_table[CFG_MAX_VOICE_VOLUME + 1];

#endif

static record_config_t record_config[MAX_RECORD_CONFIGS];

static struct audio_policy_t system_audio_policy = {
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
	.audio_out_channel = AUDIO_CHANNEL_DAC | AUDIO_CHANNEL_I2STX,
#else
	.audio_out_channel = AUDIO_CHANNEL_DAC,
#endif
	.audio_in_linein_gain = {{0,0,0,0},{360,360,360,360}}, // again:0db; dgain:36.0db
	.audio_in_fm_gain = {{0,0,0,0},{360,360,360,360}}, // again:0db; dgain:36.0db
	.audio_in_mic_gain = {{0,0,0,0},{360,360,360,360}}, // again:0db; dgain:36.0db

	//.voice_aec_tail_length_16k = CONFIG_AEC_TAIL_LENGTH,
	//.voice_aec_tail_length_8k = CONFIG_AEC_TAIL_LENGTH * 2,

#ifdef CONFIG_SOFT_VOLUME
	.tts_fixed_volume = 1,
#else
	.tts_fixed_volume = 0,
#endif
	.volume_saveto_nvram = 1,
	
	.record_configs_num = MAX_RECORD_CONFIGS,
	.record_configs = record_config,

    .music_max_volume = CFG_MAX_BT_MUSIC_VOLUME,
    .voice_max_volume = CFG_MAX_BT_CALL_VOLUME,
    .tts_max_volume = CFG_MAX_VOICE_VOLUME,
    .tts_min_volume = 0,

	.music_da_table  = music_da_table,
	.music_pa_table  = music_pa_table,
	.voice_da_table  = voice_da_table,
	.voice_pa_table  = voice_pa_table,
	.tts_da_table    = tts_da_table,
	.tts_pa_table    = tts_pa_table,
	.usound_da_table = usound_da_table,
	.usound_pa_table = usound_pa_table,
	//.usound_da_table = music_da_table,
	//.usound_pa_table = music_pa_table,

#ifdef CONFIG_TOOL_ASET
	.aset_volume_table = NULL,
#endif

	.automute_force = 0,
};

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
static media_dvfs_map_t g_media_dvfs_map[6];
static media_dvfs_config_t g_media_dvfs_config;
#endif

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
static uint8_t dsp_freq_to_dvfs_level(uint16_t freq)
{
    uint8_t dvfs_level = DVFS_LEVEL_NORMAL;
    uint16_t dsp_freq;

    while(dvfs_level <= DVFS_LEVEL_NORMAL_MAX) {
        if(dvfs_level_to_freq(dvfs_level, NULL, &dsp_freq) == 0) {
            //printk("get level: %u, %u, %u\n", dvfs_level, dsp_freq, freq);
            if(dsp_freq >= freq) {
                return dvfs_level;
            }
        }
        
        dvfs_level++;
    }
    
    return dvfs_level;
}
#endif

void system_audio_policy_init(void)
{
    u8_t i;

    //music volume table
    {
        CFG_Struct_BT_Music_Volume_Table  cfg_music_volume_Table;
        
        app_config_read
        (
            CFG_ID_BT_MUSIC_VOLUME_TABLE, 
            &cfg_music_volume_Table, 0, sizeof(CFG_Struct_BT_Music_Volume_Table)
        );
        for(i = 0; i < (CFG_MAX_BT_MUSIC_VOLUME+1); i++)
        {
            music_da_table[i] = 0;
            music_pa_table[i] = (cfg_music_volume_Table.Level[i] - 0xbf) * 375;
        }
    }

    //btcall volume table
    {
        CFG_Struct_BT_Call_Volume_Table  cfg_call_volume_Table;
        CFG_Struct_BTSpeech_User_Settings cfg_btspeech_settings;

        app_config_read
        (
            CFG_ID_BT_CALL_VOLUME_TABLE, 
            &cfg_call_volume_Table, 0, sizeof(CFG_Struct_BT_Call_Volume_Table)
        );

        app_config_read
        (
            CFG_ID_BTSPEECH_USER_SETTINGS, 
            &cfg_btspeech_settings, 0, sizeof(CFG_Struct_BTSpeech_User_Settings)
        );

        for(i = 0; i < (CFG_MAX_BT_CALL_VOLUME + 1); i++)
        {
#ifdef CONFIG_SOFT_VOLUME
            voice_da_table[i] = 0;
            voice_pa_table[i] = (((cfg_call_volume_Table.Level[i] - 0xbf) \
                                 + (cfg_btspeech_settings.BS_Max_Out_Gain - 0xbf)) * 375);
#else
            voice_da_table[i] = ((cfg_call_volume_Table.Level[i] - 0xbf) * 375) / 100;
            voice_pa_table[i] = (cfg_btspeech_settings.BS_Max_Out_Gain - 0xbf) * 375;
#endif
        }
    }
    
    //tts volume table
    {
        CFG_Struct_Volume_Settings cfg_volume_settings;
        CFG_Struct_Voice_Volume_Table cfg_voice_volume_table;
        
        app_config_read
        (
            CFG_ID_VOLUME_SETTINGS, 
            &cfg_volume_settings, 0, sizeof(CFG_Struct_Volume_Settings)
        );
        system_audio_policy.tts_max_volume = cfg_volume_settings.Voice_Max_Volume;
        system_audio_policy.tts_min_volume = cfg_volume_settings.Voice_Min_Volume;
        system_audio_policy.tone_fixed_volume_decay = (0 - cfg_volume_settings.Tone_Volume_Decay);

        app_config_read
        (
            CFG_ID_VOICE_VOLUME_TABLE, 
            &cfg_voice_volume_table, 0, sizeof(CFG_Struct_Voice_Volume_Table)
        );
        for(i = 0; i < (CFG_MAX_VOICE_VOLUME+1); i++)
        {
            tts_da_table[i] = 0;
            tts_pa_table[i] = (cfg_voice_volume_table.Level[i] - 0xbf) * 375;
        }
    }

    //mic gain
    {
        CFG_Type_MIC_Gain  MIC_Gain;
        
        app_config_read
        (
            CFG_ID_BT_CALL_QUALITY, 
            &MIC_Gain,
            offsetof(CFG_Struct_BT_Call_Quality, MIC_Gain),
            sizeof(CFG_Type_MIC_Gain)
        );
        #if CONFIG_AUDIO_IN_ADGAIN_SUPPORT
        system_audio_policy.audio_in_mic_gain.ch_again[0] = MIC_Gain.ADC0_aGain;
        system_audio_policy.audio_in_mic_gain.ch_again[1] = MIC_Gain.ADC1_aGain;

        system_audio_policy.audio_in_mic_gain.ch_dgain[0] = MIC_Gain.ADC0_dGain;
        system_audio_policy.audio_in_mic_gain.ch_dgain[1] = MIC_Gain.ADC1_dGain;
        #ifdef CONFIG_SOC_SERIES_LARK
        system_audio_policy.audio_in_mic_gain.ch_again[2] = MIC_Gain.ADC2_aGain;
        system_audio_policy.audio_in_mic_gain.ch_again[3] = MIC_Gain.ADC3_aGain;
        system_audio_policy.audio_in_mic_gain.ch_dgain[2] = MIC_Gain.ADC2_dGain;
        system_audio_policy.audio_in_mic_gain.ch_dgain[3] = MIC_Gain.ADC3_dGain;
        #endif
        #else
        system_audio_policy.audio_in_mic_gain.ch_gain[0] = MIC_Gain.ADC0_Gain;
        system_audio_policy.audio_in_mic_gain.ch_gain[1] = MIC_Gain.ADC1_Gain;
        #ifdef CONFIG_SOC_SERIES_LARK
        system_audio_policy.audio_in_mic_gain.ch_gain[2] = MIC_Gain.ADC2_Gain;
        system_audio_policy.audio_in_mic_gain.ch_gain[3] = MIC_Gain.ADC3_Gain;
        #endif
        #endif

    }

    /* bt player配置
     */
    {
        CFG_Struct_BTMusic_Player_Param cfg_music_player;
        CFG_Struct_BTMusic_User_Settings cfg_music_usr;
        CFG_Struct_BTSpeech_Player_Param cfg_speech_player;
        CFG_Struct_BTSpeech_User_Settings cfg_speech_usr;
        CFG_Struct_Low_Latency_Settings cfg_latency;
        
        app_config_read(CFG_ID_BTMUSIC_PLAYER_PARAM, &cfg_music_player, 0, sizeof(cfg_music_player));
        app_config_read(CFG_ID_BTMUSIC_USER_SETTINGS, &cfg_music_usr, 0, sizeof(cfg_music_usr));
        app_config_read(CFG_ID_BTSPEECH_PLAYER_PARAM, &cfg_speech_player, 0, sizeof(cfg_speech_player));
        app_config_read(CFG_ID_BTSPEECH_USER_SETTINGS, &cfg_speech_usr, 0, sizeof(cfg_speech_usr));
        // playback threshold for low latency mode
        app_config_read(CFG_ID_LOW_LATENCY_SETTINGS, &cfg_latency, 0, sizeof(cfg_latency));

        system_audio_policy.player_config.BM_Max_PCMBUF_Time = cfg_music_player.BM_Max_PCMBUF_Time;
        system_audio_policy.player_config.BM_TWS_WPlay_Mintime = cfg_music_player.BM_TWS_WPlay_Mintime;
        system_audio_policy.player_config.BM_TWS_WPlay_Maxtime = cfg_music_player.BM_TWS_WPlay_Maxtime;
        system_audio_policy.player_config.BM_TWS_Sync_interval = cfg_music_player.BM_TWS_Sync_interval;
        system_audio_policy.player_config.BM_StartPlay_Normal = cfg_music_player.BM_StartPlay_Normal;
        system_audio_policy.player_config.BM_StartPlay_TWS = cfg_music_player.BM_StartPlay_TWS;
        system_audio_policy.player_config.BM_SBC_Playing_CacheData = cfg_music_usr.BM_SBC_Playing_CacheData;
        system_audio_policy.player_config.BM_AAC_Playing_CacheData = cfg_music_usr.BM_AAC_Playing_CacheData;

        system_audio_policy.player_config.BS_Max_PCMBUF_Time = cfg_speech_player.BS_Max_PCMBUF_Time;
        system_audio_policy.player_config.BS_MIC_Playing_PKTCNT = cfg_speech_player.BS_MIC_Playing_PKTCNT;
        system_audio_policy.player_config.BS_TWS_WPlay_Mintime = cfg_speech_player.BS_TWS_WPlay_Mintime;
        system_audio_policy.player_config.BS_TWS_WPlay_Maxtime = cfg_speech_player.BS_TWS_WPlay_Maxtime;
        system_audio_policy.player_config.BS_TWS_Sync_interval = cfg_speech_player.BS_TWS_Sync_interval;
        system_audio_policy.player_config.BS_StartPlay_Normal = cfg_speech_player.BS_StartPlay_Normal;
        system_audio_policy.player_config.BS_StartPlay_TWS = cfg_speech_player.BS_StartPlay_TWS;
        system_audio_policy.player_config.BS_CVSD_Playing_CacheData = cfg_speech_usr.BS_CVSD_Playing_CacheData;
        system_audio_policy.player_config.BS_MSBC_Playing_CacheData = cfg_speech_usr.BS_MSBC_Playing_CacheData;
        system_audio_policy.player_config.BM_SBC_Threshold_A_LLM = cfg_latency.LLM_Basic.SBC_Threshold;
        system_audio_policy.player_config.BM_AAC_Threshold_A_LLM = cfg_latency.LLM_Basic.AAC_Threshold;
        if (system_audio_policy.player_config.BM_SBC_Threshold_A_LLM == 0) {
            system_audio_policy.player_config.BM_SBC_Threshold_A_LLM = 60;  // default 60ms
        }
        if (system_audio_policy.player_config.BM_AAC_Threshold_A_LLM == 0) {
            system_audio_policy.player_config.BM_AAC_Threshold_A_LLM = 60;  // default 60ms
        }

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
        g_media_dvfs_config.dvfs_map = g_media_dvfs_map;
        g_media_dvfs_config.dvfs_map_size = 6;


        SYS_LOG_INF("%d,%d,%d,%d,%d",cfg_music_player.BM_Work_Frequency_SBC,
									cfg_music_player.BM_Work_Frequency_AAC,
									cfg_music_player.BM_Freq_TWS_Decrement,
									cfg_speech_player.BS_Work_Frequency_MSBC,
									cfg_speech_player.BS_Work_Frequency_CVSD);

        g_media_dvfs_map[0].format = SBC_TYPE;
        g_media_dvfs_map[0].channels = 2;
        g_media_dvfs_map[0].dvfs_level = 
            dsp_freq_to_dvfs_level(cfg_music_player.BM_Work_Frequency_SBC);

        g_media_dvfs_map[1].format = SBC_TYPE;
        g_media_dvfs_map[1].channels = 1;
        g_media_dvfs_map[1].dvfs_level = 
            dsp_freq_to_dvfs_level(cfg_music_player.BM_Work_Frequency_SBC - cfg_music_player.BM_Freq_TWS_Decrement);

        g_media_dvfs_map[2].format = AAC_TYPE;
        g_media_dvfs_map[2].channels = 2;
        g_media_dvfs_map[2].dvfs_level = 
            dsp_freq_to_dvfs_level(cfg_music_player.BM_Work_Frequency_AAC);

        g_media_dvfs_map[3].format = AAC_TYPE;
        g_media_dvfs_map[3].channels = 1;
        g_media_dvfs_map[3].dvfs_level = 
            dsp_freq_to_dvfs_level(cfg_music_player.BM_Work_Frequency_AAC - cfg_music_player.BM_Freq_TWS_Decrement);

        g_media_dvfs_map[4].format = MSBC_TYPE;
        g_media_dvfs_map[4].channels = 0;
        g_media_dvfs_map[4].dvfs_level = 
            dsp_freq_to_dvfs_level(cfg_speech_player.BS_Work_Frequency_MSBC);

        g_media_dvfs_map[5].format = CVSD_TYPE;
        g_media_dvfs_map[5].channels = 0;
        g_media_dvfs_map[5].dvfs_level = 
            dsp_freq_to_dvfs_level(cfg_speech_player.BS_Work_Frequency_CVSD);

        media_register_dvfs_config(&g_media_dvfs_config);
#endif
    }

    //record configs
    {
        CFG_Struct_Audio_Settings audio_settings;
        int32_t main_mic_gain = 0x3000;//-2.5db

#ifdef CONFIG_SD_FS
        struct buffer_t buffer;

		if (sd_fmap("cfg_mic.bin", (void **)&buffer.base, &buffer.length) != 0) {
            SYS_LOG_ERR("cfg_mic.bin no exist\n");
		} else {
		    dual_mic_param_t *dual_mic_param = (dual_mic_param_t*)buffer.base;
            main_mic_gain = dual_mic_param->main_mic_gain;
            printk("main_mic_gain: %d\n", main_mic_gain);
		}
#endif

        app_config_read(CFG_ID_AUDIO_SETTINGS, &audio_settings, 0, sizeof(audio_settings));
        memset(&record_config[0], 0, sizeof(record_config));

        record_config[0].stream_type = AUDIO_STREAM_VOICE;
        record_config[0].main_mic_gain = main_mic_gain;
        if(audio_settings.main_mic != ADC_NONE) {
            record_config[0].channels++;
            record_config[0].audio_device |= audio_settings.main_mic;
        }
        if(audio_settings.sub_mic != ADC_NONE) {
            record_config[0].channels++;
            record_config[0].audio_device |= audio_settings.sub_mic;
        }
        record_config[0].dual_mic_swap = 0;
        if((record_config[0].channels == 2) && (audio_settings.sub_mic < audio_settings.main_mic)) {
            record_config[0].dual_mic_swap = 1;
        }        
        #ifdef CONFIG_SOC_SERIES_LARK
        record_config[1].stream_type = AUDIO_STREAM_VOICE_ANC;
        if(audio_settings.anc_ff_mic != ADC_NONE) {
            record_config[1].channels++;
            record_config[1].audio_device |= audio_settings.anc_ff_mic;
        }
        if(audio_settings.anc_fb_mic != ADC_NONE) {
            record_config[1].channels++;
            record_config[1].audio_device |= audio_settings.anc_fb_mic;
        }
        record_config[1].dual_mic_swap = 0;
        if((record_config[1].channels == 2) && (audio_settings.anc_fb_mic < audio_settings.anc_ff_mic)) {
            record_config[1].dual_mic_swap = 1;
        }
        #endif

        record_config[2].stream_type = AUDIO_STREAM_DEFAULT;
        if(audio_settings.main_mic != ADC_NONE) {
            record_config[2].channels++;
            record_config[2].audio_device |= audio_settings.main_mic;
        }
        record_config[2].dual_mic_swap = 0;

        /* Only For Audio Performance Test */
        record_config[3].stream_type = AUDIO_STREAM_MIC_IN;
        record_config[3].channels = 2;
        record_config[3].audio_device = ADC_0 | ADC_1;
        record_config[3].dual_mic_swap = 0;
    }

    app_config_read
    (
        CFG_ID_AUDIO_MORE_SETTINGS, 
        &system_audio_policy.automute_force, offsetof(CFG_Struct_Audio_More_Settings, Automute_Force_Enable), sizeof(cfg_uint8)
    );
    SYS_LOG_INF("automute force %d", system_audio_policy.automute_force);

	audio_policy_register(&system_audio_policy);
}

void system_audiolcy_config_init(void)
{
    audiolcy_cfg_t  cfg_audiolcy;
    CFG_Struct_Low_Latency_Settings cfg_latency;

    // playback threshold for low latency mode
    app_config_read(CFG_ID_LOW_LATENCY_SETTINGS, &cfg_latency, 0, sizeof(cfg_latency));

    // init audio latency settings
    cfg_audiolcy.Default_Low_Latency_Mode = cfg_latency.LLM_Basic.Default_Low_Latency_Mode;
    cfg_audiolcy.Save_Latency_Mode = cfg_latency.LLM_Basic.Save_Low_Latency_Mode;

    cfg_audiolcy.BM_Use_ALLM = cfg_latency.BTMusic_Use_ALLM;
    cfg_audiolcy.BM_ALLM_Factor = cfg_latency.ALLM_Factor;
    cfg_audiolcy.BM_ALLM_Upper  = cfg_latency.ALLM_Upper;

    cfg_audiolcy.BM_PLC_Mode = cfg_latency.BTMusic_PLC_Mode;
    cfg_audiolcy.BM_AAC_Decode_Delay_Optimize = cfg_latency.AAC_Decode_Delay_Optimize;

    cfg_audiolcy.BS_Latency_Optimization = cfg_latency.BTCall_Latency_Optimization;
    cfg_audiolcy.BS_PLC_NoDelay = cfg_latency.BTCall_PLC_NoDelay;
    cfg_audiolcy.BS_Use_Limiter_Not_AGC = cfg_latency.BTCall_Use_Limiter_Not_AGC;

    cfg_audiolcy.ALLM_down_timer_period_s = 120;    // default 120s, the time to wait before down adjust the latency
    cfg_audiolcy.ALLM_recommend_period_s = 10;      // default 10s, the period of recommending a adaptive latency
    cfg_audiolcy.ALLM_scc_clear_period_s = 120;     // default 120s, period to check and clear the increment by staccato
    cfg_audiolcy.ALLM_scc_clear_step = 10;          // default 10ms, not used, the step to clear the latency increment by staccato
    cfg_audiolcy.ALLM_scc_ignore_period_ms = 3000;  // default 3000ms, period to check staccato occured times
    cfg_audiolcy.ALLM_scc_ignore_count = 3;         // default 3, times of staccato in a period is less than this value, ignore it

    audiolcy_config_init(&cfg_audiolcy);
}

