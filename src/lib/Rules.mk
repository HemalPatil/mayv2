# Add elements to directory stack
sp:= $(sp).x
dirstack_$(sp):= $(d)
d:= $(dir)

# Remove elements from directory stack
d:= $(dirstack_$(sp))
sp:= $(basename $(sp))