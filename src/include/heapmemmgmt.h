#pragma once

#include <stdbool.h>
#include <stdint.h>

extern void* heapBase;
extern uint64_t heapSize;

extern bool initializeDynamicMemory();