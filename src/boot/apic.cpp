#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <kernel.h>
#include <terminal.h>

APIC::CPU *APIC::bootCpu = nullptr;
std::vector<APIC::CPU> APIC::cpus;
std::vector<APIC::InterruptSourceOverrideEntry> APIC::interruptOverrideEntries;
std::vector<APIC::IOEntry> APIC::ioEntries;

static void *ioApic = nullptr;

static const char* const initApicStr = "Initializing APIC";
static const char* const apicInitCompleteStr = "APIC initialized\n\n";
static const char* const checkingApicStr = "Checking APIC presence";
static const char* const disablingPicStr = "Disabling legacy PIC";
static const char* const parsingApicStr = "Parsing APIC table";
static const char* const apicParsedStr = "APIC table parsed\n\n";
static const char* const localApicAddrStr = "Local APIC address ";
static const char* const flagsStr = "Flags ";
static const char* const lengthStr = " Length ";
static const char* const entriesStr = "APIC entries\n";
static const char* const entryHeaderStr = "Type Flags        APICID IOapicAddr   IRQ Global\n";
static const char* const mappingApicStr = "Mapping local APIC to kernel address space";
static const char* const mappingIoApicStr = "Mapping IOAPIC to kernel address space";
static const char* const enablingApicStr = "Enabling APIC";
static const char* const bootCpuStr = "Boot CPU [";
static const char* const noIoapicStr = "no IOAPIC";
static const char* const cpusFoundStr = "CPUs found [";
static const char* const require2CpusStr = "Require at least 2 CPUs\n";

