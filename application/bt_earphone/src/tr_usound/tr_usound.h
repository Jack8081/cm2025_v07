/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tr_usound function.
 */


#ifndef _TR_USOUND_APP_H
#define _TR_USOUND_APP_H

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "tr_usound"
#include <logging/sys_log.h>
#endif

#include <audiolcy_common.h>
#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <usb/class/usb_audio.h>
#include "usb_audio_inner.h"
#include "usb_hid_inner.h"
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bt_manager.h>
#include <stream.h>
//#include <usb_audio_hal.h>
#include <property_manager.h>
#include "btservice_api.h"
#include "system_bt.h"

#include <thread_timer.h>

#ifdef CONFIG_LED
#include <ui_manager.h>
#endif

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
#include <dvfs.h>
#endif

#ifdef CONFIG_GUI
#include "../main/tr_app_view.h"
#include <api_common.h>
#endif

#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"
#include "config_al.h"

#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif
#include "soc_pinmux.h"
#define LEAUDIO_HIGH_AUDIO_QUALITY  /* 32bits output */

#define ENABLE_BT_DSP_XFER_BYPASS

#define LEAUDIO_MEDIA_HIGH_FREQ_MHZ  160

#define MAX_RECV_PKT_SIZE  128
#define MAX_SEND_PKT_SIZE  256

#define TX_ANCHOR_OFFSET_US 4500
#define RX_ANCHOR_OFFSET_US 5000

#ifdef CONFIG_SOC_SERIES_CUCKOO
#define CONFIG_DUMP_CRC_ERROR_PKT  0 //FIXME
#else
#define CONFIG_DUMP_CRC_ERROR_PKT  0
#endif


struct tr_usound_dsp_share_info_flags
{
	uint32_t enbale_share_info : 1;
	uint32_t dump_crc_error_pkt : 1;
	uint32_t reserved : 10;

	uint32_t recv_pkt_size : 10;
	uint32_t send_pkt_size : 10;
};


enum {
	MSG_USOUND_EVENT = MSG_APP_MESSAGE_START,
    MSG_SWITH_IOS,
};

enum {
	MSG_USOUND_PLAY_PAUSE_RESUME = 1,
	MSG_USOUND_PLAY_VOLUP,
	MSG_USOUND_PLAY_VOLDOWN,
#ifdef CONFIG_BT_MIC_GAIN_ADJUST
	MSG_USOUND_PLAY_GAIN_UP,
	MSG_USOUND_PLAY_GAIN_DOWN,
	MSG_USOUND_PLAY_MIC2_GAIN_UP,
	MSG_USOUND_PLAY_MIC2_GAIN_DOWN,
#endif
#ifdef CONFIG_BT_MIC_SUPPORT_DENOISE
	MSG_USOUND_PLAY_DENOISE_LEVEL_SET,
#endif
    MSG_USOUND_PLAY_KEY_SET_MIX_MODE,
    MSG_USOUND_PLAY_NEXT,
	MSG_USOUND_PLAY_PREV,
#ifdef CONFIG_BT_CHAN_SPD_TEST_ENABLE
	MSG_USOUND_HID_SPD,
#endif
#if defined CONFIG_BT_TRANS_BLEDATA_SERVER_FAREND && defined CONFIG_BT_FAREND_SPD_TEST_ENABLE
	MSG_USOUND_BLE_SPD,
#endif
};

enum {
	MSG_USOUND_STREAM_START = 1,
	MSG_USOUND_STREAM_STOP,
	MSG_USOUND_UPLOAD_STREAM_START,
	MSG_USOUND_UPLOAD_STREAM_STOP,
	MSG_USOUND_HOST_VOL_SYNC,
	MSG_USOUND_STREAM_MUTE,
	MSG_USOUND_STREAM_UNMUTE,
	MSG_USOUND_MUTE_SHORTCUT,

    MSG_USOUND_INCALL_RING,
    MSG_USOUND_INCALL_HOOK_UP,
    MSG_USOUND_INCALL_HOOK_HL,
    MSG_USOUND_INCALL_MUTE,
    MSG_USOUND_INCALL_UNMUTE,
    MSG_USOUND_INCALL_OUTGOING,
    MSG_USOUND_DEFINE_CMD_CONNECT_PAIRED_DEV,

};

