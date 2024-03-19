/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app main
 */
#include <soc.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <sys_event.h>
#include "app_switch.h"
#include "system_app.h"
#include "app_ui.h"
#include <stream.h>
#include <property_manager.h>

#include "audio_system.h"
#include "tts_manager.h"
#include "bt_manager.h"
#include "app_config.h"
#include "app_common.h"
#include "led_display.h"
#include <ota_manifest.h>
#include <ota_breakpoint.h>
#include "user_comm/ap_role_switch.h"
#include <dvfs.h>
#include "system_bt.h"
#include "xear_led_ctrl.h"

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

#ifdef CONFIG_TOOL
#include "tool_app.h"
#endif

#ifdef CONFIG_CONSUMER_EQ
#include "consumer_eq.h"
#endif

#ifdef CONFIG_MEDIA_PLAYER
#include <media_player.h>
#endif

#ifdef CONFIG_PA
#include <drivers/pa.h>
#endif

#define ADFU_CONNECTION_INQUIRY_TIMER_MS    (1000)


system_app_context_t system_app_context;

/*share stack for app thread */
char __aligned(ARCH_STACK_PTR_ALIGN) share_stack_area[CONFIG_APP_STACKSIZE];

extern void system_library_version_dump(void);

#ifdef CONFIG_BT_TRANSCEIVER
#if defined(CONFIG_SPPBLE_DATA_CHAN) || defined(CONFIG_OTA_FOREGROUND)
/* BLE */
/*	"e49a25f8-f69a-11e8-8eb2-f2801f1b9fd1" reverse order  */
#define OTA_SERVICE_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xf8, 0x25, 0x9a, 0xe4)

/* "e49a25e0-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define OTA_CHA_RX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe0, 0x25, 0x9a, 0xe4)

/* "e49a28e1-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define OTA_CHA_TX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe1, 0x28, 0x9a, 0xe4)

// static struct bt_gatt_ccc_cfg g_ota_ccc_cfg[1];

struct bt_gatt_attr ota_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(OTA_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(OTA_CHA_RX_UUID, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
						BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CHARACTERISTIC(OTA_CHA_TX_UUID, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
};

int get_ota_gatt_attr_size(void)
{
    return ARRAY_SIZE(ota_gatt_attr);
}
#endif
#endif

#ifdef CONFIG_OTA_APP
extern int ota_app_init(void);
extern int ota_app_init_bluetooth(void);
u8_t  tws_ota_access_receive(u8_t* buf, u16_t len);
u8_t  ap_ota_tws_access_receive(u8_t* buf, u16_t len);
#endif

#ifdef CONFIG_GMA_APP
extern void ais_gma_sppble_stream_init(void);
#endif

#ifdef CONFIG_NMA_APP
extern void nma_app_init(void);
#endif

#ifdef CONFIG_TME_APP
extern void tme_sppble_stream_init(void);
#endif

#ifdef CONFIG_NV_APP
extern void nv_ble_stream_init(void);
#endif

#ifdef CONFIG_TUYA_APP
extern void tuya_ble_stream_init(void);
#endif

#ifdef CONFIG_SPP_TEST_SUPPORT
extern int spp_test_app_init(void);
extern int spp_peer_long_data_cb(char* param, uint32_t param_len);
#endif

#ifdef CONFIG_TOOL_ASET
extern int32_t tws_aset_cmd_deal(uint8_t *data_buf, int32_t len);
#endif

#ifdef CONFIG_TOOL
extern void tool_sysrq_init();
extern void tool_sysrq_init_usb(void);
extern void tool_sysrq_exit_usb(void);
#endif

#ifdef CONFIG_TOOL_ASET_USB_SUPPORT
extern void start_tool_stub_timer(void);
#endif

static void system_app_bt_ctrl_test_routine(void)
{
    system_configs_t*  sys_configs = system_get_configs();
    CFG_Type_Auto_Quit_BT_Ctrl_Test*  cfg = &sys_configs->cfg_auto_quit_bt_ctrl_test;
    btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();

    if (cfg_feature->con_real_test_mode == DISABLE_TEST)
    {    
        return;
    }

    /* 定时退出控制器测试模式?
     */
    if (cfg->Quit_Timer_Sec != 0 &&
        os_uptime_get_32() > cfg->Quit_Timer_Sec * 1000)
    {
        SYS_LOG_INF("");
        /* 退出后关机?
         */
        if (cfg->Power_Off_After_Quit)
        {        
            system_power_off();
        }
        else
        {
            system_power_reboot(REBOOT_TYPE_GOTO_SYSTEM|REBOOT_REASON_NORMAL);
        }
    }
}

