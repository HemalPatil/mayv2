#include <cstring>
#include <drivers/filesystems/iso9660.h>
#include <terminal.h>

static const char* const isoNamespaceStr = "FS::ISO9660::";
static const char* const lbaSizeErrorStr = "ISO9660 device block size and LBA size don't match\n";

const std::string FS::ISO9660::cdSignature = "CD001";

// Returns Storage::Buffer to PrimaryVolumeDescriptor of an ISO filesystem if found on device
// Returns nullptr otherwise
Async::Thenable<Storage::Buffer> FS::ISO9660::isIso9660(std::shared_ptr<Storage::BlockDevice> device) {
	// Get sectors starting from sector 16 until Primary Volume Descriptor is found
	// Try until sector 32
	// TODO: do better error handling and get rid of arbitrary sector 32 check
	terminalPrintString("<is>", 4);
	for (size_t i = 16; i < 32; ++i) {
		Storage::Buffer buffer = std::move(co_await device->read(i, 1));
		if (buffer) {
			PrimaryVolumeDescriptor *descriptor = (PrimaryVolumeDescriptor*) buffer.getData();
			if (
				VolumeDescriptorType::Primary == descriptor->header.type &&
				0 == strncmp(descriptor->header.signature, FS::ISO9660::cdSignature.c_str(), FS::ISO9660::cdSignature.length())
			) {
	terminalPrintString("</is>", 5);
				co_return std::move(buffer);
			}
		}
	}
	co_return nullptr;
}

FS::ISO9660::ISO9660(std::shared_ptr<Storage::BlockDevice> device, const Storage::Buffer &primarySectorBuffer) : BaseFS(device) {
	PrimaryVolumeDescriptor *primarySector = (PrimaryVolumeDescriptor*) primarySectorBuffer.getData();

	terminalPrintString("<cr>", 4);
	this->lbaSize = primarySector->lbaSize;

	// TODO: Assumes device block size and ISO's LBA size are same i.e. 2048
	// Panic and fix the implementation if necessary although it's very rare
	if (this->lbaSize != this->device->getBlockSize()) {
		terminalPrintString(isoNamespaceStr, strlen(isoNamespaceStr));
		terminalPrintString(lbaSizeErrorStr, strlen(lbaSizeErrorStr));
		Kernel::panic();
	}

	this->rootDirectoryLba = primarySector->rootDirectory.extentLba;
	this->rootDirectoryExtentSize = primarySector->rootDirectory.extentSize;
	terminalPrintString("</cr>", 5);
}

Async::Thenable<bool> FS::ISO9660::initialize() {
	terminalPrintString("<init>", 6);
	Storage::Buffer buffer = std::move(co_await this->device->read(this->rootDirectoryLba, this->rootDirectoryExtentSize / this->lbaSize));
	if (!buffer) {
		co_return false;
	}
	this->rootDirectoryEntries = FS::ISO9660::extentToEntries((DirectoryRecord*)buffer.getData(), this->rootDirectoryExtentSize);
	terminalPrintString("</init>", 6);
	co_return true;
}

std::vector<FS::DirectoryEntry> FS::ISO9660::extentToEntries(const DirectoryRecord* const extent, size_t extentSize) {
	std::vector<DirectoryEntry> entries;
	entries.reserve(2);
	entries.push_back(DirectoryEntry(".", false, true, false));
	entries.push_back(DirectoryEntry("..", false, true, false));
	size_t seekg = 0x44;
	DirectoryRecord *entry = (DirectoryRecord*)((uint64_t)extent + seekg);	// Skip current, parent directory entries
	// Traverse all the remaining records in this extent
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

Async::Thenable<std::vector<FS::DirectoryEntry>> FS::ISO9660::readDirectory(const std::string &name) {
	// std::vector<FS::DirectoryEntry> dirEntries;
	// terminalPrintString("isoreaddirs\n", 12);
	// if (name == "/") {
	// 	co_return this->rootDirectoryEntries;
	// }
	// terminalPrintString("isoreaddire\n", 12);
	// terminalPrintString("notiml\n", 8);
	// Kernel::panic();
	std::vector<FS::DirectoryEntry> dirEntries;
	dirEntries.push_back(DirectoryEntry("hello", true, false, false));
	dirEntries.push_back(DirectoryEntry("world", true, false, false));
	terminalPrintString("[rd]", 4);
	Storage::Buffer buffer = std::move(co_await device->read(16, 1));
	terminalPrintString((char*)buffer.getData(), 6);
	co_return std::move(dirEntries);
}
