# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Subdirectories
dir := $(d)/stage1
include $(dir)/Rules.mk
dir := $(d)/stage2
include $(dir)/Rules.mk

DIR_BOOT := $(BOOT_STAGE1_OUTPUT) $(BOOT_STAGE2_OUTPUT) $(ISO_DIR)/BOOT/KERNEL

BOOT_ASMOBJFILES := $(patsubst $(SRC_DIR)/boot/%.asm,$(BUILD_DIR)/boot/%.o,$(shell find $(SRC_DIR)/boot -maxdepth 1 -type f -name "*.asm"))
BOOT_CPPOBJFILES := $(patsubst $(SRC_DIR)/boot/%.cpp,$(BUILD_DIR)/boot/%.o,$(shell find $(SRC_DIR)/boot -maxdepth 1 -type f -name "*.cpp"))

$(ISO_DIR)/BOOT/KERNEL: $(BOOT_ASMOBJFILES) $(BOOT_CPPOBJFILES) $(DRIVERS_OBJFILES) $(HEADER_FILES) $(SRC_DIR)/boot/link.ld $(BUILD_DIR)/lib/libkcrt.lib
	$(CC64) $(CC64_FLAGS) -o $@ $(BOOT_ASMOBJFILES) $(BOOT_CPPOBJFILES) $(DRIVERS_OBJFILES) $(BUILD_DIR)/lib/libkcrt.lib -T $(SRC_DIR)/boot/link.ld -z max-page-size=0x1000

$(BUILD_DIR)/boot/%.o: $(SRC_DIR)/boot/%.asm
	$(NASM64) $@ $^

$(BUILD_DIR)/boot/%.o: $(SRC_DIR)/boot/%.cpp $(HEADER_FILES)
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))
