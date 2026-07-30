# Makefile — shadowmask_sus kernel module
#
# Usage (from Ocin4everKernel CI after kernel is built):
#   make -C $KERNEL_OUT \
#        M=$(SM_ROOT)/native/src/kmod \
#        ARCH=arm64 \
#        CROSS_COMPILE=aarch64-linux-android- \
#        modules
#
# The resulting shadowmask_sus.ko is collected by build.py (B_KMOD=1)
# and embedded into ShadowMask's assets alongside other native binaries.

obj-m += shadowmask_sus.o

shadowmask_sus-objs := \
    shadowmask_sus.o \
    syscall_hook.o   \
    proc_filter.o

ccflags-y := \
    -O2 \
    -fstack-protector-strong \
    -DMODULE \
    -Wno-unused-function \
    -Wno-declaration-after-statement

# Allow the build system to pass KERNEL_SRC explicitly
ifdef KERNEL_SRC
KDIR := $(KERNEL_SRC)
else
KDIR := /lib/modules/$(shell uname -r)/build
endif

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
