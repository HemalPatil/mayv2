#pragma once

#include <async.h>
#include <memory>

namespace Storage {
	class Buffer {
		private:
			void *data = nullptr;
			size_t pageCount = SIZE_MAX;
			uint64_t physicalAddress = UINT64_MAX;

		public:
			Buffer(size_t size, size_t alignAt);
			~Buffer();
			explicit operator bool() const;
			void* getData() const;
			uint64_t getPhysicalAddress() const;
	};

	class BlockDevice {
		protected:
			size_t blockSize = 666;

		public:
			size_t getBlockSize() const;
			virtual Async::Thenable<std::shared_ptr<Buffer>> read(size_t startBlock, size_t blockCount) = 0;
	};
}
