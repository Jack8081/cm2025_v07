/*
 * dsp_hal_defs.h
 *
 *  Created on: 2019年5月23日
 *      Author: wuyufan
 */

#ifndef _DSP_HAL_DEFS_H_
#define _DSP_HAL_DEFS_H_

#include <stdint.h>
#include <drivers/dsp.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RINGBUF_TYPE uint32_t

/*****************************************************************
 * DSP compilation time options. You can change these here by *
 * uncommenting the lines, or on the compiler command line.      *
 *****************************************************************/
/* Enable DSP music effect*/
//#define DSP_SUPPORT_MUSIC_EFFECT

/* Enable DSP voice effect */
#define DSP_SUPPORT_VOICE_EFFECT

/* Enable DSP ASRC  effect */
//#define DSP_SUPPORT_ASR_EFFECT

/* Enable DSP decoder support resample*/
//#define DSP_SUPPORT_DECODER_RESAMPLE

//Enable DSP print
#define DSP_SUPPORT_DEBUG_PRINT

/* config DSP mix multi buffer num */
#define DSP_SUPPORT_MULTI_BUFFER_NUM    (2)

/* config DSP AEC refs buf num */
#define DSP_SUPPORT_AEC_REFBUF_NUM  (1)

#define DSP_AEC_REF_DELAY (1)

#ifdef DSP_SUPPORT_MUSIC_EFFECT

/* Enable DSP support player */
#define DSP_SUPPORT_PLAYER

/* Enable DSP support decoder */
#define DSP_SUPPORT_DECODER

/* Enable DSP decoder support DAE effect */
#define DSP_SUPPORT_DECODER_DAE

/* Enable DSP support sbc decoder */
//#define DSP_SUPPORT_SBC_DECODER

/* Enable DSP support aac decoder */
#define DSP_SUPPORT_AAC_DECODER

/* Enable DSP support output subwoofer */
#define DSP_SUPPORT_OUTPUT_SUBWOOFER

#endif

#ifdef DSP_SUPPORT_VOICE_EFFECT

/* Enable DSP support player */
#define DSP_SUPPORT_PLAYER

/* Enable DSP support recorder */
#define DSP_SUPPORT_RECODER

/* Enable DSP support decoder*/
#define DSP_SUPPORT_DECODER

/* Enable DSP support encoder */
#define DSP_SUPPORT_ENCODER

/* Enable DSP support AGC*/
#define DSP_SUPPORT_AGC

/* Enable DSP support PEQ*/
#define DSP_SUPPORT_PEQ

/* Enable DSP support decoder AGC */
#define DSP_SUPPORT_DECODER_AGC

/* Enable DSP support decoder PEQ */
#define DSP_SUPPORT_DECODER_PEQ

/* Enable DSP support encoder AGC */
#define DSP_SUPPORT_ENCODER_AGC

/* Enable DSP support encoder PEQ */
#define DSP_SUPPORT_ENCODER_PEQ

/* Enable DSP support msbc decoder */
#define DSP_SUPPORT_MSBC_DECODER

/* Enable DSP support msbc encoder */
#define DSP_SUPPORT_MSBC_ENCODER

/* Enable DSP support cvsd decoder */
//#define DSP_SUPPORT_CVSD_DECODER

/* Enable DSP support cvsd encoder */
//#define DSP_SUPPORT_CVSD_ENCODER

/* Enable DSP encoder support AEC */
#define DSP_SUPPORT_ENCODER_AEC

/* Enable DSP AEC SYNC with encoder */
//#define DSP_SUPPORT_PLAYER_AEC_SYNC

/* Enable DSP hardware AEC SYNC */
#define DSP_SUPPORT_HARDWARE_AEC_SYNC

/* Enable DSP encoder support CNG */
#define DSP_SUPPORT_ENCODER_CNG

#endif

#if !defined(DSP_SUPPORT_MUSIC_EFFECT) && !defined(DSP_SUPPORT_VOICE_EFFECT)
/* Enable DSP support recorder */
#define DSP_SUPPORT_RECODER

/* Enable DSP support encoder */
#define DSP_SUPPORT_ENCODER

/* Enable DSP support aac decoder */
#define DSP_SUPPORT_OPUS_ENCODER
#endif

