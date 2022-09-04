#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>

static size_t cpuCount = 0;
static size_t ioApicCount = 0;

static const char* const initApicStr = "Initializing APIC";
static const char* const apicInitCompleteStr = "APIC initialized\n\n";
static const char* const checkingApicStr = "Checking APIC presence";
static const char* const disablingPicStr = "Disabling legacy PIC";
static const char* const parsingApicStr = "Parsing APIC table";
static const char* const apicParsedStr = "APIC table parsed";
static const char* const localApicAddrStr = "Local APIC address ";
static const char* const flagsStr = " Flags ";
static const char* const lengthStr = " Length ";
static const char* const entriesStr = "APIC entries\n";
static const char* const entryHeaderStr = "Type Length Flags        CpuID IOapicID\n";

bool initializeApic() {
	terminalPrintString(initApicStr, strlen(initApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// TODO: complete APIC initialization
	// Check APIC presence
	terminalPrintSpaces4();
	terminalPrintString(checkingApicStr, strlen(checkingApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!isApicPresent() || apic == NULL || apic == INVALID_ADDRESS) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(presentStr, strlen(presentStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(presentStr, strlen(presentStr));
	terminalPrintChar('\n');

	// Parse the APIC table
	terminalPrintSpaces4();
	terminalPrintString(parsingApicStr, strlen(parsingApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');
	uint32_t *localApicAddress = (uint32_t*)((uint64_t)apic + sizeof(ACPISDTHeader));
	uint32_t *apicFlags = localApicAddress + 1;
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(localApicAddrStr, strlen(localApicAddrStr));
	terminalPrintHex(localApicAddress, sizeof(*localApicAddress));
	terminalPrintString(flagsStr, strlen(flagsStr));
	terminalPrintHex(apicFlags, sizeof(*apicFlags));
	terminalPrintString(lengthStr, strlen(lengthStr));
	terminalPrintHex(&apic->length, sizeof(apic->length));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(entriesStr, strlen(entriesStr));
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(entryHeaderStr, strlen(entryHeaderStr));
	uint64_t apicEnd = (uint64_t)apic + apic->length;
	APICEntryHeader *entry = (APICEntryHeader*)((uint64_t)apic + sizeof(ACPISDTHeader) + sizeof(*localApicAddress) + sizeof(*apicFlags));
	while ((uint64_t)entry < apicEnd) {
		terminalPrintSpaces4();
		terminalPrintSpaces4();
		terminalPrintSpaces4();
		terminalPrintDecimal(entry->type);
		terminalPrintSpaces4();
		terminalPrintHex(&entry->length, sizeof(entry->length));
		uint32_t flags;
		if (entry->type == APIC_TYPE_CPU) {
			++cpuCount;
			APICCPUEntry *cpuEntry = (APICCPUEntry*)entry;
			flags = cpuEntry->flags;
			terminalPrintChar(' ');
			terminalPrintHex(&flags, sizeof(flags));
			terminalPrintChar(' ');
			terminalPrintDecimal(cpuEntry->cpuId);
		} else if (entry->type == APIC_TYPE_INTERRUPT_SOURCE_OVERRIDE) {
			APICInterruptSourceOverrideEntry *overrideEntry = (APICInterruptSourceOverrideEntry*)entry;
			flags = overrideEntry->flags;
			terminalPrintChar(' ');
			terminalPrintHex(&flags, sizeof(flags));
		} else if (entry->type == APIC_TYPE_IO) {
			++ioApicCount;
		}
		terminalPrintChar('\n');
		entry = (APICEntryHeader*)((uint64_t)entry + entry->length);
	}
	terminalPrintSpaces4();
	terminalPrintString(apicParsedStr, strlen(apicParsedStr));
	terminalPrintChar('\n');

	// Disable legacy PIC if present
	if (*apicFlags & 1) {
		terminalPrintSpaces4();
		terminalPrintString(disablingPicStr, strlen(disablingPicStr));
		terminalPrintString(ellipsisStr, strlen(ellipsisStr));
		disableLegacyPic();
		terminalPrintString(doneStr, strlen(doneStr));
		terminalPrintChar('\n');
	}

	terminalPrintString(apicInitCompleteStr, strlen(apicInitCompleteStr));
	return true;
}

size_t getCpuCount() {
	return cpuCount;
}
