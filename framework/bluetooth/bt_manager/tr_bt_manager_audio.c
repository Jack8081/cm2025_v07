#include <property_manager.h>
#include "acts_bluetooth/host_interface.h"
#include <audio_system.h>
#include "app_config.h"



#define payload_size(a) ((a) + (a)%2 + 4)

#define BINDING_BT_ADDR_L           "BINDING_BT_ADDR_L"
#define BINDING_BT_ADDR_R           "BINDING_BT_ADDR_R"

#define TR_LE_AUDIO_LINK_ADV		"LE_AUDIO_ADV"

#define L_INDEX 0
#define R_INDEX 1
#define N_INDEX 2

#define CONN_POLICY_INTERVAL 5

#define CONFIG_BT_MAX_LE_CONN CONFIG_BT_TRANS_SUPPORT_CIS_NUMBER

static uint8_t bt_le_master_count(void);
static struct bt_audio_conn *find_conn(uint16_t handle);
static struct bt_audio_channel *find_channel(uint16_t handle, uint8_t id);
static void tr_bt_manager_audio_scan_mode_internal(uint8_t mode, u32_t delay_time);
static bool tr_le_audio_update_link_list(bt_addr_le_t *addr);
static struct bt_audio_conn *find_active_master(uint8_t type);
static struct bt_audio_call_inst *find_call_by_status(struct bt_audio_conn *audio_conn,
				uint8_t state);
static struct bt_audio_call_inst *find_fisrt_call(struct bt_audio_conn *audio_conn);
static struct bt_audio_channel *find_channel2(struct bt_audio_conn *audio_conn,
				uint8_t id);

static struct tr_le_audio_link_list_t _link_dev[2];

#ifdef CONFIG_LE_AUDIO_SR_BQB
static uint8_t mcs_adv_flag = 0;
#endif

struct le_auto_conn_policy_t
{
    u8_t start : 1;
    u8_t stop: 1;
    u8_t connect_mode : 1;

    u8_t search_mode;
    u32_t auto_conn_times;
    struct k_delayed_work connect_delay_work;
};

#ifdef CONFIG_LE_AUDIO_BQB
u8_t _match_bqb_name[30]={0};
#endif

struct le_auto_conn_policy_t *policy_info;

#if CONFIG_BT_TRANS_SUPPORT_CIS_NUMBER > 1
static struct bt_audio_conn *tr_find_le_conn_by_addr(bt_addr_le_t *addr)
{
	struct bt_audio_conn *audio_conn;
    bt_addr_le_t *addr_targ;

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {
        addr_targ = btif_get_le_addr_by_handle(audio_conn->handle);
        if(addr_targ)
        {
            if (memcmp(addr_targ, addr, sizeof(bt_addr_le_t)) == 0) {
                os_sched_unlock();
                return audio_conn;
            }
        }
	}

	os_sched_unlock();

	return NULL;
}
#endif

static uint16_t tr_audio_bt_support_sampling_min(uint16_t value)
{
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 8000) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 8000)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_8K) {
		return 8;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 11025) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 11025)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_11K) {
		return 11;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 16000) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 16000)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_16K) {
		return 16;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 22050) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 22050)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_22K) {
		return 22;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 24000) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 24000)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_24K) {
		return 24;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 32000) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 32000)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_32K) {
		return 32;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 44100) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 44100)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_44K) {
		return 44;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 48000) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 48000)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_48K) {
		return 48;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 88200) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 88200)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_88K) {
		return 88;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 96000) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 96000)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_96K) {
		return 96;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 176400) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 176400)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_176K) {
		return 176;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 192000) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 192000)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_192K) {
		return 192;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_DOWN_SAMPLE_RATE == 384000) || (CONFIG_BT_LE_AUDIO_UP_SAMPLE_RATE == 384000)
	if (value & BT_SUPPORTED_SAMPLING_FREQUENCIES_384K) {
		return 384;
	}
#endif
	return 0;
}

/* Get frame_duration according to supported_frame_durations */
static uint8_t tr_audio_bt_frame_duration_from_supported(uint8_t value)
{
#if (CONFIG_BT_LE_AUDIO_US_PER_INTERVAL == 7500)
	if (value & BT_SUPPORTED_FRAME_DURATION_7_5MS_PREF) {
		return BT_FRAME_DURATION_7_5MS;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_US_PER_INTERVAL == 10000)
	if (value & BT_SUPPORTED_FRAME_DURATION_10MS_PREF) {
		return BT_FRAME_DURATION_10MS;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_US_PER_INTERVAL == 5000)
	if (value & BT_SUPPORTED_FRAME_DURATION_5MS_PREF) {
		return BT_FRAME_DURATION_5MS;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_US_PER_INTERVAL == 2500)
	if (value & BT_SUPPORTED_FRAME_DURATION_2_5MS_PREF) {
		return BT_FRAME_DURATION_2_5MS;
	}
#endif

#if (CONFIG_BT_LE_AUDIO_US_PER_INTERVAL == 7500)
	if (value & BT_SUPPORTED_FRAME_DURATION_7_5MS) {
		return BT_FRAME_DURATION_7_5MS;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_US_PER_INTERVAL == 10000)
	if (value & BT_SUPPORTED_FRAME_DURATION_10MS) {
		return BT_FRAME_DURATION_10MS;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_US_PER_INTERVAL == 5000)
	if (value & BT_SUPPORTED_FRAME_DURATION_5MS) {
		return BT_FRAME_DURATION_5MS;
	}
#endif
#if (CONFIG_BT_LE_AUDIO_US_PER_INTERVAL == 2500)
	if (value & BT_SUPPORTED_FRAME_DURATION_2_5MS) {
		return BT_FRAME_DURATION_2_5MS;
	}
#endif
	/* default */
	return BT_FRAME_DURATION_10MS;
}

static uint16_t tr_config_to_val_bitrate_call(uint8_t value)
{
    switch(value) {
        case LC3_BITRATE_CALL_1CH_80KBPS:
            return 90;
        case LC3_BITRATE_CALL_1CH_78KBPS:
            return 78;
        case LC3_BITRATE_CALL_1CH_64KBPS:
            return 64;
        case LC3_BITRATE_CALL_1CH_48KBPS:
            return 48;
        case LC3_BITRATE_CALL_1CH_32KBPS:
            return 32;
        default:
            return 32;
    }
    return 0;
}

#if CONFIG_BT_TRANS_SUPPORT_CIS_NUMBER > 1
static uint16_t tr_config_to_val_bitrate_1ch(uint8_t value) {
    switch(value) {
        case LC3_BITRATE_MUSIC_1CH_96KBPS:
            return 96;
        case LC3_BITRATE_MUSIC_1CH_80KBPS:
            return 80;
        case LC3_BITRATE_MUSIC_1CH_78KBPS:
            return 78;
        default:
            return 96;
    }
    return 0;
}
#else
static uint16_t tr_config_to_val_bitrate_2ch(uint8_t value) {
    switch(value) {
        case LC3_BITRATE_MUSIC_2CH_192KBPS:
        return 192;
        case LC3_BITRATE_MUSIC_2CH_184KBPS:
        return 184;
        case LC3_BITRATE_MUSIC_2CH_176KBPS:
        return 176;
        case LC3_BITRATE_MUSIC_2CH_160KBPS:
        return 160;
        case LC3_BITRATE_MUSIC_2CH_158KBPS:
        return 158;
        default:
        return 192;
    }
    return 0;
}
#endif

