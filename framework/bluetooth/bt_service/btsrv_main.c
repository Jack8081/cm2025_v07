/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt service interface
 */

#define SYS_LOG_DOMAIN "btsrv_main"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

#define MAX_BTSRV_PROCESSER		(MSG_BTSRV_MAX - MSG_BTSRV_BASE)
#define BTSRV_PORC_MONITOR_TIME	(5000)		/* unit:us */

static btsrv_msg_process msg_processer[MAX_BTSRV_PROCESSER];

const char *bt_service_evt2str(int num, int max_num, const btsrv_event_strmap_t *strmap)
{
	int low = 0;
	int hi = max_num - 1;
	int mid;

	do {
		mid = (low + hi) >> 1;
		if (num > strmap[mid].event) {
			low = mid + 1;
		} else if (num < strmap[mid].event) {
			hi = mid - 1;
		} else {
			return strmap[mid].str;
		}
	} while (low <= hi);

	printk("evt num %d\n", num);

	return "Unknown!";
}


static int _bt_service_init(struct app_msg *msg)
{
	btsrv_callback cb = _btsrv_get_msg_param_callback(msg);

	if (btsrv_info && btsrv_info->init_state != BT_INIT_NONE) {
		SYS_LOG_INF("Already running");
		return 0;
	}

	return btsrv_adapter_init(cb);
}

static int _bt_service_exit(void)
{
	btsrv_adapter_stop();
	srv_manager_thread_exit(BLUETOOTH_SERVICE_NAME);

	return 0;
}

int btsrv_register_msg_processer(uint8_t msg_type, btsrv_msg_process processer)
{
	if ((msg_type < MSG_BTSRV_BASE) || (msg_type >= MSG_BTSRV_MAX) || !processer) {
		SYS_LOG_WRN("Unknow processer %p or msg_type %d\n", processer, msg_type);
		return -EINVAL;
	}

	msg_processer[msg_type - MSG_BTSRV_BASE] = processer;
	SYS_LOG_INF("Register %d processer\n", msg_type);
	return 0;
}


static const msg2str_map_t _btsrv_msg_str_maps[] =
{
    { MSG_BTSRV_BASE,    "BASE",    },
    { MSG_BTSRV_CONNECT, "CONNECT", },
    { MSG_BTSRV_A2DP,    "A2DP",    },
    { MSG_BTSRV_AVRCP,   "AVRCP",   },
    { MSG_BTSRV_HFP,     "HFP",     },
//  { MSG_BTSRV_HFP_AG,  "HFP_AG",  },
    { MSG_BTSRV_SCO,     "SCO",     },
    { MSG_BTSRV_SPP,     "SPP",     },
//  { MSG_BTSRV_PBAP,    "PBAP",    },
//  { MSG_BTSRV_HID,     "HID",     },
    { MSG_BTSRV_TWS,     "TWS",     },
//  { MSG_BTSRV_MAP,     "MAP",     },
    { MSG_BTSRV_AUDIO,   "AUDIO",   },
};

