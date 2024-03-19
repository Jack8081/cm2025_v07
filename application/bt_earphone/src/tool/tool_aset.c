/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arithmetic.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <media_mem.h>
#include <media_player.h>
#include <media_effect_param.h>
#include <volume_manager.h>
#include <app_manager.h>
#include "app_defines.h"
#include "tool_app_inner.h"
#include "utils/acts_ringbuf.h"
#include "audio_track.h"
#include "sys/crc.h"
#include "stream.h"
#include "ringbuff_stream.h"
//#include "anc_hal.h"
//#include "soc_anc.h"
#include <bt_manager.h>
#include "app_ui.h"
#include "sys/crc.h"

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#define ASQT_DUMP_BUF_SIZE_BTCALL	(1024)
#define ASQT_DUMP_BUF_SIZE_BTMUSIC	(2048)

#ifndef CONFIG_MEDIA_NEW_DEBUG_DUMP
#define ASQT_MAX_DUMP		(3)
#else
#define ASQT_MAX_DUMP		(4)
#endif

#define MAX_DAE_SIZE (512)
#define DATA_BUFFER_SIZE (520)

#define UPLOAD_TIMER_INTERVAL (2)
#define WAIT_DATA_TIMEOUT (1000)
#define DATA_FRAME_SIZE (768) /* max to 32KHz 24bits 7.5ms */
#define ANC_PCM_SAMPLE_RATE (48000)
#define ANC_PCM_FRAME_TIME (1)
#define ANC_PCM_SAMPLE_WIDTH (16)
#define ANC_PCM_MAX_CHANNELS (3)

#define DUMP_LOG (0)

#define ASQT_UPLOAD_USE_TIMER 0
#define ASQT_UPLOAD_USE_WORKQ 1
#define ASQT_UPLOAD_LOOP_USE  ASQT_UPLOAD_USE_WORKQ

//#define SUPPORT_FORCE_STUB_STOP

#define STUB_INSTALL_TIME_MS (2000)

extern void sys_manager_enable_audio_voice(bool enable);
#ifdef CONFIG_TOOL_ASET_USB_SUPPORT
static void _aset_cmd_deal_stub_stop(void);
#endif

/*
  ANCT死机问题调试代码，把改宏使能，用ANCT工具抓一次数据，然后用shell命令
  dbg_anct start
  dbg_anct stop
  反复操作即可重现死机现象
*/
#define DEBUG    (0)

typedef struct
{
    uint16_t  channel;
    uint16_t  sample_rate;
    uint32_t  sequence;
} audio_asqt_frame_info_t;

typedef struct
{
    usp_packet_head_t head;
    uint16_t payload_crc;
    audio_asqt_frame_info_t info;
} audio_asqt_upload_packet_t;

static uint8_t asqt_tag_table[7] = {
    /* playback tags */
    MEDIA_DATA_TAG_DECODE_OUT1,
    MEDIA_DATA_TAG_PLC,

    /* capture tags */
    MEDIA_DATA_TAG_REF,
    MEDIA_DATA_TAG_MIC1,
    MEDIA_DATA_TAG_AEC1,
    MEDIA_DATA_TAG_AEC2,
    MEDIA_DATA_TAG_ENCODE_IN1,
};

#define ASQT_PKT_SIZE (sizeof(audio_asqt_upload_packet_t) + DATA_FRAME_SIZE)

typedef struct  {
	/* asqt para */
	//char dae_para[MAX_DAE_SIZE];
    char data_buffer[DATA_BUFFER_SIZE]; //download buffer
    char asqt_pkt_buffer[ASQT_PKT_SIZE]; //upload buffer
#if (ASQT_UPLOAD_LOOP_USE == ASQT_UPLOAD_USE_TIMER)
    os_timer upload_timer;
#else
    os_delayed_work upload_work;
#endif
    uint32_t sequence[ARRAY_SIZE(asqt_tag_table)];

	/* asqt dump tag select */
    uint32_t asqt_channels;
    uint32_t anc_channels;
    uint8_t anc_started:1;
    uint8_t asqt_tobe_start:1;
    uint8_t asqt_started:1;
    uint8_t has_effect_set:1;
    uint8_t asqt_mode;  //0: dump call data, 1: dump music data, others: reserved
    volatile uint8_t asqt_data_process;
    uint16_t empty_time;
    uint16_t sample_rate;
    uint16_t frame_size;
#ifdef CONFIG_MEDIA_NEW_DEBUG_DUMP
    io_stream_t dump_streams[1];
#else
    io_stream_t dump_streams[ARRAY_SIZE(asqt_tag_table)];
#endif
    struct acts_ringbuf *dump_bufs[ARRAY_SIZE(asqt_tag_table)];
}asqt_data_t;

static asqt_data_t *asqt_data = NULL;
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
static uint8_t dvfs_setted = 0;
#endif
extern uint32_t get_sample_rate_hz(uint8_t fs_khz);
extern void _restart_player_sync(uint8_t efx_stream_type, bool force);
static void audio_manager_asqt_start(uint32_t asqt_mode, uint32_t asqt_channels);

#if CONFIG_ANC_HAL
static u8_t *anc_pcm_buf = NULL;
static u8_t play_pcm_flag = 0;
static uint32_t anc_pcm_len = 0;
static uint32_t anc_pcm_offset = 0;
static uint32_t anc_pcm_samplerate = 0;
static struct audio_track_t *anc_audio_track = NULL;
static io_stream_t anc_audio_stream;

static void anc_dsp_play_pcm_play(void);
static void _pcm_play_work_func(struct k_work *work);
K_DELAYED_WORK_DEFINE(pcm_play_work, _pcm_play_work_func);

#endif

void _aset_restart_player_sync(uint8_t efx_stream_type, bool force)
{
	struct app_msg msg = {0};
    char *fg_app;

    fg_app = app_manager_get_current_app();
    if (!fg_app){
        return;
    }

	msg.type = MSG_BT_EVENT;
    msg.cmd  = BT_REQ_RESTART_PLAY;

	send_async_msg(fg_app, &msg);
}

#if DUMP_LOG
static int print_hex2(const char* prefix, const void* data, size_t size)
{
    char  sbuf[64];
    
    int  i = 0;
    int  j = 0;
    
    if (prefix != NULL)
    {
        while ((sbuf[j] = prefix[j]) != '\0' && (j + 6) < sizeof(sbuf))
        {
            j++;
        }
    }

    for (i = 0; i < size; i++)
    {
        char  c1 = ((u8_t*)data)[i] >> 4;
        char  c2 = ((u8_t*)data)[i] & 0x0F;

        if (c1 >= 10)
        {
            c1 += 'a' - 10;
        }
        else
        {
            c1 += '0';
        }

        if (c2 >= 10)
        {
            c2 += 'a' - 10;
        }
        else
        {
            c2 += '0';
        }

        sbuf[j++] = ' ';
        sbuf[j++] = c1;
        sbuf[j++] = c2;
        
        if (((i + 1) % 16) == 0)
        {
            sbuf[j++] = '\n';
        }

        if (((i + 1) % 16) == 0 ||
            (j + 6) >= sizeof(sbuf))
        {
            sbuf[j] = '\0';
            j = 0;
            printk("%s", sbuf);
        }
    }

    if ((i % 16) != 0)
    {
        sbuf[j++] = '\n';
    }

    if (j > 0)
    {
        sbuf[j] = '\0';
        j = 0;
        printk("%s", sbuf);
    }

    return i;
}
#endif

#ifdef CONFIG_MEDIA_NEW_DEBUG_DUMP
struct dump_data_packet {
	uint16_t as_node_id;
	uint16_t flags_miss : 1; /*0-normal,1-miss*/
	uint16_t flags_reverved : 7;
	uint16_t flags_seq : 8;  /*0 ~ 255, 0 ~ 255 increase*/
	uint16_t len8;
	uint16_t payload8;
};
static struct dump_data_packet last_dump_data_head;
static int dump_data_head_flag = 0;
static int dump_data_succ_count = 0;
static int dump_data_fail_count = 0;
static uint8_t dump_data_tag_seqs[7];
#endif

