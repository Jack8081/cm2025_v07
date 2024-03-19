/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bt service interface
 */

#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
#else
/** auto connect info */
typedef struct {
    u8_t a2dp:1;
    u8_t avrcp:1;
    u8_t hfp:1;
}tr_bt_set_autoconn_t;
#endif

struct conn_status
{
    uint16_t handle;
    uint8_t idx;
    uint8_t connected;
    uint8_t audio_connected;
    int8_t  strength;
    int8_t  rssi;

    uint8_t addr[6];
    uint8_t name[32];
};

struct connected_param
{
    u16_t handle;
    bt_addr_le_t addr;
    uint8_t role;
    struct bt_conn *conn;
};

int tr_btif_avrcp_sync_vol_to_others(struct bt_conn *conn, u8_t volume);

int tr_btif_br_scan_set_lowpowerable(bool enable);



//avrcp macro
#define LE_AVRCP_PLAYING_STUS 0x8000
#define LE_AVRCP_MAX_VOLUME 31
#define LE_AVRCP_VOLUME_MASK 0x001f

#define LE_AVRCP_CMD_INVALID 0
#define LE_AVRCP_CMD_PREV 1
#define LE_AVRCP_CMD_NEXT 2
#define LE_AVRCP_CMD_RESUME 3
#define LE_CMD_DENOISE_SET 4


#define LE_AVRCP_CMD_MAX 7

#define LE_AVRCP_CMD_IN(x) (x << 5)
#define LE_AVRCP_CMD_OUT(x) ((x >> 5) & LE_AVRCP_CMD_MAX)

#define LE_AVRCP_CMD_MASK LE_AVRCP_CMD_IN(LE_AVRCP_CMD_MAX)

int tr_btif_audio_start(void *param);
int tr_btif_audio_stop(void);
int tr_btif_audio_auto_start(void);
int tr_btif_audio_auto_stop(void);
int tr_btif_audio_disconn(uint16_t handle);

int tr_bti_get_cis_conn_interval_us(void);

int tr_btif_audio_call_terminate(struct bt_audio_call *call, uint8_t reason);

int tr_btif_audio_call_remote_originate(uint16_t handle, uint8_t *remote_uri);
int tr_btif_audio_call_remote_alert(struct bt_audio_call *call);
int tr_btif_audio_call_remote_accept(struct bt_audio_call *call);
int tr_btif_audio_call_remote_hold(struct bt_audio_call *call);
int tr_btif_audio_call_remote_retrieve(struct bt_audio_call *call);
int tr_btif_audio_call_remote_terminate(struct bt_audio_call *call);

#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_BQB
void tr_btif_set_ccid(uint8_t ccid_count);
#endif

#ifdef CONFIG_LE_AUDIO_BQB //CONFIG_LE_AUDIO_SR_BQB
int tr_btif_contexts_set(bool available, bool supported, int val);
int tr_btif_server_provider_set(uint8_t *provider, uint8_t len);
int tr_btif_server_tech_set(uint8_t tech);
int tr_btif_server_status_set(uint16_t status);
int tr_btif_volume_init_set(uint8_t step);
void tr_btif_audio_csis_not_bonded_set(uint8_t enable);
void tr_btif_audio_csis_update_lockstate(uint8_t lockcal);
int tr_btif_mcs_adv_buf_set();
#endif

