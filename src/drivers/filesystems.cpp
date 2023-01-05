#include <drivers/filesystems.h>
#include <terminal.h>

std::vector<std::shared_ptr<FS::BaseFS>> FS::filesystems;

FS::DirectoryEntry::DirectoryEntry(
	const std::string &name, bool isFile, bool isDir, bool isSymLink
) : name(name), isFile(isFile), isDir(isDir), isSymLink(isSymLink) {}

// FS::DirectoryEntry::DirectoryEntry(const DirectoryEntry &dir) {
// 	terminalPrintString("dirent copy\n", 12);
// 	Kernel::hangSystem();
// }