static void _asqt_unprepare_data_upload(asqt_data_t *data)
{
    data->asqt_tobe_start = 0;
    if(!data->asqt_started)
        return;

    SYS_LOG_INF("stop");
    data->asqt_started = 0;

    while (data->asqt_data_process) {
        os_sleep(1);
    }

    data->empty_time = 0;
#if (ASQT_UPLOAD_LOOP_USE == ASQT_UPLOAD_USE_TIMER)
    os_timer_stop(&data->upload_timer);
#else
    os_delayed_work_cancel(&data->upload_work);
#endif
    if (data->asqt_mode == 0) {
        media_player_playback_dump_data(NULL, ARRAY_SIZE(asqt_tag_table), asqt_tag_table, NULL);
    } else {
        media_player_capture_dump_data(NULL, ARRAY_SIZE(asqt_tag_table), asqt_tag_table, NULL);
    }

#ifdef CONFIG_MEDIA_NEW_DEBUG_DUMP
    if (data->dump_streams[0]) {
        //stream_close(data->dump_streams[0]); /* dump_streams have not been opened, so needn't close */
        stream_destroy(data->dump_streams[0]);
        data->dump_streams[0] = NULL;
    }
    memset(data->dump_bufs, 0, sizeof(data->dump_bufs));
    dump_data_head_flag = 0;
    dump_data_fail_count = 0;
    dump_data_succ_count = 0;
    memset(dump_data_tag_seqs, 0xff, sizeof(dump_data_tag_seqs));
#else
	for (int32_t i = 0; i < ARRAY_SIZE(data->dump_streams); i++) {
		if (data->dump_streams[i]) {
			//stream_close(data->dump_streams[i]); /* dump_streams have not been opened, so needn't close */
            stream_destroy(data->dump_streams[i]);
			data->dump_streams[i] = NULL;
            data->dump_bufs[i] = NULL;
		}
	}
#endif

	sys_manager_enable_audio_voice(true);

}

static int32_t _asqt_prepare_data_upload(asqt_data_t *data)
{
	char *dumpbuf;
	int32_t num_tags = 0;
    int32_t buffer_size;
    int32_t dump_size;
    int32_t tmp;
    struct audio_track_t *audio_track;
    media_player_t *player = NULL;

    if (data->asqt_mode == 0) {
        player = media_player_get_current_dumpable_capture_player();
    } else {
        player = media_player_get_current_dumpable_playback_player();
    }

	if (!player) {
		SYS_LOG_INF("No dumpable player!");
		return -EFAULT;
	}

	SYS_LOG_INF("player type: %d", player->type);
    // mode 0 to dump call data, mode 1 to dump music data, and others are reserved
    if ((data->asqt_mode == 0 && ((player->type & MEDIA_PLAYER_TYPE_CAPTURE) == 0))
     || (data->asqt_mode == 1 && (player->type != MEDIA_PLAYER_TYPE_PLAYBACK))
     || (data->asqt_mode > 1)) {
        SYS_LOG_INF("asqt mode and type mismatch!");
        return -EPERM;
     }
	 
    audio_system_mutex_lock();
	audio_track = audio_system_get_track();
    if(audio_track) {
        data->sample_rate = get_sample_rate_hz(audio_track_get_samplerate(audio_track));
        if(data->sample_rate > 16000)
            data->sample_rate = 16000;
        if(data->sample_rate == 16000) {
            data->frame_size = DATA_FRAME_SIZE;
        } else {
            data->frame_size = DATA_FRAME_SIZE/2;
        }
		SYS_LOG_INF("audio track, sr:%d, frame_size:%d", data->sample_rate, data->frame_size);
    }
    audio_system_mutex_unlock();
    
    if(data->sample_rate == 0) {
		SYS_LOG_INF("sample rate err!");
        return -EFAULT;
    }
	
    if(player->type == MEDIA_PLAYER_TYPE_PLAYBACK) {
        asqt_tag_table[0] = MEDIA_DATA_TAG_DECODE_OUT1;
        asqt_tag_table[1] = MEDIA_DATA_TAG_DECODE_IN;

#ifdef CONFIG_MEDIA_NEW_DEBUG_DUMP
        dump_size = 512;
        dumpbuf = media_mem_get_cache_pool(NEW_DEBUG_DUMP_BUF, AUDIO_STREAM_MUSIC);
        buffer_size = media_mem_get_cache_pool_size(NEW_DEBUG_DUMP_BUF, AUDIO_STREAM_MUSIC);
        if (!dumpbuf)
            goto fail;
#else
        dump_size = ASQT_DUMP_BUF_SIZE_BTMUSIC;
        dumpbuf = media_mem_get_cache_pool(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_MUSIC);
        buffer_size = media_mem_get_cache_pool_size(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_MUSIC);
#endif
    } else {
        uint8_t stream_type;
        if (player->format == NAV_TYPE) {
            stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
        } else if (player->format == PCM_DSP_TYPE) {
            stream_type = AUDIO_STREAM_USOUND;
		} else {
            stream_type = AUDIO_STREAM_VOICE;
        }
        asqt_tag_table[0] = MEDIA_DATA_TAG_DECODE_OUT1;
        asqt_tag_table[1] = MEDIA_DATA_TAG_PLC;
        if (audio_policy_get_record_audio_mode(stream_type, AUDIO_STREAM_VOICE) == AUDIO_MODE_STEREO) {
    		asqt_tag_table[6] = MEDIA_DATA_TAG_MIC2;
    	} else {
    		asqt_tag_table[6] = MEDIA_DATA_TAG_ENCODE_IN1;
    	}

#ifdef CONFIG_MEDIA_NEW_DEBUG_DUMP
        dump_size = 512;
        if (player->format == NAV_TYPE) {
            dumpbuf = media_mem_get_cache_pool(NEW_DEBUG_DUMP_BUF, AUDIO_STREAM_LE_AUDIO_MUSIC);
            buffer_size = media_mem_get_cache_pool_size(NEW_DEBUG_DUMP_BUF, AUDIO_STREAM_LE_AUDIO_MUSIC);
        } else if (player->format == PCM_DSP_TYPE) {
			dumpbuf = media_mem_get_cache_pool(NEW_DEBUG_DUMP_BUF, AUDIO_STREAM_USOUND);
            buffer_size = media_mem_get_cache_pool_size(NEW_DEBUG_DUMP_BUF, AUDIO_STREAM_USOUND);
		} else {
            dumpbuf = media_mem_get_cache_pool(NEW_DEBUG_DUMP_BUF, AUDIO_STREAM_VOICE);
            buffer_size = media_mem_get_cache_pool_size(NEW_DEBUG_DUMP_BUF, AUDIO_STREAM_VOICE);
        }
        if (!dumpbuf) {
            SYS_LOG_ERR("NEW_DEBUG_DUMP_BUF not find!");
            goto fail;
        }
#else
        dump_size = ASQT_DUMP_BUF_SIZE_BTCALL;
        dumpbuf = media_mem_get_cache_pool(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_VOICE);
        buffer_size = media_mem_get_cache_pool_size(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_VOICE);
#endif
    }

    data->asqt_started = 1;
    data->empty_time = 0;
    memset(data->sequence, 0x0, sizeof(data->sequence));

#ifdef CONFIG_MEDIA_NEW_DEBUG_DUMP
    tmp = buffer_size;
    dump_data_head_flag = 0;
    dump_data_fail_count = 0;
    dump_data_succ_count = 0;
    memset(dump_data_tag_seqs, 0xff, sizeof(dump_data_tag_seqs));
    data->dump_streams[0] = ringbuff_stream_create_ext(dumpbuf, tmp);
    if (!data->dump_streams[0])
        goto fail;
#endif

	for (int32_t i = 0; i < ARRAY_SIZE(asqt_tag_table); i++) {
		if ((data->asqt_channels & (1 << i)) == 0) /* selected or not */
			continue;

		if (num_tags++ >= ASQT_MAX_DUMP) {
			SYS_LOG_WRN("exceed max %d tags", ASQT_MAX_DUMP);
			break;
		}

#ifndef CONFIG_MEDIA_NEW_DEBUG_DUMP
        if(buffer_size < data->frame_size)
            break;

        tmp = buffer_size;
        if(tmp > dump_size)
            tmp = dump_size;

		/* update to data tag */
		data->dump_streams[i] = ringbuff_stream_create_ext(dumpbuf, tmp);
		if (!data->dump_streams[i])
			goto fail;

		dumpbuf += tmp;
        buffer_size -= tmp;
#endif

#ifdef CONFIG_MEDIA_NEW_DEBUG_DUMP
        data->dump_bufs[i] = stream_get_ringbuffer(data->dump_streams[0]);
#else
        data->dump_bufs[i] = stream_get_ringbuffer(data->dump_streams[i]);
#endif
		SYS_LOG_INF("start idx=%d, tag=%u, buf=%p", i, asqt_tag_table[i], data->dump_bufs[i]);
	}

	sys_manager_enable_audio_voice(false);

#if (ASQT_UPLOAD_LOOP_USE == ASQT_UPLOAD_USE_TIMER)
    os_timer_start(&data->upload_timer, K_MSEC(UPLOAD_TIMER_INTERVAL), K_MSEC(UPLOAD_TIMER_INTERVAL));
#else
    os_delayed_work_submit(&data->upload_work, UPLOAD_TIMER_INTERVAL);
#endif
	return media_player_dump_data(player, ARRAY_SIZE(asqt_tag_table), asqt_tag_table, data->dump_bufs);
fail:
	SYS_LOG_ERR("failed");
	_asqt_unprepare_data_upload(data);
	return -ENOMEM;
}

