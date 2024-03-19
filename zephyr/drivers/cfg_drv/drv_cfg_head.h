#ifndef __CONFIG_DRV_CFG_HEAD_H__
#define __CONFIG_DRV_CFG_HEAD_H__


#if defined (CONFIG_SOC_SERIES_CUCKOO)
#include "drv_cfg_head_cuckoo.h"
#elif defined(CONFIG_SOC_SERIES_LARK)
#include "drv_cfg_head_lark.h"
#else
#error "need drv_cfg_head_xxx.h"
#endif


#endif //__CONFIG_DRV_CFG_HEAD_H__
