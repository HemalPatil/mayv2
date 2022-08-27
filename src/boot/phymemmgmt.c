#include <phymemmgmt.h>
#include <kernel.h>
#include <string.h>
#include <terminal.h>

const size_t pageSizeShift = 12;
const size_t pageSize = 1 << pageSizeShift;

ACPI3Entry* mmap = 0;
uint64_t pageAddressMasks[PHY_MEM_BUDDY_MAX_ORDER] = {0};
uint8_t* phyMemBuddyBitmaps[PHY_MEM_BUDDY_MAX_ORDER] = { 0 };
size_t phyMemBuddySizes[PHY_MEM_BUDDY_MAX_ORDER] = { 0 };
size_t phyMemPagesAvailableCount = 0;
size_t phyMemPagesTotalCount = 0;
uint64_t phyMemTotalSize = 0;
uint64_t phyMemUsableSize = 0;

const char* const initPhyMemStr = "Initializing physical memory management...\n";
const char* const initPhyMemCompleteStr = "    Physical memory management initialized\n";
const char* const phyMemSizeStr = "    Total size ";
const char* const usableMemSizeStr = "    Usable size ";
const char* const mmapBaseStr = "MMAP Base ";
const char* const countStr = "Number of MMAP entries ";
const char* const tableHeader = "    Base address         Length               Type\n";
const char* const phyMemBitmapStr = "    Allocator bitmap location ";
const char* const phyMemBitmapSizeStr = " size ";
const char* const rangeOfUsed = "Range of used physical memory\n";
const char* const invalidBuddyAccess = "Invalid physical memory buddy bitmap access for address ";
const char* const buddyStr = " buddy level ";
const char* const buddyAddrStr = "Buddy system levels\n";
const char* const levelStr = "Level ";
const char* const availCountStr = "Pages available ";

const uint64_t mib1 = 0x100000;

