#include <sys/printk.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/audio/vcs.h>
#include "audio_service.h"

#ifndef CONFIG_BT_VCS_SERVICE

const struct bt_gatt_service_static vcs_svc = {
	.attrs = NULL,
	.attr_count = 0,
};

#else /* CONFIG_BT_VCS_SERVICE */

static void volume_state_cfg_changed(const struct bt_gatt_attr *attr,
				     uint16_t value)
{
	printk("value 0x%04x\n", value);
}

static void flags_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	printk("value 0x%04x\n", value);
}


BT_GATT_SERVICE_DEFINE(vcs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_VCS),
	BT_GATT_CHARACTERISTIC(BT_UUID_VCS_STATE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_AUDIO_GATT_PERM_READ,
			       bt_vcs_read_vol_state, NULL, NULL),
	BT_GATT_CCC(volume_state_cfg_changed,
		    BT_GATT_PERM_READ | BT_AUDIO_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_VCS_CONTROL,
			       BT_GATT_CHRC_WRITE,
			       BT_AUDIO_GATT_PERM_WRITE,
			       NULL, bt_vcs_write_control, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_VCS_FLAGS,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_AUDIO_GATT_PERM_READ,
			       bt_vcs_read_flags, NULL, NULL),
	BT_GATT_CCC(flags_cfg_changed,
		    BT_GATT_PERM_READ | BT_AUDIO_GATT_PERM_WRITE),
);
#endif /* CONFIG_BT_VCS_SERVICE */
