#include <commonstrings.h>
#include <heapmemmgmt.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <pml4t.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

#define GIB_1 1024 * 1024 * 1024
#define KIB_4 4 * 1024
#define MIB_2 2 * 1024 * 1024

const uint64_t ptMask = (uint64_t)UINT64_MAX - 1024 * (uint64_t)GIB_1 + 1;
const uint64_t pdMask = ptMask + (uint64_t)PML4T_RECURSIVE_ENTRY * (uint64_t)GIB_1;
const uint64_t pdptMask = pdMask + (uint64_t)PML4T_RECURSIVE_ENTRY * (uint64_t)MIB_2;
PML4E* const pml4t = (PML4E*)(pdptMask + (uint64_t)PML4T_RECURSIVE_ENTRY * (uint64_t)KIB_4);
const size_t virtualPageIndexShift = 9;
const uint64_t virtualPageIndexMask = ((uint64_t)1 << virtualPageIndexShift) - 1;

const char* const initVirMemStr = "Initializing virtual memory management...\n";
const char* const initVirMemCompleteStr = "Virtual memory management initialized\n\n";
const char* const markingPml4 = "Marking PML4 page tables as used in physical memory...";
const char* const movingBuddies = "Mapping physical memory manager in kernel address space...";
const char* const maxVirAddrMismatch = "Max virtual address bits mismatch. Expected [";
const char* const gotStr = "] got [";
const char* const entryCreationFailed = "Failed to create PML4 entry at level ";
const char* const forAddress = " for address ";
const char* const removingId = "Removing identity mapping of first ";
const char* const mibs = "MiBs...";
const char* const reservingHeap = "Reserving memory for dynamic memory manager...";

