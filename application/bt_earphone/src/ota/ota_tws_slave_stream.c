/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager genarate bt mac and name.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include <sys_event.h>
#include <stream.h>
#include <app_ui.h>
#include "ota_app.h"

#include "ota_tws_transmit.h"
#ifdef CONFIG_OTA_BACKEND_BLUETOOTH


#define MAX_SLAVE_OTA_STREAM		(1)
#define SPPBLE_BUFF_SIZE		(1024*2)
#define SPPBLE_SEND_LEN_ONCE	(512)
#define SPPBLE_SEND_INTERVAL	(5)		/* 5ms */


enum {
	NONE_CONNECT_TYPE,
	SPP_CONNECT_TYPE,
	BLE_CONNECT_TYPE,
};

struct slave_ota_info_t {
	void (*connect_cb)(bool connected);
	uint8_t connect_type;
	uint8_t channel;
	int32_t read_timeout;
	int32_t write_timeout;
	uint8_t *buff;
	os_mutex read_mutex;
	os_sem read_sem;
	os_mutex write_mutex;
	u8_t fn;
};

//extern int ota_app_init_slave_bluetooth(void);

static void slave_ota_rx_date(io_stream_t handle, uint8_t *buf, uint16_t len);
static int slave_ota_open(io_stream_t handle, stream_mode mode);
static io_stream_t slave_ota_create_tws_stream[MAX_SLAVE_OTA_STREAM] __IN_BT_SECTION;
static OS_MUTEX_DEFINE(g_slave_ota_mutex);

static io_stream_t find_streame_by_channel(uint8_t channel)
{
	io_stream_t stream;
	struct slave_ota_info_t *info;
	int i;

	os_mutex_lock(&g_slave_ota_mutex, OS_FOREVER);

	for (i = 0; i < MAX_SLAVE_OTA_STREAM; i++) {
		stream = slave_ota_create_tws_stream[i];
		if (stream) {
			info = (struct slave_ota_info_t *)stream->data;
			if (info->channel == channel) {
				os_mutex_unlock(&g_slave_ota_mutex);
				return stream;
			}
		}
	}

	os_mutex_unlock(&g_slave_ota_mutex);
	return NULL;
}

static int slave_ota_add_stream(io_stream_t handle)
{
	int i;

	os_mutex_lock(&g_slave_ota_mutex, OS_FOREVER);
	for (i = 0; i < MAX_SLAVE_OTA_STREAM; i++) {
		if (slave_ota_create_tws_stream[i] == NULL) {
			slave_ota_create_tws_stream[i] = handle;
			break;
		}
	}
	os_mutex_unlock(&g_slave_ota_mutex);

	if (i == MAX_SLAVE_OTA_STREAM) {
		SYS_LOG_ERR("Failed to add stream handle %p", handle);
		return -EIO;
	}

	return 0;
}

static int slave_ota_remove_stream(io_stream_t handle)
{
	int i;

	os_mutex_lock(&g_slave_ota_mutex, OS_FOREVER);
	for (i = 0; i < MAX_SLAVE_OTA_STREAM; i++) {
		if (slave_ota_create_tws_stream[i] == handle) {
			slave_ota_create_tws_stream[i] = NULL;
			break;
		}
	}
	os_mutex_unlock(&g_slave_ota_mutex);

	if (i == MAX_SLAVE_OTA_STREAM) {
		SYS_LOG_ERR("Failed to remove stream handle %p", handle);
		return -EIO;
	}

	return 0;
}

static void stream_slave_ota_connected_cb(uint8_t channel, uint8_t *uuid)
{
	struct slave_ota_info_t *info;
	io_stream_t handle = find_streame_by_channel(channel);

	SYS_LOG_INF("channel:%d", channel);

	//ota_app_init_slave_bluetooth();

	if (handle && handle->data) {
		info = (struct slave_ota_info_t *)handle->data;
		if (info->connect_type == NONE_CONNECT_TYPE) {
			info->connect_type = SPP_CONNECT_TYPE;
			if (info->connect_cb) {
				//input_manager_lock();
				info->connect_cb(true);
				info->fn = 0;
			}
		} else {
			SYS_LOG_WRN("Had connecte: %d", info->connect_type);
		}
	}
}

