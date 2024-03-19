/*
 * Copyright (c) 2021 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is the abstraction layer provided for Application layer for
 * Bluetooth Audio Stream/Media/Call/Volume/Microphone control.
 */

#include <bt_manager.h>
#include <media_type.h>
#include <mem_manager.h>
#include <app_manager.h>
#include <app_switch.h>
#include <timeline.h>

#include "ctrl_interface.h"
#include <drivers/bluetooth/bt_drv.h>
#include "bt_manager_inner.h"
#include <property_manager.h>
#include <sys_event.h>
#include <audio_policy.h>
#include <audio_system.h>

#include <xear_authentication.h>


#define BT_AUDIO_VOLUME_MAX_DEV_NUM (3)
#define CFG_BTMGR_AUDIO_VOLUME	"BTMGR_AUDIO_VOLUME"
#define BT_AUDIO_VND_SEND_ID_UAC_GAIN      (0x80)

/* slave app */
#define BT_AUDIO_APP_UNKNOWN	0
#define BT_AUDIO_APP_BR_CALL	1
#define BT_AUDIO_APP_BR_MUSIC	2
#define BT_AUDIO_APP_LE_AUDIO	3
#define BT_AUDIO_APP_USOUND	    4

static const char * const app_names[] = {
	"unknown", "btcall", "btmusic", "le_audio", "usound", ""
};

/* slave conn priority, larger means higer priority */
#define BT_AUDIO_PRIO_NONE	0
#define BT_AUDIO_PRIO_MEDIA_PAUSE (1 << 0) //media stream open, but avrcp pause
#define BT_AUDIO_PRIO_MEDIA	(1 << 1)
#define BT_AUDIO_PRIO_DONGLE_CALL (1 << 2)
#define BT_AUDIO_PRIO_CALL	(1 << 3)

/* maximum channels belongs to the same connection */
#define BT_AUDIO_CHAN_MAX	4

struct bt_audio_chan_restore_max {
	uint8_t count;
	struct bt_audio_chan chans[BT_AUDIO_CHAN_MAX];
};

struct bt_audio_kbps {
	uint16_t sample;
	uint16_t sdu;
	uint16_t interval;
	uint16_t kbps;
	uint8_t channels;
};

static const struct bt_audio_kbps bt_audio_kbps_table[] = {
	/*
	 * codec frame without 4-byte header
	 */
	/* For Music typical */
	/* 1ch, typical 48k */
	{ 48, 156, 10000, 124, 1 }, /*no used?*/
	{ 48, 120, 10000, 96, 1 },
	{ 48, 100, 10000, 80, 1 },
	{ 48, 98,  10000, 78, 1 },
	{ 48, 118, 7500,  125, 1 }, /*no used?*/
	{ 48, 90,  7500,  96, 1 },
	{ 48, 76,  7500,  80, 1 },
	{ 48, 74,  7500,  78, 1 },
	/* 2ch, kbps must be even, typical 48k */
	{ 48, 240, 10000, 192, 2 },
	{ 48, 230, 10000, 184, 2 },
	{ 48, 220, 10000, 176, 2 },
	{ 48, 200, 10000, 160, 2 },
	{ 48, 198, 10000, 158, 2 },
	{ 48, 196, 10000, 156, 2 }, /*no used?*/
	{ 48, 180, 7500, 192, 2 },
	{ 48, 172, 7500, 184, 2 },
	{ 48, 166, 7500, 176, 2 },
	{ 48, 150, 7500, 160, 2 },
	{ 48, 148, 7500, 158, 2 },
	{ 48, 118, 5000, 188, 2 }, /*no used?*/
	{ 48, 102, 5000, 164, 2 }, /*no used?*/
	{ 48, 98,  5000, 158, 2 }, /*no used?*/

	/* For Call typical */
	/* 1ch, typical 32k */
	{ 32, 80,  10000, 64, 1 },
	{ 32, 60,  7500, 64, 1 },
	{ 32, 40,  5000, 64, 1 },
	/* 1ch, typical 32k */
	{ 32, 160,  10000, 128, 2 }, /*no used?*/
	{ 32, 120,  7500, 128, 2 }, /*no used?*/
	{ 32, 80,  5000, 128, 2 }, /*no used?*/
	/* 1ch, typical 24k */
	{ 24, 60,  10000, 48, 1 },
	{ 24, 46,  7500, 48, 1 },
	{ 24, 30,  5000, 48, 1 },
	/* 2ch, typical 24k */
	{ 24, 120,  10000, 96, 2 }, /*no used?*/
	{ 24, 90,  7500, 96, 2 }, /*no used?*/
	{ 24, 60,  5000, 96, 2 }, /*no used?*/
	/* 1ch, typical 16k */
	{ 16, 40,  10000, 32, 1 },
	{ 16, 30,  7500, 32, 1 },
	{ 16, 20,  5000, 32, 1 },
	/* 2ch, typical 16k */
	{ 16, 80,  10000, 64, 2 }, /*no used?*/
	{ 16, 60,  7500, 64, 2 }, /*no used?*/
	{ 16, 40,  5000, 64, 2 }, /*no used?*/
};

struct bt_audio_channel {
	sys_snode_t node;

	/*
	 * handle and id are enough to identify a audio stream in most cases,
	 * audio_handle is used in rare cases only, for example, get bt_clk.
	 */
	uint16_t audio_handle;
	uint8_t id;
	uint8_t dir;

	uint8_t state;	/* Audio Stream state */
	uint8_t format;
	uint8_t channels;	/* range: [1, 8] */
	uint8_t duration;	/* frame_duration; unit: ms, 7 means 7.5ms */
	uint32_t locations;	/* AUDIO_LOCATIONS_ */
	uint16_t kbps;
	uint16_t octets;
	uint16_t sdu;	/* max_sdu; unit: byte */
	uint32_t interval;	/* sdu_interval; unit: us */
	uint16_t sample;	/* sample_rate; unit: kHz */
	uint16_t audio_contexts;
	uint32_t delay;	/* presentation_delay */
	uint16_t max_latency;	/* max_transport_latency */
	uint8_t rtn;	/* retransmission_number */
	uint8_t framing;

	uint8_t phy;

	/* transport params: for LE Audio only now */
	uint8_t nse;
	uint8_t m_bn;
	uint8_t s_bn;
	uint8_t m_ft;
	uint8_t s_ft;
	uint32_t cig_sync_delay;
	uint32_t cis_sync_delay;
	uint32_t rx_delay;
	uint16_t cis_audio_handle;
	
	bt_audio_trigger_cb trigger;
	bt_audio_trigger_cb period_trigger;
	uint64_t bt_time;
	uint64_t bt_count;
	timeline_t *tl;
	timeline_t *period_tl;
};

struct bt_audio_call_inst {
	sys_snode_t node;

	uint16_t handle;
	uint8_t index;
	uint8_t state;

	/*
	 * UTF-8 URI format: uri_scheme:caller_id such as tel:96110
	 *
	 * end with '\0' if valid, all-zeroed if invalid
	 */
	uint8_t remote_uri[BT_AUDIO_CALL_URI_LEN];
};

struct bt_audio_conn {
	uint16_t handle;
    bd_address_t  addr;

	uint8_t type;	/* BT_TYPE_BR/BT_TYPE_LE */
	uint8_t role;	/* BT_ROLE_MASTER/BT_ROLE_SLAVE */
    uint8_t need_app;	/* BT_AUDIO_APP NONE/BR_MUSIC/BR_CALL/LE_AUDIO */

	uint8_t prio;	/* BT_AUDIO_PRIO_CALL/BT_AUDIO_PRIO_MEDIA ... */

	uint16_t media_state;
	uint8_t volume;

	uint8_t is_tws:1;
	uint8_t is_lea:1;
	
	/* for BR Audio only */
	uint16_t call_state;

	/* fake audio_contexts, for BR Audio only */
	uint16_t fake_contexts;

	sys_slist_t chan_list;	/* list of struct bt_audio_channel */

	/* for Call Terminal, the call list is attached to a connection */
	sys_slist_t ct_list;	/* list of struct bt_audio_call_inst */

	/* for LE master */
	struct bt_audio_endpoints *endps;
	struct bt_audio_capabilities *caps;

	sys_snode_t node;
	
#ifdef CONFIG_BT_TRANSCEIVER
	uint8_t audio_connected:1;
    uint8_t tws_pos;

    struct bt_audio_qoss_1 *qoss;
#endif
};

struct bt_audio_runtime {
	uint8_t le_audio_state;

	uint8_t le_audio_init : 1;
	uint8_t le_audio_start : 1;
	uint8_t le_audio_pause : 1;

	uint8_t le_audio_source_prefered_qos : 1;
	uint8_t le_audio_sink_prefered_qos : 1;

	uint8_t le_audio_codec_policy : 1;
	uint8_t le_audio_qos_policy : 1;

	uint8_t local_active_app : 2; 
	uint8_t remote_active_app : 2;

	uint8_t temporary_dis_lea_flag:1;
	uint8_t entering_btcall_flag:1;

	uint8_t le_audio_pause_br_flag:1;
	
	uint8_t force_app_id;
	uint8_t le_audio_adv_enable;
	uint32_t le_audio_pause_br_timestamp;


	/* default preferred qos for all servers */
	struct bt_audio_preferred_qos_default source_qos;
	struct bt_audio_preferred_qos_default sink_qos;

	/* default codec policy for all clients */
	struct bt_audio_codec_policy codec_policy;
	/* default qos policy for all clients */
	struct bt_audio_qos_policy qos_policy;

	sys_slist_t conn_list;	/* list of struct bt_audio_conn */

	struct bt_audio_conn *active_slave;
	/* device which be interrupted by call  */	
	struct bt_audio_conn *interrupted_active_slave; 
	bd_address_t tws_slave_interrupted_dev;
	bd_address_t tws_slave_active_dev;
	
	bt_mgr_saved_dev_volume_t saved_volume[BT_AUDIO_VOLUME_MAX_DEV_NUM];

	uint8_t  recently_connected_flag; /*1-recently a new device connected flag*/
	uint32_t recently_connected_timestamp; /*recently a new device connected timestamp*/

	/*
	 * for Call Gateway, the call list is shared with all Call Terminals.
	 *
	 * NOTICE: support for LE Audio only for now.
	 */
	sys_slist_t cg_list;	/* list of struct bt_audio_call_inst */

#ifdef CONFIG_BT_TRANSCEIVER
    uint8_t num;
#endif
};

static struct bt_audio_runtime bt_audio;
static leaudio_device_callback_t bt_leaudio_device_cb = NULL;

#ifdef CONFIG_BT_TRANSCEIVER
#include "tr_bt_manager_audio.c"
#endif

void bt_manager_audio_avrcp_play_for_leaudio(uint16_t handle)
{
	if (bt_audio.le_audio_pause_br_flag ){
		bt_manager_a2dp_check_state();
		bt_manager_avrcp_play();
#ifdef CONFIG_LEAUDIO_ACTIVE_BR_A2DP_SUSPEND		
		btif_a2dp_stream_start(handle);
#endif			
	}
	bt_audio.le_audio_pause_br_flag = 0;

}

void bt_manager_audio_avrcp_pause_for_leaudio(uint16_t handle)
{
	if (bt_audio.force_app_id == BT_AUDIO_APP_LE_AUDIO)
	{
		if (!bt_audio.le_audio_pause_br_flag || (os_uptime_get_32() - bt_audio.le_audio_pause_br_timestamp > 2000))
		{
			bt_audio.le_audio_pause_br_flag = 1;
			bt_audio.le_audio_pause_br_timestamp = os_uptime_get_32();
#ifdef CONFIG_LEAUDIO_ACTIVE_BR_A2DP_SUSPEND
			btif_a2dp_stream_suspend(handle);
#endif				
			btif_avrcp_send_command_by_hdl(handle,BTSRV_AVRCP_CMD_PAUSE);
		}
	}
}




static void bt_manager_audio_init_dev_volume(void)
{
    bt_mgr_saved_dev_volume_t*  p = bt_audio.saved_volume;;

    property_get(CFG_BTMGR_AUDIO_VOLUME, (char *)p, sizeof(*p) * BT_AUDIO_VOLUME_MAX_DEV_NUM);
}


static void bt_manager_audio_save_dev_volume(bd_address_t *addr, u8_t volume)
{
    bt_mgr_saved_dev_volume_t*  p = bt_audio.saved_volume;
    bool  changed = false;
    int     i;

    for (i = 0; i < BT_AUDIO_VOLUME_MAX_DEV_NUM; i++)
    {
        if (memcmp(&p[i].addr, addr, sizeof(bd_address_t)) == 0)
        {
            if (p[i].bt_music_vol != volume)
            {
                p[i].bt_music_vol = volume;
                changed = true;
            }
            break;
        }
    }

    if (i >= BT_AUDIO_VOLUME_MAX_DEV_NUM)
    {
        for (i = 0; i < BT_AUDIO_VOLUME_MAX_DEV_NUM - 1; i++)
        {
            p[i] = p[i+1];
        }
        memcpy(&p[i].addr, addr, sizeof(bd_address_t));
        p[i].bt_music_vol = volume;        
        changed = true;
    }

    if (changed)
    {
		SYS_LOG_INF("%02X:%02X:%02X:%02X:%02X:%02X vol->%d ",
			addr->val[5] ,addr->val[4] ,addr->val[3] ,addr->val[2] ,addr->val[1] ,addr->val[0] ,volume);
    
		property_set(CFG_BTMGR_AUDIO_VOLUME, (char *)p, sizeof(*p) * BT_AUDIO_VOLUME_MAX_DEV_NUM);
    }
}


static bool bt_manager_audio_read_dev_volume(bd_address_t *addr, u8_t* volume)
{
    bt_mgr_saved_dev_volume_t*  p = bt_audio.saved_volume;
	btmgr_sync_ctrl_cfg_t *sync_ctrl_config = bt_manager_get_sync_ctrl_config();
    int  i;
    
    for (i = 0; i < BT_AUDIO_VOLUME_MAX_DEV_NUM; i++)
    {
        if (memcmp(&p[i].addr, addr, sizeof(bd_address_t)) == 0)
        {
            *volume = p[i].bt_music_vol;
			
			SYS_LOG_INF("%02X:%02X:%02X:%02X:%02X:%02X vol->%d ",
				addr->val[5] ,addr->val[4] ,addr->val[3] ,addr->val[2] ,addr->val[1] ,addr->val[0] ,*volume);
			
            return true;
        }
    }

    *volume = sync_ctrl_config->bt_music_default_volume;
    return false;
}



static inline bool is_bt_le_master(struct bt_audio_conn *audio_conn)
{
	if ((audio_conn->type == BT_TYPE_LE) &&
	    (audio_conn->role == BT_ROLE_MASTER)) {
		return true;
	}

	return false;
}

static uint8_t bt_le_master_count(void)
{
	struct bt_audio_conn *audio_conn;
	uint8_t count = 0;

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (is_bt_le_master(audio_conn)) {
			count++;
		}
	}

	os_sched_unlock();

	return count;
}

static uint8_t get_slave_app(struct bt_audio_conn *audio_conn)
{
	if (!audio_conn) {
		return BT_AUDIO_APP_UNKNOWN;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		if (audio_conn->prio & BT_AUDIO_PRIO_CALL) {
			return BT_AUDIO_APP_BR_CALL;
		} else {
			return BT_AUDIO_APP_BR_MUSIC;
		} 
	} else if (audio_conn->type == BT_TYPE_LE) {
		return BT_AUDIO_APP_LE_AUDIO;
	}

	return BT_AUDIO_APP_UNKNOWN;
}

/*
 * current app may be different from active_slave's app, such as
 * key switch and so on.
 */
static uint8_t get_current_slave_app(void)
{
	char *current_app;
	uint8_t old_app;

	current_app = app_manager_get_current_app();

	if (!current_app) {
		old_app = BT_AUDIO_APP_UNKNOWN;
	} else if (!strcmp(current_app, APP_ID_BTMUSIC)) {
		old_app = BT_AUDIO_APP_BR_MUSIC;
	} else if (!strcmp(current_app, APP_ID_BTCALL)) {
		old_app = BT_AUDIO_APP_BR_CALL;
	} else if (!strcmp(current_app, APP_ID_LE_AUDIO)) {
		old_app = BT_AUDIO_APP_LE_AUDIO;
    } else if (!strcmp(current_app, APP_ID_USOUND)) {
		old_app = BT_AUDIO_APP_USOUND;
	} else {
		old_app = BT_AUDIO_APP_UNKNOWN;
	}

	return old_app;
}


static struct bt_audio_conn *find_conn_by_prio(uint8_t type, uint8_t prio)
{
	struct bt_audio_conn *conn = NULL;
	struct bt_audio_conn *audio_conn;

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_SLAVE || audio_conn->is_tws) {
			continue;
		}

		if ((type == BT_TYPE_LE) && !audio_conn->is_lea){
			continue;
		}

		//SYS_LOG_INF("hdl 0x%x: prio %d",audio_conn->handle ,audio_conn->prio);

		if (!prio && (audio_conn->type == type)){
			conn = audio_conn;
			break;
		}else if ((audio_conn->type == type) && audio_conn->prio & prio){
			conn = audio_conn;
		}	
		
	}
	return conn;
}



static struct bt_audio_conn *find_conn_by_addr(bd_address_t* addr)
{
	struct bt_audio_conn *conn = NULL;
	struct bt_audio_conn *audio_conn;

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_SLAVE || audio_conn->is_tws) {
			continue;
		}

		SYS_LOG_INF("hdl 0x%x: prio %d",audio_conn->handle ,audio_conn->prio);

		if (!memcmp(&(audio_conn->addr), addr, sizeof(bd_address_t))){
			conn = audio_conn;
			break;
		}	
	}
	return conn;
}





/*
 * get the slave with highest priority
 *
 * call's priority > media' priority, FIFO if priority is same
 *
 * @param except_active if except for current active_slave, true
 * means not include, false means include.
 */
static struct bt_audio_conn *get_highest_slave(bool except_active)
{
	struct bt_audio_conn *highest_slave;
	struct bt_audio_conn *audio_conn;
	uint8_t prio;
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();
	
	if (except_active) {
		highest_slave = NULL;
		prio = BT_AUDIO_PRIO_NONE;
	} else {
		highest_slave = bt_audio.active_slave;
		prio = highest_slave->prio;
	}
	
	if (bt_audio.active_slave){
		SYS_LOG_INF("active_slave hdl 0x%x: prio %d",bt_audio.active_slave->handle ,prio);
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_SLAVE || audio_conn->is_tws) {
			continue;
		}

		SYS_LOG_INF("hdl 0x%x: prio %d",audio_conn->handle ,audio_conn->prio);
		if (!highest_slave){
			highest_slave = audio_conn;
			prio = audio_conn->prio;
			continue;
		}

