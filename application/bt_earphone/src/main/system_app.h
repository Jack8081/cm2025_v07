/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app
 */

#ifndef _SYSTEM_APP_H_
#define _SYSTEM_APP_H_
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <srv_manager.h>
#include <app_manager.h>
#include <hotplug_manager.h>
#include <power_manager.h>
#include <input_manager.h>
#include <property_manager.h>
#include <sys_monitor.h>
#include "app_defines.h"
#include <sys_manager.h>
#include <thread_timer.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif
#include <list.h>
#include "app_ui.h"
#include "app_switch.h"
#include "app_defines.h"
#include "system_util.h"

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "system_app"
#ifdef SYS_LOG_LEVEL
#undef SYS_LOG_LEVEL
#endif

#ifdef CONFIG_BLUETOOTH
#include "mem_manager.h"
#include "btservice_api.h"
#endif

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
#include <drivers/gpio.h>

#define AUX_GPIO_PORT_NAME      (CONFIG_GPIO_A_NAME)
#define AUX_GPIO_PIN_IDX        (22)
#define AUX_GPIO_PIN_MASK       (0x01 << AUX_GPIO_PIN_IDX)
#endif

#define CFG_VOICE_LANGUAGE  "VOICE_LANG"
#define CFG_UPDATED_CONFIG	"UPDATED_CONFIG"
#define CFG_ENTER_BQB_TEST_MODE	"BQB_TEST_MODE"
#define CFG_CHG_BOX_BAT_LEVEL "CHG_BOX_BAT_LEVEL"
#define CFG_PT_CFO_ADJUST_PARAM "PT_CFO_ADJUST_PARAM"

#define APP_WAIT_TWS_READY_SEND_SYNC_TIMEOUT		500		/* 500ms */

enum{
	SYS_INIT_NORMAL_MODE,
	SYS_INIT_ATT_TEST_MODE,
	SYS_INIT_ALARM_MODE,
};

enum {
    SYS_BLE_LIMITED_OTA    = (1 << 0),
    SYS_BLE_LIMITED_APP    = (1 << 1),
};

enum{
    SYS_ANC_OFF_MODE = 0,
    SYS_ANC_DENOISE_MODE,
    SYS_ANC_TRANSPARENCY__MODE,
    SYS_ANC_WINDY__MODE,
    SYS_ANC_LEISURE__MODE,
    SYS_ANC_MODE_MAX
};

typedef struct
{
    CFG_Struct_System_Settings    cfg_sys_settings;
    CFG_Struct_System_More_Settings	cfg_sys_more_settings;
    CFG_Struct_ONOFF_Key          cfg_onoff_key;
    CFG_Struct_Key_Tone           cfg_key_tone;
	CFG_Struct_Event_Notify       cfg_event_notify;
    CFG_Struct_TWS_Sync           cfg_tws_sync;
    CFG_Struct_Battery_Charge     cfg_charge; /* battery charge configuration */
    CFG_Struct_Charger_Box        cfg_charger_box;

    CFG_Type_Auto_Quit_BT_Ctrl_Test  cfg_auto_quit_bt_ctrl_test;
	
    u16_t  bat_low_prompt_interval_sec;  // 电量低提示间隔时间 (秒), 0 ~ 600, 设置为 0 时只提示一次

    u8_t   tap_key_tone;              // 敲击按键音,     CFG_TYPE_TONE_ID
} system_configs_t;

typedef struct
{
    u8_t  in_front_charge;
    u8_t  in_test_mode;
    u8_t  in_power_off_stage;
    u8_t  in_power_off_charge;
    u8_t  bat_is_too_low;
    u8_t  bat_too_low_powoff;
    u8_t  charge_state;
    u8_t  charger_box_bat_level;
    u8_t  charger_box_opened;
    u8_t  in_charger_box;
    u8_t  audio_media_mutex_stopped;
    u8_t  in_ota_update;
    u8_t  tws_remote_in_charger_box;
    u8_t  chg_box_bat_low;

    u8_t  disable_auto_standby;
    u8_t  disable_front_charge;
    u8_t  disable_key_tone;
    u8_t  disable_audio_tone;
    u8_t  disable_audio_voice;

    u16_t reboot_type;
    u8_t  reboot_reason;
    u8_t  anc_mode;
} system_status_t;


