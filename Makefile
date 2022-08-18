# We'll be using the bash shell
SHELL := /bin/bash

# Root directories
PROJECT := /home/hemal/mayv2
SRCDIR := src
INCLUDEDIR := $(SRCDIR)/include
ISODIR := ISO
BUILDDIR := build
UTILITIESDIR := utilities

# Different flags for nasm depending on the type of output required 
NASMBIN := nasm -f bin -o
NASM32 := nasm -f elf32 -o
NASM64 := nasm -f elf64 -o

# The utility that we will be using for creating the ISO image from the actual folder
ISOMAKER := genisoimage

# Necessary flags and compiler and linker names required for generating binaries for x86
LD32 := i686-elf-ld
CC32 := i686-elf-gcc
CC32FLAGS := -ffreestanding -I$(INCLUDEDIR)
CWARNINGS := -Wall -Wextra

# Necessary flags and compiler and linker names required for generating binaries for x64
LD64 := x86_64-elf-ld
LD64FLAGS := -b elf64-x86-64
CC64 := x86_64-elf-gcc
CC64FLAGS := -ffreestanding -mcmodel=kernel -m64 -march=x86-64 -mno-red-zone -I$(INCLUDEDIR)

# The directory structure in the above root directories
SRC_DIRECTORIES := $(shell find $(SRCDIR) -type d -printf "%d\t%P\n" | sort -nk1 | cut -f2-)
ISO_DIRECTORIES := $(addprefix $(ISODIR)/,$(shell echo "$(SRC_DIRECTORIES)" | tr a-z A-Z))
BUILD_DIRECTORIES := $(addprefix $(BUILDDIR)/,$(SRC_DIRECTORIES))

# Source files
CFILES := $(shell find $(addprefix $(SRCDIR)/,$(SRC_DIRECTORIES)) $(UTILITIESDIR) -type f -name "*.c")
CPPFILES := $(shell find $(addprefix $(SRCDIR)/,$(SRC_DIRECTORIES)) $(UTILITIESDIR) -type f -name "*.cpp")
HEADERFILES := $(shell find $(addprefix $(SRCDIR)/,$(SRC_DIRECTORIES)) $(UTILITIESDIR) -type f -name "*.h")
ASMFILES := $(shell find $(addprefix $(SRCDIR)/,$(SRC_DIRECTORIES)) $(UTILITIESDIR) -type f -name "*.asm")
ALLSRCFILES := $(CFILES) $(CPPFILES) $(HEADERFILES) $(ASMFILES)

.PHONY: all clean directories todolist

# Include the rules from subdirectories recursively using stack-like structure
# (See implementing non-recursive make article https://evbergen.home.xs4all.nl/nonrecursive-make.html)
dir := $(SRCDIR)/include
include $(dir)/Rules.mk
dir := $(SRCDIR)/lib
include $(dir)/Rules.mk
dir := $(SRCDIR)/boot
include $(dir)/Rules.mk

# Show all TODOs in all source files
todolist:
	-@for file in $(ALLSRCFILES); do fgrep -Hn -e TODO -e FIXME $$file; done; true

# Remove all contents of the ISO and build directories
clean:
	@echo "Cleaning working directory"
	-@rm -rf ISO
	-@rm -rf build
	-@rm -f mayv2.iso

# Create directory structure after cleaning the directory 
directories:
	@echo "Making ISO and build directory structure"
	@mkdir ISO build
	@mkdir $(ISO_DIRECTORIES) $(BUILD_DIRECTORIES)

# Generate the ISO containing the OS and all required files
all: mayv2.iso

mayv2.iso: $(UTILITIESDIR)/format_iso $(DIR_INCLUDE) $(DIR_LIB) $(DIR_BOOT)
	$(ISOMAKER) -quiet -no-emul-boot -boot-load-size 4 -b BOOT/STAGE1/BOOTLOAD.BIN -o $@ ./$(ISODIR)/
	./$(UTILITIESDIR)/format_iso $@

utilities/format_iso: utilities/format_iso.cpp
	$(CXX) $^ -o $@
