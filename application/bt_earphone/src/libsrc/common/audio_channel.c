/*!
 * \file      audio_channel.c
 * \brief     声道管理接口
 * \details
 * \author
 * \date
 * \copyright Actions
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "audio channel"
	
#include <os_common_api.h>
#include <app_common.h>
#include "bt_manager.h"
#include "media_service.h"
#include <chan_sel_hal.h>
#include <property_manager.h>
#include "app_config.h"
#include "ui_key_map.h"


static audio_manager_configs_t audio_config;

audio_manager_configs_t* audio_channel_get_config(void)
{
    return &audio_config;
}


static int audio_channel_sel_by_gpio(int sel_mode)
{
    unsigned char  channel = NONE;
    unsigned char  level;
    struct audio_chansel_data data;
	CFG_Type_Channel_Select_GPIO cfg;
    app_config_read
	(
		CFG_ID_AUDIO_SETTINGS,
		&cfg,
		offsetof(CFG_Struct_Audio_Settings, Channel_Select_GPIO),
		sizeof(CFG_Type_Channel_Select_GPIO)
    );

    memset(&data,0,sizeof(struct audio_chansel_data));

    if (cfg.GPIO_Pin == GPIO_NONE)
    {
        level = GPIO_LEVEL_LOW;
    }
    else
    {
        data.Select_GPIO.GPIO_Pin = cfg.GPIO_Pin;
        data.Select_GPIO.Pull_Up_Down = cfg.Pull_Up_Down;
        data.sel_mode = AUDIO_CHANNEL_SEL_BY_GPIO;
		
        level = chan_sel_device_inquiry(&data,CHAN_SEL_DEVICE_NAME);
    }

    if (sel_mode == CHANNEL_SELECT_L_BY_GPIO)
    {
        channel = (level == cfg.Active_Level) ?
            AUDIO_DEVICE_CHANNEL_L :
            AUDIO_DEVICE_CHANNEL_R;
    }
    else if (sel_mode == CHANNEL_SELECT_R_BY_GPIO)
    {
        channel = (level == cfg.Active_Level) ?
            AUDIO_DEVICE_CHANNEL_R :
            AUDIO_DEVICE_CHANNEL_L;
    }
    SYS_LOG_INF("%d", channel);

    return channel;
}


static int audio_channel_sel_by_lradc(int sel_mode)
{
    unsigned char channel = NONE;
    unsigned int adc_val;
    struct audio_chansel_data data;
	CFG_Type_Channel_Select_LRADC cfg;

    app_config_read
	(
		CFG_ID_AUDIO_SETTINGS,
		&cfg,
		offsetof(CFG_Struct_Audio_Settings, Channel_Select_LRADC),
		sizeof(CFG_Type_Channel_Select_LRADC)
    );

    memset(&data,0,sizeof(struct audio_chansel_data));

    data.Select_LRADC.LRADC_Pull_Up = cfg.LRADC_Pull_Up;
    data.Select_LRADC.LRADC_Ctrl = cfg.LRADC_Ctrl;
    data.sel_mode = AUDIO_CHANNEL_SEL_BY_LRADC;
	
    adc_val = chan_sel_device_inquiry(&data,CHAN_SEL_DEVICE_NAME);

    if (sel_mode == CHANNEL_SELECT_L_BY_LRADC)
    {
        channel = (adc_val >= cfg.ADC_Min && adc_val <= cfg.ADC_Max) ?
            AUDIO_DEVICE_CHANNEL_L :
            AUDIO_DEVICE_CHANNEL_R;
    }
    else if (sel_mode == CHANNEL_SELECT_R_BY_LRADC)
    {
        channel = (adc_val >= cfg.ADC_Min && adc_val <= cfg.ADC_Max) ?
            AUDIO_DEVICE_CHANNEL_R :
            AUDIO_DEVICE_CHANNEL_L;
    }

    SYS_LOG_INF("%d", channel);

    return channel;
}


/* 声道选择初始化
 */
static void audio_channel_sel_init(void)
{
    audio_manager_configs_t*  cfg = audio_channel_get_config();
	unsigned char sel_mode;

    app_config_read
	(
		CFG_ID_AUDIO_SETTINGS,
		&sel_mode,
		offsetof(CFG_Struct_Audio_Settings, Channel_Select_Mode),
		sizeof(sel_mode)
    );

    switch (sel_mode)
    {
        case CHANNEL_SELECT_L_BY_GPIO:
        case CHANNEL_SELECT_R_BY_GPIO:
        {
            cfg->hw_sel_dev_channel =
                audio_channel_sel_by_gpio(sel_mode);
            break;
        }

        case CHANNEL_SELECT_L_BY_LRADC:
        case CHANNEL_SELECT_R_BY_LRADC:
        {
            cfg->hw_sel_dev_channel =
                audio_channel_sel_by_lradc(sel_mode);
            break;
        }
    }
}


