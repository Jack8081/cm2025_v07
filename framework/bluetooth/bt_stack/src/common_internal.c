/** @file common_internel.c
 * @brief Bluetooth common internel used.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 #define LOG_MODULE_NAME common_internal
#include <logging/log.h>
LOG_MODULE_DECLARE(os, LOG_LEVEL_INF);
#include <zephyr.h>
#include "stdlib.h"
#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/hfp_hf.h>
#include <acts_bluetooth/l2cap.h>
#include <acts_bluetooth/a2dp.h>
#include <acts_bluetooth/avrcp_cttg.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "rfcomm_internal.h"
#include "hfp_internal.h"
#include "avdtp_internal.h"
#include "a2dp_internal.h"
#include "avrcp_internal.h"
#include "hid_internal.h"
#include "keys.h"
#include "att_internal.h"

#include "common_internal.h"
#include "bt_porting_inner.h"

#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

#define DEFAULT_BT_CLASE_OF_DEVICE  	0x240404		/* Rendering,Audio, Audio/Video, Wearable Headset Device */
#define BT_DID_ACTIONS_COMPANY_ID		0x03E0
#define BT_DID_DEFAULT_COMPANY_ID		0xFFFF
#define BT_DID_PRODUCT_ID				0x0001
#define BT_DID_VERSION					0x0001

/* For controler acl tx
 * must large than(le acl data pkts + br acl data pkts)
 */
#define BT_ACL_TX_MAX		CONFIG_BT_CONN_TX_MAX
#define BT_BR_RESERVE_PKT	0		/* Avoid send data used all pkts, put in conn tx queue for debug send time */
#define BT_LE_RESERVE_PKT	1		/* Avoid send data used all pkts */

/* rfcomm */
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
#define RFCOMM_MAX_CREDITS_BUF         (CONFIG_BT_ACL_RX_COUNT - 1)
#else
#define RFCOMM_MAX_CREDITS_BUF         (CONFIG_BT_RX_BUF_COUNT - 1)
#endif

#if (RFCOMM_MAX_CREDITS_BUF > 7)
#define RFCOMM_MAX_CREDITS			7
#else
#define RFCOMM_MAX_CREDITS			RFCOMM_MAX_CREDITS_BUF
#endif

#ifdef CONFIG_BT_AVRCP_VOL_SYNC
#define CONFIG_SUPPORT_AVRCP_VOL_SYNC		1
#else
#define CONFIG_SUPPORT_AVRCP_VOL_SYNC		0
#endif

#define BT_ENABLE_AUTH_NO_BONDING			1

#ifdef CONFIG_BT_PTS_TEST
#define BT_PTS_TEST_MODE		1
#else
#define BT_PTS_TEST_MODE		0
#endif

#if (CONFIG_ACTS_BT_LOG_LEVEL >= 3)
#define BT_STACK_DEBUG_LOG		1
#else
#define BT_STACK_DEBUG_LOG		0
#endif

#ifdef CONFIG_BT_RFCOMM
#define BT_RFCOMM_L2CAP_MTU		CONFIG_BT_RFCOMM_L2CAP_MTU
#else
#define BT_RFCOMM_L2CAP_MTU		650		/* Just for compile */
#endif

#define L2CAP_BR_MIN_MTU	48

/* hfp */
#define SDP_CLIENT_DISCOVER_USER_BUF_LEN		256

/* Bt rx/tx data pool size */
#define BT_DATA_POOL_RX_SIZE		(4*1024)		/* TODO: Better  calculate by CONFIG */
#define BT_DATA_POOL_TX_SIZE		(4*1024)		/* TODO: Better  calculate by CONFIG */

typedef void (*property_flush_cb)(const char *key);
static property_flush_cb flush_cb_func;

static const struct bt_inner_const_t bt_inner_const_v = {
	.max_conn = CONFIG_BT_MAX_CONN,
	.br_max_conn = CONFIG_BT_MAX_BR_CONN,
	.le_max_paired = CFG_BT_MAX_LE_PAIRED,
	.br_max_paired = CONFIG_BT_MAX_BR_PAIRED,
	.br_reserve_pkts = BT_BR_RESERVE_PKT,
	.le_reserve_pkts = BT_LE_RESERVE_PKT,
	.acl_tx_max = BT_ACL_TX_MAX,
	.rfcomm_max_credits = RFCOMM_MAX_CREDITS,
	.enable_auth_no_bonding = BT_ENABLE_AUTH_NO_BONDING,
	.pts_test_mode = BT_PTS_TEST_MODE,
	.debug_log = BT_STACK_DEBUG_LOG,
	.l2cap_tx_mtu = CONFIG_BT_L2CAP_TX_MTU,
	.rfcomm_l2cap_mtu = BT_RFCOMM_L2CAP_MTU,
	.avdtp_rx_mtu = BT_AVDTP_MAX_MTU,
	.br_max_mtu_a2dp = L2CAP_BR_MAX_MTU_A2DP,
	.ag_features = BT_HFP_AG_SUPPORTED_FEATURES,
	.bt_att_mtu = BT_ATT_MTU,
};

