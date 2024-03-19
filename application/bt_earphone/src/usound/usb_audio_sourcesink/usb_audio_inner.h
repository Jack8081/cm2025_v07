/*
 * Copyright (c) 2018 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_AUDIO_HANDLER_H_
#define __USB_AUDIO_HANDLER_H_

typedef enum
{
    USB_AUDIO_GAIN_0,   /* default */
    USB_AUDIO_GAIN_1,   /* 1/2  of the origin */
    USB_AUDIO_GAIN_2,   /* 1/4  of the origin */
    USB_AUDIO_GAIN_3,   /* 1/8  of the origin */
    USB_AUDIO_GAIN_4,   /* 1/16 of the origin */
    USB_AUDIO_GAIN_5,   /* 1/32 of the origin */

    USB_AUDIO_GAIN_UNKNOWN,
}usb_audio_gain_e;

typedef struct
{
    u32_t       data_len;
    u8_t        *data_buffer;
	void (*callback)(void * callback_data, u8_t * data, u32_t len);
	void *callback_data;	
    u8_t        started;
    u8_t        download;
}usb_port_param_t;


typedef void (*usb_audio_event_callback)(u8_t event_type, int event_param);
int     usb_audio_init(usb_audio_event_callback cb);
int     usb_audio_deinit(void);
int     usb_audio_set_stream(io_stream_t stream);
void    usb_audio_set_stream_mute(u8_t mute);
void    set_umic_soft_vol(u8_t vol);
io_stream_t usb_audio_upload_stream_create(void);
int     usb_audio_upload_stream_set(io_stream_t stream);

int     usb_audio_set_skip_ms(u16_t skip_ms);

int     usb_reset_upload_skip_ms(void);
int     usb_audio_set_upload_skip_ms(int ms);


void    usb_audio_download_gain_set(uint8_t gain);

#endif /* __USB_AUDIO_HANDLER_H_ */
