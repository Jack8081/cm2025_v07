/*
 * USB Audio device(UAC + HID) HAL layer.
 *
 * Copyright (C) 2020 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <init.h>
#include <usb/class/usb_audio.h>
#include <stream.h>
#include <timeline.h>
#include <assert.h>
#include <audio_system.h>
#include "usb_hid_inner.h"
#include "usb_audio_device_desc.h"
#include "usb_audio_inner.h"
//#include <acts_ringbuf.h>
#include <drivers/hrtimer.h>
#include <kernel.h>

#if (CONFIG_USB_AUDIO_RESOLUTION == 24)
#define MAX_DOWNLOAD_BUF_SIZE   ((MAX_DOWNLOAD_PACKET/3) * 4) 
#define MAX_UPLOAD_BUF_SIZE     ((MAX_UPLOAD_PACKET/3) * 4)
#else
#define MAX_DOWNLOAD_BUF_SIZE   MAX_DOWNLOAD_PACKET
#define MAX_UPLOAD_BUF_SIZE     MAX_UPLOAD_PACKET
#endif

#ifdef CONFIG_USB_DOWNLOAD_LISTEN_SUPPORT
u8_t soundcard_sink_vol = 0;
u8_t soundcard_sink_mute_flag = 0;
#endif
#define SKIP_FRAME_CNT 200
#define SKIP_FRAME_MAC_OS_CNT 200
#ifdef CONFIG_USOUND_DOUBLE_SOUND_CARD
int soundcard1_soft_vol = 0;
int soundcard2_soft_vol = 0;
extern u8_t soundcard_cnt;
#endif
int usound_mic_vol = 0;
extern u8_t g_usound_app;

u8_t upload_req_data = 0;
//extern void cis_debug_pcm_in(u8_t *buf, int num);
/* USB max packet size */

struct acts_ringbuf {
	/* Index in buf for the head element */
	uint32_t head;
	/* Index in buf for the tail element */
	uint32_t tail;
	/* Size of buffer in elements */
	uint32_t size;
	/* Modulo mask if size is a power of 2 */
	uint32_t mask;
	/* cpu/dsp address of buffer  */
	uint32_t cpu_ptr;   /* in bytes */
	uint32_t dsp_ptr;	/* in 16-bit words */

	/* Index in actual buf for the head element */
	uint32_t head_offset;
	/* Index in actual buf for the tail element */
	uint32_t tail_offset;
};

struct usb_audio_info {
	os_work call_back_work;
	os_delayed_work upload_stream_notify_work;
	os_delayed_work upload_stream_data_check_work;
	os_delayed_work download_stream_notify_work;
	u8_t play_state;
	u16_t zero_frame_cnt;
	u16_t skip_ms;
	signed int pre_volume_db;
	u8_t call_back_type;
	u8_t upload_stream_event;
	u8_t upload_stream_data_check_event;
	u8_t download_stream_event;
	int call_back_param;
    int umic_vol;
	usb_audio_event_callback cb;
	io_stream_t usound_download_stream;
	u8_t *usb_audio_play_load;
	u8_t *download_buf;
	uint32_t last_out_packet_count;
	uint32_t out_packet_count;
	uint32_t last_diff_num;
	timeline_listener_t listener;
	timeline_t *tl;
    u8_t stream_mute;
    u8_t download_gain;
    struct acts_ringbuf *download_ringbuf;
    struct hrtimer clean_upstream_timer;
};

static struct usb_audio_info *usb_audio;

int usb_audio_set_skip_ms(u16_t skip_ms)
{
    if(usb_audio)
    {
        usb_audio->skip_ms = skip_ms;
        return 0;
    }

    return -1;
}

#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
#if 0
static int usb_audio_tx_unit(const u8_t *buf)
{
	u32_t wrote;

	return usb_write(CONFIG_USB_AUDIO_DEVICE_SOURCE_IN_EP_ADDR, buf,
					MAX_UPLOAD_PACKET, &wrote);
}
#endif
static int usb_audio_tx_dummy(void)
{
	u32_t wrote;
    int len;

    if (usb_device_get_android_os_flag() == 2)
    {
        len = FAKE_16BIT_MAX_UPLOAD_PACKET;
    }
    else
    {
        len = MAX_UPLOAD_PACKET;
    }
	memset(usb_audio->usb_audio_play_load, 0, MAX_UPLOAD_PACKET);

	return usb_write(CONFIG_USB_AUDIO_DEVICE_SOURCE_IN_EP_ADDR,
			usb_audio->usb_audio_play_load,
			len, &wrote);
}
#endif
void usound_send_key_msg(int msg_type)
{
    if(msg_type == 1)   //ring
        usb_audio->call_back_type = USOUND_INCALL_RING;
    else if(msg_type == 2 ) //hook up
        usb_audio->call_back_type = USOUND_INCALL_HOOK_UP;
    else if(msg_type == 3) //hook down
        usb_audio->call_back_type = USOUND_INCALL_HOOK_HL;
    else if(msg_type == 4) //mute
        usb_audio->call_back_type = USOUND_INCALL_MUTE;
    else if(msg_type == 5) //unmute
        usb_audio->call_back_type = USOUND_INCALL_UNMUTE;
    else if(msg_type == 6) //outgoing call
        usb_audio->call_back_type = USOUND_INCALL_OUTGOING;
    else if(msg_type == 0x100)
    {
        usb_audio->call_back_type = USOUND_DEFINE_CMD_CONNECT_PAIRED_DEV;
    }
    else
        return;

	os_work_submit(&usb_audio->call_back_work);
}

static void _usb_audio_stream_state_notify(u8_t stream_event)
{
    if(stream_event == USOUND_UPLOAD_STREAM_STOP)
    {
        if(hrtimer_is_running(&usb_audio->clean_upstream_timer))
            hrtimer_stop(&usb_audio->clean_upstream_timer);

        hrtimer_start(&usb_audio->clean_upstream_timer, 1000, 980);
        usb_audio->upload_stream_event = stream_event;
	    os_delayed_work_submit(&usb_audio->upload_stream_notify_work, 100);
    }
    else if(stream_event == USOUND_UPLOAD_STREAM_START)
    {
        if(hrtimer_is_running(&usb_audio->clean_upstream_timer))
            hrtimer_stop(&usb_audio->clean_upstream_timer);

        usb_reset_upload_skip_ms();
        usb_audio->upload_stream_event = stream_event;
	    os_delayed_work_submit(&usb_audio->upload_stream_notify_work, 10);
    }
    else if(stream_event == USOUND_STREAM_STOP)
    {
        usb_audio->download_stream_event = stream_event;
	    os_delayed_work_submit(&usb_audio->download_stream_notify_work, 1000);
        
    }
    else if(stream_event == USOUND_STREAM_START)
    {
        usb_audio->download_stream_event = stream_event;
	    os_delayed_work_submit(&usb_audio->download_stream_notify_work, 10);
        
    }
    else
    {
        usb_audio->call_back_type = stream_event;
	    os_work_submit(&usb_audio->call_back_work);
    }
}

