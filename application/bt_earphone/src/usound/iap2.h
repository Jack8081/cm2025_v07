#include <i2c.h>
#include <mem_manager.h>
#include <thread_timer.h>
#include <string.h>
#include <zephyr.h>

#define sys_cpu_to_le16(x) ((u16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))
#define IAP2_CACHE_LEN	120

#define IAP2_CONTROL_VERSION_1				0x1
#define IAP2_CONTROL_VERSION_2				0x2

#define CONFIG_USB_IAP2	"iAP Interface"
#ifdef CONFIG_USB_DEVICE_SN
//#undef CONFIG_USB_DEVICE_SN
#define CONFIG_USB_DEVICE_SN_IAP "XSP12345678"
#endif

#define IAP2_MODEID	"v1.0.0"
#define IAP2_DEVICE_NAME	 CONFIG_USB_APP_AUDIO_DEVICE_PRODUCT
#define IAP2_MANUFATORY		CONFIG_USB_APP_AUDIO_DEVICE_MANUFACTURER
#define IAP2_SERIAL_NAME	CONFIG_USB_APP_AUDIO_DEVICE_SN
#define IAP2_FIRMWAREVERSION	"v1.0.0.1"
#define IAP2_HARDWAREVERSION	"v2.0.0.1"
#define IAP2_UHOST_COMPONENT	"USBHostTransportComponent"
#define IAP2_HID_COMPONENT		"HIDComponent"

#define SESSION_ID_START_USB_DEV_MODE_AUDIO	0xDA00
#define SESSION_ID_USB_DEV_MODE_AUDIO_INF	0xDA01
#define SESSION_ID_STOP_USB_DEV_MODE_AUDIO	0XDA02
#define SESSION_ID_REQUESTAUTHCERTIFICATE	0xAA00
#define SESSION_ID_AUTHENTICATION_CERTIFICATE	0xAA01
#define SESSION_ID_REQUEST_AUTH_CHALLENG_RESP	0xAA02
#define SESSION_ID_AUTH_RESPONSE			0xAA03
#define SESSION_ID_AUTH_FAIL				0xAA04
#define SESSION_ID_AUTH_SUCCEDD				0xAA05
#define SESSION_ID_ACCESSORY_AUTH_SERIAL_NUM	0xAA06

#define SESSION_ID_START_IDENTIFICATION		0x1D00
#define SESSION_ID_IDENTIFICATION_INFO		0x1D01
#define SESSION_ID_IDEN_ACCEPTED			0x1D02
#define SESSION_ID_IDEN_REJECTED			0x1D03
#define SESSION_ID_CANCELL_IDEN				0x1D05
#define SESSION_ID_IDEN_UPDATE				0x1D06

#define SESSION_SYNC_ID_1					0x0a
#define SESSION_SYNC_ID_2					0x0b

#define USB_HOST_DEVICE_CLASS				0xEF
#define USB_HOST_DEVICE_SUB_CLASS			0x02
#define USB_HOST_DEVICE_PROTOCOL			0x01
#define USB_HOST_INTERFACE_CLASS			0xFF   //vendor define
#define USB_HOST_INTERFACE_SUB_CLASS		0xF0	//MFI accessory
#define USB_HOST_INTERFACE_PROTOCOL			0x00
#define USB_HOST_INTERFACE_STRING			"iAP Interface"
#define USB_HOST_NUM_OF_ENDPOINTS			2

#define LINK_CONTROL_SYN	BIT(7)
#define LINK_CONTROL_ACK	BIT(6)
#define LINK_CONTROL_EAK	BIT(5)
#define LINK_CONTROL_RST	BIT(4)
#define LINK_CONTROL_SLP	BIT(3)

#define IAP2_SESSION_TYPE_CONTROL	0x0
#define IAP2_SESSION_TYPE_FILE_TRANS	0x1
#define IAP2_SESSION_TYPE_EXT_ACC_PRO	0x2

#define IOS_UAC_PID	0x1200
#define IOS_UAC_VID	0x05AC

#define CP_I2C_WR	0x20>>1
#define CP_I2C_RR	0x21>>1

