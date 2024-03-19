/*
 * Copyright (c) 2018 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_HANDLER_H__
#define __USB_HANDLER_H__
int usb_hid_pre_init(struct device *dev);
int hid_tx_mouse(const u8_t *buf, u16_t len);
int usb_hid_tx_keybord(const u8_t key_code);
int usb_hid_tx_consumer(const u8_t key_code);
int usb_hid_tx_self_defined_key(const u8_t *buf, u8_t len);
int usb_hid_tx_self_defined_dat(const u8_t *buf, u8_t len);
typedef void (*System_call_status_flag)(u32_t status_flag);
void usb_hid_register_call_status_cb(System_call_status_flag cb);
#endif /* __USB_HANDLER_H__ */