static os_delayed_work usb_audio_dat_detect_work;
#ifdef CONFIG_USOUND_START_STOP_CIS_DETECT
static u32_t usb_audio_cur_cnt, usb_audio_pre_cnt;
#endif
/* Work item process function */
static void usb_audio_handle_dat_detect(struct k_work *item)
{
#ifndef CONFIG_USOUND_START_STOP_CIS_DETECT
	usb_audio->play_state = 0;
	_usb_audio_stream_state_notify(USOUND_STREAM_STOP);
#else
    if (usb_audio_pre_cnt < usb_audio_cur_cnt) {
		usb_audio_pre_cnt = usb_audio_cur_cnt;
		os_delayed_work_submit(&usb_audio_dat_detect_work, 500);
	} else if (usb_audio_pre_cnt == usb_audio_cur_cnt) {
		if (usb_audio->play_state) {
			usb_audio->play_state = 0;
			_usb_audio_stream_state_notify(USOUND_STREAM_STOP);
		}
	}
#endif
}

#ifdef CONFIG_USOUND_START_STOP_CIS_DETECT
#if (CONFIG_USB_AUDIO_RESOLUTION == 16)
static uint32_t energy(const short *pcm_data, uint32_t len)
{
	uint32_t sample_value = 0, i;
	int32_t temp_value;
	
	len /= 2;
	for (i = 0; i < len; i++)
	{
		if (pcm_data[i] < 0)
			temp_value = 0 - pcm_data[i];
		else
			temp_value = pcm_data[i];
		sample_value += (uint32_t)temp_value;
	}

	return (sample_value / len);
}
#else
static uint32_t energy(const int *pcm_data, uint32_t len)
{
	uint64_t sample_value = 0, i;
	int32_t temp_value;

	len /= 4;
	for (i = 0; i < len; i++)
	{
		if (pcm_data[i] < 0)
			temp_value = 0 - pcm_data[i];
		else
			temp_value = pcm_data[i];
		sample_value += (uint32_t)temp_value;
	}

	return (sample_value / len);
}
#endif
#endif

static int _usb_audio_check_stream(void *pcm_data, u32_t len)
{
#ifndef CONFIG_USOUND_START_STOP_CIS_DETECT
	assert(usb_audio);

	if (!usb_audio->play_state) {
		usb_audio->play_state = 1;
		_usb_audio_stream_state_notify(USOUND_STREAM_START);
	}

	/* FIXME: need to optimize the interval */
	os_delayed_work_submit(&usb_audio_dat_detect_work, 10000);
#else
    //printk("%d ", energy_statistics(pcm_data, samples));
    if (energy(pcm_data, len) == 0) {
		usb_audio->zero_frame_cnt++;
		if (usb_audio->play_state && usb_audio->zero_frame_cnt > 1000) {
			usb_audio->play_state = 0;
			usb_audio->zero_frame_cnt = 0;
			_usb_audio_stream_state_notify(USOUND_STREAM_STOP);
		}
        else
        {
            usb_audio_cur_cnt++;
        }
	} else {
		usb_audio->zero_frame_cnt = 0;
		if (!usb_audio->play_state) {
			usb_audio->play_state = 1;
			_usb_audio_stream_state_notify(USOUND_STREAM_START);
			os_delayed_work_submit(&usb_audio_dat_detect_work, 500);
			usb_audio_cur_cnt = 0;
			usb_audio_pre_cnt = 0;
		}
		usb_audio_cur_cnt++;
	}
#endif
	return 0;
}

static int _usb_audio_check_data(void* param)
{
	assert(usb_audio);
	int ret = 0;
	uint32_t diff_num = usb_audio->out_packet_count - usb_audio->last_out_packet_count;
	uint32_t intervalms = timeline_get_interval(usb_audio->tl)/1000;
	//printk("diff %d\n",diff_num);
	if(usb_audio->usound_download_stream){
		if(diff_num < intervalms - 1){
			if(!diff_num && usb_audio->last_diff_num == intervalms - 1){
				ret = stream_write(usb_audio->usound_download_stream, usb_audio->download_buf, MAX_DOWNLOAD_PACKET);
				//printk("fill %d\n",ret);
			}else{
				for(int i = 0;i < intervalms - diff_num;++i){
					ret = stream_write(usb_audio->usound_download_stream, usb_audio->download_buf, MAX_DOWNLOAD_PACKET);
					//printk("fill %d\n",ret);
				}
			}
		}else if(diff_num == intervalms - 1 && !usb_audio->last_diff_num){
			ret = stream_write(usb_audio->usound_download_stream, usb_audio->download_buf, MAX_DOWNLOAD_PACKET);
			//printk("fill %d\n",ret);
		}
		usb_audio->last_out_packet_count = usb_audio->out_packet_count;
		usb_audio->last_diff_num = diff_num;
	}
	return 0;
}
 
static void usb_audio_stream_upload_data_check(void)
{
    if(upload_req_data == 0)
    {
        printk("\nupload start req\n");
        upload_req_data = 1;

        usb_audio->upload_stream_event = USOUND_UPLOAD_STREAM_START;
	    os_delayed_work_submit(&usb_audio->upload_stream_notify_work, 0);
    }
    else
    {
        os_delayed_work_submit(&usb_audio->upload_stream_data_check_work, 2000);   
    }
}

/*
 * Interrupt Context
 */

extern io_stream_t usb_audio_upload_stream;

extern int usb_audio_stream_read(io_stream_t handle, unsigned char *buf, int len);

