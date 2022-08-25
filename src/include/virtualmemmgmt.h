#pragma once

#include <stddef.h>
#include <stdint.h>

extern const size_t virPageSize;

extern bool initializeVirtualMemory(uint64_t higherHalfUsableStart);