static int track_map_media_effect(audio_track_e type)
{
    int output_mode = MEDIA_EFFECT_OUTPUT_DEFAULT;

	switch(type)
	{
		case AUDIO_TRACK_NORMAL:
			output_mode = MEDIA_EFFECT_OUTPUT_DEFAULT;
		break;
		case AUDIO_TRACK_L_R_SWITCH:
			output_mode = MEDIA_EFFECT_OUTPUT_L_R_SWAP;
		break;
		case AUDIO_TRACK_L_R_MERGE:
			output_mode = MEDIA_EFFECT_OUTPUT_L_R_MIX;
		break;
		case AUDIO_TRACK_L_R_ALL_L:
			output_mode = MEDIA_EFFECT_OUTPUT_L_ONLY;
		break;

		case AUDIO_TRACK_L_R_ALL_R:
			output_mode = MEDIA_EFFECT_OUTPUT_R_ONLY;
		break;
		default:
		break;
	}
	
    return output_mode;
}


/* 声道选择
 */
int audio_channel_sel_track(void)
{
    audio_manager_configs_t*  cfg = audio_channel_get_config();
    unsigned char  track   = AUDIO_TRACK_NORMAL;
    unsigned char  channel = NONE;
    int dev_role = bt_manager_tws_get_dev_role();
	unsigned char sel_mode;
    app_config_read
	(
		CFG_ID_AUDIO_SETTINGS,
		&sel_mode,
		offsetof(CFG_Struct_Audio_Settings, Channel_Select_Mode),
		sizeof(sel_mode)
    );

	unsigned char TWS_Alone_Audio_Channel;
    app_config_read
	(
		CFG_ID_AUDIO_SETTINGS,
		&TWS_Alone_Audio_Channel,
		offsetof(CFG_Struct_Audio_Settings, TWS_Alone_Audio_Channel),
		sizeof(TWS_Alone_Audio_Channel)
    );

	
    switch (sel_mode)
    {
        case CHANNEL_SELECT_NORMAL_LR:
        {
            track = AUDIO_TRACK_NORMAL;
            break;
        }

        case CHANNEL_SELECT_MIX_LR:
        {
            track = AUDIO_TRACK_L_R_MERGE;
            break;
        }

        case CHANNEL_SELECT_SWAP_LR:
        {
            track = AUDIO_TRACK_L_R_SWITCH;
            break;
        }

        case CHANNEL_SELECT_BOTH_L:
        {
            track = AUDIO_TRACK_L_R_ALL_L;
            break;
        }

        case CHANNEL_SELECT_BOTH_R:
        {
            track = AUDIO_TRACK_L_R_ALL_R;
            break;
        }

        case CHANNEL_SELECT_L_BY_TWS_PAIR:
        case CHANNEL_SELECT_R_BY_TWS_PAIR:
        {
            channel = cfg->sw_sel_dev_channel;
            break;
        }

        case CHANNEL_SELECT_L_BY_GPIO:
        case CHANNEL_SELECT_R_BY_GPIO:

        case CHANNEL_SELECT_L_BY_LRADC:
        case CHANNEL_SELECT_R_BY_LRADC:
        {
            channel = cfg->hw_sel_dev_channel;
            break;
        }
    }

    switch (sel_mode)
    {
        case CHANNEL_SELECT_L_BY_TWS_PAIR:
        case CHANNEL_SELECT_R_BY_TWS_PAIR:

        case CHANNEL_SELECT_L_BY_GPIO:
        case CHANNEL_SELECT_R_BY_GPIO:

        case CHANNEL_SELECT_L_BY_LRADC:
        case CHANNEL_SELECT_R_BY_LRADC:
        {
            track = AUDIO_TRACK_L_R_MERGE;

            if (dev_role < BTSRV_TWS_MASTER)
            {
                /* TWS 未组对时声道选择
                 */
                if (TWS_Alone_Audio_Channel == TWS_ALONE_AUDIO_SINGLE_L)
                {
                    track = AUDIO_TRACK_L_R_ALL_L;
                }
                else if (TWS_Alone_Audio_Channel == TWS_ALONE_AUDIO_SINGLE_R)
                {
                    track = AUDIO_TRACK_L_R_ALL_R;
                }
                else if (TWS_Alone_Audio_Channel == TWS_ALONE_AUDIO_ADAPTIVE)
                {
                    if (channel == AUDIO_DEVICE_CHANNEL_L)
                    {
                        track = AUDIO_TRACK_L_R_ALL_L;
                    }
                    else if (channel == AUDIO_DEVICE_CHANNEL_R)
                    {
                        track = AUDIO_TRACK_L_R_ALL_R;
                    }
                }
            }
            else
            {
                if (channel == AUDIO_DEVICE_CHANNEL_L)
                {
                    track = AUDIO_TRACK_L_R_ALL_L;
                }
                else if (channel == AUDIO_DEVICE_CHANNEL_R)
                {
                    track = AUDIO_TRACK_L_R_ALL_R;
                }
            }
            break;
        }
    }

    SYS_LOG_INF("%d", track);

    return track_map_media_effect(track);
}


