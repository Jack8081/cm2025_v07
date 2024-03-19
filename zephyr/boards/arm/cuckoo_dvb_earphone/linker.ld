/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-M platforms.
 */

#include <autoconf.h>
#include <linker/sections.h>
#include <devicetree.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#ifdef CONFIG_SECTION_OVERLAY
#include <section_overlay.h>
#endif
/* physical address of RAM */
#ifdef CONFIG_XIP
#if defined(CONFIG_BOARD_NANDBOOT)
#define ROMABLE_REGION PSRAM
#else
#define ROMABLE_REGION FLASH
#endif
#define RAMABLE_REGION SRAM
#else
#define ROMABLE_REGION SRAM
#define RAMABLE_REGION SRAM
#endif

#if defined(CONFIG_XIP)
#define _DATA_IN_ROM __data_rom_start
#else
#define _DATA_IN_ROM
#endif

#if !defined(CONFIG_XIP) && (CONFIG_FLASH_SIZE == 0)
#define ROM_ADDR RAM_ADDR
#else
#if defined(CONFIG_BOARD_NANDBOOT)
#define ROM_ADDR PSRAM_ADDR
#else
#define ROM_ADDR (CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET)
#endif
#endif

#ifdef CONFIG_HAS_TI_CCFG
#define CCFG_SIZE 88
#define ROM_SIZE (CONFIG_FLASH_SIZE*1K - CONFIG_FLASH_LOAD_OFFSET - \
	CCFG_SIZE)
#define CCFG_ADDR (ROM_ADDR + ROM_SIZE)
#else
#if CONFIG_FLASH_LOAD_SIZE > 0
#define ROM_SIZE CONFIG_FLASH_LOAD_SIZE
#else
#define ROM_SIZE (CONFIG_FLASH_SIZE*1K - CONFIG_FLASH_LOAD_OFFSET)
#endif
#endif

#if defined(CONFIG_XIP)
#if defined(CONFIG_IS_BOOTLOADER)
#define RAM_SIZE (CONFIG_BOOTLOADER_SRAM_SIZE * 1K)
#define RAM_ADDR (CONFIG_SRAM_BASE_ADDRESS + \
	(CONFIG_SRAM_SIZE * 1K - RAM_SIZE))
#else
#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K - 0x400)
#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
#endif
#else
#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K - CONFIG_BOOTLOADER_SRAM_SIZE * 1K)
#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
#endif

#define PSRAM_SIZE (CONFIG_PSRAM_SIZE * 1K)
#define PSRAM_ADDR CONFIG_PSRAM_BASE_ADDRESS

#if defined(CONFIG_CUSTOM_SECTION_ALIGN)
_region_min_align = CONFIG_CUSTOM_SECTION_MIN_ALIGN_SIZE;
#else
/* Set alignment to CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
 * to make linker section alignment comply with MPU granularity.
 */
#if defined(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)
_region_min_align = CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE;
#else
/* If building without MPU support, use default 4-byte alignment. */
_region_min_align = 4;
#endif
#endif

#if !defined(CONFIG_CUSTOM_SECTION_ALIGN) && defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define MPU_ALIGN(region_size) \
	. = ALIGN(_region_min_align); \
	. = ALIGN( 1 << LOG2CEIL(region_size))
#else
#define MPU_ALIGN(region_size) \
	. = ALIGN(_region_min_align)
#endif

MEMORY
{
	FLASH                 (rx) : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
#ifdef CONFIG_HAS_TI_CCFG
	FLASH_CCFG            (rwx): ORIGIN = CCFG_ADDR, LENGTH = CCFG_SIZE
#endif
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_ccm), okay)
	CCM                   (rw) : ORIGIN = DT_REG_ADDR(DT_CHOSEN(zephyr_ccm)), LENGTH = DT_REG_SIZE(DT_CHOSEN(zephyr_ccm))
#endif
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
	DTCM                  (rw) : ORIGIN = DT_REG_ADDR(DT_CHOSEN(zephyr_dtcm)), LENGTH = DT_REG_SIZE(DT_CHOSEN(zephyr_dtcm))
