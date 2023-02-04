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
	// TODO: get rid of arbitrary sector 32 check
	for (size_t i = 16; i < 32; ++i) {
		Storage::Buffer buffer = std::move(co_await device->read(i, 1));
		// TODO: do better error handling
		if (buffer) {
			PrimaryVolumeDescriptor *descriptor = (PrimaryVolumeDescriptor*) buffer.getData();
			if (
				VolumeDescriptorType::Primary == descriptor->header.type &&
				0 == strncmp(descriptor->header.signature, FS::ISO9660::cdSignature.c_str(), FS::ISO9660::cdSignature.length())
			) {
				co_return std::move(buffer);
			}
		}
	}
	co_return nullptr;
}

FS::ISO9660::ISO9660(std::shared_ptr<Storage::BlockDevice> device, const Storage::Buffer &primarySectorBuffer) : BaseFS(device) {
	PrimaryVolumeDescriptor *primarySector = (PrimaryVolumeDescriptor*) primarySectorBuffer.getData();

	this->lbaSize = primarySector->lbaSize;

	// TODO: Assumes device block size and ISO's LBA size are same i.e. 2048
	// Panic and fix the implementation if necessary although it's very rare
	if (this->lbaSize != this->device->getBlockSize()) {
		terminalPrintString(isoNamespaceStr, strlen(isoNamespaceStr));
		terminalPrintString(lbaSizeErrorStr, strlen(lbaSizeErrorStr));
		Kernel::panic();
	}

	this->rootDirLba = primarySector->rootDirectory.extentLba;
	this->rootDirExtentSize = primarySector->rootDirectory.extentSize;
}

Async::Thenable<bool> FS::ISO9660::initialize() {
	Storage::Buffer buffer = std::move(
		co_await this->device->read(this->rootDirLba, this->rootDirExtentSize / this->lbaSize)
	);
	if (!buffer) {
		co_return false;
	}
	this->cachedDirectoryEntries.insert({
		"/",
		FS::ISO9660::extentToEntries(
			(DirectoryRecord*)buffer.getData(),
			this->rootDirLba,
			this->rootDirExtentSize,
			this->rootDirLba,
			this->rootDirExtentSize
		)
	});
	co_return true;
}

std::vector<FS::DirectoryEntry> FS::ISO9660::extentToEntries(
	const DirectoryRecord* const extent,
	size_t lba,
	size_t size,
	size_t parentLba,
	size_t parentSize
) {
	std::vector<DirectoryEntry> entries;
	entries.reserve(2);
	entries.push_back({.name = ".", .isFile = false, .isDir = true, .isSymLink = false, .lba = lba, .size = size});
	entries.push_back({.name = "..", .isFile = false, .isDir = true, .isSymLink = false, .lba = parentLba, .size = parentSize});
	size_t seekg = 0x44;
	DirectoryRecord *entry = (DirectoryRecord*)((uint64_t)extent + seekg);	// Skip current, parent directory entries
	// Traverse all the remaining records in this extent
	while (seekg < size && entry->length > 0) {
		entries.push_back({
			.name = std::string(
				entry->fileName,
				// Skip ";1" terminator for file names
				entry->fileName + entry->fileNameLength - ((entry->flags & directoryFlag) ? 0 : 2)
			),
			.isFile = !(entry->flags & directoryFlag),
			.isDir = bool(entry->flags & directoryFlag),
			.isSymLink = false,
			.lba = entry->extentLba,
			.size = entry->extentSize
		});
		seekg += entry->length;
		entry = (DirectoryRecord*)((uint64_t)extent + seekg);
	}
	return entries;
}

Async::Thenable<std::vector<FS::DirectoryEntry>> FS::ISO9660::readDirectory(const std::string &absolutePath) {
	if (1 == this->cachedDirectoryEntries.count(absolutePath)) {
		co_return std::vector<FS::DirectoryEntry>(this->cachedDirectoryEntries.at(absolutePath));
	}
	std::vector<std::string> pathParts = splitAbsolutePath(absolutePath, true);
	std::string pathSoFar = "/";
	for (const auto &pathPart : pathParts) {
		// Check if pathPart exists in the cached entries for pathSoFar
		const std::vector<DirectoryEntry> &parentEntries = this->cachedDirectoryEntries.at(pathSoFar);
		const DirectoryEntry &parentEntry = parentEntries.at(0);
		const DirectoryEntry* entry = nullptr;
		for (const auto &dirEntry : parentEntries) {
			if (dirEntry.isDir && dirEntry.name == pathPart) {
				entry = &dirEntry;
				break;
			}
		}

		if (entry) {
			if (1 != this->cachedDirectoryEntries.count(pathSoFar + pathPart + '/')) {
				// Get the entries for pathSoFar
				Storage::Buffer buffer = std::move(co_await this->device->read(entry->lba, entry->size / this->lbaSize));
				// TODO: do better error handling
				if (buffer) {
					this->cachedDirectoryEntries.insert({
						pathSoFar + pathPart + '/',
						FS::ISO9660::extentToEntries(
							(DirectoryRecord*)buffer.getData(),
							entry->lba,
							entry->size,
							parentEntry.lba,
							parentEntry.size
						)
					});
				} else {
					co_return std::vector<DirectoryEntry>();
				}
			}
			pathSoFar += pathPart + '/';
		} else {
			co_return std::vector<DirectoryEntry>();
		}
	}
	co_return std::vector<FS::DirectoryEntry>(this->cachedDirectoryEntries.at(absolutePath));
}
