#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

if (CONFIG_ACTS_BT)
  target_include_directories(acts_bluetooth INTERFACE
    .
	./include
  )

zephyr_library_sources(btsrv_main.c)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/btif/CMakeLists.txt)
    if (CONFIG_LE_AUDIO_BQB)
        SET(BT_SERVICE_LIB_NAME btservice_pts)
    else()
        SET(BT_SERVICE_LIB_NAME btservice)
    endif()
    SET(ZEPHYR_CURRENT_LIBRARY ${BT_SERVICE_LIB_NAME})
    SET(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/")

    zephyr_include_directories(./)
    
    zephyr_include_directories(./include)

    zephyr_library_named(${BT_SERVICE_LIB_NAME})
    add_subdirectory(btif)
    add_subdirectory(core)
    add_subdirectory(profile)
    add_subdirectory(tws_porting)
    add_subdirectory_ifdef(CONFIG_BT_TRANSCEIVER trs)
    target_link_libraries(${BT_SERVICE_LIB_NAME} INTERFACE zephyr_interface)
else()
    if (CONFIG_LE_AUDIO_BQB)
        zephyr_library_import(btservice ${CMAKE_CURRENT_LIST_DIR}/libbtservice_pts.a)
    else()
        zephyr_library_import(btservice ${CMAKE_CURRENT_LIST_DIR}/libbtservice.a)
    endif()
endif()
  
 
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libbt_service/CMakeLists.txt)
    add_subdirectory(libbt_service)
  else()
    zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
	if (CONFIG_SNOOP_LINK_TWS)
        if (CONFIG_LE_AUDIO_BQB)
            zephyr_library_import(bt_service ${CMAKE_CURRENT_LIST_DIR}/libbt_service_snoop_pts.a)
        else()
            zephyr_library_import(bt_service ${CMAKE_CURRENT_LIST_DIR}/libbt_service_snoop.a)
        endif()
	else()
        if (CONFIG_LE_AUDIO_BQB)
            zephyr_library_import(bt_service ${CMAKE_CURRENT_LIST_DIR}/libbt_service_retransmit_pts.a)
        else()
            zephyr_library_import(bt_service ${CMAKE_CURRENT_LIST_DIR}/libbt_service_retransmit.a)
        endif()
	endif()
  endif()


endif()
