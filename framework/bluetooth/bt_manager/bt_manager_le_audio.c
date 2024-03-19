/*
 * Copyright (c) 2021 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bt_manager.h>
#include <property_manager.h>
#include "bt_porting_inner.h"
#include "bt_manager_inner.h"
#include <audio_policy.h>
#include <audio_system.h>
#include <volume_manager.h>

#include "usb/usb_custom_handler.h"
#include "xear_hid_protocol.h"


static void bt_manager_le_audio_callback(btsrv_audio_event_e event,
				void *data, int size);

/* FIXME: move to Kconfig or any better way? */
#define NUM_PACS_CLI_PAC_PER_CONN	2
#define NUM_PACS_PAC_PER_CONN	4
#define NUM_ASCS_CLI_ASE_PER_CONN	2
#define NUM_PACS_SRV_PAC	2
#define NUM_ASCS_SRV_ASE_PER_CONN	2
#define NUM_CIS_CHAN_PER_CONN	2
#define NUM_AICS_SRV	1
#define NUM_AICS_CLI_PER_CONN	1

#if defined(CONFIG_SOC_SERIES_CUCKOO)
static uint8_t bt_le_audio_buf[CONFIG_BT_LE_AUDIO_BUF_SIZE] __aligned(4) __in_section_unique(SRAM_EXT_RAM);
#else
static uint8_t bt_le_audio_buf[CONFIG_BT_LE_AUDIO_BUF_SIZE] __aligned(4);
#endif

#ifdef CONFIG_LE_AUDIO_BQB
#define NUM_ASCS_CLI_ASE_PER_CONN_BQB	4
#endif

#ifdef CONFIG_LE_AUDIO_SR_BQB
#define NUM_ASCS_SRV_ASE_PER_CONN_BQB	4
#endif

static uint8_t lea_volume_to_music_volume(uint8_t lea_volume)
{
	uint8_t  music_vol = 0;
	uint8_t  max_volume = audio_policy_get_max_volume(AUDIO_STREAM_MUSIC);

	music_vol = (lea_volume + max_volume-1)/max_volume;
	SYS_LOG_INF("%d -> %d", lea_volume, music_vol);
	
	return music_vol;
}

int bt_manager_le_audio_init(struct bt_audio_role *role)
{
	struct bt_audio_config c = {0};
	uint8_t num;
	uint8_t sys_max_volume = audio_policy_get_max_volume(AUDIO_STREAM_LE_AUDIO_MUSIC);
	uint8_t leaudio_volume =  audio_system_get_stream_volume(AUDIO_STREAM_LE_AUDIO_MUSIC);

	if (!role) {
		return -EINVAL;
	}

	SYS_LOG_INF("num_master_conn: %d, num_slave_conn: %d",
		role->num_master_conn, role->num_slave_conn);

	c.num_master_conn = role->num_master_conn;
	c.num_slave_conn = role->num_slave_conn;
	c.br = role->br;
	c.remote_keys = role->remote_keys;
	c.test_addr = role->test_addr;
	c.master_latency = role->master_latency;
	c.sirk_enabled = role->sirk_enabled;
	c.sirk_encrypted = role->sirk_encrypted;
	if (role->sirk_enabled) {
		memcpy(c.set_sirk, role->set_sirk, 16);
	}

	/* disable remote_keys in test_addr case */
	if (role->test_addr) {
		c.remote_keys = 0;
	}

	if (role->num_master_conn) {
		c.pacs_cli_num = 1;
		c.pacs_cli_dev_num = role->num_master_conn;
		num = NUM_PACS_CLI_PAC_PER_CONN * role->num_master_conn;
		if (num > 31) {
			SYS_LOG_ERR("pacs_cli_pac_num: %d", num);
			return -EINVAL;
		}
		c.pacs_cli_pac_num = num;
		num = NUM_PACS_PAC_PER_CONN * role->num_master_conn;
		if (num > 31) {
			SYS_LOG_ERR("pacs_pac_num: %d", num);
			return -EINVAL;
		}
		c.pacs_pac_num = num;
		if (role->set_coordinator) {
			c.cas_cli_num = 1;
			c.cas_cli_dev_num = role->num_master_conn;
		}
		c.tmas_cli_num = 1;
		c.tmas_cli_dev_num = role->num_master_conn;
		if (role->call_gateway) {
			c.call_srv_num = 1;
		}
		if (role->volume_controller) {
			c.vol_cli_num = 1;
			c.vcs_cli_dev_num = role->num_master_conn;
		}
		if (role->microphone_controller) {
			c.mic_cli_num = 1;
			c.mic_cli_dev_num = role->num_master_conn;
			num = NUM_AICS_CLI_PER_CONN * role->num_master_conn;
			if (num > 31) {
				SYS_LOG_ERR("aics cli num: %d", num);
				return -EINVAL;
			}
			c.aics_cli_num = num;
		}
		if (role->media_device) {
			c.media_srv_num = 1;
		}
		if (role->unicast_client) {
			c.uni_cli_num = 1;
			c.uni_cli_dev_num = role->num_master_conn;
			num = NUM_ASCS_CLI_ASE_PER_CONN * role->num_master_conn;
#ifdef CONFIG_LE_AUDIO_BQB
            num = NUM_ASCS_CLI_ASE_PER_CONN_BQB * role->num_master_conn;
#endif
			if (num > 31) {
				SYS_LOG_ERR("ascs_cli_ase_num: %d", num);
				return -EINVAL;
			}
			c.ascs_cli_ase_num = num;
		}
	}

	if (role->num_slave_conn) {
		c.pacs_srv_num = 1;
		c.pacs_srv_pac_num = NUM_PACS_SRV_PAC;
		if (role->set_member) {
			c.cas_srv_num = 1;
			c.set_size = role->set_size;
			c.set_rank = role->set_rank;
		}
		if (role->call_terminal) {
			c.call_cli_num = 1;
			c.call_cli_dev_num = role->num_slave_conn;
		}
		if (role->volume_renderer) {
			c.vol_srv_num = 1;
		}
		if (role->microphone_device) {
			c.mic_srv_num = 1;
			c.aics_srv_num = NUM_AICS_SRV;
		}
		if (role->media_controller) {
			c.media_cli_num = 1;
			c.mcs_cli_dev_num = role->num_slave_conn;
		}
		if (role->unicast_server) {
			c.uni_srv_num = 1;
			c.uni_srv_dev_num = role->num_slave_conn;
#ifdef CONFIG_LE_AUDIO_SR_BQB
            num = NUM_ASCS_SRV_ASE_PER_CONN_BQB * role->num_slave_conn;
#else
			num = NUM_ASCS_SRV_ASE_PER_CONN * role->num_slave_conn;
#endif
			if (num > 31) {
				SYS_LOG_ERR("ascs_srv_ase_num: %d", num);
				return -EINVAL;
			}
			c.ascs_srv_ase_num = num;
		}
	}

	num = role->num_master_conn + role->num_slave_conn;
	num *= NUM_CIS_CHAN_PER_CONN;
	if (num > 31) {
		SYS_LOG_ERR("cis_chan_num: %d", num);
		return -EINVAL;
	}
	c.cis_chan_num = num;

	c.encryption = role->encryption;
	c.secure = role->secure;
	
	c.volume_step = ((uint32_t)((uint8_t)0xff + sys_max_volume -1))/sys_max_volume;

	c.initial_volume = leaudio_volume * c.volume_step;
	
	SYS_LOG_INF("volume_step %d initial_volume %d", c.volume_step,c.initial_volume);

	return btif_audio_init(c, bt_manager_le_audio_callback,
		bt_le_audio_buf, sizeof(bt_le_audio_buf));
}

