#include <kernel.h>
#include <string.h>
#include <terminal.h>

RSDPDescriptor2 *rsdp = 0;

const char* const parsingAcpi = "\nParsing ACPI3...\n";
const char* const searchingRsdp = "Searching for RSDP...\n";
const char* const acpiParseFailed = "Could not find ACPI3 tables\n";
const char* const acpiFound = "RSDP = ";
const char* const xsdtSignature = "XSDT";
const char* const verifyingChecksum = "Verifying RSD checksum...\n";
const char* const checksumStr = "Checksum ";
const char* const ok = "ok\n";
const char* const not = "not ";
const char* const verifyingXsdtSig = "Verifying XSDT signature...\nSignature ";
const char* const rsdtAddress = "RSDT Address = ";
const char* const xsdtAddress = "XSDT Address = ";
const char* const oldAcpi = " ACPI version found is older than ACPI3\n";

RSDPDescriptor2* searchRsdp() {
	// Put the magic string 'RSD PTR ' in a 64-bit number and search
	// for it in 1st MiB at every 8 byte boundary
	uint64_t rsdPtr = 0x2052545020445352;
	terminalPrintChar('\n');
	for (uint64_t *p = 0; p < (uint64_t*)0x100000; ++p) {
		if (*p == rsdPtr) {
			return (RSDPDescriptor2*)p;
		}
	}
	return 0;
}

bool parseAcpi3() {
	terminalPrintString(parsingAcpi, strlen(parsingAcpi));
	terminalPrintString(searchingRsdp, strlen(searchingRsdp));

	// At this stage 16 MiB are identity mapped
	// so we need worry about page faults
	rsdp = searchRsdp();
	if (!rsdp) {
		terminalPrintString(acpiParseFailed, strlen(acpiParseFailed));
		return false;
	}
	terminalPrintString(acpiFound, strlen(acpiFound));
	terminalPrintHex(&rsdp, sizeof(&rsdp));
	terminalPrintChar('\n');

	if (rsdp->revision <= RSDP_REVISION_2_AND_ABOVE) {
		terminalPrintString(oldAcpi, strlen(oldAcpi));
		return false;
	}

	// Verify the checksum
	terminalPrintString(verifyingChecksum, strlen(verifyingChecksum));
	uint8_t checksum = 0;
	for (size_t i = 0; i < sizeof(RSDPDescriptor2); ++i) {
		checksum += ((uint8_t*)rsdp)[i];
	}
	terminalPrintString(checksumStr, strlen(checksumStr));
	if (checksum != 0) {
		terminalPrintString(not, strlen(not));
		terminalPrintString(ok, strlen(ok));
		return false;
	}
	terminalPrintString(ok, strlen(ok));

	// TODO: Initalize ACPI3
	// Read https://uefi.org/sites/default/files/resources/ACPI_6_3_final_Jan30.pdf Section 5.2
	ACPISDTHeader *rsdt = rsdp->rsdtAddress;
	terminalPrintString(rsdtAddress, strlen(rsdtAddress));
	terminalPrintHex(&rsdt, sizeof(rsdt));
	terminalPrintChar('\n');
	ACPISDTHeader *xsdt = rsdp->xsdtAddress;
	terminalPrintString(xsdtAddress, strlen(xsdtAddress));
	terminalPrintHex(&xsdt, sizeof(xsdt));
	terminalPrintChar('\n');
	terminalPrintString(verifyingXsdtSig, strlen(verifyingXsdtSig));
	if (strcmp(xsdt->signature, xsdtSignature)) {
		terminalPrintString(not, strlen(not));
		terminalPrintString(ok, strlen(ok));
		return false;
	}
	terminalPrintString(ok, strlen(ok));

	return true;
}