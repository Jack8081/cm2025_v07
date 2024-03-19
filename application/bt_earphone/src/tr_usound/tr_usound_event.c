/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tr_usound event
 */

#include "tr_usound.h"
#include "tts_manager.h"

#ifdef CONFIG_XEAR_AUTHENTICATION
#include <usb/usb_custom_hid_info.h>
#include <xear_authentication.h>
#endif

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif
#ifdef CONFIG_TR_USOUND_DOUBLE_SOUND_CARD
extern int soundcard1_soft_vol;
extern int soundcard2_soft_vol;
#endif
#ifdef CONFIG_SUPPORT_VOL_SYNC_HOST_TO_DEV
static u8_t dev_num = 0;
#endif
//static u8_t uac_upload_cmd = 0; //pc set upload interface flag 
typedef struct
{
	uint32_t enbale_share_info : 1;
	uint32_t dump_crc_error_pkt : 1;
	uint32_t recv_pkt_size : 10;
	uint32_t send_pkt_size : 10;
	uint32_t recv_pkt_triggle_enable : 1;
	uint32_t send_pkt_triggle_enable : 1;
	uint32_t recv_pkt_report_enable : 1;
	uint32_t send_pkt_report_enable : 1;

	uint16_t sink_handle;
	uint16_t source_handle;

	int32_t tx_anchor_offset_us;
	int32_t rx_anchor_offset_us;

	struct bt_dsp_cis_share_info *cis_share_info;
} cis_xfer_bypass_config_t;

extern cis_xfer_bypass_config_t cis_xfer_bypass_cfg;

static void _tr_usound_set_share_info(uint16_t handle, bool enable, bool update)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    struct _le_dev_t *dev = NULL;
    struct bt_audio_chan *chan = tr_usound_find_chan_by_dir(0, MIC_CHAN);
    if(chan) {
        dev = tr_usound_find_dev(chan->handle);
    }

	if (!enable) {
		if (thread_timer_is_running(&tr_usound->delay_play_timer)) {
			thread_timer_stop(&tr_usound->delay_play_timer);
		}
	}

	SYS_LOG_INF("enable %d, 0x%p", enable, tr_usound->cis_share_info);
	cis_xfer_bypass_cfg.enbale_share_info = enable;
	if (enable) {
		SYS_LOG_INF("cis_share_info %p, %p, %p, %p, %p", 
            tr_usound->cis_share_info->recvbuf, 
            tr_usound->cis_share_info->recv_report_buf,
            tr_usound->cis_share_info->sendbuf,
			tr_usound->cis_share_info->send_report_buf,
            tr_usound->cis_share_info->dump_buf);
		
		cis_xfer_bypass_cfg.cis_share_info = tr_usound->cis_share_info;
		cis_xfer_bypass_cfg.dump_crc_error_pkt = CONFIG_DUMP_CRC_ERROR_PKT; //0;
		cis_xfer_bypass_cfg.recv_pkt_size = MAX_RECV_PKT_SIZE; //128;
		cis_xfer_bypass_cfg.send_pkt_size = MAX_SEND_PKT_SIZE; //256;
		cis_xfer_bypass_cfg.recv_pkt_triggle_enable = 0;
        cis_xfer_bypass_cfg.send_pkt_triggle_enable = 1;

		cis_xfer_bypass_cfg.tx_anchor_offset_us = TX_ANCHOR_OFFSET_US;
		cis_xfer_bypass_cfg.rx_anchor_offset_us = 0;

		if (dev && dev->audio_handle == handle && tr_usound->playback_player_run) {
            cis_xfer_bypass_cfg.recv_pkt_report_enable = 1;
        }
		else
			cis_xfer_bypass_cfg.recv_pkt_report_enable = 0;
		if (tr_usound->capture_player_run)
			cis_xfer_bypass_cfg.send_pkt_report_enable = 1;
		else
			cis_xfer_bypass_cfg.send_pkt_report_enable = 0;

//*************************
//临时修改
//需要设置多个stream handle
//
        SYS_LOG_INF("audio_handle 0x%x, send triggle %d, recv triggle %d\n", handle,
            cis_xfer_bypass_cfg.send_pkt_triggle_enable, cis_xfer_bypass_cfg.recv_pkt_report_enable);

        cis_xfer_bypass_cfg.sink_handle = handle;
        cis_xfer_bypass_cfg.source_handle = handle;
	}

	bt_manager_send_mailbox_sync(0x01, (uint8_t *)&cis_xfer_bypass_cfg, sizeof(cis_xfer_bypass_cfg));

	if (enable)
		tr_usound->set_share_info_flag = 1;
	else
		tr_usound->set_share_info_flag = 0;
}