static const msg2str_map_t _le_audio_event_str_maps[] =
{
    { BTSRV_ACL_CONNECTED,           "ACL_CONNECTED",           },
    { BTSRV_ACL_DISCONNECTED,        "ACL_DISCONNECTED",        },
    { BTSRV_CIS_CONNECTED,           "CIS_CONNECTED",           },
    { BTSRV_CIS_DISCONNECTED,        "CIS_DISCONNECTED",        },
    { BTSRV_ASCS_CONFIG_CODEC,       "ASCS_CONFIG_CODEC",       },
    { BTSRV_ASCS_PREFER_QOS,         "ASCS_PREFER_QOS",         },
    { BTSRV_ASCS_CONFIG_QOS,         "ASCS_CONFIG_QOS",         },
    { BTSRV_ASCS_ENABLE,             "ASCS_ENABLE",             },
    { BTSRV_ASCS_DISABLE,            "ASCS_DISABLE",            },
    { BTSRV_ASCS_UPDATE,             "ASCS_UPDATE",             },
    { BTSRV_ASCS_RECV_START,         "ASCS_RECV_START",         },
    { BTSRV_ASCS_RECV_STOP,          "ASCS_RECV_STOP",          },
    { BTSRV_ASCS_RELEASE,            "ASCS_RELEASE",            },
    { BTSRV_ASCS_ASES,               "ASCS_ASES",               },
    { BTSRV_PACS_CAPS,               "PACS_CAPS",               },
    { BTSRV_CIS_PARAMS,              "CIS_PARAMS",              },
//  { BTSRV_TBS_CONNECTED,           "TBS_CONNECTED",           },
//  { BTSRV_TBS_DISCONNECTED,        "TBS_DISCONNECTED",        },
//  { BTSRV_TBS_INCOMING,            "TBS_INCOMING",            },
//  { BTSRV_TBS_DIALING,             "TBS_DIALING",             },
//  { BTSRV_TBS_ALERTING,            "TBS_ALERTING",            },
//  { BTSRV_TBS_ACTIVE,              "TBS_ACTIVE",              },
//  { BTSRV_TBS_LOCALLY_HELD,        "TBS_LOCALLY_HELD",        },
//  { BTSRV_TBS_REMOTELY_HELD,       "TBS_REMOTELY_HELD",       },
//  { BTSRV_TBS_HELD,                "TBS_HELD",                },
//  { BTSRV_TBS_ENDED,               "TBS_ENDED",               },
    { BTSRV_VCS_UP,                  "VCS_UP",                  },
    { BTSRV_VCS_DOWN,                "VCS_DOWN",                },
    { BTSRV_VCS_VALUE,               "VCS_VALUE",               },
    { BTSRV_VCS_MUTE,                "VCS_MUTE",                },
    { BTSRV_VCS_UNMUTE,              "VCS_UNMUTE",              },
    { BTSRV_VCS_UNMUTE_UP,           "VCS_UNMUTE_UP",           },
    { BTSRV_VCS_UNMUTE_DOWN,         "VCS_UNMUTE_DOWN",         },
    { BTSRV_VCS_CLIENT_CONNECTED,    "VCS_CLIENT_CONNECTED",    },
    { BTSRV_VCS_CLIENT_UP,           "VCS_CLIENT_UP",           },
    { BTSRV_VCS_CLIENT_DOWN,         "VCS_CLIENT_DOWN",         },
    { BTSRV_VCS_CLIENT_VALUE,        "VCS_CLIENT_VALUE",        },
    { BTSRV_VCS_CLIENT_MUTE,         "VCS_CLIENT_MUTE",         },
    { BTSRV_VCS_CLIENT_UNMUTE,       "VCS_CLIENT_UNMUTE",       },
    { BTSRV_VCS_CLIENT_UNMUTE_UP,    "VCS_CLIENT_UNMUTE_UP",    },
    { BTSRV_VCS_CLIENT_UNMUTE_DOWN,  "VCS_CLIENT_UNMUTE_DOWN",  },
    { BTSRV_MICS_MUTE,               "MICS_MUTE",               },
    { BTSRV_MICS_UNMUTE,             "MICS_UNMUTE",             },
    { BTSRV_MICS_GAIN_VALUE,         "MICS_GAIN_VALUE",         },
    { BTSRV_MICS_CLIENT_CONNECTED,   "MICS_CLIENT_CONNECTED",   },
    { BTSRV_MICS_CLIENT_MUTE,        "MICS_CLIENT_MUTE",        },
    { BTSRV_MICS_CLIENT_UNMUTE,      "MICS_CLIENT_UNMUTE",      },
    { BTSRV_MICS_CLIENT_GAIN_VALUE,  "MICS_CLIENT_GAIN_VALUE",  },
    { BTSRV_MCS_SERVER_PLAY,         "MCS_SERVER_PLAY",         },
    { BTSRV_MCS_SERVER_PAUSE,        "MCS_SERVER_PAUSE",        },
    { BTSRV_MCS_SERVER_STOP,         "MCS_SERVER_STOP",         },
    { BTSRV_MCS_SERVER_FAST_REWIND,  "MCS_SERVER_FAST_REWIND",  },
    { BTSRV_MCS_SERVER_FAST_FORWARD, "MCS_SERVER_FAST_FORWARD", },
    { BTSRV_MCS_SERVER_NEXT_TRACK,   "MCS_SERVER_NEXT_TRACK",   },
    { BTSRV_MCS_SERVER_PREV_TRACK,   "MCS_SERVER_PREV_TRACK",   },
    { BTSRV_MCS_CONNECTED,           "MCS_CONNECTED",           },
    { BTSRV_MCS_PLAY,                "MCS_PLAY",                },
    { BTSRV_MCS_PAUSE,               "MCS_PAUSE",               },
    { BTSRV_MCS_STOP,                "MCS_STOP",                },
};