static uint16_t get_oct(uint16_t kbps, uint16_t interval) {

    const struct bt_audio_kbps *octets = bt_audio_kbps_table;

    for (uint8_t i = 0; i < ARRAY_SIZE(bt_audio_kbps_table); i++) {
        if ((octets->kbps == kbps) &&
                (octets->interval == interval)) {
            return octets->sdu;
        }
        octets++;
    }

    return 0;
}

static int tr_stream_discover_endp(void *data, int size)
{
	struct bt_audio_conn *audio_conn;
	struct bt_audio_endpoints *endps;
	struct bt_audio_endpoint *endp;
	struct bt_audio_capabilities *caps;
	struct bt_audio_capability *cap;
	struct bt_audio_codec *codec;
	uint8_t sink_id, source_id;
	uint8_t i, found;

    CFG_Struct_LE_AUDIO_Advertising_Mode cfg_le_audio;
    app_config_read(CFG_ID_LE_AUDIO_ADVERTISING_MODE, &cfg_le_audio, 0, sizeof(CFG_Struct_LE_AUDIO_Advertising_Mode));

    endps = (struct bt_audio_endpoints *)data;

#ifdef CONFIG_LE_AUDIO_BQB
    SYS_LOG_ERR("bqb test");
    return -EINVAL;
#endif

    audio_conn = find_conn(endps->handle);
    if (!audio_conn) {
        SYS_LOG_ERR("no conn");
        return -ENODEV;
    }

    caps = bt_manager_audio_client_get_cap(endps->handle);
    if (!caps) {
        SYS_LOG_ERR("no caps");
        return -EINVAL;
    }

    if (!caps->source_cap_num || !caps->sink_cap_num) {
        SYS_LOG_ERR("%d,%d\n", caps->source_cap_num, caps->sink_cap_num);
        return -EINVAL;
    }

    if (caps->sink_locations == BT_AUDIO_LOCATIONS_FR) {
        audio_conn->tws_pos = R_INDEX;
    } else {
        audio_conn->tws_pos = L_INDEX;
    }

    /* find 1st sink endpoint */
    for (i = 0, found = 0; i < endps->num_of_endp; i++) {
        endp = &endps->endp[i];
        if (endp->dir == BT_AUDIO_DIR_SINK) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        SYS_LOG_ERR("no sink endpoint");
        return -EINVAL;
    }

    source_id = endp->id;
    /* find 1st source endpoint */
    for (i = 0, found = 0; i < endps->num_of_endp; i++) {
        endp = &endps->endp[i];
        if (endp->dir == BT_AUDIO_DIR_SOURCE) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        SYS_LOG_ERR("no source endpoint");
        return -EINVAL;
    }

    sink_id = endp->id;

    codec = mem_malloc(sizeof(struct bt_audio_codec));

    cap = &caps->cap[0];
    codec->handle = endps->handle;
    codec->id = sink_id;
    codec->dir = BT_AUDIO_DIR_SOURCE;
    codec->format = cap->format;
    codec->target_latency = BT_AUDIO_TARGET_LATENCY_LOW;
    codec->target_phy = BT_AUDIO_TARGET_PHY_2M;
    codec->frame_duration = tr_audio_bt_frame_duration_from_supported(cap->durations);
    codec->blocks = 1;
    codec->sample_rate = tr_audio_bt_support_sampling_min(cap->samples);

    uint16_t octets_ch = get_oct(tr_config_to_val_bitrate_call(cfg_le_audio.LE_AUDIO_LC3_TX_Call_1CH_Bitrate), CONFIG_BT_LE_AUDIO_US_PER_INTERVAL);
    if(cap->octets_min <= octets_ch
            && cap->octets_max >= octets_ch) {
        codec->octets = octets_ch;
    } else {
        codec->octets = cap->octets_min;
    }

    codec->locations = caps->source_locations;
    SYS_LOG_INF("source ch:%d,fs:%d,fd:%x,sr:%d,os:%d(%d,%d)\n", cap->channels, cap->frames, codec->frame_duration, codec->sample_rate, codec->octets, cap->octets_min, cap->octets_max);
    bt_manager_audio_stream_config_codec(codec);

    cap = &caps->cap[caps->source_cap_num];
    codec->handle = endps->handle;
    codec->id = source_id;
    codec->dir = BT_AUDIO_DIR_SINK;
    codec->format = cap->format;
    codec->target_latency = BT_AUDIO_TARGET_LATENCY_LOW;
    codec->target_phy = BT_AUDIO_TARGET_PHY_2M;
    codec->frame_duration = tr_audio_bt_frame_duration_from_supported(cap->durations);
    codec->blocks = 1;
    codec->sample_rate = tr_audio_bt_support_sampling_min(cap->samples);

#if CONFIG_BT_TRANS_SUPPORT_CIS_NUMBER > 1
    octets_ch = get_oct(tr_config_to_val_bitrate_1ch(cfg_le_audio.LE_AUDIO_LC3_RX_Music_1CH_Bitrate), CONFIG_BT_LE_AUDIO_US_PER_INTERVAL);
#else
    octets_ch = get_oct(tr_config_to_val_bitrate_2ch(cfg_le_audio.LE_AUDIO_LC3_RX_Music_2CH_Bitrate), CONFIG_BT_LE_AUDIO_US_PER_INTERVAL);
#endif
    if(cap->octets_min <= octets_ch
            && cap->octets_max >= octets_ch) {
        codec->octets = octets_ch;
    } else {
        codec->octets = cap->octets_min;
    }

    codec->locations = caps->sink_locations;
    SYS_LOG_INF("sink ch:%d,fs:%d,fd:%x,sr:%d,os:%d(%d,%d)\n", cap->channels, cap->frames, codec->frame_duration, codec->sample_rate, codec->octets, cap->octets_min, cap->octets_max);
    SYS_LOG_INF("audio_conn sink_locations %d tws_pos %d\n", caps->sink_locations, audio_conn->tws_pos);
    bt_manager_audio_stream_config_codec(codec);

    mem_free(codec);
    return bt_manager_event_notify(BT_AUDIO_DISCOVER_ENDPOINTS, data, size);
}