static void stream_slave_ota_disconnected_cb(uint8_t channel)
{
	struct slave_ota_info_t *info;
	io_stream_t handle = find_streame_by_channel(channel);
	int32_t timeout = 0;

	SYS_LOG_INF("channel:%d", channel);

	if (handle && handle->data) {
		info = (struct slave_ota_info_t *)handle->data;
		if (info->connect_type == SPP_CONNECT_TYPE) {
			info->connect_type = NONE_CONNECT_TYPE;
			os_sem_give(&info->read_sem);

			while (handle->cache_size != 0) {
				if (timeout >= 500) {
					break;
				}
				SYS_LOG_INF("waitting");

				timeout += 50;
				os_sleep(50);
			}
			if (info->connect_cb) {
				info->connect_cb(false);
				//input_manager_unlock();
			}
		} else {
			SYS_LOG_WRN("Not connecte spp: %d", info->connect_type);
		}
	}
}

static void stream_slave_ota_receive_data_cb(uint8_t channel, uint8_t *data, uint32_t len)
{
	struct slave_ota_info_t *info;
	io_stream_t handle = find_streame_by_channel(channel);

	SYS_LOG_DBG("channel:%d, len:%d", channel, len);
	if (handle && handle->data) {
		info = (struct slave_ota_info_t *)handle->data;
		if (info->connect_type == SPP_CONNECT_TYPE) {
			slave_ota_open(handle, 0);
			slave_ota_rx_date(handle, data, len);
		}
	}
}


static const struct btmgr_spp_cb stream_slave_ota_cb = {
	.connected = stream_slave_ota_connected_cb,
	.disconnected = stream_slave_ota_disconnected_cb,
	.receive_data = stream_slave_ota_receive_data_cb,
};

static int ota_slave_register(struct slave_ota_info_t *info)
{
	int ret;

	ret = tws_ota_slave_register ((struct btmgr_spp_cb *)&stream_slave_ota_cb);
	if (ret < 0) {
		SYS_LOG_ERR("Failed register spp uuid");
		return ret;
	}

	return 0;
}