static struct bt_inner_variate_t bt_inner_variate_v = {
	.sniff_enable = 1,
	.linkkey_miss_reject = 0,
	.max_pair_list_num = 8,
	.le_adv_random_addr = 1,
	.avrcp_vol_sync = CONFIG_SUPPORT_AVRCP_VOL_SYNC,
	.hf_features = BT_HFP_HF_SUPPORTED_FEATURES,
	.bt_version = BT_HCI_VERSION_5_3,
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
    .bt_br_tr_enable = 0,
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_BQB
    .leaudio_bqb_enable = 0,
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
    .leaudio_bqb_sr_enable = 0,
#endif
	.bt_sub_version = 0x0001,
	.classOfDevice = DEFAULT_BT_CLASE_OF_DEVICE,
	.bt_device_id = {BT_DID_ACTIONS_COMPANY_ID, BT_DID_DEFAULT_COMPANY_ID, BT_DID_PRODUCT_ID, BT_DID_VERSION},
};

const struct bt_inner_value_t bt_inner_value = {
	.const_t = &bt_inner_const_v,
	.variate_t = &bt_inner_variate_v,
};

BT_BUF_POOL_ALLOC_DATA_DEFINE(host_rx_pool, BT_DATA_POOL_RX_SIZE);
BT_BUF_POOL_ALLOC_DATA_DEFINE(host_tx_pool, BT_DATA_POOL_TX_SIZE);

/* Pool for outgoing BR/EDR signaling packets, min MTU is 48 */
BT_BUF_POOL_DEFINE(br_sig_pool, (CONFIG_BT_MAX_BR_PAIRED + 1),
		    BT_L2CAP_BUF_SIZE(L2CAP_BR_MIN_MTU), 4, NULL, &host_tx_pool);

/* hfp */
BT_BUF_POOL_DEFINE(hfp_pool, (CONFIG_BT_MAX_BR_PAIRED + 1),
			 BT_RFCOMM_BUF_SIZE(BT_HF_CLIENT_MAX_PDU), 4, NULL, &host_tx_pool);

BT_BUF_POOL_DEFINE(sdp_client_discover_pool, (CONFIG_BT_MAX_BR_PAIRED + 1),
			 SDP_CLIENT_DISCOVER_USER_BUF_LEN, 4, NULL, &host_tx_pool);

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
BT_BUF_POOL_DEFINE(prep_pool, CONFIG_BT_ATT_PREPARE_COUNT, BT_ATT_MTU,
								4, NULL, &host_tx_pool);
#endif
BT_BUF_POOL_DEFINE(att_tx_pool, 20, 30, 4, NULL, &host_tx_pool);

