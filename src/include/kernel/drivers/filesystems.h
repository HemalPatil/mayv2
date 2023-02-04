#pragma once

#include <drivers/storage/blockdevice.h>
#include <memory>
#include <string>
#include <vector>

namespace FS {
	struct [[nodiscard]] DirectoryEntry {
		std::string name;
		bool isFile;
		bool isDir;
		bool isSymLink;
		size_t lba;
		size_t size;
	};

	class BaseFS {
		protected:
			std::shared_ptr<Storage::BlockDevice> device;

		public:
			BaseFS(std::shared_ptr<Storage::BlockDevice> blockDevice) : device(blockDevice) {}

			// Reads directory at given absolute path ending in '/'
			// Returns empty vector if given path cannot be resolved to a valid directory
			virtual Async::Thenable<std::vector<DirectoryEntry>> readDirectory(const std::string &absolutePath) = 0;
	};

	extern std::vector<std::shared_ptr<BaseFS>> filesystems;

	std::vector<std::string> splitAbsolutePath(const std::string &absolutePath, bool isDir = false);
}
