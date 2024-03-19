/**
 * @file lv_conf.h
 * Configuration file for v8.0.2
 */

/*
 * COPY THIS FILE AS `lv_conf.h` NEXT TO the `lvgl` FOLDER
 */

#ifndef ZEPHYR_LIB_GUI_LVGL_LV_CONF_H_
#define ZEPHYR_LIB_GUI_LVGL_LV_CONF_H_
/*clang-format off*/

#include <stdint.h>
#include <sys/util.h>

/*====================
   COLOR SETTINGS
 *====================*/

/*Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888)*/
#define LV_COLOR_DEPTH     CONFIG_LV_COLOR_DEPTH

/*Swap the 2 bytes of RGB565 color. Useful if the display has a 8 bit interface (e.g. SPI)*/
#define LV_COLOR_16_SWAP   IS_ENABLED(CONFIG_LV_COLOR_16_SWAP)

/*Enable more complex drawing routines to manage screens transparency.
 *Can be used if the UI is above another layer, e.g. an OSD menu or video player.
 *Requires `LV_COLOR_DEPTH = 32` colors and the screen's `bg_opa` should be set to non LV_OPA_COVER value*/
#define LV_COLOR_SCREEN_TRANSP    IS_ENABLED(CONFIG_LV_COLOR_SCREEN_TRANSP)

/* Adjust color mix functions rounding. GPUs might calculate color mix (blending) differently.
 * 0: round down, 64: round up from x.75, 128: round up from half, 192: round up from x.25, 254: round up */
#define LV_COLOR_MIX_ROUND_OFS CONFIG_LV_COLOR_MIX_ROUND_OFS

/*Images pixels with this color will not be drawn if they are  chroma keyed)*/
#define LV_COLOR_CHROMA_KEY    lv_color_hex(CONFIG_LV_COLOR_CHROMA_KEY_HEX)

/*=========================
   MEMORY SETTINGS
 *=========================*/

/*1: use custom malloc/free, 0: use the built-in `lv_mem_alloc()` and `lv_mem_free()`*/
#define LV_MEM_CUSTOM             IS_ENABLED(CONFIG_LV_MEM_CUSTOM)
#if LV_MEM_CUSTOM == 0
/*Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB)*/
#  define LV_MEM_SIZE    (CONFIG_LV_MEM_SIZE_KILOBYTES * 1024U)          /*[bytes]*/

/*Set an address for the memory pool instead of allocating it as a normal array. Can be in external SRAM too.*/
#  define LV_MEM_ADR          0     /*0: unused*/
/*Instead of an address give a memory allocator that will be called to get a memory pool for LVGL. E.g. my_malloc*/
#if LV_MEM_ADR == 0
//#define LV_MEM_POOL_INCLUDE your_alloc_library  /* Uncomment if using an external allocator*/
//#define LV_MEM_POOL_ALLOC   your_alloc          /* Uncomment if using an external allocator*/
#endif

#else       /*LV_MEM_CUSTOM*/
#  define LV_MEM_CUSTOM_INCLUDE   "lvgl_malloc.h"
#  define LV_MEM_CUSTOM_ALLOC     lvgl_malloc
#  define LV_MEM_CUSTOM_FREE      lvgl_free
#  define LV_MEM_CUSTOM_REALLOC   lvgl_realloc
#endif     /*LV_MEM_CUSTOM*/

/*Number of the intermediate memory buffer used during rendering and other internal processing mechanisms.
 *You will see an error log message if there wasn't enough buffers. */
#define LV_MEM_BUF_MAX_NUM CONFIG_LV_MEM_BUF_MAX_NUM

/*Use the standard `memcpy` and `memset` instead of LVGL's own functions. (Might or might not be faster).*/
#define LV_MEMCPY_MEMSET_STD    IS_ENABLED(CONFIG_LV_MEMCPY_MEMSET_STD)

/*====================
   HAL SETTINGS
 *====================*/

/*Use sys layer*/
#define LV_DISP_SYS_LAYER           IS_ENABLED(CONFIG_LV_DISP_SYS_LAYER)

/*Use top layer*/
#define LV_DISP_TOP_LAYER           IS_ENABLED(CONFIG_LV_DISP_TOP_LAYER)

/*Default display refresh period. LVG will redraw changed ares with this period time*/
#define LV_DISP_DEF_REFR_PERIOD     CONFIG_LV_DISP_DEF_REFR_PERIOD      /*[ms]*/

/*Input device read period in milliseconds*/
#define LV_INDEV_DEF_READ_PERIOD    CONFIG_LV_INDEV_DEF_READ_PERIOD      /*[ms]*/

/*Use a custom tick source that tells the elapsed time in milliseconds.
 *It removes the need to manually update the tick with `lv_tick_inc()`)*/
#define LV_TICK_CUSTOM                 IS_ENABLED(CONFIG_LV_TICK_CUSTOM)
#if LV_TICK_CUSTOM
#  define LV_TICK_CUSTOM_INCLUDE  "lvgl_tick.h"         /*Header for the system time function*/
#  define LV_TICK_CUSTOM_SYS_TIME_EXPR lvgl_tick_get()     /*Expression evaluating to current system time in ms*/
#endif   /*LV_TICK_CUSTOM*/

