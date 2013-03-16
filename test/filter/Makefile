KERNELDIR := /lib/modules/$(shell uname -r)/build

obj-m := filter.o

all:
	$(MAKE) -C $(KERNELDIR) M=`pwd` modules
clean:
	rm -f *.ko *.o *.mod.c *.order *.symvers
