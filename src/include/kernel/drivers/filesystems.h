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
	class Base;
	class Node;

	enum Status : uint64_t {
		Ok = 0,
		InvalidPath = 1 << 0,
		IOError = 1 << 1,
		NotDirectory = 1 << 2,
		NotFile = 1 << 3,
		NoSuchDirectoryOrFile = 1 << 4,
		NoAccess = 1 << 5,
		NonWriteable = 1 << 6,
		AlreadyLocked = 1 << 7,
		AlreadyMounted = 1 << 8,
	};

	enum NodeType : uint64_t {
		None = 0,
		Directory = 1 << 0,
		File = 1 << 1,
		MountPoint = 1 << 2,
		BlockDevice = 1 << 3,
		Process = 1 << 4,
	};

	struct Node {
		std::string name;
		NodeType type = NodeType::None;
		size_t offset = SIZE_MAX;
		size_t size = SIZE_MAX;
		std::shared_ptr<Base> mountedFs;
		std::shared_ptr<Node> parent;
		bool childrenCreated = false;
		std::vector<std::shared_ptr<Node>> children;
	};

	struct ReadDirectoryResult {
		Status status = Status::Ok;
		std::shared_ptr<Node> directory;
	};

	struct ReadFileResult {
		Status status = Status::Ok;
		Storage::Buffer data;
	};

	class Base {
		protected:
			const std::shared_ptr<Storage::BlockDevice> device;
			const size_t deviceBlockSize;
			const size_t blockSize;
			const std::shared_ptr<Node> rootNode;
			const Random::GUIDv4 guid;
			Base(
				std::shared_ptr<Storage::BlockDevice> blockDevice,
				size_t blockSize,
				std::shared_ptr<Node> rootNode
			);

		public:
			const Random::GUIDv4& getGuid() const;

			// Reads directory at given absolute path ending in '/'
			virtual Async::Thenable<ReadDirectoryResult> readDirectory(const std::string &absolutePath) = 0;

			// // TODO: may have to remove it depending on openFile
			// virtual Async::Thenable<ReadFileResult> readFile(const std::string &absolutePath) = 0;
	};

	extern std::vector<std::shared_ptr<Base>> filesystems;
	extern std::shared_ptr<Base> root;

	bool isValidAbsolutePath(const std::string &absolutePath, bool isDir);
	std::vector<std::string> splitAbsolutePath(const std::string &absolutePath);
	std::tuple<std::string, std::string> splitParentDirectory(const std::string &absolutePath);
}
}