/*Default Dot Per Inch. Used to initialize default sizes such as widgets sized, style paddings.
 *(Not so important, you can adjust it to modify default sizes and spaces)*/
#define LV_DPI_DEF                  CONFIG_LV_DPI_DEF     /*[px/inch]*/

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

/*-------------
 * Drawing
 *-----------*/

/*Enable complex draw engine.
 *Required to draw shadow, gradient, rounded corners, circles, arc, skew lines, image transformations or any masks*/
#define LV_DRAW_COMPLEX IS_ENABLED(CONFIG_LV_DRAW_COMPLEX)
#if LV_DRAW_COMPLEX != 0

/*Allow buffering some shadow calculation.
 *LV_SHADOW_CACHE_SIZE is the max. shadow size to buffer, where shadow size is `shadow_width + radius`
 *Caching has LV_SHADOW_CACHE_SIZE^2 RAM cost*/
#define LV_SHADOW_CACHE_SIZE    CONFIG_LV_SHADOW_CACHE_SIZE

/* Set number of maximally cached circle data.
 * The circumference of 1/4 circle are saved for anti-aliasing
 * radius * 4 bytes are used per circle (the most often used radiuses are saved)
 * 0: to disable caching */
#define LV_CIRCLE_CACHE_SIZE CONFIG_LV_CIRCLE_CACHE_SIZE

#endif /*LV_DRAW_COMPLEX*/

/*Default image cache size. Image caching keeps the images opened.
 *If only the built-in image formats are used there is no real advantage of caching. (I.e. if no new image decoder is added)
 *With complex image decoders (e.g. PNG or JPG) caching can save the continuous open/decode of images.
 *However the opened images might consume additional RAM.
 *0: to disable caching*/
#define LV_IMG_CACHE_DEF_SIZE       CONFIG_LV_IMG_CACHE_DEF_SIZE

/*Maximum buffer size to allocate for rotation. Only used if software rotation is enabled in the display driver.*/
#define LV_DISP_ROT_MAX_BUF         CONFIG_LV_DISP_ROT_MAX_BUF

/*-------------
 * GPU
 *-----------*/

/*Use STM32's DMA2D (aka Chrom Art) GPU*/
#define LV_USE_GPU_STM32_DMA2D  0
#if LV_USE_GPU_STM32_DMA2D
/*Must be defined to include path of CMSIS header of target processor
e.g. "stm32f769xx.h" or "stm32f429xx.h"*/
#define LV_GPU_DMA2D_CMSIS_INCLUDE
#endif

/*Use NXP's PXP GPU iMX RTxxx platforms*/
#define LV_USE_GPU_NXP_PXP      0
#if LV_USE_GPU_NXP_PXP
/*1: Add default bare metal and FreeRTOS interrupt handling routines for PXP (lv_gpu_nxp_pxp_osa.c)
 *   and call lv_gpu_nxp_pxp_init() automatically during lv_init(). Note that symbol SDK_OS_FREE_RTOS
 *   has to be defined in order to use FreeRTOS OSA, otherwise bare-metal implementation is selected.
 *0: lv_gpu_nxp_pxp_init() has to be called manually before lv_init()
 */
#define LV_USE_GPU_NXP_PXP_AUTO_INIT 0
#endif

/*Use NXP's VG-Lite GPU iMX RTxxx platforms*/
#define LV_USE_GPU_NXP_VG_LITE 0

/*Use exnternal renderer*/
#define LV_USE_EXTERNAL_RENDERER IS_ENABLED(CONFIG_LV_USE_EXTERNAL_RENDERER)

/*Use SDL renderer API. Requires LV_USE_EXTERNAL_RENDERER*/
#define LV_USE_GPU_SDL IS_ENABLED(CONFIG_LV_USE_GPU_SDL)
#if LV_USE_GPU_SDL
#  define LV_GPU_SDL_INCLUDE_PATH <SDL2/SDL.h>
#endif

/*Use custom GPU callback*/
#define LV_USE_GPU  IS_ENABLED(CONFIG_LV_USE_GPU)

/*GPU Size Limit in Pixels*/
#define LV_GPU_SIZE_LIMIT CONFIG_LV_GPU_SIZE_LIMIT

/*-------------
 * Logging
 *-----------*/

/*Enable the log module*/
#define LV_USE_LOG      IS_ENABLED(CONFIG_LV_USE_LOG)
#if LV_USE_LOG

/*How important log should be added:
 *LV_LOG_LEVEL_TRACE       A lot of logs to give detailed information
 *LV_LOG_LEVEL_INFO        Log important events
 *LV_LOG_LEVEL_WARN        Log if something unwanted happened but didn't cause a problem
 *LV_LOG_LEVEL_ERROR       Only critical issue, when the system may fail
 *LV_LOG_LEVEL_USER        Only logs added by the user
 *LV_LOG_LEVEL_NONE        Do not log anything*/
