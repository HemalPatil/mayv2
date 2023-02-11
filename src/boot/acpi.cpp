#include <commonstrings.h>
#include <cstring>
#include <kernel.h>
#include <terminal.h>

static ACPI::RSDPDescriptor2* searchRsdp();

ACPI::SDTHeader *ACPI::apicSdtHeader = nullptr;
ACPI::SDTHeader *ACPI::hpetSdtHeader = nullptr;
ACPI::SDTHeader *ACPI::mcfgSdtHeader = nullptr;
ACPI::RSDPDescriptor2 *ACPI::rsdp = nullptr;
ACPI::SDTHeader *ACPI::ssdtHeader = nullptr;
ACPI::SDTHeader *ACPI::xsdt = nullptr;

static const char* const parsingAcpiStr = "Parsing ACPI3";
static const char* const searchingRsdpStr = "Searching for RSDP";
static const char* const acpiFound = "RSDP ";
static const char* const verifyingChecksumStr = "Checking RSDP integrity";
static const char* const verifyingXsdtSigStr = "Checking XSDT signature and integrity";
static const char* const rsdtAddressStr = "RSDT ";
static const char* const xsdtAddressStr = ", XSDT ";
static const char* const oldAcpi = "ACPI version found is older than ACPI3\n";
static const char* const checkingRevisionStr = "Checking ACPI revision";
static const char* const parseCompleteStr = "ACPI3 parsed\n\n";
static const char* const kAddrSpaceStr = " kernel address space";
static const char* const mappingXsdtStr = "Mapping XSDT to";
static const char* const findingAcpiTablesStr = "Searching for APIC, HPET, MCFG, and SSDT entries";
static const char* const unmappingXsdtStr = "Unmapping XSDT from";

