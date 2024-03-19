/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file audio system api
 * @brief
*/

#ifndef __AUDIO_SYSTEM_H__
#define __AUDIO_SYSTEM_H__
#include <stream.h>

/**
 * @defgroup audio_system_apis Auido System APIs
 * @ingroup media_system_apis
 * @{
 */
typedef enum {
	AUDIO_STREAM_DEFAULT = 1,
	AUDIO_STREAM_MUSIC,
	AUDIO_STREAM_LOCAL_MUSIC,
	AUDIO_STREAM_TTS,
	AUDIO_STREAM_VOICE,
	AUDIO_STREAM_LINEIN,
	AUDIO_STREAM_LINEIN_MIX,
	AUDIO_STREAM_SUBWOOFER,
	AUDIO_STREAM_ASR,
	AUDIO_STREAM_AI,
	AUDIO_STREAM_USOUND,
	AUDIO_STREAM_USOUND_MIX,
	AUDIO_STREAM_USPEAKER,
	AUDIO_STREAM_I2SRX_IN,
	AUDIO_STREAM_SPDIF_IN,
	AUDIO_STREAM_GENERATE_IN,
	AUDIO_STREAM_GENERATE_OUT,
	AUDIO_STREAM_LOCAL_RECORD,
	AUDIO_STREAM_GMA_RECORD,
	AUDIO_STREAM_BACKGROUND_RECORD,
	AUDIO_STREAM_MIC_IN,
	AUDIO_STREAM_FM,
	AUDIO_STREAM_TWS,
	AUDIO_STREAM_TIP,
	AUDIO_STREAM_VOICE_ANC,
	AUDIO_STREAM_LE_AUDIO,
	AUDIO_STREAM_LE_AUDIO_MUSIC,
	AUDIO_STREAM_LE_AUDIO_GAME,
	AUDIO_STREAM_TR_USOUND,
}audio_stream_type_e;

typedef enum {
	AUDIO_MODE_DEFAULT = 0,
	AUDIO_MODE_MONO,                    //mono->left mono->right
	AUDIO_MODE_STEREO,                  //left->left right->right
	AUDIO_MODE_3CHENNEL,
	AUDIO_MODE_4CHENNEL,
}audio_mode_e;

typedef enum {
	AUDIO_STREAM_TRACK = 1,
	AUDIO_STREAM_RECORD,
}audio_stream_mode_e;

typedef enum {
	AUDIO_FORMAT_PCM_8_BIT = 0,
	AUDIO_FORMAT_PCM_16_BIT,
	AUDIO_FORMAT_PCM_24_BIT,
	AUDIO_FORMAT_PCM_32_BIT,
}audio_format_e;

/**
 * @cond INTERNAL_HIDDEN
 */

#define MIN_WRITE_SAMPLES    1 * 1024

#define MAX_AUDIO_TRACK_NUM  1
#define MAX_AUDIO_RECORD_NUM 2
#define MAX_AUDIO_DEVICE_NUM 1

#define MAX_VOLUME_VALUE 2
#define MIN_VOLUME_VALUE 1
#define DEFAULT_VOLUME   5

struct audio_track_t {
	uint8_t stream_type;
	uint8_t audio_format;
	uint8_t channels;
	uint8_t channel_type;
	uint8_t channel_id;
	uint8_t channel_mode;
	uint8_t sample_rate;
	uint8_t frame_size;
	uint8_t flushed;
	uint8_t muted:1;
	uint8_t stared:1;
	uint8_t waitto_start:1;
    uint8_t dump_pcm:1;
	uint8_t first_frame:1;
	uint8_t fade_mode:2;
    uint8_t dsp_fifo_src:1;
	uint16_t volume;
	uint32_t output_sample_rate_hz;

	uint16_t pcm_frame_size;
	uint8_t *pcm_frame_buff;

	io_stream_t audio_stream;
	io_stream_t mix_stream;
	uint8_t mix_sample_rate;
	uint8_t mix_channels;

	/** audio hal handle*/
	void *audio_handle;

	void (*event_cb)(uint8_t, void *);
	void *user_data;

	/* For tws sync fill samples */
	int compensate_samples;
	int fill_cnt;

	/* resample */
	void *res_handle;
	int res_in_samples;
	int res_out_samples;
	int res_remain_samples;
	int16_t *res_in_buf[2];
	int16_t *res_out_buf[2];

	/* fade in/out */
	void *fade_handle;

	/* mix */
	void *mix_handle;

    uint32_t samples_filled;
    uint32_t sdm_sample_rate;
    uint16_t sdm_osr;
    uint8_t audiopll_mode;
    uint8_t automute_force;
};

#define AUDIO_ADC_NUM   (4)

typedef struct {
#if CONFIG_AUDIO_IN_ADGAIN_SUPPORT
int16_t ch_again[AUDIO_ADC_NUM];
int16_t ch_dgain[AUDIO_ADC_NUM];
#else
	int16_t ch_gain[AUDIO_ADC_NUM];
#endif
} audio_input_gain;