#  define LV_LOG_LEVEL    CONFIG_LV_LOG_LEVEL

/*1: Print the log with 'printf';
 *0: User need to register a callback with `lv_log_register_print_cb()`*/
#  define LV_LOG_PRINTF   IS_ENABLED(CONFIG_LV_LOG_PRINTF)

/*Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs*/
#  define LV_LOG_TRACE_MEM            IS_ENABLED(CONFIG_LV_LOG_TRACE_MEM)
#  define LV_LOG_TRACE_TIMER          IS_ENABLED(CONFIG_LV_LOG_TRACE_TIMER)
#  define LV_LOG_TRACE_INDEV          IS_ENABLED(CONFIG_LV_LOG_TRACE_INDEV)
#  define LV_LOG_TRACE_DISP_REFR      IS_ENABLED(CONFIG_LV_LOG_TRACE_DISP_REFR)
#  define LV_LOG_TRACE_EVENT          IS_ENABLED(CONFIG_LV_LOG_TRACE_EVENT)
#  define LV_LOG_TRACE_OBJ_CREATE     IS_ENABLED(CONFIG_LV_LOG_TRACE_OBJ_CREATE)
#  define LV_LOG_TRACE_LAYOUT         IS_ENABLED(CONFIG_LV_LOG_TRACE_LAYOUT)
#  define LV_LOG_TRACE_ANIM           IS_ENABLED(CONFIG_LV_LOG_TRACE_ANIM)

#endif  /*LV_USE_LOG*/

/*-------------
 * Asserts
 *-----------*/

/*Enable asserts if an operation is failed or an invalid data is found.
 *If LV_USE_LOG is enabled an error message will be printed on failure*/
#define LV_USE_ASSERT_NULL          IS_ENABLED(CONFIG_LV_USE_ASSERT_NULL)   /*Check if the parameter is NULL. (Very fast, recommended)*/
#define LV_USE_ASSERT_MALLOC        IS_ENABLED(CONFIG_LV_USE_ASSERT_MALLOC)   /*Checks is the memory is successfully allocated or no. (Very fast, recommended)*/
#define LV_USE_ASSERT_STYLE         IS_ENABLED(CONFIG_LV_USE_ASSERT_STYLE)   /*Check if the styles are properly initialized. (Very fast, recommended)*/
#define LV_USE_ASSERT_MEM_INTEGRITY IS_ENABLED(CONFIG_LV_USE_ASSERT_MEM_INTEGRITY)   /*Check the integrity of `lv_mem` after critical operations. (Slow)*/
#define LV_USE_ASSERT_OBJ           IS_ENABLED(CONFIG_LV_USE_ASSERT_OBJ)   /*Check the object's type and existence (e.g. not deleted). (Slow)*/

/*Add a custom handler when assert happens e.g. to restart the MCU*/
#define LV_ASSERT_HANDLER_INCLUDE   <sys/__assert.h>
#define LV_ASSERT_HANDLER   __ASSERT(0, "LVGL fail");   /*Halt by default*/


/*-------------
 * Tracing
 *-----------*/

#define LV_STRACE                   IS_ENABLED(CONFIG_LV_STRACE)

#if LV_STRACE
#define LV_STRACE_INCLUDE           "tracing/tracing.h"

#define LV_STRACE_OBJ               IS_ENABLED(CONFIG_LV_STRACE_OBJ)
#define LV_STRACE_DRAW_COMPLEX      IS_ENABLED(CONFIG_LV_STRACE_DRAW_COMPLEX)

#define LV_STRACE_ID_INDEV_TASK     SYS_TRACE_ID_GUI_INDEV_TASK
#define LV_STRACE_ID_REFR_TASK      SYS_TRACE_ID_GUI_REFR_TASK
#define LV_STRACE_ID_DISP_REGISTER  SYS_TRACE_ID_GUI_DISPLAY_CREATE
#define LV_STRACE_ID_DISP_REMOVE    SYS_TRACE_ID_GUI_DISPLAY_DELETE
#define LV_STRACE_ID_OBJ_DRAW       SYS_TRACE_ID_GUI_WIDGET_DRAW
#define LV_STRACE_ID_OBJ_CREATE     SYS_TRACE_ID_GUI_WIDGET_CREATE
#define LV_STRACE_ID_OBJ_DELETE     SYS_TRACE_ID_GUI_WIDGET_DELETE
#define LV_STRACE_ID_DRAW_COMPLEX   SYS_TRACE_ID_GUI_DRAW_COMPLEX
#define LV_STRACE_ID_UPDATE_LAYOUT  SYS_TRACE_ID_GUI_UPDATE_LAYOUT