// Initializes the physical memory for use by higher level virtual memory manager and other kernel services
bool initializePhysicalMemory(
	void* usableMemoryStart,
	size_t kernelVirtualMemorySize,
	void* kernelElfBase,
	size_t kernelElfSize,
	size_t *phyMemBuddyPagesCount
) {
	terminalPrintString(initPhyMemStr, strlen(initPhyMemStr));

	// Display diagnostic information about memory like MMAP entries,
	// physical and usable memory size
	initMmap();

	initPhysicalMemorySize();
	terminalPrintString(phyMemSizeStr, strlen(phyMemSizeStr));
	terminalPrintHex(&phyMemTotalSize, sizeof(phyMemTotalSize));
	terminalPrintChar('\n');

	initUsablePhysicalMemorySize();
	terminalPrintString(usableMemSizeStr, strlen(usableMemSizeStr));
	terminalPrintHex(&phyMemUsableSize, sizeof(phyMemUsableSize));
	terminalPrintChar('\n');

	// Any chunk of memory left towards the end that doesn't fit in pageSize is ignored
	phyMemPagesTotalCount = phyMemPagesAvailableCount = phyMemTotalSize / pageSize;

	// The memory has important structures in 1st MiB; will be marked used and system
	// Kernel process must be marked used and system
	// Unknown number (at least 11) of pages are being occupied by PML4T starting from infoTable->pml4eRoot
	// Create buddy bitmap right after PML4 entries
	// Virtual memory manager will take of marking PML4 entries as used during its initialization
	*phyMemBuddyPagesCount = 0;
	uint64_t totalBytesRequired = 0;
	uint64_t previousLevelBytesRequired = 0;
	terminalPrintSpaces4();
	terminalPrintString(buddyAddrStr, strlen(buddyAddrStr));
	for (size_t i = 0; i < PHY_MEM_BUDDY_MAX_ORDER; ++i) {
		phyMemBuddySizes[i] = 1 << i;
		pageAddressMasks[i] = ~(phyMemBuddySizes[i] * pageSize - 1);
		uint64_t currentLevelBitsRequired = phyMemPagesTotalCount / phyMemBuddySizes[i];
		uint64_t currentLevelBytesRequired = currentLevelBitsRequired / 8;
		if (currentLevelBitsRequired != currentLevelBytesRequired * 8) {
			++currentLevelBytesRequired;
		}
		totalBytesRequired += currentLevelBytesRequired;
		phyMemBuddyBitmaps[i] = (i == 0 ? (uint8_t*)usableMemoryStart : phyMemBuddyBitmaps[i - 1]) + previousLevelBytesRequired;
		previousLevelBytesRequired = currentLevelBytesRequired;
	}
	*phyMemBuddyPagesCount = totalBytesRequired / pageSize;
	if (*phyMemBuddyPagesCount * pageSize != totalBytesRequired) {
		++(*phyMemBuddyPagesCount);
	}
	memset(phyMemBuddyBitmaps[0], 0, *phyMemBuddyPagesCount * pageSize);

	markPhysicalPages(0, 33, PhyMemEntry_Used);

	// Mark 0x0-0x80000 and 0x90000-1MiB as used
	markPhysicalPages(0, L32K64_SCRATCH_BASE / pageSize, PhyMemEntry_Used);
	markPhysicalPages(
		(void*)(L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH),
		(mib1 - (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH)) / pageSize,
		PhyMemEntry_Used
	);

	// Mark kernel process as used
	// All the kernel sections are aligned at 4KiB in the linker
	markPhysicalPages((void*)infoTable->kernel64PhyMemBase, kernelVirtualMemorySize / pageSize, PhyMemEntry_Used);

	// Mark kernel ELF as used for now
	// It will be freed during virtual memory initialization
	markPhysicalPages(kernelElfBase, kernelElfSize / pageSize, PhyMemEntry_Used);

	// Mark the phyMemBuddyBitmaps itself as used
	markPhysicalPages(phyMemBuddyBitmaps[0], *phyMemBuddyPagesCount, PhyMemEntry_Used);

	// Mark all the unusable areas in MMAP as used
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		if (mmap[i].regionType == ACPI3_MemType_Usable) {
			continue;
		}
		size_t unusuableCount = mmap[i].length / pageSize;
		if (mmap[i].length != unusuableCount * pageSize) {
			++unusuableCount;
		}
		markPhysicalPages((void*) mmap[i].base, unusuableCount, PhyMemEntry_Used);
	}

	terminalPrintString(initPhyMemCompleteStr, strlen(initPhyMemCompleteStr));
	return true;
}

PhyMemBuddyBitmapIndex getPhysicalPageBuddyBitmapIndex(void* address, size_t buddyLevel) {
	uint64_t addr = (uint64_t) address;
	PhyMemBuddyBitmapIndex index;
	if (addr >= phyMemPagesTotalCount * pageSize || buddyLevel >= PHY_MEM_BUDDY_MAX_ORDER) {
		index.byte = index.bit = 0xffffffff;
		terminalPrintString(invalidBuddyAccess, strlen(invalidBuddyAccess));
		terminalPrintHex(&addr, sizeof(addr));
		terminalPrintString(buddyStr, strlen(buddyStr));
		terminalPrintHex(&buddyLevel, sizeof(buddyLevel));
		terminalPrintChar('\n');
		return index;
	}
	addr >>= (pageSizeShift + buddyLevel);
	index.byte = addr / 8;
	index.bit = addr - index.byte * 8;
	return index;
}

