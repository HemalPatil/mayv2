#pragma once

#include <acpi.h>
#include <cstddef>
#include <cstdint>
#include <kernel.h>

#define L32_IDENTITY_MAP_SIZE 32
#define MEMORY_REQUEST_CONTIGUOUS 1
#define PHY_MEM_BUDDY_MAX_ORDER 10
#define PHY_MEM_FREE 0
#define PHY_MEM_USED 1

class PageRequestResult {
	public:
		void* address = INVALID_ADDRESS;
		size_t allocatedCount = 0;
};

struct PhyMemBuddyBitmapIndex {
	size_t byte;
	size_t bit;
};
typedef struct PhyMemBuddyBitmapIndex PhyMemBuddyBitmapIndex;

extern const size_t pageSize;
extern const size_t pageSizeShift;

extern ACPI3Entry *mmap;
extern uint8_t* phyMemBuddyBitmaps[PHY_MEM_BUDDY_MAX_ORDER];
extern size_t phyMemBuddyBitmapSizes[PHY_MEM_BUDDY_MAX_ORDER];
extern uint64_t phyMemBuddyMasks[PHY_MEM_BUDDY_MAX_ORDER];
extern size_t phyMemBuddySizes[PHY_MEM_BUDDY_MAX_ORDER];

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
extern PageRequestResult requestPhysicalPages(size_t count, uint8_t flags);