bool ACPI::parse() {
	using namespace Kernel::Memory;

	terminalPrintString(parsingAcpiStr, strlen(parsingAcpiStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Search for RSDP in the identity mapped regions from
	// 0 to L32K64_SCRATCH_BASE and (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) to 1MiB
	terminalPrintSpaces4();
	terminalPrintString(searchingRsdpStr, strlen(searchingRsdpStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	rsdp = searchRsdp();
	if (!rsdp) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(acpiFound, strlen(acpiFound));
	terminalPrintHex(&rsdp, sizeof(&rsdp));
	terminalPrintChar('\n');

	// Ensure the revision is ACPI3
	terminalPrintSpaces4();
	terminalPrintString(checkingRevisionStr, strlen(checkingRevisionStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (rsdp->revision <= ACPI::RSDPRevision::v2AndAbove) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		terminalPrintSpaces4();
		terminalPrintString(oldAcpi, strlen(oldAcpi));
		return false;
	}
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');

	// Verify the checksum
	terminalPrintSpaces4();
	terminalPrintString(verifyingChecksumStr, strlen(verifyingChecksumStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	uint8_t checksum = 0;
	for (size_t i = 0; i < sizeof(RSDPDescriptor2); ++i) {
		checksum += ((uint8_t*)rsdp)[i];
	}
	if (checksum != 0) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');

	// Read https://uefi.org/sites/default/files/resources/ACPI_6_3_final_Jan30.pdf Section 5.2
	SDTHeader *rsdtPhy = (SDTHeader*)(uint64_t)rsdp->rsdtAddress;
	terminalPrintSpaces4();
	terminalPrintString(rsdtAddressStr, strlen(rsdtAddressStr));
	terminalPrintHex(&rsdtPhy, sizeof(rsdtPhy));
	SDTHeader *xsdtPhy = (SDTHeader*)rsdp->xsdtAddress;
	terminalPrintString(xsdtAddressStr, strlen(xsdtAddressStr));
	terminalPrintHex(&xsdtPhy, sizeof(xsdtPhy));
	terminalPrintChar('\n');

	// The XSDT is most likely not mapped in the virtual address space
	// Request a kernel page and map it to XSDT's page
	terminalPrintSpaces4();
	terminalPrintString(mappingXsdtStr, strlen(mappingXsdtStr));
	terminalPrintString(kAddrSpaceStr, strlen(kAddrSpaceStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	PageRequestResult requestResult = Virtual::requestPages(
		1,
		(
			RequestType::Kernel |
			RequestType::PhysicalContiguous |
			RequestType::VirtualContiguous
		)
	);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 1 ||
		!Virtual::mapPages(
			requestResult.address,
			(void*)((uint64_t)xsdtPhy & Physical::buddyMasks[0]),
			1,
			0
		)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Verify the XSDT signature
	SDTHeader *oldXsdt = (SDTHeader*)((uint64_t)requestResult.address | ((uint64_t)xsdtPhy & ~Physical::buddyMasks[0]));
	terminalPrintSpaces4();
	terminalPrintString(verifyingXsdtSigStr, strlen(verifyingXsdtSigStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	// Assumes the entrie XSDT lies in the same page as XSDT
	if (*(uint32_t*)oldXsdt->signature != ACPI::Signature::XSDT || !validTable(oldXsdt)) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		return false;
	}
	// Copy the XSDT to heap
	xsdt = (SDTHeader*)Heap::allocate(oldXsdt->length);
	memcpy(xsdt, oldXsdt, oldXsdt->length);
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');

	// Find all the relevant tables
	terminalPrintSpaces4();
	terminalPrintString(findingAcpiTablesStr, strlen(findingAcpiTablesStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	SDTHeader* oldApic = findTable(oldXsdt, ACPI::Signature::APIC);
	SDTHeader* oldHpet = findTable(oldXsdt, ACPI::Signature::HPET);
	SDTHeader* oldMcfg = findTable(oldXsdt, ACPI::Signature::MCFG);
	SDTHeader* oldSsdt = findTable(oldXsdt, ACPI::Signature::SSDT);
	if (
		oldApic == INVALID_ADDRESS ||
		oldMcfg == INVALID_ADDRESS ||
		oldSsdt == INVALID_ADDRESS
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	// Copy the tables to heap
	apicSdtHeader = (SDTHeader*)Heap::allocate(oldApic->length);
	memcpy(apicSdtHeader, oldApic, oldApic->length);
	mcfgSdtHeader = (SDTHeader*)Heap::allocate(oldMcfg->length);
	memcpy(mcfgSdtHeader, oldMcfg, oldMcfg->length);
	ssdtHeader = (SDTHeader*)Heap::allocate(oldSsdt->length);
	memcpy(ssdtHeader, oldSsdt, oldSsdt->length);
	if (oldHpet != INVALID_ADDRESS) {
		hpetSdtHeader = (SDTHeader*)Heap::allocate(oldHpet->length);
		memcpy(hpetSdtHeader, oldHpet, oldHpet->length);
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Free the kernel page used for parsing XSDT
	terminalPrintSpaces4();
	terminalPrintString(unmappingXsdtStr, strlen(unmappingXsdtStr));
	terminalPrintString(kAddrSpaceStr, strlen(kAddrSpaceStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!Virtual::freePages(
		(void*)((uint64_t)oldXsdt & Physical::buddyMasks[0]),
		1,
		RequestType::Kernel
	)) {
		terminalPrintString(failedStr, strlen(failedStr));
		Kernel::panic();
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintString(parseCompleteStr, strlen(parseCompleteStr));
	return true;
}

// Finds the ACPI table with a given signature and returns its address
// Returns INVALID_ADDRESS is not found
// Assumes the entire XSDT and the tables pointed to by it are mapped in the same page as XSDT
ACPI::SDTHeader* ACPI::findTable(SDTHeader *xsdt, uint32_t signature) {
	using namespace Kernel::Memory;

	size_t tableCount = (xsdt->length - sizeof(SDTHeader)) / sizeof(uint64_t);
	for (size_t i = 0; i < tableCount; ++i) {
		SDTHeader *table = (SDTHeader*)(
			((uint64_t)xsdt & Physical::buddyMasks[0]) |
			(((uint64_t*)(xsdt + 1))[i] & ~Physical::buddyMasks[0])
		);
		if (*(uint32_t*)table->signature == signature) {
			return table;
		}
	}
	return (SDTHeader*)INVALID_ADDRESS;
}

// Adds up all the bytes in an ACPI table and returns true only if the checksum is 0
// Assumes the entire table is mapped in the address space
bool ACPI::validTable(SDTHeader* header) {
	uint8_t checksum = 0;
	for (size_t i = 0; i < header->length; ++i) {
		checksum += ((uint8_t*)header)[i];
	}
	return checksum == 0;
}

static ACPI::RSDPDescriptor2* searchRsdp() {
	// Search for the magic string 'RSD PTR ' at every 8 byte boundary
	// Start searching from 0x8 because GCC hates to access 0x0
	uint64_t rsdPtr = 0x2052545020445352UL;
	for (uint64_t *p = (uint64_t*)8; p < (uint64_t*)L32K64_SCRATCH_BASE; ++p) {
		if (*p == rsdPtr) {
			return (ACPI::RSDPDescriptor2*)p;
		}
	}
	for (uint64_t *p = (uint64_t*)(L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH); p < (uint64_t*)0x100000; ++p) {
		if (*p == rsdPtr) {
			return (ACPI::RSDPDescriptor2*)p;
		}
	}
	return nullptr;
}
