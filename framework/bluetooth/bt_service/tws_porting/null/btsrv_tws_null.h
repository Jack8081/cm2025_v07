/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt service tws null
 */

#ifndef _BTSRV_TWS_NULL_H_
#define _BTSRV_TWS_NULL_H_

#define btsrv_tws_protocol_data_cb(a, b, c)		false
#define btsrv_tws_check_is_woodpecker()			true		/* Not support tws, self as woodpecker */
#define btsrv_tws_hfp_set_1st_sync(a)
#define btsrv_adapter_tws_check_role(a, b)		BTSRV_TWS_NONE
#define btsrv_tws_sync_info_type(a, b, c)
#define btsrv_tws_sync_hfp_cbev_data(a, b, c, d)
#define btsrv_connect_check_switch_sbc()
#define btsrv_connect_proc_switch_sbc_state(a, b)
#define btsrv_tws_process_act_fix_cid_data(a, b, c, d)
#define btsrv_tws_monitor_get_observer()		NULL

#endif
