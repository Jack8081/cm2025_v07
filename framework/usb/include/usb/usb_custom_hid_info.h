#ifndef USB_CUSTOM_HID_INFO_
#define USB_CUSTOM_HID_INFO_

#include <drivers/usb/usb.h>


#define INFO_ID_FW_VERSION    0x01   // FW version
#define INFO_ID_MEM_WRITE     0x02   // memory write
#define INFO_ID_MEM_READ      0x03   // memory read
#define INFO_ID_FLASH_WRITE   0x04
#define INFO_ID_FLASH_READ    0x05
#define INFO_ID_XEAR_STATUS   0x81   // Xear driver installed or not

void hid_info_init(void);

void hid_info_srv_rcv_data_handler(u8_t* buf, u8_t bufLen);
void hid_info_cli_rcv_data_handler(u8_t *buf, u8_t len);
void hid_info_data_to_send(u8_t* buf, u8_t len);

int hid_info_cmd_handler(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data);

#endif  //  USB_CUSTOM_HID_INFO_