static int tr_stream_prefer_qos(void *data, int size)
{
    struct bt_audio_preferred_qos *prefer;
    struct bt_audio_conn *audio_conn;
    struct bt_audio_channel *chan;
    struct bt_audio_qoss_1 *qoss;
    struct bt_audio_qos *qos;

    prefer = (struct bt_audio_preferred_qos *)data;

    audio_conn = find_conn(prefer->handle);
    if (!audio_conn) {
        SYS_LOG_ERR("no conn");
        return -ENODEV;
    }

    chan = find_channel(prefer->handle, prefer->id);
    if (!chan) {
        SYS_LOG_ERR("no chan");
        return -ENODEV;
    }

    if(audio_conn->qoss)
    {
        qoss = audio_conn->qoss;
    }
    else
    {
        qoss = mem_malloc(sizeof(struct bt_audio_qoss_1));
        audio_conn->qoss = qoss;
    }

    qos = &qoss->qos[0];

    os_sched_lock();
    if (bt_audio_unframed_supported(prefer->framing)) {
        qoss->framing = BT_AUDIO_UNFRAMED;
    } else {
        qoss->framing = BT_AUDIO_FRAMED;
    }

    if (chan->dir == BT_AUDIO_DIR_SINK) {
        qoss->interval_m = chan->interval;
        qoss->latency_m = prefer->latency;
        qoss->delay_m = prefer->delay_min;
        qoss->processing_m = chan->interval + 250;
        qos->id_m_to_s = prefer->id;
        qos->max_sdu_m = chan->channels * chan->octets;
        qos->phy_m = prefer->phy;
        qos->rtn_m = prefer->rtn;
    } else if (chan->dir == BT_AUDIO_DIR_SOURCE) {
        qoss->interval_s = chan->interval;
        qoss->latency_s = prefer->latency;
        qoss->delay_s = prefer->delay_min;
        qoss->processing_s = chan->interval + 0;
        qos->id_s_to_m = prefer->id;
        qos->max_sdu_s = chan->channels * chan->octets;
        qos->phy_s = prefer->phy;
        qos->rtn_s = prefer->rtn;
    }
    os_sched_unlock();

    if (!qoss->interval_m || !qoss->interval_s) {
        return 0;
    }

    qoss->num_of_qos = 1;
    qoss->sca = BT_GAP_SCA_UNKNOWN;
    qoss->packing = BT_AUDIO_PACKING_SEQUENTIAL;
    qos->handle = prefer->handle;

//    bt_manager_audio_stream_config_qos((struct bt_audio_qoss *)qoss);
    bt_manager_audio_stream_bind_qos((struct bt_audio_qoss *)qoss, audio_conn->tws_pos);
    mem_free(qoss);
    audio_conn->qoss = NULL;

    return 0;
}

static int tr_stream_enable(void *data, int size)
{
	struct bt_audio_report *rep = (struct bt_audio_report *)data;
	struct bt_audio_conn *audio_conn;
	struct bt_audio_channel *chan;
#ifdef CONFIG_LE_AUDIO_BQB
    return 0;
#endif

	audio_conn = find_conn(rep->handle);
	if (!audio_conn) {
		SYS_LOG_ERR("no conn");
		return -ENODEV;
	}

	chan = find_channel2(audio_conn, rep->id);
	if (!chan) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}
#ifdef CONFIG_BT_LE_AUDIO
	if (audio_conn->type == BT_TYPE_LE)
	{
        if(chan->dir == BT_AUDIO_DIR_SOURCE ) {
            bt_manager_audio_sink_stream_start_single((struct bt_audio_chan *)data);
        }
	}
#endif
    return 0;
}

void tr_bt_manager_audio_stream_event(int event, void *data, int size)
{
	SYS_LOG_INF("%d", event);

	switch (event) {
	case BT_AUDIO_STREAM_PREFER_QOS:
        tr_stream_prefer_qos(data, size);
		break;
	case BT_AUDIO_STREAM_CONFIG_QOS:
		break;
	case BT_AUDIO_STREAM_ENABLE:
        tr_stream_enable(data, size);
		break;
	case BT_AUDIO_STREAM_UPDATE:
		break;
	case BT_AUDIO_STREAM_START:
		break;
	case BT_AUDIO_STREAM_STOP:
		break;
	case BT_AUDIO_STREAM_DISABLE:
        bt_manager_audio_sink_stream_stop_single((struct bt_audio_chan *)data);
		break;
	case BT_AUDIO_STREAM_RELEASE:
		break;
	case BT_AUDIO_DISCOVER_CAPABILITY:
		break;
	case BT_AUDIO_DISCOVER_ENDPOINTS:
		tr_stream_discover_endp(data, size);
		break;
	case BT_AUDIO_PARAMS:
		break;
	case BT_AUDIO_STREAM_FAKE:
		break;
	default:
		break;
	}
}

static void tr_bt_manager_audio_conn_event(int event, void *data, int size)
{
    //	uint16_t handle;
    //	struct bt_audio_conn *audio_conn;

    SYS_LOG_INF("%d", event);

    switch (event)
    {
        case BT_CONNECTED:
            tr_bt_manager_audio_scan_mode_internal(LE_SEARCH_MATCH, 100);
            break;
        case BT_DISCONNECTED:
        {
#ifdef CONFIG_LE_AUDIO_BQB
#else
            tr_bt_manager_audio_scan_mode_internal(LE_SEARCH_MATCH, 100);
#endif
            break;
        }
        case BT_TWS_CONNECTION_EVENT:
            break;
        case BT_AUDIO_CONNECTED:
            break;
        case BT_AUDIO_DISCONNECTED:
            break;
        default:
            break;
    }
}

static int tr_le_audio_link_bind_restore(void)
{
	int prio, ret = 0;

	prio = hostif_set_negative_prio();
    memset(_link_dev, 0, sizeof(_link_dev));
    if(property_get(BINDING_BT_ADDR_L, (char *)&_link_dev[L_INDEX].addr, sizeof(bt_addr_le_t)) >= 6)
    {
        if(memcmp(_link_dev[L_INDEX].addr.a.val, "\0\0\0\0\0\0" , 6))
        {
            _link_dev[L_INDEX].valid = 1;
            ret++;
        }
    }

    if(property_get(BINDING_BT_ADDR_R, (char *)&_link_dev[R_INDEX].addr, sizeof(bt_addr_le_t)) >= 6)
    {
        if(memcmp(_link_dev[R_INDEX].addr.a.val, "\0\0\0\0\0\0" , 6))
        {
            _link_dev[R_INDEX].valid = 1;
            ret++;
        }
    }

	hostif_revert_prio(prio);
	return ret;
}

