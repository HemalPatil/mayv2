#pragma once

#include <cstddef>
#include <cstdint>

namespace Drivers {
namespace Storage {
	class Buffer {
		private:
			void *data = nullptr;
			size_t pageCount = 0;
			uint64_t physicalAddress = UINT64_MAX;
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
}
}