void system_app_powon_wait_connect(uint32_t reboot_mode)
{	
    if (bt_manager_is_pair_mode() || bt_manager_is_tws_pair_search()){
        return;
    }

    if (reboot_mode & BT_MANAGER_REBOOT_ENTER_PAIR_MODE)
    {
        bt_manager_start_pair_mode();
    }

    if (reboot_mode & BT_MANAGER_REBOOT_CLEAR_TWS_INFO)
    {
        return;
    }

    sys_manager_event_notify(SYS_EVENT_BT_WAIT_PAIR, false);
}


static int system_app_monitor_work(void)
{
	int allow_property_flush_req = 1;

    #ifdef CONFIG_POWER_MANAGER
	system_app_check_front_charge();
    #endif
	system_app_check_auto_standby();
	system_app_bt_ctrl_test_routine();

#ifdef CONFIG_MEDIA_PLAYER
	if (media_player_get_current_main_player()) {
		allow_property_flush_req = 0;
	}
#endif

	if (!bt_manager_get_link_idle()){
		allow_property_flush_req = 0;
	}

	if (allow_property_flush_req) {
		property_flush_req(NULL);
		property_flush_req_deal();
	}

	return 0;
}

static void system_app_standby_proc(int msg_type)
{
	if (msg_type == MSG_SUSPEND_APP)
	{
        #ifdef CONFIG_POWER_MANAGER
		system_dc5v_io_ctrl_suspend();
        #endif

		led_display_sleep();
		#ifdef CONFIG_XEAR_LED_CTRL
		xear_led_ctrl_sleep();
		#endif
		ui_event_led_display(UI_EVENT_STANDBY);
	}
	else if (msg_type == MSG_RESUME_APP)
	{
	    #ifdef CONFIG_POWER_MANAGER
		system_dc5v_io_ctrl_resume();
        #endif

		ui_event_led_display(UI_EVENT_WAKE_UP);
		#ifdef CONFIG_XEAR_LED_CTRL
		xear_led_ctrl_wake_up();
		#endif
		led_display_wakeup();
	}
}

static int tws_long_data_cb(uint8_t app_event, char* param, uint32_t param_len)
{
	SYS_LOG_INF("app_event 0x%x.", app_event);
    switch(app_event) {
    case TWS_EVENT_TWS_OTA:
        tws_ota_access_receive((u8_t *)param, (u16_t)param_len);
        break;
    case TWS_EVENT_APOTA_SYNC:
        ap_ota_tws_access_receive((u8_t *)param, (u16_t)param_len);
        break;
#ifdef CONFIG_SPP_TEST_SUPPORT	
    case TWS_EVENT_SPP_TEST:
        spp_peer_long_data_cb(param, param_len);
        break;
#endif
    case TWS_EVENT_APP_SETTING_KEY_MAP:
        ui_app_setting_key_map_tws_sync(param);
        break;
    case TWS_EVENT_APP_ROLE_SWITCH:
        ap_role_switch_access_receive(param, param_len);
        break;
#ifdef CONFIG_TOOL_ASET
    case TWS_EVENT_ASET:
        tws_aset_cmd_deal(param, param_len);
        break;
#endif
#ifdef CONFIG_CONSUMER_EQ
    case TWS_EVENT_CONSUMER_EQ:
        consumer_eq_tws_set_param(param, param_len);
        break;
#endif
    default:
        break;
    }
    
	return 0;
}

