#include <device.h>
#include <drivers/flash.h>
#include <drivers/spi.h>
#include <drivers/dma.h>
#include <soc_regs.h>
#include <logging/log.h>
#include <soc.h>

/*spinand rom print level*/
#define DEBUG_LEVEL 4
#define INFO_LEVEL 	3
#define WARN_LEVEL  2
#define ERROR_LEVEL 1

/* struct FlashChipInfo takes total 60 bytes size */
#define NAND_CHIPID_LENGTH              (8)     /* nand flash chipid length */
/*
 * struct FlashChipInfo takes total 60 bytes size
 * read mode:
 * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
 * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
 * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
 * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
 * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans.
 * write mode:
 * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
 * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
 */
struct FlashChipInfo
{
    uint8_t  ChipID[NAND_CHIPID_LENGTH]; /* nand flash id */
    uint8_t  ChipCnt;                    /* the count of chip connecting in flash*/
    uint8_t  DieCntPerChip;              /* the die count in a chip */
    uint8_t  PlaneCntPerDie;             /* the plane count in a die */
    uint8_t  SectorNumPerPage;             /* page size, based on sector */
    uint8_t  SpareBytesPerSector;        /* spare bytes per sector */
    uint8_t  Frequence;                  /* spi clk, may be based on xMHz */
    uint16_t PageNumPerPhyBlk;              /* the page number of physic block */
    uint16_t TotalBlkNumPerDie;               /* total number of the physic block in a die */
    uint16_t DefaultLBlkNumPer1024Blk;   /* Logical number per 1024 physical block */
    uint16_t userMetaOffset;             /*user meta data offset, add for spinand*/
    uint8_t  userMetaShift;              /*user meta data shift, add for spinand*/
    uint8_t  userMetaSize;               /*user meta data size, add for spinand*/
    uint8_t  ECCBits;
    uint8_t  eccStatusMask;              /*ecc status mask, add for spinand*/
    uint8_t  eccStatusErrVal;            /*ecc status err value, add for spinand*/
    uint8_t  readAddressBits;            /*read address bit, 16bit or 24bit address, add for spinand*/
    uint8_t  readMode;
    uint8_t  writeMode;
    uint8_t  Version;                    /* flash chipinfo version */
    uint8_t  FlashMark[16];              /* the name of spinand */
    uint8_t  delayChain;                 /* the delaychain of spinand, value from 0 to 15 */
    uint8_t  Reserved1[12];              /* reserved */
} __attribute__ ((packed));

//10Byte
struct NandIdTblHeader
{
        u8_t num;
        u32_t magic;
        u32_t checksum;
        u8_t  Reserved;
} __attribute__ ((packed));

struct nand_info {
        /* spi controller address */
        uint32_t base;
        uint8_t bus_width;
        uint8_t delay_chain;
        uint32_t data;
        uint32_t dma_base;

        int (*printf)(const char *fmt, ...);
        //1:err;2:warn;3:info;4:debug.
        uint8_t loglevel;

        //act as a patch for spinand rom.
        uint8_t (*phy_readpage)(void *sni, uint32_t colAddr, void *buf, uint32_t len, int addrBits, int mode);
        uint8_t (*phy_writepage)(void *sni, uint32_t rowAddr, const void *buf, uint32_t len, const void *uData, int uDataSize, int mode);
        void (*prepare_hook)(void *sni);
};

struct spinand_info {
        struct nand_info *spi;
        //protect is a lock for write/erase some sectors; protect=0: lock; protect=1: unlock.
        u32_t protect;
        void *id_tbl;
        //bass data for spinand code rom.
        void *bss;
};

#ifndef CONFIG_SPINAND_LIB
struct spinand_operation_api {
        int (*init)(struct spinand_info *sni);
        u32_t (*read_chipid) (struct spinand_info *sni);
        int (*read) (struct spinand_info *sni, u32_t addr, void *buf, int len);
        int (*write) (struct spinand_info *sni, u32_t addr, const void *buf, int len);
        int (*erase) (struct spinand_info *sni, u32_t addr, int len);
        int (*flush)(struct spinand_info *sni);
        int (*flash_protect) (struct spinand_info *sni, uint32_t len);
        int (*null)(struct spinand_info *sni);
};
#else
int spinand_pdl_init(struct spinand_info *sni);
int spinand_init(struct spinand_info *sni);
u32_t spinand_read_chipid(struct spinand_info *sni);
int spinand_read(struct spinand_info *sni, u32_t addr, void *buf, u32_t len);
int spinand_write(struct spinand_info *sni, u32_t addr, const void *buf, u32_t len);
int spinand_erase(struct spinand_info *sni, u32_t addr, u32_t len);
int spinand_flush(struct spinand_info *sni, bool efficient);
int spinand_flash_protect(struct spinand_info *sni, u32_t len);
uint8_t spinand_get_feature(struct spinand_info *sni, u8_t feature);
void spinand_set_feature(struct spinand_info *sni, u8_t feature, u8_t setting);
#endif

struct spinand_acts_config {
    struct acts_spi_controller *spi;
    uint32_t spiclk_reg;
    const char *dma_dev_name;
    uint8_t txdma_id;
    uint8_t txdma_chan;
    uint8_t rxdma_id;
    uint8_t rxdma_chan;
    uint8_t clock_id;
    uint8_t reset_id;
    uint8_t flag_use_dma:1;
    uint8_t delay_chain:7;
};

#define DEV_CFG(dev) \
    ((const struct spinand_acts_config *const)(dev)->config)
#define DEV_DATA(dev) \
    ((struct spinand_info *)(dev)->data)

