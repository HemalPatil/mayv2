#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PhyMemEntry_Free 0
#define PhyMemEntry_Used 1

struct PhyMemBitmapIndex {
	uint32_t byteIndex;
	uint32_t bitIndex;
};
typedef struct PhyMemBitmapIndex PhyMemBitmapIndex;

extern const size_t phyPageSize;

extern size_t phyPagesCount;
extern uint8_t* phyMemBitmap;
extern uint64_t phyMemBitmapSize;
extern uint64_t phyMemUsableSize;
extern uint64_t phyMemTotalSize;

extern PhyMemBitmapIndex getPhysicalPageBitmapIndex(void* address);
extern void initMmap();
extern void initPhysicalMemorySize();
extern void initUsablePhysicalMemorySize();
extern bool initializePhysicalMemory(uint64_t usableMemoryStart);
extern bool isPhysicalPageAvailable(void* address, size_t numberOfPages);
extern void listMmapEntries();
extern void listUsedPhysicalPages();
extern void markPhysicalPagesAsUsed(void* address, size_t numberOfPages);