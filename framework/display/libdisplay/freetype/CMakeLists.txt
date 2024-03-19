# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(freetype)

target_include_directories(freetype INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/builds ${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_library()

zephyr_library_compile_definitions(FT2_BUILD_LIBRARY)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/builds
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src/autofit
  ${CMAKE_CURRENT_SOURCE_DIR}/src/base
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/cache
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/cff
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/psaux
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/pshinter
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/psnames
  ${CMAKE_CURRENT_SOURCE_DIR}/src/raster
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sfnt
  ${CMAKE_CURRENT_SOURCE_DIR}/src/smooth
  ${CMAKE_CURRENT_SOURCE_DIR}/src/truetype
)

zephyr_library_sources(
  builds/zephyr/ftsystem.c
  builds/zephyr/ftqsort.c
  builds/zephyr/freetype_font_api.c

  src/base/ftbase.c
  src/base/ftinit.c
  src/base/ftdebug.c
  src/base/ftbbox.c
  src/base/ftglyph.c
  src/base/ftbitmap.c
#  src/cache/ftcache.c
#  src/cff/cff.c
#  src/psaux/psaux.c
#  src/pshinter/pshinter.c
#  src/psnames/psnames.c
  src/raster/raster.c
  src/sfnt/sfnt.c
  src/smooth/smooth.c
  src/truetype/truetype.c
)

zephyr_library_sources_ifdef(CONFIG_FREETYPE_MEM_POOL_USER builds/zephyr/ftmempool.c)
zephyr_library_sources_ifdef(CONFIG_FREETYPE_MEM_POOL_KERNEL builds/zephyr/ftmemkernel.c)
zephyr_library_sources_ifdef(CONFIG_FREETYPE_MEM_POOL_HEAP_KERNEL builds/zephyr/ftmemkernel.c)

zephyr_library_link_libraries(freetype)
