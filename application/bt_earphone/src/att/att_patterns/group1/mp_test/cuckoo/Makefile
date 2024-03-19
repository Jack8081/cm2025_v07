ATT_ROOT := $(CURDIR)/../../../../
include $(ATT_ROOT)/att_patterns/scripts/config.mk

TARGET := p05_rx

SOURCES := $(wildcard *.c) $(wildcard ../*.c)  $(wildcard ../mp_lib/*.c)


INCLUDES += -I./../../../../att_patterns/include -I./../../../../att_patterns/include/soc-include -I./../../../../att_patterns/include/arm -I./../../../../att_patterns/include/gcc-include
INCLUDES += -I./../../../../att_patterns/include/sdk-include  -I./../../../../
INCLUDES +=   -I../  -I../mp_lib/ -I./../../../../att_patterns/lib/

LIBS += ./../../../../att_patterns/lib/$(IC_TYPE)/libatt.$(LIBSUFFIX)
LIBS += ./../../../../att_patterns/lib/$(IC_TYPE)/socatp/libatpatt.$(LIBSUFFIX)

include $(ATT_ROOT)/att_patterns/scripts/$(IC_TYPE).mk
$(eval $(call MAKE_PROG,$(TARGET),$(SOURCES),$(LDFILE)))

