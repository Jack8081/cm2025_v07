/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app define
 */

#ifndef _APP_DEFINES_H
#define _APP_DEFINES_H

#define APP_ID_MAIN				"main"
#define APP_ID_BTMUSIC			"btmusic"
#define APP_ID_BTCALL			"btcall"

#define APP_ID_TWS				"tws"
#define APP_ID_DMAAPP			"dma_app"

#define APP_ID_OTA				"ota"
#define APP_ID_SD_MPLAYER		"sd_mplayer"
#define APP_ID_UHOST_MPLAYER	"uhost_mplayer"
#define APP_ID_NOR_MPLAYER		"nor_mplayer"
#define APP_ID_LOOP_PLAYER		"loop_player"
#define APP_ID_LINEIN			"linein"
#define APP_ID_SPDIF_IN			"spdif_in"
#define APP_ID_I2SRX_IN			"i2srx_in"
#define APP_ID_MIC_IN			"mic_in"
#define APP_ID_RECORDER			"recorder"
#define APP_ID_USOUND			"usound"
#define APP_ID_ALARM			"alarm"
#define APP_ID_FM				"fm"
#define APP_ID_AP_RECORD		"ap_record"
#define APP_ID_ATT				"att"
#define APP_ID_OTA				"ota"
#define APP_ID_ASET				"aset"
#define APP_ID_LE_AUDIO			"le_audio"
#define APP_ID_TR_USOUND		"tr_usound"

#ifdef CONFIG_BT_BR_TRANSCEIVER
#define APP_ID_TR_BR_BQB        "tr_br_bqb"
#define APP_ID_TR_BQB_AG		"tr_bqb_ag"
#endif

#ifdef CONFIG_LE_AUDIO_BQB
#define APP_ID_USOUND_BQB		"usound_bqb"
#endif

#define APP_ID_DEFAULT		APP_ID_BTMUSIC
/*
 * app id switch list
 */

#ifdef CONFIG_BT_BR_TRANSCEIVER

#define app_id_list {\
						APP_ID_BTMUSIC,\
						APP_ID_BTCALL,\
						APP_ID_AP_RECORD,\
						APP_ID_ATT,\
						APP_ID_OTA,\
						APP_ID_LE_AUDIO,\
						APP_ID_LINEIN,\
						APP_ID_I2SRX_IN,\
                        APP_ID_TR_USOUND,\
                        APP_ID_TR_BR_BQB,\
                        APP_ID_TR_BQB_AG,\
					}
#elif CONFIG_LE_AUDIO_BQB
#define app_id_list {\
						APP_ID_BTMUSIC,\
						APP_ID_BTCALL,\
						APP_ID_AP_RECORD,\
						APP_ID_ATT,\
						APP_ID_OTA,\
						APP_ID_LE_AUDIO,\
						APP_ID_LINEIN,\
						APP_ID_I2SRX_IN,\
                        APP_ID_TR_USOUND,\
                        APP_ID_USOUND_BQB,\
					}

#else
#ifdef CONFIG_BT_TRANSCEIVER
#define APP_ID_TR_LINEIN			"tr_linein"

#define app_id_list {\
						APP_ID_BTMUSIC,\
						APP_ID_BTCALL,\
						APP_ID_AP_RECORD,\
						APP_ID_ATT,\
						APP_ID_OTA,\
						APP_ID_LE_AUDIO,\
						APP_ID_LINEIN,\
						APP_ID_I2SRX_IN,\
                        APP_ID_TR_LINEIN,\
                        APP_ID_TR_USOUND,\
                        APP_ID_USOUND,\
					}
#else
#define app_id_list {\
						APP_ID_BTMUSIC,\
						APP_ID_BTCALL,\
						APP_ID_AP_RECORD,\
						APP_ID_ATT,\
						APP_ID_OTA,\
						APP_ID_LE_AUDIO,\
					}
#endif
#endif

extern char share_stack_area[CONFIG_APP_STACKSIZE];

#define APP_PORC_MONITOR_TIME (5000) /*units:us*/


#endif  // _APP_DEFINES_H
