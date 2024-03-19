#include <bt_manager.h>
#include <tr_app_switch.h>
#include <app_manager.h>
#include <app_defines.h>
#include <drivers/nvram_config.h>
#include <audio_system.h>
#include <api_common.h>

int check_prio(const uint8_t *name)
{
    int cur_prio = k_thread_priority_get(k_current_get());
    if(cur_prio < -1)
    {
        SYS_LOG_WRN("%s cur prio %d,  recommend prio gt -1\n", name, cur_prio);
        return 1;
    }
    return 0;
}

#ifdef CONFIG_BT_MIC_GAIN_ADJUST

#ifdef CONFIG_TR_USOUND_APP
extern int tr_usound_get_energy(int mic_dev_num, bool is_original);
extern void tr_usound_mute_mic(int dev_num, bool is_mute);
extern int tr_usound_get_gain_level(int mic_dev_num);
extern void tr_usound_set_mix_mode(int mix_mode);
extern int tr_usound_get_mix_mode(void);
#if (CONFIG_LISTEN_OUT_CHANNEL == 2)
extern void tr_usound_mod_listen_out_chan(int out_channel);
#endif
#endif
#ifdef CONFIG_TR_LINE_IN_APP
extern int tr_audio_in_get_energy(int mic_dev_num, bool is_original);
extern void tr_audio_in_mute_mic(int dev_num, bool is_mute);
extern int tr_audio_in_get_gain_level(int mic_dev_num);
extern void tr_audio_in_set_mix_mode(int mix_mode);
extern int tr_audio_in_get_mix_mode(void);
#if (CONFIG_LISTEN_OUT_CHANNEL == 2)
extern void tr_audio_in_mod_listen_out_chan(int out_channel);
#endif
#endif

#if 0
int API_APP_GAIN_SET(int dev_num, int gain_level)
{
    CK_PRIO();
    int ret = 0;
    u8_t bt_mic_gain = gain_level;
    struct conn_status status;
    struct bt_audio_chan *chan = NULL;
    if(sys_get_work_mode())
    {
        tr_bt_manager_get_audio_dev_status(dev_num, &status, 1);
        if(status.connected)
        {
            chan = status.sink_chan;
        }

        ret = tr_bt_manager_audio_stream_update(chan, NULL, &bt_mic_gain);
    }else
    {
#ifdef CONFIG_BT_LE_AUDIO
        extern struct bt_audio_chan * leaudio_get_source_chan(void);
        chan = leaudio_get_source_chan();

        ret = tr_bt_manager_audio_stream_update(chan, NULL, &bt_mic_gain);
#endif
    }
    return ret;
}

u16_t API_APP_GET_ENERGY_VAL(int mic_dev_num, bool is_original)
{
    CK_PRIO();
#ifdef CONFIG_UP_LOAD_ENERGY_DETECT
    void *tr_fg_app = app_manager_get_current_app();
    if (tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_USOUND))
    {
#ifdef CONFIG_TR_USOUND_APP
        return tr_usound_get_energy(mic_dev_num, is_original);
#endif
    }
    else if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_LINE_IN))
    {
#ifdef CONFIG_TR_AUDIO_IN_APP
        return tr_audio_in_get_energy(mic_dev_num, is_original);
#endif
    }
#endif
    return 0;
}

int API_APP_GET_GAIN_LEVEL(int mic_dev_num) // mic_dev_num为获取设备号,返回对应设备的增益等级
{
    CK_PRIO();
    void *tr_fg_app = app_manager_get_current_app();
    if (tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_USOUND))
    {
#ifdef CONFIG_TR_USOUND_APP
        return tr_usound_get_gain_level(mic_dev_num);
#endif
    }
    else if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_LINE_IN))
    {
#ifdef CONFIG_TR_AUDIO_IN_APP
        return tr_audio_in_get_gain_level(mic_dev_num);
#endif
    }
    return -1;
}

