#ifndef __ATT_PATTERN_TEST_H
#define __ATT_PATTERN_TEST_H


#include <soc.h>
#include <sdk_include.h>

//att include
#include <att_debug.h>
#include <att_interface.h>
#include <ap_autotest_cmd.h>
#include "att_linker_conf.h"
#include "att_pattern_header.h"


#define LINESEP_FORMAT_WINDOWS  0
#define LINESEP_FORMAT_UNIX     1

#define DEBUG_BUF_SIZE          128

extern void clk_enable(int clk_id);
extern void reset_and_enable(int rst_id);

extern int att_printf(const char *fmt, ...);

#endif