#ifdef DSP_SUPPORT_DEBUG_PRINT
#define DSP_PUTS(a)		dsp_puts(a)
#define DSP_PRINTHEX(a) dsp_printhex(a)
#else
#define DSP_PUTS(a)
#define DSP_PRINTHEX(a)
#endif

/* session ids */
enum {
	DSP_SESSION_DEFAULT = 0,
	DSP_SESSION_MEDIA,

	DSP_NUM_SESSIONS,
};

/* session function ids */
enum {
	/* basic functions */
	DSP_FUNCTION_DECODER = 0,
	DSP_FUNCTION_ENCODER,

	/* complex functions */
	DSP_FUNCTION_PLAYER,
	DSP_FUNCTION_RECORDER,

	DSP_FUNCTION_PREPROCESS,
	DSP_FUNCTION_POSTPROCESS,

	DSP_FUNCTION_VOICE_DECODER,
	DSP_FUNCTION_VOICE_POSTPROCESS,
	
	DSP_NUM_FUNCTIONS,
};

#define DSP_FUNC_BIT(func) BIT(func)
#define DSP_FUNC_ALL BIT_MASK(DSP_NUM_FUNCTIONS)

#if defined(DSP_SUPPORT_CVSD_DECODER) || defined(DSP_SUPPORT_CVSD_ENCODER)
#define PCM_FRAMESZ (128)
#else
#define PCM_FRAMESZ (256)
#endif

/* session command ids */
enum {
	/* session global commands */
	DSP_CMD_USER_DEFINED = 32,

	DSP_MIN_SESSION_CMD = DSP_CMD_USER_DEFINED,

	/* initialize static params before first run */
	DSP_CMD_SESSION_INIT = DSP_MIN_SESSION_CMD,
	/* begin session running (with dynamic params) */
	DSP_CMD_SESSION_BEGIN,
	/* end session running */
	DSP_CMD_SESSION_END,
	/* user-defined session command started from */
	DSP_CMD_SESSION_USER_DEFINED,
	/* configure the session working mode, it is required when the same library has two or more working modes,
	 * such as a dsp library for gaming headphone has two working mode : dongle & headphone.
	 */
	DSP_CMD_SESSION_WORKING_MODE,

	DSP_MAX_SESSION_CMD = DSP_MIN_SESSION_CMD + 0xFF,

	/* session function commands, function id is required */
	DSP_MIN_FUNCTION_CMD = DSP_MAX_SESSION_CMD + 0x1,

	/* configure function 288 */
	DSP_CMD_FUNCTION_CONFIG = DSP_MIN_FUNCTION_CMD,
	/* enable function */
	DSP_CMD_FUNCTION_ENABLE,
	/* disable function */
	DSP_CMD_FUNCTION_DISABLE,
	/* debug function */
	DSP_CMD_FUNCTION_DEBUG,
	/* pause function, like function_disable, but need not finalize function */
	DSP_CMD_FUNCTION_PAUSE,
	/* debug function, like function_enable, but need not initialize function */
	DSP_CMD_FUNCTION_RESUME,
	/* pre enable function */
	DSP_CMD_FUNCTION_PRE_ENABLE,
	/* user-defined function command started from */
	DSP_CMD_FUNCTION_USER_DEFINED,

	DSP_MAX_FUNCTION_CMD = DSP_MIN_FUNCTION_CMD + 0xFF,
};

/* session function configuration ids for complex functions */
enum {
    /* configure the default parameter set, for basic functions */
	DSP_CONFIG_DEFAULT = 0,

	/* configure the decoder parameter set, struct decoder_dspfunc_params */
	DSP_CONFIG_DECODER,

	/* configure the encoder parameter set, struct encoder_dspfunc_params */
	DSP_CONFIG_ENCODER,

	/* configure the encoder parameter set, struct dae_dspfunc_params */
	DSP_CONFIG_DAE,

	/* configure the aec reference parameter set, struct aec_dspfunc_ref_params */
	DSP_CONFIG_AEC_REF,

	/* configure the aec parameter set, struct aec_dspfunc_params */
	DSP_CONFIG_AEC,

	/* configure the playback mix parameter set, struct mix_dspfunc_params */
	DSP_CONFIG_MIX,

	/* configure the playback output parameter set, struct output_dspfunc_params */
	DSP_CONFIG_OUTPUT,