void API_APP_SET_MIX_MODE(int mix_mode) //mix_mode=0为单声道模式， mix_mode=1为立体声道模式 ， mix_mode=2为安全模式
{
    CK_PRIO();
    void *tr_fg_app = app_manager_get_current_app();
    if (tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_USOUND))
    {
#ifdef CONFIG_TR_USOUND_APP
        tr_usound_set_mix_mode(mix_mode);
#endif
    }
    else if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_LINE_IN))
    {
#ifdef CONFIG_TR_AUDIO_IN_APP
        tr_audio_in_set_mix_mode(mix_mode);
#endif
    }
}

int API_APP_GET_MIX_MODE(void) //mix_mode=0为单声道模式， mix_mode=1为立体声道模式 ， mix_mode=2为安全模式
{
    CK_PRIO();
    void *tr_fg_app = app_manager_get_current_app();
    if (tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_USOUND))
    {
#ifdef CONFIG_TR_USOUND_APP
        return tr_usound_get_mix_mode();
#endif
    }
    else if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_LINE_IN))
    {
#ifdef CONFIG_TR_AUDIO_IN_APP
        return tr_audio_in_get_mix_mode();
#endif
    }
    return -1;
}

#if (CONFIG_LISTEN_OUT_CHANNEL == 2)
void API_APP_MOD_LISTEN_OUT_CHAN(int out_chan)
{
    CK_PRIO();
    void *tr_fg_app = app_manager_get_current_app();
    if (tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_USOUND))
    {
#ifdef CONFIG_TR_USOUND_APP
        tr_usound_mod_listen_out_chan(out_chan);
#endif
    }
    else if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_LINE_IN))
    {
#ifdef CONFIG_TR_AUDIO_IN_APP
        tr_audio_in_mod_listen_out_chan(out_chan);
#endif
    }
}
#endif

#endif

u8_t mic_mute_flag = 0;
int API_APP_MUTE_MIC(int mic_dev_num, bool is_mute) //mic_dev_num:设备号, is_mute 为true是静音，为false为取消静音
{
    CK_PRIO();
    void *tr_fg_app = app_manager_get_current_app();
    if (tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_USOUND))
    {
#if defined(CONFIG_TR_USOUND_APP) && defined(CONFIG_BT_MIC_GAIN_ADJUST)
        tr_usound_mute_mic(mic_dev_num, is_mute);
#endif
    }
    else if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_LINE_IN))
    {
#if defined(CONFIG_TR_AUDIO_IN_APP) && defined(CONFIG_BT_MIC_GAIN_ADJUST)
        tr_audio_in_mute_mic(mic_dev_num, is_mute);
#endif
    }
    else if(tr_fg_app && (!strcmp(tr_fg_app, APP_ID_LE_AUDIO) || !strcmp(tr_fg_app, APP_ID_BTMUSIC) || !strcmp(tr_fg_app, APP_ID_BTCALL)))
    {
#ifndef CONFIG_TR_LE_AUDIO_CALL
        if(is_mute == 1)
            bt_manager_mic_mute();
        else if(is_mute == 0)
            bt_manager_mic_unmute();
#endif
        if(!strcmp(tr_fg_app, APP_ID_LE_AUDIO))
        {
#ifndef CONFIG_BT_MIC_ENABLE_DOWNLOAD_MIC         //在使能下行麦对讲功能下不在近端静麦,远端静麦代之 
            audio_system_mute_microphone(is_mute);
#endif
        }
        else
        {
            audio_system_mute_microphone(is_mute);
        }
        mic_mute_flag = is_mute;
    }
    return 0;
}


void API_APP_SET_PRE_MUSIC_EFX(int efx_mode)
{
    CK_PRIO();
    void *tr_fg_app = app_manager_get_current_app();
    if (tr_fg_app && !strcmp(tr_fg_app, APP_ID_BTMUSIC))
    {
#ifdef CONFIG_BT_MUSIC_APP
#ifdef CONFIG_BT_MUSIC_MOD_EFX_SUPPORT
        extern void mod_music_efx(int efx_mode);
        mod_music_efx(efx_mode);
#endif
#endif
    }
    else if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_LE_AUDIO))
    {
#ifdef CONFIG_BT_LE_AUDIO
#ifdef CONFIG_LE_AUDIO_MOD_EFX_SUPPORT
        extern void leaudio_mod_music_efx(int efx_mode);
        leaudio_mod_music_efx(efx_mode);
#endif
#endif
    }
}