void set_soft_gain(unsigned int soft_coef, void *pcmbuf, int len)
{
#if (CONFIG_USB_AUDIO_RESOLUTION == 16)
	short *s_pcm = pcmbuf;
	len /= 2;
    for (; len > 0; len--, s_pcm++)
        *s_pcm = (short)((*s_pcm * soft_coef) >> 15);
#else
	int *i_pcm = pcmbuf;
	len /= 4;
	for (; len > 0; len--, i_pcm++)
		*i_pcm = (int)((*i_pcm >> 8) * (soft_coef >> 7));
#endif
}

#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
static u8_t _usb_pcm_lost_cnt;
//extern void set_mic_upload_soft_volume(int sound_vol, u8_t *in_buf, int len, int byte_depth);
short skip_frame_cnt = 0;
short skip_frame_cnt_ex = 0;
int usb_reset_upload_skip_ms(void)
{
    skip_frame_cnt = 0;
    return 0;
}

int usb_audio_set_upload_skip_ms(int ms)
{
    skip_frame_cnt_ex = ms;
    return 0;
}

#ifdef CONFIG_BT_MIC_ENABLE_DOWNLOAD_MIC
extern int tr_usound_get_mute_flag(void);
#endif

#if (CONFIG_USB_AUDIO_RESOLUTION == 16)
#define USB_AUDIO_BYTE_DEPTH	2
#else
#define USB_AUDIO_BYTE_DEPTH	4
#endif

/*soundcard soft volume*/
static const long long soundcard_range_table[17] = 
{
    0, 26971175, 42746431, 53814569, 67748528, 
	85290344, 107374182, 135176086, 170176610, 
	214239659, 269711751, 339546978, 427464319, 
	538145649, 677485289, 852903447, 1073741824,
};

static void set_soft_volume(long long soft_coef, void *pcmbuf, int len, int byte_depth)
{
	if(byte_depth == 2) {
		short *s_pcm = pcmbuf;
		len /= 2;

		for (; len > 0; len--, s_pcm++)
	        *s_pcm = (short)((*s_pcm * soft_coef) >> 30);
	} else {
		int *i_pcm = pcmbuf;
		len /= 4;
		
		for (; len > 0; len--, i_pcm++)
			*i_pcm = (int)((*i_pcm * soft_coef) >> 30);
	}
}

#ifndef CONFIG_USOUND_DOUBLE_SOUND_CARD
static void set_mic_upload_soft_volume(int sound_vol, u8_t *in_buf, int len, int byte_depth)
{
    if(sound_vol < 0 || sound_vol > 16)
    {
        SYS_LOG_INF("set mic soft error:%d\n", sound_vol);
        return;
    }

    if(!sound_vol)
    {
		memset(in_buf, 0x00, len);
        return;
    }

    set_soft_volume(soundcard_range_table[sound_vol], in_buf, len, byte_depth);

}
#endif
#ifdef CONFIG_USOUND_DOUBLE_SOUND_CARD
void set_sound_card_soft_volume(int sound_vol, u8_t *in_buf, int len, int byte_depth)
{
    if(sound_vol < 0 || sound_vol > 16)
    {
        SYS_LOG_INF("set soundcard soft error:%d\n", sound_vol);
        return;
    }

    int mute = audio_system_get_stream_mute(AUDIO_STREAM_USOUND);

    if(!sound_vol || mute)
    {
		memset(in_buf, 0x00, len);
        return;
    }

    set_soft_volume(soundcard_range_table[sound_vol], in_buf, len, byte_depth);
}
#endif
static void _usb_audio_in_ep_complete(u8_t ep,
	enum usb_dc_ep_cb_status_code cb_status)
{
	int len, upload_data_len;
    int tmp_len = 0;

	SYS_LOG_DBG("");

    usb_audio_stream_upload_data_check();

#ifdef CONFIG_BT_MIC_GAIN_ADJUST
    extern unsigned int tr_usound_get_current_soft_gain(void);
    u32_t soft_coef = tr_usound_get_current_soft_gain();
#endif
	/* In transaction request on this EP, Send recording data to PC */
	if (usb_audio_upload_stream) {
		len = stream_read(usb_audio_upload_stream,
					usb_audio->usb_audio_play_load,
					MAX_UPLOAD_BUF_SIZE);
        
        upload_data_len = len;
        /* debug check upload data len*/
        if(len == MAX_UPLOAD_BUF_SIZE)
        {
            if(_usb_pcm_lost_cnt)
            {
                printk("i(%d)\n", _usb_pcm_lost_cnt);
                _usb_pcm_lost_cnt = 0;
            }
        }
        else
        {
            _usb_pcm_lost_cnt++;
	        printk("i");
	        usb_audio_tx_dummy();
            return;
        }
        /*****************************/
        //printk("len:%d, stream_len%d\n", len, stream_get_length(usb_audio_upload_stream));
#ifdef CONFIG_BT_MIC_GAIN_ADJUST
        if(g_usound_app)
        {
            set_soft_gain(soft_coef, (short *)usb_audio->usb_audio_play_load, len);
        }
        else
        {
#ifndef CONFIG_USOUND_DOUBLE_SOUND_CARD
            set_mic_upload_soft_volume(usb_audio->umic_vol, usb_audio->usb_audio_play_load, len, USB_AUDIO_BYTE_DEPTH);
#endif
        }
#else
#ifndef CONFIG_USOUND_DOUBLE_SOUND_CARD
        set_mic_upload_soft_volume(usb_audio->umic_vol, usb_audio->usb_audio_play_load, len, USB_AUDIO_BYTE_DEPTH);
#endif
#endif
        if(usb_device_get_host_os_type_flag() == 2)
        {
            if(skip_frame_cnt < SKIP_FRAME_MAC_OS_CNT + skip_frame_cnt_ex)//过滤前面上传的SKIP_FRAME_CNT ms的数据 mac os
            {
                skip_frame_cnt++;
                memset(usb_audio->usb_audio_play_load, 0, MAX_UPLOAD_BUF_SIZE);
            }
        }
        else
        {
            if(skip_frame_cnt < SKIP_FRAME_CNT + skip_frame_cnt_ex)//过滤前面上传的SKIP_FRAME_CNT ms的数据
            {
                skip_frame_cnt++;
                memset(usb_audio->usb_audio_play_load, 0, MAX_UPLOAD_BUF_SIZE);
            }
        }
#ifdef CONFIG_BT_MIC_ENABLE_DOWNLOAD_MIC
        if(tr_usound_get_mute_flag())
        {
            memset(usb_audio->usb_audio_play_load, 0, MAX_UPLOAD_BUF_SIZE);
        }
#endif

#if (CONFIG_USB_AUDIO_RESOLUTION == 24)
	    for(u8_t *s = usb_audio->usb_audio_play_load, *d = usb_audio->usb_audio_play_load; s < usb_audio->usb_audio_play_load + len; s += 4, d += 3) {
		    d[0] = s[0];
		    d[1] = s[1];
            d[2] = s[2];
            upload_data_len += 3;
	    }
        upload_data_len -= len;
#endif
	    //bt_debug_io_display(DEBUG_GPIO_CIS_PCM_OUT_FOR_APP, usb_audio->usb_audio_play_load[0], 8, 0);
	    //usb_audio_tx_unit(usb_audio->usb_audio_play_load);

        if (usb_device_get_android_os_flag() == 2)
        {
            for(u8_t *s = usb_audio->usb_audio_play_load, *d = usb_audio->usb_audio_play_load; 
                    s < usb_audio->usb_audio_play_load + upload_data_len; s += 3, d += 2) 
            {
                d[0] = s[1];
                d[1] = s[2];
                tmp_len += 2;
            }
            upload_data_len = tmp_len;
        }

        usb_write(CONFIG_USB_AUDIO_DEVICE_SOURCE_IN_EP_ADDR, usb_audio->usb_audio_play_load, upload_data_len, NULL);
        return;

	}