	/* configure the decoder extra parameter set, struct decoder_dspfunc_ext_params */
	DSP_CONFIG_DECODER_EXT,

	/* configure the postprocess energy set, struct energy_dspfunc_params */
	DSP_CONFIG_ENERGY,
	/* configure the postprocess misc set, such as auto mute, struct misc_dspfunc_parsms */
	DSP_CONFIG_MSIC,
	/* configure the postprocess type set, uint32_t music(0, default) or voice(1)  */
	DSP_CONFIG_TYPE,
	/* configure the audio out channel parameter set, struct dspfunc_audio_out_params  */
	DSP_CONFIG_AUDIO_OUT,

	/* configure dsp sleep parameter set, global, struct dspfunc_sleep_params */
	DSP_CONFIG_DSP_SLEEP,

	/* configure the player suspend */
	DSP_CONFIG_PLAYER_SUSPEND,
	/* configure the player resume */
	DSP_CONFIG_PLAYER_RESUME,

	/* configure the recorder suspend */
	DSP_CONFIG_RECORDER_SUSPEND,
	/* configure the recorder resume */
	DSP_CONFIG_RECORDER_RESUME,

	/* configure dsp play on time parameter set, global, struct dspfunc_play_ontime_params */
	DSP_CONFIG_PLAY_ONTIME,

	/* configure decoder output channel, asdec_chn_t */
	DSP_CONFIG_DECODE_CHANNEL,

	/* configure mixer stream */
    DSP_CONFIG_STREAM_END,

	/* configure music plc parameter set, struct decoder_dspfunc_ext_params_mplc */
	DSP_CONFIG_DECODER_MPLC,
	/* configure drop or insert data set, struct decoder_dspfunc_data_adjust */
	DSP_CONFIG_DATA_ADJUST,

	/* configure the decoder extra parameter set about format, decoder_dspfunc_ext_format_params_u */
	DSP_CONFIG_DECODER_EXT_FORMAT,

	/* configure the encoder extra parameter set about format, encoder_dspfunc_ext_format_params_u */
	DSP_CONFIG_ENCODER_EXT_FORMAT,

	/* configure soft volume parameter set, int volume_coef15[2] */
	DSP_CONFIG_SOFT_VOLUME,

	/* configure audio stream debug parameter set, struct audio_stream_debug, DSP_FUNCTION_PLAYER or DSP_FUNCTION_RECORDER */
	DSP_CONFIG_AUDIO_STREAM_DEBUG_START,
	DSP_CONFIG_AUDIO_STREAM_DEBUG_STOP,

	/* configure the recorder mute parameter set, int mute_flag */
	DSP_CONFIG_RECORDER_MUTE,

	/* configure the recorder nr flag parameter set, int nr_flag */
	DSP_CONFIG_RECORDER_NR_FLAG,

#ifdef CONFIG_BT_TRANSCEIVER
	DSP_CONFIG_ENCODER_EXT = 100,
    DSP_CONFIG_LOST_PKT_MODE,
    DSP_CONFIG_SET_LEFT_CHAN_HANDLE,
    DSP_CONFIG_SET_RIGHT_CHAN_HANDLE,
#endif
};

/* session error state */
enum {
	/* 1 ~ 31 dev error state >32 session error*/
	DSP_BAD_STREAM = 32,
	DSP_DAC_FULL_EMPTY = 33,
	DSP_ADC_DATA_FULL = 34,
};

