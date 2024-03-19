/*
 * USB Stub device header file.
 *
 * Copyright (c) 2019 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Stub device public header
 */

#ifndef __USB_STUB_H__
#define __USB_STUB_H__
enum{
	STUB_FIFO_BUSY,
	STUB_FIFO_NULL,
};

bool usb_stub_enabled(void);
bool usb_stub_configured(void);

/* the old API implementation */
int usb_stub_init(struct device *dev);
int usb_stub_exit(void);
int usb_stub_ep_read(u8_t *data_buffer, u32_t data_len, u32_t timeout);
int usb_stub_ep_write(u8_t *data_buffer, u32_t data_len, u32_t timeout);

/* the new API implementation */
int usb_stub_dev_init(struct device *dev, u8_t *buf, u32_t len, u8_t *ring_buf, u32_t ring_buf_len);

int usb_stub_dev_exit(void);
/**
 * @brief read data from usb endpoint fifo.
 * @retval data bytes actually read if successful.
 *         negative errno code if failure.
 * @notes fifo data must be read at one time
 */
int usb_stub_dev_ep_read(u8_t *data_buffer, u32_t data_len, u32_t timeout);

/**
 * @brief read data from usb endpoint fifo.
 * @retval data bytes actually read if successful.
 *         negative errno code if failure.
 * @notes fifo data can be read for many times.
 */
int usb_stub_dev_read_data(u8_t *data_buffer, u32_t data_len, u32_t timeout);

void usb_stub_flush_in_ep();
void usb_stub_flush_out_ep();

int usb_stub_ep_write_async(u8_t *data_buffer, u32_t data_len, uint32_t flag);

#endif /* __USB_STUB_H__ */