static void bt_manager_le_audio_callback(btsrv_audio_event_e event,
				void *data, int size)
{
	SYS_LOG_INF("%d (BTSRV_%s)", event, msg2str(event, _le_audio_event_str_maps, 
	    ARRAY_SIZE(_le_audio_event_str_maps), "UNKNOWN"));

	switch (event) {
	case BTSRV_ACL_CONNECTED:
		bt_manager_audio_conn_event(BT_CONNECTED, data, size);
		break;
	case BTSRV_ACL_DISCONNECTED:
		bt_manager_audio_conn_event(BT_DISCONNECTED, data, size);
		break;
	case BTSRV_CIS_CONNECTED:
		bt_manager_audio_conn_event(BT_AUDIO_CONNECTED, data, size);
		break;
	case BTSRV_CIS_DISCONNECTED:
		bt_manager_audio_conn_event(BT_AUDIO_DISCONNECTED, data, size);
		break;

	/* ASCS */
	case BTSRV_ASCS_CONFIG_CODEC:
		bt_manager_audio_stream_event(BT_AUDIO_STREAM_CONFIG_CODEC, data, size);
		break;
	case BTSRV_ASCS_PREFER_QOS:
		bt_manager_audio_stream_event(BT_AUDIO_STREAM_PREFER_QOS, data, size);
		break;
	case BTSRV_ASCS_CONFIG_QOS:
		bt_manager_audio_stream_event(BT_AUDIO_STREAM_CONFIG_QOS, data, size);
		break;
	case BTSRV_ASCS_ENABLE:
		bt_manager_audio_stream_event(BT_AUDIO_STREAM_ENABLE, data, size);
		break;
	case BTSRV_ASCS_DISABLE:
		bt_manager_audio_stream_event(BT_AUDIO_STREAM_DISABLE, data, size);
		break;
	case BTSRV_ASCS_RECV_START:
		bt_manager_audio_stream_event(BT_AUDIO_STREAM_START, data, size);
		break;
	case BTSRV_ASCS_RECV_STOP:
		bt_manager_audio_stream_event(BT_AUDIO_STREAM_STOP, data, size);
		break;
	case BTSRV_ASCS_RELEASE:
		bt_manager_audio_stream_event(BT_AUDIO_STREAM_RELEASE, data, size);
		break;
	case BTSRV_ASCS_ASES:
		bt_manager_audio_stream_event(BT_AUDIO_DISCOVER_ENDPOINTS, data, size);
		break;
	/* PACS */
	case BTSRV_PACS_CAPS:
		bt_manager_audio_stream_event(BT_AUDIO_DISCOVER_CAPABILITY, data, size);
		break;
	/* CIS */
	case BTSRV_CIS_PARAMS:
		bt_manager_audio_stream_event(BT_AUDIO_PARAMS, data, size);
		break;

	/* TBS */
	case BTSRV_TBS_INCOMING:
		bt_manager_call_event(BT_CALL_INCOMING, data, size);
		break;
	case BTSRV_TBS_DIALING:
		bt_manager_call_event(BT_CALL_DIALING, data, size);
		break;
	case BTSRV_TBS_ALERTING:
		bt_manager_call_event(BT_CALL_ALERTING, data, size);
		break;
	case BTSRV_TBS_ACTIVE:
		bt_manager_call_event(BT_CALL_ACTIVE, data, size);
		break;
	case BTSRV_TBS_LOCALLY_HELD:
		bt_manager_call_event(BT_CALL_LOCALLY_HELD, data, size);
		break;
	case BTSRV_TBS_REMOTELY_HELD:
		bt_manager_call_event(BT_CALL_REMOTELY_HELD, data, size);
		break;
	case BTSRV_TBS_HELD:
		bt_manager_call_event(BT_CALL_HELD, data, size);
		break;
	case BTSRV_TBS_ENDED:
		bt_manager_call_event(BT_CALL_ENDED, data, size);
		break;

	/* VCS */
	case BTSRV_VCS_UP:
		bt_manager_volume_event(BT_VOLUME_UP, data, size);
		break;
	case BTSRV_VCS_DOWN:
		bt_manager_volume_event(BT_VOLUME_DOWN, data, size);
		break;
	case BTSRV_VCS_VALUE:
	{
		struct bt_volume_report rep;
		memcpy(&rep, data, sizeof(struct bt_volume_report));
		rep.value =  lea_volume_to_music_volume(rep.value);
		rep.from_phone = 1;
		bt_manager_volume_event(BT_VOLUME_VALUE, &rep, sizeof(struct bt_volume_report));
	}	
		break;
	case BTSRV_VCS_MUTE:
		bt_manager_volume_event(BT_VOLUME_MUTE, data, size);
		break;
	case BTSRV_VCS_UNMUTE:
		bt_manager_volume_event(BT_VOLUME_UNMUTE, data, size);
		break;
	case BTSRV_VCS_UNMUTE_UP:
		bt_manager_volume_event(BT_VOLUME_UNMUTE_UP, data, size);
		break;
	case BTSRV_VCS_UNMUTE_DOWN:
		bt_manager_volume_event(BT_VOLUME_UNMUTE_DOWN, data, size);
		break;
	case BTSRV_VCS_CLIENT_CONNECTED:
		bt_manager_volume_event(BT_VOLUME_CLIENT_CONNECTED, data, size);
		break;
	case BTSRV_VCS_CLIENT_UP:
		bt_manager_volume_event(BT_VOLUME_CLIENT_UP, data, size);
		break;
	case BTSRV_VCS_CLIENT_DOWN:
		bt_manager_volume_event(BT_VOLUME_CLIENT_DOWN, data, size);
		break;
	case BTSRV_VCS_CLIENT_VALUE:
		bt_manager_volume_event(BT_VOLUME_CLIENT_VALUE, data, size);
		break;
	case BTSRV_VCS_CLIENT_MUTE:
		bt_manager_volume_event(BT_VOLUME_CLIENT_MUTE, data, size);
		break;
	case BTSRV_VCS_CLIENT_UNMUTE:
		bt_manager_volume_event(BT_VOLUME_CLIENT_UNMUTE, data, size);
		break;
	case BTSRV_VCS_CLIENT_UNMUTE_UP:
		bt_manager_volume_event(BT_VOLUME_CLIENT_UNMUTE_UP, data, size);
		break;
	case BTSRV_VCS_CLIENT_UNMUTE_DOWN:
		bt_manager_volume_event(BT_VOLUME_CLIENT_UNMUTE_DOWN, data, size);
		break;

	/* MICS */
	case BTSRV_MICS_MUTE:
		bt_manager_mic_event(BT_MIC_MUTE, data, size);
		break;
	case BTSRV_MICS_UNMUTE:
		bt_manager_mic_event(BT_MIC_UNMUTE, data, size);
		break;
	case BTSRV_MICS_CLIENT_CONNECTED:
		bt_manager_mic_event(BT_MIC_CLIENT_CONNECTED, data, size);
	case BTSRV_MICS_GAIN_VALUE:
		bt_manager_mic_event(BT_MIC_GAIN_VALUE, data, size);
		break;
	case BTSRV_MICS_CLIENT_MUTE:
		bt_manager_mic_event(BT_MIC_CLIENT_MUTE, data, size);
		break;
	case BTSRV_MICS_CLIENT_UNMUTE:
		bt_manager_mic_event(BT_MIC_CLIENT_UNMUTE, data, size);
		break;
	case BTSRV_MICS_CLIENT_GAIN_VALUE:
		bt_manager_mic_event(BT_MIC_CLIENT_GAIN_VALUE, data, size);
		break;

	/* MCS */
	case BTSRV_MCS_SERVER_PLAY:
		bt_manager_media_event(BT_MEDIA_SERVER_PLAY, data, size);
		break;
	case BTSRV_MCS_SERVER_PAUSE:
		bt_manager_media_event(BT_MEDIA_SERVER_PAUSE, data, size);
		break;
	case BTSRV_MCS_SERVER_STOP:
		bt_manager_media_event(BT_MEDIA_SERVER_STOP, data, size);
		break;
	case BTSRV_MCS_SERVER_FAST_REWIND:
		bt_manager_media_event(BT_MEDIA_SERVER_FAST_REWIND, data, size);
		break;
	case BTSRV_MCS_SERVER_FAST_FORWARD:
		bt_manager_media_event(BT_MEDIA_SERVER_FAST_FORWARD, data, size);
		break;
	case BTSRV_MCS_SERVER_NEXT_TRACK:
		bt_manager_media_event(BT_MEDIA_SERVER_NEXT_TRACK, data, size);
		break;
	case BTSRV_MCS_SERVER_PREV_TRACK:
		bt_manager_media_event(BT_MEDIA_SERVER_PREV_TRACK, data, size);
		break;
	case BTSRV_MCS_PLAY:
		bt_manager_media_event(BT_MEDIA_PLAY, data, size);
		break;
	case BTSRV_MCS_PAUSE:
		bt_manager_media_event(BT_MEDIA_PAUSE, data, size);
		break;
	case BTSRV_MCS_STOP:
		bt_manager_media_event(BT_MEDIA_STOP, data, size);
		break;

	default:
		break;
	}
}

