# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ADC			adc_common.c)
zephyr_library_sources_ifdef(CONFIG_ADC_ITE_IT8XXX2	adc_ite_it8xxx2.c)
zephyr_library_sources_ifdef(CONFIG_ADC_SHELL		adc_shell.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE		adc_handlers.c)
zephyr_library_sources_ifdef(CONFIG_ADC_CC32XX		adc_cc32xx.c)
zephyr_library_sources_ifdef(CONFIG_ADC_EMUL		adc_emul.c)
zephyr_library_sources_ifdef(CONFIG_ADC_XEC_V2		adc_mchp_xec_v2.c)

add_subdirectory_ifdef(CONFIG_SOC_SERIES_LARK lark)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_CUCKOO cuckoo)
