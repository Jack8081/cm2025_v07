/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define SYS_LOG_DOMAIN "media"
#include <stdio.h>
#include <string.h>
#include <sdfs.h>
#include <audio_system.h>
#include <media_effect_param.h>

#define BTCALL_EFX_PATTERN			"alcfg.bin"
#define LINEIN_EFX_PATTERN			"linein.efx"
#define MUSIC_DRC_EFX_PATTERN		"alcfg.bin"
#define MUSIC_NO_DRC_EFX_PATTERN	"alcfg.bin"

static const void *btcall_effect_user_addr = NULL;
static const void *btcall_aec_user_addr = NULL;
static const void *linein_effect_user_addr = NULL;
static const void *music_effect_user_addr = NULL;
static const void *leaudio_call_effect_user_addr = NULL;

int medie_effect_set_user_param(uint8_t stream_type, uint8_t effect_type, const void *dae, const void *aec)
{
	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
		btcall_effect_user_addr = dae;
        btcall_aec_user_addr = aec;
		break;
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
		leaudio_call_effect_user_addr = dae;
        btcall_aec_user_addr = aec;
		break;
	case AUDIO_STREAM_LINEIN:
		linein_effect_user_addr = dae;
		break;
	default:
		music_effect_user_addr = dae;
		break;
	}

	return 0;
}

#ifndef CONFIG_MEDIA_DECODER_SNOOP_TWS
static const void *_media_effect_load(const char *efx_pattern, int *expected_size, uint8_t stream_type)
{
	char efx_name[16];
	void *vaddr = NULL;
	int size = 0;

	strncpy(efx_name, efx_pattern, sizeof(efx_name));
	sd_fmap(efx_name, &vaddr, &size);

	if (!vaddr || !size) {
		SYS_LOG_WRN("not found");
		return NULL;
	}

	*expected_size = size;

	SYS_LOG_INF("%s", efx_name);
	return vaddr;
}
#endif

void media_effect_get_param(uint8_t stream_type, uint8_t effect_type, const void **dae, const void **aec)
{
	const void *user_addr = NULL;
    const void *aec_addr = NULL;
	const char *efx_pattern = NULL;
	int expected_size = 0;

	switch (stream_type) {
	case AUDIO_STREAM_TTS: /* not use effect file */
    case AUDIO_STREAM_TIP:
		return ;
	case AUDIO_STREAM_VOICE:
		user_addr = btcall_effect_user_addr;
        aec_addr = btcall_aec_user_addr;
		efx_pattern = BTCALL_EFX_PATTERN;
		expected_size = sizeof(asqt_para_bin_t);
		break;
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
		user_addr = leaudio_call_effect_user_addr;
        aec_addr = btcall_aec_user_addr;
		efx_pattern = BTCALL_EFX_PATTERN;
		expected_size = sizeof(asqt_para_bin_t);
		break;
	case AUDIO_STREAM_LINEIN:
		user_addr = linein_effect_user_addr;
		efx_pattern = LINEIN_EFX_PATTERN;
		expected_size = sizeof(aset_para_bin_t);
		break;
	case AUDIO_STREAM_TWS:
		user_addr = music_effect_user_addr;
		efx_pattern = MUSIC_NO_DRC_EFX_PATTERN;
		expected_size = sizeof(aset_para_bin_t);
		break;
	default:
		user_addr = music_effect_user_addr;
	#ifdef CONFIG_MUSIC_DRC_EFFECT
		efx_pattern = MUSIC_DRC_EFX_PATTERN;
	#else
		efx_pattern = MUSIC_NO_DRC_EFX_PATTERN;
	#endif
		expected_size = sizeof(aset_para_bin_t);
		break;
	}

#ifndef CONFIG_MEDIA_DECODER_SNOOP_TWS
	if (!user_addr)
		user_addr = _media_effect_load(efx_pattern, &expected_size, stream_type);
#endif

	if (dae)
		*dae = user_addr;
    if (aec)
		*aec = aec_addr;
}

void media_dmvp_get_param(uint8_t stream_type, uint8_t effect_type, const void **ffv16, const void **polar)
{
	void *vaddr = NULL;
	int size = 0;

	sd_fmap("ffv16.bin", &vaddr, &size);
	if (!vaddr || !size) {
		SYS_LOG_WRN("not found");
		return;
	}
    if(strncmp((char*)vaddr, "FFV", 3) == 0) {
        *ffv16 = vaddr;
    }

	sd_fmap("polar.bin", &vaddr, &size);
	if (!vaddr || !size) {
		SYS_LOG_WRN("not found");
		return;
	}
    if(strncmp((char*)vaddr, "POLA", 4) == 0) {
        *polar = vaddr;
    }
    
}


void* media_ai_get_param(uint8_t stream_type, uint8_t effect_type)
{
	void *vaddr = NULL;
	int size = 0;

	sd_fmap("ainr1.bin", &vaddr, &size);
	if (!vaddr || !size) {
		SYS_LOG_WRN("not found");
		return NULL;
	}

    if(strncmp((char*)vaddr, "ANR1", 4) != 0) {
        return NULL;
    }

    return vaddr;
}


