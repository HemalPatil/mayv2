#include <commonstrings.h>
#include <cstring>
#include <heapmemmgmt.h>
#include <kernel.h>
#include <pml4t.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static const uint64_t nonCanonicalStart = (uint64_t)1 << (MAX_VIRTUAL_ADDRESS_BITS - 1);
static const uint64_t nonCanonicalEnd = ~(nonCanonicalStart - 1);

static size_t generalPagesAvailableCount = 0;
static size_t kernelPagesAvailableCount = 0;

static void defragAddressSpaceList(VirtualMemNode *list);

VirtualMemNode *generalAddressSpaceList = (VirtualMemNode*)INVALID_ADDRESS;
VirtualMemNode *kernelAddressSpaceList = (VirtualMemNode*)INVALID_ADDRESS;
const uint64_t ptMask = (uint64_t)UINT64_MAX - 1024 * (uint64_t)GIB_1 + 1;
const uint64_t pdMask = ptMask + (uint64_t)PML4T_RECURSIVE_ENTRY * (uint64_t)GIB_1;
const uint64_t pdptMask = pdMask + (uint64_t)PML4T_RECURSIVE_ENTRY * (uint64_t)MIB_2;
PML4E* const pml4t = (PML4E*)(pdptMask + (uint64_t)PML4T_RECURSIVE_ENTRY * (uint64_t)KIB_4);
const size_t virtualPageIndexShift = 9;
const uint64_t virtualPageIndexMask = ((uint64_t)1 << virtualPageIndexShift) - 1;

static const char* const initVirMemStr = "Initializing virtual memory management";
static const char* const initVirMemCompleteStr = "Virtual memory management initialized\n\n";
static const char* const markingPml4Str = "Marking PML4 page tables as used in physical memory";
static const char* const movingBuddiesStr = "Mapping physical memory manager in kernel address space";
static const char* const maxVirAddrMismatch = "Max virtual address bits mismatch. Expected [";
static const char* const gotStr = "] got [";
static const char* const entryCreationFailed = "Failed to create PML4 entry at level ";
static const char* const forAddress = " for address ";
static const char* const removingId = "Removing PML4 identity mapping of first ";
static const char* const reservingHeapStr = "Reserving memory for dynamic memory manager";
static const char* const pageTablesStr = "Page tables of ";
static const char* const isCanonicalStr = "isCanonical = ";
static const char* const crawlTableHeader = "L Tables               Physical tables      Indexes              C\n";
static const char* const addrSpaceStr = " address space list ";
static const char* const addrSpaceHeader = "Base                 Page count           Available Node address\n";
static const char* const creatingListsStr = "Creating virtual address space lists";
static const char* const recursiveStr = "Creating PML4 recursive entry";
static const char* const checkingMaxBitsStr = "Checking max virtual address bits";
static const char* const requestErrorStr = "Did not pass MEMORY_REQUEST_CONTIGUOUS to requestVirtualPages\n";

