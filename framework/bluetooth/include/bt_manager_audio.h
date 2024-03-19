/*
 * Copyright (c) 2021 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_MANAGER_AUDIO_H__
#define __BT_MANAGER_AUDIO_H__

#include "btservice_api.h"
#include <media_type.h>

#define BT_ROLE_MASTER	0
#define BT_ROLE_SLAVE	1

struct bt_audio_role {
	/*
	 * call_gateway
	 * volume_controller
	 * microphone_controller
	 * media_device
	 * unicast_client
	 */
	uint32_t num_master_conn : 3;	/* as a master, range: [0: 7] */

	/*
	 * call_terminal
	 * volume_renderer
	 * microphone_device
	 * media_controller
	 * unicast_server
	 */
	uint32_t num_slave_conn : 3;	/* as a slave, range: [0: 7] */

	/* if BR coexists */
	uint32_t br : 1;

	/* if support encryption */
	uint32_t encryption : 1;
	/*
	 * Authenticated Secure Connections and 128-bit key which depends
	 * on encryption.
	 */
	uint32_t secure : 1;

	/* call */
	uint32_t call_gateway : 1;
	uint32_t call_terminal: 1;

	/* volume */
	uint32_t volume_controller : 1;
	uint32_t volume_renderer : 1;

	/* microphone */
	uint32_t microphone_controller : 1;
	uint32_t microphone_device : 1;

	/* media */
	uint32_t media_controller : 1;
	uint32_t media_device : 1;

	/* unicast stream */
	uint32_t unicast_client : 1;
	uint32_t unicast_server : 1;

	/* group */
	uint32_t set_coordinator : 1;
	uint32_t set_member : 1;

#if 0
	/* broadcast stream */
	uint32_t broadcast_source : 1;
	uint32_t broadcast_sink : 1;
	uint32_t broadcast_assistant : 1;
	uint32_t scan_delegator : 1;
#endif

	/* if support remote keys */
	uint32_t remote_keys : 1;

	/* matching remote(s) with LE Audio test address(es) */
	uint32_t test_addr : 1;

	/* if enable master latency function */
	uint32_t master_latency : 1;

	/* if using trigger machanism, enabled by default for compatibility */
	uint32_t disable_trigger : 1;

	/* if set_sirk is enabled  */
	uint32_t sirk_enabled : 1;
	/* if set_sirk is encrypted (otherwise plain) */
	uint32_t sirk_encrypted : 1;

	/* only works for set_member */
	uint8_t set_size;
	uint8_t set_rank;
	uint8_t set_sirk[16];	/* depends on sirk_enabled */
};

struct bt_audio_chan_info {
	uint8_t format;	/* BT_AUDIO_CODEC_ */
	/* valid at config_codec time for master */
	uint8_t channels;	/* range: [1, 8] */
	/* valid at config_codec time for master */
	uint16_t octets;	/* octets for 1 channel (or 1 codec frame) */
	uint16_t sample;	/* sample_rate; unit: kHz */
	uint16_t kbps;	/* bit rate */
	uint16_t sdu;	/* sdu size */
	uint16_t contexts;	/* BT_AUDIO_CONTEXT_ */
	uint32_t locations;	/* BT_AUDIO_LOCATIONS_ */
	uint32_t interval;	/* sdu interval, unit: us */
	uint32_t delay;	/* presentation_delay */
	uint32_t cig_sync_delay;
	uint32_t cis_sync_delay;
	uint32_t rx_delay;
	uint8_t m_bn;
	uint8_t m_ft;
	uint16_t audio_handle;
};

/* helper for struct bt_audio_capabilities */
struct bt_audio_capabilities_1 {
	uint16_t handle;	/* for client to distinguish different server */
	uint8_t source_cap_num;
	uint8_t sink_cap_num;

	uint16_t source_available_contexts;
	uint16_t source_supported_contexts;
	uint32_t source_locations;

