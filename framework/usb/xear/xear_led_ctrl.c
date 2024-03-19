#include <os_common_api.h>
#include <kernel.h>
#include <string.h>
#include <property_manager.h>
#include <btservice_api.h>
#include <api_common.h>
#include <usb/class/usb_hid.h>
#include <usb/usb_custom_handler.h>
#include <xear_led_ctrl.h>
//#include <dt-bindings/pwm/pwm.h>
#include <audio_system.h>
#include <volume_manager.h>
#include <drivers/pwm.h>
#include <board.h>
#include <tr_app_switch.h>


//#include <logging/sys_log.h>
#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "xear"
#endif

#define XEAR_LED_CTRL "XEAR_LED_CTRL"
#define LED_DATA_TAG  "LED"

#ifndef int8
typedef signed char int8;
#endif

#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef int16
typedef signed short int16;
#endif


extern int tr_bt_manager_ble_cli_send(uint16_t handle, u8_t * buf, u16_t len ,s32_t timeout);
extern uint16_t leaudio_playback_energy_level(void);

#define MSB(word) 		(u8_t)(((word) >> 8) & 0x00FF)
#define LSB(word) 		(u8_t)(((word)&0x00FF))

enum {
	HID_LED_CONTROL_START = 1,
	HID_LED_CONTROL_STOP,
	HID_LED_CONTROL_CONFIG,
	HID_LED_CONTROL_TABLE,

	LED_MODE_STATIC = 0,
	LED_MODE_REPEAT_FORWARD,
	LED_MODE_BACK_AND_FORTH,
	LED_MODE_LOOK_UP_TABLE,
	LED_MODE_END,

	LED_COLOR_ARRAY_MAX = 128,

	LED_STATE_PLAY = 0,
	LED_STATE_IDLE,
	LED_STATE_SAVE_DATA,

	LED_PWM_R = 0,
	LED_PWM_G = 1,
	LED_PWM_B = 2,
}; 

static struct {
	int8 tag[3];
	uint8 opMode;
	uint8 numOfColor;
	uint8 interval;  // in 10ms
	uint8 rgbTbl[LED_COLOR_ARRAY_MAX*3];
} led_ctrl_data;

static uint8 led_state;
static uint16 led_color_idx;
static bool led_backward;
static struct k_delayed_work led_work;
static struct k_delayed_work hid_work;

u8_t GetPlaybackLevel()
{
#ifdef CONFIG_LE_AUDIO_DISPLAY_TIME_ENERGE_DETECT
	u8_t volume = system_volume_get(AUDIO_STREAM_LE_AUDIO);	// volume: 0 ~ 16
	u32_t level = leaudio_playback_energy_level();

	//SYS_LOG_INF("volume(%d) level(0x%04x)(0x%04x)\n", volume, level, level*volume);

	level = level * volume;

	return (level >> 8);
#else
	return 0;
#endif	
}

static void led_work_handle(struct k_work *work)
{
	if( led_state == LED_STATE_SAVE_DATA )
	{
		property_set(XEAR_LED_CTRL, (char*)&led_ctrl_data, sizeof(led_ctrl_data));
		property_flush(XEAR_LED_CTRL);

		led_state = LED_STATE_PLAY;
		led_color_idx = 0;
		led_backward = false;
	}

	if( led_state == LED_STATE_PLAY )
	{
		const struct device *dev_pwm = device_get_binding(CONFIG_PWM_NAME);

		switch(led_ctrl_data.opMode)
		{
		case LED_MODE_STATIC:
			led_color_idx = 0;
			led_state = LED_STATE_IDLE;
			break;

		case LED_MODE_REPEAT_FORWARD:
			if(led_color_idx == led_ctrl_data.numOfColor)
				led_color_idx = 0;
			break;

		case LED_MODE_BACK_AND_FORTH:
			if((led_color_idx==led_ctrl_data.numOfColor) && (!led_backward))
			{
				led_color_idx--;
				led_backward = true;
			}
			else if((led_color_idx==0) && led_backward)
			{
				led_backward = false;
			}
			break;

		case LED_MODE_LOOK_UP_TABLE:
			led_color_idx = GetPlaybackLevel();
			if(led_color_idx > 127)
			{
				led_color_idx = 127;
			}
			led_color_idx = (127-led_color_idx);
			break;
		}

		pwm_pin_set_duty(dev_pwm, LED_PWM_R, led_ctrl_data.rgbTbl[led_color_idx*3 + 0]);
		pwm_pin_set_duty(dev_pwm, LED_PWM_G, led_ctrl_data.rgbTbl[led_color_idx*3 + 1]);
		pwm_pin_set_duty(dev_pwm, LED_PWM_B, led_ctrl_data.rgbTbl[led_color_idx*3 + 2]);

		switch(led_ctrl_data.opMode)
		{
		case LED_MODE_REPEAT_FORWARD:
			led_color_idx++;
			break;

		case LED_MODE_BACK_AND_FORTH:
			if( !led_backward )
				led_color_idx++;
			else
				led_color_idx--;
			break;

		case LED_MODE_STATIC:
			return;
		}
		os_delayed_work_submit(&led_work, led_ctrl_data.interval*10);
	}
}