void tr_usound_set_share_info(bool enable, bool update)
{
    struct tr_usound_app_t *tr_usound = tr_usound_get_app();

    if(tr_usound) {
        for(int i = 0; i < ARRAY_SIZE(tr_usound->dev); i++) {
            if(tr_usound->dev[i].handle && tr_usound->dev[i].audio_handle) {
                _tr_usound_set_share_info(tr_usound->dev[i].audio_handle, enable, update);
                k_sleep(K_MSEC(50));
            }
        }
    }
}

static void tr_usound_set_share_info_delay_play_proc(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    uint32_t audio_handle = (uint32_t)expiry_fn_arg;
	SYS_LOG_INF("exec %x\n", audio_handle);
	_tr_usound_set_share_info((uint16_t)audio_handle, true, false);
}

static void tr_usound_restore(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    //no support tts
}

static int tr_usound_handle_config_qos(struct bt_audio_qos_configured *qos)
{
	struct bt_audio_chan *chan;
	int dir;
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	dir = bt_manager_audio_stream_dir(qos->handle, qos->id);
    chan = tr_usound_dev_add_chan(qos->handle, dir, qos->id);

    if(!chan) {
        SYS_LOG_ERR("\n");
    }

	if (dir == SPK_CHAN) {
        tr_bt_manager_audio_stream_enable_single(chan, BT_AUDIO_CONTEXT_UNSPECIFIED);
	}

    if (dir == MIC_CHAN && tr_usound->upload_start) {
        chan = tr_usound_find_chan_by_dir(0, MIC_CHAN);
        if(chan && chan->handle)
        {
            tr_bt_manager_audio_stream_enable_single(chan, BT_AUDIO_CONTEXT_UNSPECIFIED);
        }
    }

	SYS_LOG_INF("handle: %d, id: %d, dir: %d\n", qos->handle,
		qos->id, dir);

	return 0;
}

static void tr_usound_handle_enable_share_info(struct tr_usound_app_t *tr_usound, uint32_t audio_handle)
{
	uint32_t remain_period = 0;
	if (bt_manager_audio_get_recently_connectd(&remain_period)) {
		if (thread_timer_is_running(&tr_usound->delay_play_timer)) {
			thread_timer_stop(&tr_usound->delay_play_timer);
		}
		thread_timer_init(&tr_usound->delay_play_timer, tr_usound_set_share_info_delay_play_proc, (void *)audio_handle);
		thread_timer_start(&tr_usound->delay_play_timer, remain_period, 0);
		SYS_LOG_INF("delay_play_proc set %d", remain_period);
	} else {
		_tr_usound_set_share_info((uint16_t)audio_handle, true, false);
	}
}

static int tr_usound_handle_enable(struct bt_audio_report *rep)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    struct bt_audio_chan *chan = tr_usound_find_chan_by_id(rep->handle, rep->id);
    uint8_t dir = tr_usound_get_chan_dir(rep->handle, rep->id);
    struct _le_dev_t *dev = tr_usound_find_dev(rep->handle);
	struct bt_audio_chan_info chan_info;

	if (tr_usound->tts_playing) {
		SYS_LOG_INF("tts\n");
		return 0;
	}

    if(dev == NULL) {
        SYS_LOG_ERR("no dev\n");
        return -1;
    }

    if(bt_manager_audio_stream_info(chan, &chan_info) == 0) {
        dev->audio_handle = chan_info.audio_handle;
        SYS_LOG_INF("audio_handle :%x", chan_info.audio_handle);
    } else {
        SYS_LOG_ERR("no chan\n");
        return -1;
    }

	/* context switch : call or others/music */
	SYS_LOG_INF("contexts %d", rep->audio_contexts);

	if (dir == MIC_CHAN) {
		SYS_LOG_INF("sink\n");
		tr_usound_init_playback();
		tr_usound_init_capture();

		tr_usound_handle_enable_share_info(tr_usound, dev->audio_handle);
		return 0;
	}

	if (dir == SPK_CHAN) {
		SYS_LOG_INF("source\n");
		tr_usound_init_capture();
		tr_usound_init_playback();

		tr_usound_handle_enable_share_info(tr_usound, dev->audio_handle);

		return 0;
	}


	return -EINVAL;
}

