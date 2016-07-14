SHELL:= /bin/bash

SRCDIR := src
ISODIR := ISO
BUILDDIR := build

SRC_DIRECTORIES:= $(shell find $(SRCDIR) -type d -printf "%d\t%P\n" | sort -nk1 | cut -f2-)
ISO_DIRECTORIES:= $(addprefix $(ISODIR)/,$(shell echo "$(SRC_DIRECTORIES)" | tr a-z A-Z))
BUILD_DIRECTORIES:= $(addprefix $(BUILDDIR)/,$(SRC_DIRECTORIES))

BOOT_STAGE1_INPUT:= $(shell find $(SRCDIR)/boot/stage1 -type f -name "*.asm")

.PHONY: all clean directories

clean:
	rm -rf ISO
	rm -rf build
	mkdir ISO build

directories:
	@echo "Making ISO and build directory structure"
	-@mkdir $(ISO_DIRECTORIES) $(BUILD_DIRECTORIES)

all: mayv2.iso utilities/format_iso

mayv2.iso: $(BOOT_STAGE1_INPUT)

utilities/format_iso: utilities/format_iso.cpp
	c++ $^ -o $@