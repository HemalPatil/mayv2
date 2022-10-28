#include <drivers/filesystems/iso9660.h>
#include <heapmemmgmt.h>

bool FS::ISO9660::isIso9660(Storage::BlockDevice *device) {
	// Get sector 16
	void *buffer = kernelMalloc(device->getBlockSize());
	device->read(16, 1, buffer, nullptr);
	return true;
}