	uint16_t sink_available_contexts;
	uint16_t sink_supported_contexts;
	uint32_t sink_locations;

	/* source cap first if any, followed by sink cap */
	struct bt_audio_capability cap[1];
};

/* helper for struct bt_audio_capabilities */
struct bt_audio_capabilities_2 {
	uint16_t handle;	/* for client to distinguish different server */
	uint8_t source_cap_num;
	uint8_t sink_cap_num;

	uint16_t source_available_contexts;
	uint16_t source_supported_contexts;
	uint32_t source_locations;

	uint16_t sink_available_contexts;
	uint16_t sink_supported_contexts;
	uint32_t sink_locations;

	/* source cap first if any, followed by sink cap */
	struct bt_audio_capability cap[2];
};

/* helper for struct bt_audio_qoss, used to config 1 qos */
struct bt_audio_qoss_1 {
	uint8_t num_of_qos;
	uint8_t sca;
	uint8_t packing;
	uint8_t framing;

	uint32_t interval_m;
	uint32_t interval_s;

	uint16_t latency_m;
	uint16_t latency_s;

	uint32_t delay_m;
	uint32_t delay_s;

	uint32_t processing_m;
	uint32_t processing_s;

	struct bt_audio_qos qos[1];
};

/* helper for struct bt_audio_qoss, used to config 2 qos */
struct bt_audio_qoss_2 {
	uint8_t num_of_qos;
	uint8_t sca;
	uint8_t packing;
	uint8_t framing;

	uint32_t interval_m;
	uint32_t interval_s;

	uint16_t latency_m;
	uint16_t latency_s;

	uint32_t delay_m;
	uint32_t delay_s;

	uint32_t processing_m;
	uint32_t processing_s;

	struct bt_audio_qos qos[2];
};

struct bt_audio_preferred_qos_default {
	uint8_t framing;
	uint8_t phy;
	uint8_t rtn;
	uint16_t latency;	/* max_transport_latency */

	uint32_t delay_min;	/* presentation_delay_min */
	uint32_t delay_max;	/* presentation_delay_max */
	uint32_t preferred_delay_min;	/* presentation_delay_min */
	uint32_t preferred_delay_max;	/* presentation_delay_max */
};

/* FIXME: support codec policy */
struct bt_audio_codec_policy {
};

/* FIXME: support qos policy */
struct bt_audio_qos_policy {
};

void bt_manager_audio_conn_event(int event, void *data, int size);

uint8_t bt_manager_audio_get_type(uint16_t handle);

int bt_manager_audio_get_role(uint16_t handle);

bool bt_manager_audio_actived(void);

/*
 * BT Audio state
 */
/* BT Audio is uninitialized */
#define BT_AUDIO_STATE_UNKNOWN	0
/* BT Audio is initialized, could enter unknown/started state */
#define BT_AUDIO_STATE_INITED	1
/* BT Audio is started, could enter paused/user_pause/stopped state */
#define BT_AUDIO_STATE_STARTED	2
/* BT Audio is stopped, could enter started/unknown state */
#define BT_AUDIO_STATE_STOPPED	3
/* BT Audio is paused, could enter started/user_paused state */
#define BT_AUDIO_STATE_PAUSED	4

/*
 * get bt_audio state, return BT_AUDIO_STATE_XXX (see above)
 */
uint8_t bt_manager_audio_state(uint8_t type);

/*
 * init bt_audio, paired with bt_manager_audio_exit()
 */
int bt_manager_audio_init(uint8_t type, struct bt_audio_role *role);

/*
 * exit bt_audio, paired with bt_manager_audio_init()
 */
int bt_manager_audio_exit(uint8_t type);

/*
 * clear all remote keys accoring to current keys_id.
 */
int bt_manager_audio_keys_clear(uint8_t type);

/*
 * start bt_audio, paired with bt_manager_audio_stop()
 *
 * For LE Audio master, it will start scan.
 * For LE Audio slave, it will start adv.
 */
