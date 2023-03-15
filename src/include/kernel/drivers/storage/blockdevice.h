#pragma once

#include <async.h>
#include <drivers/storage/buffer.h>

namespace Drivers{
namespace Storage {
	class BlockDevice {
		protected:
			size_t blockSize = 0;

		public:
			size_t getBlockSize() const;
			virtual Async::Thenable<Buffer> read(size_t startBlock, size_t blockCount) = 0;
	};
}
}