/* 喇叭输出选择
 */
void audio_speaker_out_select(int* L, int* R)
{
    audio_manager_configs_t*  cfg = audio_channel_get_config();
	unsigned char L_Speaker_Out;
    app_config_read
	(
		CFG_ID_AUDIO_SETTINGS,
		&L_Speaker_Out,
		offsetof(CFG_Struct_Audio_Settings, L_Speaker_Out),
		sizeof(L_Speaker_Out)
    );

	unsigned char R_Speaker_Out;
    app_config_read
	(
		CFG_ID_AUDIO_SETTINGS,
		&R_Speaker_Out,
		offsetof(CFG_Struct_Audio_Settings, R_Speaker_Out),
		sizeof(R_Speaker_Out)
    );	

	unsigned char Channel_Select_Mode;
    app_config_read
	(
		CFG_ID_AUDIO_SETTINGS,
		&Channel_Select_Mode,
		offsetof(CFG_Struct_Audio_Settings, Channel_Select_Mode),
		sizeof(Channel_Select_Mode)
    );


    *L = ENABLE;
    *R = ENABLE;

    if (L_Speaker_Out == SPEAKER_OUT_DISABLE)
    {
        *L = DISABLE;
    }
    else if (L_Speaker_Out == SPEAKER_OUT_ADAPTIVE)
    {
        switch (Channel_Select_Mode)
        {
            case CHANNEL_SELECT_L_BY_TWS_PAIR:
            case CHANNEL_SELECT_R_BY_TWS_PAIR:
            {
                if (cfg->sw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_R)
                {
                    *L = DISABLE;
                }
                break;
            }

            case CHANNEL_SELECT_L_BY_GPIO:
            case CHANNEL_SELECT_R_BY_GPIO:

            case CHANNEL_SELECT_L_BY_LRADC:
            case CHANNEL_SELECT_R_BY_LRADC:
            {
                if (cfg->hw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_R)
                {
                    *L = DISABLE;
                }
                break;
            }
        }
    }

    if (R_Speaker_Out == SPEAKER_OUT_DISABLE)
    {
        *R = DISABLE;
    }
    else if (R_Speaker_Out == SPEAKER_OUT_ADAPTIVE)
    {
        switch (Channel_Select_Mode)
        {
            case CHANNEL_SELECT_L_BY_TWS_PAIR:
            case CHANNEL_SELECT_R_BY_TWS_PAIR:
            {
                if (cfg->sw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_L)
                {
                    *R = DISABLE;
                }
                break;
            }

            case CHANNEL_SELECT_L_BY_GPIO:
            case CHANNEL_SELECT_R_BY_GPIO:

            case CHANNEL_SELECT_L_BY_LRADC:
            case CHANNEL_SELECT_R_BY_LRADC:
            {
                if (cfg->hw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_L)
                {
                    *R = DISABLE;
                }
                break;
            }
        }
    }

    SYS_LOG_INF("%d, %d", *L, *R);
}



void audio_channel_init(void)
{
    audio_manager_configs_t*  cfg = audio_channel_get_config();
    unsigned char  sw_sel_dev_channel = 0;
	
    SYS_LOG_DBG("");

    memset(cfg, 0, sizeof(audio_manager_configs_t));

    unsigned char lrch = 0;
    int ret;
    ret = property_get(TWS_AUDIO_CHANNEL, (char*)&lrch, sizeof(lrch));
    if (ret > 0) {
        sw_sel_dev_channel = lrch;
    }

    /* 声道选择初始化
     */
    audio_channel_sel_init();

    cfg->sw_sel_dev_channel = sw_sel_dev_channel;
}



/* TWS 本机选择声道
 */