	usb_audio_tx_dummy();
}
#endif
/*
 * Interrupt Context
 */
#if (CONFIG_USB_AUDIO_RESOLUTION == 24)
#define sample s32_t
static u32_t _usb_24bits_convert_to_32bits(u8_t *buf, int len)
{
    s32_t *p = NULL;
	u32_t len_32bits = (len / 3) * 4;
    
	for(u8_t *s = buf + len - 3, *d = buf + len_32bits - 4; s >= buf; s -= 3, d -= 4) {
		d[3] = s[2];
		d[2] = s[1];
		d[1] = s[0];
		d[0] = 0;
        
         p   = (int*)d;
		*p   >>= (8 + usb_audio->download_gain);
	}

	return len_32bits;
}

static u32_t _usb_16bits_convert_to_32bits(u8_t *buf, int len)
{
    s32_t *p = NULL;
	u32_t len_32bits = (len / 2) * 4;
	
	for(u8_t *s = buf + len - 2, *d = buf + len_32bits - 4; s >= buf; s -= 2, d -= 4) {
		d[3] = s[1];
		d[2] = s[0];
		d[1] = 0;
		d[0] = 0;

        // p   = (int*)d;
		// *p   >>= 16;
		p = (int *)d;
        *p >>= usb_audio->download_gain;
	}

	return len_32bits;
}
#else
//#define sample s16_t
#endif
#if 0
#define sample s32_t
static u32_t _usb_16bits_convert_to_32bits(u8_t *buf, int len)
{
	u32_t len_32bits = (len / 2) * 4;
	
	for(u8_t *s = buf + len - 2, *d = buf + len_32bits - 4; s >= buf; s -= 2, d -= 4) {
		d[3] = s[2];
		d[2] = s[1];
		d[1] = 0;
		d[0] = 0;
	}

	return len_32bits;
}
#endif

void usb_audio_download_gain_set(uint8_t gain)
{
    if (usb_audio)
    {
        if (gain < USB_AUDIO_GAIN_UNKNOWN)
        {
            usb_audio->download_gain = gain;
        }
    }
}

static void _usb_audio_out_ep_complete(u8_t ep,
	enum usb_dc_ep_cb_status_code cb_status)
{
	u32_t read_byte = 0, res;
    //static u32_t pre_length = 0;
    //static u32_t ringbuf_offset = 0;
	static u8_t ISOC_out_Buf[MAX_DOWNLOAD_BUF_SIZE] __aligned(4);
	//static u8_t ISOC_out_Buf[384];
#ifdef CONFIG_USOUND_DOUBLE_SOUND_CARD
    sample *soundcard_1_buf = NULL;
    sample *soundcard_2_buf = NULL;
	static u8_t cnt = 0;
	static u8_t soundcard1_flag = 0;
	static u8_t soundcard2_flag = 0;
    static u8_t ISOC_out_Buf2[MAX_DOWNLOAD_BUF_SIZE] __aligned(4);
#endif

