#include <commonstrings.h>
#include <cstring>
#include <kernel.h>
#include <pml4t.h>
#include <terminal.h>

static const size_t maxVirtualAddressBits = 48;
static const size_t pml4tRecursiveEntry = 510;
static const uint64_t nonCanonicalStart = (uint64_t)1 << (maxVirtualAddressBits - 1);
static const uint64_t nonCanonicalEnd = ~(nonCanonicalStart - 1);
static size_t generalPagesAvailableCount = 0;
static size_t kernelPagesAvailableCount = 0;
static const uint64_t ptMask = (uint64_t)UINT64_MAX - 1024 * (uint64_t)GIB_1 + 1;
static const uint64_t pdMask = ptMask + (uint64_t)pml4tRecursiveEntry * (uint64_t)GIB_1;
static const uint64_t pdptMask = pdMask + (uint64_t)pml4tRecursiveEntry * (uint64_t)MIB_2;
static PML4E* const pml4t = (PML4E*)(pdptMask + (uint64_t)pml4tRecursiveEntry * (uint64_t)KIB_4);
static const size_t virtualPageIndexShift = 9;
static const uint64_t virtualPageIndexMask = ((uint64_t)1 << virtualPageIndexShift) - 1;

static const char* const initVirMemStr = "Initializing virtual memory management";
static const char* const initVirMemCompleteStr = "Virtual memory management initialized\n\n";
static const char* const markingPml4Str = "Marking PML4 page tables as used in physical memory";
static const char* const movingBuddiesStr = "Mapping physical memory manager in kernel address space";
static const char* const maxVirAddrMismatch = "Max virtual address bits mismatch. Expected [";
static const char* const gotStr = "] got [";
static const char* const entryCreationFailed = "Failed to create PML4 entry at level ";
static const char* const forAddress = " for address ";
static const char* const removingIdStr = "Removing PML4 identity mapping of first ";
static const char* const nullUnmapStr = "Unmapping first page";
static const char* const reservingHeapStr = "Reserving memory for dynamic memory manager";
static const char* const pageTablesStr = "Page tables of ";
static const char* const isCanonicalStr = "isCanonical = ";
static const char* const crawlTableHeader = "L Tables               Physical tables      Indexes              C\n";
static const char* const addrSpaceStr = " address space list\n";
static const char* const addrSpaceHeader = "Base                 Page count           Available\n";
static const char* const creatingListsStr = "Creating virtual address space lists";
static const char* const recursiveStr = "Creating PML4 recursive entry";
static const char* const checkingMaxBitsStr = "Checking max virtual address bits";
static const char* const virtualNamespaceStr = "Kernel::Memory::Virtual::";
static const char* const freeErrorStr = "tried to free already free pages";
static const char* const freePagesStr = "freePages ";
static const char* const requestPagesStr = "requestPages ";
static const char* const mapPagesStr = "mapPages ";
static const char* const requestErrorStr = "did not pass RequestType::VirtualContiguous\n";
static const char* const globalCtorStr = "Running global constructors";
static const char* const listDefragFailStr = "defragAddressSpaceList integrity check failed for ";
static const char* const mapFailStr = "failed to (un)map pages";

static void defragAddressSpaceList(uint32_t flags);

Kernel::Memory::Virtual::AddressSpaceList Kernel::Memory::Virtual::generalAddressSpaceList(8);
Kernel::Memory::Virtual::AddressSpaceList Kernel::Memory::Virtual::kernelAddressSpaceList(2);

