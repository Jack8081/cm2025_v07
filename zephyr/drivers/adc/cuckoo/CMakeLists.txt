# Copyright(C) 2020 Actions Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ADC_ACTS		adc_acts.c)

