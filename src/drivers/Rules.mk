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

DRIVERS_OBJFILES := $(DRIVERS_FS_OBJFILES) $(DRIVERS_PS2_OBJFILES) $(DRIVERS_STORAGE_OBJFILES) $(DRIVERS_TIMERS_OBJFILES)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))
