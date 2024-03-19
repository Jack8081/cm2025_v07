#ifndef XEAR_HID_PROTOCOL_H_
#define XEAR_HID_PROTOCOL_H_

#include <drivers/usb/usb.h>

#define HID_XEAR_FEATURE_SUPPORT  0x00  // Get Need supoort
#define HID_XEAR_FEATURE_SURROUND 0x02  // Surround Headphone
#define HID_XEAR_FEATURE_NR       0x0E  // Noise Reduction

void xear_hid_protocol_init(void);
void xear_hid_protocol_rcv_data_handler(u8_t *buf);
void xear_hid_protocol_cli_rcv_data_handler(u8_t *buf);
int xear_hid_protocol_cmd_handler(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data);

int xear_hid_protocol_send_data(u8_t featureId, u8_t value);

#endif  //  XEAR_HID_PROTOCOL_H_
