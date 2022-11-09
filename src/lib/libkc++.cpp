#include <bits/allocator.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <kernel.h>
#include <terminal.h>

static const char* const invalidArgStr = "\nInvalid argument exception\n";
static const char* const logicErrorStr = "\nLogic error exception\n";
static const char* const lengthErrorStr = "\nLength error exception\n";
static const char* const outOfRangeStr = "\nOut of range exception\n";
static const char* const outOfRangeFmtStr = "\nOut of range format exception\n";
static const char* const badFunctionCallStr = "\nBad function call exception\n";
static const char* const badAllocStr = "\nBad alloc exception\n";
static const char* const badArrayStr = "\nBad array exception\n";
static const char* const systemErrorStr = "\nSystem exception\n";

extern "C" {

size_t strlen(const char* str) {
	size_t length = 0;
	while (*str++) {
		++length;
	}
	return length;
}

int64_t strcmp(const char* str1, const char* str2) {
	while (*str1 && (*str1 == *str2)) {
		++str1;
		++str2;
	}
	return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

void* memcpy(void *dest, const void *src, size_t n) {
	// Copies from front to back
	// Copy in blocks of 8 because it's most efficient in 64-bit mode
	// Copy the rest in bytes
	// Need not worry about overlapping regions
	// TODO: probably can be improved by accessing at 8 byte boundaries first
	// but it becomes really messy really fast
	if (dest == src) {
		return dest;
	}
	uint64_t *d8 = (uint64_t*)dest;
	uint64_t *s8 = (uint64_t*)src;
	size_t iters = n / sizeof(uint64_t);
	size_t remaining = n - iters * sizeof(uint64_t);
	for (size_t i = 0; i < iters; ++i, ++d8, ++s8) {
		*d8 = *s8;
	}
	uint8_t *d = (uint8_t *)d8;
	uint8_t *s = (uint8_t *)s8;
	for (size_t i = 0; i < remaining; ++i, ++d, ++s) {
		*d = *s;
	}
	return dest;
}

void* memmove(void* dest, const void* src, size_t count) {
	if (
		(uintptr_t)src < (uintptr_t)dest &&
		(uintptr_t)src + count > (uintptr_t)dest
	) {
		// Copy from back to front
		uint8_t *dest8 = (uint8_t*) dest;
		uint8_t *src8 = (uint8_t*) src;
		for (int64_t i = count - 1; i >= 0; --i) {
			dest8[i] = src8[i];
		}
	} else {
		// Copy from front to back
		memcpy(dest, src, count);
	}
	return dest;
}

void* memset(void *address, int data, size_t length) {
	// Set in blocks of 8 bytes first because it's most efficient in 64-bit mode
	// Set the rest in bytes
	// TODO: probably can be improved by copying at 8 byte boundaries first
	uint64_t *a8 = (uint64_t*)address;
	uint64_t d8 = 0;
	uint8_t d = data & 0xff;
	for (size_t i = 0; i < 8; ++i) {
		d8 <<= 8;
		d8 |= d;
	}
	size_t iters = length / sizeof(uint64_t);
	size_t remaining = length - iters * sizeof(uint64_t);
	for (size_t i = 0; i < iters; ++i, ++a8) {
		*a8 = d8;
	}
	uint8_t *a = (uint8_t *)a8;
	for (size_t i = 0; i < remaining; ++i, ++a) {
		*a = d;
	}
	return address;
}

void *__dso_handle = 0;

int __cxa_atexit(void (*)(void*), void*, void*) {
	// TODO: should register the global destructors in some table
	// Can be avoided since destructing global objects doesn't make much sense
	// because the kernel never exits
	terminalPrintString("cAtExit\n", 8);
	return 0;
};

}

void *operator new(size_t size) {
	return Kernel::Memory::Heap::malloc(size);
}

void *operator new[](size_t size) {
	return Kernel::Memory::Heap::malloc(size);
}

void operator delete(void *memory) {
	Kernel::Memory::Heap::free(memory);
}

void operator delete[](void *memory) {
	Kernel::Memory::Heap::free(memory);
}

// TODO: Do not know the meaning of -Wsized-deallocation but this removes the warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void operator delete(void *memory, size_t size) {
	Kernel::Memory::Heap::free(memory);
}

void operator delete[](void *memory, size_t size) {
	Kernel::Memory::Heap::free(memory);
}
#pragma GCC diagnostic pop

namespace std {
	// Explicit template instantiation of allocator<char> for std::string
	template class allocator<char>;

	// Helpers for exception objects in <system_error>
	void __throw_system_error(int) {
		terminalPrintString(systemErrorStr, strlen(systemErrorStr));
		Kernel::panic();
	}

	// Helpers for exception objects in <stdexcept>
	void __throw_invalid_argument(const char*) {
		terminalPrintString(invalidArgStr, strlen(invalidArgStr));
		Kernel::panic();
	}
	void __throw_logic_error(const char*) {
		terminalPrintString(logicErrorStr, strlen(logicErrorStr));
		Kernel::panic();
	}
	void __throw_length_error(const char*) {
		terminalPrintString(lengthErrorStr, strlen(lengthErrorStr));
		Kernel::panic();
	}
	void __throw_out_of_range(const char*) {
		terminalPrintString(outOfRangeStr, strlen(outOfRangeStr));
		Kernel::panic();
	}
	void __throw_out_of_range_fmt(const char*, ...) {
		terminalPrintString(outOfRangeFmtStr, strlen(outOfRangeFmtStr));
		Kernel::panic();
	}

	// Helpers for exception objects in <functional>
	void __throw_bad_function_call() {
		terminalPrintString(badFunctionCallStr, strlen(badFunctionCallStr));
		Kernel::panic();
	}

	// Helper for exception objects in <new>
	void __throw_bad_alloc(void) {
		terminalPrintString(badAllocStr, strlen(badAllocStr));
		Kernel::panic();
	}
	void __throw_bad_array_new_length(void) {
		terminalPrintString(badArrayStr, strlen(badArrayStr));
		Kernel::panic();
	}
}
