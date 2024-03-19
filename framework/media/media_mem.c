/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file media mem interface
 */
#define SYS_LOG_DOMAIN "media"
#include <linker/section_tags.h>
#include <os_common_api.h>
#include <mem_manager.h>
#include <media_service.h>
#include <audio_system.h>
#include "media_mem.h"
#include <soc_dsp.h>


/*
32k share ram (bytes):
cpu&bt         : 0x2000
dsp&cpu        : 0x600
dsp&con        : 0x100
dsp drv&hal    : 0x900
media(next to) : 0x4BE0~0x5000
dump(max)      : 0x420~0x0
*/

struct media_memory_cell {
	uint8_t mem_type;
	uint32_t mem_base;
	uint32_t mem_size;
};

struct media_memory_block {
	uint8_t stream_type;
	struct media_memory_cell mem_cell[24];
};

typedef struct  {
    union {
        music_dae_para_t music_dae_para;
        hfp_dae_para_t hfp_dae_para;
    };
}dae_para_t;

#ifdef CONFIG_MEDIA

#ifndef CONFIG_SAMPLE_TEST_CODEC

//16bit: 0x800, 32bit: 0x1000
#define DSP_RAM_INNER_DECODER_OUTBUF          (DSP_RAM_INNER_FLAG + 0x31)  /* dr#1 decoder_outbuf*/
#define DSP_RAM_INNER_DECODER_OUTBUF_SIZE     (0x1000)

//dsp output directly, option
#define DSP_RAM_INNER_PLAYBACK_OUTBUF         (DSP_RAM_INNER_FLAG + 0x32)  /* dr#2 playback_outbuf */
#define DSP_RAM_INNER_PLAYBACK_OUTBUF_SIZE    (0x2000)

static char playback_input_buffer[0x3EF0]  	__aligned(8) __in_section_unique(DSP_SHARE_MEDIA_RAM);

//static char playback_output_buffer[0x1000]   __aligned(8) __in_section_unique(DSP_SHARE_MEDIA_RAM);  //16bit: 0x800, 32bit: 0x1000

//static char playback_output_pkghdr[0x100]   __in_section_unique(DSP_SHARE_MEDIA_RAM);

#ifndef CONFIG_PCM_BUFFER_STREAM
static char output_pcm[0x1000]  __aligned(4);
#endif


#if defined(CONFIG_TOOL_ASET)
static char aset_dump_buffer[1056]			__aligned(8) __in_section_unique(DSP_SHARE_MEDIA_RAM);
#endif

#ifdef CONFIG_TOOL_ECTT
static char ectt_tool_buf[3072]  __aligned(4);
#endif

#ifdef CONFIG_ACTIONS_DECODER
static char codec_stack[2048] __aligned(ARCH_STACK_PTR_ALIGN);
#endif
#ifdef CONFIG_ACTIONS_PARSER
static char parser_chuck_buffer[0x1000];
static char parser_stack[2048] __aligned(ARCH_STACK_PTR_ALIGN);
#endif

#ifdef CONFIG_PLAY_KEYTONE
static char resample_frame_bss[256]  __aligned(4);
#endif
#if defined(CONFIG_DECODER_ACT) && !defined(CONFIG_DECODER_ACT_HW_ACCELERATION)
static char actii_decode_buf[0x1928]  __aligned(4);
static char actii_stack[2048]  __aligned(4);
#endif
static char tip_decode_buf[512*4]  __aligned(4);
#ifdef CONFIG_PLAY_TIPTONE
//actii: 0x1928, pcm: 128
//static char tip_decode_buf[128]  __aligned(4);

#ifdef CONFIG_MIX_RESAMPLE_HW
//resample input:
//128 * 2 * 3 = 768, 16 -> 8 in 128 out  64
//80 * 2 * 3 = 480, 8 -> 16 in 80 out  160
static char tip_output_pcm[480]  __aligned(8) __in_section_unique(DSP_SHARE_MEDIA_RAM);
//max resample output:
//(441 + 256 - 1)*2=1392, 16(160 samples)->44.1(441 samples), AL frame size: 256 samples
//(441 + 256 - 1)*2=1392, 8(80 samples)->44.1(441 samples), AL frame size: 256 samples
#define DSP_RAM_INNER_TIP_RES_BUF             (DSP_RAM_INNER_FLAG + 0x33)  /* dr#3 tip_resample_buf */
#define DSP_RAM_INNER_TIP_RES_BUF_SIZE        (1392)
#else
static char tip_output_pcm[2048]  __aligned(8) __in_section_unique(DSP_SHARE_MEDIA_RAM);
//max ram use: 8->44.1 in: 80, out: 441, glen: 1106, slen: 1020
static char resample_frame_data[1048 + 1106]  __aligned(4);
//static char resample_global_data[1106]  __aligned(4);
static char resample_share_data[1020]  __aligned(4);
#endif //CONFIG_MIX_RESAMPLE_HW