static void hid_work_handle(struct k_work *work)
{
	char buf[64];
	struct conn_status status;

	API_APP_CONNECTION_CONFIG(0, &status);

	SYS_LOG_INF("status.connected(%d)\n", status.connected);

	if( status.connected )
	{
		uint8 len = (led_color_idx+50) < sizeof(led_ctrl_data) ? 50 : (sizeof(led_ctrl_data)-led_color_idx);

		memset( buf, 0, sizeof(buf) );
		buf[0] = 0x88;
		buf[1] = 4+len;  // len(lsb)

		buf[2] = HID_REPORT_ID_LED_CTRL;
		buf[3] = (led_color_idx & 0xFF00) >> 8;
		buf[4] = led_color_idx & 0x00FF;
		buf[5] = len;

		memcpy(buf+6, (char*)&led_ctrl_data + led_color_idx, len);


		SYS_LOG_INF("\n[0]=%x [1]=%x [2]=%x\n",buf[0],buf[1],buf[2]);

		tr_bt_manager_ble_cli_send(status.handle, buf, 64, 100);

		led_color_idx += len;

		/*
		memset( buf, 0, sizeof(buf) );
		buf[0] = 0x56;
		buf[1] = 0;  // len(msb)
		buf[2] = 4+len;  // len(lsb)

		buf[3] = HID_REPORT_ID_LED_CTRL;
		buf[4] = (led_color_idx & 0xFF00) >> 8;
		buf[5] = led_color_idx & 0x00FF;
		buf[6] = len;

		memcpy(buf+7, (char*)&led_ctrl_data + led_color_idx, len);

		len = tr_bt_manager_ble_cli_send(status.handle, buf, buf[2]+3, 100);
		
		if(  len > 7 )
		{
			// first 7 bytes are packet header, not real data..
			led_color_idx += (len-7);
		}
		*/
		SYS_LOG_INF("len(%d) led_color_idx(%d)\n", len, led_color_idx);

		if( led_color_idx < sizeof(led_ctrl_data) )
			os_delayed_work_submit(&hid_work, 10);
	}
}

void xear_led_ctrl_init(void)
{
	if( property_get(XEAR_LED_CTRL, (char*)&led_ctrl_data, sizeof(led_ctrl_data)) >= 0 )
	{
		if ( ! memcmp(led_ctrl_data.tag, LED_DATA_TAG, sizeof(led_ctrl_data.tag)) )
		{
			goto exit;
		}
	}

	SYS_LOG_INF("no xear led data available\n");
	led_ctrl_data.opMode = LED_MODE_STATIC;
	led_ctrl_data.numOfColor=1;
	led_ctrl_data.interval=0;
	led_ctrl_data.rgbTbl[0]=0x00;
	led_ctrl_data.rgbTbl[1]=0x00;
	led_ctrl_data.rgbTbl[2]=0x00;

exit:
	led_state = LED_STATE_PLAY;
	led_color_idx = 0;
	led_backward = false;

	if( sys_get_work_mode() == RECEIVER_MODE )
	{
		const struct device *dev_pwm = device_get_binding(CONFIG_PWM_NAME);

		pwm_pin_set_cycles(dev_pwm, LED_PWM_R, 0xff, 0, 1);
		pwm_pin_set_cycles(dev_pwm, LED_PWM_G, 0xff, 0, 1);
		pwm_pin_set_cycles(dev_pwm, LED_PWM_B, 0xff, 0, 1);

		os_delayed_work_init(&led_work, led_work_handle);
		os_delayed_work_submit(&led_work, 0);
	}
	else
	{
		os_delayed_work_init(&hid_work, hid_work_handle);
	}
}

/*
 *  return 0, if succeeded; otherwise, failed.
 *  for led control commad coming from output endpoint of HID interface.
 */
