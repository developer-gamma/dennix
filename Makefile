# Copyright (c) 2016, 2017, 2018 Dennis Wölfing
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

REPO_ROOT = .

export ARCH ?= i686

include $(REPO_ROOT)/build-aux/paths.mk
include $(REPO_ROOT)/build-aux/toolchain.mk

KERNEL = $(BUILD_DIR)/kernel/kernel
INITRD = $(BUILD_DIR)/initrd.tar
ISO = dennix.iso

all: libc kernel sh utils iso

kernel: $(INCLUDE_DIR) $(LIB_DIR)
	$(MAKE) -C kernel

libc: $(INCLUDE_DIR)
	$(MAKE) -C libc

install-all: install-headers install-libc install-sh install-utils

install-headers:
	$(MAKE) -C kernel install-headers
	$(MAKE) -C libc install-headers

install-libc:
	$(MAKE) -C libc install-libs

install-sh:
	$(MAKE) -C sh install

install-toolchain: install-headers
	SYSROOT=$(SYSROOT) $(REPO_ROOT)/build-aux/install-toolchain.sh

install-utils:
	$(MAKE) -C utils install

iso: $(ISO)

$(ISO): $(KERNEL) $(INITRD)
	rm -rf $(BUILD_DIR)/isosrc
	cp -rf isosrc $(BUILD_DIR)
	cp -f $(KERNEL) $(BUILD_DIR)/isosrc
	cp -f $(INITRD) $(BUILD_DIR)/isosrc
	$(MKRESCUE) -o $@ $(BUILD_DIR)/isosrc

$(KERNEL): $(INCLUDE_DIR)
	$(MAKE) -C kernel

$(INITRD): $(SYSROOT)
	cd $(SYSROOT) && tar cvf ../$(INITRD) --format=ustar *

qemu: $(ISO)
	qemu-system-i386 -cdrom $^

sh: $(INCLUDE_DIR)
	$(MAKE) -C sh

utils: $(INCLUDE_DIR)
	$(MAKE) -C utils

$(SYSROOT): $(INCLUDE_DIR) $(LIB_DIR) $(BIN_DIR) $(SYSROOT)/usr

$(BIN_DIR):
	$(MAKE) -C sh install
	$(MAKE) -C utils install

$(INCLUDE_DIR):
	$(MAKE) -C kernel install-headers
	$(MAKE) -C libc install-headers

$(LIB_DIR):
	$(MAKE) -C libc install-libs

$(SYSROOT)/usr:
	ln -s . $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(ISO)

distclean:
	rm -rf build sysroot
	rm -f *.iso

.PHONY: all kernel libc install-all install-headers install-libc install-sh
.PHONY: install-toolchain install-utils iso qemu sh utils clean distclean