static void btmgr_event_proc(struct app_msg* msg)
{
	system_app_context_t*  manager = system_app_get_context();

	switch (msg->cmd)
	{
	case BT_MGR_EVENT_READY:
	{
		if (manager->sys_status.in_power_off_stage ||
			manager->sys_status.in_front_charge)
		{
			SYS_LOG_INF("inPoweroff.");		
			return;
		}
	
		system_app_launch(SYS_INIT_NORMAL_MODE);

        sys_bt_ready();

        /* start waiting connect after boot hold key */
		bt_manager_start_wait_connect();
		
		if (manager->boot_hold_key_func != BOOT_HOLD_KEY_FUNC_NONE)
		{
			system_boot_hold_key_func(manager->boot_hold_key_func);
		}
		else
		{
			/* 开机自动回连
			 */
			int reason = 0;
			if (manager->sys_status.reboot_reason == REBOOT_REASON_CLEAR_TWS_INFO){ 
				reason = BT_MANAGER_REBOOT_CLEAR_TWS_INFO;
			}else if (manager->sys_status.reboot_reason == REBOOT_REASON_ENTER_PAIR_MODE){
				reason = BT_MANAGER_REBOOT_ENTER_PAIR_MODE;
			}
			bt_manager_powon_auto_reconnect(reason);
			/* 等待连接
			 */
			system_app_powon_wait_connect(reason);				
		}
	#ifdef CONFIG_OTA_BACKEND_BLUETOOTH
		ota_app_init_bluetooth();
	#endif
	#ifdef CONFIG_GMA_APP
		ais_gma_sppble_stream_init();
	#endif
				
	#ifdef CONFIG_NMA_APP
		nma_app_init();
	#endif
				
	#ifdef CONFIG_TME_APP
		tme_sppble_stream_init();
	#endif
	
 	#ifdef CONFIG_NV_APP
		nv_ble_stream_init();
	#endif
	
	#ifdef CONFIG_TUYA_APP
		tuya_ble_stream_init();
	#endif

#ifdef CONFIG_SPP_TEST_SUPPORT
		spp_test_app_init();
#endif
		bt_manager_tws_register_long_message_cb(tws_long_data_cb);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "manager");
#endif

	}
		break;
	case BT_MGR_EVENT_POWEROFF_RESULT:
	{
		if (manager->sys_status.in_power_off_stage)
			system_app_do_poweroff(msg->reserve);
        
        #ifdef CONFIG_POWER_MANAGER
		else if (manager->sys_status.in_front_charge)
		{
			system_app_start_front_charge();
		}
        #endif
	}
		break;	

	case BT_MGR_EVENT_LEA_RESTART:
	{
		sys_bt_restart_leaudio();
	}
		break;	
	default:
		break;
	}

}

static const msg2str_map_t _app_msg_str_maps[] =
{
    { MSG_APP_KEY_TONE_PLAY,    "APP_KEY_TONE_PLAY",    },
    { MSG_APP_UI_EVENT,         "APP_UI_EVENT",         },
    { MSG_APP_BTCALL_EVENT,     "APP_BTCALL_EVENT",     },
    { MSG_APP_DC5V_IO_CMD,      "APP_DC5V_IO_CMD",      },
    { MSG_APP_SYNC_KEY_MAP,     "APP_SYNC_KEY_MAP",     },
    { MSG_APP_SYNC_MUSIC_DAE,   "APP_SYNC_MUSIC_DAE",   },
    { MSG_APP_UPDATE_MUSIC_DAE, "APP_UPDATE_MUSIC_DAE", },
};

