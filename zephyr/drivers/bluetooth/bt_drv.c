/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth driver for Actions SoC
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc_atp.h>
#include <drivers/ipmsg.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <drivers/bluetooth/bt_drv.h>
#include <compensation.h>
#include "ecc_acts.h"
#include <dvfs.h>
#include <drivers/uart.h>
#include <drivers/uart_dma.h>

#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(bt_drv, CONFIG_LOG_DEFAULT_LEVEL);

#define BTDRV_RAM_FUNC	__ramfunc

#define CONFIG_HCI_RX_THREAD 1
#define CFG_ECC_PRINTK_KEY          0
#define CFG_USE_TWS_SHARERAM_PKT    0
#define BTDRV_EFUSE_CAP_RECORD  (CONFIG_COMPENSATION_FREQ_INDEX_NUM)

#ifdef CONFIG_BT_HCI_TX_PRINT
static const char *tx_type_str[] = {
	"NULL:",
	"TX:01 ",
	"TX:02 ",
	"TX:03 ",
	"TX:04 "
};
#endif

#ifdef CONFIG_BT_HCI_RX_PRINT
static const char *rx_type_str[] = {
	"NULL:",
	"RX:01 ",
	"RX:02 ",
	"RX:03 ",
	"RX:04 ",
};
#endif

#define BTC_BIN_ADDR (0x08100000)
#define BTC_BIN_SIZE (0x34000)

enum {
	BT_CPU_ENABLE,
	BT_CPU_READY,
#ifdef CONFIG_BT_CTRL_TWS
	/* Need to clear bt cpu tws0 pending */
	BT_CPU_TWS0_PENDING,
	BT_CPU_TWS1_PENDING,
#endif
	BT_CPU_NUM_FLAGS,
};

enum {
	TWS_IRQ_INNER_ENABLE_BIT   = (0x1 << 0),		/* Operate by driver */
	TWS_IRQ_EXT_ENABLE_BIT      = (0x1 << 1),		/* Operate by external */
	TWS_IRQ_ALL_ENABLE_BIT		= (TWS_IRQ_INNER_ENABLE_BIT | TWS_IRQ_EXT_ENABLE_BIT),
};

static struct {
	struct device *dev;
	ATOMIC_DEFINE(flags, BT_CPU_NUM_FLAGS);
	btdrv_hci_cb_t *hci_cb;
	struct k_sem ready_sem;
	struct k_sem hci_rx_sem;
	atomic_t hci_rx_cnt;
	atomic_t log_rx_cnt;
	/* message send to bt cpu */
	rbuf_msg_t msg;
	uint32_t msg_tx_id;
	uint32_t msg_rx_id;
	uint32_t hci_tx_id;
	uint32_t hci_rx_id;
	uint32_t log_rx_id;
#ifdef CONFIG_BT_CTRL_TWS
	btdrv_tws_cb_t tws0_cb;
	btdrv_tws_cb_t tws1_cb;
	uint32_t tws_tx_id[2];
	uint32_t tws_rx_id[2];
	uint8_t tws_irq_enable[2];
#endif
} btc = {
	.ready_sem = Z_SEM_INITIALIZER(btc.ready_sem, 0, 1),
	.hci_rx_sem = Z_SEM_INITIALIZER(btc.hci_rx_sem, 0, UINT_MAX),
	.hci_rx_cnt = ATOMIC_INIT(0),
	.log_rx_cnt = ATOMIC_INIT(0),
};

#ifdef CONFIG_BT_ECC_ACTS
static struct ecc_info {
	struct k_delayed_work work;
	ATOMIC_DEFINE(flag, NUM_ECC_FLAGS);
	/* ecc request data size */
	uint16_t size;
	/* ecc request data */
	uint8_t req[128];
	/* ecc response data */
	uint8_t rsp[128];
} btc_ecc;
#endif

#ifdef CONFIG_BT_CTRL_TWS
struct btc_tws {
	/* tws interrupt time */
	uint32_t bt_clk;
	uint16_t intra_off;
	/* timer mode: us/navtive */
	uint8_t  mode;
	/* flag of esco/iso pkt */
	uint8_t  flag;
	/* tws interrupt type */
	uint8_t  type;
	/* clear interrup pending counter */
	uint8_t  pcnt;
	/* write data counter */
	uint8_t  wcnt;
	/* read data counter */
	uint8_t  rcnt;
	/* pkt sequence number */
	uint16_t seq;
	/* data len of hci pkt */
	uint16_t data_len;
#if CFG_USE_TWS_SHARERAM_PKT
	/* hci pkt */
	uint8_t  data[256];
#endif
} __packed;

#define BT_TWS_SIZE    sizeof(struct btc_tws)

/* tws_tx: main -> bt. tws_rx: bt -> main. */
static struct btc_tws *tws_rx[2];
static struct btc_tws *tws_tx[2];

static int btdrv_tws_irq_enable_inner(uint8_t index, bool inner_call);
static int btdrv_tws_irq_disable_inner(uint8_t index, bool inner_call);
#endif

#if CONFIG_HCI_RX_THREAD
static K_THREAD_STACK_DEFINE(rx_thread_stack, 1024);
static struct k_thread rx_thread_data;
#endif

static struct btc_debug_config* pbtc_usr_config = NULL;


#define BTC_LOG_SIZE (256)
#define BTC_LOG_HEADER "<CT-I>"
#define BTC_LOG_HEADER_LEN (6)
static char log_buf[BTC_LOG_SIZE + 2 + BTC_LOG_HEADER_LEN];

/* custom hci buf header */
struct bt_hci_hdr {
	/* length of hci packet, round up to 4-byte */
	uint16_t len;
	/* hci packet type */
	uint8_t type;
	uint8_t rfu;
} __packed;

#define BT_HCI_HDR_SIZE sizeof(struct bt_hci_hdr)

/* hci rx/tx data transmit on rbuf */
static struct hci_data {
	/* hci header */
	union {
		struct bt_hci_hdr hdr;
		uint8_t hdr_buff[BT_HCI_HDR_SIZE];
	};
	/* hci packet, Core 5.2 [Vol 4, Part E, 5.4] */
	uint8_t *pkt;
	/* length of hci packet */
	uint16_t pkt_len;
	/* total length of hci data, round up to 4-byte */
	uint16_t total;
	/* current offset of hci packet */
	uint16_t offset;
	/* need to copy header if false */
	uint8_t hdr_len;
	bool has_hdr;
} rx, tx;

#if (CFG_ECC_PRINTK_KEY || CONFIG_BT_HCI_RX_PRINT || CONFIG_BT_HCI_TX_PRINT)
static int print_hex(const char *prefix, const uint8_t *data, int size)
{
	int n = 0;

	if (!size) {
		printk("%s zero-length signal packet\n", prefix);
		return 0;
	}

	if (prefix) {
		printk("%s", prefix);
	}

	while (size--) {
		printk("%02x ", *data++);
		n++;
		if (n % 16 == 0) {
			printk("\n");
		}
	}

	if (n % 16) {
		printk("\n");
	}

	return 0;
}
#endif

