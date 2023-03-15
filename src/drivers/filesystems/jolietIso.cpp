#include <cstring>
#include <drivers/filesystems/jolietIso.h>
#include <terminal.h>
#include <unicode.h>

static const char* const isoNamespaceStr = "FS::JolietISO::";
static const char* const lbaSizeErrorStr = "JolietISO device block size and LBA size don't match\n";

const std::string Drivers::FS::JolietISO::cdSignature = "CD001";

Async::Thenable<std::shared_ptr<Drivers::FS::JolietISO>>
Drivers::FS::JolietISO::isJolietIso(std::shared_ptr<Storage::BlockDevice> device) {
	// Get sectors starting from sector 16 until Supplementary Volume Descriptor is found
	// Try until sector 32
	for (size_t i = 16; i < 32; ++i) {
		Storage::Buffer buffer = std::move(co_await device->read(i, 1));
		// TODO: do better error handling
		if (buffer) {
			SupplementaryVolumeDescriptor *descriptor = (SupplementaryVolumeDescriptor*) buffer.getData();
			if (
				VolumeDescriptorType::Supplementary == descriptor->header.type &&
				0 == strncmp(descriptor->header.signature, FS::JolietISO::cdSignature.c_str(), FS::JolietISO::cdSignature.length())
			) {
				co_return std::make_shared<JolietISO>(
					device,
					(size_t)descriptor->lbaSize,
					descriptor->rootDirectory.extentLba * descriptor->lbaSize,
					(size_t)descriptor->rootDirectory.extentSize
				);
			}
		}
	}
	co_return nullptr;
}

Drivers::FS::JolietISO::JolietISO(
	std::shared_ptr<Storage::BlockDevice> blockDevice,
	size_t blockSize,
	size_t rootDirOffset,
	size_t rootDirSize
)	:	BaseFS(blockDevice, blockSize, rootDirOffset, rootDirSize) {}

Async::Thenable<bool> Drivers::FS::JolietISO::initialize() {
	size_t blockCount = this->rootDirSize / this->deviceBlockSize;
	if (this->rootDirSize != blockCount * this->deviceBlockSize) {
		++blockCount;
	}
	Storage::Buffer buffer = std::move(
		co_await this->device->read(this->rootDirOffset / this->deviceBlockSize, blockCount)
	);
	if (!buffer) {
		co_return false;
	}
	this->cachedDirectoryEntries.insert({
		"/",
		FS::JolietISO::extentToEntries(
			(DirectoryRecord*)buffer.getData(),
			this->rootDirOffset,
			this->rootDirSize,
			this->rootDirOffset,
			this->rootDirSize
		)
	});
	co_return true;
}

std::vector<Drivers::FS::DirectoryEntry> Drivers::FS::JolietISO::extentToEntries(
	const DirectoryRecord* const extent,
	size_t offset,
	size_t size,
	size_t parentOffset,
	size_t parentSize
) {
	std::vector<DirectoryEntry> entries;
	entries.reserve(2);
	DirectoryEntry defaultEntry = DirectoryEntry{
		.name = ".",
		.isDir = true,
		.isFile = false,
		.isMounted = false,
		.isSymLink = false,
		.offset = offset,
		.size = size,
		.mountedFs = nullptr
	};
	entries.push_back(defaultEntry);
	defaultEntry.name = "..";
	defaultEntry.offset = parentOffset;
	defaultEntry.size = parentSize;
	entries.push_back(defaultEntry);
	size_t seekg = 0x44;
	const DirectoryRecord *entry = (DirectoryRecord*)((uint64_t)extent + seekg);	// Skip current, parent directory entries
	// Traverse all the remaining records in this extent
	while (seekg < size && entry->length > 0) {
		char *fileName = new char[entry->fileNameLength];
		memcpy(fileName, entry->fileName, entry->fileNameLength);
		// Convert to UTF16LE by flipping bytes of UTF16BE record name
		for (size_t i = 0; i < entry->fileNameLength; i += 2) {
			char temp = fileName[i];
			fileName[i] = fileName[i + 1];
			fileName[i + 1] = temp;
		}
		std::u16string u16FileName = std::u16string((char16_t*)fileName, (char16_t*)(fileName + entry->fileNameLength));
		entries.push_back({
			.name = Unicode::toUtf8(u16FileName),
			.isDir = bool(entry->flags & directoryFlag),
			.isFile = !(entry->flags & directoryFlag),
			.isMounted = false,
			.isSymLink = false,
			.offset = entry->extentLba * 2048,
			.size = entry->extentSize,
			.mountedFs = nullptr
		});
		seekg += entry->length;
		entry = (DirectoryRecord*)((uint64_t)extent + seekg);
		delete[] fileName;
	}
	return entries;
}

