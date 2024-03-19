/*
 * USB Audio Class -- Audio Source Sink driver
 *
 * Copyright (C) 2020 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <usb/class/usb_audio.h>

#define LOG_LEVEL CONFIG_SYS_LOG_USB_SINK_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_audio_sink_handler);

/* USB max packet size */
#define UAC_REC_DAT_SIZE	(((CONFIG_USB_AUDIO_SINK_SAMPLE_RATE / 1000) * CONFIG_USB_AUDIO_SINK_RESOLUTION * CONFIG_USB_AUDIO_SINK_DOWNLOAD_CHANNEL_NUM)>>3)

/*
 * Interrupt Context
 */
static void usb_audio_out_ep_complete(u8_t ep,
	enum usb_dc_ep_cb_status_code cb_status)
{

	u32_t read_byte, res;

	u8_t ISOC_out_Buf[UAC_REC_DAT_SIZE];

	/* Out transaction on this EP, data is available for read */
	if (USB_EP_DIR_IS_OUT(ep) && cb_status == USB_DC_EP_DATA_OUT) {
		res = usb_audio_sink_ep_read(ISOC_out_Buf, sizeof(ISOC_out_Buf), &read_byte);
		LOG_DBG("read_byte:%d", read_byte);
		if (!res && read_byte != 0) {
			for (u8_t i = 0; i < read_byte; i++) {
				LOG_DBG("ISOC_out_Buf[%d]=0x%02x", i, ISOC_out_Buf[i]);
			}
		}
	}
}

#ifdef CONFIG_USB_AUDIO_SINK_SUPPORT_FEATURE_UNIT
int usb_host_sync_volume_to_device(int host_volume)
{
	int vol_db;

	if (host_volume == 0x0000) {
		vol_db = 0;
	} else {
		vol_db = (int)((host_volume - 65536)*10 / 256.0);
	}
	return vol_db;
}

static void usb_audio_vol_changed_notify(u8_t info_type, int chan_num, int *pstore_info)
{
	int volume_db;
	LOG_DBG("info_type: %d chan_num: %d *pstore_info: 0x%04x", *pstore_info, chan_num, *pstore_info);
	if (info_type == USOUND_SYNC_HOST_MUTE) {
		LOG_DBG("host set channel_%d: %d", chan_num, *pstore_info);
	} else if (info_type == USOUND_SYNC_HOST_UNMUTE) {
		LOG_DBG("host set channel_%d: %d", chan_num, *pstore_info);
	} else if (info_type == USOUND_SYNC_HOST_VOL_TYPE) {
		volume_db = usb_host_sync_volume_to_device(*pstore_info);
		LOG_DBG("host set channel_%d volume: %d", chan_num, volume_db);
	}
}
#endif

int usb_audio_sink_pre_init(void)
{
	int ret;

	/* register callbacks */
	usb_audio_sink_register_inter_out_ep_cb(usb_audio_out_ep_complete);

#ifdef CONFIG_USB_AUDIO_SINK_SUPPORT_FEATURE_UNIT
	usb_audio_sink_register_volume_sync_cb(usb_audio_vol_changed_notify);
#endif
	/* usb audio sink initialize */
	ret = usb_audio_sink_init(NULL);

	return ret;
}

int usb_audio_sink_exit(void)
{
	usb_audio_sink_deinit();
	return 0;
}

