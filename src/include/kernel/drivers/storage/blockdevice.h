#pragma once

#include <cstddef>
#include <functional>

namespace Storage {
	class BlockDevice {
		protected:
			size_t blockSize = 0;

		public:
			using CommandCallback = std::function<void()>;

			size_t getBlockSize() const;
			virtual bool read(size_t startBlock, size_t blockCount, void *buffer, const CommandCallback &callback) = 0;
	};
}