static int tr_usound_handle_start(struct bt_audio_report *rep)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    struct _le_dev_t *dev = tr_usound_find_dev(rep->handle);
    uint8_t dir = tr_usound_get_chan_dir(rep->handle, rep->id);

    if(dev) {
        if (dir == SPK_CHAN) {
            SYS_LOG_INF("source_chan started\n");
            dev->spk_chan_started = 1;

#ifdef CONFIG_DONGLE_SUPPORT_TWS
            if(tr_usound->capture_player)
            {
                struct bt_audio_chan_info chan_info;
                int ret = bt_manager_audio_stream_info(&dev->chan[SPK_CHAN], &chan_info);
                if (ret) {
                    return ret;
                }

                if(chan_info.locations == BT_AUDIO_LOCATIONS_FR)
                    media_player_set_right_chan_handle(tr_usound->capture_player, &chan_info.audio_handle);
                else
                    media_player_set_left_chan_handle(tr_usound->capture_player, &chan_info.audio_handle);
            }
#endif

            if (!tr_usound->tts_playing)
            {
                tr_usound_start_capture();
            }
        }

        if (dir == MIC_CHAN) {
            SYS_LOG_INF("sink_chan started\n");
            tr_usound_start_playback();
            dev->mic_chan_started = 1;
        }
    }
	return 0;
}

//可能存在多个连接,增加该接口主要判断所有设备都满足stop条件时才关闭 capture
static int tr_usound_try_stop_capture(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    struct _le_dev_t *dev;

    if(tr_usound) {
        for(int i = 0; i < ARRAY_SIZE(tr_usound->dev); i++) {
            dev = &tr_usound->dev[i];
            if(dev->handle && dev->audio_handle) {
                if(dev->spk_chan_started)
                    return -1;
            }
        }
    }

    tr_usound_stop_capture();
    return 0;
}

static int tr_usound_handle_stop(struct bt_audio_report *rep)
{
    struct _le_dev_t *dev = tr_usound_find_dev(rep->handle);
    uint8_t dir = tr_usound_get_chan_dir(rep->handle, rep->id);

    if(dev) {
        if (dir == MIC_CHAN) {
            tr_usound_stop_playback();
            dev->mic_chan_started = 0;
        }

        if (dir == SPK_CHAN) {
            dev->spk_chan_started = 0;
            tr_usound_try_stop_capture();
        }

        if(dev->mic_chan_started == 0 && dev->spk_chan_started == 0)
        {
            if(dev->chan[0].handle)
                bt_manager_audio_stream_disconnect_single(&dev->chan[0]);
            else if(dev->chan[1].handle)
                bt_manager_audio_stream_disconnect_single(&dev->chan[1]);

            SYS_LOG_INF("disc cis");
            k_sleep(K_MSEC(500));
        }
    }
    return 0;
}

static int tr_usound_handle_release(struct bt_audio_report *rep)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    uint8_t dir = tr_usound_get_chan_dir(rep->handle, rep->id);

	if (dir == MIC_CHAN) {
		tr_usound_stop_playback();
	}

	if (dir == SPK_CHAN) {
		tr_usound_try_stop_capture();
	}

	if ((!tr_usound->playback_player || !tr_usound->playback_player_run)
		&& (!tr_usound->capture_player || !tr_usound->capture_player_run)) {
		tr_usound_set_share_info(false, false);
		tr_usound_exit_playback();
		tr_usound_exit_capture();
	}

	return 0;
}

