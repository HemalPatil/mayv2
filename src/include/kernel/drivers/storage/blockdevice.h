#pragma once

#include <async.h>

namespace Storage {
	class Buffer {
		private:
			void *data = nullptr;
			size_t pageCount = 0;
			uint64_t physicalAddress = (uint64_t)INVALID_ADDRESS;
			size_t size = 0;

		public:
			Buffer() = default;
			Buffer(std::nullptr_t);
			Buffer(size_t size, size_t alignAt);
			Buffer(const Buffer &) = delete;
			Buffer(Buffer &&other);
			~Buffer();
			Buffer& operator=(std::nullptr_t);
			Buffer& operator=(const Buffer &) = delete;
			Buffer& operator=(Buffer &&other);
			explicit operator bool() const;
			void* getData() const;
			uint64_t getPhysicalAddress() const;
			size_t getSize() const;
	};

	class BlockDevice {
		protected:
			size_t blockSize = 0;

		public:
			size_t getBlockSize() const;
			virtual Async::Thenable<Buffer> read(size_t startBlock, size_t blockCount) = 0;
	};
}
