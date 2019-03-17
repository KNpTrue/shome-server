
#宏定义
DEFS = -D DEBUG_DEV -D DEBUG_EVENT -D DEBUG_WEB
#位值参数
export CFLAGS += \
		$(DEFS)

all : switch server sensor

switch : mkswitch mkdev-common mkcommon
	$(CROSS_COMPILE)gcc example/libdev-common.o example/switch/libswitch.o src/libcommon.o -o target/$@

sensor : mksensor mkdev-common mkcommon
	$(CROSS_COMPILE)gcc example/libdev-common.o example/sensor/libsensor.o src/libcommon.o -o target/$@

server : mkserver mkcommon mkcJSON
	$(CROSS_COMPILE)gcc src/server/libserver.o src/cJSON/libcjson.o src/libcommon.o -o target/$@ -static

mkserver :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C src/server/

mkcommon :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C src/

mkdev-common :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C example/

mkswitch :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C example/switch

mksensor :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C example/sensor

mkcJSON :
	make CROSS_COMPILE=$(CROSS_COMPILE) -C src/cJSON

.PHONY clean :
clean:
	make -C src/server/ clean
	make -C src/cJSON/ clean
	make -C src/ clean
	make -C example/ clean
	make -C example/switch/ clean
	make -C example/sensor/ clean