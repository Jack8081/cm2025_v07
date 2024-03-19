/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file message manager interface
 */

#define SYS_LOG_DOMAIN "msg_manager"

#include <os_common_api.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <sys_wakelock.h>
#include <stdio.h>
#include <string.h>

extern int os_get_pending_msg_cnt(void);
/*global data mailbox for all app thread*/
OS_MUTEX_DEFINE(msg_manager_mutex);

static sys_slist_t	global_receiver_list;

static bool lock_flag;

static struct msg_listener *msg_manager_find_by_name(char *name)
{
	int key;
	sys_snode_t *node, *tmp;
	struct msg_listener *listener = NULL;

	key = os_irq_lock();

	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_receiver_list, node, tmp) {
		listener = LISTENER_INFO(node);
		if (!strcmp(listener->name, name)) {
			goto exit;
		}
	}
	listener = NULL;
exit:
	os_irq_unlock(key);
	return listener;
}


static struct msg_listener *msg_manager_find_by_tid(os_tid_t tid)
{
	int key;
	sys_snode_t *node, *tmp;
	struct msg_listener *listener = NULL;

	key = os_irq_lock();

	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_receiver_list, node, tmp) {
		listener = LISTENER_INFO(node);
		if (listener->tid == tid) {
			goto exit;
		}
	}
	listener = NULL;
exit:
	os_irq_unlock(key);
	return listener;
}

char *msg_manager_get_current(void)
{
	struct msg_listener *listener = msg_manager_find_by_tid(os_current_get());

	if (listener != NULL) {
		return listener->name;
	}
	return NULL;
}

char *msg_manager_get_name_by_tid(int tid)
{
	struct msg_listener *listener = msg_manager_find_by_tid((os_tid_t)tid);

	if (listener != NULL) {
		return listener->name;
	}
	return NULL;
}

bool msg_manager_add_listener(char *name, os_tid_t tid)
{

	struct msg_listener *listener = NULL;

	int key = os_irq_lock();

	listener = mem_malloc(sizeof(struct msg_listener));
	if (!listener) {
		goto exit;
	}

	listener->name = name;

	listener->tid = tid;

	sys_slist_append(&global_receiver_list, (sys_snode_t *)listener);

exit:
	os_irq_unlock(key);
	return true;
}

bool msg_manager_remove_listener(char *name)
{
	struct msg_listener *listener = msg_manager_find_by_name(name);
	bool result = false;

	int key = os_irq_lock();

	if (listener != NULL) {
		sys_slist_find_and_remove(&global_receiver_list, (sys_snode_t *)listener);
		mem_free(listener);
		result = true;
		goto exit;
	}
exit:
	os_irq_unlock(key);
	return result;
}

os_tid_t  msg_manager_listener_tid(char *name)
{
	struct msg_listener *listener = msg_manager_find_by_name(name);

	if (listener != NULL) {
		return listener->tid;
	}

	return NULL;
}
/*init manager*/
bool msg_manager_init(void)
{
	os_msg_init();
	lock_flag = false;
	return true;
}

/*@brief Provide send async mesg interface
 *Note:
 *
 *@param receiver the id for who will receive this msg
 *@param msg which msg you will send
 */

bool msg_manager_send_async_msg(char *receiver, struct app_msg *msg)
{
	int prio;
	bool result = false;
	os_tid_t target_thread_tid = OS_ANY;
#ifdef CONFIG_SYS_WAKELOCK
	//sys_wake_lock(WAKELOCK_MESSAGE);
#endif
	if (lock_flag) {
		SYS_LOG_WRN("msg mng is lock %s \n",receiver);
	}

	prio = os_thread_priority_get(os_current_get());
	os_thread_priority_set(os_current_get(), -1);

	if (strcmp(receiver, ALL_RECEIVER_NAME)) {
		target_thread_tid = msg_manager_listener_tid(receiver);
		if (target_thread_tid == NULL) {
			SYS_LOG_ERR("app %s not ready\n", receiver);
			result = false;
			goto exit;
		}
	}

	if (!os_send_async_msg(target_thread_tid, msg, sizeof(struct app_msg))) {
		result = true;
	} else {
		SYS_LOG_ERR("send fail, type:%d, cmd:%d\n", msg->type,msg->cmd);
#ifdef CONFIG_MESSAGE_DEBUG
		msg_manager_dump_busy_msg();
#endif
		result = false;
	}
exit:
#ifdef CONFIG_SYS_WAKELOCK
	//sys_wake_unlock(WAKELOCK_MESSAGE);
#endif
	os_thread_priority_set(os_current_get(), prio);
	return result;
}