		if (audio_conn->prio > prio) {
			highest_slave = audio_conn;
			prio = audio_conn->prio;
		}
#ifdef CONFIG_BT_LE_AUDIO		
		else if (audio_conn->prio == prio){
			if (btif_tws_get_dev_role() != BTSRV_TWS_SLAVE){
				continue;
			}

			if ( bt_audio.remote_active_app == BT_AUDIO_APP_LE_AUDIO){
				if (audio_conn->type == BT_TYPE_LE && audio_conn->prio != BT_AUDIO_PRIO_NONE){
					highest_slave = audio_conn;
					prio = audio_conn->prio;
				}
			}else if (bt_audio.remote_active_app == BT_AUDIO_APP_BR_MUSIC){
				if (highest_slave && highest_slave->type == BT_TYPE_LE && 
					audio_conn->type != BT_TYPE_LE && audio_conn->prio == BT_AUDIO_PRIO_MEDIA){
					highest_slave = audio_conn;
					prio = audio_conn->prio;
				}
			}
		}
#endif		
	}

	if (((prio & BT_AUDIO_PRIO_CALL) == 0) && bt_audio.interrupted_active_slave){

		if (leaudio_config->app_auto_switch && 
			(highest_slave->prio > bt_audio.interrupted_active_slave->prio))
		{
			goto Exit;
		}
		highest_slave = bt_audio.interrupted_active_slave;
		SYS_LOG_INF("resume hdl 0x%x: prio %d",highest_slave->handle ,highest_slave->prio);
	}

Exit:
	if (highest_slave){
		SYS_LOG_INF("highest_slave hdl 0x%x: prio %d",highest_slave->handle ,highest_slave->prio);
	}
	return highest_slave;
}

#ifdef CONFIG_BT_LE_AUDIO
static int temporary_disconn_leaudio(void)
{
	struct bt_audio_conn *leaudio_conn = find_conn_by_prio(BT_TYPE_LE, 0);
	if (leaudio_conn){
		bt_audio.temporary_dis_lea_flag = 1;
		bt_manager_ble_disconnect((bt_addr_t*)&leaudio_conn->addr);
	}
	return 0;
}
#endif

static int switch_slave_app(struct bt_audio_conn *audio_conn, uint8_t force_app)
{
	uint8_t old_app = get_current_slave_app();
	uint8_t new_app = get_slave_app(audio_conn);
	char *switch_app = NULL;
#ifdef CONFIG_BT_LE_AUDIO
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();
#endif
	SYS_LOG_INF("active hdl: 0x%x", audio_conn?audio_conn->handle:0);

	if (force_app != BT_AUDIO_APP_UNKNOWN){
		new_app = force_app;
	}

	switch (old_app) {
	case BT_AUDIO_APP_UNKNOWN:
		switch (new_app) {
		case BT_AUDIO_APP_UNKNOWN:
			/* do nothing */
			break;
		case BT_AUDIO_APP_BR_MUSIC:
			switch_app = APP_ID_BTMUSIC;
			break;
		case BT_AUDIO_APP_BR_CALL:
			switch_app = APP_ID_BTCALL;
			break;
		case BT_AUDIO_APP_LE_AUDIO:
			switch_app = APP_ID_LE_AUDIO;
			break;
        case BT_AUDIO_APP_USOUND:
			switch_app = APP_ID_USOUND;
			break;
		}
		break;
	case BT_AUDIO_APP_BR_MUSIC:
		switch (new_app) {
		case BT_AUDIO_APP_UNKNOWN:
			/* do nothing */
			break;
		case BT_AUDIO_APP_BR_MUSIC:
			/* do nothing */
			break;
		case BT_AUDIO_APP_BR_CALL:
			switch_app = APP_ID_BTCALL;
			break;
		case BT_AUDIO_APP_LE_AUDIO:
			switch_app = APP_ID_LE_AUDIO;
			break;
        case BT_AUDIO_APP_USOUND:
			switch_app = APP_ID_USOUND;
			break;
		}
		break;
	case BT_AUDIO_APP_BR_CALL:
		switch (new_app) {
		case BT_AUDIO_APP_UNKNOWN:
			/* do nothing */
			break;
		case BT_AUDIO_APP_BR_MUSIC:
			switch_app = APP_ID_BTMUSIC;
			break;
		case BT_AUDIO_APP_BR_CALL:
			/* do nothing */
			break;
		case BT_AUDIO_APP_LE_AUDIO:
			switch_app = APP_ID_LE_AUDIO;
			break;
        case BT_AUDIO_APP_USOUND:
			switch_app = APP_ID_USOUND;
			break;
		}
		break;
	case BT_AUDIO_APP_LE_AUDIO:
		switch (new_app) {
		case BT_AUDIO_APP_UNKNOWN:
			/* do nothing */
			break;
		case BT_AUDIO_APP_BR_MUSIC:
			switch_app = APP_ID_BTMUSIC;
			break;
		case BT_AUDIO_APP_BR_CALL:
			switch_app = APP_ID_BTCALL;
			break;
		case BT_AUDIO_APP_LE_AUDIO:
			//bt_manager_event_notify(BT_AUDIO_SWITCH, NULL, 0);
			break;
        case BT_AUDIO_APP_USOUND:
			switch_app = APP_ID_USOUND;
			break;
		}
		break;
    case BT_AUDIO_APP_USOUND:
		switch (new_app) {
		case BT_AUDIO_APP_UNKNOWN:
			/* do nothing */
			break;
		case BT_AUDIO_APP_BR_MUSIC:
			switch_app = APP_ID_BTMUSIC;
			break;
		case BT_AUDIO_APP_BR_CALL:
			switch_app = APP_ID_BTCALL;
			break;
		case BT_AUDIO_APP_LE_AUDIO:
			//bt_manager_event_notify(BT_AUDIO_SWITCH, NULL, 0);
			switch_app = APP_ID_LE_AUDIO;
			break;
        case BT_AUDIO_APP_USOUND:
			//switch_app = APP_ID_USOUND;
			break;
		}
		break;
	default:
		break;
	}

	SYS_LOG_INF("%s -> %s (%s)", app_names[old_app], app_names[new_app],
		switch_app);

#ifdef CONFIG_BT_LE_AUDIO
    if (btif_tws_get_dev_role() == BTSRV_TWS_SLAVE){
		if (bt_audio.remote_active_app != new_app){
			SYS_LOG_INF("remote %s, local %s", app_names[bt_audio.remote_active_app], 
				app_names[new_app]);
			return 0;
		}
	}

	if (bt_manager_is_poweroffing()){
		SYS_LOG_INF("Poweroffing");
		return 0;
	}

	if (!bt_manager_is_poweroffing()){
		if ((new_app == BT_AUDIO_APP_LE_AUDIO) || (switch_app == NULL && old_app == BT_AUDIO_APP_LE_AUDIO)){
			btif_set_leaudio_foreground_dev(true);
		}
	}

#ifdef CONFIG_BT_ADV_MANAGER
	//btmgr_adv_update_immediately();
#endif

	if((cfg_feature->sp_tws == 0) && (bt_manager_audio_in_btcall() || bt_manager_audio_in_btmusic() ||
		(new_app != BT_AUDIO_APP_LE_AUDIO) || (switch_app == NULL && old_app != BT_AUDIO_APP_LE_AUDIO))){

		if (!leaudio_config->app_auto_switch ||
			(leaudio_config->app_auto_switch && bt_manager_media_get_status() != BT_STATUS_PAUSED)){
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
			if((old_app == BT_AUDIO_APP_LE_AUDIO && new_app == BT_AUDIO_APP_BR_CALL)
				|| (old_app == BT_AUDIO_APP_BR_CALL && new_app == BT_AUDIO_APP_LE_AUDIO)
				|| (old_app == new_app)) {
				//do noting
			} else {
				temporary_disconn_leaudio();
			}
#else
			temporary_disconn_leaudio();
#endif
		}
	}
	
#endif
	bt_audio.local_active_app = new_app;
	bt_manager_tws_sync_active_dev(NULL);

	if (switch_app) {
		if (new_app == BT_AUDIO_APP_BR_CALL){
			bt_audio.entering_btcall_flag = 1;
		}
	
		app_switch(switch_app, APP_SWITCH_CURR, true);

		bt_audio.entering_btcall_flag = 0;		
	}

#ifdef CONFIG_BT_ADV_MANAGER
	btmgr_adv_update_immediately();
#endif

#ifdef CONFIG_BT_LE_AUDIO
	if (!bt_manager_is_poweroffing()){
		if ((switch_app != NULL && new_app != BT_AUDIO_APP_LE_AUDIO) || 
			(switch_app == NULL && old_app != BT_AUDIO_APP_LE_AUDIO)){
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
			if((old_app == BT_AUDIO_APP_LE_AUDIO && new_app == BT_AUDIO_APP_BR_CALL)
				|| (old_app == new_app)) {
				//do noting
			} else {
				btif_set_leaudio_foreground_dev(false); 		
			}
#else
			btif_set_leaudio_foreground_dev(false);			
#endif
		}
	}

	if ((new_app == BT_AUDIO_APP_LE_AUDIO) || 
		(switch_app == NULL && old_app == BT_AUDIO_APP_LE_AUDIO)){
		if (audio_conn && audio_conn->type == BT_TYPE_LE){
			SYS_LOG_INF("handle: 0x%x,volume: %d", audio_conn->handle,audio_conn->volume);
			bt_manager_volume_event_to_app(audio_conn->handle, audio_conn->volume, 0);
		}
	}	
#endif

	return 0;
}

static struct bt_audio_conn * check_conn_active(struct bt_audio_conn *conn)
{
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();

	if (leaudio_config->app_auto_switch){
		return conn;
	}else{	
		if((bt_audio.force_app_id == BT_AUDIO_APP_LE_AUDIO && conn->type == BT_TYPE_LE) ||
			(bt_audio.force_app_id == BT_AUDIO_APP_BR_MUSIC && conn->type == BT_TYPE_BR) ||
			(conn->prio & BT_AUDIO_PRIO_CALL)){
			return conn;
		}
	}
	return NULL;
}

/*
 * free: true if BT is disconnected and going to be freed, else false.
 *
 * case 1: add prio
 * case 2: remove prio
 * case 3: audio_conn disconnected
 */
static int update_active_slave(struct bt_audio_conn *audio_conn, bool free)
{
	struct bt_audio_conn *new_active_slave;
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();

	if (!audio_conn || audio_conn->role != BT_ROLE_SLAVE || audio_conn->is_tws) {
		return -EINVAL;
	}

	os_sched_lock();

	if (free) {
		if (audio_conn == bt_audio.interrupted_active_slave){
			bt_audio.interrupted_active_slave = NULL;
		}
		
		if (bt_audio.active_slave == audio_conn) {
			new_active_slave = get_highest_slave(true);
			bt_audio.active_slave = check_conn_active(new_active_slave);
			goto update;
		} else {
			goto exit;
		}
	}

	if (!bt_audio.active_slave) {
		bt_audio.active_slave = check_conn_active(audio_conn);
		goto update;
	}

	new_active_slave = get_highest_slave(false);

	if (bt_audio.active_slave == new_active_slave) {
		/*
		 * If active_slave not change, no need to switch for LE Audio,
		 * but may need to switch for BR Audio.
		 */
		if (new_active_slave->type == BT_TYPE_LE && 
			bt_audio.local_active_app == BT_AUDIO_APP_LE_AUDIO) {
			goto exit;
		}
	}

	if (bt_audio.active_slave != new_active_slave){	

		if (leaudio_config->app_auto_switch && btif_a2dp_stream_is_delay_stop(bt_audio.active_slave->handle)){
			if((new_active_slave->prio & (BT_AUDIO_PRIO_CALL)) == 0 ){
				if(bt_audio.interrupted_active_slave && bt_audio.interrupted_active_slave == new_active_slave){
					SYS_LOG_INF("hdl 0x%x is interrupted dev, need resume",new_active_slave->handle);	
				}else{
					SYS_LOG_INF("hdl 0x%x delay stopped",bt_audio.active_slave->handle);
					goto update;
				}
			}
		}

		if ((new_active_slave->prio & BT_AUDIO_PRIO_CALL) || 
			((new_active_slave->prio & BT_AUDIO_PRIO_DONGLE_CALL) && leaudio_config->app_auto_switch)){
			if (!bt_audio.interrupted_active_slave){
				SYS_LOG_INF("hdl 0x%x be interrupted",bt_audio.active_slave->handle);
				bt_audio.interrupted_active_slave = bt_audio.active_slave; 
			}
		}else{
			if (bt_audio.interrupted_active_slave == new_active_slave){
				SYS_LOG_INF("hdl 0x%x resume to be active dev",new_active_slave->handle);
				bt_audio.interrupted_active_slave = NULL;
			}else if (!leaudio_config->app_auto_switch && bt_audio.active_slave){
				if (new_active_slave->type == BT_TYPE_BR && bt_audio.active_slave->type == BT_TYPE_BR){
					SYS_LOG_INF("2BR 0x%x -> 0x%x", bt_audio.active_slave->handle,new_active_slave->handle);
				}else{
					if (!check_conn_active(new_active_slave)){
						SYS_LOG_INF("hdl 0x%x dont automatic switch to be active dev",new_active_slave->handle);
						goto update;				
					}
				}
			}
		}
	}

	bt_audio.active_slave = new_active_slave;

update:
	if ((bt_audio.active_slave != NULL) && (bt_audio.active_slave->prio & BT_AUDIO_PRIO_CALL)){
		switch_slave_app(bt_audio.active_slave, BT_AUDIO_APP_UNKNOWN);
	}else{	
		bt_audio.active_slave = check_conn_active(bt_audio.active_slave);
		switch_slave_app(bt_audio.active_slave, bt_audio.force_app_id);
	}

exit:
	os_sched_unlock();

	return 0;
}

/*
 * check if need to add or remove prio for the audio_conn (slave only)
 *
 * case 1: stream_enable
 * case 2: stream_stop
 * case 3: stream_update
 * case 4: stream_release
 */
static int update_slave_prio(struct bt_audio_conn *audio_conn, bool update_active_dev)
{
	struct bt_audio_channel *chan;
	uint8_t old_prio;
	uint8_t new_prio;

	/* do nothing for master */
	if (audio_conn->role == BT_ROLE_MASTER) {
		return 0;
	}

	os_sched_lock();

	new_prio = BT_AUDIO_PRIO_NONE;
	old_prio = audio_conn->prio;

	if (audio_conn->fake_contexts & BT_AUDIO_CONTEXT_CALL) {
		new_prio = BT_AUDIO_PRIO_CALL;
		SYS_LOG_INF("hdl:0x%x fake_contexts %d",audio_conn->handle, audio_conn->fake_contexts);
		goto done;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&audio_conn->chan_list, chan, node) {
		if (chan->audio_contexts & BT_AUDIO_CONTEXT_CALL) {
			new_prio = BT_AUDIO_PRIO_CALL;
			break;
		}

		if (chan->dir == BT_AUDIO_DIR_SOURCE && chan->audio_contexts == BT_AUDIO_CONTEXT_UNSPECIFIED) {
			new_prio = BT_AUDIO_PRIO_DONGLE_CALL;
			break;
		}

		if (chan->audio_contexts != 0) {
			if (audio_conn->media_state == BT_STATUS_PAUSED){
				new_prio = BT_AUDIO_PRIO_MEDIA_PAUSE;
			}else{		
				new_prio = BT_AUDIO_PRIO_MEDIA;
			}
			break;
		}
	}

done:
	os_sched_unlock();
	SYS_LOG_INF("hdl:0x%x prio %d -> %d",audio_conn->handle, old_prio ,new_prio);

	if (update_active_dev == false){
		return 0;
	}

	if (old_prio != new_prio) {
		audio_conn->prio = new_prio;
		return update_active_slave(audio_conn, false);
	}else if (new_prio == BT_AUDIO_PRIO_NONE && bt_audio.active_slave == audio_conn){
		SYS_LOG_INF("hdl:0x%x unactive, switch to others",audio_conn->handle);
		return update_active_slave(audio_conn, false);
	}

	return 0;
}

static struct bt_audio_conn *find_active_slave(void)
{
	if (bt_audio.active_slave){
		uint8_t cur_app = get_current_slave_app();
		if(cur_app == BT_AUDIO_APP_LE_AUDIO && bt_audio.active_slave->type != BT_TYPE_LE){
			return NULL;
		} else if ((cur_app == BT_AUDIO_APP_BR_MUSIC ||  cur_app == BT_AUDIO_APP_BR_CALL) && 
			bt_audio.active_slave->type != BT_TYPE_BR) {
			return NULL;
		}
	}
    else
    {
        SYS_LOG_INF("null\n");
    }

	return bt_audio.active_slave;
}
#ifdef CONFIG_BT_TRANSCEIVER
static struct bt_audio_conn *find_active_master(uint8_t type)
{
	struct bt_audio_conn *audio_conn;

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->type == type) {
			os_sched_unlock();
			return audio_conn;
		}
	}

	os_sched_unlock();
	return NULL;
}

static struct bt_audio_call_inst *find_call_by_status(struct bt_audio_conn *audio_conn,
				uint8_t state)
{
	struct bt_audio_call_inst *call;
	sys_slist_t *list;

	if (is_bt_le_master(audio_conn)) {
		list = &bt_audio.cg_list;
	} else if (audio_conn->role == BT_ROLE_SLAVE) {
		list = &audio_conn->ct_list;
	} else {
		return NULL;
	}

	os_sched_lock();
	SYS_SLIST_FOR_EACH_CONTAINER(list, call, node) {
		if (call->state == state) {
			os_sched_unlock();
			return call;
		}
	}

	os_sched_unlock();

	return NULL;
}

static struct bt_audio_call_inst *find_fisrt_call(struct bt_audio_conn *audio_conn)
{
	struct bt_audio_call_inst *call;
	sys_slist_t *list;

	if (is_bt_le_master(audio_conn)) {
		list = &bt_audio.cg_list;
	} else if (audio_conn->role == BT_ROLE_SLAVE) {
		list = &audio_conn->ct_list;
	} else {
		return NULL;
	}

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(list, call, node) {
		os_sched_unlock();
		return call;
	}

	os_sched_unlock();

	return NULL;
}

#endif

#ifdef CONFIG_BT_LE_AUDIO
/* find first slave call by state */
static struct bt_audio_call_inst *find_slave_call(struct bt_audio_conn *audio_conn,
				uint8_t state)
{
	struct bt_audio_call_inst *call;

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&audio_conn->ct_list, call, node) {
		if (call->state == state) {
			os_sched_unlock();
			return call;
		}
	}

	os_sched_unlock();

	return NULL;
}
				
static struct bt_audio_call_inst *find_fisrt_slave_call(struct bt_audio_conn *audio_conn)
{
	struct bt_audio_call_inst *call;

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&audio_conn->ct_list, call, node) {
		os_sched_unlock();
		return call;
	}

	os_sched_unlock();

	return NULL;
}
#endif

