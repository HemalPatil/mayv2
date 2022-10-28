# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Subdirectories
dir := $(d)/iso9660
include $(dir)/Rules.mk

DRIVERS_FS_CPPOBJFILES := $(patsubst $(SRC_DIR)/drivers/filesystems/%.cpp,$(BUILD_DIR)/drivers/filesystems/%.o,$(shell find $(SRC_DIR)/drivers/filesystems -maxdepth 1 -type f -name "*.cpp"))
DRIVERS_FS_OBJFILES := $(DRIVERS_FS_CPPOBJFILES) $(DRIVERS_FS_ISO_9660_CPPOBJFILES)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))
