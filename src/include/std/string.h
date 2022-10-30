#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void* memcpy(void *dest, const void *src, size_t n);
extern void* memmove(void* dest, const void* src, size_t count);
extern void* memset(void *address, int data, size_t length);
extern int64_t strcmp(const char* str1, const char* str2);
extern size_t strlen(const char* str);

#ifdef __cplusplus
}
#endif
