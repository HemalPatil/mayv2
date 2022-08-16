#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

void* memcpy(void *dest, void *src, size_t n) {
	// Copy in blocks of 8 because it's most efficient in 64-bit mode
	// Copy the rest in bytes
	if (dest == src) {
		return;
	}
	uint64_t *d8 = dest;
	uint64_t *s8 = src;
	size_t iters = n / sizeof(uint64_t);
	size_t remaining = n - iters * sizeof(uint64_t);
	bool isCopyUp = d8 < s8;
	if (isCopyUp) {
		for (size_t i = 0; i < iters; ++i, ++d8, ++s8) {
			*d8 = *s8;
		}
		uint8_t *d = d8;
		uint8_t *s = s8;
		for (size_t i = 0; i < remaining; ++i, ++d, ++s) {
			*d = *s;
		}
	}
	// TODO: copy down not implemented
}

void* memset(void *address, int data, size_t length) {
	uint64_t *a8 = address;
	uint64_t d8 = 0;
	uint8_t d = data & 0xff;
	for (size_t i = 0; i < 8; ++i) {
		d8 |= d;
		d8 <<= 8;
	}
	size_t iters = length / sizeof(uint64_t);
	size_t remaining = length - iters * sizeof(uint64_t);
	for (size_t i = 0; i < iters; ++i, ++a8) {
		*a8 = d8;
	}
	uint8_t *a = a8;
	for (size_t i = 0; i < remaining; ++i, ++a) {
		*a = d;
	}
	return address;
}