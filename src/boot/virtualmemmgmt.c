#include <commonstrings.h>
#include <heapmemmgmt.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <pml4t.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static const uint64_t nonCanonicalStart = (uint64_t)1 << (MAX_VIRTUAL_ADDRESS_BITS - 1);
static const uint64_t nonCanonicalEnd = ~(nonCanonicalStart - 1);

static size_t generalPagesAvailableCount = 0;
static size_t kernelPagesAvailableCount = 0;

VirtualMemNode *generalAddressSpaceList = INVALID_ADDRESS;
VirtualMemNode *kernelAddressSpaceList = INVALID_ADDRESS;
const uint64_t ptMask = (uint64_t)UINT64_MAX - 1024 * (uint64_t)GIB_1 + 1;
const uint64_t pdMask = ptMask + (uint64_t)PML4T_RECURSIVE_ENTRY * (uint64_t)GIB_1;
const uint64_t pdptMask = pdMask + (uint64_t)PML4T_RECURSIVE_ENTRY * (uint64_t)MIB_2;
PML4E* const pml4t = (PML4E*)(pdptMask + (uint64_t)PML4T_RECURSIVE_ENTRY * (uint64_t)KIB_4);
const size_t virtualPageIndexShift = 9;
const uint64_t virtualPageIndexMask = ((uint64_t)1 << virtualPageIndexShift) - 1;

static const char* const initVirMemStr = "Initializing virtual memory management...\n";
static const char* const initVirMemCompleteStr = "Virtual memory management initialized\n\n";
static const char* const markingPml4 = "Marking PML4 page tables as used in physical memory...";
static const char* const movingBuddies = "Mapping physical memory manager in kernel address space...";
static const char* const maxVirAddrMismatch = "Max virtual address bits mismatch. Expected [";
static const char* const gotStr = "] got [";
static const char* const entryCreationFailed = "Failed to create PML4 entry at level ";
static const char* const forAddress = " for address ";
static const char* const removingId = "Removing PML4 identity mapping of first ";
static const char* const mibs = "MiBs...";
static const char* const reservingHeap = "Reserving memory for dynamic memory manager...";
static const char* const pageTablesStr = "Page tables of ";
static const char* const isCanonicalStr = "isCanonical = ";
static const char* const crawlTableHeader = "Level Tables               Physical tables      Indexes\n";
static const char* const addrSpaceStr = " address space list ";
static const char* const addrSpaceHeader = "Base                 Page count           Available Node address\n";
static const char* const creatingLists = "Creating virtual address space lists...";
static const char* const recursiveStr = "Creating PML4 recursive entry...";
static const char* const checkingMaxBits = "Checking max virtual address bits...";

