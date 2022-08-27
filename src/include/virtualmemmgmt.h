#pragma once

#include <elf64.h>
#include <stddef.h>
#include <stdint.h>

struct VirtualMemNode {
	void *base;
	uint64_t pageCount;
	struct VirtualMemNode *next;
	struct VirtualMemNode *previous;
};
typedef struct VirtualMemNode VirtualMemNode;

extern void* getMappedPhysicalAddress(void* virtualAddress);
extern bool initializeVirtualMemory(void* kernelElfBase, uint64_t kernelElfSize, ELF64ProgramHeader* programHeader);