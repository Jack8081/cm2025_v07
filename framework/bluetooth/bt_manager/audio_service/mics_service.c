#include <sys/printk.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/audio/mics.h>
#include "audio_service.h"

/* FIXME: not support AICS yet */
const struct bt_gatt_service_static mics_aics_svc0 = {
	.attrs = NULL,
	.attr_count = 0,
};

const struct bt_gatt_service_static mics_aics_svc1 = {
	.attrs = NULL,
	.attr_count = 0,
};

#ifndef CONFIG_BT_MICS_SERVICE

const struct bt_gatt_service_static mics_svc = {
	.attrs = NULL,
	.attr_count = 0,
};

#else /* CONFIG_BT_MICS_SERVICE */

static void mics_mute_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	printk("mics state 0x%04x\n", value);
}

#if 0
BT_AICS_SERVICE_DEFINE(mics_aics_svc0);
#endif

BT_GATT_SERVICE_DEFINE(mics_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MICS),
	BT_GATT_CHARACTERISTIC(BT_UUID_MICS_MUTE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
                   BT_AUDIO_GATT_PERM_READ | BT_AUDIO_GATT_PERM_WRITE,
			       bt_mics_read_mute, bt_mics_write_mute, NULL),
    BT_GATT_CCC(mics_mute_cfg_changed,
                   BT_GATT_PERM_READ | BT_AUDIO_GATT_PERM_WRITE),
#if 0
	BT_GATT_INCLUDE_SERVICE((void *)attr_mics_aics_svc0),
#endif
);
#endif /* CONFIG_BT_MICS_SERVICE */