Async::Thenable<Drivers::FS::ReadFileResult> Drivers::FS::JolietISO::readFile(const std::string &absolutePath) {
	isValidAbsolutePath(absolutePath, false);
	const auto parentSplit = splitParentDirectory(absolutePath);
	const auto parentDirResult = co_await this->readDirectory(std::get<0>(parentSplit));
	ReadFileResult errorResult;
	if (parentDirResult.status != Status::Ok) {
		errorResult.status = parentDirResult.status;
		co_return std::move(errorResult);
	}
	const DirectoryEntry* entry = nullptr;
	for (const auto &dirEntry : parentDirResult.entries) {
		if (dirEntry.name == std::get<1>(parentSplit)) {
			if (dirEntry.isFile) {
				entry = &dirEntry;
				break;
			} else {
				errorResult.status = Status::NotFile;
				co_return std::move(errorResult);
			}
		}
	}
	if (entry) {
		if (entry->size == 0) {
			co_return std::move(errorResult);
		} else {
			size_t blockCount = entry->size / this->deviceBlockSize;
			if (entry->size != blockCount * this->deviceBlockSize) {
				++blockCount;
			}
			Storage::Buffer data = std::move(
				co_await this->device->read(entry->offset / this->blockSize, blockCount)
			);
			co_return {
				.status = data ? Status::Ok : Status::IOError,
				.data = std::move(data)
			};
		}
	}
	errorResult.status = Status::NoSuchDirectoryOrFile;
	co_return std::move(errorResult);
}

Async::Thenable<Drivers::FS::ReadDirectoryResult> Drivers::FS::JolietISO::readDirectory(const std::string &absolutePath) {
	isValidAbsolutePath(absolutePath, true);
	if (1 != this->cachedDirectoryEntries.count(absolutePath)) {
		std::vector<std::string> pathParts = splitAbsolutePath(absolutePath);
		std::string pathSoFar = "/";
		ReadDirectoryResult errorResult;
		for (const auto &pathPart : pathParts) {
			// Check if pathPart exists in the cached entries for pathSoFar
			const std::vector<DirectoryEntry> &parentEntries = this->cachedDirectoryEntries.at(pathSoFar);
			const DirectoryEntry &parentEntry = parentEntries.at(0);
			const DirectoryEntry* entry = nullptr;
			for (const auto &dirEntry : parentEntries) {
				if (dirEntry.name == pathPart) {
					if (dirEntry.isDir) {
						entry = &dirEntry;
						break;
					} else {
						errorResult.status = Status::NotDirectory;
						co_return std::move(errorResult);
					}
				}
			}
			if (entry) {
				if (1 != this->cachedDirectoryEntries.count(pathSoFar + pathPart + '/')) {
					// Get the entries for pathSoFar
					size_t blockCount = entry->size / this->deviceBlockSize;
					if (entry->size != blockCount * this->deviceBlockSize) {
						++blockCount;
					}
					Storage::Buffer buffer = std::move(
						co_await this->device->read(entry->offset / this->deviceBlockSize, blockCount)
					);
					if (buffer) {
						this->cachedDirectoryEntries.insert({
							pathSoFar + pathPart + '/',
							FS::JolietISO::extentToEntries(
								(DirectoryRecord*)buffer.getData(),
								entry->offset,
								entry->size,
								parentEntry.offset,
								parentEntry.size
							)
						});
					} else {
						errorResult.status = Status::IOError;
						co_return std::move(errorResult);
					}
				}
				pathSoFar += pathPart + '/';
			} else {
				errorResult.status = Status::NoSuchDirectoryOrFile;
				co_return std::move(errorResult);
			}
		}
	}
	co_return {
		.status = Status::Ok,
		.entries = this->cachedDirectoryEntries.at(absolutePath)
	};
}
