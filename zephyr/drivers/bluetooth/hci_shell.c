/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <stdlib.h>
#include <drivers/adc.h>
#include <ctype.h>
#include <sys/util.h>
#include <devicetree.h>

#ifdef CONFIG_ACTS_BT
#include <acts_bluetooth/buf.h>
#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/hci.h>
#else
#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#endif
#include <drivers/bluetooth/hci_driver.h>
#include <drivers/bluetooth/bt_drv.h>


#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(hci_shell);

#define CONSOLE_CMD_BUF_SIZE  60

int console_get_byte_value(char* str_buf, u8_t* value)
{
    char*  s = str_buf;
    char   tmp_buf[3];

    if (!(isxdigit(s[0]) && isxdigit(s[1])))
        return 0;

    tmp_buf[0] = s[0];
    tmp_buf[1] = s[1];
    tmp_buf[2] = '\0';

    *value = (u8_t)strtoul(tmp_buf, NULL, 16);
    s += 2;

    return (int)(s - str_buf);
}

static int print_hex(const char *prefix, const uint8_t *data, int size)
{
	int n = 0;

	if (!size) {
		printk("%s zero-length signal packet\n", prefix);
		return 0;
	}

	if (prefix) {
		printk("%s", prefix);
	}

	while (size--) {
		printk("%02x ", *data++);
		n++;
		if (n % 16 == 0) {
			printk("\n");
		}
	}

	if (n % 16) {
		printk("\n");
	}

	return 0;
}



// HCI_Command
static int cmd_hci_tx(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 4)
	{
		shell_error(shell, "Usage:\n hcitx [OGF] [OCF] [PayloadLen] [PAYLOAD], eg. {hcitx 03 1a 01 00}\n");
		return -1;
	}

	int argc_valid = argc -1;
	u8_t  data_buf[20];
	int   data_len = 0;

	u8_t   OGF = 0;
	u8_t   OCF = 0;
	u16_t  OpCode;
	u16_t  payload_len = 0;
	char* s = NULL;
	int result = 0;

	data_buf[0] = 0x01;

	s = argv[1];
	console_get_byte_value(s, &OGF);
	s = argv[2];
	console_get_byte_value(s, &OCF);
	OpCode = (OGF << 10) | OCF;
	memcpy(&data_buf[1], &OpCode, 2);

	s = argv[3];
	console_get_byte_value(s, (u8_t*)&payload_len);
	data_buf[3] = payload_len;  // Parameter Total Length
	data_len = 4 + payload_len;

	if ((argc_valid - 3) < payload_len)
	{
		LOG_ERR("date_len %d invalid\n", data_len);
		goto Err;
	}

	int i;
	for(i=0; i < payload_len; i++)
	{
		s = argv[4+i];
		console_get_byte_value(s, &data_buf[4+i]);
	}
	print_hex("TX:", data_buf, data_len);
		    
	if (data_len > 0)
	{    
		result = btdrv_send(data_buf[0], &data_buf[1], data_len-1);
	}

	if (result != 0)
	{
	    LOG_ERR("%d", result);
	}

Err:	
	free(data_buf);
	return 0;
}


SHELL_CMD_REGISTER(hcitx, NULL, "HCI TX CMD", cmd_hci_tx);