static int32_t _asqt_upload_pkt(asqt_data_t *data, int32_t channel, struct acts_ringbuf *rbuf, uint16_t len)
{
    audio_asqt_upload_packet_t *packet = (audio_asqt_upload_packet_t*)data->asqt_pkt_buffer;
    uint8_t *playload;
    int32_t ret;
    int32_t pkt_size;

    packet->info.channel = channel;
    packet->info.sample_rate = data->sample_rate;
    packet->info.sequence = data->sequence[channel];

    packet->head.type              = USP_PACKET_TYPE_ISO;
    packet->head.sequence_number   = 0;
    packet->head.protocol_type     = USP_PROTOCOL_TYPE_ASET;
    packet->head.payload_len       = sizeof(audio_asqt_frame_info_t) + len;
    packet->head.predefine_command = CONFIG_PROTOCOL_CMD_ASQT_FRAME;
    packet->head.crc8_val =
        crc8_maxim(0, (void*)&packet->head, sizeof(usp_packet_head_t) - 1);
    
    playload = (uint8_t*)packet + sizeof(audio_asqt_upload_packet_t);
    acts_ringbuf_peek(rbuf, playload, len);
    //print_hex2("dump:", playload, 20);

    packet->payload_crc =
        crc16_ccitt(0, (void*)&packet->info, packet->head.payload_len);

    pkt_size = sizeof(audio_asqt_upload_packet_t) + len;
	//printk("send: %d, %d\n", packet->info.channel, packet->info.sequence);
    ret = g_tool_data.usp_handle.api.write((uint8_t*)packet, pkt_size, USP_WRITE_TIMEOUT_MS);
    if(ret < 0) {
        //printk("send asqt pkt fail: %d, %d\n", packet->info.channel, packet->info.sequence);
        return -1;
    } else {
        //printk("dump:%d,%d,%d\n", channel, data->sequence[channel], len);
    }

    data->sequence[channel] += 1;
    acts_ringbuf_get(rbuf, NULL, len);
    return 0;
}

#ifndef CONFIG_MEDIA_NEW_DEBUG_DUMP
static void get_channel(asqt_data_t *data, char *inbuf, char *outbuf, int channel)
{
    int i = 0, ch = 0;

    ch = channel;
    for(i=0; i<channel; i++){
        if(!(data->anc_channels & 0x1<<i))
            ch--;
    }

    memcpy(outbuf, inbuf+(ANC_PCM_SAMPLE_WIDTH/8)*(ANC_PCM_SAMPLE_RATE/1000)*ANC_PCM_FRAME_TIME*ch, 
            (ANC_PCM_SAMPLE_WIDTH/8)*(ANC_PCM_SAMPLE_RATE/1000)*ANC_PCM_FRAME_TIME);
}

static uint8_t pcm_data[(ANC_PCM_SAMPLE_WIDTH/8)*ANC_PCM_MAX_CHANNELS*ANC_PCM_FRAME_TIME*(ANC_PCM_SAMPLE_RATE/1000)];
static int32_t _anc_upload_pkt(asqt_data_t *data, struct acts_ringbuf *rbuf)
{
    audio_asqt_upload_packet_t *packet = (audio_asqt_upload_packet_t*)data->asqt_pkt_buffer;
    char *playload;
    int32_t ret;
    int32_t pkt_size;
    int i;
    playload = (uint8_t*)packet + sizeof(audio_asqt_upload_packet_t);
    ret = acts_ringbuf_get(rbuf, pcm_data, data->frame_size);
    if(ret != data->frame_size){
        printk("get data err\n");
        return 0;
    }

#if 1
    for(i=0; i<3; i++){
        if(data->anc_channels & 0x1<<i){
            get_channel(data, pcm_data, playload, i);

            packet->info.channel = i;
            packet->info.sample_rate = data->sample_rate;
            packet->info.sequence = data->sequence[i];

            packet->head.type              = USP_PACKET_TYPE_ISO;
            packet->head.sequence_number   = 0;
            packet->head.protocol_type     = USP_PROTOCOL_TYPE_ASET;
            packet->head.predefine_command = CONFIG_PROTOCOL_CMD_ASQT_FRAME;
            packet->head.crc8_val =
                crc8_maxim(0, (void*)&packet->head, sizeof(usp_packet_head_t) - 1);

            packet->head.payload_len       = sizeof(audio_asqt_frame_info_t) + 
                            (ANC_PCM_SAMPLE_WIDTH/8)*ANC_PCM_FRAME_TIME*(ANC_PCM_SAMPLE_RATE/1000);
            packet->payload_crc =
                crc16_ccitt(0, (void*)&packet->info, packet->head.payload_len);

            pkt_size = sizeof(audio_asqt_upload_packet_t) + 
                            (ANC_PCM_SAMPLE_WIDTH/8)*ANC_PCM_FRAME_TIME*(ANC_PCM_SAMPLE_RATE/1000);

            // if(i != 2)
            //     continue;

            ret = g_tool_data.usp_handle.api.write((uint8_t*)packet, pkt_size, USP_WRITE_TIMEOUT_MS);
            if(pkt_size != ret) {
                printk("send asqt pkt fail\n");
                return -1;
            }

            data->sequence[i] += 1;
        }
    }
    //acts_ringbuf_get(rbuf, NULL, data->frame_size);
#endif
    return 0;
}
#endif

#ifdef CONFIG_MEDIA_NEW_DEBUG_DUMP
static int32_t get_tag_by_as_node(uint16_t as_node)
{
    if (as_node == AS_CONTEXT_NODE(AS_CONTEXT_PLAYER, AS_NODE_INPUT_REMOTE))
        return 1;//MEDIA_DATA_TAG_DECODE_IN;
    else if (as_node == AS_CONTEXT_NODE(AS_CONTEXT_PLAYER, AS_NODE_DECODE))
        return 0;//MEDIA_DATA_TAG_DECODE_OUT1;
    else if (as_node == AS_CONTEXT_NODE(AS_CONTEXT_PLAYER, AS_NODE_PLC))
        return 1;//MEDIA_DATA_TAG_PLC;
    else if (as_node == AS_CONTEXT_NODE(AS_CONTEXT_PLAYER, AS_NODE_EFFECT))
        return 6;//MEDIA_DATA_TAG_DECODE_OUT2;
    else if (as_node == AS_CONTEXT_NODE(AS_CONTEXT_RECORDER, AS_NODE_INPUT_MAIN_MIC))
        return 3;//MEDIA_DATA_TAG_MIC1;
    else if (as_node == AS_CONTEXT_NODE(AS_CONTEXT_RECORDER, AS_NODE_INPUT_REF_MIC))
        return 6;//MEDIA_DATA_TAG_MIC2;
    else if (as_node == AS_CONTEXT_NODE(AS_CONTEXT_PLAYER, AS_NODE_AEC_REF))
        return 2;//MEDIA_DATA_TAG_REF;
    else if (as_node == AS_CONTEXT_NODE(AS_CONTEXT_RECORDER, AS_NODE_AEC))
        return 4;//MEDIA_DATA_TAG_AEC1;
    else if (as_node == AS_CONTEXT_NODE(AS_CONTEXT_RECORDER, AS_NODE_DRC))
        return 5;//MEDIA_DATA_TAG_AEC2;

    return -1;
}
#endif

