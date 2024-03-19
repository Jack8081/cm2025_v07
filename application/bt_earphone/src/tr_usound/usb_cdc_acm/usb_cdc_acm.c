/*
 * Copyright (c) 2019 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for CDC ACM class driver
 *
 * Sample app for USB CDC ACM class driver. The received data is echoed back
 * to the serial port.
 */

#include <zephyr.h>
#include <drivers/uart.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <usb/class/usb_cdc.h>
#include <usb/usb_device.h>

#ifdef CONFIG_NVRAM_CONFIG
#include <string.h>
#include <drivers/nvram_config.h>
#endif

#define LOG_LEVEL CONFIG_SYS_LOG_USB_CDC_ACM_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_cdc_acm);

static const char *banner1 = "Send characters to the UART device\r\n";
static const char *banner2 = "Characters read:\r\n";

static volatile bool data_transmitted;
static volatile bool data_arrived;

static bool cdc_acm_enabled;
static char data_buf[512] __aligned(4);

static u8_t cdc_acm_perf;
static u32_t perf_count;

#define CDC_ACM_THREAD_STACK_SZ	512
#define CDC_ACM_THREAD_PRIO	-4

static K_THREAD_STACK_DEFINE(cdc_acm_thread_stack, CDC_ACM_THREAD_STACK_SZ);
static struct k_thread cdc_acm_thread_data;

#define SOFT_VERSION	"SOFT_VERSION"
static char version_str[] = "00010001";

/* fixing compiler warning */
#undef __DEPRECATED_MACRO
#define __DEPRECATED_MACRO

static void interrupt_handler(struct device *dev)
{
	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (uart_irq_rx_ready(dev)) {
		data_arrived = true;
	}
}

static void write_data(struct device *dev, const char *buf, int len)
{
	uart_irq_tx_enable(dev);

	while (len) {
		int written;

		data_transmitted = false;
		written = uart_fifo_fill(dev, (const u8_t *)buf, len);
		while (data_transmitted == false) {
			k_yield();
		}

		len -= written;
		buf += written;
	}

	uart_irq_tx_disable(dev);
}

static int command_handler(struct device *dev, char *str, int len)
{
	/* end with ENTER */
	if (str[len - 1] != 0x0D) {
		printk("0x%x\n", str[len - 1]);
		return -EINVAL;
	}

	if (!memcmp(data_buf, "AT+WSSW", 7)) {
		write_data(dev, version_str, strlen(version_str));
		return 0;
	}

	/* Not Implemented */
	return -EINVAL;
}

static int read_and_echo_data(struct device *dev, int *bytes_read)
{
	if (data_arrived == false) {
		k_sleep(K_MSEC(10));
		return 0;
	}

	data_arrived = false;

	/* read all data and echo it back */
	while ((*bytes_read = uart_fifo_read(dev,
	       (u8_t *)data_buf, sizeof(data_buf)))) {
		if (*bytes_read <= 0) {
			break;
		}

		/* echo */
		if (data_buf[*bytes_read - 2] == 0x0D) {
			data_buf[*bytes_read] = 0x0A;
			write_data(dev, data_buf, *bytes_read + 1);
		} else {
			write_data(dev, data_buf, *bytes_read);
		}

		/* handle */
		command_handler(dev, data_buf, *bytes_read);
	}

	return 0;
}

static void cdc_acm_thread_main(void *p1, void *p2, void *p3)
{
	u32_t baudrate, dtr = 0;
	const struct device *dev;
	u32_t bytes_read;
	int ret;
	u8_t c = 'a';
	u32_t time_stamp, ms, count = 0;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	dev = device_get_binding(CONFIG_CDC_ACM_PORT_NAME);
	if (!dev) {
		LOG_ERR("device not found");
		return;
	}

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	LOG_DBG("Wait for DTR");
	while (cdc_acm_enabled) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}

		k_sleep(K_MSEC(10));
	}
	if (!cdc_acm_enabled) {
		return;
	}

	LOG_DBG("DTR set, start test");

	/* They are optional, we use them to test the interrupt endpoint */
	ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 1);
	if (ret) {
		LOG_ERR("Failed to set DCD, ret code %d", ret);
	}

	ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 1);
	if (ret) {
		LOG_ERR("Failed to set DSR, ret code %d", ret);
	}

	/* Wait 1 sec for the host to do all settings */
	k_sleep(K_SECONDS(1));

	ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		LOG_DBG("Failed to get baudrate, ret code %d", ret);
	} else {
		LOG_DBG("Baudrate detected: %d", baudrate);
	}

	uart_irq_callback_set(dev, interrupt_handler);
	write_data(dev, banner1, strlen(banner1));
	write_data(dev, banner2, strlen(banner2));

	while (cdc_acm_enabled) {
		if (cdc_acm_perf == 0) {
			read_and_echo_data(dev, (int *) &bytes_read);
		} else if (cdc_acm_perf == 1) {
			if (count == 0) {
				time_stamp = k_uptime_get_32();
			}

			if (c++ == 'z') {
				c = 'a';
			}
			memset(data_buf, c, sizeof(data_buf));
			write_data(dev, data_buf, sizeof(data_buf));
			if (++count == perf_count) {
				ms = (uint32_t)k_uptime_delta((int64_t *)&time_stamp);
				cdc_acm_perf = 0;
				LOG_WRN("write ms: %d, speed: %ldKB/s\n", ms,
					sizeof(data_buf) * count / 1024 * 1000 / ms);
				count = 0;
			}
		}
	}
}

int usb_cdc_acm_start(void)
{
	struct device *dev;
	int ret;

#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_get(SOFT_VERSION, version_str, strlen(version_str));
	if (ret < 0) {
		LOG_WRN("cannot find SOFT_VERSION(%d)", ret);
	}
#endif
	if (cdc_acm_enabled) {
		LOG_ERR("Already");
		return 0;
	}

	dev = device_get_binding(CONFIG_CDC_ACM_PORT_NAME);
	if (!dev) {
		LOG_ERR("device not found");
		return -ENODEV;
	}

	ret = usb_cdc_acm_init(dev);
	if (ret) {
		LOG_ERR("failed to init CDC ACM device");
		return ret;
	}

	cdc_acm_enabled = true;

	/* Start a thread to offload disk ops */
	k_thread_create(&cdc_acm_thread_data, cdc_acm_thread_stack,
			CDC_ACM_THREAD_STACK_SZ,
			(k_thread_entry_t)cdc_acm_thread_main, NULL, NULL, NULL,
			CDC_ACM_THREAD_PRIO, 0, K_NO_WAIT);

	return 0;
}

int usb_cdc_acm_stop(void)
{
	if (!cdc_acm_enabled) {
		LOG_WRN("Already");
		return 0;
	}
	cdc_acm_enabled = false;

	usb_cdc_acm_exit();

	return 0;
}

static int config_set_soft_version(int argc, char *argv[])
{
	if (argc != 2) {
		return -EINVAL;
	}
#ifdef CONFIG_NVRAM_CONFIG
	return nvram_config_set(SOFT_VERSION, argv[1], strlen(argv[1]));
#else
	return 0;
#endif
}

static int config_get_soft_version(int argc, char *argv[])
{
	char value[64] = {0};
	u8_t i;

	if (argc != 1) {
		return -EINVAL;
	}

#ifdef CONFIG_NVRAM_CONFIG
	if (nvram_config_get(SOFT_VERSION, value, sizeof(value)) < 0) {
		LOG_ERR("not found\n");
		return 0;
	}
#endif
	value[63] = 0;
	for (i = 0; i < 16; i++) {
		LOG_DBG("0x%02x", value[i]);
	}

	return 0;
}

static int cdc_perf(const struct shell *shell, size_t argc, char **argv)
{

	if (!strcmp(argv[1], "in")) {
		perf_count = strtoul(argv[2], NULL, 0);
		if (perf_count < 100) {
			return -EINVAL;
		}
		cdc_acm_perf = 1;
		LOG_DBG("perf: %d, count: %d\n", cdc_acm_perf, perf_count);
	} else {
		return -EINVAL;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(cdc_acm_demo,
	SHELL_CMD(set, NULL, "set soft version.", config_set_soft_version),
	SHELL_CMD(get, NULL, "get soft version.", config_get_soft_version),
	SHELL_CMD(perf, NULL, "perf", cdc_perf),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cdc, &cdc_acm_demo, "demo commands", NULL);