static int tr_usound_restore_state(struct bt_audio_chan *chan)
{
	struct bt_audio_chan_info info;
	struct bt_audio_report rep;
	int state;
	int ret;

	ret = bt_manager_audio_stream_info(chan, &info);
	if (ret < 0) {
		return ret;
	}

	state = bt_manager_audio_stream_state(chan->handle, chan->id);
	if (state < 0) {
		return ret;
	}

	switch (state) {
	case BT_AUDIO_STREAM_ENABLED:
		rep.handle = chan->handle;
		rep.id = chan->id;
		rep.audio_contexts = info.contexts;
		tr_usound_handle_enable(&rep);
		break;
	case BT_AUDIO_STREAM_STARTED:
		rep.handle = chan->handle;
		rep.id = chan->id;
		rep.audio_contexts = info.contexts;
		/* FIXME: sink_chan no need to stream_start again! */
		tr_usound_handle_enable(&rep);
		tr_usound_handle_start(&rep);
		break;
	default:
		break;
	}

	return 0;
}

static int tr_usound_handle_restore(struct bt_audio_chan_restore *res)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    struct _le_dev_t *dev;
	struct bt_audio_chan *chan;
    struct bt_audio_chan_info info;
    uint8_t source, sink;
	uint8_t i;
	int dir;

	if (!res) {
		SYS_LOG_ERR("NULL");
		return -EINVAL;
	}

	/* NOTICE: should be 0/1/2 */
	if (res->count > 2) {
		SYS_LOG_WRN("count: %d", res->count);
	}

	SYS_LOG_INF("\n");

	tr_usound_stop_playback();

	tr_usound_stop_capture();

	if ((!tr_usound->playback_player || !tr_usound->playback_player_run)
		&& (!tr_usound->capture_player || !tr_usound->capture_player_run)) {
		tr_usound_set_share_info(false, false);
		tr_usound_exit_playback();
		tr_usound_exit_capture();
	}

	source = sink = 0;
	for (i = 0; i < res->count; i++) {
		chan = &res->chans[i];

		dir = bt_manager_audio_stream_dir(chan->handle, chan->id);
        bt_manager_audio_stream_info(chan, &info);

        dev = tr_usound_find_dev(chan->handle);
        if(dev == NULL)
        {
            dev = tr_usound_dev_add(chan->handle);
        }

        if(dev)
        {
            if (dir == MIC_CHAN) {
                sink = 1;
                dev->chan[MIC_CHAN].handle = chan->handle;
                dev->chan[MIC_CHAN].id = chan->id;
                dev->audio_handle = info.audio_handle;
                SYS_LOG_INF("sink_audio_handle :%d", dev->audio_handle);
            } else if (dir == SPK_CHAN) {
                source = 1;
                dev->chan[SPK_CHAN].handle = chan->handle;
                dev->chan[SPK_CHAN].id = chan->id;
                dev->audio_handle = info.audio_handle;
                SYS_LOG_INF("source_audio_handle :%d", dev->audio_handle);
            }
        }

		SYS_LOG_INF("handle: %d, id: %d, dir: %d\n", chan->handle,
			chan->id, dir);
	}

	/* wait for tts complete */
	if (tr_usound->tts_playing) {
		SYS_LOG_INF("tts\n");
		return 0;
	}

	if (source == 1) {
		SYS_LOG_INF("source\n");
		tr_usound_restore_state(&dev->chan[SPK_CHAN]);
	}

	if (sink == 1) {
		SYS_LOG_INF("sink\n");
		tr_usound_restore_state(&dev->chan[MIC_CHAN]);
	}

	SYS_LOG_INF("done\n");

	return 0;
}

static int tr_usound_handle_switch(void)
{
	return bt_manager_audio_stream_restore(BT_TYPE_LE);
}

static void tr_usound_handle_connect(uint16_t *handle)
{
	uint8_t type;
	uint8_t role;

	/*
	 * validity check
	 */
	type = bt_manager_audio_get_type(*handle);
	if (type != BT_TYPE_LE) {
		SYS_LOG_ERR("type: %d\n", type);
		return;
	}

	role = bt_manager_audio_get_role(*handle);
	if (role != BT_ROLE_MASTER) {
		SYS_LOG_ERR("role: %d\n", role);
		return;
	}

    tr_usound_dev_add(*handle);
}

static void tr_usound_handle_disconnect(uint16_t *handle)
{
    if(tr_usound_find_dev(*handle)) {
        tr_usound_dev_del(*handle);
    }
}

static int tr_usound_handle_volume_mute(struct bt_volume_report *rep)
{
	SYS_LOG_INF("\n");

	return audio_system_set_stream_mute(AUDIO_STREAM_LE_AUDIO_MUSIC, true);
}