#if (ASQT_UPLOAD_LOOP_USE == ASQT_UPLOAD_USE_TIMER)
static void _asqt_upload_data_timer_proc(os_timer *ttimer)
#else
static void _asqt_upload_data_timer_proc(struct k_work *work)
#endif
{
    asqt_data_t *data = asqt_data;
    int32_t max_tag;
    int32_t ret;
    int32_t stop_proc_flag = 0;

    if(!data->asqt_started) {
        SYS_LOG_WRN("asqt_started == 0");
        return;
    }

    data->asqt_data_process = 1;

#ifdef CONFIG_MEDIA_NEW_DEBUG_DUMP
    struct acts_ringbuf *dump_buf = stream_get_ringbuffer(data->dump_streams[0]);

    while (1) {
        uint32_t debug_dump_buf_len = acts_ringbuf_length(dump_buf);
        if (dump_data_head_flag == 0) {
            if (debug_dump_buf_len >= sizeof(struct dump_data_packet)) {
                acts_ringbuf_get(dump_buf, &last_dump_data_head, sizeof(struct dump_data_packet));
                if (last_dump_data_head.payload8 > 0) {
                    dump_data_head_flag = 1;
                    debug_dump_buf_len = acts_ringbuf_length(dump_buf);
                }
            } else {
                break;
            }
        }
        if (dump_data_head_flag == 1) {
            if (debug_dump_buf_len < last_dump_data_head.payload8) {
                /* wait dsp put data to ringbuf */
                break;
            }
        }

        max_tag = get_tag_by_as_node(last_dump_data_head.as_node_id);
        if (max_tag < 0) {
            SYS_LOG_INF("tag err:%d, id:%x", max_tag, last_dump_data_head.as_node_id);
            acts_ringbuf_get(dump_buf, NULL, last_dump_data_head.payload8);
            dump_data_head_flag = 0;
            break;
        } else {
            if (max_tag >= 2 && max_tag <= 6) { /* preprocess tags */
                if ((uint8_t)last_dump_data_head.flags_seq != (uint8_t)(dump_data_tag_seqs[max_tag] + 1)) {
                    SYS_LOG_WRN("tag %d miss [%d - %d)", max_tag, \
                        (uint8_t)(dump_data_tag_seqs[max_tag] + 1), \
                        (uint8_t)last_dump_data_head.flags_seq);
                }
            }
        }
        data->empty_time = 0;
        //printk("dump %d: %d\n", max_tag, last_dump_data_head.payload8);
        ret = _asqt_upload_pkt(data, max_tag, dump_buf, last_dump_data_head.payload8);
        if(ret < 0) {
            ++dump_data_fail_count;
            if (dump_data_fail_count == 5 || dump_data_fail_count % 100 == 0) {
                printk("_asqt_upload_pkt fail:%d\n", dump_data_fail_count);
            }
#ifdef SUPPORT_FORCE_STUB_STOP
            if (dump_data_fail_count >= 500 && dump_data_succ_count >= 100) {
                /* stub asqt dump, it will miss the stop cmd possibly. */
                SYS_LOG_INF("Force stub asqt dump stop!!");
                stop_proc_flag = 1;
            }
#endif
            break;
        } else {
            ++dump_data_succ_count;
            if (max_tag >= 2 && max_tag <= 6) { /* preprocess tags */
                dump_data_tag_seqs[max_tag] = (uint8_t)last_dump_data_head.flags_seq;
            }
        }
        dump_data_fail_count = 0;
        dump_data_head_flag = 0;
    }

#else

	int32_t i, j;
    int32_t max_size;
    int32_t cur_size;

    if(data->anc_started){
        for(i=0; i<UPLOAD_TIMER_INTERVAL; i++){
            cur_size = acts_ringbuf_length(data->dump_bufs[0]);
            if(cur_size >= data->frame_size)
                _anc_upload_pkt(data, data->dump_bufs[0]);
        }
        
        return;
    }

    //for(j=0; j<5; j++) {
	for(j=0; j<1; j++) {
        max_size = 0;
        max_tag = 0;
    	for (i = 0; i < ARRAY_SIZE(asqt_tag_table); i++) {
    		if (!data->dump_bufs[i]) {
    			continue;
    		}
			
            cur_size = acts_ringbuf_length(data->dump_bufs[i]);
    		if (cur_size > max_size) {
    			max_size = cur_size;
                max_tag = i;
    		}
    	}

        if(max_size == 0) {
            data->empty_time += UPLOAD_TIMER_INTERVAL;
            break;
        }

        data->empty_time = 0;
        //SYS_LOG_INF("dump %d: %d\n", asqt_tag_table[max_tag], max_size);
        ret = _asqt_upload_pkt(data, max_tag, data->dump_bufs[max_tag], max_size);
        if(ret < 0) {
            SYS_LOG_INF("asqt upload pkt fail!\n");
            break;
        }
    }
#endif

    if (stop_proc_flag == 0) {
        #if (ASQT_UPLOAD_LOOP_USE == ASQT_UPLOAD_USE_WORKQ)
        os_delayed_work_submit(&data->upload_work, UPLOAD_TIMER_INTERVAL);
        #endif
    }

    data->asqt_data_process = 0;

#ifdef SUPPORT_FORCE_STUB_STOP
    if (stop_proc_flag == 1) {
        #ifdef CONFIG_TOOL_ASET_USB_SUPPORT
        _aset_cmd_deal_stub_stop();
        #endif
    }
#endif
}

/* ??????
 */
