#include <commonstrings.h>
#include <kernel.h>
#include <string.h>
#include <terminal.h>

RSDPDescriptor2 *rsdp = 0;

static const char* const parsingAcpi = "Parsing ACPI3...\n";
static const char* const searchingRsdp = "Searching for RSDP...";
static const char* const acpiFound = "RSDP = ";
static const char* const xsdtSignature = "XSDT";
static const char* const verifyingChecksum = "Checking RSD integrity...";
static const char* const verifyingXsdtSig = "Verifying XSDT signature...\nSignature ";
static const char* const rsdtAddress = "RSDT Address = ";
static const char* const xsdtAddress = "XSDT Address = ";
static const char* const oldAcpi = "ACPI version found is older than ACPI3\n";
static const char* const checkingRevision = "Checking ACPI revision...";

RSDPDescriptor2* searchRsdp() {
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

bool parseAcpi3() {
	terminalPrintString(parsingAcpi, strlen(parsingAcpi));

	// Search for RSDP in the identity mapped regions from
	// 0 to L32K64_SCRATCH_BASE and (L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH) to 1MiB
	terminalPrintSpaces4();
	terminalPrintString(searchingRsdp, strlen(searchingRsdp));
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
	terminalPrintString(checkingRevision, strlen(checkingRevision));
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
	terminalPrintString(verifyingChecksum, strlen(verifyingChecksum));
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
	hangSystem();

	// // TODO: Initalize ACPI3
	// // Read https://uefi.org/sites/default/files/resources/ACPI_6_3_final_Jan30.pdf Section 5.2
	// ACPISDTHeader *rsdt = rsdp->rsdtAddress;
	// terminalPrintString(rsdtAddress, strlen(rsdtAddress));
	// terminalPrintHex(&rsdt, sizeof(rsdt));
	// terminalPrintChar('\n');
	// ACPISDTHeader *xsdt = rsdp->xsdtAddress;
	// terminalPrintString(xsdtAddress, strlen(xsdtAddress));
	// terminalPrintHex(&xsdt, sizeof(xsdt));
	// terminalPrintChar('\n');
	// terminalPrintString(verifyingXsdtSig, strlen(verifyingXsdtSig));
	// if (strcmp(xsdt->signature, xsdtSignature)) {
	// 	terminalPrintString(not, strlen(not));
	// 	terminalPrintString(ok, strlen(ok));
	// 	return false;
	// }
	// terminalPrintString(ok, strlen(ok));

	return true;
}