#define LV_STRACE_VOID(id)  sys_trace_void(id)
#define LV_STRACE_U32(id, p1)  sys_trace_u32(id, p1)
#define LV_STRACE_U32X2(id, p1, p2)  sys_trace_u32x2(id, p1, p2)
#define LV_STRACE_U32X3(id, p1, p2, p3)  sys_trace_u32x3(id, p1, p2, p3)
#define LV_STRACE_U32X4(id, p1, p2, p3, p4)  sys_trace_u32x4(id, p1, p2, p3, p4)
#define LV_STRACE_U32X5(id, p1, p2, p3, p4, p5)  sys_trace_u32x5(id, p1, p2, p3, p4, p5)
#define LV_STRACE_U32X6(id, p1, p2, p3, p4, p5, p6)  sys_trace_u32x6(id, p1, p2, p3, p4, p5, p6)
#define LV_STRACE_U32X7(id, p1, p2, p3, p4, p5, p6, p7)  sys_trace_u32x7(id, p1, p2, p3, p4, p5, p6, p7)
#define LV_STRACE_U32X8(id, p1, p2, p3, p4, p5, p6, p7, p8)  sys_trace_u32x8(id, p1, p2, p3, p4, p5, p6, p7, p8)
#define LV_STRACE_U32X9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)  sys_trace_u32x9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define LV_STRACE_U32X10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)  sys_trace_u32x10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)

#define LV_STRACE_STRING(id, string)  sys_trace_string(id, string)

#define LV_STRACE_CALL(id)  sys_trace_end_call(id)
#define LV_STRACE_CALL_U32(id, retv)  sys_trace_end_call_u32(id, retv)

#endif /* LV_STRACE */

/*-------------
 * Others
 *-----------*/

/*1: Show CPU usage and FPS count in the right bottom corner*/
#define LV_USE_PERF_MONITOR     IS_ENABLED(CONFIG_LV_USE_PERF_MONITOR)
#if LV_USE_PERF_MONITOR
#define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT
#endif

/*1: Show the used memory and the memory fragmentation in the left bottom corner
 * Requires LV_MEM_CUSTOM = 0*/
#define LV_USE_MEM_MONITOR      IS_ENABLED(CONFIG_LV_USE_MEM_MONITOR)
#if LV_USE_MEM_MONITOR
#define LV_USE_MEM_MONITOR_POS LV_ALIGN_BOTTOM_LEFT
#endif

/*1: Draw random colored rectangles over the redrawn areas*/
#define LV_USE_REFR_DEBUG       IS_ENABLED(CONFIG_LV_USE_REFR_DEBUG)

/*Change the built in (v)snprintf functions*/
#define LV_SPRINTF_CUSTOM   IS_ENABLED(CONFIG_LV_SPRINTF_CUSTOM)
#if LV_SPRINTF_CUSTOM
#  define LV_SPRINTF_INCLUDE <stdio.h>
#  define lv_snprintf     snprintf
#  define lv_vsnprintf    vsnprintf
#else   /*LV_SPRINTF_CUSTOM*/
#  define LV_SPRINTF_USE_FLOAT IS_ENABLED(CONFIG_LV_SPRINTF_USE_FLOAT)
#endif  /*LV_SPRINTF_CUSTOM*/

#define LV_USE_USER_DATA      IS_ENABLED(CONFIG_LV_USE_USER_DATA)

/*Garbage Collector settings
 *Used if lvgl is bound to higher level language and the memory is managed by that language*/
#define LV_ENABLE_GC 0
#if LV_ENABLE_GC != 0
#  define LV_GC_INCLUDE "gc.h"                           /*Include Garbage Collector related things*/
#endif /*LV_ENABLE_GC*/

/*=====================
 *  COMPILER SETTINGS
 *====================*/

/*For big endian systems set to 1*/
#define LV_BIG_ENDIAN_SYSTEM    IS_ENABLED(CONFIG_LV_BIG_ENDIAN_SYSTEM)

/*Define a custom attribute to `lv_tick_inc` function*/
#define LV_ATTRIBUTE_TICK_INC

/*Define a custom attribute to `lv_timer_handler` function*/
#define LV_ATTRIBUTE_TIMER_HANDLER

/*Define a custom attribute to `lv_disp_flush_ready` function*/
#define LV_ATTRIBUTE_FLUSH_READY

/*Required alignment size for buffers*/
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE CONFIG_LV_ATTRIBUTE_MEM_ALIGN_SIZE

/*Will be added where memories needs to be aligned (with -Os data might not be aligned to boundary by default).
 * E.g. __attribute__((aligned(4)))*/
#define LV_ATTRIBUTE_MEM_ALIGN __attribute__((__aligned__(4)))

/*Attribute to mark large constant arrays for example font's bitmaps*/
#define LV_ATTRIBUTE_LARGE_CONST

/*Complier prefix for a big array declaration in RAM*/
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/*Place performance critical functions into a faster memory (e.g RAM)*/
#define LV_ATTRIBUTE_FAST_MEM

/*Prefix variables that are used in GPU accelerated operations, often these need to be placed in RAM sections that are DMA accessible*/
#define LV_ATTRIBUTE_DMA

/*Export integer constant to binding. This macro is used with constants in the form of LV_<CONST> that
 *should also appear on LVGL binding API such as Micropython.*/
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning /*The default value just prevents GCC warning*/

