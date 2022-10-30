BOOT_STAGE2_OUTPUT := $(ISO_DIR)/BOOT/STAGE2/LOADER32 $(ISO_DIR)/BOOT/STAGE2/ELFPARSE.BIN $(ISO_DIR)/BOOT/STAGE2/K64LOAD.BIN
BOOT_STAGE2_OBJFILES := $(BUILD_DIR)/boot/stage2/loader32asm.o $(BUILD_DIR)/boot/stage2/loader32.o

$(ISO_DIR)/BOOT/STAGE2/LOADER32: $(BOOT_STAGE2_OBJFILES) $(SRC_DIR)/boot/stage2/link.ld
	$(CC32) -o $@ $(BOOT_STAGE2_OBJFILES) -T $(SRC_DIR)/boot/stage2/link.ld $(C_WARNINGS) $(CC32_FLAGS)

$(ISO_DIR)/BOOT/STAGE2/ELFPARSE.BIN: $(SRC_DIR)/boot/stage2/elfparse.asm
	$(NASM_BIN) $@ $^

$(ISO_DIR)/BOOT/STAGE2/K64LOAD.BIN: $(SRC_DIR)/boot/stage2/k64load.asm
	$(NASM_BIN) $@ $^

$(BUILD_DIR)/boot/stage2/loader32.o: $(SRC_DIR)/boot/stage2/loader32.cpp
	$(CC32) -o $@ -c $^ $(C_WARNINGS) $(CC32_FLAGS)

$(BUILD_DIR)/boot/stage2/loader32asm.o: $(SRC_DIR)/boot/stage2/loader32asm.asm
	$(NASM32) $@ $^