static uint16_t get_kbps(uint16_t sample, uint16_t sdu, uint16_t interval, uint8_t channels)
{
	const struct bt_audio_kbps *kbps = bt_audio_kbps_table;
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(bt_audio_kbps_table); i++) {
		if ((kbps->sample == sample) &&
			(kbps->sdu == sdu) &&
			(kbps->interval == interval) &&
			(kbps->channels == channels)) {
			return kbps->kbps;
		}
		kbps++;
	}

	return 0;
}

static struct bt_audio_conn *find_conn(uint16_t handle)
{
	struct bt_audio_conn *audio_conn;

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->handle == handle) {
			os_sched_unlock();
			return audio_conn;
		}
	}

	os_sched_unlock();

	return NULL;
}

#define CONNECTED_DELAY_PLAY_TIMER_MS 1000
#ifdef CONFIG_BT_LE_AUDIO
static void connected_delay_play_timer_handler(struct k_work* work)
{
	bt_audio.recently_connected_flag = 0;
	bt_audio.recently_connected_timestamp = 0;
}

static int le_audio_connected(uint16_t handle)
{
	struct bt_audio_conn *audio_conn;
	struct bt_manager_context_t*  bt_manager = bt_manager_get_context();
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();
	bt_addr_le_t *peer;
	int ret;

	audio_conn = find_conn(handle);
	if (audio_conn->is_lea){
		return 0;
	}

#ifdef CONFIG_BT_TRANSCEIVER
	if (sys_get_work_mode() && (audio_conn->type == BT_TYPE_LE) && (audio_conn->role == BT_ROLE_MASTER)) {
        return 0;
    }
#endif

	audio_conn->is_lea = 1;

	if((cfg_feature->sp_tws == 0) && (bt_manager_audio_in_btcall() || bt_manager_audio_in_btmusic())){

		if (!leaudio_config->app_auto_switch ||
			(leaudio_config->app_auto_switch && bt_manager_media_get_status() != BT_STATUS_PAUSED)){
			temporary_disconn_leaudio();
			SYS_LOG_INF("in btcall or btmusic, unconnected Lea");
		}	
		return 0;
	}

	if (bt_audio.temporary_dis_lea_flag == 0)
	{
		bt_audio.recently_connected_timestamp = os_uptime_get_32();
		bt_audio.recently_connected_flag = 1;
		os_delayed_work_init(&bt_manager->connected_delay_play_timer, connected_delay_play_timer_handler);
		os_delayed_work_submit(&bt_manager->connected_delay_play_timer, CONNECTED_DELAY_PLAY_TIMER_MS);

		/*if BR connected, then leaudio is 2nd device*/
		if (!bt_manager_get_connected_dev_num()){
			bt_manager_sys_event_notify(SYS_EVENT_BT_CONNECTED);
		}else {
			bt_manager_sys_event_notify(SYS_EVENT_2ND_CONNECTED);	
		}
	}

	bt_audio.temporary_dis_lea_flag = 0;

	if (get_current_slave_app() == BT_AUDIO_APP_LE_AUDIO){
		btif_set_leaudio_foreground_dev(true);
	}else{
		btif_set_leaudio_foreground_dev(false);
	}

	btif_set_leaudio_connected(1);
	
	if (bt_leaudio_device_cb){
		bt_leaudio_device_cb(audio_conn->handle, 1);
	}
	peer = btif_get_le_addr_by_handle(handle);
	ret = bt_manager_audio_save_peer_addr(peer);
	if ((ret == 0) && (btif_tws_get_dev_role() == BTSRV_TWS_MASTER)){
		bt_manager_tws_sync_lea_info(NULL); 
		bt_manager_sync_volume_from_phone_leaudio(&(audio_conn->addr), audio_conn->volume);
	}
	return 0;
}


static int le_audio_vnd_rx_cb(uint16_t handle, const uint8_t *buf, uint16_t len)
{
	struct bt_audio_conn *audio_conn;

	if (!buf || !len) {
		return 0;
	}

	SYS_LOG_INF("buf: %p, len: %d", buf, len);
	audio_conn = find_conn(handle);
	if (audio_conn && !memcmp(buf, "Dongle", len)) {
		SYS_LOG_INF("connected to Dongle");
		le_audio_connected(handle);
	}

    /* Set uac pcm gain */
    if (len == 2)
    {
        if (buf[0] == BT_AUDIO_VND_SEND_ID_UAC_GAIN)
        {
            struct app_msg  msg = {0};

	        msg.type    = MSG_BT_EVENT;
	        msg.cmd     = BT_VOLUME_UAC_GAIN;
            msg.reserve = buf[1];

	        return send_async_msg("tr_usound", &msg);
        }
    }

	return 0;
}


#endif

static int new_audio_conn(uint16_t handle)
{
	struct bt_audio_conn *audio_conn;
	int type = 0;
	int role;

	audio_conn = find_conn(handle);
	if (audio_conn){
		SYS_LOG_ERR("0x%x alreay exist.", handle);
		return 0;
	}

	type = btif_get_type_by_handle(handle);
	if (type == BT_TYPE_SCO || type ==BT_TYPE_BR_SNOOP || type == BT_TYPE_SCO_SNOOP) {
		type = BT_TYPE_BR;
	}

	if (type == BT_TYPE_BR) {
		role = BT_ROLE_SLAVE;
	} else if (type == BT_TYPE_LE) {
		role = btif_get_conn_role(handle);
	} else {
		SYS_LOG_ERR("type: %d", type);
		return -EINVAL;
	}

	audio_conn = mem_malloc(sizeof(struct bt_audio_conn));
	if (!audio_conn) {
		SYS_LOG_ERR("no mem");
		return -ENOMEM;
	}
	memset(audio_conn, 0, sizeof(struct bt_audio_conn));
	audio_conn->handle = handle;
	audio_conn->type = type;
	audio_conn->role = role;
    audio_conn->media_state = BT_STATUS_PAUSED;
	audio_conn->call_state = BT_STATUS_HFP_NONE;
	memcpy(&(audio_conn->addr),btif_get_addr_by_handle(handle), sizeof(bd_address_t));

	if (type == BT_TYPE_LE){
		if (!bt_manager_audio_read_dev_volume(&(audio_conn->addr), &(audio_conn->volume))){
			audio_conn->volume =  audio_system_get_stream_volume(AUDIO_STREAM_LE_AUDIO_MUSIC);
		}

#ifdef CONFIG_LE_AUDIO_SR_BQB
#else
		uint8_t vcs_volume = (audio_conn->volume >=16)? (255): (audio_conn->volume*16);
	 	bt_manager_le_volume_set(audio_conn->handle, vcs_volume);
#endif
	}

	os_sched_lock();
	sys_slist_append(&bt_audio.conn_list, &audio_conn->node);
#ifdef CONFIG_BT_TRANSCEIVER
    bt_audio.num++;
#endif
	os_sched_unlock();

	SYS_LOG_INF("%p, handle: 0x%x, type: %d, role: %d", audio_conn,
		handle, type, role);

#ifdef CONFIG_BT_LE_AUDIO		
	if ((type == BT_TYPE_LE) && (role == BT_ROLE_SLAVE)) {
		bt_manager_audio_le_vnd_register_rx_cb(handle, le_audio_vnd_rx_cb);
		
		bt_addr_le_t tmp_peer;
		if(bt_manager_audio_get_peer_addr(&tmp_peer) == 0){
			bt_addr_le_t* peer = btif_get_le_addr_by_handle(handle);			
			if (peer && memcmp((char*)peer, (char*)&tmp_peer, sizeof(bt_addr_le_t)) == 0){
				SYS_LOG_INF("peer addr is lea dev");
				le_audio_connected(audio_conn->handle);		
			}
		}
	}
#endif

#ifdef CONFIG_BT_TRANSCEIVER
	if (sys_get_work_mode() && (type == BT_TYPE_LE) && (role == BT_ROLE_MASTER)) {
        bt_addr_le_t *peer = btif_get_le_addr_by_handle(handle);
        bt_manager_audio_le_vnd_register_rx_cb(handle, le_audio_vnd_rx_cb);
        tr_le_audio_update_link_list(peer);
        return bt_manager_event_notify(BT_CONNECTED, &handle, sizeof(handle));
    }
#endif

	if (btif_tws_get_dev_role() == BTSRV_TWS_SLAVE){
		/*TWS Master sync interrupted active addr to TWS slave,but slave is not yet connect dev*/
		if (!memcmp(&bt_audio.tws_slave_interrupted_dev, &audio_conn->addr, sizeof(bd_address_t))){
			bt_audio.interrupted_active_slave = find_conn_by_addr(&bt_audio.tws_slave_interrupted_dev);
			memset(&bt_audio.tws_slave_interrupted_dev, 0,sizeof(bd_address_t));			
		}
		/*TWS Master sync active addr to TWS slave,but slave is not yet connect dev*/
		if (!memcmp(&bt_audio.tws_slave_active_dev, &audio_conn->addr, sizeof(bd_address_t))){
			bt_audio.active_slave = audio_conn;
			memset(&bt_audio.tws_slave_active_dev, 0,sizeof(bd_address_t));
		}
	}

	/* filter non active_slave */
	if ((audio_conn->role == BT_ROLE_SLAVE) && find_active_slave()) {
		return 0;
	}

	return bt_manager_event_notify(BT_CONNECTED, &handle, sizeof(handle));
}

static int delete_audio_conn(uint16_t handle, uint8_t reason)
{
	struct bt_audio_conn *audio_conn;
	struct bt_audio_channel *chan;
	struct bt_audio_call_inst *call;
	struct bt_audio_call_inst *tmp;
	uint8_t report = 1;

	audio_conn = find_conn(handle);
	if (!audio_conn) {
		SYS_LOG_ERR("not found");
		return -ENODEV;
	}

	os_sched_lock();

	if (audio_conn->is_lea){
		if (reason != 0x16){
			bt_manager_sys_event_notify(SYS_EVENT_BT_DISCONNECTED);
			/*if BR no connected, then send SYS_EVENT_BT_UNLINKED*/
			if (!bt_manager_get_connected_dev_num()) {
				bt_manager_sys_event_notify(SYS_EVENT_BT_UNLINKED);
			}
		}
		btif_set_leaudio_connected(0);
		bt_manager_audio_save_dev_volume(&(audio_conn->addr), audio_conn->volume);
	}

	/* remove all channel */
	SYS_SLIST_FOR_EACH_CONTAINER(&audio_conn->chan_list, chan, node) {
		if (chan->tl) {
			timeline_release(chan->tl);
		}
		if (chan->period_tl) {
			timeline_release(chan->period_tl);
		}
		sys_slist_remove(&audio_conn->chan_list, NULL, &chan->node);
		mem_free(chan);
	}

	if (audio_conn->caps) {
		mem_free(audio_conn->caps);
	}

	if (audio_conn->endps) {
		mem_free(audio_conn->endps);
	}

	/* NOTICE: need to free media/vol/mic if any */

	/* call list */
	if (is_bt_le_master(audio_conn)) {
		if (bt_le_master_count() == 0) {
			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bt_audio.cg_list, call,
							tmp, node) {
				sys_slist_find_and_remove(&bt_audio.cg_list, &call->node);
				mem_free(call);
			}
		}
	} else if (audio_conn->role == BT_ROLE_SLAVE) {
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&audio_conn->ct_list, call,
						tmp, node) {
			sys_slist_find_and_remove(&audio_conn->ct_list, &call->node);
			mem_free(call);
		}
	}

	sys_slist_find_and_remove(&bt_audio.conn_list, &audio_conn->node);

	if (audio_conn->role == BT_ROLE_SLAVE) {
		/* filter non active_slave */
		if (find_active_slave() && (audio_conn != find_active_slave())) {
			report = 0;
		}

		/* NOTICE: should call until audio_conn has been removed from list */
		update_active_slave(audio_conn, true);
	}

	SYS_LOG_INF("%p, handle: 0x%x, reason:0x%x", audio_conn, handle, reason);
	mem_free(audio_conn);
#ifdef CONFIG_BT_TRANSCEIVER
    bt_audio.num--;
#endif
	os_sched_unlock();

	/* no need to report */
	if (report == 0) {
		return 0;
	}

	return bt_manager_event_notify(BT_DISCONNECTED, &handle, sizeof(handle));
}

static int tws_audio_conn(uint16_t handle)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_conn(handle);
	if (!audio_conn) {
		SYS_LOG_ERR("not found");
		return -ENODEV;
	}

	os_sched_lock();
	audio_conn->is_tws = 1;
	os_sched_unlock();

	SYS_LOG_INF("%p, handle: %d", audio_conn, handle);

	return 0;
}



void bt_manager_audio_conn_event(int event, void *data, int size)
{
	uint16_t handle;
	struct bt_audio_conn *audio_conn;
#ifdef CONFIG_BT_TRANSCEIVER
    struct bt_audio_conn *br_conn;
#endif

	SYS_LOG_INF("%d", event);

	switch (event) {
	case BT_CONNECTED:
		handle = *(uint16_t *)data;
		new_audio_conn(handle);
#ifdef CONFIG_BT_TRANSCEIVER
		audio_conn = find_conn(handle);
		if (audio_conn && audio_conn->type == BT_TYPE_BR)
        {
            br_conn = find_active_master(BT_TYPE_LE);
            if(br_conn && tr_bt_manager_le_audio_get_cis_num()) {
                tr_hostif_bt_set_link_policy(handle, 0);
            }
        }
#endif
		break;
	case BT_DISCONNECTED:{
		struct bt_disconn_report* rep = (struct bt_disconn_report*) data;		
		delete_audio_conn(rep->handle, rep->reason);
	}
		break;
	case BT_TWS_CONNECTION_EVENT:
		handle = *(uint16_t *)data;
		tws_audio_conn(handle);
		break;
	case BT_AUDIO_CONNECTED:{	
		struct bt_audio_report* rep =  (struct bt_audio_report*) data;
		audio_conn = find_conn(rep->handle);
#ifdef CONFIG_BT_TRANSCEIVER
		if (audio_conn && audio_conn->type == BT_TYPE_LE)
        {
            br_conn = find_active_master(BT_TYPE_BR);
            if(br_conn && tr_bt_manager_le_audio_get_cis_num() == 0) {
                tr_hostif_bt_set_link_policy(br_conn->handle, 0);
            }
            audio_conn->audio_connected = 1;
        }
#endif
		if (audio_conn && audio_conn->type == BT_TYPE_BR){
			update_active_slave(audio_conn, false);
		}
	
		bt_manager_event_notify(BT_AUDIO_CONNECTED, data, size);
	}
		break;
	case BT_AUDIO_DISCONNECTED: {
#ifdef CONFIG_BT_TRANSCEIVER
        struct bt_audio_report* rep =  (struct bt_audio_report*) data;
        audio_conn = find_conn(rep->handle);
		if (audio_conn && audio_conn->type == BT_TYPE_LE)
        {
            audio_conn->audio_connected = 0;
            br_conn = find_active_master(BT_TYPE_BR);
            if(br_conn && tr_bt_manager_le_audio_get_cis_num() == 0) {
                tr_hostif_bt_set_link_policy(br_conn->handle, 1);
            }
        }
#endif
		bt_manager_event_notify(BT_AUDIO_DISCONNECTED, data, size);
    }
		break;
	default:
		break;
	}
#ifdef CONFIG_BT_TRANSCEIVER
    if(sys_get_work_mode())
        tr_bt_manager_audio_conn_event(event, data, size);
#endif
}

uint8_t bt_manager_audio_get_type(uint16_t handle)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_conn(handle);
	if (!audio_conn) {
		SYS_LOG_ERR("not found");
		return 0;
	}

	return audio_conn->type;
}

int bt_manager_audio_get_role(uint16_t handle)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_conn(handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	return audio_conn->role;
}

bool bt_manager_audio_actived(void)
{
	if (sys_slist_is_empty(&bt_audio.conn_list)) {
		return false;
	}

	return true;
}

uint8_t bt_manager_audio_get_active_app(uint8_t* active_app, uint8_t* force_active_app,bd_address_t* active_dev_addr,bd_address_t* interrupt_dev_addr)
{
	*active_app = bt_audio.local_active_app;
	*force_active_app = bt_audio.force_app_id;
	

	if (bt_audio.active_slave){
		memcpy(active_dev_addr, &(bt_audio.active_slave->addr), sizeof(bd_address_t));
	}

	if (bt_audio.interrupted_active_slave){
		memcpy(interrupt_dev_addr, &(bt_audio.interrupted_active_slave->addr), sizeof(bd_address_t));
	}

	return 0;
}

uint8_t bt_manager_audio_sync_remote_active_app(uint8_t active_app, uint8_t force_active_app, bd_address_t* active_dev_addr, bd_address_t* interrupt_dev_addr)
{
	struct bt_audio_conn *new_active_slave = NULL;
	bd_address_t tmp_addr;

	bt_audio.remote_active_app = active_app;
	bt_audio.force_app_id = force_active_app;

	if (active_dev_addr){
		new_active_slave = find_conn_by_addr(active_dev_addr);
		//if (!new_active_slave){
		//	new_active_slave = bt_audio.active_slave;
		//}
		memcpy(&(bt_audio.tws_slave_active_dev), active_dev_addr, sizeof(bd_address_t));		
	}

	if (interrupt_dev_addr){
		memset(&tmp_addr,0,sizeof(bd_address_t));
		if(!memcmp(interrupt_dev_addr, &tmp_addr, sizeof(bd_address_t))){
			bt_audio.interrupted_active_slave = NULL;
		}else{
			bt_audio.interrupted_active_slave = find_conn_by_addr(interrupt_dev_addr);
		}
		memcpy(&(bt_audio.tws_slave_interrupted_dev), interrupt_dev_addr, sizeof(bd_address_t));
		
		SYS_LOG_INF("interrupted dev: 0x%x",(bt_audio.interrupted_active_slave? bt_audio.interrupted_active_slave->handle:0));
	}

	SYS_LOG_INF("%d, 0x%x",bt_audio.remote_active_app, new_active_slave?new_active_slave->handle:0);

	bt_audio.active_slave = new_active_slave;
	switch_slave_app(bt_audio.active_slave, bt_audio.remote_active_app);

	return 0;
}



static struct bt_audio_channel *find_channel(uint16_t handle, uint8_t id)
{
	struct bt_audio_conn *audio_conn;
	struct bt_audio_channel *chan;

	audio_conn = find_conn(handle);
	if (!audio_conn) {
		return NULL;
	}

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&audio_conn->chan_list, chan, node) {
		if (chan->id == id) {
			os_sched_unlock();
			return chan;
		}
	}

	os_sched_unlock();

	return NULL;
}

static struct bt_audio_channel *find_channel2(struct bt_audio_conn *audio_conn,
				uint8_t id)
{
	struct bt_audio_channel *chan;

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&audio_conn->chan_list, chan, node) {
		if (chan->id == id) {
			os_sched_unlock();
			return chan;
		}
	}

	os_sched_unlock();

	return NULL;
}

