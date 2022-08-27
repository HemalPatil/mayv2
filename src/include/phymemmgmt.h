#pragma once

#include <acpi.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PhyMemEntry_Free 0
#define PhyMemEntry_Used 1

#define L32_IDENTITY_MAP_SIZE 32
#define PHY_MEM_BUDDY_MAX_ORDER 10

struct PhyMemBuddyBitmapIndex {
	size_t byte;
	size_t bit;
};
typedef struct PhyMemBuddyBitmapIndex PhyMemBuddyBitmapIndex;

extern const size_t pageSize;
extern const size_t pageSizeShift;

extern ACPI3Entry *mmap;
extern uint64_t pageAddressMasks[PHY_MEM_BUDDY_MAX_ORDER];
extern uint8_t* phyMemBuddyBitmaps[PHY_MEM_BUDDY_MAX_ORDER];
extern size_t phyMemBuddySizes[PHY_MEM_BUDDY_MAX_ORDER];
extern size_t phyMemPagesAvailableCount;
extern size_t phyMemPagesTotalCount;
extern size_t phyMemTotalSize;
extern size_t phyMemUsableSize;

extern bool arePhysicalBuddiesOfType(void* address, size_t buddyLevel, size_t buddyCount, uint8_t type);
extern PhyMemBuddyBitmapIndex getPhysicalPageBuddyBitmapIndex(void* address, size_t buddyLevel);
extern void initMmap();
extern void initPhysicalMemorySize();
extern void initUsablePhysicalMemorySize();
extern bool initializePhysicalMemory(
	void* usableMemoryStart,
	size_t kernelVirtualMemorySize,
	void* kernelElfBase,
	size_t kernelElfSize,
	size_t *phyMemBuddyPagesCount
);
extern void listMmapEntries();
extern void listUsedPhysicalBuddies(size_t buddyLevel);
extern void markPhysicalPages(void* address, size_t pageCount, uint8_t type);