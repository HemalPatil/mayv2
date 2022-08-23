BOOT_STAGE1_INPUT := $(shell find $(SRC_DIR)/boot/stage1 -type f -name "*.asm")
BOOT_STAGE1_OUTPUT := $(addprefix $(ISO_DIR)/BOOT/STAGE1/,$(shell echo "$(patsubst %.asm,%.bin,$(notdir $(BOOT_STAGE1_INPUT)))" | tr a-z A-Z))

$(ISO_DIR)/BOOT/STAGE1/MMAP.BIN: $(SRC_DIR)/boot/stage1/mmap.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/BOOT/STAGE1/A20.BIN: $(SRC_DIR)/boot/stage1/a20.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/BOOT/STAGE1/VIDMODES.BIN: $(SRC_DIR)/boot/stage1/vidmodes.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/BOOT/STAGE1/GDT.BIN: $(SRC_DIR)/boot/stage1/gdt.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/BOOT/STAGE1/X64.BIN: $(SRC_DIR)/boot/stage1/x64.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/BOOT/STAGE1/BOOTLOAD.BIN: $(SRC_DIR)/boot/stage1/bootload.asm
	$(NASM_BIN) $@ $^