int bt_manager_audio_stream_dir(uint16_t handle, uint8_t id)
{
	struct bt_audio_channel *chan;

	chan = find_channel(handle, id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	SYS_LOG_INF("%d", chan->dir);

	return chan->dir;
}

int bt_manager_audio_stream_state(uint16_t handle, uint8_t id)
{
	struct bt_audio_channel *chan;

	chan = find_channel(handle, id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	SYS_LOG_INF("%d", chan->state);

	return chan->state;
}

int bt_manager_audio_stream_info(struct bt_audio_chan *chan,
				struct bt_audio_chan_info *info)
{
	struct bt_audio_channel *channel;

	if (!info) {
		return -EINVAL;
	}

	channel = find_channel(chan->handle, chan->id);
	if (!channel) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	os_sched_lock();

	info->format = channel->format;
	info->channels = channel->channels;
	info->octets = channel->octets;
	info->sample = channel->sample;
	info->interval = channel->interval;
	info->kbps = channel->kbps;
	info->sdu = channel->sdu;
	info->locations = channel->locations;
	info->contexts = channel->audio_contexts;
	info->delay = channel->delay;
	info->cig_sync_delay = channel->cig_sync_delay;
	info->cis_sync_delay = channel->cis_sync_delay;
	info->rx_delay = channel->rx_delay;
	info->m_bn = channel->m_bn;
	info->m_ft = channel->m_ft;
	info->audio_handle = channel->cis_audio_handle;
	os_sched_unlock();

	return 0;
}

struct bt_audio_endpoints *bt_manager_audio_client_get_ep(uint16_t handle)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_conn(handle);
	if (!audio_conn) {
		return NULL;
	}

	if ((audio_conn->type != BT_TYPE_LE) ||
	    (audio_conn->role != BT_ROLE_MASTER)) {
		SYS_LOG_ERR("invalid %d %d", audio_conn->role,
			audio_conn->type);
		return NULL;
	}

	return audio_conn->endps;
}

struct bt_audio_capabilities *bt_manager_audio_client_get_cap(uint16_t handle)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_conn(handle);
	if (!audio_conn) {
		return NULL;
	}

	if ((audio_conn->type != BT_TYPE_LE) ||
		(audio_conn->role != BT_ROLE_MASTER)) {
		SYS_LOG_ERR("invalid %d %d %d", handle, audio_conn->role,
			audio_conn->type);
		return NULL;
	}

	return audio_conn->caps;
}
#ifdef CONFIG_BT_LE_AUDIO
static int stream_prefer_qos_default(uint16_t handle, uint8_t id, uint8_t dir)
{
	struct bt_audio_preferred_qos_default *default_qos;
	struct bt_audio_preferred_qos qos;

	if (dir == BT_AUDIO_DIR_SOURCE) {
		if (!bt_audio.le_audio_source_prefered_qos) {
			return 0;
		}
		default_qos = &bt_audio.source_qos;
	} else if (dir == BT_AUDIO_DIR_SINK) {
		if (!bt_audio.le_audio_sink_prefered_qos) {
			return 0;
		}
		default_qos = &bt_audio.sink_qos;
	} else {
		return 0;
	}

	qos.handle = handle;
	qos.id = id;
	qos.framing = default_qos->framing;
	qos.phy = default_qos->phy;
	qos.rtn = default_qos->rtn;
	qos.latency = default_qos->latency;
	qos.delay_min = default_qos->delay_min;
	qos.delay_max = default_qos->delay_max;
	qos.preferred_delay_min = default_qos->preferred_delay_min;
	qos.preferred_delay_max = default_qos->preferred_delay_max;

	return bt_manager_le_audio_stream_prefer_qos(&qos);
}
#endif
static struct bt_audio_channel *new_channel(struct bt_audio_conn *audio_conn)
{
	struct bt_audio_channel *chan;

	chan = mem_malloc(sizeof(struct bt_audio_channel));
	if (!chan) {
		SYS_LOG_ERR("no mem");
		return NULL;
	}

	os_sched_lock();
	sys_slist_append(&audio_conn->chan_list, &chan->node);
	os_sched_unlock();

	return chan;
}

