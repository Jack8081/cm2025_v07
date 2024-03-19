/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief le audio app media profile
 */

#include "leaudio.h"
#include "app_common.h"

/*unit is 0.1MHz*/
/*dimensional-1: lc3 interval, [0] 10ms, [1] 7.5ms*/
/*dimensional-2: lc3 bit width, [0] 16bits, [1] 24bits*/
/*dimensional-3: lc3 samplerate, [0] 48khz, [1] 32khz, [2] 24khz, [3] 16khz*/
/*dimensional-4: lc3 bitrate, [0] 192kbps, [1] 184kbps, [2] 176kbps, [3] 160kbps, [4] 158kbps*/
const uint16_t lc3_decode_run_mhz_dsp_music_2ch[2][2][1][LC3_BITRATE_MUSIC_2CH_COUNT] = 
{
	/*10ms*/
	{   /*16bits*/
		{   /*48khz*/
			{ 295 /*Confirmed*/, 295, 295, 295, 295, },
		},
		/*24bits*/
		{   /*48khz*/
			{ 325, 325, 325, 325, 325, },
		},
	},
	/*7.5ms*/
	{   /*16bits*/
		{   /*48khz*/
			{ 345 + 10 /*FIXME*/, 345, 345, 345, 345, },
		},
		/*24bits*/
		{   /*48khz*/
			{ 350 /*Confirmed*/, 350, 350, 350, 350, },
		},
	},
};

/*unit is 0.1MHz*/
/*dimensional-1: lc3 interval, [0] 10ms, [1] 7.5ms*/
/*dimensional-2: lc3 bit width, [0] 16bits, [1] 24bits*/
/*dimensional-3: lc3 samplerate, [0] 48khz, [1] 32khz, [2] 24khz, [3] 16khz*/
/*dimensional-4: lc3 bitrate, [0] 96kbps, [1] 80kbps, [2] 78kbps*/
const uint16_t lc3_decode_run_mhz_dsp_music_1ch[2][2][1][LC3_BITRATE_MUSIC_1CH_COUNT] = 
{
	/*10ms*/
	{   /*16bits*/
		{  	/*48khz*/
			{ 150, 150, 150, },
		},
		/*24bits*/
		{   /*48khz*/
			{ 165, 165, 165, },
		},
	},
	/*7.5ms*/
	{   /*16bits*/
		{   /*48khz*/
			{ 175, 175, 175, },
		},
		/*24bits*/
		{   /*48khz*/
			{ 194, 194, 194, },
		},
	},
};

/*unit is 0.1MHz*/
/*dimensional-1: lc3 interval, [0] 10ms, [1] 7.5ms*/
/*dimensional-2: lc3 bit width, [0] 16bits, [1] 24bits*/
/*dimensional-3: lc3 samplerate, [0] 48khz, [1] 32khz, [2] 24khz, [3] 16khz*/
/*dimensional-4: lc3 bitrate, [0] 80kbps, [1] 78kbps, [2] 64kbps, [3] 48kbps, [4] 32kbps*/
const uint16_t lc3_decode_run_mhz_dsp_call_1ch[2][2][4][LC3_BITRATE_CALL_1CH_COUNT] = 
{
	/*10ms*/
	{   /*16bits*/
		{   /*48khz*/
			{ 150, 150, 150, 150, 150, },
			/*32khz*/
			{ 150, 150, 150, 150, 150, },
			/*24khz*/
			{ 150, 150, 150, 150, 150, },
			/*16khz*/
			{ 150, 150, 150, 150, 150, },
		},
		/*24bits*/
		{   /*48khz*/
			{ 165, 165, 165, 165, 165, },
			/*32khz*/
			{ 165, 165, 165, 165, 165, },
			/*24khz*/
			{ 165, 165, 165, 165, 165, },
			/*16khz*/
			{ 165, 165, 165, 165, 165, },
		},
	},
	/*7.5ms*/
	{   /*16bits*/
		{   /*48khz*/
			{ 175, 175, 175, 175, 175, },
			/*32khz*/
			{ 175, 175, 175, 175, 175, },
			/*24khz*/
			{ 175, 175, 175, 175, 175, },
			/*16khz*/
			{ 175, 175, 175, 175, 175, },
		},
		/*24bits*/
		{   /*48khz*/
			{ 194, 194, 194, 194, 194, },
			/*32khz*/
			{ 194, 194, 194, 194, 194, },
			/*24khz*/
			{ 194, 194, 194, 194, 194, },
			/*16khz*/
			{ 194, 194, 194, 194, 194, },
		},
	},
};

