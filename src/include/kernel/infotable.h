#pragma once

#include <cstdint>

// Do not convert address values to actual pointers
// due to pointer size differences between 32 and 64 bit modes

struct InfoTable {
	uint16_t bootDiskNumber;
	uint16_t mmapEntryCount;
	uint16_t mmapEntriesOffset;
	uint16_t mmapEntriesSegment;
	uint32_t vbeModesInfoLocation;
	char gdt32Descriptor[6];
	uint16_t maxPhysicalAddress;
	uint16_t maxLinearAddress;
	uint16_t vbeModesInfoCount;
	uint64_t kernelPhyMemBase;
	uint64_t pml4tPhysicalAddress;
	uint64_t vbeModeNumbersLocation;
} __attribute__((packed));
typedef struct InfoTable InfoTable;
