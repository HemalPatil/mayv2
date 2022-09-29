DRIVERS_STORAGE_AHCI_ASMOBJFILES := $(patsubst $(SRC_DIR)/drivers/storage/ahci/%.asm,$(BUILD_DIR)/drivers/storage/ahci/%.o,$(shell find $(SRC_DIR)/drivers/storage/ahci -maxdepth 1 -type f -name "*.asm"))
DRIVERS_STORAGE_AHCI_CPPOBJFILES := $(patsubst $(SRC_DIR)/drivers/storage/ahci/%.cpp,$(BUILD_DIR)/drivers/storage/ahci/%.o,$(shell find $(SRC_DIR)/drivers/storage/ahci -maxdepth 1 -type f -name "*.cpp"))

$(BUILD_DIR)/drivers/storage/ahci/%.o: $(SRC_DIR)/drivers/storage/ahci/%.asm
	$(NASM64) $@ $^

$(BUILD_DIR)/drivers/storage/ahci/%.o: $(SRC_DIR)/drivers/storage/ahci/%.cpp $(HEADER_FILES)
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)