static const msg2str_map_t _bt_common_event_str_maps[] =
{
    { MSG_BTSRV_UPDATE_SCAN_MODE,               "UPDATE_SCAN_MODE",               },
    { MSG_BTSRV_SET_USER_VISUAL,                "SET_USER_VISUAL",                },
    { MSG_BTSRV_AUTO_RECONNECT,                 "AUTO_RECONNECT",                 },
    { MSG_BTSRV_AUTO_RECONNECT_STOP,            "AUTO_RECONNECT_STOP",            },
    { MSG_BTSRV_AUTO_RECONNECT_STOP_DEVICE,     "AUTO_RECONNECT_STOP_DEVICE",     },
    { MSG_BTSRV_AUTO_RECONNECT_COMPLETE,        "AUTO_RECONNECT_COMPLETE",        },
    { MSG_BTSRV_CONNECT_TO,                     "CONNECT_TO",                     },
    { MSG_BTSRV_DISCONNECT,                     "DISCONNECT",                     },
    { MSG_BTSRV_READY,                          "READY",                          },
    { MSG_BTSRV_REQ_FLUSH_NVRAM,                "REQ_FLUSH_NVRAM",                },
    { MSG_BTSRV_CONNECTED,                      "CONNECTED",                      },
    { MSG_BTSRV_CONNECTED_FAILED,               "CONNECTED_FAILED",               },
    { MSG_BTSRV_SECURITY_CHANGED,               "SECURITY_CHANGED",               },
    { MSG_BTSRV_ROLE_CHANGE,                    "ROLE_CHANGE",                    },
    { MSG_BTSRV_MODE_CHANGE,                    "MODE_CHANGE",                    },
    { MSG_BTSRV_DISCONNECTED,                   "DISCONNECTED",                   },
    { MSG_BTSRV_DISCONNECTED_REASON,            "DISCONNECTED_REASON",            },
    { MSG_BTSRV_GET_NAME_FINISH,                "GET_NAME_FINISH",                },
    { MSG_BTSRV_CLEAR_LIST_CMD,                 "CLEAR_LIST_CMD",                 },
    { MGS_BTSRV_CLEAR_AUTO_INFO,                "CLEAR_AUTO_INFO",                },
    { MSG_BTSRV_DISCONNECT_DEVICE,              "DISCONNECT_DEVICE",              },
    { MSG_BTSRV_ALLOW_SCO_CONNECT,              "ALLOW_SCO_CONNECT",              },
    { MSG_BTSRV_CLEAR_SHARE_TWS,                "CLEAR_SHARE_TWS",                },
    { MSG_BTSRV_DISCONNECT_DEV_BY_PRIORITY,     "DISCONNECT_DEV_BY_PRIORITY",     },
    { MSG_BTSRV_CONNREQ_ADD_MONITOR,            "CONNREQ_ADD_MONITOR",            },
    { MSG_BTSRV_SET_LEAUDIO_CONNECTED,          "SET_LEAUDIO_CONNECTED",          },
    { MSG_BTSRV_A2DP_START,                     "A2DP_START",                     },
    { MSG_BTSRV_A2DP_STOP,                      "A2DP_STOP",                      },
    { MSG_BTSRV_A2DP_CHECK_STATE,               "A2DP_CHECK_STATE",               },
    { MSG_BTSRV_A2DP_USER_PAUSE,                "A2DP_USER_PAUSE",                },
    { MSG_BTSRV_A2DP_CONNECT_TO,                "A2DP_CONNECT_TO",                },
    { MSG_BTSRV_A2DP_DISCONNECT,                "A2DP_DISCONNECT",                },
    { MSG_BTSRV_A2DP_CONNECTED,                 "A2DP_CONNECTED",                 },
    { MSG_BTSRV_A2DP_DISCONNECTED,              "A2DP_DISCONNECTED",              },
    { MSG_BTSRV_A2DP_MEDIA_STATE_CB,            "A2DP_MEDIA_STATE_CB",            },
    { MSG_BTSRV_A2DP_SET_CODEC_CB,              "A2DP_SET_CODEC_CB",              },
    { MSG_BTSRV_A2DP_SEND_DELAY_REPORT,         "A2DP_SEND_DELAY_REPORT",         },
    { MSG_BTSRV_A2DP_CHECK_SWITCH_SBC,          "A2DP_CHECK_SWITCH_SBC",          },
    { MSG_BTSRV_A2DP_UPDATE_AVRCP_PLAYSTATE,    "A2DP_UPDATE_AVRCP_PLAYSTATE",    },
    { MSG_BTSRV_AVRCP_START,                    "AVRCP_START",                    },
    { MSG_BTSRV_AVRCP_STOP,                     "AVRCP_STOP",                     },
    { MSG_BTSRV_AVRCP_CONNECT_TO,               "AVRCP_CONNECT_TO",               },
    { MSG_BTSRV_AVRCP_DISCONNECT,               "AVRCP_DISCONNECT",               },
    { MSG_BTSRV_AVRCP_CONNECTED,                "AVRCP_CONNECTED",                },
    { MSG_BTSRV_AVRCP_DISCONNECTED,             "AVRCP_DISCONNECTED",             },
    { MSG_BTSRV_AVRCP_NOTIFY_CB,                "AVRCP_NOTIFY_CB",                },
    { MSG_BTSRV_AVRCP_PASS_CTRL_CB,             "AVRCP_PASS_CTRL_CB",             },
    { MSG_BTSRV_AVRCP_SEND_CMD,                 "AVRCP_SEND_CMD",                 },
    { MSG_BTSRV_AVRCP_SYNC_VOLUME,              "AVRCP_SYNC_VOLUME",              },
    { MSG_BTSRV_AVRCP_GET_ID3_INFO,             "AVRCP_GET_ID3_INFO",             },
    { MSG_BTSRV_AVRCP_SET_ABSOLUTE_VOLUME,      "AVRCP_SET_ABSOLUTE_VOLUME",      },
    { MSG_BTSRV_HFP_START,                      "HFP_START",                      },
    { MSG_BTSRV_HFP_STOP,                       "HFP_STOP",                       },
    { MSG_BTSRV_HFP_CONNECTED,                  "HFP_CONNECTED",                  },
    { MSG_BTSRV_HFP_DISCONNECTED,               "HFP_DISCONNECTED",               },
    { MSG_BTSRV_HFP_STACK_EVENT,                "HFP_STACK_EVENT",                },
    { MSG_BTSRV_HFP_VOLUME_CHANGE,              "HFP_VOLUME_CHANGE",              },
    { MSG_BTSRV_HFP_PHONE_NUM,                  "HFP_PHONE_NUM",                  },
    { MSG_BTSRV_HFP_CODEC_INFO,                 "HFP_CODEC_INFO",                 },
    { MSG_BTSRV_HFP_SIRI_STATE,                 "HFP_SIRI_STATE",                 },
    { MSG_BTSRV_SCO_START,                      "SCO_START",                      },
    { MSG_BTSRV_SCO_STOP,                       "SCO_STOP",                       },
    { MSG_BTSRV_SCO_DISCONNECT,                 "SCO_DISCONNECT",                 },
    { MSG_BTSRV_SCO_CONNECTED,                  "SCO_CONNECTED",                  },
    { MSG_BTSRV_SCO_DISCONNECTED,               "SCO_DISCONNECTED",               },
    { MSG_BTSRV_CREATE_SCO_SNOOP,               "CREATE_SCO_SNOOP",               },
    { MSG_BTSRV_HFP_SWITCH_SOUND_SOURCE,        "HFP_SWITCH_SOUND_SOURCE",        },
    { MSG_BTSRV_HFP_HF_DIAL_NUM,                "HFP_HF_DIAL_NUM",                },
    { MSG_BTSRV_HFP_HF_DIAL_LAST_NUM,           "HFP_HF_DIAL_LAST_NUM",           },
    { MSG_BTSRV_HFP_HF_DIAL_MEMORY,             "HFP_HF_DIAL_MEMORY",             },
    { MSG_BTSRV_HFP_HF_VOLUME_CONTROL,          "HFP_HF_VOLUME_CONTROL",          },
    { MSG_BTSRV_HFP_HF_ACCEPT_CALL,             "HFP_HF_ACCEPT_CALL",             },
    { MSG_BTSRV_HFP_HF_BATTERY_REPORT,          "HFP_HF_BATTERY_REPORT",          },
    { MSG_BTSRV_HFP_HF_REJECT_CALL,             "HFP_HF_REJECT_CALL",             },
    { MSG_BTSRV_HFP_HF_HANGUP_CALL,             "HFP_HF_HANGUP_CALL",             },
    { MSG_BTSRV_HFP_HF_HANGUP_ANOTHER_CALL,     "HFP_HF_HANGUP_ANOTHER_CALL",     },
    { MSG_BTSRV_HFP_HF_HOLDCUR_ANSWER_CALL,     "HFP_HF_HOLDCUR_ANSWER_CALL",     },
    { MSG_BTSRV_HFP_HF_HANGUPCUR_ANSWER_CALL,   "HFP_HF_HANGUPCUR_ANSWER_CALL",   },
    { MSG_BTSRV_HFP_HF_VOICE_RECOGNITION_START, "HFP_HF_VOICE_RECOGNITION_START", },
    { MSG_BTSRV_HFP_HF_VOICE_RECOGNITION_STOP,  "HFP_HF_VOICE_RECOGNITION_STOP",  },
    { MSG_BTSRV_HFP_HF_VOICE_SEND_AT_COMMAND,   "HFP_HF_VOICE_SEND_AT_COMMAND",   },
    { MSG_BTSRV_HFP_ACTIVED_DEV_CHANGED,        "HFP_ACTIVED_DEV_CHANGED",        },
    { MSG_BTSRV_HFP_GET_TIME,                   "HFP_GET_TIME",                   },
    { MSG_BTSRV_HFP_TIME_UPDATE,                "HFP_TIME_UPDATE",                },
    { MSG_BTSRV_HFP_EVENT_FROM_TWS,             "HFP_EVENT_FROM_TWS",             },
//  { MSG_BTSRV_HFP_AG_START,                   "HFP_AG_START",                   },
//  { MSG_BTSRV_HFP_AG_STOP,                    "HFP_AG_STOP",                    },
//  { MSG_BTSRV_HFP_AG_CONNECTED,               "HFP_AG_CONNECTED",               },
//  { MSG_BTSRV_HFP_AG_DISCONNECTED,            "HFP_AG_DISCONNECTED",            },
//  { MSG_BTSRV_HFP_AG_UPDATE_INDICATOR,        "HFP_AG_UPDATE_INDICATOR",        },
//  { MSG_BTSRV_HFP_AG_SEND_EVENT,              "HFP_AG_SEND_EVENT",              },
    { MSG_BTSRV_SPP_START,                      "SPP_START",                      },
    { MSG_BTSRV_SPP_STOP,                       "SPP_STOP",                       },
    { MSG_BTSRV_SPP_REGISTER,                   "SPP_REGISTER",                   },
    { MSG_BTSRV_SPP_CONNECT,                    "SPP_CONNECT",                    },
    { MSG_BTSRV_SPP_DISCONNECT,                 "SPP_DISCONNECT",                 },
    { MSG_BTSRV_SPP_CONNECT_FAILED,             "SPP_CONNECT_FAILED",             },
    { MSG_BTSRV_SPP_CONNECTED,                  "SPP_CONNECTED",                  },
    { MSG_BTSRV_SPP_DISCONNECTED,               "SPP_DISCONNECTED",               },
//  { MSG_BTSRV_PBAP_CONNECT_FAILED,            "PBAP_CONNECT_FAILED",            },
//  { MSG_BTSRV_PBAP_CONNECTED,                 "PBAP_CONNECTED",                 },
//  { MSG_BTSRV_PBAP_DISCONNECTED,              "PBAP_DISCONNECTED",              },
//  { MSG_BTSRV_PBAP_GET_PB,                    "PBAP_GET_PB",                    },
//  { MSG_BTSRV_PBAP_ABORT_GET,                 "PBAP_ABORT_GET",                 },
//  { MSG_BTSRV_HID_START,                      "HID_START",                      },
//  { MSG_BTSRV_HID_STOP,                       "HID_STOP",                       },
//  { MSG_BTSRV_HID_CONNECTED,                  "HID_CONNECTED",                  },
//  { MSG_BTSRV_HID_DISCONNECTED,               "HID_DISCONNECTED",               },
//  { MSG_BTSRV_HID_EVENT_CB,                   "HID_EVENT_CB",                   },
//  { MSG_BTSRV_HID_REGISTER,                   "HID_REGISTER",                   },
//  { MSG_BTSRV_HID_CONNECT,                    "HID_CONNECT",                    },
//  { MSG_BTSRV_HID_DISCONNECT,                 "HID_DISCONNECT",                 },
//  { MSG_BTSRV_HID_SEND_CTRL_DATA,             "HID_SEND_CTRL_DATA",             },
//  { MSG_BTSRV_HID_SEND_INTR_DATA,             "HID_SEND_INTR_DATA",             },
//  { MSG_BTSRV_HID_SEND_RSP,                   "HID_SEND_RSP",                   },
//  { MSG_BTSRV_HID_UNPLUG,                     "HID_UNPLUG",                     },
    { MSG_BTSRV_DID_REGISTER,                   "DID_REGISTER",                   },
    { MSG_BTSRV_TWS_INIT,                       "TWS_INIT",                       },
    { MSG_BTSRV_TWS_DEINIT,                     "TWS_DEINIT",                     },
//  { MSG_BTSRV_TWS_VERSION_NEGOTIATE,          "TWS_VERSION_NEGOTIATE",          },
//  { MSG_BTSRV_TWS_ROLE_NEGOTIATE,             "TWS_ROLE_NEGOTIATE",             },
//  { MSG_BTSRV_TWS_NEGOTIATE_SET_ROLE,         "TWS_NEGOTIATE_SET_ROLE",         },
//  { MSG_BTSRV_TWS_CONNECTED,                  "TWS_CONNECTED",                  },
    { MSG_BTSRV_TWS_DISCONNECTED,               "TWS_DISCONNECTED",               },
    { MSG_BTSRV_TWS_DISCONNECTED_ADDR,          "TWS_DISCONNECTED_ADDR",          },
    { MSG_BTSRV_TWS_START_PAIR,                 "TWS_START_PAIR",                 },
    { MSG_BTSRV_TWS_END_PAIR,                   "TWS_END_PAIR",                   },
    { MSG_BTSRV_TWS_DISCOVERY_RESULT,           "TWS_DISCOVERY_RESULT",           },
    { MSG_BTSRV_TWS_DISCONNECT,                 "TWS_DISCONNECT",                 },
//  { MSG_BTSRV_TWS_RESTART,                    "TWS_RESTART",                    },
    { MSG_BTSRV_TWS_PROTOCOL_DATA,              "TWS_PROTOCOL_DATA",              },
//  { MSG_BTSRV_TWS_EVENT_SYNC,                 "TWS_EVENT_SYNC",                 },
    { MSG_BTSRV_TWS_SCO_DATA,                   "TWS_SCO_DATA",                   },
    { MSG_BTSRV_TWS_DO_SNOOP_CONNECT,           "TWS_DO_SNOOP_CONNECT",           },
    { MSG_BTSRV_TWS_DO_SNOOP_DISCONNECT,        "TWS_DO_SNOOP_DISCONNECT",        },
    { MSG_BTSRV_TWS_SNOOP_CONNECTED,            "TWS_SNOOP_CONNECTED",            },
    { MSG_BTSRV_TWS_SNOOP_DISCONNECTED,         "TWS_SNOOP_DISCONNECTED",         },
    { MSG_BTSRV_TWS_SNOOP_MODE_CHANGE,          "TWS_SNOOP_MODE_CHANGE",          },
    { MSG_BTSRV_TWS_SNOOP_ACTIVE_LINK,          "TWS_SNOOP_ACTIVE_LINK",          },
    { MSG_BTSRV_TWS_SYNC_1ST_PKT_CHG,           "TWS_SYNC_1ST_PKT_CHG",           },
    { MSG_BTSRV_TWS_SWITCH_SNOOP_LINK,          "TWS_SWITCH_SNOOP_LINK",          },
    { MSG_BTSRV_TWS_SNOOP_SWITCH_COMPLETE,      "TWS_SNOOP_SWITCH_COMPLETE",      },
    { MSG_BTSRV_TWS_UPDATE_VISUAL_PAIR,         "TWS_UPDATE_VISUAL_PAIR",         },
    { MSG_BTSRV_TWS_RSSI_RESULT,                "TWS_RSSI_RESULT",                },
    { MSG_BTSRV_TWS_LINK_QUALITY_RESULT,        "TWS_LINK_QUALITY_RESULT",        },
    { MSG_BTSRV_TWS_PHONE_ACTIVE_CHANGE,        "TWS_PHONE_ACTIVE_CHANGE",        },
    { MSG_BTSRV_TWS_SET_STATE,                  "TWS_SET_STATE",                  },
//  { MSG_BTSRV_MAP_CONNECT,                    "MAP_CONNECT",                    },
//  { MSG_BTSRV_MAP_DISCONNECT,                 "MAP_DISCONNECT",                 },
//  { MSG_BTSRV_MAP_SET_FOLDER,                 "MAP_SET_FOLDER",                 },
//  { MSG_BTSRV_MAP_GET_FOLDERLISTING,          "MAP_GET_FOLDERLISTING",          },
//  { MSG_BTSRV_MAP_GET_MESSAGESLISTING,        "MAP_GET_MESSAGESLISTING",        },
//  { MSG_BTSRV_MAP_GET_MESSAGE,                "MAP_GET_MESSAGE",                },
//  { MSG_BTSRV_MAP_ABORT_GET,                  "MAP_ABORT_GET",                  },
//  { MSG_BTSRV_MAP_CONNECT_FAILED,             "MAP_CONNECT_FAILED",             },
//  { MSG_BTSRV_MAP_CONNECTED,                  "MAP_CONNECTED",                  },
//  { MSG_BTSRV_MAP_DISCONNECTED,               "MAP_DISCONNECTED",               },
    { MSG_BTSRV_DUMP_INFO,                      "DUMP_INFO",                      },
};

