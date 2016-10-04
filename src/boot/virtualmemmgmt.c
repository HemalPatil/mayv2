#include "kernel.h"

// Returns kernel size in bytes in virtual memory (sum of all section sizes in virtual memory)
uint64_t GetKernelSize()
{
	return *((uint64_t*)(InfoTable + 12));
}

// Initializes virtual memory space for use by higher level dynamic memory manager and other kernel services
bool InitializeVirtualMemory()
{
	return true;
}