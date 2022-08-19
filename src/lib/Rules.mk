# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

LIBRARY_INPUT := $(shell find $(SRC_DIR)/lib -maxdepth 1 -type f -name "*.c")
DIR_LIB := $(patsubst $(SRC_DIR)/lib/%.c,$(ISO_DIR)/LIB/%.lib,$(LIBRARY_INPUT))

$(ISO_DIR)/LIB/%.lib: $(SRC_DIR)/lib/%.c
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))