    //printk("soundcard num:%d, ep:%d\n", soundcard_cnt, ep);
	/* Out transaction on this EP, data is available for read */
	if (USB_EP_DIR_IS_OUT(ep) && cb_status == USB_DC_EP_DATA_OUT) {
#ifdef CONFIG_USOUND_DOUBLE_SOUND_CARD
        if(soundcard_cnt == 2)
        {
            //printk("ep:%d\n", ep);
            if(ep == 1)
            {
		        res = usb_audio_device_ep_read(ISOC_out_Buf2,
				        sizeof(ISOC_out_Buf2), &read_byte, ep);
#if (CONFIG_USB_AUDIO_RESOLUTION == 24)

                if(usb_device_get_switch_os_flag() == 1)
                {
			        read_byte = _usb_16bits_convert_to_32bits(ISOC_out_Buf2, read_byte);
                }
                else
                {
			        read_byte = _usb_24bits_convert_to_32bits(ISOC_out_Buf2, read_byte);
                }
				//read_byte = _usb_24bits_convert_to_32bits(ISOC_out_Buf2, read_byte);
#endif
		        if (res || read_byte == 0)
                {
                    memset(ISOC_out_Buf2, 0, sizeof(ISOC_out_Buf2));
                    SYS_LOG_ERR("usb audio read error:%d\n", ep);
                }
                else
                {
                    set_sound_card_soft_volume(soundcard2_soft_vol, ISOC_out_Buf2, sizeof(ISOC_out_Buf2), USB_AUDIO_BYTE_DEPTH);
                }
                cnt++;
           
                if(soundcard2_flag)
                {
                    usb_audio->out_packet_count++;
			        _usb_audio_check_stream(ISOC_out_Buf2, sizeof(ISOC_out_Buf2));
                    
                    if (usb_audio->usound_download_stream) 
                    {
				        //cis_debug_pcm_in(ISOC_out_Buf2, sizeof(ISOC_out_Buf2));

				        res = stream_write(usb_audio->usound_download_stream, ISOC_out_Buf2, sizeof(ISOC_out_Buf2));
				        if (res != read_byte) {
					        //SYS_LOG_ERR("%d", res);
				        }
			        }
                    cnt = 0;
                    return;
                }

                soundcard1_flag = 0;
                soundcard2_flag = 1;

            }
            else 
            {
		        res = usb_audio_device_ep_read(ISOC_out_Buf,
				        sizeof(ISOC_out_Buf), &read_byte, ep);
#if (CONFIG_USB_AUDIO_RESOLUTION == 24)
				if(usb_device_get_switch_os_flag() == 1)
                {
			        read_byte = _usb_16bits_convert_to_32bits(ISOC_out_Buf, read_byte);
                }
                else
                {
			        read_byte = _usb_24bits_convert_to_32bits(ISOC_out_Buf, read_byte);
                }
                //read_byte = _usb_24bits_convert_to_32bits(ISOC_out_Buf, read_byte);
#endif
		        if (res || read_byte == 0)
                {
                    memset(ISOC_out_Buf, 0, sizeof(ISOC_out_Buf));
                    SYS_LOG_ERR("usb audio read error:%d\n", ep);
                }
                else
                {
                    set_sound_card_soft_volume(soundcard1_soft_vol, ISOC_out_Buf, sizeof(ISOC_out_Buf), USB_AUDIO_BYTE_DEPTH);
                }
                cnt++;
                
                if(soundcard1_flag)
                {
                    usb_audio->out_packet_count++;
			        _usb_audio_check_stream(ISOC_out_Buf, sizeof(ISOC_out_Buf));
                    
                    if (usb_audio->usound_download_stream) 
                    {
				        //cis_debug_pcm_in(ISOC_out_Buf, sizeof(ISOC_out_Buf));

				        res = stream_write(usb_audio->usound_download_stream, ISOC_out_Buf, sizeof(ISOC_out_Buf));
				        if (res != read_byte) {
					        //SYS_LOG_ERR("%d", res);
				        }
			        }
                    cnt = 0;
                    return;
                }
                
                soundcard1_flag = 1;
                soundcard2_flag = 0;
            }

            if(cnt == 2)
            {
                soundcard_1_buf = (sample *)ISOC_out_Buf;
                soundcard_2_buf = (sample *)ISOC_out_Buf2;

                for(int i = 0; i < sizeof(ISOC_out_Buf)/sizeof(sample); i++)
                    soundcard_1_buf[i] += soundcard_2_buf[i];
                
                usb_audio->out_packet_count++;
			    _usb_audio_check_stream(soundcard_1_buf, sizeof(ISOC_out_Buf));
			    if (usb_audio->usound_download_stream) {
				    //cis_debug_pcm_in(ISOC_out_Buf, sizeof(ISOC_out_Buf));

			    	res = stream_write(usb_audio->usound_download_stream, ISOC_out_Buf, sizeof(ISOC_out_Buf));
                }
                
                if(res < 0)
                {
					SYS_LOG_ERR("res:%d\n", res);
                }
                cnt = 0;
            }
        }
        else
        {
            if(ep == 1)
            {
		        res = usb_audio_device_ep_read(ISOC_out_Buf2,
				        sizeof(ISOC_out_Buf2), &read_byte, ep);
#if (CONFIG_USB_AUDIO_RESOLUTION == 24)
				if(usb_device_get_switch_os_flag() == 1)
                {
			        read_byte = _usb_16bits_convert_to_32bits(ISOC_out_Buf2, read_byte);
                }
                else
                {
			        read_byte = _usb_24bits_convert_to_32bits(ISOC_out_Buf2, read_byte);
                }
                //read_byte = _usb_24bits_convert_to_32bits(ISOC_out_Buf2, read_byte);
#endif
		        if (res || read_byte == 0)
                {
                    memset(ISOC_out_Buf2, 0, sizeof(ISOC_out_Buf2));
                    SYS_LOG_ERR("usb audio read error:%d\n", ep);
                }
                else
                {
                    set_sound_card_soft_volume(soundcard2_soft_vol, ISOC_out_Buf2, sizeof(ISOC_out_Buf2), USB_AUDIO_BYTE_DEPTH);
			        usb_audio->out_packet_count++;
			        _usb_audio_check_stream(ISOC_out_Buf2, sizeof(ISOC_out_Buf2));
                    
                    if (usb_audio->usound_download_stream) 
                    {
				        //cis_debug_pcm_in(ISOC_out_Buf2, sizeof(ISOC_out_Buf2));

				        res = stream_write(usb_audio->usound_download_stream, ISOC_out_Buf2, sizeof(ISOC_out_Buf2));
				        if (res != read_byte) {
					        //SYS_LOG_ERR("%d", res);
				        }
			        }
                }
            }
            else
            {
                res = usb_audio_device_ep_read(ISOC_out_Buf,
				        sizeof(ISOC_out_Buf), &read_byte, ep);
#if (CONFIG_USB_AUDIO_RESOLUTION == 24)
				if(usb_device_get_switch_os_flag() == 1)
                {
			        read_byte = _usb_16bits_convert_to_32bits(ISOC_out_Buf, read_byte);
                }
                else
                {
			        read_byte = _usb_24bits_convert_to_32bits(ISOC_out_Buf, read_byte);
                }
                //read_byte = _usb_24bits_convert_to_32bits(ISOC_out_Buf, read_byte);
#endif
		        if (res || read_byte == 0)
                {
                    memset(ISOC_out_Buf, 0, sizeof(ISOC_out_Buf));
                    SYS_LOG_ERR("usb audio read error:%d\n", ep);
                }
                else
                {
                    set_sound_card_soft_volume(soundcard1_soft_vol, ISOC_out_Buf, sizeof(ISOC_out_Buf), USB_AUDIO_BYTE_DEPTH);
			        usb_audio->out_packet_count++;
			        _usb_audio_check_stream(ISOC_out_Buf, sizeof(ISOC_out_Buf));
                    
                    if (usb_audio->usound_download_stream) 
                    {
				        //cis_debug_pcm_in(ISOC_out_Buf, sizeof(ISOC_out_Buf));

				        res = stream_write(usb_audio->usound_download_stream, ISOC_out_Buf, sizeof(ISOC_out_Buf));
				        if (res != read_byte) {
					        //SYS_LOG_ERR("%d", res);
				        }
			        }
                }
            }
		    
	    }
#endif
        res = usb_audio_device_ep_read(ISOC_out_Buf,
				MAX_DOWNLOAD_PACKET, &read_byte, ep);
        //printk("pc download res:%d read_byte:%d\n", res, read_byte);
		if (!res && read_byte != 0) {
#if (CONFIG_USB_AUDIO_RESOLUTION == 24)
            if(usb_device_get_switch_os_flag() == 1)
            {
			    read_byte = _usb_16bits_convert_to_32bits(ISOC_out_Buf, read_byte);
            }
            else
            {
			    read_byte = _usb_24bits_convert_to_32bits(ISOC_out_Buf, read_byte);
            }
#endif
			//read_byte = _usb_16bits_convert_to_32bits(ISOC_out_Buf, read_byte);
            usb_audio->out_packet_count++;

			if (usb_audio->usound_download_stream) {
				//cis_debug_pcm_in(ISOC_out_Buf, read_byte);
#if 0
            //pre_length += acts_ringbuf_put(usb_audio->download_ringbuf, ISOC_out_Buf, read_byte);
            if(stream_get_length(usb_audio->usound_download_stream) + pre_length > 4992)
            {
                printk("c (%d)\n", stream_get_length(usb_audio->usound_download_stream));
            }
                
            if(usb_audio->download_ringbuf->size - ringbuf_offset < read_byte)
            {
                read_byte = usb_audio->download_ringbuf->size - ringbuf_offset;
            }
          
            memcpy((void *)(usb_audio->download_ringbuf->cpu_ptr + ringbuf_offset), ISOC_out_Buf, read_byte);
            pre_length += read_byte;
            ringbuf_offset += read_byte;

            if(pre_length >= usb_audio->download_ringbuf->size/2)
            {
                stream_write(usb_audio->usound_download_stream, NULL, usb_audio->download_ringbuf->size/2);
                pre_length -= usb_audio->download_ringbuf->size/2;
                if(ringbuf_offset >= usb_audio->download_ringbuf->size)
                    ringbuf_offset = 0;
            }
            return;
#endif       
               
                if(usb_audio->stream_mute)
                {
                    memset(ISOC_out_Buf, 0, read_byte);
                }

			    _usb_audio_check_stream(ISOC_out_Buf, read_byte);

                if(usb_audio->skip_ms > 0)
                {
                    usb_audio->skip_ms--;//过滤
                    memset(ISOC_out_Buf, 0, read_byte);
                }
 
#ifdef CONFIG_USB_DOWNLOAD_LISTEN_SUPPORT
                if(soundcard_sink_mute_flag)
                {
                    memset(ISOC_out_Buf, 0, read_byte); 
                }else
                {
                    set_sound_card_soft_volume(soundcard_sink_vol, ISOC_out_Buf, sizeof(ISOC_out_Buf), USB_AUDIO_BYTE_DEPTH);
                }
#endif
                
				res = stream_write(usb_audio->usound_download_stream, ISOC_out_Buf, read_byte);
				if (res != read_byte) {
					//SYS_LOG_ERR("res %d read_byte %d sl %d", res, read_byte, stream_get_length(usb_audio->usound_download_stream));
				}
			}
		}
    }
}

