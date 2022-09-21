#pragma once

#include <elf64.h>
#include <phymemmgmt.h>
#include <pml4t.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_VIRTUAL_ADDRESS_BITS 48
#define MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE 4
#define MEMORY_REQUEST_CACHE_DISABLE 8
#define MEMORY_REQUEST_KERNEL_PAGE 2
#define PML4T_RECURSIVE_ENTRY 510

struct PML4CrawlResult {
	size_t indexes[5];
	bool isCanonical;
	PML4E* physicalTables[5];
	PML4E* tables[5];
	bool cached[5];
};
typedef struct PML4CrawlResult PML4CrawlResult;

struct VirtualMemNode {
	bool available;
	void *base;
	size_t pageCount;
	struct VirtualMemNode *next;
	struct VirtualMemNode *previous;
};
typedef struct VirtualMemNode VirtualMemNode;

extern VirtualMemNode *generalAddressSpaceList;
extern VirtualMemNode *kernelAddressSpaceList;
extern const uint64_t pdMask;
extern const uint64_t pdptMask;
extern PML4E* const pml4t;
extern const uint64_t ptMask;
extern const uint64_t virtualPageIndexMask;
extern const size_t virtualPageIndexShift;

extern PML4CrawlResult crawlPageTables(void *virtualAddress);
extern void displayCrawlPageTablesResult(void *virtualAddress);
extern bool freeVirtualPages(void *virtualAddress, size_t count, uint8_t flags);
extern bool initializeVirtualMemory(void *usableKernelSpaceStart, size_t kernelLowerHalfSize, size_t phyMemBuddyPagesCount);
extern bool isCanonicalVirtualAddress(void *address);
extern bool mapVirtualPages(void *virtualAddress, void *physicalAddress, size_t count, uint8_t flags);
extern PageRequestResult requestVirtualPages(size_t count, uint8_t flags);
extern void traverseAddressSpaceList(uint8_t flags, bool forwardDirection);
extern bool unmapVirtualPages(void *virtualAddress, size_t count, bool freePhysicalPage);