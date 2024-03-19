/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bt controller and dsp xfer bypass define
 */

#ifndef __MEDIA_BT_DSP_XFER_H__
#define __MEDIA_BT_DSP_XFER_H__

#include <os_common_api.h>
#include <msg_manager.h>
#include <stream.h>
#include <thread_timer.h>
#include <arithmetic.h>
#include <media_type.h>
#ifdef CONFIG_BT_MANAGER
#include <btservice_api.h>
#endif

/* CIS BEGIN */

enum {
    CIS_RECV_CORRECT = 0, /* receive correct */
	CIS_RECV_CRC_ERR = 1,/* crc error */
    CIS_RECV_NO_PKT = 2, /* no pkt error */
};

struct bt_dsp_cis_recv_report {
    uint32_t anchor_point_us;     /* bt clock of cis iso interval anchor point */
    uint32_t no_pkt_err;          /* every bit record one packet recv no pkt err flag, bit[0] for the first packet */
    uint32_t crc_err;             /* every bit record one packet recv crc err flag, bit[0] for the first packet */
    uint16_t recv_rtn : 5;           /* retransmission index when received the pkt */
    uint16_t iso_interval_delay : 3; /* FT > 1 valid, 0 for no delay, 1 for delay 1 iso interval */
    uint16_t null_pkt : 1;           /* it means the source send a null packet */
    uint16_t pkt_bn : 4;             /* pkt_bn is 0,1,2 when BN = 3 */
    uint16_t crc_num;                /* only controller used */
    uint32_t recv_offset_us;         /* timestamp relative to iso interval anchor point */
    uint16_t reserved2[6];
}; /* align to 32Bytes */

struct bt_dsp_cis_send_report {
    uint32_t anchor_point_us;     /* bt clock of cis iso interval anchor point */
    uint32_t send_offset_us;         /* timestamp relative to iso interval anchor point */
    uint16_t send_rtn : 5;           /* retransmission index when sent the pkt */
    uint16_t send_prefetch_flag : 1; /* 1, prefetch succ; 0, no pkt, it will send a null pkt */
    uint16_t send_result : 1;        /* send pkt result: 1, succ or HAS ACK; 0, fail or NO ack */
    uint16_t reserved2[3];
}; /* align to 16Bytes */

struct bt_dsp_cis_share_info {
    struct acts_ringbuf * recvbuf;            /* bt -> dsp, all crc error pkts payload save */
                                              /* format: dsp_pkthdr_t + header of nav + nav raw data + padding (to recv_pkt_size) */
    struct acts_ringbuf * recv_report_buf;    /* bt -> dsp, element is struct bt_dsp_cis_recv_report */
    struct acts_ringbuf * sendbuf;            /* dsp -> bt, format: header of nav + nav raw data + padding (to send_pkt_size) */
    struct acts_ringbuf * send_report_buf;    /* dsp -> bt, element is struct bt_dsp_cis_send_report */
#ifdef CONFIG_BT_TRANSCEIVER
    void *dump_buf;
#endif
};

/* CIS END */


/* SCO BEGIN */

struct bt_dsp_sco_xfer_report {
    uint32_t anchor_point_us;     /* bt clock of sco iso interval anchor point */
    uint32_t reserved0;
    uint16_t no_pkt_err;          /* every bit record one packet recv no pkt err flag, bit[0] for the first packet */
    uint16_t crc_err;             /* every bit record one packet recv crc err flag, bit[0] for the first packet */
    uint16_t recv_rtn : 4;           /* retransmission index when received the pkt */
    uint16_t null_pkt : 1;           /* it means the source send a null packet */
    uint16_t reserved1;
    uint32_t recv_offset_us;         /* timestamp relative to iso interval anchor point */
    uint16_t send_rtn : 4;           /* 0~3 send succ; -1, send fail; */
    uint16_t send_prefetch_flag : 1; /* 1, prefetch succ; 0, no pkt, it will send a null pkt */
    uint16_t reserved2[5];
}; /* align to 32Bytes */

struct bt_dsp_sco_share_info {
    struct acts_ringbuf * recvbuf;            /* bt -> dsp, all crc error pkts payload save */
                                              /* format: dsp_pkthdr_t + sco raw data + padding (to recv_pkt_size) */
    struct acts_ringbuf * xfer_report_buf;    /* bt -> dsp, element is struct bt_dsp_sco_xfer_report */
    struct acts_ringbuf * sendbuf;            /* dsp -> bt, format: sco raw data + padding (to send_pkt_size) */
};

/* SCO END */

#endif /* __MEDIA_BT_DSP_XFER_H__ */