// Initializes virtual memory space for use by higher level dynamic memory manager and other kernel services
bool Kernel::Memory::Virtual::initialize(
	void* usableKernelSpaceStart,
	size_t kernelLowerHalfSize,
	size_t phyMemBuddyPagesCount,
	GlobalConstructor (&globalCtors)[]
) {
	uint64_t mib1 = 0x100000;
	terminalPrintString(initVirMemStr, strlen(initVirMemStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	terminalPrintSpaces4();
	terminalPrintString(checkingMaxBitsStr, strlen(checkingMaxBitsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (infoTable.maxLinearAddress != maxVirtualAddressBits) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		terminalPrintSpaces4();
		terminalPrintString(maxVirAddrMismatch, strlen(maxVirAddrMismatch));
		terminalPrintDecimal(maxVirtualAddressBits);
		terminalPrintString(gotStr, strlen(gotStr));
		terminalPrintDecimal(infoTable.maxLinearAddress);
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
	PML4E *pml4tId = (PML4E*) infoTable.pml4tPhysicalAddress;
	Physical::markPages(pml4tId, 1, MarkPageType::Used);
	for (size_t i = 0; i < PML4_ENTRY_COUNT; ++i) {
		if (pml4tId[i].present) {
			PDPTE *pdptId = (PDPTE*)((uint64_t)pml4tId[i].physicalAddress << pageSizeShift);
			Physical::markPages(pdptId, 1, MarkPageType::Used);
			for (size_t j = 0; j < PML4_ENTRY_COUNT; ++j) {
				if (pdptId[j].present) {
					PDE *pdtId = (PDE*)((uint64_t)pdptId[j].physicalAddress << pageSizeShift);
					Physical::markPages(pdtId, 1, MarkPageType::Used);
					for (size_t k = 0; k < PML4_ENTRY_COUNT; ++k) {
						if (pdtId[k].present) {
							PTE *ptId = (PTE*)((uint64_t)pdtId[k].physicalAddress << pageSizeShift);
							Physical::markPages(ptId, 1, MarkPageType::Used);
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
	PML4E *root = (PML4E*)infoTable.pml4tPhysicalAddress;
	root[pml4tRecursiveEntry].present = root[pml4tRecursiveEntry].writable = 1;
	root[pml4tRecursiveEntry].physicalAddress = infoTable.pml4tPhysicalAddress >> pageSizeShift;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Map physical memory buddy bitmap to kernel address space
	terminalPrintSpaces4();
	terminalPrintString(movingBuddiesStr, strlen(movingBuddiesStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!mapPages(usableKernelSpaceStart, Physical::buddyBitmaps[0], phyMemBuddyPagesCount, 0)) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	Physical::buddyBitmaps[0] = (uint8_t*)usableKernelSpaceStart;
	for (size_t i = 1; i < PHY_MEM_BUDDY_MAX_ORDER; ++i) {
		Physical::buddyBitmaps[i] = (uint8_t*)(
			(uint64_t)Physical::buddyBitmaps[i - 1] +
			Physical::buddyBitmapSizes[i - 1]
		);
	}
	usableKernelSpaceStart = (void*)(
		(uint64_t)Physical::buddyBitmaps[0] +
		phyMemBuddyPagesCount * pageSize
	);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Remove identity mapping for virtual addresses from 1MiB to L32_IDENTITY_MAP_SIZE MiBs,
	// scratch memory from L32K64_SCRATCH_BASE of length L32K64_SCRATCH_LENGTH
	// and mapping for lower half of kernel
	terminalPrintSpaces4();
	terminalPrintString(removingIdStr, strlen(removingIdStr));
	terminalPrintDecimal(L32_IDENTITY_MAP_SIZE);
	terminalPrintString("MiBs", 4);
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (
		!unmapPages((void*)mib1, (L32_IDENTITY_MAP_SIZE - 1) * mib1 / pageSize, false) ||
		!unmapPages((void*)L32K64_SCRATCH_BASE, L32K64_SCRATCH_LENGTH / pageSize, false) ||
		!unmapPages((void*)KERNEL_LOWERHALF_ORIGIN, kernelLowerHalfSize / pageSize, false)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Unmap the first page to detect nullptr accesses
	terminalPrintSpaces4();
	terminalPrintString(nullUnmapStr, strlen(nullUnmapStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!unmapPages((void*)0, 1, false)) {
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
	if (!Heap::create(
			usableKernelSpaceStart,
			(void**)((uint64_t)usableKernelSpaceStart + Heap::newRegionSize)
		)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	usableKernelSpaceStart = (void*)((uint64_t)usableKernelSpaceStart + Heap::newRegionSize + Heap::entryTableSize);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Run the global constructors
	terminalPrintSpaces4();
	terminalPrintString(globalCtorStr, strlen(globalCtorStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	for (size_t i = 0; i < infoTable.globalCtorsCount; ++i) {
		globalCtors[i]();
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Used areas of virtual address space so far
	// 1) 0x0 to L32K64_SCRATCH_BASE
	// 2) (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) to 1MiB
	// 3) PML4 recursive mapping
	// 4) KERNEL_HIGHERHALF_ORIGIN to usableKernelSpaceStart
	terminalPrintSpaces4();
	terminalPrintString(creatingListsStr, strlen(creatingListsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	// Used kernel space
	kernelAddressSpaceList.at(0).available = false;
	kernelAddressSpaceList.at(0).base = (void*) KERNEL_HIGHERHALF_ORIGIN;
	kernelAddressSpaceList.at(0).pageCount = ((uint64_t)usableKernelSpaceStart - KERNEL_HIGHERHALF_ORIGIN) / pageSize;
	// Available kernel space
	kernelAddressSpaceList.at(1).available = true;
	kernelAddressSpaceList.at(1).base = usableKernelSpaceStart;
	kernelPagesAvailableCount =
		kernelAddressSpaceList.at(1).pageCount =
		((uint64_t)UINT64_MAX - (uint64_t)usableKernelSpaceStart + 1) / pageSize;
	// General 0 to L32K64_SCRATCH_BASE used
	generalAddressSpaceList.at(0).available = false;
	generalAddressSpaceList.at(0).base = 0;
	generalAddressSpaceList.at(0).pageCount = L32K64_SCRATCH_BASE / pageSize;
	// General L32K64_SCRATCH_BASE to (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) available
	generalAddressSpaceList.at(1).available = true;
	generalAddressSpaceList.at(1).base = (void*) L32K64_SCRATCH_BASE;
	generalAddressSpaceList.at(1).pageCount = L32K64_SCRATCH_LENGTH / pageSize;
	generalPagesAvailableCount += generalAddressSpaceList.at(1).pageCount;
	// General (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) to 1MiB used
	generalAddressSpaceList.at(2).available = false;
	generalAddressSpaceList.at(2).base = (void*)(L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH);
	generalAddressSpaceList.at(2).pageCount = (mib1 - L32K64_SCRATCH_BASE - L32K64_SCRATCH_LENGTH) / pageSize;
	// General 1MiB to valid lower half canonical address available
	generalAddressSpaceList.at(3).available = true;
	generalAddressSpaceList.at(3).base = (void*) mib1;
	generalAddressSpaceList.at(3).pageCount = (nonCanonicalStart - mib1) / pageSize;
	generalPagesAvailableCount += generalAddressSpaceList.at(3).pageCount;
	// Mark non canonical address range as used
	generalAddressSpaceList.at(4).available = false;
	generalAddressSpaceList.at(4).base = (void*) nonCanonicalStart;
	generalAddressSpaceList.at(4).pageCount = (nonCanonicalEnd - nonCanonicalStart) / pageSize;
	// General valid higher half canonical address to PML4 recursive map available
	generalAddressSpaceList.at(5).available = true;
	generalAddressSpaceList.at(5).base = (void*) nonCanonicalEnd;
	generalAddressSpaceList.at(5).pageCount = (ptMask - nonCanonicalEnd) / pageSize;
	generalPagesAvailableCount += generalAddressSpaceList.at(5).pageCount;
	// PML4 recursive map used
	generalAddressSpaceList.at(6).available = false;
	generalAddressSpaceList.at(6).base = (void*) ptMask;
	generalAddressSpaceList.at(6).pageCount = ((uint64_t)512 * GIB_1) / pageSize;
	// General PML4 recursive map to KERNEL_HIGHERHALF_ORIGIN available
	generalAddressSpaceList.at(7).available = true;
	generalAddressSpaceList.at(7).base = (void*)(ptMask + 512 * GIB_1);
	generalAddressSpaceList.at(7).pageCount = ((uint64_t)KERNEL_HIGHERHALF_ORIGIN - ptMask - 512 * GIB_1) / pageSize;
	generalPagesAvailableCount += generalAddressSpaceList.at(7).pageCount;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintString(initVirMemCompleteStr, strlen(initVirMemCompleteStr));
	return true;
}

// Returns a region in the kernel or general address space depending on RequestType::Kernel flag
// that is the closest fit to the number of requested pages
// If RequestType::AllocatePhysical flag is passed,
// the returned virtual addresses are mapped to newly allocated physical pages
// When RequestType::CacheDisable flag is passed, the physical page is marked as cachedDisabled(1) in the PTE
// Returns INVALID_ADDRESS and allocatedCount = 0 if request count is count == 0
// or greater than currently available kernel pages
// Unsafe to call this function until virtual memory manager is initialized
Kernel::Memory::PageRequestResult Kernel::Memory::Virtual::requestPages(size_t count, uint32_t flags) {
	PageRequestResult result;
	if (count == 0 || count > ((flags & RequestType::Kernel) ? kernelPagesAvailableCount : generalPagesAvailableCount)) {
		return result;
	}
	AddressSpaceList &list = (flags & RequestType::Kernel) ? kernelAddressSpaceList : generalAddressSpaceList;
	if (flags & RequestType::VirtualContiguous) {
		size_t bestFitIndex = SIZE_MAX;
		for (size_t i = 0; const auto &block : list) {
			if (
				block.available &&
				block.pageCount >= count &&
				(bestFitIndex == SIZE_MAX || list.at(bestFitIndex).pageCount > block.pageCount)
			) {
				bestFitIndex = i;
			}
			++i;
		}
		list.at(bestFitIndex).available = false;
		if (list.at(bestFitIndex).pageCount != count) {
			list.insert(
				list.begin() + bestFitIndex + 1,
				{
					.available = true,
					.base = (void*)((uint64_t)list.at(bestFitIndex).base + count * pageSize),
					.pageCount = list.at(bestFitIndex).pageCount - count
				}
			);
			list.at(bestFitIndex).pageCount = count;
		}
		if (flags & RequestType::Kernel) {
			kernelPagesAvailableCount -= count;
		} else {
			generalPagesAvailableCount -= count;
		}
		result.address = list.at(bestFitIndex).base;
		result.allocatedCount = count;
		defragAddressSpaceList(flags);

		if (flags & RequestType::AllocatePhysical) {
			size_t total = 0;
			while (total != count) {
				PageRequestResult phyResult = Physical::requestPages(count - total, flags);
				if (phyResult.address == INVALID_ADDRESS || phyResult.allocatedCount == 0) {
					// Out of memory
					// TODO: should swap out pages instead of panicking
					terminalPrintString(virtualNamespaceStr, strlen(virtualNamespaceStr));
					terminalPrintString(requestPagesStr, strlen(requestPagesStr));
					terminalPrintString(outOfMemoryStr, strlen(outOfMemoryStr));
					panic();
				}
				if (!mapPages(
					(void*)((uint64_t)result.address + total * pageSize),
					phyResult.address,
					phyResult.allocatedCount,
					flags
				)) {
					terminalPrintString(virtualNamespaceStr, strlen(virtualNamespaceStr));
					terminalPrintString(requestPagesStr, strlen(requestPagesStr));
					terminalPrintString(mapFailStr, strlen(mapFailStr));
					panic();
				}
				total += phyResult.allocatedCount;
			}
		}
	} else {
		// FIXME: serve non-contiguous virtual addresses
		// is this case even needed?
		terminalPrintString(virtualNamespaceStr, strlen(virtualNamespaceStr));
		terminalPrintString(requestPagesStr, strlen(requestPagesStr));
		terminalPrintString(requestErrorStr, strlen(requestErrorStr));
		panic();
	}
	return result;
}

// Frees and unmaps virtual pages from the kernel or general address space
// depending on RequestType::Kernel flag
// It is assumed all virtual addresses are canonical and pageSize boundary aligned
// If RequestType::AllocatePhysical flag is passed,
// the physical pages to which a virtual page is mapped is also freed
// Returns false if the virtual pages to be freed do not entirely fit in a used region
bool Kernel::Memory::Virtual::freePages(void *virtualAddress, size_t count, uint32_t flags) {
	uint64_t vBeg = (uint64_t)virtualAddress;
	uint64_t vEnd = vBeg + count * pageSize;

	// Ensure the virtual addresses are pageSize boundary aligned and canonical
	if (count == 0 || (vBeg & ~Physical::buddyMasks[0]) || !isCanonical(virtualAddress)) {
		return false;
	}

	AddressSpaceList &list = (flags & RequestType::Kernel) ? kernelAddressSpaceList : generalAddressSpaceList;
	for (size_t i = 0; auto &block : list) {
		uint64_t blockBeg = (uint64_t)block.base;
		uint64_t blockEnd = blockBeg + block.pageCount * pageSize;
		if (blockBeg <= vBeg && blockEnd >= vEnd) {
			if (block.available) {
				// Tried to free an available block
				terminalPrintString(virtualNamespaceStr, strlen(virtualNamespaceStr));
				terminalPrintString(freePagesStr, strlen(freePagesStr));
				terminalPrintString(freeErrorStr, strlen(freeErrorStr));
				panic();
				return false;
			}
			if (blockBeg == vBeg && blockEnd == vEnd) {
				// Region to be freed fits exactly in current block
				block.available = true;
			} else {
				list.insert(
					list.begin() + i + 1,
					{
						.available = false,
						.base = (void*)vEnd,
						.pageCount = (blockEnd - vEnd) / pageSize
					}
				);
				list.insert(
					list.begin() + i + 1,
					{
						.available = true,
						.base = (void*)vBeg,
						.pageCount = count
					}
				);
				block.pageCount -= count;
			}
			if (flags & RequestType::Kernel) {
				kernelPagesAvailableCount += count;
			} else {
				generalPagesAvailableCount += count;
			}
			defragAddressSpaceList(flags);

			if (!unmapPages(virtualAddress, count, flags & RequestType::AllocatePhysical ? true : false)) {
				terminalPrintString(virtualNamespaceStr, strlen(virtualNamespaceStr));
				terminalPrintString(freePagesStr, strlen(freePagesStr));
				terminalPrintString(mapFailStr, strlen(mapFailStr));
				panic();
			}
			return true;
		}
		++i;
	}
	return true;
}

static void defragAddressSpaceList(uint32_t flags) {
	using namespace Kernel::Memory::Virtual;

	bool kernelList = flags & Kernel::Memory::RequestType::Kernel;
	Kernel::Memory::Virtual::AddressSpaceList &list = kernelList ? kernelAddressSpaceList : generalAddressSpaceList;

	// Remove blocks with pageCount = 0
	for (size_t i = 0; i < list.size(); ++i) {
		if (list.at(i).pageCount == 0) {
			list.erase(list.begin() + i);
			--i;
		}
	}

	// Merge all blocks that have same availability
	for (size_t i = 0; i < list.size() - 1; ++i) {
		if (list.at(i).available == list.at(i + 1).available) {
			list.at(i).pageCount += list.at(i + 1).pageCount;
			list.erase(list.begin() + i + 1);
			--i;
		}
	}

	// TODO: remove expensive address space list integrity check
	size_t total = 0;
	bool listMono = true;
	for (size_t i = 0; i < list.size() - 1; ++i) {
		if (list.at(i).available) {
			total += list.at(i).pageCount;
		}
		if (
			list.at(i).pageCount == 0 ||
			list.at(i).available == list.at(i + 1).available ||
			(uint64_t)list.at(i + 1).base != (uint64_t)list.at(i).base + list.at(i).pageCount * Kernel::Memory::pageSize
		) {
			listMono = false;
			break;
		}
	}
	total += list.back().pageCount;
	bool isStartValid = kernelList ? list.at(0).base == (void*)KERNEL_HIGHERHALF_ORIGIN : list.at(0).base == (void*)0;
	if (
		!isStartValid ||
		!listMono ||
		list.back().pageCount == 0 ||
		total != (kernelList ? kernelPagesAvailableCount : generalPagesAvailableCount)
	) {
		terminalPrintString(virtualNamespaceStr, strlen(virtualNamespaceStr));
		terminalPrintString(listDefragFailStr, strlen(listDefragFailStr));
		terminalPrintString(kernelList ? "Kernel" : "General", kernelList ? 6 : 7);
		Kernel::Memory::Virtual::showAddressSpaceList(kernelList);
		Kernel::panic();
	}
}

// Maps virtual pages to physical pages
// It is assumed that all virtual pages and physical pages are contiguous, reserved, pageSize boundary aligned
// and within bounds of physical memory and canonical virtual address space before calling this function
// When MEMORY_REQUEST_CACHE_DISABLE flag is passed, the physical page is marked as cachedDisabled(1) in the PTE
// Returns true only on successful mapping
bool Kernel::Memory::Virtual::mapPages(void* virtualAddress, void* physicalAddress, size_t count, uint32_t flags) {
	uint64_t phyAddr = (uint64_t)physicalAddress;
	uint64_t virAddr = (uint64_t)virtualAddress;

	// Ensure both physical and virtual address are pageSize boundary aligned
	if (count == 0 || phyAddr & ~Physical::buddyMasks[0] || virAddr & ~Physical::buddyMasks[0]) {
		return false;
	}

	PageRequestResult requestResult;
	for (size_t i = 0; i < count; ++i, phyAddr += pageSize, virAddr += pageSize) {
		CrawlResult crawlResult((void*)virAddr);
		if (!crawlResult.isCanonical) {
			return false;
		}
		for (size_t j = 3; j >= 1; --j) {
			if (crawlResult.physicalTables[j] == INVALID_ADDRESS) {
				// Create a new page table if entry is not present
				requestResult = Physical::requestPages(1, RequestType::PhysicalContiguous);
				if (requestResult.address == INVALID_ADDRESS || requestResult.allocatedCount != 1) {
					// TODO: should swap out a physical page instead of panicking
					terminalPrintString(entryCreationFailed, strlen(entryCreationFailed));
					terminalPrintDecimal(j);
					terminalPrintString(forAddress, strlen(forAddress));
					terminalPrintHex(&virAddr, sizeof(virAddr));
					terminalPrintChar('\n');
					terminalPrintString(virtualNamespaceStr, strlen(virtualNamespaceStr));
					terminalPrintString(mapPagesStr, strlen(mapPagesStr));
					terminalPrintString(outOfMemoryStr, strlen(outOfMemoryStr));
					panic();
				}
				crawlResult.tables[j + 1][crawlResult.indexes[j + 1]].present = 1;
				crawlResult.tables[j + 1][crawlResult.indexes[j + 1]].writable = 1;
				crawlResult.tables[j + 1][crawlResult.indexes[j + 1]].physicalAddress = (uint64_t)requestResult.address >> pageSizeShift;
				memset(crawlResult.tables[j], 0, pageSize);
			}
		}
		if (crawlResult.physicalTables[0] == INVALID_ADDRESS) {
			crawlResult.tables[1][crawlResult.indexes[1]].present = 1;
			crawlResult.tables[1][crawlResult.indexes[1]].physicalAddress = phyAddr >> pageSizeShift;
			crawlResult.tables[1][crawlResult.indexes[1]].cacheDisable = (flags & RequestType::CacheDisable) ? 1 : 0;
			crawlResult.tables[1][crawlResult.indexes[1]].writable = (flags & RequestType::Writable) ? 1 : 0;
			crawlResult.tables[1][crawlResult.indexes[1]].executeDisable = (flags & RequestType::Executable) ? 0 : 1;
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
bool Kernel::Memory::Virtual::unmapPages(void* virtualAddress, size_t count, bool freePhysicalPage) {
	uint64_t addr = (uint64_t) virtualAddress;

	// Ensure the virtual addresses are pageSize boundary aligned
	if (count == 0 || addr & ~Physical::buddyMasks[0]) {
		return false;
	}

	for (size_t i = 0; i < count; ++i, addr += pageSize) {
		CrawlResult crawlResult((void*) addr);
		if (!crawlResult.isCanonical) {
			return false;
		}
		if (crawlResult.physicalTables[0] != INVALID_ADDRESS) {
			// Virtual address is fully resolved
			crawlResult.tables[1][crawlResult.indexes[1]].present = 0;
			if (freePhysicalPage) {
				Physical::markPages(crawlResult.physicalTables[0], 1, MarkPageType::Free);
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
				Physical::markPages((void*)crawlResult.physicalTables[j], 1, MarkPageType::Free);
			}
		}
	}
	return true;
}

// Returns true only if virtual address is canonical i.e. lies in the range
// 0 - 0x00007fffffffffff or 0xffff800000000000 - 0xffffffffffffffff
bool Kernel::Memory::Virtual::isCanonical(void* address) {
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
Kernel::Memory::Virtual::CrawlResult::CrawlResult(void *virtualAddress) {
	this->isCanonical = false;
	uint64_t addr = (uint64_t)virtualAddress & Physical::buddyMasks[0];
	this->indexes[0] = (uint64_t)virtualAddress - addr;
	addr >>= pageSizeShift;
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

	if (Virtual::isCanonical(virtualAddress)) {
		this->isCanonical = true;
		this->physicalTables[4] = (PML4E*)infoTable.pml4tPhysicalAddress;
		for (size_t i = 4; i >= 1; --i) {
			if (this->tables[i][indexes[i]].present) {
				this->physicalTables[i - 1] = (PML4E*)((uint64_t)this->tables[i][this->indexes[i]].physicalAddress << pageSizeShift);
				this->cached[i - 1] = (this->tables[i][this->indexes[i]].cacheDisable & 1) ? false : true;
			} else {
				break;
			}
		}
	}
}

// Debug helper to display result of CrawlResult for a given virtual address
void Kernel::Memory::Virtual::displayCrawlPageTablesResult(void *virtualAddress) {
	terminalPrintString(pageTablesStr, strlen(pageTablesStr));
	terminalPrintHex(&virtualAddress, sizeof(virtualAddress));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(isCanonicalStr, strlen(isCanonicalStr));
	CrawlResult result(virtualAddress);
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
void Kernel::Memory::Virtual::showAddressSpaceList(bool kernelList) {
	std::vector<AddressSpaceNode> &list = kernelList ? kernelAddressSpaceList : generalAddressSpaceList;
	terminalPrintString(kernelList ? "Kernel" : "General", kernelList ? 6 : 7);
	terminalPrintString(addrSpaceStr, strlen(addrSpaceStr));
	terminalPrintSpaces4();
	terminalPrintString(pagesAvailableStr, strlen(pagesAvailableStr));
	if (kernelList) {
		terminalPrintHex(&kernelPagesAvailableCount, sizeof(kernelPagesAvailableCount));
	} else {
		terminalPrintHex(&generalPagesAvailableCount, sizeof(generalPagesAvailableCount));
	}
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(addrSpaceHeader, strlen(addrSpaceHeader));

	for (const auto &x : list) {
		terminalPrintSpaces4();
		terminalPrintHex(&x.base, sizeof(x.base));
		terminalPrintChar(' ');
		terminalPrintHex(&x.pageCount, sizeof(x.pageCount));
		terminalPrintChar(' ');
		terminalPrintDecimal(x.available);
		terminalPrintChar('\n');
	}
}