static void tr_le_audio_link_bind_clean(int index)
{
    SYS_LOG_INF("\n");
	int prio = hostif_set_negative_prio();
    if(index == L_INDEX)
    {
        property_set(BINDING_BT_ADDR_L, "\0", 1);
        property_flush(BINDING_BT_ADDR_L);

        memset(&_link_dev[L_INDEX], 0, sizeof(_link_dev[L_INDEX]));
    }
    else if(index == R_INDEX)
    {
        property_set(BINDING_BT_ADDR_R, "\0", 1);
        property_flush(BINDING_BT_ADDR_R);

        memset(&_link_dev[R_INDEX], 0, sizeof(_link_dev[R_INDEX]));
    }
    else
    {
        property_set(BINDING_BT_ADDR_L, "\0", 1);
        property_flush(BINDING_BT_ADDR_L);
        property_set(BINDING_BT_ADDR_R, "\0", 1);
        property_flush(BINDING_BT_ADDR_R);

        memset(_link_dev, 0, sizeof(_link_dev));
    }
	hostif_revert_prio(prio);
}

static struct bt_audio_conn *tr_find_conn_by_addr(bd_address_t* addr)
{
	struct bt_audio_conn *conn = NULL;
	struct bt_audio_conn *audio_conn;

	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node) {

		SYS_LOG_INF("hdl 0x%x: prio %d",audio_conn->handle ,audio_conn->prio);
		if (!memcmp(&(audio_conn->addr), addr, sizeof(bd_address_t))){
			conn = audio_conn;
			break;
		}
	}

	os_sched_unlock();

	return conn;
}

#ifdef CONFIG_LE_AUDIO_BQB
void set_bqb_full_name(u8_t *name)
{
    if(name)
    {
        u8_t len = strlen(name);
        len = len > 20 ? 20: len;
        memcpy(_match_bqb_name, name, len);
        _match_bqb_name[len] = 0;
    }
}

#endif

static bool _match_cb(const uint8_t *local_name, const uint8_t *name, uint8_t len, bt_addr_le_t *addr, int8_t rssi)
{
    struct tr_le_audio_link_list_t *link_dev;

    SYS_LOG_INF("%d, local: %s, name: %s\n", rssi, local_name, name ? name : NULL);
    if(policy_info == NULL)
    {
        return false;
    }

    if(policy_info->stop)
    {
        return false;
    }

    SYS_LOG_INF("add: 0x%02x:0x%02x:0x%02x\n", addr->a.val[2], addr->a.val[1], addr->a.val[0]);
    if(tr_find_conn_by_addr((bd_address_t*)&addr->a)) {
        return false;
    }

    if(policy_info->connect_mode)
    {
        if(name) //no Direct
        {
            return false;
        }

        tr_le_audio_link_bind_restore();
        link_dev = _link_dev;
        for(int i = 0; i < 2; i++)
        {
            if (!memcmp((void *)addr, (void *)&link_dev[i].addr, sizeof(bt_addr_le_t)))
            {
                SYS_LOG_INF("%d\n", rssi);
                return true;
            }
        }
        return false;
    }

    if(rssi < CONFIG_BT_AUTO_CONNECT_RSSI_THRESHOLD)
    {
        return false;
    }

    if(name == NULL) //Direct
    {
        return true;
    }
#ifdef CONFIG_BT_TRANS_BLE_APK_TEST_MODE
    if (!strncmp(local_name, name, len))
    {
        return true;
    }
    return false;
#endif

#ifdef CONFIG_LE_AUDIO_BQB
    if (!strncmp(_match_bqb_name, name, strlen(_match_bqb_name)))
    {
        return true;
    }
#else
    u8_t _match_name[30];
    if(property_get(CFG_BLE_AUDIO, _match_name, 30) > 0)
    {
        if (!strncmp(_match_name, name, len))
        {
            return true;
        }
    }
    else if (!strncmp(local_name, name, len))
    {
        return true;
    }
#endif
    return false;
}

static int le_audio_conn_policy_start(u8_t search)
{
    if(!policy_info)
    {
        return -1;
    }

    hostif_bt_le_scan_stop();
    if(bt_le_master_count() >= CONFIG_BT_MAX_LE_CONN)
    {
        policy_info->search_mode = LE_SEARCH_MATCH;
        SYS_LOG_INF("max dev\n");
        return -2;
    }

    if(search == LE_SEARCH_MATCH)
        policy_info->connect_mode = 1;
    else
        policy_info->connect_mode = 0;

    SYS_LOG_INF("search mode: %s\n", policy_info->connect_mode ? "match" : "search");
#ifdef CONFIG_TR_BT_HOST_BQB
        return 0;
#endif

    tr_btif_audio_start(_match_cb);

    return 0;
}

static void le_audio_conn_policy_timeout(struct k_work *work)
{
    if(policy_info->stop)
    {
        return;
    }

    if(le_audio_conn_policy_start(policy_info->search_mode))
    {
        return;
    }

    k_delayed_work_submit(&policy_info->connect_delay_work, K_MSEC(CONN_POLICY_INTERVAL * 1000));
}

static void le_audio_conn_policy_stop(void)
{
    if(!policy_info)
    {
        return;
    }

    k_delayed_work_cancel(&policy_info->connect_delay_work);
    hostif_bt_le_scan_stop();
}

static int le_audio_conn_policy_try_start(u32_t delay_time)
{
    if(!policy_info)
    {
        SYS_LOG_ERR("unready\n");
        return -1;
    }

    if(policy_info && policy_info->stop)
    {
        SYS_LOG_ERR("force stop\n\n");
        return -1;
    }

    if(policy_info->stop)
    {
        return -1;
    }

    return k_delayed_work_submit(&policy_info->connect_delay_work, K_MSEC(delay_time));
}

static void tr_bt_manager_audio_scan_mode_internal(uint8_t mode, u32_t delay_time)
{
    if(!policy_info)
    {
        SYS_LOG_ERR("unready\n");
        return;
    }

    if(policy_info->search_mode != LE_SEARCH_ALL)
    {
        policy_info->search_mode = mode;
    }
    SYS_LOG_INF("search_mode: %d\n", policy_info->search_mode);
    le_audio_conn_policy_try_start(delay_time);
}

/*
 *
#define LE_SEARCH_MATCH 0
#define LE_SEARCH_L 1
#define LE_SEARCH_R 2
#define LE_SEARCH_ALL 3
 *
 * */
