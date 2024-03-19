/** @file keys_br_store.h
 * @brief Bluetooth store br keys.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TR_KEYS_BR_STORE_H__
#define __TR_KEYS_BR_STORE_H__
int tr_stack_get_storage_by_index(u8_t index, paired_dev_info_t *dev);
int tr_stack_storage_disallowed_reconnect(const u8_t *addr, u8_t disallowed);
int tr_stack_del_storage_by_index(u8_t index);
int tr_stack_get_storage_name_by_addr(u8_t *addr, u8_t *name, int len);
int tr_bt_storage_check_linkey(const bt_addr_le_t *addr);
#endif /* __TR_KEYS_BR_STORE_H__ */