int bt_manager_le_audio_exit(void)
{
	return btif_audio_exit();
}

int bt_manager_le_audio_keys_clear(void)
{
	return btif_audio_keys_clear();
}

int bt_manager_le_audio_start(void)
{
	return btif_audio_start();
}

int bt_manager_le_audio_stop(int blocked)
{
	int time_out = 0;
	SYS_LOG_INF("");

	btif_audio_stop();

	if (blocked){
		/* wait forever */
		while (!btif_audio_stopped()) {
			if(time_out++ > 300){
				break;
			}
			
			os_sleep(10);
		}
	}

	SYS_LOG_INF("done");

	return 0;
}

int bt_manager_le_audio_pause(void)
{
	return btif_audio_pause();
}

int bt_manager_le_audio_resume(void)
{
	return btif_audio_resume();
}

int bt_manager_le_audio_server_cap_init(struct bt_audio_capabilities *caps)
{
	return btif_audio_server_cap_init(caps);
}

int bt_manager_le_audio_stream_config_codec(struct bt_audio_codec *codec)
{
	return btif_audio_stream_config_codec(codec);
}

int bt_manager_le_audio_stream_prefer_qos(struct bt_audio_preferred_qos *qos)
{
	return btif_audio_stream_prefer_qos(qos);
}

