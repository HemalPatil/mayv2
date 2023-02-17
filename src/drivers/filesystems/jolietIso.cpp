#include <cstring>
#include <drivers/filesystems/jolietIso.h>
#include <terminal.h>
#include <unicode.h>

static const char* const isoNamespaceStr = "FS::JolietISO::";
static const char* const lbaSizeErrorStr = "JolietISO device block size and LBA size don't match\n";

const std::string FS::JolietISO::cdSignature = "CD001";

// Returns Storage::Buffer to PrimaryVolumeDescriptor of an ISO filesystem if found on device
// Returns nullptr otherwise
Async::Thenable<Storage::Buffer> FS::JolietISO::isJolietIso(std::shared_ptr<Storage::BlockDevice> device) {
	// Get sectors starting from sector 16 until Supplementary Volume Descriptor is found
	// Try until sector 32
	// TODO: get rid of arbitrary sector 32 check
	for (size_t i = 16; i < 32; ++i) {
		Storage::Buffer buffer = std::move(co_await device->read(i, 1));
		// TODO: do better error handling
		if (buffer) {
			SupplementaryVolumeDescriptor *descriptor = (SupplementaryVolumeDescriptor*) buffer.getData();
			if (
				VolumeDescriptorType::Supplementary == descriptor->header.type &&
				0 == strncmp(descriptor->header.signature, FS::JolietISO::cdSignature.c_str(), FS::JolietISO::cdSignature.length())
			) {
				co_return std::move(buffer);
			}
		}
	}
	co_return nullptr;
}

FS::JolietISO::JolietISO(std::shared_ptr<Storage::BlockDevice> device, const Storage::Buffer &svdSectorBuffer) : BaseFS(device) {
	SupplementaryVolumeDescriptor *svdSector = (SupplementaryVolumeDescriptor*) svdSectorBuffer.getData();

	this->lbaSize = svdSector->lbaSize;

	// TODO: Assumes device block size and ISO's LBA size are same i.e. 2048
	// Panic and fix the implementation if necessary although it's very rare
	if (this->lbaSize != this->device->getBlockSize()) {
		terminalPrintString(isoNamespaceStr, strlen(isoNamespaceStr));
		terminalPrintString(lbaSizeErrorStr, strlen(lbaSizeErrorStr));
		Kernel::panic();
	}

	this->rootDirLba = svdSector->rootDirectory.extentLba;
	this->rootDirExtentSize = svdSector->rootDirectory.extentSize;
}

Async::Thenable<bool> FS::JolietISO::initialize() {
	Storage::Buffer buffer = std::move(
		co_await this->device->read(this->rootDirLba, this->rootDirExtentSize / this->lbaSize)
	);
	if (!buffer) {
		co_return false;
	}
	this->cachedDirectoryEntries.insert({
		"/",
		FS::JolietISO::extentToEntries(
			(DirectoryRecord*)buffer.getData(),
			this->rootDirLba,
			this->rootDirExtentSize,
			this->rootDirLba,
			this->rootDirExtentSize
		)
	});
	co_return true;
}

std::vector<FS::DirectoryEntry> FS::JolietISO::extentToEntries(
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
		// Convert to UTF16LE by flipping bytes of UTF16BE record name
		for (size_t i = 0; i < entry->fileNameLength; i += 2) {
			char temp = entry->fileName[i];
			entry->fileName[i] = entry->fileName[i + 1];
			entry->fileName[i + 1] = temp;
		}
		std::u16string u16FileName = std::u16string((char16_t*)entry->fileName, (char16_t*)(entry->fileName + entry->fileNameLength));
		entries.push_back({
			.name = Unicode::toUtf8(u16FileName),
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

Async::Thenable<std::vector<FS::DirectoryEntry>> FS::JolietISO::readDirectory(const std::string &absolutePath) {
	if (1 != this->cachedDirectoryEntries.count(absolutePath)) {
		std::vector<std::string> pathParts = splitAbsolutePath(absolutePath, true);
		std::string pathSoFar = "/";
		std::vector<DirectoryEntry> empty;
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
							FS::JolietISO::extentToEntries(
								(DirectoryRecord*)buffer.getData(),
								entry->lba,
								entry->size,
								parentEntry.lba,
								parentEntry.size
							)
						});
					} else {
						co_return std::move(empty);
					}
				}
				pathSoFar += pathPart + '/';
			} else {
				co_return std::move(empty);
			}
		}
	}
	co_return std::vector<DirectoryEntry>(this->cachedDirectoryEntries.at(absolutePath));
}
