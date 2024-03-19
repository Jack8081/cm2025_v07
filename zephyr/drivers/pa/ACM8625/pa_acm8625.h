#ifndef _DRIVER_PA_ACM8625_H_
#define _DRIVER_PA_ACM8625_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_ACM_AUDIO_DSP_EQ

/**
 * Audio output total levels 
 */
#define ACM_AUDIO_VOLUME_STEP       (5)
#define ACM_AUDIO_VOLUME_LEVEL      (PA_AUDIO_VOLUME_MAX / ACM_AUDIO_VOLUME_STEP + 1)

int         pa_acm8625_audio_init(const struct device *pa_dev);
int         pa_acm8625_audio_format_config(const struct device *pa_dev, const struct pa_i2s_format *p_i2s_fmt);
int         pa_acm8625_audio_volume_set(const struct device *pa_dev,  uint8_t channel, uint8_t volume);
int         pa_acm8625_audio_mute_set(const struct device *pa_dev,    uint8_t channel, bool mute);

#ifdef __cplusplus
}
#endif

#endif