enum USOUND_PLAY_STATUS {
	USOUND_STATUS_NULL = 0x0000,
	USOUND_STATUS_PLAYING = 0x0001,
	USOUND_STATUS_PAUSED = 0x0002,
};

enum USOUND_VOLUME_REQ_TYPE {
	USOUND_VOLUME_NONE = 0x0000,
	USOUND_VOLUME_DEC = 0x0001,
	USOUND_VOLUME_INC = 0x0002,
};

#define USOUND_STATUS_ALL  (USOUND_STATUS_PLAYING | USOUND_STATUS_PAUSED)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

enum
{
	MIC_CMD_NULL,
	MIC_CMD_STREAM_START,
	MIC_CMD_STREAM_STOP,
};

enum {
	PLAYBACK_REQ_NULL,
	PLAYBACK_REQ_STOP,
	PLAYBACK_REQ_START,
	PLAYBACK_REQ_RESTART,
};

enum
{
    VOICE_STATE_NULL,
    VOICE_STATE_INGOING,
    VOICE_STATE_OUTGOING,
    VOICE_STATE_ONGOING,
};


enum
{
/* Audio Source: capture_stream -> source_stream -> BT */
    SPK_CHAN,
/* Audio Sink: BT -> sink_stream -> playback_stream */
    MIC_CHAN,
};

struct _le_dev_t {
    uint16_t handle;
	uint16_t audio_handle;
	struct bt_audio_chan chan[2];

	uint32_t spk_chan_started : 1;
	uint32_t mic_chan_started : 1;


	uint32_t vcs_connected : 1;
    uint32_t client_vol_connected : 1;
};

struct tr_usound_app_t {
	struct thread_timer monitor_timer;
	struct thread_timer key_timer;
	struct thread_timer delay_play_timer;
	struct thread_timer tts_monitor_timer;
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    struct thread_timer auto_search_timer;
#endif

	uint32_t tts_playing : 1;
	uint32_t mic_mute : 1;
	uint32_t need_resume_key : 1;
	uint32_t key_handled : 1;

	uint32_t capture_player_load : 1;
	uint32_t capture_player_run : 1;

	uint32_t playback_player_load : 1;
	uint32_t playback_player_run : 1;

	uint32_t downchan_mute_data_detect_disable : 1;

	/* Audio Sink: BT -> sink_stream -> playback_stream */
	media_player_t *playback_player;
	io_stream_t sink_stream;

	/* Audio Source: capture_stream -> source_stream -> BT */
	media_player_t *capture_player;
	io_stream_t source_stream;

	uint8_t dae_enalbe;
	uint8_t cfg_dae_data[CFG_SIZE_BT_MUSIC_DAE_AL];
	uint8_t multidae_enable;
	uint8_t current_dae;
	uint8_t dae_cfg_nums;
	uint8_t set_share_info_flag;
	struct bt_dsp_cis_share_info *cis_share_info;
    io_stream_t recv_report_stream;	
    io_stream_t send_report_stream;
	uint32_t playback_delay;
	uint32_t cig_sync_delay;
	uint32_t cis_sync_delay;
	uint32_t cis_rx_delay;
	uint8_t cis_m_bn;
	uint8_t cis_m_ft;
	uint8_t curr_view_id;
	uint8_t last_disp_status;

	u8_t dev_num;;
    struct _le_dev_t dev[2];

    //283gh 原有结构 
//    mic_dev_t mic[2];
	u8_t remote_mic_num;
#ifdef CONFIG_DOWNLOAD_SHARE_ME_EN
    u8_t remote_spk_chan_num;
#endif
    u8_t mic_cmd;
	void *cache_buf;
    u8_t chan_reconf;
	
    void *spk_chan ;
#if (CONFIG_BT_TRANS_SUPPORT_CIS_NUMBER == 2)
    void *ready_mic_chan[2];
    u8_t ready_mic_num;
#endif