static void rbuf_info_dump(rbuf_t *rbuf)
{
	if ((rbuf->tmp_head != rbuf->head) || (rbuf->tmp_tail != rbuf->tail)) {
		printk("tmp head %u, head %u, tmp tail %u, tail %u\n",
			rbuf->tmp_head, rbuf->head, rbuf->tmp_tail, rbuf->tail);
	}
}

static unsigned int rbuf_put_data(uint32_t id, void *data, uint16_t len)
{
	uint32_t size;
	void *buf = NULL;

	if (!data || !len) {
		return 0;
	}

	buf = rbuf_put_claim(RBUF_FR_OF(id), len, &size);
	if (!buf) {
		return 0;
	}

	memcpy(buf, data, MIN(len, size));
	rbuf_put_finish(RBUF_FR_OF(id), size);

	return size;
}

static unsigned int rbuf_get_data(uint32_t id, void *dst, uint16_t len)
{
	uint32_t size;
	void *buf = NULL;

	if (!dst || !len) {
		return 0;
	}

	buf = rbuf_get_claim(RBUF_FR_OF(id), len, &size);
	if (!buf) {
		return 0;
	}

	memcpy(dst, buf, MIN(len, size));
	rbuf_get_finish(RBUF_FR_OF(id), size);

	return size;
}

#ifdef CONFIG_BT_ECC_ACTS
static void ecc_send(unsigned short type, int err, int id, void *data, uint16_t len)
{
	unsigned short flag = 0;
	uint8_t *p_data = data;
	uint16_t pos = 0, copy_len;

	k_sched_lock();

	btc.msg.type = type;
	btc.msg.flag = flag++;
	btc.msg.data.w[0] = err;
	btc.msg.data.w[1] = id;
	rbuf_put_data(btc.msg_tx_id, &btc.msg, sizeof(rbuf_msg_t));

	if ((err == 0) && data) {
		while (pos < len) {
			btc.msg.type = type;
			btc.msg.flag = flag++;
			copy_len = ((len - pos) > 16) ? 16 : (len - pos);
			memcpy(btc.msg.data.b, &p_data[pos], copy_len);
			rbuf_put_data(btc.msg_tx_id, &btc.msg, sizeof(rbuf_msg_t));
			pos += copy_len;
		}
	}

	ipmsg_notify(btc.dev);

	k_sched_unlock();
}

static int emulate_le_p192_public_key(void)
{
	int ret;
	struct bt_gen_p192_pk_rsp *rsp = (void *)btc_ecc.rsp;

	LOG_INF("gen p192 public key");

	ret = ecc_gen_p192_pk(rsp->public_key, rsp->private_key);
	if (ret) {
		goto done;
	}

	/* Convert key from big-endian to little-endian */
	sys_mem_swap(rsp->public_key, 24);
	sys_mem_swap(&rsp->public_key[24], 24);
	sys_mem_swap(rsp->private_key, 24);
#if CFG_ECC_PRINTK_KEY
	print_hex("public key: ", rsp->public_key, 48);
	print_hex("private key: ", rsp->private_key, 24);
#endif

done:
	ecc_send(MSG_BT_GEN_P192_PK, ret, sizeof(*rsp), rsp, sizeof(*rsp));
	return ret;
}

static int emulate_le_p192_dhkey(void)
{
	int err;
	struct bt_gen_p192_dhkey_req *req = (void *)btc_ecc.req;
	struct bt_gen_p192_dhkey_rsp *rsp = (void *)btc_ecc.rsp;

	LOG_INF("gen p192 dhkey");
#if CFG_ECC_PRINTK_KEY
	print_hex("remote pk: ", req->remote_pk, 48);
	print_hex("private key: ", req->private_key, 24);
#endif

	/* Convert X and Y coordinates from little-endian to
	 * big-endian (expected by the crypto API).
	 */
	sys_mem_swap(req->remote_pk, 24);
	sys_mem_swap(&req->remote_pk[24], 24);
	/* valid remote public key first and send the result to btc */
	err = ecc_valid_p192_pk(req->remote_pk);
	if (err) {
		goto done;
	}

	sys_mem_swap(req->private_key, 24);
	err = ecc_gen_p192_dhkey(req->remote_pk, req->private_key, rsp->dhkey);
	if (err) {
		goto done;
	}

	/* Convert dhkey from big-endian to little-endian */
	sys_mem_swap(rsp->dhkey, 24);
	rsp->id = req->id;
#if CFG_ECC_PRINTK_KEY
	print_hex("dhkey: ", rsp->dhkey, 24);
#endif

done:
	ecc_send(MSG_BT_GEN_P192_DHKEY, err, (int)req->id, rsp->dhkey, sizeof(rsp->dhkey));
	return err;
}

static int emulate_le_p256_public_key(void)
{
	int ret;
	struct bt_gen_p256_pk_rsp *rsp = (void *)btc_ecc.rsp;

	LOG_INF("gen p256 pk");

	ret = ecc_gen_p256_pk(rsp->public_key, rsp->private_key);
	if (ret) {
		goto done;
	}

	sys_mem_swap(rsp->public_key, 32);
	sys_mem_swap(&rsp->public_key[32], 32);
	sys_mem_swap(rsp->private_key, 32);
#if CFG_ECC_PRINTK_KEY
	print_hex("public key: ", rsp->public_key, 64);
	print_hex("private key: ", rsp->private_key, 32);
#endif

done:
	ecc_send(MSG_BT_GEN_P256_PK, ret, sizeof(*rsp), rsp, sizeof(*rsp));

	return ret;
}

static int emulate_le_p256_dhkey(void)
{
	int err;
	struct bt_gen_p256_dhkey_req *req = (void *)btc_ecc.req;
	struct bt_gen_p256_dhkey_rsp *rsp = (void *)btc_ecc.rsp;

	LOG_INF("gen p256 dhkey");
#if CFG_ECC_PRINTK_KEY
	print_hex("remote pk: ", req->remote_pk, 64);
	print_hex("private key: ", req->private_key, 32);
#endif

	/* Convert X and Y coordinates from little-endian to
	 * big-endian (expected by the crypto API).
	 */
	sys_mem_swap(req->remote_pk, 32);
	sys_mem_swap(&req->remote_pk[32], 32);
	/* valid remote public key first and send the result to btc */
	err = ecc_valid_p256_pk(req->remote_pk);
	if (err) {
		goto done;
	}

	sys_mem_swap(req->private_key, 32);
	err = ecc_gen_p256_dhkey(req->remote_pk, req->private_key, rsp->dhkey);
	if (err) {
		goto done;
	}

	/* Convert dhkey from little-endian to big-endian */
	sys_mem_swap(rsp->dhkey, 32);
	rsp->id = req->id;
#if CFG_ECC_PRINTK_KEY
	print_hex("dhkey: ", rsp->dhkey, 32);
#endif

done:
	ecc_send(MSG_BT_GEN_P256_DHKEY, err, (int)req->id, rsp->dhkey, sizeof(rsp->dhkey));
	return err;
}