static bool tr_le_audio_update_link_list(bt_addr_le_t *addr)
{
    if(addr == NULL) {
        return false;
    }

    SYS_LOG_INF("t %d MAC %02x:%02x:%02x:%02x:%02x:%02x", addr->type, addr->a.val[5], addr->a.val[4], addr->a.val[3], 
            addr->a.val[2], addr->a.val[1], addr->a.val[0]);

#if CONFIG_BT_TRANS_SUPPORT_CIS_NUMBER > 1
    int i;
    for(i = 0; i < 2; i++)
    {
        if(!memcmp(&_link_dev[i].addr, addr, sizeof(bt_addr_le_t)))
            break;
    }
    
    if(i < 2)
    {
        _link_dev[i].valid = 1;
        return true;
    }

    SYS_LOG_INF("%d\n", policy_info->search_mode);

	int prio = hostif_set_negative_prio();
    switch(policy_info->search_mode)
    {
        case LE_SEARCH_L:
        {
            memcpy(&_link_dev[L_INDEX].addr, addr, sizeof(bt_addr_le_t));
            _link_dev[L_INDEX].valid = 1;
            property_set(BINDING_BT_ADDR_L, (char *)addr, sizeof(bt_addr_le_t));
            property_flush(BINDING_BT_ADDR_L);
        }
        break;
        case LE_SEARCH_R:
        {
            memcpy(&_link_dev[R_INDEX].addr, addr, sizeof(bt_addr_le_t));
            _link_dev[R_INDEX].valid = 1;
            property_set(BINDING_BT_ADDR_R, (char *)addr, sizeof(bt_addr_le_t));
            property_flush(BINDING_BT_ADDR_R);
        }
        break;
        case LE_SEARCH_ONE:
        case LE_SEARCH_ALL:
        {
            if(bt_audio.num == 1) {
                memcpy(&_link_dev[L_INDEX].addr, addr, sizeof(bt_addr_le_t));
                _link_dev[L_INDEX].valid = 1;
                property_set(BINDING_BT_ADDR_L, (char *)addr, sizeof(bt_addr_le_t));
                property_flush(BINDING_BT_ADDR_L);
            } else {
                SYS_LOG_INF("%d, %d\n", _link_dev[R_INDEX].valid, _link_dev[L_INDEX].valid);
                if(_link_dev[L_INDEX].valid && tr_find_le_conn_by_addr(&_link_dev[L_INDEX].addr))
                {
                    memcpy(&_link_dev[R_INDEX].addr, addr, sizeof(bt_addr_le_t));
                    _link_dev[R_INDEX].valid = 1;
                    property_set(BINDING_BT_ADDR_R, (char *)addr, sizeof(bt_addr_le_t));
                    property_flush(BINDING_BT_ADDR_R);
                }
                else if(_link_dev[R_INDEX].valid && tr_find_le_conn_by_addr(&_link_dev[R_INDEX].addr))
                {
                    memcpy(&_link_dev[L_INDEX].addr, addr, sizeof(bt_addr_le_t));
                    _link_dev[L_INDEX].valid = 1;
                    property_set(BINDING_BT_ADDR_L, (char *)addr, sizeof(bt_addr_le_t));
                    property_flush(BINDING_BT_ADDR_L);
                }
                else
                {
                    SYS_LOG_ERR("no record\n");
                }
            }
        }
        break;
        default:
        break;
    }
	hostif_revert_prio(prio);
    return true;
#else
    if(!memcmp(&_link_dev[0].addr, addr , sizeof(bt_addr_le_t)))
    {
        return true;
    }
    else
    {
        memcpy(&_link_dev[L_INDEX].addr, addr, sizeof(bt_addr_le_t));
        _link_dev[L_INDEX].valid = 1;
        property_set(BINDING_BT_ADDR_L, (char *)addr, sizeof(bt_addr_le_t));
        property_flush(BINDING_BT_ADDR_L);
        return true;
    }
#endif
    return false;
}

/******************************
 *
 *user api
 *
void tr_bt_manager_audio_scan_mode(uint8_t mode)//api wrapper
void tr_bt_manager_audio_disconn(uint16_t handle)
int tr_bt_manager_audio_auto_start(void)
int tr_bt_manager_audio_auto_stop(void)
void *tr_le_audio_get_link_info(void)
 * ***********************************/

void tr_bt_manager_audio_disconn(uint16_t handle)
{
    if(!handle)
        return;

    if(policy_info)
        policy_info->stop = 1;

    tr_btif_audio_disconn(handle);
}

int tr_bt_manager_audio_auto_start(void)
{
    if(policy_info)
    {
        return -1;
    }

    policy_info = mem_malloc(sizeof(struct le_auto_conn_policy_t));
    if(!policy_info)
    {
        SYS_LOG_ERR("\n");
        return -1;
    }
    memset(policy_info, 0, sizeof(struct le_auto_conn_policy_t));

    k_delayed_work_init(&policy_info->connect_delay_work, le_audio_conn_policy_timeout);
    if(tr_le_audio_link_bind_restore())
        tr_bt_manager_audio_scan_mode_internal(LE_SEARCH_MATCH, 100);
    return 0;
}

int tr_bt_manager_audio_auto_stop(void)
{
	SYS_LOG_INF("");
    if(!policy_info)
    {
        return -1;
    }

    policy_info->stop = 1;
    le_audio_conn_policy_stop();

	btif_audio_stop();

		/* wait forever */
    while (!btif_audio_stopped()) {
        os_sleep(10);
    }

    mem_free(policy_info);
    policy_info = NULL;

	SYS_LOG_INF("done");
    return 0;
}

void *tr_le_audio_get_link_info(void)
{
    tr_le_audio_link_bind_restore();
    return _link_dev;
}

void tr_bt_manager_audio_scan_mode(uint8_t mode)
{
    if(!policy_info)
    {
        SYS_LOG_INF("policy_info not init\n");
        return;
    }
    
    switch(mode)
    {
        case LE_DEL_ALL:
        {
            policy_info->stop = 1;
            bt_manager_le_audio_stop(1);
            tr_le_audio_link_bind_clean(N_INDEX);
            return;
        }
        break;
        case LE_DEL_L:
        {
            policy_info->stop = 1;
            bt_manager_le_audio_stop(1);
            tr_le_audio_link_bind_clean(L_INDEX);
            return;
        }
        break;
        case LE_DEL_R:
        {
            policy_info->stop = 1;
            bt_manager_le_audio_stop(1);
            tr_le_audio_link_bind_clean(R_INDEX);
            return;
        }
        case LE_SEARCH_STOP:
        {
            policy_info->stop = 1;
            tr_bt_manager_audio_auto_stop();
            return;
        }
        break;
        case LE_SEARCH_MATCH:
        case LE_SEARCH_L:
        case LE_SEARCH_R:
        case LE_SEARCH_ALL:
        case LE_SEARCH_ONE:
            policy_info->stop = 0;
        break;
        default:
        SYS_LOG_ERR("parma erro\n");
        return;
    }

    if(policy_info->search_mode != LE_SEARCH_ALL)
    {
        policy_info->search_mode = mode;
    }
    SYS_LOG_INF("search_mode: %d\n", policy_info->search_mode);
    le_audio_conn_policy_try_start(0);
}

int tr_bt_manager_call_originate(uint16_t handle, uint8_t *remote_uri)
{
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_conn *audio_conn;

    if (handle == 0) {
        audio_conn = find_active_master(BT_TYPE_LE);
    } else {
        audio_conn = find_conn(handle);
    }

    if (!audio_conn) {
        return -ENODEV;
    }

    if (audio_conn->type == BT_TYPE_LE) {
        return bt_manager_le_call_originate(audio_conn->handle, remote_uri);
    }
#endif
    return -EINVAL;
}

