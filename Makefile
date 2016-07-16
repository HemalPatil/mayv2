# We'll be using the bash shell
SHELL:= /bin/bash

# Different flags for nasm depending on the type of output required 
NASMBIN:= nasm -f bin -o
NASM32:= nasm -f elf32 -o

# The utility that we will be using for creating the ISO image from the actual folder
ISOMAKER:= genisoimage

# Necessary flags and compiler and linker names required for generating binaries for x86
LD32:=i686-elf-ld
CC32:=i686-elf-gcc
WARNINGS:=-Wall -Wextra

# root directories
SRCDIR := src
ISODIR := ISO
BUILDDIR := build

# The directory structure in the above root directories
SRC_DIRECTORIES:= $(shell find $(SRCDIR) -type d -printf "%d\t%P\n" | sort -nk1 | cut -f2-)
ISO_DIRECTORIES:= $(addprefix $(ISODIR)/,$(shell echo "$(SRC_DIRECTORIES)" | tr a-z A-Z))
BUILD_DIRECTORIES:= $(addprefix $(BUILDDIR)/,$(SRC_DIRECTORIES))

.PHONY: all clean directories

# Include the rules from subdirectories recursively using stack-like structure (See implementing non-recursive make article https://evbergen.home.xs4all.nl/nonrecursive-make.html)
dir:=$(SRCDIR)/boot
include $(dir)/Rules.mk

# Remove all contents of the ISO and build directories
clean:
	rm -rf ISO
	rm -rf build
	mkdir ISO build

# Create directory structure after cleaning the directory 
directories:
	@echo "Making ISO and build directory structure"
	-@mkdir $(ISO_DIRECTORIES) $(BUILD_DIRECTORIES)

all: mayv2.iso

mayv2.iso: utilities/format_iso $(DIR_BOOT)
	@$(ISOMAKER) -no-emul-boot -boot-load-size 4 -b BOOT/STAGE1/BOOTLOAD.BIN -o $@ ./$(ISODIR)/
	./utilities/format_iso $@

utilities/format_iso: utilities/format_iso.cpp
	$(CXX) $^ -o $@