static int stream_config_codec(void *data, int size)
{
	struct bt_audio_conn *audio_conn;
	struct bt_audio_channel *chan;
	struct bt_audio_codec *codec;

	codec = (struct bt_audio_codec *)data;

	audio_conn = find_conn(codec->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	chan = find_channel2(audio_conn, codec->id);
	if (chan) {	/* update codec */
		SYS_LOG_DBG("update");
	} else {	/* new codec */
		chan = new_channel(audio_conn);
		if (!chan) {
			SYS_LOG_ERR("no mem");
			return -ENOMEM;
		}
	}

	chan->id = codec->id;
	chan->dir = codec->dir;
	chan->format = codec->format;
#ifdef 	CONFIG_BT_AUDIO
	chan->channels = bt_channels_by_locations(codec->locations);
	/* NOTICE: some masters may not set locations when config_codec */
	if (chan->channels == 0) {
		chan->channels = 1;
	}
	chan->duration = bt_frame_duration_to_ms(codec->frame_duration);
#endif
	/* sdu & interval: for master used to config qos */
	chan->sdu = chan->channels * codec->blocks * codec->octets;
	if (chan->duration == 7) {
		chan->interval = 7500;
	} else {
		chan->interval = chan->duration * 1000;
	}
	chan->interval *= codec->blocks;

	chan->sample = codec->sample_rate;
	chan->octets = codec->octets;
	chan->locations = codec->locations;

	SYS_LOG_INF("config_codec id %d dir %d format %d channels %d duration %d",
			chan->id, chan->dir, chan->format, chan->channels, chan->duration);
	SYS_LOG_INF("config_codec sdu %d interval %d sample %d octets %d locations %d",
			chan->sdu, chan->interval, chan->sample, chan->octets, chan->locations);
	SYS_LOG_INF("target_latency %d", codec->target_latency);

	/* if initiated by master, no need to report message */
	if (audio_conn->role == BT_ROLE_MASTER) {
		return 0;
	}

	chan->state = BT_AUDIO_STREAM_CODEC_CONFIGURED;
#ifdef CONFIG_BT_LE_AUDIO
	/* LE Slave */
	if (audio_conn->type == BT_TYPE_LE) {
		stream_prefer_qos_default(codec->handle, codec->id,
					codec->dir);
	}
#endif
	return bt_manager_event_notify(BT_AUDIO_STREAM_CONFIG_CODEC, data, size);
}

static int stream_prefer_qos(void *data, int size)
{
	struct bt_audio_preferred_qos *qos;
	struct bt_audio_channel *chan;

	qos = (struct bt_audio_preferred_qos *)data;

	chan = find_channel(qos->handle, qos->id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	chan->state = BT_AUDIO_STREAM_CODEC_CONFIGURED;

	return bt_manager_event_notify(BT_AUDIO_STREAM_PREFER_QOS, data, size);
}

static int stream_config_qos(void *data, int size)
{
	struct bt_audio_qos_configured *qos;
	struct bt_audio_channel *chan;

	qos = (struct bt_audio_qos_configured *)data;

	chan = find_channel(qos->handle, qos->id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	chan->framing = qos->framing;
	chan->interval = qos->interval;
	chan->delay = qos->delay;
	chan->max_latency= qos->latency;
	chan->sdu = qos->max_sdu;
	chan->phy = qos->phy;
	chan->rtn = qos->rtn;
	chan->kbps = get_kbps(chan->sample, chan->sdu, chan->interval, chan->channels);

	SYS_LOG_INF("config_qos framing %d interval %d delay %d max_latency %d sdu %d phy %d rtn %d kbps %d",
			chan->framing, chan->interval, chan->delay, chan->max_latency, chan->sdu,
			qos->phy, chan->rtn, chan->kbps);

	chan->state = BT_AUDIO_STREAM_QOS_CONFIGURED;

	return bt_manager_event_notify(BT_AUDIO_STREAM_CONFIG_QOS, data, size);
}

static int stream_fake(void *data, int size)
{
	struct bt_audio_report *rep = (struct bt_audio_report *)data;
	struct bt_audio_conn *audio_conn;

	audio_conn = find_conn(rep->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	audio_conn->fake_contexts = rep->audio_contexts;

	SYS_LOG_INF("contexts: 0x%x", audio_conn->fake_contexts);

	update_slave_prio(audio_conn,true);

	return 0;
}

static int stream_enable(void *data, int size)
{
	struct bt_audio_report *rep = (struct bt_audio_report *)data;
	struct bt_audio_conn *audio_conn;
	struct bt_audio_channel *chan;
	
	audio_conn = find_conn(rep->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	chan = find_channel2(audio_conn, rep->id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}
#ifdef CONFIG_BT_LE_AUDIO		
	if (audio_conn->type == BT_TYPE_LE)
	{
		le_audio_connected(rep->handle);
	}
#endif
	chan->audio_contexts = rep->audio_contexts;
	chan->state = BT_AUDIO_STREAM_ENABLED;

	if (chan->dir == BT_AUDIO_DIR_SINK) {
		audio_conn->media_state = BT_STATUS_PLAYING;
	}
	
	SYS_LOG_INF("dir:%d contexts: 0x%x", chan->dir, chan->audio_contexts);

	update_slave_prio(audio_conn,true);

	return bt_manager_event_notify(BT_AUDIO_STREAM_ENABLE, data, size);
}

static int stream_update(void *data, int size)
{
	struct bt_audio_report *rep = (struct bt_audio_report *)data;
	struct bt_audio_conn *audio_conn;
	struct bt_audio_channel *chan;

	audio_conn = find_conn(rep->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	chan = find_channel2(audio_conn, rep->id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	chan->audio_contexts = rep->audio_contexts;

	SYS_LOG_INF("contexts: 0x%x", chan->audio_contexts);

	update_slave_prio(audio_conn,true);

	return bt_manager_event_notify(BT_AUDIO_STREAM_UPDATE, data, size);
}

static int stream_disable(void *data, int size)
{
	struct bt_audio_report *rep = (struct bt_audio_report *)data;
	struct bt_audio_channel *chan;

	chan = find_channel(rep->handle, rep->id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	chan->state = BT_AUDIO_STREAM_DISABLED;

	/* NOTICE: reset audio_contexts until stream_stop */

	return bt_manager_event_notify(BT_AUDIO_STREAM_DISABLE, data, size);
}

static int stream_release(void *data, int size)
{
	struct bt_audio_report *rep = (struct bt_audio_report *)data;
	struct bt_audio_conn *audio_conn;
	struct bt_audio_channel *chan;
#ifdef CONFIG_BT_LE_AUDIO
	struct bt_audio_chan tmp;
	uint8_t released = 0;
#endif

	audio_conn = find_conn(rep->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	chan = find_channel2(audio_conn, rep->id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

#ifdef CONFIG_BT_TRANSCEIVER
    SYS_LOG_INF("dir:%d", chan->dir);

    if(sys_get_work_mode()) {
        if(chan->state != BT_AUDIO_STREAM_QOS_CONFIGURED && chan->state != BT_AUDIO_STREAM_IDLE) {
            chan->state = BT_AUDIO_STREAM_QOS_CONFIGURED;
            chan->audio_contexts = 0;

            if (chan->dir == BT_AUDIO_DIR_SINK) {
                audio_conn->media_state = BT_STATUS_PAUSED;
            }

            bt_manager_event_notify(BT_AUDIO_STREAM_STOP, data, size);
        }
    }
#endif

	os_sched_lock();

	if (chan->tl) {
		timeline_release(chan->tl);
	}

	if (chan->period_tl) {
		timeline_release(chan->period_tl);
	}

#ifdef CONFIG_BT_LE_AUDIO
	/* add released operation for LE Slave only */
	if ((audio_conn->type == BT_TYPE_LE) &&
	    (audio_conn->role == BT_ROLE_SLAVE)) {
	    tmp.handle = audio_conn->handle;
		tmp.id = chan->id;
		released = 1;
	}
#endif

	/* BT_AUDIO_STREAM_IDLE */
	sys_slist_find_and_remove(&audio_conn->chan_list, &chan->node);
	mem_free(chan);

	os_sched_unlock();

	update_slave_prio(audio_conn,true);

	bt_manager_event_notify(BT_AUDIO_STREAM_RELEASE, data, size);

#ifdef CONFIG_BT_LE_AUDIO
	/* released at last to avoid race condition */
	if (released == 1) {
		bt_manager_le_audio_stream_released(&tmp);
	}
#endif

	return 0;
}

static int stream_start(void *data, int size)
{
	struct bt_audio_report *rep = (struct bt_audio_report *)data;
	struct bt_audio_channel *chan;

	chan = find_channel(rep->handle, rep->id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	chan->state = BT_AUDIO_STREAM_STARTED;

	return bt_manager_event_notify(BT_AUDIO_STREAM_START, data, size);
}

static int stream_stop(void *data, int size)
{
	struct bt_audio_report *rep = (struct bt_audio_report *)data;
	struct bt_audio_conn *audio_conn;
	struct bt_audio_channel *chan;

	audio_conn = find_conn(rep->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	chan = find_channel2(audio_conn, rep->id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	chan->state = BT_AUDIO_STREAM_QOS_CONFIGURED;
	/* reset audio_contexts */
	chan->audio_contexts = 0;

	if (chan->dir == BT_AUDIO_DIR_SINK) {
		audio_conn->media_state = BT_STATUS_PAUSED;
	}
	
	SYS_LOG_INF("dir:%d", chan->dir);

	update_slave_prio(audio_conn,true);

	return bt_manager_event_notify(BT_AUDIO_STREAM_STOP, data, size);
}

static int stream_discover_cap(void *data, int size)
{
	struct bt_audio_conn *audio_conn;
	struct bt_audio_capabilities *caps;

	caps = (struct bt_audio_capabilities *)data;

	audio_conn = find_conn(caps->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	audio_conn->caps = mem_malloc(size);
	if (!audio_conn->caps) {
		SYS_LOG_ERR("no mem");
		return -ENOMEM;
	}

	memcpy(audio_conn->caps, caps, size);

	return bt_manager_event_notify(BT_AUDIO_DISCOVER_CAPABILITY, data, size);
}

static int stream_discover_endp(void *data, int size)
{
	struct bt_audio_conn *audio_conn;
	struct bt_audio_endpoints *endps;

	endps = (struct bt_audio_endpoints *)data;

	audio_conn = find_conn(endps->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	audio_conn->endps = mem_malloc(size);
	if (!audio_conn->endps) {
		SYS_LOG_ERR("no mem");
		return -ENOMEM;
	}

	memcpy(audio_conn->endps, endps, size);

	return bt_manager_event_notify(BT_AUDIO_DISCOVER_ENDPOINTS, data, size);
}

static int stream_params(void *data, int size)
{
	struct bt_audio_params *params = (struct bt_audio_params *)data;
	struct bt_audio_channel *chan;

	chan = find_channel(params->handle, params->id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	chan->nse = params->nse;
	chan->m_bn = params->m_bn;
	chan->s_bn = params->s_bn;
	chan->m_ft = params->m_ft;
	chan->s_ft = params->s_ft;
	chan->cig_sync_delay = params->cig_sync_delay;
	chan->cis_sync_delay = params->cis_sync_delay;
	chan->rx_delay = params->rx_delay;
	chan->cis_audio_handle = params->audio_handle;

	return 0;
}

void bt_manager_audio_stream_event(int event, void *data, int size)
{
	SYS_LOG_INF("%d", event);

	switch (event) {
	case BT_AUDIO_STREAM_CONFIG_CODEC:
		stream_config_codec(data, size);
		break;
	case BT_AUDIO_STREAM_PREFER_QOS:
		stream_prefer_qos(data, size);
		break;
	case BT_AUDIO_STREAM_CONFIG_QOS:
		stream_config_qos(data, size);
		break;
	case BT_AUDIO_STREAM_ENABLE:
		stream_enable(data, size);
		break;
	case BT_AUDIO_STREAM_UPDATE:
		stream_update(data, size);
		break;
	case BT_AUDIO_STREAM_START:
		stream_start(data, size);
		break;
	case BT_AUDIO_STREAM_STOP:
		stream_stop(data, size);
		break;
	case BT_AUDIO_STREAM_DISABLE:
		stream_disable(data, size);
		break;
	case BT_AUDIO_STREAM_RELEASE:
		stream_release(data, size);
		break;
	case BT_AUDIO_DISCOVER_CAPABILITY:
		stream_discover_cap(data, size);
		break;
	case BT_AUDIO_DISCOVER_ENDPOINTS:
		stream_discover_endp(data, size);
		break;
	case BT_AUDIO_PARAMS:
		stream_params(data, size);
		break;
	case BT_AUDIO_STREAM_FAKE:
		stream_fake(data, size);
		break;
	default:
		bt_manager_event_notify(event, data, size);
		break;
	}
#ifdef CONFIG_BT_TRANSCEIVER
    if(sys_get_work_mode())
    {
        tr_bt_manager_audio_stream_event(event, data, size);
    }
#endif
}

#ifdef CONFIG_BT_LE_AUDIO

static uint64_t get_le_time(uint16_t handle)
{
	struct le_link_time time = {0, 0, 0};

	ctrl_get_le_link_time(handle, &time);

	return (uint64_t)((int64_t)time.ce_cnt * time.ce_interval_us + time.ce_offs_us);
}

static void stream_trigger(uint16_t handle, uint8_t id, uint16_t audio_handle)
{
	struct bt_audio_channel *chan;

	chan = find_channel(handle, id);
	if (!chan) {
		return;
	}

	chan->audio_handle = audio_handle;
	timeline_start(chan->tl);
	if (chan->trigger) {
		chan->trigger(handle, id, audio_handle);
	}

	chan->bt_time = get_le_time(audio_handle);
	chan->bt_count++;
	timeline_trigger_listener(chan->tl);
}

static void stream_period_trigger(uint16_t handle, uint8_t id,
				uint16_t audio_handle)
{
	struct bt_audio_channel *chan;

	chan = find_channel(handle, id);
	if (!chan) {
		return;
	}

	chan->audio_handle = audio_handle;
	timeline_start(chan->period_tl);
	if (chan->period_trigger) {
		chan->period_trigger(handle, id, audio_handle);
	}
	timeline_trigger_listener(chan->period_tl);
}

static const struct bt_audio_stream_cb stream_cb = {
	.tx_trigger = stream_trigger,
	.tx_period_trigger = stream_period_trigger,
	.rx_trigger = stream_trigger,
	.rx_period_trigger = stream_period_trigger,
};

static inline struct bt_audio_conn *find_le_slave(void)
{
	struct bt_audio_conn *audio_conn;

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if ((audio_conn->type == BT_TYPE_LE) &&
		    (audio_conn->role == BT_ROLE_SLAVE)) {
			return audio_conn;
		}
	}

	return NULL;
}

static struct bt_audio_channel *find_le_slave_chan(uint8_t dir)
{
	struct bt_audio_conn *audio_conn;
	struct bt_audio_channel *chan;

	audio_conn = find_le_slave();
	if (!audio_conn) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&audio_conn->chan_list, chan, node) {
		if (chan->dir == dir) {
			return chan;
		}
	}

	return NULL;
}

void bt_manager_audio_stream_tx_current_time(uint64_t *bt_time)
{
	struct bt_audio_channel *chan = find_le_slave_chan(BT_AUDIO_DIR_SOURCE);

	if (!chan) {
		*bt_time = 0;
		return;
	}

	*bt_time = get_le_time(chan->audio_handle);
}

void bt_manager_audio_stream_tx_last_time(uint64_t *time, uint64_t *count)
{
	struct bt_audio_channel *chan = find_le_slave_chan(BT_AUDIO_DIR_SOURCE);

	if (!chan) {
		*time = 0;
		*count = 0;
		return;
	}

	*time = chan->bt_time;
	*count = chan->bt_count;
}

void bt_manager_audio_stream_rx_current_time(uint64_t *bt_time)
{
	struct bt_audio_channel *chan = find_le_slave_chan(BT_AUDIO_DIR_SINK);

	if (!chan) {
		*bt_time = 0;
		return;
	}

	*bt_time = get_le_time(chan->audio_handle);
}

void bt_manager_audio_stream_rx_last_time(uint64_t *time, uint64_t *count)
{
	struct bt_audio_channel *chan = find_le_slave_chan(BT_AUDIO_DIR_SINK);

	if (!chan) {
		*time = 0;
		*count = 0;
		return;
	}

	*time = chan->bt_time;
	*count = chan->bt_count;
}

int bt_manager_audio_stream_cb_register(struct bt_audio_chan *chan,
				bt_audio_trigger_cb trigger,
				bt_audio_trigger_cb period_trigger)
{
	struct bt_audio_conn *audio_conn;
	struct bt_audio_channel *channel;
	int type;

	if (!chan) {
		return -EINVAL;
	}

	if (!trigger && !period_trigger) {
		return -EINVAL;
	}

	audio_conn = find_conn(chan->handle);
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type != BT_TYPE_LE) {
		return -EINVAL;
	}

	channel = find_channel2(audio_conn, chan->id);
	if (!channel) {
		return -EINVAL;
	}

	if (!trigger) {
		goto period;
	}

	if (channel->tl || channel->trigger) {
		return -EINVAL;
	}

	if (audio_conn->role == BT_ROLE_MASTER) {
		if (channel->dir == BT_AUDIO_DIR_SINK) {
			type = TIMELINE_TYPE_BLUETOOTH_LE_TX;
		} else {
			type = TIMELINE_TYPE_BLUETOOTH_LE_RX;
		}
	} else {
		if (channel->dir == BT_AUDIO_DIR_SINK) {
			type = TIMELINE_TYPE_BLUETOOTH_LE_RX;
		} else {
			type = TIMELINE_TYPE_BLUETOOTH_LE_TX;
		}
	}

	os_sched_lock();

	channel->tl = timeline_create(type, channel->interval);
	if (!channel->tl) {
		os_sched_unlock();
		return -ENOMEM;
	}

	channel->trigger = trigger;

	os_sched_unlock();

period:
	if (channel->period_tl || channel->period_trigger) {
		return -EINVAL;
	}

	/* Audio Source does not need period timeline yet */
	if (audio_conn->role == BT_ROLE_MASTER) {
		if (channel->dir == BT_AUDIO_DIR_SOURCE) {
			type = TIMELINE_TYPE_MEDIA_DECODE;
		} else {
			return 0;
		}
	} else {
		if (channel->dir == BT_AUDIO_DIR_SINK) {
			type = TIMELINE_TYPE_MEDIA_DECODE;
		} else {
			return 0;
		}
	}

	os_sched_lock();

	channel->period_tl = timeline_create(type, channel->interval);
	if (!channel->period_tl) {
		os_sched_unlock();
		return -ENOMEM;
	}

	channel->period_trigger = period_trigger;

	os_sched_unlock();

	return 0;
}

int bt_manager_audio_stream_cb_bind(struct bt_audio_chan *bound_chan,
				struct bt_audio_chan *new_chan,
				bt_audio_trigger_cb trigger,
				bt_audio_trigger_cb period_trigger)
{
#if 0
	struct bt_audio_channel *bond_channel;
	struct bt_audio_channel *new_channel;
	struct bt_audio_conn *bond_conn;
	struct bt_audio_conn *new_conn;

	if (!bound_chan || !new_chan) {
		return -EINVAL;
	}

	if (!trigger && !period_trigger) {
		return -EINVAL;
	}

	bond_conn = find_conn(bound_chan->handle);
	if (!bond_conn) {
		return -ENODEV;
	}

	new_conn = find_conn(new_chan->handle);
	if (!new_conn) {
		return -ENODEV;
	}

	if ((bond_conn->type != BT_TYPE_LE) || (new_conn->type != BT_TYPE_LE)) {
		return -EINVAL;
	}

	bond_channel = find_channel2(bond_conn, bound_chan->id);
	if (!bond_channel) {
		return -ENODEV;
	}

	new_channel = find_channel2(new_conn, new_chan->id);
	if (!new_channel) {
		return -ENODEV;
	}

	if (!trigger) {
		goto period;
	}

	os_sched_lock();

	if (!bond_channel->tl || !bond_channel->trigger) {
		os_sched_unlock();
		return -EINVAL;
	}

	timeline_ref(bond_channel->tl);
	new_channel->tl = bond_channel->tl;
	new_channel->trigger = trigger;

	os_sched_unlock();

	if (!period_trigger) {
		return 0;
	}

period:
	os_sched_lock();

	if (!bond_channel->period_tl || !bond_channel->period_trigger) {
		os_sched_unlock();
		return -EINVAL;
	}

	timeline_ref(bond_channel->period_tl);
	new_channel->period_tl = bond_channel->period_tl;
	new_channel->period_trigger = trigger;

	os_sched_unlock();
#endif

	return 0;
}
#endif

uint8_t bt_manager_audio_state(uint8_t type)
{
	if (type == BT_TYPE_LE) {
		return bt_audio.le_audio_state;
	}

	return BT_AUDIO_STATE_UNKNOWN;
}

#ifdef CONFIG_BT_LE_AUDIO

#ifdef CONFIG_BT_ADV_MANAGER
static void lea_adv_req_start_cb(void)
{
	SYS_LOG_INF("LEA ADV req start");

	bt_audio.le_audio_adv_enable = ADV_STATE_BIT_ENABLE;
	bt_audio.le_audio_adv_enable |= ADV_STATE_BIT_UPDATE;
	
}

static void lea_adv_req_stop_cb(void)
{
	SYS_LOG_INF("LEA ADV req stop");
	
	bt_audio.le_audio_adv_enable = ADV_STATE_BIT_DISABLE;
	bt_audio.le_audio_adv_enable |= ADV_STATE_BIT_UPDATE;	
}

static int btmgr_le_audio_adv_state(void)
{
	struct bt_audio_conn *audio_conn;
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();

	if (bt_audio.le_audio_adv_enable & ADV_STATE_BIT_ENABLE){

		if (bt_manager_is_poweroffing()){
			return (bt_audio.le_audio_adv_enable&(~ADV_STATE_BIT_ENABLE));
		}
	
		/*if device has been connected 2 BR, don't advertising LEA*/
		if (bt_manager_get_connected_dev_num() >=2){
			return (bt_audio.le_audio_adv_enable&(~ADV_STATE_BIT_ENABLE));
		}

		/*If br calling, don't advertising LEA */
		if (bt_manager_audio_in_btcall()){
			return (bt_audio.le_audio_adv_enable&(~ADV_STATE_BIT_ENABLE));
		}

		/*If br playing, don't advertising LEA */
		if (bt_manager_audio_in_btmusic()){
			if (!leaudio_config->app_auto_switch ||
				(leaudio_config->app_auto_switch && bt_manager_media_get_status() != BT_STATUS_PAUSED)){
				return (bt_audio.le_audio_adv_enable&(~ADV_STATE_BIT_ENABLE));
			}
		}

		/*if device has been connected LEA, don't advertising LEA*/
		audio_conn = find_conn_by_prio(BT_TYPE_LE, 0);
		if (audio_conn){
			return (bt_audio.le_audio_adv_enable&(~ADV_STATE_BIT_ENABLE));
		}
	}
	return bt_audio.le_audio_adv_enable;
}

static int btmgr_le_audio_adv_enable(void)
{
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();
	bt_addr_le_t peer;
	int ret = 0;

	SYS_LOG_INF("LEA ADV start");

	if (leaudio_config->leaudio_dir_adv_enable){
		if (cfg_feature->sp_tws){
			if (btif_tws_get_dev_role() == BTSRV_TWS_SLAVE){		
				/*TWS slave: only use LEA directed advertising*/
				ret = bt_manager_audio_get_peer_addr(&peer);
				if (ret != 0){			
					return 0;
				}
				bt_manager_audio_adv_param_init(&peer);
			}else{ 
				/*Others: use LEA legacy advertising*/			
				bt_manager_audio_adv_param_init(NULL);
			} 
		}else{
#ifdef CONFIG_LE_AUDIO_SR_BQB
            ret = -1;
#else
			ret = bt_manager_audio_get_peer_addr(&peer);
#endif
			if (ret == 0){	
				/*only use LEA directed advertising*/			
				bt_manager_audio_adv_param_init(&peer);
			}else{
#ifdef CONFIG_LE_AUDIO_SR_BQB
                if(!mcs_adv_flag)
                    bt_manager_audio_adv_param_init(NULL);
#else
				bt_manager_audio_adv_param_init(NULL);
#endif
			}
		}
	}else{
#ifdef CONFIG_LE_AUDIO_SR_BQB
        if(!mcs_adv_flag)
            bt_manager_audio_adv_param_init(NULL);
#else
		bt_manager_audio_adv_param_init(NULL);
#endif
	}

	if (bt_audio.le_audio_adv_enable & ADV_STATE_BIT_ENABLE){
		bt_audio.le_audio_adv_enable &= ~ADV_STATE_BIT_UPDATE;
	}

	return bt_manager_audio_le_start_adv();
}

static int btmgr_le_audio_adv_disable(void)
{
	SYS_LOG_INF("LEA ADV stop");

	if ((bt_audio.le_audio_adv_enable & ADV_STATE_BIT_ENABLE) == 0){
		bt_audio.le_audio_adv_enable &= ~ADV_STATE_BIT_UPDATE;
	}	

	return bt_manager_audio_le_stop_adv();
}

static const btmgr_adv_register_func_t lea_adv_funcs = {
	.adv_get_state = btmgr_le_audio_adv_state,
	.adv_enable = btmgr_le_audio_adv_enable,
	.adv_disable = btmgr_le_audio_adv_disable,
};
#endif

#endif

int bt_manager_audio_clear_paired_list(uint8_t type)
{
	if (type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO				
#ifdef CONFIG_BT_TRANSCEIVER
        if(sys_get_work_mode())
        {
            tr_bt_manager_audio_scan_mode(LE_DEL_ALL);
            return 0;
        }
#endif
		bt_addr_le_t peer;
		memset(&peer,0,sizeof(bt_addr_le_t));
		bt_manager_audio_save_peer_addr(&peer);

		struct bt_audio_conn *leaudio_conn = find_conn_by_prio(BT_TYPE_LE, 0);
		if (leaudio_conn){
			bt_manager_ble_disconnect((bt_addr_t*)&leaudio_conn->addr);
		}	
		
	#ifdef CONFIG_BT_ADV_MANAGER
		bt_audio.le_audio_adv_enable |= ADV_STATE_BIT_UPDATE;
		btmgr_adv_update_immediately();
	#endif

#endif
		SYS_LOG_INF("leaudio clear paired");
	}else{
		bt_manager_clear_paired_list();    
	}
	
	return 0;
}



/*
 * Audio Stream Control
 */
int bt_manager_audio_init(uint8_t type, struct bt_audio_role *role)
{
	if (type == BT_TYPE_LE) {
		bt_manager_audio_init_dev_volume();

#ifdef CONFIG_BT_LE_AUDIO	
		if (bt_audio.le_audio_init) {
			SYS_LOG_WRN("already");
			return -EALREADY;
		}

		int ret = bt_manager_le_audio_init(role);
		if (ret) {
			SYS_LOG_ERR("%d", ret);
			return ret;
		}

		bt_audio.le_audio_init = 1;
		bt_audio.le_audio_state = BT_AUDIO_STATE_INITED;
		bt_audio.le_audio_adv_enable = 0;

#ifdef CONFIG_BT_ADV_MANAGER
#ifdef CONFIG_BT_TRANSCEIVER
        if(sys_get_work_mode()) {
            tr_bt_le_audio_bandwidth_init();
        }
        else {
            bt_manager_audio_le_adv_cb_register(lea_adv_req_start_cb, lea_adv_req_stop_cb);
            btmgr_adv_register(ADV_TYPE_LEAUDIO, (btmgr_adv_register_func_t*)&lea_adv_funcs);
        }
#else
		bt_manager_audio_le_adv_cb_register(lea_adv_req_start_cb, lea_adv_req_stop_cb);
		btmgr_adv_register(ADV_TYPE_LEAUDIO, (btmgr_adv_register_func_t*)&lea_adv_funcs);
#endif
#endif

		if (role->disable_trigger) {
			return 0;
		}

		bt_manager_le_audio_stream_cb_register((struct bt_audio_stream_cb *)&stream_cb);
#endif		
		return 0;
	}

	return -EINVAL;
}

int bt_manager_audio_exit(uint8_t type)
{
	if (type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO		
		if (!bt_audio.le_audio_init) {
			SYS_LOG_WRN("already");
			return -EALREADY;
		}

		if (bt_audio.le_audio_start) {
			SYS_LOG_WRN("not stop");
			return -EINVAL;
		}

		int ret = bt_manager_le_audio_exit();
		if (ret) {
			SYS_LOG_ERR("%d", ret);
			return ret;
		}

		#ifdef CONFIG_BT_ADV_MANAGER
		btmgr_adv_register(ADV_TYPE_LEAUDIO, NULL);
		#endif
#endif

		bt_audio.le_audio_init = 0;
		bt_audio.le_audio_state = BT_AUDIO_STATE_UNKNOWN;

		return 0;
 	}

	return -EINVAL;
}

int bt_manager_audio_keys_clear(uint8_t type)
{
#ifdef CONFIG_BT_LE_AUDIO		
	if (type == BT_TYPE_LE) {
		return bt_manager_le_audio_keys_clear();
	}
#endif
	return -EINVAL;
}

#ifdef CONFIG_BT_LE_AUDIO		
int bt_manager_audio_server_cap_init(struct bt_audio_capabilities *caps)
{
	if (!caps) {
		return -EINVAL;
	}

	return bt_manager_le_audio_server_cap_init(caps);
}

int bt_manager_audio_server_qos_init(bool source,
				struct bt_audio_preferred_qos_default *qos)
{
	struct bt_audio_preferred_qos_default *default_qos;

	if (!qos) {
		return -EINVAL;
	}

	if (source) {
		bt_audio.le_audio_source_prefered_qos = 1;
		default_qos = &bt_audio.source_qos;
	} else {
		bt_audio.le_audio_sink_prefered_qos = 1;
		default_qos = &bt_audio.sink_qos;
	}

	default_qos->framing = qos->framing;
	default_qos->phy = qos->phy;
	default_qos->rtn = qos->rtn;
	default_qos->latency = qos->latency;
	default_qos->delay_min = qos->delay_min;
	default_qos->delay_max = qos->delay_max;
	default_qos->preferred_delay_min = qos->preferred_delay_min;
	default_qos->preferred_delay_max = qos->preferred_delay_max;

	return 0;
}

int bt_manager_audio_client_codec_init(struct bt_audio_codec_policy *policy)
{
	return 0;
}

int bt_manager_audio_client_qos_init(struct bt_audio_qos_policy *policy)
{
	return 0;
}
#endif

static int audio_start(uint8_t type, uint8_t lazy)
{
	if (type == BT_TYPE_LE) {
		if (!bt_audio.le_audio_init) {
			SYS_LOG_WRN("not init");
			return -EINVAL;
		}

		if (bt_audio.le_audio_start) {
			SYS_LOG_WRN("already");
			return -EALREADY;
		}
#ifdef CONFIG_BT_LE_AUDIO		
#ifdef CONFIG_BT_TRANSCEIVER
        int ret;
        if(sys_get_work_mode())
            ret = tr_bt_manager_audio_auto_start();
        else
            ret = bt_manager_le_audio_start();
       
#else
		int ret = bt_manager_le_audio_start();
#endif
		if (ret) {
			SYS_LOG_ERR("%d", ret);
			return ret;
		}
#endif
		bt_audio.le_audio_start = 1;
		bt_audio.le_audio_state = BT_AUDIO_STATE_STARTED;

		return 0;
	}

	return -EINVAL;
}

int bt_manager_audio_start(uint8_t type)
{
	int ret;

	os_sched_lock();
	ret = audio_start(type, 0);
	os_sched_lock();

	return ret;
}

static int audio_stop(uint8_t type,int blocked)
{
	int ret = 0;
	if (type == BT_TYPE_LE) {
		if (!bt_audio.le_audio_init) {
			SYS_LOG_WRN("not init");
			ret = -EINVAL;
			goto Exit;
		}

		if (!bt_audio.le_audio_start) {
			SYS_LOG_WRN("already");
			ret =  -EALREADY;
			goto Exit;
		
		}
Exit:	

#ifdef CONFIG_BT_LE_AUDIO		
#ifdef CONFIG_BT_TRANSCEIVER
        if(sys_get_work_mode())
            ret = tr_bt_manager_audio_auto_stop();
        else
            ret = bt_manager_le_audio_stop(blocked);
#else
		ret = bt_manager_le_audio_stop(blocked);
#endif
		if (ret) {
			SYS_LOG_ERR("%d", ret);
			return ret;
		}
#endif

		bt_audio.le_audio_start = 0;
		/* force to clear puase bits too */
		bt_audio.le_audio_pause = 0;
		bt_audio.le_audio_state = BT_AUDIO_STATE_STOPPED;

		return ret;
	}

	return -EINVAL;
}

int bt_manager_audio_stop(uint8_t type)
{
	int ret;

	os_sched_lock();
	ret = audio_stop(type, 1);
	os_sched_lock();

	return ret;
}

int bt_manager_audio_unblocked_stop(uint8_t type)
{
	int ret;

	os_sched_lock();
	ret = audio_stop(type, 0);
	os_sched_lock();

	return ret;
}

static int audio_pause(uint8_t type)
{
	if (type == BT_TYPE_LE) {
		if (!bt_audio.le_audio_init) {
			SYS_LOG_WRN("not init");
			return -EINVAL;
		}

		if (!bt_audio.le_audio_start) {
			SYS_LOG_WRN("not start");
			return -EINVAL;
		}

		if (bt_audio.le_audio_pause) {
			SYS_LOG_WRN("already");
			return -EALREADY;
		}
#ifdef CONFIG_BT_LE_AUDIO		
		int ret = bt_manager_le_audio_pause();
		if (ret) {
			SYS_LOG_ERR("%d", ret);
			return ret;
		}
#endif
		bt_audio.le_audio_pause = 1;
		bt_audio.le_audio_state = BT_AUDIO_STATE_PAUSED;

		return 0;
	}

	return -EINVAL;
}

static int audio_resume(uint8_t type)
{
	if (type == BT_TYPE_LE) {
		if (!bt_audio.le_audio_init) {
			SYS_LOG_WRN("not init");
			return -EINVAL;
		}

		if (!bt_audio.le_audio_start) {
			SYS_LOG_WRN("not start");
			return -EINVAL;
		}

		if (!bt_audio.le_audio_pause) {
			SYS_LOG_WRN("not pause");
			return -EINVAL;
		}
#ifdef CONFIG_BT_LE_AUDIO		
		int ret = bt_manager_le_audio_resume();
		if (ret) {
			SYS_LOG_ERR("%d", ret);
			return ret;
		}
#endif
		bt_audio.le_audio_pause = 0;
		bt_audio.le_audio_state = BT_AUDIO_STATE_STARTED;

		return 0;
	}

	return -EINVAL;
}

int bt_manager_audio_pause(uint8_t type)
{
	int ret;

	os_sched_lock();
	ret = audio_pause(type);
	os_sched_lock();

	return ret;
}

int bt_manager_audio_resume(uint8_t type)
{
	int ret;

	os_sched_lock();
	ret = audio_resume(type);
	os_sched_lock();

	return ret;
}

#ifdef CONFIG_BT_LE_AUDIO		
int bt_manager_audio_stream_config_codec(struct bt_audio_codec *codec)
{
	uint8_t type;
	int ret;

	if (!codec) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(codec->handle);

	if (type == BT_TYPE_LE) {
		ret = bt_manager_le_audio_stream_config_codec(codec);
		if (ret) {
			return ret;
		}

		return stream_config_codec(codec, sizeof(*codec));
	}

	return -EINVAL;
}

int bt_manager_audio_stream_prefer_qos(struct bt_audio_preferred_qos *qos)
{
	uint8_t type;

	if (!qos) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(qos->handle);

	if (type == BT_TYPE_LE) {
		return bt_manager_le_audio_stream_prefer_qos(qos);
	}

	return -EINVAL;
}

int bt_manager_audio_stream_config_qos(struct bt_audio_qoss *qoss)
{
	uint8_t type;

	if (!qoss || !qoss->num_of_qos) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(qoss->qos[0].handle);

	if (type == BT_TYPE_LE) {
		return bt_manager_le_audio_stream_config_qos(qoss);
	}

	return -EINVAL;
}

int bt_manager_audio_stream_bind_qos(struct bt_audio_qoss *qoss, uint8_t index)
{
	uint8_t type;

	if (!qoss || !qoss->num_of_qos) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(qoss->qos[0].handle);

	if (type == BT_TYPE_LE) {
		return bt_manager_le_audio_stream_bind_qos(qoss, index);
	}

	return -EINVAL;
}
#endif

/* check if all channels have the same handle. */
static bool check_valid(struct bt_audio_chan **chans, uint8_t num_chans)
{
	uint16_t handle;
	uint8_t i;

	if (!chans || !num_chans || !chans[0]) {
		return false;
	}

	if (num_chans == 1) {
		return true;
	}

	handle = chans[0]->handle;
	for (i = 1; i < num_chans; i++) {
		if (!chans[i] || (handle != chans[i]->handle)) {
			return false;
		}
	}

	return true;
}

#ifdef CONFIG_BT_LE_AUDIO

/*
 * check if the direction is valid.
 *
 * NOTICE: no need to check direction for BR Audio yet!
 */
static bool check_direction(struct bt_audio_chan **chans, uint8_t num_chans,
				bool source_stream)
{
	struct bt_audio_chan *chan;
	uint8_t i, role, dir;

	role = bt_manager_audio_get_role(chans[0]->handle);
	if (role == BT_ROLE_MASTER) {
		if (source_stream) {
			dir = BT_AUDIO_DIR_SINK;
		} else {
			dir = BT_AUDIO_DIR_SOURCE;
		}
	} else if (role == BT_ROLE_SLAVE) {
		if (source_stream) {
			dir = BT_AUDIO_DIR_SOURCE;
		} else {
			dir = BT_AUDIO_DIR_SINK;
		}
	} else {
		return false;
	}

	for (i = 0; i < num_chans; i++) {
		chan = chans[i];
		if (bt_manager_audio_stream_dir(chan->handle, chan->id) != dir) {
			return false;
		}
	}

	return true;
}

int bt_manager_audio_stream_sync_forward(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
	uint8_t type;

	if (!chans || !num_chans || !chans[0]) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(chans[0]->handle);

	if (type == BT_TYPE_LE) {
		return bt_manager_le_audio_stream_sync_forward(chans, num_chans);
	}

	return -EINVAL;
}

int bt_manager_audio_stream_sync_forward_single(struct bt_audio_chan *chan)
{
	return bt_manager_audio_stream_sync_forward(&chan, 1);
}

int bt_manager_audio_stream_enable(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t contexts)
{
	uint8_t type;

	if (!check_valid(chans, num_chans)) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(chans[0]->handle);

	if (type == BT_TYPE_LE) {
		return bt_manager_le_audio_stream_enable(chans, num_chans, contexts);
	}

	return -EINVAL;
}

int bt_manager_audio_stream_enable_single(struct bt_audio_chan *chan,
				uint32_t contexts)
{
	return bt_manager_audio_stream_enable(&chan, 1, contexts);
}

int bt_manager_audio_stream_disable(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
	uint8_t type;

	if (!check_valid(chans, num_chans)) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(chans[0]->handle);

	if (type == BT_TYPE_LE) {
		return bt_manager_le_audio_stream_disable(chans, num_chans);
	}

	return -EINVAL;
}

int bt_manager_audio_stream_disable_single(struct bt_audio_chan *chan)
{
	return bt_manager_audio_stream_disable(&chan, 1);
}
#endif
io_stream_t bt_manager_audio_source_stream_create(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t locations)
{
	struct bt_audio_channel *chan;
	uint8_t type, codec;

	if (!chans || !num_chans || !chans[0]) {
		return false;
	}

	type = bt_manager_audio_get_type(chans[0]->handle);

	if (type == BT_TYPE_BR) {
		if (chans[0]->id != BT_AUDIO_ENDPOINT_CALL) {
			return NULL;
		}

		chan = find_channel(chans[0]->handle, chans[0]->id);
		if (!chan) {
			return NULL;
		}
		codec = BTSRV_SCO_MSBC;

		if (chan->format == BT_AUDIO_CODEC_MSBC) {
			codec = BTSRV_SCO_MSBC;
		} else if (chan->format == BT_AUDIO_CODEC_CVSD) {
			codec = BTSRV_SCO_CVSD;
		}

		return sco_upload_stream_create(codec);
	} else if (type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		if (!check_direction(chans, num_chans, true)) {
			return NULL;
		}

		return bt_manager_le_audio_source_stream_create(chans,
						num_chans, locations);
#endif
	}

	return NULL;
}

io_stream_t bt_manager_audio_source_stream_create_single(struct bt_audio_chan *chan,
				uint32_t locations)
{
	return bt_manager_audio_source_stream_create(&chan, 1, locations);
}

int bt_manager_audio_source_stream_set(struct bt_audio_chan **chans,
				uint8_t num_chans, io_stream_t stream,
				uint32_t locations)
{
	uint8_t type;

	if (!chans || !num_chans || !chans[0]) {
		return -EINVAL;
	}

	/* NOTICE: should use 1 stream for each chan later ... */
	if (num_chans != 1) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(chans[0]->handle);
#ifdef CONFIG_BT_LE_AUDIO
	if (type == BT_TYPE_LE) {
		if (!check_direction(chans, num_chans, true)) {
			return -EINVAL;
		}

		return bt_manager_le_audio_source_stream_set(chans,
						num_chans, stream, locations);
	}
#endif
	return -EINVAL;
}

int bt_manager_audio_source_stream_set_single(struct bt_audio_chan *chan,
				io_stream_t stream, uint32_t locations)
{
	return bt_manager_audio_source_stream_set(&chan, 1, stream, locations);
}

int bt_manager_audio_sink_stream_set(struct bt_audio_chan *chan,
				io_stream_t stream)
{
	uint8_t type;

	if (!chan) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(chan->handle);

	if (type == BT_TYPE_BR) {
		if (chan->id == BT_AUDIO_ENDPOINT_MUSIC) {
			return bt_manager_set_stream(STREAM_TYPE_A2DP, stream);
		} else if (chan->id == BT_AUDIO_ENDPOINT_CALL) {
			return bt_manager_set_stream(STREAM_TYPE_SCO, stream);
		}
	} else if (type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		if (!check_direction(&chan, 1, false)) {
			return -EINVAL;
		}

		return bt_manager_le_audio_sink_stream_set(chan, stream);
#endif
	}

	return -EINVAL;
}

int bt_manager_audio_sink_stream_start(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
#ifdef CONFIG_BT_LE_AUDIO

	uint8_t type;

	if (!check_valid(chans, num_chans)) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(chans[0]->handle);

	if (type == BT_TYPE_LE) {
		if (!check_direction(chans, num_chans, false)) {
			return -EINVAL;
		}

		return bt_manager_le_audio_sink_stream_start(chans, num_chans);
	}
#endif
	return -EINVAL;
}

int bt_manager_audio_sink_stream_start_single(struct bt_audio_chan *chan)
{
	return bt_manager_audio_sink_stream_start(&chan, 1);
}

int bt_manager_audio_sink_stream_stop(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
#ifdef CONFIG_BT_LE_AUDIO

	uint8_t type;

	if (!check_valid(chans, num_chans)) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(chans[0]->handle);

	if (type == BT_TYPE_LE) {
		if (!check_direction(chans, num_chans, false)) {
			return -EINVAL;
		}

		return bt_manager_le_audio_sink_stream_stop(chans, num_chans);
	}
#endif
	return -EINVAL;
}

int bt_manager_audio_sink_stream_stop_single(struct bt_audio_chan *chan)
{
	return bt_manager_audio_sink_stream_stop(&chan, 1);
}

int bt_manager_audio_stream_update(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t contexts)
{
	uint8_t type;

	if (!check_valid(chans, num_chans)) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(chans[0]->handle);
#ifdef CONFIG_BT_LE_AUDIO
	if (type == BT_TYPE_LE) {
		return bt_manager_le_audio_stream_update(chans, num_chans, contexts);
	}
#endif
	return -EINVAL;
}

int bt_manager_audio_stream_update_single(struct bt_audio_chan *chan,
				uint32_t contexts)
{
	return bt_manager_audio_stream_update(&chan, 1, contexts);
}

int bt_manager_audio_stream_release(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
	uint8_t type;

	if (!check_valid(chans, num_chans)) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(chans[0]->handle);
#ifdef CONFIG_BT_LE_AUDIO
	if (type == BT_TYPE_LE) {
		return bt_manager_le_audio_stream_release(chans, num_chans);
	}
#endif
	return -EINVAL;
}

int bt_manager_audio_stream_release_single(struct bt_audio_chan *chan)
{
	return bt_manager_audio_stream_release(&chan, 1);
}

int bt_manager_audio_stream_disconnect(struct bt_audio_chan **chans,
				uint8_t num_chans)
{
	uint8_t type;

	if (!check_valid(chans, num_chans)) {
		return -EINVAL;
	}

	type = bt_manager_audio_get_type(chans[0]->handle);
#ifdef CONFIG_BT_LE_AUDIO
	if (type == BT_TYPE_LE) {
		return bt_manager_le_audio_stream_disconnect(chans, num_chans);
	}
#endif
	return -EINVAL;
}

int bt_manager_audio_stream_disconnect_single(struct bt_audio_chan *chan)
{
	return bt_manager_audio_stream_disconnect(&chan, 1);
}

#ifdef CONFIG_BT_LE_AUDIO		
void *bt_manager_audio_stream_bandwidth_alloc(struct bt_audio_qoss *qoss)
{
	return bt_manager_le_audio_stream_bandwidth_alloc(qoss);
}

int bt_manager_audio_stream_bandwidth_free(void *cig_handle)
{
	return bt_manager_le_audio_stream_bandwidth_free(cig_handle);
}
#endif

static inline int stream_restore_br(void)
{
	return bt_manager_a2dp_check_state();
}

#ifdef CONFIG_BT_LE_AUDIO
static int stream_restore_le(void)
{
	struct bt_audio_conn *audio_conn = find_active_slave();
	struct bt_audio_chan_restore_max restore;
	struct bt_audio_channel *chan;
	struct bt_audio_chan *ch;
	uint8_t count;

	/* could restore LE slave only */
	if (!audio_conn || audio_conn->type != BT_TYPE_LE) {
		return -EINVAL;
	}

	os_sched_lock();

	if (sys_slist_is_empty(&audio_conn->chan_list)) {
		os_sched_unlock();
		return -ENODEV;
	}

	count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&audio_conn->chan_list, chan, node) {
		if (count >= BT_AUDIO_CHAN_MAX) {
			break;
		}

		ch = &restore.chans[count];
		ch->handle = audio_conn->handle;
		ch->id = chan->id;

		count++;
	}

	restore.count = count;

	os_sched_unlock();

	return bt_manager_event_notify(BT_AUDIO_STREAM_RESTORE, &restore,
				sizeof(restore));
}
#endif

int bt_manager_audio_stream_restore(uint8_t type)
{
	if (type == BT_TYPE_BR) {
		return stream_restore_br();
	} else if (type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		return stream_restore_le();
#endif
	}

	return -EINVAL;
}

enum media_type bt_manager_audio_codec_type(uint8_t coding_format)
{
	switch (coding_format) {
	case BT_AUDIO_CODEC_LC3:
		return NAV_TYPE;
	case BT_AUDIO_CODEC_CVSD:
		return CVSD_TYPE;
	case BT_AUDIO_CODEC_MSBC:
		return MSBC_TYPE;
	case BT_AUDIO_CODEC_AAC:
		return AAC_TYPE;
	case BT_AUDIO_CODEC_SBC:
		return SBC_TYPE;
	default:
		break;
	}

	return UNSUPPORT_TYPE;
}

int bt_manager_media_play(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_avrcp_play();
	} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		return bt_manager_le_media_play(audio_conn->handle);
#endif
	}

	return -EINVAL;
}

int bt_manager_media_stop(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_avrcp_stop();
	} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		return bt_manager_le_media_stop(audio_conn->handle);
#endif
	}

	return -EINVAL;
}

int bt_manager_media_pause(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_avrcp_pause();
	} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		return bt_manager_le_media_pause(audio_conn->handle);
#endif
	}

	return -EINVAL;
}

int bt_manager_media_play_next(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_avrcp_play_next();
	} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		return bt_manager_le_media_play_next(audio_conn->handle);
#endif
	}

	return -EINVAL;
}

int bt_manager_media_play_previous(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_avrcp_play_previous();
	} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		return bt_manager_le_media_play_previous(audio_conn->handle);
#endif
	}

	return -EINVAL;
}

int bt_manager_media_fast_forward(bool start)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_avrcp_fast_forward(start);
	} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		return bt_manager_le_media_fast_forward(audio_conn->handle);
#endif
	}

	return -EINVAL;
}