static const msg2str_map_t _bt_audio_event_str_maps[] =
{
    { MSG_BTSRV_AUDIO_START,                  "START",                  },
    { MSG_BTSRV_AUDIO_STOP,                   "STOP",                   },
//  { MSG_BTSRV_AUDIO_EXIT,                   "EXIT",                   },
    { MSG_BTSRV_AUDIO_CONNECT,                "CONNECT",                },
    { MSG_BTSRV_AUDIO_PREPARE,                "PREPARE",                },
    { MSG_BTSRV_AUDIO_DISCONNECT,             "DISCONNECT",             },
    { MSG_BTSRV_AUDIO_CIS_DISCONNECT,         "CIS_DISCONNECT",         },
    { MSG_BTSRV_AUDIO_CIS_CONNECT,            "CIS_CONNECT",            },
    { MSG_BTSRV_AUDIO_CIS_RECONNECT,          "CIS_RECONNECT",          },
    { MSG_BTSRV_AUDIO_PAUSE,                  "PAUSE",                  },
    { MSG_BTSRV_AUDIO_RESUME,                 "RESUME",                 },
    { MSG_BTSRV_AUDIO_CORE_CALLBACK,		  "CORE_CALLBACK", 		},
    { MSG_BTSRV_AUDIO_SRV_CAP_INIT,           "SRV_CAP_INIT",           },
    { MSG_BTSRV_AUDIO_STREAM_CONFIG_CODEC,    "STREAM_CONFIG_CODEC",    },
    { MSG_BTSRV_AUDIO_STREAM_PREFER_QOS,      "STREAM_PREFER_QOS",      },
    { MSG_BTSRV_AUDIO_STREAM_CONFIG_QOS,      "STREAM_CONFIG_QOS",      },
    { MSG_BTSRV_AUDIO_STREAM_ENABLE,          "STREAM_ENABLE",          },
    { MSG_BTSRV_AUDIO_STREAM_DISBLE,          "STREAM_DISBLE",          },
    { MSG_BTSRV_AUDIO_STREAM_SET,             "STREAM_SET",             },
    { MSG_BTSRV_AUDIO_STREAM_START,           "STREAM_START",           },
    { MSG_BTSRV_AUDIO_STREAM_STOP,            "STREAM_STOP",            },
    { MSG_BTSRV_AUDIO_STREAM_UPDATE,          "STREAM_UPDATE",          },
    { MSG_BTSRV_AUDIO_STREAM_RELEASE,         "STREAM_RELEASE",         },
    { MSG_BTSRV_AUDIO_STREAM_DISCONNECT,      "STREAM_DISCONNECT",      },
    { MSG_BTSRV_AUDIO_STREAM_CB_REGISTER,     "STREAM_CB_REGISTER",     },
    { MSG_BTSRV_AUDIO_STREAM_SOURCE_SET,      "STREAM_SOURCE_SET",      },
    { MSG_BTSRV_AUDIO_STREAM_SYNC_FORWARD,    "STREAM_SYNC_FORWARD",    },
    { MSG_BTSRV_AUDIO_STREAM_BANDWIDTH_FREE,  "STREAM_BANDWIDTH_FREE",  },
//  { MSG_BTSRV_AUDIO_STREAM_BANDWIDTH_ALLOC, "STREAM_BANDWIDTH_ALLOC", },
    { MSG_BTSRV_AUDIO_STREAM_BIND_QOS,        "STREAM_BIND_QOS",        },
    { MSG_BTSRV_AUDIO_STREAM_RELEASED,        "STREAM_RELEASED",        },
    { MSG_BTSRV_AUDIO_VOLUME_UP,              "VOLUME_UP",              },
    { MSG_BTSRV_AUDIO_VOLUME_DOWN,            "VOLUME_DOWN",            },
    { MSG_BTSRV_AUDIO_VOLUME_SET,             "VOLUME_SET",             },
    { MSG_BTSRV_AUDIO_VOLUME_MUTE,            "VOLUME_MUTE",            },
    { MSG_BTSRV_AUDIO_VOLUME_UNMUTE,          "VOLUME_UNMUTE",          },
    { MSG_BTSRV_AUDIO_VOLUME_UNMUTE_UP,       "VOLUME_UNMUTE_UP",       },
    { MSG_BTSRV_AUDIO_VOLUME_UNMUTE_DOWN,     "VOLUME_UNMUTE_DOWN",     },
    { MSG_BTSRV_AUDIO_MIC_MUTE,               "MIC_MUTE",               },
    { MSG_BTSRV_AUDIO_MIC_UNMUTE,             "MIC_UNMUTE",             },
    { MSG_BTSRV_AUDIO_MIC_MUTE_DISABLE,       "MIC_MUTE_DISABLE",       },
    { MSG_BTSRV_AUDIO_MIC_GAIN_SET,           "MIC_GAIN_SET",           },
    { MSG_BTSRV_AUDIO_MEDIA_PLAY,             "MEDIA_PLAY",             },
    { MSG_BTSRV_AUDIO_MEDIA_PAUSE,            "MEDIA_PAUSE",            },
    { MSG_BTSRV_AUDIO_MEDIA_STOP,             "MEDIA_STOP",             },
    { MSG_BTSRV_AUDIO_MEDIA_FAST_REWIND,      "MEDIA_FAST_REWIND",      },
    { MSG_BTSRV_AUDIO_MEDIA_FAST_FORWARD,     "MEDIA_FAST_FORWARD",     },
    { MSG_BTSRV_AUDIO_MEDIA_PLAY_PREV,        "MEDIA_PLAY_PREV",        },
    { MSG_BTSRV_AUDIO_MEDIA_PLAY_NEXT,        "MEDIA_PLAY_NEXT",        },
//  { MSG_BTSRV_AUDIO_SET_PARAM,              "SET_PARAM",              },
    { MSG_BTSRV_AUDIO_VND_CB_REGISTER,        "VND_CB_REGISTER",        },
    { MSG_BTSRV_AUDIO_KEYS_CLEAR,             "KEYS_CLEAR",             },
    { MSG_BTSRV_AUDIO_ADV_CB_REGISTER,        "ADV_CB_REGISTER",        },
//  { MSG_BTSRV_AUDIO_CALL_ORIGINATE,         "CALL_ORIGINATE",         },
//  { MSG_BTSRV_AUDIO_CALL_ACCEPT,            "CALL_ACCEPT",            },
//  { MSG_BTSRV_AUDIO_CALL_HOLD,              "CALL_HOLD",              },
//  { MSG_BTSRV_AUDIO_CALL_RETRIEVE,          "CALL_RETRIEVE",          },
//  { MSG_BTSRV_AUDIO_CALL_TERMINATE,         "CALL_TERMINATE",         },
};