#define CP_DEVICE_VERSION			0x00
#define CP_AUTHENTICATION_RIVISON	0x01
#define CP_AUTHENTICATION_MAJOR_VER	0x02
#define CP_AUTHENTICATION_MIN_VER	0x03
#define CP_DEVICE_ID				0x04
#define CP_ERROR_CODE				0x05
#define CP_AUTHENTICATION_CONTROL_STATUES		0x10
#define CP_CHALLENGE_RESPONSE_DATA_LEN			0x11
#define CP_CHALLENGE_RESPONSE_DATA				0x12
#define CP_CHALLENGE_DATA_LEN					0x20
#define CP_CHALLENGE_DATA						0x21
#define CP_ACCESSORY_CERTIFICATION_DATA_LEN		0x30
#define CP_ACCESSORY_CERTIFICATION_DATA_1		0x31
#define CP_ACCESSORY_CERTIFICATION_DATA_2		0x32
#define CP_ACCESSORY_CERTIFICATION_DATA_3		0x33
#define CP_ACCESSORY_CERTIFICATION_DATA_4		0x34
#define CP_ACCESSORY_CERTIFICATION_DATA_5		0x35
#define CP_SELF_TEST_STATUS						0x40
#define CP_DEV_CERTIFICATION_SERIAL_NUM			0x4e
#define CP_SLEEP								0x60

#define IAP2_CONTROL_EP_BULK_IN		0x83
#define IAP2_CONTROL_EP_BULK_OUT	0x01
#define IAP2_CONTROL_EP_INTE_IN		0x83

#define IAP2_LINK_PKT_LEN sizeof(struct iap2_session_link_packet)

enum{
	INIT,
	LINK_ESTABLISH,
	DEETCET_DEV,
	SYNC,
	IDENTIF,
	DISCONNECT,
}iap2_mac_status;
enum{
	NONE,
	LIGHTING_RECEPTCABLE_PASSTHROUGH,
	ADVANCED,
}PowerprovidingCap;
enum{
	NO_ACTION,
	OPTIONAL_ACTION,
	NO_ALERT,
	NO_COMMUICATION_ACTION,
}MatchAction;
enum{
	SAMPLE_8K_HZ,
	SAMPLE_11025_HZ,
	SAMPLE_12K_HZ,
	SAMPLE_16K_HZ,
	SAMPLE_22050_HZ,
	SAMPLE_24K_HZ,
	SAMPLE_32K_HZ,
	SAMPLE_44100_HZ,
	SAMPLE_48K_HZ,
}USBDevAudioSampleRate;
enum{
	KEYBOARD,
	MEDIA_PLAYBACK_REMOTE,
	ASSISTIVE_TOUCH_POINTER,
	RESERVED_1,
	GAMEPAD_FROM_FITTING,
	RESERVED_2,
	GAMEPAD_NO_FROM_FITTING,
	ASSISTIVE_SWITCH_CONTROL,
	HEADSET,
}HidCompentFunction;
typedef struct{
	u8_t ExtAccessoryProtocolID;
	u8_t ExtAccessoryProtocolName;
	u8_t ExtAccessoryProtocolMatchAction;
	u16_t NativeTransportCompentID;
}SupportExtAccessory;
typedef struct{
	u16_t transportComID;
	u8_t transportComName;
	u32_t transportSupportiap2conn;
}UartTransportCompent;
typedef struct{
	u16_t transportComID;
	u8_t transportComName;
	u32_t transportSupportiap2conn;
	u8_t usbdevSupportedAudioSamp;
}UsbDeviceCompent;
typedef struct{
	u16_t len;
	u16_t transportComID;
	u8_t transportComName;
	u32_t transportSupportiap2conn;
}UsbHostCompent;
typedef struct{
	u16_t transportComID;
	u8_t transportComName;
	u32_t transportSupportiap2conn;
	u8_t BluetoothTransportControlAddr[6];
}BluetoothTransportCompent;
typedef struct{
	u16_t HidCompID;
	u8_t HidcompName;
	u8_t HidCompfunc;
}iAP2HIDCompent;
typedef struct{
	u16_t Identifier;
	u8_t name;
	u32_t globalpositionsysfixdata;
	u32_t recommendminspecificGPStransdata;
	u32_t vehicleSpeeddata;
}LocationInfoCompent;
typedef struct{
	u16_t len;
	u16_t Identifier;
	u8_t name;
	u8_t HidCompfunc;
	u16_t UsbHostTransportComId;
	u16_t UsbHostTransportName;
}UsbHostHidCompent;
typedef struct{
	u16_t Identifier;
	u8_t name;
	u8_t HidCompfunc;
	u16_t BluetoothTransportComId;
}BluetoothHidCompent;
struct Iap2_message_str{
	u16_t sofMessage;	//0x4040
	u16_t messageLen;
	u16_t messageId;
	u16_t ParamLen;
	u16_t ParamID;
	u8_t *paramData;
}__packed;
struct lap2_message_param_str{
	u16_t len;
	u16_t id;
	u8_t *data;
}__packed;
typedef struct identificationInfo{
	struct lap2_message_param_str name;
	struct lap2_message_param_str modeID;
	struct lap2_message_param_str manufacurer;
	struct lap2_message_param_str serialNum;
	struct lap2_message_param_str firmwareversion;
	struct lap2_message_param_str hardwareversion;
	struct lap2_message_param_str messagesendbyAcessory;
	struct lap2_message_param_str messagereceivefromDevice;
	struct lap2_message_param_str powerprovidingCap;
	struct lap2_message_param_str MaxCurrentDrawnFromDev;
	//SupportExtAccessory extAccessory;
	//u8_t AppMatchTeamID;
	struct lap2_message_param_str CurrentLanguage;
	struct lap2_message_param_str SupportedLanguage;
	//UartTransportCompent uarttrancom;
	//UsbDeviceCompent usbdevcomp;
	UsbHostCompent usbhostcomp;
	//BluetoothTransportCompent bluetoothcomp;
	//iAP2HIDCompent iap2hidcomp;
	//LocationInfoCompent localinfocomp;
	UsbHostHidCompent usbhosthidcomp;
	//BluetoothHidCompent bluetoothhidcomp;
}iap2_identificationInfo;