static const msg2str_map_t _bt_event_str_maps[] =
{
    { BT_CONNECTED,                           "CONNECTED",                           },
    { BT_DISCONNECTED,                        "DISCONNECTED",                        },
    { BT_AUDIO_CONNECTED,                     "AUDIO_CONNECTED",                     },
    { BT_AUDIO_DISCONNECTED,                  "AUDIO_DISCONNECTED",                  },
//  { BT_A2DP_CONNECTION_EVENT,               "A2DP_CONNECTION_EVENT",               },
//  { BT_A2DP_DISCONNECTION_EVENT,            "A2DP_DISCONNECTION_EVENT",            },
//  { BT_HFP_CONNECTION_EVENT,                "HFP_CONNECTION_EVENT",               },
    { BT_HFP_DISCONNECTION_EVENT,             "HFP_DISCONNECTION_EVENT",             },
    { BT_AUDIO_STREAM_CONFIG_CODEC,           "AUDIO_STREAM_CONFIG_CODEC",           },
    { BT_AUDIO_STREAM_PREFER_QOS,             "AUDIO_STREAM_PREFER_QOS",             },
    { BT_AUDIO_STREAM_CONFIG_QOS,             "AUDIO_STREAM_CONFIG_QOS",             },
    { BT_AUDIO_STREAM_ENABLE,                 "AUDIO_STREAM_ENABLE",                 },
    { BT_AUDIO_STREAM_UPDATE,                 "AUDIO_STREAM_UPDATE",                 },
    { BT_AUDIO_STREAM_DISABLE,                "AUDIO_STREAM_DISABLE",                },
    { BT_AUDIO_STREAM_START,                  "AUDIO_STREAM_START",                  },
    { BT_AUDIO_STREAM_STOP,                   "AUDIO_STREAM_STOP",                   },
    { BT_AUDIO_STREAM_RELEASE,                "AUDIO_STREAM_RELEASE",                },
    { BT_AUDIO_DISCOVER_CAPABILITY,           "AUDIO_DISCOVER_CAPABILITY",           },
    { BT_AUDIO_DISCOVER_ENDPOINTS,            "AUDIO_DISCOVER_ENDPOINTS",            },
    { BT_AUDIO_PARAMS,                        "AUDIO_PARAMS",                        },
    { BT_AUDIO_STREAM_RESTORE,                "AUDIO_STREAM_RESTORE",                },
    { BT_AUDIO_STREAM_FAKE,                   "AUDIO_STREAM_FAKE",                   },
    { BT_MEDIA_CONNECTED,                     "MEDIA_CONNECTED",                     },
    { BT_MEDIA_DISCONNECTED,                  "MEDIA_DISCONNECTED",                  },
    { BT_MEDIA_PLAYBACK_STATUS_CHANGED_EVENT, "MEDIA_PLAYBACK_STATUS_CHANGED_EVENT", },
    { BT_MEDIA_TRACK_CHANGED_EVENT,           "MEDIA_TRACK_CHANGED_EVENT",           },
    { BT_MEDIA_UPDATE_ID3_INFO_EVENT,         "MEDIA_UPDATE_ID3_INFO_EVENT",         },
    { BT_MEDIA_SERVER_PLAY,                   "MEDIA_SERVER_PLAY",                   },
    { BT_MEDIA_SERVER_PAUSE,                  "MEDIA_SERVER_PAUSE",                  },
    { BT_MEDIA_SERVER_STOP,                   "MEDIA_SERVER_STOP",                   },
    { BT_MEDIA_SERVER_FAST_REWIND,            "MEDIA_SERVER_FAST_REWIND",            },
    { BT_MEDIA_SERVER_FAST_FORWARD,           "MEDIA_SERVER_FAST_FORWARD",           },
    { BT_MEDIA_SERVER_NEXT_TRACK,             "MEDIA_SERVER_NEXT_TRACK",             },
    { BT_MEDIA_SERVER_PREV_TRACK,             "MEDIA_SERVER_PREV_TRACK",             },
    { BT_MEDIA_PLAY,                          "MEDIA_PLAY",                          },
    { BT_MEDIA_PAUSE,                         "MEDIA_PAUSE",                         },
    { BT_MEDIA_STOP,                          "MEDIA_STOP",                          },
    { BT_CALL_CONNECTED,                      "CALL_CONNECTED",                      },
    { BT_CALL_DISCONNECTED,                   "CALL_DISCONNECTED",                   },
    { BT_CALL_INCOMING,                       "CALL_INCOMING",                       },
    { BT_CALL_DIALING,                        "CALL_DIALING",                        },
    { BT_CALL_ALERTING,                       "CALL_ALERTING",                       },
    { BT_CALL_ACTIVE,                         "CALL_ACTIVE",                         },
    { BT_CALL_LOCALLY_HELD,                   "CALL_LOCALLY_HELD",                   },
    { BT_CALL_REMOTELY_HELD,                  "CALL_REMOTELY_HELD",                  },
    { BT_CALL_HELD,                           "CALL_HELD",                           },
    { BT_CALL_ENDED,                          "CALL_ENDED",                          },
    { BT_CALL_SIRI_MODE,                      "CALL_SIRI_MODE",                      },
    { BT_CALL_3WAYIN,                         "CALL_3WAYIN",                         },
    { BT_CALL_3WAYOUT,                        "CALL_3WAYOUT",                        },
    { BT_CALL_MULTIPARTY,                     "CALL_MULTIPARTY",                     },
    { BT_CALL_RING_STATR_EVENT,               "CALL_RING_STATR_EVENT",               },
    { BT_CALL_RING_STOP_EVENT,                "CALL_RING_STOP_EVENT",                },
    { BT_VOLUME_UP,                           "VOLUME_UP",                           },
    { BT_VOLUME_DOWN,                         "VOLUME_DOWN",                         },
    { BT_VOLUME_VALUE,                        "VOLUME_VALUE",                        },
    { BT_VOLUME_MUTE,                         "VOLUME_MUTE",                         },
    { BT_VOLUME_UNMUTE,                       "VOLUME_UNMUTE",                       },
    { BT_VOLUME_UNMUTE_UP,                    "VOLUME_UNMUTE_UP",                    },
    { BT_VOLUME_UNMUTE_DOWN,                  "VOLUME_UNMUTE_DOWN",                  },
    { BT_VOLUME_CLIENT_CONNECTED,             "VOLUME_CLIENT_CONNECTED",             },
    { BT_VOLUME_CLIENT_UP,                    "VOLUME_CLIENT_UP",                    },
    { BT_VOLUME_CLIENT_DOWN,                  "VOLUME_CLIENT_DOWN",                  },
    { BT_VOLUME_CLIENT_VALUE,                 "VOLUME_CLIENT_VALUE",                 },
    { BT_VOLUME_CLIENT_MUTE,                  "VOLUME_CLIENT_MUTE",                  },
    { BT_VOLUME_CLIENT_UNMUTE,                "VOLUME_CLIENT_UNMUTE",                },
    { BT_VOLUME_CLIENT_UNMUTE_UP,             "VOLUME_CLIENT_UNMUTE_UP",             },
    { BT_VOLUME_CLIENT_UNMUTE_DOWN,           "VOLUME_CLIENT_UNMUTE_DOWN",           },
    { BT_MIC_MUTE,                            "MIC_MUTE",                            },
    { BT_MIC_UNMUTE,                          "MIC_UNMUTE",                          },
    { BT_MIC_GAIN_VALUE,                      "MIC_GAIN_VALUE",                      },
    { BT_MIC_CLIENT_CONNECTED,                "MIC_CLIENT_CONNECTED",                },
    { BT_MIC_CLIENT_MUTE,                     "MIC_CLIENT_MUTE",                     },
    { BT_MIC_CLIENT_UNMUTE,                   "MIC_CLIENT_UNMUTE",                   },
    { BT_MIC_CLIENT_GAIN_VALUE,               "MIC_CLIENT_GAIN_VALUE",               },
    { BT_TWS_CONNECTION_EVENT,                "TWS_CONNECTION_EVENT",                },
    { BT_TWS_DISCONNECTION_EVENT,             "TWS_DISCONNECTION_EVENT",             },
//  { BT_TWS_CHANNEL_MODE_SWITCH,             "TWS_CHANNEL_MODE_SWITCH",             },
    { BT_TWS_ROLE_CHANGE,                     "TWS_ROLE_CHANGE",                     },
    { BT_REQ_RESTART_PLAY,                    "REQ_RESTART_PLAY",                    },
//  { BT_TWS_START_PLAY,                      "TWS_START_PLAY",                      },
//  { BT_TWS_STOP_PLAY,                       "TWS_STOP_PLAY",                       },
//  { BT_HFP_SIRI_START,                      "HFP_SIRI_START",                      },
//  { BT_HFP_SIRI_STOP,                       "HFP_SIRI_STOP",                       },
//  { BT_HID_CONNECTION_EVENT,                "HID_CONNECTION_EVENT",                },
//  { BT_HID_DISCONNECTION_EVENT,             "HID_DISCONNECTION_EVENT",             },
//  { BT_HID_ACTIVEDEV_CHANGE_EVENT,          "HID_ACTIVEDEV_CHANGE_EVENT",          },
    { BT_AUDIO_SWITCH,                        "AUDIO_SWITCH",                        },
};

