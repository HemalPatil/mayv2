#pragma once

#include <elf64.h>
#include <pml4t.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_VIRTUAL_ADDRESS_BITS 48
#define PML4T_RECURSIVE_ENTRY 510

struct PML4CrawlResult {
	size_t indexes[5];
	bool isCanonical;
	PML4E* mappedTables[5];
	PML4E* tables[5];
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

extern VirtualMemNode *kernelAddressSpaceList;
extern VirtualMemNode *normalAddressSpaceList;
extern const uint64_t pdMask;
extern const uint64_t pdptMask;
extern PML4E* const pml4t;
extern const uint64_t ptMask;
extern const uint64_t virtualPageIndexMask;
extern const size_t virtualPageIndexShift;

extern PML4CrawlResult crawlPageTables(void *virtualAddress);
extern void displayCrawlPageTablesResult(void *virtualAddress);
extern bool initializeVirtualMemory(void* usableKernelSpaceStart, size_t kernelLowerHalfSize, size_t phyMemBuddyPagesCount);
extern bool isCanonicalVirtualAddress(void* address);
extern void listKernelAddressSpace();
extern void listNormalAddressSpace();
extern bool mapVirtualPages(void* virtualAddress, void* physicalAddress, size_t count);
extern bool unmapVirtualPages(void* virtualAddress, size_t count, bool freePhysicalPage);