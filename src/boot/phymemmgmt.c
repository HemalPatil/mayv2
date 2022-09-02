#include <commonstrings.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>

const size_t pageSizeShift = 12;
const size_t pageSize = 1 << pageSizeShift;

ACPI3Entry* mmap = 0;
uint8_t* phyMemBuddyBitmaps[PHY_MEM_BUDDY_MAX_ORDER] = { 0 };
size_t phyMemBuddyBitmapSizes[PHY_MEM_BUDDY_MAX_ORDER] = { 0 };
uint64_t phyMemBuddyMasks[PHY_MEM_BUDDY_MAX_ORDER] = { 0 };
size_t phyMemBuddySizes[PHY_MEM_BUDDY_MAX_ORDER] = { 0 };
size_t phyMemPagesAvailableCount = 0;
size_t phyMemPagesTotalCount = 0;
uint64_t phyMemTotalSize = 0;
uint64_t phyMemUsableSize = 0;

static const char* const initPhyMemStr = "Initializing physical memory management...\n";
static const char* const initPhyMemCompleteStr = "Physical memory management initialized\n\n";
static const char* const phyMemStr = "physical memory ";
static const char* const totalStr = "Total ";
static const char* const usableStr = "Usable ";
static const char* const mmapBaseStr = "MMAP Base ";
static const char* const countStr = "Number of MMAP entries ";
static const char* const mmapTableHeader = "Base address         Length               Type\n";
static const char* const phyMemTableHeader = "Base                 Length\n";
static const char* const rangeOfUsed = "Range of used physical memory\n";
static const char* const creatingBuddy = "Creating buddy bitmaps...";
static const char* const availCountStr = "Pages available ";

// Initializes the physical memory for use by higher level virtual memory manager and other kernel services
bool initializePhysicalMemory(
	void* usablePhyMemStart,
	size_t kernelLowerHalfSize,
	size_t kernelHigherHalfSize,
	size_t *phyMemBuddyPagesCount
) {
	terminalPrintString(initPhyMemStr, strlen(initPhyMemStr));

	// Display diagnostic information about memory like MMAP entries,
	// physical and usable memory size
	initMmap();

	initPhysicalMemorySize();
	terminalPrintSpaces4();
	terminalPrintString(totalStr, strlen(totalStr));
	terminalPrintString(phyMemStr, strlen(phyMemStr));
	terminalPrintHex(&phyMemTotalSize, sizeof(phyMemTotalSize));
	terminalPrintChar('\n');

	initUsablePhysicalMemorySize();
	terminalPrintSpaces4();
	terminalPrintString(usableStr, strlen(usableStr));
	terminalPrintString(phyMemStr, strlen(phyMemStr));
	terminalPrintHex(&phyMemUsableSize, sizeof(phyMemUsableSize));
	terminalPrintChar('\n');

	// Any chunk of memory left towards the end that doesn't fit in pageSize is ignored
	phyMemPagesTotalCount = phyMemPagesAvailableCount = phyMemTotalSize / pageSize;

	// The memory has important structures in 1st MiB; will be marked used and system
	// Kernel process must be marked used and system
	// Unknown number (at least 11) of pages are being occupied by PML4T starting from infoTable->pml4eRootPhysicalAddress
	// Create buddy bitmap right after PML4T
	// Virtual memory manager will take of marking PML4T as used during its initialization
	*phyMemBuddyPagesCount = 0;
	uint64_t totalBytesRequired = 0;
	terminalPrintSpaces4();
	terminalPrintString(creatingBuddy, strlen(creatingBuddy));
	for (size_t i = 0; i < PHY_MEM_BUDDY_MAX_ORDER; ++i) {
		phyMemBuddySizes[i] = 1 << i;
		phyMemBuddyMasks[i] = ~(phyMemBuddySizes[i] * pageSize - 1);
		uint64_t currentLevelBitsRequired = phyMemPagesTotalCount / phyMemBuddySizes[i];
		uint64_t currentLevelBytesRequired = currentLevelBitsRequired / 8;
		if (currentLevelBitsRequired != currentLevelBytesRequired * 8) {
			++currentLevelBytesRequired;
		}
		totalBytesRequired += currentLevelBytesRequired;
		phyMemBuddyBitmaps[i] = (i == 0 ? (uint8_t*)usablePhyMemStart : phyMemBuddyBitmaps[i - 1]) + phyMemBuddyBitmapSizes[i - 1];
		phyMemBuddyBitmapSizes[i] = currentLevelBytesRequired;
	}
	*phyMemBuddyPagesCount = totalBytesRequired / pageSize;
	if (*phyMemBuddyPagesCount * pageSize != totalBytesRequired) {
		++(*phyMemBuddyPagesCount);
	}
	memset(phyMemBuddyBitmaps[0], 0, *phyMemBuddyPagesCount * pageSize);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Mark 0x0-L32K64_SCRATCH_BASE and (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH)-1MiB as used
	markPhysicalPages(0, L32K64_SCRATCH_BASE / pageSize, PHY_MEM_USED);
	markPhysicalPages(
		(void*)(L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH),
		(0x100000 - (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH)) / pageSize,
		PHY_MEM_USED
	);

	// Mark pages used by higher half of kernel process as used
	// All the kernel sections are aligned at 4KiB in the linker
	markPhysicalPages((void*)(infoTable->kernelPhyMemBase + kernelLowerHalfSize), kernelHigherHalfSize / pageSize, PHY_MEM_USED);

	// Mark the phyMemBuddyBitmaps themselves as used
	markPhysicalPages(phyMemBuddyBitmaps[0], *phyMemBuddyPagesCount, PHY_MEM_USED);

	// Mark all the unusable areas in MMAP as used
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		if (mmap[i].regionType == ACPI3_MEM_TYPE_USABLE) {
			continue;
		}
		size_t unusuableCount = mmap[i].length / pageSize;
		if (mmap[i].length != unusuableCount * pageSize) {
			++unusuableCount;
		}
		markPhysicalPages((void*) mmap[i].base, unusuableCount, PHY_MEM_USED);
	}

	terminalPrintString(initPhyMemCompleteStr, strlen(initPhyMemCompleteStr));
	return true;
}

