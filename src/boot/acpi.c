#include "kernel.h"
#include <string.h>

struct RSDPDescriptor2 *rsdp = 0;

const char* const parsingACPI = "\nParsing ACPI3...\n";
const char* const searchingRSDP = "Searching for RSDP...\n";
const char* const acpiParseFailed = "Could not find ACPI3 tables\n";
const char* const acpiFound = "RSDP = ";
const char* const XSDTSignature = "XSDT";
const char* const verifyingChecksum = "Verifying RSD checksum...\n";
const char* const checksumStr = "Checksum ";
const char* const ok = "ok\n";
const char* const not = "not ";
const char* const verifyingXSDTsig = "Verifying XSDT signature...\nSignature ";
const char* const rsdtAddress = "RSDT Address = ";
const char* const xsdtAddress = "XSDT Address = ";

bool ParseACPI3() {
	terminalPrintString(parsingACPI, strlen(parsingACPI));
	terminalPrintString(searchingRSDP, strlen(searchingRSDP));
	rsdp = SearchRSDP();
	if (!rsdp) {
		terminalPrintString(acpiParseFailed, strlen(acpiParseFailed));
		return false;
	}
	terminalPrintString(acpiFound, strlen(acpiFound));
	terminalPrintHex(&rsdp, sizeof(&rsdp));
	terminalPrintChar('\n');

	if (rsdp->revision <= RSDP_Revision_2_and_above) {
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
	struct ACPISDTHeader *rsdt = rsdp->RSDTAddress;
	terminalPrintString(rsdtAddress, strlen(rsdtAddress));
	terminalPrintHex(&rsdt, sizeof(rsdt));
	terminalPrintChar('\n');
	struct ACPISDTHeader *xsdt = rsdp->XSDTAddress;
	terminalPrintString(xsdtAddress, strlen(xsdtAddress));
	terminalPrintHex(&xsdt, sizeof(xsdt));
	terminalPrintChar('\n');
	terminalPrintString(verifyingXSDTsig, strlen(verifyingXSDTsig));
	if (strcmp(xsdt->Signature, XSDTSignature)) {
		terminalPrintString(not, strlen(not));
		terminalPrintString(ok, strlen(ok));
		return false;
	}
	terminalPrintString(ok, strlen(ok));

	return true;
}