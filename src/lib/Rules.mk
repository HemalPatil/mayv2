# Add elements to directory stack
sp:= $(sp).x
dirstack_$(sp):= $(d)
d:= $(dir)

LIBRARY_INPUT:=$(shell find $(SRCDIR)/lib -maxdepth 1 -type f -name "*.c")
DIR_LIB:=$(patsubst $(SRCDIR)/lib/%.c,$(ISODIR)/LIB/%.lib,$(LIBRARY_INPUT))

$(ISODIR)/LIB/%.lib: $(SRCDIR)/lib/%.c
	$(CC64) -o $@ -c $< $(CWARNINGS) $(CC64FLAGS)

# Remove elements from directory stack
d:= $(dirstack_$(sp))
sp:= $(basename $(sp))