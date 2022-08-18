# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Subdirectories
dir := $(d)/stage1
include $(dir)/Rules.mk
dir := $(d)/stage2
include $(dir)/Rules.mk

DIR_BOOT := $(BOOT_STAGE1_OUTPUT) $(BOOT_STAGE2_OUTPUT) $(ISODIR)/BOOT/KERNEL

BOOT_ASMOBJFILES := $(patsubst $(SRCDIR)/boot/%.asm,$(BUILDDIR)/boot/%.o,$(shell find $(SRCDIR)/boot -maxdepth 1 -type f -name "*.asm"))
BOOT_COBJFILES := $(patsubst $(SRCDIR)/boot/%.c,$(BUILDDIR)/boot/%.o,$(shell find $(SRCDIR)/boot -maxdepth 1 -type f -name "*.c"))

$(ISODIR)/BOOT/KERNEL: $(BOOT_ASMOBJFILES) $(BOOT_COBJFILES) $(HEADERFILES) $(SRCDIR)/boot/link.ld $(ISODIR)/LIB/libkcrt.lib
	$(LD64) $(LD64FLAGS) -o $@ $(BOOT_ASMOBJFILES) $(BOOT_COBJFILES) $(ISODIR)/LIB/libkcrt.lib -T $(SRCDIR)/boot/link.ld -z max-page-size=0x1000

$(BUILDDIR)/boot/%.o: $(SRCDIR)/boot/%.asm
	$(NASM64) $@ $^

$(BUILDDIR)/boot/%.o: $(SRCDIR)/boot/%.c $(HEADERFILES)
	$(CC64) -o $@ -c $< $(CWARNINGS) $(CC64FLAGS)

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))