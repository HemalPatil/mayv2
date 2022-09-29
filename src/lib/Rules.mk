# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

DIR_LIB := $(patsubst $(SRC_DIR)/lib/%.cpp,$(ISO_DIR)/LIB/%.lib,$(shell find $(SRC_DIR)/lib -maxdepth 1 -type f -name "*.cpp"))

$(ISO_DIR)/LIB/%.lib: $(BUILD_DIR)/lib/%.lib
	cp $< $@

$(BUILD_DIR)/lib/%.lib: $(SRC_DIR)/lib/%.cpp
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))