// Marks physical pages as used in the physical memory manager
void markPhysicalPages(void* address, size_t pageCount, uint8_t type) {
	uint64_t addr = (uint64_t) address;
	if (addr >= phyMemPagesTotalCount * pageSize) {
		return;
	}

	// Set all the pages at buddy level 0 first and then build up from there
	addr &= pageAddressMasks[0];
	// TODO: performance can be improved by working in groups of 8 pages
	for (size_t i = 0; i < pageCount; ++i) {
		PhyMemBuddyBitmapIndex index = getPhysicalPageBuddyBitmapIndex((void*)(addr + i * pageSize), 0);
		bool currentUsed = phyMemBuddyBitmaps[0][index.byte] & (1 << index.bit);
		if (type == PhyMemEntry_Used) {
			phyMemBuddyBitmaps[0][index.byte] |= (1 << index.bit);
			if (!currentUsed) {
				--phyMemPagesAvailableCount;
			}
		} else if (type == PhyMemEntry_Free) {
			phyMemBuddyBitmaps[0][index.byte] &= ~(1 << index.bit);
			if (currentUsed) {
				++phyMemPagesAvailableCount;
			}
		}
	}

	// FIXME: build on upper levels of buddy
	size_t levelCount = pageCount;
	for (size_t i = 1; i < PHY_MEM_BUDDY_MAX_ORDER; ++i) {
		if (levelCount == 0) {
			levelCount = 2;
		} else if (levelCount & 1) {
			++levelCount;
		}
		levelCount >>= 1;
		// terminalPrintDecimal(levelCount);
		for (size_t j = 0; j < levelCount; ++j) {
			uint64_t currentLevelAddr = (addr & pageAddressMasks[i]) + j * phyMemBuddySizes[i] * pageSize;
			bool bothBuddiesFree = arePhysicalBuddiesOfType((void*)currentLevelAddr, i - 1, 2, PhyMemEntry_Free);
			if (!bothBuddiesFree) {
				PhyMemBuddyBitmapIndex index = getPhysicalPageBuddyBitmapIndex((void*)currentLevelAddr, i);
				phyMemBuddyBitmaps[i][index.byte] |= (1 << index.bit);
			}
		}
		// terminalPrintChar('\n');
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
	terminalPrintString(tableHeader, strlen(tableHeader));
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

// Debug helper to list all physical pages marked as used
void listUsedPhysicalBuddies(size_t buddyLevel) {
	terminalPrintString(rangeOfUsed, strlen(rangeOfUsed));
	terminalPrintSpaces4();
	terminalPrintString(availCountStr, strlen(availCountStr));
	terminalPrintHex(&phyMemPagesAvailableCount, sizeof(phyMemPagesAvailableCount));
	terminalPrintChar('\n');
	uint64_t lastUsed = 0, length = 0;
	bool ongoingAvailable = false;
	for (size_t i = 0; i < phyMemTotalSize; i += phyMemBuddySizes[buddyLevel] * pageSize) {
		if (arePhysicalBuddiesOfType((void*)i, buddyLevel, 1, PhyMemEntry_Free)) {
			if (ongoingAvailable) {
				continue;
			} else {
				lastUsed = i - length;
				terminalPrintSpaces4();
				terminalPrintHex(&lastUsed, sizeof(lastUsed));
				terminalPrintHex(&length, sizeof(length));
				terminalPrintChar('\n');
				length = 0;
				ongoingAvailable = true;
			}
		} else {
			length += phyMemBuddySizes[buddyLevel] * pageSize;
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

// Returns true if all the physical pages starting at given address are of given type, false if even one page is different
bool arePhysicalBuddiesOfType(void* address, size_t buddyLevel, size_t buddyCount, uint8_t type) {
	uint64_t addr = (uint64_t) address;
	if (addr >= phyMemPagesTotalCount * pageSize) {
		return false;
	}
	addr &= pageAddressMasks[buddyLevel];
	for (size_t i = 0; i < buddyCount; ++i) {
		PhyMemBuddyBitmapIndex index = getPhysicalPageBuddyBitmapIndex(
			(void*)(addr + i * phyMemBuddySizes[buddyLevel] * pageSize),
			buddyLevel
		);
		if ((phyMemBuddyBitmaps[buddyLevel][index.byte] & (1 << index.bit) ? PhyMemEntry_Used : PhyMemEntry_Free) != type) {
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

// Initializes usable (conventional ACPI3_MemType_Usable) physical memory size
void initUsablePhysicalMemorySize() {
	phyMemUsableSize = 0;
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		if (mmap[i].regionType == ACPI3_MemType_Usable) {
			phyMemUsableSize += mmap[i].length;
		}
	}
	// Make it 4KiB aligned
	phyMemUsableSize = (phyMemUsableSize >> 12) * pageSize;
}