/* codec format definition */
enum {
	DSP_CODEC_FORMAT_MP3  =  1, /* MP3_TYPE */
	DSP_CODEC_FORMAT_WMA  =  2, /* WMA_TYPE */
	DSP_CODEC_FORMAT_WAV  =  3, /* WAV_TYPE */
	DSP_CODEC_FORMAT_FLAC =  4, /* FLA_TYPE */
	DSP_CODEC_FORMAT_APE  =  5, /* APE_TYPE */
	DSP_CODEC_FORMAT_AAC  =  6, /* AAC_TYPE */
	DSP_CODEC_FORMAT_OGG  =  7, /* OGGC_TYPE */
	DSP_CODEC_FORMAT_ACT  =  8, /* ACT_TYPE */
	DSP_CODEC_FORMAT_AMR  =  9, /* AMR_TYPE */
	DSP_CODEC_FORMAT_SBC  = 10, /* SBC_TYPE */
	DSP_CODEC_FORMAT_MSBC = 11, /* MSBC_TYPE */
	DSP_CODEC_FORMAT_OPUS = 12, /* OPUS_TYPE */
	DSP_CODEC_FORMAT_CVSD = 15, /* CVSD_TYPE */
	DSP_CODEC_FORMAT_SPEECH = 16, /* CVSD_TYPE */
	DSP_CODEC_FORMAT_PCM = 17, /* LDAC_TYPE */
	DSP_CODEC_FORMAT_LDAC = 18, /* LDAC_TYPE */
	DSP_CODEC_FORMAT_NAV = 19, /* NAV_TYPE */
	DSP_CODEC_FORMAT_NAV_SEP = 20, /* NAV_SEP_TYPE */
	DSP_CODEC_FORMAT_AAC_M4A = 21, /* m4A_SEP_TYPE */
	DSP_CODEC_FORMAT_PCM_DSP = 22, /* PCM_DSP_TYPE */
};

enum {
	RESAMPLE_SAMPLERATE_16 = 16000,
	RESAMPLE_SAMPLERATE_44_1 = 44100,
	RESAMPLE_SAMPLERATE_48 = 48000,
};

enum {
	RESAMPLE_FRAMESZ_44_1 = 441,
	RESAMPLE_FRAMESZ_48 = 480,
	RESAMPLE_FRAMESZ_16 = 160,
};

enum {
    REF_STREAM_NULL = 0,
    REF_STREAM_MONO,
    REF_STREAM_LEFT,
    REF_STREAM_RIGHT,
};

enum {
    MIX_MODE_NULL,
    MIX_MODE_LEFT_RIGHT,   //左右输出
    MIX_MODE_LEFT,         //左输出
    MIX_MODE_RIGHT,        //右输出
    MIX_MODE_L_R_SWITCH,   //左右交换
    MIX_MODE_L_R_MERGE,    //左右混合
};

enum {
	AEC_NOT_INIT = 0,
    AEC_CONFIGED,
	AEC_START,
};

enum {
	RESAMPLE_APS_NONE = 0,
	RESAMPLE_APS_INVALID,
	RESAMPLE_APS_WORK,
};

struct resample_aps_params {
	uint32_t *step;
	uint32_t *state;
}__packed;

struct media_dspssn_params {
	int16_t aec_en;
} __packed;

struct media_dspssn_info {
	struct media_dspssn_params params;
} __packed;

/* test function parameters */
struct test_dspfunc_params {
	RINGBUF_TYPE inbuf;
	RINGBUF_TYPE refbuf;
	RINGBUF_TYPE outbuf;
} __packed;

/* test function runtime */
struct test_dspfunc_runtime {
	uint32_t sample_count;
} __packed;

/* test function runtime */
struct test_dspfunc_debug {
	RINGBUF_TYPE inbuf;
	RINGBUF_TYPE refbuf;
	RINGBUF_TYPE outbuf;
} __packed;


/* test function information */
struct test_dspfunc_info {
	struct test_dspfunc_params params;
	struct test_dspfunc_runtime runtime;
} __packed;

/* decoder resample function parameters */
struct decoder_resample_params{
	uint32_t input_sr;
	uint32_t output_sr;
	uint32_t input_sample_pairs;
	uint32_t output_sample_pairs;
} __packed;

/* decoder function parameters */
/* inbuf-->outbuf-->(resbuf)*/
struct decoder_dspfunc_params {
	uint16_t slot;	/* decoder slot */
	uint16_t format;
	uint16_t channels;
	uint16_t sample_bits;
	uint32_t sample_rate;
	RINGBUF_TYPE inbuf;
	RINGBUF_TYPE resbuf;
	RINGBUF_TYPE outbuf;
	struct decoder_resample_params resample_param;
} __packed;

/* decoder function extended parameters */
struct decoder_dspfunc_ext_params {
	/* format specific parameter buffer */
	uint32_t format_pbuf;
	uint32_t format_pbufsz;
	/* store events passed by decoder */
	RINGBUF_TYPE evtbuf;
} __packed;

