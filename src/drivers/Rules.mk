# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Subdirectories
dir := $(d)/filesystems
include $(dir)/Rules.mk
dir := $(d)/ps2
include $(dir)/Rules.mk
dir := $(d)/storage
include $(dir)/Rules.mk
dir := $(d)/timers
include $(dir)/Rules.mk

DRIVERS_CPPOBJFILES := $(patsubst $(SRC_DIR)/drivers/%.cpp,$(BUILD_DIR)/drivers/%.o,$(shell find $(SRC_DIR)/drivers -maxdepth 1 -type f -name "*.cpp"))
DRIVERS_OBJFILES := $(DRIVERS_CPPOBJFILES) $(DRIVERS_FS_OBJFILES) $(DRIVERS_PS2_OBJFILES) $(DRIVERS_STORAGE_OBJFILES) $(DRIVERS_TIMERS_OBJFILES)

$(BUILD_DIR)/drivers/%.o: $(SRC_DIR)/drivers/%.cpp $(HEADER_FILES)
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))
