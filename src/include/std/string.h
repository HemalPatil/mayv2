#pragma once

#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

extern int memcmp (const void *addr1, const void *addr2, size_t count);
extern void* memcpy(void *dest, const void *src, size_t count);
extern void* memmove(void *dest, const void *src, size_t count);
extern void* memset(void *address, int value, size_t count);
extern int strcmp(const char* str1, const char* str2);
extern size_t strlen(const char* str);
extern int strncmp(const char *str1, const char *str2, size_t count);

#ifdef __cplusplus
}
#endif
