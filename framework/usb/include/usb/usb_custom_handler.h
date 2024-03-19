#ifndef USB_CUSTOM_HANDLER_H_
#define USB_CUSTOM_HANDLER_H_

#include <usb/usb_device.h>

#define HID_REPORT_ID_XEAR        0xED
#define HID_REPORT_ID_LED_CTRL    0xFF
#define HID_REPORT_ID_INFO        0xAA

extern const struct usb_cfg_data custom_usb_cfg_data[CONFIG_USB_COMPOSITE_DEVICE_CLASS_NUM];

void usb_custom_init(void);

#endif  //  USB_CUSTOM_HANDLER_H_
