#pragma once

#include <drivers/storage/blockdevice.h>
#include <memory>
#include <random.h>
#include <string>
#include <tuple>
#include <vector>

namespace Drivers {
namespace FS {
	class Base;
	struct FileDescriptor;
	struct Node;

	enum Status : uint64_t {
		Ok = 0,
		InvalidPath = 1 << 0,
		IOError = 1 << 1,
		NotDirectory = 1 << 2,
		NotFile = 1 << 3,
		NoSuchDirectoryOrFile = 1 << 4,
		NoAccess = 1 << 5,
		NonWritable = 1 << 6,
		AlreadyLocked = 1 << 7,
		AlreadyMounted = 1 << 8,
		OutOfBounds = 1 << 9
	};

	enum NodeType : uint64_t {
		None = 0,
		Directory = 1 << 0,
		File = 1 << 1,
		MountPoint = 1 << 2,
		BlockDevice = 1 << 3,
		Process = 1 << 4,
	};

	enum OpenFileType : uint64_t {
		Read = 0,
		CreateIfNotExists = 1 << 0,
		Write = 1 << 1,
		Append = 1 << 2,
		SeekEnd = 1 << 3,
	};

	struct FileDescriptor {
		const std::shared_ptr<Node> node;
		const OpenFileType openType;
	};

	struct FileBuffer {
		size_t base = SIZE_MAX;
		bool isBusy = false;	// TODO: Change to mutex and use it while reading/writing
		Storage::Buffer buffer;
	};

	struct Node {
		std::string name;
		NodeType type = NodeType::None;
		size_t offset = SIZE_MAX;
		size_t size = SIZE_MAX;
		std::shared_ptr<Base> fs;
		std::shared_ptr<Node> mountedNode;
		std::shared_ptr<Storage::BlockDevice> blockDevice;
		std::shared_ptr<Node> parent;
		bool childrenCreated = false;
		std::vector<std::shared_ptr<Node>> children;
		std::vector<std::shared_ptr<FileDescriptor>> openFileDescriptors;
		std::vector<FileBuffer> fileBuffers;
		bool isLocked = false;
		bool isBusy = false;	// TODO: Change to mutex and use it while manipulating children, descriptors, buffers
	};

	struct InfoResult {
		Status status = Status::Ok;
		std::shared_ptr<Node> node;
	};

	struct OpenFileResult {
		Status status = Status::Ok;
		std::shared_ptr<FileDescriptor> file;
	};

	struct ReadDirectoryResult {
		Status status = Status::Ok;
		std::shared_ptr<Node> directory;
	};

	extern std::vector<std::shared_ptr<Base>> filesystems;
	extern std::shared_ptr<Base> root;

	Async::Thenable<Status> closeFile(const std::shared_ptr<FileDescriptor> &file);

	Async::Thenable<InfoResult> getInfo(
		const std::string &absolutePath,
		const std::shared_ptr<Base> &fs = root
	);

	bool isValidAbsolutePath(const std::string &absolutePath);

	Async::Thenable<OpenFileResult> openFile(
		const std::string &absolutePath,
		const OpenFileType &openType,
		const std::shared_ptr<Base> &fs = root
	);

	// Reads directory at given absolute path ending in '/'
	Async::Thenable<ReadDirectoryResult> readDirectory(
		const std::string &absolutePath,
		const std::shared_ptr<Base> &fs = root
	);

	// Reads count number of bytes starting from offset into the readBuffer provided
	// Assumes the buffer provided is long enough to hold all the bytes requested
	// OutOfBounds status is returned for following conditions:
	// 1. count == 0
	// 2. offset + count > file->size
	Async::Thenable<Status> readFile(
		const std::shared_ptr<FileDescriptor> &file,
		void* const readBuffer,
		size_t offset,
		size_t count
	);

	// Splits an absolute path into components using '/' as the delimiter
	// Assumes the path is a valid absolute path
	std::vector<std::string> splitAbsolutePath(const std::string &absolutePath);

	// Splits an absolute path into parent directory absolute path
	// and file/directory name using '/' as the delimiter
	// Assumes the path is a valid absolute path
	std::tuple<std::string, std::string> splitParentDirectory(const std::string &absolutePath);

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
			virtual Async::Thenable<Status> readDirectory(const std::shared_ptr<Node> &node) = 0;
			virtual Async::Thenable<Storage::Buffer> readFile(
				const std::shared_ptr<Node> &node,
				const size_t offset,
				const size_t count
			) = 0;

		public:
			const Random::GUIDv4& getGuid() const;

		friend Async::Thenable<InfoResult> getInfo(
			const std::string &absolutePath,
			const std::shared_ptr<Base> &fs
		);

		friend Async::Thenable<ReadDirectoryResult> readDirectory(
			const std::string &absolutePath,
			const std::shared_ptr<Base> &fs
		);

		friend Async::Thenable<Status> readFile(
			const std::shared_ptr<FileDescriptor> &file,
			void* const readBuffer,
			size_t offset,
			size_t count
		);
	};
}
}
