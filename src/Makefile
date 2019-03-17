#common

CFILES = $(wildcard *.c) 
COBJS = $(patsubst %.c, %.o, $(CFILES))

OBJS += $(COBJS)

libcommon.o : $(OBJS)
	$(CROSS_COMPILE)ld -r $^ -o $@

%.o : %.c
	$(CROSS_COMPILE)gcc $< -c -o $@ $(CFLAGS)

.PHONY : clean
clean : 
	rm *.o -f