struct audio_record_t {
	uint8_t stream_type;
    uint8_t ext_stream_type;
	uint8_t audio_format;
	uint8_t audio_mode;
	uint8_t channel_type;
	uint8_t channel_id;
	uint8_t channel_mode;
	uint8_t sample_rate;
	uint8_t output_sample_rate;
	uint8_t frame_size;
	uint8_t sample_bits;
	uint8_t muted:1;
	uint8_t paused:1;
	uint8_t first_frame:1;
	/**debug flag*/
	uint8_t dump_pcm:1;
	uint8_t fill_zero:1;
    uint8_t reload_buff_alloced:1;
    uint8_t drop_cnt:2;

	int16_t adc_gain;
	uint16_t pcm_buff_size;
	uint8_t *pcm_buff;

    /* block to save adc pcm data if playback no start yet, 0: no block, 1: block */
    uint16_t *play_flag;

	/** audio hal handle*/
	void *audio_handle;
	io_stream_t audio_stream;

	/**dma is operated by cpu, consumer is another core (dsp or others)*/
	uint8_t is_low_latency:1; /* if yes, dma drm set to single mode */
	uint8_t is_dest_directly:1; /* if yes, need't copy dma buffer to another bigger buffer */

	uint8_t last_aps_level;
	uint8_t audiopll_mode;
};

struct audio_system_t {
	os_mutex audio_system_mutex;
	struct audio_track_t *audio_track_pool[MAX_AUDIO_TRACK_NUM];
	struct audio_record_t *audio_record_pool[MAX_AUDIO_RECORD_NUM];
	struct audio_device_t *audio_device_pool[MAX_AUDIO_DEVICE_NUM];
	uint8_t audio_track_num;
	uint8_t audio_record_num;
	bool microphone_muted;
	uint8_t microphone_nr_flag;
	uint8_t output_sample_rate;
	uint8_t capture_output_sample_rate;
	bool master_muted;
	uint8_t master_volume;

	uint8_t tts_volume;
	uint8_t music_volume;
	uint8_t voice_volume;
	uint8_t linein_volume;
	uint8_t fm_volume;
	uint8_t i2srx_in_volume;
	uint8_t mic_in_volume;
	uint8_t spidf_in_volume;
	uint8_t usound_volume;
	uint8_t lcmusic_volume;
	uint8_t lemusic_volume;
	uint8_t levoice_volume;	
    uint8_t left_channel_enable;
    uint8_t right_channel_enable;
};

/** cace info ,used for cache stream */
typedef struct
{
	uint8_t audio_type;
	uint8_t audio_mode;
	uint8_t channel_mode;
	uint8_t stream_start:1;
	uint8_t dma_start:1;
	uint8_t dma_reload:1;
	uint8_t pcm_buff_owner:1;
	uint8_t data_finished:4;
	uint16_t dma_send_len;
	uint16_t pcm_frame_size;
	struct acts_ringbuf *pcm_buff;
	/**pcm cache*/
	io_stream_t pcm_stream;
} audio_info_t;

typedef enum
{
    APS_OPR_SET          = (1 << 0),
    APS_OPR_ADJUST       = (1 << 1),
    APS_OPR_FAST_SET     = (1 << 2),
}aps_ops_type_e;

typedef enum
{
    APS_STATUS_DEC,
    APS_STATUS_INC,
    APS_STATUS_DEFAULT,
}aps_status_e;

typedef struct {
	uint8_t current_level;
	uint8_t dest_level;
	uint8_t aps_status;
	uint8_t aps_mode;

	uint8_t aps_min_level;
	uint8_t aps_max_level;
	uint8_t aps_default_level;
	uint8_t role;
	uint8_t duration;
	uint8_t last_level;
	uint8_t need_aps:1;
#ifdef CONFIG_AUDIO_APS_ADJUST_FINE
	uint8_t need_aps_fine:1; /* it also means tws can adjust aps separately */
	uint8_t aps_fine_flag:1;
	uint8_t audiopll_mode:2; /* 0-default,1-integer-N,2-fractional-N */
#endif
	uint8_t lea_restarting:1;

    uint16_t aps_count;

	/**in usec*/
	uint32_t aps_reduce_water_mark;
	uint32_t aps_increase_water_mark;
	uint32_t aps_water_adjust_thres;
    
    int len;
    int max_len;
    int min_len;
 	int average_min_len;
	int average_min_left;
	int average_diff;
	int average_diff_left;
	int average_expect_len;
	int average_expect_left;
    
	int last_min_len;
	int boundary_diff;
	int boundary_diff_left;

	int last_expect_len;
	int expect_diff;
	int expect_diff_left;
    
	int boundary_count;
	u8_t interval_count;   
	uint32_t aps_fine_adjust_time;
	uint32_t aps_water_adjust_last;

	struct audio_track_t *audio_track;
	void *tws_observer;
}aps_monitor_info_t;
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief set audio system output sample rate
 *
 * @details This routine provides to set audio system output sample rate,
 *  if set audio system output sample rate, all out put stream may resample to
 *  the target sample rate
 *
 * @param value sample rate
 *
 * @return 0 excute successed , others failed
 */
