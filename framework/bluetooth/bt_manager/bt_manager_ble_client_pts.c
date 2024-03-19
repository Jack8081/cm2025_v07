#define SYS_LOG_DOMAIN "btmgr_ble_pts"


#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <sys_event.h>
#include <btservice_api.h>
#include <shell/shell.h>
#include <acts_bluetooth/host_interface.h>
#include <property_manager.h>

struct bt_conn *pts_ble_conn;

static int set_pts_name(const struct shell *shell, int argc, char *argv[])
{
    char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts hfp_cmd xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("cmd:%s\n", cmd);

    set_bqb_full_name(cmd);
    return 0;

}

static int set_pts_ext_connect_flag(const struct shell *shell, int argc, char *argv[])
{
    char val;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts xx");
		return -EINVAL;
	}

	val = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("cmd val = %d\n", val);

    tr_bt_set_ext_connect_flag(val);
    return 0;

}

static int set_pts_ccc_auto_flag(const struct shell *shell, int argc, char *argv[])
{
    char val;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts xx");
		return -EINVAL;
	}

	val = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("cmd val = %d\n", val);

    tr_bt_set_ccc_flag(val);
    return 0;

}

static int gatt_disconnect(const struct shell *shell, size_t argc, char *argv[])
{

    int err;

    err = bt_conn_disconnect(pts_ble_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        SYS_LOG_INF("Disconnection failed (err %d)", err);
        return err;
    }

    return 0;
}

#define CHAR_SIZE_MAX           512
static struct bt_gatt_write_params write_params;
static uint8_t gatt_write_buf[CHAR_SIZE_MAX];


void set_ble_pts_con(struct bt_conn *conn)
{
    if (!pts_ble_conn) {
        pts_ble_conn = bt_conn_ref(conn);
    }
}

void clear_ble_pts_con(struct bt_conn *conn)
{
    if (pts_ble_conn == conn) {
        bt_conn_unref(pts_ble_conn);
        pts_ble_conn = NULL;
    }
}

static void write_func(struct bt_conn *conn, uint8_t err,
               struct bt_gatt_write_params *params)
{
    SYS_LOG_INF("Write complete: err 0x%02x", err);

    (void)memset(&write_params, 0, sizeof(write_params));
}

static int gatt_write(const struct shell *shell, size_t argc, char *argv[])
{
    int err;
    uint16_t handle, offset;

    if (!pts_ble_conn) {
        SYS_LOG_INF("Not connected");
        return -ENOEXEC;
    }

    if (write_params.func) {
        SYS_LOG_INF("Write ongoing");
        return -ENOEXEC;
    }

    handle = strtoul(argv[1], NULL, 16);
    offset = strtoul(argv[2], NULL, 16);

    write_params.length = hex2bin(argv[3], strlen(argv[3]),
                      gatt_write_buf, sizeof(gatt_write_buf));
    if (write_params.length == 0) {
        SYS_LOG_INF("No data set");
        return -ENOEXEC;
    }

    write_params.data = gatt_write_buf;
    write_params.handle = handle;
    write_params.offset = offset;
    write_params.func = write_func;

    err = bt_gatt_write(pts_ble_conn, &write_params);
    if (err) {
        write_params.func = NULL;
        SYS_LOG_INF("Write failed (err %d)", err);
    } else {
        SYS_LOG_INF("Write pending");
    }

    return err;
}

static struct write_stats {
	uint32_t count;
	uint32_t len;
	uint32_t total;
	uint32_t rate;
} write_stats;

static void update_write_stats(uint16_t len)
{
	static uint32_t cycle_stamp;
	uint32_t delta;

	delta = k_cycle_get_32() - cycle_stamp;
	delta = (uint32_t)k_cyc_to_ns_floor64(delta);

	if (!delta) {
		delta = 1;
	}

	write_stats.count++;
	write_stats.total += len;

	/* if last data rx-ed was greater than 1 second in the past,
	 * reset the metrics.
	 */
	if (delta > 1000000000) {
		write_stats.len = 0U;
		write_stats.rate = 0U;
		cycle_stamp = k_cycle_get_32();
	} else {
		write_stats.len += len;
		write_stats.rate = ((uint64_t)write_stats.len << 3) *
				   1000000000U / delta;
	}
}

