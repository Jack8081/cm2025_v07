
#ifndef __PRODUCTION_TEST_H__
#define __PRODUCTION_TEST_H__
#include "soc.h"

// Self test
// #define PRODUCTION_TEST_MASTER_DEVICE_SELF_TEST


typedef struct {
	uint8_t length;
	uint8_t type;
	uint8_t company_id_0;
	uint8_t company_id_1;
	uint8_t type_0;
	uint8_t type_1;
} production_test_ble_adv_header_t;

typedef struct {
	uint8_t length;
	uint8_t cmd;
	uint8_t param[0];
} production_test_ble_adv_cmd_t;

enum {
	ADV_TYPE_0_ATS3019						= 0x19,
};

enum {
	ADV_TYPE_1_PRODUCTION_TEST				= 0x01,
};

enum {
	PRODUCTION_TEST_ADV_CMD_CHANGE_BT_NAME 	= 0x01,
};

#define ADV_TYPE_MANUFACTORY_DATA			(0xFF)
#define ADV_COMPANY_ID_ACTIONS				(0x03E0)




#endif // __PRODUCTION_TEST_H__