int tr_bt_manager_call_accept(struct bt_audio_call *call)
{
    struct bt_audio_conn *audio_conn;
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_call_inst *call_inst;
    struct bt_audio_call tmp;
#endif

    /* default: select current active call */
    if (!call) {
        audio_conn = find_active_master(BT_TYPE_LE);
        if (!audio_conn) {
            return -ENODEV;
        }

        if (audio_conn->type == BT_TYPE_BR) {
            return bt_manager_hfp_accept_call();
        } else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
            call_inst = find_call_by_status(audio_conn, BT_CALL_STATE_INCOMING);
            if (!call_inst) {
                return -ENODEV;
            }

            tmp.handle = call_inst->handle;
            tmp.index = call_inst->index;
            return bt_manager_le_call_accept(&tmp);
#endif
        }

        return -EINVAL;
    }

    /* select by upper layer */

    audio_conn = find_conn(call->handle);
    if (!audio_conn) {
        return -EINVAL;
    }

    if (audio_conn->type == BT_TYPE_BR) {
        return bt_manager_hfp_accept_call();
    } else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
        return bt_manager_le_call_accept(call);
#endif
    }

    return -EINVAL;
}

int tr_bt_manager_call_hold(struct bt_audio_call *call)
{
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_conn *audio_conn;
    struct bt_audio_call_inst *call_inst;
    struct bt_audio_call tmp;

    /* default: select current active call */
    if (!call) {
        audio_conn = find_active_master(BT_TYPE_LE);
        if (!audio_conn) {
            return -ENODEV;
        }

        if (audio_conn->type == BT_TYPE_BR) {
            return -EINVAL;
        } else if (audio_conn->type == BT_TYPE_LE) {
            /* FIXME: find a active call? */
            call_inst = find_call_by_status(audio_conn, BT_CALL_STATE_ACTIVE);
            if (!call_inst) {
                return -ENODEV;
            }

            tmp.handle = call_inst->handle;
            tmp.index = call_inst->index;
            return bt_manager_le_call_hold(&tmp);
        }

        return -EINVAL;
    }

    /* select by upper layer */

    audio_conn = find_conn(call->handle);
    if (!audio_conn) {
        return -EINVAL;
    }

    if (audio_conn->type == BT_TYPE_BR) {
        return -EINVAL;
    } else if (audio_conn->type == BT_TYPE_LE) {
        return bt_manager_le_call_hold(call);
    }
#endif

    return -EINVAL;
}

int tr_bt_manager_call_retrieve(struct bt_audio_call *call)
{
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_conn *audio_conn;
    struct bt_audio_call_inst *call_inst;
    struct bt_audio_call tmp;

    /* default: select current active call */
    if (!call) {
        audio_conn = find_active_master(BT_TYPE_LE);
        if (!audio_conn) {
            return -ENODEV;
        }

        if (audio_conn->type == BT_TYPE_BR) {
            return -EINVAL;
        } else if (audio_conn->type == BT_TYPE_LE) {
            /* FIXME: find a locally held call? */
            call_inst = find_call_by_status(audio_conn, BT_CALL_STATE_LOCALLY_HELD);
            if (!call_inst) {
                call_inst = find_call_by_status(audio_conn, BT_CALL_STATE_HELD);
            }

            if (!call_inst) {
                return -ENODEV;
            }

            tmp.handle = call_inst->handle;
            tmp.index = call_inst->index;
            return bt_manager_le_call_retrieve(&tmp);
        }

        return -EINVAL;
    }

    /* select by upper layer */
    audio_conn = find_conn(call->handle);
    if (!audio_conn) {
        return -EINVAL;
    }

    if (audio_conn->type == BT_TYPE_BR) {
        return -EINVAL;
    } else if (audio_conn->type == BT_TYPE_LE) {
        return bt_manager_le_call_retrieve(call);
    }
#endif

    return -EINVAL;
}

int tr_bt_manager_call_terminate(struct bt_audio_call *call, uint8_t reason)
{
    struct bt_audio_conn *audio_conn;
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_call_inst *call_inst;
    struct bt_audio_call tmp;
#endif

    /* default: select current active call */
    if (!call) {
        audio_conn = find_active_master(BT_TYPE_LE);
        if (!audio_conn) {
            return -ENODEV;
        }

        if (audio_conn->type == BT_TYPE_BR) {
            return -EINVAL;
        } else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
            call_inst = find_call_by_status(audio_conn, BT_CALL_STATE_ACTIVE);
            if (!call_inst) {
                call_inst = find_fisrt_call(audio_conn);
            }

            if (!call_inst) {
                return -ENODEV;
            }

            tmp.handle = call_inst->handle;
            tmp.index = call_inst->index;
            return tr_btif_audio_call_terminate(&tmp, reason);
#endif
        }

        return -EINVAL;
    }

    /* select by upper layer */
    audio_conn = find_conn(call->handle);
    if (!audio_conn) {
        return -EINVAL;
    }

    if (audio_conn->type == BT_TYPE_BR) {
        return -EINVAL;
    } else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
        return tr_btif_audio_call_terminate(call, reason);
#endif
    }

    return -EINVAL;
}

int tr_bt_manager_call_remote_originate(uint16_t handle, uint8_t *remote_uri)
{
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_conn *audio_conn;

    if (handle == 0) {
        audio_conn = find_active_master(BT_TYPE_LE);
    } else {
        audio_conn = find_conn(handle);
    }

    if (!audio_conn) {
        return -ENODEV;
    }

    if (audio_conn->type == BT_TYPE_LE) {
        return tr_btif_audio_call_remote_originate(audio_conn->handle, remote_uri);
    }
#endif
    return -EINVAL;
}

#ifdef CONFIG_BT_LE_AUDIO
static struct bt_audio_call *tr_bt_manager_get_audio_call(struct bt_audio_call *call, struct bt_audio_call *out, uint8_t status)
{
    struct bt_audio_conn *audio_conn;
    struct bt_audio_call_inst *call_inst;

    if (!call)
    {
        if(!out)
        {
            return NULL;
        }

        audio_conn = find_active_master(BT_TYPE_LE);
        if (!audio_conn)
        {
            return NULL;
        }

        if (audio_conn->type == BT_TYPE_BR)
        {
            return NULL;
        }
        else if (audio_conn->type == BT_TYPE_LE)
        {
            call_inst = find_call_by_status(audio_conn, status);
            if (!call_inst)
            {
                return NULL;
            }

            out->handle = call_inst->handle;
            out->index = call_inst->index;
            return out;
        }
        return NULL;
    }

    audio_conn = find_conn(call->handle);
    if (!audio_conn)
    {
        return NULL;
    }

    if (audio_conn->type == BT_TYPE_BR)
    {
        return NULL;
    }
    else if (audio_conn->type == BT_TYPE_LE)
    {
        return call;
    }
    return NULL;
}
#endif

int tr_bt_manager_call_remote_alert(struct bt_audio_call *call)
{
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_call tmp;
    call = tr_bt_manager_get_audio_call(call, &tmp, BT_CALL_STATE_DIALING);
    if(call)
        return tr_btif_audio_call_remote_alert(call);
#endif
    return -EINVAL;
}