static void emulate_key(struct k_work *item)
{
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ecc");
#endif

	if (btc_ecc.size) {
		k_sleep(K_MSEC(1));		/* Ensure controler push all data, for easy process, just sleep. */
		rbuf_get_data(btc.msg_rx_id, (void *)btc_ecc.req, btc_ecc.size);
	}

	if (atomic_test_bit(btc_ecc.flag, PENDING_P192_PUB_KEY)) {
		emulate_le_p192_public_key();
		atomic_clear_bit(btc_ecc.flag, PENDING_P192_PUB_KEY);
	} else if (atomic_test_bit(btc_ecc.flag, PENDING_P192_DHKEY)) {
		emulate_le_p192_dhkey();
		atomic_clear_bit(btc_ecc.flag, PENDING_P192_DHKEY);
	} else if (atomic_test_bit(btc_ecc.flag, PENDING_P256_PUB_KEY)) {
		emulate_le_p256_public_key();
		atomic_clear_bit(btc_ecc.flag, PENDING_P256_PUB_KEY);
	} else if (atomic_test_bit(btc_ecc.flag, PENDING_P256_DHKEY)) {
		emulate_le_p256_dhkey();
		atomic_clear_bit(btc_ecc.flag, PENDING_P256_DHKEY);
	} else {
		LOG_ERR("Unhandled ECC request");
	}

	/* Trigger process next message */
	if (ipmsg_pending(btc.msg_rx_id)) {
		sys_write32(IRQ_ID_BT, NVIC_STIR);
	}

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ecc");
#endif
}
#endif

static void btc_msg_handler(void)
{
	uint16_t ret;
	rbuf_msg_t rx_msg = {0};

#ifdef CONFIG_BT_ECC_ACTS
	/* wait for ecc request processing to complte */
	if (atomic_get(btc_ecc.flag)) {
		return;
	}
#endif

	ret = rbuf_get_data(btc.msg_rx_id, &rx_msg, sizeof(rbuf_msg_t));
	if (ret != sizeof(rbuf_msg_t)) {
		return;
	}

	LOG_DBG("msg type: %x, data size: %u\n", rx_msg.type, rx_msg.data.w[0]);

	switch (rx_msg.type) {
	case MSG_BT_HCI_OK:
		k_sem_give(&btc.ready_sem);
		atomic_set_bit(btc.flags, BT_CPU_READY);
		break;
#ifdef CONFIG_BT_ECC_ACTS
	case MSG_BT_GEN_P192_PK:
		btc_ecc.size = 0;
		atomic_set_bit(btc_ecc.flag, PENDING_P192_PUB_KEY);
		break;
	case MSG_BT_GEN_P192_DHKEY:
		btc_ecc.size = sizeof(struct bt_gen_p192_dhkey_req);
		atomic_set_bit(btc_ecc.flag, PENDING_P192_DHKEY);
		break;
	case MSG_BT_GEN_P256_PK:
		btc_ecc.size = 0;
		atomic_set_bit(btc_ecc.flag, PENDING_P256_PUB_KEY);
		break;
	case MSG_BT_GEN_P256_DHKEY:
		btc_ecc.size = sizeof(struct bt_gen_p256_dhkey_req);
		atomic_set_bit(btc_ecc.flag, PENDING_P256_DHKEY);
		break;
#endif
	default:
		LOG_ERR("unknown msg(type %x)!", rx_msg.type);
		break;
	}

#ifdef CONFIG_BT_ECC_ACTS
	/* process ecc requests */
	if (atomic_get(btc_ecc.flag) > 0) {
		acts_work_submit(&btc_ecc.work);
		return;
	}
#endif
}

/* process hci rx buf after reciving */
static inline void read_complete(void)
{
#ifdef CONFIG_BT_HCI_RX_PRINT
	print_hex(rx_type_str[rx.hdr.type], rx.pkt, rx.pkt_len);
#endif
	if (btc.hci_cb && rx.pkt) {
		btc.hci_cb->recv(rx.pkt_len);
	}

	memset(&rx, 0, sizeof(struct hci_data));

	/* if hci rx rbuf is not empty and hci rx sem is zero, then read again */
	if (!k_sem_count_get(&btc.hci_rx_sem) && ipmsg_pending(btc.hci_rx_id)) {
		atomic_inc(&btc.hci_rx_cnt);
		k_sem_give(&btc.hci_rx_sem);
	}
}

static uint16_t btdrv_cal_acl_packet_need_len(uint8_t *hdr, uint16_t hdr_size, uint16_t exp_len)
{
	uint16_t handle, need_len, l2cap_len;
	uint8_t flags;

	handle = (hdr[1] <<8 | hdr[0]);
	flags = (handle >> 12) & 0xF;

	if (flags == BT_ACL_HDL_FLAG_START) {
		if (hdr_size >= 6) {
			l2cap_len = (hdr[5] << 8 | hdr[4]);
			need_len = l2cap_len + HCI_ACL_HDR_SIZE + HCI_L2CAP_HEAD_SIZE;
			if (l2cap_len > L2CAP_BR_MAX_MTU_A2DP_AAC || exp_len > need_len) {
				LOG_ERR("l2cap_len too length %d exp_len %d hdr %2x %2x %2x %2x %2x %2x\n",
						l2cap_len, exp_len, hdr[0], hdr[1], hdr[2], hdr[3], hdr[4], hdr[5]);
			}
			return (need_len > exp_len)? need_len : exp_len;
		} else {
			if (BT_MAX_RX_ACL_LEN) {
				/* If the packet split by controler in transfer,
				 * can't calculate the max need length, just alloc max length.
				 */
				return (L2CAP_BR_MAX_MTU_A2DP_AAC + HCI_ACL_HDR_SIZE + HCI_L2CAP_HEAD_SIZE);
			} else {
				/* default buffer lengh */
				return 0;
			}
		}
	} else {
		return exp_len;
	}
}

static void get_rx_buf(uint8_t *hdr, uint16_t hdr_size)
{
	uint8_t evt = 0;
	uint16_t need_buf_len;

	switch (rx.hdr.type) {
	case HCI_ACL:
		rx.pkt_len = (hdr[3]<<8 | hdr[2]) + HCI_ACL_HDR_SIZE;
		need_buf_len = btdrv_cal_acl_packet_need_len(hdr, hdr_size, rx.pkt_len);
		break;
	case HCI_SCO:
		rx.pkt_len = hdr[2] + HCI_SCO_HDR_SIZE;
		need_buf_len = rx.pkt_len;
		break;
	case HCI_EVT:
		evt = hdr[0];
		rx.pkt_len = hdr[1] + HCI_EVT_HDR_SIZE;
		need_buf_len = rx.pkt_len;
		break;
	case HCI_ISO:
		rx.pkt_len = (hdr[3]<<8 | hdr[2]) + HCI_ISO_HDR_SIZE;
		need_buf_len = rx.pkt_len;
		break;
	default:
		rx.pkt_len = 0;
		LOG_ERR("get_rx_buf unknow type %d !\n", rx.hdr.type);
		return;
	}

	if (btc.hci_cb && btc.hci_cb->get_buf) {
		rx.pkt = btc.hci_cb->get_buf(rx.hdr.type, evt, need_buf_len);
	}
}