bool APIC::parse() {
	using namespace Kernel::Memory;

	terminalPrintString(parsingApicStr, strlen(parsingApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!ACPI::apicSdtHeader || ACPI::apicSdtHeader == INVALID_ADDRESS) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintChar('\n');

	terminalPrintSpaces4();
	terminalPrintString(flagsStr, strlen(flagsStr));
	const uint32_t* const apicFlags = (uint32_t*)((uint64_t)ACPI::apicSdtHeader + sizeof(ACPI::SDTHeader) + 4);
	terminalPrintHex(apicFlags, 4);
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

	terminalPrintSpaces4();
	terminalPrintString(entriesStr, strlen(entriesStr));
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(entryHeaderStr, strlen(entryHeaderStr));
	uint64_t apicEnd = (uint64_t)ACPI::apicSdtHeader + ACPI::apicSdtHeader->length;
	EntryHeader *entry = (EntryHeader*)(apicFlags + 1);
	while ((uint64_t)entry < apicEnd) {
		terminalPrintSpaces4();
		terminalPrintSpaces4();
		terminalPrintDecimal(entry->type);
		terminalPrintSpaces4();
		uint32_t flags;
		size_t x, y;
		terminalGetCursorPosition(&x, &y);
		if (entry->type == EntryType::xAPIC) {
			xAPICEntry *xApicEntry = (xAPICEntry*)entry;
			CPU cpu;
			cpu.apicId = xApicEntry->apicId;
			flags = cpu.flags = xApicEntry->flags;
			cpus.push_back(cpu);
			terminalPrintHex(&flags, sizeof(flags));
			terminalPrintChar(' ');
			terminalPrintDecimal(cpu.apicId);
			terminalSetCursorPosition(33, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(46, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(50, y);
			terminalPrintChar('-');
		} else if (entry->type == EntryType::InterruptSourceOverride) {
			InterruptSourceOverrideEntry *overrideEntry = (InterruptSourceOverrideEntry*)entry;
			interruptOverrideEntries.push_back(*overrideEntry);
			flags = overrideEntry->flags;
			terminalPrintHex(&flags, sizeof(flags));
			terminalPrintChar(' ');
			terminalPrintChar('-');
			terminalSetCursorPosition(33, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(46, y);
			terminalPrintDecimal(overrideEntry->irqSource);
			terminalSetCursorPosition(50, y);
			terminalPrintHex(&overrideEntry->globalSystemInterrupt, sizeof(overrideEntry->globalSystemInterrupt));
		} else if (entry->type == EntryType::IOAPIC) {
			IOEntry *ioEntry = (IOEntry*)entry;
			ioEntries.push_back(*ioEntry);
			terminalPrintChar('-');
			terminalSetCursorPosition(26, y);
			terminalPrintDecimal(ioEntry->apicId);
			terminalSetCursorPosition(33, y);
			terminalPrintHex(&ioEntry->address, sizeof(ioEntry->address));
			terminalSetCursorPosition(46, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(50, y);
			terminalPrintChar('-');
		} else if (entry->type == EntryType::x2APIC) {
			x2APICEntry *x2ApicEntry = (x2APICEntry*)entry;
			CPU cpu;
			cpu.apicId = x2ApicEntry->apicId;
			flags = cpu.flags = x2ApicEntry->flags;
			cpus.push_back(cpu);
			terminalPrintHex(&flags, sizeof(flags));
			terminalPrintChar(' ');
			terminalPrintDecimal(cpu.apicId);
			terminalSetCursorPosition(33, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(46, y);
			terminalPrintChar('-');
			terminalSetCursorPosition(50, y);
			terminalPrintChar('-');
		}
		terminalPrintChar('\n');
		entry = (EntryHeader*)((uint64_t)entry + entry->length);
	}

	// Check at least 2 CPUs are present
	terminalPrintSpaces4();
	terminalPrintString(cpusFoundStr, strlen(cpusFoundStr));
	terminalPrintDecimal(cpus.size());
	terminalPrintString("]\n", 2);
	if (cpus.size() < 2) {
		terminalPrintSpaces4();
		terminalPrintString(require2CpusStr, strlen(require2CpusStr));
		return false;
	}

	// Map the 1st IOAPIC page to kernel address space
	// TODO: deal with multiple IOAPICs
	terminalPrintSpaces4();
	terminalPrintString(mappingIoApicStr, strlen(mappingIoApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (ioEntries.size() == 0) {
		terminalPrintString(noIoapicStr, strlen(noIoapicStr));
		terminalPrintChar('\n');
		return false;
	}
	const auto requestResult = Virtual::requestPages(
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
			(void*)(uint64_t)ioEntries.at(0).address,
			1,
			RequestType::CacheDisable | RequestType::Writable
		)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	ioApic = requestResult.address;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintString(apicParsedStr, strlen(apicParsedStr));
	return true;
}

APIC::IORedirectionEntry APIC::readIoRedirectionEntry(const Kernel::IRQ irq) {
	IORedirectionEntry x;
	x.lowDword = readIo(IOAPIC_READTBL_LOW(irq));
	x.highDword = readIo(IOAPIC_READTBL_HIGH(irq));
	return x;
}

void APIC::writeIoRedirectionEntry(const Kernel::IRQ irq, const IORedirectionEntry entry) {
	writeIo(IOAPIC_READTBL_LOW(irq), entry.lowDword);
	writeIo(IOAPIC_READTBL_HIGH(irq), entry.highDword);
}

uint32_t APIC::readIo(const uint8_t offset) {
	// Put offset in IOREGSEL
	*(volatile uint32_t*)(ioApic) = offset;
	// Read from IOWIN
	return *(volatile uint32_t*)((uint64_t)ioApic + 0x10);
}

void APIC::writeIo(const uint8_t offset, const uint32_t value) {
	// Put offset in IOREGSEL
	*(volatile uint32_t*)(ioApic) = offset;
	// Write to IOWIN
	*(volatile uint32_t*)((uint64_t)ioApic + 0x10) = value;
}

void APIC::acknowledgeLocalInterrupt() {
	Kernel::writeMsr(Kernel::MSR::x2ApicEOI, 0);
}
