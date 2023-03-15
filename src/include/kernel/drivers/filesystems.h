#pragma once

#include <drivers/storage/blockdevice.h>
#include <map>
#include <memory>
#include <random.h>
#include <string>
#include <tuple>
#include <vector>

namespace Drivers {
namespace FS {
	class BaseFS;

	enum Status : uint64_t {
		Ok = 0,
		IOError = 1 << 0,
		NotDirectory = 1 << 1,
		NotFile = 1 << 2,
		NoSuchDirectoryOrFile = 1 << 3,
		NoAccess = 1 << 4,
		NonWriteable = 1 << 5,
		AlreadyLocked = 1 << 6,
		AlreadyMounted = 1 << 7,
	};

	struct [[nodiscard]] DirectoryEntry {
		std::string name;
		bool isDir;
		bool isFile;
		bool isMounted;
		bool isSymLink;
		size_t offset = SIZE_MAX;
		size_t size = SIZE_MAX;
		std::shared_ptr<BaseFS> mountedFs;
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
			const std::shared_ptr<Storage::BlockDevice> device;
			const size_t deviceBlockSize;
			const size_t blockSize;
			const size_t rootDirOffset;
			const size_t rootDirSize;
			const Random::GUIDv4 guid;
			std::map<std::string, std::vector<DirectoryEntry>> cachedDirectoryEntries;

		public:
			BaseFS(
				std::shared_ptr<Storage::BlockDevice> blockDevice,
				size_t blockSize,
				size_t rootDirOffset,
				size_t rootDirSize
			);
			const Random::GUIDv4& getGuid() const;

			// Reads directory at given absolute path ending in '/'
			virtual Async::Thenable<ReadDirectoryResult> readDirectory(const std::string &absolutePath) = 0;

			// TODO: may have to remove it depending on openFile
			virtual Async::Thenable<ReadFileResult> readFile(const std::string &absolutePath) = 0;
	};

	extern std::vector<std::shared_ptr<BaseFS>> filesystems;
	extern std::shared_ptr<BaseFS> root;

	bool isValidAbsolutePath(const std::string &absolutePath, bool isDir);
	std::vector<std::string> splitAbsolutePath(const std::string &absolutePath);
	std::tuple<std::string, std::string> splitParentDirectory(const std::string &absolutePath);
}
}