static void reset_write_stats(void)
{
	memset(&write_stats, 0, sizeof(write_stats));
}

static void print_write_stats(void)
{
	SYS_LOG_INF("Write #%u: %u bytes (%u bps)",
		    write_stats.count, write_stats.total, write_stats.rate);
}

static void write_without_rsp_cb(struct bt_conn *conn, void *user_data)
{
	uint16_t len = POINTER_TO_UINT(user_data);

	update_write_stats(len);

	print_write_stats();
}

static int gatt_write_without_rsp(const struct shell *shell,
				 size_t argc, char *argv[])
{
	uint16_t handle;
	uint16_t repeat;
	int err;
	uint16_t len;
	bool sign;
	bt_gatt_complete_func_t func = NULL;

	if (!pts_ble_conn) {
		SYS_LOG_INF("Not connected");
		return -ENOEXEC;
	}

	sign = !strcmp(argv[0], "signed-write");
	if (!sign) {
		if (!strcmp(argv[0], "write-without-response-cb")) {
			func = write_without_rsp_cb;
			reset_write_stats();
		}
	}

	handle = strtoul(argv[1], NULL, 16);
	gatt_write_buf[0] = strtoul(argv[2], NULL, 16);
	len = 1U;

	if (argc > 3) {
		int i;

		len = MIN(strtoul(argv[3], NULL, 16), sizeof(gatt_write_buf));

		for (i = 1; i < len; i++) {
			gatt_write_buf[i] = gatt_write_buf[0];
		}
	}

	repeat = 0U;

	if (argc > 4) {
		repeat = strtoul(argv[4], NULL, 16);
	}

	if (!repeat) {
		repeat = 1U;
	}

	while (repeat--) {
		err = bt_gatt_write_without_response_cb(pts_ble_conn, handle,
							gatt_write_buf, len,
							sign, func,
							UINT_TO_POINTER(len));
		if (err) {
			break;
		}

		k_yield();

	}

	shell_print(shell, "Write Complete (err %d)", err);
	return err;
}

static struct bt_gatt_read_params read_params;

static uint8_t read_func(struct bt_conn *conn, uint8_t err,
			 struct bt_gatt_read_params *params,
			 const void *data, uint16_t length)
{
	SYS_LOG_INF("Read complete: err 0x%02x length %u", err, length);

	if (!data) {
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int gatt_read(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!pts_ble_conn) {
		SYS_LOG_INF("Not connected");
		return -ENOEXEC;
	}

	if (read_params.func) {
		SYS_LOG_INF("Read ongoing");
		return -ENOEXEC;
	}

	read_params.func = read_func;
	read_params.handle_count = 1;
	read_params.single.handle = strtoul(argv[1], NULL, 16);
	read_params.single.offset = 0U;

	if (argc > 2) {
		read_params.single.offset = strtoul(argv[2], NULL, 16);
	}

	err = bt_gatt_read(pts_ble_conn, &read_params);
	if (err) {
		SYS_LOG_INF("Read failed (err %d)", err);
	} else {
		SYS_LOG_INF("Read pending");
	}

	return err;
}


SHELL_STATIC_SUBCMD_SET_CREATE(bt_mgr_ble_pts_cmds,
    SHELL_CMD(setname, NULL, "pts set pts dongle name", set_pts_name),
    SHELL_CMD(setextflag, NULL, "pts set pts ext flag", set_pts_ext_connect_flag),
    SHELL_CMD(setccc, NULL, "pts set pts ccc flag", set_pts_ccc_auto_flag),
    SHELL_CMD(write, NULL, "<handle> <offset> <data>", gatt_write),
    SHELL_CMD(write_cmd, NULL, "<handle> <data> [length] [repeat]", gatt_write_without_rsp),
    SHELL_CMD(read, NULL, "<handle> <offset>", gatt_read),
    SHELL_CMD(disconnect, NULL, "", gatt_disconnect),
	SHELL_SUBCMD_SET_END
);

static int cmd_bt_ble_pts(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(blepts, &bt_mgr_ble_pts_cmds, "Bluetooth manager ble pts test shell commands", cmd_bt_ble_pts);

