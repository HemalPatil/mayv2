#pragma once

#include <drivers/storage/blockdevice.h>
#include <memory>
#include <random.h>
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

	struct ReadFileResult {
		Status status = Status::Ok;
		Storage::Buffer data;
	};

	class BaseFS {
		protected:
			std::shared_ptr<Storage::BlockDevice> device;
			Random::GUIDv4 guid;

		public:
			BaseFS(std::shared_ptr<Storage::BlockDevice> blockDevice);
			const Random::GUIDv4& getGuid() const;

			// Reads directory at given absolute path ending in '/'
			virtual Async::Thenable<ReadDirectoryResult> readDirectory(const std::string &absolutePath) = 0;

			// TODO: may have to remove it depending on openFile
			virtual Async::Thenable<ReadFileResult> readFile(const std::string &absolutePath) = 0;
	};

	extern std::vector<std::shared_ptr<BaseFS>> filesystems;
	extern std::shared_ptr<BaseFS> root;

	std::vector<std::string> splitAbsolutePath(const std::string &absolutePath, bool isDir = false);
}
