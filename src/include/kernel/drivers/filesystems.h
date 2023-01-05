#pragma once

#include <drivers/storage/blockdevice.h>
#include <string>
#include <vector>

namespace FS {
	class DirectoryEntry {
		public:
			const std::string name;
			const bool isFile;
			const bool isDir;
			const bool isSymLink;

			DirectoryEntry(const std::string &name, bool isFile, bool isDir, bool isSymLink);
			// DirectoryEntry(const DirectoryEntry &dir);
	};

	class BaseFS {
		protected:
			std::shared_ptr<Storage::BlockDevice> device;

		public:
			BaseFS(std::shared_ptr<Storage::BlockDevice> blockDevice) : device(blockDevice) {}
			virtual std::vector<DirectoryEntry> readDirectory(const std::string &name) = 0;
	};

	extern std::vector<std::shared_ptr<BaseFS>> filesystems;
}