static int tr_usound_handle_volume_unmute(struct bt_volume_report *rep)
{
	SYS_LOG_INF("\n");

	return audio_system_set_stream_mute(AUDIO_STREAM_LE_AUDIO_MUSIC, false);
}

static int tr_usound_handle_mic_mute(struct bt_mic_report *rep)
{
	SYS_LOG_INF("\n");

	return audio_system_mute_microphone(1);
}

static int tr_usound_handle_mic_unmute(struct bt_mic_report *rep)
{
	SYS_LOG_INF("\n");

	return audio_system_mute_microphone(0);
}

static u8_t vol_to_level(u8_t value)
{
    u8_t vol_level;
    if(value == 0 || value == 255)
    {
        vol_level = (value+1)/16;
    }
    else
    {
        vol_level = (value+17)/17;
    }
    return vol_level;
}

static void tr_usound_handle_volume_value(struct bt_volume_report *rep)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    struct _le_dev_t *dev = tr_usound_find_dev(rep->handle);
    u8_t volume_level = vol_to_level(rep->value);

    if(dev) {
        dev->vcs_connected = 1;
    }
#ifdef CONFIG_SUPPORT_VOL_SYNC_HOST_TO_DEV
    if(dev) {
        if(!dev->client_vol_connected)
            return;
    }

    if(tr_usound->dev_num != dev_num)
        return;
#endif
    SYS_LOG_INF("bt_earphone vol sync level: %d (%d)\n",rep->value, volume_level);
#ifdef CONFIG_TR_USOUND_DOUBLE_SOUND_CARD
    soundcard1_soft_vol = volume_level; 
    soundcard2_soft_vol = 16 - volume_level;
    SYS_LOG_INF("soundcard1_vol:%d, soundcard2_vol:%d\n", soundcard1_soft_vol, soundcard2_soft_vol);
    return;
#endif
    if(tr_usound->current_volume_level > volume_level)
    {
        tr_usound->volume_req_type = USOUND_VOLUME_DEC;
	    tr_usound->volume_req_level = volume_level;
        tr_system_volume_set(AUDIO_STREAM_TR_USOUND, tr_usound->volume_req_level, false, false);
	    usb_hid_control_volume_dec();
    }
    else if(tr_usound->current_volume_level < volume_level)
    {
	    tr_usound->volume_req_type = USOUND_VOLUME_INC;
	    tr_usound->volume_req_level = volume_level;
        tr_system_volume_set(AUDIO_STREAM_TR_USOUND, tr_usound->volume_req_level, false, false);
	    usb_hid_control_volume_inc();
    }
    else
    {
        return;
    }
	
	tr_usound->current_volume_level = tr_usound->volume_req_level;
}

