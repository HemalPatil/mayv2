#pragma once

#include <stddef.h>
#include <stdint.h>

extern size_t strlen(const char* str);
extern int64_t strcmp(const char* str1, const char* str2);
extern void* memcpy(void *dest, void *src, size_t n);
extern void* memset(void *address, int data, size_t length);