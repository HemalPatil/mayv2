#include <cstring>
#include <drivers/filesystems/iso9660.h>
#include <terminal.h>

static const char* const isoNamespaceStr = "FS::ISO9660::";
static const char* const lbaSizeErrorStr = "ISO9660 device block size and LBA size don't match\n";

const std::string FS::ISO9660::cdSignature = "CD001";

std::tuple<bool, size_t> FS::ISO9660::isIso9660(std::shared_ptr<Storage::BlockDevice> device) {
	// Get sectors starting from sector 16 until Primary Volume Descriptor is found
	// Try until sector 32
	// TODO: do better error handling and get rid of arbitrary sector 32 check
	std::shared_ptr<PrimaryVolumeDescriptor> buffer(
		(PrimaryVolumeDescriptor*)Kernel::Memory::Heap::allocate(device->getBlockSize())
	);
	bool primaryVolumeFound = false;
	size_t primarySectorNumber = SIZE_MAX;
	for (size_t i = 16; i < 32; ++i) {
		if (device->read(i, 1, buffer)->awaitGet()) {
			if (
				strncmp(buffer->header.signature, FS::ISO9660::cdSignature.c_str(), FS::ISO9660::cdSignature.length()) == 0 &&
				buffer->header.type == VolumeDescriptorType::Primary
			) {
				primaryVolumeFound = true;
				primarySectorNumber = i;
				break;
			}
		}
	}
	return {primaryVolumeFound, primarySectorNumber};
}

FS::ISO9660::ISO9660(std::shared_ptr<Storage::BlockDevice> device, size_t primarySectorNumber) : BaseFS(device) {
	std::shared_ptr<PrimaryVolumeDescriptor> primarySector(
		(PrimaryVolumeDescriptor*)Kernel::Memory::Heap::allocate(device->getBlockSize())
	);
	this->device->read(primarySectorNumber, 1, primarySector)->awaitGet();
	this->lbaSize = primarySector->lbaSize;

	// TODO: Assumes device block size and ISO's LBA size are same i.e. 2048
	// Panic and fix the implementation if necessary although it's very rare
	if (this->lbaSize != this->device->getBlockSize()) {
		terminalPrintString(isoNamespaceStr, strlen(isoNamespaceStr));
		terminalPrintString(lbaSizeErrorStr, strlen(lbaSizeErrorStr));
		Kernel::panic();
	}

	std::shared_ptr<DirectoryRecord> rootDirectoryExtent = std::shared_ptr<DirectoryRecord>(
		(DirectoryRecord*)Kernel::Memory::Heap::allocate(primarySector->rootDirectory.extentSize)
	);
	this->device->read(
		primarySector->rootDirectory.extentLba,
		primarySector->rootDirectory.extentSize / this->lbaSize,
		rootDirectoryExtent
	)->awaitGet();
	this->rootDirectoryEntries = this->extentToEntries(rootDirectoryExtent.get(), primarySector->rootDirectory.extentSize);
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