// Returns the closest size match physical buddy
// If wastage in a buddy is more than 25% of its size then returns a smaller size buddy
// Actual number of pages assigned is returned in allocatedCount
// Returns INVALID_ADDRESS and allocatedCount = 0 if request count is greater than currently available pages
PhysicalPageRequestResult requestPhysicalPages(size_t count, uint8_t flags) {
	PhysicalPageRequestResult result;
	result.address = INVALID_ADDRESS;
	result.allocatedCount = 0;
	// FIXME: handle requests for sizes > 2MiB i.e. 512 count
	// FIXME: handle contiguous allocation
	if (
		count == 0 ||
		count > phyMemPagesAvailableCount ||
		count > phyMemBuddySizes[PHY_MEM_BUDDY_MAX_ORDER - 1] ||
		flags & PHY_MEM_ALLOCATE_CONTIGUOUS
	) {
		return result;
	}
	size_t closestLevel;
	for (closestLevel = 0; closestLevel < PHY_MEM_BUDDY_MAX_ORDER; ++closestLevel) {
		if (count <= phyMemBuddySizes[closestLevel]) {
			break;
		}
	}
	bool perfectFit = count == phyMemBuddySizes[closestLevel];
	size_t byte, bit;
	uint8_t currentBitmap;
	uint64_t addr;
	bool found;
	if (!perfectFit) {
		// Page requests of count == 1 || count == 2 will have perfect fits
		// So it's guaranteed that closestLevel is >= 2 here
		// If remaining count is < phyMemBuddySizes[closestLevel - 2], i.e. count < 0.75 * phyMemBuddySizes[closestLevel]
		// find and assign a buddy of closest fit less than count or go down to lower levels and return address
		// otherwise find and assign a buddy from closestLevel or go down to lower levels
		size_t remaining = count - phyMemBuddySizes[closestLevel - 1];
		if (remaining < phyMemBuddySizes[closestLevel - 2]) {
			--closestLevel;
		}
	}
	found = false;
	for (byte = 0; byte < phyMemBuddyBitmapSizes[closestLevel]; ++byte) {
		if (phyMemBuddyBitmaps[closestLevel][byte] != 0xff) {
			found = true;
			break;
		}
	}
	if (found) {
		currentBitmap = phyMemBuddyBitmaps[closestLevel][byte];
		for (bit = 0; bit < 8; ++bit) {
			if (!(currentBitmap & 1)) {
				break;
			}
			currentBitmap >>= 1;
		}
		addr = (bit + byte * 8) << (pageSizeShift + closestLevel);
		markPhysicalPages((void*) addr, phyMemBuddySizes[closestLevel], PHY_MEM_USED);
		result.allocatedCount = phyMemBuddySizes[closestLevel];
		result.address = (void*) addr;
		return result;
	} else {
		// Page requests of count == 1 will have perfect fits
		// So it's guaranteed that closestLevel is >= 1 here
		// Go down to lower levels, find the biggest possible buddy
		// that can be assigned and return its address
		for (int i = closestLevel - 1; i >= 0; --i) {
			found = false;
			for (byte = 0; byte < phyMemBuddyBitmapSizes[i]; ++byte) {
				if (phyMemBuddyBitmaps[i][byte] != 0xff) {
					found = true;
					break;
				}
			}
			if (found) {
				currentBitmap = phyMemBuddyBitmaps[i][byte];
				for (bit = 0; bit < 8; ++bit) {
					if (!(currentBitmap & 1)) {
						break;
					}
					currentBitmap >>= 1;
				}
				addr = (bit + byte * 8) << (pageSizeShift + i);
				markPhysicalPages((void*) addr, phyMemBuddySizes[i], PHY_MEM_USED);
				result.allocatedCount = phyMemBuddySizes[i];
				result.address = (void*) addr;
				return result;
			}
		}
	}
	return result;
}

// Returns the byte and bit index of a physical buddy
// Returns SIZE_MAX in both byte and bit if the address or order is out of bounds
PhyMemBuddyBitmapIndex getPhysicalBuddyBitmapIndex(void* address, size_t order) {
	uint64_t addr = (uint64_t) address;
	PhyMemBuddyBitmapIndex index;
	if (addr >= phyMemPagesTotalCount * pageSize || order >= PHY_MEM_BUDDY_MAX_ORDER) {
		index.byte = index.bit = SIZE_MAX;
		return index;
	}
	addr >>= (pageSizeShift + order);
	index.byte = addr / 8;
	index.bit = addr - index.byte * 8;
	return index;
}

// Marks physical pages as used or free in the physical memory manager
void markPhysicalPages(void* address, size_t count, uint8_t type) {
	if (count == 0) {
		return;
	}

	uint64_t addr = (uint64_t) address;
	if (addr >= phyMemPagesTotalCount * pageSize) {
		return;
	}

	// Set all the pages at buddy order 0 first and then build up from there
	addr &= phyMemBuddyMasks[0];
	// TODO: performance can be improved by working in groups of 8 pages
	for (size_t i = 0; i < count; ++i) {
		PhyMemBuddyBitmapIndex index = getPhysicalBuddyBitmapIndex((void*)(addr + i * pageSize), 0);
		bool currentUsed = phyMemBuddyBitmaps[0][index.byte] & (1 << index.bit);
		if (type == PHY_MEM_USED) {
			phyMemBuddyBitmaps[0][index.byte] |= (1 << index.bit);
			if (!currentUsed) {
				--phyMemPagesAvailableCount;
			}
		} else if (type == PHY_MEM_FREE) {
			phyMemBuddyBitmaps[0][index.byte] &= ~((uint8_t)1 << index.bit);
			if (currentUsed) {
				++phyMemPagesAvailableCount;
			}
		}
	}

	// Sync higher order buddies
	size_t levelCount = count;
	for (size_t i = 1; i < PHY_MEM_BUDDY_MAX_ORDER; ++i) {
		if (levelCount == 0) {
			levelCount = 2;
		} else if (levelCount & 1) {
			++levelCount;
		}
		levelCount >>= 1;
		for (size_t j = 0; j < levelCount; ++j) {
			uint64_t currentLevelAddr = (addr & phyMemBuddyMasks[i]) + j * phyMemBuddySizes[i] * pageSize;
			bool bothBuddiesFree = arePhysicalBuddiesOfType((void*)currentLevelAddr, i - 1, 2, PHY_MEM_FREE);
			if (!bothBuddiesFree) {
				PhyMemBuddyBitmapIndex index = getPhysicalBuddyBitmapIndex((void*)currentLevelAddr, i);
				phyMemBuddyBitmaps[i][index.byte] |= (1 << index.bit);
			}
		}
	}
}

