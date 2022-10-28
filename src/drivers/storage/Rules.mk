# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Subdirectories
dir := $(d)/ahci
include $(dir)/Rules.mk

DRIVERS_STORAGE_CPPOBJFILES := $(patsubst $(SRC_DIR)/drivers/storage/%.cpp,$(BUILD_DIR)/drivers/storage/%.o,$(shell find $(SRC_DIR)/drivers/storage -maxdepth 1 -type f -name "*.cpp"))
DRIVERS_STORAGE_OBJFILES := $(DRIVERS_STORAGE_CPPOBJFILES) $(DRIVERS_STORAGE_AHCI_ASMOBJFILES) $(DRIVERS_STORAGE_AHCI_CPPOBJFILES)

$(BUILD_DIR)/drivers/storage/%.o: $(SRC_DIR)/drivers/storage/%.cpp $(HEADER_FILES)
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))
