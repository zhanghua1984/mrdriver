obj-m := mrdev-usbip.o
mrdev-usbip-objs := \
	net/net_main.o \
	net/net_tx.o \
	net/net_rx.o \
	net/svr.o \
	file/logfile.o \
	mr.o
KERNEL_DIR := /usr/src/linux-source-4.13.0/linux-source-4.13.0/

PWD := $(shell pwd)
all:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules ARCH=x86_64
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
