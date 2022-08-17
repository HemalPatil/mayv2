#pragma once
#include <stdint.h>

// Program Header of 64-bit ELF binaries
struct ELF64ProgramHeader {
	uint32_t typeOfSegment;
	uint32_t segmentFlags;
	uint64_t fileOffset;
	uint64_t virtualMemoryAddress;
	uint64_t ignore;
	uint64_t segmentSizeInFile;
	uint64_t segmentSizeInMemory;
	uint64_t alignment;
} __attribute__((packed));
typedef struct ELF64ProgramHeader ELF64ProgramHeader;