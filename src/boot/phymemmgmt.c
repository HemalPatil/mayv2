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

const char* const getPhyMemSizeStr = "Getting physical memory size...\n";
const char* const phyMemSizeStr = "Physical memory size ";
const char* const usableMemSizeStr = "Usable physical memory size ";
const char* const mmapBaseStr = "MMAP Base ";
const char* const entry = "    ";
const char* const countStr = "Number of MMAP entries ";
const char* const tableHeader = "    Base address         Length               Type\n";

// Initializes the physical memory for use by higher level virtual memory manager and other kernel services
bool initializePhysicalMemory() {
	// TODO : add initialize phy mem implementation
	terminalPrintString(getPhyMemSizeStr, strlen(getPhyMemSizeStr));

	// Display diagnostic information about memory like MMAP entries,
	// physical and usable memory size
	struct ACPI3Entry* mmapBase = getMmapBase();
	terminalPrintString(mmapBaseStr, strlen(mmapBaseStr));
	terminalPrintHex(&mmapBase, sizeof(mmapBase));
	terminalPrintChar('\n');

	size_t i, count = getNumberOfMMAPEntries();
	terminalPrintString(countStr, strlen(countStr));
	terminalPrintHex(&count, sizeof(count));
	terminalPrintChar('\n');
	terminalPrintString(tableHeader, strlen(tableHeader));
	for (i = 0; i < count; ++i) {
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
	return true;

	numberOfPhysicalPages = phyMemSize / phyPageSize;
	if (phyMemSize % phyPageSize) {
		numberOfPhysicalPages++;
	}

	// remove the size of kernel from this usable memory
	// also make first 1MiB of physical memory unusable, we have important structures there
	// although some areas of the first 1MiB are already decalred unusable
	// by GetUsablePhysicalMemorySize, we are ignoring this overlapping memory (size : around 240 KiB), it won't make much difference
	freePhysicalMemory = usableMemSize - getKernelSize() - 0x100000;

	// we have physical memory available right after the kernel,
	// this is where we will be setting up our buddy allocator structure
	// at this stage this region of memory has the kernel elf that was loaded
	uint64_t temp = 1;
	while (temp < numberOfPhysicalPages) {
		temp *= 2;
		++buddyLevels;
	}
	if (temp >= numberOfPhysicalPages) {
		++buddyLevels;
	}
	// TODO : we need to make sure the entire buddy structure is mapped in virtual memory
	buddyStructure = (char*)(getKernelBasePhysicalMemory() + getKernelSize());
	// we have 2 bits for each page, hence we require twice the size, divide by 3 to get size in bytes
	buddyStructureSize = (uint64_t)1 << (buddyLevels + 1 - 3);
	memset((void*)buddyStructure, 0, buddyStructureSize);

	// Mark non exitstant areas of memory as in-use
	markPhysicalPagesAsUsed(phyMemSize, temp - numberOfPhysicalPages);

	// Mark areas from memory map that are not usable to in-use list
	struct ACPI3Entry *mmap = getMmapBase();
	const size_t n = getNumberOfMMAPEntries();
	for (size_t i = 0; i < n; ++i) {
		if (mmap[i].regionType != ACPI3_MemType_Usable) {
			uint64_t tempLen = mmap[i].length / phyPageSize;
			if(mmap[i].length % phyPageSize)
			{
				++tempLen;
			}
			MarkPhysicalPagesAsUsed(mmap[i].baseAddress, tempLen);
		}
	}

	return true;
}

// marks physical pages as used in the physical memory allocator
void markPhysicalPagesAsUsed(uint64_t address, size_t numberOfPages) {
	size_t currentLevel = 0;
}

// returns true if all the physical pages starting at given address are free, false if even one page is allocated
bool isPhysicalPageAvailable(uint64_t address, size_t numberOfPages) {
	return true;
}

// Get base address of array of MMAP entries
struct ACPI3Entry* getMmapBase() {
	uint16_t mmapSegment = *(infoTable + 3);
	uint16_t mmapOffset = *(infoTable + 2);
	return (struct ACPI3Entry*)((uint64_t)(mmapSegment<<4) + (uint64_t)mmapOffset);
}

// Get number of entries in the MMAP array
size_t getNumberOfMMAPEntries() {
	return (size_t)(*(infoTable + 1));
}

// Returns physical memory size in bytes
uint64_t getPhysicalMemorySize() {
	struct ACPI3Entry* mmapBase = getMmapBase();
	size_t i, count = getNumberOfMMAPEntries();
	uint64_t phyMemSize = 0;
	for (i = 0; i < count; ++i) {
		phyMemSize += mmapBase[i].length;
	}
	return phyMemSize;
}

// Returns usable (conventional ACPI3_MemType_Usable) physical memory size
uint64_t getUsablePhysicalMemorySize() {
	struct ACPI3Entry* mmapBase = getMmapBase();
	size_t i, number = getNumberOfMMAPEntries();
	uint64_t usableMemSize = 0;
	for (i = 0; i < number; ++i) {
		if (mmapBase[i].regionType == ACPI3_MemType_Usable) {
			usableMemSize += mmapBase[i].length;
		}
	}
	return usableMemSize;
}

// Returns the base address of kernel in physical memory in bytes
uint64_t getKernelBasePhysicalMemory() {
	return *((uint64_t*)(infoTable + 16));
}