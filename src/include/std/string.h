#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int memcmp (const void *str1, const void *str2, size_t count);
extern void* memcpy(void *dest, const void *src, size_t n);
extern void* memmove(void* dest, const void* src, size_t count);
extern void* memset(void *address, int data, size_t length);
extern int strcmp(const char* str1, const char* str2);
extern size_t strlen(const char* str);
extern int strncmp(const char *str1, const char *str2, size_t count);

#ifdef __cplusplus
}
#endif