#endif


#ifdef CONFIG_MEDIA_EFFECT
static dae_para_t dae_para  __aligned(8) __in_section_unique(DSP_SHARE_MEDIA_RAM);
#endif

#define DSP_RAM_INNER_INPUT_ENCBUF            (DSP_RAM_INNER_FLAG + 0x34)  /* dr#4 input_encbuf */
#define DSP_RAM_INNER_INPUT_ENCBUF_SIZE       (0x400)

static struct bt_dsp_cis_share_info cis_share_info_buf  __in_section_unique(DSP_BT_SHARE_RAM);

static const struct media_memory_block media_memory_config[] = {
	{
		.stream_type = AUDIO_STREAM_MUSIC,
		.mem_cell = {
            {.mem_type = OUTPUT_PKG_HDR,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x100,},
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0x100], .mem_size = sizeof(playback_input_buffer)-0x100,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)DSP_RAM_INNER_DECODER_OUTBUF, .mem_size = DSP_RAM_INNER_DECODER_OUTBUF_SIZE,},
            {.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)DSP_RAM_INNER_PLAYBACK_OUTBUF, .mem_size = DSP_RAM_INNER_PLAYBACK_OUTBUF_SIZE,},
        #ifndef CONFIG_PCM_BUFFER_STREAM
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = sizeof(output_pcm),},
        #endif

        #if defined(CONFIG_TOOL_ASET)
 			{.mem_type = TOOL_ASQT_DUMP_BUF, .mem_base = (uint32_t)&aset_dump_buffer[0], .mem_size = sizeof(aset_dump_buffer),},
		#endif
        #ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
            {.mem_type = DAE_BUFFER,  .mem_base = (uint32_t)&dae_para.music_dae_para.dae_para_info_array, .mem_size = sizeof(dae_para.music_dae_para.dae_para_info_array),},
        #endif
		},
	},

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	{
		.stream_type = AUDIO_STREAM_VOICE,
		.mem_cell = {
			//voice独占的内存区域：
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0x000], .mem_size = 0x4C8,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)&playback_input_buffer[0x4C8], .mem_size = 960,},
    		{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0x888], .mem_size = 960,},
            {.mem_type = OUTPUT_PKG_HDR,  .mem_base = (uint32_t)&playback_input_buffer[0xC48], .mem_size = 0x100,},
 
            {.mem_type = AI_PARAM,        .mem_base = (uint32_t)&playback_input_buffer[0x4C8], .mem_size = 0x600,}, //tmp used at startup
            {.mem_type = DMVP_PARAM, .mem_base = (uint32_t)&playback_input_buffer[0xAC8], .mem_size = 0x1174,}, //tmp used at startup  end 0x1BAC
  
			{.mem_type = INPUT_ENCBUF,   .mem_base = (uint32_t)&playback_input_buffer[0xD48], .mem_size = 960,},
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (uint32_t)&playback_input_buffer[0x1108], .mem_size = 0x15C,},

			{.mem_type = OUTPUT_SCO, .mem_base = (uint32_t)&playback_input_buffer[0x1264], .mem_size = 0xE8,},
			{.mem_type = TX_SCO,     .mem_base = (uint32_t)&playback_input_buffer[0x134C], .mem_size = 0x7C,},
			{.mem_type = RX_SCO,     .mem_base = (uint32_t)&playback_input_buffer[0x13C8], .mem_size = 0xF0,},

			{.mem_type = AEC_REFBUF0,    .mem_base = (uint32_t)&playback_input_buffer[0x14B8], .mem_size = 1440,},
            {.mem_type = INPUT_PCM,      .mem_base = (uint32_t)&playback_input_buffer[0x1A58], .mem_size = 1920,},//not support 24bits
			{.mem_type = INPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x1A58], .mem_size = 1920,},//not support 24bits
            {.mem_type = PLAY_FLAG_BUFFER,  .mem_base = (uint32_t)&playback_input_buffer[0x21D8], .mem_size = 0x4,},


            {.mem_type = NEW_DEBUG_DUMP_BUF, .mem_base = (uint32_t)&aset_dump_buffer[0], .mem_size = sizeof(aset_dump_buffer),},

			//仅在audio_track_dis时使用
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)DSP_RAM_INNER_INPUT_ENCBUF, .mem_size = 960,},

        #ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
            {.mem_type = DAE_BUFFER,  .mem_base = (uint32_t)&dae_para.hfp_dae_para.dae_para_info_array, .mem_size = sizeof(dae_para.hfp_dae_para.dae_para_info_array),},
            {.mem_type = AEC_BUFFER,  .mem_base = (uint32_t)&dae_para.hfp_dae_para.aec_para, .mem_size = sizeof(dae_para.hfp_dae_para.aec_para),},
        #endif

			//和leaudio共享的内存区域：
