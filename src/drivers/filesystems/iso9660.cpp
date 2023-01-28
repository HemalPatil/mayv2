#include <cstring>
#include <drivers/filesystems/iso9660.h>
#include <terminal.h>

static const char* const isoNamespaceStr = "FS::ISO9660::";
static const char* const lbaSizeErrorStr = "ISO9660 device block size and LBA size don't match\n";

const std::string FS::ISO9660::cdSignature = "CD001";

// Returns Storage::Buffer to PrimaryVolumeDescriptor of an ISO filesystem if found on device
// Returns nullptr otherwise
Async::Thenable<std::shared_ptr<Storage::Buffer>> FS::ISO9660::isIso9660(std::shared_ptr<Storage::BlockDevice> device) {
	// Get sectors starting from sector 16 until Primary Volume Descriptor is found
	// Try until sector 32
	// TODO: do better error handling and get rid of arbitrary sector 32 check
	for (size_t i = 16; i < 32; ++i) {
		std::shared_ptr<Storage::Buffer> buffer = co_await device->read(i, 1);
		if (buffer) {
			PrimaryVolumeDescriptor *descriptor = (PrimaryVolumeDescriptor*) buffer->getData();
			if (
				VolumeDescriptorType::Primary == descriptor->header.type &&
				0 == strncmp(descriptor->header.signature, FS::ISO9660::cdSignature.c_str(), FS::ISO9660::cdSignature.length())
			) {
				co_return buffer;
			}
		}
	}
	co_return nullptr;
}

FS::ISO9660::ISO9660(std::shared_ptr<Storage::BlockDevice> device, std::shared_ptr<Storage::Buffer> primarySectorBuffer) : BaseFS(device) {
	PrimaryVolumeDescriptor *primarySector = (PrimaryVolumeDescriptor*) primarySectorBuffer->getData();
	this->lbaSize = primarySector->lbaSize;

	// TODO: Assumes device block size and ISO's LBA size are same i.e. 2048
	// Panic and fix the implementation if necessary although it's very rare
	if (this->lbaSize != this->device->getBlockSize()) {
		terminalPrintString(isoNamespaceStr, strlen(isoNamespaceStr));
		terminalPrintString(lbaSizeErrorStr, strlen(lbaSizeErrorStr));
		Kernel::panic();
	}

	// terminalPrintDecimal(primarySector->rootDirectory.extentSize / this->lbaSize);
	// std::shared_ptr<DirectoryRecord> rootDirectoryExtent = std::shared_ptr<DirectoryRecord>(
	// 	(DirectoryRecord*)Kernel::Memory::Heap::allocate(primarySector->rootDirectory.extentSize)
	// );
	// this->device->read(
	// 	primarySector->rootDirectory.extentLba,
	// 	primarySector->rootDirectory.extentSize / this->lbaSize,
	// 	rootDirectoryExtent
	// )->awaitGet();
	// this->rootDirectoryEntries = this->extentToEntries(rootDirectoryExtent.get(), primarySector->rootDirectory.extentSize);
}

std::vector<FS::DirectoryEntry> FS::ISO9660::extentToEntries(const DirectoryRecord* const extent, size_t extentSize) {
	std::vector<DirectoryEntry> entries;
	entries.push_back(DirectoryEntry(".", false, true, false));
	entries.push_back(DirectoryEntry("..", false, true, false));
	size_t seekg = 0x44;
	DirectoryRecord *entry = (DirectoryRecord*)((uint64_t)extent + seekg);	// Skip current, parent directory entries
	// Check if all records have been traversed
	while (seekg < extentSize && entry->length > 0) {
		entries.push_back(DirectoryEntry(
			std::string(
				entry->fileName,
				// Skip ";1" terminator for file names
				entry->fileName + entry->fileNameLength - ((entry->flags & directoryFlag) ? 0 : 2)
			),
			!(entry->flags & directoryFlag),
			(entry->flags & directoryFlag),
			false
		));
		seekg += entry->length;
		entry = (DirectoryRecord*)((uint64_t)extent + seekg);
	}
	return entries;
}

std::vector<FS::DirectoryEntry> FS::ISO9660::readDirectory(const std::string &name) {
	std::vector<FS::DirectoryEntry> dirEntries;
	terminalPrintString("isoreaddirs\n", 12);
	if (name == "/") {
		return this->rootDirectoryEntries;
	}
	terminalPrintString("isoreaddire\n", 12);
	terminalPrintString("notiml\n", 8);
	Kernel::panic();
	return dirEntries;
}
