/** @file
 * @brief Bluetooth host interface header file.
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __TR_HOST_INTERFACE_H
#define __TR_HOST_INTERFACE_H

int tr_btif_get_pairedlist(u8_t index, paired_dev_info_t *dev);
int tr_btif_disallowed_reconnect(const u8_t *addr, u8_t disallowed);
int tr_btif_del_storage_by_index(u8_t index);
int tr_btif_get_remote_name(u8_t *addr, u8_t *name, int len);

int tr_hostif_bt_a2dp_send_audio_data(struct bt_conn *conn, u8_t *data, u16_t len, void(*cb)(struct bt_conn *, void *));
int tr_hostif_bt_a2dp_send_audio_data_pass_through(struct bt_conn *conn, u8_t *data);
int tr_hostif_bt_a2dp_get_no_completed_pkt_num(struct bt_conn *conn);
int tr_hostif_bt_connect_start_security(struct bt_conn *conn);
int tr_hostif_bt_get_security_status(struct bt_conn *conn);
int tr_hostif_bt_set_lowpower_mode(u8_t enable);
int tr_hostif_bt_remote_version_request(struct bt_conn *conn);
bool tr_hostif_bt_all_conn_can_in_sniff(void);

void tr_hostif_bt_avrcp_cttg_update_playback_status(struct bt_conn *conn, int cmd);

int tr_hostif_bt_conn_le_param_update(uint16_t handle,
			    const struct bt_le_conn_param *param);
int tr_hostif_bt_set_link_policy(uint16_t handle, int8_t enable);
#endif /* __TR_HOST_INTERFACE_H */
