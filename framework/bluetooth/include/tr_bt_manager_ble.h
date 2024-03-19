
void tr_bt_manager_set_ble_power_mode(bool lowpower);
void tr_bt_manager_ble_cli_init(void);
void bt_manager_updata_parma_work_pass(bool passed);
void tr_bt_manager_ble_svr_far_init(void);

void tr_ble_advertise(void);
bool tr_bt_manager_ble_is_connected(void);
void tr_bt_manager_ble_reg_rv_data_cb(void (*cb)(const uint8_t *data, uint16_t len));
int tr_bt_manager_ble_send_data(uint8_t *data, uint16_t len);
int far_ble_read(uint8_t *buf, int num, int timeout);

