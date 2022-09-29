#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <heapmemmgmt.h>
#include <idt64.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

uint8_t bootCpu = 0xff;

static uint32_t apicFlags = 0;
static size_t cpuCount = 0;
static APICCPUEntry *cpuEntries = NULL;
static size_t interruptOverrideCount = 0;
static APICInterruptSourceOverrideEntry *interruptOverrideEntries = NULL;
static size_t ioApicCount = 0;
static APICIOEntry *ioApicEntries = NULL;
static void *localApicPhysicalAddress = NULL;
static LocalAPIC *localApic = NULL;
static void *ioApic = NULL;

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
static const char* const entryHeaderStr = "Type Flags        CpuID APICID IOapicAddr   IRQ Global\n";
static const char* const mappingApicStr = "Mapping local APIC to kernel address space";
static const char* const mappingIoApicStr = "Mapping IOAPIC to kernel address space";
static const char* const enablingApicStr = "Enabling APIC";
static const char* const bootCpuStr = "Boot CpuID [";

bool initializeApic() {
	terminalPrintString(initApicStr, strlen(initApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Check APIC presence
	terminalPrintSpaces4();
	terminalPrintString(checkingApicStr, strlen(checkingApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!isApicPresent() || apicSdtHeader == NULL || apicSdtHeader == INVALID_ADDRESS) {
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
	uint32_t *localApicAddress = (uint32_t*)((uint64_t)apicSdtHeader + sizeof(ACPISDTHeader));
	localApicPhysicalAddress = (void*)(uint64_t)(*localApicAddress);
	apicFlags = *(localApicAddress + 1);
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(localApicAddrStr, strlen(localApicAddrStr));
	terminalPrintHex(localApicAddress, sizeof(*localApicAddress));
	terminalPrintString(flagsStr, strlen(flagsStr));
	terminalPrintHex(&apicFlags, sizeof(apicFlags));
	terminalPrintString(lengthStr, strlen(lengthStr));
	terminalPrintHex(&apicSdtHeader->length, sizeof(apicSdtHeader->length));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(entriesStr, strlen(entriesStr));
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(entryHeaderStr, strlen(entryHeaderStr));
	uint64_t apicEnd = (uint64_t)apicSdtHeader + apicSdtHeader->length;
	APICEntryHeader *entry = (APICEntryHeader*)((uint64_t)apicSdtHeader + sizeof(ACPISDTHeader) + sizeof(*localApicAddress) + sizeof(apicFlags));
	APICEntryHeader *reEntry = entry;
	while ((uint64_t)entry < apicEnd) {
		terminalPrintSpaces4();
		terminalPrintSpaces4();
		terminalPrintSpaces4();
		terminalPrintDecimal(entry->type);
		terminalPrintSpaces4();
		uint32_t flags;
		size_t x, y;
		terminalGetCursorPosition(&x, &y);
		if (entry->type == APIC_TYPE_CPU) {
			++cpuCount;
			APICCPUEntry *cpuEntry = (APICCPUEntry*)entry;
			flags = cpuEntry->flags;
			terminalPrintHex(&flags, sizeof(flags));
			terminalPrintChar(' ');
			terminalPrintDecimal(cpuEntry->cpuId);
			terminalSetCursorPosition(36, y);
			terminalPrintDecimal(cpuEntry->apicId);
			terminalSetCursorPosition(43, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(56, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(60, y);
			terminalPrintChar('-');
		} else if (entry->type == APIC_TYPE_INTERRUPT_SOURCE_OVERRIDE) {
			++interruptOverrideCount;
			APICInterruptSourceOverrideEntry *overrideEntry = (APICInterruptSourceOverrideEntry*)entry;
			flags = overrideEntry->flags;
			terminalPrintHex(&flags, sizeof(flags));
			terminalPrintChar(' ');
			terminalPrintChar('-');
			terminalSetCursorPosition(36, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(43, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(56, y);
			terminalPrintDecimal(overrideEntry->irqSource);
			terminalSetCursorPosition(60, y);
			terminalPrintHex(&overrideEntry->globalSystemInterrupt, sizeof(overrideEntry->globalSystemInterrupt));
		} else if (entry->type == APIC_TYPE_IO) {
			++ioApicCount;
			APICIOEntry *ioEntry = (APICIOEntry*)entry;
			terminalPrintChar('-');
			terminalSetCursorPosition(30, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(36, y);
			terminalPrintDecimal(ioEntry->apicId);
			terminalSetCursorPosition(43, y);
			terminalPrintHex(&ioEntry->address, sizeof(ioEntry->address));
			terminalSetCursorPosition(56, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(60, y);
			terminalPrintChar('-');
		}
		terminalPrintChar('\n');
		entry = (APICEntryHeader*)((uint64_t)entry + entry->length);
	}
	// Go through the table once again and copy the entries to their own arrays
	entry = reEntry;
	cpuEntries = (APICCPUEntry*)kernelMalloc(cpuCount * sizeof(APICCPUEntry));
	ioApicEntries = (APICIOEntry*)kernelMalloc(ioApicCount * sizeof(APICIOEntry));
	interruptOverrideEntries = (APICInterruptSourceOverrideEntry*)kernelMalloc(interruptOverrideCount * sizeof(APICInterruptSourceOverrideEntry));
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

	// Map the local APIC page to kernel address space
	terminalPrintSpaces4();
	terminalPrintString(mappingApicStr, strlen(mappingApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	PageRequestResult requestResult = requestVirtualPages(1, MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 1 ||
		!mapVirtualPages(requestResult.address, localApicPhysicalAddress, 1, MEMORY_REQUEST_CACHE_DISABLE)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	localApic = (LocalAPIC*)requestResult.address;
	bootCpu = (localApic->apicId >> 24) & 0xff;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintSpaces4();
	terminalPrintString(bootCpuStr, strlen(bootCpuStr));
	terminalPrintDecimal(bootCpu);
	terminalPrintChar(']');
	terminalPrintChar('\n');

	// Map the 1st IOAPIC page to kernel address space
	// TODO: deal with multiple IOAPICs
	terminalPrintSpaces4();
	terminalPrintString(mappingIoApicStr, strlen(mappingIoApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	requestResult = requestVirtualPages(1, MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 1 ||
		!mapVirtualPages(requestResult.address, (void*)(uint64_t)ioApicEntries[0].address, 1, MEMORY_REQUEST_CACHE_DISABLE)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	ioApic = requestResult.address;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Enable APIC by setting bit 8 in localApic->spuriousInterruptVector
	terminalPrintSpaces4();
	terminalPrintString(enablingApicStr, strlen(enablingApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	localApic->spuriousInterruptVector |= 0x100;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintString(apicInitCompleteStr, strlen(apicInitCompleteStr));
	return true;
}

IOAPICRedirectionEntry readIoRedirectionEntry(const uint8_t irq) {
	IOAPICRedirectionEntry x;
	x.lowDword = readIoApic(IOAPIC_READTBL_LOW(irq));
	x.highDword = readIoApic(IOAPIC_READTBL_HIGH(irq));
	return x;
}

void writeIoRedirectionEntry(const uint8_t irq, const IOAPICRedirectionEntry entry) {
	writeIoApic(IOAPIC_READTBL_LOW(irq), entry.lowDword);
	writeIoApic(IOAPIC_READTBL_HIGH(irq), entry.highDword);
}

uint32_t readIoApic(const uint8_t offset) {
	// Put offset in IOREGSEL
	*(volatile uint32_t*)(ioApic) = offset;
	// Read from IOWIN
	return *(volatile uint32_t*)((uint64_t)ioApic + 0x10);
}

void writeIoApic(const uint8_t offset, const uint32_t value) {
	// Put offset in IOREGSEL
	*(volatile uint32_t*)(ioApic) = offset;
	// Write to IOWIN
	*(volatile uint32_t*)((uint64_t)ioApic + 0x10) = value;
}

size_t getCpuCount() {
	return cpuCount;
}

LocalAPIC* getLocalApic() {
	return localApic;
}

void acknowledgeLocalApicInterrupt() {
	localApic->endOfInterrupt = 0;
}
