/*!
 * \file      ap_status.c
 * \brief     
 * \details   
 * \author 
 * \date  
 * \copyright Actions
 */

#include <user_comm/ap_status.h>
#include <os_common_api.h>

static ap_status_t ap_status = AP_STATUS_NONE;
static ap_tts_status_t tts_status = AP_TTS_STATUS_NONE;

void ap_status_set(ap_status_t status)
{
	SYS_LOG_INF("last %d, cur %d.",ap_status, status);
	ap_status = status;
}

ap_status_t ap_status_get(void)
{
	return ap_status;
}

void ap_tts_status_set(ap_tts_status_t status)
{
	SYS_LOG_INF("last %d, cur %d.",tts_status, status);
	tts_status = status;
}

ap_tts_status_t ap_tts_status_get(void)
{
	return tts_status;
}

