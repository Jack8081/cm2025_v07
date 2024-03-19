# Copyright(C) 2020 Actions Corporation.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_SOC_SERIES_LARK lark)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_CUCKOO_FPGA cuckoo)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_CUCKOO cuckoo)
