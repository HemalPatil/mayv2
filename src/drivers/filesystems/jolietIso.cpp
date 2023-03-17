#include <cstring>
#include <drivers/filesystems/jolietIso.h>
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
				strncmp(
					descriptor->header.signature,
					FS::JolietISO::cdSignature.c_str(),
					FS::JolietISO::cdSignature.length()
				) == 0
			) {
				size_t rootDirOffset = descriptor->rootDirectory.extentLba * descriptor->lbaSize;
				size_t rootDirSize = descriptor->rootDirectory.extentSize;
				size_t deviceBlockSize = device->getBlockSize();
				size_t blockCount = rootDirSize / deviceBlockSize;
				if (rootDirSize != blockCount * deviceBlockSize) {
					++blockCount;
				}
				Storage::Buffer rootBuffer = std::move(
					co_await device->read(rootDirOffset / deviceBlockSize, blockCount)
				);
				// TODO: do better error handling
				if (!rootBuffer) {
					co_return nullptr;
				}
				const auto rootNode = std::make_shared<Node>();
				rootNode->type = NodeType::Directory;
				rootNode->offset = rootDirOffset;
				rootNode->size = rootDirSize;
				rootNode->childrenCreated = true;
				rootNode->children = extentToNodes(
					(DirectoryRecord*)rootBuffer.getData(),
					(size_t)descriptor->lbaSize,
					rootNode
				);
				co_return std::make_shared<JolietISO>(
					device,
					(size_t)descriptor->lbaSize,
					rootNode
				);
			}
		}
	}
	co_return nullptr;
}

Drivers::FS::JolietISO::JolietISO(
	std::shared_ptr<Storage::BlockDevice> blockDevice,
	size_t blockSize,
	std::shared_ptr<Node> rootNode
)	:	Base(blockDevice, blockSize, rootNode) {}

std::vector<std::shared_ptr<Drivers::FS::Node>> Drivers::FS::JolietISO::extentToNodes(
	const DirectoryRecord* const extent,
	size_t blockSize,
	const std::shared_ptr<Node> &selfNode
) {
	std::vector<std::shared_ptr<Node>> nodes;
	nodes.reserve(2);
	size_t seekg = 0x44;
	const DirectoryRecord *record = (DirectoryRecord*)((uint64_t)extent + seekg);	// Skip current, parent directory entries
	// Traverse all the remaining records in this extent
	while (seekg < selfNode->size && record->length > 0) {
		char *fileName = new char[record->fileNameLength];
		memcpy(fileName, record->fileName, record->fileNameLength);
		// Convert to UTF16LE by flipping bytes of UTF16BE record name
		for (size_t i = 0; i < record->fileNameLength; i += 2) {
			char temp = fileName[i];
			fileName[i] = fileName[i + 1];
			fileName[i + 1] = temp;
		}
		std::u16string u16FileName = std::u16string((char16_t*)fileName, (char16_t*)(fileName + record->fileNameLength));
		nodes.push_back(std::make_shared<Node>());
		auto &node = nodes.back();
		node->name = Unicode::toUtf8(u16FileName);
		node->type = (record->flags & directoryFlag) ? NodeType::Directory : NodeType::File;
		node->offset = record->extentLba * blockSize;
		node->size = record->extentSize;
		node->parent = selfNode;
		seekg += record->length;
		record = (DirectoryRecord*)((uint64_t)extent + seekg);
		delete[] fileName;
	}
	return nodes;
}

// Async::Thenable<Drivers::FS::ReadFileResult> Drivers::FS::JolietISO::readFile(const std::string &absolutePath) {
// 	isValidAbsolutePath(absolutePath, false);
// 	const auto parentSplit = splitParentDirectory(absolutePath);
// 	const auto parentDirResult = co_await this->readDirectory(std::get<0>(parentSplit));
// 	ReadFileResult errorResult;
// 	if (parentDirResult.status != Status::Ok) {
// 		errorResult.status = parentDirResult.status;
// 		co_return std::move(errorResult);
// 	}
// 	const DirectoryEntry* entry = nullptr;
// 	for (const auto &dirEntry : parentDirResult.entries) {
// 		if (dirEntry.name == std::get<1>(parentSplit)) {
// 			if (dirEntry.isFile) {
// 				entry = &dirEntry;
// 				break;
// 			} else {
// 				errorResult.status = Status::NotFile;
// 				co_return std::move(errorResult);
// 			}
// 		}
// 	}
// 	if (entry) {
// 		if (entry->size == 0) {
// 			co_return std::move(errorResult);
// 		} else {
// 			size_t blockCount = entry->size / this->deviceBlockSize;
// 			if (entry->size != blockCount * this->deviceBlockSize) {
// 				++blockCount;
// 			}
// 			Storage::Buffer data = std::move(
// 				co_await this->device->read(entry->offset / this->blockSize, blockCount)
// 			);
// 			co_return {
// 				.status = data ? Status::Ok : Status::IOError,
// 				.data = std::move(data)
// 			};
// 		}
// 	}
// 	errorResult.status = Status::NoSuchDirectoryOrFile;
// 	co_return std::move(errorResult);
// }

Async::Thenable<Drivers::FS::ReadDirectoryResult> Drivers::FS::JolietISO::readDirectory(const std::string &absolutePath) {
	ReadDirectoryResult errorResult;
	if (!isValidAbsolutePath(absolutePath, true)) {
		errorResult.status = Status::InvalidPath;
		co_return std::move(errorResult);
	}
	std::vector<std::string> pathParts = splitAbsolutePath(absolutePath);
	if (pathParts.size() == 0) {
		co_return ReadDirectoryResult{
			.status = Status::Ok,
			.directory = this->rootNode
		};
	}
	std::shared_ptr<Node> currentNode = this->rootNode;
	for (size_t i = 0; i <= pathParts.size(); ++i) {
		if (currentNode->type & NodeType::Directory) {
			if (currentNode->type & NodeType::MountPoint) {
				std::string remainingPath = "/";
				for (size_t j = i; j < pathParts.size(); ++j) {
					remainingPath += pathParts.at(j);
					remainingPath += '/';
				}
				co_return co_await currentNode->mountedFs->readDirectory(remainingPath);
			}
			if (!currentNode->childrenCreated) {
				size_t blockCount = currentNode->size / this->deviceBlockSize;
				if (currentNode->size != blockCount * this->deviceBlockSize) {
					++blockCount;
				}
				Storage::Buffer buffer = std::move(
					co_await this->device->read(currentNode->offset / this->deviceBlockSize, blockCount)
				);
				if (buffer) {
					currentNode->childrenCreated = true;
					currentNode->children = extentToNodes(
						(DirectoryRecord*)buffer.getData(),
						this->blockSize,
						currentNode
					);
				} else {
					errorResult.status = Status::IOError;
					co_return std::move(errorResult);
				}
			}
			if (i == pathParts.size()) {
				co_return {
					.status = Status::Ok,
					.directory = currentNode
				};
			}
			bool found = false;
			for (size_t j = 0; j < currentNode->children.size(); ++j) {
				if (currentNode->children.at(j)->name == pathParts[i]) {
					currentNode = currentNode->children.at(j);
					found = true;
					break;
				}
			}
			if (!found) {
				errorResult.status = Status::NoSuchDirectoryOrFile;
				co_return std::move(errorResult);
			}
		} else {
			errorResult.status = Status::NotDirectory;
			co_return std::move(errorResult);
		}
	}
	co_return std::move(errorResult);
}
