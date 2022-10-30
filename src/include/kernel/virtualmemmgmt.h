#pragma once

#include <cstddef>
#include <cstdint>
#include <elf64.h>
#include <phymemmgmt.h>
#include <pml4t.h>

#define MAX_VIRTUAL_ADDRESS_BITS 48
#define MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE 4
#define MEMORY_REQUEST_CACHE_DISABLE 8
#define MEMORY_REQUEST_KERNEL_PAGE 2
#define PML4T_RECURSIVE_ENTRY 510

class PML4CrawlResult {
	public:
		size_t indexes[5];
		bool isCanonical;
		PML4E* physicalTables[5];
		PML4E* tables[5];
		bool cached[5];

		PML4CrawlResult(void* virtualAddress);
};

class VirtualMemNode {
	public:
		bool available = false;
		void *base = INVALID_ADDRESS;
		size_t pageCount = 0;
		VirtualMemNode *next = nullptr;
		VirtualMemNode *previous = nullptr;
};

extern VirtualMemNode *generalAddressSpaceList;
extern VirtualMemNode *kernelAddressSpaceList;
extern const uint64_t pdMask;
extern const uint64_t pdptMask;
extern PML4E* const pml4t;
extern const uint64_t ptMask;
extern const uint64_t virtualPageIndexMask;
extern const size_t virtualPageIndexShift;

extern void displayCrawlPageTablesResult(void *virtualAddress);
extern bool freeVirtualPages(void *virtualAddress, size_t count, uint8_t flags);
extern bool initializeVirtualMemory(void *usableKernelSpaceStart, size_t kernelLowerHalfSize, size_t phyMemBuddyPagesCount);
extern bool isCanonicalVirtualAddress(void *address);
extern bool mapVirtualPages(void *virtualAddress, void *physicalAddress, size_t count, uint8_t flags);
extern PageRequestResult requestVirtualPages(size_t count, uint8_t flags);
extern void traverseAddressSpaceList(uint8_t flags, bool forwardDirection);
extern bool unmapVirtualPages(void *virtualAddress, size_t count, bool freePhysicalPage);