int bt_manager_audio_start(uint8_t type);

/*
 * stop bt_audio, paired with bt_manager_audio_start()
 *
 * For LE Audio master, it will stop scan, disconnect all connections
 * and be ready to exit.
 * For LE Audio slave, it will stop adv, disconnect all connections
 * and be ready to exit.
 */
int bt_manager_audio_stop(uint8_t type);

/*
 * stop bt_audio unblocked, paired with bt_manager_audio_start()
 *
 * For LE Audio master, it will stop scan, disconnect all connections
 * and be ready to exit.
 * For LE Audio slave, it will stop adv, disconnect all connections
 * and be ready to exit.
 */
int bt_manager_audio_unblocked_stop(uint8_t type);

/*
 * pause bt_audio, paired with bt_manager_audio_resume()
 *
 * For LE Audio master, it will stop scan.
 * For LE Audio slave, it will stop adv.
 */
int bt_manager_audio_pause(uint8_t type);

/*
 * resume bt_audio, paired with bt_manager_audio_pause()
 *
 * For LE Audio master, it will restore scan if necessary.
 * For LE Audio slave, it will restore adv if necessary.
 */
int bt_manager_audio_resume(uint8_t type);

/*
 * Audio Stream Control
 */

/*
 * BT Audio Stream state
 */
#define BT_AUDIO_STREAM_IDLE	0x0
/* the audio stream is codec configured, need to configure qos */
#define BT_AUDIO_STREAM_CODEC_CONFIGURED	0x1
/* the audio stream is qos configured, need to enable it */
#define BT_AUDIO_STREAM_QOS_CONFIGURED	0x2
/* the audio stream is enabled and wait for the sink to be ready */
#define BT_AUDIO_STREAM_ENABLED	0x3
/* the audio stream is ready or is transporting */
#define BT_AUDIO_STREAM_STARTED	0x4
/*
 * the audio stream is disabled, but the stream may be working still,
 * if the stream is stopped, it will transport to configured state.
 */
#define BT_AUDIO_STREAM_DISABLED	0x5

/* used to restore all current audio channels */
struct bt_audio_chan_restore {
	uint8_t count;
	struct bt_audio_chan chans[0];
};

int bt_manager_audio_stream_dir(uint16_t handle, uint8_t id);

int bt_manager_audio_stream_state(uint16_t handle, uint8_t id);

int bt_manager_audio_stream_info(struct bt_audio_chan *chan,
				struct bt_audio_chan_info *info);

struct bt_audio_endpoints *bt_manager_audio_client_get_ep(uint16_t handle);

struct bt_audio_capabilities *bt_manager_audio_client_get_cap(uint16_t handle);

int bt_manager_audio_stream_cb_register(struct bt_audio_chan *chan,
				bt_audio_trigger_cb trigger,
				bt_audio_trigger_cb period_trigger);

/*
 * Register callback(s) but will bind the trigger callback(s) of the new
 * channel to the bound channel which makes the timeline be shared by the
 * new channel and the bound channel.
 */
int bt_manager_audio_stream_cb_bind(struct bt_audio_chan *bound,
				struct bt_audio_chan *new_chan,
				bt_audio_trigger_cb trigger,
				bt_audio_trigger_cb period_trigger);

/*
 * For LE Audio slave only
 */
int bt_manager_audio_server_cap_init(struct bt_audio_capabilities *caps);

/*
 * For LE Audio slave only
 */
int bt_manager_audio_server_qos_init(bool source,
				struct bt_audio_preferred_qos_default *qos);

/*
 * For LE Audio master only
 */
int bt_manager_audio_client_codec_init(struct bt_audio_codec_policy *policy);

/*
 * For LE Audio master only
 */
int bt_manager_audio_client_qos_init(struct bt_audio_qos_policy *policy);