	struct thread_timer host_vol_sync_timer;
	u16_t playing : 1;
	u16_t need_resume_play : 1;
	u16_t media_opened : 1;
	u16_t host_vol_sync : 1;
	u16_t volume_req_type : 2;
    u16_t auto_open_listen:1;
    u16_t bt_first_cnt:1;
#if (CONFIG_LISTEN_OUT_CHANNEL == 2)
    u16_t listen_out_channel:1;
#endif
    u16_t upload_start:1;

    void *tx_tl;

	short volume_req_level : 8;
	short current_volume_level : 8;
    u8_t host_vol;
    u8_t host_mic_volume;
    u8_t host_mic_muted;
    u8_t host_hid_mic_muted;

	/* Audio Source: tr_usound_stream -> source_stream -> BT */
	media_player_t *player;	/* capture */
    //io_stream_t tr_usound_stream;
    io_stream_t usb_download_stream;
    struct acts_ringbuf *tr_usound_stream_ringbuf;
	io_stream_t capture_output_stream;
	int lost_pkt_num;
	
	/* Audio Sink: BT -> sink_stream -> tr_usound_upload_stream */
	//io_stream_t tr_usound_upload_stream;
	io_stream_t usb_upload_stream;
	
    u8_t playback_request;
	struct thread_timer playback_delay_timer;

    u8_t interval_count;
    u8_t last_usb_chan_is_valid;
	uint32_t last_usb_count;
	void *last_usb_chan;

	struct thread_timer usb_event_barrier_timer;
#if 0
	uint8_t usb_sink_start : 1;	/* usb audio sink start */
	uint8_t bt_sink_start : 1;	/* remote bt audio sink start */
	uint8_t usb_sink_started : 1;
	uint8_t usb_source_start : 1;
	uint8_t usb_source_started : 1;
#endif
	uint8_t usb_sink_started : 1;	/* usb audio sink started */
	uint8_t bt_source_configured : 1;	/* bt audio source configured */
	uint8_t usb_to_bt_started : 1;

	uint8_t usb_source_started : 1;	/* usb audio source started */
	uint8_t bt_sink_configured : 1;	/* bt audio sink configured */
	uint8_t bt_sink_started : 1;	/* bt audio sink started */
	uint8_t bt_to_usb_started : 1;	// TODO:

	uint8_t req_fill_zero : 1;
	uint8_t req_quit : 1;

	uint8_t voice_state_need_rest: 1;

	uint8_t usb_to_bt_fill_count;
	uint8_t usb_download_started;
	uint8_t usb_download_mute;
    
	u8_t dvfs_level_2mic;
#ifdef CONFIG_BT_MIC_GAIN_ADJUST
	u8_t bt_mic_gain[2];
#endif

#if (CONFIG_USB_AUDIO_UPLOAD_CHANNEL_NUM == 2)
    u8_t bt_mic_mix_mode;
#endif

	uint32_t pre_soft_coef[2];
	uint32_t post_soft_coef[2];
	uint32_t channel_power[2];
#ifdef CONFIG_GUI
    u8_t gainf_flag;
    u8_t connect_flag;
    u8_t mute0_flag;
    u8_t mute1_flag;
    u8_t rssi0_tmp;
    u8_t rssi1_tmp;
    u8_t bat0_note;
    u8_t bat1_note;
    u8_t gain0_show;
    u8_t gain1_show;
	struct thread_timer gain_to_energy_timer;
	struct thread_timer energy_show_timer;
	struct thread_timer bt_signal_show_timer;
	struct thread_timer mic2_bat_show_timer;
#endif
    u8_t *bt0_name;
    u8_t *bt1_name;
    u8_t bt0_state;
    u8_t bt1_state;
	struct thread_timer bt_nameget_timer;
#ifdef CONFIG_BT_CHAN_SPD_TEST_ENABLE
    u8_t test_buf[64];
    uint16_t bt_spd_handle;
    struct device *gpio_dev;
    u32_t spd_time;
    u32_t pin_mode;
#endif

	uint8_t repairing;
	struct thread_timer repair_delay_timer;

    uint8_t voice_state;
    uint8_t voice_state_last;
    struct audio_record_t *audio_record;
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
	struct thread_timer usb_cis_start_stop_work;
#endif
};