#endif
	SRAM                  (wx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
#ifdef CONFIG_BT_STM32_IPM
	SRAM1                 (rw) : ORIGIN = RAM1_ADDR, LENGTH = RAM1_SIZE
	SRAM2                 (rw) : ORIGIN = RAM2_ADDR, LENGTH = RAM2_SIZE
#endif
	PSRAM                 (rx) : ORIGIN = PSRAM_ADDR, LENGTH = PSRAM_SIZE

	SHARE_DSP_CONFIG_RAM  (rw) : ORIGIN = 0x1082000, LENGTH = 0x600
	SHARE_DSP_BT_RAM      (rw) : ORIGIN = 0x1082600, LENGTH = 0x100
	ANC_RAM               (rw) : ORIGIN = 0x1082700, LENGTH = 0x0
	SHARE_RAM             (rw) : ORIGIN = 0x1082700, LENGTH = 0x5900

  /* DSP RAM7(16KB) usd by CPU  */
	SRAM_EXT              (rw) : ORIGIN = 0x1074000, LENGTH = 0x4000
	
	
    /* Used by and documented in include/linker/intlist.ld */
	IDT_LIST  (wx)      : ORIGIN = (RAM_ADDR + RAM_SIZE), LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{

#include <linker/rel-sections.ld>

	/*
	* .plt and .iplt are here according to 'arm-zephyr-elf-ld --verbose',
	* before text section.
	*/
	/DISCARD/ :
	{
		*(.plt)
	}

	/DISCARD/ :
	{
		*(.iplt)
	}

	GROUP_START(ROMABLE_REGION)

	__rom_region_start = ROM_ADDR;

	SECTION_PROLOGUE(rom_start,,)
	{

/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_START ...). This typically contains the vector
 * table and debug information.
 */
#include <snippets-rom-start.ld>

	} GROUP_LINK_IN(ROMABLE_REGION)

#ifdef CONFIG_SENSOR_ROM_BASE
	SECTION_PROLOGUE(sensor_rom,,) AT (CONFIG_SENSOR_ROM_BASE)
	{
		. = ALIGN(4);
		KEEP(*(.sensor_rom))
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

	SECTION_PROLOGUE(_ACTIONS_RODATA_SECTION_NAME,,)
	{
		. = ALIGN(4);
		__app_entry_table = .;
		KEEP(*(.app_entry))
		__app_entry_end = .;

		. = ALIGN(4);
		__service_entry_table = .;
		KEEP(*(.service_entry))
		__service_entry_end = .;

		. = ALIGN(4);
		__view_entry_table = .;
		KEEP(*(.view_entry))
		__view_entry_end = .;

	} GROUP_LINK_IN(ROMABLE_REGION)

#ifdef CONFIG_CODE_DATA_RELOCATION

#include <linker_relocate.ld>

#endif /* CONFIG_CODE_DATA_RELOCATION */

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
	__text_region_start = .;

#include <linker/kobject-text.ld>

		*(.text)
		*(".text.*")
		*(.gnu.linkonce.t.*)

		/*
		 * These are here according to 'arm-zephyr-elf-ld --verbose',
		 * after .gnu.linkonce.t.*
		 */
		*(.glue_7t) *(.glue_7) *(.vfp11_veneer) *(.v4_bx)

	} GROUP_LINK_IN(ROMABLE_REGION)

	__text_region_end = .;

/*#if defined (CONFIG_CPLUSPLUS)*/
	SECTION_PROLOGUE(.ARM.extab,,)
	{
		/*
		 * .ARM.extab section containing exception unwinding information.
		 */
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} GROUP_LINK_IN(ROMABLE_REGION)
/*#endif*/

	SECTION_PROLOGUE(.ARM.exidx,,)
	{
		/*
		 * This section, related to stack and exception unwinding, is placed
		 * explicitly to prevent it from being shared between multiple regions.
		 * It  must be defined for gcc to support 64-bit math and avoid
		 * section overlap.
		 */
		__start_unwind_idx = .;
		__exidx_start = .;
	#if defined (__GCC_LINKER_CMD__)
		*(.ARM.exidx* gnu.linkonce.armexidx.*)
	#endif
		__exidx_end = .;
		__stop_unwind_idx = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	__rodata_region_start = .;

#include <linker/common-rom.ld>
#include <linker/thread-local-storage.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		*(.rodata)
		*(".rodata.*")
		*(.gnu.linkonce.r.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>
#ifdef CONFIG_SOC_RODATA_LD
#include <soc-rodata.ld>
#endif
#ifdef CONFIG_SECTION_OVERLAY
	    . = ALIGN(4);
		__overlay_table = .;
		LONG(OVERLAY_TABLE_MAGIC)
		/* overlay items count */
		LONG(7)

		/* for a1_wav_p.a */
		LONG(OVERLAY_ID_LIBAPWAV)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apwav)));
		LONG(SIZEOF(.overlay.data.apwav));
		LONG(LOADADDR(.overlay.data.apwav));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apwav)));
		LONG(SIZEOF(.overlay.bss.apwav));

		/* for a1_mp3_p.a */
		LONG(OVERLAY_ID_LIBAPMP3)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apmp3)));
		LONG(SIZEOF(.overlay.data.apmp3));
		LONG(LOADADDR(.overlay.data.apmp3));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apmp3)));
		LONG(SIZEOF(.overlay.bss.apmp3));

		/* for a1_ape_p.a */
		LONG(OVERLAY_ID_LIBAPAPE)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apape)));
		LONG(SIZEOF(.overlay.data.apape));
		LONG(LOADADDR(.overlay.data.apape));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apape)));
		LONG(SIZEOF(.overlay.bss.apape));

		/* for a1_w13_p.a */
		LONG(OVERLAY_ID_LIBAPWMA)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apwma)));
		LONG(SIZEOF(.overlay.data.apwma));
		LONG(LOADADDR(.overlay.data.apwma));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apwma)));
		LONG(SIZEOF(.overlay.bss.apwma));

		/* for a1_fla_p.a */
		LONG(OVERLAY_ID_LIBADFLA)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apfla)));
		LONG(SIZEOF(.overlay.data.apfla));
		LONG(LOADADDR(.overlay.data.apfla));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apfla)));
		LONG(SIZEOF(.overlay.bss.apfla));

		/* for a1_a23_p.a */
		LONG(OVERLAY_ID_LIBAPAAC)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apaac)));
		LONG(SIZEOF(.overlay.data.apaac));
		LONG(LOADADDR(.overlay.data.apaac));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apaac)));
		LONG(SIZEOF(.overlay.bss.apaac));

		. = ALIGN(4);
