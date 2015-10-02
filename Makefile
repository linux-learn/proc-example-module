obj-m += demomodule.o

KDIR := /usr/src/linux-$(shell uname -r)
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
