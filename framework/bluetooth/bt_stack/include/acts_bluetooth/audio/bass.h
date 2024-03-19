/** @file
 *  @brief Broadcast Audio Scan Service
 */

/*
 * Copyright (c) 2021 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_H_

#include <sys/util.h>
#include <acts_bluetooth/addr.h>

#ifdef __cplusplus
extern "C" {
#endif

#if 1
// if Scan Delegator collocated with Broadcast Sink
struct sd_adv {
	uint8_t len;
	uint8_t type;
	uint16_t uuid;	/* BASS */
};
#endif

/* Error codes */
#define BT_BASS_ERR_OPCODE_NOT_SUPPORTED	0x80
#define BT_BASS_ERR_INVALID_SOURCE_ID	0x81

/* Broadcast Audio Scan Control Point opcode */
#define BT_BASS_OPCODE_REMOTE_SCAN_STOPPED	0x00
#define BT_BASS_OPCODE_REMOTE_SCAN_STARTED	0x01
#define BT_BASS_OPCODE_ADD_SOURCE	0x02
#define BT_BASS_OPCODE_MODIFY_SOURCE	0x03
#define BT_BASS_OPCODE_SET_BROADCAST_CODE	0x04
#define BT_BASS_OPCODE_REMOVE_SOURCE	0x05

struct subgroup {
	uint32_t bis_sync;
	uint8_t meta_len;
	uint8_t meta[0];
};

#define BT_BASS_PA_NOT_SYNC	0x00
#define BT_BASS_PA_SYNC_PAST	0x01
#define BT_BASS_PA_SYNC_NOT_PAST	0x02

#define BT_BASS_PA_INTERVAL_UNKNOWN	0xFFFF

struct bt_bass_add_source {
	bt_addr_le_t adv_addr;
	uint8_t adv_sid;
	uint8_t broadcast_id[3];
	uint8_t pa_sync;
	uint16_t pa_interval;

	uint8_t num_subgroups;
	struct subgroup value[0];
#if 0
	uint32_t bis_sync;
	uint8_t meta_len;
	uint8_t meta[0];
#endif
} __packed;

struct bt_bass_modify_source {
	uint8_t source_id;
	uint8_t pa_sync;
	uint16_t pa_interval;

	uint8_t num_subgroups;
	struct subgroup value[0];
} __packed;

struct bt_bass_set_broadcast_code {
	uint8_t source_id;
	uint8_t broadcast_code[16];
} __packed;

struct bt_bass_remove_source {
	uint8_t source_id;
} __packed;

struct subgroup_state {
	uint32_t bis_sync_state;
	uint8_t meta_len;
	uint8_t meta[0];
};

struct bt_bass_broadcast_receive_state {
	uint8_t source_id;	// TODO: assigned by server
	bt_addr_le_t source_addr;
	uint8_t source_adv_sid;
	uint8_t broadcast_id[3];
	uint8_t pa_sync_state;
	uint8_t big_encryption;
	uint8_t bad_code[16];

	uint8_t num_subgroups;
	struct subgroup_state state[0];
} __packed;

#if 1

#endif

struct bt_bass_server {
};

struct bt_bass_client {
	uint16_t start_handle;
	uint16_t end_handle;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_H_ */
