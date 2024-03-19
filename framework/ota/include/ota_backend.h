/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA backend interface
 */

#ifndef __OTA_BACKEND_H__
#define __OTA_BACKEND_H__

/** type of backend*/
#define OTA_BACKEND_TYPE_UNKNOWN		(0)
#define OTA_BACKEND_TYPE_CARD			(1)
#define OTA_BACKEND_TYPE_BLUETOOTH		(2)
#define OTA_BACKEND_TYPE_TEMP_PART		(3)

#ifdef CONFIG_BT_TRANSCEIVER
#define OTA_BACKEND_TYPE_USBHID         (5)
#endif

#define OTA_BACKEND_IOCTL_REPORT_IMAGE_VALID	(0x10001)
#define OTA_BACKEND_IOCTL_REPORT_PROCESS		(0x10002)

#define OTA_BACKEND_UPGRADE_STATE	(1)
#define OTA_BACKEND_UPGRADE_PROGRESS	(2)

struct ota_backend;

typedef void (*ota_backend_notify_cb_t)(struct ota_backend *backend, int cmd, int state);

/**
 * @brief Logger backend API.
 */
struct ota_backend_api
{
	/** ota backend init operation Function pointer*/
	struct ota_backend * (*init)(ota_backend_notify_cb_t cb, void *init_param);
	/** ota backend exit operation Function pointer*/
	void (*exit)(struct ota_backend *backend);
	/** ota backend open operation Function pointer*/
	int (*open)(struct ota_backend *backend);
	/** ota backend read operation Function pointer*/
	int (*read)(struct ota_backend *backend, int offset, unsigned char *buf, int size);
	/** ota backend ioctl operation Function pointer*/
	int (*ioctl)(struct ota_backend *backend, int cmd, unsigned int param);
	/** ota backend close operation Function pointer*/
	int (*close)(struct ota_backend *backend);
#ifdef CONFIG_SPPBLE_DATA_CHAN
	/** ota backend read data operation Function pointer*/
	int (*tr_read_data)(struct ota_backend *backend, unsigned char *buf, int size);
	/** ota backend write data operation Function pointer*/
	int (*tr_write_data)(struct ota_backend *backend, unsigned char *buf, int size);
#endif
};

/** structure of backend*/
struct ota_backend {
	/** backend api */
	struct ota_backend_api *api;
	/** backend type */
	int type;
	ota_backend_notify_cb_t cb;
};

/**
 * @brief get backend type
 *
 * This routine provides get backend type
 *
 * @param backend backend of ota
 *
 * @return backend type
 */

static inline int ota_backend_get_type(struct ota_backend *backend)
{
	 __ASSERT_NO_MSG(backend);

	return backend->type;
}

/**
 * @brief backend ioctl calls by libota to tell user about ota state or progress.
 *
 * @param backend backend of ota
 *
 * @param cmd user define comand
 *
 * @param param comand value
 *
 * @return 0: success.
 *
 * @return other: fail.

 */

static inline int ota_backend_ioctl(struct ota_backend *backend, int cmd,
				    unsigned int param)
{
	 __ASSERT_NO_MSG(backend);

	 if (backend->api->ioctl) {
		 return backend->api->ioctl(backend, cmd, param);
	 }

	return 0;
}

/**
 * @brief read data by backend,calls by libota.
 *
 * @param backend pointer to backend
 *
 * @param offs starting offset to read
 *
 * @param buf out put data buffer pointer
 *
 * @param size bytes user want to read
 *
 * @return 0: read success.
 *
 * @return other: read fail.
 */

static inline int ota_backend_read(struct ota_backend *backend, int offset,
				   unsigned char *buf, int size)
{
	 __ASSERT_NO_MSG(backend);

	return backend->api->read(backend, offset, buf, size);
}

/**
 * @brief backend open.
 *
 * @param backend pointer to backend
 *
 * @return 0: open success.
 *
 * @return other: open fail.
 */

static inline int ota_backend_open(struct ota_backend *backend)
{
	 __ASSERT_NO_MSG(backend);

	return backend->api->open(backend);
}
/**
 * @brief backend close.
 *
 * @param backend pointer to backend
 *
 * @return 0: close success.
 *
 * @return other: close fail.
 */

static inline int ota_backend_close(struct ota_backend *backend)
{
	 __ASSERT_NO_MSG(backend);

	return backend->api->close(backend);
}
/**
 * @brief backend exit.
 *
 * This routine free backend
 *
 * @param backend pointer to backend
 */

static inline void ota_backend_exit(struct ota_backend *backend)
{
	 __ASSERT_NO_MSG(backend);
	 return backend->api->exit(backend);
}
/**
 * @brief backend init.
 *
 * @param backend pointer to backend
 */

static inline int ota_backend_init(struct ota_backend *backend, int type,
				   struct ota_backend_api *api,
				   ota_backend_notify_cb_t cb)
{
	 __ASSERT_NO_MSG(backend);

	backend->type = type;
	backend->api = api;
	backend->cb = cb;

	return 0;
}

#ifdef CONFIG_SPPBLE_DATA_CHAN
/**
 * @brief read data by backend,calls by libota.
 *
 * @param backend pointer to backend
 *
 * @param buf out put data buffer pointer
 *
 * @param size bytes user want to read
 *
 * @return 0: read success.
 *
 * @return other: read fail.
 */

static inline int tr_ota_backend_read(struct ota_backend *backend, unsigned char *buf, int size)
{
	 __ASSERT_NO_MSG(backend);

	return backend->api->tr_read_data(backend, buf, size);
}

/**
 * @brief write data by backend,calls by libota.
 *
 * @param backend pointer to backend
 *
 * @param buf out put data buffer pointer
 *
 * @param size bytes user want to write
 *
 * @return 0: read success.
 *
 * @return other: read fail.
 */
static inline int tr_ota_backend_write(struct ota_backend *backend, unsigned char *buf, int size)
{
	 __ASSERT_NO_MSG(backend);

	return backend->api->tr_write_data(backend, buf, size);
}

#endif
#endif /* __OTA_BACKEND_H__ */
