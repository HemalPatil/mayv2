#include <drivers/storage/blockdevice.h>

size_t Drivers::Storage::BlockDevice::getBlockSize() const {
	return this->blockSize;
}