#endif

#ifdef CONFIG_CUSTOM_RODATA_LD
/* Located in project source directory */
#include <custom-rodata.ld>
#endif

#include <linker/kobject-rom.ld>

		/*
		 * For XIP images, in order to avoid the situation when __data_rom_start
		 * is 32-bit aligned, but the actual data is placed right after rodata
		 * section, which may not end exactly at 32-bit border, pad rodata
		 * section, so __data_rom_start points at data and it is 32-bit aligned.
		 *
		 * On non-XIP images this may enlarge image size up to 3 bytes. This
		 * generally is not an issue, since modern ROM and FLASH memory is
		 * usually 4k aligned.
		 */
		. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

#include <linker/cplusplus-rom.ld>

	__rodata_region_end = .;
	MPU_ALIGN(__rodata_region_end -__rom_region_start);
	__rom_region_end = .;
	__rom_region_size = __rom_region_end - __rom_region_start;

    GROUP_END(ROMABLE_REGION)


	/*
	 * These are here according to 'arm-zephyr-elf-ld --verbose',
	 * before data section.
	 */
	/DISCARD/ : {
		*(.got.plt)
		*(.igot.plt)
		*(.got)
		*(.igot)
	}

    GROUP_START(RAMABLE_REGION)

	. = RAM_ADDR;
	/* Align the start of image SRAM with the
	 * minimum granularity required by MPU.
	 */
	. = ALIGN(_region_min_align);
	_image_ram_start = .;

	SECTION_DATA_PROLOGUE(_SRAM_BSS_SECTION_NAME,(NOLOAD),)
	{
		. = ALIGN(4);
		__kernel_ram_start = .;
		*(.dsp_hex_buf*)
		*(.media_mem_pool*)
		*(.interrupt.noinit.stack*)
		*(.page_mempool*)
		. = ALIGN(4096);
		*(.srm_irq_vector*)
		*(.codec.noinit.stack*)
		*(.main.noinit.stack*)
		*(.system.bss.sdfs_cache*)
		*(.diskio.cache.pool*)
		*(.ram.noinit*)

		__kernel_ram_save_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)	

