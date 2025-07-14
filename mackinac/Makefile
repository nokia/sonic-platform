KVERSION   ?= $(shell uname -r)
KERNEL_SRC ?=  /lib/modules/$(KVERSION)
PACKAGE_SRC_DIR:= $(shell pwd)
CCAUTOCONF_PROJ_DIR = ccdriver
this_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

.PHONY: fmtlib kmods userspace install

all: kmods userspace


install:
	$(MAKE) modules_install -C $(KERNEL_SRC)/build M=$(PACKAGE_SRC_DIR)/modules INSTALL_MOD_PATH=$(DEST)
	DESTDIR=$(DEST) $(MAKE) -C $(CCAUTOCONF_PROJ_DIR)/$(CONFIGURED_ARCH) install

kmods:
	$(MAKE) modules -C $(KERNEL_SRC)/build M=$(PACKAGE_SRC_DIR)/modules

userspace: fmtlib
	cd $(CCAUTOCONF_PROJ_DIR) && ./autogen.sh && mkdir -p $(CONFIGURED_ARCH) && cd $(CONFIGURED_ARCH) && ../configure PKG_CONFIG_PATH=$(this_dir)/usr/lib/pkgconfig/ && $(MAKE)
	
fmtlib:
	cmake -S fmt -B fmt/$(CONFIGURED_ARCH) --install-prefix $(this_dir)/usr && $(MAKE) -C fmt/$(CONFIGURED_ARCH) install