#if 0
bool msg_manager_send_sync_msg(char *receiver, struct app_msg *msg)
{
	int prio;
	bool result = false;
	os_tid_t target_thread_tid = OS_ANY;

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(WAKELOCK_MESSAGE);
#endif

	prio = os_thread_priority_get(os_current_get());
	os_thread_priority_set(os_current_get(), -1);

	if (strcmp(receiver, ALL_RECEIVER_NAME)) {
		target_thread_tid = msg_manager_listener_tid(receiver);
		if (target_thread_tid == NULL) {
			SYS_LOG_ERR("app %s not ready\n", receiver);
			result = false;
			goto exit;
		}
	}

	if (!os_send_sync_msg(target_thread_tid, msg, sizeof(struct app_msg))) {
		result = true;
	} else {
		result = false;
	}
exit:
	os_thread_priority_set(os_current_get(), prio);

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(WAKELOCK_MESSAGE);
#endif

	return result;
}
#endif

void msg_manager_lock(void)
{
	lock_flag = true;
}

void msg_manager_unlock(void)
{
	lock_flag = false;
}

bool msg_manager_receive_msg(struct app_msg *msg, int timeout)
{
	bool result = false;

	if (!os_receive_msg(msg, sizeof(struct app_msg), timeout))
		result  = true;

	return result;
}

int msg_manager_get_free_msg_num(void)
{
	return msg_pool_get_free_msg_num();
}

void msg_manager_drop_all_msg(void)
{
	os_msg_clean();
}

int msg_manager_get_pending_msg_cnt(void)
{
	return os_get_pending_msg_cnt();
}

static void _msg_manager_dump_cb(os_tid_t sender, os_tid_t receiver,
		const char *content, int max_size)
{
	const struct app_msg *msg = (struct app_msg *)content;
	const char *s_sender = msg_manager_get_name_by_tid((int)sender);
	const char *s_receiver = msg_manager_get_name_by_tid((int)receiver);

	if (s_sender) {
		os_printk("\t sender %s\n", s_sender);
	} else {
		os_printk("\t sender %p\n", sender);
	}

	if (s_receiver) {
		os_printk("\t receiver %s\n", s_receiver);
	} else {
		os_printk("\t receiver %p\n", receiver);
	}

	os_printk("\t type %d\n", msg->type);
	os_printk("\t cmd %d\n", msg->cmd);
	os_printk("\t reserve %d\n", msg->reserve);
	os_printk("\t value 0x%x\n", (uint32_t)msg->value);
	os_printk("\t callback %p\n", msg->callback);
	os_printk("\t sync_sem %p\n", msg->sync_sem);
}

void msg_manager_dump_busy_msg(void)
{
	msg_pool_dump(_msg_manager_dump_cb);
}

const char* msg2str(uint8_t msg, const msg2str_map_t* maps, 
    int num, const char* unmapped)
{
    int i;
    
    for (i = 0; i < num; i++)
    {
        if (maps[i].msg == msg)
        {
            return maps[i].str;
        }
    }

    return unmapped;
}

