# If called directly from the command line, invoke the kernel build system.
ifeq ($(KERNELRELEASE),)
 
    KERNEL_SOURCE := /usr/src/linux
    PWD := $(shell pwd)
default: module
 
module:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(PWD) modules
 
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(PWD) clean
 
# Otherwise KERNELRELEASE is defined; we've been invoked from the
# kernel build system and can use its language.
else
 
    obj-m := ram.o
    ram-y := ram_block.o ram_device.o partition.o
 
endif