/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hotplug usb interface
 */
#include <usb/usb_device.h>
#ifdef CONFIG_USB_HOST
#include <usb/usb_host.h>
#endif
#include <drivers/usb/usb_phy.h>

#include "../hotplug_manager.h"
#include "../../sys_app_defines.h"
#include "hotplug_usb.h"

#define LOG_LEVEL LOG_LEVEL_WRN
#include <logging/log.h>
LOG_MODULE_REGISTER(hotplug_usb);

#define USB_HOTPLUG_NONE	0
/* host mode: peripheral device connected */
#define USB_HOTPLUG_A_IN	1
/* host mode: peripheral device disconnected */
#define USB_HOTPLUG_A_OUT	2
/* device mode: connected to host */
#define USB_HOTPLUG_B_IN	3
/* device mode: disconnected from host */
#define USB_HOTPLUG_B_OUT	4
/* device mode: connected to charger */
#define USB_HOTPLUG_C_IN	5
/* device mode: disconnected from charger */
#define USB_HOTPLUG_C_OUT	6
/* usb hotplug suspend: stop detection */
#define USB_HOTPLUG_SUSPEND	7

static u8_t usb_hotplug_state;

static char *state_string[] = {
	"undefined",
	"b_idle",
	"b_srp_init",
	"b_peripheral",
	"b_wait_acon",
	"b_host",
	"a_idle",
	"a_wait_vrise",
	"a_wait_bcon",
	"a_host",
	"a_suspend",
	"a_peripheral",
	"a_wait_vfall",
	"a_vbus_err"
};

static u8_t otg_state;

#ifdef CONFIG_USB_DEVICE
/* distinguish pc/charger: nearly 10s */
#define USB_CONNECT_COUNT_MAX	(10000 / CONFIG_MONITOR_PERIOD)
static u16_t usb_connect_count;

/* optimize: if no_plugin */
#define KEEP_IN_B_IDLE_RETRY	10
static u8_t keep_in_b_idle;

/* doing b_peripheral exit process */
static u8_t otg_b_peripheral_exiting;
#endif /* CONFIG_USB_DEVICE */

#ifdef CONFIG_USB_HOST
/* timeout: nearly 1s */
#define OTG_A_WAIT_BCON_MAX	(1000 / CONFIG_MONITOR_PERIOD)
static u8_t otg_a_wait_bcon_count;

/* timeout: nearly 4s */
#define OTG_A_IDLE_MAX	(4000 / CONFIG_MONITOR_PERIOD)
static u8_t otg_a_idle_count;

/* doing a_host exit process */
static u8_t otg_a_host_exiting;
#endif /* CONFIG_USB_HOST */

#ifdef CONFIG_USB_HOTPLUG_THREAD_ENABLED
#define STACK_SZ	CONFIG_USB_HOTPLUG_STACKSIZE
#define THREAD_PRIO	CONFIG_USB_HOTPLUG_PRIORITY

static u8_t usb_hotplug_stack[CONFIG_USB_HOTPLUG_STACKSIZE];

#ifdef CONFIG_USB_HOST

//originally defined in: \zephyr\framework\osal\os_wrapper.c
int _os_thread_create(char *stack, size_t stack_size,
					 void (*entry)(void *, void *, void*),
					 void *p1, void *p2, void *p3,
					 int prio, u32_t options, int delay) {
	k_tid_t tid = NULL;

	os_thread *thread = NULL;

	thread = (os_thread *)stack;

	tid = k_thread_create(thread, (os_thread_stack_t *)&stack[sizeof(os_thread)],
				stack_size - sizeof(os_thread),
				entry,
				p1, p2, p3,
				prio,
				options,
				SYS_TIMEOUT_MS(delay));

	return (int)tid;
}

static void _host_scan_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	usbh_scan_device();
}

static inline int _usb_thread_init_host_scan(void)
{
	usbh_prepare_scan();

	/* Start a thread to offload USB scan/enumeration */
	_os_thread_create(usb_hotplug_stack, STACK_SZ,
			_host_scan_thread,
			NULL, NULL, NULL,
			THREAD_PRIO, 0, 0);

	return 0;
}

