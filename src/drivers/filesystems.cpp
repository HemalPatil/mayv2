#include <cstring>
#include <drivers/filesystems.h>
#include <unicode.h>

static const char* const malformedStr = "FS::isValidAbsolutePath malformed absolute path [";

std::vector<std::shared_ptr<Drivers::FS::Base>> Drivers::FS::filesystems;
std::shared_ptr<Drivers::FS::Base> Drivers::FS::root;

Async::Thenable<Drivers::FS::OpenFileResult> Drivers::FS::openFile(
	const std::string &absolutePath,
	const OpenFileType &openType,
	const std::shared_ptr<Base> &fs
) {
	OpenFileResult errorResult;
	if (!isValidAbsolutePath(absolutePath, false)) {
		errorResult.status = Status::InvalidPath;
		co_return std::move(errorResult);
	}
	const auto parentSplit = splitParentDirectory(absolutePath);
	const auto parentDirResult = co_await readDirectory(std::get<0>(parentSplit), fs);
	if (parentDirResult.status != Status::Ok) {
		errorResult.status = parentDirResult.status;
		co_return std::move(errorResult);
	}
	std::shared_ptr<Node> fileNode;
	for (const auto &child : parentDirResult.directory->children) {
		if (child->name == std::get<1>(parentSplit)) {
			if (child->type & NodeType::File) {
				fileNode = child;
			} else {
				errorResult.status = Status::NotFile;
				co_return std::move(errorResult);
			}
		}
	}
	if (!fileNode) {
		if (openType & OpenFileType::CreateIfNotExists) {
			// TODO: complete implementation
			Kernel::panic();
		} else {
			errorResult.status = Status::NoSuchDirectoryOrFile;
			co_return std::move(errorResult);
		}
	}
	const auto descriptor = std::make_shared<FileDescriptor>();
	descriptor->node = fileNode;
	descriptor->openType = openType;
	descriptor->readOffset =
	descriptor->writeOffset =
		(openType & OpenFileType::SeekEnd) ? fileNode->size : 0;
	fileNode->openFileDescriptors.push_back(descriptor);
	co_return {
		.status = Status::Ok,
		.file = descriptor
	};
}

Async::Thenable<Drivers::FS::Status> Drivers::FS::closeFile(const std::shared_ptr<FileDescriptor> &file) {
	if (file && file->node) {
		// Find the descriptor in the list of open file descriptors
		// Remove it if found or return error
		for (size_t i = 0; i < file->node->openFileDescriptors.size(); ++i) {
			if (file->node->openFileDescriptors.at(i) == file) {
				file->node->openFileDescriptors.erase(file->node->openFileDescriptors.begin() + i);

				// TODO: should check if this was the last descriptor to be closed
				// and do something about it. Maybe free all the buffers.

				co_return Status::Ok;
			}
		}
		co_return Status::NotFile;
	}
	co_return Status::NotFile;
}

Async::Thenable<Drivers::FS::ReadDirectoryResult> Drivers::FS::readDirectory(
	const std::string &absolutePath,
	const std::shared_ptr<Base> &fs
) {
	ReadDirectoryResult errorResult;
	if (!isValidAbsolutePath(absolutePath, true)) {
		errorResult.status = Status::InvalidPath;
		co_return std::move(errorResult);
	}
	std::vector<std::string> pathParts = splitAbsolutePath(absolutePath);
	std::shared_ptr<Node> currentNode = fs->rootNode;
	for (size_t i = 0; i <= pathParts.size(); ++i) {
		if (currentNode->type & NodeType::Directory) {
			if (currentNode->type & NodeType::MountPoint) {
				currentNode = currentNode->mountedNode;
			}
			if (!currentNode->childrenCreated) {
				auto status = co_await currentNode->fs->readDirectory(currentNode);
				if (status != Status::Ok) {
					errorResult.status = status;
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
			for (const auto &child : currentNode->children) {
				if (child->name == pathParts[i]) {
					currentNode = child;
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
}

std::tuple<std::string, std::string> Drivers::FS::splitParentDirectory(const std::string &absolutePath) {
	if (absolutePath == "/") {
		return {absolutePath, ""};
	}
	bool isDir = absolutePath.back() == '/';
	int length = absolutePath.length() - (isDir ? 2 : 1);
	for (; length >= 0; --length) {
		if (absolutePath.at(length) == '/') {
			break;
		}
	}
	return {
		absolutePath.substr(0, length + 1),
		absolutePath.substr(length + 1, absolutePath.length() - length - (isDir ? 2 : 1))
	};
}

std::vector<std::string> Drivers::FS::splitAbsolutePath(const std::string &absolutePath) {
	std::vector<std::string> pathParts;
	size_t pos = 1;
	size_t currentPos = 1;
	for (; currentPos < absolutePath.length(); ++currentPos) {
		if ('/' == absolutePath.at(currentPos)) {
			pathParts.push_back(absolutePath.substr(pos, currentPos - pos));
			pos = currentPos + 1;
		}
	}
	if ('/' != absolutePath.back()) {
		pathParts.push_back(absolutePath.substr(pos));
	}
	return pathParts;
}

Drivers::FS::Base::Base(
	std::shared_ptr<Storage::BlockDevice> blockDevice,
	size_t blockSize,
	std::shared_ptr<Node> rootNode
)	:	device(blockDevice),
		deviceBlockSize(blockDevice->getBlockSize()),
		blockSize(blockSize),
		rootNode(rootNode),
		guid() {}

const Random::GUIDv4& Drivers::FS::Base::getGuid() const {
	return this->guid;
}

bool Drivers::FS::isValidAbsolutePath(const std::string &absolutePath, bool isDir) {
	if (!Unicode::isValidUtf8(absolutePath)) {
		return false;
	}
	if (!(
		0 == absolutePath.length() ||
		'/' != absolutePath.at(0) ||
		(isDir && '/' != absolutePath.back()) ||
		(!isDir && '/' == absolutePath.back())
	)) {
		for (size_t i = 1; i < absolutePath.length(); ++i) {
			auto c = absolutePath.at(i);
			// Check consecutive '/'
			if (c == '/' && absolutePath.at(i - 1) == '/') {
				return false;
			}
			if (
				(c >= 0 && c <= 0x1f) ||	// Unicode control characters C0
				c == '\\' ||
				c == '|'
			) {
				return false;
			}
		}
		return true;
	}
	return false;
}