void config_protocol_adjust(uint32_t cfg_id, uint32_t cfg_size, u8_t* cfg_data)
{
    media_player_t *playback_player = NULL;
    media_player_t *capture_player = NULL;
#if CONFIG_ANC_HAL
    int ret;
    CFG_Struct_Audio_Settings audio_settings;
    anc_info_t anc_init_info = {0};
#endif
    
    SYS_LOG_INF("id: %x, size: %d", cfg_id, cfg_size);
    if(cfg_id != CFG_ID_ANC_AL) {
        capture_player = media_player_get_current_dumpable_capture_player();
        playback_player = media_player_get_current_dumpable_playback_player();

    	if (!playback_player && !capture_player) {
			SYS_LOG_INF("No dumpable player now!\n");
    		return;
    	} else {
			SYS_LOG_INF("dumpable player type: %d, %d", playback_player->type, capture_player->type);
    	}
    }
    
    switch (cfg_id)
    {
    case CFG_ID_BT_MUSIC_DAE:
        if (!playback_player) {
            SYS_LOG_INF("No dumpable player now!\n");
            break;
        }
        if ((playback_player->type & MEDIA_PLAYER_TYPE_PLAYBACK) == 0) {
            SYS_LOG_INF("Invalid dumpable player type! %d", playback_player->type);
            break;
        }
        if(cfg_size == sizeof(CFG_Struct_BT_Music_DAE)) {
            CFG_Struct_BT_Music_DAE *p = (CFG_Struct_BT_Music_DAE*)cfg_data;
            uint8_t stream_type;
            
            media_player_set_effect_enable(playback_player, p->Enable_DAE);
            if (playback_player->format == NAV_TYPE) {
                stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
            } else if (playback_player->format == PCM_DSP_TYPE) {
                stream_type = AUDIO_STREAM_USOUND;
			} else {
                stream_type = AUDIO_STREAM_MUSIC;
            }
            system_volume_set(stream_type, p->Test_Volume, false);
        } else {
            SYS_LOG_ERR("size not match, id %d, cfg_size %d, expected %d", cfg_id, cfg_size, sizeof(CFG_Struct_BT_Music_DAE));
        }
        break;
    case CFG_ID_BT_MUSIC_DAE_AL:
        if (!playback_player) {
            SYS_LOG_INF("No dumpable player now!\n");
            break;
        }
        if ((playback_player->type & MEDIA_PLAYER_TYPE_PLAYBACK) == 0) {
            SYS_LOG_INF("Invalid dumpable player type! %d", playback_player->type);
            break;
        }
#if DUMP_LOG
        print_hex2("music dae:", cfg_data, cfg_size);
#endif
        media_player_update_effect_param(playback_player, cfg_data, cfg_size);
        break;
        
    case CFG_ID_BT_CALL_OUT_DAE:
        if (!capture_player) {
            SYS_LOG_INF("No dumpable player now!\n");
            break;
        }
        if ((capture_player->type & MEDIA_PLAYER_TYPE_CAPTURE) == 0) {
            SYS_LOG_INF("Invalid dumpable player type! %d", capture_player->type);
            break;
        }

        if (capture_player->format == NAV_TYPE) {
            SYS_LOG_INF("Invalid dumpable player format! %d", capture_player->format);
            break;
        }

        if(cfg_size == sizeof(CFG_Struct_BT_Call_Out_DAE)) {
            CFG_Struct_BT_Call_Out_DAE *p = (CFG_Struct_BT_Call_Out_DAE*)cfg_data;
            
            media_player_set_effect_enable(capture_player, p->Enable_DAE);
            system_volume_set(AUDIO_STREAM_VOICE, p->Test_Volume, false);
        } else {
            SYS_LOG_ERR("size not match, id %d, cfg_size %d, expected %d", cfg_id, cfg_size, sizeof(CFG_Struct_BT_Call_Out_DAE));
        }
        break;
    case CFG_ID_BT_CALL_MIC_DAE:
        if (!capture_player) {
            SYS_LOG_INF("No dumpable player now!\n");
            break;
        }
        if ((capture_player->type & MEDIA_PLAYER_TYPE_CAPTURE) == 0) {
            SYS_LOG_INF("Invalid dumpable player type! %d", capture_player->type);
            break;
        }

        if (capture_player->format == NAV_TYPE) {
            SYS_LOG_INF("Invalid dumpable player format! %d", capture_player->format);
            break;
        }

        if(cfg_size == sizeof(CFG_Struct_BT_Call_MIC_DAE)) {
            CFG_Struct_BT_Call_MIC_DAE *p = (CFG_Struct_BT_Call_MIC_DAE*)cfg_data;
            media_player_set_upstream_dae_enable(capture_player, p->Enable_DAE);
            system_volume_set(AUDIO_STREAM_VOICE, p->Test_Volume, false);
        } else {
            SYS_LOG_ERR("size not match, id %d, cfg_size %d, expected %d", cfg_id, cfg_size, sizeof(CFG_Struct_BT_Call_MIC_DAE));
        }
        break;
    case CFG_ID_BT_CALL_DAE_AL:
        {
            const void *aec_param = NULL;
            void *dae_param = NULL;
            
            if (!capture_player) {
                SYS_LOG_INF("No dumpable player now!\n");
                break;
            }
            if ((capture_player->type & MEDIA_PLAYER_TYPE_CAPTURE) == 0) {
                SYS_LOG_INF("Invalid dumpable player type! %d", capture_player->type);
                break;
            }

            if (capture_player->format == NAV_TYPE) {
                SYS_LOG_INF("Invalid dumpable player format! %d", capture_player->format);
                break;
            }

            dae_param = media_mem_get_cache_pool(DAE_BUFFER, AUDIO_STREAM_VOICE);
            if(NULL == dae_param)
                break;

#if DUMP_LOG
            print_hex2("call dae:", cfg_data, cfg_size);
#endif
            media_effect_get_param(AUDIO_STREAM_VOICE, AUDIO_STREAM_VOICE, NULL, &aec_param);
            memcpy(dae_param, cfg_data, cfg_size);
            medie_effect_set_user_param(AUDIO_STREAM_VOICE, AUDIO_STREAM_VOICE, dae_param, aec_param);
            media_player_update_effect_param(capture_player, cfg_data, cfg_size);

            if(asqt_data) {
                asqt_data->has_effect_set = 1;
            }
        }
        break;

    case CFG_ID_LEAUDIO_CALL_OUT_DAE:
        if (!capture_player) {
            SYS_LOG_INF("No dumpable player now!\n");
            break;
        }
        if ((capture_player->type & MEDIA_PLAYER_TYPE_CAPTURE) == 0) {
            SYS_LOG_INF("Invalid dumpable player type! %d", capture_player->type);
            break;
        }

        /*FIXME LEAUDIO downstream is music*/
        if ((capture_player->format != NAV_TYPE) && (capture_player->format != PCM_DSP_TYPE)) {
            SYS_LOG_INF("Invalid dumpable player format! %d", capture_player->format);
            break;
        }
        if(cfg_size == sizeof(CFG_Struct_BT_Call_Out_DAE)) {
            CFG_Struct_LEAUDIO_Call_Out_DAE *p = (CFG_Struct_LEAUDIO_Call_Out_DAE*)cfg_data;
            uint8_t stream_type;
            
            media_player_set_effect_enable(capture_player, p->Enable_DAE);
            if (capture_player->format == PCM_DSP_TYPE) {
                stream_type = AUDIO_STREAM_USOUND;
			} else {
				stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
			}
            system_volume_set(stream_type, p->Test_Volume, false);
        } else {
            SYS_LOG_ERR("size not match, id %d, cfg_size %d, expected %d", cfg_id, cfg_size, sizeof(CFG_Struct_BT_Call_Out_DAE));
        }
        break;
    case CFG_ID_LEAUDIO_CALL_MIC_DAE:
        if (!capture_player) {
            SYS_LOG_INF("No dumpable player now!\n");
            break;
        }
        if ((capture_player->type & MEDIA_PLAYER_TYPE_CAPTURE) == 0) {
            SYS_LOG_INF("Invalid dumpable player type! %d", capture_player->type);
            break;
        }

        if ((capture_player->format != NAV_TYPE) && (capture_player->format != PCM_DSP_TYPE)) {
            SYS_LOG_INF("Invalid dumpable player format! %d", capture_player->format);
            break;
        }

        if(cfg_size == sizeof(CFG_Struct_BT_Call_MIC_DAE)) {
            CFG_Struct_LEAUDIO_Call_MIC_DAE *p = (CFG_Struct_LEAUDIO_Call_MIC_DAE*)cfg_data;
			uint8_t stream_type;

            media_player_set_upstream_dae_enable(capture_player, p->Enable_DAE);
			if (capture_player->format == PCM_DSP_TYPE) {
                stream_type = AUDIO_STREAM_USOUND;
			} else {
				stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
			}
			system_volume_set(stream_type, p->Test_Volume, false);
        } else {
            SYS_LOG_ERR("size not match, id %d, cfg_size %d, expected %d", cfg_id, cfg_size, sizeof(CFG_Struct_BT_Call_MIC_DAE));
        }
        break;
    case CFG_ID_LEAUDIO_CALL_DAE_AL:
        {
            const void *aec_param = NULL;
            void *dae_param = NULL;
			uint8_t stream_type;
            
            if (!capture_player) {
                SYS_LOG_INF("No dumpable player now!\n");
                break;
            }
            if ((capture_player->type & MEDIA_PLAYER_TYPE_CAPTURE) == 0) {
                SYS_LOG_INF("Invalid dumpable player type! %d", capture_player->type);
                break;
            }

            if ((capture_player->format != NAV_TYPE) && (capture_player->format != PCM_DSP_TYPE)) {
                SYS_LOG_INF("Invalid dumpable player format! %d", capture_player->format);
                break;
            }

			if (capture_player->format == PCM_DSP_TYPE) {
                stream_type = AUDIO_STREAM_USOUND;
			} else {
				stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
			}

			dae_param = media_mem_get_cache_pool(DAE_BUFFER, stream_type);
            if(NULL == dae_param)
                break;

#if DUMP_LOG
            print_hex2("call dae:", cfg_data, cfg_size);
#endif
            media_effect_get_param(stream_type, AUDIO_STREAM_VOICE, NULL, &aec_param);
            memcpy(dae_param, cfg_data, cfg_size);
            medie_effect_set_user_param(stream_type, AUDIO_STREAM_VOICE, dae_param, aec_param);
            media_player_update_effect_param(capture_player, cfg_data, cfg_size);

            if(asqt_data) {
                asqt_data->has_effect_set = 1;
            }
        }
        break;

    case CFG_ID_BT_CALL_QUALITY:
        if (!capture_player) {
            SYS_LOG_INF("No dumpable player now!\n");
            break;
        }
        if ((capture_player->type & MEDIA_PLAYER_TYPE_CAPTURE) == 0) {
            SYS_LOG_INF("Invalid dumpable player type! %d", capture_player->type);
            break;
        }
        if(cfg_size == sizeof(CFG_Struct_BT_Call_Quality)) {
            CFG_Struct_BT_Call_Quality *quality = (CFG_Struct_BT_Call_Quality*)cfg_data;
            audio_input_gain input_gain;
            uint8_t stream_type;
            int32_t i;
            #if CONFIG_AUDIO_IN_ADGAIN_SUPPORT
            uint16_t *adc_gain = &quality->MIC_Gain.ADC0_aGain;

            for(i=0; i<AUDIO_ADC_NUM; i++) {
                input_gain.ch_again[i] = *adc_gain;
                adc_gain++;
                input_gain.ch_dgain[i] = *adc_gain;
                adc_gain++;
            }
            #else
            uint16_t *adc_gain = &quality->MIC_Gain.ADC0_Gain;

            for(i=0; i<AUDIO_ADC_NUM; i++) {
                input_gain.ch_gain[i] = *adc_gain;
                adc_gain++;
            }
            #endif
            if (capture_player->format == NAV_TYPE) {
                stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
            } else if (capture_player->format == PCM_DSP_TYPE) {
                stream_type = AUDIO_STREAM_USOUND;
			} else {
                stream_type = AUDIO_STREAM_VOICE;
            }
            system_volume_set(stream_type, quality->Test_Volume, false);
            audio_system_set_microphone_volume(stream_type, &input_gain);
        } else {
            SYS_LOG_ERR("size not match, id %d, cfg_size %d, expected %d", cfg_id, cfg_size, sizeof(CFG_Struct_BT_Call_Quality));
        }
        break;
    case CFG_ID_BT_CALL_QUALITY_AL:
        {
            uint8_t stream_type;
            void *aec_param = NULL;
            const void *dae_param = NULL;
            
            if (!capture_player) {
                SYS_LOG_INF("No dumpable player now!\n");
                break;
            }
            if ((capture_player->type & MEDIA_PLAYER_TYPE_CAPTURE) == 0) {
                SYS_LOG_INF("Invalid dumpable player type! %d", capture_player->type);
                break;
            }

            if (capture_player->format == NAV_TYPE) {
                stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
			} else if(capture_player->format == PCM_DSP_TYPE) {
                stream_type = AUDIO_STREAM_USOUND;
            } else {
                stream_type = AUDIO_STREAM_VOICE;
            }

            aec_param = media_mem_get_cache_pool(AEC_BUFFER, stream_type);
            if(NULL == aec_param)
                break;

#if DUMP_LOG
            print_hex2("call aec:", cfg_data, cfg_size);
#endif
            media_effect_get_param(stream_type, AUDIO_STREAM_VOICE, &dae_param, NULL);
            memcpy(aec_param, cfg_data, cfg_size);
            medie_effect_set_user_param(stream_type, AUDIO_STREAM_VOICE, dae_param, aec_param);
            
			if (stream_type == AUDIO_STREAM_USOUND) {
                _aset_restart_player_sync(AUDIO_STREAM_VOICE, true);
			} else {
                _restart_player_sync(AUDIO_STREAM_VOICE, true);
			}

            if(asqt_data) {
                asqt_data->has_effect_set = 1;
                audio_manager_asqt_start(asqt_data->asqt_mode, asqt_data->asqt_channels);
            }
        }
        break;

    case CFG_ID_ANC_AL:
#if CONFIG_ANC_HAL
#if DUMP_LOG
        print_hex2("anct:", cfg_data, cfg_size);
#endif   
        ret = anc_dsp_send_command(ANC_COMMAND_ANCTDATA, cfg_data, cfg_size);
        if(ret){
            anc_dsp_close();
            app_config_read(CFG_ID_AUDIO_SETTINGS, &audio_settings, 0, sizeof(audio_settings));
            anc_init_info.mode = ANC_MODE_NORMAL;
            anc_init_info.ffmic = audio_settings.anc_ff_mic;
            anc_init_info.fbmic = audio_settings.anc_fb_mic;
            anc_init_info.speak = audio_settings.ANC_Speaker_Out;
            anc_init_info.rate = audio_settings.ANC_Sample_rate;
            anc_init_info.aal = 0;
            anc_init_info.rate = ((anct_data_t *)cfg_data)->bRate;    
            anc_dsp_open(&anc_init_info);
            anc_dsp_send_command(ANC_COMMAND_ANCTDATA, cfg_data, cfg_size);
        }
#endif        
        break;
    }
}

