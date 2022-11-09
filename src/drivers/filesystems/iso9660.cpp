#include <drivers/filesystems/iso9660.h>
#include <terminal.h>

bool FS::ISO9660::isIso9660(Storage::BlockDevice *device) {
	// Get sectors starting from sector 16 until Primary Volume Descriptor is found
	// Try until sector 32
	// TODO: do better error handling and get rid of arbitrary sector 32 check
	void *buffer = Kernel::Memory::Heap::malloc(device->getBlockSize());
	bool primaryVolumeFound = false;
	for (size_t i = 16; i < 32; ++i) {
		if (device->read(i, 1, buffer)->awaitGet()) {
			if (((PrimaryVolumeDescriptor*)buffer)->header.type == VolumeDescriptorType::Primary) {
				primaryVolumeFound = true;
				break;
			}
		}
	}
	Kernel::Memory::Heap::free(buffer);
	return primaryVolumeFound;
}

bool FS::ISO9660::openFile() {
	return true;
}
