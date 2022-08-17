# Add elements to directory stack
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

INCLUDE_HEADERS := $(shell find $(SRCDIR)/include -maxdepth 1 -type f -name "*.h")
DIR_INCLUDE := $(patsubst $(SRCDIR)/include/%,$(ISODIR)/INCLUDE/%,$(INCLUDE_HEADERS))

$(ISODIR)/INCLUDE/%: $(SRCDIR)/include/%
	cp $< $@

# Remove elements from directory stack
d := $(dirstack_$(sp))
sp := $(basename $(sp))