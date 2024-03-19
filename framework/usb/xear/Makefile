# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

obj-$(CONFIG_XEAR_LED_CTRL) += xear_led_ctrl.o

ifneq ($(wildcard $(srctree)/ext/actions/system/xear/lib/Makefile),)
obj-y += lib/
endif