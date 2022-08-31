#pragma once

#include <acpi.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define L32_IDENTITY_MAP_SIZE 32
#define PHY_MEM_ALLOCATE_CONTIGUOUS 1
#define PHY_MEM_BUDDY_MAX_ORDER 10
#define PHY_MEM_FREE 0
#define PHY_MEM_USED 1

struct PhyMemBuddyBitmapIndex {
	size_t byte;
	size_t bit;
};
typedef struct PhyMemBuddyBitmapIndex PhyMemBuddyBitmapIndex;

struct PhysicalPageRequestResult {
	void* address;
	size_t allocatedCount;
};
typedef struct PhysicalPageRequestResult PhysicalPageRequestResult;

extern const size_t pageSize;
extern const size_t pageSizeShift;

extern ACPI3Entry *mmap;
extern uint8_t* phyMemBuddyBitmaps[PHY_MEM_BUDDY_MAX_ORDER];
extern size_t phyMemBuddyBitmapSizes[PHY_MEM_BUDDY_MAX_ORDER];
extern uint64_t phyMemBuddyMasks[PHY_MEM_BUDDY_MAX_ORDER];
extern size_t phyMemBuddySizes[PHY_MEM_BUDDY_MAX_ORDER];
extern size_t phyMemPagesAvailableCount;
extern size_t phyMemPagesTotalCount;
extern size_t phyMemTotalSize;
extern size_t phyMemUsableSize;

extern bool arePhysicalBuddiesOfType(void* address, size_t order, size_t count, uint8_t type);
extern PhyMemBuddyBitmapIndex getPhysicalBuddyBitmapIndex(void* address, size_t order);
extern bool initializePhysicalMemory(
	void* usablePhyMemStart,
	size_t kernelLowerHalfSize,
	size_t kernelHigherHalfSize,
	size_t *phyMemBuddyPagesCount
);
extern void initMmap();
extern void initPhysicalMemorySize();
extern void initUsablePhysicalMemorySize();
extern void listMmapEntries();
extern void listUsedPhysicalBuddies(size_t order);
extern void markPhysicalPages(void* address, size_t count, uint8_t type);
extern PhysicalPageRequestResult requestPhysicalPages(size_t count, uint8_t flags);