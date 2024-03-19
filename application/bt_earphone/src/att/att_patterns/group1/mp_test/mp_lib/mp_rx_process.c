#include "ap_autotest_rf_rx.h"
#include "mp_cuckoo.h"


void mp_rx_packet_dealwith(struct mp_test_stru *mp_manager)
{
    int8 x;
    int16 y;
    uint8 i,j;
    int32 rssi_avg = 0;
    int32 cfo_avg = 0;
    int16 cfo_real = 0;
    uint8 index = mp_manager->mp_valid_packets_cn % 16;

    for(i=0;i<8;i++)
    {
        for(j=1;j<8-i;j++)
        {
            if(mp_manager->rssi_tmp[j-1] > mp_manager->rssi_tmp[j])
            {
                x = mp_manager->rssi_tmp[j];
                mp_manager->rssi_tmp[j]=mp_manager->rssi_tmp[j-1];
                mp_manager->rssi_tmp[j-1]=x;
            }

            if(mp_manager->cfo_tmp[j-1] > mp_manager->cfo_tmp[j])
            {
                y = mp_manager->cfo_tmp[j];
                mp_manager->cfo_tmp[j] = mp_manager->cfo_tmp[j-1];
                mp_manager->cfo_tmp[j-1] = y;
            }
        }
    }

    for( i = 2 ; i < 6 ; i++)
    {
        rssi_avg += mp_manager->rssi_tmp[i];
        cfo_avg += mp_manager->cfo_tmp[i];

		LOG_I("rssi %d avg %d\n", mp_manager->rssi_tmp[i], rssi_avg);
    }

    mp_manager->rssi_val[index] = rssi_avg >> 2;



    cfo_real = (int16)(cfo_avg >> 2);

	mp_manager->cfo_val[index] = cfo_real;

    mp_manager->mp_valid_packets_cn++;
    LOG_I("----rssi v:%d cfo v :%d----\n",mp_manager->rssi_val[index],mp_manager->cfo_val[index]);

}


int mp_analyse_cfo_result(struct mp_test_stru *mp_manager, int16_t cfo, int16_t rssi)
{
	int index;

	index = mp_manager->mp_packets_cn;
	index %= 8;

	mp_manager->rssi_tmp[index] = rssi;
	mp_manager->cfo_tmp[index] = cfo;
	mp_manager->mp_packets_cn++;

	LOG_D("recv pkg cfo %d rssi %d\n", cfo, rssi);

	if((mp_manager->mp_packets_cn % 8) == 0){
		mp_rx_packet_dealwith(mp_manager);
		if(mp_manager->mp_valid_packets_cn == 16){
			mp_manager->mp_valid_packets_cn = 0;
			if(mp_manager->mp_rx_data_ready == 0){
				mp_manager->mp_rx_data_ready = 1;
			}
		}

		return 0;
	}

	return -1;

}

int mp_bt_packet_receive_start(void)
{
	return mp_btc_rx_begin();
}

int mp_bt_packet_receive_stop(void)
{
	mp_btc_rx_stop();

	mp_btc_clear_hci_rx_buffer();

	return 0;
}

int mp_bt_packet_receive_process(unsigned int channel, unsigned int packet_counter)
{
	int ret;
	uint16_t cfo_data[2];
	struct mp_test_stru *mp_test = &g_mp_test;


	while(1){
		ret = mp_btc_read_hci_data((uint8_t *)&cfo_data, sizeof(cfo_data));

		if(ret == sizeof(cfo_data)){
			ret = mp_analyse_cfo_result(mp_test, cfo_data[0], cfo_data[1]);

			if(ret == 0){
				break;
			}
		}
	}

	return ret;
}


bool att_bttool_rx_begin(u8_t channel)
{
	mp_btc_rx_init(channel);

	return true;
}

bool att_bttool_rx_stop(void)
{
	mp_btc_rx_stop();

	return true;
}

int att_bttool_rx_report(mp_rx_report_t *rx_report)
{
#if 0
	int ret;

	ret = mp_btc_read_hci_data((uint8_t *)rx_report, sizeof(mp_rx_report_t));

	if(ret == sizeof(mp_rx_report_t)){
		return ret;
	}
#endif

	return 0;

}

int mp_bt_rx_report(mp_rx_report_t *rx_report)
{
	att_bttool_rx_stop();

	mp_btc_get_report();

	return att_bttool_rx_report(rx_report);
}