int bt_manager_audio_stream_config_codec(struct bt_audio_codec *codec);

int bt_manager_audio_stream_prefer_qos(struct bt_audio_preferred_qos *qos);

int bt_manager_audio_stream_config_qos(struct bt_audio_qoss *qoss);

/*
 * For LE Audio master only
 *
 * combined with bt_manager_audio_stream_bandwidth_alloc() to replace
 * bt_manager_audio_stream_config_qos().
 */
int bt_manager_audio_stream_bind_qos(struct bt_audio_qoss *qoss, uint8_t index);

/*
 * WARNNING: Move the sync point forward to optimize the transport delay in
 * some special cases, do NOT call this function if unsure.
 *
 * NOTICE: make sure called before stream_enable()/stream_enabled if used.
 */
int bt_manager_audio_stream_sync_forward(struct bt_audio_chan **chans,
				uint8_t num_chans);

int bt_manager_audio_stream_sync_forward_single(struct bt_audio_chan *chan);

int bt_manager_audio_stream_enable(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t contexts);

int bt_manager_audio_stream_enable_single(struct bt_audio_chan *chan,
				uint32_t contexts);

int bt_manager_audio_stream_disable(struct bt_audio_chan **chans,
				uint8_t num_chans);

int bt_manager_audio_stream_disable_single(struct bt_audio_chan *chan);

/*
 * for Audio Source to create Source Stream used to send data via Bluetooth
 *
 * Kinds of samples are as follows.
 * case 1: two channels exist in one connection.
 * case 2: one channel exist for each connection.
 * case 3: two channels exist in one connection and one channel exist in
 *         another connection.
 *
 * @param shared Whether all channels will share the same date
 */
io_stream_t bt_manager_audio_source_stream_create(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t locations);

io_stream_t bt_manager_audio_source_stream_create_single(struct bt_audio_chan *chan,
				uint32_t locations);

int bt_manager_audio_source_stream_set(struct bt_audio_chan **chans,
				uint8_t num_chans, io_stream_t stream,
				uint32_t locations);

int bt_manager_audio_source_stream_set_single(struct bt_audio_chan *chan,
				io_stream_t stream, uint32_t locations);

/*
 * for Audio Sink to set Sink Stream used to receive data via Bluetooth
 */
int bt_manager_audio_sink_stream_set(struct bt_audio_chan *chan,
				io_stream_t stream);

/*
 * for Audio Sink
 *
 * Notify the remote (Audio Source) that the local (Audio Sink) is ready to
 * receive/start the audio stream.
 *
 * Valid after stream is enabled
 */
int bt_manager_audio_sink_stream_start(struct bt_audio_chan **chans,
				uint8_t num_chans);

int bt_manager_audio_sink_stream_start_single(struct bt_audio_chan *chan);

/*
 * for Audio Sink
 *
 * Notify the remote (Audio Source) that the local (Audio Sink) is going to
 * stop the audio stream.
 *
 * Valid after stream is disabled
 */
int bt_manager_audio_sink_stream_stop(struct bt_audio_chan **chans,
				uint8_t num_chans);

int bt_manager_audio_sink_stream_stop_single(struct bt_audio_chan *chan);

int bt_manager_audio_stream_update(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t contexts);

int bt_manager_audio_stream_update_single(struct bt_audio_chan *chan,
				uint32_t contexts);

int bt_manager_audio_stream_release(struct bt_audio_chan **chans,
				uint8_t num_chans);

int bt_manager_audio_stream_release_single(struct bt_audio_chan *chan);

int bt_manager_audio_stream_disconnect(struct bt_audio_chan **chans,
				uint8_t num_chans);

int bt_manager_audio_stream_disconnect_single(struct bt_audio_chan *chan);

/*
 * For LE Audio master only
 *
 * Allocate the bandwidth earlier initiated by master, combined with
 * bt_manager_audio_stream_bind_qos() instead of
 * bt_manager_audio_stream_config_qos().
 */
