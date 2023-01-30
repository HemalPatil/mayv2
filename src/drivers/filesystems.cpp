#include <drivers/filesystems.h>
#include <terminal.h>

std::vector<std::shared_ptr<FS::BaseFS>> FS::filesystems;