//typedef struct {
struct iap2_link_sync_payload{
	u8_t link_version;
	u8_t maxnumoutstandingpacket;
	u16_t maxnumreceviedpacketlen;
	u16_t retransmit_timeout;
	u16_t Cumulativ_ack_Timeout;
	u8_t maxnumretransmit;
	u8_t maxcumulativackNum;

	u8_t sessionID1;
	u8_t sessionType1;
	u8_t session_ver1;

	u8_t sessionID2;
	u8_t sessionType2;
	u8_t session_ver2;

	u8_t payload_crc;
//}iap2_link_sync_payload __packed;
} __packed;
//typedef struct{
struct iap2_session_link_packet{
	u16_t startofPacket;
	u16_t PacketLength;
	u8_t controlByte;
	u8_t PacketSeqNum;
	u8_t PacketAckNum;
	u8_t SessionID;
	u8_t headercrc;
	//u8_t *PayloadData;
	//u8_t Payloadcrc;
//}iap2_session_link_packet __packed;
} __packed;

struct iap2_link_sync_packet{
	struct iap2_session_link_packet link_packet;
	struct iap2_link_sync_payload link_sync_payload;

}__packed;
typedef struct{
	struct iap2_session_link_packet link_packet;

}iap2_session_control;
typedef struct{
	struct iap2_session_link_packet link_packet;
	void *payload;
}iap2_session_ext_accessory;
typedef struct{
	struct iap2_session_link_packet link_packet;

}iap2_hid_s;

typedef struct{
	void *handle;
	u8_t* buffer;
	u32_t sendAcktimer;
	u16_t NextSentPSN;
	u16_t OldestSentUnackPSN;
	u16_t InitialSentPSN;
	u16_t LastReceivedInSeqPSN;
	u16_t InitialRecevPSN;
	u16_t ReceivedOutOfSeqPSN;

	u8_t maxoutstandingpacket;
	u16_t maxrevpacketlen;
	u16_t retransmissiontimeout;
	u16_t cumulativeacktimeout;
	u8_t maxnumofretransmission;
	u8_t maxcumulativeack;

	u8_t sync_seq;
	u8_t ack_seq;
	u8_t session_id;

	struct thread_timer link_timer_handle;

	u8_t status;

	u32_t link_dete_time;

	u8_t *iap2_thread_stack;
	u8_t iap2_thread_exit;

	struct k_sem read_sem;
	struct k_sem write_sem;

	struct device * act_i2c;
}iap2;

extern int usb_write(u8_t ep, const u8_t *data, u32_t data_len,
		u32_t *bytes_ret);
extern int usb_read(u8_t ep, u8_t *data, u32_t max_data_len,
		u32_t *ret_bytes);
extern int usb_dc_ep_flush(const u8_t ep);


