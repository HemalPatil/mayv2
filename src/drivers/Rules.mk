# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Subdirectories
dir := $(d)/storage
include $(dir)/Rules.mk

DRIVERS_OBJFILES := $(DRIVERS_STORAGE_COBJFILES)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))