#if defined(CONFIG_TOOL_ASET)
	        {.mem_type = TOOL_ASQT_DUMP_BUF, .mem_base = (uint32_t)&playback_input_buffer[0x3150], .mem_size = 0x400,},
#endif
		},		
	},

	{
		.stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC,
		.mem_cell = {
			//和voice复用内存区域：
			//编码内存分配
			{.mem_type = INPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x0], .mem_size = 0xF00,},
			{.mem_type = INPUT_ENCBUF,   .mem_base = (uint32_t)DSP_RAM_INNER_INPUT_ENCBUF, .mem_size = 0x3C0,}, /*max support 7.5ms*32KHz*24bits*/
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (uint32_t)&playback_input_buffer[0xF00], .mem_size = 0x100,},
			
			//playback也会使用，但audio_track_dis==1时不使用
			{.mem_type = AEC_REFBUF0,    .mem_base = (uint32_t)&playback_input_buffer[0x1000], .mem_size = 0x780,},
			
			//capture使用的前处理部分
			{.mem_type = AI_PARAM, .mem_base = (uint32_t)&playback_input_buffer[0xF00], .mem_size = 0x600,}, //tmp used at startup
			{.mem_type = DMVP_PARAM, .mem_base = (uint32_t)&playback_input_buffer[0x1500], .mem_size = 0x1174,},////tmp used at startup 0x10E4  0xF74

        #ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PRE_PARAM,  .mem_base = (uint32_t)&playback_input_buffer[0x26F4], .mem_size = sizeof(dae_para),}, //0x380
        #endif

			//不复用的内存区域：
			//解码内存分配
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0x3150], .mem_size = 0x800,},	//和voice的ASQT复用，所以双模共存时，不能使用ASQT工具
			/*FIXME 后续如论如何解码输出都按照16bits保存*/
            {.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)DSP_RAM_INNER_PLAYBACK_OUTBUF, .mem_size = 0xF00,},
            {.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)DSP_RAM_INNER_DECODER_OUTBUF, .mem_size = 0xF00,},

			//编解码共用内存分配
			{.mem_type = CIS_SEND_REPORT_DATA, .mem_base = (uint32_t)&playback_input_buffer[0x3950], .mem_size = 0x80,},
			{.mem_type = CIS_XFER_REPORT_DATA, .mem_base = (uint32_t)&playback_input_buffer[0x39D0], .mem_size = 0x100,},
			{.mem_type = CIS_SHARE_INFO, .mem_base = (uint32_t)&cis_share_info_buf, .mem_size = sizeof(struct bt_dsp_cis_share_info),},

#ifdef CONFIG_MEDIA_EFFECT
			//解码使用的后处理部分，但audio_track_dis==1时不使用
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&playback_input_buffer[0x3AD0], .mem_size = sizeof(dae_para),},
			{.mem_type = DAE_BUFFER,  .mem_base = (uint32_t)&((dae_para_t*)(&playback_input_buffer[0x3AD0]))->hfp_dae_para.dae_para_info_array, .mem_size = sizeof(dae_para.hfp_dae_para.dae_para_info_array),},
            {.mem_type = AEC_BUFFER,  .mem_base = (uint32_t)&((dae_para_t*)(&playback_input_buffer[0x3AD0]))->hfp_dae_para.aec_para, .mem_size = sizeof(dae_para.hfp_dae_para.aec_para),},
#endif

			//{.mem_type = NEW_DEBUG_DUMP_BUF, .mem_base = (uint32_t)&playback_input_buffer[0x3AD0], .mem_size = 0x400,},
		},
	},
#else
	{
		.stream_type = AUDIO_STREAM_VOICE,
		.mem_cell = {
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0x000], .mem_size = 0x4C8,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)&playback_input_buffer[0x4C8], .mem_size = 0x3C0,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0x888], .mem_size = 0x3C0,},
            {.mem_type = OUTPUT_PKG_HDR,  .mem_base = (uint32_t)&playback_input_buffer[0xC48], .mem_size = 0x100,},

			{.mem_type = INPUT_ENCBUF,   .mem_base = (uint32_t)&playback_input_buffer[0xD48], .mem_size = 0x3C0,}, //__aligned(8)
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (uint32_t)&playback_input_buffer[0x1108], .mem_size = 0x15C,}, //__aligned(8)

			{.mem_type = OUTPUT_SCO, .mem_base = (uint32_t)&playback_input_buffer[0x1268], .mem_size = 0xE8,},
			{.mem_type = TX_SCO,     .mem_base = (uint32_t)&playback_input_buffer[0x1350], .mem_size = 0x7C,},
			{.mem_type = RX_SCO,     .mem_base = (uint32_t)&playback_input_buffer[0x13D0], .mem_size = 0xF0,},

			{.mem_type = AEC_REFBUF0,    .mem_base = (uint32_t)&playback_input_buffer[0x14C0], .mem_size = 0x5A0,},

            {.mem_type = INPUT_PCM,      .mem_base = (uint32_t)&playback_input_buffer[0x1A60], .mem_size = 0xF00,},//support 24bits
			{.mem_type = INPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x1A60], .mem_size = 0xF00,},//support 24bits

