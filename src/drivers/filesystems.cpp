#include <cstring>
#include <drivers/filesystems.h>
#include <terminal.h>

static const char* const malformedStr = "FS::splitAbsolutePath malformed absolute path [";

static void malformedAbsolutePath(const std::string &absolutePath);

std::vector<std::shared_ptr<FS::BaseFS>> FS::filesystems;

std::vector<std::string> FS::splitAbsolutePath(const std::string &absolutePath) {
	if (absolutePath.length() == 0 || '/' != absolutePath[0]) {
		malformedAbsolutePath(absolutePath);
	}
	std::vector<std::string> pathParts;
	size_t pos = 1;
	size_t currentPos = 1;
	for (; currentPos < absolutePath.length(); ++currentPos) {
		if (absolutePath[currentPos] == '/') {
			if (currentPos == pos) {
				malformedAbsolutePath(absolutePath);
			}
			pathParts.push_back(absolutePath.substr(pos, currentPos - pos));
			pos = currentPos + 1;
		}
	}
	if (absolutePath.back() != '/') {
		pathParts.push_back(absolutePath.substr(pos));
	}
	return pathParts;
}

static void malformedAbsolutePath(const std::string &absolutePath) {
	terminalPrintString(malformedStr, strlen(malformedStr));
	terminalPrintString(absolutePath.c_str(), absolutePath.length());
	terminalPrintChar(']');
	Kernel::panic();
}
