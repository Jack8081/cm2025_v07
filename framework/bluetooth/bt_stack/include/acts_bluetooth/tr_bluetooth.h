/** @file
 *  @brief Bluetooth subsystem core APIs.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Bluetooth APIs
 * @defgroup bluetooth Bluetooth APIs
 * @{
 */

#define CONFIG_MAX_BT_NAME_LEN      32

typedef struct{
    /** Remote device address */
    u8_t addr[6];
    /** Remote device name */
    u8_t name[CONFIG_MAX_BT_NAME_LEN + 1];
} paired_dev_info_t;

int tr_bt_set_lowpower_mode(u8_t enable);
int tr_bt_read_rssi(u16_t handle, int8_t *rssi);
int tr_bt_vs_set_debug_params(u32_t dbg_enable, u32_t dbg_cfg);

int tr_bt_set_link_policy(u16_t handle, int8_t enable);

#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_BQB
void tr_bt_set_ext_connect_flag(bool val);
void tr_bt_set_ccc_flag(bool val);
#endif

/**
 * @}
 */
