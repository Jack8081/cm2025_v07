/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usound_bqb.h"
#include <sys_wakelock.h>

static struct usound_app_t *p_usound;

static int bt_le_audio_init(void)
{
	struct bt_audio_role role = {0};

	role.num_master_conn = 1;

	role.unicast_client = 1;
	role.volume_controller = 1;
	role.microphone_controller = 1;
	role.media_device = 1;
	role.media_controller = 1;
	role.call_gateway = 1;
	role.encryption = 1;
	role.set_coordinator = 1;

	bt_manager_audio_init(BT_TYPE_LE, &role);

	bt_manager_audio_start(BT_TYPE_LE);

	return 0;
}

static int bt_le_audio_exit(void)
{
	bt_manager_audio_stop(BT_TYPE_LE);

	bt_manager_audio_exit(BT_TYPE_LE);

	return 0;
}

static int _usound_init(void)
{
	if (p_usound) {
		return 0;
	}

	p_usound = app_mem_malloc(sizeof(struct usound_app_t));
	if (!p_usound) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}

	memset(p_usound, 0, sizeof(struct usound_app_t));

	bt_le_audio_init();
    sys_wake_lock(FULL_WAKE_LOCK);

	SYS_LOG_INF("init ok\n");

	return 0;
}

void _usound_exit(void)
{
	if (!p_usound) {
		goto exit;
	}

	usound_stop_playback();
	usound_exit_playback();

	usound_stop_capture();
	usound_exit_capture();

	bt_le_audio_exit();

	app_mem_free(p_usound);
    sys_wake_unlock(FULL_WAKE_LOCK);

	p_usound = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

exit:
	app_manager_thread_exit(app_manager_get_current_app());

	SYS_LOG_INF("exit ok\n");
}

struct usound_app_t *usound_get_app(void)
{
	return p_usound;
}

static void _usound_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = { 0 };
	bool terminated = false;

	if (_usound_init()) {
		SYS_LOG_ERR("init failed");
		_usound_exit();
		return;
	}

	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n", msg.type, msg.value);
			switch (msg.type) {
			case MSG_BT_EVENT:
				usound_bt_event_proc(&msg);
				break;
			case MSG_EXIT_APP:
				_usound_exit();
				terminated = true;
				break;
			default:
				break;
			}
			if (msg.callback) {
				msg.callback(&msg, 0, NULL);
			}
		}
		if (!terminated) {
			thread_timer_handle_expired();
		}
	}
}

APP_DEFINE(usound_bqb, share_stack_area, sizeof(share_stack_area),
	CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	_usound_main_loop, NULL);
#if 1
#include <shell/shell.h>

