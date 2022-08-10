#include "kernel.h"

static uint64_t FreePhysicalMemory = 0;
static uint64_t AllocatorBuffer[512] = {0};
static size_t AllocatorIndex = 0;
static char *BuddyStructure;
static size_t BuddyStructureSize = 0;
static size_t BuddyLevels = 0;
static size_t NumberOfPhysicalPages = 0;
static const size_t PhyPageSize = 0x1000;

const char* const getPhyMemSizeStr = "Getting physical memory size...\n";
const char* const phyMemSizeStr = "Physical memory size ";

// Initializes the physical memory for use by higher level virtual memory manager and other kernel services
bool InitializePhysicalMemory()
{
	// TODO : add initialize phy mem implementation
	TerminalPrintString(getPhyMemSizeStr, strlen(getPhyMemSizeStr));
	uint64_t PhyMemSize = GetPhysicalMemorySize();
	TerminalPrintString(phyMemSizeStr, strlen(phyMemSizeStr));
	TerminalPrintHex(&PhyMemSize, sizeof(PhyMemSize));
	return true;
	NumberOfPhysicalPages = PhyMemSize / PhyPageSize;
	if(PhyMemSize % PhyPageSize)
	{
		NumberOfPhysicalPages++;
	}

	// remove the size of kernel from this usable memory
	// also make first 1MiB of physical memory unusable, we have important structures there
	// although some areas of the first 1MiB are already decalred unusable
	// by GetUsablePhysicalMemorySize, we are ignoring this overlapping memory (size : around 240 KiB), it won't make much difference
	FreePhysicalMemory = GetUsablePhysicalMemorySize() - GetKernelSize() - 0x100000;

	// we have physical memory available right after the kernel,
	// this is where we will be setting up our buddy allocator structure
	// at this stage this region of memory has the kernel elf that was loaded
	uint64_t temp = 1;
	while(temp < NumberOfPhysicalPages)
	{
		temp *= 2;
		BuddyLevels++;
	}
	if(temp >= NumberOfPhysicalPages)
	{
		BuddyLevels++;
	}
	// TODO : we need to make sure the entire buddy structure is mapped in virtual memory
	BuddyStructure = (char*)(GetKernelBasePhysicalMemory() + GetKernelSize());
	// we have 2 bits for each page, hence we require twice the size, divide by 3 to get size in bytes
	BuddyStructureSize = (uint64_t)1 << (BuddyLevels + 1 - 3);
	memset((void*)BuddyStructure, 0, BuddyStructureSize);

	// Mark non exitstant areas of memory as in-use
	MarkPhysicalPagesAsUsed(PhyMemSize, temp - NumberOfPhysicalPages);

	// Mark areas from memory map that are not usable to in-use list
	struct ACPI3Entry *mmap = GetMMAPBase();
	const size_t n = GetNumberOfMMAPEntries();
	for(size_t i = 0; i < n; i++)
	{
		if(mmap[i].RegionType != ACPI3_MemType_Usable)
		{
			uint64_t templen = mmap[i].Length / PhyPageSize;
			if(mmap[i].Length % PhyPageSize)
			{
				templen++;
			}
			MarkPhysicalPagesAsUsed(mmap[i].BaseAddress, templen);
		}
	}

	return true;
}

// marks physical pages as used in the physical memory allocator
void MarkPhysicalPagesAsUsed(uint64_t address, size_t NumberOfPages)
{
	size_t CurrentLevel = 0;
}

// returns true if all the physical pages starting at given address are free, false if even one page is allocated
bool IsPhysicalPageAvailable(uint64_t address, size_t NumberOfPages)
{

}

// Get base address of array of MMAP entries
struct ACPI3Entry* GetMMAPBase()
{
	uint16_t MMAPSegment = *(InfoTable + 3);
	uint16_t MMAPOffset = *(InfoTable + 2);
	return (struct ACPI3Entry*)((uint64_t)(MMAPSegment<<4) + (uint64_t)MMAPOffset);
}

// Get number of entries in the MMAP array
size_t GetNumberOfMMAPEntries()
{
	return (size_t)(*(InfoTable + 1));
}

// Returns physical memory size in bytes
uint64_t GetPhysicalMemorySize() {
	struct ACPI3Entry* BaseMMAP = GetMMAPBase();
	size_t i, count = GetNumberOfMMAPEntries();
	uint64_t phyMemSize = 0;
	for (i = 0; i < count; ++i) {
		phyMemSize += BaseMMAP[i].Length;
	}
	return phyMemSize;
}

// Returns usable (conventional ACPI3_MemType_Usable) physical memory size
uint64_t GetUsablePhysicalMemorySize()
{
	struct ACPI3Entry* BaseMMAP = GetMMAPBase();
	size_t i, number = GetNumberOfMMAPEntries();
	uint64_t UsableMemSize = 0;
	for (i = 0; i < number; i++)
	{
		if (BaseMMAP[i].RegionType == ACPI3_MemType_Usable)
		{
			UsableMemSize += BaseMMAP[i].Length;
		}
	}
	return UsableMemSize;
}

// Returns the base address of kernel in physical memory in bytes
uint64_t GetKernelBasePhysicalMemory()
{
	return *((uint64_t*)(InfoTable + 16));
}