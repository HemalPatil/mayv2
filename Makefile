# Use the bash shell
SHELL := /bin/bash

# Root directories
SRC_DIR := src
INCLUDE_DIR := $(SRC_DIR)/include
KERNEL_INCLUDE_DIR := $(INCLUDE_DIR)/kernel
STD_INCLUDE_DIR := $(INCLUDE_DIR)/std
ISO_DIR := ISO
BUILD_DIR := build
UTILITIES_BUILD_DIR := build_utilities
UTILITIES_DIR := utilities

# Different flags for nasm depending on the type of output required
NASM_BIN := nasm -f bin -o
NASM32 := nasm -f elf32 -o
NASM64 := nasm -f elf64 -o

# Utility to create the ISO image
ISO_MAKER := mkisofs

# Necessary flags and compiler and linker names required for generating binaries for x86
CC32 := i686-elf-g++
CC32_FLAGS := -ffreestanding -nostdlib -lgcc -I$(STD_INCLUDE_DIR) -I$(KERNEL_INCLUDE_DIR)
C_WARNINGS := -Wall -Wextra

# Necessary flags and compiler and linker names required for generating binaries for x64
CC64 := x86_64-elf-g++
CC64_FLAGS := --std=c++23 -O3 -ffreestanding -fno-exceptions -fno-rtti -mcmodel=kernel -m64 -march=x86-64 -mno-red-zone -msse4.2 -nostdlib -lgcc -I$(STD_INCLUDE_DIR) -I$(KERNEL_INCLUDE_DIR)

# The directory structure in the above root directories
SRC_DIRECTORIES := $(shell find $(SRC_DIR) -type d -printf "%d\t%P\n" | sort -nk1 | cut -f2-)
ISO_DIRECTORIES := $(ISO_DIR)/boot $(ISO_DIR)/boot/stage1 $(ISO_DIR)/boot/stage2
BUILD_DIRECTORIES := $(addprefix $(BUILD_DIR)/,$(SRC_DIRECTORIES))

# Source files
C_FILES := $(shell find $(addprefix $(SRC_DIR)/,$(SRC_DIRECTORIES)) $(UTILITIES_DIR) -type f -name "*.c")
CPP_FILES := $(shell find $(addprefix $(SRC_DIR)/,$(SRC_DIRECTORIES)) $(UTILITIES_DIR) -type f -name "*.cpp")
HEADER_FILES := $(shell find $(KERNEL_INCLUDE_DIR) $(STD_INCLUDE_DIR) -type f)
ASM_FILES := $(shell find $(addprefix $(SRC_DIR)/,$(SRC_DIRECTORIES)) $(UTILITIES_DIR) -type f -name "*.asm")
ALL_SRC_FILES := $(C_FILES) $(CPP_FILES) $(HEADER_FILES) $(ASM_FILES)

# Final ISO name
ISO_NAME := mayv2.iso

# Root FS GUID
EXISTING_ROOT_GUIDS := $(shell find $(ISO_DIR)/boot -type f -name "*.root-fs")
ROOT_GUID := $(shell uuidgen -r)

.PHONY: all clean directories disassemble fixme todo

# Include the rules from subdirectories recursively using stack-like structure
# (See implementing non-recursive make article https://accu.org/journals/overload/14/71/miller_2004/)
dir := $(SRC_DIR)/lib
include $(dir)/Rules.mk
dir := $(SRC_DIR)/drivers
include $(dir)/Rules.mk
dir := $(SRC_DIR)/boot
include $(dir)/Rules.mk

GREP := grep --color='auto' -rni $(SRC_DIR) $(UTILITIES_DIR) -e

# Disassemble kernel
disassemble:
	x86_64-elf-objdump -S --disassemble -M intel -M intel-mnemonic ISO/boot/kernel > k.asm

# Show all TODOs in all source files
todo:
	@$(GREP) 'TODO'

# Show all TODOs in all source files
fixme:
	@$(GREP) 'FIXME'

# Remove all contents of the ISO and build directories
clean:
	@echo "Cleaning working directory"
	@rm -rf $(ISO_DIR)
	@rm -rf $(BUILD_DIR)
	@rm -rf $(UTILITIES_BUILD_DIR)
	@rm -f $(ISO_NAME)
	@echo "Done"

# Create directory structure after cleaning the directory 
directories:
	@echo "Creating ISO and build directory structure"
	@mkdir $(ISO_DIR) $(BUILD_DIR) $(UTILITIES_BUILD_DIR)
	@mkdir $(ISO_DIRECTORIES) $(BUILD_DIRECTORIES)
	@echo "Done"

# Generate the ISO containing the OS and all required files
all: configure $(ISO_NAME)

# Boot load size is set to 4 because BOOTLOAD.BIN is 2048 bytes
# and El-Torito standard must load 4 512 byte sectors
$(ISO_NAME): $(DIR_BOOT) $(UTILITIES_BUILD_DIR)/format_iso
	if [ ! -z "$(EXISTING_ROOT_GUIDS)" ]; then rm $(EXISTING_ROOT_GUIDS); fi
	touch ./$(ISO_DIR)/boot/$(ROOT_GUID).root-fs
	$(ISO_MAKER) -J -quiet -no-emul-boot -boot-load-size 4 -b boot/stage1/bootload.bin -o $@ ./$(ISO_DIR)/
	./$(UTILITIES_BUILD_DIR)/format_iso $@ $(ROOT_GUID)

$(UTILITIES_BUILD_DIR)/format_iso: $(UTILITIES_DIR)/format_iso.cpp
	$(CXX) --std=c++23 -O3 $^ -o $@ $(C_WARNINGS)
