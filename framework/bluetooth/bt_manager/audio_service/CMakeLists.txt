#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_library_sources(ascs_service.c)
zephyr_library_sources(mcs_service.c)
zephyr_library_sources(mics_service.c)
zephyr_library_sources(pacs_service.c)
zephyr_library_sources(tbs_service.c)
zephyr_library_sources(vcs_service.c)
zephyr_library_sources(cas_service.c)
zephyr_library_sources(vnd_service.c)
zephyr_library_sources_ifdef(CONFIG_BT_TMAS_SERVICE    tmas_service.c)