#if defined(CONFIG_TOOL_ASET)
            {.mem_type = TOOL_ASQT_DUMP_BUF, .mem_base = (uint32_t)&playback_input_buffer[0x2960], .mem_size = 0x400,},
#endif
            //{.mem_type = INPUT_RELOAD_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x24A8], .mem_size = 0x200,},
            {.mem_type = PLAY_FLAG_BUFFER,  .mem_base = (uint32_t)&playback_input_buffer[0x2E60], .mem_size = 0x4,},

            {.mem_type = AI_PARAM,        .mem_base = (uint32_t)&playback_input_buffer[0x4C8], .mem_size = 0x800,}, //tmp used at startup
            //size: 0x10E4
            {.mem_type = DMVP_PARAM,     .mem_base = (uint32_t)&playback_input_buffer[0xCC8], .mem_size = 0x1174,}, //tmp used at startup

            {.mem_type = NEW_DEBUG_DUMP_BUF, .mem_base = (uint32_t)&aset_dump_buffer[0], .mem_size = sizeof(aset_dump_buffer),},

        #ifndef CONFIG_PCM_BUFFER_STREAM
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = sizeof(output_pcm),},
        #endif

        #ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
            {.mem_type = DAE_BUFFER,  .mem_base = (uint32_t)&dae_para.hfp_dae_para.dae_para_info_array, .mem_size = sizeof(dae_para.hfp_dae_para.dae_para_info_array),},
            {.mem_type = AEC_BUFFER,  .mem_base = (uint32_t)&dae_para.hfp_dae_para.aec_para, .mem_size = sizeof(dae_para.hfp_dae_para.aec_para),},
        #endif
		},
	},

	{
		.stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC,
		.mem_cell = {
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x800,},
			/*FIXME 后续如论如何解码输出都按照16bits保存*/
            {.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)DSP_RAM_INNER_DECODER_OUTBUF, .mem_size = 0xF00,},
            {.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)DSP_RAM_INNER_PLAYBACK_OUTBUF, .mem_size = 0xF00,},
        #ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PRE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
			{.mem_type = DAE_BUFFER,  .mem_base = (uint32_t)&dae_para.hfp_dae_para.dae_para_info_array, .mem_size = sizeof(dae_para.hfp_dae_para.dae_para_info_array),},
            {.mem_type = AEC_BUFFER,  .mem_base = (uint32_t)&dae_para.hfp_dae_para.aec_para, .mem_size = sizeof(dae_para.hfp_dae_para.aec_para),},
        #endif

			{.mem_type = INPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x800], .mem_size = 0xF00,},
			{.mem_type = AEC_REFBUF0,    .mem_base = (uint32_t)&playback_input_buffer[0x1700], .mem_size = 0x780,},
			{.mem_type = INPUT_ENCBUF,   .mem_base = (uint32_t)DSP_RAM_INNER_INPUT_ENCBUF, .mem_size = 0x3C0,}, /*max support 7.5ms*32KHz*24bits*/
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (uint32_t)&playback_input_buffer[0x1E80], .mem_size = 0x100,},
			{.mem_type = CIS_SHARE_INFO, .mem_base = (uint32_t)&cis_share_info_buf, .mem_size = sizeof(struct bt_dsp_cis_share_info),},
			{.mem_type = CIS_XFER_REPORT_DATA, .mem_base = (uint32_t)&playback_input_buffer[0x1F80], .mem_size = 0x100,},
			{.mem_type = CIS_SEND_REPORT_DATA, .mem_base = (uint32_t)&playback_input_buffer[0x2080], .mem_size = 0x80,},

			{.mem_type = AI_PARAM, .mem_base = (uint32_t)&playback_input_buffer[0x1700], .mem_size = 0x800,}, //tmp used at startup
		#ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&playback_input_buffer[0x2100], .mem_size = sizeof(dae_para_t),}, //0x500
		#endif
			{.mem_type = DMVP_PARAM, .mem_base = (uint32_t)&playback_input_buffer[0x2100 + sizeof(dae_para_t)], .mem_size = 0x1174,}, //tmp used at startup

			//{.mem_type = NEW_DEBUG_DUMP_BUF, .mem_base = (uint32_t)&aset_dump_buffer[0], .mem_size = sizeof(aset_dump_buffer),},
			{.mem_type = NEW_DEBUG_DUMP_BUF, .mem_base = (uint32_t)&playback_input_buffer[0x3600], .mem_size = 0x600,},
		},
	},