/* decoder function nav ext format parameters */
struct decoder_dspfunc_ext_format_nav_params {
	uint16_t cig_sync_delay;
	uint16_t cis_sync_delay_play;
	uint16_t cis_sync_delay_xfer;
	uint16_t cis_sdu;
	uint16_t cis_bitrate_kbps;
	uint16_t cis_sdu_interval;
	uint16_t cis_m_bn : 4;
	uint16_t cis_m_ft : 3;
	uint16_t codec_stream_type : 1; /* 0-standard stream, 1-private stream */
	uint16_t codec_bit_width : 1; /* 0-16bits, 1-24bits */
	uint16_t working_mode : 3;
	uint16_t sync_delay_mode : 1; /* it requires recv report packet */
	uint16_t real_time_mode : 1; /* no used */
	uint16_t dump_crc_error_pkt : 1;
	uint16_t reserved_bits : 1;
	uint16_t codec_time_us;
	uint16_t recv_pkt_size;
	uint16_t reserved;
	RINGBUF_TYPE recv_report_buf;
} __packed;

/* decoder function ext format parameters */
typedef union {
	struct decoder_dspfunc_ext_format_nav_params nav_format_params;
} decoder_dspfunc_ext_format_params_u;

/* decoder function music plc parameters */
struct decoder_dspfunc_mplc_params {
	uint16_t mode:2;            // 0:disable, 1:empty frame plc, 2:history frame plc
	uint16_t thres_pause:6;     // mplc pause, due to too many empty frames(continuous)
	uint16_t thres_pemframe:8;  // mplc pause, due to too many empty frames in a period
	uint16_t thres_period;      // mplc empty frame statistics period
	uint16_t thres_resume;      // mplc resume, enough normal frames(continuous)
	uint16_t reserved;
	uint16_t samples_th1;       // rawdata is empty, do interpolation when pcmdata is lower
	uint16_t samples_th2;       // bigger than th1, keep doing interlolation thres after mplc started
} __packed;

/* decoder function data adjust parameters */
struct decoder_dspfunc_data_adjust {
	uint16_t start_seq;      // adjust start pkt seq, 0:the next decoded pkt
	uint16_t adj_flag:2;     // 1: drop data, 2:insert data, else reserved
	uint16_t reserved:14;    // part of bits has been used in dsp
	int      adj_samples;    // samples to be dropped or inserted
} __packed;

/* mix channel parameters */
struct mix_channel_params{
    //uint16_t master_channel:1;
    uint16_t channels:2;
    uint16_t vol:6;    // 0 ~ 31
    uint16_t is_mute:1;
    uint16_t mix_mode:3;  //MIX_MODE_NULL ...
    uint16_t sample_bits:1;  //0:16bit, 1:32bit
    uint16_t running:1;
    uint16_t master_channel:1;
    uint16_t mix_volume_enable:1;
    uint16_t input_sr:8;
    uint16_t output_sr:8;
    int32_t  mix_volume;  //if mix_volume_enable, it is coef15
    RINGBUF_TYPE chan_buf;
    RINGBUF_TYPE res_buf;
} __packed;

/* mix function parameters */
struct mix_dspfunc_params{
	uint32_t channel_num;
	struct mix_channel_params channel_params[DSP_SUPPORT_MULTI_BUFFER_NUM];
} __packed;

/* output function parameters */
struct output_dspfunc_params{
	RINGBUF_TYPE outbuf;
	RINGBUF_TYPE outbuf_subwoofer;
    RINGBUF_TYPE hdrbuf;
    uint32_t sample_bits:1;  //0:16bit, 1:32bit
	uint32_t dac_fifo:1;  //0:cpu dma dac fifo, 1:dsp directly dac fifo
	uint32_t latency_calc:1; //1:dsp latency calc
	uint32_t reserved:29;
} __packed;

/* energy function parameters */
struct energy_dspfunc_params{
	RINGBUF_TYPE energy_buf;
} __packed;


/* decoder function runtime */
struct decoder_dspfunc_runtime {
	uint32_t frame_size;
	uint32_t channels;
	uint32_t sample_count;
	uint32_t datalost_count;
	uint32_t raw_count;
    short dec_channel; //asdec_chn_t
    uint16_t dec_channel_change:1;
    uint16_t stream_ended:1;
    uint16_t stream_ending:1;
    uint16_t reserved:13;
} __packed;

