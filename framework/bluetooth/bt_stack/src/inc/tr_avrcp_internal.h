
enum {
	BT_A2DP_STATE_OPEN	= 0x06,
	BT_A2DP_STATE_START	= 0x07,
	BT_A2DP_STATE_CLOSE	= 0x08,
	BT_A2DP_STATE_SUSPEND	= 0x09,
};


int tr_bt_avrcp_notify_change_vol(struct bt_avrcp *session, u8_t event_id, u8_t *param, u8_t len);
void tr_bt_avrcp_update_playback_status(struct bt_avrcp *session, int cmd);

