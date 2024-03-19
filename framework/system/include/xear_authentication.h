#ifndef XEAR_AUTHENTICATION_H_
#define XEAR_AUTHENTICATION_H_

#include <drivers/usb/usb.h>

#define XEAR_STATUS_DISABLE  0
#define XEAR_STATUS_ENABLE   1

int xear_auth_cmd_handler(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data);

u8_t xear_get_status(void);
void xear_set_status(u8_t status);

#endif  //  XEAR_AUTHENTICATION_H_
