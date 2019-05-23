
#宏定义
DEFS = -D DEBUG_DEV -D DEBUG_EVENT -D DEBUG_WEB -D DEBUG -D SHOME_SERVER

TOPDIR = $(shell pwd)
#include
INCS += -I$(TOPDIR)/include
INCS += -I$(TOPDIR)/third_party/include

#位值参数
export CFLAGS += \
		$(DEFS)\
		$(INCS)\
		-std=c99\

all : switch server sensor zigbee-gateway

switch : mkswitch mkdev-common mkcommon
	$(CROSS_COMPILE)gcc example/libdev-common.o example/switch/libswitch.o src/libcommon.o -o target/$@

sensor : mksensor mkdev-common mkcommon
	$(CROSS_COMPILE)gcc example/libdev-common.o example/sensor/libsensor.o src/libcommon.o -o target/$@

zigbee-gateway : mkzigebee-gateway mkdev-common mkevent
	$(CROSS_COMPILE)gcc example/libdev-common.o example/zigbee-gateway/libzigbee-gateway.o \
						src/event/libevent.o src/libcommon.o -o target/$@

server : mkserver mkcommon mkcJSON mkevent mksha1
	$(CROSS_COMPILE)gcc src/server/libserver.o third_party/cJSON/libcjson.o third_party/sha1/libsha1.o \
						src/event/libevent.o src/libcommon.o -o target/$@ -static

mkserver :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C src/server/

mkevent :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C src/event/

mkcommon :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C src/

mkdev-common :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C example/

mkswitch :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C example/switch

mksensor :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C example/sensor

mkzigebee-gateway :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C example/zigbee-gateway

mkcJSON :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C third_party/cJSON

mksha1 :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C third_party/sha1

.PHONY clean :
clean:
	make -C src/server/ clean
	make -C src/event/ clean
	make -C src/ clean
	make -C example/ clean
	make -C example/switch/ clean
	make -C example/sensor/ clean
	make -C example/zigbee-gateway clean
	make -C third_party/cJSON clean
	make -C third_party/sha1 clean
	make -C third_party/sqlite3 clean