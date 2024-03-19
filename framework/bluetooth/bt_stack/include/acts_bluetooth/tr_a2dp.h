/** @file
 * @brief Advance Audio Distribution Profile header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __TR_BT_A2DP_H
#define __TR_BT_A2DP_H

int tr_bt_a2dp_send_audio_data(struct bt_conn *conn, u8_t *data, u16_t len, void(*cb)(struct bt_conn *, void *));
int tr_bt_a2dp_send_audio_data_pass_through(struct bt_conn *conn, u8_t *data);
int tr_bt_a2dp_get_no_completed_pkt_num(struct bt_conn *conn);

#endif /* __TR_BT_A2DP_H */