int bt_manager_media_fast_rewind(bool start)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_avrcp_fast_backward(start);
	} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		return bt_manager_le_media_fast_rewind(audio_conn->handle);
#endif
	}

	return -EINVAL;
}

int bt_manager_media_get_id3_info(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_avrcp_get_id3_info();
	}

	return -EINVAL;
}

void bt_manager_media_event(int event, void *data, int size)
{
	struct bt_media_report *rep = (struct bt_media_report *)data;
	struct bt_audio_conn *audio_conn;
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();

	SYS_LOG_INF("%d", event);

	audio_conn = find_conn(rep->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return;
	}

	if (audio_conn->role == BT_ROLE_MASTER) {
		bt_manager_event_notify(event, data, size);
		return;
	}

	switch (event) {
		case BT_MEDIA_PLAY:
			audio_conn->media_state = BT_STATUS_PLAYING;
			update_slave_prio(audio_conn,true);	
			break;
		case BT_MEDIA_PAUSE:
		case BT_MEDIA_STOP:
			audio_conn->media_state = BT_STATUS_PAUSED;
			update_slave_prio(audio_conn,true);
			break;
		case BT_MEDIA_FAKE_PAUSE:
			audio_conn->media_state = BT_STATUS_PAUSED;
			if (leaudio_config->app_auto_switch){
				update_slave_prio(audio_conn,true);
			}else{
				update_slave_prio(audio_conn,false);
			}
			break;
	}

	if (audio_conn == find_active_slave()) {
		bt_manager_event_notify(event, data, size);
	} else {
		SYS_LOG_INF("drop msg %p %p", audio_conn, find_active_slave());
	}
}

int bt_manager_media_get_status(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return BT_STATUS_PAUSED;
	}

	return audio_conn->media_state;
}

static struct bt_audio_call_inst *new_bt_call(struct bt_audio_conn *audio_conn,
				uint8_t index)
{
	struct bt_audio_call_inst *call;
	sys_slist_t *list;

	if (is_bt_le_master(audio_conn)) {
		list = &bt_audio.cg_list;
	} else if (audio_conn->role == BT_ROLE_SLAVE) {
		list = &audio_conn->ct_list;
	} else {
		return NULL;
	}

	call = mem_malloc(sizeof(struct bt_audio_call_inst));
	if (!call) {
		SYS_LOG_ERR("no mem");
		return NULL;
	}

	call->handle = audio_conn->handle;
	call->index = index;

	os_sched_lock();
	sys_slist_append(list, &call->node);
	os_sched_unlock();

	return call;
}

static struct bt_audio_call_inst *find_bt_call(struct bt_audio_conn *audio_conn,
				uint8_t index)
{
	struct bt_audio_call_inst *call;
	sys_slist_t *list;

	if (is_bt_le_master(audio_conn)) {
		list = &bt_audio.cg_list;
	} else if (audio_conn->role == BT_ROLE_SLAVE) {
		list = &audio_conn->ct_list;
	} else {
		return NULL;
	}

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(list, call, node) {
		if (call->index == index) {
			os_sched_unlock();
			return call;
		}
	}

	os_sched_unlock();

	return NULL;
}

/* if not found, create a new call */
static struct bt_audio_call_inst *find_new_bt_call(struct bt_audio_conn *audio_conn,
				uint8_t index)
{
	struct bt_audio_call_inst *call;

	call = find_bt_call(audio_conn, index);
	if (!call) {
		call = new_bt_call(audio_conn, index);
	}

	if (!call) {
		return NULL;
	}

	return call;
}

