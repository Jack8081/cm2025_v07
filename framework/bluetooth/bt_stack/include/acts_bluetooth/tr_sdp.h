#ifndef __BT_TR_SDP_H
#define __BT_TR_SDP_H

struct profile_info {
    uint16_t uuid;
    uint16_t ver;
};

int tr_get_bt_profile_version(struct profile_info *info, uint8_t *num);

int tr_sdp_pnp_discover(struct bt_conn *conn);

#endif /* __BT_TR_SDP_H */