void *bt_manager_audio_stream_bandwidth_alloc(struct bt_audio_qoss *qoss);

/*
 * For LE Audio master only
 *
 * Free the bandwidth allocated by config_qos() initiated by master.
 */
int bt_manager_audio_stream_bandwidth_free(void *cig_handle);

int bt_manager_audio_stream_restore(uint8_t type);

void bt_manager_audio_stream_tx_current_time(uint64_t *bt_time);

void bt_manager_audio_stream_tx_last_time(uint64_t *time, uint64_t *count);

void bt_manager_audio_stream_rx_current_time(uint64_t *bt_time);

void bt_manager_audio_stream_rx_last_time(uint64_t *time, uint64_t *count);

void bt_manager_audio_stream_event(int event, void *data, int size);

enum media_type bt_manager_audio_codec_type(uint8_t coding_format);

/*
 * Media Control
 */

int bt_manager_media_play(void);

int bt_manager_media_stop(void);

int bt_manager_media_pause(void);

int bt_manager_media_play_next(void);

int bt_manager_media_play_previous(void);

int bt_manager_media_fast_forward(bool start);

int bt_manager_media_fast_rewind(bool start);

void bt_manager_media_event(int event, void *data, int size);

int bt_manager_media_get_status(void);

/*
 * Call Control
 */

#define BT_CALL_STATE_UNKNOWN          0x00
#define BT_CALL_STATE_INCOMING         0x01
#define BT_CALL_STATE_DIALING          0x02	/* outgoing */
#define BT_CALL_STATE_ALERTING         0x03	/* outgoing */
#define BT_CALL_STATE_ACTIVE           0x04
#define BT_CALL_STATE_LOCALLY_HELD     0x05
#define BT_CALL_STATE_REMOTELY_HELD    0x06
/* locally and remotely held */
#define BT_CALL_STATE_HELD             0x07
#define BT_CALL_STATE_SIRI             0x08

struct bt_audio_call_info {
	uint8_t state;

	/*
	 * UTF-8 URI format: uri_scheme:caller_id such as tel:96110
	 *
	 * end with '\0' if valid, all-zeroed if invalid
	 */
	uint8_t remote_uri[BT_AUDIO_CALL_URI_LEN];
};

/*
 * NOTE: the following bt_manager_call APIs are used for call terminal only.
 */

/* Originate an outgoing call */
int bt_manager_call_originate(uint16_t handle, uint8_t *remote_uri);

/* Accept the incoming call */
int bt_manager_call_accept(struct bt_audio_call *call);

/* Hold the call */
int bt_manager_call_hold(struct bt_audio_call *call);

/* Active the call which is locally held */
int bt_manager_call_retrieve(struct bt_audio_call *call);

/* Join a multiparty call */
int bt_manager_call_join(struct bt_audio_call **calls, uint8_t num_calls);

/* Terminate the call, NOTICE: reason is not used yet */
int bt_manager_call_terminate(struct bt_audio_call *call, uint8_t reason);

int bt_manager_call_allow_stream_connect(bool allowed);

int bt_manager_call_hangup_another_call(void);

int bt_manager_call_holdcur_answer_call(void);

int bt_manager_call_hangupcur_answer_call(void);

int bt_manager_call_switch_sound_source(void);

int bt_manager_call_start_siri(void);
int bt_manager_call_stop_siri(void);
int bt_manager_call_dial_last(void);


void bt_manager_call_event(int event, void *data, int size);

int bt_manager_call_get_status(void);

int bt_manager_volume_up(void);

int bt_manager_volume_down(void);

int bt_manager_volume_set(uint8_t value);

int bt_manager_volume_mute(void);

int bt_manager_volume_unmute(void);

int bt_manager_volume_unmute_up(void);

int bt_manager_volume_unmute_down(void);

int bt_manager_volume_client_up(void);

int bt_manager_volume_client_down(void);

