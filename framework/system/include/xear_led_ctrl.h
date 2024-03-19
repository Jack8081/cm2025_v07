#ifndef XEAR_LED_CTRL_H_
#define XEAR_LED_CTRL_H_

#include <drivers/usb/usb.h>

void xear_led_ctrl_init(void);
int xear_led_ctrl_cmd_handler2(u8_t *buf, u8_t len);
int xear_led_ctrl_cmd_handler(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data);
void xear_led_ctrl_rcv_data_handler(u8_t *buf);

int xear_led_ctrl_dummy_handler(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data);

void xear_led_ctrl_sleep(void);
void xear_led_ctrl_wake_up(void);

#endif  //  XEAR_LED_CTRL_H_