#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
static void _usb_audio_source_start_stop(bool start)
{
	if (!start) {
		usb_audio_source_inep_flush();
		/*
		 * Don't cleanup in "Set Alt Setting 0" case, Linux may send
		 * this before going "Set Alt Setting 1".
		 */
		/* uac_cleanup(); */
		_usb_audio_stream_state_notify(USOUND_UPLOAD_STREAM_STOP);
	} else {
		/*
		 * First packet is all-zero in case payload be flushed (Linux
		 * may send "Set Alt Setting" several times).
		 */

        upload_req_data = 0;
		if (usb_audio_upload_stream) {
			/**drop stream data before */
			stream_flush(usb_audio_upload_stream);
		}
		usb_audio_tx_dummy();
		_usb_audio_stream_state_notify(USOUND_UPLOAD_STREAM_START);
	}
}
#endif
static void _usb_audio_sink_start_stop(bool start)
{
	if (!start && usb_audio->play_state) {
		usb_audio->play_state = 0;
		_usb_audio_stream_state_notify(USOUND_STREAM_STOP);
	}
}

void usb_audio_tx_flush(void)
{
	usb_audio_source_inep_flush();
}

int usb_audio_set_stream(io_stream_t stream)
{
	SYS_IRQ_FLAGS flags;

	sys_irq_lock(&flags);
	if(!stream){
		usb_audio->last_out_packet_count = 0;
		usb_audio->out_packet_count = 0;
		usb_audio->last_diff_num = 0;
	}
        
	usb_audio->usound_download_stream = stream;
    usb_audio->download_ringbuf = stream_get_ringbuffer(usb_audio->usound_download_stream);
	sys_irq_unlock(&flags);
	SYS_LOG_INF("stream %p\n", stream);
	return 0;
}

void usb_audio_set_stream_mute(u8_t mute)
{
    usb_audio->stream_mute = mute; 
}

void set_umic_soft_vol(u8_t vol)
{
    usb_audio->umic_vol = vol;
}

static int usb_host_sync_volume_to_device(int host_volume)
{
	int vol_db;

	if (host_volume == 0x0000) {
		vol_db = 0;
	} else {
		vol_db = (int)((host_volume - 65536)*10 / 256.0);
	}
	return vol_db;
}

static void _usb_audio_sink_vol_changed_notify(u8_t info_type, int chan_num, int *pstore_info)
{
	int volume_db;

	if (!usb_audio) {
		return;
	}

	if (info_type != USOUND_SYNC_HOST_VOL_TYPE) {
		usb_audio->call_back_type = info_type;
		os_work_submit(&usb_audio->call_back_work);
		return;
	}

	volume_db = usb_host_sync_volume_to_device(*pstore_info);

	if (volume_db != usb_audio->pre_volume_db || usb_audio->pre_volume_db == 0) {
		usb_audio->pre_volume_db = volume_db;
		usb_audio->call_back_type = info_type;
		usb_audio->call_back_param = *pstore_info;
		os_work_submit(&usb_audio->call_back_work);
	}
}