void app_msg_print(const struct app_msg* msg, const char* func_name)
{
    if (msg->type == MSG_BT_EVENT)
    {
        const char* str = msg2str(msg->cmd, _bt_event_str_maps, 
            ARRAY_SIZE(_bt_event_str_maps), "UNKNOWN");
        
        printk("<I> [%s] %d (MSG_BT_EVENT), %d (BT_%s), 0x%x\n", 
            func_name, msg->type, msg->cmd, str, msg->value);
    }
    else
    {
        const char* str = sys_msg2str(msg->type, NULL);

        if (str == NULL)
        {
            str = msg2str(msg->type, _app_msg_str_maps, 
                ARRAY_SIZE(_app_msg_str_maps), "UNKNOWN");
        }
        
        printk("<I> [%s] %d (MSG_%s), %d, 0x%x\n", 
            func_name, msg->type, str, msg->cmd, msg->value);
    }
}

void main_msg_proc(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;
    int ingore_callback = 0;
    system_app_context_t*  manager = system_app_get_context();
	uint32_t start_time, cost_time;
	
	if (receive_msg(&msg, thread_timer_next_timeout())) {
        app_msg_print(&msg, __FUNCTION__);
        
		start_time = k_cycle_get_32();
		switch (msg.type) {
		case MSG_SYS_EVENT:
		{
			system_app_sys_event_deal(&msg);
		}
			break;
		case MSG_UI_EVENT:
			ui_message_dispatch(msg.sender, msg.cmd, msg.value);
			break;
		case MSG_APP_UI_EVENT:
			system_app_ui_event_deal(&msg);
			break;
		case MSG_TTS_EVENT:
			if (msg.cmd == TTS_EVENT_START_PLAY) {
				tts_manager_play_process();
			}
			break;
		case MSG_KEY_INPUT:
			/**input event means esd proecess finished*/
			system_input_event_handle(msg.value);
			break;
		case MSG_INPUT_EVENT:
			system_key_event_handle(&msg);
			break;

		case MSG_VOLUME_CHANGED_EVENT:
			system_app_volume_show(&msg);
			break;

		case MSG_POWER_OFF:
			manager->is_auto_timer_powoff = msg.value;
			system_app_enter_poweroff(0);
			break;
		case MSG_REBOOT:
			system_power_reboot(msg.cmd);
			break;
		case MSG_NO_POWER:
			system_app_enter_poweroff(0);
			break;
		case MSG_BT_MGR_EVENT:
			btmgr_event_proc(&msg);
			break;	
		case MSG_START_APP:
			SYS_LOG_INF("start %s\n", (char *)msg.ptr);
			app_switch((char *)msg.ptr, msg.reserve, true);
			break;
		case MSG_EXIT_APP:
			SYS_LOG_DBG("exit %s\n", (char *)msg.ptr);
			app_manager_exit_app((char *)msg.ptr, true);
			break;

		case MSG_TWS_EVENT:
		{
		    if (system_tws_event_handle(&msg) != 0)
				ingore_callback =  1;
		}
			break;	
		case MSG_APP_KEY_TONE_PLAY:
		{
			system_key_tone_play(msg.value);
		}
			break;	
        #ifdef CONFIG_POWER_MANAGER
		case MSG_BAT_CHARGE_EVENT:
			system_bat_charge_event_proc(msg.cmd);
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
            if(msg.cmd == POWER_EVENT_CHARGE_START)
            {
                manager->gpio_dev = device_get_binding("GPIOA");
                gpio_pin_configure(manager->gpio_dev, 7, GPIO_OUTPUT_ACTIVE);
                gpio_pin_set(manager->gpio_dev, 7, 0);
            }
#endif
			break;
		case MSG_APP_DC5V_IO_CMD:
			system_dc5v_io_cmd_proc(msg.cmd);
			break;
        #endif
		case MSG_APP_SYNC_KEY_MAP:
		{
			system_app_view_init();
			char *fg_app = app_manager_get_current_app();
			if (!fg_app)
				break;

			if (memcmp(fg_app, "main", strlen("main")) == 0)
				break;
       
			send_async_msg(fg_app, &msg);			
		}	
			break;
#ifdef CONFIG_TOOL
		case MSG_APP_TOOL_INIT:
			if (msg.cmd == 0) {
				tool_sysrq_init();  //uart
			} else if (msg.cmd == 1) {
#ifdef CONFIG_TOOL_ASET_USB_SUPPORT			
				tool_sysrq_init_usb();  //usb
#endif				
			} else {
				;
			}
			  
			break;

		case MSG_APP_TOOL_EXIT:
			if (msg.cmd == 1) {
#ifdef CONFIG_TOOL_ASET_USB_SUPPORT				
				tool_sysrq_exit_usb();  //usb
#endif				
			}

			break;
#endif

        case MSG_HOTPLUG_EVENT:
            system_hotplug_event_handle(&msg);
            break;

		default:
			SYS_LOG_ERR(" error: %d\n", msg.type);
			break;
		}

		if (msg.callback && !ingore_callback)
			msg.callback(&msg, result, NULL);

		cost_time = k_cyc_to_us_floor32(k_cycle_get_32() - start_time);
		if (cost_time > APP_PORC_MONITOR_TIME) {
			printk("xxxx:(%s) msg.type:%d run %d us\n",__func__, msg.type, cost_time);
		}
	}

	start_time = k_cycle_get_32();
	thread_timer_handle_expired();
	cost_time = k_cyc_to_us_floor32(k_cycle_get_32() - start_time) ;		

	if ((cost_time) > APP_PORC_MONITOR_TIME) {
		printk("xxxx:(%s) timer run %d us\n",__func__, cost_time);
	}
}


