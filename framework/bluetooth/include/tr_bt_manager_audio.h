#ifdef CONFIG_BT_TRANSCEIVER
enum
{
    LE_SEARCH_MATCH = 0,
    LE_SEARCH_L,
    LE_SEARCH_R,
    LE_SEARCH_ALL,
    LE_SEARCH_STOP,
    LE_DEL_ALL,
    LE_DEL_L,
    LE_DEL_R,
    LE_SEARCH_ONE,
    LE_SEARCH_MAX,
};

int tr_bt_manager_audio_init(uint8_t type, struct bt_audio_role *role);
int tr_bt_manager_audio_sink_stream_set(struct bt_audio_chan *chan, io_stream_t stream);
int tr_bt_manager_audio_stream_start(struct bt_audio_chan *chan);
int tr_bt_manager_audio_stream_stop(struct bt_audio_chan *chan);
io_stream_t tr_bt_manager_le_audio_source_stream_create(struct bt_audio_chan **chans,
				uint8_t num_chans, uint32_t locations, bool shared);
int tr_bt_manager_audio_source_stream_set_notify_cb(struct bt_audio_chan *chan, void (*cb)(void *chan, uint32_t));
int tr_bt_manager_audio_sink_stream_set_notify_cb(struct bt_audio_chan *chan, void (*cb)(void *chan, uint32_t));
int tr_bt_manager_audio_check_status(void);
int tr_bt_manager_audio_stream_update(struct bt_audio_chan *chan, uint8_t *playing, uint8_t *vol);
int tr_bt_manager_le_audio_stream_update_avrcp(struct bt_audio_chan *chan, uint8_t *playing, uint8_t *vol);
int tr_bt_manager_audio_stream_update_ctrl_cmd(struct bt_audio_chan *chan, uint16_t cmd);
int tr_bt_manager_get_audio_dev_status(int index, struct conn_status *status, int ble);
int tr_bt_manager_get_audio_dev_status_by_chan(void *chan, struct conn_status *status);
int tr_bt_manager_get_audio_dev_status_by_handle(uint16_t handle, struct conn_status *status);
int tr_bt_manager_audio_get_rssi(struct bt_audio_chan *chan, int8_t *rssi);
int tr_bt_manager_media_stream_start(uint16_t handle);
int tr_bt_manager_media_stream_stop(uint16_t handle);
int8_t tr_bt_manager_update_rssi(int index, int ble);

void tr_bt_manager_audio_adv_mode(uint8_t mode);

int tr_bt_le_audio_init(void);
int tr_bt_le_audio_exit(void);
int le_audio_conn_policy_init(void);
int le_audio_conn_policy_exit(void);

void tr_bt_manager_audio_scan_mode(uint8_t mode);
void tr_bt_manager_audio_scan_stop(void);

#ifdef CONFIG_LE_AUDIO_BQB
void set_bqb_full_name(u8_t *name);
void tr_bt_manager_audio_set_ccid(uint8_t ccid_count);
#endif

#ifdef CONFIG_LE_AUDIO_SR_BQB
int tr_bt_manager_contexts_set(bool available, bool supported, int val);
int tr_bt_manager_server_provider_set(uint8_t *provider, uint8_t len);
int tr_bt_manager_server_tech_set(uint8_t tech);
int tr_bt_manager_server_status_set(uint16_t status);
int tr_bt_manager_volume_init_set(uint8_t step);
void tr_bt_manager_csis_not_bonded_set(uint8_t enable);
void tr_bt_manager_csis_update_lockstate(uint8_t lockval);
void tr_bt_manager_mics_mute(uint16_t handle, uint8_t val);
int tr_bt_manager_audio_mcs_adv_set();
#endif

int tr_bt_manager_audio_auto_start(void);
int tr_bt_manager_audio_auto_stop(void);

int tr_bt_manager_media_play(struct bt_audio_chan *chan);
int tr_bt_manager_media_pause(struct bt_audio_chan *chan);
int tr_bt_manager_media_stop(struct bt_audio_chan *chan);

int tr_bt_manager_audio_stream_cb_unregister(struct bt_audio_chan *chan);
void tr_bt_manager_audio_disconn(uint16_t handle);

uint16_t get_audio_handle_by_index(uint8_t index, uint8_t type);

int tr_bt_manager_le_set_interval(uint16_t interval);

int tr_bt_manager_audio_stream_info(struct bt_audio_chan_info *info);
uint16_t get_call_audio_handle_by_index(uint8_t index, uint8_t type, uint8_t *call_index);

int tr_bt_manager_call_remote_originate(uint16_t handle, uint8_t *remote_uri);
int tr_bt_manager_call_remote_accept(struct bt_audio_call *call);
int tr_bt_manager_call_remote_alert(struct bt_audio_call *call);
int tr_bt_manager_call_remote_hold(struct bt_audio_call *call);
int tr_bt_manager_call_remote_retrieve(struct bt_audio_call *call);
int tr_bt_manager_call_remote_terminate(struct bt_audio_call *call);
void tr_bt_manager_mic_mute(void);
void tr_bt_manager_mic_unmute(void);
void tr_bt_manager_volume_up(void);
void tr_bt_manager_volume_down(void);
void tr_bt_manager_volume_mute(void);
void tr_bt_manager_volume_unmute(void);
void tr_bt_manager_volume_unmute_up(void);
void tr_bt_manager_volume_unmute_down(void);
void tr_bt_manager_volume_set(uint16_t handle, uint8_t value);

int tr_bt_manager_call_originate(uint16_t handle, uint8_t *remote_uri);
int tr_bt_manager_call_accept(struct bt_audio_call *call);
int tr_bt_manager_call_hold(struct bt_audio_call *call);
int tr_bt_manager_call_retrieve(struct bt_audio_call *call);
int tr_bt_manager_call_terminate(struct bt_audio_call *call, uint8_t reason);

int tr_bt_manager_audio_stream_enable_single(struct bt_audio_chan *chan,
				uint32_t contexts);

int tr_bt_manager_audio_stream_disable_single(struct bt_audio_chan *chan);

int tr_bt_manager_xear_set(uint8_t featureId, uint8_t value);
#endif