static inline int read_hdr(void)
{
	uint32_t size = 0;
	void *data = NULL;
	uint8_t read_size = BT_HCI_HDR_SIZE - rx.hdr_len;

	if (rx.has_hdr) {
		return rx.hdr_len;
	}

	data = rbuf_get_claim(RBUF_FR_OF(btc.hci_rx_id), read_size, &size);
	if (!data) {
		if (size) {
			LOG_INF("get buf header failed size %d", size);
		}
		rbuf_info_dump(RBUF_FR_OF(btc.hci_rx_id));
		return 0;
	}

	if (size != read_size) {
		LOG_INF("get buf header failed read %d size %d", read_size, size);
	}

	memcpy(&rx.hdr_buff[rx.hdr_len], data, size);
	rx.hdr_len += size;
	rbuf_get_finish(RBUF_FR_OF(btc.hci_rx_id), size);

	if (rx.hdr_len == BT_HCI_HDR_SIZE) {
		rx.total = rx.hdr.len;
		rx.has_hdr = true;
	}

	return rx.hdr_len;
}

static inline int read_buf(void)
{
	unsigned int size = 0;
	void *data = NULL;
	uint16_t cpy_len = 0;

	data = rbuf_get_claim(RBUF_FR_OF(btc.hci_rx_id), rx.total, &size);
	if (!data) {
		LOG_INF("alloc hci rx rbuf failed total %d(%d) size %d", rx.total, rx.hdr.len, size);
		return 0;
	}

	/* l2cap_len too length to get rx buf, avoid get_rx_buf for part of data. */
	if (!rx.pkt && (rx.total == rx.hdr.len)) {
		get_rx_buf((uint8_t *)data, (uint16_t)size);
	}

	if (rx.pkt) {
		cpy_len = MIN(rx.pkt_len - rx.offset, size);
		memcpy(rx.pkt + rx.offset, data, cpy_len);
		rx.offset += cpy_len;
	} else {
		LOG_ERR("read_buf total %d drop data %d!\n", rx.total, size);
	}

	rbuf_get_finish(RBUF_FR_OF(btc.hci_rx_id), size);
	rx.total -= size;
	return size;
}

static int btc_log_handler(void *context, void *data, unsigned int size)
{
	unsigned int log_len = 0;
	char* tmp = (char*)data;

	if(tmp[0] != '<')
	{
		memcpy(log_buf, BTC_LOG_HEADER, BTC_LOG_HEADER_LEN);
		log_len += BTC_LOG_HEADER_LEN;
	}
	memcpy((void *)(log_buf+log_len), data, size);
	log_len +=  size;
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	log_buf[log_len++] = '\n';
	log_buf[log_len] = '\r';
	uart_dma_send_buf(log_buf, log_len);
#else
	log_buf[log_len] = '\0';
	printk("%s", log_buf);
#endif

	return size;
}

#if CONFIG_HCI_RX_THREAD
static void rx_thread(void *p1, void *p2, void *p3)
{
	unsigned int size = 0;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("rx thread started");

	while (1) {
		k_sem_take(&btc.hci_rx_sem, K_FOREVER);

		if (btc.hci_rx_cnt > 2){
			LOG_INF("hci_rx_cnt:%d",btc.hci_rx_cnt);		
		}

		if (atomic_get(&btc.hci_rx_cnt)) {
			atomic_dec(&btc.hci_rx_cnt);

			if (read_hdr() != BT_HCI_HDR_SIZE) {
				goto next_sem;
			}

			do {
				size = read_buf();
			} while (size && rx.total);

			if (!rx.total) {
				read_complete();
			} else {
				if (rx.pkt) {
					LOG_INF("read hci rx buf incompletely");
				} else {
					LOG_INF("read hci rx buf NULL");
				}
			}
		}
		else if (atomic_get(&btc.log_rx_cnt)) {
			atomic_dec(&btc.log_rx_cnt);

			if (!ipmsg_pending(btc.log_rx_id)) {
				goto next_sem;
			}

			ipmsg_recv(btc.log_rx_id, BTC_LOG_SIZE, btc_log_handler, NULL);
		}

next_sem:
		k_yield();
	}
}
#endif

static inline unsigned int send_buf(void)
{
	uint8_t *buf = NULL;
	uint16_t copy_len;
	unsigned int size = 0;

	buf = rbuf_put_claim(RBUF_FR_OF(btc.hci_tx_id), tx.total, &size);
	if (buf == NULL) {
		LOG_ERR("alloc hci tx rbuf failed");
		return 0;
	}

	if (!tx.has_hdr) {
		memcpy(buf, &tx.hdr, BT_HCI_HDR_SIZE);
		buf += BT_HCI_HDR_SIZE;
		copy_len = MIN(size - BT_HCI_HDR_SIZE, tx.pkt_len - tx.offset);
		tx.has_hdr = true;
	} else {
		copy_len = MIN(size, tx.pkt_len - tx.offset);
	}

	memcpy(buf, tx.pkt + tx.offset, copy_len);
	tx.offset += copy_len;

	rbuf_put_finish(RBUF_FR_OF(btc.hci_tx_id), size);
	tx.total -= size;

	return size;
}

/* data fmt: hdr(4) + payload(n) */
int btdrv_send(uint8_t type, uint8_t *data, uint16_t len)
{
	int err = 0;
	unsigned int size = 0;
	//uint32_t cycle = k_cycle_get_32();

	if (!atomic_test_bit(btc.flags, BT_CPU_READY)) {
		return -EINVAL;
	}

	if (!data || !len) {
		return -EINVAL;
	}

	tx.pkt = data;
	tx.total = ROUND_UP(len, 4) + BT_HCI_HDR_SIZE;
	tx.hdr.len = tx.total - BT_HCI_HDR_SIZE;
	tx.hdr.type = type;
	tx.pkt_len = len;

	do {
		size = send_buf();
	} while (size && tx.total);

	if (!tx.total) {
		ipmsg_notify(btc.dev);
#ifdef CONFIG_BT_HCI_TX_PRINT
		print_hex(tx_type_str[tx.hdr.type], tx.pkt, tx.pkt_len);
#endif
	} else {
		err = -EINVAL;
		LOG_ERR("send hci rbuf failed");
	}

	memset(&tx, 0, sizeof(struct hci_data));

	//LOG_DBG("tx cycle: %u\n", k_cycle_get_32() - cycle);
	return err;
}

struct btc_protocol_mailbox {
	/* message id */
	volatile uint32_t msg_magic : 8; /* 0xcc */
	volatile uint32_t msg_status : 2;  /* 0-null, 1-new message, 2-complite */
	volatile uint32_t msg_flags : 14;  /* reserved for future */
	volatile uint32_t msg_id : 8; /* 0x1 - configure cis xfer bypass; 0x02 - configure sco xfer bypass */
	/* message result */
	volatile uint32_t result;
	/* user-defined parameter list */
	volatile uint32_t param1;
	volatile uint32_t param2;
};

struct btc_protocol_mailbox btc_prt_mailbox __in_section_unique(DSP_BT_SHARE_RAM);

void * btdrv_get_mailbox_ctx(void)
{
	return &btc_prt_mailbox;
}