/*Extend the default -32k..32k coordinate range to -4M..4M by using int32_t for coordinates instead of int16_t*/
#define LV_USE_LARGE_COORD  IS_ENABLED(CONFIG_LV_USE_LARGE_COORD)

/*==================
 *   FONT USAGE
 *===================*/

/*Montserrat fonts with ASCII range and some symbols using bpp = 4
 *https://fonts.google.com/specimen/Montserrat*/
#define LV_FONT_MONTSERRAT_8     IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_8)
#define LV_FONT_MONTSERRAT_10    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_10)
#define LV_FONT_MONTSERRAT_12    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_12)
#define LV_FONT_MONTSERRAT_14    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_14)
#define LV_FONT_MONTSERRAT_16    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_16)
#define LV_FONT_MONTSERRAT_18    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_18)
#define LV_FONT_MONTSERRAT_20    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_20)
#define LV_FONT_MONTSERRAT_22    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_22)
#define LV_FONT_MONTSERRAT_24    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_24)
#define LV_FONT_MONTSERRAT_26    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_26)
#define LV_FONT_MONTSERRAT_28    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_28)
#define LV_FONT_MONTSERRAT_30    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_30)
#define LV_FONT_MONTSERRAT_32    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_32)
#define LV_FONT_MONTSERRAT_34    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_34)
#define LV_FONT_MONTSERRAT_36    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_36)
#define LV_FONT_MONTSERRAT_38    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_38)
#define LV_FONT_MONTSERRAT_40    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_40)
#define LV_FONT_MONTSERRAT_42    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_42)
#define LV_FONT_MONTSERRAT_44    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_44)
#define LV_FONT_MONTSERRAT_46    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_46)
#define LV_FONT_MONTSERRAT_48    IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_48)

/*Demonstrate special features*/
#define LV_FONT_MONTSERRAT_12_SUBPX      IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_12_SUBPX)
#define LV_FONT_MONTSERRAT_28_COMPRESSED IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_28_COMPRESSED)  /*bpp = 3*/
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW IS_ENABLED(CONFIG_LV_FONT_DEJAVU_16_PERSIAN_HEBREW)  /*Hebrew, Arabic, Perisan letters and all their forms*/
#define LV_FONT_SIMSUN_16_CJK            IS_ENABLED(CONFIG_LV_FONT_SIMSUN_16_CJK)  /*1000 most common CJK radicals*/

/*Pixel perfect monospace fonts*/
#define LV_FONT_UNSCII_8        IS_ENABLED(CONFIG_LV_FONT_UNSCII_8)
#define LV_FONT_UNSCII_16       IS_ENABLED(CONFIG_LV_FONT_UNSCII_16)

/*Optionally declare custom fonts here.
 *You can use these fonts as default font too and they will be available globally.
 *E.g. #define LV_FONT_CUSTOM_DECLARE   LV_FONT_DECLARE(my_font_1) LV_FONT_DECLARE(my_font_2)*/
#define LV_FONT_CUSTOM_DECLARE

/*Always set a default font*/
#ifdef CONFIG_LV_FONT_DEFAULT_MONTSERRAT_8
#  define LV_FONT_DEFAULT &lv_font_montserrat_8
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_10)
#  define LV_FONT_DEFAULT &lv_font_montserrat_10
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_12)
#  define LV_FONT_DEFAULT &lv_font_montserrat_12
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_14)
#  define LV_FONT_DEFAULT &lv_font_montserrat_14
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_16)
#  define LV_FONT_DEFAULT &lv_font_montserrat_16
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_18)
#  define LV_FONT_DEFAULT &lv_font_montserrat_18
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_20)
#  define LV_FONT_DEFAULT &lv_font_montserrat_20
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_22)
#  define LV_FONT_DEFAULT &lv_font_montserrat_22
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_24)
#  define LV_FONT_DEFAULT &lv_font_montserrat_24
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_26)
#  define LV_FONT_DEFAULT &lv_font_montserrat_26
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_28)
#  define LV_FONT_DEFAULT &lv_font_montserrat_28
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_30)
#  define LV_FONT_DEFAULT &lv_font_montserrat_30
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_32)
#  define LV_FONT_DEFAULT &lv_font_montserrat_32
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_34)
#  define LV_FONT_DEFAULT &lv_font_montserrat_34
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_36)
#  define LV_FONT_DEFAULT &lv_font_montserrat_36
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_38)
#  define LV_FONT_DEFAULT &lv_font_montserrat_38
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_40)
#  define LV_FONT_DEFAULT &lv_font_montserrat_40
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_42)
#  define LV_FONT_DEFAULT &lv_font_montserrat_42
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_44)
#  define LV_FONT_DEFAULT &lv_font_montserrat_44
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_46)
#  define LV_FONT_DEFAULT &lv_font_montserrat_46
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_48)
#  define LV_FONT_DEFAULT &lv_font_montserrat_48
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_12_SUBPX)
#  define LV_FONT_DEFAULT &lv_font_montserrat_12_subpx
#elif defined(CONFIG_LV_FONT_DEFAULT_MONTSERRAT_28_COMPRESSED)
#  define LV_FONT_DEFAULT &lv_font_montserrat_28_compressed
#elif defined(CONFIG_LV_FONT_DEFAULT_DEJAVU_16_PERSIAN_HEBREW)
#  define LV_FONT_DEFAULT &lv_font_dejavu_16_persian_hebrew
#elif defined(CONFIG_LV_FONT_DEFAULT_SIMSUN_16_CJK)
#  define LV_FONT_DEFAULT &lv_font_simsun_16_cjk
#elif defined(CONFIG_LV_FONT_DEFAULT_UNSCII_8)
#  define LV_FONT_DEFAULT &lv_font_unscii_8
#elif defined(CONFIG_LV_FONT_DEFAULT_UNSCII_16)
#  define LV_FONT_DEFAULT &lv_font_unscii_16
#endif

