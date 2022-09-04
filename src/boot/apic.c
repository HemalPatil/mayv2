#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <heapmemmgmt.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>

static uint32_t apicFlags = 0;
static size_t cpuCount = 0;
static APICCPUEntry *cpuEntries = NULL;
static size_t interruptOverrideCount = 0;
static APICInterruptSourceOverrideEntry *interruptOverrideEntries = NULL;
static size_t ioApicCount = 0;
static APICIOEntry *ioApicEntries = NULL;
static void* localApicPhysicalAddress = NULL;

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
static const char* const entryHeaderStr = "Type Flags        CpuID IOapicID IOapicAddr\n";

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
	localApicPhysicalAddress = (void*)(uint64_t)(*localApicAddress);
	apicFlags = *(localApicAddress + 1);
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(localApicAddrStr, strlen(localApicAddrStr));
	terminalPrintHex(localApicAddress, sizeof(*localApicAddress));
	terminalPrintString(flagsStr, strlen(flagsStr));
	terminalPrintHex(&apicFlags, sizeof(apicFlags));
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
	APICEntryHeader *entry = (APICEntryHeader*)((uint64_t)apic + sizeof(ACPISDTHeader) + sizeof(*localApicAddress) + sizeof(apicFlags));
	APICEntryHeader *reEntry = entry;
	while ((uint64_t)entry < apicEnd) {
		terminalPrintSpaces4();
		terminalPrintSpaces4();
		terminalPrintSpaces4();
		terminalPrintDecimal(entry->type);
		terminalPrintSpaces4();
		uint32_t flags;
		size_t x, y;
		if (entry->type == APIC_TYPE_CPU) {
			++cpuCount;
			APICCPUEntry *cpuEntry = (APICCPUEntry*)entry;
			flags = cpuEntry->flags;
			terminalPrintHex(&flags, sizeof(flags));
			terminalPrintChar(' ');
			terminalPrintDecimal(cpuEntry->cpuId);
			terminalGetCursorPosition(&x, &y);
			terminalSetCursorPosition(36, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(45, y);
			terminalPrintChar('-');
		} else if (entry->type == APIC_TYPE_INTERRUPT_SOURCE_OVERRIDE) {
			++interruptOverrideCount;
			APICInterruptSourceOverrideEntry *overrideEntry = (APICInterruptSourceOverrideEntry*)entry;
			flags = overrideEntry->flags;
			terminalPrintHex(&flags, sizeof(flags));
			terminalPrintChar(' ');
			terminalPrintChar('-');
			terminalGetCursorPosition(&x, &y);
			terminalSetCursorPosition(36, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(45, y);
			terminalPrintChar('-');
		} else if (entry->type == APIC_TYPE_IO) {
			++ioApicCount;
			APICIOEntry *ioEntry = (APICIOEntry*)entry;
			terminalPrintChar('-');
			terminalSetCursorPosition(30, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(36, y);
			terminalPrintDecimal(ioEntry->apicId);
			terminalSetCursorPosition(45, y);
			terminalPrintHex(&ioEntry->address, sizeof(ioEntry->address));
		}
		terminalPrintChar('\n');
		entry = (APICEntryHeader*)((uint64_t)entry + entry->length);
	}
	// Go through the table once again and copy the entries to their own arrays
	entry = reEntry;
	cpuEntries = kernelMalloc(cpuCount * sizeof(APICCPUEntry));
	ioApicEntries = kernelMalloc(ioApicCount * sizeof(APICIOEntry));
	interruptOverrideEntries = kernelMalloc(interruptOverrideCount * sizeof(APICInterruptSourceOverrideEntry));
	size_t cI = 0, ioI = 0, inI = 0;
	while ((uint64_t)entry < apicEnd) {
		if (entry->type == APIC_TYPE_CPU) {
			memcpy(&cpuEntries[cI], entry, sizeof(APICCPUEntry));
			++cI;
		} else if (entry->type == APIC_TYPE_INTERRUPT_SOURCE_OVERRIDE) {
			memcpy(&interruptOverrideEntries[inI], entry, sizeof(APICInterruptSourceOverrideEntry));
			++inI;
		} else if (entry->type == APIC_TYPE_IO) {
			memcpy(&ioApicEntries[ioI], entry, sizeof(APICIOEntry));
			++ioI;
		}
		entry = (APICEntryHeader*)((uint64_t)entry + entry->length);
	}
	terminalPrintSpaces4();
	terminalPrintString(apicParsedStr, strlen(apicParsedStr));
	terminalPrintChar('\n');

	// Disable legacy PIC if present
	if (apicFlags & 1) {
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