static const msg2str_map_t _sys_msg_str_maps[] =
{
    { MSG_START_APP,            "START_APP",            },
//  { MSG_SWITCH_APP,           "SWITCH_APP",           },
    { MSG_INIT_APP,             "INIT_APP",             },
    { MSG_EXIT_APP,             "EXIT_APP",             },
    { MSG_SUSPEND_APP,          "SUSPEND_APP",          },
    { MSG_RESUME_APP,           "RESUME_APP",           },
    { MSG_KEY_INPUT,            "KEY_INPUT",            },
    { MSG_INPUT_EVENT,          "INPUT_EVENT",          },
    { MSG_UI_EVENT,             "UI_EVENT",             },
    { MSG_TTS_EVENT,            "TTS_EVENT",            },
//  { MSG_HOTPLUG_EVENT,        "HOTPLUG_EVENT",        },
    { MSG_VOLUME_CHANGED_EVENT, "VOLUME_CHANGED_EVENT", },
    { MSG_POWER_OFF,            "POWER_OFF",            },
//  { MSG_STANDBY,              "STANDBY",              },
//  { MSG_LOW_POWER,            "LOW_POWER",            },
    { MSG_NO_POWER,             "NO_POWER",             },
    { MSG_REBOOT,               "REBOOT",               },
//  { MSG_REPLY_NO_REPLY,       "REPLY_NO_REPLY",       },
//  { MSG_REPLY_SUCCESS,        "REPLY_SUCCESS",        },
//  { MSG_REPLY_FAILED,         "REPLY_FAILED",         },
    { MSG_BAT_CHARGE_EVENT,     "BAT_CHARGE_EVENT",     },
//  { MSG_SR_INPUT,             "SR_INPUT",             },
//  { MSG_FACTORY_DEFAULT,      "FACTORY_DEFAULT",      },
//  { MSG_START_RECONNECT,      "START_RECONNECT",      },
//  { MSG_ENTER_PAIRING_MODE,   "ENTER_PAIRING_MODE",   },
//  { MSG_WIFI_DISCONNECT,      "WIFI_DISCONNECT",      },
//  { MSG_RETURN_TO_CALLER,     "RETURN_TO_CALLER",     },
    { MSG_BT_EVENT,             "BT_EVENT",             },
//  { MSG_BT_ENGINE_READY,      "BT_ENGINE_READY",      },
    { MSG_BT_MGR_EVENT,         "BT_MGR_EVENT",         },
    { MSG_TWS_EVENT,            "TWS_EVENT",            },
//  { MSG_SHUT_DOWN,            "SHUT_DOWN",            },
//  { MSG_BT_REMOTE_VOL_SYNC,   "BT_REMOTE_VOL_SYNC",   },
    { MSG_SYS_EVENT,            "SYS_EVENT",            },
//  { MSG_SENSOR_EVENT,         "SENSOR_EVENT",         },
//  { MSG_EARLY_SUSPEND_SCREEN, "EARLY_SUSPEND_SCREEN", },
//  { MSG_LATE_RESUME_SCREEN,   "LATE_RESUME_SCREEN",   },
//  { MSG_EARLY_SUSPEND_APP,    "EARLY_SUSPEND_APP",    },
//  { MSG_LATE_RESUME_APP,      "LATE_RESUME_APP",      },
//  { MSG_BT_PAIRING_EVENT,     "BT_PAIRING_EVENT",     },
//  { MSG_BLE_ANCS_AMS_SERVICE, "BLE_ANCS_AMS_SERVICE", },
//  { MSG_APP_BLE_SUPER_BQB,    "APP_BLE_SUPER_BQB",    },
    { MSG_APP_TOOL_INIT,        "APP_TOOL_INIT",        },
//  { MSG_BT_DEVICE_ERROR,      "BT_DEVICE_ERROR",      },
};

const char* sys_msg2str(uint8_t msg, const char* unmapped)
{
    const char* str = msg2str(msg, _sys_msg_str_maps, 
        ARRAY_SIZE(_sys_msg_str_maps), unmapped);

    return str;
}

