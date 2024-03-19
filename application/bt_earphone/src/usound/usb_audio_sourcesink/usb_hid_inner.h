/*
 * Copyright (c) 2018 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _USB_HID_INNER_H_
#define _USB_HID_INNER_H_
#include <usb/class/usb_hid.h>

#define CONFIG_USB_SELF_DEFINED_REPORT 1
/* system control */
#define REPORT_ID_1		0x01
/* consumer control */
#define REPORT_ID_2		0x02
/* mouse */
#define REPORT_ID_3		0x03
/* keyboard */
#define REPORT_ID_4		0x04
/* self-defined */
#define REPORT_ID_5		0x05
/* Audio Volume Ctrl */
#define REPORT_ID_6		0x06
/* Telephone Status */
#define TELEPHONE_REPORT_ID 0X07
#define PLAYER_REPORT_ID    0x06
#define TELEPHONE_REPORT_ID1   0x07
/*
 * Report descriptor for keys, button and so on
 */
static const u8_t hid_report_desc[] = {
	
    0x05, 0x0c, //USAGE_PAGE (Consumer)
    0x09, 0x01, //USAGE (Consumer Control)
    0xa1, 0x01, //COLLECTION (Application)
	0x85, PLAYER_REPORT_ID,  // Report ID(0x1 )
    0x15, 0x00, //Logical Minimum (0x00)
    0x25, 0x01, //Logical Maximum (0x01)
    0x09, 0xe9, //USAGE (Volume Up)
    0x09, 0xea, //USAGE (Volume Down)
    0x09, 0xe2, //USAGE (Mute)
    0x09, 0xcd, //USAGE (Play/Pause)
    //0x09, 0xb0, //USAGE (Play)
    0x09, 0xb5, //USAGE (Scan Next Track)
    0x09, 0xb6, //USAGE (Scan Previous Track)
    //0x09, 0xb3, //USAGE (Fast Forward)
    0x09, 0xb1, //USAGE (Pause)
    0x09, 0xb7, //USAGE (Stop)
    0x75, 0x01, //Report Size (0x01)
    0x95, 0x08, //Report Count (0x08),报告的个数为8，即总共有8个bits
    0x81, 0x42, //Input (Data,Variable,Absolute,No Wrap,Linear,Preferred State,Null state)
    0x75, 0x01, //report size(1)
    0x95, 0x08, //report count(8)
	0x81, 0x03,  //Input (Cnst,Variable,Absolute)
    0xc0, //END_COLLECTION           //END_COLLECTION

	0x05, 0x0b,  // Usage Page(Telephony )
#ifndef CONFIG_USB_HID_TEAMS_CERT
	0x09, 0x01,  // Usage(Phone)
#else
    0x09, 0x05,  //Usage(Headset)
#endif
	0xa1, 0x01,  // Collection(Application )
	0x85, TELEPHONE_REPORT_ID1,  // Report ID(0x2 )
	0x15, 0x00, //Logical Minimum (0x00)
    0x25, 0x01, //Logical Maximum (0x01)
	0x09, 0x20, //USAGE (Hook Switch)
    0x09, 0x97, //Usage(Line BusyTone)
	0x75, 0x01,  // Report Size(0x1)
	0x95, 0x02,  // Report Count(0x1)
	0x81, 0x23,  //Input (Data,Variable,Absolute)

	0x75, 0x06,  // Report Size(6)
	0x95, 0x01,  // Report Count(1)
	0x81, 0x03,  //Input (Cnst,Variable,Absolute)

	0x09, 0x21,  // Usage(Flash)
	0x09, 0x2F,  // Usage(Phone Mute)
    0x09, 0x50,  // Usage(speed dail)
	0x75, 0x01,  // Report Size(1)
	0x95, 0x03,  // Report Count(3)
	0x81, 0x07,  //Input (Data,Variable,Absolute)

    0x09, 0x06,  //usage (telephony key pad)
    0xa1, 0x02,  //collection(logical)
    0x19, 0xb0,  //usage minimum(phone key 0)
    0x29, 0xbb,  //usage maximum(phone key pound)
    0x15, 0x00,  //logical minimum(0)
    0x25, 0x0c,  //logical maximum(12)
    0x75, 0x04,  //report size(4)
    0x95, 0x01,  //report count(1)
    0x81, 0x40,  //input(Data,Ary,Abs)
    0xc0,        //end collection

    0x09, 0x07,  //usage(programmable Button)
    0x05, 0x09,  //usage(Button)
    0x75, 0x01,  //report size(1)
    0x95, 0x01,  //report count(1)
    0x81, 0x02,  //Input(data, var, abs,Nwrp,Lin,Pref,Nnul,Bit)

	// 下面是主机会发送REPORT
	0x05, 0x08,  // Usage Page(LEDs )
	0x15, 0x00,  // Logical Minimum(0x0 )
	0x25, 0x01,  // Logical Maximum(0x1 )
    0x09, 0x08, //USAGE (Do Not Disturb) // 0x01
    0x09, 0x17, //USAGE (Off-Hook) // 0x02
    0x09, 0x18, //USAGE (Ring) // 0x04
    0x09, 0x19, //USAGE (Message Waiting) // 0x08
    0x09, 0x1a, //USAGE (Data Mode) // 0x10
    0x09, 0x1e, //USAGE (Speaker) // 0x20
    0x09, 0x1f, //USAGE (Head Set) // 0x40
    0x09, 0x20, //USAGE (Hold) // 0x80
    0x09, 0x21, //USAGE (Microphone) // 0x0100
    0x09, 0x22, //USAGE (Coverage) // 0x0200
    0x09, 0x23, //USAGE (Night Mode) // 0x0400
    0x09, 0x24, //USAGE (Send Calls) // 0x0800
    0x09, 0x25, //USAGE (Call Pickup) // 0x1000
    0x09, 0x26, //USAGE (Conference) // 0x2000
    0x09, 0x09,  //Usage(Mute) // 0x4000
    0x75, 0x01, //Report Size (0x01)
    0x95, 0x0f, //Report Count (0x0f),报告的个数为15，即总共有15个bits
    0x91, 0x02, //Output (Data,Variable,Absolute,No Wrap,Linear,Preferred State,Null state)

    0x75, 0x01, //Report Size (0x01)
    0x95, 0x01, //Report Count (0x01),报告的个数为1
    0x91, 0x03, //Output (Const,Variable,Absolute)

    0xc0, //END_COLLECTION

#ifdef CONFIG_USB_HID_TEAMS_CERT
    //增加自定义的hid
    0x06, 0x00, 0xFF, //USAGE PAGE(Vendor-define 1)
    0x09, 0x01,     //USAGE(Vendor-defined 1)
    0xA1, 0x01, //Collection(Application)
    0x85, 0x05, //Report ID(5)
    0x09, 0x01, //Usage(Vendor-define 1)
    0x15, 0x00, //LOGICAL_MINIMUM(0x0)
    0x26, 0xFF, 0x00, //LOGICAL_MAXIMUM(255)
    0x75, 0x08, //report size(0x8)
    0x95, 0x3F, //report count(63)
    0x92, 0x02, 0x01, //Output(Data, Ary, Abs, )
    0x09, 0x01, //Usage(Vendor-define 1)
    0x15, 0x00, //logical minmum(0)
    0x26, 0xff, 0x00, //logical maxmum(255)
    0x75, 0x08, //report size(8)
    0x95, 0x3f, //report count(63)
    0x82, 0x02, 0x01, //Input(Data,Var,Abs,Nwrp,Lin,Pref,NNul,Buf)

    0x85, 0x04, //report ID(4)
    0x06, 0x30, 0xff,   //Usage(vendor-define 49)
    0x15, 0x00, //logical minmum(0)
    0x25, 0x01, //logical maxmum(1)
    0x09, 0x20, //Usage(vendor-define 32)
    0x09, 0x97, //Usage(vendor-define 97)
    0x75, 0x01, //Report Size(1)
    0x95, 0x02, //Report Count(2)
    0x81, 0x23, //Input(Cnst,Var,Abs,Nwrp,Lin,Npref,NNul,Bit)
    0x09, 0x2f, //Usage(Vendor-define 47)
    0x09, 0x21, //Usage(Vendor-define 33)
    0x0A, 0xfd, 0xff, //Usage(Vendor-define 65533)
    0x09, 0x70, //Usage(Vendor-define 112)
    0x09, 0x50, //Usage(Vendor-define 80)
    0x75, 0x01, //report size(1)
    0x95, 0x05, //report count(5)
    0x81, 0x07, //Input(Cnst,Var,Rel,Nwrp,Lin,Pref,NNul,Bit)
    0x09, 0x06, //Usage(Vendor-define 6)
    0xa1, 0x02, //Collection(Logical)
    0x19, 0xb0, //Usage Minimum(Vendor-define 176)
    0x29, 0xbb, //Usage Maximum(Vendor-define 187)
    0x15, 0x00, //logical Minimum(0)
    0x25, 0x0c, //logical Maximum(12)
    0x75, 0x04, //report size(4)
    0x95, 0x01, //report count(1)
    0x81, 0x40, //Input(Data,Ary,Abs)
    0xc0,       //end collection
    0x0a, 0xfc, 0xff, //Usage(Vendor-define 65532)
    0x15, 0x00, //logical Minimum(0)
    0x25, 0x01, //logical Maximum(1)
    0x75, 0x01, //report size(1)
    0x95, 0x01, //report count(1)
    0x81, 0x23, //Input(Cnst,Var,Abs,Nwrp,Lin,Npref,NNul,Bit)
    0x75, 0x01, //report size(1)
    0x95, 0x04, //report count(4)
    0x81, 0x01, //Input(Cnst,Ary,Abs)
    0x06, 0x40, 0xff, //Usage Page(Vendor-define 65)
    0x15, 0x00, //logical Minimum(0)
    0x25, 0x01, //logical Maximum(1)
    0x09, 0x17, //Usage(Vendor-define 23)
    0x09, 0x09, //Usage(Vendor-define 9)
    0x09, 0x18, //Usage(Vendor-define 24)
    0x09, 0x20, //Usage(Vendor-define 32)
    0x09, 0x21, //Usage(Vendor-define 33)
    0x75, 0x01, //report size(1)
    0x95, 0x05, //report count(5)
    0x91, 0x22, //output(Data,Var,Abs,Nwrp,Lin,Npref,NNul,Nvol,Bit)
    0x06, 0x30, 0xff, //Usage page(Vendor-define 49)
    0x15, 0x00, //logical Minimum(0)
    0x25, 0x01, //logical Maximum(1)
    0x09, 0x9e, //Usage(Vendor-define 158)
    0x75, 0x01, //report size(1)
    0x95, 0x01, //report count(1)
    0x91, 0x22, //Output(Data,Var,Abs,Nwrp,Lin,Npref,NNul,NVol,Bit)
    0x75, 0x01, //report size(1)
    0x95, 0x0a, //report count(10)
    0x91, 0x01, //Output(Cnst,Ary,Abs,Nwrp,Lin,Npref,NNul,NVol,Bit)
    0x0a, 0xff, 0xff, //Usage(Vendor-define 65535)
    0x15, 0x00, //logical Minimum(0)
    0x25, 0x01, //logical Maximum(1)
    0x75, 0x01, //report size(1)
    0x95, 0x01, //report count(1)
    0xb1, 0x22, //Feature(Data,Var,Abs,Nwrp,Lin,Nprf,NNul,Nvol,Bit)
    0x75, 0x01, //report size(1)
    0x95, 0x07, //report count(7)
    0xb1, 0x01, //Feature(Cnst,Ary,Abs,...)
    0xc0,       //End collection

    0x06, 0x99, 0xff, //Usage Page(Vendor-define 154)
    0x09, 0x01, //Usage(Vendor-define 1)
    0xa1, 0x01, //Collection(Application)
    0x09, 0x03, //Usage(Vendor-define 3)
    0x85, 0x9a, //report ID(154)
    0x15, 0x00, //logical Minimum(0)
    0x26, 0xff, 0x00, //logical Maximum(255)
    0x75, 0x08, //report size(8)
    0x95, 0x3f, //report count(63)
    0xb2, 0x02, 0x01, //Feature(Data,Var,Abs,Nwrp,Lin,Pref,NNul,NVol,Buf)
    0x09, 0x04, //Usage(Vendor-define 4)
    0x85, 0x9b, //Report ID(155)
    0x15, 0x00, //logical Minimum(0)
    0x25, 0x01, //logical Maximum(1)
    0x75, 0x01, //report size(1)
    0x95, 0x01, //report count(1)
    0x81, 0x02, //Input(Data,Var,Abs..)
    0x75, 0x01, //report size(1)
    0x95, 0x0f, //report count(15)
    0x81, 0x01, //Input(Cnst,Ary,Abs)
    0xc0,   //End collection
#endif
#if 1
    //add for hid updata
    0x06, 0x90, 0xff, //Usage Page(Vendor-define 144)
    0x09, 0x01, //Usage(Vendor-define 1)
    0xa1, 0x01, //Collection(Application)
    0x09, 0x03, //Usage(Vendor-define 3)
    0x85, 0x55, //Report ID(85)
    0x15, 0x00, //logical Minimum(0)
    0x26, 0xff, 0x00, //logical Maximum(255)
    0x75, 0x08, //report size(8)
    //0x95, 0x40, //report count(64)
    0x95, 0x3f, //report count(63)
    0x82, 0x02, 0x01, //Feature(Data,Var,Abs,Nwrp,Lin,Pref,NNul,NVol,Buf)
    0x09, 0x04, //Usage(Vendor-define 4)
    0x15, 0x00, //logical Minimum(0)
    0x25, 0x01, //logical Maximum(1)
    0x75, 0x08, //report size(1)
    //0x95, 0x40, //report count(64)
    0x95, 0x3f, //report count(63)
    0x91, 0x01, //Input(Cnst,Ary,Abs)
    0xc0,   //End collection
    
    //add for hid send and receive AT command
    0x06, 0x82, 0xff, //Usage Page(Vendor-define 130)
    0x09, 0x01, //Usage(Vendor-define 1)
    0xa1, 0x01, //Collection(Application)
    0x09, 0x03, //Usage(Vendor-define 3)
    0x85, 0x41, //Report ID(65)
    0x15, 0x00, //logical Minimum(0)
    0x26, 0xff, 0x00, //logical Maximum(255)
    0x75, 0x08, //report size(8)
    //0x95, 0x40, //report count(64)
    0x95, 0x3f, //report count(63)
    0x81, 0x02, //Input(Data,Var,Abs,Nwrp,Lin,Pref,NNul,NVol,Buf)
    0x09, 0x04, //Usage(Vendor-define 4)
    0x15, 0x00, //logical Minimum(0)
    0x26, 0xff, 0x00, //logical Maximum(1)
    0x75, 0x08, //report size(8)
    //0x95, 0x40, //report count(64)
    0x95, 0x3f, //report count(63)
    0x91, 0x02, //Output(Cnst,Ary,Abs)
    0xc0,   //End collection

#endif


#if 0
    /*
	 * Report ID: 1
	 */
	/* 0x05, 0x01,	USAGE_PAGE (Generic Desktop) */
	HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,
	/* 0x09, 0x80,	USAGE (System Control) */
	HID_LI_USAGE, 0x80,
	/* 0xa1, 0x01,	COLLECTION (Application) */
	HID_MI_COLLECTION, COLLECTION_APPLICATION,
	/* 0x85, 0x01,	REPORT_ID (1) */
	HID_GI_REPORT_ID, REPORT_ID_1,
	/* 0x19, 0x01,	USAGE_MINIMUM (0x1) */
	0x19, 0x01,
	/* 0x29, 0xb7,	USAGE_MAXIMUM (0xb7) */
	0x29, 0xb7,
	/* 0x15, 0x01,	LOGICAL_MINIMUM (0x1) */
	HID_GI_LOGICAL_MIN(1), 0x01,
	/* 0x26, 0xb7, 0x00,	LOGICAL_MAXIMUM (0xb7) */
	HID_GI_LOGICAL_MAX(2), 0xb7, 0x00,
	/* 0x95, 0x01,	REPORT_COUNT (1) */
	HID_GI_REPORT_COUNT, 0x01,
	/* 0x75, 0x08,	REPORT_SIZE (8) */
	HID_GI_REPORT_SIZE, 0x08,
	/* v0x81, 0x00,	INPUT (Data, Array, Abs) */
	HID_MI_INPUT, 0x00,
	/* 0xc0 ,	END_COLLECTION */
	HID_MI_COLLECTION_END,

	/*
	 * Report ID: 2
	 * Home, Back, Minus, Plus, OK
	 */
	/* 0x05, 0x0c,	USAGE_PAGE (Consumer) */
	HID_GI_USAGE_PAGE, 0x0c,
	/* 0x09, 0x01,	USAGE (Consumer Control) */
	HID_LI_USAGE, 0x01,
	/* 0xa1, 0x01,	COLLECTION (Application) */
	HID_MI_COLLECTION, COLLECTION_APPLICATION,
	/* 0x85, 0x02,	REPORT_ID (2) */
	HID_GI_REPORT_ID, REPORT_ID_2,
	/* 0x19, 0x00,	USAGE_MINIMUM (0x0) */
	0x19, 0x00,
	/* 0x2a, 0x0514,	USAGE_MAXIMUM (0x0514) */
	0x2a, 0x14, 0x05,
	/* 0x15, 0x00,	LOGICAL_MINIMUM (0x0) */
	HID_GI_LOGICAL_MIN(1), 0x00,
	/* 0x26, 0x14, 0x05,	LOGICAL_MAXIMUM (0x0514) */
	HID_GI_LOGICAL_MAX(2), 0x14, 0x05,
	/* 0x95, 0x01,	REPORT_COUNT (1) */
	HID_GI_REPORT_COUNT, 0x01,
	/* 0x75, 0x08,	REPORT_SIZE (16) */
	HID_GI_REPORT_SIZE, 0x10,
	/* v0x81, 0x00,	INPUT (Data, Array, Abs) */
	HID_MI_INPUT, 0x00,
	/* 0xc0 ,	END_COLLECTION */
	HID_MI_COLLECTION_END,
#ifndef CONFIG_BT_TRANSCEIVER
	/*
	 * Report ID: 3
	 */
	/* 0x05, 0x01,	USAGE_PAGE (Generic Desktop) */
	HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,
	/* 0x09, 0x02,	USAGE (Mouse) */
	HID_LI_USAGE, 0x02,
	/* 0xa1, 0x01,	COLLECTION (Application) */
	HID_MI_COLLECTION, COLLECTION_APPLICATION,
	/* 0x85, 0x03,	REPORT_ID (3) */
	HID_GI_REPORT_ID, REPORT_ID_3,
	/* 0x09, 0x01,	USAGE (Pointer) */
	HID_LI_USAGE, 0x01,
	/* 0xa1, 0x00,	COLLECTION (Physical) */
	HID_MI_COLLECTION, 0x00,
	/* 0x05, 0x09,	USAGE_PAGE (Button) */
	HID_GI_USAGE_PAGE, 0x09,
	/* 0x19, 0x01,	USAGE_MINIMUM (0x1) */
	0x19, 0x01,
	/* 0x29, 0x08,	USAGE_MAXIMUM (0x8) */
	0x29, 0x08,
	/* 0x15, 0x00,	LOGICAL_MINIMUM (0x0) */
	HID_GI_LOGICAL_MIN(1), 0x00,
	/* 0x25, 0x01,	LOGICAL_MAXIMUM (0x1) */
	HID_GI_LOGICAL_MAX(1), 0x01,
	/* 0x95, 0x08,	REPORT_COUNT (8) */
	HID_GI_REPORT_COUNT, 0x08,
	/* 0x75, 0x01,	REPORT_SIZE (1) */
	HID_GI_REPORT_SIZE, 0x01,
	/* v0x81, 0x02,	INPUT (Data, Var, Abs) */
	HID_MI_INPUT, 0x02,

	/* 0x05, 0x01,	USAGE_PAGE (Generic Desktop) */
	HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,
	/* 0x09, 0x30,	USAGE (X) */
	HID_LI_USAGE, 0x30,
	/* 0x09, 0x31,	USAGE (Y) */
	HID_LI_USAGE, 0x31,
	/* 0x15, 0x81,	LOGICAL_MINIMUM (0x81) */
	HID_GI_LOGICAL_MIN(1), 0x81,
	/* 0x25, 0x7f,	LOGICAL_MAXIMUM (0x7f) */
	HID_GI_LOGICAL_MAX(1), 0x7f,
	/* 0x75, 0x08,	REPORT_SIZE (8) */
	HID_GI_REPORT_SIZE, 0x08,
	/* 0x95, 0x02,	REPORT_COUNT (2) */
	HID_GI_REPORT_COUNT, 0x02,
	/* v0x81, 0x06,	INPUT (Data, Var, Rel) */
	HID_MI_INPUT, 0x06,

	/* 0x09, 0x38,	USAGE (Wheel) */
	HID_LI_USAGE, 0x38,
	/* 0x15, 0x81,	LOGICAL_MINIMUM (0x81) */
	HID_GI_LOGICAL_MIN(1), 0x81,
	/* 0x25, 0x7f,	LOGICAL_MAXIMUM (0x7f) */
	HID_GI_LOGICAL_MAX(1), 0x7f,
	/* 0x75, 0x08,	REPORT_SIZE (8) */
	HID_GI_REPORT_SIZE, 0x08,
	/* 0x95, 0x01,	REPORT_COUNT (1) */
	HID_GI_REPORT_COUNT, 0x01,
	/* v0x81, 0x06, INPUT (Data, Var, Rel) */
	HID_MI_INPUT, 0x06,
	/* 0xc0 ,	END_COLLECTION */
	HID_MI_COLLECTION_END,
	/* 0xc0 ,	END_COLLECTION */
	HID_MI_COLLECTION_END,

	/*
	 * Report ID: 4
	 * Left, Right, Down, Up, Assist, Menu, On/Off
	 */
	/* 0x05, 0x01,	USAGE_PAGE (Generic Desktop) */
	HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,
	/* 0x09, 0x06,	USAGE (Keyboard) */
	HID_LI_USAGE, 0x06,
	/* 0xa1, 0x01,	COLLECTION (Application) */
	HID_MI_COLLECTION, COLLECTION_APPLICATION,
	/* 0x85, 0x04,	REPORT_ID (4) */
	HID_GI_REPORT_ID, REPORT_ID_4,

	/* 0x05, 0x07,	USAGE_PAGE (Keyboard) */
	HID_GI_USAGE_PAGE, 0x07,
	/* 0x19, 0xe0,	USAGE_MINIMUM (0xe0) */
	0x19, 0xe0,
	/* 0x29, 0xe7,	USAGE_MAXIMUM (0xe7) */
	0x29, 0xe7,
	/* 0x15, 0x00,	LOGICAL_MINIMUM (0x0) */
	HID_GI_LOGICAL_MIN(1), 0x00,
	/* 0x25, 0x01,	LOGICAL_MAXIMUM (0x1) */
	HID_GI_LOGICAL_MAX(1), 0x01,
	/* 0x75, 0x01,	REPORT_SIZE (1) */
	HID_GI_REPORT_SIZE, 0x01,
	/* 0x95, 0x08,	REPORT_COUNT (8) */
	HID_GI_REPORT_COUNT, 0x08,
	/* 0x81, 0x02,	INPUT (Data, Var, Abs) */
	HID_MI_INPUT, 0x02,

	/* Output: LEDs */
	/* 0x95, 0x03,	REPORT_COUNT (3) */
	HID_GI_REPORT_COUNT, 0x03,
	/* 0x75, 0x01,	REPORT_SIZE (1) */
	HID_GI_REPORT_SIZE, 0x01,
	/* 0x05, 0x08,	USAGE_PAGE (LEDs) */
	HID_GI_USAGE_PAGE, 0x08,
	/* 0x19, 0x01,	USAGE_MINIMUM (0x01) */
	0x19, 0x01,
	/* 0x29, 0x03,	USAGE_MAXIMUM (0x03) */
	0x29, 0x03,
	/* v0x91, 0x02,	OUTPUT (Data, Var, Abs) */
	HID_MI_OUTPUT, 0x02,
	/* 0x95, 0x05,	REPORT_COUNT (5) */
	HID_GI_REPORT_COUNT, 0x05,
	/* 0x75, 0x01,	REPORT_SIZE (1) */
	HID_GI_REPORT_SIZE, 0x01,
	/* 0x91, 0x01,	OUTPUT (Cnst, Array, Abs) */
	HID_MI_OUTPUT, 0x01,
	/* 0x95, 0x06,	REPORT_COUNT (6) */
	HID_GI_REPORT_COUNT, 0x06,
	/* 0x75, 0x08,	REPORT_SIZE (8) */
	HID_GI_REPORT_SIZE, 0x08,
	/* 0x15, 0x00,	LOGICAL_MINIMUM (0x0) */
	HID_GI_LOGICAL_MIN(1), 0x00,
	/* 0x26, 0x00ff,	LOGICAL_MAXIMUM (0x00ff) */
	HID_GI_LOGICAL_MAX(2), 0xff, 0x00,
	/* 0x05, 0x07,	USAGE_PAGE (Keyboard) */
	HID_GI_USAGE_PAGE, 0x07,
	/* 0x19, 0x00,	USAGE_MINIMUM (0x00) */
	0x19, 0x00,
	/* 0x2a, 0x00ff,	USAGE_MAXIMUM (0x00ff) */
	0x2a, 0xff, 0x00,
	/* 0x81, 0x00,	INPUT (Data, Array, Abs) */
	HID_MI_INPUT, 0x00,

	/* 0x09, 0x00,	USAGE (Reserved) */
	HID_LI_USAGE, 0x00,
	/* 0x75, 0x08,	REPORT_SIZE (8) */
	HID_GI_REPORT_SIZE, 0x08,
	/* 0x95, 0x14,	REPORT_COUNT (20) */
	HID_GI_REPORT_COUNT, 0x14,
	/* 0xb1, 0x00,	FEATURE (Data, Array, Abs) */
	0xb1, 0x00,
	/* 0xc0 ,	END_COLLECTION */
	HID_MI_COLLECTION_END,
#endif
	#if CONFIG_USB_SELF_DEFINED_REPORT
	/*
	 * Report ID: 5
	 * User-defined: 63-byte input/output
	 */
	/* 0x06, 0xff01,	USAGE_PAGE (Undefined) */
	0x06, 0x01, 0xff,
	/* 0x09, 0x00,	USAGE (0) */
	HID_LI_USAGE, 0x0,
	/* 0xa1, 0x80,	COLLECTION (Reserved) */
	HID_MI_COLLECTION, 0x80,
	/* 0x85, 0x05,	REPORT_ID (5) */
	HID_GI_REPORT_ID, 5,
	/* 0x15, 0x00,	LOGICAL_MINIMUM (0x0) */
	HID_GI_LOGICAL_MIN(1), 0x00,
	/* 0x26, 0x00ff,	LOGICAL_MAXIMUM (0x00ff) */
	HID_GI_LOGICAL_MAX(2), 0xff, 0x00,
	/* 0x75, 0x08,	REPORT_SIZE (8) */
	HID_GI_REPORT_SIZE, 0x08,
	/* 0x95, 0x3f,	REPORT_COUNT (63) */
	HID_GI_REPORT_COUNT, 0x3f,
	/* 0x09, 0x00,	USAGE (0) */
	HID_LI_USAGE, 0x0,
	/* 0x81, 0x02,	INPUT (Data, Var, Abs) */
	HID_MI_INPUT, 0x02,
	/* 0x09, 0x00,	USAGE (0) */
	HID_LI_USAGE, 0x0,
	/* 0x91, 0x02,	OUTPUT (Data, Var, Abs) */
	HID_MI_OUTPUT, 0x02,
	/* 0xc0 ,	END_COLLECTION */
	HID_MI_COLLECTION_END,
	#endif

	/*
	 * Report ID: 6
	 * USB Audio Volume Control
	 */
	0x05, 0x0c, /* USAGE_PAGE (Consumer) */
	0x09, 0x01, /* USAGE (Consumer Control) */
	0xa1, 0x01, /* COLLECTION (Application) */
	0x85, REPORT_ID_6,/* Report ID*/
	0x15, 0x00, /* Logical Minimum (0x00) */
	0x25, 0x01, /* Logical Maximum (0x01) */
	0x09, 0xe9, /* USAGE (Volume Up) */
	0x09, 0xea, /* USAGE (Volume Down) */
	0x09, 0xe2, /* USAGE (Mute) */
	0x09, 0xcd, /* USAGE (Play/Pause) */
	0x09, 0xb5, /* USAGE (Scan Next Track) */
	0x09, 0xb6, /* USAGE (Scan Previous Track) */
	0x09, 0xb3, /* USAGE (Fast Forward) */
	0x09, 0xb7, /* USAGE (Stop) */
	0x75, 0x01, /* Report Size (0x01) */
	0x95, 0x08, /* Report Count (0x08) */
	0x81, 0x42, /* Input() */
	0xc0,	/* END_COLLECTION */

	/*
	 * Report ID: 7
	 * Telephone Status
	 */
	0x05, 0x0b, /* Usage Page(Telephony ) */
	0x09, 0x01, /* Usage(Phone) */
	0xa1, 0x01, /* Collection(Application ) */
	0x85, TELEPHONE_REPORT_ID,	/* Report ID() */
	0x15, 0x00, /* Logical Minimum (0x00) */
	0x25, 0x01, /* Logical Maximum (0x01) */
	0x09, 0x20, /* USAGE (Hook Switch) */
	0x09, 0x21, /* Usage(Flash) */
	0x09, 0x2F, /* Usage(Phone Mute) */
	0x75, 0x01, /* Report Size(0x1 ) */
	0x95, 0x03,	/* Report Count(0x3 ) */
	0x81, 0x02, /* Input (Data,Variable,Absolute) */
	0x75, 0x05, /* Report Size(0x5) */
	0x95, 0x01, /* Report Count(0x1) */
	0x81, 0x03, /* Input (Cnst,Variable,Absolute) */
	0x05, 0x08, /* Usage Page(LEDs) */
	0x15, 0x00, /* Logical Minimum(0x0) */
	0x25, 0x01, /* Logical Maximum(0x1) */
	0x09, 0x08,	/* USAGE (Do Not Disturb):0x01 */
	0x09, 0x17, /* USAGE (Off-Hook):0x02 */
	0x09, 0x18, /* USAGE (Ring):0x04 */
	0x09, 0x19, /* USAGE (Message Waiting):0x08 */
	0x09, 0x1a, /* USAGE (Data Mode):0x10 */
	0x09, 0x1e, /* USAGE (Speaker):0x20 */
	0x09, 0x1f, /* USAGE (Head Set):0x40 */
	0x09, 0x20, /* USAGE (Hold):0x80 */
	0x09, 0x21, /* USAGE (Microphone):0x0100 */
	0x09, 0x22, /* USAGE (Coverage):0x0200 */
	0x09, 0x23, /* USAGE (Night Mode):0x0400 */
	0x09, 0x24, /* USAGE (Send Calls):0x0800 */
	0x09, 0x25, /* USAGE (Call Pickup):0x1000 */
	0x09, 0x26, /* USAGE (Conference):0x2000 */
	0x09, 0x09, /* Usage(Mute):0x4000 */
	0x75, 0x01, /* Report Size (0x01) */
	0x95, 0x0f, /* Report Count (0x0f) */
	0x91, 0x02, /* Output()*/
	0x75, 0x01, /* Report Size (0x01) */
	0x95, 0x01, /* Report Count (0x01) */
	0x91, 0x03, /* Output (Const,Variable,Absolute) */
	0xc0, /* END_COLLECTION */
#endif
   
    //actions 自定义描述符
    0xa1, 0x01, //COLLECTION (Application)
    0x15, 0x00, //Logical Minimum (0x00)
    0x25, 0x01, //Logical Maximum (0x01)
    0x09, 0x08,
    0x09, 0x0f,
    0x09, 0x18,
    0x09, 0x1f,
    0xc0, //END_COLLECTION           //END_COLLECTION

};

