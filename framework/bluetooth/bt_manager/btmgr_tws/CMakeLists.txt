#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

if (CONFIG_TWS)
if (CONFIG_SNOOP_LINK_TWS)
add_subdirectory(snoop)
else()
add_subdirectory(retransmit)
endif()
else()
add_subdirectory(null)
endif()