void tr_usound_bt_event_proc(struct app_msg *msg)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	SYS_LOG_INF("%d\n", msg->cmd);
	switch (msg->cmd) {

	case BT_CONNECTED:
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
        if (thread_timer_is_running(&tr_usound->auto_search_timer))
		    thread_timer_stop(&tr_usound->auto_search_timer);
        ui_event_led_display(UI_EVENT_CUSTOMED_5);
#endif
		tr_usound_handle_connect(msg->ptr);
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
        thread_timer_start(&tr_usound->usb_cis_start_stop_work, CONFIG_USOUND_DISCONNECT_CIS_WAIT_TIME, 0);
#endif

#ifdef CONFIG_XEAR_AUTHENTICATION
        {
        // notify Rx about xear status
        u8_t buf[2] = {INFO_ID_XEAR_STATUS };
        buf[1] = xear_get_status();
        hid_info_data_to_send(buf, sizeof(buf));
        }
#endif
		break;
	case BT_DISCONNECTED:
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
        ui_event_led_display(UI_EVENT_CUSTOMED_4);
#endif
		tr_usound_handle_disconnect(msg->ptr);

        tr_usound->downchan_mute_data_detect_disable = 0;
#ifdef CONFIG_SUPPORT_VOL_SYNC_HOST_TO_DEV
        dev_num--;
#endif
        break;
	case BT_AUDIO_STREAM_CONFIG_QOS:
		tr_usound_handle_config_qos(msg->ptr);
		break;
	case BT_AUDIO_STREAM_ENABLE:
		tr_usound_handle_enable(msg->ptr);
		break;
	case BT_AUDIO_STREAM_START:
		tr_usound_handle_start(msg->ptr);
		break;
	case BT_AUDIO_STREAM_STOP:
		tr_usound_handle_stop(msg->ptr);
		break;
	case BT_AUDIO_STREAM_RELEASE:
		tr_usound_handle_release(msg->ptr);
		break;
	case BT_AUDIO_STREAM_RESTORE:
		tr_usound_handle_restore(msg->ptr);
		break;
     
    case BT_VOLUME_CLIENT_CONNECTED:
        {
#ifdef CONFIG_SUPPORT_VOL_SYNC_HOST_TO_DEV
            struct bt_volume_report *rep = (struct bt_volume_report *)msg->ptr ;
            struct _le_dev_t *dev = tr_usound_find_dev(rep->handle);
            uint8_t vcs_volume = (tr_usound->current_volume_level >=16)? (255): (tr_usound->current_volume_level*16);
            int bt_manager_le_volume_set(uint16_t handle, uint8_t value);
            bt_manager_le_volume_set(rep->handle, vcs_volume);
            tr_system_volume_set(AUDIO_STREAM_TR_USOUND, tr_usound->current_volume_level, false, false);
            dev->client_vol_connected = 1;
            dev_num++;
            printk("sync_host_vol:%d\n", tr_usound->current_volume_level);
#endif
        }
        break;
	case BT_VOLUME_CLIENT_VALUE:
		tr_usound_handle_volume_value(msg->ptr);
		break;
	case BT_VOLUME_CLIENT_MUTE:
		tr_usound_handle_volume_mute(msg->ptr);
		break;
	case BT_VOLUME_CLIENT_UNMUTE:
		tr_usound_handle_volume_unmute(msg->ptr);
		break;
	case BT_MIC_MUTE:
		tr_usound_handle_mic_mute(msg->ptr);
		break;
	case BT_MIC_UNMUTE:
		tr_usound_handle_mic_unmute(msg->ptr);
		break;

	case BT_AUDIO_SWITCH:
		tr_usound_handle_switch();
		break;

    case BT_MEDIA_SERVER_NEXT_TRACK:
        usb_hid_control_play_next();
        break;
    case BT_MEDIA_SERVER_PREV_TRACK:
        usb_hid_control_play_prev();
        break;
    
    case BT_VOLUME_CLIENT_UP:
        break;
    case BT_VOLUME_CLIENT_DOWN:
        break;

    case BT_VOLUME_UAC_GAIN:
        SYS_LOG_INF("BT_VOLUME_UAC_GAIN\n");
        usb_audio_download_gain_set(msg->reserve);
        break;
	
    case BT_MEDIA_SERVER_PLAY:
	case BT_MEDIA_SERVER_PAUSE:
	    usb_hid_control_pause_play();
        break;

    case BT_MIC_CLIENT_MUTE:
        SYS_LOG_INF("BT_MIC_CLIENT_MUTE\n");

        if(!tr_usound->host_hid_mic_muted && tr_usound->voice_state != VOICE_STATE_NULL)
        {
            tr_usound->host_hid_mic_muted = 1;
            usb_hid_mute();
        }
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
        //os_delayed_work_submit(&tr_usound->mic[0].usb_cis_start_stop_work, OS_SECONDS(CONFIG_USOUND_DISCONNECT_CIS_WAIT_TIME));
#endif
        break;
    case BT_MIC_CLIENT_UNMUTE:
        SYS_LOG_INF("BT_MIC_CLIENT_UNMUTE\n");

        if(tr_usound->host_hid_mic_muted && tr_usound->voice_state != VOICE_STATE_NULL)
        {
            tr_usound->host_hid_mic_muted = 0;
            usb_hid_mute();
        }
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
        //os_delayed_work_submit(&tr_usound->mic[0].usb_cis_start_stop_work, 0);
#endif
        break;


    case BT_CALL_INCOMING:
        SYS_LOG_INF("BT_CALL_INCOMING\n");
        break;
    case BT_CALL_DIALING:	/* outgoing */
        SYS_LOG_INF("BT_CALL_DIALING\n");
        break;
    case BT_CALL_ALERTING:	/* outgoing */
        SYS_LOG_INF("BT_CALL_ALERTING\n");
        tr_bt_manager_call_remote_accept(NULL);
        break;
    case BT_CALL_ACTIVE:
        SYS_LOG_INF("BT_CALL_ACTIVE\n");
        usb_hid_answer();
        break;
    case BT_CALL_ENDED:
        SYS_LOG_INF("BT_CALL_ENDED: %d -> %d\n", tr_usound->voice_state_last, tr_usound->voice_state);
        if(tr_usound->voice_state != VOICE_STATE_NULL || (tr_usound->voice_state == VOICE_STATE_NULL && tr_usound->voice_state_last != VOICE_STATE_INGOING))
            usb_hid_refuse_phone();
        break;
    case BT_APP_CTRL:
        SYS_LOG_INF("APP_CTRL msg->value:%x", msg->value);

        if(msg->value == 1) {
            tr_usound->downchan_mute_data_detect_disable = 1;
        } else if(msg->value == 0) {
            tr_usound->downchan_mute_data_detect_disable = 0;
        }
        break;
	default:
		break;
	}
}