void btsrv_msg_print(const struct app_msg* msg, const char* func_name)
{
	//ignore next msg print
	if (msg->type == MSG_BTSRV_TWS){
		if (msg->cmd == MSG_BTSRV_TWS_RSSI_RESULT||
			msg->cmd == MSG_BTSRV_TWS_LINK_QUALITY_RESULT||
			msg->cmd == MSG_BTSRV_TWS_PROTOCOL_DATA) {
			return;
		}
	}

    if (msg->type < MSG_BTSRV_BASE)
    {
        const char* str = sys_msg2str(msg->type, "UNKNOWN");
        
        printk("<I> [%s] %d (MSG_%s), %d, 0x%x\n", 
            func_name, msg->type, str, msg->cmd, msg->value);
    }
    else
    {
        const char* msg_str = msg2str(msg->type, _btsrv_msg_str_maps, 
            ARRAY_SIZE(_btsrv_msg_str_maps), "UNKNOWN");
        
        const char* event_str;

        if (msg->type == MSG_BTSRV_AUDIO)
        {
            event_str = msg2str(msg->cmd, _bt_audio_event_str_maps, 
                ARRAY_SIZE(_bt_audio_event_str_maps), "UNKNOWN");
        }
        else
        {
            event_str = msg2str(msg->cmd, _bt_common_event_str_maps, 
                ARRAY_SIZE(_bt_common_event_str_maps), "UNKNOWN");
        }
        
        printk("<I> [%s] %d (MSG_BTSRV_%s), %d (%s), 0x%x\n", 
            func_name, msg->type, msg_str, msg->cmd, event_str, msg->value);
    }
}

void bt_service_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;
	int result = 0;
	uint32_t start_time, cost_time;

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
		    btsrv_msg_print(&msg, __FUNCTION__);
		    
			switch (msg.type) {
			case MSG_EXIT_APP:
				_bt_service_exit();
				terminaltion = true;
				break;
			case MSG_INIT_APP:
				_bt_service_init(&msg);
				break;
			default:
				if (msg.type >= MSG_BTSRV_BASE && msg.type < MSG_BTSRV_MAX &&
					msg_processer[msg.type - MSG_BTSRV_BASE]) {

					start_time = k_cycle_get_32();
					msg_processer[msg.type - MSG_BTSRV_BASE](&msg);
					cost_time = k_cyc_to_us_floor32(k_cycle_get_32() - start_time);
					if (cost_time > BTSRV_PORC_MONITOR_TIME) {
						printk("xxxx:(%s) Btsrv type %d cmd %d proc used %d us\n", __func__, msg.type, msg.cmd, cost_time);
					}
				}
				break;
			}

			if (msg.callback) {
				msg.callback(&msg, result, NULL);
			}
		}
		thread_timer_handle_expired();
	}
}
