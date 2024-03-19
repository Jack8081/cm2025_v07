/*!
 * \file      app_common.h
 * \brief     应用通用接口
 * \details   
 * \author    
 * \date      
 * \copyright Actions
 */

#ifndef _APP_AUDIO_CHANNEL_H
#define _APP_AUDIO_CHANNEL_H


#include <config.h>

#define TWS_AUDIO_CHANNEL       "TWS_AUDIO_CH"  /*1:L, 2:R*/
#define BTMUSIC_MULTI_DAE       "BTMUSIC_MULTI_DAE"
enum UI_EVENT_APP_RESERVED {
    UI_EVENT_BT_MUSIC_DISP_NONE = 0xf1,
    UI_EVENT_BT_CALL_DISP_NONE,
};

/*!
 * \brief 音频管理配置结构类型
 */
typedef struct
{
    unsigned char  hw_sel_dev_channel;     //!< 硬件设定设备声道
    unsigned char  sw_sel_dev_channel;     //!< 软件设定设备声道

	unsigned char  reomte_sel_dev_channel;  //TWS远端设备声道
} audio_manager_configs_t;


#define AUDIO_DEVICE_CHANNEL_L  1  //!< 左声道
#define AUDIO_DEVICE_CHANNEL_R  2  //!< 右声道

typedef enum
{
    AUDIO_TRACK_NORMAL = 0,      /* L = L0, R = R0 */
    AUDIO_TRACK_L_R_SWITCH,      /* L = R0, R = L0 */
    AUDIO_TRACK_L_R_MERGE,       /* L = R = (L0 + R0) / 2 */
    AUDIO_TRACK_L_R_ALL_L,       /* L = R = L0 */
    AUDIO_TRACK_L_R_ALL_R        /* L = R = R0 */
} audio_track_e;


audio_manager_configs_t* audio_channel_get_config(void);


void audio_channel_init(void);

/* 喇叭输出选择
 */
void audio_speaker_out_select(int* L, int* R);

/* 声道选择
 * 返回值见：media_effect_output_mode_e
 */
int audio_channel_sel_track(void);

/*!
 * \brief 状态事件 LED 显示 (不播放提示音或语音)
 * \n  ui_event : CFG_TYPE_SYS_EVENT
 */
void ui_event_led_display(uint32_t ui_event);


/*!
 * \brief 事件通知 (播放提示音和语音,显示LED)
 * \n  event_id : CFG_TYPE_SYS_EVENT
 * \n  wait_end : 是否等待通知结束 (播放完提示音和语音) 
 */
void ui_event_notify(uint32_t ui_event, void* param, bool wait_end);

void sys_manager_event_notify(uint32_t sys_event, bool sync_wait);


#endif  // _APP_AUDIO_CHANNEL_H

