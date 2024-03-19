/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file media audio stream define
 */

#ifndef __MEDIA_AUDIO_STREAM_H__
#define __MEDIA_AUDIO_STREAM_H__

#include <os_common_api.h>

#define AS_CONTEXT_PLAYER           0x01 /* include Music, TTS, Keytone */
#define AS_CONTEXT_RECORDER         0x02
#define AS_CONTEXT_CONVERSATION     0x03
#define AS_CONTEXT_GAMING           0x04
#define AS_CONTEXT_ANY              0xff

#define AS_NODE_INPUT_AUDIO         0x01 /* include ADC, I2SRx, S/PDIFRx, UAC download */
#define AS_NODE_INPUT_LOCAL         0x02 /* include DAC, I2STx, UAC upload */
#define AS_NODE_INPUT_REMOTE        0x03 /* include BT */
#define AS_NODE_INPUT_MAIN_MIC      0x04
#define AS_NODE_INPUT_REF_MIC       0x05
#define AS_NODE_PRE_RESAMPLE        0x10
#define AS_NODE_PRE_ASRC            0x11
#define AS_NODE_CONVERT             0x12 /* bitwidth, mono/stereo format convert */
#define AS_NODE_AEC                 0x13
#define AS_NODE_ENC                 0x14
#define AS_NODE_PEQ                 0x15
#define AS_NODE_DRC                 0x16
#define AS_NODE_AGC                 0x17
#define AS_NODE_CNG                 0x18
#define AS_NODE_AEC_REF             0x19
#define AS_NODE_MIC_EFFECT_PRE1     0x20
#define AS_NODE_MIC_EFFECT_PRE2     0x21
#define AS_NODE_MIC_EFFECT          0x22
#define AS_NODE_MIC_EFFECT_POST1    0x23
#define AS_NODE_MIC_EFFECT_POST2    0x24
#define AS_NODE_ENCODE              0x30
#define AS_NODE_DECODE              0x40
#define AS_NODE_PLC                 0x41
#define AS_NODE_RESAMPLE            0x50
#define AS_NODE_ASRC                0x51
#define AS_NODE_PRE_VOLUME          0x60
#define AS_NODE_EFFECT_PRE1         0x61
#define AS_NODE_EFFECT_PRE2         0x62
#define AS_NODE_EFFECT              0x63
#define AS_NODE_EFFECT_POST1        0x64
#define AS_NODE_EFFECT_POST2        0x65
#define AS_NODE_FADE                0x70
#define AS_NODE_VOLUME              0x71
#define AS_NODE_MIX                 0x72
#define AS_NODE_OUTPUT_AUDIO        0x80 /* include DAC, I2STx, UAC upload */
#define AS_NODE_OUTPUT_LOCAL        0x81 /* include card, uhost, norflash, nandflash */
#define AS_NODE_OUTPUT_REMOTE       0x82 /* include BT */

#define AS_CONTEXT_NODE(c, n)		(((unsigned short)c<<8) | n)

#endif /* __MEDIA_AUDIO_STREAM_H__ */
