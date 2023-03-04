#include <cstring>
#include <drivers/filesystems.h>
#include <terminal.h>

static const char* const malformedStr = "FS::isValidAbsolutePath malformed absolute path [";

std::vector<std::shared_ptr<FS::BaseFS>> FS::filesystems;
std::shared_ptr<FS::BaseFS> FS::root;

std::tuple<std::string, std::string> FS::splitParentDirectory(const std::string &absolutePath) {
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

std::vector<std::string> FS::splitAbsolutePath(const std::string &absolutePath) {
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

FS::BaseFS::BaseFS(std::shared_ptr<Storage::BlockDevice> blockDevice) : device(blockDevice), guid() {}

const Random::GUIDv4& FS::BaseFS::getGuid() const {
	return this->guid;
}

bool FS::isValidAbsolutePath(const std::string &absolutePath, bool isDir) {
	if (!(
		0 == absolutePath.length() ||
		'/' != absolutePath.at(0) ||
		(isDir && '/' != absolutePath.back()) ||
		(!isDir && '/' == absolutePath.back())
	)) {
		for (size_t i = 1; i < absolutePath.length(); ++i) {
			if (absolutePath.at(i) == '/' && absolutePath.at(i - 1) == '/') {
				return false;
			}
		}
		return true;
	} else {
		terminalPrintString(malformedStr, strlen(malformedStr));
		terminalPrintString(absolutePath.c_str(), absolutePath.length());
		terminalPrintChar(']');
		Kernel::panic();
	}
}
