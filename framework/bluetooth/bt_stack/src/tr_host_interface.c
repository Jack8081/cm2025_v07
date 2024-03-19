int tr_btif_get_pairedlist(u8_t index, paired_dev_info_t *dev)
{
    int ret, flags;

    flags = hostif_set_negative_prio();
    ret = tr_stack_get_storage_by_index(index, dev);
    hostif_revert_prio(flags);

    return ret;
}

int tr_btif_disallowed_reconnect(const u8_t *addr, u8_t disallowed)
{
    int ret, flags;

    flags = hostif_set_negative_prio();
    ret = tr_stack_storage_disallowed_reconnect(addr, disallowed);
    hostif_revert_prio(flags);

    return ret;
}

int tr_btif_del_storage_by_index(u8_t index)
{
    int ret, flags;

    flags = hostif_set_negative_prio();
    ret = tr_stack_del_storage_by_index(index);
    hostif_revert_prio(flags);

    return ret;
}

int tr_btif_get_remote_name(u8_t *addr, u8_t *name, int len)
{
    int ret, flags;

    flags = hostif_set_negative_prio();
    ret = tr_stack_get_storage_name_by_addr(addr, name, len);
//    if(ret < 0)
//       ebtsrv_get_inquiry_name_by_addr(addr, name, len);
    hostif_revert_prio(flags);

    return ret;
}

int tr_hostif_bt_a2dp_send_audio_data(struct bt_conn *conn, u8_t *data, u16_t len, void(*cb)(struct bt_conn *, void *))
{
#ifdef CONFIG_BT_A2DP
    int prio, ret;

    prio = hostif_set_negative_prio();
    ret = tr_bt_a2dp_send_audio_data(conn, data, len, cb);
    hostif_revert_prio(prio);

    return ret;
#else
    return -EIO;
#endif
}

int tr_hostif_bt_a2dp_send_audio_data_pass_through(struct bt_conn *conn, u8_t *data)
{
#ifdef CONFIG_BT_A2DP
    int prio, ret;

    prio = hostif_set_negative_prio();
    ret = tr_bt_a2dp_send_audio_data_pass_through(conn, data);
    hostif_revert_prio(prio);

    return ret;
#else
    return -EIO;
#endif
}

int tr_hostif_bt_a2dp_get_no_completed_pkt_num(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
    int prio, ret;

    prio = hostif_set_negative_prio();
    ret = tr_bt_a2dp_get_no_completed_pkt_num(conn);
    hostif_revert_prio(prio);

    return ret;
#else
    return -EIO;
#endif
}


int tr_hostif_bt_connect_start_security(struct bt_conn *conn)
{
    int prio, ret;

    prio = hostif_set_negative_prio();
    ret = bt_conn_set_security(conn, BT_SECURITY_L2);
    hostif_revert_prio(prio);

    return ret;
}

void tr_hostif_bt_avrcp_cttg_update_playback_status(struct bt_conn *conn, int cmd)
{
#ifdef CONFIG_BT_AVRCP
    int prio;

    prio = hostif_set_negative_prio();
    tr_bt_avrcp_cttg_update_playback_status(conn, cmd);
    hostif_revert_prio(prio);

    return;

#endif


}

int tr_hostif_bt_get_security_status(struct bt_conn *conn)
{
    int prio, status;

    prio = hostif_set_negative_prio();
    status = tr_bt_conn_get_security_status(conn);
    hostif_revert_prio(prio);

    return status;
}

int tr_hostif_bt_set_lowpower_mode(u8_t enable)
{
    int prio, ret;

    prio = hostif_set_negative_prio();
#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
    if(bti_br_tr_mode() && bti_pts_test_mode())
    {
        ret = 0;
    }
    else
        ret = tr_bt_set_lowpower_mode(enable);
#else
    ret = tr_bt_set_lowpower_mode(enable);
#endif
    hostif_revert_prio(prio);

    return ret;
}

int tr_hostif_bt_remote_version_request(struct bt_conn *conn)
{
    int prio, ret;

    prio = hostif_set_negative_prio();
    ret = tr_sdp_pnp_discover(conn);
    hostif_revert_prio(prio);

    return ret;
}

bool tr_hostif_bt_all_conn_can_in_sniff(void)
{
#ifdef CONFIG_BT_BREDR
    bool sniff;
    int prio;

    prio = hostif_set_negative_prio();
    sniff = tr_bt_all_conn_can_in_sniff();
    hostif_revert_prio(prio);

    return sniff;
#else
    return false;
#endif
}

int tr_hostif_bt_conn_le_param_update(uint16_t handle,
			    const struct bt_le_conn_param *param)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = tr_bt_conn_le_param_update(handle, param);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int tr_hostif_bt_set_link_policy(uint16_t handle, int8_t enable)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
    ret = tr_bt_set_link_policy(handle, enable);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

