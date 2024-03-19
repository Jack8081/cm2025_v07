/** @file
 *  @brief Bluetooth L2CAP handling
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef TR_ZEPHYR_INCLUDE_BLUETOOTH_L2CAP_H_
#define TR_ZEPHYR_INCLUDE_BLUETOOTH_L2CAP_H_

#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
enum {
    L2CAP_MODE_BASIC    =   0,
    L2CAP_MODE_RTX      =   1,
    L2CAP_MODE_FC       =   2,
    L2CAP_MODE_ERTX     =   3,
    L2CAP_MODE_SM       =   4
};

enum {
	ERT_XMIT = 			0,
	ERT_WAIT_F = 		1,
	ERT_RECV = 			2,
	ERT_SREJ_SENT = 	3,
	ERT_REJ_SENT = 		4
};

/* I,S Frame type */
enum {
	FRM_RR = 			0,					/* S-frame : RR */
	FRM_REJ = 			1,					/* S-frame : REJ */
	FRM_RNR = 			2,					/* S-frame : RNR - Receiver Not Ready */
	FRM_SREJ = 			3,					/* S-frame : SREJ - Select Reject */
	FRM_DATA = 			4,					/* I-frame */
	FRM_REDATA = 		5					/* Retry I-frame, used internal */
};

struct bt_l2cap_chan_opts {
    u8_t  mode;
    u8_t  tx_win;
    u8_t  tx_win_remote;
    u8_t  max_tx;
    u16_t rto;
    u16_t mto;
    u16_t mps;
    u16_t mps_remote;

    u8_t tx_state;
    u8_t rx_state;
    u8_t ext_len;

    u32_t extinf;
    /** l2cap retrans*/
    u8_t txseq;
    u8_t next_txseq;
    u8_t exp_ackseq;
    u8_t reqseq;
    u8_t exp_txseq;
    u8_t bufseq;

    u8_t last_ackseq;

    u16_t srej_savereqseq;

    u16_t unacked_frames;
    void *tx_pos;
    u8_t retry_count;

    sys_slist_t send_list;
    sys_slist_t srej_list;
    sys_slist_t recv_list;

    u16_t max_txwin;

    struct k_delayed_work rtx_work;
    struct k_delayed_work mto_work;
    struct k_delayed_work ack_work;

    void *chan;

    u8_t local_busy     :   1;
    u8_t remote_busy    :   1;
    u8_t rej_action     :   1;
    u8_t srej_action    :   1;
    u8_t send_rej       :   1;
    u8_t send_rnr       :   1;

    u8_t rtx_work_r     :   1;
    u8_t mto_work_r     :   1;
};
#endif

int tr_bt_l2cap_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf, void (*cb)(struct bt_conn *, void *));
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_L2CAP_H_ */
