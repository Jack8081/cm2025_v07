/*!
 * \file      ap_status.h
 * \brief     
 * \details   
 * \author 
 * \date   
 * \copyright Actions
 */

#ifndef _AP_STATUS_H
#define _AP_STATUS_H

typedef enum
{
    AP_STATUS_NONE,
    AP_STATUS_TTS_START,
    AP_STATUS_TTS_STOP,
    AP_STATUS_LINKED,
    AP_STATUS_AUTH,
    AP_STATUS_RECORDING,
    AP_STATUS_TTS_PLAYING,
    AP_STATUS_OTA_RUNING,
    AP_STATUS_ERROR,

} ap_status_t;

typedef enum
{
    AP_TTS_STATUS_NONE,
    AP_TTS_STATUS_PLAY,

} ap_tts_status_t;

void ap_status_set(ap_status_t status);

ap_status_t ap_status_get(void);

void ap_tts_status_set(ap_tts_status_t status);

ap_tts_status_t ap_tts_status_get(void);

#endif  // _AP_STATUS_H


