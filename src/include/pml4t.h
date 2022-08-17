#pragma once
#include <stdint.h>

// 64-bit long mode paging structures
// Right now in this very early stage of system initialization we just need present,
// r/w and address fields. Hence all the 4 structures are using a common struct.
struct PML4E {
	uint8_t present : 1;
	uint8_t readWrite : 1;
	uint8_t userAccess : 1;
	uint8_t pageWriteThrough : 1;
	uint8_t cacheDisable : 1;
	uint8_t accessed : 1;
	uint16_t ignore1 : 6;
	uint64_t pageAddress : 40;
	uint16_t ignore2 : 11;
	uint8_t executeDisable : 1;
} __attribute__((packed));
typedef struct PML4E PML4E;
typedef struct PML4E PDPTE;
typedef struct PML4E PDE;
typedef struct PML4E PTE;