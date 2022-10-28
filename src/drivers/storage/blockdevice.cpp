#include <drivers/storage/blockdevice.h>

size_t Storage::BlockDevice::getBlockSize() const {
	return this->blockSize;
}