int tr_bt_manager_call_remote_accept(struct bt_audio_call *call)
{
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_call tmp;
    call = tr_bt_manager_get_audio_call(call, &tmp, BT_CALL_STATE_ALERTING);
    if(call)
        return tr_btif_audio_call_remote_accept(call);
#endif
    return -EINVAL;
}

int tr_bt_manager_call_remote_hold(struct bt_audio_call *call)
{
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_call tmp;
    call = tr_bt_manager_get_audio_call(call, &tmp, BT_CALL_STATE_ACTIVE);
    if(call)
        return tr_btif_audio_call_remote_hold(call);
#endif
    return -EINVAL;
}

int tr_bt_manager_call_remote_retrieve(struct bt_audio_call *call)
{
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_call tmp;
    call = tr_bt_manager_get_audio_call(call, &tmp, BT_CALL_STATE_REMOTELY_HELD);
    if(call)
        return tr_btif_audio_call_remote_retrieve(call);
#endif
    return -EINVAL;
}

int tr_bt_manager_call_remote_terminate(struct bt_audio_call *call)
{
    struct bt_audio_conn *audio_conn;
#ifdef CONFIG_BT_LE_AUDIO
    struct bt_audio_call_inst *call_inst;
    struct bt_audio_call tmp;
#endif

    /* default: select current active call */
    if (!call) {
        audio_conn = find_active_master(BT_TYPE_LE);
        if (!audio_conn) {
            return -ENODEV;
        }

        if (audio_conn->type == BT_TYPE_BR) {
            return -EINVAL;
        } else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
            call_inst = find_call_by_status(audio_conn, BT_CALL_STATE_ACTIVE);
            if (!call_inst) {
                call_inst = find_fisrt_call(audio_conn);
            }

            if (!call_inst) {
                return -ENODEV;
            }

            tmp.handle = call_inst->handle;
            tmp.index = call_inst->index;
            return tr_btif_audio_call_remote_terminate(&tmp);
#endif
        }

        return -EINVAL;
    }

    /* select by upper layer */
    audio_conn = find_conn(call->handle);
    if (!audio_conn) {
        return -EINVAL;
    }

    if (audio_conn->type == BT_TYPE_BR) {
        return -EINVAL;
    } else if (audio_conn->type == BT_TYPE_LE) {
#ifdef CONFIG_BT_LE_AUDIO
        return tr_btif_audio_call_remote_terminate(call);
#endif
    }
    return -EINVAL;
}

u8_t tr_bt_manager_le_audio_get_cis_num(void)
{
    struct bt_audio_conn *audio_conn;
    u8_t count = 0;
    os_sched_lock();

    SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node)
    {
        if(audio_conn->audio_connected && audio_conn->type == BT_TYPE_LE)
        {
            count++;
        }
    }

    os_sched_unlock();
    return count;
}

static struct bt_audio_conn *tr_find_le_conn_by_index(int index)
{
	struct bt_audio_conn *audio_conn;
    int count = 0;
	os_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_audio.conn_list, audio_conn, node)
    {
        if(count == index)
        {
            os_sched_unlock();
            return audio_conn;
        }
        count++;
	}

	os_sched_unlock();

	return NULL;
}

int API_APP_CONNECTION_CONFIG(int index, struct conn_status *status)
{
    if (!status)
    {
        return -EINVAL;
    }

    memset(status, 0, sizeof(struct conn_status));

    if(index >= CONFIG_BT_MAX_LE_CONN)
    {
        return -EINVAL;
    }

	struct bt_audio_conn *audio_conn = tr_find_le_conn_by_index(index);
    if(audio_conn)
    {
        status->idx = index;
        status->connected = 1;
        status->audio_connected = audio_conn->audio_connected;
        status->handle = audio_conn->handle;

        memcpy(status->addr, audio_conn->addr.val, 6);
//        status->rssi = tr_le_audio_info.le_audio_dev_info[index]->rssi;
//        memcpy(status->name, tr_le_audio_info.le_audio_dev_info[index]->name, 32);
    }

    return 0;
}

