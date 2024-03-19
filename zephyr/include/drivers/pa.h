#ifndef _DRIVER_PA_H_
#define _DRIVER_PA_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_PA_ACM8625
#define CONFIG_PA_DEV_NAME  "pa_acm8625"
#else 
#define CONFIG_PA_DEV_NAME  "pa_null"
#endif

#if IS_ENABLED(CONFIG_I2C_0)
#define CONFIG_PA_I2C_NAME  CONFIG_I2C_0_NAME 
#else
#define CONFIG_PA_I2C_NAME  CONFIG_I2C_1_NAME
#endif

/**
 * Audio output volume
 */
#define PA_AUDIO_VOLUME_MIN     (0)
#define PA_AUDIO_VOLUME_MAX   (100)

/**
 * Definitions for audio output channels
*/
#define PA_AUDIO_CH_L           (0x01 << 0)
#define PA_AUDIO_CH_R           (0x01 << 1)
#define PA_AUDIO_CH_ALL         (PA_AUDIO_CH_L | PA_AUDIO_CH_R)

/**
 * Definitions for audio mute state
*/
typedef enum{
    PA_AUDIO_MUTE_OFF,
    PA_AUDIO_MUTE_ON,
} audio_mute_t;

/**
 * Definitions for i2s type
*/
typedef enum {
    I2S_TYPE_PHILIPS,
    I2S_TYPE_TDM,
    I2S_TYPE_RTJ,
    I2S_TYPE_LTJ,
} i2s_type_t;

/**
 * Definitions for i2s sample rate
*/
typedef enum
{
    I2S_SAMPLE_RATE_44_1K,
    I2S_SAMPLE_RATE_48K,
    I2S_SAMPLE_RATE_88_2K,
    I2S_SAMPLE_RATE_96K,
    I2S_SAMPLE_RATE_176_4K,
    I2S_SAMPLE_RATE_192K,
} i2s_sample_rate_t;

/**
 * Definitions for i2s data width
*/
typedef enum
{
    I2S_DATA_WIDTH_16,
    I2S_DATA_WIDTH_20,
    I2S_DATA_WIDTH_24,
    I2S_DATA_WIDTH_32,
} i2s_data_width_t;

struct pa_i2s_format
{
    uint8_t type;       /* refer to @i2s_type_t         */
    uint8_t rate;       /* refer to @i2s_sample_rate_t  */
    uint8_t width;      /* refer to @i2s_data_width_t   */
};

struct pa_driver_state
{
    uint8_t type;       /* refer to @i2s_type_t         */
    uint8_t rate;       /* refer to @i2s_sample_rate_t  */
    uint8_t width;      /* refer to @i2s_data_width_t   */

    uint8_t volume_l;   /* range 0 ~ 100, default 50    */
    uint8_t volume_r;   /* range 0 ~ 100, default 50    */
    uint8_t mute_l  :1;
    uint8_t mute_r  :1;
};

struct pa_device_data 
{
    struct device          *i2c_dev;
    struct pa_driver_state  state;
};

typedef int (*pa_api_audio_init)(const struct device *dev);
typedef int (*pa_api_audio_fmt)(const struct device *dev, const struct pa_i2s_format *p_i2s_fmt);
typedef int (*pa_api_audio_volume)(const struct device *dev, uint8_t channel, uint8_t volume);
typedef int (*pa_api_audio_mute)(const struct device *dev, uint8_t channel, bool mute);

struct pa_driver_api
{
    pa_api_audio_init   api_init;
    pa_api_audio_fmt    api_config;
    pa_api_audio_volume api_volume;
    pa_api_audio_mute   api_mute;
};

/* PA APIS */
int         pa_audio_init(const struct device *dev);
int         pa_audio_format_config(const struct device *dev, const struct pa_i2s_format *p_i2s_fmt);
int         pa_audio_volume_set(const struct device *dev,  uint8_t channel,  uint8_t volume);
int         pa_audio_mute_set(const struct device *dev,    uint8_t channel,  bool mute);

int         pa_audio_state_get(const struct device *dev, struct pa_driver_state *pa_state);

#ifdef __cplusplus
}
#endif

#endif
