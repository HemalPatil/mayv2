#pragma once

#include <cstddef>
#include <cstdint>

// Represent the strings "FREE" and "USED"
#define HEAP_ENTRY_FREE 0x45455246
#define HEAP_ENTRY_USED 0x44455355

#define HEAP_MIN_BLOCK_SIZE 8

// Must always be 2MiB due to current limitations in physical memory manager
#define HEAP_NEW_REGION_SIZE 0x200000

struct HeapEntry {
	uint32_t signature;
	uint32_t size;
} __attribute__((packed));
typedef struct HeapEntry HeapEntry;

struct HeapHeader {
	size_t entryCount;
	void **entryTable;
	HeapEntry *latestEntrySearched;
	size_t remaining;
	size_t size;
	struct HeapHeader *next;
	struct HeapHeader *previous;
};
typedef struct HeapHeader HeapHeader;

extern HeapHeader *heapRegionsList;

extern bool initializeDynamicMemory();
extern void kernelFree(void *address);
extern void* kernelMalloc(size_t count);
extern void listAllHeapRegions();
