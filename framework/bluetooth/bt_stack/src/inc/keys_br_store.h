/** @file keys_br_store.h
 * @brief Bluetooth store br keys.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __KEYS_BR_STORE_H__
#define __KEYS_BR_STORE_H__


#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
/** LTK key flags */
enum {
	/* Key has been generated with MITM protection */
	BT_STORAGE_LTK_AUTHENTICATED   = BIT(0),

	/* Key has been generated using the LE Secure Connection pairing */
	BT_STORAGE_LTK_SC              = BIT(1),
};
#endif

int bt_store_read_linkkey(const bt_addr_t *addr, void *key);
void bt_store_write_linkkey(const bt_addr_t *addr, const void *key);
void bt_store_clear_linkkey(const bt_addr_t *addr);

void bt_store_share_db_addr_valid_set(bool set);
int bt_store_br_init(void);
int bt_store_br_deinit(void);

int bt_store_get_linkkey(struct br_linkkey_info *info, uint8_t cnt);
int bt_store_update_linkkey(struct br_linkkey_info *info, uint8_t cnt);
void bt_store_write_ori_linkkey(bt_addr_t *addr, uint8_t *link_key);
void *bt_store_sync_get_linkkey_info(uint16_t *len);
void bt_store_sync_set_linkkey_info(void *data, uint16_t len, uint8_t local_is_master);

#ifdef CONFIG_BT_TRANSCEIVER
#include "tr_keys_br_store.h"
#endif
#endif /* __KEYS_BR_STORE_H__ */
