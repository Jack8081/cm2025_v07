/*static int tr_leaudio_qos_init(void)
{
	struct bt_audio_preferred_qos_default qos;
	uint32_t delay;

	qos.framing = BT_AUDIO_UNFRAMED_SUPPORTED;
	qos.phy = BT_AUDIO_PHY_2M;
	qos.rtn = 3;

#ifdef ENABLE_75_MS_INTERVAL
	qos.latency = 8;
	delay = 7500;
#else
	qos.latency = 10;
	delay = 10000;
#endif
	delay += 1000;

	qos.delay_min = delay;
	qos.delay_max = delay;
	qos.preferred_delay_min = delay;
	qos.preferred_delay_max = delay;

	bt_manager_audio_server_qos_init(true, &qos);

	delay = 2000;

	qos.delay_min = delay;
	qos.delay_max = delay;
	qos.preferred_delay_min = delay;
	qos.preferred_delay_max = delay;

	bt_manager_audio_server_qos_init(false, &qos);

	return 0;
}
*/


static int tr_leaudio_le_audio_init(void)
{
	struct bt_audio_role role = {0};
	
    role.num_master_conn = CONFIG_BT_TRANS_SUPPORT_CIS_NUMBER;
    role.unicast_client = 1;
    role.volume_controller = 1;
    role.microphone_controller = 1;
    role.media_device = 1;
    role.call_gateway = 1; //TODO
    //	role.set_member = 1;

#ifdef CONFIG_LE_AUDIO_PHONE
    role.encryption = 1;
#endif

#ifdef CONFIG_LE_AUDIO_BQB
    role.encryption = 1;
    role.num_slave_conn = 1;

    role.media_controller = 1;
    role.media_device = 1;

    role.call_terminal = 1;
    role.call_gateway = 1;

    role.set_coordinator = 1;
    role.encryption = 1;

#endif

//    bt_manager_audio_get_sirk(role.set_sirk, sizeof(role.set_sirk));
//    bt_manager_audio_get_rank_and_size(&role.set_rank, &role.set_size);
//    role.sirk_enabled = 1;

    role.disable_trigger = 1;

    bt_manager_audio_init(BT_TYPE_LE, &role);

//    tr_leaudio_cap_init();
//    tr_leaudio_qos_init();

    bt_manager_audio_start(BT_TYPE_LE);

	return 0;
}

