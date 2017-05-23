EXTRA_CFLAGS += $(shell rtai-config --module-cflags)

obj-m += rt_controller_k.o

rt_controller_k-objs := controller_k.o

prefix := $(shell rtai-config --prefix)

ifeq ($(prefix),)
$(error Please add <rtai-install>/bin to your PATH variable)
endif

CC = gcc
#$(shell rtai-config --cc)
LXRT_CFLAGS = $(shell rtai-config --lxrt-cflags)
LXRT_LDFLAGS = $(shell rtai-config --lxrt-ldflags)

all: plant reference controller test allarm

plant: plant.c
	$(CC) $(LXRT_CFLAGS) -o $@ $< $(LXRT_LDFLAGS)

test: test.c
	$(CC) $(LXRT_CFLAGS) -o $@ $< $(LXRT_LDFLAGS)

reference: reference.c
	$(CC) $(LXRT_CFLAGS) -o $@ $< $(LXRT_LDFLAGS)

controller: controller.c
	$(CC) $(LXRT_CFLAGS) -o $@ $< $(LXRT_LDFLAGS)

allarm:allarm.c
	$(CC) $(LXRT_CFLAGS) -o $@ $< $(LXRT_LDFLAGS)


clean:
	rm -f *.o *~ plant reference controller test allarm

.PHONY: clean
