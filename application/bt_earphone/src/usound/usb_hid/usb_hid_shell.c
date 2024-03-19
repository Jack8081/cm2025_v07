/*
 * Copyright (c) 2018 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <stdlib.h>
#include <init.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>

#define LOG_LEVEL CONFIG_USB_HID_DEVICE_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_hid_shell);

#include "usb_hid_report_desc.h"
#include "usb_hid_handler.h"

/*
 * byte-0: X
 * byte-1: Y
 * byte-2: Button
 *
 * Examples:
 *     hid mouse 1 20 e1 (X: 1, Y: 20, Button: 1)
 *     hid mouse 1 20 e0 (X: 1, Y: 20, Button: 0)
 */
static int shell_hid_mouse(const struct shell *shell, size_t argc, char **argv)
{
	unsigned int value;
	u8_t data[3];

	value = strtoul(argv[3], NULL, 16);
	if (value > 0xff || value < 0xe0) {
		printk("value: 0x%x\n", value);
		return -EINVAL;
	}
	data[2] = value;

	value = strtoul(argv[1], NULL, 16);
	if (value > 0xff) {
		printk("value: 0x%x\n", value);
		return -EINVAL;
	}
	data[0] = value;

	value = strtoul(argv[2], NULL, 16);
	if (value > 0xff) {
		printk("value: 0x%x\n", value);
		return -EINVAL;
	}
	data[1] = value;

	return hid_tx_mouse(data, sizeof(data));
}

/*
 *
 * Examples:
 *     hid kbd num
 *     hid kbd cap
 */
static int shell_hid_keyboard(const struct shell *shell, size_t argc, char **argv)
{
	if (!strcmp(argv[1], "num")) {
		usb_hid_tx_keybord(NUM_LOCK_CODE);
	} else if (!strcmp(argv[1], "cap")) {
		usb_hid_tx_keybord(CAPS_LOCK_CODE);
	}

	return 0;
}

/*
 *
 * Examples:
 *     hid vol up
 *     hid vol down
 */
static int shell_hid_consumer(const struct shell *shell, size_t argc, char **argv)
{
	if (!strcmp(argv[1], "up")) {
		usb_hid_tx_consumer(PLAYING_VOLUME_INC);
	} else if (!strcmp(argv[1], "down")) {
		usb_hid_tx_consumer(PLAYING_VOLUME_DEC);
	}

	return 0;
}

#ifdef CONFIG_USB_SELF_DEFINED_REPORT
static int shell_hid_self_defined_key(const struct shell *shell, size_t argc, char **argv)
{
	/* self-defined key value */
	u8_t dat_buff[7]={0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0X07};
	if (!strcmp(argv[1], "tx")) {
		usb_hid_tx_self_defined_key(dat_buff, sizeof(dat_buff)/sizeof(u8_t));
	}
	return 0;
}

static int shell_hid_self_defined_dat(const struct shell *shell, size_t argc, char **argv)
{
	/* self-defined data */
	u8_t dat_buff[]={"ABCDEFGHIJKLMNOPQRSTUVWXYZ123456ABCDEFGHIJKLMNOPQRSTUVWXYZ12345"};
	if (!strcmp(argv[1], "tx")) {
		usb_hid_tx_self_defined_dat(dat_buff, strlen(dat_buff));
	}
	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(usb_hid,
	SHELL_CMD(mouse, NULL, "hid mouse", shell_hid_mouse),
	SHELL_CMD(kbd, NULL, "hid keyboard", shell_hid_keyboard),
	SHELL_CMD(vol, NULL, "media_ctrl", shell_hid_consumer),
#ifdef CONFIG_USB_SELF_DEFINED_REPORT
	SHELL_CMD(sel-key, NULL, "self-defined hid key", shell_hid_self_defined_key),
	SHELL_CMD(sel-dat, NULL, "self-defined data", shell_hid_self_defined_dat),
#endif
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(hid, &usb_hid, "hid commands", NULL);