void API_APP_SETPAIRADDR(u8_t index, u8_t *param)
{
    CK_PRIO();
    u8_t pairaddr[6] = {0};
    
    SYS_LOG_INF("set pairaddr:%d\n", index);
	memcpy(pairaddr, param, 6);
    if(index == 0)
    {
	    nvram_config_set("BINDING_BT_ADDR_L", pairaddr, 6);
    }
    else if(index == 1)
    {
	    nvram_config_set("BINDING_BT_ADDR_R", pairaddr, 6);
    }
}

void API_APP_GETPAIRADDR(u8_t index, u8_t *param)
{
    CK_PRIO();
    SYS_LOG_INF("get pairaddr:%d\n", index);
    if(index == 0)
    {
	    nvram_config_get("BINDING_BT_ADDR_L", param, 6);
    }
    else if(index == 1)
    {
	    nvram_config_get("BINDING_BT_ADDR_R", param, 6);
    }
}
#endif
void API_APP_SCAN_WORK(u8_t mode)
{
    CK_PRIO();
    tr_bt_manager_audio_scan_mode(mode);
}

void API_APP_ADV_WORK(u8_t mode)
{
    CK_PRIO();
    tr_bt_manager_audio_adv_mode(mode);
}

#ifdef CONFIG_MEDIA_EFFECT
extern int _music_peq_modify(int eq_point, point_info_t *eq_point_info, short peq_mod_type);
#endif
int API_APP_MOD_PEQ(int eq_point, point_info_t *point_info, short peq_mod_type)
{
    int ret = 0;
    void *tr_fg_app = app_manager_get_current_app();
    if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_LE_AUDIO))
    {
#ifdef CONFIG_VOICE_EFFECT
        ret = _music_peq_modify(eq_point, point_info, peq_mod_type);
#endif
    }
    else
    {
        ret = -2;
    }
    SYS_LOG_INF("peq_mod_type：0x%x\n", peq_mod_type);

    return ret;
}

#ifdef CONFIG_MEDIA_EFFECT
extern int _music_multi_peq_modify(point_info_t *eq_point_info, int q_valid, int type_valid, int en_valid);
#endif
int API_APP_MOD_MULTI_PEQ(point_info_t *point_info, int q_valid, int type_valid, int en_valid)
{
    int ret = 0;
    void *tr_fg_app = app_manager_get_current_app();
    if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_LE_AUDIO))
    {
#ifdef CONFIG_VOICE_EFFECT
        ret = _music_multi_peq_modify(point_info, q_valid, type_valid, en_valid);
#endif
    }
    else
    {
        ret = -2;
    }

    return ret;
}

#ifdef CONFIG_MEDIA_EFFECT
extern int _music_peq_get(int eq_point, point_info_t *eq_point_info);
#endif

int API_APP_GET_PEQ(int eq_point, point_info_t *point_info)
{
    int ret = 0;
    if(!point_info)
    {
        ret = -2;
        return ret;
    }

    void *tr_fg_app = app_manager_get_current_app();
    if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_LE_AUDIO))
    {
#ifdef CONFIG_VOICE_EFFECT
        ret = _music_peq_get(eq_point, point_info);
#endif
    }
    else
    {
        ret = -2;
    }
    SYS_LOG_INF("peq point_info_%d:\nfc:%d\ngain:%d\nqvalue:%d\npoint_type:%d\npoint_en:%d\n", eq_point, point_info->eq_fc, 
            point_info->eq_gain, point_info->eq_qvalue, point_info->eq_point_type, point_info->eq_point_en);

    return ret;
}
