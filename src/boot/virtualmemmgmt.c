#include "kernel.h"

// Returns kernel size in bytes in virtual memory (sum of all section sizes in virtual memory)
uint64_t getKernelSize() {
	return infoTable->kernel64VirtualMemSize;
}

// Initializes virtual memory space for use by higher level dynamic memory manager and other kernel services
bool initializeVirtualMemory() {
	// TODO : add initialize vir mem implementation
	return true;
}