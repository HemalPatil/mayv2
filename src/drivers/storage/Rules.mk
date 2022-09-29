# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Subdirectories
dir := $(d)/ahci
include $(dir)/Rules.mk

DRIVERS_STORAGE_OBJFILES := $(DRIVERS_STORAGE_AHCI_ASMOBJFILES) $(DRIVERS_STORAGE_AHCI_CPPOBJFILES)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))
