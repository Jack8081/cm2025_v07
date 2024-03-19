#include <usb/usb_custom_handler.h>
#include <usb/usb_custom_hid_info.h>
#include <xear_authentication.h>
#include <xear_hid_protocol.h>
#include <xear_led_ctrl.h>
#include <os_common_api.h>

//#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_LEVEL
//#define SYS_LOG_DOMAIN "usb_custom_handler"
//#include <logging/sys_log.h>


static int class_handler0(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("bRequest(0x%x) wValue(0x%x)\n", pSetup->bRequest, pSetup->wValue);

#ifdef CONFIG_XEAR_HID_PROTOCOL
	if( ! xear_hid_protocol_cmd_handler(pSetup, len, data) )
		return 0;
#endif

#ifdef CONFIG_XEAR_LED_CTRL
	if( ! xear_led_ctrl_cmd_handler(pSetup, len, data) )
		return 0;
#endif

#ifdef CONFIG_XEAR_HID_PROTOCOL
	if( ! hid_info_cmd_handler(pSetup, len, data) )
		return 0;
#endif

	return -ENOTSUP;
}

static int vendor_handler0(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("pSetup bmRequestType(0x%x) bRequest(0x%x)\n", pSetup->bmRequestType, pSetup->bRequest);

#ifdef CONFIG_XEAR_AUTHENTICATION
	if( ! xear_auth_cmd_handler(pSetup, len, data) )
	{
		if( (pSetup->bmRequestType == 0xC0) && (pSetup->bRequest == 0x13) )
		{
			// notify Rx about xear status
			u8_t buf[2] = {INFO_ID_XEAR_STATUS };
			buf[1] = xear_get_status();
			hid_info_data_to_send(buf, sizeof(buf));
		}
		return 0;
	}
#endif

#ifdef CONFIG_XEAR_LED_CTRL
	if( ! xear_led_ctrl_dummy_handler(pSetup, len, data) )
		return 0;
#endif

	return -EINVAL;
}

void usb_custom_init(void)
{
#ifdef CONFIG_XEAR_HID_PROTOCOL
	xear_hid_protocol_init();
#endif

	hid_info_init();
}

const struct usb_cfg_data custom_usb_cfg_data[CONFIG_USB_COMPOSITE_DEVICE_CLASS_NUM] =
{
	// interface_descriptor is used to store interfaceNumber
	{0,(const void *)0,0,0, {class_handler0, vendor_handler0, 0}, 0, 0 },
};
