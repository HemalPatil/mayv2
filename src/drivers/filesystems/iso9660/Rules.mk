DRIVERS_FS_ISO_9660_CPPOBJFILES := $(patsubst $(SRC_DIR)/drivers/filesystems/iso9660/%.cpp,$(BUILD_DIR)/drivers/filesystems/iso9660/%.o,$(shell find $(SRC_DIR)/drivers/filesystems/iso9660 -maxdepth 1 -type f -name "*.cpp"))
