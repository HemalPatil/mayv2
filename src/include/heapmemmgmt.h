#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct HeapHeader {
	size_t entryCount;
	void* latestEntrySearched;
	size_t remaining;
	size_t size;
	struct HeapHeader *next;
	struct HeapHeader *previous;
};
typedef struct HeapHeader HeapHeader;

extern HeapHeader *heapRegionsList;
extern uint64_t initialHeapSize;
extern HeapHeader *latestHeapSearched;

extern bool initializeDynamicMemory();
extern void kernelFree(void *address);
extern void* kernelMalloc(size_t count);
extern void listAllHeapRegions();