/*Enable handling large font and/or fonts with a lot of characters.
 *The limit depends on the font size, font face and bpp.
 *Compiler error will be triggered if a font needs it.*/
#define LV_FONT_FMT_TXT_LARGE   IS_ENABLED(CONFIG_LV_FONT_FMT_TXT_LARGE)

/*Enables/disables support for compressed fonts.*/
#define LV_USE_FONT_COMPRESSED  IS_ENABLED(CONFIG_LV_USE_FONT_COMPRESSED)

/*Enable subpixel rendering*/
#define LV_USE_FONT_SUBPX       IS_ENABLED(CONFIG_LV_USE_FONT_SUBPX)
#if LV_USE_FONT_SUBPX
/*Set the pixel order of the display. Physical order of RGB channels. Doesn't matter with "normal" fonts.*/
#define LV_FONT_SUBPX_BGR       IS_ENABLED(CONFIG_LV_FONT_SUBPX_BGR)  /*0: RGB; 1:BGR order*/
#endif

/*=================
 *  TEXT SETTINGS
 *=================*/

/**
 * Select a character encoding for strings.
 * Your IDE or editor should have the same character encoding
 * - LV_TXT_ENC_UTF8
 * - LV_TXT_ENC_ASCII
 */
#ifdef CONFIG_LV_TXT_ENC_UTF8
#  define LV_TXT_ENC LV_TXT_ENC_UTF8
#else
#  define LV_TXT_ENC LV_TXT_ENC_ASCII
#endif

 /*Can break (wrap) texts on these chars*/
#define LV_TXT_BREAK_CHARS                  CONFIG_LV_TXT_BREAK_CHARS

/*If a word is at least this long, will break wherever "prettiest"
 *To disable, set to a value <= 0*/
#define LV_TXT_LINE_BREAK_LONG_LEN          CONFIG_LV_TXT_LINE_BREAK_LONG_LEN
#if LV_TXT_LINE_BREAK_LONG_LEN > 0
/*Minimum number of characters in a long word to put on a line before a break.
 *Depends on LV_TXT_LINE_BREAK_LONG_LEN.*/
#  define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  CONFIG_LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN

/*Minimum number of characters in a long word to put on a line after a break.
 *Depends on LV_TXT_LINE_BREAK_LONG_LEN.*/
#  define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN CONFIG_LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN
#else
#  define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN   3
#  define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN  3
#endif

/*The control character to use for signalling text recoloring.*/
#define LV_TXT_COLOR_CMD  CONFIG_LV_TXT_COLOR_CMD

/*Support bidirectional texts. Allows mixing Left-to-Right and Right-to-Left texts.
 *The direction will be processed according to the Unicode Bidirectioanl Algorithm:
 *https://www.w3.org/International/articles/inline-bidi-markup/uba-basics*/
#define LV_USE_BIDI         IS_ENABLED(CONFIG_LV_USE_BIDI)
#if LV_USE_BIDI
/*Set the default direction. Supported values:
 *`LV_BASE_DIR_LTR` Left-to-Right
 *`LV_BASE_DIR_RTL` Right-to-Left
 *`LV_BASE_DIR_AUTO` detect texts base direction*/
#ifdef CONFIG_LV_BIDI_DIR_LTR
#  define LV_BIDI_BASE_DIR_DEF  LV_BASE_DIR_LTR
#elif defined(LV_BIDI_DIR_RTL)
#  define LV_BIDI_BASE_DIR_DEF  LV_BASE_DIR_RTL
#else
#  define LV_BIDI_BASE_DIR_DEF  LV_BASE_DIR_AUTO
#endif
#endif /* LV_USE_BIDI */

/*Enable Arabic/Persian processing
 *In these languages characters should be replaced with an other form based on their position in the text*/
#define LV_USE_ARABIC_PERSIAN_CHARS IS_ENABLED(CONFIG_LV_USE_ARABIC_PERSIAN_CHARS)

/*==================
 *  WIDGET USAGE
 *================*/

/*Documentation of the widgets: https://docs.lvgl.io/latest/en/html/widgets/index.html*/

#define LV_USE_ARC          IS_ENABLED(CONFIG_LV_USE_ARC)

