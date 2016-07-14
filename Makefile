SHELL:= /bin/bash

NASMBIN:= nasm -f bin -o
NASM32:= nasm -f elf32 -o

ISOMAKER:= genisoimage

LD32:=i686-elf-ld
CC32:=i686-elf-gcc
WARNINGS:=-Wall -Wextra

SRCDIR := src
ISODIR := ISO
BUILDDIR := build

SRC_DIRECTORIES:= $(shell find $(SRCDIR) -type d -printf "%d\t%P\n" | sort -nk1 | cut -f2-)
ISO_DIRECTORIES:= $(addprefix $(ISODIR)/,$(shell echo "$(SRC_DIRECTORIES)" | tr a-z A-Z))
BUILD_DIRECTORIES:= $(addprefix $(BUILDDIR)/,$(SRC_DIRECTORIES))

BOOT_FILES:= $(BOOT_STAGE1_OUTPUT) $(BOOT_STAGE2_OUTPUT) $(ISODIR)/BOOT/KERNEL64

.PHONY: all clean directories

include $(SRCDIR)/boot/stage1/Rules.mk
include $(SRCDIR)/boot/stage2/Rules.mk

clean:
	rm -rf ISO
	rm -rf build
	mkdir ISO build

directories:
	@echo "Making ISO and build directory structure"
	-@mkdir $(ISO_DIRECTORIES) $(BUILD_DIRECTORIES)

all: mayv2.iso

mayv2.iso: utilities/format_iso $(BOOT_FILES)
	$(ISOMAKER) -no-emul-boot -boot-load-size 4 -b BOOT/STAGE1/BOOTLOAD.BIN -o $@ ./$(ISODIR)/
	./utilities/format_iso $@

utilities/format_iso: utilities/format_iso.cpp
	$(CXX) $^ -o $@

$(ISODIR)/BOOT/KERNEL64: