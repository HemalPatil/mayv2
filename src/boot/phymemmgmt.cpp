#include <commonstrings.h>
#include <cstring>
#include <kernel.h>
#include <terminal.h>

static size_t phyMemPagesAvailableCount = 0;
static size_t phyMemPagesTotalCount = 0;
static size_t phyMemTotalSize = 0;
static size_t phyMemUsableSize = 0;

static const char* const initPhyMemStr = "Initializing physical memory management";
static const char* const initPhyMemCompleteStr = "Physical memory management initialized\n\n";
static const char* const phyMemStr = "physical memory ";
static const char* const totalStr = "Total ";
static const char* const usableStr = "Usable ";
static const char* const mmapBaseStr = "MMAP Base ";
static const char* const countStr = "Number of MMAP entries ";
static const char* const mmapTableHeader = "Base address         Length               Type\n";
static const char* const phyMemTableHeader = "Base                 Length\n";
static const char* const rangeOfUsed = "Range of used physical memory\n";
static const char* const creatingBuddyStr = "Creating buddy bitmaps";

const size_t Kernel::Memory::pageSizeShift = 12;
const size_t Kernel::Memory::pageSize = 1 << pageSizeShift;

ACPI3Entry* Kernel::Memory::Physical::map = nullptr;
uint8_t* Kernel::Memory::Physical::buddyBitmaps[PHY_MEM_BUDDY_MAX_ORDER] = { 0 };
size_t Kernel::Memory::Physical::buddyBitmapSizes[PHY_MEM_BUDDY_MAX_ORDER] = { 0 };
uint64_t Kernel::Memory::Physical::buddyMasks[PHY_MEM_BUDDY_MAX_ORDER] = { 0 };
size_t Kernel::Memory::Physical::buddySizes[PHY_MEM_BUDDY_MAX_ORDER] = { 0 };

