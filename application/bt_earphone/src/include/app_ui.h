/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app ui define
 */

#ifndef _APP_UI_EVENT_H_
#define _APP_UI_EVENT_H_
#include <sys_event.h>
#include <msg_manager.h>
#include <config.h>
#include "ui_key_map.h"
#include <bt_manager.h>
#include "app_common.h"
#include "app_config.h"

//APP msg
enum {
	MSG_APP_KEY_TONE_PLAY = MSG_APP_MESSAGE_START,

	MSG_APP_UI_EVENT,
	MSG_APP_BTCALL_EVENT,
	MSG_APP_DC5V_IO_CMD,
	MSG_APP_SYNC_KEY_MAP,
	MSG_APP_SYNC_MUSIC_DAE,
	MSG_APP_UPDATE_MUSIC_DAE,
	MSG_APP_MESSAGE_MAX_INDEX,
};

//APP define sys event
enum {
	SYS_EVENT_ALEXA_TONE_PLAY = SYS_EVENT_APP_DEFINE_START,
    SYS_EVENT_BT_CONNECTED_USB,
    SYS_EVENT_BT_DISCONNECTED_USB,
    SYS_EVENT_BT_CONNECTED_BR,
    SYS_EVENT_BT_DISCONNECTED_BR,
    SYS_EVENT_ENTER_PAIRING_MODE,
};



enum TWS_APP_EVENT_ID
{
    /***************************************************************************************
	 * 下面消息是TWS短消息，通过  TWS_SHORT_MSG_EVENT发送.
	 *
	 * 例如：
	 * bt_manager_tws_send_message(TWS_SHORT_MSG_EVENT, TWS_APP_EVENT_KEY_TONE_PLAY, NULL, 0)
     *
     **************************************************************************************/
    TWS_EVENT_SYSTEM_KEY_CTRL = TWS_EVENT_APP_MSG_BEGIN_ID,      // 系统控制
    TWS_EVENT_BT_MUSIC_KEY_CTRL,      // 蓝牙音乐控制
    TWS_EVENT_BT_CALL_KEY_CTRL,       // 蓝牙通话控制
	TWS_EVENT_KEY_CTRL_COMPLETE,	  // 按键控制完成

    TWS_EVENT_KEY_TONE_PLAY,          // 播放按键音

	TWS_EVENT_POWER_OFF,			  // 关机

	TWS_EVENT_IN_CHARGER_BOX_STATE,   // 是否在充电盒中状态
	
	TWS_EVENT_REMOTE_KEY_PROC_BEGIN,  // 对端按键处理开始
	TWS_EVENT_REMOTE_KEY_PROC_END,	  // 对端按键处理结束
	TWS_EVENT_AVDTP_FILTER, 		  // AVDTP 数据过滤
	TWS_EVENT_BT_CALL_START_RING,	  // 开始来电响铃
	TWS_EVENT_BT_CALL_RING_NUM_BEGIN, // 开始播放来电号码
	TWS_EVENT_BT_CALL_RING_NUM_END,   // 结束播放来电号码
	
    TWS_EVENT_DIAL_PHONE_NO,          // 拨打指定电话号码
    TWS_EVENT_WAIT_DIAL,              // 等待拨打电话
    TWS_EVENT_WAIT_VOICE_ASSIST,      // 等待启动 Siri 等语音助手    

    TWS_EVENT_SET_VOICE_LANG,         // 设置语音语言
    TWS_EVENT_SYNC_LOW_LATENCY_MODE,  // 同步低延迟模式
    TWS_EVENT_BT_MUSIC_PLAYER_ERR,    // 蓝牙音乐播放器错误
    TWS_EVENT_OTA_CMD,                // TWS OTA命令
    TWS_EVENT_SYNC_PRIVMA_STATUS,     // 同步SPP/BLE协议连接状态
    TWS_EVENT_SYNC_CHG_BOX_STATUS,    // 同步充电盒状态

    TWS_EVENT_SYNC_BT_MUSIC_DAE,      // 同步蓝牙音乐音效
    TWS_EVENT_SYNC_TRANSPARENCY_MODE, // 同步通透模式状态    
    
    TWS_EVENT_AVDTP_UNFILTER_AFTER_BT_CALL,  // 蓝牙通话结束后取消 AVDTP 数据过滤

    TWS_EVENT_BT_CALL_PRE_RING,       // 准备来电响铃
	TWS_EVENT_SYNC_STATUS_INFO, 	  //同步应用状态信息


    TWS_EVENT_AP_RECORD_STATUS,          //录音状态

    TWS_EVENT_BT_MUSIC_START,          //音乐起始播放时间
    TWS_EVENT_BT_CALL_STOP,           //通话停止播放时间
    /***************************************************************************************
	 * 下面消息是TWS短消息，通过  TWS_LONG_MSG_EVENT发送.
	 *
	 * 例如：
	 * bt_manager_tws_send_message(TWS_LONG_MSG_EVENT, TWS_EVENT_TWS_OTA, data, len)
     *
     **************************************************************************************/
    /* 长数据事件消息
     */
    TWS_EVENT_TWS_OTA = TWS_EVENT_APP_LONG_MSG_BEGIN_ID,  // TWS OTA大数据传输

    TWS_EVENT_APOTA_SYNC,  // 类recovery ota接口数据传输

	TWS_EVENT_SPP_TEST,  // SPP TEST
	
	TWS_EVENT_APP_SETTING_KEY_MAP,  // 同步手机应用设置下来的按键映射表
	
	TWS_EVENT_APP_ROLE_SWITCH,  // spp/ble role switch

    TWS_EVENT_ASET,  // aset sync dae/aec param
    TWS_EVENT_CONSUMER_EQ,  // sync consumer eq param
};


enum APP_UI_VIEW_ID {
    MAIN_VIEW,
	BTMUSIC_VIEW,
	BTCALL_VIEW,
	CHARGER_VIEW,
	OTA_VIEW,
#ifdef CONFIG_BT_BR_TRANSCEIVER
    TR_BQB_VIEW,
    TR_BQB_AG_VIEW,
#endif
};


struct event_strmap_s {
	int8_t event;
	const char *str;
};

struct event_ui_map {
    uint8_t  sys_event;
    uint8_t  ui_event;
};

#define MAX_PHONENUM                   (16)

typedef struct
{
    bool   is_play_tone;
    u8_t   voice_list[1 + MAX_PHONENUM];
    int    voice_num;
    void  (*start_callback)(void);
    void  (*end_callback)(void);
} event_call_incoming_t;

typedef struct
{
    u8_t  bt_music_vol;
    u8_t  bt_call_vol;
    u8_t  voice_language;  //!< 当前选择语音语言
    u8_t  low_latency_mode;	
	u8_t  dae_index;
    u8_t  anc_mode;
} app_tws_sync_status_info_t;

void app_msg_print(const struct app_msg* msg, const char* func_name);

#endif
