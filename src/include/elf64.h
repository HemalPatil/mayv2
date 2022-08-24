#pragma once
#include <stdint.h>

// Program Header of 64-bit ELF binaries

#define ELF_Signature 0x464c457f

#define ELF_Type_x32 1
#define ELF_Type_x64 2

#define ELF_SegmentType_Load 1

#define ELF_LittleEndian 1
#define ELF_BigEndian 2

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
	uint64_t virtualMemoryAddress;
	uint64_t ignore;
	uint64_t segmentSizeInFile;
	uint64_t segmentSizeInMemory;
	uint64_t alignment;
} __attribute__((packed));
typedef struct ELF64ProgramHeader ELF64ProgramHeader;