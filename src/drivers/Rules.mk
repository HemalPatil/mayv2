# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Subdirectories
dir := $(d)/ps2
include $(dir)/Rules.mk
dir := $(d)/storage
include $(dir)/Rules.mk
dir := $(d)/timers
include $(dir)/Rules.mk

#DRIVERS_OBJFILES := $(DRIVERS_PS2_ASMOBJFILES) $(DRIVERS_PS2_COBJFILES) $(DRIVERS_STORAGE_COBJFILES) $(DRIVERS_TIMERS_ASMOBJFILES) $(DRIVERS_TIMERS_COBJFILES)
DRIVERS_OBJFILES := $(DRIVERS_PS2_ASMOBJFILES) $(DRIVERS_PS2_CPPOBJFILES) $(DRIVERS_TIMERS_ASMOBJFILES) $(DRIVERS_TIMERS_CPPOBJFILES)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))
