/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
static u8_t bit_rate = 0;
static u8_t codec_id = 0;
static u8_t sample_rate = 0;

static void _tr_bt_manager_a2dp_callback(btsrv_a2dp_event_e event, void *packet, int size)
{
	switch (event) {
	case TR_BTSRV_A2DP_BIT_RATE:
	{
		u8_t *codec_info = (u8_t *)packet;

        codec_id = codec_info[0];
		sample_rate = codec_info[1];
        bit_rate = codec_info[2];
        SYS_LOG_INF("update codec:%d, sample_rate:%d, bit_rate:%d\n", codec_id, sample_rate, bit_rate);
		break;
	}
	case TR_BTSRV_A2DP_CODEC_INFO_CHANGED:
	{
		bt_manager_event_notify(BT_REQ_RESTART_PLAY, NULL, 0);
		break;
	}
	default:
	break;
	}
}

int tr_bt_manager_a2dp_get_bit_rate(void)
{
	SYS_LOG_INF("bit_rate %d\n", bit_rate);
	return bit_rate;
}