#define LV_USE_ANIMIMG	    IS_ENABLED(CONFIG_LV_USE_ANIMIMG)

#define LV_USE_BAR          IS_ENABLED(CONFIG_LV_USE_BAR)

#define LV_USE_BTN          IS_ENABLED(CONFIG_LV_USE_BTN)

#define LV_USE_BTNMATRIX    IS_ENABLED(CONFIG_LV_USE_BTNMATRIX)

#define LV_USE_CANVAS       IS_ENABLED(CONFIG_LV_USE_CANVAS)

#define LV_USE_CHECKBOX     IS_ENABLED(CONFIG_LV_USE_CHECKBOX)

#define LV_USE_DROPDOWN     IS_ENABLED(CONFIG_LV_USE_DROPDOWN)   /*Requires: lv_label*/

#define LV_USE_IMG          IS_ENABLED(CONFIG_LV_USE_IMG)   /*Requires: lv_label*/

#define LV_USE_LABEL        IS_ENABLED(CONFIG_LV_USE_LABEL)
#if LV_USE_LABEL
#  define LV_LABEL_TEXT_SELECTION         IS_ENABLED(CONFIG_LV_LABEL_TEXT_SELECTION)   /*Enable selecting text of the label*/
#  define LV_LABEL_LONG_TXT_HINT    IS_ENABLED(CONFIG_LV_LABEL_LONG_TXT_HINT)   /*Store some extra info in labels to speed up drawing of very long texts*/
#endif

#define LV_USE_LINE         IS_ENABLED(CONFIG_LV_USE_LINE)

#define LV_USE_ROLLER       IS_ENABLED(CONFIG_LV_USE_ROLLER)   /*Requires: lv_label*/
#if LV_USE_ROLLER
#  define LV_ROLLER_INF_PAGES       CONFIG_LV_ROLLER_INF_PAGES   /*Number of extra "pages" when the roller is infinite*/
#endif

#define LV_USE_SLIDER       IS_ENABLED(CONFIG_LV_USE_SLIDER)   /*Requires: lv_bar*/

#define LV_USE_SWITCH    IS_ENABLED(CONFIG_LV_USE_SWITCH)

#define LV_USE_TEXTAREA   IS_ENABLED(CONFIG_LV_USE_TEXTAREA)     /*Requires: lv_label*/
#if LV_USE_TEXTAREA != 0
#  define LV_TEXTAREA_DEF_PWD_SHOW_TIME     CONFIG_LV_TEXTAREA_DEF_PWD_SHOW_TIME    /*ms*/
#endif

#define LV_USE_TABLE  IS_ENABLED(CONFIG_LV_USE_TABLE)

/*==================
 * EXTRA COMPONENTS
 *==================*/

/*-----------
 * Widgets
 *----------*/
#define LV_USE_CALENDAR     IS_ENABLED(CONFIG_LV_USE_CALENDAR)
#if LV_USE_CALENDAR
# define LV_CALENDAR_WEEK_STARTS_MONDAY IS_ENABLED(CONFIG_LV_CALENDAR_WEEK_STARTS_MONDAY)
# if LV_CALENDAR_WEEK_STARTS_MONDAY
#  define LV_CALENDAR_DEFAULT_DAY_NAMES {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}
# else
#  define LV_CALENDAR_DEFAULT_DAY_NAMES {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"}
# endif

# define LV_CALENDAR_DEFAULT_MONTH_NAMES {"January", "February", "March",  "April", "May",  "June", "July", "August", "September", "October", "November", "December"}
# define LV_USE_CALENDAR_HEADER_ARROW       IS_ENABLED(CONFIG_LV_USE_CALENDAR_HEADER_ARROW)
# define LV_USE_CALENDAR_HEADER_DROPDOWN    IS_ENABLED(CONFIG_LV_USE_CALENDAR_HEADER_DROPDOWN)
#endif  /*LV_USE_CALENDAR*/

#define LV_USE_CHART        IS_ENABLED(CONFIG_LV_USE_CHART)

#define LV_USE_COLORWHEEL   IS_ENABLED(CONFIG_LV_USE_COLORWHEEL)

#define LV_USE_IMGBTN       IS_ENABLED(CONFIG_LV_USE_IMGBTN)

#define LV_USE_KEYBOARD     IS_ENABLED(CONFIG_LV_USE_KEYBOARD)

#define LV_USE_LED          IS_ENABLED(CONFIG_LV_USE_LED)

#define LV_USE_LIST         IS_ENABLED(CONFIG_LV_USE_LIST)

#define LV_USE_METER        IS_ENABLED(CONFIG_LV_USE_METER)

#define LV_USE_MSGBOX       IS_ENABLED(CONFIG_LV_USE_MSGBOX)

#define LV_USE_SPINBOX      IS_ENABLED(CONFIG_LV_USE_SPINBOX)

#define LV_USE_SPINNER      IS_ENABLED(CONFIG_LV_USE_SPINNER)

#define LV_USE_TABVIEW      IS_ENABLED(CONFIG_LV_USE_TABVIEW)

