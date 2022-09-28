DRIVERS_TIMERS_ASMOBJFILES := $(patsubst $(SRC_DIR)/drivers/timers/%.asm,$(BUILD_DIR)/drivers/timers/%.o,$(shell find $(SRC_DIR)/drivers/timers -maxdepth 1 -type f -name "*.asm"))
DRIVERS_TIMERS_COBJFILES := $(patsubst $(SRC_DIR)/drivers/timers/%.c,$(BUILD_DIR)/drivers/timers/%.o,$(shell find $(SRC_DIR)/drivers/timers -maxdepth 1 -type f -name "*.c"))

$(BUILD_DIR)/drivers/timers/%.o: $(SRC_DIR)/drivers/timers/%.asm
	$(NASM64) $@ $^

$(BUILD_DIR)/drivers/timers/%.o: $(SRC_DIR)/drivers/timers/%.c $(HEADER_FILES)
	$(CC64) -o $@ -c $< $(C_WARNINGS) $(CC64_FLAGS)
