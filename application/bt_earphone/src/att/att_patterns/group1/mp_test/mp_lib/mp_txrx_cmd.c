#include "ap_autotest_rf_rx.h"
#include <soc.h>

uint8 bt_rx_cmd[] = {
    K_CMD_SET_FREQ,
    // rf_channel
    36,

#if 0
    K_CMD_CFG_AGC,
    // m_bAgcEn
    0,
#endif

    K_CMD_SET_ACCESSCODE,
    // nAccessCode0
    0xe2,
    0x3a,
    0x1a,
    0x33,
    // nAccessCode1
    0xce,
    0x2c,
    0x7a,
    0x4e,

    K_CMD_SET_PAYLOAD,
    // nPayloadMode
    K_FCC_01010101,

    K_CMD_SET_RX_MODE,
    K_RX_MODE_DH1,
};


static int search_cmd(u8_t cmd)
{
	int i;
	for(i = 0; i < sizeof(bt_rx_cmd); i++){
		if(bt_rx_cmd[i] == cmd){
			return i;
		}
	}

	return -1;
}

int mp_btc_rx_init(uint8_t channel)
{
	int index;

	index = search_cmd(K_CMD_SET_FREQ);
	bt_rx_cmd[index + 1] = channel;
	index = search_cmd(K_CMD_SET_RX_MODE);
	//bt_rx_cmd[index + 1] = K_RX_MODE_DH1;
	bt_rx_cmd[index + 1] = K_RX_MODE_DH1;
	index = search_cmd(K_CMD_SET_PAYLOAD);
	//bt_rx_cmd[index + 1] = K_FCC_PN9;
	bt_rx_cmd[index+1] = K_FCC_01010101;

	return btdrv_send(1, bt_rx_cmd, sizeof(bt_rx_cmd));
}


int mp_btc_rx_begin(void)
{
	uint8_t tx_data[2];

	tx_data[0] = K_CMD_RX_EXCUTE_CFO;
	//0:single receive 1:continus receive
	tx_data[1] = 1;

	return btdrv_send(1, tx_data, sizeof(tx_data));

}

int mp_btc_rx_stop(void)
{
	uint8_t tx_data ;

	tx_data = K_CMD_RX_EXCUTE_STOP;
	return btdrv_send(1, &tx_data, sizeof(tx_data));
}

int mp_btc_clear_rxfifo(void)
{
	uint8_t tx_data;

	tx_data = K_CMD_RX_CLEAR_FIFO;

	return btdrv_send(1, &tx_data, sizeof(tx_data));
}

int mp_btc_get_report(void)
{
	uint8_t tx_data ;

	tx_data = K_CMD_RX_GET_REPORT;
	return btdrv_send(1, &tx_data, sizeof(tx_data));
}