#endif


	{ /*FIXME*/
		.stream_type = AUDIO_STREAM_LE_AUDIO,
		.mem_cell = {
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x600,},
            {.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)DSP_RAM_INNER_DECODER_OUTBUF, .mem_size = 0xF00,},
            {.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0x600], .mem_size = 0xF00,},
        #ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
        #endif

			{.mem_type = AEC_REFBUF0,    .mem_base = (uint32_t)&playback_input_buffer[0x1500], .mem_size = 0x400,},
			{.mem_type = INPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x1900], .mem_size = 0x780,},
			{.mem_type = INPUT_ENCBUF,   .mem_base = (uint32_t)&playback_input_buffer[0x2080], .mem_size = 0xA00,},
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (uint32_t)&playback_input_buffer[0x2A80], .mem_size = 0x100,},
			{.mem_type = CIS_SHARE_INFO, .mem_base = (uint32_t)&cis_share_info_buf, .mem_size = sizeof(struct bt_dsp_cis_share_info),},
			{.mem_type = CIS_XFER_REPORT_DATA, .mem_base = (uint32_t)&playback_input_buffer[0x2B80], .mem_size = 0xC0,},
			{.mem_type = CIS_SEND_REPORT_DATA, .mem_base = (uint32_t)&playback_input_buffer[0x2C40], .mem_size = 0x80,},
		},
	},

	{
		.stream_type = AUDIO_STREAM_LOCAL_MUSIC,
		.mem_cell = {
			//{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x1400,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)DSP_RAM_INNER_DECODER_OUTBUF, .mem_size = DSP_RAM_INNER_DECODER_OUTBUF_SIZE,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x1000,},
        #ifndef CONFIG_PCM_BUFFER_STREAM
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = sizeof(output_pcm),},
        #endif
        #ifdef CONFIG_ACTIONS_PARSER
			{.mem_type = PARSER_CHUCK,    .mem_base = (uint32_t)&parser_chuck_buffer[0], .mem_size = sizeof(parser_chuck_buffer),},
			{.mem_type = PARSER_STACK, 	  .mem_base = (uint32_t)&parser_stack[0], .mem_size = sizeof(parser_stack),},
        #endif
			{.mem_type = OUTPUT_PARSER,   .mem_base = (uint32_t)&playback_input_buffer[0x1000], .mem_size = 0x2000,},
			{.mem_type = BT_TRANSMIT_INPUT,    .mem_base = (uint32_t)&playback_input_buffer[0x3000], .mem_size = 0x800,},
			{.mem_type = BT_TRANSMIT_OUTPUT,   .mem_base = (uint32_t)&playback_input_buffer[0x3800], .mem_size = 0x600,},
        #ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
        #endif

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (uint32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif

		#ifdef CONFIG_TOOL_ECTT
			{.mem_type = TOOL_ECTT_BUF, .mem_base = (uint32_t)&ectt_tool_buf[0], .mem_size = sizeof(ectt_tool_buf),},
		#endif

		},
	},
	{
		.stream_type = AUDIO_STREAM_LINEIN,
		.mem_cell = {
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x800,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)DSP_RAM_INNER_DECODER_OUTBUF, .mem_size = DSP_RAM_INNER_DECODER_OUTBUF_SIZE,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0x800], .mem_size = 0x400,},

			{.mem_type = INPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0xC00], .mem_size = 0x1000,},
			{.mem_type = INPUT_PCM,       .mem_base = (uint32_t)&playback_input_buffer[0x1C00], .mem_size = 0x800,},
			{.mem_type = INPUT_ENCBUF,    .mem_base = (uint32_t)&playback_input_buffer[0x2400], .mem_size = 0x400,},
			{.mem_type = OUTPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x2800], .mem_size = 0x800,},

            {.mem_type = INPUT_RELOAD_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x3000], .mem_size = 0x400,},

        #ifndef CONFIG_PCM_BUFFER_STREAM
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&playback_input_buffer[0x3400], .mem_size = 0x800,},
        #endif
			{.mem_type = NEW_DEBUG_DUMP_BUF, .mem_base = (uint32_t)&aset_dump_buffer[0], .mem_size = sizeof(aset_dump_buffer),},

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (uint32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif
		},
	},
	{
		.stream_type = AUDIO_STREAM_ASR,
		.mem_cell = {
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x800,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)&playback_input_buffer[0x800], .mem_size = 0x800,},
            {.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0x1000], .mem_size = 0x1000,},
            {.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&playback_input_buffer[0x1000], .mem_size = 0x1000,}, //same to OUTPUT_PLAYBACK
			{.mem_type = INPUT_CAPTURE,   .mem_base = (uint32_t)&playback_input_buffer[0x2000], .mem_size = 0x800,},
            {.mem_type = INPUT_PCM,       .mem_base = (uint32_t)&playback_input_buffer[0x2000], .mem_size = 0x800,}, //same to INPUT_CAPTURE
			{.mem_type = INPUT_ENCBUF,    .mem_base = (uint32_t)&playback_input_buffer[0x2800], .mem_size = 0x400,},
			{.mem_type = OUTPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x2C00], .mem_size = 0x400,},
		},
	},