struct dec_nav_stat_info {
	uint16_t stat_info_flag; /* 0 - invalid, 1 - valid, 2 - disable */
	uint16_t xfer_pkt_total;
	uint16_t xfer_no_pkt_count;
	uint16_t xfer_crc_err_count;
	uint16_t xfer_null;
	uint16_t xfer_miss_total;
	uint16_t xfer_rt_count[16];
} __packed;

typedef union {
	struct dec_nav_stat_info nav_info;
} dec_stat_info_u;

/* decoder function debug options */
struct decoder_dspfunc_debug {
	RINGBUF_TYPE stream;
	RINGBUF_TYPE pcmbuf;
	RINGBUF_TYPE plcbuf;
	dec_stat_info_u dec_stat_info;
} __packed;

/* decoder function information */
struct decoder_dspfunc_info {
	struct decoder_dspfunc_params params;
	struct decoder_dspfunc_runtime runtime;
	struct decoder_dspfunc_debug debug;
	struct decoder_dspfunc_ext_params ext_params;
	decoder_dspfunc_ext_format_params_u ext_format_params;
	struct decoder_dspfunc_mplc_params mplc_params;
	struct decoder_dspfunc_data_adjust data_adjust;
} __packed;

/* encoder function parameters */
struct encoder_dspfunc_params {
	uint16_t slot;	/* encoder slot */
	uint16_t format;
	uint16_t sub_format;
	uint16_t sample_bits;
	uint16_t sample_rate_orig;
	uint16_t sample_rate;
	uint32_t bit_rate;	/* bps, bits per second */
	uint16_t complexity;
	uint16_t channels : 2;
	uint16_t source : 2;  /* 0:don't care, 1:adc, 2:i2srx */
	uint16_t reserved_bits : 12;
	uint32_t frame_size;
	RINGBUF_TYPE inbuf;
	RINGBUF_TYPE outbuf;
} __packed;

struct encoder_dspfunc_params_ext{
	uint16_t tx_offset;
	uint16_t is_double_encoder;
	uint16_t chan_handle[2];
} __packed;

struct encoder_dspfunc_ext_format_nav_params {
	uint16_t cig_sync_delay;
	uint16_t cis_sync_delay_play;
	uint16_t cis_sync_delay_xfer;
	uint16_t cis_sdu;
	uint16_t cis_bitrate_kbps;
	uint16_t cis_sdu_interval;
	uint16_t cis_m_bn : 4;
	uint16_t cis_m_ft : 3;
	uint16_t codec_stream_type : 1; /* 0-standard stream, 1-private stream */
	uint16_t codec_bit_width : 1; /* 0-16bits, 1-24bits */
	uint16_t working_mode : 3;
	uint16_t sync_delay_mode : 1; /* it requires send report packet */
	uint16_t real_time_mode : 1; /* if real_time_mode enable, it requires sync_delay_mode enable */
	uint16_t dump_crc_error_pkt : 1;
	uint16_t reserved_bits : 1;
	uint16_t codec_time_us;
	uint16_t send_pkt_size;
	uint16_t reserved;
	RINGBUF_TYPE send_report_buf;
} __packed;

typedef union {
	struct encoder_dspfunc_ext_format_nav_params nav_format_params;
} encoder_dspfunc_ext_format_params_u;

/* encoder function runtime inbuf latency */
struct encoder_dspfunc_runtime_inbuf_latency {
	uint16_t cpu_read_seq;
	uint16_t dsp_write_seq;
	uint32_t latency_us;
} __packed;

/* encoder function runtime */
struct encoder_dspfunc_runtime {
	uint32_t frame_size;
	uint32_t channels;
	uint32_t compression_ratio;
	uint32_t sample_count;
	struct encoder_dspfunc_runtime_inbuf_latency inbuf_latency;
} __packed;

struct enc_nav_stat_info {
	uint16_t stat_info_flag; /* 0 - invalid, 1 - valid, 2 - disable */
	uint16_t send_pkt_total;
	uint16_t send_prefetch_fail_count;
	uint16_t send_fail_total;
	uint16_t send_rt_count[16];
} __packed;

typedef union {
	struct enc_nav_stat_info nav_info;
} enc_stat_info_u;

/* encoder function debug options */
struct encoder_dspfunc_debug {
	RINGBUF_TYPE pcmbuf;
	RINGBUF_TYPE stream;
	enc_stat_info_u enc_stat_info;
} __packed;