/*!
 * \brief ???ASQT ????
 */
static void audio_manager_asqt_start(uint32_t asqt_mode, uint32_t asqt_channels)
{
    asqt_data_t *data = asqt_data;
    
    SYS_LOG_INF("%d,%x", asqt_mode, asqt_channels);
    _asqt_unprepare_data_upload(asqt_data);
    
    if(TOOL_DEV_TYPE_SPP == g_tool_data.dev_type) {
        return ;
    }

    data->asqt_mode = asqt_mode;
    data->asqt_channels = asqt_channels;
    data->asqt_tobe_start = 1;
    data->sample_rate = 0;
    _asqt_prepare_data_upload(data);
}

/*!
 * \brief ?? ASQT ????
 */
static void audio_manager_asqt_stop(void)
{
    SYS_LOG_INF("%d", __LINE__);
    _asqt_unprepare_data_upload(asqt_data);
}

#ifdef CONFIG_TOOL_ASET_USB_SUPPORT
static void _aset_cmd_deal_stub_stop(void)
{
    SYS_LOG_INF("tool stub back to init!");

    if((asqt_data != NULL) && (asqt_data->asqt_started != 0)) {
        audio_manager_asqt_stop();
    }

    g_tool_data.aset_protocol_connect_flag = 0;
    ExitASET(&g_tool_data.usp_handle);

    g_tool_data.aset_usb_status = ASET_USB_STATUS_INIT;
    g_tool_data.wait_stub_drv_ok_flag = 0;
}
#endif


#ifdef CONFIG_ANC_HAL
static int32_t _anc_prepare_data_upload(asqt_data_t *data)
{
    int i;
    char *buf;
    int bufsize = 0;

    data->sample_rate = ANC_PCM_SAMPLE_RATE;
    for(i=0; i<3; i++){
        if(data->anc_channels & 0x1<<i){
            data->frame_size += (ANC_PCM_FRAME_TIME)*(ANC_PCM_SAMPLE_RATE/1000)*(ANC_PCM_SAMPLE_WIDTH/8);
        }
    }

    buf = media_mem_get_cache_pool(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_MUSIC);
	bufsize = media_mem_get_cache_pool_size(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_MUSIC);
    data->dump_streams[0] = ringbuff_stream_create_ext(buf, bufsize);
    data->dump_bufs[0] = stream_get_ringbuffer(data->dump_streams[0]);

#if (ASQT_UPLOAD_LOOP_USE == ASQT_UPLOAD_USE_TIMER)
    os_timer_start(&data->upload_timer, K_MSEC(UPLOAD_TIMER_INTERVAL), K_MSEC(UPLOAD_TIMER_INTERVAL));
#else
    os_delayed_work_submit(&data->upload_work, UPLOAD_TIMER_INTERVAL);
#endif
    return anc_dsp_dump_data(1, (uint32_t)data->dump_bufs[0], (uint32_t)data->anc_channels);
}

static void _anc_unprepare_data_upload(asqt_data_t *data)
{
    if(!data->anc_started)
        return;

    SYS_LOG_INF("stop");
    data->anc_started = 0;
    data->empty_time = 0;
#if (ASQT_UPLOAD_LOOP_USE == ASQT_UPLOAD_USE_TIMER)
    os_timer_stop(&data->upload_timer);
#else
    os_delayed_work_cancel(&data->upload_work);
#endif

    anc_dsp_dump_data(0, 0, 0);

    if(data->dump_streams[0]){
        stream_close(data->dump_streams[0]);
        stream_destroy(data->dump_streams[0]);
        data->dump_streams[0] = NULL;
        data->dump_bufs[0] = NULL;
    }
}



static void anc_dsp_dump_data_start(uint32_t anc_mode, uint32_t anc_channels)
{
    asqt_data_t *data = asqt_data;

    SYS_LOG_INF("%x",anc_channels);
    _anc_unprepare_data_upload(asqt_data);

    if(TOOL_DEV_TYPE_SPP == g_tool_data.dev_type) {
        return ;
    }

    data->anc_channels = anc_channels;
    data->anc_started = 1;
    data->asqt_tobe_start = 1;
    data->sample_rate = 0;
    _anc_prepare_data_upload(data);
}

static void anc_dsp_dump_data_stop(void)
{
    SYS_LOG_INF("%d", __LINE__);
    _anc_unprepare_data_upload(asqt_data);
}

static void anc_dsp_play_pcm_store(int data_length, int sample_rate)
{
    int ret = 0;
    char *buf = NULL;
    int buf_size = 0;

    SYS_LOG_INF("data length %d, sample rate %d", data_length, sample_rate);

    buf = media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC);
    buf_size = media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC);
    if(buf == NULL){
        SYS_LOG_ERR("get mem fail");
        return;
    }

    if(data_length > buf_size){
        SYS_LOG_ERR("space not enougth,%d, %d", data_length, buf_size);
        return;
    }

    ret = ReadASETPacket(&g_tool_data.usp_handle, buf, data_length);
    if(ret != data_length){
        SYS_LOG_ERR("read data err");
        return;
    }


    play_pcm_flag = 1;
    anc_pcm_buf = buf;
    anc_pcm_len = data_length;
    anc_pcm_samplerate = sample_rate;
    anc_pcm_offset = 0;

    return;
}

