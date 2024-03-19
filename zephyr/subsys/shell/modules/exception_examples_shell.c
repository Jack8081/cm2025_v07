/*
 * Copyright (c) 2021 Actions  Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <init.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <device.h>
#include <sys/arch_interface.h>
#include <soc.h>
static bool IllegalWrite_examples(const struct shell *shell, int level)
{
    int r;
    volatile unsigned int* p;

    r = 0;

    sys_write32(sys_read32(MEMORYCTL) | 0x3, MEMORYCTL);

    //0x00100000 is reserved mem region for lark
    p = (unsigned int*)0x00100000;
    *p = 0x00BADA55;

    return r;
}

static bool Illegalread_examples(const struct shell *shell, int level)
{
    int r;
    volatile unsigned int* p;

    r = 0;

    sys_write32(sys_read32(MEMORYCTL) | 0x3, MEMORYCTL);

    //0x00100000 is reserved mem region for lark
    p = (unsigned int*)0x00100000;
    r = *p;

    return r;
}

static bool Illegalfunc_examples(const struct shell *shell, int level)
{
    int r;
    int (*pF)(void);

    r = 0;

    sys_write32(sys_read32(MEMORYCTL) | 0x3, MEMORYCTL);

    //0x00100000 is reserved mem region for lark
    pF = (int(*)(void))0x00100001;
    r = pF();

    return r;
}

static bool Illegalinstruction_examples(const struct shell *shell, int level)
{
    static const unsigned short _UDF[4] = {0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD}; // 0xDEAD: UDF #<imm> (permanently undefined)
    int r;
    int (*pF)(void);

    sys_write32(sys_read32(MEMORYCTL) | 0x3, MEMORYCTL);

    pF = (int(*)(void))(((char*)&_UDF) + 1);
    r = pF();
    return r;
}

static bool Illegalstate_examples(const struct shell *shell, int level)
{
    int r;
    int (*pF)(void);

    r = 0;

    sys_write32(sys_read32(MEMORYCTL) | 0x3, MEMORYCTL);

    pF = (int(*)(void))0x10000000;
    r = pF();

    return r;
}

static bool dividezero_examples(const struct shell *shell, int level)
{
    volatile int r;
    volatile unsigned int a;
    volatile unsigned int b;
    a = 0x3456;

    //0x10 data is zero
    b = *(int *)0x10;

    r = a / b;

    printk("div %d\n", r);

    return r;
}

static bool UnalignedAccess_examples(const struct shell *shell, int level)
{
    volatile unsigned int* p;

    p = (unsigned int*)0x18000001;

    *p = 0x12345678;

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_device,
    SHELL_CMD(write_error, NULL, "illegal write example", IllegalWrite_examples),
    SHELL_CMD(read_error, NULL, "illegal read example", Illegalread_examples),
    SHELL_CMD(func_error, NULL, "illegal func example", Illegalfunc_examples),
    SHELL_CMD(instruction_error, NULL, "illegal instruction example", Illegalinstruction_examples),
    SHELL_CMD(state_error, NULL, "illegal state example", Illegalstate_examples),
    SHELL_CMD(dividezero_error, NULL, "divide zero example", dividezero_examples),
    SHELL_CMD(unaligned_error, NULL, "unaligned access example", UnalignedAccess_examples),
    SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(exception, &sub_device, "exception examples", NULL);