int btdrv_send_mailbox_sync(uint8_t msg_id, uint8_t *data, uint16_t len)
{
	int err = 0;
	unsigned int wait_times = 0;

	btc_prt_mailbox.msg_id = msg_id;

	btc_prt_mailbox.msg_flags = 0;
	btc_prt_mailbox.result = 0;
	btc_prt_mailbox.param1 = (uint32_t)data;
	btc_prt_mailbox.param2 = (uint32_t)len;

	btc_prt_mailbox.msg_magic = 0xcc;
	btc_prt_mailbox.msg_status = 1;

	ipmsg_notify(btc.dev);

	while (1) {
		if (btc_prt_mailbox.msg_status == 2) {
			err = btc_prt_mailbox.result;
			break;
		}
		k_busy_wait(100);
		wait_times++;
		/* FIXME: wait timeout 100ms */
		if (wait_times > 1000) {
			LOG_ERR("send mailbox 0x%x timeout", msg_id);
			err = -1;
			break;
		}
	}

	return err;
}

/* BTC interupt handler */
static void btc_recv_cb(void *context, void *arg)
{
	if (ipmsg_pending(btc.msg_rx_id)) {
		btc_msg_handler();
	}

	if (ipmsg_pending(btc.hci_rx_id)) {
#if CONFIG_HCI_RX_THREAD
		atomic_inc(&btc.hci_rx_cnt);
		k_sem_give(&btc.hci_rx_sem);
		if (btc.hci_rx_cnt > 2){
			LOG_INF("irq rx_cnt:%d",btc.hci_rx_cnt);		
		}		
#endif
	}

	if (ipmsg_pending(btc.log_rx_id)) {
		atomic_inc(&btc.log_rx_cnt);
		k_sem_give(&btc.hci_rx_sem);
	}

#ifdef CONFIG_BT_CTRL_TWS
	/* tws0 interrupt pending has been cleared */
	if (atomic_test_bit(btc.flags, BT_CPU_TWS0_PENDING) &&
		(tws_rx[0]->pcnt == tws_tx[0]->pcnt)) {
		atomic_clear_bit(btc.flags, BT_CPU_TWS0_PENDING);
		btdrv_tws_irq_enable_inner(BT_TWS_0, true);
	}

	if (atomic_test_bit(btc.flags, BT_CPU_TWS1_PENDING) &&
		(tws_rx[1]->pcnt == tws_tx[1]->pcnt)) {
		atomic_clear_bit(btc.flags, BT_CPU_TWS1_PENDING);
		btdrv_tws_irq_enable_inner(BT_TWS_1, true);
	}
#endif
}

#ifdef CONFIG_BT_CTRL_TWS
static void btc_tws0_cb(void *context, void *arg)
{
	if (btc.tws0_cb) {
		btc.tws0_cb(tws_rx[0]->bt_clk, tws_rx[0]->intra_off);
	}

	atomic_set_bit(btc.flags, BT_CPU_TWS0_PENDING);
	btdrv_tws_irq_disable_inner(BT_TWS_0, true);		/* Disable irq until controler clear tws irq pending */
	tws_tx[0]->pcnt++;
	ipmsg_notify(btc.dev);
}

BTDRV_RAM_FUNC static void btc_tws1_cb(void *context, void *arg)
{
	if (btc.tws1_cb) {
		btc.tws1_cb(tws_rx[1]->bt_clk, tws_rx[1]->intra_off);
	}

	atomic_set_bit(btc.flags, BT_CPU_TWS1_PENDING);
	btdrv_tws_irq_disable_inner(BT_TWS_1, true);		/* Disable irq until controler clear tws irq pending */
	tws_tx[1]->pcnt++;
	ipmsg_notify(btc.dev);
}
#endif

#ifdef CONFIG_PM_DEVICE
static void bt_log_controler(bool log_on)
{
	if (!atomic_test_bit(btc.flags, BT_CPU_READY)) {
		return;
	}

    printk("bt_log_controler:%d\n",log_on);
	if (log_on) {
		btc.msg.type = MSG_BT_LOG_ON;
		btc.msg.data.w[0] = btc.log_rx_id;
	} else {
		btc.msg.type = MSG_BT_LOG_OFF;
		btc.msg.data.w[0] = 1;		/* Log out from uart */
	}

	rbuf_put_data(btc.msg_tx_id, &btc.msg, sizeof(rbuf_msg_t));
	ipmsg_notify(btc.dev);
}

static void btc_pm_ctrl_cb(uint32_t command, uint32_t state)
{

	switch (state) {
	case PM_DEVICE_ACTION_RESUME:
		bt_log_controler(true);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		bt_log_controler(false);
		break;
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		break;
	case PM_DEVICE_ACTION_LATE_RESUME:
		break;
	default:
		break;;
	}
}
#else
#define btc_pm_ctrl_cb	NULL
#endif

static void set_mac(void)
{
	int ret;
	const uint8_t *default_mac = "f44efd12a3b4";
	uint8_t addr[6], mac_str[13];

#ifdef CONFIG_PROPERTY
	ret = property_get(CFG_BT_MAC, mac_str, 12);
	if(ret < 12) {
		LOG_WRN("property_get CFG_BT_MAC ret: %d", ret);
		memcpy(mac_str, default_mac, 12);
	}
#else
	memcpy(mac_str, default_mac, 12);
#endif

	ret = hex2bin(mac_str, 12, addr, 6);
	if (ret != 6) {
		LOG_ERR("invalid bt address");
		return;
	}

	btc.msg.type = MSG_BT_INIT;
	btc.msg.data.w[0] = addr[2] << 24 | addr[3] << 16 | addr[4] << 8 | addr[5];
	btc.msg.data.w[1] = addr[0] << 8 | addr[1];

	rbuf_put_data(btc.msg_tx_id, &btc.msg, sizeof(rbuf_msg_t));
}

#ifdef CONFIG_BT_CTRL_TWS
static struct btc_tws *tws_init(uint32_t *id)
{
	struct btc_tws *tws;
	*id = ipmsg_create(RBUF_RAW, BT_TWS_SIZE);
	if (*id == 0) {
		return NULL;
	}

	rbuf_t *rbuf = RBUF_FR_OF(*id);
	tws = (void *)RBUF_FR_OF(rbuf->buf_off);

	memset(tws, 0, sizeof(struct btc_tws));
	return tws;
}
#endif

static void send_init_msg(void)
{
	set_mac();

#ifdef CONFIG_BT_CTRL_RF_DEBUG
	btc.msg.type = MSG_BT_MDM_RF_DEBUG;
	rbuf_put_data(btc.msg_tx_id, &btc.msg, sizeof(rbuf_msg_t));
#endif

	btc.msg.type = MSG_BT_HCI_BUF;
	btc.msg.data.w[0] = btc.hci_rx_id;  /* bt -> main */
	btc.msg.data.w[1] = btc.hci_tx_id;  /* main -> bt */
	btc.msg.data.w[2] = btc.log_rx_id;  /* bt -> main */
	rbuf_put_data(btc.msg_tx_id, &btc.msg, sizeof(rbuf_msg_t));

#ifdef CONFIG_BT_CTRL_TWS
	btc.msg.type = MSG_BT_TWS_BUF;
	btc.msg.data.w[0] = btc.tws_rx_id[0];  /* bt -> main */
	btc.msg.data.w[1] = btc.tws_tx_id[0];  /* main -> bt */
	btc.msg.data.w[2] = btc.tws_rx_id[1];
	btc.msg.data.w[3] = btc.tws_tx_id[1];
	rbuf_put_data(btc.msg_tx_id, &btc.msg, sizeof(rbuf_msg_t));
#endif
}