#ifdef CONFIG_DECODER_ACT_HW_ACCELERATION
	{
		.stream_type = AUDIO_STREAM_TTS,
		.mem_cell = {
	    #ifndef CONFIG_PCM_BUFFER_STREAM
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = sizeof(output_pcm),},
        #endif
            {.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x800,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)&playback_input_buffer[0x800], .mem_size = 0x400,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0xc00], .mem_size = 0x800,},
		},
	},
#else
	{
		.stream_type = AUDIO_STREAM_TTS,
		.mem_cell = {
	    #ifndef CONFIG_PCM_BUFFER_STREAM
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = sizeof(output_pcm),},
        #endif
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x100,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0x100], .mem_size = 0x100,},
        #ifdef CONFIG_MEDIA_DECODER_RESTRANSMIT_TWS
			{.mem_type = TWS_LOCAL_INPUT, .mem_base = (uint32_t)&playback_input_buffer[0x200], .mem_size = 0x1400,},
        #endif
        #ifdef CONFIG_DECODER_ACT
            {.mem_type = DECODER_GLOBAL_DATA, .mem_base = (uint32_t)&actii_decode_buf[0], .mem_size = 0x1928,},
        #endif

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (uint32_t)&actii_stack[0], .mem_size = sizeof(actii_stack),},
		#endif

		#ifdef CONFIG_TOOL_ECTT
			{.mem_type = TOOL_ECTT_BUF, .mem_base = (uint32_t)&ectt_tool_buf[0], .mem_size = sizeof(ectt_tool_buf),},
		#endif

        #ifdef CONFIG_PLAY_TIPTONE
            {.mem_type = RESAMPLE_FRAME_DATA, .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x400,},
        #elif defined(CONFIG_PLAY_KEYTONE)
			{.mem_type = RESAMPLE_FRAME_DATA, .mem_base = (uint32_t)resample_frame_bss, .mem_size = sizeof(resample_frame_bss),},
        #endif
		},
	},
#endif

