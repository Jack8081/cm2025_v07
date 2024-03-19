typedef enum {
	TRS_STATE_INIT,
	TRS_STATE_WAIT_PAIR,
	TRS_STATE_INQURIY,
	TRS_STATE_CONNECTING,
	TRS_STATE_DISCONNECTING,
	TRS_STATE_DETECT_ROLE,
	TRS_STATE_CONNECTED,
	TRS_STATE_READY_PLAY,
	TRS_STATE_RESTART_PLAY,
} btsrv_trs_state_e;

typedef struct 
{
	/** Maximum length of the discovery in units of 1.28 seconds.
	 *  Valid range is 0x01 - 0x30.
	 */
	u8_t length;

	/* 0: Unlimited number of responses,
	 * Maximum number of responses from the Inquiry before the Inquiry is halted.
     * condition filter_cod = 0.
	 */
	u8_t nums;
	
    /* 0: unused otherwise,filter result by cod.
	 */
    u32_t filter_cod;
}bt_mgr_inquriy_param_t;

enum {
    PROFILE_A2DP    = 0x01,
    PROFILE_AVRCP   = 0x02,
    PROFILE_AG      = 0x04,
 
    P_DEV_TRS       = 0x20,
    LINK_KEY_VALID  = 0x40,
    P_VALID         = 0x80,
};

enum {
    MSG_BTSRV_TRS_INIT = MSG_BTSRV_TRS_START,
    MSG_BTSRV_TRS_DEINIT,
    MSG_BTSRV_TRS_INQUIRY,
    MSG_BTSRV_TRS_INQUIRY_RESULT,
    MSG_BTSRV_TRS_INQUIRY_COMPLETE,
    MSG_BTSRV_TRS_INQUIRY_UPDATE_NAME,
    MSG_BTSRV_TRS_CANCEL_INQUIRY,
    MSG_BTSRV_TRS_CONNECTED,
    MSG_BTSRV_TRS_DISCONNECT,
    MSG_BTSRV_TRS_CANCEL_CONNECT,
    MSG_BTSRV_TRS_DISCONNECTED,
    MSG_BTSRV_TRS_DEL_PAIRLIST,

    MSG_BTSRV_TRS_NOTIFY_BATTERY,

    MSG_BTSRV_TRS_AVRCP_SYNC_VOLUME,
    MSG_BTSRV_TRS_AVRCP_SYNC_VOLUME_ACCEPT,
    MSG_BTSRV_TRS_AVRCP_GET_ID3_INFO,

    MSG_BTSRV_TRS_AVRCP_SYNC_VOLUME_TO_OTHERS,

    MSG_BTSRV_TRS_SCAN_LOWPOWERABLE,
};

typedef void (*tr_rdm_connected_dev_cb)(struct bt_conn *base_conn, u8_t *addr, u8_t index, void *cb_param);

int tr_btsrv_rdm_a2dp_set_media_info(struct bt_conn *base_conn, struct bt_a2dp_media_codec *codec, u16_t cid, u16_t mtu, u8_t role);
int tr_btsrv_rdm_a2dp_get_media_info(struct bt_conn *base_conn, struct bt_a2dp_media_codec *codec, u16_t *cid, u16_t *mtu, u8_t *role);
int tr_btsrv_rdm_get_connected_dev(tr_rdm_connected_dev_cb cb, void *cb_param);
u8_t *tr_btsrv_rdm_get_addr_by_conn(struct bt_conn *base_conn);

#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
void tr_btsrv_a2dp_call_user_callback(struct bt_conn *conn, btsrv_a2dp_event_e event, void *packet, int size);
#endif

int btsrv_get_inquiry_name_by_addr(u8_t *addr, u8_t *name, int len);
void update_inquiry_name(inquiry_info_t *param, int name_len);
void _stop_inquriy_timer_and_notify_event(void);
void tr_btsrv_a2dp_mute(u8_t enable);
int tr_btsrv_connect_auto_connection_stop_cmd(void);

void tr_btsrv_avrcp_update_playback_status(struct bt_conn *conn, int cmd);

int tr_btsrv_hfp_ag_connect(struct bt_conn *conn);

bool tr_btsrv_rdm_a2dp_is_actived(struct bt_conn *base_conn);
void tr_btsrv_rdm_a2dp_foreach_actived_dev(void(*cb)(struct bt_conn *, void *), void *data);

void tr_btsrv_scan_set_user_lowpowerable(bool enable, bool immediate);

int tr_btsrv_trs_process(struct app_msg *msg);
int tr_btsrv_trs_get_state(void);
int btsrv_hfp_ag_process(struct app_msg *msg);
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
void btsrv_audio_csis_not_bonded_set(uint8_t enable);
void btsrv_audio_csis_update_lockstate(uint8_t val);
int mcs_adv_buf_set(void);
#endif

int tr_bti_get_cis_conn_interval_us(void);
