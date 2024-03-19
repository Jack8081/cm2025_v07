
#ifndef __CONSUMER_EQ_H__
#define __CONSUMER_EQ_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONSUMER_EQ_NUM  (10)
#define SPEAKER_EQ_NUM  (5)

typedef struct
{
    /*中心频点（或者高通滤波器的截止频率），单位Hz，比如100 for 100Hz，推荐范围是30到12000，默认值由客户自定义*/
    short cutoff;
    /*Q值，单位0.1，推荐范围是3到30，比如7 for 0.7, 默认值由客户自定义*/
    short q;
    /*增益，单位0.1db，推荐范围是-240到+120，比如60 for 6.0db，默认值为0*/
    short gain;
    /*滤波器类型：固定为1 for peaking filter*/
    short type;
}peq_band_t;

typedef struct
{
    peq_band_t eq[CONSUMER_EQ_NUM];
}consumer_eq_t;

typedef struct
{
    peq_band_t eq[SPEAKER_EQ_NUM];
}speaker_eq_t;


/**
 *  \brief Get consumer eq param
 *
 *  \param [out] eq   Output consumer eq
 *  \return 0 if success or -1 fail
 */
int32_t consumer_eq_get_param(consumer_eq_t *eq);

/**
 *  \brief Set consumer eq param
 *
 *  \param [in] eq   consumer eq
 *  \return 0 if success or -1 fail
 */
int32_t consumer_eq_set_param(consumer_eq_t *eq);

/**
 *  \brief Patch the consumer eq to dae param
 *
 *  \param [in] eq_info   Whole 512 bytes dae param
 *  \param [in] size      Dae param size, must be 512
 *  \return 0 if success or -1 fail
 */
int32_t consumer_eq_patch_param(void *eq_info, int32_t size);

/**
 *  \brief Consumer eq param sync from tws
 *
 *  \param [in] data_buf   Consumer eq
 *  \param [in] size       Consumer eq size
 *  \return 0 if success or -1 fail
 */
int32_t consumer_eq_tws_set_param(uint8_t *data_buf, int32_t size);

/**
 *  \brief Notify consumer_eq that tws connected
 *
 *  \return 0 if success or -1 fail
 */
int32_t consumer_eq_on_tws_connect(void);


/*------------------------------------------------------------------------*/

/**
 *  \brief Close effect eq
 *
 *  \return 0 if success or -1 fail
 */
int32_t effect_eq_close(void);

/**
 *  \brief Close speaker eq
 *
 *  \return 0 if success or -1 fail
 */
int32_t speaker_eq_close(void);

/**
 *  \brief Set speaker eq
 *  \param [in] eq    speaker eq
 *  \param [in] save  save eq to nvram or no
 *
 *  \return 0 if success or -1 fail
 */
int32_t speaker_eq_set_param(speaker_eq_t *eq, bool save);


#ifdef __cplusplus
}
#endif

#endif //end __CONSUMER_EQ_H__