#ifdef CONFIG_PLAY_TIPTONE
    {
		.stream_type = AUDIO_STREAM_TIP,
		.mem_cell = {
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&tip_output_pcm[0], .mem_size = sizeof(tip_output_pcm),},
            {.mem_type = DECODER_GLOBAL_DATA, .mem_base = (uint32_t)&tip_decode_buf[0], .mem_size = sizeof(tip_decode_buf),},
        #ifdef CONFIG_MIX_RESAMPLE_HW
            {.mem_type = OUTPUT_RESAMPLE, .mem_base = (uint32_t)DSP_RAM_INNER_TIP_RES_BUF, .mem_size = DSP_RAM_INNER_TIP_RES_BUF_SIZE,},
        #else
            {.mem_type = RESAMPLE_FRAME_DATA,      .mem_base = (uint32_t)resample_frame_data, .mem_size = sizeof(resample_frame_data),},
            //{.mem_type = RESAMPLE_GLOBAL_DATA, .mem_base = (uint32_t)resample_global_data, .mem_size = sizeof(resample_global_data),},
            {.mem_type = RESAMPLE_GLOBAL_DATA, .mem_base = (uint32_t)&resample_frame_data[1048], .mem_size = 1106,},
            {.mem_type = RESAMPLE_SHARE_DATA, .mem_base = (uint32_t)resample_share_data, .mem_size = sizeof(resample_share_data),},
        #endif //CONFIG_MIX_RESAMPLE_HW

		#ifdef CONFIG_ACTIONS_DECODER
			{.mem_type = CODEC_STACK, .mem_base = (uint32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif
		},
	},
#endif

    {
		.stream_type = AUDIO_STREAM_TR_USOUND,
		.mem_cell = {
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x200,},
			/*FIXME 后续如论如何解码输出都按照16bits保存*/
            {.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0x200], .mem_size = 0x800,},
            {.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)DSP_RAM_INNER_DECODER_OUTBUF, .mem_size = 0xF00,},
        #ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
			{.mem_type = DAE_BUFFER,  .mem_base = (uint32_t)&dae_para.hfp_dae_para.dae_para_info_array, .mem_size = sizeof(dae_para.hfp_dae_para.dae_para_info_array),},
            {.mem_type = AEC_BUFFER,  .mem_base = (uint32_t)&dae_para.hfp_dae_para.aec_para, .mem_size = sizeof(dae_para.hfp_dae_para.aec_para),},
        #endif

			{.mem_type = INPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x800 +0x200], .mem_size = 0x1200,},
			{.mem_type = USB_UPLOAD_CACHE, .mem_base = (uint32_t)&tip_decode_buf[0], .mem_size = sizeof(tip_decode_buf),},
			/* reserved playback_input_buffer[0x1700] 0x780 */
			{.mem_type = INPUT_ENCBUF,   .mem_base = (uint32_t)DSP_RAM_INNER_INPUT_ENCBUF, .mem_size = 0x3C0,}, /*max support 7.5ms*32KHz*24bits*/
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (uint32_t)&playback_input_buffer[0x2240 + 0x300], .mem_size = 0x100,},
			{.mem_type = CIS_SHARE_INFO, .mem_base = (uint32_t)&cis_share_info_buf, .mem_size = sizeof(struct bt_dsp_cis_share_info),},
			{.mem_type = CIS_XFER_REPORT_DATA, .mem_base = (uint32_t)&playback_input_buffer[0x2340 + 0x300], .mem_size = 0x100,},

			{.mem_type = AI_PARAM, .mem_base = (uint32_t)&playback_input_buffer[0x1700 + 0x300], .mem_size = 0x800,}, //tmp used at startup
			{.mem_type = DAE_PRE_PARAM,  .mem_base = (uint32_t)&playback_input_buffer[0x2440 + 0x300], .mem_size = sizeof(dae_para),},
			{.mem_type = DMVP_PARAM, .mem_base = (uint32_t)&playback_input_buffer[0x2800 + 0x300], .mem_size = 0xF74,},
			{.mem_type = AEC_REFBUF0, .mem_base = (uint32_t)&playback_input_buffer[0x3774 + 0x300], .mem_size = 0x3C0,},

			{.mem_type = CIS_SEND_REPORT_DATA, .mem_base = (uint32_t)&playback_input_buffer[0x3B34 + 0x300], .mem_size = 0x80,},
		},
	},

    {
        .stream_type = AUDIO_STREAM_USOUND,
        .mem_cell = {


			{.mem_type = INPUT_ENCBUF,   .mem_base = (uint32_t)DSP_RAM_INNER_INPUT_ENCBUF, .mem_size = 0x700,}, /*max support 7.5ms*32KHz*24bits*/
#if (CONFIG_NRF_CHANNEL_NUM == 2)
            {.mem_type = INPUT_PLAYBACK,        .mem_base = (uint32_t)&playback_input_buffer[0],        .mem_size = 0x900,  },
            {.mem_type = OUTPUT_DECODER,        .mem_base = (uint32_t)DSP_RAM_INNER_DECODER_OUTBUF,     .mem_size = 0xF00,},
            {.mem_type = OUTPUT_PLAYBACK,       .mem_base = (uint32_t)&playback_input_buffer[0x900],    .mem_size = 0xD00,  },

            {.mem_type = INPUT_CAPTURE,         .mem_base = (uint32_t)&playback_input_buffer[0x1600],   .mem_size = 0xA00,  }, 
            {.mem_type = INPUT_RELOAD_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x3000],   .mem_size = 0x200,  },
            {.mem_type = OUTPUT_CAPTURE,        .mem_base = (uint32_t)&playback_input_buffer[0x2000],   .mem_size = 0x800,  },
			{.mem_type = AEC_REFBUF0,    .mem_base = (uint32_t)&playback_input_buffer[0x2800], .mem_size = 0x780,},

			{.mem_type = DMVP_PARAM, .mem_base = (uint32_t)&playback_input_buffer[0x1800], .mem_size = 0x1174,},////tmp used at startup 0x10E4  0xF74
#else
            {.mem_type = INPUT_PLAYBACK,        .mem_base = (uint32_t)&playback_input_buffer[0],        .mem_size = 0xC00,  },
            {.mem_type = OUTPUT_DECODER,        .mem_base = (uint32_t)DSP_RAM_INNER_DECODER_OUTBUF,     .mem_size = 0xF00,},
            {.mem_type = OUTPUT_PLAYBACK,       .mem_base = (uint32_t)&playback_input_buffer[0xC00],    .mem_size = 0xC00,  },

            {.mem_type = INPUT_CAPTURE,         .mem_base = (uint32_t)&playback_input_buffer[0x1800],   .mem_size = 0x500,  }, 
            {.mem_type = OUTPUT_CAPTURE,        .mem_base = (uint32_t)&playback_input_buffer[0x1D00],   .mem_size = 0x800,  },
			{.mem_type = AEC_REFBUF0,    .mem_base = (uint32_t)&playback_input_buffer[0x2500], .mem_size = 0x780,},

            {.mem_type = INPUT_RELOAD_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x2D00],   .mem_size = 0x400,  },
#endif

#ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PRE_PARAM,  .mem_base = (uint32_t)&playback_input_buffer[0x3200], .mem_size = sizeof(dae_para),}, //0x380
#endif

			//capture使用的前处理部分
			{.mem_type = AI_PARAM, .mem_base = (uint32_t)&playback_input_buffer[0x1200], .mem_size = 0x600,}, //tmp used at startup