int iap2_protocol_init();
void iap2_protocol_deinit();

void tr_usound_bt_event_proc(struct app_msg *msg);
void tr_usound_tts_event_proc(struct app_msg *msg);
void tr_usound_input_event_proc(struct app_msg *msg);
void tr_usound_tws_event_proc(struct app_msg *msg);
void tr_usound_view_init(uint8_t view_id, bool key_remap);
void tr_usound_view_deinit(void);
void tr_usound_view_switch(uint8_t view_id);
void tr_usound_view_display(void);
void tr_usound_call_view_display(void);
void tr_usound_event_notify(int event);

int tr_usound_init_capture(void);
int tr_usound_start_capture(void);
int tr_usound_stop_capture(void);
int tr_usound_exit_capture(void);
int tr_usound_init_playback(void);
int tr_usound_start_playback(void);
int tr_usound_stop_playback(void);
int tr_usound_exit_playback(void);
struct tr_usound_app_t *tr_usound_get_app(void);

void tr_usound_start_play(void);
void tr_usound_stop_play(void);

void tr_usound_set_share_info(bool enable, bool update);
void tr_usound_event_proc(struct app_msg *msg);

/* profile datas */

/*unit is 0.1MHz*/
/*dimensional-1: lc3 interval, [0] 10ms, [1] 7.5ms*/
/*dimensional-2: lc3 bit width, [0] 16bits, [1] 24bits*/
/*dimensional-3: lc3 samplerate, [0] 48khz, [1] 32khz, [2] 24khz, [3] 16khz*/

/*dimensional-4: lc3 bitrate, [0] 192kbps, [1] 184kbps, [2] 176kbps, [3] 160kbps, [4] 158kbps*/
extern const uint16_t lc3_decode_run_mhz_dsp_music_2ch[2][2][1][LC3_BITRATE_MUSIC_2CH_COUNT];
/*dimensional-4: lc3 bitrate, [0] 80kbps, [1] 78kbps*/
extern const uint16_t lc3_decode_run_mhz_dsp_music_1ch[2][2][1][LC3_BITRATE_MUSIC_1CH_COUNT];
/*dimensional-4: lc3 bitrate, [0] 80kbps, [1] 78kbps, [2] 64kbps, [3] 48kbps, [4] 32kbps*/
extern const uint16_t lc3_decode_run_mhz_dsp_call_1ch[2][2][4][LC3_BITRATE_CALL_1CH_COUNT];
extern const uint16_t lc3_encode_run_mhz_dsp_call_1ch[2][2][4][LC3_BITRATE_CALL_1CH_COUNT];

/*unit is 0.1MHz*/
/*dimensional-1: lc3 samplerate, [0] 48khz, [1] 32khz, [2] 24khz, [3] 16khz*/
extern const uint16_t postdae_run_mhz_dsp_music_2ch[4];
extern const uint16_t postdae_run_mhz_dsp_music_1ch[4];
extern const uint16_t postdae_run_mhz_dsp_call_1ch[4];
extern const uint16_t predae_run_mhz_dsp_call_1ch[4];
extern const uint16_t preaec_dual_mic_run_mhz_dsp_call_1ch[4];
extern const uint16_t preaec_single_mic_run_mhz_dsp_call_1ch[4];

extern const uint16_t playback_system_run_mhz;
extern const uint16_t capture_system_run_mhz;

struct _le_dev_t *tr_usound_dev_add(uint16_t handle);
int tr_usound_dev_del(uint16_t handle);
int tr_usound_get_dev_num(void);
struct bt_audio_chan *tr_usound_dev_add_chan(uint16_t handle, uint8_t dir, uint8_t id);
struct _le_dev_t *tr_usound_find_dev(uint16_t handle);
struct bt_audio_chan *tr_usound_find_chan_by_id(uint16_t handle, uint8_t id);
struct bt_audio_chan *tr_usound_find_chan_by_dir(uint16_t handle, uint8_t dir);
uint8_t tr_usound_get_chan_dir(uint16_t handle, uint8_t id);
#endif  /* _USOUND_APP_H */

