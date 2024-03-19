/*
 * Copyright (c) 2019 Marc Reilly
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef PANEL_jd9854_DRIVER_H__
#define PANEL_jd9854_DRIVER_H__

#define jd9854_CMD_READID			0x04
#define jd9854_CMD_READSTATUS		0x09
#define jd9854_CMD_SLEEP_IN			0x10
#define jd9854_CMD_SLEEP_OUT		0x11
#define jd9854_CMD_PARTIALMOD		0x12
#define jd9854_CMD_NORMALMOD		0x13
#define jd9854_CMD_INV_OFF			0x20
#define jd9854_CMD_INV_ON			0x21
#define jd9854_CMD_DISP_OFF			0x28
#define jd9854_CMD_DISP_ON			0x29
#define jd9854_CMD_CASET			0x2a
#define jd9854_CMD_RASET			0x2b
#define jd9854_CMD_RAMWR			0x2c
#define jd9854_CMD_PARTIALAREA		0x30
#define jd9854_CMD_VERTSCROLL		0x33
#define jd9854_CMD_TE_OFF			0x34
#define jd9854_CMD_TE_ON			0x35

#define jd9854_CMD_RAMACC			0x36
#define jd9854_RAMACC_HREFR_INV		(0x1 << 2)
#define jd9854_RAMACC_VREFR_INV		(0x1 << 4)
#define jd9854_RAMACC_RGB_ORDER		(0x0 << 3)
#define jd9854_RAMACC_BGR_ORDER		(0x1 << 3)
#define jd9854_RAMACC_ROWCOL_SWAP	(0x1 << 5)
#define jd9854_RAMACC_COL_INV		(0x1 << 6)
#define jd9854_RAMACC_ROW_INV		(0x1 << 7)

#define jd9854_CMD_VERTSCROLL_ADDR	0x37
#define jd9854_CMD_IDLEMOD_OFF		0x38
#define jd9854_CMD_IDLEMOD_ON		0x39

#define jd9854_CMD_COLMOD			0x3a
#define jd9854_COLMOD_RGB_16bit		(0x5 << 4)
#define jd9854_COLMOD_RGB_18bit		(0x6 << 4)
#define jd9854_COLMOD_MCU_gray		(0)
#define jd9854_COLMOD_MCU_3bit		(1)
#define jd9854_COLMOD_MCU_8bit		(2)
#define jd9854_COLMOD_MCU_12bit		(3)
#define jd9854_COLMOD_MCU_16bit		(5)
#define jd9854_COLMOD_MCU_18bit		(6)
#define jd9854_COLMOD_MCU_24bit		(7)

#define jd9854_CMD_RAMWR_CONTINUE	0x3c
#define jd9854_CMD_TESET			0x44
#define jd9854_CMD_SCANLINEGET		0x45
#define jd9854_CMD_BRIGHTNESS		0x51
#define jd9854_CMD_DISPCTRL			0x53

#define jd9854_CMD_RGBCTRL			0xb0
#define jd9854_CMD_PORCHCTRL		0xb5
#define jd9854_CMD_DISPFUNCTRL		0xb6
#define jd9854_CMD_TECTRL			0xb4
#define jd9854_CMD_IFCTRL			0xf6

#define jd9854_CMD_INVERSION		0xec
#define jd9854_CMD_POWCTRL1			0xc1
#define jd9854_CMD_POWCTRL2			0xc3
#define jd9854_CMD_POWCTRL3			0xc4
#define jd9854_CMD_POWCTRL4			0xc9
#define jd9854_CMD_GAMSET1      	0xf0
#define jd9854_CMD_GAMSET2      	0xf1
#define jd9854_CMD_GAMSET3      	0xf2
#define jd9854_CMD_GAMSET4      	0xf3
#define jd9854_CMD_INTERREG_EN1		0xfe
#define jd9854_CMD_INTERREG_EN2		0xef

#endif /* PANEL_jd9854_DRIVER_H__ */
