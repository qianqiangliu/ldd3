obj-m := virnet.o
KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

clean:
	rm -f *.o *.ko *.mod.c Module.symvers modules.order

.PHONY:clean
