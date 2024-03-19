/** @file common_internal.h
 * @brief Bluetooth internel variables.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __COMMON_INTERNAL_H__
#define __COMMON_INTERNAL_H__

#define __IN_BT_SECTION	__in_section_unique(bthost_bss)

/* Const value, can't change after build*/
struct bt_inner_const_t {
	uint32_t max_conn:4;
	uint32_t br_max_conn:3;
	uint32_t le_max_paired:3;
	uint32_t br_max_paired:3;
	uint32_t br_reserve_pkts:3;
	uint32_t le_reserve_pkts:3;
	uint32_t acl_tx_max:5;
	uint32_t rfcomm_max_credits:4;
	uint32_t enable_auth_no_bonding:1;
	uint32_t pts_test_mode:1;
	uint32_t debug_log:1;
	uint16_t l2cap_tx_mtu;
	uint16_t rfcomm_l2cap_mtu;
	uint16_t avdtp_rx_mtu;
	uint16_t br_max_mtu_a2dp;		/* For receive aac large than avdtp_rx_mtu */
	uint16_t ag_features;
	uint16_t bt_att_mtu;
};

/* Variate value, can change in init */
struct bt_inner_variate_t {
	uint8_t sniff_enable:1;
	uint8_t linkkey_miss_reject:1;
	uint8_t max_pair_list_num:4;
	uint8_t le_adv_random_addr:1;
	uint32_t avrcp_vol_sync:1;
	uint16_t hf_features;
	uint8_t bt_version;
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
    uint8_t bt_br_tr_enable:1;
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_BQB
    uint8_t leaudio_bqb_enable:1;
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
    uint8_t leaudio_bqb_sr_enable:1;
#endif
	uint16_t bt_sub_version;
	uint32_t classOfDevice;
	uint16_t bt_device_id[4];
};

struct bt_inner_value_t {
	const struct bt_inner_const_t *const_t;
	const struct bt_inner_variate_t *variate_t;
};

extern const struct bt_inner_value_t bt_inner_value;
/* Value from CONFIG or define */
#define bti_max_conn()                      bt_inner_value.const_t->max_conn
#define bti_br_max_conn()                   bt_inner_value.const_t->br_max_conn
#define bti_le_max_paired()                 bt_inner_value.const_t->le_max_paired
#define bti_br_max_paired()                 bt_inner_value.const_t->br_max_paired
#define bti_br_reserve_pkts()               bt_inner_value.const_t->br_reserve_pkts
#define bti_le_reserve_pkts()               bt_inner_value.const_t->le_reserve_pkts
#define bti_acl_tx_max()                    bt_inner_value.const_t->acl_tx_max
#define bti_rfcomm_max_credits()            bt_inner_value.const_t->rfcomm_max_credits
#define bti_enable_auth_no_bonding()        bt_inner_value.const_t->enable_auth_no_bonding
#define bti_pts_test_mode()                 bt_inner_value.const_t->pts_test_mode
#define bti_debug_log()                     bt_inner_value.const_t->debug_log
#define bti_l2cap_tx_mtu()                  bt_inner_value.const_t->l2cap_tx_mtu
#define bti_rfcomm_l2cap_mtu()              bt_inner_value.const_t->rfcomm_l2cap_mtu
#define bti_avdtp_rx_mtu()                  bt_inner_value.const_t->avdtp_rx_mtu
#define bti_br_max_mtu_a2dp()               bt_inner_value.const_t->br_max_mtu_a2dp
#define bti_ag_features()                   bt_inner_value.const_t->ag_features
#define bti_bt_att_mtu()                    bt_inner_value.const_t->bt_att_mtu

/* Value set by user */
#define bti_sniff_enable()                  bt_inner_value.variate_t->sniff_enable
#define bti_linkkey_miss_reject()           bt_inner_value.variate_t->linkkey_miss_reject
#define bti_max_pair_list_num()             bt_inner_value.variate_t->max_pair_list_num
#define bti_le_adv_random_addr()            bt_inner_value.variate_t->le_adv_random_addr
#define bti_avrcp_vol_sync()                bt_inner_value.variate_t->avrcp_vol_sync
#define bti_hf_features()                   bt_inner_value.variate_t->hf_features
#define bti_bt_version()                    bt_inner_value.variate_t->bt_version
#define bti_bt_sub_version()                bt_inner_value.variate_t->bt_sub_version
#define bti_classOfDevice()                 bt_inner_value.variate_t->classOfDevice
#define bti_bt_device_id()                  bt_inner_value.variate_t->bt_device_id

#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
#define bti_br_tr_mode()                    bt_inner_value.variate_t->bt_br_tr_enable
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_BQB
#define bti_leaudio_bqb_mode()              bt_inner_value.variate_t->leaudio_bqb_enable
#endif
#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
#define bti_leaudio_bqb_sr_mode()           bt_inner_value.variate_t->leaudio_bqb_sr_enable
#endif

void *bt_malloc(size_t size);
void bt_free(void *ptr);
/**
 * @brief save value in property.
 * @param key the key of item.
 * @param value the value of item.
 * @param value_len length to set.
 * @return 0 on success, non-zero on failure.
 */
int bt_property_set(const char *key, char *value, int value_len);
/**
 * @brief get value in property.
 * @param key the key of item.
 * @param value the destination buffer.
 * @param value_len length to get.
 * @return return the length of value, otherwise nagative on failure.
 */
int bt_property_get(const char *key, char *value, int value_len);

/**
 * @brief Register property flush callback function.
 * @param key the key of item.
 */
int bt_property_reg_flush_cb(void *cb);

void hci_data_log_init(void);
void hci_data_log_debug(bool send, struct net_buf *buf);
void hci_data_log_enable(bool enable);

#endif /* __COMMON_INTERNAL_H__ */