void adfu_det_ttimer_handler(os_timer *ttimer)
{
	bool new_usb_status;
#ifdef CONFIG_POWER_MANAGER
	new_usb_status = power_manager_get_dc5v_status();
#else
    new_usb_status = true;
#endif
	if(new_usb_status) {
		SYS_LOG_INF("usb in, need reboot to adfu!");
		k_busy_wait(200 * 1000);
		sys_pm_reboot(REBOOT_TYPE_GOTO_ADFU);
	}

	os_timer_start(ttimer, K_MSEC(ADFU_CONNECTION_INQUIRY_TIMER_MS), K_MSEC(0));
}


static void start_adfu_det_timer(void)
{
	system_app_context_t*  manager = system_app_get_context();

	SYS_LOG_INF("start adfu timer!");
    os_timer_init(&manager->adfu_det_timer, adfu_det_ttimer_handler, NULL);   
    os_timer_start(&manager->adfu_det_timer, K_MSEC(ADFU_CONNECTION_INQUIRY_TIMER_MS), K_MSEC(0)); 
}

int main(void)
{
	bool play_welcome = true;
	u16_t reboot_type = 0;
	u8_t reason = 0;
    bool  front_charge = false;	
    system_app_context_t*  manager = system_app_get_context();
	system_configs_t*  sys_configs = system_get_configs();

	LOG_INF("Fireware Build time: %s %s", __DATE__, __TIME__);
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "manager");
#endif

	system_power_get_reboot_reason(&reboot_type, &reason);
	manager->sys_status.reboot_type = reboot_type;
	manager->sys_status.reboot_reason = reason;

    mem_manager_init();
	system_app_config_init();
    led_display_init();

	system_library_version_dump();
	system_init();
	system_audio_policy_init();
	system_event_map_init();
	system_input_handle_init();
	system_app_launch_init();

	system_check_power_on();

	if ((reboot_type == REBOOT_TYPE_SF_RESET) && (reason == REBOOT_REASON_GOTO_BQB))
    {
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
        manager->bqb_enter = 1;
#endif
		SYS_LOG_INF("Goto BQB TEST");
    }
	else if ((reboot_type == REBOOT_TYPE_SF_RESET) && (reason == REBOOT_REASON_GOTO_BQB_ATT)) {
		input_manager_lock();		
		manager->att_enter_bqb_flag = true;
		SYS_LOG_INF("Goto BQB ATT TEST");		
	}
    #ifdef CONFIG_POWER_MANAGER
	else if ((reboot_type == REBOOT_TYPE_SF_RESET) && (reason == REBOOT_REASON_CHG_BOX_OPENED)) {
		struct app_msg msg = {0};
		msg.type = MSG_APP_DC5V_IO_CMD;
		send_async_msg("main", &msg);
	} else if (reason == REBOOT_REASON_PRODUCTION_TEST_CFO_ADJUST){
		pt_cfo_adjust();
	}
    #endif
	if (!play_welcome) {
		tts_manager_lock();
	} else if(reason != REBOOT_REASON_OTA_FINISHED){
		//Todo:       
	}

	system_app_view_init();
	system_ready();