#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
static void _usb_audio_source_vol_changed_notify(u8_t info_type, int chan_num, int *pstore_info)
{
	int volume_db;

	if (!usb_audio) {
		return;
	}

	if (info_type != UMIC_SYNC_HOST_VOL_TYPE) {
		usb_audio->call_back_type = info_type;
		os_work_submit(&usb_audio->call_back_work);
		return;
	}

	volume_db = usb_host_sync_volume_to_device(*pstore_info);

	usb_audio->call_back_type = info_type;
	//usb_audio->call_back_param = volume_db;
	usb_audio->call_back_param = *pstore_info;
	os_work_submit(&usb_audio->call_back_work);
}
#endif
void _usb_audio_switch_to_ios(void)
{
	if (!usb_audio) {
		return;
	}

	usb_audio->call_back_type = USOUND_SWITCH_TO_IOS;
	usb_audio->call_back_param = 0;
	os_work_submit(&usb_audio->call_back_work);
}
static void _usb_audio_callback(struct k_work *work)
{
	if (usb_audio && usb_audio->cb) {
		usb_audio->cb(usb_audio->call_back_type, usb_audio->call_back_param);
	}
}

static void _usb_upload_stream_callback(struct k_work *work)
{
    if(usb_audio->upload_stream_event == USOUND_UPLOAD_STREAM_START && !upload_req_data)
    {
        SYS_LOG_INF("usb upload not real req!\n");
        return;
    }
    
    if(usb_audio->upload_stream_event == USOUND_UPLOAD_STREAM_STOP && upload_req_data)
    {
        SYS_LOG_INF("usb upload not req!\n");
        upload_req_data = 0;
    }

	if (usb_audio && usb_audio->cb) {
		usb_audio->cb(usb_audio->upload_stream_event, usb_audio->call_back_param);
	}
}
static void _usb_download_stream_callback(struct k_work *work)
{
    if (usb_audio && usb_audio->cb) {
		usb_audio->cb(usb_audio->download_stream_event, usb_audio->call_back_param);
	}
}
static void _usb_upload_stream_data_check_callback(struct k_work *work)
{
    if (usb_audio && usb_audio->cb) {
        upload_req_data = 0;
        SYS_LOG_INF("usb upload req stop!\n");
        usb_audio->upload_stream_data_check_event = USOUND_UPLOAD_STREAM_STOP;
		usb_audio->cb(usb_audio->upload_stream_data_check_event, usb_audio->call_back_param);
	}
}

static void _usb_audio_call_status_cb(u32_t status_rec)
{
	SYS_LOG_DBG("status_rec: 0x%04x", status_rec);
}

#ifdef CONFIG_NVRAM_CONFIG
#include <string.h>
#include <drivers/nvram_config.h>
#include <property_manager.h>
#endif

int usb_device_composite_fix_dev_sn(void)
{
	static u8_t mac_str[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];
	int ret;
	int read_len;
#ifdef CONFIG_NVRAM_CONFIG
	read_len = nvram_config_get(CFG_BT_MAC, mac_str, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
	read_len = -1;
	if (read_len < 0) {
		SYS_LOG_DBG("no sn data in nvram: %d", read_len);
#ifdef CONFIG_USOUND_DOUBLE_SOUND_CARD
        u8_t buf[13] = "0123456789AC";
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, buf, strlen(buf));
#else
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_APP_AUDIO_DEVICE_SN, strlen(CONFIG_USB_APP_AUDIO_DEVICE_SN));
#endif
        if (ret)
			return ret;
	} else {
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, mac_str, read_len);
		if (ret)
			return ret;
	}
#else
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_APP_AUDIO_DEVICE_SN, strlen(CONFIG_USB_APP_AUDIO_DEVICE_SN));
		if (ret)
			return ret;
#endif
	return 0;
}

static const int usb_audio_sink_pa_table[16] = {
       -59000, -44000, -38000, -33900,
       -29000, -24000, -22000, -19400,
       -17000, -14700, -12300, -10000,
       -7400,  -5000,  -3000,  0,
};
#ifndef CONFIG_USB_IAP2
#define CONFIG_USB_IAP2	"iAP Interface"
#endif
#define CONFIG_USB_DEVICE_SN_IAP "XSP12345678"
static void _usb_audio_clean_upstream_cb(struct hrtimer *ttimer, void *expiry_fn_arg)
{
    //printk("xy:%d ", stream_get_length(usb_audio_upload_stream));
    //关闭上行时,保持dsp继续运行,1ms定时器定时来取数模拟usb上行常开的场景
    if (stream_get_length(usb_audio_upload_stream) >= MAX_UPLOAD_BUF_SIZE) {
    stream_read(usb_audio_upload_stream, NULL, MAX_UPLOAD_BUF_SIZE);
    }
}

