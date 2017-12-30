obj-m := scullc.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	$(CC) -o user user.c

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -f user
