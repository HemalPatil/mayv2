#include <string.h>
#include "kernel.h"

static uint64_t freePhysicalMemory = 0;
static uint64_t allocatorBuffer[512] = {0};
static size_t allocatorIndex = 0;
static char *buddyStructure;
static size_t buddyStructureSize = 0;
static size_t buddyLevels = 0;
static size_t numberOfPhysicalPages = 0;
static const size_t phyPageSize = 0x1000;

uint8_t* phyMemBitmap = 0;
uint64_t phyMemBitmapSize = 0;

const char* const getPhyMemSizeStr = "Getting physical memory size...\n";
const char* const phyMemSizeStr = "Physical memory size ";
const char* const usableMemSizeStr = "Usable physical memory size ";
const char* const mmapBaseStr = "MMAP Base ";
const char* const entry = "    ";
const char* const countStr = "Number of MMAP entries ";
const char* const tableHeader = "    Base address         Length               Type\n";
const char* const phyMemBitmapLocn = "Physical memory bitmap location ";

// Initializes the physical memory for use by higher level virtual memory manager and other kernel services
bool initializePhysicalMemory() {
	// TODO : add initialize phy mem implementation
	terminalPrintString(getPhyMemSizeStr, strlen(getPhyMemSizeStr));

	// Display diagnostic information about memory like MMAP entries,
	// physical and usable memory size
	ACPI3Entry* mmapBase = getMmapBase();
	terminalPrintString(mmapBaseStr, strlen(mmapBaseStr));
	terminalPrintHex(&mmapBase, sizeof(mmapBase));
	terminalPrintChar('\n');

	size_t i = 0;
	terminalPrintString(countStr, strlen(countStr));
	terminalPrintHex(&infoTable->numberOfMmapEntries, sizeof(infoTable->numberOfMmapEntries));
	terminalPrintChar('\n');
	terminalPrintString(tableHeader, strlen(tableHeader));
	for (i = 0; i < infoTable->numberOfMmapEntries; ++i) {
		terminalPrintString(entry, strlen(entry));
		terminalPrintHex(&(mmapBase[i].baseAddress), sizeof(mmapBase->baseAddress));
		terminalPrintChar(' ');
		terminalPrintHex(&(mmapBase[i].length), sizeof(mmapBase->length));
		terminalPrintChar(' ');
		uint8_t usable = mmapBase[i].regionType == ACPI3_MemType_Usable;
		terminalPrintHex(&(mmapBase[i].regionType), 1);
		terminalPrintChar('\n');
	}

	uint64_t phyMemSize = getPhysicalMemorySize();
	terminalPrintString(phyMemSizeStr, strlen(phyMemSizeStr));
	terminalPrintHex(&phyMemSize, sizeof(phyMemSize));
	terminalPrintChar('\n');

	uint64_t usableMemSize = getUsablePhysicalMemorySize();
	terminalPrintString(usableMemSizeStr, strlen(usableMemSizeStr));
	terminalPrintHex(&usableMemSize, sizeof(usableMemSize));
	terminalPrintChar('\n');

	numberOfPhysicalPages = phyMemSize / phyPageSize;
	if (phyMemSize % phyPageSize) {
		++numberOfPhysicalPages;
	}

	// Right now the memory has important structures in 1st MiB; will be marked used and system
	// 1MiB + 64KiB was occupied by loader32
	// Unknown number of pages are being occupied by PML4T starting from 1MiB + 64KiB
	// Kernel process is at 2MiB + kernelVirtualMemSize; must be marked used and system
	// Kernel ELF located from 2MiB + 4KiB buffer + kernelVirtualMemSize which can be overwritten
	// Create bitmap at 4KiB boundary after kernel process
	// Virtual memory manager will take of marking PML4 entries as used during its initialization
	// NOTE: bitmap is at 4KiB boundary not exactly 4KiB after
	phyMemBitmap = (uint8_t*)(((uint64_t)infoTable->kernel64Base + 0x1000 + infoTable->kernel64VirtualMemSize) & 0xfffffffffffff000);
	terminalPrintString(phyMemBitmapLocn, strlen(phyMemBitmapLocn));
	terminalPrintHex(&phyMemBitmap, sizeof(phyMemBitmap));
	return true;
	phyMemBitmapSize = numberOfPhysicalPages / 8;
	memset(phyMemBitmap, 0, phyMemBitmapSize);
	memset(phyMemBitmap, 0xff, (0x100000 + 0x10000) / phyPageSize / 8); // Mark 1MiB + 64KiB as used

	// Mark the phyMemBitmap itself as used
	// PhyMemBitmapIndex x = getPhysicalPageBitmapIndex(4097);
	// terminalPrintHex(&x.byteIndex, sizeof(x.byteIndex));
	// terminalPrintHex(&x.bitIndex, sizeof(x.bitIndex));
	// terminalPrintChar('\n');
	// x = getPhysicalPageBitmapIndex(8 * 4096 + 1);
	// terminalPrintHex(&x.byteIndex, sizeof(x.byteIndex));
	// terminalPrintHex(&x.bitIndex, sizeof(x.bitIndex));
	// terminalPrintChar('\n');


	// // remove the size of kernel from this usable memory
	// // also make first 1MiB of physical memory unusable, we have important structures there
	// // although some areas of the first 1MiB are already decalred unusable
	// // by getUsablePhysicalMemorySize, we are ignoring this overlapping memory (size : around 240 KiB), it won't make much difference
	// freePhysicalMemory = usableMemSize - infoTable->kernel64VirtualMemSize - 0x100000;

	// // we have physical memory available right after the kernel,
	// // this is where we will be setting up our buddy allocator structure
	// // at this stage this region of memory has the kernel elf that was loaded
	// uint64_t temp = 1;
	// while (temp < numberOfPhysicalPages) {
	// 	temp *= 2;
	// 	++buddyLevels;
	// }
	// if (temp >= numberOfPhysicalPages) {
	// 	++buddyLevels;
	// }
	// // TODO : we need to make sure the entire buddy structure is mapped in virtual memory
	// buddyStructure = (char*)(getKernelBasePhysicalMemory() + infoTable->kernel64VirtualMemSize);
	// // we have 2 bits for each page, hence we require twice the size, divide by 3 to get size in bytes
	// buddyStructureSize = (uint64_t)1 << (buddyLevels + 1 - 3);
	// memset((void*)buddyStructure, 0, buddyStructureSize);

	// // Mark non exitstant areas of memory as in-use
	// markPhysicalPagesAsUsed(phyMemSize, temp - numberOfPhysicalPages);

	// // Mark areas from memory map that are not usable to in-use list
	// ACPI3Entry *mmap = getMmapBase();
	// const size_t n = getNumberOfMmapEntries();
	// for (size_t i = 0; i < n; ++i) {
	// 	if (mmap[i].regionType != ACPI3_MemType_Usable) {
	// 		uint64_t tempLen = mmap[i].length / phyPageSize;
	// 		if(mmap[i].length % phyPageSize)
	// 		{
	// 			++tempLen;
	// 		}
	// 		MarkPhysicalPagesAsUsed(mmap[i].baseAddress, tempLen);
	// 	}
	// }

	return true;
}