int usb_audio_init(usb_audio_event_callback cb)
{

	int ret = 0;
	int audio_cur_level, audio_cur_dat;

	usb_audio = mem_malloc(sizeof(struct usb_audio_info));
	if (!usb_audio)
		return -ENOMEM;

	usb_audio->usb_audio_play_load = mem_malloc(MAX_UPLOAD_BUF_SIZE);
	if (!usb_audio->usb_audio_play_load) {
		mem_free(usb_audio);
		return -ENOMEM;
	}

	usb_audio->download_buf = mem_malloc(MAX_DOWNLOAD_PACKET);
	if (!usb_audio->download_buf) {
		mem_free(usb_audio->usb_audio_play_load);
		mem_free(usb_audio);
		return -ENOMEM;
	}

	os_work_init(&usb_audio->call_back_work, _usb_audio_callback);
	os_delayed_work_init(&usb_audio->upload_stream_notify_work, _usb_upload_stream_callback);
	os_delayed_work_init(&usb_audio->upload_stream_data_check_work, _usb_upload_stream_data_check_callback);
	os_delayed_work_init(&usb_audio->download_stream_notify_work, _usb_download_stream_callback);
	usb_audio->cb = cb;
	usb_audio->play_state = 0;
	usb_audio->zero_frame_cnt = 0;
	usb_audio->out_packet_count = 0;
	usb_audio->listener.param = usb_audio;
	usb_audio->listener.trigger = _usb_audio_check_data;

	audio_cur_level = audio_system_get_current_volume(AUDIO_STREAM_USOUND);
	audio_cur_dat = (usb_audio_sink_pa_table[audio_cur_level]/1000) * 256 + 65536;
	audio_cur_dat = _USB_AUDIO_MAX_VOL;
	SYS_LOG_DBG("audio_cur_level:%d, audio_cur_dat:0x%x", audio_cur_level, audio_cur_dat);

	usb_audio_device_sink_register_start_cb(_usb_audio_sink_start_stop);
	usb_audio_device_register_inter_out_ep_cb(_usb_audio_out_ep_complete);
	usb_audio_device_sink_register_volume_sync_cb(_usb_audio_sink_vol_changed_notify);
	usb_audio_register_call_status_cb(_usb_audio_call_status_cb);
	usb_audio_device_sink_set_cur_vol(MAIN_CH, audio_cur_dat);
	usb_audio_device_sink_set_cur_vol(CHANNEL_1, audio_cur_dat);
	usb_audio_device_sink_set_cur_vol(CHANNEL_2, audio_cur_dat);
	usb_audio_device_sink_set_vol_info(MAXIMUM_VOLUME, _USB_AUDIO_MAX_VOL);
	usb_audio_device_sink_set_vol_info(MINIMUM_VOLUME, _USB_AUDIO_MIN_VOL);
	usb_audio_device_sink_set_vol_info(VOLUME_STEP, _USB_AUDIO_VOL_STEP);

#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	usb_audio_device_source_register_start_cb(_usb_audio_source_start_stop);
	usb_audio_device_register_inter_in_ep_cb(_usb_audio_in_ep_complete);
	usb_audio_device_source_register_volume_sync_cb(_usb_audio_source_vol_changed_notify);
	usb_audio_device_source_set_cur_vol(MAIN_CH, _USB_AUDIO_MIC_MAX_VOL);
	usb_audio_device_source_set_vol_info(MAXIMUM_VOLUME, _USB_AUDIO_MAX_VOL);
	usb_audio_device_source_set_vol_info(MINIMUM_VOLUME, _USB_AUDIO_MIN_VOL);
	usb_audio_device_source_set_vol_info(VOLUME_STEP, _USB_AUDIO_VOL_STEP);
#endif

	
	/* register composite device descriptor */
	usb_device_register_descriptors(usb_audio_dev_fs_desc, usb_audio_dev_hs_desc);
    
    usb_device_register_switch_descriptors(usb_audio_dev_switch_desc);

    usb_device_register_android_descriptors(usb_audio_dev_android_desc);

	/* register composite device string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_APP_AUDIO_DEVICE_MANUFACTURER, strlen(CONFIG_USB_APP_AUDIO_DEVICE_MANUFACTURER));
	if (ret) {
		goto exit;
	}
	
    ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_APP_AUDIO_DEVICE_PRODUCT, strlen(CONFIG_USB_APP_AUDIO_DEVICE_PRODUCT));
	if (ret) {
		goto exit;
	}

#ifdef CONFIG_USOUND_DOUBLE_SOUND_CARD
    ret = usb_device_register_string_descriptor(DEV_GAME, CONFIG_USB_APP_AUDIO_DEVICE_GAME, strlen(CONFIG_USB_APP_AUDIO_DEVICE_GAME));
	if (ret) {
		goto exit;
	}
	
    ret = usb_device_register_string_descriptor(DEV_VOICE, CONFIG_USB_APP_AUDIO_DEVICE_VOICE, strlen(CONFIG_USB_APP_AUDIO_DEVICE_VOICE));
	if (ret) {
		goto exit;
	}
#endif

#ifdef CONFIG_USB_IOS_IAP2_SUPPORT
	ret = usb_device_register_string_descriptor(IAP2_DESC, CONFIG_USB_IAP2, strlen(CONFIG_USB_IAP2));
	if (ret) {
		goto exit;
	}
#endif
    ret = usb_device_composite_fix_dev_sn();
	if (ret) {
		goto exit;
	}
	
#ifdef CONFIG_USB_IOS_IAP2_SUPPORT
    ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_DEVICE_SN_IAP, strlen(CONFIG_USB_DEVICE_SN_IAP) );
	if (ret) {
		goto exit;
	}
#endif

	/* USB Hid Device Init*/
	ret = usb_hid_dev_init();
	if (ret) {
		SYS_LOG_ERR("usb hid init failed");
		goto exit;
	}

	/* USB Audio Source & Sink Initialize */
	ret = usb_audio_composite_dev_init();
	if (ret) {
		SYS_LOG_ERR("usb audio init failed");
		goto exit;
	}

#ifdef CONFIG_USOUND_DOUBLE_SOUND_CARD 
    /* USB Audio Source & Sink Initialize */
	ret = usb_audio_composite_dev_init2();
	if (ret) {
		SYS_LOG_ERR("usb audio init failed");
		goto exit;
	}
#endif
    usb_device_composite_init(NULL);

	/* init system work item */
	os_delayed_work_init(&usb_audio_dat_detect_work, usb_audio_handle_dat_detect);

    hrtimer_init(&usb_audio->clean_upstream_timer, _usb_audio_clean_upstream_cb, NULL);
    hrtimer_start(&usb_audio->clean_upstream_timer, 10000, 980);

    //usound_out_dma_init();
	
    SYS_LOG_INF("ok");
exit:
	return ret;
}

int usb_audio_deinit(void)
{
#ifdef CONFIG_BT_TRANSCEIVER
    usb_hid_device_exit();
#endif
    os_delayed_work_cancel(&usb_audio->upload_stream_notify_work);
    os_delayed_work_cancel(&usb_audio->upload_stream_data_check_work);
    os_delayed_work_cancel(&usb_audio->download_stream_notify_work);
    
    usb_device_composite_exit();

    os_delayed_work_cancel(&usb_audio_dat_detect_work);

    if(hrtimer_is_running(&usb_audio->clean_upstream_timer))
    {
        hrtimer_stop(&usb_audio->clean_upstream_timer);
    }
	if (usb_audio) {
		if (usb_audio->usb_audio_play_load)
			mem_free(usb_audio->usb_audio_play_load);

		if (usb_audio->download_buf)
			mem_free(usb_audio->download_buf);

		mem_free(usb_audio);
		usb_audio = NULL;
	}

	return 0;
}

uint32_t usb_audio_out_packet_count(void)
{
	if (!usb_audio) {
		return 0;
	}

	return usb_audio->out_packet_count;
}