#ifdef CONFIG_SECTION_OVERLAY
	OVERLAY : NOCROSSREFS
	{
		.overlay.data.apfla {
			*a1_fla_p.a:*(.data .data.*)
		}

		.overlay.data.apwma {
			*a1_w13_p.a:*(.data .data.*)
		}

		.overlay.data.apape {
			*a1_ape_p.a:*(.data .data.*)
		}

		.overlay.data.apmp3 {
			*a1_mp3_p.a:*(.data .data.*)
		}

		.overlay.data.apwav {
			*a1_wav_p.a:*(.data .data.*)
		}

		.overlay.data.apaac {
			*a1_a13_p.a:*(.data .data.*)
		}
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>
	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		__data_region_start = .;
		__data_start = .;
		*(.data)
		*(".data.*")
		*(".kernel.*")

		/* Located in generated directory. This file is populated by the
		 * zephyr_linker_sources() Cmake function.
		 */
		#include <snippets-rwdata.ld>
		#ifdef CONFIG_SOC_RWDATA_LD
		#include <soc-rwdata.ld>
		#endif

		#ifdef CONFIG_CUSTOM_RWDATA_LD
		/* Located in project source directory */
		#include <custom-rwdata.ld>
		#endif

		#ifdef CONFIG_CODE_DATA_RELOCATION
		#include <linker_sram_data_relocate.ld>
		#endif
	__data_end = .;

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
    __data_size = __data_end - __data_start;
    __data_load_start = LOADADDR(_DATA_SECTION_NAME);

    __data_region_load_start = LOADADDR(_DATA_SECTION_NAME);

	#include <linker/common-ram.ld>
	#include <linker/kobject-data.ld>
	#include <linker/cplusplus-ram.ld>

    __data_region_end = .;
	
#ifndef CONFIG_USERSPACE
	SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		. = ALIGN(4);
		__bss_start = .;


#ifdef CONFIG_SECTION_OVERLAY
		*(EXCLUDE_FILE (*libdecencsbc.a:* *libdecaac.a:* *libdecape.a:* *libdecflac.a:* *libdecamr.a:* *libdecwav.a:* *libdecmp3.a:* *libdecwma.a:* *a1_act_d.a:* *libencamr.a:* *libwav.a:* *a1_wav_p.a:* *a1_w13_p.a:* *a1_ape_p.a:* *a1_mp3_p.a:* *a1_fla_p.a:* *a1_a13_p.a:* *libkshow.a:* *libencpp.a:* *libencopus.a:* *libencspeex.a:* *libdnoise.a:* *libCSpotter_16k24d.a:* *libresample.a:*) .bss)
		*(EXCLUDE_FILE (*libdecencsbc.a:* *libdecaac.a:* *libdecape.a:* *libdecflac.a:* *libdecamr.a:* *libdecwav.a:* *libdecmp3.a:* *libdecwma.a:* *a1_act_d.a:* *libencamr.a:* *libwav.a:* *a1_wav_p.a:* *a1_w13_p.a:* *a1_ape_p.a:* *a1_mp3_p.a:* *a1_fla_p.a:* *a1_a13_p.a:* *libkshow.a:* *libencpp.a:* *libencopus.a:* *libencspeex.a:* *libdnoise.a:* *libCSpotter_16k24d.a:* *libresample.a:*) .bss.*)
		*(EXCLUDE_FILE (*libdecencsbc.a:* *libdecaac.a:* *libdecape.a:* *libdecflac.a:* *libdecamr.a:* *libdecwav.a:* *libdecmp3.a:* *libdecwma.a:* *a1_act_d.a:* *libencamr.a:* *libwav.a:* *a1_wav_p.a:* *a1_w13_p.a:* *a1_ape_p.a:* *a1_mp3_p.a:* *a1_fla_p.a:* *a1_a13_p.a:* *libkshow.a:* *libencpp.a:* *libencopus.a:* *libencspeex.a:* *libdnoise.a:* *libCSpotter_16k24d.a:* *libresample.a:*) .scommon)
		*(EXCLUDE_FILE (*libdecencsbc.a:* *libdecaac.a:* *libdecape.a:* *libdecflac.a:* *libdecamr.a:* *libdecwav.a:* *libdecmp3.a:* *libdecwma.a:* *a1_act_d.a:* *libencamr.a:* *libwav.a:* *a1_wav_p.a:* *a1_w13_p.a:* *a1_ape_p.a:* *a1_mp3_p.a:* *a1_fla_p.a:* *a1_a13_p.a:* *libkshow.a:* *libencpp.a:* *libencopus.a:* *libencspeex.a:* *libdnoise.a:* *libCSpotter_16k24d.a:* *libresample.a:*) COMMON)

#else
		*(.scommon)
		*(.bss)
		*(".bss.*")
		*(COMMON)
#endif

		*(".kernel_bss.*")
		*(.bthost_bss*)
		*(.btsrv_bss*)
		
#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_bss_relocate.ld>
#endif

        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		 */
		__bss_end = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
	
	
#ifdef CONFIG_SLEEP_FUNC_IN_SRAM
	SECTION_DATA_PROLOGUE(_SRAM_SLEEPFUNC_SECTION_NAME,,)
	{
		. = ALIGN(4);
		_sleepfunc_ram_start = .;
		 *(.sleepfunc)
		 *(".sleepfunc.*")
		 *(.sleep.noinit*)
		_sleepfunc_ram_end = .;
	}GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
	_sleepfunc_ram_size = _sleepfunc_ram_end - _sleepfunc_ram_start;
	_sleepfunc_rom_start = LOADADDR(_SRAM_SLEEPFUNC_SECTION_NAME);
#endif

	SECTION_PROLOGUE(_SLEEP_NOT_SAVE_SECTION_NAME,(NOLOAD),)
	{
            __sleepnosave_ram_start = .;
		*(.act_s2_not_save_mem*)
            __sleepnosave_ram_end = .;
	}GROUP_LINK_IN(RAMABLE_REGION)

#ifdef CONFIG_SECTION_OVERLAY
	OVERLAY : NOCROSSREFS
	{
		.overlay.bss.apfla {
			*a1_fla_p.a:*(.bss .bss.* .scommon COMMON)
		}

		.overlay.bss.apwma {
			*a1_w13_p.a:*(.bss .bss.* .scommon COMMON)
		}

		.overlay.bss.apape {
			*a1_ape_p.a:*(.bss .bss.* .scommon COMMON)
		}

		.overlay.bss.apmp3 {
			*a1_mp3_p.a:*(.bss .bss.* .scommon COMMON)
		}

		.overlay.bss.apwav {
			*a1_wav_p.a:*(.bss .bss.* .scommon COMMON)
		}

		.overlay.bss.apaac {
			*a1_a13_p.a:*(.bss .bss.* .scommon COMMON)
		}
		/*
		.overlay.bss.dummy {
            . += 0x1fc;
		}
		*/
	} GROUP_LINK_IN(RAMABLE_REGION)
#endif


	SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
	{
		/*
		* This section is used for non-initialized objects that
		* will not be cleared during the boot process.
		*/
		*(.noinit)
		*(".noinit.*")
		*(".kernel_noinit.*")

		/* Located in generated directory. This file is populated by the
		 * zephyr_linker_sources() Cmake function.
		 */
	#include <snippets-noinit.ld>
	#ifdef CONFIG_SOC_NOINIT_LD
	#include <soc-noinit.ld>
	#endif

	} GROUP_LINK_IN(RAMABLE_REGION)
#endif /* CONFIG_USERSPACE */

	/* Define linker symbols */

	_image_ram_end = .;

	_end = .; /* end of image */

	__kernel_ram_end = RAM_ADDR + RAM_SIZE;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

	GROUP_END(RAMABLE_REGION)

	SECTION_DATA_PROLOGUE(_SHARE_DSP_CONFIG_RAM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__share_dsp_config_ram_start = .;
		*(.DSP_SHARE_CONFIG_RAM*)
	} GROUP_LINK_IN(SHARE_DSP_CONFIG_RAM)
	
	__share_dsp_config_ram_end = .;

	SECTION_DATA_PROLOGUE(_SHARE_DSP_BT_RAM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__share_dsp_bt_ram_start = .;
		*(.DSP_BT_SHARE_RAM*)
	} GROUP_LINK_IN(SHARE_DSP_BT_RAM)
	
	__share_dsp_bt_ram_end = .;

	SECTION_DATA_PROLOGUE(_SHARE_RAM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__share_ram_start = .;
		*(.DSP_SHARE_RAM*)
		__share_media_ram_start = .;
		*(.DSP_SHARE_MEDIA_RAM*)
		__share_media_ram_end = .;
	} GROUP_LINK_IN(SHARE_RAM)

	__share_ram_end = .;

	SECTION_DATA_PROLOGUE(_ANC_RAM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__anc_ram_start = .;
		*(.ANC_SHARE_RAM*)
	} GROUP_LINK_IN(ANC_RAM)
	
	__anc_ram_end = .;

	SECTION_DATA_PROLOGUE(_EXT_RAM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__ext_ram_start = .;
		*(.SRAM_EXT_RAM*)
	} GROUP_LINK_IN(SRAM_EXT)
	
	__ext_ram_end = .;


#if defined(CONFIG_BOARD_NANDBOOT)
	SECTION_DATA_PROLOGUE(_PRAM_BSS_SECTION_NAME, ALIGN(0x200),SUBALIGN(0x400))
#else
	SECTION_DATA_PROLOGUE(_PRAM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(0x400))
#endif
	{
		*(.UI_PSRAM_REGION*)
	} GROUP_LINK_IN(PSRAM)

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
GROUP_START(DTCM)

	SECTION_PROLOGUE(_DTCM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__dtcm_start = .;
		__dtcm_bss_start = .;
		*(.dtcm_bss)
		*(".dtcm_bss.*")
	} GROUP_LINK_IN(DTCM)

	__dtcm_bss_end = .;

	SECTION_PROLOGUE(_DTCM_NOINIT_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__dtcm_noinit_start = .;
		*(.dtcm_noinit)
		*(".dtcm_noinit.*")
	} GROUP_LINK_IN(DTCM)

	__dtcm_noinit_end = .;

	SECTION_PROLOGUE(_DTCM_DATA_SECTION_NAME,,SUBALIGN(4))
	{
		__dtcm_data_start = .;
		*(.dtcm_data)
		*(".dtcm_data.*")
	} GROUP_LINK_IN(DTCM AT> ROMABLE_REGION)

	__dtcm_data_end = .;
	__dtcm_end = .;

	__dtcm_data_rom_start = LOADADDR(_DTCM_DATA_SECTION_NAME);

GROUP_END(DTCM)
#endif

#ifdef CONFIG_CUSTOM_SECTIONS_LD
/* Located in project source directory */
#include <custom-sections.ld>
#endif

	/* Located in generated directory. This file is populated by the
	 * zephyr_linker_sources() Cmake function.
	 */
#include <snippets-sections.ld>

#include <linker/debug-sections.ld>

    /DISCARD/ : { *(.note.GNU-stack) }

    SECTION_PROLOGUE(.ARM.attributes, 0,)
	{
		KEEP(*(.ARM.attributes))
		KEEP(*(.gnu.attributes))
	}

	/* Must be last in romable region */
	SECTION_PROLOGUE(.last_section,(NOLOAD),)
	{

	} GROUP_LINK_IN(ROMABLE_REGION)

	/* To provide the image size as a const expression,
	 * calculate this value here. */
	_flash_used = LOADADDR(.last_section) - __rom_region_start;

}