#ifndef CONFIG_PCM_BUFFER_STREAM
            {.mem_type = OUTPUT_PCM,            .mem_base = (uint32_t)&playback_input_buffer[0x3600],   .mem_size = 0x800,  },
#endif

#ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
            {.mem_type = DAE_BUFFER,  .mem_base = (uint32_t)&dae_para.music_dae_para.dae_para_info_array, .mem_size = sizeof(dae_para.music_dae_para.dae_para_info_array),},
            {.mem_type = AEC_BUFFER,  .mem_base = (uint32_t)&dae_para.hfp_dae_para.aec_para, .mem_size = sizeof(dae_para.hfp_dae_para.aec_para),},
#endif


            {.mem_type = NEW_DEBUG_DUMP_BUF,    .mem_base = (uint32_t)&aset_dump_buffer[0],             .mem_size = sizeof(aset_dump_buffer),},

#ifdef CONFIG_ACTIONS_DECODER
            {.mem_type = CODEC_STACK,           .mem_base = (uint32_t)&codec_stack[0],                  .mem_size = sizeof(codec_stack),},
#endif
        },
    },
};

static const struct media_memory_block *_memdia_mem_find_memory_block(int stream_type)
{
	const struct media_memory_block *mem_block = NULL;
	if (stream_type == AUDIO_STREAM_FM
		|| stream_type == AUDIO_STREAM_I2SRX_IN
		|| stream_type == AUDIO_STREAM_SPDIF_IN
		|| stream_type == AUDIO_STREAM_MIC_IN) {
		stream_type = AUDIO_STREAM_LINEIN;
	}

	for (int i = 0; i < ARRAY_SIZE(media_memory_config) ; i++) {
		mem_block = &media_memory_config[i];
		if (mem_block->stream_type == stream_type) {
			return mem_block;
		}
	}

	return NULL;
}

#else

extern const struct media_memory_block *_memdia_mem_find_memory_block(int stream_type);

#endif /* CONFIG_SAMPLE_TEST_CODEC */

static const struct media_memory_cell *_memdia_mem_find_memory_cell(const struct media_memory_block *mem_block, int mem_type)
{
	const struct media_memory_cell *mem_cell = NULL;

	for (int i = 0; i < ARRAY_SIZE(mem_block->mem_cell) ; i++) {
		mem_cell = &mem_block->mem_cell[i];
		if (mem_cell->mem_type == mem_type) {
			return mem_cell;
		}
	}

	return NULL;
}

void *media_mem_get_cache_pool(int mem_type, int stream_type)
{
	const struct media_memory_block *mem_block = NULL;
	const struct media_memory_cell *mem_cell = NULL;
	void *addr = NULL;

	mem_block = _memdia_mem_find_memory_block(stream_type);

	if (!mem_block) {
		goto exit;
	}

	mem_cell = _memdia_mem_find_memory_cell(mem_block, mem_type);

	if (!mem_cell) {
		goto exit;
	}

	return (void *)mem_cell->mem_base;

exit:
	return addr;
}

int media_mem_get_cache_pool_size(int mem_type, int stream_type)
{
	const struct media_memory_block *mem_block = NULL;
	const struct media_memory_cell *mem_cell = NULL;
	int mem_size = 0;

	mem_block = _memdia_mem_find_memory_block(stream_type);

	if (!mem_block) {
		goto exit;
	}

	mem_cell = _memdia_mem_find_memory_cell(mem_block, mem_type);

	if (!mem_cell) {
		goto exit;
	}

	return mem_cell->mem_size;

exit:
	return mem_size;
}

int media_mem_get_cache_pool_size_ext(int mem_type, int stream_type, int format, int block_size, int block_num)
{
	int max_size = media_mem_get_cache_pool_size(mem_type, stream_type);
	int need_size = max_size;

	if (format == NAV_TYPE) {
		need_size = (block_size + 4) * block_num;
		if (need_size > max_size) {
			printk("need_size:%d > max_size:%d\n", need_size, max_size);
			return 0;
		}
	}

	return need_size;
}

static char __aligned(4) media_mem_buffer[512] __in_section_unique(media_mem_pool);
STRUCT_SECTION_ITERABLE(k_heap, media_mem_pool) = {
	.heap = {
		.init_mem = media_mem_buffer,
		.init_bytes = sizeof(media_mem_buffer),
	},
};

void *media_mem_malloc(int size, int memory_type)
{
	return k_heap_alloc(&media_mem_pool, size, K_NO_WAIT);
}

void media_mem_free(void *ptr)
{
	if (ptr != NULL) {
		k_heap_free(&media_mem_pool, ptr);
	}
}
#else
void *media_mem_get_cache_pool(int mem_type, int stream_type)
{
	return NULL;
}
int media_mem_get_cache_pool_size(int mem_type, int stream_type)
{
	return 0;
}

void *media_mem_malloc(int size, int memory_type)
{
	return NULL;
}

void media_mem_free(void *ptr)
{

}
#endif