#ifdef CONFIG_OTA_APP
	struct ota_breakpoint bp;
	ota_breakpoint_load(&bp);
	if (bp.state == OTA_BP_STATE_UPGRADE_WRITING) {
		ota_breakpoint_update_state(&bp, OTA_BP_STATE_WRITING_IMG_FAIL);
	}
	ota_app_init();
#endif

	if(manager->bt_manager_config_inited){
        sys_bt_init();
        audio_tws_pre_init();
	}
#ifdef CONFIG_POWER_MANAGER
    if (reason == 0)
    {
        /* 是否前台充电
         */
        front_charge = system_powon_check_front_charge();
    }
	system_dc5v_io_ctrl_init();

	system_dc5v_uart_init();
#endif

#ifdef CONFIG_PA
    const struct device * pa_device = device_get_binding(CONFIG_PA_DEV_NAME);
    struct pa_i2s_format pa_i2s_fmt;
    if (pa_device) {
        pa_audio_init(pa_device);
        
        pa_i2s_fmt.type  = I2S_TYPE_PHILIPS;
        pa_i2s_fmt.rate  = I2S_SAMPLE_RATE_48K;
        pa_i2s_fmt.width = I2S_DATA_WIDTH_24;
        pa_audio_format_config(pa_device, &pa_i2s_fmt);
    }