#define LV_USE_TILEVIEW     IS_ENABLED(CONFIG_LV_USE_TILEVIEW)

#define LV_USE_WIN          IS_ENABLED(CONFIG_LV_USE_WIN)

#define LV_USE_SPAN         IS_ENABLED(CONFIG_LV_USE_SPAN)
#if LV_USE_SPAN
/*A line text can contain maximum num of span descriptor */
#  define LV_SPAN_SNIPPET_STACK_SIZE   CONFIG_LV_SPAN_SNIPPET_STACK_SIZE
#endif

/*-----------
 * Themes
 *----------*/

/*A simple, impressive and very complete theme*/
#define LV_USE_THEME_DEFAULT    IS_ENABLED(CONFIG_LV_USE_THEME_DEFAULT)
#if LV_USE_THEME_DEFAULT

/*0: Light mode; 1: Dark mode*/
# define LV_THEME_DEFAULT_DARK     !IS_ENABLED(CONFIG_LV_THEME_DEFAULT_DARK)

/*1: Enable grow on press*/
# define LV_THEME_DEFAULT_GROW              IS_ENABLED(CONFIG_LV_THEME_DEFAULT_GROW)

/*Default transition time in [ms]*/
# define LV_THEME_DEFAULT_TRANSITION_TIME    CONFIG_LV_THEME_DEFAULT_TRANSITION_TIME
#endif /*LV_USE_THEME_DEFAULT*/

/*A very simple them that is a good starting point for a custom theme*/
 #define LV_USE_THEME_BASIC    IS_ENABLED(CONFIG_LV_USE_THEME_BASIC)

/*A theme designed for monochrome displays*/
#define LV_USE_THEME_MONO       IS_ENABLED(CONFIG_LV_USE_THEME_MONO)

/*-----------
 * Layouts
 *----------*/

/*A layout similar to Flexbox in CSS.*/
#define LV_USE_FLEX     IS_ENABLED(CONFIG_LV_USE_FLEX)

/*A layout similar to Grid in CSS.*/
#define LV_USE_GRID     IS_ENABLED(CONFIG_LV_USE_GRID)

/*---------------------
 * 3rd party libraries
 *--------------------*/

/*File system interfaces for common APIs
 *To enable set a driver letter for that API*/
#define LV_USE_FS_STDIO CONFIG_LV_USE_FS_STDIO        /*Uses fopen, fread, etc*/
#define LV_FS_STDIO_PATH CONFIG_LV_FS_STDIO_PATH    /*Set the working directory. If commented it will be "./" */

#define LV_USE_FS_POSIX CONFIG_LV_USE_FS_POSIX         /*Uses open, read, etc*/
#define LV_FS_POSIX_PATH CONFIG_LV_FS_POSIX_PATH    /*Set the working directory. If commented it will be "./" */

#define LV_USE_FS_WIN32 CONFIG_LV_USE_FS_WIN32        /*Uses CreateFile, ReadFile, etc*/
#define LV_FS_WIN32_PATH CONFIG_LV_FS_WIN32_PATH    /*Set the working directory. If commented it will be ".\\" */

#define LV_USE_FS_FATFS IS_ENABLED(CONFIG_LV_USE_FS_FATFS)        /*Uses f_open, f_read, etc*/

/*PNG decoder library*/
#define LV_USE_PNG IS_ENABLED(CONFIG_LV_USE_PNG)

/*BMP decoder library*/
#define LV_USE_BMP IS_ENABLED(CONFIG_LV_USE_BMP)

/* JPG + split JPG decoder library.
 * Split JPG is a custom format optimized for embedded systems. */
#define LV_USE_SJPG IS_ENABLED(CONFIG_LV_USE_SJPG)

/*GIF decoder library*/
#define LV_USE_GIF IS_ENABLED(CONFIG_LV_USE_GIF)

/*QR code library*/
#define LV_USE_QRCODE IS_ENABLED(CONFIG_LV_USE_QRCODE)

/*FreeType library*/
#define LV_USE_FREETYPE IS_ENABLED(CONFIG_LV_USE_FREETYPE)
#if LV_USE_FREETYPE
/*Memory used by FreeType to cache characters [bytes] (-1: no caching)*/
# define LV_FREETYPE_CACHE_SIZE CONFIG_LV_FREETYPE_CACHE_SIZE
#endif

/*Rlottie library*/
#define LV_USE_RLOTTIE IS_ENABLED(CONFIG_LV_USE_RLOTTIE)

/*-----------
 * Others
 *----------*/

/*1: Enable API to take snapshot for object*/
#define LV_USE_SNAPSHOT IS_ENABLED(CONFIG_LV_USE_SNAPSHOT)


/*==================
* EXAMPLES
*==================*/

/*Enable the examples to be built with the library*/
#define LV_BUILD_EXAMPLES IS_ENABLED(CONFIG_LV_BUILD_EXAMPLES)

/*--END OF LV_CONF_H--*/

#endif /*ZEPHYR_LIB_GUI_LVGL_LV_CONF_H_*/
