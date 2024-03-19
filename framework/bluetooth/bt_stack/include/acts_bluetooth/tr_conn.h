/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __TR_BT_CONN_H
#define __TR_BT_CONN_H

int tr_bt_conn_peer_in_discovery(struct bt_conn *conn);
int tr_bt_conn_get_security_status(struct bt_conn *conn);
bool tr_check_bt_conn_enter_sniff_state(void);
struct bt_conn *tr_bt_conn_lookup_addr_br_others(const bt_addr_t *peer);
bool tr_bt_all_conn_can_in_sniff(void);
int tr_bt_conn_le_param_update(uint16_t handle,
			    const struct bt_le_conn_param *param);

#endif /* __TR_BT_CONN_H */
