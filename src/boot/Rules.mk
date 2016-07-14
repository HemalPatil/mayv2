# Add elements to directory stack
sp:= $(sp).x
dirstack_$(sp):= $(d)
d:= $(dir)

# Subdirectories
dir:= $(d)/stage1
include $(dir)/Rules.mk
dir:= $(d)/stage2
include $(dir)/Rules.mk

DIR_BOOT:= $(BOOT_STAGE1_OUTPUT) $(BOOT_STAGE2_OUTPUT) $(ISODIR)/BOOT/KERNEL64

$(ISODIR)/BOOT/KERNEL64:

# Remove elements from directory stack
d:= $(dirstack_$(sp))
sp:= $(basename $(sp))