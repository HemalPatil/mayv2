#include <apic.h>
#include <elf64.h>
#include <idt64.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>
#include <tss64.h>
#include <virtualmemmgmt.h>

const char* const infoTableStr = "InfoTable located at ";
const char* const kernel64Loaded = "Kernel64 loaded\nRunning in 64-bit long mode\n";
const char* const systemInitializationFailed = "\nSystem initialization failed. Cannot boot. Halting the system\n";
const char* const kernelPanicString = "\nKernel panic!!!\n";
const char* const k64SizeStr = "Kernel virtual memory size ";
const char* const kBaseStr = "Kernel ELF base ";
const char* const usableMem = "Usable memory start ";
const char* const higherHalfStr = "Kernel higher half size ";

uint64_t kernelVirtualMemorySize = 0;
InfoTable *infoTable;

// First C-function to be called
void kernelMain(InfoTable *infoTableAddress, uint64_t kernelElfBase, uint64_t usableMemoryStart) {
	terminalSetBgColour(TERMINAL_COLOUR_WHITE);
	terminalSetTextColour(TERMINAL_COLOUR_BLACK);
	terminalClearScreen();
	terminalSetCursorPosition(0, 0);
	terminalPrintString(kernel64Loaded, strlen(kernel64Loaded));

	infoTable = infoTableAddress;
	terminalPrintString(infoTableStr, strlen(infoTableStr));
	terminalPrintHex(&infoTable, sizeof(infoTable));
	terminalPrintChar('\n');
	terminalPrintString(kBaseStr, strlen(kBaseStr));
	terminalPrintHex(&kernelElfBase, sizeof(kernelElfBase));
	terminalPrintChar('\n');

	// Parse the ELF header and find the size of kernel in physical memory and higher half
	kernelVirtualMemorySize = 0;
	uint64_t higherHalfSize = 0;
	ELF64Header* elfHeader = (ELF64Header*)kernelElfBase;
	ELF64ProgramHeader* programHeader = (ELF64ProgramHeader*)(kernelElfBase + elfHeader->headerTablePosition);
	for (uint16_t i = 0; i < elfHeader->headerEntryCount; ++i) {
		if (programHeader[i].segmentType != ELF_SegmentType_Load) {
			continue;
		}
		size_t pageCount = programHeader[i].segmentSizeInMemory / phyPageSize;
		if (programHeader[i].segmentSizeInMemory - pageCount * phyPageSize) {
			++pageCount;
		}
		kernelVirtualMemorySize += pageCount * phyPageSize;
		if (programHeader[i].virtualMemoryAddress >= K64_HIGHERHALF_ORIGIN) {
			higherHalfSize += pageCount * phyPageSize;
		}
	}

	terminalPrintString(k64SizeStr, strlen(k64SizeStr));
	terminalPrintHex(&kernelVirtualMemorySize, sizeof(kernelVirtualMemorySize));
	terminalPrintChar('\n');
	terminalPrintString(higherHalfStr, strlen(higherHalfStr));
	terminalPrintHex(&higherHalfSize, sizeof(higherHalfSize));
	terminalPrintChar('\n');
	terminalPrintString(usableMem, strlen(usableMem));
	terminalPrintHex(&usableMemoryStart, sizeof(usableMemoryStart));
	terminalPrintChar('\n');

	// Do memory setup
	if (!initializePhysicalMemory(usableMemoryStart)) {
		kernelPanic(systemInitializationFailed);
	}
	listUsedPhysicalPages();
	if (!initializeVirtualMemory(K64_HIGHERHALF_ORIGIN + higherHalfSize)) {
		kernelPanic(systemInitializationFailed);
	}
	listUsedPhysicalPages();
	return;

	// Initialize TSS first because ISTs in IDT require TSS
	setupTss64();
	setupIdt64();
	
	if (!parseAcpi3()) {
		kernelPanic(systemInitializationFailed);
	}

	// Disable PIC and setup APIC
	setupApic();

	// Get basic interrupts and exceptions setup
	// Initializing ACPI will generate page faults while parsing ACPI tables
	// so we must first fill the IDT with offsets to basic exceptions and interrupts
	// PopulateIDT();
	// if (!SetupHardwareInterrupts())
	// {
	// 	KernelPanic(SystemInitializationFailed);
	// }

	// terminalSetCursorPosition(33, 12);
	// terminalPrintString(HelloWorld, strlen(HelloWorld));
}

void kernelPanic() {
	// TODO : improve kernel panic implementation
	terminalPrintString(kernelPanicString, strlen(kernelPanicString));
	hangSystem();
}