// Debug helper to list all MMAP entries
void listMmapEntries() {
	terminalPrintString(mmapBaseStr, strlen(mmapBaseStr));
	terminalPrintHex(&mmap, sizeof(mmap));
	terminalPrintChar('\n');

	size_t i = 0;
	terminalPrintString(countStr, strlen(countStr));
	terminalPrintHex(&infoTable->mmapEntryCount, sizeof(infoTable->mmapEntryCount));
	terminalPrintChar('\n');
	terminalPrintString(mmapTableHeader, strlen(mmapTableHeader));
	for (i = 0; i < infoTable->mmapEntryCount; ++i) {
		terminalPrintSpaces4();
		terminalPrintHex(&(mmap[i].base), sizeof(mmap->base));
		terminalPrintChar(' ');
		terminalPrintHex(&(mmap[i].length), sizeof(mmap->length));
		terminalPrintChar(' ');
		terminalPrintHex(&(mmap[i].regionType), sizeof(mmap->regionType));
		terminalPrintChar('\n');
	}
}

// Debug helper to list all physical buddies at given order marked as used
void listUsedPhysicalBuddies(size_t order) {
	if (order >= PHY_MEM_BUDDY_MAX_ORDER) {
		return;
	}
	terminalPrintString(rangeOfUsed, strlen(rangeOfUsed));
	terminalPrintSpaces4();
	terminalPrintString(availCountStr, strlen(availCountStr));
	terminalPrintHex(&phyMemPagesAvailableCount, sizeof(phyMemPagesAvailableCount));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(phyMemTableHeader, strlen(phyMemTableHeader));
	uint64_t lastUsed = 0, length = 0;
	bool ongoingAvailable = false;
	for (uint64_t i = 0; i < phyMemPagesTotalCount * pageSize; i += phyMemBuddySizes[order] * pageSize) {
		if (arePhysicalBuddiesOfType((void*)i, order, 1, PHY_MEM_FREE)) {
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
			length += phyMemBuddySizes[order] * pageSize;
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
bool arePhysicalBuddiesOfType(void* address, size_t order, size_t count, uint8_t type) {
	if (count == 0) {
		return true;
	}

	uint64_t addr = (uint64_t) address;
	if (addr >= phyMemPagesTotalCount * pageSize) {
		return false;
	}
	addr &= phyMemBuddyMasks[order];
	for (size_t i = 0; i < count; ++i) {
		PhyMemBuddyBitmapIndex index = getPhysicalBuddyBitmapIndex(
			(void*)(addr + i * phyMemBuddySizes[order] * pageSize),
			order
		);
		if ((phyMemBuddyBitmaps[order][index.byte] & (1 << index.bit) ? PHY_MEM_USED : PHY_MEM_FREE) != type) {
			return false;
		}
	}
	return true;
}

// Initializes base address of MMAP entries
void initMmap() {
	mmap = (ACPI3Entry*)((uint64_t)(infoTable->mmapEntriesSegment << 4) + infoTable->mmapEntriesOffset);
}

// Initializes physical memory total size in bytes
void initPhysicalMemorySize() {
	phyMemTotalSize = 0;
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		phyMemTotalSize += mmap[i].length;
	}
}

// Initializes usable (conventional ACPI3_MEM_TYPE_USABLE) physical memory size
void initUsablePhysicalMemorySize() {
	phyMemUsableSize = 0;
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		if (mmap[i].regionType == ACPI3_MEM_TYPE_USABLE) {
			phyMemUsableSize += mmap[i].length;
		}
	}
	// Make it 4KiB aligned
	phyMemUsableSize = (phyMemUsableSize >> pageSizeShift) * pageSize;
}