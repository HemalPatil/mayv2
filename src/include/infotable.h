#pragma once

#include <stdint.h>

// Do not convert address values to actual pointers
// due to pointer size differences between 32 and 64 bit modes

struct InfoTable {
	uint16_t bootDiskNumber;
	uint16_t mmapEntryCount;
	uint16_t mmapEntriesOffset;
	uint16_t mmapEntriesSegment;
	uint16_t vesaInfoOffset;
	uint16_t vesaInfoSegment;
	char gdt32Descriptor[6];
	uint16_t maxPhysicalAddress;
	uint16_t maxLinearAddress;
	uint16_t ignore;
	uint64_t pml4eRoot;
	uint64_t kernel64PhyMemBase;
} __attribute__((packed));
typedef struct InfoTable InfoTable;