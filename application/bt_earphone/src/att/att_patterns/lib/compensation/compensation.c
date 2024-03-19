#include "soc_atp.h"
#include "gcc_include.h"
#include "compensation.h"
#include "att_debug.h"

//3*8=24bit
#define MAX_EFUSE_CAP_RECORD  (CONFIG_COMPENSATION_FREQ_INDEX_NUM)
#define CONFIG_COMPENSATION_DEFAULT_HOSC_CAP 100

static uint32_t read_efuse_freq_value(uint32_t *read_cap_value, int *index)
{
    int i;
	int ret_val;
    uint32_t cap_value[MAX_EFUSE_CAP_RECORD];

	memset(cap_value, 0, MAX_EFUSE_CAP_RECORD);
	
    for (i = 0; i < MAX_EFUSE_CAP_RECORD; i++) {
        ret_val = soc_atp_get_hosc_calib(i, &cap_value[i]);
		if(ret_val){
			*read_cap_value = 0xff;
			*index = 0xff;
			LOG_I("read efuse fail, index:%d\n", i);
			return -1;
		}
    }

    //return the last modified value, it is the newest value
    for (i = 0; i < MAX_EFUSE_CAP_RECORD; i++) {
        if (cap_value[i] == 0) {
            break;
        }
    }

    //Not written efuse
    if (i == 0) {
        *read_cap_value = 0xff;
        *index = 0xff;
		LOG_I("no write efuse!\n");
    } else {
        *read_cap_value = cap_value[i - 1];
        *index = i - 1;
		LOG_I("efuse, idx:%d, val:0x%x\n", i-1, *read_cap_value);
    }

    return 0;
}

static int32_t write_efuse_new_value(int new_value, int old_index)
{
    int new_index;
    int ret_val;

    if (old_index != 0xff) {
        new_index = old_index + 1;
    } else {
        new_index = 0;
    }

    if (new_index < MAX_EFUSE_CAP_RECORD) {
        ret_val = soc_atp_set_hosc_calib(new_index, new_value);
    } else {
        return -2;
    }

    return ret_val;
}

static int32_t write_efuse_freq_compensation(uint32_t *cap_value)
{
    uint32_t trim_cap_value;
    int index, ret_val;
    uint32_t old_cap_value;

    //write low 8bit only
    trim_cap_value = (*cap_value) & 0xff;

    ret_val = read_efuse_freq_value(&old_cap_value, &index);

	LOG_D("old val: %d index: %d ret:%d\n", old_cap_value, index, ret_val);

	if(ret_val){
		return ret_val;
	}

    if (index != 0xff) {
        if (old_cap_value == trim_cap_value) {
			LOG_D("value same, no need write!\n");
            return 0;
        } else {
            //Add a new frequency offset value
            //efuse already written fully
            if (index == (MAX_EFUSE_CAP_RECORD - 1)) {
				LOG_E("efuse position full!\n");
                return -2;
            } else {
                return write_efuse_new_value(trim_cap_value, index);
            }
        }
    } else {
        //Not written efuse
        return write_efuse_new_value(trim_cap_value, index);
    }
}

static int32_t spi_nor_freq_compensation_param_write(uint32_t *trim_cap)
{
#if 0
    int32_t ret_val;

    ret_val = property_set(CFG_BT_CFO_VAL, (void *) trim_cap, sizeof(u8_t));
    if (ret_val < 0)
        return ret_val;

    property_flush(CFG_BT_CFO_VAL);

    return ret_val;
#else
	//return -1;
	return 0;
#endif
}