static int slave_ota_init(io_stream_t handle, void *param)
{
	int ret = 0;
	struct slave_ota_info_t *info = NULL;
	struct sppble_stream_init_param *init_param = (struct sppble_stream_init_param *)param;

	if (slave_ota_add_stream(handle)) {
		ret = -EIO;
		goto err_exit;
	}

	info = mem_malloc(sizeof(struct slave_ota_info_t));
	if (!info) {
		SYS_LOG_ERR("cache stream info malloc failed\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	memset(info, 0, sizeof(struct slave_ota_info_t));
	info->connect_cb = init_param->connect_cb;
	info->read_timeout = init_param->read_timeout;
	info->write_timeout = init_param->write_timeout;
	info->channel = 0;
	os_mutex_init(&info->read_mutex);
	os_sem_init(&info->read_sem, 0, 1);
	os_mutex_init(&info->write_mutex);

	handle->data = info;

	SYS_LOG_ERR("232");
	if (ota_slave_register(info)) {
		ret = -EIO;
		goto err_exit;
	}

	return 0;

err_exit:
	if (info) {
		mem_free(info);
	}
	
	slave_ota_remove_stream(handle);
	return ret;
}

static int slave_ota_open(io_stream_t handle, stream_mode mode)
{
	struct slave_ota_info_t *info = NULL;

	info = (struct slave_ota_info_t *)handle->data;
	if (info->connect_type == NONE_CONNECT_TYPE) {
		return -EIO;
	}

	if (info->buff) {
		return 0;
	}

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	info->buff = mem_malloc(SPPBLE_BUFF_SIZE);
	if (!info->buff) {
		os_mutex_unlock(&info->read_mutex);
		return -ENOMEM;
	}

	handle->total_size = SPPBLE_BUFF_SIZE;
	handle->cache_size = 0;
	handle->rofs = 0;
	handle->wofs = 0;
	os_mutex_unlock(&info->read_mutex);

	return 0;
}

static void slave_ota_rx_date(io_stream_t handle, uint8_t *buf, uint16_t len)
{
	struct slave_ota_info_t *info = NULL;
	uint16_t w_len, r_len;

	info = (struct slave_ota_info_t *)handle->data;
	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	if ((handle->cache_size + len) <= handle->total_size) {
		if ((handle->wofs + len) > handle->total_size) {
			w_len = handle->total_size - handle->wofs;
			memcpy(&info->buff[handle->wofs], &buf[0], w_len);
			r_len = len - w_len;
			memcpy(&info->buff[0], &buf[w_len], r_len);
			handle->wofs = r_len;
		} else {
			memcpy(&info->buff[handle->wofs], buf, len);
			handle->wofs += len;
		}

		handle->cache_size += len;
		os_sem_give(&info->read_sem);
	} else {
		SYS_LOG_WRN("Not enough buffer: %d, %d, %d", handle->cache_size, len, handle->total_size);
	}
	os_mutex_unlock(&info->read_mutex);
}

static int slave_ota_read(io_stream_t handle, uint8_t *buf, int num)
{
	struct slave_ota_info_t *info = NULL;
	uint16_t r_len, rr_len;

	info = (struct slave_ota_info_t *)handle->data;

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	if ((info->connect_type == NONE_CONNECT_TYPE) ||
		(info->buff == NULL)) {
		os_mutex_unlock(&info->read_mutex);
		return -EIO;
	}

	if ((info->connect_type != NONE_CONNECT_TYPE) && (handle->cache_size == 0) &&
		(info->read_timeout != OS_NO_WAIT)) {
		os_sem_reset(&info->read_sem);
		os_mutex_unlock(&info->read_mutex);

		os_sem_take(&info->read_sem, info->read_timeout);
		os_mutex_lock(&info->read_mutex, OS_FOREVER);
	}

	if (handle->cache_size == 0) {
		os_mutex_unlock(&info->read_mutex);
		return 0;
	}

	r_len = (handle->cache_size > num) ? num : handle->cache_size;
	if ((handle->rofs + r_len) > handle->total_size) {
		rr_len = handle->total_size - handle->rofs;
		memcpy(&buf[0], &info->buff[handle->rofs], rr_len);
		memcpy(&buf[rr_len], &info->buff[0], (r_len - rr_len));
		handle->cache_size -= r_len;
		handle->rofs = r_len - rr_len;
	} else {
		memcpy(&buf[0], &info->buff[handle->rofs], r_len);
		handle->cache_size -= r_len;
		handle->rofs += r_len;
	}

	os_mutex_unlock(&info->read_mutex);
	return r_len;
}

static int slave_ota_tell(io_stream_t handle)
{
	int ret = 0;
	struct slave_ota_info_t *info = NULL;

	info = (struct slave_ota_info_t *)handle->data;
	if (info->connect_type == NONE_CONNECT_TYPE) {
		return -EIO;
	}

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	ret = handle->cache_size;
	os_mutex_unlock(&info->read_mutex);

	return ret;
}

static int slave_ota_spp_send_data(struct slave_ota_info_t *info, uint8_t *buf, int num)
{
	tws_ota_info_t st;

	info->fn++;
	memset(&st, 0, sizeof(tws_ota_info_t));
	st.type = OTA_TWS_TYPE_DATA_MASTER;
	st.ctx_len = num;
	st.fn = info->fn;

	tws_ota_transmit_data(&st, buf);
	SYS_LOG_INF("info->fn %d", info->fn);

	return num;
}

static int slave_ota_write(io_stream_t handle, uint8_t *buf, int num)
{
	int ret = 0;
	struct slave_ota_info_t *info = NULL;

	info = (struct slave_ota_info_t *)handle->data;
	if (info->connect_type == NONE_CONNECT_TYPE) {
		return -EIO;
	}

	os_mutex_lock(&info->write_mutex, OS_FOREVER);
	if (info->connect_type == SPP_CONNECT_TYPE) {
		ret = slave_ota_spp_send_data(info, buf, num);
	}
	os_mutex_unlock(&info->write_mutex);

	return ret;
}

static int slave_ota_close(io_stream_t handle)
{
	struct slave_ota_info_t *info = NULL;

	info = (struct slave_ota_info_t *)handle->data;
	if (info->connect_type != NONE_CONNECT_TYPE) {
		SYS_LOG_INF("Active do %d disconnect", info->connect_type);
		//bt_manager_spp_disconnect(info->spp_chl);
	}

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	if (info->buff) {
		mem_free(info->buff);
		info->buff = NULL;
		handle->rofs = 0;
		handle->wofs = 0;
		handle->cache_size = 0;
		handle->total_size = 0;
	}
	os_mutex_unlock(&info->read_mutex);
	ota_app_init_bluetooth();

	return 0;
}

static int slave_ota_destroy(io_stream_t handle)
{
	SYS_LOG_WRN("slave ota stream not support destroy new!!");
	return -EIO;
}

const stream_ops_t slave_ota_stream_tws_ops = {
	.init = slave_ota_init,
	.open = slave_ota_open,
	.read = slave_ota_read,
	.seek = NULL,
	.tell = slave_ota_tell,
	.write = slave_ota_write,
	.close = slave_ota_close,
	.destroy = slave_ota_destroy,
};

io_stream_t slave_ota_stream_tws_create(void *param)
{
	return stream_create(&slave_ota_stream_tws_ops, param);
}
#endif