static void init_rbuf(void)
{
	btc.msg_tx_id = rbuf_msg_create(CPU, BT, RB_MSG_SIZE);
	btc.msg_rx_id = rbuf_msg_create(BT, CPU, RB_MSG_SIZE);
	LOG_INF("msg tx id: %d, msg rx id: %d", btc.msg_tx_id, btc.msg_rx_id);

	if (pbtc_usr_config->BTC_Log == 2){
		btc.log_rx_id = ipmsg_create(RBUF_RAW, BTC_LOG_SIZE);
		LOG_INF("log id: %d", btc.log_rx_id);
	}

	btc.hci_rx_id = ipmsg_create(RBUF_RAW, CONFIG_BT_HCI_RX_RBUF_SIZE);
	btc.hci_tx_id = ipmsg_create(RBUF_RAW, CONFIG_BT_HCI_TX_RBUF_SIZE);
	LOG_INF("hci rx id: %d, hci tx id: %d", btc.hci_rx_id, btc.hci_tx_id);

#ifdef CONFIG_BT_CTRL_TWS
	for (uint8_t i = 0; i < 2; i++) {
		tws_rx[i] = tws_init(&btc.tws_rx_id[i]);
		tws_tx[i] = tws_init(&btc.tws_tx_id[i]);
		LOG_INF("tws%d rx id: %d, tx id: %d", i, btc.tws_rx_id[i], btc.tws_tx_id[i]);
	}
#endif

	send_init_msg();
}

int btdrv_set_init_param(btdrv_init_param_t *param)
{
	ipmsg_btc_init_param_t btc_param;

#ifdef CONFIG_COMPENSATION_ACTS
	uint32_t cap_value = 0xFF;
    int ret = 0;
#endif

	btc.dev = (struct device *)device_get_binding("BTC");
	if (!btc.dev) {
		LOG_ERR("get device BTC failed");
		return -EINVAL;
	}

	btc_param.hosc_capacity = param->hosc_capacity;

#ifdef CONFIG_COMPENSATION_ACTS
	if (!param->set_hosc_cap)
	{
		ret = freq_compensation_get_cap(&cap_value);
		if(ret == TRIM_CAP_READ_NO_ERROR){
			btc_param.hosc_capacity = (cap_value & 0xFF);
		}
		printk("efuse cap valid:%d cap:0x%x\n",ret,cap_value);
	}
#endif
    btc_param.set_hosc_cap = 1;
	btc_param.set_max_rf_power = param->set_max_rf_power;
	btc_param.set_ble_rf_power = param->set_ble_rf_power;
	btc_param.bt_max_rf_tx_power = param->bt_max_rf_tx_power;
	btc_param.ble_rf_tx_power = param->ble_rf_tx_power;
	memcpy(&btc_param.rf_param, &param->rf_param, sizeof(btc_param.rf_param));
	ipmsg_init_param(btc.dev, &btc_param);

	return 0;
}



#if defined(CONFIG_BT_FCC_TEST) || defined(CONFIG_BT_NS_TEST)

#ifdef CONFIG_SOC_SERIES_CUCKOO
#define FCC_NS_USE_UART_0 1

