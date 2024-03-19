/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int tr_bt_manager_config_connect_dev_num(void)
{
	return CONFIG_BT_TRANS_SUPPORT_DEV_NUMBER;
}

int tr_bt_manager_config_dev_pairlist_num(void)
{
	return CONFIG_BT_TRANS_SUPPORT_DEV_PAIRLIST_NUMBER;
}

int tr_bt_manager_config_get_adjust_bitpool(void)
{
	return CONFIG_BT_ADJUST_BITPOOL;
}

int tr_bt_manager_config_get_default_bitpool(void)
{
	return CONFIG_BT_DEFAULT_BITPOOL;
}

int tr_bt_manager_config_get_low_lantency_en(void)
{
#ifdef CONFIG_TRANSMITTER_LOW_LANTENCY
	return 1;
#else
	return 0;
#endif
}

int tr_bt_manager_config_get_low_lantency_fix_bitpool(void)
{
#ifdef CONFIG_TRANSMITTER_LOW_LANTENCY_FIX_BITPOOL
	return 1;
#else
	return 0;
#endif
}

int tr_bt_manager_config_get_low_lantency_framenum(void)
{
	return CONFIG_TRANSMITTER_LOW_LANTENCY_FRAME_NUM;
}

int tr_bt_manager_config_get_low_lantency_timeout_slots(void)
{
	return CONFIG_TRANSMITTER_LOW_LANTENCY_TIMEOUT_SLOTS;
}

int tr_bt_manager_config_get_headset_adjust_bitpool(void)
{
#ifdef CONFIG_TRANSMITTER_HEADSET_ADJUST_BITPOOL
	return 1;
#else
	return 0;
#endif
}

int tr_bt_manager_config_get_multi_dev_re_align(void)
{
#ifdef CONFIG_BT_TRANS_MULTI_DEV_RE_ALIGN
	return 1;
#else
	return 0;
#endif
}

int tr_bt_manager_config_get_a2dp_48K_en(void)
{
#if (CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD==48000)
	return 1;
#else
	return 0;
#endif
}

int tr_bt_manager_config_get_max_pending_pkts(void)
{
	//return CONFIG_TR_BT_MAX_PENDING_PKT_NUM / CONFIG_BT_TRANS_SUPPORT_DEV_NUMBER;
	return 4;
}

bool tr_bt_manager_config_get_switch_en_when_connect_out(void)
{
#ifdef CONFIG_BT_TRANS_NOT_ALLOW_SWITCH_ROLE_WHEN_CONNECT_OUT
	return false;
#else
	return true;
#endif
}

int tr_bt_manager_config_get_lantency_detect_en(void)
{
#ifdef CONFIG_TRANSMITTER_LANTENCY_DETECT_ENABLE
	return 1;
#else
	return 0;
#endif
}


