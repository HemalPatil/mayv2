#include <cstring>
#include <drivers/filesystems.h>
#include <terminal.h>
#include <unicode.h>

static const char* const malformedStr = "FS::isValidAbsolutePath malformed absolute path [";

std::vector<std::shared_ptr<Drivers::FS::Base>> Drivers::FS::filesystems;
std::shared_ptr<Drivers::FS::Base> Drivers::FS::root;

Async::Thenable<Drivers::FS::OpenFileResult>
Drivers::FS::Base::openFile(const std::string &absolutePath, OpenFileType openType) {
	OpenFileResult errorResult;
	if (!isValidAbsolutePath(absolutePath, false)) {
		errorResult.status = Status::InvalidPath;
		co_return std::move(errorResult);
	}
	const auto parentSplit = splitParentDirectory(absolutePath);
	const auto parentDirResult = co_await this->readDirectory(std::get<0>(parentSplit));
	if (parentDirResult.status != Status::Ok) {
		errorResult.status = parentDirResult.status;
		co_return std::move(errorResult);
	}
	std::shared_ptr<Node> fileNode;
	for (const auto &child : parentDirResult.directory->children) {
		if (child->type & NodeType::File) {
			
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