/* encoder function information */
struct encoder_dspfunc_info {
	struct encoder_dspfunc_params params;
	struct encoder_dspfunc_runtime runtime;
	struct encoder_dspfunc_debug debug;
	encoder_dspfunc_ext_format_params_u ext_format_params;
} __packed;

/* preprocess function information */
struct preprocess_dspfunc_info {
	struct encoder_dspfunc_params params;
	struct encoder_dspfunc_runtime runtime;
	struct encoder_dspfunc_debug debug;
} __packed;

/* dae function parameters */
struct dae_dspfunc_params {
	uint16_t enable;	/* dae enable or not */
	uint16_t pbufsz;	/* parameter buffer size */
	uint32_t pbuf;		/* parameter buffer address */
} __packed;

struct aec_dspfunc_channel_params{
	/* aec reference stream type */
	uint16_t stream_type;
    uint16_t aec_type : 2;
    uint16_t sample_bits : 1; /* 0-16bits, 1-24bits */
	uint16_t ref_gain : 1; /* reference gain enable */
    uint16_t reserved_bits : 12;

	/* ref stream source buffers */
    RINGBUF_TYPE srcbuf;

	/* ref stream buffers */
    RINGBUF_TYPE refbuf;
} __packed;

/* aec refs channel parameters */
struct aec_dspfunc_ref_params{
    /* delayed frames */
	uint16_t delay;

    /* ref buffer num */
	uint16_t channel_num;

    /* ref buffer num */
    struct aec_dspfunc_channel_params channel[DSP_SUPPORT_AEC_REFBUF_NUM];
} __packed;

/* aec function parameters */
struct aec_dspfunc_params {
	/* aec enable or not and aec reterance type */
	uint16_t enable; //0:disable 1:soft 2:hardware

    /* ref buffer num */
	uint16_t channel_num : 2;
	/* inbuf&outbuf sample bits */
	uint16_t sample_bits : 1; //0:16bits 1:24bits
	uint16_t pcm_type : 1; 
	uint16_t reserved_bits : 12;

	/* aec block size*/
	uint16_t block_size;

	/* dropped count of mic samples to sync with aec reference */
	uint16_t dropped_samples;

    /* mic data buffer */
	RINGBUF_TYPE inbuf;

    /* ref stream buffers */
    RINGBUF_TYPE refbuf[DSP_SUPPORT_AEC_REFBUF_NUM];

    /* aec out buffer */
    RINGBUF_TYPE outbuf;
} __packed;

/* misc function parameters */
struct misc_dspfunc_parsms {
	int16_t auto_mute;
	int16_t auto_mute_threshold;
	int16_t reserved[2];
}__packed;

/* audio channel parameters */
struct dspfunc_audio_out_params {
	uint16_t aout_volume_changed : 1;
	uint16_t aout_mute_changed : 1;
	uint16_t aout_mute : 1;
	uint16_t aout_fade_changed : 1;
	uint16_t aout_fade_mode : 2; //as FADE_STA_NONE, FADE_STA_IN, FADE_STA_OUT
	uint16_t reserved : 10;
	uint16_t aout_lchvol;
	uint16_t aout_rchvol;
	uint16_t aout_fade_time; //millisecond
} __packed;

/* play ontime mode */
enum {
	PLAY_ONTIME_LATENCY = 0,
	PLAY_ONTIME_TIMESTAMP = 1,
	PLAY_ONTIME_DELAY = 2,
};

/* dsp play on time parameters */
struct dspfunc_play_ontime_params {
	uint32_t play_ontime_enable : 1;
	uint32_t play_ontime_mode : 2;
	uint32_t reserved : 29;
	uint32_t time_val_lo;
	uint32_t time_val_hi;
    uint16_t *play_flag;
} __packed;

/* postprocessor function parameters */
struct streamout_dspfunc_params {
	struct dae_dspfunc_params dae;
	struct aec_dspfunc_ref_params aec_ref;
	struct output_dspfunc_params output;
	struct mix_dspfunc_params mix;
	struct energy_dspfunc_params energy;
	struct misc_dspfunc_parsms misc;
	struct dspfunc_audio_out_params audio;
	struct dspfunc_play_ontime_params play_ontime;
} __packed;

