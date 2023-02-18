#pragma once

#include <drivers/storage/blockdevice.h>
#include <memory>
#include <string>
#include <vector>

namespace FS {
	enum Status : uint32_t {
		Ok = 0,
		IOError = 1,
		NotDirectory = 2,
		NotFile = 4,
		NoSuchDirectoryOrFile = 8,
		NoAccess = 16,
		Locked = 32
	};

	struct [[nodiscard]] DirectoryEntry {
		std::string name;
		bool isFile;
		bool isDir;
		bool isSymLink;
		size_t lba;
		size_t size;
	};

	struct ReadDirectoryResult {
		Status status = Status::Ok;
		std::vector<DirectoryEntry> entries;
	};

	class BaseFS {
		protected:
			std::shared_ptr<Storage::BlockDevice> device;

		public:
			BaseFS(std::shared_ptr<Storage::BlockDevice> blockDevice) : device(blockDevice) {}

			// Reads directory at given absolute path ending in '/'
			virtual Async::Thenable<ReadDirectoryResult> readDirectory(const std::string &absolutePath) = 0;
	};

	extern std::vector<std::shared_ptr<BaseFS>> filesystems;

	std::vector<std::string> splitAbsolutePath(const std::string &absolutePath, bool isDir = false);
}
