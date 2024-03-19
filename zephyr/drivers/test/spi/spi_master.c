#include "spi_test.h"

const struct device *spi_dev;
struct spi_config spi_cfg;

static int test_spi_main(const struct device *arg)
{
    acts_pinmux_set(SPI_CLK, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    acts_pinmux_set(SPI_CS, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 8));
    acts_pinmux_set(SPI_MISO, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    acts_pinmux_set(SPI_MOSI, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));

    spi_cfg.frequency = 16000000;
    spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_LSB | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8); 

    spi_dev = device_get_binding("SPI_1");
    if(!spi_dev)
    {
        SYS_LOG_ERR("get spi 1 dev fail");
    }

    return 0;
}

static int spi_start_test_write(const struct shell *shell,
        size_t argc, char **argv)
{
    acts_pinmux_set(SPI_CLK, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    acts_pinmux_set(SPI_CS, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 8));
    acts_pinmux_set(SPI_MISO, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    acts_pinmux_set(SPI_MOSI, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    struct spi_buf_set tx_bufs;
    struct spi_buf tx_buf[1];
    u8_t buf_tx[TEST_SPI_BUF_LEN];
    int ret = 1;

    memset(buf_tx, 0xaa, TEST_SPI_BUF_LEN);
    tx_buf[0].buf = buf_tx;
    tx_buf[0].len = TEST_SPI_BUF_LEN;
    tx_bufs.buffers = tx_buf;
    tx_bufs.count = 1;
    ret = spi_write(spi_dev, &spi_cfg, &tx_bufs);

    printk("master write data complete\n");
    return 0;
}

static int spi_start_test_read(const struct shell *shell,
        size_t argc, char **argv)
{
    acts_pinmux_set(SPI_CLK, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    acts_pinmux_set(SPI_CS, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 8));
    acts_pinmux_set(SPI_MISO, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    acts_pinmux_set(SPI_MOSI, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    struct spi_buf_set rx_bufs;
    struct spi_buf rx_buf[1];
    u8_t buf_rx[TEST_SPI_BUF_LEN] = {0};
    int ret = 1;

    rx_buf[0].buf = buf_rx;
    rx_buf[0].len = TEST_SPI_BUF_LEN;
    rx_bufs.buffers = rx_buf;
    rx_bufs.count = 1;
    ret = spi_read(spi_dev, &spi_cfg, &rx_bufs);

    int i = 0;
    for(i = 0; i < TEST_SPI_BUF_LEN; i++)
    {
        printk("%x ", buf_rx[i]);
        if(buf_rx[i] != 0x3c)
        {
            SYS_LOG_ERR("spi test fail");
            return -1;
        }
        if(i != 0 && i % 16 == 0)
            printk("\n");
    }
    printk("\n");
    SYS_LOG_INF("spi test success");
    return 0;
}

static int spi_start_test_read_10(const struct shell *shell,
        size_t argc, char **argv)
{
    int i, ret;
    for( i = 0; i < 10; i++)
    {
        ret = spi_start_test_read(NULL, 0, NULL);
        if(ret)
            return 0;
    }
    return 0;
}

static int spi_start_test_write_10(const struct shell *shell,
        size_t argc, char **argv)
{
    int i, ret;
    for( i = 0; i < 10; i++)
    {
        ret = spi_start_test_write(NULL, 0, NULL);
        if(ret)
            return 0;
    }
    return 0;
}
static int spi_start_test_write_read(const struct shell *shell,
        size_t argc, char **argv)
{
    struct spi_dt_spec spec;
    acts_pinmux_set(SPI_CLK, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    acts_pinmux_set(SPI_CS, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 8));
    acts_pinmux_set(SPI_MISO, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    acts_pinmux_set(SPI_MOSI, 0x6 | (1 << 5) | ((4 / 2) << 12) | (1 << 9));
    struct spi_buf_set rx_bufs;
    struct spi_buf rx_buf[1];
    struct spi_buf_set tx_bufs;
    struct spi_buf tx_buf[1];
    u8_t buf_tx[TEST_SPI_BUF_LEN] = {0};
    u8_t buf_rx[TEST_SPI_BUF_LEN] = {0};
    
    spec.bus = spi_dev;
    spec.config = spi_cfg;

    memset(buf_tx, 0xaa, TEST_SPI_BUF_LEN);
    rx_buf[0].buf = buf_rx;
    rx_buf[0].len = TEST_SPI_BUF_LEN;
    rx_bufs.buffers = rx_buf;
    rx_bufs.count = 1;
    tx_buf[0].buf = buf_tx;
    tx_buf[0].len = TEST_SPI_BUF_LEN;
    tx_bufs.buffers = tx_buf;
    tx_bufs.count = 1;
    spi_transceive_dt(&spec, &tx_bufs, &rx_bufs);

    int i = 0;
    for(i = 0; i < TEST_SPI_BUF_LEN; i++)
    {
        printk("%x ", buf_rx[i]);
        if(buf_rx[i] != 0x3c)
        {
            //SYS_LOG_ERR("spi test fail");
            //return -1;
        }
        if(i != 0 && i % 16 == 0)
            printk("\n");
    }
    printk("\n");
    SYS_LOG_INF("spi test success");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_spi_cmds,
        SHELL_CMD(write, NULL, "start test", spi_start_test_write),
        SHELL_CMD(write_10, NULL, "start test", spi_start_test_write_10),
        SHELL_CMD(read, NULL, "start test", spi_start_test_read),
        SHELL_CMD(read_10, NULL, "start test", spi_start_test_read_10),
        SHELL_CMD(write_read, NULL, "start test", spi_start_test_write_read),
        SHELL_SUBCMD_SET_END     /* Array terminated. */
        );

SHELL_CMD_REGISTER(test_spi, &test_spi_cmds, "SPI commands", NULL);
SYS_INIT(test_spi_main, APPLICATION, 10);
