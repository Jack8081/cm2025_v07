/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_L2CAP_ECHO_REQ		0x08
struct bt_l2cap_echo_req {
	u8_t data[0];
} __packed;

#define BT_L2CAP_ECHO_RSP		0x09
struct bt_l2cap_echo_rsp {
	u8_t data[0];
} __packed;


#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
#define BT_L2CAP_CONF_OPT_RETRANS   0x04
struct bt_l2cap_conf_opt_retrans {
	u8_t  mode;
	u8_t  tx_win;
	u8_t  max_tx;
	u16_t rto;
	u16_t  mto;
	u16_t mps;
} __packed;
int bt_l2cap_br_chan_send_ert(struct bt_l2cap_chan *chan, struct net_buf *buf);
#endif
int tr_bt_l2cap_br_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf, bt_conn_tx_cb_t cb);

