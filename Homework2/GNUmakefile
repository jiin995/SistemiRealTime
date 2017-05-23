prefix := $(shell rtai-config --prefix)

ifeq ($(prefix),)
$(error Please add <rtai-install>/bin to your PATH variable)
endif

CC = $(shell rtai-config --cc)

LXRT_CFLAGS = $(shell rtai-config --lxrt-cflags)
LXRT_LDFLAGS = $(shell rtai-config --lxrt-ldflags)
LINUX_DIR = $(shell rtai-config --linux-dir)


ifneq ($(findstring 3.14.,$(shell rtai-config --linux-version 2>/dev/null)),)

all:plant test controller reference allarm
	@if test -e $(LINUX_DIR)/Module.symvers; then mv -f $(LINUX_DIR)/Module.symvers $(LINUX_DIR)/__Module.symvers; fi; if test -e $(LINUX_DIR)/vmlinux; then mv -f $(LINUX_DIR)/vmlinux $(LINUX_DIR)/__vmlinux; fi; \
	$(MAKE) -C $(LINUX_DIR) CC=$(CC) SUBDIRS=$$PWD V=$(V) modules \
	&& if test -e $(LINUX_DIR)/__Module.symvers; then mv -f $(LINUX_DIR)/__Module.symvers $(LINUX_DIR)/Module.symvers; fi && if test -e $(LINUX_DIR)/__vmlinux; then mv -f $(LINUX_DIR)/__vmlinux $(LINUX_DIR)/vmlinux; fi

clean:
	$(RM) $(LINUX_DIR)/.tmp_versions/*_rt.mod *.o *.ko *.mod.c .*.cmd
	rm -f *.o
	rm -f *.o *~ plant reference controller test allarm

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



else

MODULE_CFLAGS = $(shell rtai-config --module-cflags)

all: controller_k.o plant reference controller test allarm

controller_k.o: controller_k.c
	$(CC) $(MODULE_CFLAGS) -c $<

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
	rm -f *.o
	rm -f *.o *~ plant reference controller test allarm


endif

.PHONY: clean
