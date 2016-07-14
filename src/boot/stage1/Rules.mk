BOOT_STAGE1_INPUT:= $(shell find $(SRCDIR)/boot/stage1 -type f -name "*.asm")
BOOT_STAGE1_OUTPUT:= $(addprefix $(ISODIR)/BOOT/STAGE1/,$(shell echo "$(patsubst %.asm,%.bin,$(notdir $(BOOT_STAGE1_INPUT)))" | tr a-z A-Z))

$(ISODIR)/BOOT/STAGE1/MMAP.BIN: $(SRCDIR)/boot/stage1/mmap.asm
	$(NASMBIN) $@ $^

$(ISODIR)/BOOT/STAGE1/A20.BIN: $(SRCDIR)/boot/stage1/a20.asm
	$(NASMBIN) $@ $^

$(ISODIR)/BOOT/STAGE1/VIDMODES.BIN: $(SRCDIR)/boot/stage1/vidmodes.asm
	$(NASMBIN) $@ $^

$(ISODIR)/BOOT/STAGE1/GDT.BIN: $(SRCDIR)/boot/stage1/gdt.asm
	$(NASMBIN) $@ $^

$(ISODIR)/BOOT/STAGE1/X64.BIN: $(SRCDIR)/boot/stage1/x64.asm
	$(NASMBIN) $@ $^

$(ISODIR)/BOOT/STAGE1/BOOTLOAD.BIN: $(SRCDIR)/boot/stage1/bootload.asm
	$(NASMBIN) $@ $^