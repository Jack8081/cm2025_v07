/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Bluetooth config for earphone
 */

#ifdef __BT_CFG_EARPHONE_H__
#error "Can't direct include this file, only include by bt_configs.h"
#endif

#define __BT_CFG_EARPHONE_H__

/* All config with perfix "CFG_BT_" */

#if (CONFIG_BT_MAX_CONN < CONFIG_BT_MAX_BR_CONN)
#error "CONFIG_BT_MAX_CONN < CONFIG_BT_MAX_BR_CONN"
#endif
#define CFG_BT_MAX_LE_CONN			(CONFIG_BT_MAX_CONN - CONFIG_BT_MAX_BR_CONN)
#define CFG_BT_MAX_LE_PAIRED		(CONFIG_BT_MAX_PAIRED - CONFIG_BT_MAX_BR_PAIRED)

#define CFG_BT_SHARE_TWS_MAC		"BT_SHARE_MAC"		/* Snoop tws  slave, simulate master mac address */
#define CFG_BT_AUTOCONN_TWS         "BT_AUTOCONN_TWS"

#define CONFIG_MAX_BT_NAME_LEN      32
#define CONFIG_MAX_BT_LE_AUDIO_ADV_ENTRY    16
#define CONFIG_MAX_BT_LE_AUDIO_ADV_LEN      128
#define CONFIG_MAX_BT_ISO_RX_LEN            200
#define CONFIG_MAX_BT_ISO_TX_LEN            200

#ifdef CONFIG_TWS
#define CONFIG_SUPPORT_TWS          1
#endif

#define CONFIG_MAX_A2DP_ENDPOINT	3

#define CFG_BT_MAX_SPP_CHANNEL		3

#define CONFIG_MAX_PBAP_CONNECT		2

#define CONFIG_MAX_MAP_CONNECT		2

#define CONFIG_DEBUG_DATA_RATE      1

#ifdef CONFIG_BT_MAX_BR_CONN
#define CFG_BT_MANAGER_MAX_BR_NUM	CONFIG_BT_MAX_BR_CONN
#else
#define CFG_BT_MANAGER_MAX_BR_NUM	1
#endif

//#define CONFIG_LEAUDIO_ACTIVE_BR_A2DP_SUSPEND        1