static int delete_bt_call(struct bt_audio_conn *audio_conn, uint8_t index)
{
	struct bt_audio_call_inst *call;
	sys_slist_t *list;

	if (is_bt_le_master(audio_conn)) {
		list = &bt_audio.cg_list;
	} else if (audio_conn->role == BT_ROLE_SLAVE) {
		list = &audio_conn->ct_list;
	} else {
		return -EINVAL;
	}

	call = find_bt_call(audio_conn, index);
	if (!call) {
		SYS_LOG_ERR("no call");
		return -ENODEV;
	}

	os_sched_lock();

	sys_slist_find_and_remove(list, &call->node);
	mem_free(call);

	os_sched_unlock();

	return 0;
}

#ifdef CONFIG_BT_LE_AUDIO
static struct bt_audio_call_inst* find_le_audio_call(struct bt_audio_conn* audio_conn, uint8_t state)
{
	struct bt_audio_call_inst *call_inst;

	if (audio_conn->type == BT_TYPE_LE) {
		/* FIXME: find a incoming call? */
		call_inst = find_slave_call(audio_conn, BT_CALL_STATE_INCOMING);
			if (!call_inst) {
				return NULL;
			}
			return call_inst;
	}
	return NULL;
}
#endif



int bt_manager_audio_call_info(struct bt_audio_call *call,
				struct bt_audio_call_info *info)
{
	struct bt_audio_call_inst *call_inst;
	struct bt_audio_conn *audio_conn;

	if (!info) {
		return -EINVAL;
	}

	audio_conn = find_conn(call->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	call_inst = find_bt_call(audio_conn, call->index);
	if (!call_inst) {
		SYS_LOG_ERR("no call");
		return -ENODEV;
	}

	os_sched_lock();

	info->state = call_inst->state;
	strncpy(info->remote_uri, call_inst->remote_uri,
		BT_AUDIO_CALL_URI_LEN - 1);

	os_sched_unlock();

	return 0;
}

int bt_manager_call_originate(uint16_t handle, uint8_t *remote_uri)
{
#ifdef CONFIG_BT_LE_AUDIO
	struct bt_audio_conn *audio_conn;

	if (handle == 0) {
		audio_conn = find_active_slave();
	} else {
		audio_conn = find_conn(handle);
	}

	if (!audio_conn) {
		return -ENODEV;
	}

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_call_originate(audio_conn->handle, remote_uri);
	}
#endif
	return -EINVAL;
}

int bt_manager_call_accept(struct bt_audio_call *call)
{
	struct bt_audio_conn *audio_conn;
#ifdef CONFIG_BT_LE_AUDIO
	struct bt_audio_call_inst *call_inst;	
	struct bt_audio_call tmp;
#endif

	/* default: select current active call */
	if (!call) {
		audio_conn = find_active_slave();
		if (!audio_conn) {
			return -ENODEV;
		}

		if (audio_conn->type == BT_TYPE_BR) {
			return bt_manager_hfp_accept_call();
		} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO		
			/* FIXME: find a incoming call? */
			call_inst = find_slave_call(audio_conn, BT_CALL_STATE_INCOMING);
			if (!call_inst) {
				return -ENODEV;
			}

			tmp.handle = call_inst->handle;
			tmp.index = call_inst->index;
			return bt_manager_le_call_accept(&tmp);
#endif
		}

		return -EINVAL;
	}

	/* select by upper layer */

	audio_conn = find_conn(call->handle);
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_hfp_accept_call();
	} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO	
		return bt_manager_le_call_accept(call);
#endif
	}

	return -EINVAL;
}

int bt_manager_call_hold(struct bt_audio_call *call)
{
#ifdef CONFIG_BT_LE_AUDIO
	struct bt_audio_conn *audio_conn;
	struct bt_audio_call_inst *call_inst;
	struct bt_audio_call tmp;

	/* default: select current active call */
	if (!call) {
		audio_conn = find_active_slave();
		if (!audio_conn) {
			return -ENODEV;
		}

		if (audio_conn->type == BT_TYPE_BR) {
			return -EINVAL;
		} else if (audio_conn->type == BT_TYPE_LE) {
			/* FIXME: find a active call? */
			call_inst = find_slave_call(audio_conn, BT_CALL_STATE_ACTIVE);
			if (!call_inst) {
				return -ENODEV;
			}

			tmp.handle = call_inst->handle;
			tmp.index = call_inst->index;
			return bt_manager_le_call_hold(&tmp);
		}

		return -EINVAL;
	}

	/* select by upper layer */

	audio_conn = find_conn(call->handle);
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return -EINVAL;
	} else if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_call_hold(call);
	}
#endif

	return -EINVAL;
}

int bt_manager_call_retrieve(struct bt_audio_call *call)
{
#ifdef CONFIG_BT_LE_AUDIO
	struct bt_audio_conn *audio_conn;
	struct bt_audio_call_inst *call_inst;
	struct bt_audio_call tmp;

	/* default: select current active call */
	if (!call) {
		audio_conn = find_active_slave();
		if (!audio_conn) {
			return -ENODEV;
		}

		if (audio_conn->type == BT_TYPE_BR) {
			return -EINVAL;
		} else if (audio_conn->type == BT_TYPE_LE) {
			/* FIXME: find a locally held call? */
			call_inst = find_slave_call(audio_conn, BT_CALL_STATE_LOCALLY_HELD);
			if (!call_inst) {
				call_inst = find_slave_call(audio_conn, BT_CALL_STATE_HELD);
			}

			if (!call_inst) {
				return -ENODEV;
			}

			tmp.handle = call_inst->handle;
			tmp.index = call_inst->index;
			return bt_manager_le_call_retrieve(&tmp);
		}

		return -EINVAL;
	}

	/* select by upper layer */
	audio_conn = find_conn(call->handle);
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return -EINVAL;
	} else if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_call_retrieve(call);
	}
#endif

	return -EINVAL;
}

int bt_manager_call_join(struct bt_audio_call **calls, uint8_t num_calls)
{
	return -EINVAL;
}

int bt_manager_call_terminate(struct bt_audio_call *call, uint8_t reason)
{
	struct bt_audio_conn *audio_conn;
#ifdef CONFIG_BT_LE_AUDIO
	struct bt_audio_call_inst *call_inst;
	struct bt_audio_call tmp;
#endif

	/* default: select current active call */
	if (!call) {
		audio_conn = find_active_slave();
		if (!audio_conn) {
			return -ENODEV;
		}

		if (audio_conn->type == BT_TYPE_BR) {
			return bt_manager_hfp_hangup_call();
		} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO		
			/* FIXME: find a active call? */
			call_inst = find_slave_call(audio_conn, BT_CALL_STATE_ACTIVE);
			if (!call_inst) {
				/* FIXME: find the first call? */
				call_inst = find_fisrt_slave_call(audio_conn);
			}

			if (!call_inst) {
				return -ENODEV;
			}

			tmp.handle = call_inst->handle;
			tmp.index = call_inst->index;
			return bt_manager_le_call_terminate(&tmp);
#endif			
		}

		return -EINVAL;
	}

	/* select by upper layer */
	audio_conn = find_conn(call->handle);
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_hfp_hangup_call();
	} else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO			
		return bt_manager_le_call_terminate(call);
#endif
	}

	return -EINVAL;
}

int bt_manager_call_hangup_another_call(void)
{
	struct bt_audio_conn *audio_conn;
	struct bt_audio_conn *audio_conn2;
#ifdef CONFIG_BT_LE_AUDIO	
	struct bt_audio_call tmp;
	struct bt_audio_call_inst*	call_inst;
#endif

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		SYS_LOG_INF("call_state:%d hfp state:%d",audio_conn->call_state,bt_manager_hfp_get_status());
		
		if (audio_conn->call_state == BT_STATUS_3WAYIN  ||
			audio_conn->call_state == BT_STATUS_3WAYOUT ||
			audio_conn->call_state == BT_STATUS_MULTIPARTY){
			return bt_manager_hfp_hangup_another_call();
		}else{
#ifdef CONFIG_BT_LE_AUDIO
			audio_conn2 = find_conn_by_prio(BT_TYPE_LE, BT_AUDIO_PRIO_CALL);
			if (!audio_conn2){
				SYS_LOG_INF("BT_TYPE_LE call FIXME");
				return -EINVAL;
			}
			call_inst = find_le_audio_call(audio_conn2, BT_CALL_STATE_INCOMING);
			if (!call_inst){
				SYS_LOG_INF("BT_TYPE_LE call_inst unexist, FIXME");
				return -EINVAL;
			}			

			tmp.handle = call_inst->handle;
			tmp.index = call_inst->index;
			return bt_manager_call_terminate(&tmp, 0);
#endif			
		}
	}else if(audio_conn->type == BT_TYPE_LE){

		audio_conn2 = find_conn_by_prio(BT_TYPE_BR, BT_AUDIO_PRIO_CALL);
		if (audio_conn2){
			SYS_LOG_INF("BT_TYPE_BR call FIXME");
			return bt_manager_hfp_hangup_call();
		}
	}

	return -EINVAL;
}

int bt_manager_call_holdcur_answer_call(void)
{
	struct bt_audio_conn *audio_conn;
#ifdef CONFIG_BT_LE_AUDIO
	struct bt_audio_conn *audio_conn2;
	struct bt_audio_call tmp;
	struct bt_audio_call_inst*	call_inst;
#endif

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		SYS_LOG_INF("call_state:%d hfp state:%d",audio_conn->call_state,bt_manager_hfp_get_status());
		
		if (audio_conn->call_state == BT_STATUS_3WAYIN	||
			audio_conn->call_state == BT_STATUS_3WAYOUT ||
			audio_conn->call_state == BT_STATUS_MULTIPARTY){
			return bt_manager_hfp_holdcur_answer_call();
		}else{
#ifdef CONFIG_BT_LE_AUDIO
			audio_conn2 = find_conn_by_prio(BT_TYPE_LE, BT_AUDIO_PRIO_CALL);
			if (!audio_conn2){
				SYS_LOG_INF("BT_TYPE_LE call FIXME");
				return  -EINVAL;
			}
			bt_manager_hfp_hangup_call();

			call_inst = find_le_audio_call(audio_conn2, BT_CALL_STATE_INCOMING);
			if (!call_inst){
				SYS_LOG_INF("BT_TYPE_LE call_inst unexist, FIXME");
				return -EINVAL;
			}			

			tmp.handle = call_inst->handle;
			tmp.index = call_inst->index;
			return bt_manager_le_call_accept(&tmp);	
#endif			
		}
	}else if(audio_conn->type == BT_TYPE_LE){
#ifdef CONFIG_BT_LE_AUDIO
		call_inst = find_le_audio_call(audio_conn, BT_CALL_STATE_INCOMING);
		if (!call_inst){
			SYS_LOG_INF("BT_TYPE_LE call_inst unexist, FIXME");
			return -EINVAL;
		}
		tmp.handle = call_inst->handle;
		tmp.index = call_inst->index;
		bt_manager_call_hold(&tmp);

		audio_conn2 = find_conn_by_prio(BT_TYPE_BR, BT_AUDIO_PRIO_CALL);
		if (audio_conn2){
			SYS_LOG_INF("BT_TYPE_BR call FIXME");
			return bt_manager_hfp_accept_call();
		}
#endif		
	}

	return -EINVAL;
}

int bt_manager_call_hangupcur_answer_call(void)
{
	struct bt_audio_conn *audio_conn;
#ifdef CONFIG_BT_LE_AUDIO	
	struct bt_audio_conn *audio_conn2;
	struct bt_audio_call tmp;
	struct bt_audio_call_inst*	call_inst;
#endif
	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		SYS_LOG_INF("call_state:%d hfp state:%d",audio_conn->call_state,bt_manager_hfp_get_status());
		
		if (audio_conn->call_state == BT_STATUS_3WAYIN	||
			audio_conn->call_state == BT_STATUS_3WAYOUT ||
			audio_conn->call_state == BT_STATUS_MULTIPARTY){
			return bt_manager_hfp_hangupcur_answer_call();
		}else{
#ifdef CONFIG_BT_LE_AUDIO			
			audio_conn2 = find_conn_by_prio(BT_TYPE_LE, BT_AUDIO_PRIO_CALL);
			if (!audio_conn2){
				SYS_LOG_INF("BT_TYPE_LE call FIXME");
				return  -EINVAL;
			}
			bt_manager_hfp_hangup_call();

			call_inst = find_le_audio_call(audio_conn2, BT_CALL_STATE_INCOMING);
			if (!call_inst){
				SYS_LOG_INF("BT_TYPE_LE call_inst unexist, FIXME");
				return -EINVAL;
			}			

			tmp.handle = call_inst->handle;
			tmp.index = call_inst->index;
			return bt_manager_le_call_accept(&tmp);	
#endif			
		}
	}else if(audio_conn->type == BT_TYPE_LE){
#ifdef CONFIG_BT_LE_AUDIO		
		call_inst = find_le_audio_call(audio_conn, BT_CALL_STATE_INCOMING);
		if (!call_inst){
			SYS_LOG_INF("BT_TYPE_LE call_inst unexist, FIXME");
			return -EINVAL;
		}
		tmp.handle = call_inst->handle;
		tmp.index = call_inst->index;
		bt_manager_call_terminate(&tmp, 0);

		audio_conn2 = find_conn_by_prio(BT_TYPE_BR, BT_AUDIO_PRIO_CALL);
		if (audio_conn2){
			SYS_LOG_INF("BT_TYPE_BR call FIXME");
			return bt_manager_hfp_accept_call();
		}
#endif		
	}

	return -EINVAL;
}

int bt_manager_call_switch_sound_source(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_hfp_switch_sound_source();
	}

	return -EINVAL;
}

int bt_manager_call_allow_stream_connect(bool allowed)
{
	return bt_manager_allow_sco_connect(allowed);
}

int bt_manager_call_start_siri(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_hfp_start_siri();
	}

	return -EINVAL;
}

int bt_manager_call_stop_siri(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_hfp_stop_siri();
	}

	return -EINVAL;
}

int bt_manager_call_dial_last(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_BR) {
		return bt_manager_hfp_dial_last_number();
	}

	return -EINVAL;
}

void bt_manager_call_event(int event, void *data, int size)
{
	struct bt_call_report *rep = (struct bt_call_report *)data;
	struct bt_audio_conn *audio_conn;
	struct bt_audio_call_inst *call;
	struct phone_num_param {
		struct bt_call_report rep;
		uint8_t data[20];
	} sparam;

	SYS_LOG_INF("%d", event);

#ifdef CONFIG_BT_TRANSCEIVER
	audio_conn = find_conn(rep->handle);
	if (!audio_conn) {
        if(sys_get_work_mode())
        {
            audio_conn = find_active_master(BT_TYPE_LE);
            if (!audio_conn)
            {
                SYS_LOG_ERR("no conn");
                return;
            }
            if(find_fisrt_call(audio_conn) == NULL)
            {
                if (!audio_conn)
                {
                    SYS_LOG_ERR("no call");
                    return;
                }
            }
        }
        else
        {
            SYS_LOG_ERR("no conn");
            return;
        }
	}
#else
	audio_conn = find_conn(rep->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return;
	}

	/* FIXME: support master later ... */
	if (audio_conn->role != BT_ROLE_SLAVE) {
		return;
	}
#endif

	if (audio_conn->type == BT_TYPE_BR) {
		if (event == BT_CALL_ENDED) {
			audio_conn->call_state = BT_STATUS_HFP_NONE;
			audio_conn->fake_contexts = 0;
			goto report;
		}

		switch (event) {
		case BT_CALL_RING_STATR_EVENT:
			memset(&sparam,0,sizeof(sparam));
			sparam.rep.handle = rep->handle;
			sparam.rep.index = 1;
			if (strlen(rep->uri) > sizeof(sparam.data) -1)
				memcpy(sparam.data,rep->uri,sizeof(sparam.data) -1);
			else
				memcpy(sparam.data,rep->uri,strlen(rep->uri));

			bt_manager_event_notify(BT_CALL_RING_STATR_EVENT, (void*)&sparam, sizeof(struct phone_num_param));
			return;

		case BT_CALL_INCOMING:
			audio_conn->call_state = BT_STATUS_INCOMING;
			break;
		case BT_CALL_DIALING:
			audio_conn->call_state = BT_STATUS_OUTGOING;
			break;
		case BT_CALL_ALERTING:
			audio_conn->call_state = BT_STATUS_OUTGOING;
			break;
		case BT_CALL_ACTIVE:
			audio_conn->call_state = BT_STATUS_ONGOING;
			break;
		case BT_CALL_3WAYIN:
			audio_conn->call_state = BT_STATUS_3WAYIN;
			break;
		case BT_CALL_MULTIPARTY:
			audio_conn->call_state = BT_STATUS_MULTIPARTY;
			break;
		case BT_CALL_3WAYOUT:
			audio_conn->call_state = BT_STATUS_3WAYOUT;
			break;
		case BT_CALL_SIRI_MODE:
			audio_conn->call_state = BT_STATUS_SIRI;
			break;
		default:
			break;
		}

		goto report;
	}

	/* LE Audio */
	if (event == BT_CALL_ENDED) {
		delete_bt_call(audio_conn, rep->index);
		goto report;
	}

	/* the first message may come with any state */
	call = find_new_bt_call(audio_conn, rep->index);
	if (!call) {
		SYS_LOG_ERR("no call");
		return;
	}

	switch (event) {
	case BT_CALL_INCOMING:
		call->state = BT_CALL_STATE_INCOMING;
		strncpy(call->remote_uri, rep->uri, BT_AUDIO_CALL_URI_LEN - 1);
		break;
	case BT_CALL_DIALING:
		call->state = BT_CALL_STATE_DIALING;
		break;
	case BT_CALL_ALERTING:
		call->state = BT_CALL_STATE_ALERTING;
		break;
	case BT_CALL_ACTIVE:
		call->state = BT_CALL_STATE_ACTIVE;
		break;
	case BT_CALL_LOCALLY_HELD:
		call->state = BT_CALL_STATE_LOCALLY_HELD;
		break;
	case BT_CALL_REMOTELY_HELD:
		call->state = BT_CALL_STATE_REMOTELY_HELD;
		break;
	case BT_CALL_HELD:
		call->state = BT_CALL_STATE_HELD;
		break;
	default:
		break;
	}

	/* report event */
report:
#ifdef CONFIG_BT_TRANSCEIVER
    bt_manager_event_notify(event, data, size);
#else
	if (audio_conn == find_active_slave()) {
		bt_manager_event_notify(event, data, size);
	} else {
		SYS_LOG_INF("drop msg %p %p", audio_conn, find_active_slave());
	}
#endif
}

static uint16_t get_le_call_status(struct bt_audio_conn *audio_conn)
{
	struct bt_audio_call_inst *call;
	uint16_t status;
	uint8_t count;

	status = BT_STATUS_HFP_NONE;

	os_sched_lock();

	if (sys_slist_is_empty(&audio_conn->ct_list)) {
		goto exit;
	}

	count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&audio_conn->ct_list, call, node) {
		count++;
	}

	if (count == 1) {
		call = SYS_SLIST_PEEK_HEAD_CONTAINER(&audio_conn->ct_list, call, node);
		switch (call->state) {
		case BT_CALL_STATE_INCOMING:
			status = BT_STATUS_INCOMING;
			break;
		case BT_CALL_STATE_DIALING:
		case BT_CALL_STATE_ALERTING:
			status = BT_STATUS_OUTGOING;
			break;
		case BT_CALL_STATE_ACTIVE:
			status = BT_STATUS_ONGOING;
			break;
		/* FIXME: what if held */
		default:
			break;
		}
	} else if (count > 1) {
		status = BT_STATUS_MULTIPARTY;
	}

