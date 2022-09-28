DRIVERS_PS2_ASMOBJFILES := $(patsubst $(SRC_DIR)/drivers/ps2/%.asm,$(BUILD_DIR)/drivers/ps2/%.o,$(shell find $(SRC_DIR)/drivers/ps2 -maxdepth 1 -type f -name "*.asm"))
DRIVERS_PS2_COBJFILES := $(patsubst $(SRC_DIR)/drivers/ps2/%.c,$(BUILD_DIR)/drivers/ps2/%.o,$(shell find $(SRC_DIR)/drivers/ps2 -maxdepth 1 -type f -name "*.c"))

$(BUILD_DIR)/drivers/ps2/%.o: $(SRC_DIR)/drivers/ps2/%.asm
	$(NASM64) $@ $^

$(BUILD_DIR)/drivers/ps2/%.o: $(SRC_DIR)/drivers/ps2/%.c $(HEADER_FILES)
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)