/*unit is 0.1MHz*/
/*dimensional-1: lc3 interval, [0] 10ms, [1] 7.5ms*/
/*dimensional-2: lc3 bit width, [0] 16bits, [1] 24bits*/
/*dimensional-3: lc3 samplerate, [0] 48khz, [1] 32khz, [2] 24khz, [3] 16khz*/
/*dimensional-4: lc3 bitrate, [0] 80kbps, [1] 78kbps, [2] 64kbps, [3] 48kbps, [4] 32kbps*/
const uint16_t lc3_encode_run_mhz_dsp_call_1ch[2][2][4][LC3_BITRATE_CALL_1CH_COUNT] = 
{
	/*10ms*/
	{   /*16bits*/
		{   /*48khz*/
			{ 209, 209 /*Confirmed*/, 200, 190, 180, },
			/*32khz*/
			{ 200, 200, 188 /*Confirmed*/, 180, 170, },
			/*24khz*/
			{ 200, 200, 188, 180, 170, },
			/*16khz*/
			{ 200, 200, 188, 180, 170, },
		},
		/*24bits*/
		{   /*48khz*/
			{ 240, 240, 230, 220, 210, },
			/*32khz*/
			{ 220, 220, 210, 200, 190, },
			/*24khz*/
			{ 220, 220, 210, 200, 190, },
			/*16khz*/
			{ 220, 220, 210, 200, 190, },
		},
	},
	/*7.5ms*/
	{   /*16bits*/
		{   /*48khz*/
			{ 250, 250, 240, 230, 220, },
			/*32khz*/
			{ 210, 210, 200, 190 + 10 /*FIXME*/, 185, },
			/*24khz*/
			{ 210, 210, 200, 190, 185, },
			/*16khz*/
			{ 210, 210, 200, 190, 185, },
		},
		/*24bits*/
		{   /*48khz*/
			{ 260, 260, 250, 240, 230, },
			/*32khz*/
			{ 220, 220, 210 /*Confirmed*/, 200, 195, },
			/*24khz*/
			{ 220, 220, 210, 200, 195, },
			/*16khz*/
			{ 220, 220, 210, 200, 195, },
		},
	},
};

/*unit is 0.1MHz*/
/*dimensional-1: lc3 samplerate, [0] 48khz, [1] 32khz, [2] 24khz, [3] 16khz*/

/*20peq + drc(-0.5db) + 48->32 resample*/
const uint16_t postdae_run_mhz_dsp_music_2ch[4] = { 370, 320, 270, 270, };

/*20peq + drc(-0.5db) + 48->32 resample*/
const uint16_t postdae_run_mhz_dsp_music_1ch[4] = { 270, 220, 170, 170, };

/*10peq + drc*/
const uint16_t postdae_run_mhz_dsp_call_1ch[4] = { 80, 80, 62, 62, };

/*10peq + drc*/
const uint16_t predae_run_mhz_dsp_call_1ch[4] = { 80, 80, 62, 62, };

/*aec + dual mic ai enc*/
const uint16_t preaec_dual_mic_run_mhz_dsp_call_1ch[4] = { 570, 570, 570, 570 };

/*aec + single mic ai enc*/
const uint16_t preaec_single_mic_run_mhz_dsp_call_1ch[4] = { 500, 430, 400, 400 };

const uint16_t playback_system_run_mhz = 60;
const uint16_t capture_system_run_mhz = 60;
