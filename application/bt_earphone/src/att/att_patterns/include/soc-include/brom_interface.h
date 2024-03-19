
#ifndef _BROM_INTERFACE_H_
#define _BROM_INTERFACE_H_

#include "types.h"


typedef enum
{
    BOOT_TYPE_NULL = 0,
    BOOT_TYPE_SNOR,
    BOOT_TYPE_SNAND,
    BOOT_TYPE_SDMMC,
    BOOT_TYPE_UART,
    BOOT_TYPE_USB,
    BOOT_TYPE_POWERON,
    BOOT_TYPE_EFUSE,
	BOOT_TYPE_NOT_XIP_SNOR, /* Boot from SPI NOR that support NOT XIP solution. */
    BOOT_TYPE_MAX,
} boot_type_e;

typedef struct image_head {
	uint32_t  ih_magic0;
	uint32_t  ih_magic1;   //
	uint32_t  ih_load_addr;			/*load addr, include header*/
	uint8_t   ih_name[8];  //
	void (*entry)(unsigned int type, void *api, void *arg, void *mbrec_phy_addr);
	uint32_t ih_img_size;
	uint32_t ih_img_chksum;   	/*if not sign, use check sum，ih_img_size not include header */ 
	uint32_t ih_hdr_chksum;        /* include header. */
	uint16_t ih_hdr_size;          	/* Size of image header (bytes). */
	uint16_t ih_ptlv_size;   	/* Size of protected TLV area (bytes). */
	uint16_t ih_tlv_size;		 /*tlv size*/
	uint16_t ih_version;       
	uint32_t ih_flags;         /*IMAGE_TLV_***/   
	uint8_t  ih_ext_ictype[7];      			
	uint8_t  ih_storage_type;	 // mbrec
}image_head_t;

#define  BROM_API_ADDR		0x188

typedef struct 
{
	//for boot
	unsigned int (*p_mbrc_brec_data_check)(unsigned char* buf, unsigned char digital_sign); /* 校验函数*/
	unsigned int (*p_brom_nor_read)(unsigned int, unsigned int, unsigned char*);            /* spi nor read 函数*/
	unsigned int (*p_brom_snand_read) (unsigned int, unsigned int, unsigned char*);         /* spi nand read 函数*/
	unsigned int (*p_brom_card_read)(unsigned int,unsigned int,unsigned char*);             /* card read 函数*/
	void (*p_brom_uart_launcher)(uint32 connect_mode, uint32 timeout_ms);       																                /* UART launcher 函数*/
	void (*p_adfu_launcher)(uint32 connect_mode, uint32 timeout_ms);                                                          /* adfu launcher 函数*/
	void (*p_launch)(boot_type_e type, unsigned int run_addr, unsigned int phy_addr);       /* launcher 函数*/
	void* p_spinor_api;	    
	void* (*p_memset)(void *dst, int val, unsigned int count);
	void* (*p_memcpy)(void *dest, const void *src, unsigned int count);
	int (*p_memcmp)(const void *s1, const void *s2, unsigned int len);
	int (*p_vsnprintf)(char* buf, int size, uint32 linesep, const char* fmt, va_list args);                                                              /* spinor mem */
	unsigned int (*p_calculate_crc32)(unsigned char *buffer, unsigned int buf_len,	unsigned int crc_initial, bool last);	
	int (*p_verify_signature)(const unsigned char *key, const unsigned char *sig, const unsigned char *data, unsigned int len);
	unsigned char * (*p_image_tlv_find)(image_head_t *image_head, unsigned short it_type, unsigned short *it_len, int is_prot);
	const unsigned char* (*p_SHA256_hash)(const void* data, int len, unsigned char* digest);
	unsigned int (*p_image_checksum32)(void *data, unsigned int len);
	unsigned int (*p_brom_card_write)(unsigned int,unsigned int,unsigned char*);             /* card write 函数*/
    unsigned char (*p_crc8_maxim)(unsigned char seed, unsigned char *p, unsigned int counter);
    unsigned short (*p_crc16_ccitt)(uint16_t seed, unsigned char* ptr, unsigned int len);
}brom_api_t;


#define  pbrom_api   ((brom_api_t *)BROM_API_ADDR)

#endif /* _BROM_INTERFACE_H_ */
