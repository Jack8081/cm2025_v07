/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA USBHID backend interface
 */

#ifndef __OTA_BACKEND_USBHID_H__
#define __OTA_BACKEND_USBHID_H__

#include <ota_backend.h>

struct ota_backend_usbhid_init_param {
	const char *mode;
};

struct ota_backend *ota_backend_usbhid_init(ota_backend_notify_cb_t cb,
		struct ota_backend_usbhid_init_param *param);

#endif /* __OTA_BACKEND_USBHID_H__ */
