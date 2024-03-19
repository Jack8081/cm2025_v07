#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>


#define RTC_REMAIN2_BIT_CAP_FLAG	1

int soc_pstore_set(u32_t tag, u32_t value)
{
	switch(tag){
		case SOC_PSTORE_TAG_CAPACITY:
			sys_write32(value, RTC_REMAIN1);
			break;
		case SOC_PSTORE_TAG_FLAG_CAP:
			if(value)
				sys_set_bit(RTC_REMAIN2, RTC_REMAIN2_BIT_CAP_FLAG);
			else
				sys_clear_bit(RTC_REMAIN2, RTC_REMAIN2_BIT_CAP_FLAG);
			break;		
		
		default:
			return -1;
			break;

	}

	return 0;
}


int soc_pstore_get(u32_t tag, u32_t *p_value)
{
	switch(tag){
		case SOC_PSTORE_TAG_CAPACITY:
			*p_value = sys_read32(RTC_REMAIN1);
			break;
		case SOC_PSTORE_TAG_FLAG_CAP:
			if(sys_test_bit(RTC_REMAIN2, RTC_REMAIN2_BIT_CAP_FLAG))
				*p_value = 1;
			else
				*p_value = 0;
			break;	

		default:
			return -1;
			break;

	}
	return 0;
}


int soc_pstore_reset_all(void)
{
	int i;
	for(i = 0; i <6; i++ )
		sys_write32(0x0, RTC_REMAIN0+i*4); 
	return 0;
}

