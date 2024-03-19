/* hci.h - Bluetooth Host Control Interface definitions */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __TR_BT_HCI_H
#define __TR_BT_HCI_H

#define BT_HCI_OP_ENABLE_LOWPOWER_MODE            BT_OP(BT_OGF_TEST, 0x0006)
struct bt_hci_cp_enable_lowpower_mode {
	u8_t enable;
} __packed;

enum
{
	WHITELIST_NULL,
	WHITELIST_TYPE1,
};


#endif /* __TR_BT_HCI_H */
