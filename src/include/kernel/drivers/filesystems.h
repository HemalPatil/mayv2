#pragma once

#include <drivers/storage/blockdevice.h>
#include <memory>
#include <string>
#include <vector>

namespace FS {
	struct DirectoryEntry {
		std::string name;
		bool isFile;
		bool isDir;
		bool isSymLink;
	};

	class BaseFS {
		protected:
			std::shared_ptr<Storage::BlockDevice> device;

		public:
			BaseFS(std::shared_ptr<Storage::BlockDevice> blockDevice) : device(blockDevice) {}
			virtual Async::Thenable<std::vector<DirectoryEntry>> readDirectory(const std::string &name) = 0;
	};

	extern std::vector<std::shared_ptr<BaseFS>> filesystems;
}