int xear_led_ctrl_cmd_handler2(u8_t *buf, u8_t len)
{
	u16_t tmp;

	SYS_LOG_INF("cmd(%d)\n", buf[1]);

	if(buf[1] == HID_LED_CONTROL_START)
	{
		led_color_idx = 0;

		// add tag
		memcpy(led_ctrl_data.tag, LED_DATA_TAG, sizeof(led_ctrl_data.tag));

		os_delayed_work_submit(&hid_work, 0);
	}
	else if(buf[1] == HID_LED_CONTROL_STOP)
	{
		// clear tag
		memset(led_ctrl_data.tag, 0, sizeof(led_ctrl_data.tag));
	}
	else if(buf[1] == HID_LED_CONTROL_CONFIG)
	{
		if(buf[2] >= LED_MODE_END)
			return -1;
		else
			led_ctrl_data.opMode = buf[2];

		tmp = buf[3] | ((uint16)buf[4] << 8);
		if(tmp > LED_COLOR_ARRAY_MAX)
			return -2;
		else
			led_ctrl_data.numOfColor = tmp;

		tmp = buf[5] | ((uint16)buf[6] << 8);
		if(tmp < 10)
			tmp = 10;
		led_ctrl_data.interval = tmp/10;//in 10ms
	}
	else if(buf[1] == HID_LED_CONTROL_TABLE)
	{
		u8_t numOfColor;

		tmp = buf[2] | ((uint16)buf[3] << 8);
		if(tmp >= LED_COLOR_ARRAY_MAX)
			return -2;

		numOfColor = (LED_COLOR_ARRAY_MAX-tmp) < 4 ? (LED_COLOR_ARRAY_MAX-tmp) : 4;
		memcpy( &(led_ctrl_data.rgbTbl[3*tmp]), buf+4, 3*numOfColor );
	}

	return 0;
}

/*
 *  return 0, if succeeded; otherwise, failed.
 *  for led control commad coming from control endpoint
 */
int xear_led_ctrl_cmd_handler(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data)
{
	if( LSB(pSetup->wValue) == HID_REPORT_ID_LED_CTRL )
	{
		SYS_LOG_INF("bRequest(0x%x) wValue(0x%x)\n", pSetup->bRequest, pSetup->wValue);

		if( (pSetup->bRequest == HID_SET_REPORT) && (MSB(pSetup->wValue)==HID_REPORT_OUTPUT) )
		{
			if( ! xear_led_ctrl_cmd_handler2(*data, *len) )
				return 0;
		}
	}

	return -ENOTSUP;
}

/*
 * buf[] format:
 *          0: 0xFF (HID_REPORT_ID_LED_CTRL)
 *      1~2: data offset (big-endian)
 *          3: data len
 *       4~: data
 */
void xear_led_ctrl_rcv_data_handler(u8_t *buf)
{
	if( led_state == LED_STATE_PLAY )
	{
		os_delayed_work_cancel(&led_work);
		led_state = LED_STATE_IDLE;
	}

	led_color_idx = ((uint16)buf[1] << 8 ) | buf[2];
	memcpy( (char*)&led_ctrl_data + led_color_idx, buf+4, buf[3] );

	SYS_LOG_INF("offset(%d) len(%d)\n", led_color_idx, buf[3]);

	if( (led_color_idx + buf[3]) == sizeof(led_ctrl_data) )
	{
		led_state = LED_STATE_SAVE_DATA;

		os_delayed_work_submit(&led_work, 0);
	}
}

int xear_led_ctrl_dummy_handler(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data)
{
	// for xear led control App to start ....
	if( pSetup->bmRequestType == 0xC3 )
	{
		if( pSetup->bRequest == 0x02 )
		{
			// just return any data is fine.

			*data = (u8_t*)&led_ctrl_data;
			*len = 16;
			return 0;
		}
	}

	return -ENOTSUP;
}

void xear_led_ctrl_sleep(void)
{
	const struct device *dev_pwm = device_get_binding(CONFIG_PWM_NAME);

	os_delayed_work_cancel(&led_work);
	led_state = LED_STATE_IDLE;

	pwm_pin_set_duty(dev_pwm, LED_PWM_R, 0);
	pwm_pin_set_duty(dev_pwm, LED_PWM_G, 0);
	pwm_pin_set_duty(dev_pwm, LED_PWM_B, 0);
}

void xear_led_ctrl_wake_up(void)
{
	led_state = LED_STATE_PLAY;
	os_delayed_work_submit(&led_work, 0);
}