int bt_manager_le_audio_stream_config_qos(struct bt_audio_qoss *qoss)
{
	return btif_audio_stream_config_qos(qoss);
}

int bt_manager_le_audio_stream_bind_qos(struct bt_audio_qoss *qoss, uint8_t index)
{
	return btif_audio_stream_bind_qos(qoss, index);
}

int bt_manager_le_audio_stream_sync_forward(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
	return btif_audio_stream_sync_forward(chans, num_chans);
}

int bt_manager_le_audio_stream_enable(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t contexts)
{
	return btif_audio_stream_enable(chans, num_chans, contexts);
}

int bt_manager_le_audio_stream_disable(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
	return btif_audio_stream_disble(chans, num_chans);
}

io_stream_t bt_manager_le_audio_source_stream_create(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t locations)
{
	return btif_audio_source_stream_create(chans, num_chans, locations);
}

int bt_manager_le_audio_source_stream_set(struct bt_audio_chan **chans,
				uint8_t num_chans, io_stream_t stream,
				uint32_t locations)
{
	return btif_audio_source_stream_set(chans, num_chans, stream, locations);
}

int bt_manager_le_audio_sink_stream_set(struct bt_audio_chan *chan,
				io_stream_t stream)
{
	return btif_audio_sink_stream_set(chan, stream);
}

int bt_manager_le_audio_sink_stream_start(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
	return btif_audio_sink_stream_start(chans, num_chans);
}

int bt_manager_le_audio_sink_stream_stop(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
	return btif_audio_sink_stream_stop(chans, num_chans);
}

int bt_manager_le_audio_stream_update(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t contexts)
{
	return btif_audio_stream_update(chans, num_chans, contexts);
}

int bt_manager_le_audio_stream_release(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
	return btif_audio_stream_release(chans, num_chans);
}

