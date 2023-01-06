#pragma once

#include <cstdint>

namespace ACPI {
	enum Signature : uint32_t {
		APIC = 0x43495041,
		FADT = 0x50434146,
		HPET = 0x54455048,
		MCFG = 0x4746434d,
		SSDT = 0x54445353,
		XSDT = 0x54445358
	};

	enum MemoryType : uint8_t {
		Usable = 1,
		Reserved = 2,
		Reclaimable = 3,
		ACPINVS = 4,
		Bad = 5,
		Hole = 10
	};

	enum RSDPRevision {
		v1 = 0,
		v2AndAbove = 1
	};

	struct Entryv3 {
		uint64_t base;
		uint64_t length;
		uint32_t regionType;
		uint32_t extendedAttributes;
	} __attribute__((packed));

	struct SDTHeader {
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

	extern SDTHeader *apicSdtHeader;
	extern SDTHeader *hpetSdtHeader;
	extern SDTHeader *mcfgSdtHeader;
	extern RSDPDescriptor2 *rsdp;
	extern SDTHeader *ssdtHeader;
	extern SDTHeader *xsdt;

	extern SDTHeader* findTable(SDTHeader *xsdt, uint32_t signature);
	extern bool parse();
	extern bool validTable(SDTHeader* header);
};