// Initializes virtual memory space for use by higher level dynamic memory manager and other kernel services
bool initializeVirtualMemory(void* usableKernelSpaceStart, size_t kernelLowerHalfSize, size_t phyMemBuddyPagesCount) {
	// TODO : complete initialize vir mem implementation
	terminalPrintString(initVirMemStr, strlen(initVirMemStr));

	if (infoTable->maxLinearAddress != MAX_VIRTUAL_ADDRESS_BITS) {
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

	// Mark all existing identity mapped page tables as marked in physical memory
	terminalPrintSpaces4();
	terminalPrintString(markingPml4, strlen(markingPml4));
	PML4E *pml4tId = (PML4E*) infoTable->pml4eRootPhysicalAddress;
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
	PML4E *root = (PML4E*)infoTable->pml4eRootPhysicalAddress;
	root[PML4T_RECURSIVE_ENTRY].present = root[PML4T_RECURSIVE_ENTRY].readWrite = 1;
	root[PML4T_RECURSIVE_ENTRY].physicalAddress = infoTable->pml4eRootPhysicalAddress >> pageSizeShift;

	// Map physical memory buddy bitmap to higher half
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
	uint64_t mib1 = 0x100000;
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

	// TODO: Reserve heapSize amount memory for dynamic memory manager
	// Reserve in batches of 2MiBs because physical memory manager
	// currently doesn't support requesting more than 2MiB at once
	terminalPrintSpaces4();
	terminalPrintString(reservingHeap, strlen(reservingHeap));
	heapBase = usableKernelSpaceStart;
	PhysicalPageRequestResult requestResult;
	for (uint64_t i = 0; i < heapSize; i += 2 * mib1, usableKernelSpaceStart = (void*)((uint64_t)usableKernelSpaceStart + 2 * mib1)) {
		requestResult = requestPhysicalPages(2 * mib1 / pageSize, 0);
		if (!mapVirtualPages(usableKernelSpaceStart, requestResult.address, requestResult.allocatedCount)) {
			terminalPrintString(failedStr, strlen(failedStr));
			terminalPrintChar('\n');
			return false;
		}
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// TODO: Create virtual address space linked list
	// Used areas of virtual address space
	// 1) 0x0 to L32K64_SCRATCH_BASE
	// 2) (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) to 1MiB
	// 3) KERNEL_HIGHERHALF_ORIGIN to usableKernelSpaceStart

	terminalPrintString(initVirMemCompleteStr, strlen(initVirMemCompleteStr));
	return true;
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

	PhysicalPageRequestResult requestResult;
	PML4CrawlResult crawlResult;
	for (size_t i = 0; i < count; ++i, phyAddr += pageSize, virAddr += pageSize) {
		crawlResult = crawlPageTables((void*)virAddr);
		if (!crawlResult.isCanonical) {
			return false;
		}
		for (size_t j = 4; j >= 2; --j) {
			if (crawlResult.mappedTables[j] != crawlResult.tables[j]) {
				// Create a new page table if entry is not present
				requestResult = requestPhysicalPages(1, 0);
				if (requestResult.address == INVALID_ADDRESS || requestResult.allocatedCount != 1) {
					// TODO: should swap out a physical page instead of panicking
					terminalPrintString(entryCreationFailed, strlen(entryCreationFailed));
					terminalPrintDecimal(j);
					terminalPrintString(forAddress, strlen(forAddress));
					terminalPrintHex(&virAddr, sizeof(virAddr));
					terminalPrintChar('\n');
					kernelPanic();
				}
				crawlResult.tables[j][crawlResult.indexes[j]].present = 1;
				crawlResult.tables[j][crawlResult.indexes[j]].readWrite = 1;
				crawlResult.tables[j][crawlResult.indexes[j]].physicalAddress = (uint64_t)requestResult.address >> pageSizeShift;
			}
		}
		if (crawlResult.mappedTables[1] != crawlResult.tables[1]) {
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
		if (
			crawlResult.mappedTables[2] == crawlResult.tables[2] &&
			crawlResult.mappedTables[1] == crawlResult.tables[1]
		) {
			// Page table is present and virtual address is mapped
			crawlResult.mappedTables[1][crawlResult.indexes[1]].present = 0;
			if (freePhysicalPage) {
				markPhysicalPages(crawlResult.mappedTables[0], 1, PHY_MEM_FREE);
			}
		}
		for (size_t j = 1; j <= 3; ++j) {
			if (crawlResult.mappedTables[j + 1] != crawlResult.tables[j + 1]) {
				// Skip this level since entry for it doesn't exist in the upper level
				continue;
			}
			// Page table at current level exists
			// Free it if all entries inside it are absent
			bool freeEntry = true;
			for (size_t k = 0; k < PML4_ENTRY_COUNT; ++k) {
				if (crawlResult.tables[j][k].present) {
					freeEntry = false;
					break;
				}
			}
			if (freeEntry) {
				crawlResult.mappedTables[j + 1][crawlResult.indexes[j + 1]].present = 0;
				markPhysicalPages(
					(void*)((uint64_t)crawlResult.mappedTables[j + 1][crawlResult.indexes[j + 1]].physicalAddress << pageSizeShift),
					1,
					PHY_MEM_FREE
				);
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
		addr < ((uint64_t)1 << (MAX_VIRTUAL_ADDRESS_BITS - 1)) ||
		(addr >= ~(((uint64_t)1 << (MAX_VIRTUAL_ADDRESS_BITS - 1)) - 1) && addr <= UINT64_MAX)
	);
}

// Crawls the PML4T for a virtual address
// and sets the virtual addresses of page tables of a virtual address where 1 <= level <= 4
// All levels in tables and mappedTables are identical,
// mappedTables[0] is set to physical page address,
// and indexes[0] is set to physical page offset if an address can be fully resolved
// Address of a level and subsequent lower levels are set to INVALID_ADDRESS in mappedTables
// if mapping while crawling the PML4 structure is not present for that level
// If an address is not canonical all levels in mappedTables are set to INVALID_ADDRESS
PML4CrawlResult crawlPageTables(void *virtualAddress) {
	PML4CrawlResult result;
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

	result.mappedTables[0] =
	result.mappedTables[1] =
	result.mappedTables[2] =
	result.mappedTables[3] =
	result.mappedTables[4] =
		INVALID_ADDRESS;

	// Ensure address is canonical
	if (isCanonicalVirtualAddress(virtualAddress)) {
		result.isCanonical = true;
		for (size_t i = 4; i >= 1; --i) {
			if (result.tables[i][result.indexes[i]].present) {
				result.mappedTables[i] = result.tables[i];
			} else {
				goto getPageTableFromPml4IndexesReturn;
			}
		}
		result.mappedTables[0] =
		result.tables[0] =
			(void*)((uint64_t)result.tables[1][result.indexes[1]].physicalAddress << pageSizeShift);
	} else {
		result.isCanonical = false;
	}

	getPageTableFromPml4IndexesReturn:
		return result;
}