typedef struct
{
    system_configs_t  sys_configs;
    system_status_t   sys_status;

    struct list_head  notify_ctrl_list;
	struct thread_timer notify_ctrl_timer;
	struct thread_timer notify_slave_timer;
    struct thread_timer front_charge_timer;
    struct thread_timer bat_low_prompt_timer;
    struct thread_timer poweroff_charge_timer;
    struct thread_timer dc5v_io_timer;
    struct thread_timer ble_delay_pwroff_timer;

    u8_t   boot_hold_key_func;
	struct thread_timer boot_hold_key_release_timer;
    u8_t   key_tone_play_msg_count;
	
	bool   att_enter_bqb_flag;
	bool   bt_manager_config_inited;
	
    u8_t   select_voice_language;  //!< 当前选择语音语言
    u8_t   alter_voice_language;   //!< 当前选择语音语言文件不存在时备选语言

    u8_t   is_auto_timer_powoff;
    u8_t   chg_box_ctrl_powoff;
    
    u32_t  boot_hold_key_begin_time;
    u8_t  boot_hold_key_checking;

	os_timer adfu_det_timer;
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    u8_t bqb_enter;
	const struct device	*gpio_dev;
#endif
    
} system_app_context_t;

extern system_app_context_t system_app_context;

static inline system_app_context_t* system_app_get_context(void)
{
    return &system_app_context;
}



/** system app init*/
void system_app_init(void);

/** system app input handle*/
void system_key_event_handle(struct app_msg *msg);
void system_input_handle_init(void);
void system_input_event_handle(uint32_t key_event);
int sys_launch_app_init(uint8_t mode);
int system_app_launch(u8_t mode);
int system_app_launch_init(void);

extern void system_app_view_init(void);
extern void system_app_view_exit(void);
extern int system_app_ui_event(int event);
void system_hotplug_event_handle(struct app_msg *msg);
void system_audio_policy_init(void);
void system_audiolcy_config_init(void);
int system_event_map_init(void);
void system_led_policy_init(void);
void system_app_volume_show(struct app_msg *msg);

int system_app_config_init(void);
system_configs_t* system_get_configs(void);

int system_tws_event_handle(struct app_msg *msg);

void system_boot_hold_key_func(unsigned int key_func);
int system_check_boot_hold_key(void);

void system_switch_voice_language(void);
void system_tws_set_voice_language(u8_t voice_lang);

void system_key_tone_on_event_down(uint32_t vkey);
void system_key_tone_on_event_long(uint32_t event);
void system_key_tone_play(u8_t key_tone);
bool system_key_tone_is_playing(void);


void system_app_sys_event_deal(struct app_msg *msg);
void system_app_ui_event_deal(struct app_msg *msg);

int system_bat_charge_event_proc(int event);
void system_do_event_notify(u32_t ui_event, bool led_disp_only, event_call_incoming_t *incoming);
void system_powon_event_notify(uint32_t reboot_mode, bool front_charge);
bool system_powon_check_front_charge(void);
int system_check_power_on(void);
void system_app_enter_poweroff(int tws_poweroff);
void system_app_do_poweroff(int result);
int system_app_check_auto_standby(void);
int system_app_check_front_charge(void);
void system_app_start_front_charge(void);
void system_app_start_poweroff_charge(void);
void system_enter_bqb_test_mode(uint8_t mode);
void system_exit_bqb_test_mode(void);
bool system_check_dc5v_cmd_pending(void);
bool system_dc5v_uart_init(void);
void pt_cfo_adjust(void);
int system_dc5v_io_ctrl_init(void);
void system_dc5v_io_cmd_proc(int cmd_id);
void system_dc5v_io_ctrl_suspend(void);
void system_dc5v_io_ctrl_resume(void);
void system_check_in_charger_box(void);
int system_app_check_onnff_key_state(void);
bool system_tws_sync_chg_box_status(void* pdata);


void system_switch_low_latency_mode(void);

char* audio_voice_get_name_ex(u32_t voice_id, char* name_buf);
char* audio_tone_get_name(u32_t tone_id, char* name_buf);
CFG_Type_Event_Notify* system_notify_get_config(uint32_t ui_event);

u8_t system_event_sys_2_ui(u8_t event);
void system_tws_key_tone_play(void* tws_tone);

void ble_adv_limit_set(u8_t value);
void ble_adv_limit_clear(u8_t value);
int sys_ble_advertise_init(void);
void sys_ble_advertise_deinit(void);

void system_slave_notify_proc(uint8_t *slave_notify);

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
bool system_check_aux_plug_in(void);

/* Lite the process of event notify */
void system_app_enter_poweroff_lite(int tws_poweroff);
#endif

#endif
