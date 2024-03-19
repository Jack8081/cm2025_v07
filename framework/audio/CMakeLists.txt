# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_AUDIO)

set(ZEPHYR_CURRENT_LIBRARY acts_audio)

add_library(acts_audio INTERFACE)

zephyr_include_directories(
	.
	include
)

target_include_directories(acts_audio INTERFACE
	include
)

zephyr_library()

zephyr_library_sources(
    audio_aps.c
    audio_policy.c
    audio_record.c
    audio_system.c
    audio_track.c
)
zephyr_library_sources_ifdef(CONFIG_VOLUME_MANAGER
    volume_manager.c
)
zephyr_library_sources_ifdef(CONFIG_PCM_BUFFER_STREAM
	pcm_buffer_stream.c
)

if (CONFIG_TWS)
	if (CONFIG_SNOOP_LINK_TWS)
		zephyr_library_sources(
			audio_tws_aps_snoop.c
		)
		
		if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libaudio/CMakeLists.txt)
			add_subdirectory(libaudio)
		else()
			zephyr_library_import(audio ${CMAKE_CURRENT_LIST_DIR}/libaudio.a)
		endif()
	else()
		zephyr_library_sources(audio_tws_aps.c)
	endif()
endif()

zephyr_library_link_libraries(acts_audio)

zephyr_library_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM ELMFAT INTERFACE)

target_link_libraries(acts_audio INTERFACE zephyr_interface)

add_subdirectory(audiolcy)

endif()

