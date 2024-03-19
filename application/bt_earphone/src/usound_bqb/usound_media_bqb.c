/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usound_bqb.h"

int usound_init_capture(void)
{
	return 0;
}

int usound_start_capture(void)
{
	struct usound_app_t *usound = usound_get_app();

	if (usound->capture_player_run) {
		SYS_LOG_INF("already\n");
		return -EINVAL;
	}

	usound->capture_player_load = 1;
	usound->capture_player_run = 1;

	SYS_LOG_INF("\n");

	return 0;
}

int usound_stop_capture(void)
{
	struct usound_app_t *usound = usound_get_app();

	if (!usound->capture_player_run) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

	usound->usb_to_bt_started = 0;
	usound->capture_player_run = 0;

	SYS_LOG_INF("\n");

	if (!usound->playback_player_run) {
		usound_exit_playback();
		usound_exit_capture();
	}

	return 0;
}

int usound_exit_capture(void)
{
	struct usound_app_t *usound = usound_get_app();

	if (!usound->capture_player) {
		SYS_LOG_INF("already\n");
		return -EALREADY;
	}

	SYS_LOG_INF("\n");

	usound->capture_player_load = 0;
	usound->capture_player_run = 0;

	return 0;
}

int usound_init_playback(void)
{
	return 0;
}

int usound_start_playback(void)
{
	struct usound_app_t *usound = usound_get_app();

	if (!usound->playback_player) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

	if (usound->playback_player_run) {
		SYS_LOG_INF("already\n");
		return -EINVAL;
	}

	usound->playback_player_load = 1;
	usound->playback_player_run = 1;

	SYS_LOG_INF("\n");

	return 0;
}

int usound_stop_playback(void)
{
	struct usound_app_t *usound = usound_get_app();

	if (!usound->playback_player_run) {
		SYS_LOG_INF("not ready\n");
		return -EINVAL;
	}

	usound->bt_to_usb_started = 0;
	usound->playback_player_run = 0;

	SYS_LOG_INF("\n");

	if (!usound->capture_player_run) {
		usound_exit_playback();
		usound_exit_capture();
	}

	return 0;
}

int usound_exit_playback(void)
{
	struct usound_app_t *usound = usound_get_app();

	SYS_LOG_INF("\n");

	usound->playback_player_load = 0;
	usound->playback_player_run = 0;

	return 0;
}