int bt_manager_volume_client_set(uint8_t value);

int bt_manager_volume_client_mute(void);

int bt_manager_volume_client_unmute(void);

void bt_manager_volume_event(int event, void *data, int size);
void bt_manager_volume_event_to_app(uint16_t handle, uint8_t volume, uint8_t from_phone);

int bt_manager_mic_mute(void);

int bt_manager_mic_unmute(void);

int bt_manager_mic_mute_disable(void);

int bt_manager_mic_gain_set(uint8_t gain);

int bt_manager_mic_client_mute(void);

int bt_manager_mic_client_unmute(void);

int bt_manager_mic_client_gain_set(uint8_t gain);

void bt_manager_mic_event(int event, void *data, int size);



/*
 * Specific APIs for LE Audio
 */

/* Connectable advertisement parameter for LE Audio Slave */
int bt_manager_audio_le_adv_param_init(struct bt_le_adv_param *param);

/* Scan parameters for LE Audio Master */
int bt_manager_audio_le_scan_param_init(struct bt_le_scan_param *param);

/* Create connection parameters for LE Audio Master */
int bt_manager_audio_le_conn_create_param_init(struct bt_conn_le_create_param *param);

/* Initial connection parameters for LE Audio Master */
int bt_manager_audio_le_conn_param_init(struct bt_le_conn_param *param);

/* Idle (such as CIS non-connected) connection parameters for LE Audio Master */
int bt_manager_audio_le_conn_idle_param_init(struct bt_le_conn_param *param);

int bt_manager_audio_le_conn_max_len(uint16_t handle);

int bt_manager_audio_le_vnd_register_rx_cb(uint16_t handle,
				bt_audio_vnd_rx_cb rx_cb);

int bt_manager_audio_le_vnd_send(uint16_t handle, uint8_t *buf, uint16_t len);

int bt_manager_audio_le_adv_cb_register(bt_audio_adv_cb start,
				bt_audio_adv_cb stop);

int bt_manager_audio_le_start_adv(void);

int bt_manager_audio_le_stop_adv(void);

/* Set media status */
void bt_manager_media_set_status(uint16_t hdl, uint16_t state);

/* Set call status */
void bt_manager_call_set_status(uint16_t hdl, uint16_t state);


int bt_manager_audio_get_sirk(char* sirkbuf, int sirkbuf_size);

int bt_manager_audio_set_public_sirk(char* sirkbuf, int sirkbuf_size);
int bt_manager_audio_get_public_sirk(char* sirkbuf, int sirkbuf_size);
int bt_manager_audio_set_rank(uint8_t rank);
int bt_manager_audio_get_rank_and_size(uint8_t* rank, uint8_t* size);

int bt_manager_audio_get_recently_connectd(uint32_t *remain_period);

int bt_manager_audio_entering_btcall(void);


/* Manually switch active app name init:  'btmusic' or 'le_audio' */
int bt_manager_switch_active_app_init(char* app_name);
/* Manually switch active app:  btmusic or le_audio app switched */
int bt_manager_switch_active_app(void);
/* Manually resume active app:  btmusic or le_audio app switched, BT_AUDIO_APP_LE_AUDIO / BT_AUDIO_APP_BR_MUSIC */
int bt_manager_resume_active_app(uint8_t bt_audio_app);

/* Manually switch active app check: if switch to btmusic or le_audio app */
int bt_manager_switch_active_app_check(void);

/*Get connected devices number include "BR and LEaudio*/
int bt_manager_get_connected_audio_dev_num(void);

int bt_manager_audio_clear_paired_list(uint8_t type);

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
int bt_manager_volume_uac_gain(uint8_t gain);
#endif

int bt_manager_get_active_app_id(void);

#ifdef CONFIG_BT_TRANSCEIVER
#include "tr_bt_manager_audio.h"
#endif
#endif /* __BT_MANAGER_AUDIO_H__ */
