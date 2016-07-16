# Add elements to directory stack
sp:= $(sp).x
dirstack_$(sp):= $(d)
d:= $(dir)

# Subdirectories
dir:= $(d)/stage1
include $(dir)/Rules.mk
dir:= $(d)/stage2
include $(dir)/Rules.mk

DIR_BOOT:= $(BOOT_STAGE1_OUTPUT) $(BOOT_STAGE2_OUTPUT) $(ISODIR)/BOOT/KERNEL

BOOT_OBJFILES:= $(patsubst $(SRCDIR)/boot/%.asm,$(BUILDDIR)/boot/%.o,$(shell find $(SRCDIR)/boot -maxdepth 1 -type f -name "*.asm"))
BOOT_OBJFILES:= $(BOOT_OBJFILES) $(patsubst $(SRCDIR)/boot/%.c,$(BUILDDIR)/boot/%.o,$(shell find $(SRCDIR)/boot -maxdepth 1 -type f -name "*.c"))

$(ISODIR)/BOOT/KERNEL: $(BOOT_OBJFILES) $(SRCDIR)/boot/link.ld
	$(LD64) -o $@ $(BOOT_OBJFILES) -T $(SRCDIR)/boot/link.ld

$(BUILDDIR)/boot/kernelasm.o: $(SRCDIR)/boot/kernelasm.asm
	$(NASM64) $@ $^

$(BUILDDIR)/boot/%.o: $(SRCDIR)/boot/%.c
	$(CC64) -o $@ -c $^ $(CWARNINGS) -ffreestanding

# Remove elements from directory stack
d:= $(dirstack_$(sp))
sp:= $(basename $(sp))