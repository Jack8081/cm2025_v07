/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file gui api interface 
 */
#ifndef __GUI_API_H__
#define __GUI_API_H__

int GUI_UTF8_to_Unicode(uint8_t* utf8, int len, uint16_t** unicode);
int GUI_MBCS_to_Unicode(uint8_t* mbcs, int len, uint16_t** unicode);
int GUI_Unicode_to_UTF8(uint16_t* unicode, int len, uint8_t** utf8);
int GUI_Unicode_to_MBCS(uint16_t* unicode, int len, uint8_t** mbcs);
int GUI_UTF8_to_MBCS(uint8_t* utf8, int len, uint8_t** mbcs);
int GUI_MBCS_to_UTF8(uint8_t* mbcs, int len, uint8_t** utf8);

#endif