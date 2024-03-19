/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt service tws
 */

#if CONFIG_TWS
#if CONFIG_SNOOP_LINK_TWS
#include "snoop/btsrv_tws_snoop.h"
#else
#include "retransmit/btsrv_tws_retransmit.h"
#endif
#else
#include "null/btsrv_tws_null.h"
#endif