struct streamout_dspfunc_runtime_pcmbuf {
	uint16_t cpu_read_seq;
	uint16_t dsp_write_seq;
	uint16_t packet_num;
	uint16_t frame_num;
	uint32_t latency_us;
	uint32_t time_stamp_us;
} __packed;

typedef enum {
	FADE_STA_NONE = 0,
	FADE_STA_IN = 1,
	FADE_STA_OUT = 2,
	FADE_STA_IN_FINISH = 3,
	FADE_STA_OUT_FINISH = 4,
} dsp_fade_sta_e;

/* postprocessor function runtime */
struct streamout_dspfunc_runtime {
	uint16_t fromat;
	uint16_t fade_sta;
	uint32_t sample_rate;
	struct streamout_dspfunc_runtime_pcmbuf pcmbuf;
} __packed;

/* postprocessor function debug options */
struct streamout_dspfunc_debug {
	RINGBUF_TYPE dae_data;
} __packed;

/* encoder function information */
struct streamout_dspfunc_info {
	struct streamout_dspfunc_params params;
	struct streamout_dspfunc_runtime runtime;
	struct streamout_dspfunc_debug debug;
} __packed;

/* player function debug options */
struct player_dspfunc_debug {
	RINGBUF_TYPE decode_stream;
	RINGBUF_TYPE decode_data;
	RINGBUF_TYPE plc_data;
    RINGBUF_TYPE dae_data;
} __packed;

/* player function information */
struct player_dspfunc_info {
	struct dae_dspfunc_params dae;
	struct aec_dspfunc_ref_params aec_ref;
	struct output_dspfunc_params output;
	const struct decoder_dspfunc_info *decoder_info;
	struct mix_dspfunc_params mix;
	struct player_dspfunc_debug debug;
	struct energy_dspfunc_params energy;
	const struct streamout_dspfunc_info *streamout_info;
} __packed;

/* recorder function debug options */
struct recorder_dspfunc_debug {
	RINGBUF_TYPE mic1_data;
	RINGBUF_TYPE mic2_data;
	RINGBUF_TYPE ref_data;
	RINGBUF_TYPE aec1_data;
	RINGBUF_TYPE aec2_data;
	RINGBUF_TYPE encode_data;
	RINGBUF_TYPE encode_stream;
} __packed;

/* recorder function information */
struct recorder_dspfunc_info {
	struct dae_dspfunc_params dae;
	struct aec_dspfunc_params aec;
	const struct preprocess_dspfunc_info * preprocess_info;
	const struct encoder_dspfunc_info * encoder_info;

	struct recorder_dspfunc_debug debug;
} __packed;

/* dsp sleep parameters */
struct dspfunc_sleep_params {
	uint16_t sleep_enable; /* 0 is disable, 1 is enable*/
	uint16_t sleep_mode : 2; /* 0 is light sleep, 1 is deep sleep*/
	uint16_t sleep_stat_log : 1; /* log statistics dsp sleep time in 1,000,000 us */
	uint16_t sleep_debug_log : 1; /* log dsp sleep details */
	uint16_t reserved : 12;
} __packed;

typedef struct {
	uint16_t as_context_node;
	uint16_t bypass : 1;
	uint16_t bypass_fill : 2; /*0-null,1-fill zeros,2-fill sine 2khz*/
	uint16_t dumpdata : 1;
	uint16_t dumpslice : 1;
	uint16_t channel_option : 2; /*0-default,1-only left,2-only right*/
	uint16_t bitwidth_option : 2; /*0-default,1-max 16bits,2-max 24bits*/
} as_context_node_t;

/* audio stream debug parameters */
struct audio_stream_debug {
	uint16_t debug_enable : 1;
	uint16_t nodes_count : 4;
	uint16_t dump_block_enable : 1; /* block codec process when dump cache is not space */
	uint16_t dump_align_enable : 1; /* drop all when dump cache is not space, ensure data align */
	uint16_t reserved_bits : 9;
	uint16_t reserved_short;
	RINGBUF_TYPE dumpbuf;
	as_context_node_t debug_as_context_nodes[8];
} __packed;

enum {
	DSP_DEBUG_FLAG_DATA_IND  =  1, 
};

#ifdef __cplusplus
}
#endif

#endif /* _DSP_HAL_DEFS_H_ */
