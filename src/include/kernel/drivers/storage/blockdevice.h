#pragma once

#include <cstddef>
#include <memory>
#include <promise.h>

namespace Storage {
	class BlockDevice {
		protected:
			size_t blockSize = 0;

		public:
			size_t getBlockSize() const;
			virtual std::shared_ptr<Kernel::Promise<bool>> read(size_t startBlock, size_t blockCount, void *buffer) = 0;
	};
}
