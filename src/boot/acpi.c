#include <commonstrings.h>
#include <kernel.h>
#include <heapmemmgmt.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static const uint32_t apicSignature = 0x43495041; // 'APIC'
static const uint32_t fadtSignature = 0x50434146; // 'FADT'
static const uint32_t ssdtSignature = 0x54445353; // 'SSDT'
static const uint32_t xsdtSignature = 0x54445358; // 'XSDT'

static RSDPDescriptor2* searchRsdp();

ACPISDTHeader *apic = NULL;
ACPISDTHeader *fadt = NULL;
RSDPDescriptor2 *rsdp = NULL;
ACPISDTHeader *ssdt = NULL;
ACPISDTHeader *xsdt = NULL;

static const char* const parsingAcpiStr = "Parsing ACPI3";
static const char* const searchingRsdpStr = "Searching for RSDP";
static const char* const acpiFound = "RSDP ";
static const char* const verifyingChecksumStr = "Checking RSDT integrity";
static const char* const verifyingXsdtSigStr = "Checking XSDT signature and integrity";
static const char* const rsdtAddressStr = "RSDT ";
static const char* const xsdtAddressStr = ", XSDT ";
static const char* const oldAcpi = "ACPI version found is older than ACPI3\n";
static const char* const checkingRevisionStr = "Checking ACPI revision";
static const char* const parseCompleteStr = "ACPI3 parsed\n\n";
static const char* const mappingXsdtStr = "Mapping XSDT to kernel address space";
static const char* const findingAcpiTablesStr = "Searching for APIC, FADT, and SSDT entries";

bool parseAcpi3() {
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
	if (rsdp->revision <= RSDP_REVISION_2_AND_ABOVE) {
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

	// TODO: Initalize ACPI3
	// Read https://uefi.org/sites/default/files/resources/ACPI_6_3_final_Jan30.pdf Section 5.2
	ACPISDTHeader *rsdtPhy = (ACPISDTHeader*)(uint64_t)rsdp->rsdtAddress;
	terminalPrintSpaces4();
	terminalPrintString(rsdtAddressStr, strlen(rsdtAddressStr));
	terminalPrintHex(&rsdtPhy, sizeof(rsdtPhy));
	ACPISDTHeader *xsdtPhy = (ACPISDTHeader*)rsdp->xsdtAddress;
	terminalPrintString(xsdtAddressStr, strlen(xsdtAddressStr));
	terminalPrintHex(&xsdtPhy, sizeof(xsdtPhy));
	terminalPrintChar('\n');

	// The XSDT is most likely not mapped in the virtual address space
	// Request a kernel page and map it to XSDT's page
	terminalPrintSpaces4();
	terminalPrintString(mappingXsdtStr, strlen(mappingXsdtStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	PageRequestResult requestResult = requestVirtualPages(1, MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 1 ||
		!mapVirtualPages(requestResult.address, (void*)((uint64_t)xsdtPhy & phyMemBuddyMasks[0]), 1)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');

	// Verify the XSDT signature
	ACPISDTHeader *oldXsdt = (ACPISDTHeader*)((uint64_t)requestResult.address | ((uint64_t)xsdtPhy & ~phyMemBuddyMasks[0]));
	terminalPrintSpaces4();
	terminalPrintString(verifyingXsdtSigStr, strlen(verifyingXsdtSigStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	// Assumes the entrie XSDT lies in the same page as XSDT
	if (*(uint32_t*)oldXsdt->signature != xsdtSignature || !validAcpi3Table(oldXsdt)) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		return false;
	}
	// Copy the XSDT to heap
	xsdt = kernelMalloc(oldXsdt->length);
	memcpy(xsdt, oldXsdt, oldXsdt->length);
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');

	// Find all the relevant tables
	terminalPrintSpaces4();
	terminalPrintString(findingAcpiTablesStr, strlen(findingAcpiTablesStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	ACPISDTHeader* oldApic = findAcpiTable(oldXsdt, apicSignature);
	ACPISDTHeader* oldFadt = findAcpiTable(oldXsdt, fadtSignature);
	ACPISDTHeader* oldSsdt = findAcpiTable(oldXsdt, ssdtSignature);
	if (oldApic == INVALID_ADDRESS || oldFadt == INVALID_ADDRESS || oldSsdt == INVALID_ADDRESS) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	// Copy the tables to heap
	apic = kernelMalloc(oldApic->length);
	memcpy(apic, oldApic, oldApic->length);
	fadt = kernelMalloc(oldFadt->length);
	memcpy(fadt, oldFadt, oldFadt->length);
	ssdt = kernelMalloc(oldSsdt->length);
	memcpy(ssdt, oldSsdt, oldSsdt->length);
	// Free the kernel page used for parsing XSDT
	freeVirtualPages((void*)((uint64_t)oldXsdt & phyMemBuddyMasks[0]), 1, MEMORY_REQUEST_KERNEL_PAGE);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintString(parseCompleteStr, strlen(parseCompleteStr));
	return true;
}

// Finds the ACPI table with a given signature and returns its address
// Returns INVALID_ADDRESS is not found
// Assumes the entire XSDT and the tables pointed to by it are mapped in the same page as XSDT
ACPISDTHeader* findAcpiTable(ACPISDTHeader *xsdt, uint32_t signature) {
	size_t tableCount = (xsdt->length - sizeof(ACPISDTHeader)) / sizeof(uint64_t);
	for (size_t i = 0; i < tableCount; ++i) {
		ACPISDTHeader *table = (ACPISDTHeader*)(
			((uint64_t)xsdt & phyMemBuddyMasks[0]) |
			(((uint64_t*)(xsdt + 1))[i] & ~phyMemBuddyMasks[0])
		);
		if (*(uint32_t*)table->signature == signature) {
			return table;
		}
	}
	return INVALID_ADDRESS;
}

// Adds up all the bytes in an ACPI table and returns true only if the checksum is 0
// Assumes the entire table is mapped in the address space
bool validAcpi3Table(ACPISDTHeader* header) {
	uint8_t checksum = 0;
	for (size_t i = 0; i < header->length; ++i) {
		checksum += ((uint8_t*)header)[i];
	}
	return checksum == 0;
}

static RSDPDescriptor2* searchRsdp() {
	// Search for the magic string 'RSD PTR ' at every 8 byte boundary
	uint64_t rsdPtr = 0x2052545020445352;
	for (uint64_t *p = 0; p < (uint64_t*)L32K64_SCRATCH_BASE; ++p) {
		if (*p == rsdPtr) {
			return (RSDPDescriptor2*)p;
		}
	}
	for (uint64_t *p = (uint64_t*)(L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH); p < (uint64_t*)0x100000; ++p) {
		if (*p == rsdPtr) {
			return (RSDPDescriptor2*)p;
		}
	}
	return NULL;
}