void tr_usound_tts_event_proc(struct app_msg *msg)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		tr_usound->tts_playing = 1;
		if (thread_timer_is_running(&tr_usound->tts_monitor_timer)) {
			thread_timer_stop(&tr_usound->tts_monitor_timer);
		}

		tr_usound_stop_playback();
		tr_usound_exit_playback();
		tr_usound_stop_capture();
		tr_usound_exit_capture();

		break;
	case TTS_EVENT_STOP_PLAY:
		tr_usound->tts_playing = 0;
		if (thread_timer_is_running(&tr_usound->tts_monitor_timer)) {
			thread_timer_stop(&tr_usound->tts_monitor_timer);
		}
		thread_timer_init(&tr_usound->tts_monitor_timer, tr_usound_restore, NULL);
		thread_timer_start(&tr_usound->tts_monitor_timer, 1000, 0);
		break;
	}
}

static void _tr_usound_host_vol_sync_cb(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    static u8_t sync_vol;
    
    sync_vol = tr_usound->current_volume_level;
    SYS_LOG_INF("host sync vol:%d\n", sync_vol);
    
    tr_system_volume_set(AUDIO_STREAM_TR_USOUND, sync_vol, false, tr_usound->dev[0].vcs_connected);
    tr_usound->host_vol_sync = 0;
}

