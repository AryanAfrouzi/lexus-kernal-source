obj-m := 13cy_triton_pciex.o
13cy_triton_pciex-y := triton_ep.o pciex_manageshm.o 

KERN_DIR:=/lib/modules/$(shell uname -r)/build

PWD:=$(shell pwd)


all : 
# run kernel build system to make module
	$(MAKE) -C $(KERN_DIR) M=$(PWD)

clean:
# run kernel build system to cleanup in current directory
	$(MAKE) -C $(KERN_DIR) M=$(PWD) clean

load:
	insmod ./$(TARGET_MODULE).ko

unload:
	rmmod ./$(TARGET_MODULE).ko