// Initializes virtual memory space for use by higher level dynamic memory manager and other kernel services
bool initializeVirtualMemory(void* usableKernelSpaceStart, size_t kernelLowerHalfSize, size_t phyMemBuddyPagesCount) {
	uint64_t mib1 = 0x100000;
	terminalPrintString(initVirMemStr, strlen(initVirMemStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	terminalPrintSpaces4();
	terminalPrintString(checkingMaxBitsStr, strlen(checkingMaxBitsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
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
	terminalPrintString(markingPml4Str, strlen(markingPml4Str));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	PML4E *pml4tId = (PML4E*) infoTable->pml4tPhysicalAddress;
	Kernel::Memory::Physical::markPages(pml4tId, 1, Kernel::Memory::MarkPageType::Used);
	for (size_t i = 0; i < PML4_ENTRY_COUNT; ++i) {
		if (pml4tId[i].present) {
			PDPTE *pdptId = (PDPTE*)((uint64_t)pml4tId[i].physicalAddress << Kernel::Memory::pageSizeShift);
			Kernel::Memory::Physical::markPages(pdptId, 1, Kernel::Memory::MarkPageType::Used);
			for (size_t j = 0; j < PML4_ENTRY_COUNT; ++j) {
				if (pdptId[j].present) {
					PDE *pdtId = (PDE*)((uint64_t)pdptId[j].physicalAddress << Kernel::Memory::pageSizeShift);
					Kernel::Memory::Physical::markPages(pdtId, 1, Kernel::Memory::MarkPageType::Used);
					for (size_t k = 0; k < PML4_ENTRY_COUNT; ++k) {
						if (pdtId[k].present) {
							PTE *ptId = (PTE*)((uint64_t)pdtId[k].physicalAddress << Kernel::Memory::pageSizeShift);
							Kernel::Memory::Physical::markPages(ptId, 1, Kernel::Memory::MarkPageType::Used);
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
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	PML4E *root = (PML4E*)infoTable->pml4tPhysicalAddress;
	root[PML4T_RECURSIVE_ENTRY].present = root[PML4T_RECURSIVE_ENTRY].readWrite = 1;
	root[PML4T_RECURSIVE_ENTRY].physicalAddress = infoTable->pml4tPhysicalAddress >> Kernel::Memory::pageSizeShift;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Map physical memory buddy bitmap to kernel address space
	terminalPrintSpaces4();
	terminalPrintString(movingBuddiesStr, strlen(movingBuddiesStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!mapVirtualPages(usableKernelSpaceStart, Kernel::Memory::Physical::buddyBitmaps[0], phyMemBuddyPagesCount, 0)) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	Kernel::Memory::Physical::buddyBitmaps[0] = (uint8_t*)usableKernelSpaceStart;
	for (size_t i = 1; i < PHY_MEM_BUDDY_MAX_ORDER; ++i) {
		Kernel::Memory::Physical::buddyBitmaps[i] = (uint8_t*)(
			(uint64_t)Kernel::Memory::Physical::buddyBitmaps[i - 1] +
			Kernel::Memory::Physical::buddyBitmapSizes[i - 1]
		);
	}
	usableKernelSpaceStart = (void*)(
		(uint64_t)Kernel::Memory::Physical::buddyBitmaps[0] +
		phyMemBuddyPagesCount * Kernel::Memory::pageSize
	);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Remove identity mapping for virtual addresses from 1MiB to L32_IDENTITY_MAP_SIZE MiBs,
	// scratch memory from L32K64_SCRATCH_BASE of length L32K64_SCRATCH_LENGTH
	// and mapping for lower half of kernel
	terminalPrintSpaces4();
	terminalPrintString(removingId, strlen(removingId));
	terminalPrintDecimal(L32_IDENTITY_MAP_SIZE);
	terminalPrintString("MiBs", 4);
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (
		!unmapVirtualPages((void*)mib1, (L32_IDENTITY_MAP_SIZE - 1) * mib1 / Kernel::Memory::pageSize, false) ||
		!unmapVirtualPages((void*)L32K64_SCRATCH_BASE, L32K64_SCRATCH_LENGTH / Kernel::Memory::pageSize, false) ||
		!unmapVirtualPages((void*)KERNEL_LOWERHALF_ORIGIN, kernelLowerHalfSize / Kernel::Memory::pageSize, false)
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
	terminalPrintString(reservingHeapStr, strlen(reservingHeapStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	heapRegionsList = (HeapHeader*)usableKernelSpaceStart;
	Kernel::Memory::PageRequestResult requestResult = Kernel::Memory::Physical::requestPages(HEAP_NEW_REGION_SIZE / Kernel::Memory::pageSize, 0);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != (HEAP_NEW_REGION_SIZE / Kernel::Memory::pageSize) ||
		!mapVirtualPages(usableKernelSpaceStart, requestResult.address, requestResult.allocatedCount, 0)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	memset(heapRegionsList, 0, HEAP_NEW_REGION_SIZE);
	usableKernelSpaceStart = (void*)((uint64_t)usableKernelSpaceStart + HEAP_NEW_REGION_SIZE);
	const size_t entryTableSize = HEAP_NEW_REGION_SIZE / (sizeof(HeapEntry) + HEAP_MIN_BLOCK_SIZE);
	requestResult = Kernel::Memory::Physical::requestPages(entryTableSize / Kernel::Memory::pageSize, 0);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != (entryTableSize / Kernel::Memory::pageSize) ||
		!mapVirtualPages(usableKernelSpaceStart, requestResult.address, requestResult.allocatedCount, 0)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	heapRegionsList->entryTable = (void**)usableKernelSpaceStart;
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
	terminalPrintString(creatingListsStr, strlen(creatingListsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	requestResult = Kernel::Memory::Physical::requestPages(1, 0);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 1 ||
		!mapVirtualPages(usableKernelSpaceStart, requestResult.address, requestResult.allocatedCount, 0)
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
	kernelAddressSpaceList->pageCount = ((uint64_t)usableKernelSpaceStart - KERNEL_HIGHERHALF_ORIGIN) / Kernel::Memory::pageSize;
	kernelAddressSpaceList->next = ++current;
	kernelAddressSpaceList->previous = nullptr;
	// Available kernel space
	current->available = true;
	current->base = usableKernelSpaceStart;
	kernelPagesAvailableCount =
		current->pageCount =
		((uint64_t)UINT64_MAX - (uint64_t)usableKernelSpaceStart + 1) / Kernel::Memory::pageSize;
	current->next = nullptr;
	current->previous = kernelAddressSpaceList;
	++current;
	// General 0 to L32K64_SCRATCH_BASE used
	generalAddressSpaceList = current;
	generalAddressSpaceList->available = false;
	generalAddressSpaceList->base = 0;
	generalAddressSpaceList->pageCount = L32K64_SCRATCH_BASE / Kernel::Memory::pageSize;
	generalAddressSpaceList->next = ++current;
	generalAddressSpaceList->previous = nullptr;
	// General L32K64_SCRATCH_BASE to (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) available
	current->available = true;
	current->base = (void*) L32K64_SCRATCH_BASE;
	current->pageCount = L32K64_SCRATCH_LENGTH / Kernel::Memory::pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	generalPagesAvailableCount += current->pageCount;
	++current;
	// General (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) to 1MiB used
	current->available = false;
	current->base = (void*)(L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH);
	current->pageCount = (mib1 - L32K64_SCRATCH_BASE - L32K64_SCRATCH_LENGTH) / Kernel::Memory::pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	++current;
	// General 1MiB to valid lower half canonical address available
	current->available = true;
	current->base = (void*) mib1;
	current->pageCount = (nonCanonicalStart - mib1) / Kernel::Memory::pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	generalPagesAvailableCount += current->pageCount;
	++current;
	// Mark non canonical address range as used
	current->available = false;
	current->base = (void*) nonCanonicalStart;
	current->pageCount = (nonCanonicalEnd - nonCanonicalStart) / Kernel::Memory::pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	++current;
	// General valid higher half canonical address to PML4 recursive map available
	current->available = true;
	current->base = (void*) nonCanonicalEnd;
	current->pageCount = (ptMask - nonCanonicalEnd) / Kernel::Memory::pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	generalPagesAvailableCount += current->pageCount;
	++current;
	// PML4 recursive map used
	current->available = false;
	current->base = (void*) ptMask;
	current->pageCount = ((uint64_t)512 * GIB_1) / Kernel::Memory::pageSize;
	current->next = current + 1;
	current->previous = current - 1;
	++current;
	// General PML4 recursive map to KERNEL_HIGHERHALF_ORIGIN available
	current->available = true;
	current->base = (void*)(ptMask + 512 * GIB_1);
	current->pageCount = ((uint64_t)KERNEL_HIGHERHALF_ORIGIN - ptMask - 512 * GIB_1) / Kernel::Memory::pageSize;
	current->next = nullptr;
	current->previous = current - 1;
	generalPagesAvailableCount += current->pageCount;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintString(initVirMemCompleteStr, strlen(initVirMemCompleteStr));
	return true;
}

// Returns a region in the kernel or general address space depending on MEMORY_REQUEST_KERNEL_PAGE flag
// that is the closest fit to the number of requested pages
// If MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE flag is passed,
// the returned virtual addresses are mapped to newly allocated physical pages
// When MEMORY_REQUEST_CACHE_DISABLE flag is passed, the physical page is marked as cachedDisabled(1) in the PTE
// Returns INVALID_ADDRESS and allocatedCount = 0 if request count is count == 0
// or greater than currently available kernel pages
// Unsafe to call this function until dynamic memory manager is initialized
Kernel::Memory::PageRequestResult requestVirtualPages(size_t count, uint8_t flags) {
	Kernel::Memory::PageRequestResult result;
	if (count > ((flags & MEMORY_REQUEST_KERNEL_PAGE) ? kernelPagesAvailableCount : generalPagesAvailableCount)) {
		return result;
	}
	VirtualMemNode *list = (flags & MEMORY_REQUEST_KERNEL_PAGE) ? kernelAddressSpaceList : generalAddressSpaceList;
	VirtualMemNode *bestFit = nullptr, *current = list;
	while (current) {
		if (
			current->available &&
			current->pageCount >= count &&
			(bestFit == nullptr || bestFit->pageCount > current->pageCount)
		) {
			bestFit = current;
		}
		current = current->next;
	}
	if (flags & Kernel::Memory::RequestType::Contiguous) {
		bestFit->available = false;
		if (bestFit->pageCount != count) {
			VirtualMemNode *newNode = new VirtualMemNode();
			newNode->available = true;
			newNode->base = (void*)((uint64_t)bestFit->base + count * Kernel::Memory::pageSize);
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
		defragAddressSpaceList(list);

		if (flags & MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE) {
			size_t total = 0;
			while (total != count) {
				Kernel::Memory::PageRequestResult phyResult = Kernel::Memory::Physical::requestPages(count - total, 0);
				if (phyResult.address == INVALID_ADDRESS || phyResult.allocatedCount == 0) {
					// Out of memory
					// TODO: should swap out pages instead of panicking
					terminalPrintString(outOfMemoryStr, strlen(outOfMemoryStr));
					kernelPanic();
				}
				mapVirtualPages(
					(void*)((uint64_t)result.address + total * Kernel::Memory::pageSize),
					phyResult.address,
					phyResult.allocatedCount,
					flags
				);
				total += phyResult.allocatedCount;
			}
		}
	} else {
		// FIXME: serve non-contiguous virtual addresses
		// is this case even needed?
		terminalPrintString(requestErrorStr, strlen(requestErrorStr));
		kernelPanic();
	}
	return result;
}

// Frees and unmaps virtual pages from the kernel or general address space
// depending on MEMORY_REQUEST_KERNEL_PAGE flag
// It is assumed all virtual addresses are canonical and pageSize boundary aligned
// If MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE flag is passed,
// the physical pages to which a virtual page is mapped is also freed
// Returns false if the virtual pages to be freed do not entirely fit in a used region
bool freeVirtualPages(void *virtualAddress, size_t count, uint8_t flags) {
	uint64_t vBeg = (uint64_t)virtualAddress;
	uint64_t vEnd = vBeg + count * Kernel::Memory::pageSize;

	// Ensure the virtual addresses are pageSize boundary aligned and canonical
	if ((vBeg & ~Kernel::Memory::Physical::buddyMasks[0]) || !isCanonicalVirtualAddress(virtualAddress)) {
		return false;
	}

	VirtualMemNode *list = (flags & MEMORY_REQUEST_KERNEL_PAGE) ? kernelAddressSpaceList : generalAddressSpaceList;
	VirtualMemNode *current = list;
	while (current) {
		uint64_t cBeg = (uint64_t)current->base;
		uint64_t cEnd = cBeg + current->pageCount * Kernel::Memory::pageSize;
		if (!current->available && cBeg <= vBeg && cEnd >= vEnd) {
			if (cBeg == vBeg && cEnd == vEnd) {
				// Region to be freed fits exactly in current block
				current->available = true;
			} else {
				VirtualMemNode *newNode1 = new VirtualMemNode();
				VirtualMemNode *newNode2 = new VirtualMemNode();
				VirtualMemNode *newNode3 = new VirtualMemNode();
				newNode1->available = false;
				newNode1->base = current->base;
				newNode1->pageCount = (vBeg - cBeg) / Kernel::Memory::pageSize;
				newNode1->next = newNode2;
				newNode1->previous = current->previous;
				if (newNode1->previous) {
					newNode1->previous->next = newNode1;
				}
				newNode2->available = true;
				newNode2->base = (void*)vBeg;
				newNode2->pageCount = count;
				newNode2->next = newNode3;
				newNode2->previous = newNode1;
				newNode3->available = false;
				newNode3->base = (void*)vEnd;
				newNode3->pageCount = (cEnd - vEnd) / Kernel::Memory::pageSize;
				newNode3->next = current->next;
				newNode3->previous = newNode2;
				if (newNode3->next) {
					newNode3->next->previous = newNode3;
				}
				delete current;
				if (!newNode1->previous) {
					// newNode1 and by implication current is the first node in the list
					// current is already freed
					if (flags & MEMORY_REQUEST_KERNEL_PAGE) {
						kernelAddressSpaceList = newNode1;
					} else {
						generalAddressSpaceList = newNode1;
					}
				}
				if (newNode1->pageCount == 0) {
					// Region to be freed is at the beginning of the current block
					newNode2->previous = newNode1->previous;
					delete newNode1;
					if (newNode2->previous) {
						newNode2->previous->next = newNode2;
					} else {
						// newNode2 is the first node in the list
						if (flags & MEMORY_REQUEST_KERNEL_PAGE) {
							kernelAddressSpaceList = newNode2;
						} else {
							generalAddressSpaceList = newNode2;
						}
					}
				}
				if (newNode3->pageCount == 0) {
					// Region to be freed is at the end of the current block
					newNode2->next = newNode3->next;
					if (newNode2->next) {
						newNode2->next->previous = newNode2;
					}
					delete newNode3;
				}
			}
			if (flags & MEMORY_REQUEST_KERNEL_PAGE) {
				kernelPagesAvailableCount += count;
			} else {
				generalPagesAvailableCount += count;
			}
			defragAddressSpaceList((flags & MEMORY_REQUEST_KERNEL_PAGE) ? kernelAddressSpaceList : generalAddressSpaceList);

			unmapVirtualPages(virtualAddress, count, flags & MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE);
			return true;
		}
		current = current->next;
	}
	return true;
}

static void defragAddressSpaceList(VirtualMemNode *list) {
	while(list->next) {
		if (list->available == list->next->available) {
			// Merge blocks
			VirtualMemNode *nextBlock = list->next;
			list->pageCount += list->next->pageCount;
			list->next = list->next->next;
			if (list->next) {
				list->next->previous = list;
			}
			delete nextBlock;
		} else {
			// Move to the next block
			list = list->next;
		}
	}
}

// Maps virtual pages to physical pages
// It is assumed that all virtual pages and physical pages are contiguous, reserved, pageSize boundary aligned
// and within bounds of physical memory and canonical virtual address space before calling this function
// When MEMORY_REQUEST_CACHE_DISABLE flag is passed, the physical page is marked as cachedDisabled(1) in the PTE
// Returns true only on successful mapping
bool mapVirtualPages(void* virtualAddress, void* physicalAddress, size_t count, uint8_t flags) {
	uint64_t phyAddr = (uint64_t)physicalAddress;
	uint64_t virAddr = (uint64_t)virtualAddress;

	// Ensure both physical and virtual address are pageSize boundary aligned
	if (phyAddr & ~Kernel::Memory::Physical::buddyMasks[0] || virAddr & ~Kernel::Memory::Physical::buddyMasks[0]) {
		return false;
	}

	Kernel::Memory::PageRequestResult requestResult;
	for (size_t i = 0; i < count; ++i, phyAddr += Kernel::Memory::pageSize, virAddr += Kernel::Memory::pageSize) {
		PML4CrawlResult crawlResult((void*)virAddr);
		if (!crawlResult.isCanonical) {
			return false;
		}
		for (size_t j = 3; j >= 1; --j) {
			if (crawlResult.physicalTables[j] == INVALID_ADDRESS) {
				// Create a new page table if entry is not present
				requestResult = Kernel::Memory::Physical::requestPages(1, 0);
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
				crawlResult.tables[j + 1][crawlResult.indexes[j + 1]].physicalAddress = (uint64_t)requestResult.address >> Kernel::Memory::pageSizeShift;
				memset(crawlResult.tables[j], 0, Kernel::Memory::pageSize);
			}
		}
		if (crawlResult.physicalTables[0] == INVALID_ADDRESS) {
			crawlResult.tables[1][crawlResult.indexes[1]].present = 1;
			// FIXME: should add only appropriate permissions when mapping pages
			crawlResult.tables[1][crawlResult.indexes[1]].readWrite = 1;
			crawlResult.tables[1][crawlResult.indexes[1]].physicalAddress = phyAddr >> Kernel::Memory::pageSizeShift;
			crawlResult.tables[1][crawlResult.indexes[1]].cacheDisable = (flags & MEMORY_REQUEST_CACHE_DISABLE) ? 1 : 0;
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

	// Ensure the virtual addresses are pageSize boundary aligned
	if (addr & ~Kernel::Memory::Physical::buddyMasks[0]) {
		return false;
	}

	for (size_t i = 0; i < count; ++i, addr += Kernel::Memory::pageSize) {
		PML4CrawlResult crawlResult((void*) addr);
		if (!crawlResult.isCanonical) {
			return false;
		}
		if (crawlResult.physicalTables[0] != INVALID_ADDRESS) {
			// Virtual address is fully resolved
			crawlResult.tables[1][crawlResult.indexes[1]].present = 0;
			if (freePhysicalPage) {
				Kernel::Memory::Physical::markPages(crawlResult.physicalTables[0], 1, Kernel::Memory::MarkPageType::Free);
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
				Kernel::Memory::Physical::markPages((void*)crawlResult.physicalTables[j], 1, Kernel::Memory::MarkPageType::Free);
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
PML4CrawlResult::PML4CrawlResult(void *virtualAddress) {
	this->isCanonical = false;
	uint64_t addr = (uint64_t)virtualAddress & Kernel::Memory::Physical::buddyMasks[0];
	this->indexes[0] = (uint64_t)virtualAddress - addr;
	addr >>= Kernel::Memory::pageSizeShift;
	for (size_t i = 1; i <= 4; ++i) {
		this->indexes[i] = addr & virtualPageIndexMask;
		addr >>= virtualPageIndexShift;
	}

	this->tables[0] = (PML4E*)INVALID_ADDRESS;
	this->tables[1] = (PML4E*)((uint64_t)ptMask + this->indexes[4] * GIB_1 + this->indexes[3] * MIB_2 + this->indexes[2] * KIB_4);
	this->tables[2] = (PML4E*)((uint64_t)pdMask + this->indexes[4] * MIB_2 + this->indexes[3] * KIB_4);
	this->tables[3] = (PML4E*)((uint64_t)pdptMask + this->indexes[4] * KIB_4);
	this->tables[4] = pml4t;

	this->physicalTables[0] =
	this->physicalTables[1] =
	this->physicalTables[2] =
	this->physicalTables[3] =
	this->physicalTables[4] =
		(PML4E*)INVALID_ADDRESS;

	this->cached[0] = 
	this->cached[1] = 
	this->cached[2] = 
	this->cached[3] = 
		false;
	this->cached[4] = (pml4t->cacheDisable & 1) ? false : true;

	if (isCanonicalVirtualAddress(virtualAddress)) {
		this->isCanonical = true;
		this->physicalTables[4] = (PML4E*)infoTable->pml4tPhysicalAddress;
		for (size_t i = 4; i >= 1; --i) {
			if (this->tables[i][indexes[i]].present) {
				this->physicalTables[i - 1] = (PML4E*)((uint64_t)this->tables[i][this->indexes[i]].physicalAddress << Kernel::Memory::pageSizeShift);
				this->cached[i - 1] = (this->tables[i][this->indexes[i]].cacheDisable & 1) ? false : true;
			} else {
				break;
			}
		}
	}
}

// Debug helper to display result of PML4CrawlResult for a given virtual address
void displayCrawlPageTablesResult(void *virtualAddress) {
	terminalPrintString(pageTablesStr, strlen(pageTablesStr));
	terminalPrintHex(&virtualAddress, sizeof(virtualAddress));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(isCanonicalStr, strlen(isCanonicalStr));
	PML4CrawlResult result(virtualAddress);
	terminalPrintString(result.isCanonical ? trueStr : falseStr, strlen(result.isCanonical ? trueStr : falseStr));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(crawlTableHeader, strlen(crawlTableHeader));
	for (int i = 4; i >= 0; --i) {
		terminalPrintSpaces4();
		terminalPrintDecimal(i);
		terminalPrintChar(' ');
		terminalPrintHex(&result.tables[i], sizeof(result.tables[i]));
		terminalPrintChar(' ');
		terminalPrintHex(&result.physicalTables[i], sizeof(result.physicalTables[i]));
		terminalPrintChar(' ');
		terminalPrintHex(&result.indexes[i], sizeof(result.indexes[i]));
		terminalPrintChar(' ');
		terminalPrintDecimal(result.cached[i]);
		terminalPrintChar('\n');
	}
}

// Debug helper to list all entries in a given virtual address space list
// depending on flags MEMORY_REQUEST_KERNEL_PAGE
void traverseAddressSpaceList(uint8_t flags, bool forwardDirection) {
	VirtualMemNode *list;
	if (flags & MEMORY_REQUEST_KERNEL_PAGE) {
		list = kernelAddressSpaceList;
		terminalPrintString("Kernel", 6);
	} else {
		list = generalAddressSpaceList;
		terminalPrintString("General", 7);
	}
	terminalPrintString(addrSpaceStr, strlen(addrSpaceStr));
	terminalPrintHex(&list, sizeof(list));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(pagesAvailableStr, strlen(pagesAvailableStr));
	if (flags & MEMORY_REQUEST_KERNEL_PAGE) {
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
