BOOT_STAGE2_OUTPUT:=$(ISODIR)/BOOT/STAGE2/LOADER32 $(ISODIR)/BOOT/STAGE2/ELFPARSE.BIN $(ISODIR)/BOOT/STAGE2/K64LOAD.BIN
BOOT_STAGE2_OBJFILES:= $(BUILDDIR)/boot/stage2/loader32asm.o $(BUILDDIR)/boot/stage2/loader32c.o $(BUILDDIR)/boot/stage2/loader32lib.o

$(ISODIR)/BOOT/STAGE2/LOADER32: $(BOOT_STAGE2_OBJFILES) $(SRCDIR)/boot/stage2/link.ld
	$(LD32) -o $@ $(BOOT_STAGE2_OBJFILES) -T $(SRCDIR)/boot/stage2/link.ld

$(ISODIR)/BOOT/STAGE2/ELFPARSE.BIN: $(SRCDIR)/boot/stage2/elfparse.asm
	$(NASMBIN) $@ $^

$(ISODIR)/BOOT/STAGE2/K64LOAD.BIN: $(SRCDIR)/boot/stage2/k64load.asm
	$(NASMBIN) $@ $^

$(BUILDDIR)/boot/stage2/loader32c.o: $(SRCDIR)/boot/stage2/loader32c.c
	$(CC32) -o $@ -c $^ $(CWARNINGS) -ffreestanding

$(BUILDDIR)/boot/stage2/%.o: $(SRCDIR)/boot/stage2/%.asm
	$(NASM32) $@ $^

$(SRCDIR)/boot/stage2/link.ld: