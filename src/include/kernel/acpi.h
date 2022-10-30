#pragma once

#include <cstdint>

#define ACPI_APIC_SIGNATURE 0x43495041
#define ACPI_FADT_SIGNATURE 0x50434146
#define ACPI_HPET_SIGNATURE 0x54455048
#define ACPI_MCFG_SIGNATURE 0x4746434d
#define ACPI_SSDT_SIGNATURE 0x54445353
#define ACPI_XSDT_SIGNATURE 0x54445358

#define ACPI3_MEM_TYPE_USABLE 1
#define ACPI3_MEM_TYPE_RESERVED 2
#define ACPI3_MEM_TYPE_RECLAIMABLE 3
#define ACPI3_MEM_TYPE_ACPINVS 4
#define ACPI3_MEM_TYPE_BAD 5
#define ACPI3_MEM_TYPE_HOLE 10

#define RSDP_REVISION_1 0
#define RSDP_REVISION_2_AND_ABOVE 1

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
	uint32_t creatorId;
	uint32_t creatorRevision;
} __attribute__((packed));
typedef struct ACPISDTHeader ACPISDTHeader;

struct RSDPDescriptor2 {
	char signature[8];
	uint8_t checksum;
	char oemId[6];
	uint8_t revision;
	uint32_t rsdtAddress;
	uint32_t length;
	uint64_t xsdtAddress;
	uint8_t extendedChecksum;
	uint8_t reserved[3];
} __attribute__((packed));
typedef struct RSDPDescriptor2 RSDPDescriptor2;

extern ACPISDTHeader *apicSdtHeader;
extern ACPISDTHeader *hpetSdtHeader;
extern ACPISDTHeader *mcfgSdtHeader;
extern RSDPDescriptor2 *rsdp;
extern ACPISDTHeader *ssdtHeader;
extern ACPISDTHeader *xsdt;

extern ACPISDTHeader* findAcpiTable(ACPISDTHeader *xsdt, uint32_t signature);
extern bool parseAcpi3();
extern bool validAcpi3Table(ACPISDTHeader* header);
