#ifndef __CASE_CONFIG_H__
#define __CASE_CONFIG_H__

#if defined (CONFIG_SOC_SERIES_CUCKOO)
#include <config_cuckoo.h>
#elif defined(CONFIG_SOC_SERIES_LARK)
#include <config_lark.h>
#else
#error "need config_xxx.h"
#endif

#endif //__CASE_CONFIG_H__