int audio_system_set_output_sample_rate(int value);
/**
 * @brief get audio system output sample rate
 *
 * @details This routine provides to get audio system output sample rate,
 *
 * @return value of sample rate
 */
int audio_system_get_output_sample_rate(void);

/**
 * @brief set audio system master volume
 *
 * @details This routine provides to set audio system master volume
 *
 * @param value volume value
 *
 * @return 0 excute successed , others failed
 */

int audio_system_set_master_volume(int value);

/**
 * @brief get audio system master volume
 *
 * @details This routine provides to get audio system master volume
 *
 * @return value of volume level
 */

int audio_system_get_master_volume(void);

/**
 * @brief set audio system master mute
 *
 * @details This routine provides to set audio system master mute
 *
 * @param value mute value 1: mute 0: unmute
 *
 * @return 0 excute successed , others failed
 */

int audio_system_set_master_mute(int value);

/**
 * @brief get audio system master mute state
 *
 * @details This routine provides to get audio system master mute state
 *
 * @return  1: audio system muted
 * @return  0: audio system unmuted
 */

int audio_system_get_master_mute(void);

int audio_system_set_stream_volume(int stream_type, int value);

int audio_system_save_volume(const char *name, uint8_t volume);

int audio_system_get_stream_volume(int stream_type);

int audio_system_get_current_volume(int stream_type);

int audio_system_set_stream_mute(int stream_type, int value);

int audio_system_get_stream_mute(int stream_type);

int audio_system_mute_microphone(int value);

int audio_system_get_microphone_muted(void);

int audio_system_set_microphone_nr_flag(uint8_t nr_flag);
int audio_system_get_microphone_nr_flag(void);

int audio_system_get_current_pa_volume(int stream_type);

/* @volume in 0.001 dB */
int audio_system_set_stream_pa_volume(int stream_type, int volume);

/* @volume in 0.1 dB */
int audio_system_set_microphone_volume(int stream_type, audio_input_gain *input_gain);

int audio_system_get_lr_channel_enable(bool *l_enable, bool *r_enable);
int audio_system_set_lr_channel_enable(bool l_enable, bool r_enable);

int aduio_system_init(void);
/**
 * @cond INTERNAL_HIDDEN
 */
int audio_system_register_track(struct audio_track_t *audio_track);

int audio_system_unregister_track(struct audio_track_t *audio_track);

int audio_system_register_record(struct audio_record_t *audio_record);

int audio_system_unregister_record(struct audio_record_t *audio_record);

void audio_aps_monitor(int pcm_time);

void audio_aps_set_latency(int latency_us);

u32_t audio_aps_get_latency(void);

u32_t audio_aps_get_samplerate_hz(u8_t is_48khz);

#define APS_FORMAT_SHIFT        (0)
#define APS_FORMAT_MASK         (0xff<<0)
#define APS_AUDIOPLL_MODE_SHIFT (16)
#define APS_AUDIOPLL_MODE_MASK  (0x3<<16)
void audio_aps_monitor_init(uint32_t format_and_audiopll_mode, void *tws_observer, struct audio_track_t *audio_track);

void audio_aps_monitor_init_add_samples(int format, uint8_t *need_notify, uint8_t *need_sync);

void audio_aps_monitor_exchange_samples(uint32_t *ext_add_samples, uint32_t *sync_ext_samples);

void audio_aps_notify_decode_err(uint16_t err_cnt);
void audio_lea_notify_decode_err(uint16_t err_cnt);

void audio_aps_monitor_deinit(int format, void *tws_observer, struct audio_track_t *audio_track);

void audio_tws_pre_init(void);
void audio_aps_monitor_tws_init(void *tws_observer);

void audio_aps_tws_notify_decode_err(uint16_t err_cnt);

int32_t audio_tws_set_stream_info(
    uint8_t format, uint16_t first_seq, uint8_t sample_rate, uint32_t pkt_time_us, uint64_t play_time);

#define SYNC_TYPE_PKT_INFO  (1<<0)  /* default ENABLE */
#define SYNC_TYPE_SPLAY     (1<<1)  /* default ENABLE */
int32_t audio_tws_set_sync_policy(uint32_t type, uint8_t enable);

/**
 * @brief get snoop tws first stream packet seq to play
 *
 * @param start_decode  indicate to start decode or no
 * @param start_play  indicate to start play or no
 * @param first_seq     first seq to decode
 *
 * @return 0 excute successed , others failed
 */
int audio_tws_get_playback_first_seq(uint8_t *start_decode, uint8_t *start_play, uint16_t *first_seq, uint16_t *playtime_us);

int32_t audio_tws_set_pkt_info(uint16_t pkt_seq, uint16_t pkt_len, uint16_t frame_cnt, uint16_t pcm_len);
uint64_t audio_tws_get_play_time_us(void);
uint64_t audio_tws_get_bt_clk_us(void *tws_observer);

void audio_aps_monitor_tws_deinit(void *tws_observer);

aps_monitor_info_t *audio_aps_monitor_get_instance(void);

struct audio_track_t * audio_system_get_track(void);

int audio_system_mutex_lock(void);
int audio_system_mutex_unlock(void);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @} end defgroup audio_system_apis
 */

#endif