static int mas_leaudio_config_sink(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_audio_codec codec;
	uint32_t type;
	uint8_t id;

	if (argc != 3) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	type = strtoul(argv[2], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, type: %d", id, type);

	codec.handle = p_usound->handle;
	codec.id = id;
	codec.dir = BT_AUDIO_DIR_SINK;
	codec.format = BT_AUDIO_CODEC_LC3;
	codec.target_latency = BT_AUDIO_TARGET_LATENCY_LOW;
	codec.target_phy = BT_AUDIO_TARGET_PHY_2M;
	codec.blocks = 1;
	codec.locations = BT_AUDIO_LOCATIONS_FL;

	switch (type) {
	case 82:
		codec.sample_rate = 8;
		codec.octets = 30;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 162:
		codec.sample_rate = 16;
		codec.octets = 40;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 242:
		codec.sample_rate = 24;
		codec.octets = 60;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 322:
		codec.sample_rate = 32;
		codec.octets = 80;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 482:
		codec.sample_rate = 48;
		codec.octets = 100;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 484:
		codec.sample_rate = 48;
		codec.octets = 120;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 486:
		codec.sample_rate = 48;
		codec.octets = 155;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	default:
		return -EINVAL;
	}

	return bt_manager_audio_stream_config_codec(&codec);
}

static int mas_leaudio_config_sink_right(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_audio_codec codec;
	uint32_t type;
	uint8_t id;

	if (argc != 3) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	type = strtoul(argv[2], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, type: %d", id, type);

	codec.handle = p_usound->handle;
	codec.id = id;
	codec.dir = BT_AUDIO_DIR_SINK;
	codec.format = BT_AUDIO_CODEC_LC3;
	codec.target_latency = BT_AUDIO_TARGET_LATENCY_LOW;
	codec.target_phy = BT_AUDIO_TARGET_PHY_2M;
	codec.blocks = 1;
	codec.locations = BT_AUDIO_LOCATIONS_FR;

	switch (type) {
	case 82:
		codec.sample_rate = 8;
		codec.octets = 30;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 162:
		codec.sample_rate = 16;
		codec.octets = 40;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 242:
		codec.sample_rate = 24;
		codec.octets = 60;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 322:
		codec.sample_rate = 32;
		codec.octets = 80;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 482:
		codec.sample_rate = 48;
		codec.octets = 100;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 484:
		codec.sample_rate = 48;
		codec.octets = 120;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 486:
		codec.sample_rate = 48;
		codec.octets = 155;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	default:
		return -EINVAL;
	}

	return bt_manager_audio_stream_config_codec(&codec);
}

static int mas_leaudio_config_source(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_audio_codec codec;
	uint32_t type;
	uint8_t id;

	if (argc != 3) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	type = strtoul(argv[2], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, type: %d", id, type);

	codec.handle = p_usound->handle;
	codec.id = id;
	codec.dir = BT_AUDIO_DIR_SOURCE;
	codec.format = BT_AUDIO_CODEC_LC3;
	codec.target_latency = BT_AUDIO_TARGET_LATENCY_LOW;
	codec.target_phy = BT_AUDIO_TARGET_PHY_2M;
	codec.blocks = 1;
	codec.locations = BT_AUDIO_LOCATIONS_FL;

	switch (type) {
	case 162:
		codec.sample_rate = 16;
		codec.octets = 40;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 242:
		codec.sample_rate = 24;
		codec.octets = 60;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 322:
		codec.sample_rate = 32;
		codec.octets = 80;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 482:
		codec.sample_rate = 48;
		codec.octets = 100;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 484:
		codec.sample_rate = 48;
		codec.octets = 120;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	case 486:
		codec.sample_rate = 48;
		codec.octets = 155;
		codec.frame_duration = BT_FRAME_DURATION_10MS;
		break;
	default:
		return -EINVAL;
	}

	return bt_manager_audio_stream_config_codec(&codec);
}

static int mas_leaudio_qos_sink(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	struct bt_audio_qoss_1 *qoss = &usound->qoss;
	struct bt_audio_qos *qos = &qoss->qos[0];
	uint32_t type;
	uint8_t id;

	if (argc != 3) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	type = strtoul(argv[2], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, type: %d", id, type);

	memset(qoss, 0, sizeof(*qoss));

	qoss->framing = BT_AUDIO_UNFRAMED;
	qoss->num_of_qos = 1;
	qoss->sca = BT_GAP_SCA_UNKNOWN;
	qoss->packing = BT_AUDIO_PACKING_SEQUENTIAL;
	qoss->interval_m = 10000;
	qoss->delay_m = 40000;
	qoss->processing_m = 4000;

	qos->handle = usound->handle;
	qos->id_m_to_s = id;
	qos->phy_m = BT_AUDIO_PHY_2M;

	switch (type) {
	case 821:
		qos->max_sdu_m = 30;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		break;
	case 1621:
		qos->max_sdu_m = 40;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		break;
	case 1622:
		qos->max_sdu_m = 40;
		qos->rtn_m = 13;
		qoss->latency_m = 95;
		break;
	case 2421:
		qos->max_sdu_m = 60;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		break;
	case 2422:
		qos->max_sdu_m = 60;
		qos->rtn_m = 13;
		qoss->latency_m = 95;
		break;
	case 3221:
		qos->max_sdu_m = 80;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		break;
	case 4821:
		qos->max_sdu_m = 100;
		qos->rtn_m = 5;
		qoss->latency_m = 20;
		break;
	case 4822:
		qos->max_sdu_m = 100;
		qos->rtn_m = 13;
		qoss->latency_m = 95;
		break;
	case 4841:
		qos->max_sdu_m = 120;
		qos->rtn_m = 5;
		qoss->latency_m = 20;
		break;
	case 4842:
		qos->max_sdu_m = 120;
		qos->rtn_m = 13;
		qoss->latency_m = 100;
		break;
	case 4861:
		qos->max_sdu_m = 155;
		qos->rtn_m = 5;
		qoss->latency_m = 20;
		break;
	case 4862:
		qos->max_sdu_m = 155;
		qos->rtn_m = 13;
		qoss->latency_m = 100;
		break;
	default:
		return -EINVAL;
	}

	return bt_manager_audio_stream_config_qos((struct bt_audio_qoss *)qoss);
}

static struct bt_audio_qoss_2 mas_qoss_2;

static int mas_leaudio_qos_sink2(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	struct bt_audio_qoss_2 *qoss = &mas_qoss_2;
	struct bt_audio_qos *qos = &qoss->qos[0];
	struct bt_audio_qos *qos2 = &qoss->qos[1];
	uint32_t type;
	uint8_t id;
	uint8_t id2;

	if (argc != 4) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	id2 = strtoul(argv[2], (char **) NULL, 0);
	type = strtoul(argv[3], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, id2: %d, type: %d", id, id2, type);

	memset(qoss, 0, sizeof(*qoss));

	qoss->framing = BT_AUDIO_UNFRAMED;
	qoss->num_of_qos = 2;
	qoss->sca = BT_GAP_SCA_UNKNOWN;
	qoss->packing = BT_AUDIO_PACKING_SEQUENTIAL;
	qoss->interval_m = 10000;
	qoss->delay_m = 40000;
	qoss->processing_m = 4000;

	qos->handle = usound->handle;
	qos->id_m_to_s = id;
	qos->phy_m = BT_AUDIO_PHY_2M;

	qos2->handle = usound->handle;
	qos2->id_m_to_s = id2;
	qos2->phy_m = BT_AUDIO_PHY_2M;

	switch (type) {
	case 821:
		qos->max_sdu_m = 30;
		qos->rtn_m = 2;
		qos2->max_sdu_m = 30;
		qos2->rtn_m = 2;
		qoss->latency_m = 10;
		break;
	case 1621:
		qos->max_sdu_m = 40;
		qos->rtn_m = 2;
		qos2->max_sdu_m = 40;
		qos2->rtn_m = 2;
		qoss->latency_m = 10;
		break;
	case 1622:
		qos->max_sdu_m = 40;
		qos->rtn_m = 13;
		qos2->max_sdu_m = 40;
		qos2->rtn_m = 13;
		qoss->latency_m = 95;
		break;
	case 2421:
		qos->max_sdu_m = 60;
		qos->rtn_m = 2;
		qos2->max_sdu_m = 60;
		qos2->rtn_m = 2;
		qoss->latency_m = 10;
		break;
	case 2422:
		qos->max_sdu_m = 60;
		qos->rtn_m = 13;
		qos2->max_sdu_m = 60;
		qos2->rtn_m = 13;
		qoss->latency_m = 95;
		break;
	case 3221:
		qos->max_sdu_m = 80;
		qos->rtn_m = 2;
		qos2->max_sdu_m = 80;
		qos2->rtn_m = 2;
		qoss->latency_m = 10;
		break;
	case 4821:
		qos->max_sdu_m = 100;
		qos->rtn_m = 5;
		qos2->max_sdu_m = 100;
		qos2->rtn_m = 5;
		qoss->latency_m = 20;
		break;
	case 4822:
		qos->max_sdu_m = 100;
		qos->rtn_m = 13;
		qos2->max_sdu_m = 100;
		qos2->rtn_m = 13;
		qoss->latency_m = 95;
		break;
	case 4841:
		qos->max_sdu_m = 120;
		qos->rtn_m = 5;
		qos2->max_sdu_m = 120;
		qos2->rtn_m = 5;
		qoss->latency_m = 20;
		break;
	case 4842:
		qos->max_sdu_m = 120;
		qos->rtn_m = 13;
		qos2->max_sdu_m = 120;
		qos2->rtn_m = 13;
		qoss->latency_m = 100;
		break;
	case 4861:
		qos->max_sdu_m = 155;
		qos->rtn_m = 5;
		qos2->max_sdu_m = 155;
		qos2->rtn_m = 5;
		qoss->latency_m = 20;
		break;
	case 4862:
		qos->max_sdu_m = 155;
		qos->rtn_m = 13;
		qos2->max_sdu_m = 155;
		qos2->rtn_m = 13;
		qoss->latency_m = 100;
		break;
	default:
		return -EINVAL;
	}

	return bt_manager_audio_stream_config_qos((struct bt_audio_qoss *)qoss);
}

static int mas_leaudio_qos_source(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	struct bt_audio_qoss_1 *qoss = &usound->qoss;
	struct bt_audio_qos *qos = &qoss->qos[0];
	uint32_t type;
	uint8_t id;

	if (argc != 3) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	type = strtoul(argv[2], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, type: %d", id, type);

	memset(qoss, 0, sizeof(*qoss));

	qoss->framing = BT_AUDIO_UNFRAMED;
	qoss->num_of_qos = 1;
	qoss->sca = BT_GAP_SCA_UNKNOWN;
	qoss->packing = BT_AUDIO_PACKING_SEQUENTIAL;
	qoss->interval_s = 10000;
	qoss->delay_s = 40000;
	qoss->processing_s = 4000;

	qos->handle = usound->handle;
	qos->id_s_to_m = id;
	qos->phy_s = BT_AUDIO_PHY_2M;

	switch (type) {
	case 1621:
		qos->max_sdu_s = 40;
		qos->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 1622:
		qos->max_sdu_s = 40;
		qos->rtn_s = 13;
		qoss->latency_s = 95;
		break;
	case 2421:
		qos->max_sdu_s = 60;
		qos->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 3221:
		qos->max_sdu_s = 80;
		qos->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 4821:
		qos->max_sdu_s = 100;
		qos->rtn_s = 5;
		qoss->latency_s = 20;
		break;
	case 4822:
		qos->max_sdu_s = 100;
		qos->rtn_s = 13;
		qoss->latency_s = 95;
		break;
	case 4841:
		qos->max_sdu_s = 120;
		qos->rtn_s = 5;
		qoss->latency_s = 20;
		break;
	case 4842:
		qos->max_sdu_s = 120;
		qos->rtn_s = 13;
		qoss->latency_s = 100;
		break;
	case 4861:
		qos->max_sdu_s = 155;
		qos->rtn_s = 5;
		qoss->latency_s = 20;
		break;
	case 4862:
		qos->max_sdu_s = 155;
		qos->rtn_s = 13;
		qoss->latency_s = 100;
		break;
	default:
		return -EINVAL;
	}

	return bt_manager_audio_stream_config_qos((struct bt_audio_qoss *)qoss);
}

static int mas_leaudio_qos_source2(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	struct bt_audio_qoss_2 *qoss = &mas_qoss_2;
	struct bt_audio_qos *qos = &qoss->qos[0];
	struct bt_audio_qos *qos2 = &qoss->qos[1];
	uint32_t type;
	uint8_t id;
	uint8_t id2;

	if (argc != 4) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	id2 = strtoul(argv[2], (char **) NULL, 0);
	type = strtoul(argv[3], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, id2: %d, type: %d", id, id2, type);

	memset(qoss, 0, sizeof(*qoss));

	qoss->framing = BT_AUDIO_UNFRAMED;
	qoss->num_of_qos = 2;
	qoss->sca = BT_GAP_SCA_UNKNOWN;
	qoss->packing = BT_AUDIO_PACKING_SEQUENTIAL;
	qoss->interval_s = 10000;
	qoss->delay_s = 40000;
	qoss->processing_s = 4000;

	qos->handle = usound->handle;
	qos->id_s_to_m = id;
	qos->phy_s = BT_AUDIO_PHY_2M;

	qos2->handle = usound->handle;
	qos2->id_s_to_m = id2;
	qos2->phy_s = BT_AUDIO_PHY_2M;

	switch (type) {
	case 1621:
		qos->max_sdu_s = 40;
		qos->rtn_s = 2;
		qos2->max_sdu_s = 40;
		qos2->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 1622:
		qos->max_sdu_s = 40;
		qos->rtn_s = 13;
		qos2->max_sdu_s = 40;
		qos2->rtn_s = 13;
		qoss->latency_s = 95;
		break;
	case 2421:
		qos->max_sdu_s = 60;
		qos->rtn_s = 2;
		qos2->max_sdu_s = 60;
		qos2->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 3221:
		qos->max_sdu_s = 80;
		qos->rtn_s = 2;
		qos2->max_sdu_s = 80;
		qos2->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 4821:
		qos->max_sdu_s = 100;
		qos->rtn_s = 5;
		qos2->max_sdu_s = 100;
		qos2->rtn_s = 5;
		qoss->latency_s = 20;
		break;
	case 4822:
		qos->max_sdu_s = 100;
		qos->rtn_s = 13;
		qos2->max_sdu_s = 100;
		qos2->rtn_s = 13;
		qoss->latency_s = 95;
		break;
	case 4841:
		qos->max_sdu_s = 120;
		qos->rtn_s = 5;
		qos2->max_sdu_s = 120;
		qos2->rtn_s = 5;
		qoss->latency_s = 20;
		break;
	case 4842:
		qos->max_sdu_s = 120;
		qos->rtn_s = 13;
		qos->max_sdu_s = 120;
		qos->rtn_s = 13;
		qoss->latency_s = 100;
		break;
	case 4861:
		qos->max_sdu_s = 155;
		qos->rtn_s = 5;
		qos2->max_sdu_s = 155;
		qos2->rtn_s = 5;
		qoss->latency_s = 20;
		break;
	case 4862:
		qos->max_sdu_s = 155;
		qos->rtn_s = 13;
		qos2->max_sdu_s = 155;
		qos2->rtn_s = 13;
		qoss->latency_s = 100;
		break;
	default:
		return -EINVAL;
	}

	return bt_manager_audio_stream_config_qos((struct bt_audio_qoss *)qoss);
}

static int mas_leaudio_qos(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	struct bt_audio_qoss_1 *qoss = &usound->qoss;
	struct bt_audio_qos *qos = &qoss->qos[0];
	uint32_t type;
	uint8_t id;	/* sink */
	uint8_t id2;	/* source */

	if (argc != 4) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	id2 = strtoul(argv[2], (char **) NULL, 0);
	type = strtoul(argv[3], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, id2: %d, type: %d", id, id2, type);

	memset(qoss, 0, sizeof(*qoss));

	qoss->framing = BT_AUDIO_UNFRAMED;
	qoss->num_of_qos = 1;
	qoss->sca = BT_GAP_SCA_UNKNOWN;
	qoss->packing = BT_AUDIO_PACKING_SEQUENTIAL;
	qoss->interval_m = 10000;
	qoss->interval_s = 10000;
	qoss->delay_m = 40000;
	qoss->delay_s = 40000;
	qoss->processing_m = 4000;
	qoss->processing_s = 4000;

	qos->handle = usound->handle;
	qos->id_m_to_s = id;
	qos->id_s_to_m = id2;
	qos->phy_m = BT_AUDIO_PHY_2M;
	qos->phy_s = BT_AUDIO_PHY_2M;

	switch (type) {
	case 1621:
		qos->max_sdu_m = 40;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		qos->max_sdu_s = 40;
		qos->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 1622:
		qos->max_sdu_m = 40;
		qos->rtn_m = 13;
		qoss->latency_m = 95;
		qos->max_sdu_s = 40;
		qos->rtn_s = 13;
		qoss->latency_s = 95;
		break;
	case 2421:
		qos->max_sdu_m = 60;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		qos->max_sdu_s = 60;
		qos->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 2422:
		qos->max_sdu_m = 60;
		qos->rtn_m = 13;
		qoss->latency_m = 95;
		qos->max_sdu_s = 60;
		qos->rtn_s = 13;
		qoss->latency_s = 95;
		break;
	case 3221:
		qos->max_sdu_m = 80;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		qos->max_sdu_s = 80;
		qos->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 4821:
		qos->max_sdu_m = 100;
		qos->rtn_m = 5;
		qoss->latency_m = 20;
		qos->max_sdu_s = 100;
		qos->rtn_s = 5;
		qoss->latency_s = 20;
		break;
	case 4841:
		qos->max_sdu_m = 120;
		qos->rtn_m = 5;
		qoss->latency_m = 20;
		qos->max_sdu_s = 120;
		qos->rtn_s = 5;
		qoss->latency_s = 20;
		break;
	case 4861:
		qos->max_sdu_m = 155;
		qos->rtn_m = 2;
		qoss->latency_m = 20;
		qos->max_sdu_s = 155;
		qos->rtn_s = 2;
		qoss->latency_s = 20;
		break;
	default:
		return -EINVAL;
	}

	return bt_manager_audio_stream_config_qos((struct bt_audio_qoss *)qoss);
}

static int mas_leaudio_qos2(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	struct bt_audio_qoss_2 *qoss = &mas_qoss_2;
	struct bt_audio_qos *qos = &qoss->qos[0];
	struct bt_audio_qos *qos2 = &qoss->qos[1];
	uint32_t type;
	uint8_t id;	/* sink */
	uint8_t id2;	/* source */

	if (argc != 4) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	id2 = strtoul(argv[2], (char **) NULL, 0);
	type = strtoul(argv[3], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, id2: %d, type: %d", id, id2, type);

	memset(qoss, 0, sizeof(*qoss));

	qoss->framing = BT_AUDIO_UNFRAMED;
	qoss->num_of_qos = 2;
	qoss->sca = BT_GAP_SCA_UNKNOWN;
	qoss->packing = BT_AUDIO_PACKING_SEQUENTIAL;
	qoss->interval_m = 10000;
	qoss->interval_s = 10000;
	qoss->delay_m = 40000;
	qoss->delay_s = 40000;
	qoss->processing_m = 4000;
	qoss->processing_s = 4000;

	qos->handle = usound->handle;
	qos->id_m_to_s = id;
	qos->phy_m = BT_AUDIO_PHY_2M;

	qos2->handle = usound->handle;
	qos2->id_s_to_m = id2;
	qos2->phy_s = BT_AUDIO_PHY_2M;

	switch (type) {
	case 1621:
		qos->max_sdu_m = 40;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		qos2->max_sdu_s = 40;
		qos2->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 1622:
		qos->max_sdu_m = 40;
		qos->rtn_m = 13;
		qoss->latency_m = 95;
		qos2->max_sdu_s = 40;
		qos2->rtn_s = 13;
		qoss->latency_s = 95;
		break;
	case 2421:
		qos->max_sdu_m = 60;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		qos2->max_sdu_s = 60;
		qos2->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 2422:
		qos->max_sdu_m = 60;
		qos->rtn_m = 13;
		qoss->latency_m = 95;
		qos2->max_sdu_s = 60;
		qos2->rtn_s = 13;
		qoss->latency_s = 95;
		break;
	case 3221:
		qos->max_sdu_m = 80;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		qos2->max_sdu_s = 80;
		qos2->rtn_s = 2;
		qoss->latency_s = 10;
		break;
	case 4821:
		qos->max_sdu_m = 100;
		qos->rtn_m = 5;
		qoss->latency_m = 20;
		qos2->max_sdu_s = 100;
		qos2->rtn_s = 5;
		qoss->latency_s = 20;
		break;
	case 4841:
		qos->max_sdu_m = 120;
		qos->rtn_m = 5;
		qoss->latency_m = 20;
		qos2->max_sdu_s = 120;
		qos2->rtn_s = 5;
		qoss->latency_s = 20;
		break;
	case 4861:
		qos->max_sdu_m = 155;
		qos->rtn_m = 2;
		qoss->latency_m = 20;
		qos2->max_sdu_s = 155;
		qos2->rtn_s = 2;
		qoss->latency_s = 20;
		break;
	default:
		return -EINVAL;
	}

	return bt_manager_audio_stream_config_qos((struct bt_audio_qoss *)qoss);
}

static int mas_leaudio_qos3(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	struct bt_audio_qoss_2 *qoss = &mas_qoss_2;
	struct bt_audio_qos *qos = &qoss->qos[0];
	struct bt_audio_qos *qos2 = &qoss->qos[1];
	uint32_t type;
	uint8_t id;	/* cis1 sink */
	uint8_t id2;	/* cis1 source */
	uint8_t id3;	/* cis2 sink */

	if (argc != 5) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	id2 = strtoul(argv[2], (char **) NULL, 0);
	id3 = strtoul(argv[3], (char **) NULL, 0);
	type = strtoul(argv[4], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, id2: %d, id3: %d, type: %d", id, id2, id3, type);

	memset(qoss, 0, sizeof(*qoss));

	qoss->framing = BT_AUDIO_UNFRAMED;
	qoss->num_of_qos = 2;
	qoss->sca = BT_GAP_SCA_UNKNOWN;
	qoss->packing = BT_AUDIO_PACKING_SEQUENTIAL;
	qoss->interval_m = 10000;
	qoss->interval_s = 10000;
	qoss->delay_m = 40000;
	qoss->delay_s = 40000;
	qoss->processing_m = 4000;
	qoss->processing_s = 4000;

	qos->handle = usound->handle;
	qos->id_m_to_s = id;
	qos->id_s_to_m = id2;
	qos->phy_m = BT_AUDIO_PHY_2M;
	qos->phy_s = BT_AUDIO_PHY_2M;

	qos2->handle = usound->handle;
	qos2->id_m_to_s = id3;
	qos2->phy_m = BT_AUDIO_PHY_2M;

	switch (type) {
	case 821:
		qos->max_sdu_m = 30;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		qos->max_sdu_s = 30;
		qos->rtn_s = 2;
		qoss->latency_s = 10;

		qos2->max_sdu_m = 30;
		qos2->rtn_m = 2;
		break;
	case 1621:
		qos->max_sdu_m = 40;
		qos->rtn_m = 2;
		qoss->latency_m = 10;
		qos->max_sdu_s = 40;
		qos->rtn_s = 2;
		qoss->latency_s = 10;

		qos2->max_sdu_m = 40;
		qos2->rtn_m = 2;
		break;
	default:
		return -EINVAL;
	}

	return bt_manager_audio_stream_config_qos((struct bt_audio_qoss *)qoss);
}

static int mas_leaudio_enable_sink(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	usound->source_chan.handle = usound->handle;
	usound->source_chan.id = id;

	return bt_manager_audio_stream_enable_single(&usound->source_chan, BT_AUDIO_CONTEXT_MEDIA);
}

static int mas_leaudio_enable_source(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	usound->sink_chan.handle = usound->handle;
	usound->sink_chan.id = id;

	return bt_manager_audio_stream_enable_single(&usound->sink_chan, BT_AUDIO_CONTEXT_CONVERSATIONAL);
}

static int mas_leaudio_enable_sink2(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint32_t type;
	uint8_t id;

	if (argc != 3) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	type = strtoul(argv[2], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, type: 0x%x", id, type);

	usound->source_chan.handle = usound->handle;
	usound->source_chan.id = id;

	return bt_manager_audio_stream_enable_single(&usound->source_chan, type);
}

static int mas_leaudio_enable_source2(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint32_t type;
	uint8_t id;

	if (argc != 3) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	type = strtoul(argv[2], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, type: 0x%x", id, type);

	usound->sink_chan.handle = usound->handle;
	usound->sink_chan.id = id;

	return bt_manager_audio_stream_enable_single(&usound->sink_chan, type);
}

static int mas_leaudio_enable2(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	struct bt_audio_chan *chans[2];
	uint32_t type;
	uint8_t id;
	uint8_t id2;

	if (argc != 4) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	id2 = strtoul(argv[2], (char **) NULL, 0);
	type = strtoul(argv[3], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, id2: %d, type: 0x%x", id, id2, type);

	usound->source_chan.handle = usound->handle;
	usound->source_chan.id = id;
	usound->sink_chan.handle = usound->handle;
	usound->sink_chan.id = id2;

	chans[0] = &usound->source_chan;
	chans[1] = &usound->sink_chan;

	return bt_manager_audio_stream_enable(chans, 2, type);
}

static int mas_leaudio_start_source(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	usound->sink_chan.handle = usound->handle;
	usound->sink_chan.id = id;

	return bt_manager_audio_sink_stream_start_single(&usound->sink_chan);
}

static int mas_leaudio_stop_source(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	usound->sink_chan.handle = usound->handle;
	usound->sink_chan.id = id;

	return bt_manager_audio_sink_stream_stop_single(&usound->sink_chan);
}

static int mas_leaudio_disable_sink(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	usound->source_chan.handle = usound->handle;
	usound->source_chan.id = id;

	return bt_manager_audio_stream_disable_single(&usound->source_chan);
}

static int mas_leaudio_disable_source(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	usound->sink_chan.handle = usound->handle;
	usound->sink_chan.id = id;

	return bt_manager_audio_stream_disable_single(&usound->sink_chan);
}

static int mas_leaudio_release_sink(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	usound->source_chan.handle = usound->handle;
	usound->source_chan.id = id;

	return bt_manager_audio_stream_release_single(&usound->source_chan);
}

static int mas_leaudio_release_source(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	usound->sink_chan.handle = usound->handle;
	usound->sink_chan.id = id;

	return bt_manager_audio_stream_release_single(&usound->sink_chan);
}

static int mas_leaudio_update_sink(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	usound->source_chan.handle = usound->handle;
	usound->source_chan.id = id;

	return bt_manager_audio_stream_update_single(&usound->source_chan, BT_AUDIO_CONTEXT_MEDIA);
}

static int mas_leaudio_update_source(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint8_t id;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("id: %d", id);

	usound->sink_chan.handle = usound->handle;
	usound->sink_chan.id = id;

	return bt_manager_audio_stream_update_single(&usound->sink_chan, BT_AUDIO_CONTEXT_CONVERSATIONAL);
}

static int mas_leaudio_update_sink2(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint32_t type;
	uint8_t id;

	if (argc != 3) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	type = strtoul(argv[2], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, type: 0x%x", id, type);

	usound->source_chan.handle = usound->handle;
	usound->source_chan.id = id;

	return bt_manager_audio_stream_update_single(&usound->source_chan, type);
}

static int mas_leaudio_update_source2(const struct shell *shell, size_t argc, char *argv[])
{
	struct usound_app_t *usound = usound_get_app();
	uint32_t type;
	uint8_t id;

	if (argc != 3) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	id = strtoul(argv[1], (char **) NULL, 0);
	type = strtoul(argv[2], (char **) NULL, 0);
	SYS_LOG_INF("id: %d, type: 0x%x", id, type);

	usound->sink_chan.handle = usound->handle;
	usound->sink_chan.id = id;

	return bt_manager_audio_stream_update_single(&usound->sink_chan, type);
}

static int mas_set_volume(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	val = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("val: %d", val);

	return bt_manager_volume_client_set(val);
}

static int mas_volume_up(const struct shell *shell, size_t argc, char *argv[])
{
	SYS_LOG_INF("");

	return bt_manager_volume_client_up();
}

static int mas_volume_down(const struct shell *shell, size_t argc, char *argv[])
{
	SYS_LOG_INF("");

	return bt_manager_volume_client_down();
}


static int mas_volume_mute(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc != 1) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	SYS_LOG_INF("");

	return bt_manager_volume_client_mute();
}

static int mas_volume_unmute(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc != 1) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	SYS_LOG_INF("");

	return bt_manager_volume_client_unmute();
}

static int mas_mic_mute(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc != 1) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	SYS_LOG_INF("");

	return bt_manager_mic_client_mute();
}

static int mas_mic_unmute(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc != 1) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	SYS_LOG_INF("");

	return bt_manager_mic_client_unmute();
}

static int mas_mic_gain(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	val = strtoul(argv[1], (char **) NULL, 0);
	SYS_LOG_INF("val: %d", val);

	return bt_manager_mic_client_gain_set(val);
}

static int ccp_accept(const struct shell *shell, size_t argc, char *argv[])
{
    uint8_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

    val = strtoul(argv[1], (char **) NULL, 0);

	SYS_LOG_INF("");

    struct bt_audio_call call_t = {0};
    call_t.handle = usound_get_app()->handle;
    call_t.index = val;

	return bt_manager_call_accept(&call_t);
}

static int ccp_terminate(const struct shell *shell, size_t argc, char *argv[])
{
    uint8_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

    val = strtoul(argv[1], (char **) NULL, 0);

	SYS_LOG_INF("");

    struct bt_audio_call call_t = {0};
    call_t.handle = usound_get_app()->handle;
    call_t.index = val;

	return bt_manager_call_terminate(&call_t, 0);
}

static int ccp_hold(const struct shell *shell, size_t argc, char *argv[])
{
    uint8_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

    val = strtoul(argv[1], (char **) NULL, 0);

	SYS_LOG_INF("");

    struct bt_audio_call call_t = {0};
    call_t.handle = usound_get_app()->handle;
    call_t.index = val;

	return bt_manager_call_hold(&call_t);
}

static int ccp_retrieve(const struct shell *shell, size_t argc, char *argv[])
{
    uint8_t val;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

    val = strtoul(argv[1], (char **) NULL, 0);

	SYS_LOG_INF("");

    struct bt_audio_call call_t = {0};
    call_t.handle = usound_get_app()->handle;
    call_t.index = val;

	return bt_manager_call_retrieve(&call_t);
}

static int ccid_set(const struct shell *shell, size_t argc, char *argv[])
{
    uint8_t ccidcount;

	if (argc != 2) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

    ccidcount = strtoul(argv[1], (char **) NULL, 0);

    SYS_LOG_INF("");


    tr_bt_manager_audio_set_ccid(ccidcount);
    return 0;
}

extern int bt_hci_disconnect(uint16_t handle, uint8_t reason);
static int mas_acl_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc != 1) {
		SYS_LOG_ERR("argc: %d", argc);
		return -EINVAL;
	}

	SYS_LOG_INF("");

	return bt_hci_disconnect(usound_get_app()->handle, 0x13);
}

SHELL_STATIC_SUBCMD_SET_CREATE(mas_app_cmds,
	SHELL_CMD(csink, NULL, "config_codec sink", mas_leaudio_config_sink),
	SHELL_CMD(csinkr, NULL, "config_codec sink right", mas_leaudio_config_sink_right),
	SHELL_CMD(csrc, NULL, "config_codec source", mas_leaudio_config_source),

	SHELL_CMD(qsink, NULL, "config_qos sink", mas_leaudio_qos_sink),
	SHELL_CMD(qsink2, NULL, "config_qos sink2", mas_leaudio_qos_sink2),
	SHELL_CMD(qsrc, NULL, "config_qos source", mas_leaudio_qos_source),
	SHELL_CMD(qsrc2, NULL, "config_qos source2", mas_leaudio_qos_source2),
	SHELL_CMD(q, NULL, "config_qos", mas_leaudio_qos),
	SHELL_CMD(q2, NULL, "config_qos2", mas_leaudio_qos2),
	SHELL_CMD(q3, NULL, "config_qos3", mas_leaudio_qos3),

	SHELL_CMD(esink, NULL, "enable sink", mas_leaudio_enable_sink),
	SHELL_CMD(esrc, NULL, "enable source", mas_leaudio_enable_source),

	SHELL_CMD(esink2, NULL, "enable sink2", mas_leaudio_enable_sink2),
	SHELL_CMD(esrc2, NULL, "enable source2", mas_leaudio_enable_source2),
	SHELL_CMD(e2, NULL, "enable2", mas_leaudio_enable2),

	SHELL_CMD(ssrc, NULL, "start source", mas_leaudio_start_source),
	SHELL_CMD(sssrc, NULL, "stop source", mas_leaudio_stop_source),

	SHELL_CMD(dsink, NULL, "disable sink", mas_leaudio_disable_sink),
	SHELL_CMD(dsrc, NULL, "disable source", mas_leaudio_disable_source),

	SHELL_CMD(rsink, NULL, "release sink", mas_leaudio_release_sink),
	SHELL_CMD(rsrc, NULL, "release source", mas_leaudio_release_source),

	SHELL_CMD(usink, NULL, "update sink", mas_leaudio_update_sink),
	SHELL_CMD(usrc, NULL, "update source", mas_leaudio_update_source),

	SHELL_CMD(usink2, NULL, "update sink2", mas_leaudio_update_sink2),
	SHELL_CMD(usrc2, NULL, "update source2", mas_leaudio_update_source2),

	SHELL_CMD(vol, NULL, "set volume", mas_set_volume),
	SHELL_CMD(up, NULL, "volume_up", mas_volume_up),
	SHELL_CMD(down, NULL, "volume_down", mas_volume_down),
	SHELL_CMD(mute, NULL, "volume mute", mas_volume_mute),
	SHELL_CMD(unmute, NULL, "volume unmute", mas_volume_unmute),

	SHELL_CMD(micm, NULL, "mic mute", mas_mic_mute),
	SHELL_CMD(micu, NULL, "mic unmute", mas_mic_unmute),
	SHELL_CMD(gain, NULL, "mic gain", mas_mic_gain),

	SHELL_CMD(dis, NULL, "disconnect", mas_acl_disconnect),

	SHELL_CMD(ccpac, NULL, "ccp accept", ccp_accept),
	SHELL_CMD(ccpte, NULL, "ccp terminate", ccp_terminate),
	SHELL_CMD(ccpho, NULL, "ccp hold", ccp_hold),
	SHELL_CMD(ccpre, NULL, "ccp retrieve", ccp_retrieve),

    SHELL_CMD(ccidset, NULL, "ccid set", ccid_set),


	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(mas, &mas_app_cmds, "master leaudio commands", NULL);
#endif

