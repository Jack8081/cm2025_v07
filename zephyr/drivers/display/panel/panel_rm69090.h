/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PANEL_RM69090_DRIVER_H__
#define PANEL_RM69090_DRIVER_H__

#define RM69090_CMD_NOP				0x00
#define RM69090_CMD_SWRESET			0x01 /* Software Reset */
#define RM69090_CMD_RDDID			0x04 /* Read Display ID */
#define RM69090_CMD_RDNUMED			0x05 /* Read Number of Errors on DSI */
#define RM69090_CMD_RDDPM			0x0A /* Read Display Power Mode */
#define RM69090_CMD_RDDMADCTR		0x0B /* Read Display MADCTR */
#define RM69090_CMD_RDDCOLMOD		0x0C /* Read Display Pixel Format */
#define RM69090_CMD_RDDIM			0x0D /* Read Display Image Mode */
#define RM69090_CMD_RDDSM			0x0E /* Read Display Signal Mode */
#define RM69090_CMD_RDDSDR			0x0F /* Read Display Self-Diagnostic Result */
#define RM69090_CMD_SLPIN			0x10 /* Sleep In */
#define RM69090_CMD_SLPOUT			0x11 /* Sleep Out */
#define RM69090_CMD_PTLON			0x12 /* Partial Display Mode On */
#define RM69090_CMD_NORON			0x13 /* Normal Display Mode On */
#define RM69090_CMD_INVOFF			0x20 /* Display Inversion Off */
#define RM69090_CMD_INVON			0x21 /* Display Inversion On */
#define RM69090_CMD_ALLPOFF			0x22 /* All Pixel Off */
#define RM69090_CMD_ALLPON			0x23 /* All Pixel On */
#define RM69090_CMD_DISPOFF			0x28 /* Display Off */
#define RM69090_CMD_DISPON			0x29 /* Display On */
#define RM69090_CMD_CASET			0x2A /* Set Column Start Address */
#define RM69090_CMD_RASET			0x2B /* Set Row Start Address */
#define RM69090_CMD_RAMWR			0x2C /* Memory Write */
#define RM69090_CMD_PTLAR			0x30 /* Partial Area */
#define RM69090_CMD_VPTLAR			0x31 /* Vertical Partial Area */
#define RM69090_CMD_TEOFF			0x34 /* Tearing Effect Line OFF */
#define RM69090_CMD_TEON			0x35 /* Tearing Effect Line ON */
#define RM69090_CMD_MADCTR			0x36 /* Scan Direction Control */
#define RM69090_CMD_IDMOFF			0x38 /* Idle Mode Off */
#define RM69090_CMD_IDMON			0x39 /* Enter Idle Mode */
#define RM69090_CMD_COLMOD			0x3A /* Interface Pixel Format */
#define RM69090_CMD_RAMWRC			0x3C /* Memory Continuous Write */

#define RM69090_CMD_STESL			0x44 /* Set Tear Scanline */
#define RM69090_CMD_GSL				0x45 /* Get Scanline */
#define RM69090_CMD_DSTBON			0x4F /* Deep Standby Mode On */
#define RM69090_CMD_WRDISBV			0x51 /* Write Display Brightness */
#define RM69090_CMD_RDDISBV			0x52 /* Read Display Brightness */
#define RM69090_CMD_WRCTRLD			0x53 /* Write Display Control */
#define RM69090_CMD_RDCTRLD			0x54 /* Read Display Control */
#define RM69090_CMD_WRRADACL		0x55 /* RAD_ACL Control */
#define RM69090_CMD_SCE				0x58 /* Set Color Enhance */
#define RM69090_CMD_GCE				0x59 /* Read Color Enhance */
#define RM69090_CMD_WRHBMDISBV		0x63 /* Write HBM Display Brightness */
#define RM69090_CMD_RDHBMDISBV		0x64 /* Read HBM Display Brightness */
#define RM69090_CMD_HBM				0x66 /* Set HBM Mode */
#define RM69090_CMD_DEEPIDM			0x67 /* Set Deep Idle Mode */

#define RM69090_CMD_COLSET			0x70 /* Interface Pixel Format Set */
#define RM69090_CMD_COLOPT			0x80 /* Interface Pixel Format Option */
#define RM69090_CMD_RDDDBS			0xA1 /* Read DDB Start */
#define RM69090_CMD_RDDDBC			0xA8 /* Read DDB Continous */
#define RM69090_CMD_RDFCS			0xAA /* Read First Checksum */
#define RM69090_CMD_RDCCS			0xAF /* Read Continue Checksum */
#define RM69090_CMD_SETDISPMOD		0xC2 /* Set DISP Mode */
#define RM69090_CMD_SETDSPIMOD		0xC4 /* Set DSPI Mode */

#define RM69090_CMD_RDID1			0xDA /* Read ID1 Code */
#define RM69090_CMD_RDID2			0xDB /* Read ID2 Code */
#define RM69090_CMD_RDID3			0xDC /* Read ID3 Code */
#define RM69090_CMD_MAUCCTR			0xFE /* CMD Mode Switch */
#define RM69090_CMD_RDMAUCCTR		0xFF /* Read CMD Status */

#endif /* PANEL_RM69090_DRIVER_H__ */
