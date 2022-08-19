# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

INCLUDE_HEADERS := $(shell find $(SRC_DIR)/include -maxdepth 1 -type f -name "*.h")
DIR_INCLUDE := $(patsubst $(SRC_DIR)/include/%,$(ISO_DIR)/INCLUDE/%,$(INCLUDE_HEADERS))

$(ISO_DIR)/INCLUDE/%: $(SRC_DIR)/include/%
	cp $< $@

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))