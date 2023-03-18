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
				const auto rootNode = std::make_shared<Node>();
				rootNode->type = NodeType::Directory;
				rootNode->offset = rootDirOffset;
				rootNode->size = rootDirSize;
				auto iso = std::shared_ptr<JolietISO>(new JolietISO(
					device,
					(size_t)descriptor->lbaSize,
					rootNode
				));
				iso->rootNode->fs = iso;
				co_return std::move(iso);
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

Async::Thenable<Drivers::FS::Status> Drivers::FS::JolietISO::readDirectory(const std::shared_ptr<Node> &node) {
	if (node->childrenCreated) {
		co_return Status::Ok;
	}
	size_t blockCount = node->size / this->deviceBlockSize;
	if (node->size != blockCount * this->deviceBlockSize) {
		++blockCount;
	}
	Storage::Buffer buffer = std::move(
		co_await this->device->read(node->offset / this->deviceBlockSize, blockCount)
	);
	if (buffer) {
		const DirectoryRecord* const extent = (DirectoryRecord*)buffer.getData();
		node->children.clear();
		size_t seekg = 0x44;	// Skip current, parent directory entries
		const DirectoryRecord *record = (DirectoryRecord*)((uint64_t)extent + seekg);
		while (seekg < node->size && record->length > 0) {
			char *fileName = new char[record->fileNameLength];
			memcpy(fileName, record->fileName, record->fileNameLength);
			// Convert to UTF16LE by flipping bytes of UTF16BE record name
			for (size_t i = 0; i < record->fileNameLength; i += 2) {
				char temp = fileName[i];
				fileName[i] = fileName[i + 1];
				fileName[i + 1] = temp;
			}
			std::u16string u16FileName = std::u16string((char16_t*)fileName, (char16_t*)(fileName + record->fileNameLength));
			node->children.push_back(std::make_shared<Node>());
			auto &childNode = node->children.back();
			childNode->name = Unicode::toUtf8(u16FileName);
			childNode->type = (record->flags & directoryFlag) ? NodeType::Directory : NodeType::File;
			childNode->offset = record->extentLba * this->blockSize;
			childNode->size = record->extentSize;
			childNode->fs = node->fs;
			childNode->parent = node;
			seekg += record->length;
			record = (DirectoryRecord*)((uint64_t)extent + seekg);
			delete[] fileName;
		}
		node->childrenCreated = true;
		co_return Status::Ok;
	} else {
		co_return Status::IOError;
	}
}
