/*!
 * \file      bqb_app.h
 * \brief     音频输入应用头文件
 * \details
 * \author    Tommy Wang
 * \date
 * \copyright Actions
 */

#ifndef _TR_BQB_APP_H
#define _TR_BQB_APP_H

#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "bqb"
#ifdef SYS_LOG_LEVEL
#undef SYS_LOG_LEVEL
#endif
#define SYS_LOG_LEVEL 4		/* 4, debug */
//#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <section_overlay.h>

#include <thread_timer.h>

#include "msg_manager.h"
#include "mem_manager.h"
//#include "global_mem.h"
#include "stream.h"
#include "dvfs.h"
#include "btservice_api.h"

#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"
#include "app_switch.h"
#include <bt_manager.h>
//#include <app_message.h>

#include <media_player.h>

#include "srv_manager.h"
#include "app_manager.h"
#include "api_bt_module.h"
#include "audio_system.h"

#define BQB_STATUS_ALL  (BQB_STATUS_PLAYING | BQB_STATUS_PAUSED)

#ifdef CONFIG_BT_TRANS_A2DP_48K_EN
#define BQB_SAMPLE_RATE                         (SAMPLE_48KHZ)
#else
#define BQB_SAMPLE_RATE                         (SAMPLE_44KHZ)
#endif

/* 蓝牙推歌按键功能*/
enum BQB_KEY_FUN
{
    BQB_KEY_NULL,
    BQB_KEY_PLAY_PAUSE,
    BQB_KEY_VOLUP,
    BQB_KEY_VOLDOWN,
};

enum {
    MSG_BQB_PLAY_PAUSE_RESUME = MSG_APP_MESSAGE_START,
    MSG_BQB_PLAY_VOLUP,
    MSG_BQB_PLAY_VOLDOWN,
    MSG_BQB_CONNECT_START,
    MSG_BQB_CONNECT_DISCONNECT_A2DP,
    MSG_BQB_CONNECT_DISCONNECT_ACL,
    MSG_BQB_MUTE,
    MSG_BQB_CLEAR_LIST,
};

enum BQB_PLAY_STATUS
{
    BQB_STATUS_NULL      = 0x0000,
    BQB_STATUS_PAUSED    = 0x0001,
    BQB_STATUS_PLAYING   = 0x0002,
};


struct tr_bqb_app_t
{
    media_player_t *player;
    media_player_t *recorder;
    struct thread_timer monitor_timer;
    u32_t recording : 1;
	u32_t need_resample : 1;
    u8_t pass_avcrp_cmd : 1;
    io_stream_t audio_out_stream;
	
    u8_t last_connect_dev_bd_addr[6];
    u32_t avrcp_barrier_time_start;
};

struct tr_bqb_app_t *tr_bqb_get_app(void);


void tr_bqb_input_event_proc(struct app_msg *msg);
void tr_bqb_bt_event_proc(struct app_msg *msg);

void tr_bqb_view_init(void);
void tr_bqb_view_deinit(void);
void tr_bqb_start_or_stop_record(void);
int tr_bqb_get_status(void);




/* 按键播放/暂停
 */
void bqb_key_func_play_pause(void);

void bqb_volume_init(void);

/* 音量控制
 */
bool deal_key_for_bqb(int key_event);
void bqb_key_func_play_ctrl(u32_t func_id);
void bqb_key_func_volume_ctrl(u32_t func_id);

int bqb_bt_event_msg_proc(void *msg_data);

bool bqb_key_event_handle(int key_event, int event_stage);

void bqb_stop_deal(void);

void bqb_input_event_msg_proc(struct app_msg *msg);
void tr_bqb_open_record(void);

void tr_bqb_stop_record(void);
void tr_bqb_start_record(void);

#endif  // _BQB_APP_H
