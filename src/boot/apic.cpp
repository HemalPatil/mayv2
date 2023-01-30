#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <idt64.h>
#include <kernel.h>
#include <terminal.h>

uint8_t APIC::bootCpuId = 0xff;
std::vector<APIC::CPUEntry> APIC::cpuEntries;
std::vector<APIC::InterruptSourceOverrideEntry> APIC::interruptOverrideEntries;
std::vector<APIC::IOEntry> APIC::ioEntries;

static uint32_t apicFlags = 0;
static void *localApicPhysicalAddress = nullptr;
static APIC::LocalAPIC *localApic = nullptr;
static void *ioApic = nullptr;

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
static const char* const bootCpuStr = "Boot CPU [";

bool APIC::initialize() {
	using namespace Kernel::Memory;

	terminalPrintString(initApicStr, strlen(initApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Check APIC presence
	terminalPrintSpaces4();
	terminalPrintString(checkingApicStr, strlen(checkingApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!isApicPresent() || ACPI::apicSdtHeader == nullptr || ACPI::apicSdtHeader == INVALID_ADDRESS) {
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
	uint32_t *localApicAddress = (uint32_t*)((uint64_t)ACPI::apicSdtHeader + sizeof(ACPI::SDTHeader));
	localApicPhysicalAddress = (void*)(uint64_t)(*localApicAddress);
	apicFlags = *(localApicAddress + 1);
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(localApicAddrStr, strlen(localApicAddrStr));
	terminalPrintHex(localApicAddress, sizeof(*localApicAddress));
	terminalPrintString(flagsStr, strlen(flagsStr));
	terminalPrintHex(&apicFlags, sizeof(apicFlags));
	terminalPrintString(lengthStr, strlen(lengthStr));
	terminalPrintHex(&ACPI::apicSdtHeader->length, sizeof(ACPI::apicSdtHeader->length));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(entriesStr, strlen(entriesStr));
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(entryHeaderStr, strlen(entryHeaderStr));
	uint64_t apicEnd = (uint64_t)ACPI::apicSdtHeader + ACPI::apicSdtHeader->length;
	EntryHeader *entry = (EntryHeader*)((uint64_t)ACPI::apicSdtHeader + sizeof(ACPI::SDTHeader) + sizeof(*localApicAddress) + sizeof(apicFlags));
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
			CPUEntry *cpuEntry = (CPUEntry*)entry;
			cpuEntries.push_back(*cpuEntry);
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
			InterruptSourceOverrideEntry *overrideEntry = (InterruptSourceOverrideEntry*)entry;
			interruptOverrideEntries.push_back(*overrideEntry);
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
			IOEntry *ioEntry = (IOEntry*)entry;
			ioEntries.push_back(*ioEntry);
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
		entry = (EntryHeader*)((uint64_t)entry + entry->length);
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
		!Virtual::mapPages(requestResult.address, localApicPhysicalAddress, 1, RequestType::CacheDisable)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	localApic = (LocalAPIC*)requestResult.address;
	bootCpuId = (localApic->apicId >> 24) & 0xff;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintSpaces4();
	terminalPrintString(bootCpuStr, strlen(bootCpuStr));
	terminalPrintDecimal(bootCpuId);
	terminalPrintChar(']');
	terminalPrintChar('\n');

	// Map the 1st IOAPIC page to kernel address space
	// TODO: deal with multiple IOAPICs
	terminalPrintSpaces4();
	terminalPrintString(mappingIoApicStr, strlen(mappingIoApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	requestResult = Virtual::requestPages(
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
			RequestType::CacheDisable
		)
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

APIC::LocalAPIC* APIC::getLocal() {
	return localApic;
}

void APIC::acknowledgeLocalInterrupt() {
	localApic->endOfInterrupt = 0;
}