PhyMemBitmapIndex getPhysicalPageBitmapIndex(void* address) {
	PhyMemBitmapIndex index;
	index.byteIndex = ((uint64_t)address >> 12) / 8;
	index.bitIndex = ((uint64_t)address >> 12) - index.byteIndex * 8;
	return index;
}

// Marks physical pages as used in the physical memory bitmap
void markPhysicalPagesAsUsed(void* address, size_t numberOfPages) {
	size_t currentLevel = 0;
}

// returns true if all the physical pages starting at given address are free, false if even one page is allocated
bool isPhysicalPageAvailable(void* address, size_t numberOfPages) {
	return true;
}

// Get base address of array of MMAP entries
ACPI3Entry* getMmapBase() {
	return (ACPI3Entry*)((uint64_t)(infoTable->mmapEntriesSegment << 4) + infoTable->mmapEntriesOffset);
}

// Returns physical memory size in bytes
uint64_t getPhysicalMemorySize() {
	ACPI3Entry* mmapBase = getMmapBase();
	uint64_t phyMemSize = 0;
	for (size_t i = 0; i < infoTable->numberOfMmapEntries; ++i) {
		phyMemSize += mmapBase[i].length;
	}
	return phyMemSize;
}

// Returns usable (conventional ACPI3_MemType_Usable) physical memory size
uint64_t getUsablePhysicalMemorySize() {
	ACPI3Entry* mmapBase = getMmapBase();
	uint64_t usableMemSize = 0;
	for (size_t i = 0; i < infoTable->numberOfMmapEntries; ++i) {
		if (mmapBase[i].regionType == ACPI3_MemType_Usable) {
			usableMemSize += mmapBase[i].length;
		}
	}
	return usableMemSize;
}