/* released channel to idle state after release, for slave only */
int bt_manager_le_audio_stream_released(struct bt_audio_chan *chan)
{
	return btif_audio_stream_released(chan);
}

int bt_manager_le_audio_stream_disconnect(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
	return btif_audio_stream_disconnect(chans, num_chans);
}

void *bt_manager_le_audio_stream_bandwidth_alloc(struct bt_audio_qoss *qoss)
{
	return btif_audio_stream_bandwidth_alloc(qoss);
}

int bt_manager_le_audio_stream_bandwidth_free(void *cig_handle)
{
	return btif_audio_stream_bandwidth_free(cig_handle);
}

int bt_manager_le_audio_stream_cb_register(struct bt_audio_stream_cb *cb)
{
	return btif_audio_stream_cb_register(cb);
}

int bt_manager_le_volume_up(uint16_t handle)
{
	return btif_audio_volume_up(handle);
}

int bt_manager_le_volume_down(uint16_t handle)
{
	return btif_audio_volume_down(handle);
}

int bt_manager_le_volume_set(uint16_t handle, uint8_t value)
{
	return btif_audio_volume_set(handle, value);
}

int bt_manager_le_volume_mute(uint16_t handle)
{
	return btif_audio_volume_mute(handle);
}

int bt_manager_le_volume_unmute(uint16_t handle)
{
	return btif_audio_volume_unmute(handle);
}

int bt_manager_le_volume_unmute_up(uint16_t handle)
{
	return btif_audio_volume_unmute_up(handle);
}

int bt_manager_le_volume_unmute_down(uint16_t handle)
{
	return btif_audio_volume_unmute_down(handle);
}

int bt_manager_le_mic_mute(uint16_t handle)
{
	return btif_audio_mic_mute(handle);
}

int bt_manager_le_mic_unmute(uint16_t handle)
{
	return btif_audio_mic_unmute(handle);
}

int bt_manager_le_mic_mute_disable(uint16_t handle)
{
	return btif_audio_mic_mute_disable(handle);
}

int bt_manager_le_mic_gain_set(uint16_t handle, uint8_t gain)
{
	return btif_audio_mic_gain_set(handle, gain);
}

int bt_manager_le_media_play(uint16_t handle)
{
	return btif_audio_media_play(handle);
}

int bt_manager_le_media_pause(uint16_t handle)
{
	return btif_audio_media_pause(handle);
}

int bt_manager_le_media_fast_rewind(uint16_t handle)
{
	return btif_audio_media_fast_rewind(handle);
}

int bt_manager_le_media_fast_forward(uint16_t handle)
{
	return btif_audio_media_fast_forward(handle);
}

int bt_manager_le_media_stop(uint16_t handle)
{
	return btif_audio_media_stop(handle);
}

int bt_manager_le_media_play_previous(uint16_t handle)
{
	return btif_audio_media_play_prev(handle);
}

int bt_manager_le_media_play_next(uint16_t handle)
{
	return btif_audio_media_play_next(handle);
}

int bt_manager_audio_le_adv_param_init(struct bt_le_adv_param *param)
{
	if (!param) {
		return -EINVAL;
	}

	return btif_audio_adv_param_init(param);
}

int bt_manager_audio_le_scan_param_init(struct bt_le_scan_param *param)
{
	if (!param) {
		return -EINVAL;
	}

	return btif_audio_scan_param_init(param);
}

int bt_manager_audio_le_conn_create_param_init(struct bt_conn_le_create_param *param)
{
	if (!param) {
		return -EINVAL;
	}

	return btif_audio_conn_create_param_init(param);
}

int bt_manager_audio_le_conn_param_init(struct bt_le_conn_param *param)
{
	if (!param) {
		return -EINVAL;
	}

	return btif_audio_conn_param_init(param);
}

int bt_manager_audio_le_conn_idle_param_init(struct bt_le_conn_param *param)
{
	if (!param) {
		return -EINVAL;
	}

	return btif_audio_conn_idle_param_init(param);
}

int bt_manager_audio_le_conn_max_len(uint16_t handle)
{
	if (!handle) {
		return -EINVAL;
	}

	return btif_audio_conn_max_len(handle);
}

int bt_manager_audio_le_vnd_register_rx_cb(uint16_t handle,
				bt_audio_vnd_rx_cb rx_cb)
{
	if (!handle || !rx_cb) {
		return -EINVAL;
	}

	return btif_audio_vnd_register_rx_cb(handle, rx_cb);
}

int bt_manager_audio_le_vnd_send(uint16_t handle, uint8_t *buf, uint16_t len)
{
	if (!handle || !buf || !len) {
		return -EINVAL;
	}

	return btif_audio_vnd_send(handle, buf, len);
}

int bt_manager_audio_le_adv_cb_register(bt_audio_adv_cb start,
				bt_audio_adv_cb stop)
{
	return btif_audio_adv_cb_register(start, stop);
}

int bt_manager_audio_le_start_adv(void)
{
	return btif_audio_start_adv();
}

int bt_manager_audio_le_stop_adv(void)
{
	return btif_audio_stop_adv();
}

int bt_manager_le_call_originate(uint16_t handle, uint8_t *remote_uri)
{
	return btif_audio_call_originate(handle, remote_uri);
}

