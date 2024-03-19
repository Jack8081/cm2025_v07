# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})

if (CONFIG_PARSER_MP3)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libmp3_parser/CMakeLists.txt)
	add_subdirectory(libmp3_parser)
else()
    zephyr_library_import(a1_mp3_p ${CMAKE_CURRENT_LIST_DIR}/a1_mp3_p.a)
endif()
endif()


if (CONFIG_PARSER_WAV)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libwav_parser/CMakeLists.txt)
	add_subdirectory(libwav_parser)
else()
    zephyr_library_import(a1_wav_p ${CMAKE_CURRENT_LIST_DIR}/a1_wav_p.a)
endif()
endif()

if (CONFIG_PARSER_WMA)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libwma_parser/CMakeLists.txt)
	add_subdirectory(libwma_parser)
else()
	zephyr_library_import(a1_wma_p ${CMAKE_CURRENT_LIST_DIR}/a1_wma_p.a)
endif()
endif()


if (CONFIG_PARSER_APE)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libape_parser/CMakeLists.txt)
	add_subdirectory(libape_parser)
else()
	zephyr_library_import(a1_ape_p ${CMAKE_CURRENT_LIST_DIR}/a1_ape_p.a)
endif()
endif()

if (CONFIG_PARSER_FLAC)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libflac_parser/CMakeLists.txt)
	add_subdirectory(libflac_parser)
else()
	zephyr_library_import(a1_flac_p ${CMAKE_CURRENT_LIST_DIR}/a1_flac_p.a)
endif()
endif()

if (CONFIG_PARSER_AAC)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libaac_parser/CMakeLists.txt)
	add_subdirectory(libaac_parser)
else()
	zephyr_library_import(a1_aac_p ${CMAKE_CURRENT_LIST_DIR}/a1_a13_p.a)
endif()
endif()

if (CONFIG_DECODER_ACT AND NOT CONFIG_DECODER_ACT_HW_ACCELERATION)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libact_decoder/CMakeLists.txt)
	add_subdirectory(libact_decoder)
else()
	zephyr_library_import(a1_act_d ${CMAKE_CURRENT_LIST_DIR}/a1_act_d.a)
endif()
endif()

if (CONFIG_MUSIC_DAE_FADE)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libfade/CMakeLists.txt)
	add_subdirectory(libfade)
else()
	zephyr_library_import(a1_fad_p ${CMAKE_CURRENT_LIST_DIR}/a1_fad_p.a)
endif()
endif()

if (CONFIG_RESAMPLE)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libresample/CMakeLists.txt)
	add_subdirectory(libresample)
else()
	zephyr_library_import(a1_res_s_p ${CMAKE_CURRENT_LIST_DIR}/a1_res_s_p.a)
endif()
endif()