#define BT_UART_MFP_SEL 12
#if FCC_NS_USE_UART_0
static const struct acts_pin_config btc_fcc_uart[] = {
	{1, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
	{2, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
};
#else
static const struct acts_pin_config btc_fcc_uart[] = {
	{8, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
	{9, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
};
#endif

#else

#define FCC_NS_USE_UART_0 0
#define BT_UART_MFP_SEL 23
static const struct acts_pin_config btc_fcc_uart[] = {
	{28, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
	{29, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
};
#endif

#if FCC_NS_USE_UART_0

#define FCC_UART_STA	UART0_STA
#define FCC_UART_SET_BR	115200
#define FCC_UART	"UART_0"
static struct device *uart_dev;


static int _stdout_hook_null(int c)
{
	(void)(c);

	return EOF;
}

static int fcc_uart_init(void)
{
	struct uart_config cfg;

	uart_dev = (struct device *)device_get_binding(FCC_UART);
	if (uart_dev == NULL) {
		printk("cannot get device %s\n", FCC_UART);
		return -ENODEV;
	}

	extern void __printk_hook_install(int (*fn)(int));
	extern void __stdout_hook_install(int (*fn)(int));
	__printk_hook_install(_stdout_hook_null);
	__stdout_hook_install(_stdout_hook_null);

	uart_config_get(uart_dev, &cfg);
	cfg.baudrate = FCC_UART_SET_BR;
	uart_configure(uart_dev, &cfg);

	uart_dma_send_stop(uart_dev);
	uart_fifo_switch(uart_dev, 1, UART_FIFO_TYPE_CPU);
	uart_dma_receive_stop(uart_dev);
	uart_fifo_switch(uart_dev, 0, UART_FIFO_TYPE_CPU);

	uart_irq_rx_disable(uart_dev);
	uart_irq_tx_disable(uart_dev);

	return 0;
}



#endif

static void btdrv_run_fcc(void)
{
	unsigned int *cmd_reg_write, *cmd_reg_write_addr, *cmd_reg_write_val;
	unsigned int *cmd_reg_read, *cmd_reg_read_addr, *cmd_reg_read_val;
	unsigned int *cmd_efuse_write, *cmd_efuse_write_addr, *cmd_efuse_write_val;
	unsigned int *cmd_efuse_read, *cmd_efuse_read_addr, *cmd_efuse_read_val;
	unsigned int *cmd_bt_test_end;
	unsigned int *cmd_rf_printf, *cmd_rf_printf_addr;

	k_sched_lock();

	cmd_reg_write =(unsigned int *)(INTER_RAM_ADDR + 2*4);
	cmd_reg_write_addr =(unsigned int *)(INTER_RAM_ADDR + 3*4);
	cmd_reg_write_val =(unsigned int *)(INTER_RAM_ADDR + 4*4);
	*cmd_reg_write = 0;

	cmd_reg_read =(unsigned int *)(INTER_RAM_ADDR + 5*4);
	cmd_reg_read_addr =(unsigned int *)(INTER_RAM_ADDR + 6*4);
	cmd_reg_read_val =(unsigned int *)(INTER_RAM_ADDR + 7*4);
	*cmd_reg_read = 0;

	cmd_efuse_write =(unsigned int *)(INTER_RAM_ADDR + 8*4);
	cmd_efuse_write_addr =(unsigned int *)(INTER_RAM_ADDR + 9*4);
	cmd_efuse_write_val =(unsigned int *)(INTER_RAM_ADDR + 10*4);
	*cmd_efuse_write = 0;

	cmd_efuse_read =(unsigned int *)(INTER_RAM_ADDR + 11*4);
	cmd_efuse_read_addr =(unsigned int *)(INTER_RAM_ADDR + 12*4);
	cmd_efuse_read_val =(unsigned int *)(INTER_RAM_ADDR + 13*4);
	*cmd_efuse_read = 0;

	cmd_bt_test_end =(unsigned int *)(INTER_RAM_ADDR + 14*4);
	*cmd_bt_test_end = 0;

	cmd_rf_printf=(unsigned int *)(INTER_RAM_ADDR + 1024*4);
	cmd_rf_printf_addr =(unsigned int *)(INTER_RAM_ADDR + 1028*4);
	*cmd_rf_printf = 0;

#if defined(CONFIG_BT_FCC_TEST)
	LOG_INF("Enter FCC test mode!!\n");
#elif defined(CONFIG_BT_NS_TEST)
	LOG_INF("Enter Non-Signaling test mode!!\n");
#endif

#if FCC_NS_USE_UART_0
	fcc_uart_init();
#endif

	k_sleep(K_MSEC(10));
	acts_pinmux_setup_pins(btc_fcc_uart, ARRAY_SIZE(btc_fcc_uart));

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(PARTIAL_WAKE_LOCK);
#endif

#ifdef CONFIG_BT_NS_TEST
	sys_write32((sys_read32(VOUT_CTL1_S1) & ~(0xf << 0)) | (0xd << 0), VOUT_CTL1_S1);//vdd=1.2v
#endif
	sys_write32(sys_read32(WD_CTL) & ~(1 << 4), WD_CTL);

	while (true) {
		if (*cmd_reg_write == 0x88888888) {
			LOG_INF("cmd_reg_write\n");
			*cmd_reg_write = 0;
		}

		if (*cmd_reg_read == 0x88888888) {
			LOG_INF("cmd_reg_read\n");
			*cmd_reg_read = 0;
		}

		if (*cmd_efuse_write == 0x88888888) {
			LOG_INF("cmd_efuse_write\n");
			*cmd_efuse_write = 0;
		}

		if (*cmd_efuse_read == 0x88888888) {
			if ((*cmd_efuse_read_addr) == 6) {
				if (soc_atp_get_fcc_param(0, cmd_efuse_read_val)) {
					*cmd_efuse_read_val = 0;
					LOG_INF("invoked efuse\n");
				}
			} else if ((*cmd_efuse_read_addr) == 7) {
				if (soc_atp_get_fcc_param(1, cmd_efuse_read_val)) {
					*cmd_efuse_read_val = 0;
					LOG_INF("invoked efuse\n");
				}
			} else {
				*cmd_efuse_read_val = 0;
			}

			LOG_INF("cmd_efuse_read addr 0x%x value 0x%x\n", *cmd_efuse_read_addr, *cmd_efuse_read_val);
			*cmd_efuse_read = 0;
		}

		if (*cmd_rf_printf == 0x88888888) {
			LOG_INF("cmd_rf_printf %s\n", (const char *)cmd_rf_printf_addr);
			*cmd_rf_printf = 0;
		}

		if (*cmd_bt_test_end == 0x88888888) {
			LOG_INF("cmd_bt_test_end\n");
			*cmd_bt_test_end = 0;
			k_sched_unlock();
			break;
		}
	}
}

#endif

int btdrv_init(btdrv_hci_cb_t *cb)
{
	int ret;
	uint8_t irq;

	LOG_INF("bt driver init");

	/* init dsp bt share ram */
	memset(&btc_prt_mailbox, 0x0, sizeof(btc_prt_mailbox));

	if (atomic_test_bit(btc.flags, BT_CPU_ENABLE)) {
		goto start_cpu;
	}

	if (!cb) {
		return -EINVAL;
	}
	btc.hci_cb = cb;

#ifdef CONFIG_BT_ECC_ACTS
	ret = ecc_init();
	if (ret) {
		LOG_ERR("Ecc init failed");
	}
	
    k_delayed_work_init(&btc_ecc.work, emulate_key);
#endif

	pbtc_usr_config = ipmsg_get_btc_debug_config();

	btc.dev = (struct device *)device_get_binding("BTC");
	if (!btc.dev) {
		LOG_ERR("get device BTC failed");
		return -EINVAL;
	}

	irq = IPMSG_BTC_IRQ;
	ipmsg_register_callback(btc.dev, btc_recv_cb, &irq);
#ifdef CONFIG_BT_CTRL_TWS
	irq = IPMSG_TWS0_IRQ;
	/* Default enable, disable in tws irq cb, enable after controler clear tws irq pending. */
	btc.tws_irq_enable[BT_TWS_0] |= TWS_IRQ_INNER_ENABLE_BIT;
	ipmsg_register_callback(btc.dev, btc_tws0_cb, &irq);
	irq = IPMSG_TWS1_IRQ;
	btc.tws_irq_enable[BT_TWS_1] |= TWS_IRQ_INNER_ENABLE_BIT;
	ipmsg_register_callback(btc.dev, btc_tws1_cb, &irq);
#endif

	if (pbtc_usr_config->BTC_Log == 2){
		irq = IPMSG_REG_PW_CTRL;
		ipmsg_register_callback(btc.dev, (ipmsg_callback_t)btc_pm_ctrl_cb, &irq);
	}


	init_rbuf();

#if CONFIG_HCI_RX_THREAD
	/* Start RX thread */
	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_thread_stack),
			rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_HCI_RX_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&rx_thread_data, "BT HCI RX");
#endif

	atomic_set_bit(btc.flags, BT_CPU_ENABLE);

start_cpu:
	if (atomic_test_bit(btc.flags, BT_CPU_READY)) {
		return 0;
	}

	ret = ipmsg_load(btc.dev, (void *)BTC_BIN_ADDR, BTC_BIN_SIZE);
	if (ret) {
		LOG_ERR("load bt bin failed");
		return -EINVAL;
	}

	/* Start BT CPU */
	ret = ipmsg_start(btc.dev, NULL, NULL);
	if (ret) {
		LOG_ERR("start bt cpu failed");
		return -EINVAL;
	}

#if defined(CONFIG_BT_FCC_TEST) || defined(CONFIG_BT_NS_TEST)
	btdrv_run_fcc();
#endif

	/* BT CPU will let us know when it's ready */
	LOG_INF("wait bt cpu ready...");
	k_sem_take(&btc.ready_sem, K_FOREVER);
	LOG_INF("bt cpu ready");

	return 0;
}

int btdrv_exit(void)
{
	if (!atomic_test_bit(btc.flags, BT_CPU_ENABLE)) {
		return -ENODEV;
	}

	LOG_INF("exit");

	ipmsg_stop(btc.dev);
	btc.hci_cb = NULL;
#ifdef CONFIG_BT_CTRL_TWS
	btc.tws0_cb = NULL;
	btc.tws1_cb = NULL;
#endif

	// destroy default message queue
	rbuf_msg_destroy(CPU, BT);
	rbuf_msg_destroy(BT, CPU);

	ipmsg_destroy(btc.hci_rx_id);
	ipmsg_destroy(btc.hci_tx_id);
	ipmsg_destroy(btc.log_rx_id);

#ifdef CONFIG_BT_CTRL_TWS
	for (uint8_t i = 0; i < 2; i++) {
		ipmsg_destroy(btc.tws_rx_id[i]);
		ipmsg_destroy(btc.tws_tx_id[i]);
	}
#endif

	atomic_clear(btc.flags);

	return 0;
}

int btdrv_reset(void)
{
	/* Reset BT CPU */
	atomic_clear_bit(btc.flags, BT_CPU_READY);

	ipmsg_stop(btc.dev);

	send_init_msg();

	return btdrv_init(NULL);
}

#ifdef CONFIG_BT_CTRL_TWS
static int btdrv_tws_irq_enable_inner(uint8_t index, bool inner_call)
{
	if (index > BT_TWS_1) {
		return -EINVAL;
	}

	if (!atomic_test_bit(btc.flags, BT_CPU_ENABLE)) {
		return -ENOENT;
	}

	if (inner_call) {
		btc.tws_irq_enable[index] |= TWS_IRQ_INNER_ENABLE_BIT;
	} else {
		btc.tws_irq_enable[index] |= TWS_IRQ_EXT_ENABLE_BIT;
	}

	if (btc.tws_irq_enable[index] == TWS_IRQ_ALL_ENABLE_BIT) {
		ipmsg_tws_irq_enable(btc.dev, index + 1);
	}

	return 0;
}

BTDRV_RAM_FUNC static int btdrv_tws_irq_disable_inner(uint8_t index, bool inner_call)
{
	uint8_t per_enable;

	if (index > BT_TWS_1) {
		return -EINVAL;
	}

	if (!atomic_test_bit(btc.flags, BT_CPU_ENABLE)) {
		return -ENOENT;
	}

	per_enable = btc.tws_irq_enable[index];
	if (inner_call) {
		btc.tws_irq_enable[index] &= (~TWS_IRQ_INNER_ENABLE_BIT);
	} else {
		btc.tws_irq_enable[index] &= (~TWS_IRQ_EXT_ENABLE_BIT);
	}

	if (per_enable == TWS_IRQ_ALL_ENABLE_BIT) {
		ipmsg_tws_irq_disable(btc.dev, index + 1);
	}

	return 0;
}

int btdrv_tws_irq_enable(uint8_t index)
{
	return btdrv_tws_irq_enable_inner(index, false);
}

int btdrv_tws_irq_disable(uint8_t index)
{
	return btdrv_tws_irq_disable_inner(index, false);
}

int btdrv_tws_irq_cb_set(uint8_t index, btdrv_tws_cb_t cb)
{
	if (index > BT_TWS_1) {
		return -EINVAL;
	}

	if (!atomic_test_bit(btc.flags, BT_CPU_ENABLE)) {
		return -ENOENT;
	}

	if (index == BT_TWS_0) {
		btc.tws0_cb = cb;
	} else if (index == BT_TWS_1) {
		btc.tws1_cb = cb;
	}

	return 0;
}

int btdrv_tws_set_mode(uint8_t index, uint8_t mode)
{
	if (index > BT_TWS_1) {
		return -EINVAL;
	}

	tws_tx[index]->mode = mode;
	return 0;
}

int btdrv_tws_data_read(uint8_t index, uint8_t *buf)
{
#if CFG_USE_TWS_SHARERAM_PKT
	if ((tws_rx[index]->wcnt - tws_rx[index]->rcnt) == 0) {
		return -ENODATA;
	}

	memcpy(buf, tws_rx[index]->data, tws_rx[index]->data_len);
	tws_rx[index]->rcnt++;

	return tws_rx[index]->data_len;
#else
	return 0;
#endif
}

int btdrv_tws_data_write(uint8_t index, uint8_t *data, uint16_t len)
{
#if CFG_USE_TWS_SHARERAM_PKT
	if (!data || !len) {
		return -EINVAL;
	}

	if ((tws_tx[index]->wcnt - tws_tx[index]->rcnt) >= 1) {
		return -ENOBUFS;
	}

	memcpy(tws_tx[index]->data, data, len);
	tws_tx[index]->wcnt++;

	return len;
#else
	return 0;
#endif
}
#endif

extern int bt_bqb_vs_write_bb_reg(uint32_t addr, uint32_t val);
extern int bt_bqb_vs_read_bb_reg(uint32_t addr, uint8_t size);
extern int bt_bqb_vs_write_rf_reg(uint16_t addr, uint16_t val);
extern int bt_bqb_vs_read_rf_reg(uint16_t addr, uint8_t size);
extern int bt_stack_vs_write_bb_reg(uint32_t addr, uint32_t val);
extern int bt_stack_vs_read_bb_reg(uint32_t addr, uint8_t size);
extern int bt_stack_vs_write_rf_reg(uint16_t addr, uint16_t val);
extern int bt_stack_vs_read_rf_reg(uint16_t addr, uint8_t size);
extern bool bt_bqb_is_in_test(void);

int bt_vs_write_bb_reg(uint32_t addr, uint32_t val)
{
#ifdef CONFIG_BT_CTRL_BQB
	if (bt_bqb_is_in_test()) {
		return bt_bqb_vs_write_bb_reg(addr, val);
	} else {
#if defined(CONFIG_BT_HCI) || defined(CONFIG_BT_HCI_ACTS)
		return bt_stack_vs_write_bb_reg(addr, val);
#else
		return 0;
#endif
	}
#else
	return bt_stack_vs_write_bb_reg(addr, val);
#endif
}

int bt_vs_read_bb_reg(uint32_t addr, uint8_t size)
{
#ifdef CONFIG_BT_CTRL_BQB
	if (bt_bqb_is_in_test()) {
		return bt_bqb_vs_read_bb_reg(addr, size);
	} else {
#if defined(CONFIG_BT_HCI) || defined(CONFIG_BT_HCI_ACTS)
		return bt_stack_vs_read_bb_reg(addr, size);
#else
		return 0;
#endif
	}
#else
	return bt_stack_vs_read_bb_reg(addr, size);
#endif
}

int bt_vs_write_rf_reg(uint16_t addr, uint16_t val)
{
#ifdef CONFIG_BT_CTRL_BQB
	if (bt_bqb_is_in_test()) {
		return bt_bqb_vs_write_rf_reg(addr, val);
	} else {
#if defined(CONFIG_BT_HCI) || defined(CONFIG_BT_HCI_ACTS)
		return bt_stack_vs_write_rf_reg(addr, val);
#else
		return 0;
#endif
	}
#else
	return bt_stack_vs_write_rf_reg(addr, val);
#endif
}

int bt_vs_read_rf_reg(uint16_t addr, uint8_t size)
{
#ifdef CONFIG_BT_CTRL_BQB
	if (bt_bqb_is_in_test()) {
		return bt_bqb_vs_read_rf_reg(addr, size);
	} else {
#if defined(CONFIG_BT_HCI) || defined(CONFIG_BT_HCI_ACTS)
		return bt_stack_vs_read_rf_reg(addr, size);
#else
		return 0;
#endif
	}
#else
	return bt_stack_vs_read_rf_reg(addr, size);
#endif
}
