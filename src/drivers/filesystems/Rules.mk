# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

DRIVERS_FS_CPPOBJFILES := $(patsubst $(SRC_DIR)/drivers/filesystems/%.cpp,$(BUILD_DIR)/drivers/filesystems/%.o,$(shell find $(SRC_DIR)/drivers/filesystems -maxdepth 1 -type f -name "*.cpp"))
DRIVERS_FS_OBJFILES := $(DRIVERS_FS_CPPOBJFILES)

$(BUILD_DIR)/drivers/filesystems/%.o: $(SRC_DIR)/drivers/filesystems/%.cpp $(HEADER_FILES)
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))
