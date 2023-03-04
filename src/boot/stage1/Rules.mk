BOOT_STAGE1_INPUT := $(shell find $(SRC_DIR)/boot/stage1 -type f -name "*.asm")
BOOT_STAGE1_OUTPUT := $(addprefix $(ISO_DIR)/boot/stage1/,$(shell echo "$(patsubst %.asm,%.bin,$(notdir $(BOOT_STAGE1_INPUT)))"))

$(ISO_DIR)/boot/stage1/mmap.bin: $(SRC_DIR)/boot/stage1/mmap.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/boot/stage1/a20.bin: $(SRC_DIR)/boot/stage1/a20.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/boot/stage1/videoModes.bin: $(SRC_DIR)/boot/stage1/videoModes.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/boot/stage1/gdt.bin: $(SRC_DIR)/boot/stage1/gdt.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/boot/stage1/x64.bin: $(SRC_DIR)/boot/stage1/x64.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/boot/stage1/bootload.bin: $(SRC_DIR)/boot/stage1/bootload.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/boot/stage1/apu.bin: $(SRC_DIR)/boot/stage1/apu.asm
	$(NASM_BIN) $@ $^
