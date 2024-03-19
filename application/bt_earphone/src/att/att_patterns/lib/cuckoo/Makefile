ATT_ROOT := $(CURDIR)/../../../
include $(ATT_ROOT)/att_patterns/scripts/config.mk

IC_TYPE ?= cuckoo
ARCH ?= arm

LIB = libatt.$(LIBSUFFIX)

SOURCES	:= $(wildcard ../*.c) $(wildcard ..//rbuf/*.c) $(wildcard ..//compensation/*.c)
SOURCES	+= $(wildcard *.c) $(wildcard ./mp_btc/*.c) 

INCLUDES += -I./../../../att_patterns/include -I./../../../att_patterns/include/soc-include -I./../../../att_patterns/include/arm -I./../../../att_patterns/include/gcc-include
INCLUDES += -I./../../../att_patterns/include/sdk-include  -I./../../../ -I../

include $(ATT_ROOT)/att_patterns/scripts/$(IC_TYPE).mk
$(eval $(call MAKE_LIB,$(LIB),$(SOURCES)))
