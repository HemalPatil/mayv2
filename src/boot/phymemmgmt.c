#include "kernel.h"

static uint64_t FreePhysicalMemory = 0;

// Initializes the physical memory for use by higher level virtual memory manager and other kernel services
bool InitializePhysicalMemory()
{
	// TODO : add initialize phy mem implementation
	uint64_t size = GetPhysicalMemorySize();

	// remove the size of kernel from this usable memory
	// also make first 1MiB of physical memory unusable, we have important structures there
	// although some areas of the first 1MiB are already decalred unusable
	// by GetUsablePhysicalMemorySize, we are ignoring this overlapping memory (size : around 240 KiB), it won't make much difference
	FreePhysicalMemory = GetUsablePhysicalMemorySize() - GetKernelSize() - 0x100000;

	

	return true;
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
uint64_t GetPhysicalMemorySize()
{
	struct ACPI3Entry* BaseMMAP = GetMMAPBase();
	size_t i, number = GetNumberOfMMAPEntries();
	uint64_t PhyMemSize = 0;
	for (i = 0; i < number; i++)
	{
		PhyMemSize += BaseMMAP[i].Length;
	}
	return PhyMemSize;
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