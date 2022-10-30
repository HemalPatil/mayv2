#pragma once

#include <cstddef>
#include <cstdint>

#define PML4_ENTRY_COUNT 512

struct PML4E {
	uint8_t present : 1;
	uint8_t readWrite : 1;
	uint8_t userAccess : 1;
	uint8_t pageWriteThrough : 1;
	uint8_t cacheDisable : 1;
	uint8_t accessed : 1;
	uint16_t ignore1 : 6;
	uint64_t physicalAddress : 40;
	uint16_t ignore2 : 11;
	uint8_t executeDisable : 1;
} __attribute__((packed));
typedef struct PML4E PML4E;
typedef struct PML4E PDPTE;
typedef struct PML4E PDE;
typedef struct PML4E PTE;