/*
 * according to USB HID report descriptor.
 */
#define PLAYING_VOLUME_INC  	0x01
#define PLAYING_VOLUME_DEC  	0x02
#define PLAYING_VOLUME_MUTE  	0x04
#define PLAYING_AND_PAUSE   	0x08
#define PLAYING_NEXTSONG    	0x10
#define PLAYING_PREVSONG    	0x20

//set report telephone status
#define TELEPHONE_HOOK_SWITCH_STATE     (1<<1)
#define TELEPHONE_RING                  (1<<2)
#define TELEPHONE_MUTE_STAT             (1<<6)

#define TELEPHONE_HOOK_SWITCH   ((0x01 << 8) | TELEPHONE_REPORT_ID1) //接听电话，没有足够按键，暂时没有调用
#define TELEPHONE_HOOK_SWITCH_LINE_BUSY	((0x03<<8)|TELEPHONE_REPORT_ID1)  //去电忙等待
#define TELEPHONE_HOOK_SWITCH_END   ((0x00 << 8) | TELEPHONE_REPORT_ID1) //接听电话，没有足够按键，暂时没有调用
#define TELEPHONE_FLASH         ((0x01 << 16) | TELEPHONE_REPORT_ID1) //挂断电话，没有足够按键，暂时没有调用
//#define TELEPHONE_MUTE          ((0x02 << 16) | TELEPHONE_REPORT_ID1) //电话静音，没有足够按键，暂时没有调用
#define TELEPHONE_MUTE1          ((0x02 << 16) | TELEPHONE_REPORT_ID1 |(0x03 <<8)) //电话静音，没有足够按键，暂时没有调用
#define TELEPHONE_MUTE2          (TELEPHONE_REPORT_ID1 |(0x03 <<8)) //电话静音，没有足够按键，暂时没有调用
#define TELEPHONE_MUTE3          ((0x02 << 16) | TELEPHONE_REPORT_ID1 |(0x00 <<8)) //电话静音，没有足够按键，暂时没有调用
#define TELEPHONE_MUTE4          (TELEPHONE_REPORT_ID1 |(0x00 <<8)) //电话静音，没有足够按键，暂时没有调用
#define TELEPHONE_BUSY          ((0x02)<<8 | TELEPHONE_REPORT_ID1)  //忙等待
#define TELEPHONE_BUTTON        ((0x80<<16 ) | TELEPHONE_REPORT_ID1)     //辅助按键

#define INCALL_STATUS_NULL          0x00
#define INCALL_STATUS_RING          0x01
#define INCALL_STATUS_ACTIVE        0x02
#define INCALL_STATUS_OUTCALL       0x03

typedef void (*System_call_status_flag)(u32_t status_flag);

void usb_audio_register_call_status_cb(System_call_status_flag cb);
void usb_hid_answer(void);
void usb_hid_hung_up(void);
void usb_hid_mute(void);
void usb_hid_refuse_phone(void);

int usb_hid_dev_init(void);
int usb_hid_control_pause_play(void);
int usb_hid_control_volume_dec(void);
int usb_hid_control_volume_inc(void);
int usb_hid_control_play_next(void);
int usb_hid_control_play_prev(void);

int usb_hid_device_exit(void);

#endif /* __HID_HANDLER_H_ */