static inline int _usb_thread_exit_host_scan(void)
{
	return usbh_disconnect();
}
#endif /* CONFIG_USB_HOST */

#ifdef CONFIG_USB_MASS_STORAGE_SHARE_THREAD
int usb_mass_storage_start(void)
{
	LOG_INF("shared");

	os_thread_create(usb_hotplug_stack, STACK_SZ,
			usb_mass_storage_thread,
			NULL, NULL, NULL,
			CONFIG_MASS_STORAGE_PRIORITY, 0, 0);

	return 0;
}
#endif /* CONFIG_USB_MASS_STORAGE_SHARE_THREAD */

#else /* !CONFIG_USB_HOTPLUG_THREAD_ENABLED */
static inline int _usb_thread_init_host_scan(void)
{
	return 0;
}

static inline int _usb_thread_exit_host_scan(void)
{
	return 0;
}
#endif /* CONFIG_USB_HOTPLUG_THREAD_ENABLED */

static int _usb_hotplug_dpdm_switch(bool host_mode)
{
	return 0;
}

static void _usb_hotplug_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_HIGHSPEED:
		LOG_WRN("USB HS detected");
		usb_disable();
		break;
	case USB_DC_SOF:
		LOG_WRN("USB SOF detected");
		usb_disable();
		break;
	default:
		break;
	}
}

static const struct usb_cfg_data _usb_hotplug_config = {
	.cb_usb_status = _usb_hotplug_status_cb,
};