int bt_manager_le_call_accept(struct bt_audio_call *call)
{
	return btif_audio_call_accept(call);
}

int bt_manager_le_call_hold(struct bt_audio_call *call)
{
	return btif_audio_call_hold(call);
}

int bt_manager_le_call_retrieve(struct bt_audio_call *call)
{
	return btif_audio_call_retrieve(call);
}

int bt_manager_le_call_terminate(struct bt_audio_call *call)
{
	return btif_audio_call_terminate(call);
}

#define CFG_BT_PUBLIC_SIRK                 "BT_PUBLIC_SIRK"
#define CFG_BT_PRIVATE_SIRK                "BT_PRIVATE_SIRK"
#define CFG_BT_LEA_RANK                    "BT_LEA_RANK"

#define SIRK_LEN   16
const char sirk_priv_pre[4]={0xcd, 0xcc, 0x72, 0xdd};   
const char sirk_publ_pre[4]={0xaa, 0xbb, 0xcc, 0xdd};   

int bt_manager_audio_get_sirk(char* sirkbuf, int sirkbuf_size)
{
	int ret = 0;
#ifdef CONFIG_PROPERTY	
	uint8_t sirk[SIRK_LEN];
	uint8_t *cfg_name = CFG_BT_PUBLIC_SIRK;	

	if (!sirkbuf){
		return -1;
	}

	memset(sirk, 0, SIRK_LEN);
	memset(sirkbuf, 0, sirkbuf_size);
	
	ret = property_get(cfg_name, sirk, (SIRK_LEN));
	if ((ret > 0) && (ret <= sirkbuf_size)) {
		memcpy(sirkbuf, sirk, ret);
		ret = 0;
		goto Exit;
	}

	cfg_name = CFG_BT_PRIVATE_SIRK;
	memset(sirk, 0, SIRK_LEN);
	
	ret = property_get(cfg_name, sirk, (SIRK_LEN));
	if ((ret > 0) && (ret <= sirkbuf_size)) {
		memcpy(sirkbuf, sirk, ret);
		ret = 0;
		goto Exit;
	}else{
		uint8_t mac_str[12];
		uint8_t mac[6];
		bt_manager_bt_mac(mac_str);		
		hex2bin(mac_str, 12, mac, 6);

		memcpy(sirk, sirk_priv_pre,4);	
		memcpy(sirk+4, mac,6);	
		memcpy(sirk+10, mac,6);	
		property_set_factory(cfg_name, sirk, SIRK_LEN);
		if(SIRK_LEN <= sirkbuf_size){
			memcpy(sirkbuf, sirk, SIRK_LEN);
		}
		ret = 0;
	}
	
Exit:
	stack_print_hex("get sirk:", sirk, SIRK_LEN);
#endif
	return ret;

}

/*retval: 0 unneed restart leaudio, 1 need restart leaudio*/
int bt_manager_audio_set_public_sirk(char* sirkbuf, int sirkbuf_size)
{
	int ret = 0;
#ifdef CONFIG_PROPERTY
	uint8_t sirk[SIRK_LEN];
	uint8_t *cfg_name = CFG_BT_PUBLIC_SIRK;
	
	if (!sirkbuf){
		return -1;
	}

	memset(sirk, 0, SIRK_LEN);
	ret = property_get(cfg_name, sirk, (SIRK_LEN));
	if ((ret > 0) && (ret <= sirkbuf_size)) {
		if (memcmp(sirk, sirkbuf, ret) == 0){
			ret = 0;
			goto Exit;
		}
	}

	if(SIRK_LEN <= sirkbuf_size){
		property_set_factory(cfg_name, sirkbuf, SIRK_LEN);
		ret = 1;
	}else{
		ret = -1;
		SYS_LOG_INF("fail set %s, %d", cfg_name, ret);				
	}

Exit:
	SYS_LOG_INF("ret:%d", ret);
	stack_print_hex("set sirk", sirkbuf, SIRK_LEN);
#endif
	return ret;

}

/*retval: 0 unneed restart leaudio, 1 need restart leaudio*/
int bt_manager_audio_get_public_sirk(char* sirkbuf, int sirkbuf_size)
{
	int ret = 0;
#ifdef CONFIG_PROPERTY	
	uint8_t sirk[SIRK_LEN];
	uint8_t *cfg_name = CFG_BT_PUBLIC_SIRK;	

	if (!sirkbuf){
		return -1;
	}

	memset(sirk, 0, SIRK_LEN);
	memset(sirkbuf, 0, sirkbuf_size);
	
	ret = property_get(cfg_name, sirk, (SIRK_LEN));
	if ((ret > 0) && (ret <= sirkbuf_size)) {
		memcpy(sirkbuf, sirk, ret);
		ret = 0;
		goto Exit;
	}else{
		if (btif_tws_get_dev_role() != BTSRV_TWS_MASTER) {
			ret = -1;
			SYS_LOG_INF("Can't get %s, %d", cfg_name, ret);							
			goto Exit;
		}
	
		uint8_t mac_str[12];
		uint8_t mac[6];
		bt_manager_bt_mac(mac_str);		
		hex2bin(mac_str, 12, mac, 6);

		memcpy(sirk, sirk_publ_pre,4);	
		memcpy(sirk+4, mac,6);	
		memcpy(sirk+10, mac,6);	
		property_set_factory(cfg_name, sirk, SIRK_LEN);
		if(SIRK_LEN <= sirkbuf_size){
			memcpy(sirkbuf, sirk, SIRK_LEN);
		}
		ret = 1;
	}
Exit:
	stack_print_hex("get publ sirk:", sirk, SIRK_LEN);
#endif
	return ret;

}

