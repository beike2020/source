KERNELDIR ?= /lib/modules/$(shell uname -r)/build
INCLUDEDIR = $(KERNELDIR)/include
CFLAGS = -O2 -fomit-frame-pointer -Wall -I$(INCLUDEDIR)

all: kernel_app jiffies_api wait_api

clean:
	rm -f $(FILES) *~ core

