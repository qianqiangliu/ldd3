obj-m := hello.o
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD)

.PHONY: clean
clean:
	rm -f *.mod.c *.o *.ko *.symvers *.order
