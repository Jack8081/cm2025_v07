/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system wakelock
 */

#include <os_common_api.h>
#include <string.h>
#include <sys_manager.h>
#include <sys_wakelock.h>
#include <assert.h>

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "sys_wakelock"
#endif

struct wakelock_t {
	uint8_t wake_lock_type;
	uint16_t ref_cnt;
	uint32_t free_timestamp;
};

struct wakelocks_manager_context_t {
	struct wakelock_t wakelocks[MAX_WAKE_LOCK_TYPE];
};

struct wakelocks_manager_context_t wakelocks_manager;

static struct wakelock_t *_sys_wakelock_lookup(int wake_lock_type)
{
	for (int i = 0; i < MAX_WAKE_LOCK_TYPE; i++) {
		if (wakelocks_manager.wakelocks[i].wake_lock_type == wake_lock_type) {
			return &wakelocks_manager.wakelocks[i];
		}
	}
	return NULL;
}

int sys_wake_lock(int wake_lock_type)
{
	int res = 0;
	struct wakelock_t *wakelock = NULL;

#ifdef CONFIG_SYS_IRQ_LOCK
	SYS_IRQ_FLAGS flags;
	sys_irq_lock(&flags);
#else
	int key = os_irq_lock();
#endif

	wakelock = _sys_wakelock_lookup(wake_lock_type);
	if (!wakelock) {
		SYS_LOG_ERR("err, holder: %d", wake_lock_type);
		res = -EEXIST;
		goto exit;
	}

	assert(wakelock->ref_cnt < 0xFFFF);

	wakelock->ref_cnt++;
	wakelock->free_timestamp = 0;
exit:

#ifdef CONFIG_SYS_IRQ_LOCK 
	sys_irq_unlock(&flags);
#else
	os_irq_unlock(key);
#endif

#ifndef CONFIG_SIMULATOR
	SYS_LOG_INF("%d %d %d caller %p \n",wakelock->wake_lock_type, wakelock->ref_cnt, wakelock->free_timestamp, __builtin_return_address(0));
#endif
	return res;
}

int sys_wake_unlock(int wake_lock_type)
{
	int res = 0;
	struct wakelock_t *wakelock = NULL;

#ifdef CONFIG_SYS_IRQ_LOCK 
	SYS_IRQ_FLAGS flags;
	sys_irq_lock(&flags);
#else
	int key = os_irq_lock();
#endif

	wakelock = _sys_wakelock_lookup(wake_lock_type);
	if (!wakelock) {
		SYS_LOG_ERR("err, holder: %d", wake_lock_type);
		res = -ESRCH;
		goto exit;
	}

	assert(wakelock->ref_cnt > 0);
	if (wakelock->ref_cnt--) {
		/** reset all wake lock timestamp */
		if (wake_lock_type == FULL_WAKE_LOCK) {
			for (int i = 0; i < MAX_WAKE_LOCK_TYPE; i++) {
				wakelocks_manager.wakelocks[i].free_timestamp = os_uptime_get_32();
			}
		} else {
			wakelock->free_timestamp = os_uptime_get_32();
		}
	}

	system_clear_fast_standby();
exit:

#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_unlock(&flags);
#else
	os_irq_unlock(key);
#endif

#ifndef CONFIG_SIMULATOR
	SYS_LOG_INF("%d %d %d caller %p \n",wakelock->wake_lock_type, wakelock->ref_cnt, wakelock->free_timestamp, __builtin_return_address(0));
#endif

	return res;
}


int sys_wake_lock_reset(int wake_lock_type)
{
	int res = 0;
	struct wakelock_t *wakelock = NULL;

#ifdef CONFIG_SYS_IRQ_LOCK
	SYS_IRQ_FLAGS flags;
	sys_irq_lock(&flags);
#else
	int key = os_irq_lock();
#endif

	wakelock = _sys_wakelock_lookup(wake_lock_type);
	if (!wakelock) {
		SYS_LOG_ERR("err, holder: %d", wake_lock_type);
		res = -EEXIST;
		goto exit;
	}

	assert(wakelock->ref_cnt < 0xFFFF);

	wakelock->ref_cnt++;
	wakelock->free_timestamp = 0;

	assert(wakelock->ref_cnt > 0);
	if (wakelock->ref_cnt--) {
		/** reset all wake lock timestamp */
		if (wake_lock_type == FULL_WAKE_LOCK) {
			for (int i = 0; i < MAX_WAKE_LOCK_TYPE; i++) {
				wakelocks_manager.wakelocks[i].free_timestamp = os_uptime_get_32();
			}
		} else {
			wakelock->free_timestamp = os_uptime_get_32();
		}
	}

	system_clear_fast_standby();
	exit:
	
#ifdef CONFIG_SYS_IRQ_LOCK 
	sys_irq_unlock(&flags);
#else
	os_irq_unlock(key);
#endif
	
	return res;
}



int sys_wakelocks_init(void)
{
	int current_timestamp = os_uptime_get_32();

	memset(&wakelocks_manager, 0 , sizeof(struct wakelocks_manager_context_t));

	for (int i = 0; i < MAX_WAKE_LOCK_TYPE; i++) {
		wakelocks_manager.wakelocks[i].wake_lock_type = i;
		wakelocks_manager.wakelocks[i].free_timestamp = current_timestamp;
		wakelocks_manager.wakelocks[i].ref_cnt = 0;
	}

	/** only for debug*/
	sys_wake_lock(FULL_WAKE_LOCK);

	return 0;
}


int sys_wakelocks_check(int wake_lock_type)
{
	int ref_cnt = 0;
	struct wakelock_t *wakelock = NULL;
#ifdef CONFIG_SYS_IRQ_LOCK 
	SYS_IRQ_FLAGS flags;
	sys_irq_lock(&flags);
#else
	int key = os_irq_lock();
#endif

	wakelock = _sys_wakelock_lookup(wake_lock_type);
	if (!wakelock) {
		SYS_LOG_ERR("err, holder: %d", wake_lock_type);
		goto exit;
	}

	ref_cnt =  wakelock->ref_cnt;

exit:
#ifdef CONFIG_SYS_IRQ_LOCK 
	sys_irq_unlock(&flags);
#else
	os_irq_unlock(key);
#endif

	return ref_cnt;
}

uint32_t sys_wakelocks_get_free_time(int wake_lock_type)
{
	int free_time = 0;
	struct wakelock_t *wakelock = NULL;
#ifdef CONFIG_SYS_IRQ_LOCK 
	SYS_IRQ_FLAGS flags;
	sys_irq_lock(&flags);
#else
	int key = os_irq_lock();
#endif

	wakelock = _sys_wakelock_lookup(wake_lock_type);
	if (!wakelock) {
		SYS_LOG_ERR("err, holder: %d", wake_lock_type);
		goto exit;
	}

	if (!wakelock->ref_cnt) {
		free_time = os_uptime_get_32() - wakelock->free_timestamp;
	}

exit:

#ifdef CONFIG_SYS_IRQ_LOCK 
	sys_irq_unlock(&flags);
#else
	os_irq_unlock(key);
#endif

	return free_time;
}

