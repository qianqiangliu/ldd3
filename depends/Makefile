obj-m +=  module1.o
obj-m +=  module2.o

export KROOT=/lib/modules/$(shell uname -r)/build

modules:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules

modules_install:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules_install

kernel_clean:
	@$(MAKE) -C $(KROOT) M=$(PWD) clean

clean: kernel_clean
	rm -f Module.symvers modules.order
