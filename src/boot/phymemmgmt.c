#include "kernel.h"
#include "string.h"

const size_t phyPageSize = 0x1000;
size_t phyPagesCount = 0;
uint8_t* phyMemBitmap = 0;
uint64_t phyMemBitmapSize = 0;
uint64_t phyMemUsableSize;
uint64_t phyMemTotalSize;
ACPI3Entry* mmap = 0;

const char* const initPhyMemStr = "Initializing physical memory management...\n";
const char* const initPhyMemCompleteStr = "    Physical memory management initialized\n";
const char* const phyMemSizeStr = "    Total size ";
const char* const usableMemSizeStr = "    Usable size ";
const char* const mmapBaseStr = "MMAP Base ";
const char* const countStr = "Number of MMAP entries ";
const char* const tableHeader = "    Base address         Length               Type\n";
const char* const phyMemBitmapStr = "    Allocator bitmap location ";
const char* const phyMemBitmapSizeStr = " size ";
const char* const rangeOfUsed = "Range of used memory\n";

const uint64_t mib1 = 0x100000;

// Initializes the physical memory for use by higher level virtual memory manager and other kernel services
bool initializePhysicalMemory(uint64_t usableMemoryStart) {
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

	// Any chunk of memory left towards the end
	// that doesn't fit in phyPageSize is ignored
	phyPagesCount = phyMemTotalSize / phyPageSize;

	// The memory has important structures in 1st MiB; will be marked used and system
	// Kernel process must be marked used and system
	// Unknown number (at least 11) of pages are being occupied by PML4T starting from infoTable->pml4eRoot
	// Create bitmap right after PML4 entries
	// Virtual memory manager will take of marking PML4 entries as used during its initialization
	phyMemBitmap = (uint8_t*)usableMemoryStart;
	terminalPrintString(phyMemBitmapStr, strlen(phyMemBitmapStr));
	terminalPrintHex(&phyMemBitmap, sizeof(phyMemBitmap));
	phyMemBitmapSize = phyPagesCount / 8;
	terminalPrintString(phyMemBitmapSizeStr, strlen(phyMemBitmapSizeStr));
	terminalPrintHex(&phyMemBitmapSize, sizeof(phyMemBitmapSize));
	terminalPrintChar('\n');
	memset(phyMemBitmap, 0, phyMemBitmapSize);

	// Mark first 1MiB as used
	memset(phyMemBitmap, 0xff, mib1 / phyPageSize / 8);

	// Mark kernel process as used
	// All the kernel sections are aligned at 4KiB in the linker
	markPhysicalPagesAsUsed((void*) infoTable->kernel64PhyMemBase, kernelVirtualMemorySize / phyPageSize);

	// Mark the phyMemBitmap itself as used
	size_t bitmapPageCount = phyMemBitmapSize / phyPageSize;
	if (phyMemBitmapSize - bitmapPageCount * phyPageSize) {
		++bitmapPageCount;
	}
	markPhysicalPagesAsUsed(phyMemBitmap, bitmapPageCount);

	// Mark all the unusable areas above 1MiB in MMAP as used
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		if (mmap[i].regionType == ACPI3_MemType_Usable) {
			continue;
		}
		if (mmap[i].base + mmap[i].length < mib1) {
			continue;
		} else if (mmap[i].base < mib1 && mmap[i].base + mmap[i].length > mib1) {
			size_t unusuableCount = (mmap[i].base + mmap[i].length - mib1) / phyPageSize;
			if (mmap[i].length - unusuableCount * phyPageSize > 0) {
				++unusuableCount;
			}
			markPhysicalPagesAsUsed((void*) mmap[i].base, unusuableCount);
		} else if (mmap[i].base > mib1) {
			size_t unusuableCount = mmap[i].length / phyPageSize;
			if (mmap[i].length - unusuableCount * phyPageSize > 0) {
				++unusuableCount;
			}
			markPhysicalPagesAsUsed((void*) mmap[i].base, unusuableCount);
		}
	}

	terminalPrintString(initPhyMemCompleteStr, strlen(initPhyMemCompleteStr));
	return true;
}

PhyMemBitmapIndex getPhysicalPageBitmapIndex(void* address) {
	uint64_t addr = (uint64_t) address;
	PhyMemBitmapIndex index;
	if (addr >= phyMemTotalSize) {
		index.byteIndex = index.bitIndex = 0xffffffff;
		return index;
	}
	addr >>= 12;
	index.byteIndex = addr / 8;
	index.bitIndex = addr - index.byteIndex * 8;
	return index;
}

// Marks physical pages as used in the physical memory bitmap
void markPhysicalPagesAsUsed(void* address, size_t pageCount) {
	// TODO: performance can be improved by working in groups of 8 pages
	uint64_t addr = (uint64_t) address;
	if (addr >= phyMemTotalSize) {
		return;
	}
	for (size_t i = 0; i < pageCount; ++i) {
		PhyMemBitmapIndex index = getPhysicalPageBitmapIndex((void*)(addr + i * phyPageSize));
		phyMemBitmap[index.byteIndex] |= (1 << index.bitIndex);
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
void listUsedPhysicalPages() {
	terminalPrintString(rangeOfUsed, strlen(rangeOfUsed));
	uint64_t lastUsed = 0, length = 0;
	bool ongoingAvailable = false;
	for (uint64_t i = 0; i < phyMemTotalSize; i += phyPageSize) {
		if (isPhysicalPageAvailable((void*) i, 1)) {
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
			length += phyPageSize;
			ongoingAvailable = false;
		}
	}
	if (!ongoingAvailable) {
		lastUsed = phyMemTotalSize - length;
		terminalPrintHex(&lastUsed, sizeof(lastUsed));
		terminalPrintHex(&length, sizeof(length));
		terminalPrintChar('\n');
	}
}

// Returns true if all the physical pages starting at given address are free, false if even one page is allocated
bool isPhysicalPageAvailable(void* address, size_t pageCount) {
	uint64_t addr = (uint64_t) address;
	if (addr >= phyMemTotalSize) {
		return false;
	}
	for (size_t i = 0; i < pageCount; ++i) {
		PhyMemBitmapIndex index = getPhysicalPageBitmapIndex((void*)(addr + i * phyPageSize));
		if (phyMemBitmap[index.byteIndex] & (1 << index.bitIndex)) {
			return false;
		}
	}
	return true;
}

// Get base address of array of MMAP entries
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
}