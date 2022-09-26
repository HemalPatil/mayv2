DRIVERS_STORAGE_ASMOBJFILES := $(patsubst $(SRC_DIR)/drivers/storage/%.asm,$(BUILD_DIR)/drivers/storage/%.o,$(shell find $(SRC_DIR)/drivers/storage -maxdepth 1 -type f -name "*.asm"))
DRIVERS_STORAGE_COBJFILES := $(patsubst $(SRC_DIR)/drivers/storage/%.c,$(BUILD_DIR)/drivers/storage/%.o,$(shell find $(SRC_DIR)/drivers/storage -maxdepth 1 -type f -name "*.c"))

$(BUILD_DIR)/drivers/storage/%.o: $(SRC_DIR)/drivers/storage/%.asm
	$(NASM64) $@ $^

$(BUILD_DIR)/drivers/storage/%.o: $(SRC_DIR)/drivers/storage/%.c $(HEADER_FILES)
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)