// Initializes virtual memory space for use by higher level dynamic memory manager and other kernel services
bool initializeVirtualMemory(void* usableKernelSpaceStart, size_t kernelLowerHalfSize, size_t phyMemBuddyPagesCount) {
	uint64_t mib1 = 0x100000;
	terminalPrintString(initVirMemStr, strlen(initVirMemStr));

	terminalPrintSpaces4();
	terminalPrintString(checkingMaxBits, strlen(checkingMaxBits));
	if (infoTable->maxLinearAddress != MAX_VIRTUAL_ADDRESS_BITS) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		terminalPrintSpaces4();
		terminalPrintString(maxVirAddrMismatch, strlen(maxVirAddrMismatch));
		terminalPrintDecimal(MAX_VIRTUAL_ADDRESS_BITS);
		terminalPrintString(gotStr, strlen(gotStr));
		terminalPrintDecimal(infoTable->maxLinearAddress);
		terminalPrintChar(']');
		terminalPrintChar('.');
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');

	// Mark all existing identity mapped page tables as marked in physical memory
	terminalPrintSpaces4();
	terminalPrintString(markingPml4, strlen(markingPml4));
	PML4E *pml4tId = (PML4E*) infoTable->pml4tPhysicalAddress;
	markPhysicalPages(pml4tId, 1, PHY_MEM_USED);
	for (size_t i = 0; i < PML4_ENTRY_COUNT; ++i) {
		if (pml4tId[i].present) {
			PDPTE *pdptId = (PDPTE*)((uint64_t)pml4tId[i].physicalAddress << pageSizeShift);
			markPhysicalPages(pdptId, 1, PHY_MEM_USED);
			for (size_t j = 0; j < PML4_ENTRY_COUNT; ++j) {
				if (pdptId[j].present) {
					PDE *pdtId = (PDE*)((uint64_t)pdptId[j].physicalAddress << pageSizeShift);
					markPhysicalPages(pdtId, 1, PHY_MEM_USED);
					for (size_t k = 0; k < PML4_ENTRY_COUNT; ++k) {
						if (pdtId[k].present) {
							PTE *ptId = (PTE*)((uint64_t)pdtId[k].physicalAddress << pageSizeShift);
							markPhysicalPages(ptId, 1, PHY_MEM_USED);
						}
					}
				}
			}
		}
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// loader32 has identity mapped first L32_IDENTITY_MAP_SIZE MiBs making things easy till now
	// Do recursive mapping to make things easier when manipulating page tables
	// Use entry 510 in PML4T to recursively map to itself since entry 511
	// i.e. last 512 GiB entry will be used by kernel
	// This causes address range from 0xffff ff00 0000 0000 to 0xffff ff80 0000 0000
	// i.e. the second last 512 GiB of virtual address space to be used
	// which must be marked as used in the virtual memory list later
	terminalPrintSpaces4();
	terminalPrintString(recursiveStr, strlen(recursiveStr));
	PML4E *root = (PML4E*)infoTable->pml4tPhysicalAddress;
	root[PML4T_RECURSIVE_ENTRY].present = root[PML4T_RECURSIVE_ENTRY].readWrite = 1;
	root[PML4T_RECURSIVE_ENTRY].physicalAddress = infoTable->pml4tPhysicalAddress >> pageSizeShift;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Map physical memory buddy bitmap to kernel address space
	terminalPrintSpaces4();
	terminalPrintString(movingBuddies, strlen(movingBuddies));
	if (!mapVirtualPages(usableKernelSpaceStart, phyMemBuddyBitmaps[0], phyMemBuddyPagesCount)) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	phyMemBuddyBitmaps[0] = (uint8_t*)usableKernelSpaceStart;
	for (size_t i = 1; i < PHY_MEM_BUDDY_MAX_ORDER; ++i) {
		phyMemBuddyBitmaps[i] = (uint8_t*)((uint64_t)phyMemBuddyBitmaps[i - 1] + phyMemBuddyBitmapSizes[i - 1]);
	}
	usableKernelSpaceStart = (void*)((uint64_t)phyMemBuddyBitmaps[0] + phyMemBuddyPagesCount * pageSize);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Remove identity mapping for virtual addresses from 1MiB to L32_IDENTITY_MAP_SIZE MiBs,
	// scratch memory from L32K64_SCRATCH_BASE of length L32K64_SCRATCH_LENGTH
	// and mapping for lower half of kernel
	terminalPrintSpaces4();
	terminalPrintString(removingId, strlen(removingId));
	terminalPrintDecimal(L32_IDENTITY_MAP_SIZE);
	terminalPrintString(mibs, strlen(mibs));
	if (
		!unmapVirtualPages((void*)mib1, (L32_IDENTITY_MAP_SIZE - 1) * mib1 / pageSize, false) ||
		!unmapVirtualPages((void*)L32K64_SCRATCH_BASE, L32K64_SCRATCH_LENGTH / pageSize, false) ||
		!unmapVirtualPages((void*)KERNEL_LOWERHALF_ORIGIN, kernelLowerHalfSize / pageSize, false)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Create new heap region of size HEAP_NEW_REGION_SIZE
	// and reserve an entry table for this heap
	terminalPrintSpaces4();
	terminalPrintString(reservingHeap, strlen(reservingHeap));
	heapRegionsList = usableKernelSpaceStart;
	PageRequestResult requestResult = requestPhysicalPages(HEAP_NEW_REGION_SIZE / pageSize, 0);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != (HEAP_NEW_REGION_SIZE / pageSize) ||
		!mapVirtualPages(usableKernelSpaceStart, requestResult.address, requestResult.allocatedCount
	)) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	memset(heapRegionsList, 0, HEAP_NEW_REGION_SIZE);
	usableKernelSpaceStart = (void*)((uint64_t)usableKernelSpaceStart + HEAP_NEW_REGION_SIZE);
	const size_t entryTableSize = HEAP_NEW_REGION_SIZE / (sizeof(HeapEntry) + HEAP_MIN_BLOCK_SIZE);
	requestResult = requestPhysicalPages(entryTableSize / pageSize, 0);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != (entryTableSize / pageSize) ||
		!mapVirtualPages(usableKernelSpaceStart, requestResult.address, requestResult.allocatedCount)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	heapRegionsList->entryTable = usableKernelSpaceStart;
	usableKernelSpaceStart = (void*)((uint64_t)usableKernelSpaceStart + entryTableSize);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Create a ghost page which will contain virtual address space lists
	// i.e. this page is mapped in kernel space but no entry for it exists in the kernel space list
	// Dynamic memory manager will take care of making these lists
	// dynamic during its initialization and unmap the ghost page
	// Used areas of virtual address space so far
	// 1) 0x0 to L32K64_SCRATCH_BASE
	// 2) (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) to 1MiB
	// 3) PML4 recursive mapping
	// 4) KERNEL_HIGHERHALF_ORIGIN to usableKernelSpaceStart
	terminalPrintSpaces4();
	terminalPrintString(creatingLists, strlen(creatingLists));
	requestResult = requestPhysicalPages(1, 0);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 1 ||
		!mapVirtualPages(usableKernelSpaceStart, requestResult.address, requestResult.allocatedCount)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	VirtualMemNode *current = (VirtualMemNode*)usableKernelSpaceStart;
	// Used kernel space
	kernelAddressSpaceList = current;
	kernelAddressSpaceList->available = false;
	kernelAddressSpaceList->base = (void*) KERNEL_HIGHERHALF_ORIGIN;
	kernelAddressSpaceList->pageCount = ((uint64_t)usableKernelSpaceStart - KERNEL_HIGHERHALF_ORIGIN) / pageSize;
	kernelAddressSpaceList->next = ++current;
	kernelAddressSpaceList->previous = NULL;
	// Available kernel space
	current->available = true;
	current->base = usableKernelSpaceStart;
	kernelPagesAvailableCount =
		current->pageCount =
		((uint64_t)UINT64_MAX - (uint64_t)usableKernelSpaceStart + 1) / pageSize;
	current->next = NULL;
	current->previous = kernelAddressSpaceList;
	++current;
	// General 0 to L32K64_SCRATCH_BASE used
	generalAddressSpaceList = current;
	generalAddressSpaceList->available = false;
	generalAddressSpaceList->base = 0;
	generalAddressSpaceList->pageCount = L32K64_SCRATCH_BASE / pageSize;
	generalAddressSpaceList->next = ++current;
	generalAddressSpaceList->previous = NULL;
	// General L32K64_SCRATCH_BASE to (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) available
	current->available = true;
	current->base = (void*) L32K64_SCRATCH_BASE;
	current->pageCount = L32K64_SCRATCH_LENGTH / pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	generalPagesAvailableCount += current->pageCount;
	++current;
	// General (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) to 1MiB used
	current->available = false;
	current->base = (void*)(L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH);
	current->pageCount = (mib1 - L32K64_SCRATCH_BASE - L32K64_SCRATCH_LENGTH)/pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	++current;
	// General 1MiB to valid lower half canonical address available
	current->available = true;
	current->base = (void*) mib1;
	current->pageCount = (nonCanonicalStart - mib1) / pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	generalPagesAvailableCount += current->pageCount;
	++current;
	// Mark non canonical address range as used
	current->available = false;
	current->base = (void*) nonCanonicalStart;
	current->pageCount = (nonCanonicalEnd - nonCanonicalStart) / pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	++current;
	// General valid higher half canonical address to PML4 recursive map available
	current->available = true;
	current->base = (void*) nonCanonicalEnd;
	current->pageCount = (ptMask - nonCanonicalEnd) / pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	generalPagesAvailableCount += current->pageCount;
	++current;
	// PML4 recursive map used
	current->available = false;
	current->base = (void*) ptMask;
	current->pageCount = ((uint64_t)512 * GIB_1) / pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	++current;
	// General PML4 recursive map to KERNEL_HIGHERHALF_ORIGIN available
	current->available = true;
	current->base = (void*)(ptMask + 512 * GIB_1);
	current->pageCount = ((uint64_t)KERNEL_HIGHERHALF_ORIGIN - ptMask - 512 * GIB_1) / pageSize;
	current->next = NULL;
	current->previous = current - 1;
	generalPagesAvailableCount += current->pageCount;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintString(initVirMemCompleteStr, strlen(initVirMemCompleteStr));
	return true;
}

// Returns a region in the kernel address space that is the closest fit to the number of requested pages
// Returns INVALID_ADDRESS and allocatedCount = 0 if request count is count == 0
// or greater than currently available kernel pages
// Unsafe to call this function until dynamic memory manager is initialized
PageRequestResult requestVirtualPages(size_t count, uint8_t flags) {
	PageRequestResult result;
	result.address = INVALID_ADDRESS;
	result.allocatedCount = 0;
	if (count > ((flags & MEMORY_REQUEST_KERNEL_PAGE) ? kernelPagesAvailableCount : generalPagesAvailableCount)) {
		return result;
	}
	VirtualMemNode *list = (flags & MEMORY_REQUEST_KERNEL_PAGE) ? kernelAddressSpaceList : generalAddressSpaceList;
	VirtualMemNode *bestFit = NULL, *current = list;
	while (current) {
		if (
			current->available &&
			current->pageCount >= count &&
			(bestFit == NULL || bestFit->pageCount > current->pageCount)
		) {
			bestFit = current;
		}
		current = current->next;
	}
	if (flags & MEMORY_REQUEST_CONTIGUOUS) {
		bestFit->available = false;
		if (bestFit->pageCount != count) {
			VirtualMemNode *newNode = kernelMalloc(sizeof(VirtualMemNode));
			newNode->available = true;
			newNode->base = (void*)((uint64_t)bestFit->base + count * pageSize);
			newNode->pageCount = bestFit->pageCount - count;
			newNode->next = bestFit->next;
			newNode->previous = bestFit;
			bestFit->next = newNode;
			if (newNode->next) {
				newNode->next->previous = newNode;
			}
			bestFit->pageCount = count;
		}
		if (flags & MEMORY_REQUEST_KERNEL_PAGE) {
			kernelPagesAvailableCount -= count;
		} else {
			generalPagesAvailableCount -= count;
		}
		result.address = bestFit->base;
		result.allocatedCount = count;

		// Merge blocks of same type in the list
		current = list;
		while(current->next) {
			if (current->available == current->next->available) {
				// Merge blocks
				VirtualMemNode *nextBlock = current->next;
				current->pageCount += nextBlock->pageCount;
				current->next = nextBlock->next;
				if (nextBlock->next) {
					nextBlock->next->previous = current;
				}
				kernelFree(nextBlock);
			} else {
				// Move to the next block
				current = current->next;
			}
		}

		if (flags & MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE) {
			size_t total = 0;
			while (total != count) {
				PageRequestResult phyResult = requestPhysicalPages(count - total, 0);
				if (phyResult.address == INVALID_ADDRESS || phyResult.allocatedCount == 0) {
					// Out of memory
					// TODO: should swap out pages instead of panicking
					terminalPrintString(outOfMemoryStr, strlen(outOfMemoryStr));
					kernelPanic();
				}
				mapVirtualPages(
					(void*)((uint64_t)result.address + total * pageSize),
					phyResult.address,
					phyResult.allocatedCount
				);
				total += phyResult.allocatedCount;
			}
		}
	} else {
		// FIXME: serve non-contiguous virtual addresses
		// is this case even needed?
	}
	return result;
}

// Maps virtual pages to physical pages
// It is assumed that all virtual pages and physical pages are contiguous, reserved, pageSize boundary aligned
// and within bounds of physical memory and canonical virtual address space before calling this function
// Returns true on successful mapping
bool mapVirtualPages(void* virtualAddress, void* physicalAddress, size_t count) {
	uint64_t phyAddr = (uint64_t)physicalAddress;
	uint64_t virAddr = (uint64_t)virtualAddress;

	// Ensure both physical and virtual address are pageSize boundary aligned
	if (phyAddr & ~phyMemBuddyMasks[0] || virAddr & ~phyMemBuddyMasks[0]) {
		return false;
	}

	PageRequestResult requestResult;
	PML4CrawlResult crawlResult;
	for (size_t i = 0; i < count; ++i, phyAddr += pageSize, virAddr += pageSize) {
		crawlResult = crawlPageTables((void*)virAddr);
		if (!crawlResult.isCanonical) {
			return false;
		}
		for (size_t j = 3; j >= 1; --j) {
			if (crawlResult.physicalTables[j] == INVALID_ADDRESS) {
				// Create a new page table if entry is not present
				requestResult = requestPhysicalPages(1, 0);
				if (requestResult.address == INVALID_ADDRESS || requestResult.allocatedCount != 1) {
					// TODO: should swap out a physical page instead of panicking
					terminalPrintString(entryCreationFailed, strlen(entryCreationFailed));
					terminalPrintDecimal(j);
					terminalPrintString(forAddress, strlen(forAddress));
					terminalPrintHex(&virAddr, sizeof(virAddr));
					terminalPrintChar('\n');
					terminalPrintString(outOfMemoryStr, strlen(outOfMemoryStr));
					kernelPanic();
				}
				crawlResult.tables[j + 1][crawlResult.indexes[j + 1]].present = 1;
				crawlResult.tables[j + 1][crawlResult.indexes[j + 1]].readWrite = 1;
				crawlResult.tables[j + 1][crawlResult.indexes[j + 1]].physicalAddress = (uint64_t)requestResult.address >> pageSizeShift;
				memset(crawlResult.tables[j], 0, pageSize);
			}
		}
		if (crawlResult.physicalTables[0] == INVALID_ADDRESS) {
			crawlResult.tables[1][crawlResult.indexes[1]].present = 1;
			// FIXME: should add only appropriate permissions when mapping pages
			crawlResult.tables[1][crawlResult.indexes[1]].readWrite = 1;
			crawlResult.tables[1][crawlResult.indexes[1]].physicalAddress = phyAddr >> pageSizeShift;
		}
	}
	return true;
}

// Unmaps the page table entries for given count starting from virtualAddress
// It is assumed all the virtual addresses are canonical, freed, and pageSize boundary aligned
// If a virtual address is fully resolved,
// the corresponding physical page is also freed if freePhysicalPage == true
// If all the entries in a page table are absent,
// the page table is also freed and marked absent in upper level page table
bool unmapVirtualPages(void* virtualAddress, size_t count, bool freePhysicalPage) {
	uint64_t addr = (uint64_t) virtualAddress;

	// Ensure the virtual addresses are 4KiB page aligned
	if (addr & ~phyMemBuddyMasks[0]) {
		return false;
	}

	PML4CrawlResult crawlResult;
	for (size_t i = 0; i < count; ++i, addr += pageSize) {
		crawlResult = crawlPageTables((void*) addr);
		if (!crawlResult.isCanonical) {
			return false;
		}
		if (crawlResult.physicalTables[0] != INVALID_ADDRESS) {
			// Virtual address is fully resolved
			crawlResult.tables[1][crawlResult.indexes[1]].present = 0;
			if (freePhysicalPage) {
				markPhysicalPages(crawlResult.physicalTables[0], 1, PHY_MEM_FREE);
			}
		}
		for (size_t j = 1; j <= 3; ++j) {
			if (crawlResult.physicalTables[j] == INVALID_ADDRESS) {
				// Skip this level since it is not present in physical memory
				continue;
			}
			// Page table at current level exists
			// Free it if all entries inside it are absent
			bool freePageTable = true;
			for (size_t k = 0; k < PML4_ENTRY_COUNT; ++k) {
				if (crawlResult.tables[j][k].present) {
					freePageTable = false;
					break;
				}
			}
			if (freePageTable) {
				crawlResult.tables[j + 1][crawlResult.indexes[j + 1]].present = 0;
				markPhysicalPages((void*)crawlResult.physicalTables[j], 1, PHY_MEM_FREE);
			}
		}
	}
	return true;
}

// Returns true only if virtual address is canonical i.e. lies in the range
// 0 - 0x00007fffffffffff or 0xffff800000000000 - 0xffffffffffffffff
bool isCanonicalVirtualAddress(void* address) {
	uint64_t addr = (uint64_t) address;
	return (
		addr < nonCanonicalStart ||
		(addr >= nonCanonicalEnd && addr <= UINT64_MAX)
	);
}

// Crawls the PML4T for a virtual address
// and sets the physical and virtual addresses of page tables of a virtual address where 0 <= level <= 3
// physicalTables[4] is always set to pml4t root's physical address for canonical addresses
// tables[0] is always set to INVALID_ADDRESS
// Subsequent levels in physicalTables are set to physical address of a table if it is present in memory,
// and indexes[0] is set to the offset in physical page if an address can be fully resolved
// Address of a level and subsequent lower levels are set to INVALID_ADDRESS in physicalTables
// if mapping while crawling the PML4 structure is not present for that level
// If an address is not canonical all levels in physicalTables are set to INVALID_ADDRESS
PML4CrawlResult crawlPageTables(void *virtualAddress) {
	PML4CrawlResult result;
	result.isCanonical = false;
	uint64_t addr = (uint64_t)virtualAddress & phyMemBuddyMasks[0];
	result.indexes[0] = (uint64_t)virtualAddress - addr;
	addr >>= pageSizeShift;
	for (size_t i = 1; i <= 4; ++i) {
		result.indexes[i] = addr & virtualPageIndexMask;
		addr >>= virtualPageIndexShift;
	}

	result.tables[0] = (PML4E*)INVALID_ADDRESS;
	result.tables[1] = (PML4E*)((uint64_t)ptMask + result.indexes[4] * GIB_1 + result.indexes[3] * MIB_2 + result.indexes[2] * KIB_4);
	result.tables[2] = (PML4E*)((uint64_t)pdMask + result.indexes[4] * MIB_2 + result.indexes[3] * KIB_4);
	result.tables[3] = (PML4E*)((uint64_t)pdptMask + result.indexes[4] * KIB_4);
	result.tables[4] = pml4t;

	result.physicalTables[0] =
	result.physicalTables[1] =
	result.physicalTables[2] =
	result.physicalTables[3] =
	result.physicalTables[4] =
		INVALID_ADDRESS;

	if (isCanonicalVirtualAddress(virtualAddress)) {
		result.isCanonical = true;
		result.physicalTables[4] = (PML4E*) infoTable->pml4tPhysicalAddress;
		for (size_t i = 4; i >= 1; --i) {
			if (result.tables[i][result.indexes[i]].present) {
				result.physicalTables[i - 1] = (void*)((uint64_t)result.tables[i][result.indexes[i]].physicalAddress << pageSizeShift);
			} else {
				break;
			}
		}
	}

	return result;
}

// Debug helper to display result of crawlPageTables for a given virtual address
void displayCrawlPageTablesResult(void *virtualAddress) {
	terminalPrintString(pageTablesStr, strlen(pageTablesStr));
	terminalPrintHex(&virtualAddress, sizeof(virtualAddress));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(isCanonicalStr, strlen(isCanonicalStr));
	PML4CrawlResult result = crawlPageTables(virtualAddress);
	terminalPrintString(result.isCanonical ? trueStr : falseStr, strlen(result.isCanonical ? trueStr : falseStr));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(crawlTableHeader, strlen(crawlTableHeader));
	for (int i = 4; i >= 0; --i) {
		terminalPrintSpaces4();
		terminalPrintDecimal(i);
		terminalPrintSpaces4();
		terminalPrintChar(' ');
		terminalPrintHex(&result.tables[i], sizeof(result.tables[i]));
		terminalPrintChar(' ');
		terminalPrintHex(&result.physicalTables[i], sizeof(result.physicalTables[i]));
		terminalPrintChar(' ');
		terminalPrintHex(&result.indexes[i], sizeof(result.indexes[i]));
		terminalPrintChar('\n');
	}
}

// Debug helper to list all entries in a given virtual address space list
void traverseAddressSpaceList(VirtualMemNode *list, bool forwardDirection) {
	if (list == kernelAddressSpaceList) {
		terminalPrintString("Kernel", 6);
	} else {
		terminalPrintString("General", 7);
	}
	terminalPrintString(addrSpaceStr, strlen(addrSpaceStr));
	terminalPrintHex(&list, sizeof(list));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(pagesAvailableStr, strlen(pagesAvailableStr));
	if (list == kernelAddressSpaceList) {
		terminalPrintHex(&kernelPagesAvailableCount, sizeof(kernelPagesAvailableCount));
	} else {
		terminalPrintHex(&generalPagesAvailableCount, sizeof(generalPagesAvailableCount));
	}
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(addrSpaceHeader, strlen(addrSpaceHeader));
	if (!forwardDirection) {
		while (list->next) {
			list = list->next;
		}
	}
	while (list) {
		terminalPrintSpaces4();
		terminalPrintHex(&list->base, sizeof(list->base));
		terminalPrintChar(' ');
		terminalPrintHex(&list->pageCount, sizeof(list->pageCount));
		terminalPrintChar(' ');
		terminalPrintDecimal(list->available);
		terminalPrintSpaces4();
		terminalPrintSpaces4();
		terminalPrintChar(' ');
		terminalPrintHex(&list, sizeof(list));
		terminalPrintChar('\n');
		list = forwardDirection ? list->next : list->previous;
	}
}