void bt_manager_tws_prepare_sel_channel
(
    int tws_pair_search, 
    int* local_ch, int* remote_ch)
{
    audio_manager_configs_t*  cfg = audio_channel_get_config();
    u8_t  channel  = cfg->sw_sel_dev_channel;
	u8_t sel_mode;

    app_config_read
	(
		CFG_ID_AUDIO_SETTINGS,
		&sel_mode,
		offsetof(CFG_Struct_Audio_Settings, Channel_Select_Mode),
		sizeof(sel_mode)
    );


    cfg->reomte_sel_dev_channel = NONE;
    
    if (sel_mode == CHANNEL_SELECT_L_BY_TWS_PAIR ||
        sel_mode == CHANNEL_SELECT_R_BY_TWS_PAIR)
    {
        if (tws_pair_search)
        {
            if (sel_mode == CHANNEL_SELECT_L_BY_TWS_PAIR)
            {
                channel = AUDIO_DEVICE_CHANNEL_L;
                cfg->reomte_sel_dev_channel = AUDIO_DEVICE_CHANNEL_R;
            }
            else
            {
                channel = AUDIO_DEVICE_CHANNEL_R;
                cfg->reomte_sel_dev_channel = AUDIO_DEVICE_CHANNEL_L;
            }
        }
    }

    *local_ch = channel;
	*remote_ch = cfg->reomte_sel_dev_channel;
   
    if (channel != NONE)
    {
        SYS_LOG_INF("%d", channel);
    }
}


void bt_manager_tws_confirm_sel_channel
(
    int local_acl_requested, 
    int remote_sw_sel_dev_channel, int tws_sel_channel)
{
    audio_manager_configs_t*  cfg = audio_channel_get_config();
    u8_t  channel  = cfg->sw_sel_dev_channel;
	u8_t sel_mode;

    app_config_read
	(
		CFG_ID_AUDIO_SETTINGS,
		&sel_mode,
		offsetof(CFG_Struct_Audio_Settings, Channel_Select_Mode),
		sizeof(sel_mode)
    );

    if (sel_mode == CHANNEL_SELECT_L_BY_TWS_PAIR ||
        sel_mode == CHANNEL_SELECT_R_BY_TWS_PAIR)
    {
        /* 对方给本机指定的设备声道?
         */
        if (tws_sel_channel != NONE)
        {
            /* 本机给对方指定了相同的设备声道?
             */
            if (cfg->reomte_sel_dev_channel == tws_sel_channel)
            {
                /* 以发起连接的一方指定的设备声道为准
                 */
                if (!local_acl_requested)
                {
                    channel = tws_sel_channel;
                    cfg->reomte_sel_dev_channel = NONE;
                }
            }
            else
            {
                channel = tws_sel_channel;
            }
        }
        /* 本机和对方选择了相同的设备声道?
         */
        else if (channel == remote_sw_sel_dev_channel)
        {
            /* 以发起连接的一方选定的设备声道为准
             */
            if (sel_mode == CHANNEL_SELECT_L_BY_TWS_PAIR)
            {
                if (!local_acl_requested)
                {
                    channel = AUDIO_DEVICE_CHANNEL_R;
                }
                else
                {
                    channel = AUDIO_DEVICE_CHANNEL_L;
                }
            }
            else if (sel_mode == CHANNEL_SELECT_R_BY_TWS_PAIR)
            {
                if (!local_acl_requested)
                {
                    channel = AUDIO_DEVICE_CHANNEL_L;
                }
                else
                {
                    channel = AUDIO_DEVICE_CHANNEL_R;
                }
            }
        }
        /* 本机和对方选择了不同的设备声道?
         */
        else
        {
            if (channel == NONE)
            {
                if (remote_sw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_L)
                {
                    channel = AUDIO_DEVICE_CHANNEL_R;
                }
                else if (remote_sw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_R)
                {
                    channel = AUDIO_DEVICE_CHANNEL_L;
                }
            }
        }
    }
    
    if (cfg->sw_sel_dev_channel != channel)
    {
        cfg->sw_sel_dev_channel = channel;
    
        unsigned char lrch = channel;
        property_set_factory(TWS_AUDIO_CHANNEL, (char*)&lrch, sizeof(lrch));

        ui_key_notify_remap_keymap();

        /*sw channel change, LEA must restart: sink_locations need change*/		
        struct app_msg	msg = {0};
        msg.type = MSG_BT_MGR_EVENT;
        msg.cmd = BT_MGR_EVENT_LEA_RESTART;
        send_async_msg("main", &msg);
    }
    
    if (channel != NONE)
    {
        SYS_LOG_INF("%c", (channel == AUDIO_DEVICE_CHANNEL_L) ? 'L' : 'R');
    }
}