static void anc_dsp_play_pcm_start(void)
{
    if(play_pcm_flag)
    {
        anc_audio_track = audio_track_create(AUDIO_STREAM_TTS, anc_pcm_samplerate / 1000,
                        ANC_PCM_SAMPLE_WIDTH, AUDIO_MODE_MONO, NULL, NULL, NULL);
        if(anc_audio_track){
            anc_audio_stream = audio_track_get_stream(anc_audio_track);
            audio_track_start(anc_audio_track);
            k_delayed_work_submit(&pcm_play_work, K_MSEC(5));
        }
        else{
            play_pcm_flag = 0;
            anc_audio_track = NULL;
            SYS_LOG_ERR("audio tranc create err");
        }
    }
}

static void anc_dsp_play_pcm_stop(void)
{
    
    play_pcm_flag = 0;
    /*wait delay work exit*/
    os_sleep(10);

    if(anc_audio_track){
        audio_track_stop(anc_audio_track);
        audio_track_destory(anc_audio_track);
    }

    anc_audio_track = NULL;
    anc_audio_stream = NULL;
    anc_pcm_offset = 0;
    return;
}

static void anc_dsp_play_pcm_play(void)
{
    int32_t write_bytes, free_bytes, offset;

    if(play_pcm_flag && anc_audio_track){
        free_bytes = stream_get_space(anc_audio_stream);
        offset = anc_pcm_offset;

        if((offset + free_bytes) >= anc_pcm_len){
            write_bytes = anc_pcm_len - offset;
            anc_pcm_offset = 0;
        }
        else{
            anc_pcm_offset += free_bytes;
            write_bytes = free_bytes;
        }

        audio_track_write(anc_audio_track, anc_pcm_buf+offset, write_bytes);
    }
}

static void _pcm_play_work_func(struct k_work *work)
{
    if(play_pcm_flag){
        anc_dsp_play_pcm_play();
        k_delayed_work_submit(&pcm_play_work, K_MSEC(5));
    }     
}

#if DEBUG
#include "shell/shell.h"
static os_timer dbg_timer;
static void _dbg_play_pcm_timer_proc(os_timer *ttimer)
{
    anc_dsp_play_pcm_play();
}

static int cmd_dbg_pcm_start(const struct shell *shell,
			      size_t argc, char **argv)
{
    SYS_LOG_INF("enter");
    play_pcm_flag = 1;
    anc_pcm_buf = media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC);;
    anc_pcm_len = 9600;
    anc_pcm_samplerate = 48000;
    anc_pcm_offset = 0;

    anc_audio_track = audio_track_create(AUDIO_STREAM_TTS, anc_pcm_samplerate / 1000,
            ANC_PCM_SAMPLE_WIDTH, AUDIO_MODE_MONO, NULL, NULL, NULL);
    if(anc_audio_track){
        anc_audio_stream = audio_track_get_stream(anc_audio_track);
        audio_track_start(anc_audio_track);
    }
    else{
        return 0;
    }

    os_timer_init(&dbg_timer, _dbg_play_pcm_timer_proc, NULL);
    os_timer_start(&dbg_timer, K_MSEC(UPLOAD_TIMER_INTERVAL), K_MSEC(UPLOAD_TIMER_INTERVAL));
    return 0;
}

