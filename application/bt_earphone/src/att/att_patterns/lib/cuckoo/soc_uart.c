/**
 *  ***************************************************************************
 *  Copyright (c) 2014-2020 Actions (Zhuhai) Technology. All rights reserved.
 *
 *  \file       soc_uart.c
 *  \brief      uart operations
 *  \author     songzhining
 *  \version    1.00
 *  \date       2020/5/9
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "sys_io.h"
#include "types.h"
#include "att_pattern_test.h"


#define CONFIG_HOSC_CLK_MHZ 32

#define     GPIO_MFP_BASE                                                     0x40068000
#define     IO0_CTL                                                           (GPIO_MFP_BASE+0x04)

#define CLOCK_UART0              9
#define RESET_UART0              9

#define     UART0_BASE                                                        0x40038000
//#define     UART0_BR                                                          (UART0_BASE+0x0010)

#define GPIOX_CTL(n)            (IO0_CTL + (n) * 4)

#define OWL_MAX_UART            (7)
#define UART_REG_CONFIG_NUM     (3)

/* UART registers */
#define UART_CTL                (0x0000)
#define UART_RXDAT              (0x0004)
#define UART_TXDAT              (0x0008)
#define UART_STAT               (0x000C)

#define UART_STAT_UTBB          (0x1 << 21)
#define UART_STAT_TFES          (0x1 << 10)
#define UART_STAT_TFFU          (0x1 << 6)
#define UART_STAT_RFEM          (0x1 << 5)

struct uart_reg_config {
    unsigned int reg;
    unsigned int mask;
    unsigned int val;
};

struct owl_uart_dev {
    unsigned char chan;
    unsigned char mfp;
    unsigned char clk_id;
    unsigned char rst_id;
    unsigned long base;
    unsigned long uart_clk;
    struct uart_reg_config reg_cfg[UART_REG_CONFIG_NUM];
};

#define UART_BAUD_DIV(freq, baud_rate)  (((freq) / (baud_rate))) + ((((freq) % (baud_rate)) + ((baud_rate) >> 1)) / baud_rate)

static const struct owl_uart_dev owl_uart_ports[] = {
    /*uart0*/
    {0, 0, CLOCK_UART0, RESET_UART0, UART0_BASE, UART0_BR,
        {{GPIOX_CTL(1), 0xFFFF, 0x3022}, {GPIOX_CTL(2), 0xFFFF, 0x3122},}},
    {0, 1, CLOCK_UART0, RESET_UART0, UART0_BASE, UART0_BR,
        {{GPIOX_CTL(18), 0xFFFF, 0x3022},{GPIOX_CTL(19), 0xFFFF, 0x3122},}},
    {0, 2, CLOCK_UART0, RESET_UART0, UART0_BASE, UART0_BR,
        {{GPIOX_CTL(21), 0xFFFF, 0x3022},{GPIOX_CTL(22), 0xFFFF, 0x3122},}},
    {0, 3, CLOCK_UART0, RESET_UART0, UART0_BASE, UART0_BR,
        {{GPIOX_CTL(5), 0xFFFF, 0x3022},{GPIOX_CTL(6), 0xFFFF, 0x3122},}},        
};

static const struct owl_uart_dev *g_uart_dev;

static const struct owl_uart_dev *match_uart_port(int id, int mfp)
{
    const struct owl_uart_dev *dev;
    int i, num;

    num = sizeof(owl_uart_ports) / sizeof(owl_uart_ports[0]);

    for (i = 0; i < num; i++) {
        dev = &owl_uart_ports[i];
        if (dev->chan == id && dev->mfp == mfp) {
            /* found */
            return dev;
        }
    }
    return NULL;
}

static int init_uart_port(const struct owl_uart_dev *dev, unsigned int baud)
{
    const struct uart_reg_config *cfg;
    int i;
    unsigned int div;

    /* init mfp config */
    for (i = 0; i < UART_REG_CONFIG_NUM; i++) {
        cfg = &dev->reg_cfg[i];

        if (cfg->mask != 0)
            sys_clrsetbits(cfg->reg, cfg->mask, cfg->val);
    }

    sys_writel(0xf0000003, dev->base + UART_CTL);
    div = UART_BAUD_DIV(CONFIG_HOSC_CLK_MHZ * 1000000, baud);;
    sys_writel(div | div << 16, dev->uart_clk);
    sys_writel(0xf0008003, dev->base + UART_CTL);

    return 0;
}

void uart_init(int id, int mfp, int baud)
{
    const struct owl_uart_dev *dev;

    dev = match_uart_port(id, mfp);
    if (!dev) {
        return;
    }
    clk_enable(dev->clk_id);
    reset_and_enable(dev->rst_id);
    init_uart_port(dev, baud);
    g_uart_dev = dev;
}

void uart_puts(char *s, unsigned int len)
{
    const struct owl_uart_dev *dev;

    if (g_uart_dev == NULL) {
		uart_init(0, 3, 2000000);    //GPIO5/6
    }
	
    dev = g_uart_dev;
	
    while (*s != 0 && len != 0) {
        /* Wait for fifo to flush out */
        while (sys_readl(dev->base + UART_STAT) & UART_STAT_UTBB);

        sys_writel(*s, dev->base + UART_TXDAT);
        s++;
        len--;
    }
}