/*retval: 0 unneed restart leaudio, 1 need restart leaudio*/
int bt_manager_audio_set_rank(uint8_t rank)
{
	int ret = 0;
#ifdef CONFIG_PROPERTY
	uint8_t _rank = 0;
	uint8_t *cfg_name = CFG_BT_LEA_RANK;
	
	ret = property_get(cfg_name, &_rank, sizeof(uint8_t));
	if (ret > 0) {
		if (rank == _rank){
			ret = 0;
			goto Exit;
		}
	}
	property_set_factory(cfg_name, &rank, sizeof(uint8_t));
	ret = 1;
Exit:
	SYS_LOG_INF("set rank:%d, ret:%d", rank,ret);
#endif
	return ret;
}

int bt_manager_audio_get_rank_and_size(uint8_t* rank, uint8_t* size)
{
	int ret = 0;
#ifdef CONFIG_PROPERTY
	uint8_t _rank = 0;
	uint8_t _size = 0;
	uint8_t *cfg_name = CFG_BT_LEA_RANK;
	
	ret = property_get(cfg_name, &_rank, sizeof(uint8_t));
	if (ret > 0) {
		_size = 2;
		ret = 0;
	}else{
		btmgr_tws_pair_cfg_t *cfg_tws = bt_manager_get_tws_pair_config();
		if(cfg_tws->tws_device_channel_l_r != TWS_DEVICE_CHANNEL_NONE){
			_rank = (cfg_tws->tws_device_channel_l_r == TWS_DEVICE_CHANNEL_L)?1:2;
			ret = 0;
		}else{
			_rank = 1;
			ret = 1;			
		}
		_size = 2;//1; FIXME
	}

	SYS_LOG_INF("rank:%d, size:%d", _rank, _size);
	
	if (rank) *rank = _rank;
	if (size) *size = _size;

#endif
	return ret;
}


#define CFG_BT_LEA_PEER_ADDR                 "BT_LEA_PEER"

int bt_manager_audio_adv_param_init(bt_addr_le_t *peer)
{
	struct bt_le_adv_param param = {0};

	param.id = BT_ID_DEFAULT;
	param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
	param.options = BT_LE_ADV_OPT_CONNECTABLE;
#ifdef CONFIG_BT_EXT_ADV
#ifdef CONFIG_LE_AUDIO_BQB
	param.options |= BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_NO_2M;
#endif
#endif	
	if (peer){
		param.options |= BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY;
		param.peer = peer;
		char addr_str[BT_ADDR_LE_STR_LEN];
		bt_addr_le_to_str(peer, addr_str, sizeof(addr_str));
		SYS_LOG_INF("directed ADV, peer:%s, ", addr_str);
	}

	return bt_manager_audio_le_adv_param_init(&param);
}


int bt_manager_audio_get_peer_addr(bt_addr_le_t* peer)
{
	int ret = 0;
#ifdef CONFIG_PROPERTY	
	uint8_t *cfg_name = CFG_BT_LEA_PEER_ADDR;
	if (!peer){
		return -1;
	}

	bt_addr_le_t tmp_peer;

	memset(peer, 0, sizeof(bt_addr_le_t));
	ret = property_get(cfg_name, (char*)peer, sizeof(bt_addr_le_t));
	if ((ret > 0) && (ret <= sizeof(bt_addr_le_t))) {
		memset(&tmp_peer, 0, sizeof(bt_addr_le_t));		
		if (memcmp((char*)peer, (char*)&tmp_peer, sizeof(bt_addr_le_t)) == 0){
			ret = -1;
		}else{		
			ret = 0;
		}
	}else{
		ret = -1;
	}
#endif
	return ret;

}

/*retval:0 new peer saved, 1: peer has been saved before*/
int bt_manager_audio_save_peer_addr(bt_addr_le_t* peer)
{
#ifdef CONFIG_PROPERTY	
	uint8_t *cfg_name = CFG_BT_LEA_PEER_ADDR;
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t tmp_peer;
	
	if (!peer){
		return -1;
	}
	bt_addr_le_to_str(peer, addr_str, sizeof(addr_str));
	if (bt_manager_audio_get_peer_addr(&tmp_peer) == 0){
		if (memcmp((char*)peer, (char*)&tmp_peer, sizeof(bt_addr_le_t)) == 0){
			SYS_LOG_INF("peer:%s has been saved before", addr_str);			
			return 1;
		}
	}

	property_set(cfg_name, (char*)peer, sizeof(bt_addr_le_t));
	SYS_LOG_INF("peer:%s ", addr_str);
#endif
	return 0;

}

#ifdef CONFIG_XEAR_HID_PROTOCOL
int bt_manager_le_xear_set(uint8_t featureId, uint8_t value)
{
	return xear_hid_protocol_send_data(featureId, value);
}
#endif