static int cmd_dbg_pcm_stop(const struct shell *shell,
			      size_t argc, char **argv)
{
    os_timer_stop(&dbg_timer);
    anc_dsp_play_pcm_stop();
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dbg_anct,
	SHELL_CMD(start, NULL, "start play pcm", cmd_dbg_pcm_start),
	SHELL_CMD(stop, NULL, "stop play pcm", cmd_dbg_pcm_stop),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(dbg_anct, &sub_dbg_anct, "Actions anc commands", NULL);

#endif
#endif

int32_t tws_aset_cmd_deal(uint8_t *data_buf, int32_t len)
{
    if(asqt_data || (len != DATA_BUFFER_SIZE)) {
        return 0;
    }

    #ifdef CONFIG_DVFS_DYNAMIC_LEVEL
    if(!dvfs_setted) {
        dvfs_setted = 1;
    	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
    }
    #endif

    config_protocol_adjust(data_buf[0], data_buf[2] | ((uint32_t)data_buf[3] << 8), &data_buf[4]);
    return 0;
}

static int32_t _aset_cmd_deal(aset_cmd_packet_t *aset_cmd_pkt, uint8_t *para0, uint8_t *para1)
{
    uint32_t aset_cmd, size;
    uint8_t *data_buf;
    int ret;

    aset_cmd = aset_cmd_pkt->opcode;
    size = aset_cmd_pkt->para_length;
    SYS_LOG_INF("cmd: %x(%d)", aset_cmd, size);

    if (CONFIG_PROTOCOL_CMD_STUB_STOP != aset_cmd) {
        data_buf = asqt_data->data_buffer;
        ret = ReadASETPacket(&g_tool_data.usp_handle, data_buf, size);
        if (ret != size)
        {
            SYS_LOG_WRN("ASET rcv err: %d", ret);
            return -1;
        }
    }

    switch (aset_cmd)
    {
    case CONFIG_PROTOCOL_CMD_ADJUST_CFG:
        size = data_buf[2] | ((uint32_t)data_buf[3] << 8);
        if (ret < (size + 4)) {
            break;
        }
        
        //Sync to another earphone
        bt_manager_tws_send_message(TWS_LONG_MSG_EVENT, TWS_EVENT_ASET, data_buf, DATA_BUFFER_SIZE);
        config_protocol_adjust(data_buf[0], size, &data_buf[4]);
        break;

    case CONFIG_PROTOCOL_CMD_ASQT_START:
        audio_manager_asqt_start(data_buf[0], data_buf[1] | ((uint32_t)data_buf[2] << 8));
        break;

    case CONFIG_PROTOCOL_CMD_ASQT_STOP:
        audio_manager_asqt_stop();
        break;
#ifdef CONFIG_ANC_HAL
    case CONFIG_PROTOCOL_CMD_ANC_START:
        anc_dsp_play_pcm_start();
        anc_dsp_dump_data_start(data_buf[0], data_buf[1] | ((uint32_t)data_buf[2] << 8));
        break;

    case CONFIG_PROTOCOL_CMD_ANC_STOP:
        anc_dsp_dump_data_stop();
        anc_dsp_play_pcm_stop();
        break;

    case CONFIG_PROTOCOL_CMD_PCM_DATA:
        anc_dsp_play_pcm_store(*(uint32_t *)data_buf, *(uint32_t *)(data_buf+4));
        break;
#endif

    case CONFIG_PROTOCOL_CMD_STUB_STOP:
#ifdef CONFIG_TOOL_ASET_USB_SUPPORT
        _aset_cmd_deal_stub_stop();
#endif
		break;
		
    default:
        return -1;
    }

	return 0;
}

#ifdef CONFIG_BT_TRANSCEIVER
void tip_manager_lock(void);
void tip_manager_unlock(void);
#endif

void tool_aset_loop(void *p1, void *p2, void *p3)
{
    int32_t ret;

	SYS_LOG_INF("Enter");
	
#ifdef CONFIG_BT_TRANSCEIVER
	tip_manager_lock();
#endif
	
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
    if(!dvfs_setted) {
        dvfs_setted = 1;
    	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
    }
#endif

    if (TOOL_DEV_TYPE_UART == g_tool_data.dev_type) {
        tool_uart_init(1024);
    }

#ifdef CONFIG_BT_TRANSCEIVER
	asqt_data = media_mem_get_cache_pool(DECODER_GLOBAL_DATA, AUDIO_STREAM_TIP);
#else
    asqt_data = mem_malloc(sizeof(*asqt_data));
    if(NULL == asqt_data) {
        SYS_LOG_ERR("malloc fail\n");
        goto ERROUT;
    }
#endif
    memset(asqt_data, 0x0, sizeof(*asqt_data));

    g_tool_data.usp_handle.handle_hook = (void*)_aset_cmd_deal;
    InitASET(&g_tool_data.usp_handle);
	SetUSPProtocolMaxPayloadSize(&g_tool_data.usp_handle, 1024, 128);
#if (ASQT_UPLOAD_LOOP_USE == ASQT_UPLOAD_USE_TIMER)
    os_timer_init(&asqt_data->upload_timer, _asqt_upload_data_timer_proc, NULL);
#else
    os_delayed_work_init(&asqt_data->upload_work, _asqt_upload_data_timer_proc);
#endif

	do {
        // ret = ASET_Protocol_Rx_Fsm(&g_tool_data.usp_handle, asqt_data->data_buffer, sizeof(asqt_data->data_buffer));
        ret = ASET_Protocol_Rx_Fsm(&g_tool_data.usp_handle);
        if (0 == ret)
        {
            g_tool_data.connect = 1;
			// SYS_LOG_ERR("ASET error %d", ret);
			// break;
        }

        if(asqt_data->asqt_tobe_start && !asqt_data->asqt_started) {
            _asqt_prepare_data_upload(asqt_data);
        }
        if(asqt_data->empty_time > WAIT_DATA_TIMEOUT) {
            printk("wait data timeout\n");
            _asqt_unprepare_data_upload(asqt_data);
            //break;
        }

        thread_timer_handle_expired();
		// os_sleep(100);
	} while (!tool_is_quitting());

#ifndef CONFIG_BT_TRANSCEIVER
ERROUT:
#endif

    if(asqt_data) {
        if(asqt_data->has_effect_set) {
            media_player_t *player = NULL;
            if (asqt_data->asqt_mode == 0) {
                player = media_player_get_current_dumpable_capture_player();
            } else {
                player = media_player_get_current_dumpable_playback_player();
            }
            if (player) {
                uint8_t stream_type;
                if (player->format == NAV_TYPE) {
                    stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
				} else if (player->format == PCM_DSP_TYPE) {
                    stream_type = AUDIO_STREAM_USOUND;
                } else {
                    stream_type = AUDIO_STREAM_VOICE;
                }
                medie_effect_set_user_param(stream_type, AUDIO_STREAM_VOICE, NULL, NULL);
            }
        }
        
    	_asqt_unprepare_data_upload(asqt_data);
        mem_free(asqt_data);
        asqt_data = NULL;
    }

    if (TOOL_DEV_TYPE_SPP != g_tool_data.dev_type)
    {
        tool_uart_exit();
    }
    g_tool_data.quited = 1;
	
#ifdef CONFIG_BT_TRANSCEIVER
	tip_manager_lock();
#endif

	SYS_LOG_INF("Exit");
}


#ifdef CONFIG_TOOL_ASET_USB_SUPPORT

#define ASET_MAGIC_SIZE (12)
#define ASET_PROTOCOL_CONNECT_TRY_COUNT (200)


/**************************************************************
* Description: open aset protocol
* Input:  timeout ms
* Output: none
* Return: success/fail
* Note:
***************************************************************
**/
bool OpenASET(uint32_t timeout_ms)
{
    bool connected = FALSE;

    if (g_tool_data.aset_protocol_connect_flag) {
		SYS_LOG_INF("ASET protocol already ok!");
        return FALSE;
    }

    timeout_ms--;

    connected = WaitUSPConnect(&g_tool_data.usp_handle, timeout_ms);

    if (connected) {
        g_tool_data.aset_protocol_connect_flag = 1;
		return TRUE;
    } else {
        return FALSE;
    }
}


/**************************************************************
* Description: aset tool thread loop for stub device
* Input:  param
* Output: none
* Return: none
* Note:
***************************************************************
**/
void tool_stub_loop_usb(void *p1, void *p2, void *p3)
{
    int32_t ret;
	uint8_t aset_open_try = 0;

	char key_buf[32];
    int key_size = sizeof(sysrq_toolbuf);

	SYS_LOG_INF("Enter tool stub loop!");

	while(1) {
        //SYS_LOG_INF("Status: %d", g_tool_data.aset_usb_status);
	
		if(g_tool_data.aset_usb_status == ASET_USB_STATUS_INIT) {
			ret = g_tool_data.usp_handle.api.read((uint8_t *)key_buf, ASET_MAGIC_SIZE, 100);
			if(ret == ASET_MAGIC_SIZE) {
				//receive data
                if(0 == memcmp(key_buf, sysrq_toolbuf, key_size)) {
					SYS_LOG_INF("parse tool type:%s\n", &key_buf[key_size]);
					//write ack to tools
					g_tool_data.usp_handle.api.write((u8_t *)tool_ack, sizeof(tool_ack), 0);	
						
					if(0 == memcmp(&key_buf[key_size], "aset", 4)) {
						g_tool_data.type = USP_PROTOCOL_TYPE_ASET;
						g_tool_data.aset_usb_status = ASET_USB_STATUS_HANDSHAKE_OK;
					} else if (0 == memcmp(&key_buf[key_size], "adfu", 4)) {
						printk("reboot to adfu!\n");
        				k_busy_wait(200 * 1000);
        				sys_pm_reboot(REBOOT_TYPE_GOTO_ADFU);
    				} else {
						; //empty
    				}
                }
			}

			thread_timer_handle_expired();		
		}

		if (g_tool_data.aset_usb_status == ASET_USB_STATUS_HANDSHAKE_OK) {
			#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
    		if(!dvfs_setted) {
        		dvfs_setted = 1;
    			dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
    		}
			#endif

			if(asqt_data == NULL)
			{
#ifdef CONFIG_BT_TRANSCEIVER
	            asqt_data = media_mem_get_cache_pool(DECODER_GLOBAL_DATA, AUDIO_STREAM_TIP);
#else
				asqt_data = mem_malloc(sizeof(*asqt_data));
#endif
				if(NULL == asqt_data) {
					SYS_LOG_ERR("malloc fail\n");
					goto ERROUT_2;
				}
				memset(asqt_data, 0x0, sizeof(*asqt_data));
			}

            SYS_LOG_INF("init usp for aset!\n");
			g_tool_data.usp_handle.handle_hook = (void*)_aset_cmd_deal;
			InitASET(&g_tool_data.usp_handle);

			g_tool_data.aset_protocol_connect_flag = 0;
            while(aset_open_try < ASET_PROTOCOL_CONNECT_TRY_COUNT) {
				if(OpenASET(50)) {
					SYS_LOG_INF("open try: %d\n", aset_open_try);
					break;
				}
				aset_open_try++;
            }

			if(aset_open_try >= ASET_PROTOCOL_CONNECT_TRY_COUNT) {
        		SYS_LOG_ERR("aset protocol connect fail!\n");
        		goto ERROUT_2;				
			}

#if (ASQT_UPLOAD_LOOP_USE == ASQT_UPLOAD_USE_TIMER)
			os_timer_init(&asqt_data->upload_timer, _asqt_upload_data_timer_proc, NULL);
#else
            os_delayed_work_init(&asqt_data->upload_work, _asqt_upload_data_timer_proc);
#endif

			g_tool_data.aset_usb_status = ASET_USB_STATUS_RUNNING;
			SYS_LOG_INF("status running!!");
		}
	
		while((g_tool_data.aset_usb_status == ASET_USB_STATUS_RUNNING) && (tool_is_quitting()==0)) {
			//running loop
        	ret = ASET_Protocol_Rx_Fsm(&g_tool_data.usp_handle);
        	if (0 == ret) {
            	g_tool_data.connect = 1;
        	}
			
        	if(asqt_data->asqt_tobe_start && !asqt_data->asqt_started) {
            	_asqt_prepare_data_upload(asqt_data);
        	}

        	if(asqt_data->empty_time > WAIT_DATA_TIMEOUT) {
            	SYS_LOG_INF("wait data timeout\n");
            	_asqt_unprepare_data_upload(asqt_data);
            	asqt_data->empty_time = 0;
        	}

        	thread_timer_handle_expired();
		}

		if((g_tool_data.aset_usb_status == ASET_USB_STATUS_FINISH) || (tool_is_quitting()!=0)) {
			SYS_LOG_INF("aset usb finish!\n");
			break;
		}
	}
        
ERROUT_2:
    if(asqt_data) {
        if(asqt_data->has_effect_set) {
            media_player_t *player = NULL;
            if (asqt_data->asqt_mode == 0) {
                player = media_player_get_current_dumpable_capture_player();
            } else {
                player = media_player_get_current_dumpable_playback_player();
            }
            if (player) {
                uint8_t stream_type;
                if (player->format == NAV_TYPE) {
                    stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
                } else {
                    stream_type = AUDIO_STREAM_VOICE;
                }
                medie_effect_set_user_param(stream_type, AUDIO_STREAM_VOICE, NULL, NULL);
            }
        }
        
    	_asqt_unprepare_data_upload(asqt_data);
        mem_free(asqt_data);
        asqt_data = NULL;
    }

	if(g_tool_data.stub_read_buf != NULL) {
		mem_free(g_tool_data.stub_read_buf);
		g_tool_data.stub_read_buf = NULL;
	}

	if(g_tool_data.stub_write_buf != NULL) {
		mem_free(g_tool_data.stub_write_buf);
		g_tool_data.stub_write_buf = NULL;
	}	


	usb_stub_dev_exit();

    g_tool_data.quited = 1;

	SYS_LOG_INF("Exit!");
}

#endif

