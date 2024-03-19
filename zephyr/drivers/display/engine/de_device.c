/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include "de_device.h"

DEVICE_DECLARE(de);

static int de_drv_init(const struct device *dev)
{
    int res = de_init(dev);

    if (res == 0) {
        IRQ_CONNECT(IRQ_ID_DE, 0, de_isr, DEVICE_GET(de), 0);
        irq_enable(IRQ_ID_DE);
    }

    return res;
}

#if IS_ENABLED(CONFIG_DISPLAY_ENGINE_DEV)
DEVICE_DEFINE(de, CONFIG_DISPLAY_ENGINE_DEV_NAME, &de_drv_init,
            de_pm_control, &de_drv_data, NULL, POST_KERNEL,
            CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &de_drv_api);
#endif
