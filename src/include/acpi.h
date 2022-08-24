#pragma once
#include <stdint.h>

#define ACPI3_MemType_Usable 1
#define ACPI3_MemType_Reserved 2
#define ACPI3_MemType_ACPIReclaimable 3
#define ACPI3_MemType_ACPINVS 4
#define ACPI3_MemType_Bad 5
#define ACPI3_MemType_Hole 10

#define RSDP_Revision_1 0
#define RSDP_Revision_2_and_above 1

// ACPI 3.0 entry format (we have used extended entries of 24 bytes)
struct ACPI3Entry {
	uint64_t base;
	uint64_t length;
	uint32_t regionType;
	uint32_t extendedAttributes;
} __attribute__((packed));
typedef struct ACPI3Entry ACPI3Entry;

struct ACPISDTHeader {
	// Read https://uefi.org/sites/default/files/resources/ACPI_6_3_final_Jan30.pdf Section 5.2.8
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oemId[6];
	char oemTableID[8];
	uint32_t oemRevision;
	uint32_t creatorID;
	uint32_t creatorRevision;
} __attribute__((packed));
typedef struct ACPISDTHeader ACPISDTHeader;

struct RSDPDescriptor2 {
	char signature[8];
	uint8_t checksum;
	char oemId[6];
	uint8_t revision;
	ACPISDTHeader *rsdtAddress;
	uint32_t length;
	ACPISDTHeader *xsdtAddress;
	uint8_t extendedChecksum;
	uint8_t reserved[3];
} __attribute__((packed));
typedef struct RSDPDescriptor2 RSDPDescriptor2;