exit:
	os_sched_unlock();

	return status;
}

int bt_manager_call_get_status(void)
{
	struct bt_audio_conn *audio_conn;
	struct bt_audio_conn *audio_conn2;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return BT_STATUS_HFP_NONE;
	}

	if (audio_conn->type == BT_TYPE_BR) {	
		audio_conn2 = find_conn_by_prio(BT_TYPE_LE, BT_AUDIO_PRIO_CALL);
		if (audio_conn2){
			return BT_STATUS_3WAYIN;
		}else{
			return audio_conn->call_state;
		}
	} else if (audio_conn->type == BT_TYPE_LE) {

		audio_conn2 = find_conn_by_prio(BT_TYPE_BR, BT_AUDIO_PRIO_CALL);
		if (audio_conn2){
			return BT_STATUS_3WAYIN;
		}else{
			return get_le_call_status(audio_conn);
		}
	}

	return BT_STATUS_HFP_NONE;
}

int bt_manager_volume_up(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_volume_up(audio_conn->handle);
	}

	return -EINVAL;
}

int bt_manager_volume_down(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_volume_down(audio_conn->handle);
	}

	return -EINVAL;
}

int bt_manager_volume_set(uint8_t value)
{
	struct bt_audio_conn *audio_conn;
#ifdef CONFIG_BT_TRANSCEIVER
    if(sys_get_work_mode())
        audio_conn = find_active_master(BT_TYPE_LE);
    else
        audio_conn = find_active_slave();
#else
	audio_conn = find_active_slave();
#endif
	if (!audio_conn) {
		return -EINVAL;
	}

	if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
		audio_conn->volume = value;
		uint8_t vcs_volume = (value >=16)? (255): (value*16);
		return bt_manager_le_volume_set(audio_conn->handle, vcs_volume);
#endif
	} else if (audio_conn->type == BT_TYPE_BR) {
		if (audio_conn->call_state != BT_STATUS_HFP_NONE) {
			bt_manager_hfp_sync_vol_to_remote(value);
		} else {
			bt_manager_avrcp_sync_vol_to_remote(value);
		}
	}

	return -EINVAL;
}

int bt_manager_volume_mute(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}
#ifdef CONFIG_BT_LE_AUDIO
	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_volume_mute(audio_conn->handle);
	}
#endif
	return -EINVAL;
}

int bt_manager_volume_unmute(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}
#ifdef CONFIG_BT_LE_AUDIO

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_volume_unmute(audio_conn->handle);
	}
#endif
	return -EINVAL;
}

int bt_manager_volume_unmute_up(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}
#ifdef CONFIG_BT_LE_AUDIO

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_volume_unmute_up(audio_conn->handle);
	}
#endif
	return -EINVAL;
}

int bt_manager_volume_unmute_down(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}
#ifdef CONFIG_BT_LE_AUDIO

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_volume_down(audio_conn->handle);
	}
#endif
	return -EINVAL;
}

int bt_manager_volume_client_up(void)
{
	struct bt_audio_conn *audio_conn;

	/* for each master */
	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_MASTER) {
			continue;
		}
#ifdef CONFIG_BT_LE_AUDIO

		if (audio_conn->type == BT_TYPE_LE) {
			bt_manager_le_volume_up(audio_conn->handle);
		}
#endif
	}

	return 0;
}

int bt_manager_volume_client_down(void)
{
	struct bt_audio_conn *audio_conn;

	/* for each master */
	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_MASTER) {
			continue;
		}
#ifdef CONFIG_BT_LE_AUDIO
		if (audio_conn->type == BT_TYPE_LE) {
			bt_manager_le_volume_down(audio_conn->handle);
		}
#endif
	}

	return 0;
}

int bt_manager_volume_client_set(uint8_t value)
{
	struct bt_audio_conn *audio_conn;

	/* for each master */
	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_MASTER) {
			continue;
		}
#ifdef CONFIG_BT_LE_AUDIO

		if (audio_conn->type == BT_TYPE_LE) {
			bt_manager_le_volume_set(audio_conn->handle, value);
		}
#endif
	}

	return 0;
}

int bt_manager_volume_client_mute(void)
{
	struct bt_audio_conn *audio_conn;

	/* for each master */
	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_MASTER) {
			continue;
		}
#ifdef CONFIG_BT_LE_AUDIO

		if (audio_conn->type == BT_TYPE_LE) {
			bt_manager_le_volume_mute(audio_conn->handle);
		}
#endif
	}

	return 0;
}

int bt_manager_volume_client_unmute(void)
{
	struct bt_audio_conn *audio_conn;

	/* for each master */
	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_MASTER) {
			continue;
		}
#ifdef CONFIG_BT_LE_AUDIO

		if (audio_conn->type == BT_TYPE_LE) {
			bt_manager_le_volume_unmute(audio_conn->handle);
		}
#endif
	}

	return 0;
}

int bt_manager_volume_client_unmute_up(void)
{
	struct bt_audio_conn *audio_conn;

	/* for each master */
	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_MASTER) {
			continue;
		}
#ifdef CONFIG_BT_LE_AUDIO

		if (audio_conn->type == BT_TYPE_LE) {
			bt_manager_le_volume_unmute_up(audio_conn->handle);
		}
#endif
	}

	return 0;
}

int bt_manager_volume_client_unmute_down(void)
{
	struct bt_audio_conn *audio_conn;

	/* for each master */
	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_MASTER) {
			continue;
		}
#ifdef CONFIG_BT_LE_AUDIO

		if (audio_conn->type == BT_TYPE_LE) {
			bt_manager_le_volume_unmute_down(audio_conn->handle);
		}
#endif
	}

	return 0;
}

void bt_manager_volume_event(int event, void *data, int size)
{
	struct bt_audio_conn *audio_conn;

	SYS_LOG_INF("%d", event);

	if (BT_VOLUME_VALUE == event)
	{
		struct bt_volume_report *rep = (struct bt_volume_report *)data;
		audio_conn = find_conn(rep->handle);
		if (!audio_conn) {
			SYS_LOG_ERR("no conn");
			return;
		}
		audio_conn->volume = rep->value;
		SYS_LOG_INF("audio_conn->volume: %d", audio_conn->volume);
		
		if (rep->from_phone){
		 	if (bt_manager_sync_volume_from_phone_leaudio(&(audio_conn->addr), audio_conn->volume) == 0){
				return;
			}
		}
	}

	bt_manager_event_notify(event, data, size);
}

void bt_manager_volume_event_to_app(uint16_t handle, uint8_t volume, uint8_t from_phone)
{
	struct bt_volume_report rep;
	rep.handle = handle;
	rep.value = volume;
	rep.from_phone = from_phone;
	
	bt_manager_event_notify(BT_VOLUME_VALUE,&rep, sizeof(rep));
}

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL

int bt_manager_volume_uac_gain(uint8_t gain)
{
    struct bt_audio_conn *audio_conn;

    audio_conn = find_active_master(BT_TYPE_LE);
    if (!audio_conn) {
        printk("le master null\n");
        return -EINVAL;
    }

#ifdef CONFIG_BT_LE_AUDIO
    if (audio_conn->type == BT_TYPE_LE) {
        uint8_t gain_buf[2] = {0};

		gain_buf[0] = BT_AUDIO_VND_SEND_ID_UAC_GAIN;
        gain_buf[1] = gain;
		return bt_manager_audio_le_vnd_send(audio_conn->handle, gain_buf, 2);
	}
#endif

	return -EINVAL;
}
#endif

int bt_manager_mic_mute(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}
#ifdef CONFIG_BT_LE_AUDIO

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_mic_mute(audio_conn->handle);
	}
#endif
	return -EINVAL;
}

int bt_manager_mic_unmute(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}
#ifdef CONFIG_BT_LE_AUDIO

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_mic_unmute(audio_conn->handle);
	}
#endif
	return -EINVAL;
}

int bt_manager_mic_mute_disable(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}
#ifdef CONFIG_BT_LE_AUDIO

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_mic_mute_disable(audio_conn->handle);
	}
#endif
	return -EINVAL;
}

int bt_manager_mic_gain_set(uint8_t gain)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();
	if (!audio_conn) {
		return -EINVAL;
	}
#ifdef CONFIG_BT_LE_AUDIO

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_mic_gain_set(audio_conn->handle, gain);
	}
#endif
	return -EINVAL;
}

int bt_manager_mic_client_mute(void)
{
	struct bt_audio_conn *audio_conn;

	/* for each master */
	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_MASTER) {
			continue;
		}
#ifdef CONFIG_BT_LE_AUDIO

		if (audio_conn->type == BT_TYPE_LE) {
			bt_manager_le_mic_mute(audio_conn->handle);
		}
#endif
	}

	return 0;
}

int bt_manager_mic_client_unmute(void)
{
	struct bt_audio_conn *audio_conn;

	/* for each master */
	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_MASTER) {
			continue;
		}
#ifdef CONFIG_BT_LE_AUDIO

		if (audio_conn->type == BT_TYPE_LE) {
			bt_manager_le_mic_unmute(audio_conn->handle);
		}
#endif
	}

	return 0;
}

int bt_manager_mic_client_gain_set(uint8_t gain)
{
	struct bt_audio_conn *audio_conn;

	/* for each master */
	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_MASTER) {
			continue;
		}
#ifdef CONFIG_BT_LE_AUDIO

		if (audio_conn->type == BT_TYPE_LE) {
			bt_manager_le_mic_gain_set(audio_conn->handle, gain);
		}
#endif
	}

	return 0;
}

void bt_manager_mic_event(int event, void *data, int size)
{
	SYS_LOG_INF("%d", event);

	bt_manager_event_notify(event, data, size);
}

void bt_manager_media_set_status(uint16_t hdl, uint16_t state)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_conn(hdl);
	if (!audio_conn) {
		return;
	}

	audio_conn->media_state = state;
}

void bt_manager_call_set_status(uint16_t hdl, uint16_t state)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_conn(hdl);
	if (!audio_conn) {
		return;
	}

	audio_conn->call_state = state;
}


int bt_manager_switch_active_app_check(void)
{
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();

	if (leaudio_config->app_auto_switch){
		return 0;
	}

	if (bt_audio.force_app_id == BT_AUDIO_APP_LE_AUDIO){
		return BT_AUDIO_APP_BR_MUSIC;
	}else if (bt_audio.force_app_id == BT_AUDIO_APP_BR_MUSIC){
		return BT_AUDIO_APP_LE_AUDIO;
	}else{
		return -1;
	}
}


int bt_manager_switch_active_app(void)
{
	struct bt_audio_conn *audio_conn;
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();

	if (leaudio_config->app_auto_switch){
		SYS_LOG_WRN("unsupported.");
		return 0;
	}

	bt_audio.interrupted_active_slave = NULL;

	if (bt_audio.force_app_id == BT_AUDIO_APP_LE_AUDIO){
		bt_audio.force_app_id = BT_AUDIO_APP_BR_MUSIC;
		audio_conn = find_conn_by_prio(BT_TYPE_BR, 0);
		if (!audio_conn){
			/*If unconnected Br, enter pair to wait connecting.*/
			bt_manager_enter_pair_mode();
		}else{
			bt_manager_audio_avrcp_play_for_leaudio(audio_conn->handle);
		}
	}else if (bt_audio.force_app_id == BT_AUDIO_APP_BR_MUSIC){
		bt_audio.force_app_id = BT_AUDIO_APP_LE_AUDIO;
		audio_conn = find_conn_by_prio(BT_TYPE_LE, 0);
		bt_audio.le_audio_pause_br_flag = 0;
	}else{
		return -1;
	}

	SYS_LOG_INF("Switch 0x%x -> 0x%x, app:%d", bt_audio.active_slave? bt_audio.active_slave->handle:0, 
		audio_conn? audio_conn->handle:0, bt_audio.force_app_id);
	
	bt_audio.active_slave = audio_conn;
	switch_slave_app(bt_audio.active_slave, bt_audio.force_app_id);
	
	return 0;
}

int bt_manager_resume_active_app(uint8_t bt_audio_app)
{
	struct bt_audio_conn *audio_conn = NULL;
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();

	if (leaudio_config->app_auto_switch){
		SYS_LOG_WRN("unsupported.");
		return 0;
	}

	bt_audio.interrupted_active_slave = NULL;

	if (bt_audio_app == BT_AUDIO_APP_BR_MUSIC){
        bt_audio.force_app_id = BT_AUDIO_APP_BR_MUSIC;
		audio_conn = find_conn_by_prio(BT_TYPE_BR, 0);
		if (!audio_conn){
			/*If unconnected Br, enter pair to wait connecting.*/
			bt_manager_enter_pair_mode();
		}else{
			bt_manager_audio_avrcp_play_for_leaudio(audio_conn->handle);
		}
	}else if (bt_audio_app == BT_AUDIO_APP_LE_AUDIO) {
	    bt_audio.force_app_id = BT_AUDIO_APP_LE_AUDIO;
		audio_conn = find_conn_by_prio(BT_TYPE_LE, 0);
		bt_audio.le_audio_pause_br_flag = 0;
    }else if (bt_audio_app == BT_AUDIO_APP_USOUND) {
	    bt_audio.force_app_id = BT_AUDIO_APP_USOUND;
	}else{
		return -1;
	}

	SYS_LOG_INF("Switch 0x%x -> 0x%x, app:%d", bt_audio.active_slave? bt_audio.active_slave->handle:0, 
		audio_conn? audio_conn->handle:0, bt_audio.force_app_id);
	
	bt_audio.active_slave = audio_conn;
	switch_slave_app(bt_audio.active_slave, bt_audio.force_app_id);
	
	return 0;
}


int bt_manager_switch_active_app_init(char* app_name)
{
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();

	if (leaudio_config->app_auto_switch){
		SYS_LOG_WRN("unsupported.");
		return 0;
	}

	if (!app_name) {
		bt_audio.force_app_id = BT_AUDIO_APP_UNKNOWN;
	} else if (!strcmp(app_name, APP_ID_BTMUSIC)) {
		bt_audio.force_app_id = BT_AUDIO_APP_BR_MUSIC;
	} else if (!strcmp(app_name, APP_ID_BTCALL)) {
		bt_audio.force_app_id = BT_AUDIO_APP_BR_CALL;
	} else if (!strcmp(app_name, APP_ID_LE_AUDIO)) {
		bt_audio.force_app_id = BT_AUDIO_APP_LE_AUDIO;
	} else {
		bt_audio.force_app_id = BT_AUDIO_APP_UNKNOWN;
	}

	SYS_LOG_INF("set init app id %s , %d", app_name,bt_audio.force_app_id); 
	
	return 0;
}

int bt_manager_get_active_app_id(void)
{
    return bt_audio.force_app_id;
}

bd_address_t* bt_manager_audio_lea_addr(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_conn_by_prio(BT_TYPE_LE, 0);
	if (!audio_conn){
		return NULL;
	}
	
	return &(audio_conn->addr);
}


int bt_manager_audio_update_volume(bd_address_t* addr, uint8_t vol)
{
	struct bt_audio_conn *audio_conn;
	bd_address_t tmp_addr;
	
	memset(&tmp_addr,0,sizeof(bd_address_t));

	if(!memcmp(addr, &tmp_addr, sizeof(bd_address_t))){
		return 0;
	}

	audio_conn = find_conn_by_addr(addr);
	if (!audio_conn){
		bt_manager_audio_save_dev_volume(addr, vol);		
		return 0;
	}
	audio_conn->volume = vol;
	return 0;
}

int bt_manager_audio_tws_connected(void)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_conn_by_prio(BT_TYPE_LE, 0);
	if (!audio_conn){
		return 0;
	}
	bt_manager_sync_volume_from_phone_leaudio(&(audio_conn->addr), audio_conn->volume);
	return 0;
}

int bt_manager_audio_tws_disconnected(void)
{
	bt_audio.remote_active_app = 0;
	return 0;
}

int bt_manager_audio_leaudio_dev_cb_register(leaudio_device_callback_t cb)
{
	bt_leaudio_device_cb = cb;
	return 0;
}

int bt_manager_get_connected_audio_dev_num(void)
{
	struct bt_audio_conn *audio_conn;
	int num = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
		if (audio_conn->role != BT_ROLE_SLAVE || audio_conn->is_tws) {
			continue;
		}

		if (audio_conn->type == BT_TYPE_BR){
			num++;
		}else if (audio_conn->type == BT_TYPE_LE && audio_conn->is_lea){
			num++;
		}	
	}

	return num;
}

int bt_manager_audio_get_recently_connectd(uint32_t *remain_period)
{
	int ret = false;
	os_sched_lock();
	if (bt_audio.recently_connected_flag) {
		uint32_t cur_time = os_uptime_get_32();
		if (cur_time - bt_audio.recently_connected_timestamp < CONNECTED_DELAY_PLAY_TIMER_MS) {
			*remain_period = CONNECTED_DELAY_PLAY_TIMER_MS - (cur_time - bt_audio.recently_connected_timestamp);
			ret = true;
		}
	}
	os_sched_unlock();
	return ret;
}

int bt_manager_audio_in_btcall(void)
{
	/*If br calling, don't advertising LEA */
	if ((bt_audio.active_slave != NULL) && (bt_audio.active_slave->prio & BT_AUDIO_PRIO_CALL)){
		return 1;
	}

	if (get_current_slave_app() == BT_AUDIO_APP_BR_CALL){
		return 1;
	}

	return 0;
}

int bt_manager_audio_in_btmusic(void)
{
	if (get_current_slave_app() == BT_AUDIO_APP_BR_MUSIC){
		return 1;
	}

	return 0;
}

int bt_manager_audio_entering_btcall(void)
{
	return bt_audio.entering_btcall_flag?1:0;
}

#ifdef CONFIG_XEAR_HID_PROTOCOL
int tr_bt_manager_xear_set(uint8_t featureId, uint8_t value)
{
	struct bt_audio_conn *audio_conn;

	audio_conn = find_active_slave();

	if (!audio_conn) {
		SYS_LOG_INF("audio_conn null\n");
		return -EINVAL;
	}
#ifdef CONFIG_XEAR_AUTHENTICATION
	if(xear_get_status()==XEAR_STATUS_DISABLE) {
		SYS_LOG_INF("xear disabled\n");
		return -EINVAL;
	}
#else
	SYS_LOG_INF("xear authentication not enabled\n");
	return -EINVAL;
#endif

	if (audio_conn->type == BT_TYPE_LE) {
		return bt_manager_le_xear_set(featureId, value);
	}

	return -EINVAL;
}
#endif


