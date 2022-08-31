#pragma once

#include <stdint.h>

// Program Header of 64-bit ELF binaries

#define ELF_SIGNATURE 0x464c457f

#define ELF_TYPE_32 1
#define ELF_TYPE_64 2

#define ELF_SEGMENT_TYPE_LOAD 1

#define ELF_LITTLE_ENDIAN 1
#define ELF_BIG_ENDIAN 2

struct ELF64Header {
	char signature[4];
	uint8_t isX64;
	uint8_t endianness;
	uint8_t headerVersion;
	uint8_t abiType;
	uint64_t ignore;
	uint16_t type;
	uint16_t instructionSet;
	uint32_t version;
	uint64_t entryPoint;
	uint64_t headerTablePosition;
	uint64_t sectionTablePosition;
	uint32_t flags;
	uint16_t headerSize;
	uint16_t headerEntrySize;
	uint16_t headerEntryCount;
	uint16_t sectionEntrySize;
	uint16_t sectionEntryCount;
	uint16_t sectionNamesIndex;
} __attribute__((packed));
typedef struct ELF64Header ELF64Header;

struct ELF64ProgramHeader {
	uint32_t segmentType;
	uint32_t segmentFlags;
	uint64_t fileOffset;
	uint64_t virtualAddress;
	uint64_t ignore;
	uint64_t segmentSizeInFile;
	uint64_t segmentSizeInMemory;
	uint64_t alignment;
} __attribute__((packed));
typedef struct ELF64ProgramHeader ELF64ProgramHeader;