void tr_usound_event_proc(struct app_msg *msg)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    if (!tr_usound) {
		return;
	}

	SYS_LOG_INF("\n--- cmd %d ---\n", msg->cmd);

	switch (msg->cmd) {
	case MSG_USOUND_STREAM_START:
		//tr_usound_sink_start();
        tr_usound->usb_download_started = 1;
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
        thread_timer_start(&tr_usound->usb_cis_start_stop_work, 0, 0);
#endif
        break;

	case MSG_USOUND_STREAM_STOP:
		//tr_usound_sink_stop();
        tr_usound->usb_download_started = 0;
#ifdef CONFIG_TR_USOUND_START_STOP_CIS_DETECT
        thread_timer_start(&tr_usound->usb_cis_start_stop_work, CONFIG_USOUND_DISCONNECT_CIS_WAIT_TIME, 0);
#endif
        break;

	case MSG_USOUND_UPLOAD_STREAM_START: {
        struct bt_audio_chan *upload_chan = tr_usound_find_chan_by_dir(0, MIC_CHAN);
        tr_usound->upload_start = 1;
        if(upload_chan && upload_chan->handle)
        {
            tr_bt_manager_audio_stream_enable_single(upload_chan, BT_AUDIO_CONTEXT_UNSPECIFIED);
        }
        break;
    }
	case MSG_USOUND_UPLOAD_STREAM_STOP: {
        struct bt_audio_chan *upload_chan = tr_usound_find_chan_by_dir(0, MIC_CHAN);
        if(upload_chan && upload_chan->handle) {
            tr_bt_manager_audio_stream_disable_single(upload_chan);
        }
        tr_usound->upload_start = 0;
        break;
    }
    case MSG_USOUND_STREAM_MUTE:
        SYS_LOG_INF("Host Set Mute");
        usb_audio_set_stream_mute(1);
        tr_usound->volume_req_type = USOUND_VOLUME_NONE;
        break;
	
    case MSG_USOUND_STREAM_UNMUTE:
        SYS_LOG_INF("Host Set Unmute");
        if(tr_usound->usb_download_stream)
            usb_audio_set_stream(tr_usound->usb_download_stream);

        usb_audio_set_stream_mute(0);
        break;

    case MSG_USOUND_MUTE_SHORTCUT:
        break;

    case MSG_USOUND_HOST_VOL_SYNC:

        if (thread_timer_is_running(&tr_usound->host_vol_sync_timer)) {
			thread_timer_stop(&tr_usound->host_vol_sync_timer);
		}
		thread_timer_init(&tr_usound->host_vol_sync_timer, _tr_usound_host_vol_sync_cb, NULL);
		thread_timer_start(&tr_usound->host_vol_sync_timer, 100, 0);
            
        tr_usound->host_vol_sync = 1;
        break;

    case MSG_USOUND_INCALL_RING:
        SYS_LOG_INF("MSG_USOUND_INCALL_RING\n");
#ifdef CONFIG_BT_LE_AUDIO
        tr_usound->voice_state_last = tr_usound->voice_state;
        tr_usound->voice_state = VOICE_STATE_INGOING;

        tr_bt_manager_call_remote_originate(0, NULL);
#endif
        break;

    case MSG_USOUND_INCALL_OUTGOING:
        SYS_LOG_INF("MSG_USOUND_INCALL_OUTGOING\n");
#ifdef CONFIG_BT_LE_AUDIO
        tr_usound->voice_state_last = tr_usound->voice_state;
        tr_usound->voice_state = VOICE_STATE_OUTGOING;
        tr_bt_manager_call_originate(0, "actions:zs303ah");
        tr_bt_manager_call_remote_alert(NULL);
#endif
        break;

    case MSG_USOUND_INCALL_HOOK_UP:
        SYS_LOG_INF("MSG_USOUND_INCALL_HOOK_UP\n");
#ifdef CONFIG_BT_LE_AUDIO
        tr_usound->voice_state_last = tr_usound->voice_state;
        tr_usound->voice_state = VOICE_STATE_ONGOING;
        tr_bt_manager_call_remote_accept(NULL);
#endif
        break;

    case MSG_USOUND_INCALL_HOOK_HL:
        SYS_LOG_INF("MSG_USOUND_INCALL_HOOK_HL\n");
#ifdef CONFIG_BT_LE_AUDIO
        if(tr_usound->voice_state_last && tr_usound->voice_state == VOICE_STATE_NULL) break;

        tr_usound->voice_state_last = tr_usound->voice_state;
        tr_usound->voice_state = VOICE_STATE_NULL;

        if(tr_usound->host_hid_mic_muted)
        {
            tr_usound->host_hid_mic_muted = 0;
            //tr_bt_manager_mic_unmute();
            bt_manager_mic_client_unmute();
        }
        tr_bt_manager_call_remote_terminate(NULL);
#endif
        break;

    case MSG_USOUND_INCALL_MUTE:
        SYS_LOG_INF("MSG_USOUND_INCALL_MUTE\n");
#ifdef CONFIG_BT_LE_AUDIO
        if(!tr_usound->host_hid_mic_muted)
        {
            tr_usound->host_hid_mic_muted = 1;
            //tr_bt_manager_mic_mute();
            bt_manager_mic_client_mute();
        }
#endif
        break;

    case MSG_USOUND_INCALL_UNMUTE:
        SYS_LOG_INF("MSG_USOUND_INCALL_UNMUTE\n");
#ifdef CONFIG_BT_LE_AUDIO
        if(tr_usound->host_hid_mic_muted)
        {
            tr_usound->host_hid_mic_muted = 0;
            //tr_bt_manager_mic_unmute();
            bt_manager_mic_client_unmute();
        }
#endif
        break;

    case MSG_USOUND_DEFINE_CMD_CONNECT_PAIRED_DEV:
        break;


    default:
        break;
    }

#ifdef CONFIG_ESD_MANAGER
	{
		u8_t playing = tr_usound->playing;

		esd_manager_save_scene(TAG_PLAY_STATE, &playing, 1);
	}
#endif

}