int32_t freq_compensation_write(uint32_t *trim_cap, uint32_t mode)
{
    int ret_val;

    //Two bytes in total, two bytes of symmetric data
    *trim_cap &= 0xffff;

#ifdef CONFIG_COMPENSATION_HOSC_CAP_NVRAM_PRIORITY
    ret_val = spi_nor_freq_compensation_param_write(trim_cap);
    if (ret_val == 0) {
        ret_val = TRIM_CAP_WRITE_NO_ERROR;
    } else {
        ret_val = TRIM_CAP_WRITE_ERR_HW;
    }
#else
    ret_val = write_efuse_freq_compensation(trim_cap);

	LOG_I("write efuse val %d ret %d\n", *trim_cap, ret_val);

    if (mode == RW_TRIM_CAP_SNOR) {
        //efuse already written fully
        if (ret_val == -2) {
            //write norflash nvram
            ret_val = spi_nor_freq_compensation_param_write(trim_cap);

            if (ret_val == 0) {
                ret_val = TRIM_CAP_WRITE_NO_ERROR;
            } else {
                ret_val = TRIM_CAP_WRITE_ERR_HW;
            }
        } else if (ret_val == -1) {
            ret_val = TRIM_CAP_WRITE_ERR_HW;
        } else {
            ret_val = TRIM_CAP_WRITE_NO_ERROR;
        }
    } else {
        //efuse already written fully
        if (ret_val == -2) {
            ret_val = TRIM_CAP_WRITE_ERR_NO_RESOURSE;
        } else if (ret_val == -1) {
            ret_val = TRIM_CAP_WRITE_ERR_HW;
        } else {
            ret_val = TRIM_CAP_WRITE_NO_ERROR;
        }
    }
#endif

    return ret_val;
}

static int32_t read_efuse_freq_compensation(uint32_t *cap_value, int *index)
{
    uint32_t trim_cap_value = 0;
	int ret_val;

    ret_val = read_efuse_freq_value(&trim_cap_value, index);

	if(ret_val){
		return ret_val;
	}

    if (*index != 0xff) {
        *cap_value = trim_cap_value;
    }

    return 0;
}


int32_t freq_compensation_read(uint32_t *trim_cap, uint32_t mode)
{
	int ret_val;
	int index;
	uint32_t trim_cap_value_bak = 0;
	int ret_val_nvram = -1;

#if 0
	ret_val_nvram = spi_nor_freq_compensation_param_read(&trim_cap_value_bak);

#ifdef CONFIG_COMPENSATION_HOSC_CAP_NVRAM_PRIORITY
	if (ret_val_nvram > 0) {
		// update the norflash parameters
		*trim_cap = trim_cap_value_bak;
		// the parameter partition is valid
		ret_val = TRIM_CAP_READ_NO_ERROR;

		return ret_val;
	}
#endif
#endif

	ret_val = read_efuse_freq_compensation(trim_cap, &index);

	LOG_I("read efuse val 0x%x ret 0x%x idx 0x%x\n", *trim_cap, ret_val, index);

	if(ret_val) {
		return TRIM_CAP_READ_ERR_HW;
	}

	if (index == 0xff) {
		//efuse not write
		if (ret_val_nvram > 0) {
			*trim_cap = trim_cap_value_bak;
		} else {
			*trim_cap = CONFIG_COMPENSATION_DEFAULT_HOSC_CAP;
		}
		// the parameter partition is valid.
		//ret_val = TRIM_CAP_READ_NO_ERROR;
		ret_val = TRIM_CAP_READ_ERR_NO_WRITE;
	} else if (index < (MAX_EFUSE_CAP_RECORD - 1)) {
		// if efuse value is already valid and there is also a space to write, return the frequency offset value.
		//ret_val = TRIM_CAP_READ_NO_ERROR;
		LOG_I("efuse have space to write: %d\n", index+1);
		ret_val = TRIM_CAP_READ_HAVE_SPACE;
	} else {
		if (mode == RW_TRIM_CAP_EFUSE) {
			if (*trim_cap == 0xff) {
				*trim_cap = CONFIG_COMPENSATION_DEFAULT_HOSC_CAP;
				ret_val = TRIM_CAP_ERAD_ERR_VALUE;
			} else{
				//use efuse value
				//ret_val = TRIM_CAP_READ_NO_ERROR;
				ret_val = TRIM_CAP_READ_EFUSE_FULL;
			}
		} else {
			if (ret_val_nvram > 0) {
				// update the norflash parameters
				*trim_cap = trim_cap_value_bak;
			} else if(*trim_cap == 0xff) {
				*trim_cap = CONFIG_COMPENSATION_DEFAULT_HOSC_CAP;
			}
			// the parameter partition is valid.
			ret_val = TRIM_CAP_READ_NO_ERROR;
		}
	}

	return ret_val;
}

