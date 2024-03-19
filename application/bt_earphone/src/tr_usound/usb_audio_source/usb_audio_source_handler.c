/*
 * USB Audio source device HAL layer.
 *
 * Copyright (C) 2020 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <usb/class/usb_audio.h>

#define LOG_LEVEL CONFIG_SYS_LOG_USB_SOURCE_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_audio_source_handler);

/* max upload packet for USB audio source device */
#define RESOLUTION	CONFIG_USB_AUDIO_SOURCE_RESOLUTION
#define SUB_FRAME_SIZE	(RESOLUTION >> 3)
#define MAX_UPLOAD_PACKET_LEN       (((CONFIG_USB_AUDIO_SOURCE_SAMPLE_RATE / 1000) * RESOLUTION * CONFIG_USB_AUDIO_SOURCE_UPLOAD_CHANNEL_NUM)>>3)

/* USB max packet size */
#define UAC_TX_UNIT_SIZE	MAX_UPLOAD_PACKET_LEN
#define UAC_TX_DUMMY_SIZE	MAX_UPLOAD_PACKET_LEN

static u8_t usb_audio_play_load[UAC_TX_UNIT_SIZE]="abcdefghijklmnopqrstuvwxyz012345";

static int usb_audio_tx_unit(const u8_t *buf)
{
	u32_t wrote;

	return usb_write(CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR, buf,
					UAC_TX_UNIT_SIZE, &wrote);
}

static int usb_audio_tx_dummy(void)
{
	/* NOTE: Could use stack to save memory */
	static const u8_t dummy_buf[UAC_TX_DUMMY_SIZE];
	u32_t wrote;
	return usb_write(CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR, dummy_buf,
					sizeof(dummy_buf), &wrote);
}

/*
 * Interrupt Context
 */
static void usb_audio_source_in_ep_complete(u8_t ep,
	enum usb_dc_ep_cb_status_code cb_status)
{
	LOG_DBG("complete: ep = 0x%02x  ep_status_code = %d", ep, cb_status);
	/* In transaction request on this EP, read data from app, then send to PC */
	if (USB_EP_DIR_IS_IN(ep)) {
		usb_audio_tx_unit(usb_audio_play_load);
	} else {
		usb_audio_tx_dummy();
	}
}

static void usb_audio_source_start_stop(bool start)
{
	if (!start) {
		usb_audio_source_ep_flush();
		/*
		 * Don't cleanup in "Set Alt Setting 0" case, Linux may send
		 * this before going "Set Alt Setting 1".
		 */
		/* uac_cleanup(); */
	} else {
		/*
		 * First packet is all-zero in case payload be flushed (Linux
		 * may send "Set Alt Setting" several times).
		 */
		usb_audio_tx_dummy();
	}
}

int usb_audio_source_pre_init(void)
{
	int ret = 0;

	/* register callbacks */
	usb_audio_source_register_start_cb(usb_audio_source_start_stop);
	usb_audio_source_register_inter_in_ep_cb(usb_audio_source_in_ep_complete);

	/* usb audio source init */
	ret = usb_audio_source_init(NULL);
	if (ret) {
		LOG_ERR("usb audio source init failed");
		goto exit;
	}

exit:
	return ret;
}

int usb_audio_source_exit(void)
{
	usb_audio_source_deinit();
	return 0;
}