// Initializes the physical memory for use by higher level virtual memory manager and other kernel services
bool Kernel::Memory::Physical::initialize(
	void* usablePhyMemStart,
	size_t kernelLowerHalfSize,
	size_t kernelHigherHalfSize,
	size_t &phyMemBuddyPagesCount
) {
	terminalPrintString(initPhyMemStr, strlen(initPhyMemStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Display diagnostic information about memory like MMAP entries,
	// physical and usable memory size
	initMap();

	initSize();
	terminalPrintSpaces4();
	terminalPrintString(totalStr, strlen(totalStr));
	terminalPrintString(phyMemStr, strlen(phyMemStr));
	terminalPrintHex(&phyMemTotalSize, sizeof(phyMemTotalSize));
	terminalPrintChar('\n');

	initUsableSize();
	terminalPrintSpaces4();
	terminalPrintString(usableStr, strlen(usableStr));
	terminalPrintString(phyMemStr, strlen(phyMemStr));
	terminalPrintHex(&phyMemUsableSize, sizeof(phyMemUsableSize));
	terminalPrintChar('\n');

	// Any chunk of memory left towards the end that doesn't fit in pageSize is ignored
	phyMemPagesTotalCount = phyMemPagesAvailableCount = phyMemTotalSize / pageSize;

	// The memory has important structures in 1st MiB; will be marked used and system
	// Kernel process must be marked used and system
	// Unknown number (at least 11) of pages are being occupied by PML4T starting from infoTable->pml4tPhysicalAddress
	// Create buddy bitmap right after PML4T
	// Virtual memory manager will take of marking PML4T as used during its initialization
	phyMemBuddyPagesCount = 0;
	uint64_t totalBytesRequired = 0;
	terminalPrintSpaces4();
	terminalPrintString(creatingBuddyStr, strlen(creatingBuddyStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	for (size_t i = 0; i < PHY_MEM_BUDDY_MAX_ORDER; ++i) {
		buddySizes[i] = 1 << i;
		buddyMasks[i] = ~(buddySizes[i] * pageSize - 1);
		uint64_t currentLevelBitsRequired = phyMemPagesTotalCount / buddySizes[i];
		uint64_t currentLevelBytesRequired = currentLevelBitsRequired / 8;
		if (currentLevelBitsRequired != currentLevelBytesRequired * 8) {
			++currentLevelBytesRequired;
		}
		totalBytesRequired += currentLevelBytesRequired;
		buddyBitmaps[i] = (i == 0 ? (uint8_t*)usablePhyMemStart : buddyBitmaps[i - 1]) + buddyBitmapSizes[i - 1];
		buddyBitmapSizes[i] = currentLevelBytesRequired;
	}
	phyMemBuddyPagesCount = totalBytesRequired / pageSize;
	if (phyMemBuddyPagesCount * pageSize != totalBytesRequired) {
		++phyMemBuddyPagesCount;
	}
	memset(buddyBitmaps[0], 0, phyMemBuddyPagesCount * pageSize);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Mark 0x0-L32K64_SCRATCH_BASE and (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH)-1MiB as used
	markPages(0, L32K64_SCRATCH_BASE / pageSize, MarkPageType::Used);
	markPages(
		(void*)(L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH),
		(0x100000 - (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH)) / pageSize,
		MarkPageType::Used
	);

	// Mark pages used by higher half of kernel process as used
	// All the kernel sections are aligned at 4KiB in the linker
	markPages((void*)(infoTable->kernelPhyMemBase + kernelLowerHalfSize), kernelHigherHalfSize / pageSize, MarkPageType::Used);

	// Mark the phyMemBuddyBitmaps themselves as used
	markPages(buddyBitmaps[0], phyMemBuddyPagesCount, MarkPageType::Used);

	// Mark all the unusable areas in MMAP as used
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		if (map[i].regionType == ACPI3_MEM_TYPE_USABLE) {
			continue;
		}
		size_t unusuableCount = map[i].length / pageSize;
		if (map[i].length != unusuableCount * pageSize) {
			++unusuableCount;
		}
		markPages((void*) map[i].base, unusuableCount, MarkPageType::Used);
	}

	terminalPrintString(initPhyMemCompleteStr, strlen(initPhyMemCompleteStr));
	return true;
}

// Returns the closest size match physical buddy
// If wastage in a buddy is more than 25% of its size then returns a smaller size buddy
// Actual number of pages assigned is returned in allocatedCount
// Returns INVALID_ADDRESS and allocatedCount = 0 if request count is count == 0
// or greater than currently available pages
// Unsafe to call this function until virtual memory manager is initialized
Kernel::Memory::PageRequestResult Kernel::Memory::Physical::requestPages(size_t count, uint32_t flags) {
	PageRequestResult result;
	// FIXME: handle requests for sizes > 2MiB i.e. 512 count
	// FIXME: handle contiguous allocation
	if (
		count == 0 ||
		count > phyMemPagesAvailableCount ||
		count > buddySizes[PHY_MEM_BUDDY_MAX_ORDER - 1] ||
		flags & RequestType::Contiguous
	) {
		return result;
	}
	size_t closestLevel;
	for (closestLevel = 0; closestLevel < PHY_MEM_BUDDY_MAX_ORDER; ++closestLevel) {
		if (count <= buddySizes[closestLevel]) {
			break;
		}
	}
	bool perfectFit = count == buddySizes[closestLevel];
	size_t byte, bit;
	uint8_t currentBitmap;
	uint64_t addr;
	bool found;
	if (!perfectFit) {
		// Page requests of count == 1 || count == 2 will have perfect fits
		// So it's guaranteed that closestLevel is >= 2 here
		// If remaining count is < buddySizes[closestLevel - 2], i.e. count < 0.75 * buddySizes[closestLevel]
		// find and assign a buddy of closest fit less than count or go down to lower levels and return address
		// otherwise find and assign a buddy from closestLevel or go down to lower levels
		size_t remaining = count - buddySizes[closestLevel - 1];
		if (remaining < buddySizes[closestLevel - 2]) {
			--closestLevel;
		}
	}
	found = false;
	for (byte = 0; byte < buddyBitmapSizes[closestLevel]; ++byte) {
		if (buddyBitmaps[closestLevel][byte] != 0xff) {
			found = true;
			break;
		}
	}
	if (found) {
		currentBitmap = buddyBitmaps[closestLevel][byte];
		for (bit = 0; bit < 8; ++bit) {
			if (!(currentBitmap & 1)) {
				break;
			}
			currentBitmap >>= 1;
		}
		addr = (bit + byte * 8) << (pageSizeShift + closestLevel);
		markPages((void*) addr, buddySizes[closestLevel], MarkPageType::Used);
		result.allocatedCount = buddySizes[closestLevel];
		result.address = (void*) addr;
		return result;
	} else {
		// Page requests of count == 1 will have perfect fits
		// So it's guaranteed that closestLevel is >= 1 here
		// Go down to lower levels, find the biggest possible buddy
		// that can be assigned and return its address
		for (int i = closestLevel - 1; i >= 0; --i) {
			found = false;
			for (byte = 0; byte < buddyBitmapSizes[i]; ++byte) {
				if (buddyBitmaps[i][byte] != 0xff) {
					found = true;
					break;
				}
			}
			if (found) {
				currentBitmap = buddyBitmaps[i][byte];
				for (bit = 0; bit < 8; ++bit) {
					if (!(currentBitmap & 1)) {
						break;
					}
					currentBitmap >>= 1;
				}
				addr = (bit + byte * 8) << (pageSizeShift + i);
				markPages((void*) addr, buddySizes[i], MarkPageType::Used);
				result.allocatedCount = buddySizes[i];
				result.address = (void*) addr;
				return result;
			}
		}
	}
	return result;
}

// Returns the byte and bit index of a physical buddy
// Returns SIZE_MAX in both byte and bit if the address or order is out of bounds
Kernel::Memory::Physical::BuddyBitmapIndex Kernel::Memory::Physical::getBuddyBitmapIndex(void* address, size_t order) {
	uint64_t addr = (uint64_t) address;
	BuddyBitmapIndex index;
	if (addr >= phyMemPagesTotalCount * pageSize || order >= PHY_MEM_BUDDY_MAX_ORDER) {
		return index;
	}
	addr >>= (pageSizeShift + order);
	index.byte = addr / 8;
	index.bit = addr - index.byte * 8;
	return index;
}

// Marks physical pages as used or free in the physical memory manager
void Kernel::Memory::Physical::markPages(void* address, size_t count, MarkPageType type) {
	if (count == 0) {
		return;
	}

	uint64_t addr = (uint64_t) address;
	if (addr >= phyMemPagesTotalCount * pageSize) {
		return;
	}

	// Set all the pages at buddy order 0 first and then build up from there
	addr &= buddyMasks[0];
	// TODO: performance can be improved by working in groups of 8 pages
	for (size_t i = 0; i < count; ++i) {
		BuddyBitmapIndex index = getBuddyBitmapIndex((void*)(addr + i * pageSize), 0);
		bool currentUsed = buddyBitmaps[0][index.byte] & (1 << index.bit);
		if (type == MarkPageType::Used) {
			buddyBitmaps[0][index.byte] |= (1 << index.bit);
			if (!currentUsed) {
				--phyMemPagesAvailableCount;
			}
		} else if (type == MarkPageType::Free) {
			buddyBitmaps[0][index.byte] &= ~((uint8_t)1 << index.bit);
			if (currentUsed) {
				++phyMemPagesAvailableCount;
			}
		}
	}

	// Sync higher order buddies
	uint64_t endAddr = addr + count * pageSize;
	for (size_t i = 1; i < PHY_MEM_BUDDY_MAX_ORDER; ++i) {
		uint64_t currentLevelAddr = addr & buddyMasks[i];
		while (currentLevelAddr < endAddr) {
			bool bothBuddiesFree = areBuddiesOfType((void*)currentLevelAddr, i - 1, 2, MarkPageType::Free);
			if (!bothBuddiesFree) {
				BuddyBitmapIndex index = getBuddyBitmapIndex((void*)currentLevelAddr, i);
				buddyBitmaps[i][index.byte] |= (1 << index.bit);
			}
			currentLevelAddr += buddySizes[i] * pageSize;
		}
	}
}

// Debug helper to list all MMAP entries
void Kernel::Memory::Physical::listMapEntries() {
	terminalPrintString(mmapBaseStr, strlen(mmapBaseStr));
	terminalPrintHex(&map, sizeof(map));
	terminalPrintChar('\n');

	size_t i = 0;
	terminalPrintString(countStr, strlen(countStr));
	terminalPrintHex(&infoTable->mmapEntryCount, sizeof(infoTable->mmapEntryCount));
	terminalPrintChar('\n');
	terminalPrintString(mmapTableHeader, strlen(mmapTableHeader));
	for (i = 0; i < infoTable->mmapEntryCount; ++i) {
		terminalPrintSpaces4();
		terminalPrintHex(&(map[i].base), sizeof(map->base));
		terminalPrintChar(' ');
		terminalPrintHex(&(map[i].length), sizeof(map->length));
		terminalPrintChar(' ');
		terminalPrintHex(&(map[i].regionType), sizeof(map->regionType));
		terminalPrintChar('\n');
	}
}

// Debug helper to list all physical buddies at given order marked as used
void Kernel::Memory::Physical::listUsedBuddies(size_t order) {
	if (order >= PHY_MEM_BUDDY_MAX_ORDER) {
		return;
	}
	terminalPrintString(rangeOfUsed, strlen(rangeOfUsed));
	terminalPrintSpaces4();
	terminalPrintString(pagesAvailableStr, strlen(pagesAvailableStr));
	terminalPrintHex(&phyMemPagesAvailableCount, sizeof(phyMemPagesAvailableCount));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(phyMemTableHeader, strlen(phyMemTableHeader));
	uint64_t lastUsed = 0, length = 0;
	bool ongoingAvailable = false;
	for (uint64_t i = 0; i < phyMemPagesTotalCount * pageSize; i += buddySizes[order] * pageSize) {
		if (areBuddiesOfType((void*)i, order, 1, MarkPageType::Free)) {
			if (ongoingAvailable) {
				continue;
			} else {
				lastUsed = i - length;
				terminalPrintSpaces4();
				terminalPrintHex(&lastUsed, sizeof(lastUsed));
				terminalPrintChar(' ');
				terminalPrintHex(&length, sizeof(length));
				terminalPrintChar('\n');
				length = 0;
				ongoingAvailable = true;
			}
		} else {
			length += buddySizes[order] * pageSize;
			ongoingAvailable = false;
		}
	}
	if (!ongoingAvailable) {
		lastUsed = phyMemPagesTotalCount * pageSize - length;
		terminalPrintSpaces4();
		terminalPrintHex(&lastUsed, sizeof(lastUsed));
		terminalPrintHex(&length, sizeof(length));
		terminalPrintChar('\n');
	}
}

// Returns true if all the physical buddies starting at a given address and order
// are of given type, false if even one buddy is different
bool Kernel::Memory::Physical::areBuddiesOfType(void* address, size_t order, size_t count, MarkPageType type) {
	if (count == 0) {
		return true;
	}

	uint64_t addr = (uint64_t) address;
	if (addr >= phyMemPagesTotalCount * pageSize) {
		return false;
	}
	addr &= buddyMasks[order];
	for (size_t i = 0; i < count; ++i) {
		BuddyBitmapIndex index = getBuddyBitmapIndex(
			(void*)(addr + i * buddySizes[order] * pageSize),
			order
		);
		if ((buddyBitmaps[order][index.byte] & (1 << index.bit) ? MarkPageType::Used : MarkPageType::Free) != type) {
			return false;
		}
	}
	return true;
}

// Initializes base address of MMAP entries
void Kernel::Memory::Physical::initMap() {
	map = (ACPI3Entry*)((uint64_t)(infoTable->mmapEntriesSegment << 4) + infoTable->mmapEntriesOffset);
}

// Initializes physical memory total size in bytes
void Kernel::Memory::Physical::initSize() {
	phyMemTotalSize = 0;
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		phyMemTotalSize += map[i].length;
	}
}

// Initializes usable (conventional ACPI3_MEM_TYPE_USABLE) physical memory size
void Kernel::Memory::Physical::initUsableSize() {
	phyMemUsableSize = 0;
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		if (map[i].regionType == ACPI3_MEM_TYPE_USABLE) {
			phyMemUsableSize += map[i].length;
		}
	}
	// Make it 4KiB aligned
	phyMemUsableSize = (phyMemUsableSize >> pageSizeShift) * pageSize;
}
