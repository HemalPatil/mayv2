#pragma once
#include <stdint.h>

struct InfoTable {
	uint16_t bookDiskNumber;
	uint16_t numberOfMmapEntries;
	uint16_t mmapEntriesOffset;
	uint16_t mmapEntriesSegment;
	uint16_t vesaInfoOffset;
	uint16_t vesaInfoSegment;
	char gdt32Descriptor[6];
	uint16_t maxPhysicalAddress;
	uint16_t maxLinearAddress;
	uint16_t ignore;
	uint64_t kernel64VirtualMemSize;
	void* kernel64Base;
} __attribute__((packed));
typedef struct InfoTable InfoTable;