# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Subdirectories
dir := $(d)/ahci
include $(dir)/Rules.mk

DRIVERS_STORAGE_OBJFILES := $(BUILD_DIR)/drivers/storage/ahci.o $(DRIVERS_STORAGE_AHCI_ASMOBJFILES) $(DRIVERS_STORAGE_AHCI_CPPOBJFILES)

$(BUILD_DIR)/drivers/storage/ahci.o: $(SRC_DIR)/drivers/storage/ahci.cpp $(HEADER_FILES)
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))