static inline int _usb_hotplug_detect_device(void)
{
	int ret;

	ret = usb_enable((struct usb_cfg_data *)&_usb_hotplug_config);
	if (ret < 0) {
		LOG_ERR("enable");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_USB_HOST
static inline int _usb_hotplug_host_plugin(void)
{
	_usb_thread_init_host_scan();

	return 0;
}

static inline int _usb_hotplug_host_plugout(void)
{
	usb_hotplug_state = USB_HOTPLUG_A_OUT;

	return _usb_thread_exit_host_scan();
}

#ifdef CONFIG_USB_HOST_STORAGE
extern void usb_stor_set_access(bool accessed);
extern bool usb_host_storage_enabled(void);

int usb_hotplug_disk_accessed(bool access)
{
	usb_stor_set_access(access);
#ifdef CONFIG_FS_MANAGER
	if (access) {
		fs_manager_disk_init("USB:");
	} else {
		fs_manager_disk_uninit("USB:");
	}
#endif

	return 0;
}

static inline int _usb_hotplug_host_storage_plugin(void)
{
	usb_hotplug_state = USB_HOTPLUG_A_IN;

	return 0;
}
#endif /* CONFIG_USB_HOST_STORAGE */
#endif /* CONFIG_USB_HOST */

#ifdef CONFIG_USB_DEVICE
static inline int _usb_hotplug_device_plugin(void)
{
	usb_hotplug_state = USB_HOTPLUG_B_IN;

	return 0;
}

static inline int _usb_hotplug_device_plugout(void)
{
	usb_hotplug_state = USB_HOTPLUG_B_OUT;

	return 0;
}

static inline int _usb_hotplug_charger_plugin(void)
{
	usb_hotplug_state = USB_HOTPLUG_C_IN;

	return 0;
}

static inline int _usb_hotplug_charger_plugout(void)
{
	usb_hotplug_state = USB_HOTPLUG_C_OUT;

	return 0;
}
#endif /* CONFIG_USB_DEVICE */

static inline u8_t _usb_hotplug_get_vbus(void)
{
#ifdef CONFIG_USB_DEVICE
	return usb_phy_get_vbus();
#else
	return USB_VBUS_LOW;	/* Host-only mode: never enter b_idle */
#endif
}

static inline void _usb_hotplug_update_state(void)
{
	u8_t vbus = _usb_hotplug_get_vbus();
#ifdef CONFIG_USB_DEVICE
	bool dc_attached;
	u8_t i;
#endif
	LOG_WRN("otg_state = %d, vbus = %d\n", otg_state, vbus);

	switch (otg_state) {
#ifdef CONFIG_USB_DEVICE
	case OTG_STATE_B_IDLE:
		if (vbus == USB_VBUS_LOW) {
			keep_in_b_idle = 0;
			otg_state = OTG_STATE_A_IDLE;
#ifndef CONFIG_USB_HOST
			usb_phy_reset();
#endif
			_usb_hotplug_charger_plugout();
			break;
		}

		if (keep_in_b_idle >= KEEP_IN_B_IDLE_RETRY) {
			break;
		}

		usb_phy_reset();
		usb_phy_enter_b_idle();
		k_sleep(K_MSEC(3));	/* necessary */
repeat:
		dc_attached = usb_phy_dc_attached();
		for (i = 0; i < 4; i++) {
			k_busy_wait(1000);
			if (dc_attached != usb_phy_dc_attached()) {
				LOG_INF("dc_attached: %d, i: %d",
				       dc_attached, i);
				goto repeat;
			}
		}
		if (dc_attached) {
			keep_in_b_idle = 0;
			otg_state = OTG_STATE_B_WAIT_ACON;
			_usb_hotplug_detect_device();
		} else {
			if (++keep_in_b_idle >= KEEP_IN_B_IDLE_RETRY) {
				LOG_INF("b_idle");
			}
			usb_phy_enter_a_idle();
			_usb_hotplug_charger_plugin();
		}
		break;

	case OTG_STATE_B_WAIT_ACON:
		if (vbus == USB_VBUS_LOW) {
			usb_connect_count = 0;
			otg_state = OTG_STATE_A_IDLE;
			/* exit device mode */
			usb_disable();
			_usb_hotplug_charger_plugout();
			break;
		}

		if (usb_phy_dc_connected()) {
			usb_connect_count = 0;
			otg_state = OTG_STATE_B_PERIPHERAL;
			/* enter device mode */
			_usb_hotplug_device_plugin();
			break;
		}

		if (++usb_connect_count >= USB_CONNECT_COUNT_MAX) {
			LOG_DBG("connected to charger");

			keep_in_b_idle = KEEP_IN_B_IDLE_RETRY;
			usb_connect_count = 0;
			otg_state = OTG_STATE_B_IDLE;

			/* exit device mode */
			usb_disable();
			_usb_hotplug_charger_plugin();
			break;
		}
		break;

	case OTG_STATE_B_PERIPHERAL:
		if (otg_b_peripheral_exiting) {
			/* wait for exit done */
			if (usb_phy_dc_detached()) {
				otg_state = OTG_STATE_A_IDLE;
				otg_b_peripheral_exiting = 0;
			}
			break;
		}

		if (vbus == USB_VBUS_LOW) {
			/* exit device mode */
			_usb_hotplug_device_plugout();
			otg_b_peripheral_exiting = 1;

			/* wait for exit done */
			if (usb_phy_dc_detached()) {
				otg_state = OTG_STATE_A_IDLE;
				otg_b_peripheral_exiting = 0;
			}
			break;
		}

		if (usb_device_unconfigured()) {
			_usb_hotplug_device_plugout();
			otg_b_peripheral_exiting = 1;
			LOG_INF("usb unconfigured");
			break;
		}
#ifdef CONFIG_USB_MASS_STORAGE
		if (usb_mass_storage_ejected()) {
			_usb_hotplug_device_plugout();
			otg_b_peripheral_exiting = 1;
			LOG_INF("usb msc ejected");
			break;
		}
#endif
		break;
#endif /* CONFIG_USB_DEVICE */

#ifdef CONFIG_USB_HOST
	case OTG_STATE_A_IDLE:
		if (vbus == USB_VBUS_HIGH) {
			otg_a_idle_count = 0;
			otg_state = OTG_STATE_B_IDLE;
			usbh_vbus_set(false);
			break;
		}

		/* Enable Vbus */
		usbh_vbus_set(true);

		if (usb_phy_hc_attached()) {
			otg_a_idle_count = 0;
			otg_state = OTG_STATE_A_WAIT_BCON;
			usb_phy_enter_a_wait_bcon();
			break;
		}

		/* reset Vbus */
		if (++otg_a_idle_count >= OTG_A_IDLE_MAX) {
			otg_a_idle_count = 0;
			usbh_vbus_set(false);
			break;
		}
		break;

	case OTG_STATE_A_WAIT_BCON:
		if (vbus == USB_VBUS_HIGH) {
			otg_a_wait_bcon_count = 0;
			otg_state = OTG_STATE_B_IDLE;
			/* exit host mode */
			usbh_vbus_set(false);
			break;
		}

		if (usb_phy_hc_connected()) {
			otg_a_wait_bcon_count = 0;
			otg_state = OTG_STATE_A_HOST;
			/* enter host mode */
			_usb_hotplug_host_plugin();
			break;
		}

		if (++otg_a_wait_bcon_count >= OTG_A_WAIT_BCON_MAX) {
			otg_a_wait_bcon_count = 0;
			otg_state = OTG_STATE_A_IDLE;
			usbh_vbus_set(false);
			break;
		}
		break;

	case OTG_STATE_A_HOST:
		if (vbus == USB_VBUS_HIGH) {
			if (otg_a_host_exiting) {
				if (!_usb_hotplug_host_plugout()) {
					otg_a_host_exiting = 0;
					otg_state = OTG_STATE_B_IDLE;
				}
				break;
			}

			/* exit host mode */
			usbh_vbus_set(false);
			if (_usb_hotplug_host_plugout()) {
				otg_a_host_exiting = 1;
			} else {
				otg_state = OTG_STATE_B_IDLE;
			}
			break;
		}

		/* handle disconnect and connnect quickly */
		if (otg_a_host_exiting) {
			if (!_usb_hotplug_host_plugout()) {
				otg_a_host_exiting = 0;
				otg_state = OTG_STATE_A_IDLE;
			}
			break;
		}

		if (usb_phy_hc_disconnected()) {
			/* exit host mode */
			usbh_vbus_set(false);
			if (_usb_hotplug_host_plugout()) {
				otg_a_host_exiting = 1;
			} else {
				otg_state = OTG_STATE_A_IDLE;
			}
			break;
		}

#ifdef CONFIG_USB_HOST_STORAGE
		if (usb_host_storage_enabled()) {
			_usb_hotplug_host_storage_plugin();
			break;
		}
#endif
		break;
#else /* CONFIG_USB_HOST */
	case OTG_STATE_A_IDLE:
		if (vbus == USB_VBUS_HIGH) {
			otg_state = OTG_STATE_B_IDLE;
			break;
		}
		break;
#endif

	default:
		break;
	}
}

static int _usb_hotplug_loop(void)
{
	enum usb_otg_state old_state = otg_state;

	_usb_hotplug_update_state();

	if (otg_state == old_state) {
		return 0;
	}

	LOG_INF("%s -> %s", state_string[old_state],
	       state_string[otg_state]);

	switch (otg_state) {
	case OTG_STATE_B_IDLE:
		_usb_hotplug_dpdm_switch(false);
		break;

	case OTG_STATE_A_IDLE:
		_usb_hotplug_dpdm_switch(true);
#ifdef CONFIG_USB_HOST
		usb_phy_reset();
		usb_phy_enter_a_idle();
#endif
		break;

	default:
		break;
	}

	return 0;
}

static inline int _usb_hotplug_init(void)
{
	u8_t vbus = _usb_hotplug_get_vbus();

#ifdef CONFIG_USB_DEVICE
	/* suspend comes before hotplug and keep_in_b_idle maybe not reset */
	keep_in_b_idle = 0;
#endif

	if (vbus == USB_VBUS_HIGH) {
		_usb_hotplug_dpdm_switch(false);
#ifdef CONFIG_USB_HOST
		usbh_vbus_set(false);
#endif
		otg_state = OTG_STATE_B_IDLE;
	} else {
		_usb_hotplug_dpdm_switch(true);
#ifdef CONFIG_USB_HOST
		usb_phy_enter_a_idle();
#endif
		otg_state = OTG_STATE_A_IDLE;
	}

	LOG_INF("cur_state %s", state_string[otg_state]);

	/* For a_idle, detect later */
	if (otg_state == OTG_STATE_B_IDLE) {
		_usb_hotplug_loop();
	}

	return 0;
}

/*
 * Check if USB is attached to charger or host
 */
bool _usb_hotplug_device_mode(void)
{
	if ((otg_state == OTG_STATE_B_WAIT_ACON) ||
	    (otg_state == OTG_STATE_B_PERIPHERAL)) {
	    return true;
	}

	return false;
}

int _usb_hotplug_suspend(void)
{
	usb_hotplug_state = USB_HOTPLUG_SUSPEND;

#ifdef CONFIG_USB_HOST
	usbh_vbus_set(false);

	if (otg_state == OTG_STATE_A_HOST) {
		/* should be fast */
		while (usbh_disconnect()) {
			k_sleep(K_MSEC(10));
			LOG_INF("");
		}
	}
#endif

	usb_phy_exit();

	return 0;
}

int _usb_hotplug_resume(void)
{
	usb_hotplug_state = HOTPLUG_NONE;

#ifdef CONFIG_USB_HOST
	if (otg_state == OTG_STATE_A_HOST) {
		/* broadcast when resume */
		struct app_msg	msg = {0};

		msg.type = MSG_HOTPLUG_EVENT;
		msg.cmd = HOTPLUG_USB_HOST;
		msg.value = HOTPLUG_OUT;

		LOG_INF("host");

		return send_async_msg("main", &msg);
	}
#endif

	return _usb_hotplug_init();
}

/* check and enumerate USB device directly */
int _usb_hotplug_host_check(void)
{
	int timeout = 100;	/* 1s */

	if (otg_state != OTG_STATE_A_IDLE) {
		return -EINVAL;
	}

	do {
		_usb_hotplug_loop();

		switch (otg_state) {
		case OTG_STATE_A_WAIT_BCON:
			goto attached;
		case OTG_STATE_A_IDLE:
			break;
		default:
			goto exit;
		}

		k_sleep(K_MSEC(10));
	} while (--timeout);

	/* timeout */
	if (timeout == 0) {
		return -ENODEV;
	}

attached:
	timeout = 10;	/* 1s */

	do {
		_usb_hotplug_loop();

		switch (otg_state) {
		case OTG_STATE_A_HOST:
#ifdef CONFIG_USB_HOST
			usbh_prepare_scan();
			usbh_scan_device();
#endif
		/* fall through */
		default:
			goto exit;
		case OTG_STATE_A_WAIT_BCON:
			break;
		}

		/* should be same as CONFIG_MONITOR_PERIOD */
		k_sleep(K_MSEC(100));
	} while (--timeout);

	/* timeout */
	if (timeout == 0) {
		return -ENODEV;
	}

exit:
	return 0;
}

static int _hotplug_usb_get_type(void)
{
	switch (usb_hotplug_state) {
	case USB_HOTPLUG_A_IN:
	case USB_HOTPLUG_A_OUT:
		return HOTPLUG_USB_HOST;

	case USB_HOTPLUG_B_IN:
	case USB_HOTPLUG_B_OUT:
		return HOTPLUG_USB_DEVICE;
	}

	/* never */
	return 0;
}

static int _hotplug_usb_detect(void)
{
	u8_t old_state = usb_hotplug_state;

	if (usb_hotplug_state == USB_HOTPLUG_SUSPEND) {
		return HOTPLUG_NONE;
	}

	_usb_hotplug_loop();

	if (usb_hotplug_state == old_state) {
		return HOTPLUG_NONE;
	}

	switch (usb_hotplug_state) {
	case USB_HOTPLUG_A_IN:
	case USB_HOTPLUG_B_IN:
		return HOTPLUG_IN;

	case USB_HOTPLUG_A_OUT:
	case USB_HOTPLUG_B_OUT:
		return HOTPLUG_OUT;
	}

	return HOTPLUG_NONE;
}

static int _hotplug_usb_process(int device_state)
{
	int res = -1;

	LOG_INF("%d", usb_hotplug_state);

	switch (usb_hotplug_state) {
	case USB_HOTPLUG_A_IN:
#ifdef CONFIG_FS_MANAGER
		res = fs_manager_disk_init("USB:");
#endif
		break;

	case USB_HOTPLUG_A_OUT:
#ifdef CONFIG_FS_MANAGER
		res = fs_manager_disk_uninit("USB:");
#endif
		break;

	case USB_HOTPLUG_B_IN:
	case USB_HOTPLUG_B_OUT:
		res = 0;
		break;

	default:
		break;
	}

	return res;
}

#ifdef CONFIG_USB_DEVICE
static int _hotplug_usb_device_get_state(void)
{
	if (otg_b_peripheral_exiting == 1) {
		return HOTPLUG_OUT;
	}

	if (otg_state == OTG_STATE_B_PERIPHERAL) {
		return HOTPLUG_IN;
	}

	return HOTPLUG_OUT;
}

static const struct hotplug_device_t _hotplug_usb_device = {
	.type = HOTPLUG_USB_DEVICE,
	.get_state = _hotplug_usb_device_get_state,
	.get_type = _hotplug_usb_get_type,
	.hotplug_detect = _hotplug_usb_detect,
	.fs_process = _hotplug_usb_process,
};
#endif /* CONFIG_USB_DEVICE */

#ifdef CONFIG_USB_HOST
static int _hotplug_usb_host_get_state(void)
{
	if (otg_state == OTG_STATE_A_HOST) {
		return HOTPLUG_IN;
	}

	return HOTPLUG_OUT;
}

static int __unused _hotplug_usb_host_detect(void)
{
	return HOTPLUG_NONE;
}

static const struct hotplug_device_t _hotplug_usb_host = {
	.type = HOTPLUG_USB_HOST,
	.get_state = _hotplug_usb_host_get_state,
#ifndef CONFIG_USB_DEVICE
	.get_type = _hotplug_usb_get_type,
	.hotplug_detect = _hotplug_usb_detect,
	.fs_process = _hotplug_usb_process,
#else
	.hotplug_detect = _hotplug_usb_host_detect,
#endif
};
#endif /* CONFIG_USB_HOST */

int _hotplug_usb_init(void)
{
#ifdef CONFIG_USB_DEVICE
	_hotplug_device_register(&_hotplug_usb_device);
#endif

#ifdef CONFIG_USB_HOST
	_hotplug_device_register(&_hotplug_usb_host);
#endif

	return 0;
}

#ifdef CONFIG_USB_DEVICE
static struct k_timer _usb_hotplug_timer;
static void _usb_hotplug_expiry_fn(struct k_timer *timer)
{
	switch (otg_state) {
	case OTG_STATE_B_WAIT_ACON:
	case OTG_STATE_B_PERIPHERAL:
		if (!usb_phy_get_vbus() && !usb_phy_dc_detached()) {
			usb_phy_dc_disconnect();
		}
		break;

	default:
		break;
	}
}
#endif /* CONFIG_USB_DEVICE */

static int _usb_hotplug_pre_init(const struct device *dev)
{
#ifdef CONFIG_USB_DEVICE
	k_timer_init(&_usb_hotplug_timer, _usb_hotplug_expiry_fn, NULL);
	k_timer_start(&_usb_hotplug_timer, K_SECONDS(1), K_MSEC(400));
#endif

	usb_phy_init();

#ifdef CONFIG_USB_HOST
	/* turn off Vbus by default */
	usbh_vbus_set(false);
#endif

	_usb_hotplug_init();

	return 0;
}

#define USB_HOTPLUG_PRE_INIT_PRIORITY CONFIG_KERNEL_INIT_PRIORITY_DEFAULT
SYS_INIT(_usb_hotplug_pre_init, POST_KERNEL, USB_HOTPLUG_PRE_INIT_PRIORITY);