#if CONFIG_BT_TRANS_SUPPORT_CIS_NUMBER > 1
struct bt_audio_qoss_2 _qoss_2;
#else
struct bt_audio_qoss_1 _qoss_1;
#endif
//static int tr_stream_prefer_qos(void *data, int size)
//static int tr_stream_discover_endp(void *data, int size)
static int tr_bt_le_audio_bandwidth_init(void)
{
    CFG_Struct_LE_AUDIO_Advertising_Mode cfg_le_audio;
    app_config_read(CFG_ID_LE_AUDIO_ADVERTISING_MODE, &cfg_le_audio, 0, sizeof(CFG_Struct_LE_AUDIO_Advertising_Mode));

#if CONFIG_BT_TRANS_SUPPORT_CIS_NUMBER > 1
    struct bt_audio_qoss_2 *qoss = &_qoss_2;
    struct bt_audio_qos *qos;

    qoss->num_of_qos = 2;
    qoss->sca = BT_GAP_SCA_UNKNOWN;
    qoss->packing = BT_AUDIO_PACKING_SEQUENTIAL;
    qoss->framing = BT_AUDIO_UNFRAMED;

    qoss->interval_m = CONFIG_BT_LE_AUDIO_US_PER_INTERVAL;
    qoss->interval_s = CONFIG_BT_LE_AUDIO_US_PER_INTERVAL;
    if(CONFIG_BT_LE_AUDIO_US_PER_INTERVAL == 7500) {
        qoss->latency_m = 8;
        qoss->latency_s = 8;
    } else {
        qoss->latency_m = 10;
        qoss->latency_s = 10;
    }

    /* device 1: 48k*16bit*2ch download + 16k*16bit*1ch upload */
    qos = &qoss->qos[0];

    qos->max_sdu_m = get_oct(tr_config_to_val_bitrate_1ch(cfg_le_audio.LE_AUDIO_LC3_RX_Music_1CH_Bitrate), CONFIG_BT_LE_AUDIO_US_PER_INTERVAL);
    qos->max_sdu_s = get_oct(tr_config_to_val_bitrate_call(cfg_le_audio.LE_AUDIO_LC3_TX_Call_1CH_Bitrate), CONFIG_BT_LE_AUDIO_US_PER_INTERVAL);
    qos->phy_m = BT_AUDIO_PHY_2M;
    qos->phy_s = BT_AUDIO_PHY_2M;
    qos->rtn_m = 2;
    qos->rtn_s = 2;

    /* device 2: 48k*16bit*2ch download + 16k*16bit*1ch upload */
    qos = &qoss->qos[1];
    qos->max_sdu_m = get_oct(tr_config_to_val_bitrate_1ch(cfg_le_audio.LE_AUDIO_LC3_RX_Music_1CH_Bitrate), CONFIG_BT_LE_AUDIO_US_PER_INTERVAL);
    qos->max_sdu_s = get_oct(tr_config_to_val_bitrate_call(cfg_le_audio.LE_AUDIO_LC3_TX_Call_1CH_Bitrate), CONFIG_BT_LE_AUDIO_US_PER_INTERVAL);
    qos->phy_m = BT_AUDIO_PHY_2M;
    qos->phy_s = BT_AUDIO_PHY_2M;
    qos->rtn_m = 2;
    qos->rtn_s = 2;

#ifdef CONFIG_LE_AUDIO_BQB
    //bt_manager_audio_stream_bandwidth_alloc((struct bt_audio_qoss *)qoss);
#else
    bt_manager_audio_stream_bandwidth_alloc((struct bt_audio_qoss *)qoss);
#endif
#else
    struct bt_audio_qoss_1 *qoss = &_qoss_1;
    struct bt_audio_qos *qos;

    qoss->num_of_qos = 1;
    qoss->sca = BT_GAP_SCA_UNKNOWN;
    qoss->packing = BT_AUDIO_PACKING_SEQUENTIAL;
    qoss->framing = BT_AUDIO_UNFRAMED;

    qoss->interval_m = CONFIG_BT_LE_AUDIO_US_PER_INTERVAL;
    qoss->interval_s = CONFIG_BT_LE_AUDIO_US_PER_INTERVAL;
    if(CONFIG_BT_LE_AUDIO_US_PER_INTERVAL == 7500) {
        qoss->latency_m = 8;
        qoss->latency_s = 8;
    } else {
        qoss->latency_m = 10;
        qoss->latency_s = 10;
    }

    /* device 1: 48k*16bit*2ch download + 16k*16bit*1ch upload */
    qos = &qoss->qos[0];
    qos->max_sdu_m = get_oct(tr_config_to_val_bitrate_2ch(cfg_le_audio.LE_AUDIO_LC3_RX_Music_2CH_Bitrate), CONFIG_BT_LE_AUDIO_US_PER_INTERVAL);
    qos->max_sdu_s = get_oct(tr_config_to_val_bitrate_call(cfg_le_audio.LE_AUDIO_LC3_TX_Call_1CH_Bitrate), CONFIG_BT_LE_AUDIO_US_PER_INTERVAL);
    qos->phy_m = BT_AUDIO_PHY_2M;
    qos->phy_s = BT_AUDIO_PHY_2M;
    qos->rtn_m = 4;
    qos->rtn_s = 4;

#ifdef CONFIG_LE_AUDIO_BQB
    //bt_manager_audio_stream_bandwidth_alloc((struct bt_audio_qoss *)qoss);
#else
    bt_manager_audio_stream_bandwidth_alloc((struct bt_audio_qoss *)qoss);
#endif
#endif
    printk("max_sdu_m %d, max_sdu_s %d\n", qos->max_sdu_m, qos->max_sdu_s);
    return 0;
}

int tr_bt_manager_audio_stream_enable_single(struct bt_audio_chan *chan,
				uint32_t contexts)
{
    int timeout_cnt = 0;

	struct bt_audio_channel *ch = find_channel(chan->handle, chan->id);
	if (!ch) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	if(ch->state != BT_AUDIO_STREAM_QOS_CONFIGURED && ch->state != BT_AUDIO_STREAM_DISABLED) {
		SYS_LOG_ERR("no state %d\n", ch->state);
		return -EACCES;
    }

	bt_manager_audio_stream_enable(&chan, 1, contexts);

    do {
        if(timeout_cnt++ > 20) {
            SYS_LOG_ERR("timeout %d\n", ch->state);
            return -ETIMEDOUT;
        }
        ch = find_channel(chan->handle, chan->id);
        if (!ch) {
            SYS_LOG_ERR("no chan");
            return -ENODEV;
        }
        k_sleep(K_MSEC(100));
    } while(ch->state != BT_AUDIO_STREAM_STARTED);

    return 0;
}

int tr_bt_manager_audio_stream_disable_single(struct bt_audio_chan *chan)
{
    int timeout_cnt = 0;

	struct bt_audio_channel *ch = find_channel(chan->handle, chan->id);
	if (!ch) {
		SYS_LOG_ERR("no chan");
		return -ENODEV;
	}

	if(ch->state != BT_AUDIO_STREAM_ENABLED && ch->state != BT_AUDIO_STREAM_STARTED) {
		SYS_LOG_ERR("no state %d\n", ch->state);
		return -EACCES;
    }

	bt_manager_audio_stream_disable(&chan, 1);

    do {
        if(timeout_cnt++ > 20) {
            SYS_LOG_ERR("timeout %d\n", ch->state);
            return -ETIMEDOUT;
        }
        ch = find_channel(chan->handle, chan->id);
        if (!ch) {
            SYS_LOG_ERR("no chan");
            return -ENODEV;
        }

        k_sleep(K_MSEC(100));
    } while(ch->state != BT_AUDIO_STREAM_QOS_CONFIGURED);

    return 0;
}

#ifdef CONFIG_LE_AUDIO_BQB
void tr_bt_manager_audio_set_ccid(uint8_t ccid_count)
{
    tr_btif_set_ccid(ccid_count);
}
#endif

#ifdef CONFIG_LE_AUDIO_SR_BQB
int tr_bt_manager_contexts_set(bool available, bool supported, int val)
{
    return tr_btif_contexts_set(available, supported, val);
}

int tr_bt_manager_server_provider_set(uint8_t *provider, uint8_t len)
{
    return tr_btif_server_provider_set(provider, len);
}

int tr_bt_manager_server_tech_set(uint8_t tech)
{
    return tr_btif_server_tech_set(tech);
}

int tr_bt_manager_server_status_set(uint16_t status)
{
    return tr_btif_server_status_set(status);
}

int tr_bt_manager_volume_init_set(uint8_t step)
{
    return tr_btif_volume_init_set(step);
}

void tr_bt_manager_csis_not_bonded_set(uint8_t enable)
{
    tr_btif_audio_csis_not_bonded_set(enable);
}

void tr_bt_manager_csis_update_lockstate(uint8_t lockval)
{
    tr_btif_audio_csis_update_lockstate(lockval);
}

void tr_bt_manager_mics_mute(uint16_t handle, uint8_t val)
{
    if(val == 1)
        bt_manager_le_mic_mute(handle);
    else if(val == 2)
        bt_manager_le_mic_mute_disable(handle);

}

int tr_bt_manager_audio_mcs_adv_set()
{
    struct bt_le_adv_param param = {0};

    param.id = BT_ID_DEFAULT;
    param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
    param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
    param.options = BT_LE_ADV_OPT_CONNECTABLE;

    //param.options |= BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_NO_2M;
    tr_btif_mcs_adv_buf_set();
    mcs_adv_flag = 1;

    return bt_manager_audio_le_adv_param_init(&param);
}

#endif

