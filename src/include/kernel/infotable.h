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
	uint32_t vbeModeNumbersLocation;
	uint32_t reserved0;
	uint64_t globalCtorsLocation;
	uint64_t globalCtorsCount;
	char rootFsGuid[36];
} __attribute__((packed));

struct ApuInfoTable {
	uint64_t apuLongModeStart;
	uint64_t pml4tPhysicalAddress;
	uint64_t stackPointer;
	char gdt64Descriptor[10];
} __attribute__((packed));