/* L2cap br */
struct bt_l2cap_br bt_l2cap_br_pool[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;
struct bt_keys_link_key br_key_pool[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* conn */
struct bt_conn acl_conns[CONFIG_BT_MAX_CONN] __IN_BT_SECTION;
/* CONFIG_BT_MAX_SCO_CONN == CONFIG_BT_MAX_BR_CONN */
struct bt_conn sco_conns[CONFIG_BT_MAX_BR_PAIRED] __IN_BT_SECTION;

/* hfp, Wait todo: Can use union to manager  hfp_hf_connection and hfp_ag_connection, for reduce memory */
struct bt_hfp_hf hfp_hf_connection[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;
struct bt_hfp_ag hfp_ag_connection[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* a2dp */
struct bt_avdtp_conn avdtp_conn[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* avrcp */
struct bt_avrcp avrcp_connection[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* rfcomm */
struct bt_rfcomm_session bt_rfcomm_connection[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* hid */
struct bt_hid_conn hid_conn[CONFIG_BT_MAX_BR_PAIRED] __IN_BT_SECTION;

void *bt_malloc(size_t size)
{
	return bt_mem_malloc(size);
}

void bt_free(void *ptr)
{
	return bt_mem_free(ptr);
}

int bt_property_set(const char *key, char *value, int value_len)
{
	int ret = -EIO;

#ifdef CONFIG_PROPERTY
	ret = property_set(key, value, value_len);
	if (flush_cb_func) {
		flush_cb_func(key);
	}
#endif
	return ret;
}

int bt_property_get(const char *key, char *value, int value_len)
{
	int ret = -EIO;

#ifdef CONFIG_PROPERTY
	ret = property_get(key, value, value_len);
#endif
	return ret;
}

int bt_property_reg_flush_cb(void *cb)
{
	flush_cb_func = cb;

	return 0;
}

int bt_init_config_param(struct bt_stack_config_param *param)
{
	bt_inner_variate_v.sniff_enable = param->sniff_enable;
	LOG_INF("sniff_enable:%d",param->sniff_enable);
	bt_inner_variate_v.linkkey_miss_reject = param->linkkey_miss_reject;
	LOG_INF("linkkey_miss_reject:%d",param->linkkey_miss_reject);
	bt_inner_variate_v.max_pair_list_num = param->max_pair_list_num;
	bt_inner_variate_v.le_adv_random_addr = param->le_adv_random_addr;
	bt_inner_variate_v.bt_version = param->bt_version;
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
    bt_inner_variate_v.bt_br_tr_enable = param->bt_br_tr_enable;
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_BQB
    bt_inner_variate_v.leaudio_bqb_enable = param->leaudio_bqb_enable;
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
    bt_inner_variate_v.leaudio_bqb_sr_enable = param->leaudio_bqb_sr_enable;
#endif
	bt_inner_variate_v.bt_sub_version = param->bt_sub_version;
	bt_inner_variate_v.classOfDevice = param->classOfDevice;
	memcpy(bt_inner_variate_v.bt_device_id, param->bt_device_id, sizeof(bt_inner_variate_v.bt_device_id));

	if (bt_inner_variate_v.max_pair_list_num < 2) {
		bt_inner_variate_v.max_pair_list_num = 2;
	} else if (bt_inner_variate_v.max_pair_list_num > 8) {
		bt_inner_variate_v.max_pair_list_num = 8;
	}

	if (!bti_pts_test_mode() && param->avrcp_vol_sync) {
		bt_inner_variate_v.avrcp_vol_sync = 1;
	} else {
		bt_inner_variate_v.avrcp_vol_sync = 0;
	}
	LOG_INF("avrcp_vol_sync:%d",param->avrcp_vol_sync);

	if (!bti_pts_test_mode() && param->hfp_sp_codec_neg) {
		bt_inner_variate_v.hf_features |= BT_CONFIG_HFP_HF_FEATURE_CODEC_NEG;
	} else {
		bt_inner_variate_v.hf_features &= (~BT_CONFIG_HFP_HF_FEATURE_CODEC_NEG);
	}
	LOG_INF("hfp_sp_codec_neg:%d",param->hfp_sp_codec_neg);

	if (!bti_pts_test_mode() && param->hfp_sp_voice_recg) {
		bt_inner_variate_v.hf_features |= BT_HFP_HF_FEATURE_VOICE_RECG;
	} else {
		bt_inner_variate_v.hf_features &= (~BT_HFP_HF_FEATURE_VOICE_RECG);
	}
	LOG_INF("hfp_sp_voice_recg:%d",param->hfp_sp_voice_recg);

	if (!bti_pts_test_mode() && param->hfp_sp_3way_call) {
		bt_inner_variate_v.hf_features |= BT_HFP_HF_FEATURE_3WAY_CALL;
	} else {
		bt_inner_variate_v.hf_features &= (~BT_HFP_HF_FEATURE_3WAY_CALL);
	}
	LOG_INF("hfp_sp_3way_call:%d",param->hfp_sp_3way_call);

	if (!bti_pts_test_mode() && param->hfp_sp_volume) {
		bt_inner_variate_v.hf_features |= BT_HFP_HF_FEATURE_VOLUME;
	} else {
		bt_inner_variate_v.hf_features &= (~BT_HFP_HF_FEATURE_VOLUME);
	}
	LOG_INF("hfp_sp_volume:%d",param->hfp_sp_volume);

	if (param->hfp_sp_ecnr) {
		bt_inner_variate_v.hf_features |= BT_HFP_HF_FEATURE_ECNR;
	} else {
		bt_inner_variate_v.hf_features &= (~BT_HFP_HF_FEATURE_ECNR);
	}	
	LOG_INF("hfp_sp_ecnr:%d",param->hfp_sp_ecnr);
	
	LOG_INF("hf_features:0x%x",bt_inner_variate_v.hf_features);
	return 0;
}
