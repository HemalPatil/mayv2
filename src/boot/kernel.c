#include <string.h>
#include "kernel.h"

const char* const infoTableStr = "InfoTable located at ";
const char* const kernel64Loaded = "Kernel64 loaded\nRunning in 64-bit long mode\n";
const char* const systemInitializationFailed = "\nSystem initialization failed. Cannot boot. Halting the system\n";
const char* const kernelPanicString = "\nKernel panic!!!\n";
const char* const k64SizeStr = "Kernel virtual memory size ";
const char* const usableMem = "Usable memory start ";

uint64_t kernelVirtualMemorySize = 0;
InfoTable *infoTable;

// First C-function to be called
void kernelMain(InfoTable *infoTableAddress, uint64_t k64Size, uint64_t usableMemoryStart) {
	terminalClearScreen();
	terminalSetCursorPosition(0, 0);
	terminalPrintString(kernel64Loaded, strlen(kernel64Loaded));

	infoTable = infoTableAddress;
	terminalPrintString(infoTableStr, strlen(infoTableStr));
	terminalPrintHex(&infoTable, sizeof(infoTable));
	terminalPrintChar('\n');
	kernelVirtualMemorySize = k64Size;
	terminalPrintString(k64SizeStr, strlen(k64SizeStr));
	terminalPrintHex(&kernelVirtualMemorySize, sizeof(kernelVirtualMemorySize));
	terminalPrintChar('\n');
	terminalPrintString(usableMem, strlen(usableMem));
	terminalPrintHex(&usableMemoryStart, sizeof(usableMemoryStart));
	terminalPrintChar('\n');

	// Do memory setup
	if (!initializePhysicalMemory(usableMemoryStart)) {
		kernelPanic(systemInitializationFailed);
	}
	listUsedPhysicalPages();
	return;
	if (!initializeVirtualMemory()) {
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