#endif

    /* 开机提示 */ 
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    if (sys_get_work_mode() == RECEIVER_MODE) {
        if (system_check_aux_plug_in())
        {
            /* Aux plug in, shutdown !! */
            system_app_enter_poweroff_lite(0);	    
        }
        else
        {
            /* Aux plug out, notify power on event */
            system_powon_event_notify(reason, front_charge);
        }
    }
#else
    system_powon_event_notify(reason, front_charge);
#endif

	if (!front_charge) {
	    system_check_boot_hold_key();
	}
	sys_monitor_add_work(system_app_monitor_work);

	system_register_standby_notifier(system_app_standby_proc);

#ifdef CONFIG_TOOL_ASET_USB_SUPPORT
    if(sys_configs->cfg_sys_more_settings.Default_USB_Device_Mode == USB_DEVICE_MODE_CHARGE_STUB) {
    	start_tool_stub_timer();
    }
#endif

    if(sys_configs->cfg_sys_more_settings.Default_USB_Device_Mode == USB_DEVICE_MODE_ADFU_ONLY) {
    	start_adfu_det_timer();
    }

	while (1) {
		main_msg_proc(NULL, NULL, NULL);
	}
	return 0;
}

extern char z_main_stack[CONFIG_MAIN_STACK_SIZE];

APP_DEFINE(main, z_main_stack, CONFIG_MAIN_STACK_SIZE,\
			APP_PRIORITY, RESIDENT_APP,\
			NULL, NULL, NULL,\
			NULL, NULL);

